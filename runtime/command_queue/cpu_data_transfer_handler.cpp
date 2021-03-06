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

#include "runtime/command_queue/command_queue.h"
#include "runtime/device/device.h"
#include "runtime/context/context.h"
#include "runtime/event/event_builder.h"
#include "runtime/helpers/get_info.h"
#include "runtime/mem_obj/buffer.h"
#include "runtime/mem_obj/image.h"

namespace OCLRT {
void *CommandQueue::cpuDataTransferHandler(TransferProperties &transferProperties, EventsRequest &eventsRequest, cl_int &retVal) {

    EventBuilder eventBuilder;
    bool eventCompleted = false;
    ErrorCodeHelper err(&retVal, CL_SUCCESS);

    auto image = castToObject<Image>(transferProperties.memObj);

    if (eventsRequest.outEvent) {
        eventBuilder.create<Event>(this, transferProperties.cmdType, Event::eventNotReady, Event::eventNotReady);
        eventBuilder.getEvent()->setQueueTimeStamp();
        eventBuilder.getEvent()->setCPUProfilingPath(true);
        *eventsRequest.outEvent = eventBuilder.getEvent();
    }

    TakeOwnershipWrapper<Device> deviceOwnership(*device);
    TakeOwnershipWrapper<CommandQueue> queueOwnership(*this);

    auto blockQueue = false;
    auto taskLevel = 0u;
    obtainTaskLevelAndBlockedStatus(taskLevel, eventsRequest.numEventsInWaitList, eventsRequest.eventWaitList, blockQueue, transferProperties.cmdType);

    DBG_LOG(LogTaskCounts, __FUNCTION__, "taskLevel", taskLevel);

    if (eventsRequest.outEvent) {
        eventBuilder.getEvent()->taskLevel = taskLevel;
    }

    if (blockQueue &&
        (transferProperties.cmdType == CL_COMMAND_MAP_BUFFER ||
         transferProperties.cmdType == CL_COMMAND_MAP_IMAGE ||
         transferProperties.cmdType == CL_COMMAND_UNMAP_MEM_OBJECT)) {

        enqueueBlockedMapUnmapOperation(eventsRequest.eventWaitList,
                                        static_cast<size_t>(eventsRequest.numEventsInWaitList),
                                        transferProperties.cmdType == CL_COMMAND_UNMAP_MEM_OBJECT ? UNMAP : MAP,
                                        transferProperties.memObj,
                                        eventBuilder);
    }

    queueOwnership.unlock();
    deviceOwnership.unlock();

    // read/write buffers are always blocking
    if (!blockQueue || transferProperties.blocking) {
        err.set(Event::waitForEvents(eventsRequest.numEventsInWaitList, eventsRequest.eventWaitList));

        if (eventBuilder.getEvent()) {
            eventBuilder.getEvent()->setSubmitTimeStamp();
        }
        //wait for the completness of previous commands
        if (transferProperties.cmdType != CL_COMMAND_UNMAP_MEM_OBJECT) {
            if (!transferProperties.memObj->isMemObjZeroCopy() || transferProperties.blocking) {
                finish(true);
                eventCompleted = true;
            }
        }

        if (eventBuilder.getEvent()) {
            eventBuilder.getEvent()->setStartTimeStamp();
        }

        switch (transferProperties.cmdType) {
        case CL_COMMAND_MAP_BUFFER:
            if (!transferProperties.memObj->isMemObjZeroCopy()) {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL, CL_ENQUEUE_MAP_BUFFER_REQUIRES_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj));
                }
                transferProperties.memObj->transferDataToHostPtr({{transferProperties.memObj->getSize(), 0, 0}}, {{0, 0, 0}});
                eventCompleted = true;
            } else {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_GOOD_INTEL, CL_ENQUEUE_MAP_BUFFER_DOESNT_REQUIRE_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj));
                }
            }
            transferProperties.memObj->incMapCount();
            break;
        case CL_COMMAND_MAP_IMAGE:
            if (!image->isMemObjZeroCopy()) {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL, CL_ENQUEUE_MAP_IMAGE_REQUIRES_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj));
                }
                auto &imgDesc = image->getImageDesc();
                std::array<size_t, 3> copySize = {{getValidParam(imgDesc.image_width),
                                                   getValidParam(imgDesc.image_height),
                                                   getValidParam((std::max(imgDesc.image_depth, imgDesc.image_array_size)))}};
                image->transferDataToHostPtr(copySize, {{0, 0, 0}});
                GetInfoHelper::set(transferProperties.retSlicePitch, image->getHostPtrSlicePitch());
                GetInfoHelper::set(transferProperties.retRowPitch, image->getHostPtrRowPitch());
                eventCompleted = true;
            } else {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_GOOD_INTEL, CL_ENQUEUE_MAP_IMAGE_DOESNT_REQUIRE_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj));
                }
                GetInfoHelper::set(transferProperties.retSlicePitch, image->getImageDesc().image_slice_pitch);
                GetInfoHelper::set(transferProperties.retRowPitch, image->getImageDesc().image_row_pitch);
            }
            image->incMapCount();
            break;
        case CL_COMMAND_UNMAP_MEM_OBJECT:
            if (!transferProperties.memObj->isMemObjZeroCopy()) {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL, CL_ENQUEUE_UNMAP_MEM_OBJ_REQUIRES_COPY_DATA, transferProperties.ptr, static_cast<cl_mem>(transferProperties.memObj));
                }
                std::array<size_t, 3> copySize = {{transferProperties.memObj->getSize(), 0, 0}};
                if (image) {
                    auto imgDesc = image->getImageDesc();
                    copySize = {{getValidParam(imgDesc.image_width),
                                 getValidParam(imgDesc.image_height),
                                 getValidParam((std::max(imgDesc.image_depth, imgDesc.image_array_size)))}};
                }
                transferProperties.memObj->transferDataFromHostPtr(copySize, {{0, 0, 0}});
                eventCompleted = true;
            } else {
                if (context->isProvidingPerformanceHints()) {
                    context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_GOOD_INTEL, CL_ENQUEUE_UNMAP_MEM_OBJ_DOESNT_REQUIRE_COPY_DATA, transferProperties.ptr);
                }
            }
            transferProperties.memObj->decMapCount();
            break;
        case CL_COMMAND_READ_BUFFER:
            if (context->isProvidingPerformanceHints()) {
                context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL, CL_ENQUEUE_READ_BUFFER_REQUIRES_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj), transferProperties.ptr);
            }
            memcpy_s(transferProperties.ptr, *transferProperties.size, ptrOffset(transferProperties.memObj->getCpuAddressForMemoryTransfer(), *transferProperties.offset), *transferProperties.size);
            eventCompleted = true;
            break;
        case CL_COMMAND_WRITE_BUFFER:
            if (context->isProvidingPerformanceHints()) {
                context->providePerformanceHint(CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL, CL_ENQUEUE_WRITE_BUFFER_REQUIRES_COPY_DATA, static_cast<cl_mem>(transferProperties.memObj), transferProperties.ptr);
            }
            memcpy_s(ptrOffset(transferProperties.memObj->getCpuAddressForMemoryTransfer(), *transferProperties.offset), *transferProperties.size, transferProperties.ptr, *transferProperties.size);
            eventCompleted = true;
            break;
        case CL_COMMAND_MARKER:
            break;
        default:
            err.set(CL_INVALID_OPERATION);
        }

        if (eventBuilder.getEvent()) {
            eventBuilder.getEvent()->setEndTimeStamp();
            eventBuilder.getEvent()->updateTaskCount(this->taskCount);
            if (eventCompleted) {
                eventBuilder.getEvent()->setStatus(CL_COMPLETE);
            } else {
                eventBuilder.getEvent()->updateExecutionStatus();
            }
        }
    }

    if (transferProperties.cmdType == CL_COMMAND_MAP_BUFFER) {
        return transferProperties.memObj->setAndReturnMappedPtr(*transferProperties.offset);
    }

    if (transferProperties.cmdType == CL_COMMAND_MAP_IMAGE) {
        size_t mapOffset = image->getSurfaceFormatInfo().ImageElementSizeInBytes * transferProperties.offset[0] +
                           image->getImageDesc().image_row_pitch * transferProperties.offset[1] +
                           image->getImageDesc().image_slice_pitch * transferProperties.offset[2];
        void *ptrToReturn = nullptr;
        if (image->isMemObjZeroCopy()) {
            ptrToReturn = ptrOffset(image->getCpuAddress(), mapOffset);
        } else {
            ptrToReturn = ptrOffset(image->getHostPtr(), mapOffset);
        }
        image->setMappedPtr(ptrToReturn);
        return ptrToReturn;
    }

    return nullptr; // only map returns pointer
}
} // namespace OCLRT
