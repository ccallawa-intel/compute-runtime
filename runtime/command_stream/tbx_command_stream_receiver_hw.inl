/*
 * Copyright (c) 2017, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hw_cmds.h"
#include "runtime/helpers/aligned_memory.h"
#include "runtime/helpers/debug_helpers.h"
#include "runtime/helpers/ptr_math.h"
#include "runtime/memory_manager/graphics_allocation.h"
#include <cstring>

namespace OCLRT {

template <typename GfxFamily>
TbxCommandStreamReceiverHw<GfxFamily>::TbxCommandStreamReceiverHw(const HardwareInfo &hwInfoIn)
    : BaseClass(hwInfoIn) {
    for (auto &engineInfo : engineInfoTable) {
        engineInfo.pLRCA = nullptr;
        engineInfo.ggttLRCA = 0u;
        engineInfo.pGlobalHWStatusPage = nullptr;
        engineInfo.ggttHWSP = 0u;
        engineInfo.pRCS = nullptr;
        engineInfo.ggttRCS = 0u;
        engineInfo.sizeRCS = 0;
        engineInfo.tailRCS = 0;
    }
}

template <typename GfxFamily>
TbxCommandStreamReceiverHw<GfxFamily>::~TbxCommandStreamReceiverHw() {
    stream.close();

    for (auto &engineInfo : engineInfoTable) {
        alignedFree(engineInfo.pLRCA);
        gttRemap.unmap(engineInfo.pLRCA);
        engineInfo.pLRCA = nullptr;

        alignedFree(engineInfo.pGlobalHWStatusPage);
        gttRemap.unmap(engineInfo.pGlobalHWStatusPage);
        engineInfo.pGlobalHWStatusPage = nullptr;

        alignedFree(engineInfo.pRCS);
        gttRemap.unmap(engineInfo.pRCS);
        engineInfo.pRCS = nullptr;
    }
}

template <typename GfxFamily>
const AubMemDump::LrcaHelper &TbxCommandStreamReceiverHw<GfxFamily>::getCsTraits(EngineType engineOrdinal) {
    return *AUBFamilyMapper<GfxFamily>::csTraits[engineOrdinal];
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::initGlobalMMIO() {
    for (auto &mmioPair : AUBFamilyMapper<GfxFamily>::globalMMIO) {
        stream.writeMMIO(mmioPair.first, mmioPair.second);
    }
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::initEngineMMIO(EngineType engineOrdinal) {
    auto mmioList = AUBFamilyMapper<GfxFamily>::perEngineMMIO[engineOrdinal];

    DEBUG_BREAK_IF(!mmioList);
    for (auto &mmioPair : *mmioList) {
        stream.writeMMIO(mmioPair.first, mmioPair.second);
    }
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::initializeEngine(EngineType engineOrdinal) {
    auto mmioBase = getCsTraits(engineOrdinal).mmioBase;
    auto &engineInfo = engineInfoTable[engineOrdinal];

    initGlobalMMIO();
    initEngineMMIO(engineOrdinal);

    // Global HW Status Page
    {
        const size_t sizeHWSP = 0x1000;
        const size_t alignHWSP = 0x1000;
        engineInfo.pGlobalHWStatusPage = alignedMalloc(sizeHWSP, alignHWSP);
        engineInfo.ggttHWSP = gttRemap.map(engineInfo.pGlobalHWStatusPage, sizeHWSP);
        auto physHWSP = ggtt.map(engineInfo.ggttHWSP, sizeHWSP);

        // Write our GHWSP
        AUB::reserveAddressGGTT(stream, engineInfo.ggttHWSP, sizeHWSP, physHWSP);
        stream.writeMMIO(mmioBase + 0x2080, engineInfo.ggttHWSP);
    }

    // Allocate the LRCA
    auto csTraits = getCsTraits(engineOrdinal);
    const size_t sizeLRCA = csTraits.sizeLRCA;
    const size_t alignLRCA = csTraits.alignLRCA;
    auto pLRCABase = alignedMalloc(sizeLRCA, alignLRCA);
    engineInfo.pLRCA = pLRCABase;

    // Initialize the LRCA to a known state
    csTraits.initialize(pLRCABase);

    // Reserve the RCS ring buffer
    engineInfo.sizeRCS = 0x4 * 0x1000;
    {
        const size_t alignRCS = 0x1000;
        engineInfo.pRCS = alignedMalloc(engineInfo.sizeRCS, alignRCS);
        engineInfo.ggttRCS = gttRemap.map(engineInfo.pRCS, engineInfo.sizeRCS);
        auto physRCS = ggtt.map(engineInfo.ggttRCS, engineInfo.sizeRCS);

        AUB::reserveAddressGGTT(stream, engineInfo.ggttRCS, engineInfo.sizeRCS, physRCS);
    }

    // Initialize the ring MMIO registers
    {
        uint32_t ringHead = 0x000;
        uint32_t ringTail = 0x000;
        auto ringBase = engineInfo.ggttRCS;
        auto ringCtrl = (uint32_t)((engineInfo.sizeRCS - 0x1000) | 1);
        csTraits.setRingHead(pLRCABase, ringHead);
        csTraits.setRingTail(pLRCABase, ringTail);
        csTraits.setRingBase(pLRCABase, ringBase);
        csTraits.setRingCtrl(pLRCABase, ringCtrl);
    }

    // Write our LRCA
    {
        engineInfo.ggttLRCA = gttRemap.map(engineInfo.pLRCA, sizeLRCA);
        auto lrcAddressPhys = ggtt.map(engineInfo.ggttLRCA, sizeLRCA);

        AUB::reserveAddressGGTT(stream, engineInfo.ggttLRCA, sizeLRCA, lrcAddressPhys);
        AUB::addMemoryWrite(
            stream,
            lrcAddressPhys,
            pLRCABase,
            sizeLRCA,
            AubMemDump::AddressSpaceValues::TraceNonlocal,
            csTraits.aubHintLRCA);
    }
}

template <typename GfxFamily>
CommandStreamReceiver *TbxCommandStreamReceiverHw<GfxFamily>::create(const HardwareInfo &hwInfoIn) {
    auto csr = new TbxCommandStreamReceiverHw<GfxFamily>(hwInfoIn);

    // Open our stream
    csr->stream.open(nullptr);

    // Add the file header.
    csr->stream.init(AubMemDump::SteppingValues::A, AUB::Traits::device);

    return csr;
}

template <typename GfxFamily>
FlushStamp TbxCommandStreamReceiverHw<GfxFamily>::flush(BatchBuffer &batchBuffer, EngineType engineOrdinal, ResidencyContainer *allocationsForResidency) {
    uint32_t mmioBase = getCsTraits(engineOrdinal).mmioBase;
    auto &engineInfo = engineInfoTable[engineOrdinal];

    if (!engineInfo.pLRCA) {
        initializeEngine(engineOrdinal);
        DEBUG_BREAK_IF(!engineInfo.pLRCA);
    }

    // Write our batch buffer
    auto pBatchBuffer = ptrOffset(batchBuffer.commandBufferAllocation->getUnderlyingBuffer(), batchBuffer.startOffset);
    auto currentOffset = batchBuffer.usedSize;
    DEBUG_BREAK_IF(currentOffset < batchBuffer.startOffset);
    auto sizeBatchBuffer = currentOffset - batchBuffer.startOffset;
    {
        auto physBatchBuffer = ppgtt.map(reinterpret_cast<uintptr_t>(pBatchBuffer), sizeBatchBuffer);
        AUB::reserveAddressPPGTT(stream, reinterpret_cast<uintptr_t>(pBatchBuffer), sizeBatchBuffer, physBatchBuffer);

        AUB::addMemoryWrite(
            stream,
            physBatchBuffer,
            pBatchBuffer,
            sizeBatchBuffer,
            AubMemDump::AddressSpaceValues::TraceNonlocal,
            AubMemDump::DataTypeHintValues::TraceBatchBufferPrimary);
    }

    // Add a batch buffer start to the RCS
    auto previousTail = engineInfo.tailRCS;
    {
        typedef typename GfxFamily::MI_LOAD_REGISTER_IMM MI_LOAD_REGISTER_IMM;
        typedef typename GfxFamily::MI_BATCH_BUFFER_START MI_BATCH_BUFFER_START;
        typedef typename GfxFamily::MI_NOOP MI_NOOP;

        auto pTail = ptrOffset(engineInfo.pRCS, engineInfo.tailRCS);
        auto ggttTail = ptrOffset(engineInfo.ggttRCS, engineInfo.tailRCS);

        auto sizeNeeded =
            sizeof(MI_BATCH_BUFFER_START) +
            sizeof(MI_NOOP) +
            sizeof(MI_LOAD_REGISTER_IMM);
        if (engineInfo.tailRCS + sizeNeeded >= engineInfo.sizeRCS) {
            // Pad the remaining ring with NOOPs
            auto sizeToWrap = engineInfo.sizeRCS - engineInfo.tailRCS;
            memset(pTail, 0, sizeToWrap);
            // write remaining ring
            auto physDumpStart = ggtt.map(ggttTail, sizeToWrap);
            AUB::addMemoryWrite(
                stream,
                physDumpStart,
                pTail,
                sizeToWrap,
                AubMemDump::AddressSpaceValues::TraceNonlocal,
                AubMemDump::DataTypeHintValues::TraceCommandBuffer);
            previousTail = 0;
            engineInfo.tailRCS = 0;
            pTail = engineInfo.pRCS;
        } else if (engineInfo.tailRCS == 0) {
            // Add a LRI if this is our first submission
            auto lri = MI_LOAD_REGISTER_IMM::sInit();
            lri.setRegisterOffset(mmioBase + 0x2244);
            lri.setDataDword(0x00010000);
            *(MI_LOAD_REGISTER_IMM *)pTail = lri;
            pTail = ((MI_LOAD_REGISTER_IMM *)pTail) + 1;
        }

        // Add our BBS
        auto bbs = MI_BATCH_BUFFER_START::sInit();
        bbs.setBatchBufferStartAddressGraphicsaddress472(AUB::ptrToPPGTT(pBatchBuffer));
        bbs.setAddressSpaceIndicator(MI_BATCH_BUFFER_START::ADDRESS_SPACE_INDICATOR_PPGTT);
        *(MI_BATCH_BUFFER_START *)pTail = bbs;
        pTail = ((MI_BATCH_BUFFER_START *)pTail) + 1;

        // Add a NOOP as our tail needs to be aligned to a QWORD
        *(MI_NOOP *)pTail = MI_NOOP::sInit();
        pTail = ((MI_NOOP *)pTail) + 1;

        // Compute our new ring tail.
        engineInfo.tailRCS = (uint32_t)ptrDiff(pTail, engineInfo.pRCS);

        // Only dump the new commands
        auto ggttDumpStart = ptrOffset(engineInfo.ggttRCS, previousTail);
        auto dumpStart = ptrOffset(engineInfo.pRCS, previousTail);
        auto dumpLength = engineInfo.tailRCS - previousTail;

        // write RCS
        auto physDumpStart = ggtt.map(ggttDumpStart, dumpLength);
        AUB::addMemoryWrite(
            stream,
            physDumpStart,
            dumpStart,
            dumpLength,
            AubMemDump::AddressSpaceValues::TraceNonlocal,
            AubMemDump::DataTypeHintValues::TraceCommandBuffer);

        // update the RCS mmio tail in the LRCA
        auto physLRCA = ggtt.map(engineInfo.ggttLRCA, sizeof(engineInfo.tailRCS));
        AUB::addMemoryWrite(
            stream,
            physLRCA + 0x101c,
            &engineInfo.tailRCS,
            sizeof(engineInfo.tailRCS),
            AubMemDump::AddressSpaceValues::TraceNonlocal);

        DEBUG_BREAK_IF(engineInfo.tailRCS >= engineInfo.sizeRCS);
    }

    // Submit our execlist by submitting to the execlist submit ports
    {
        typename AUB::MiContextDescriptorReg contextDescriptor = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

        contextDescriptor.sData.Valid = true;
        contextDescriptor.sData.ForcePageDirRestore = false;
        contextDescriptor.sData.ForceRestore = false;
        contextDescriptor.sData.Legacy = true;
        contextDescriptor.sData.FaultSupport = 0;
        contextDescriptor.sData.PrivilegeAccessOrPPGTT = true;
        contextDescriptor.sData.ADor64bitSupport = AUB::Traits::addressingBits > 32;

        auto ggttLRCA = engineInfo.ggttLRCA;
        contextDescriptor.sData.LogicalRingCtxAddress = ggttLRCA / 4096;
        contextDescriptor.sData.ContextID = 0;

        submitLRCA(engineOrdinal, contextDescriptor);
    }

    pollForCompletion(engineOrdinal);
    return 0;
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::submitLRCA(EngineType engineOrdinal, const MiContextDescriptorReg &contextDescriptor) {
    auto mmioBase = getCsTraits(engineOrdinal).mmioBase;
    stream.writeMMIO(mmioBase + 0x2230, 0);
    stream.writeMMIO(mmioBase + 0x2230, 0);
    stream.writeMMIO(mmioBase + 0x2230, contextDescriptor.ulData[1]);
    stream.writeMMIO(mmioBase + 0x2230, contextDescriptor.ulData[0]);
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::pollForCompletion(EngineType engineOrdinal) {
    typedef typename AubMemDump::CmdServicesMemTraceRegisterPoll CmdServicesMemTraceRegisterPoll;

    auto mmioBase = getCsTraits(engineOrdinal).mmioBase;
    bool pollNotEqual = false;
    stream.registerPoll(
        mmioBase + 0x2234, //EXECLIST_STATUS
        0x100,
        0x100,
        pollNotEqual,
        CmdServicesMemTraceRegisterPoll::TimeoutActionValues::Abort);
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::makeResident(GraphicsAllocation &gfxAllocation) {
    if (gfxAllocation.residencyTaskCount < (int)this->taskCount) {
        auto cpuAddress = gfxAllocation.getUnderlyingBuffer();
        auto gpuAddress = gfxAllocation.getGpuAddress();
        auto size = gfxAllocation.getUnderlyingBufferSize();

        if (size == 0 || !(((MemoryAllocation *)&gfxAllocation)->allowAubFileWrite))
            return;

        PageWalker walker = [&](uint64_t physAddress, size_t size, size_t offset) {
            static const size_t pageSize = 4096;
            auto vmAddr = (static_cast<uintptr_t>(gpuAddress) + offset) & ~(pageSize - 1);
            auto pAddr = physAddress & ~(pageSize - 1);

            AUB::reserveAddressPPGTT(stream, vmAddr, pageSize, pAddr);

            AUB::addMemoryWrite(stream, physAddress,
                                reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(cpuAddress) + offset),
                                size, AubMemDump::AddressSpaceValues::TraceNonlocal);
        };
        ppgtt.pageWalk(static_cast<uintptr_t>(gpuAddress), size, 0, walker);

        this->getMemoryManager()->pushAllocationForResidency(&gfxAllocation);
    }
    gfxAllocation.residencyTaskCount = (int)this->taskCount;
}

template <typename GfxFamily>
void TbxCommandStreamReceiverHw<GfxFamily>::makeCoherent(void *address, size_t length) {
    if (length) {
        PageWalker walker = [&](uint64_t physAddress, size_t size, size_t offset) {
            DEBUG_BREAK_IF(offset > length);
            stream.readMemory(physAddress, ptrOffset(address, offset), size);
        };

        ppgtt.pageWalk(reinterpret_cast<uintptr_t>(address), length, 0, walker);
    }
}
} // namespace OCLRT