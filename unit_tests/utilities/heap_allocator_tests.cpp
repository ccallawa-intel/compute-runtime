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

#include "test.h"
#include "gtest/gtest.h"
#include "runtime/utilities/heap_allocator.h"

#include <random>
#include <iostream>

using namespace OCLRT;
using namespace std;

const size_t sizeThreshold = 16 * 4096;

class HeapAllocatorUnderTest : public HeapAllocator {
  public:
    HeapAllocatorUnderTest(void *address, uint64_t size, size_t threshold) : HeapAllocator(address, size, threshold) {}
    HeapAllocatorUnderTest(void *address, uint64_t size) : HeapAllocator(address, size) {}

    uint64_t getLeftBound() { return this->pLeftBound; }
    uint64_t getRightBound() { return this->pRightBound; }
    uint64_t getavailableSize() { return this->availableSize; }
    size_t getThresholdSize() { return this->sizeThreshold; }
    void defragment() { return HeapAllocator::defragment(); }

    void *getFromFreedChunks(size_t size, std::vector<HeapChunk> &vec) {
        size_t sizeOfFreedChunk;
        return HeapAllocator::getFromFreedChunks(size, vec, sizeOfFreedChunk);
    }
    void storeInFreedChunks(void *ptr, size_t size, std::vector<HeapChunk> &vec) { return HeapAllocator::storeInFreedChunks(ptr, size, vec); }

    std::vector<HeapChunk> &getFreedChunksSmall() { return this->freedChunksSmall; };
    std::vector<HeapChunk> &getFreedChunksBig() { return this->freedChunksBig; };

    void overrideAlignement(size_t newAlignement) { allocationAlignment = newAlignement; }
    size_t peekAlignement() { return allocationAlignment; }
};

TEST(HeapAllocatorTest, DefaultCtorHasThresholdSet) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size);
    EXPECT_NE(0u, heapAllocator->getThresholdSize());
    EXPECT_EQ(MemoryConstants::pageSize, heapAllocator->peekAlignement());
    delete heapAllocator;
}

//this test is no longer valid as pt
TEST(HeapAllocatorTest, DISABLED_FreeNotAllocatedPointer) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size);
    heapAllocator->free(reinterpret_cast<void *>(0x123000), size);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    delete heapAllocator;
}

TEST(HeapAllocatorTest, StatisticsMethods) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size);
    EXPECT_EQ(heapAllocator->getavailableSize(), heapAllocator->getLeftSize());
    EXPECT_EQ(0u, heapAllocator->getUsedSize());
    EXPECT_EQ((double)0.0, heapAllocator->getUsage());

    size_t ptrSize = 4096;
    void *ptr = heapAllocator->allocate(ptrSize);
    EXPECT_EQ(4096u, heapAllocator->getUsedSize());
    EXPECT_LT((double)0.0, heapAllocator->getUsage());

    heapAllocator->free(ptr, ptrSize);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenExactSizeChunkInFreedChunksWhenGetIsCalledThenChunkIsReturned) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;
    void *ptrFreed = reinterpret_cast<void *>(0x101000);
    size_t sizeFreed = MemoryConstants::pageSize * 2;
    freedChunks.emplace_back(ptrFreed, sizeFreed);

    void *ptrReturned = heapAllocator->getFromFreedChunks(sizeFreed, freedChunks);

    EXPECT_EQ(ptrFreed, ptrReturned);  // ptr returned is the one that was stored
    EXPECT_EQ(0u, freedChunks.size()); // entry in freed container is removed

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenOnlySmallerSizeChunksInFreedChunksWhenGetIsCalledThenNullptrIsReturned) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;

    freedChunks.emplace_back(reinterpret_cast<void *>(0x100000), 4096);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x101000), 4096);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x105000), 4096);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x104000), 4096);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x102000), 8192);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x109000), 8192);
    freedChunks.emplace_back(reinterpret_cast<void *>(0x107000), 4096);

    EXPECT_EQ(7u, freedChunks.size());

    void *ptrReturned = heapAllocator->getFromFreedChunks(4 * 4096, freedChunks);

    EXPECT_EQ(nullptr, ptrReturned);
    EXPECT_EQ(7u, freedChunks.size());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenOnlyBiggerSizeChunksInFreedChunksWhenGetIsCalledThenBestFitChunkIsReturned) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    uintptr_t pUpperBound = reinterpret_cast<uintptr_t>(ptrBase) + size;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;
    void *ptrExpected = nullptr;

    pUpperBound -= 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pUpperBound), 4096);
    pUpperBound -= 5 * 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pUpperBound), 5 * 4096);
    pUpperBound -= 4 * 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pUpperBound), 4 * 4096);
    ptrExpected = reinterpret_cast<void *>(pUpperBound);

    pUpperBound -= 5 * 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pUpperBound), 5 * 4096);
    pUpperBound -= 4 * 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pUpperBound), 4 * 4096);

    EXPECT_EQ(5u, freedChunks.size());

    void *ptrReturned = heapAllocator->getFromFreedChunks(3 * 4096, freedChunks);

    EXPECT_EQ(ptrExpected, ptrReturned);
    EXPECT_EQ(4u, freedChunks.size());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenOnlyMoreThanTwiceBiggerSizeChunksInFreedChunksWhenGetIsCalledThenSplittedChunkIsReturned) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    uintptr_t pLowerBound = reinterpret_cast<uintptr_t>(ptrBase);

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;
    void *ptrExpected = nullptr;
    size_t requestedSize = 3 * 4096;

    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 4096);
    pLowerBound += 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 9 * 4096);
    pLowerBound += 9 * 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 7 * 4096);

    size_t deltaSize = 7 * 4096 - requestedSize;
    ptrExpected = reinterpret_cast<void *>(pLowerBound + deltaSize);

    EXPECT_EQ(3u, freedChunks.size());

    void *ptrReturned = heapAllocator->getFromFreedChunks(requestedSize, freedChunks);

    EXPECT_EQ(ptrExpected, ptrReturned);
    EXPECT_EQ(3u, freedChunks.size());

    EXPECT_EQ(reinterpret_cast<void *>(pLowerBound), freedChunks[2].ptr);
    EXPECT_EQ(deltaSize, freedChunks[2].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenStoredChunkAdjacentToLeftBoundaryOfIncomingChunkWhenStoreIsCalledThenChunkIsMerged) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    uintptr_t pLowerBound = reinterpret_cast<uintptr_t>(ptrBase);

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;
    void *ptrExpected = nullptr;
    size_t expectedSize = 9 * 4096;

    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 4096);
    pLowerBound += 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 9 * 4096);
    ptrExpected = reinterpret_cast<void *>(pLowerBound);
    pLowerBound += 9 * 4096;

    EXPECT_EQ(ptrExpected, freedChunks[1].ptr);
    EXPECT_EQ(expectedSize, freedChunks[1].size);

    EXPECT_EQ(2u, freedChunks.size());

    void *ptrToStore = reinterpret_cast<void *>(pLowerBound);
    size_t sizeToStore = 2 * 4096;

    expectedSize += sizeToStore;

    heapAllocator->storeInFreedChunks(ptrToStore, sizeToStore, freedChunks);

    EXPECT_EQ(2u, freedChunks.size());

    EXPECT_EQ(ptrExpected, freedChunks[1].ptr);
    EXPECT_EQ(expectedSize, freedChunks[1].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenStoredChunkAdjacentToRightBoundaryOfIncomingChunkWhenStoreIsCalledThenChunkIsMerged) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    uintptr_t pLowerBound = reinterpret_cast<uintptr_t>(ptrBase);

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;
    void *ptrExpected = nullptr;
    size_t expectedSize = 9 * 4096;

    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 4096);
    pLowerBound += 4096;
    pLowerBound += 4096; // space between stored chunk and chunk to store

    void *ptrToStore = reinterpret_cast<void *>(pLowerBound);
    size_t sizeToStore = 2 * 4096;
    pLowerBound += sizeToStore;

    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 9 * 4096);
    ptrExpected = reinterpret_cast<void *>(pLowerBound);

    EXPECT_EQ(ptrExpected, freedChunks[1].ptr);
    EXPECT_EQ(expectedSize, freedChunks[1].size);

    EXPECT_EQ(2u, freedChunks.size());

    expectedSize += sizeToStore;
    ptrExpected = ptrToStore;

    heapAllocator->storeInFreedChunks(ptrToStore, sizeToStore, freedChunks);

    EXPECT_EQ(2u, freedChunks.size());

    EXPECT_EQ(ptrExpected, freedChunks[1].ptr);
    EXPECT_EQ(expectedSize, freedChunks[1].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, GivenStoredChunkNotAdjacentToIncomingChunkWhenStoreIsCalledThenNewFreeChunkIsCreated) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    uintptr_t pLowerBound = reinterpret_cast<uintptr_t>(ptrBase);

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    std::vector<HeapChunk> freedChunks;

    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 4096);
    pLowerBound += 4096;
    freedChunks.emplace_back(reinterpret_cast<void *>(pLowerBound), 9 * 4096);
    pLowerBound += 9 * 4096;

    pLowerBound += 9 * 4096;

    void *ptrToStore = reinterpret_cast<void *>(pLowerBound);
    size_t sizeToStore = 4096;

    EXPECT_EQ(2u, freedChunks.size());

    heapAllocator->storeInFreedChunks(ptrToStore, sizeToStore, freedChunks);

    EXPECT_EQ(3u, freedChunks.size());

    EXPECT_EQ(ptrToStore, freedChunks[2].ptr);
    EXPECT_EQ(sizeToStore, freedChunks[2].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateReturnsPointerAndAddsEntryToMap) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize = 4096;
    void *ptr = heapAllocator->allocate(ptrSize);

    EXPECT_NE(nullptr, ptr);
    EXPECT_LE(ptrBase, ptr);

    size_t ptrSize2 = sizeThreshold + 4096;
    ptr = heapAllocator->allocate(ptrSize2);

    EXPECT_NE(nullptr, ptr);
    EXPECT_LE(ptrBase, ptr);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, FreeReclaimsSpaceAndRemovesEntriesFromMap) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024u * 4096u;
    uintptr_t pLeftBound = reinterpret_cast<uintptr_t>(ptrBase);
    uintptr_t pRightBound = pLeftBound + size;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize = 4096;
    void *ptr = heapAllocator->allocate(ptrSize);

    EXPECT_NE(nullptr, ptr);
    EXPECT_LE(ptrBase, ptr);

    size_t ptrSize2 = sizeThreshold + 4096;
    void *ptr2 = heapAllocator->allocate(ptrSize2);

    EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound + sizeThreshold + 4096u);
    EXPECT_EQ(heapAllocator->getRightBound(), pRightBound - 4096u);

    EXPECT_EQ(heapAllocator->getavailableSize(), size - (sizeThreshold + 4096u) - 4096u);

    heapAllocator->free(ptr, ptrSize);
    heapAllocator->free(ptr2, ptrSize2);

    EXPECT_EQ(heapAllocator->getavailableSize(), size);

    EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound);
    EXPECT_EQ(heapAllocator->getRightBound(), pRightBound);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateMultiple) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;

    size_t allocSize = 4096;
    size_t doubleAllocSize = 4096 * 2;

    for (uint32_t i = 0u; i < 2u; i++) {
        HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);
        doubleAllocSize = allocSize * 2;
        uintptr_t pLeftBound = reinterpret_cast<uintptr_t>(ptrBase);
        uintptr_t pRightBound = pLeftBound + size;

        void *ptr1 = heapAllocator->allocate(allocSize);
        EXPECT_NE(nullptr, ptr1);
        EXPECT_LE(ptrBase, ptr1);

        void *ptr2 = heapAllocator->allocate(allocSize);
        EXPECT_NE(nullptr, ptr2);

        void *ptr3 = heapAllocator->allocate(doubleAllocSize);
        EXPECT_NE(nullptr, ptr3);

        void *ptr4 = heapAllocator->allocate(allocSize);
        EXPECT_NE(nullptr, ptr4);

        EXPECT_NE(ptr1, ptr2);
        EXPECT_NE(ptr1, ptr3);
        EXPECT_NE(ptr1, ptr4);
        EXPECT_NE(ptr2, ptr3);
        EXPECT_NE(ptr2, ptr4);
        EXPECT_NE(ptr3, ptr4);

        size_t totalAllocationSize = 3 * allocSize + 2 * allocSize;
        EXPECT_LE(heapAllocator->getavailableSize(), size - totalAllocationSize);

        if (i == 0u) {
            EXPECT_EQ(heapAllocator->getRightBound(), pRightBound - totalAllocationSize);
        } else if (i == 1u) {
            EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound + totalAllocationSize);
        }

        delete heapAllocator;
        allocSize += sizeThreshold;
    }
}

TEST(HeapAllocatorTest, AllocateWholeSpace) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize = 4096;
    void *ptr1 = heapAllocator->allocate(ptrSize);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    size_t ptrSize2 = 1023 * 4096;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_NE(nullptr, ptr2);

    EXPECT_EQ(heapAllocator->getLeftBound(), heapAllocator->getRightBound());
    EXPECT_EQ(0u, heapAllocator->getavailableSize());

    size_t ptrSize3 = 8192;
    void *ptr3 = heapAllocator->allocate(ptrSize3);
    EXPECT_EQ(nullptr, ptr3);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, FreeInReverseOrder) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    uintptr_t pLeftBound = reinterpret_cast<uintptr_t>(ptrBase);
    uintptr_t pRightBound = pLeftBound + size;

    size_t ptr1Size = 4096;
    void *ptr1 = heapAllocator->allocate(ptr1Size);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    size_t ptrSize2 = sizeThreshold + 4096;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_NE(nullptr, ptr2);

    size_t ptrSize3 = 8192;
    void *ptr3 = heapAllocator->allocate(ptrSize3);
    EXPECT_NE(nullptr, ptr3);

    heapAllocator->free(ptr3, ptrSize3);
    heapAllocator->free(ptr2, ptrSize2);
    heapAllocator->free(ptr1, ptr1Size);

    EXPECT_EQ(heapAllocator->getavailableSize(), size);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound);
    EXPECT_EQ(heapAllocator->getRightBound(), pRightBound);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, SizeNotAvailable) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 0;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize = 4096;
    void *ptr = heapAllocator->allocate(ptrSize);

    EXPECT_EQ(nullptr, ptr);
    EXPECT_EQ(0u, heapAllocator->getavailableSize());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, SizeAvailableButInsufficient) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 11 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, 3 * 4096);
    size_t remainingSize = size;

    // first small succeeds
    size_t ptrSize = 4096;
    void *ptr = heapAllocator->allocate(ptrSize);
    EXPECT_NE(nullptr, ptr);
    // second small suceeds
    size_t ptrSize1 = 4096;
    void *ptr1 = heapAllocator->allocate(ptrSize1);
    // free first to store on free list
    heapAllocator->free(ptr, ptrSize);
    remainingSize -= 4096;

    EXPECT_NE(nullptr, ptr1);
    EXPECT_EQ(remainingSize, heapAllocator->getavailableSize());

    // first big succeeds
    size_t ptrSize2 = 4 * 4096;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_NE(nullptr, ptr2);
    // second big succeeds
    size_t ptrSize3 = 4 * 4096;
    void *ptr3 = heapAllocator->allocate(ptrSize3);
    EXPECT_NE(nullptr, ptr3);
    // free first big to store on free list
    heapAllocator->free(ptr2, 4 * 4096);
    remainingSize -= 4 * 4096;

    EXPECT_EQ(remainingSize, heapAllocator->getavailableSize());

    // third small fails
    size_t ptrSize4 = 2 * 4096;
    void *ptr4 = heapAllocator->allocate(ptrSize4);
    EXPECT_EQ(nullptr, ptr4);
    // third big fails
    size_t ptrSize5 = 5 * 4096;
    void *ptr5 = heapAllocator->allocate(ptrSize5);
    EXPECT_EQ(nullptr, ptr5);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, FreeNullDoesNothing) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, sizeThreshold, sizeThreshold);

    heapAllocator->free(nullptr, 0);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateAfterFree) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    uintptr_t pLeftBound = reinterpret_cast<uintptr_t>(ptrBase);
    uintptr_t pRightBound = pLeftBound + size;

    size_t ptrSize = 8192;
    void *ptr = heapAllocator->allocate(ptrSize);
    EXPECT_NE(nullptr, ptr);
    EXPECT_LE(ptrBase, ptr);

    size_t ptrSize1 = 8192;
    void *ptr1 = heapAllocator->allocate(ptrSize1);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    size_t ptrSize2 = 8192;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_NE(nullptr, ptr2);

    heapAllocator->free(ptr1, ptrSize1);

    EXPECT_EQ(1u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    size_t ptrSize3 = 8192;
    void *ptr3 = heapAllocator->allocate(ptrSize3);
    EXPECT_NE(nullptr, ptr3);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    heapAllocator->free(ptr2, ptrSize2);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    heapAllocator->free(ptr3, ptrSize3);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    heapAllocator->free(ptr, ptrSize);

    EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound);
    EXPECT_EQ(heapAllocator->getRightBound(), pRightBound);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateFromFreedBiggerChunk) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    uintptr_t pLeftBound = reinterpret_cast<uintptr_t>(ptrBase);
    uintptr_t pRightBound = pLeftBound + size;

    size_t sizeAllocated = 0;

    size_t ptrSize = 8192;
    void *ptr = heapAllocator->allocate(ptrSize);
    EXPECT_NE(nullptr, ptr);
    EXPECT_LE(ptrBase, ptr);

    sizeAllocated += 8192;

    size_t ptrSize1 = 4 * 4096;
    void *ptr1 = heapAllocator->allocate(ptrSize1);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    sizeAllocated += 4 * 4096;

    size_t ptrSize2 = 8192;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_NE(nullptr, ptr2);

    sizeAllocated += 8192;
    EXPECT_EQ(size - sizeAllocated, heapAllocator->getavailableSize());

    heapAllocator->free(ptr1, ptrSize1);

    sizeAllocated -= 4 * 4096;
    EXPECT_EQ(size - sizeAllocated, heapAllocator->getavailableSize());

    EXPECT_EQ(1u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    size_t ptrSize3 = 3 * 4096;
    void *ptr3 = heapAllocator->allocate(ptrSize3);
    EXPECT_NE(nullptr, ptr3);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    sizeAllocated += 4 * 4096; // 4*4096 because this was chunk that was stored on free list
    EXPECT_EQ(size - sizeAllocated, heapAllocator->getavailableSize());

    heapAllocator->free(ptr2, ptrSize2);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    heapAllocator->free(ptr3, ptrSize3);

    EXPECT_EQ(0u, heapAllocator->getFreedChunksSmall().size());
    EXPECT_EQ(0u, heapAllocator->getFreedChunksBig().size());

    heapAllocator->free(ptr, ptrSize);

    EXPECT_EQ(heapAllocator->getLeftBound(), pLeftBound);
    EXPECT_EQ(heapAllocator->getRightBound(), pRightBound);
    EXPECT_EQ(size, heapAllocator->getavailableSize());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateWhenNoSpaceForSmallAllocation) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize1 = size - 4096;
    void *ptr1 = heapAllocator->allocate(ptrSize1);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    EXPECT_EQ(4096u, heapAllocator->getavailableSize());

    size_t ptrSize2 = 8192;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_EQ(nullptr, ptr2);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocateWhenNoSpaceForBigAllocation) {

    void *ptrBase = reinterpret_cast<void *>(0x100000);
    size_t size = 1024 * 4096;
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, sizeThreshold);

    size_t ptrSize1 = 8192;
    void *ptr1 = heapAllocator->allocate(ptrSize1);
    EXPECT_NE(nullptr, ptr1);
    EXPECT_LE(ptrBase, ptr1);

    EXPECT_EQ(size - 8192u, heapAllocator->getavailableSize());

    size_t ptrSize2 = size - 4096;
    void *ptr2 = heapAllocator->allocate(ptrSize2);
    EXPECT_EQ(nullptr, ptr2);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, AllocationsDoNotOverlap) {
    std::ranlux24 generator(1);

    const uint32_t maxIndex = 2000;

    unique_ptr<void *[]> ptrs(new void *[maxIndex]);
    unique_ptr<size_t[]> sizes(new size_t[maxIndex]);

    memset(ptrs.get(), 0, sizeof(void *) * maxIndex);
    memset(sizes.get(), 0, sizeof(size_t) * maxIndex);

    uint16_t *freeIndexes = new uint16_t[maxIndex];
    unique_ptr<uint16_t[]> indexes(new uint16_t[maxIndex]);
    memset(freeIndexes, 0, sizeof(uint16_t) * maxIndex);
    memset(indexes.get(), 0, sizeof(uint16_t) * maxIndex);

    // Generate random unique indexes
    for (uint32_t i = 0; i < maxIndex; i++) {
        uint16_t index = (generator() + 1) % maxIndex;

        if (freeIndexes[index] == 0) {
            indexes[i] = index;
            freeIndexes[index] = 1;
        }
    }

    delete[] freeIndexes;
    uint64_t allocatorSize = 1024llu * 1024llu; // 1 MB
    void *pBasePtr = alignedMalloc(static_cast<size_t>(allocatorSize), 4096);
    constexpr size_t reqAlignment = 4;
    size_t bigAllocationThreshold = (512 + 256) * reqAlignment;

    memset(pBasePtr, 0, static_cast<size_t>(allocatorSize));
    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(pBasePtr, allocatorSize, bigAllocationThreshold);

    heapAllocator->overrideAlignement(reqAlignment);

    for (uint32_t i = 0; i < maxIndex; i++) {
        if (indexes[i] != 0) {
            size_t sizeToAllocate = (indexes[i] % 1024) * reqAlignment;
            ASSERT_LT(sizeToAllocate, allocatorSize);
            sizes[i] = sizeToAllocate;
            ptrs[i] = heapAllocator->allocate(sizes[i]);

            if (ptrs[i] == nullptr)
                break;

            uint8_t *pTemp = reinterpret_cast<uint8_t *>(ptrs[i]);
            for (uint32_t j = 0; j < sizes[i] / 4096; j++) {
                *pTemp += 1;
                pTemp += 4096;
            }

            uint32_t indexToFree = indexes[i] % (i * 2 + 1);
            //if (indexToFree < i) {
            if (ptrs[indexToFree] != nullptr) {
                memset(ptrs[indexToFree], 0, sizes[indexToFree]);
                heapAllocator->free(ptrs[indexToFree], sizes[indexToFree]);
                ptrs[indexToFree] = nullptr;
                sizes[indexToFree] = 0;
            }
            //}
        }
    }

    uint8_t *pTemp = reinterpret_cast<uint8_t *>(pBasePtr);

    for (uint32_t i = 0; i < allocatorSize / reqAlignment; i++) {
        if (*pTemp > 1) {
            EXPECT_TRUE(false) << "Heap from Allocator corrupted at page offset " << i << endl;
        }
    }

    for (uint32_t i = 0; i < maxIndex; i++) {
        if (ptrs[i] != 0) {
            heapAllocator->free(ptrs[i], sizes[i]);
        }
    }

    //at this point we should be able to allocate full size
    size_t totalSize = (size_t)(allocatorSize - reqAlignment);
    auto finalPtr = heapAllocator->allocate(totalSize);
    EXPECT_NE(nullptr, finalPtr);

    heapAllocator->free(finalPtr, totalSize);

    alignedFree(pBasePtr);

    //  delete[] indexes;
    //    delete[] ptrs;
    //  delete[] sizes;

    delete heapAllocator;
}

TEST(HeapAllocatorTest, defragmentBig) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    uintptr_t basePtr = 0x100000;
    size_t size = 1024 * 4096;

    size_t threshold = 4096;
    size_t allocSize = 2 * MemoryConstants::pageSize;
    size_t doubleallocSize = 2 * allocSize;
    size_t tripleallocSize = 3 * allocSize;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, threshold);

    std::vector<HeapChunk> &freedChunks = heapAllocator->getFreedChunksBig();

    // 0, 1, 2 - can be merged to one
    // 6,7,8,10 - can be merged to one

    void *ptrs[12];
    ptrs[0] = heapAllocator->allocate(allocSize);
    ptrs[1] = heapAllocator->allocate(allocSize);
    ptrs[2] = heapAllocator->allocate(allocSize);
    ptrs[3] = heapAllocator->allocate(tripleallocSize);
    ptrs[4] = nullptr;
    ptrs[5] = nullptr;
    ptrs[6] = heapAllocator->allocate(allocSize);
    ptrs[7] = heapAllocator->allocate(allocSize);
    ptrs[8] = heapAllocator->allocate(doubleallocSize);
    ptrs[9] = nullptr;
    ptrs[10] = heapAllocator->allocate(allocSize);
    ptrs[11] = heapAllocator->allocate(allocSize);

    heapAllocator->free(ptrs[0], allocSize);
    heapAllocator->free(ptrs[10], allocSize);
    heapAllocator->free(ptrs[2], allocSize);
    heapAllocator->free(ptrs[6], allocSize);
    heapAllocator->free(ptrs[1], allocSize);
    heapAllocator->free(ptrs[7], allocSize);
    heapAllocator->free(ptrs[8], doubleallocSize);

    // 0,1  merged on free,
    // 2
    // 6,7 - merged on free
    // 8, 10 - merged on free
    EXPECT_EQ(4u, freedChunks.size());

    heapAllocator->defragment();

    ASSERT_EQ(2u, freedChunks.size());

    EXPECT_EQ(reinterpret_cast<void *>(basePtr), freedChunks[0].ptr);
    EXPECT_EQ(3 * allocSize, freedChunks[0].size);

    EXPECT_EQ(reinterpret_cast<void *>(basePtr + 6 * allocSize), freedChunks[1].ptr);
    EXPECT_EQ(5 * allocSize, freedChunks[1].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, defragmentSmall) {
    void *ptrBase = reinterpret_cast<void *>(0x100000);
    uintptr_t basePtr = 0x100000;

    size_t size = 1024 * 4096;
    uintptr_t upperLimitPtr = basePtr + size;

    size_t threshold = 2 * MemoryConstants::pageSize;
    size_t allocSize = MemoryConstants::pageSize;
    size_t doubleallocSize = 2 * allocSize;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, threshold);

    std::vector<HeapChunk> &freedChunks = heapAllocator->getFreedChunksSmall();

    // 0, 1, 2 - can be merged to one
    // 6,7,8,10 - can be merged to one

    void *ptrs[12];
    ptrs[0] = heapAllocator->allocate(allocSize);
    ptrs[1] = heapAllocator->allocate(allocSize);
    ptrs[2] = heapAllocator->allocate(allocSize);
    ptrs[3] = heapAllocator->allocate(doubleallocSize);
    ptrs[4] = nullptr;
    ptrs[5] = nullptr;
    ptrs[6] = heapAllocator->allocate(allocSize);
    ptrs[7] = heapAllocator->allocate(allocSize);
    ptrs[8] = heapAllocator->allocate(doubleallocSize);
    ptrs[9] = nullptr;
    ptrs[10] = heapAllocator->allocate(allocSize);
    ptrs[11] = heapAllocator->allocate(allocSize);

    heapAllocator->free(ptrs[0], allocSize);
    heapAllocator->free(ptrs[2], allocSize);
    heapAllocator->free(ptrs[8], doubleallocSize);
    heapAllocator->free(ptrs[1], allocSize);
    heapAllocator->free(ptrs[6], allocSize);
    heapAllocator->free(ptrs[7], allocSize);
    heapAllocator->free(ptrs[10], allocSize);

    // 0,1  merged on free,
    // 2
    // 6, - merged on free
    // 7, 8, 10 - merged on free
    EXPECT_EQ(4u, freedChunks.size());

    heapAllocator->defragment();

    ASSERT_EQ(2u, freedChunks.size());

    EXPECT_EQ(reinterpret_cast<void *>(upperLimitPtr - 3 * allocSize), freedChunks[0].ptr);
    EXPECT_EQ(3 * allocSize, freedChunks[0].size);

    EXPECT_EQ(reinterpret_cast<void *>(upperLimitPtr - 10 * allocSize), freedChunks[1].ptr);
    EXPECT_EQ(5 * allocSize, freedChunks[1].size);

    delete heapAllocator;
}

TEST(HeapAllocatorTest, Given10SmallAllocationsWhenFreedInTheSameOrderThenLastChunkFreedReturnsWholeSpaceToFreeRange) {
    void *ptrBase = reinterpret_cast<void *>(0);
    size_t size = 1024 * 4096;
    size_t threshold = 2 * 4096;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, threshold);

    std::vector<HeapChunk> &freedChunks = heapAllocator->getFreedChunksSmall();

    void *ptrs[10];
    size_t sizes[10];

    for (uint32_t i = 0; i < 10; i++) {
        sizes[i] = 4096;
        ptrs[i] = heapAllocator->allocate(sizes[i]);
    }

    EXPECT_EQ(0u, freedChunks.size());

    for (uint32_t i = 0; i < 10; i++) {
        heapAllocator->free(ptrs[i], sizes[i]);
        // After free chunk gets merged to existing one on freed list
        if (i < 9) {
            EXPECT_EQ(1u, freedChunks.size());
        }
    }

    // Last chunk released merges freed chunk to free range
    EXPECT_EQ(0u, freedChunks.size());
    delete heapAllocator;
}

TEST(HeapAllocatorTest, Given10SmallAllocationsWhenMergedToBigAllocatedAsSmallSplittedAndReleasedThenItDoesNotGoToFreedBigChunksList) {
    void *ptrBase = reinterpret_cast<void *>(0);
    uintptr_t basePtr = 0;

    // Size for 10 small allocs plus one single 2 page plus some space
    size_t size = (10 + 2 + 1) * 4096;
    uintptr_t upperLimitPtr = basePtr + size;

    size_t threshold = 4 * 4096;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, threshold);

    std::vector<HeapChunk> &freedChunksSmall = heapAllocator->getFreedChunksSmall();
    std::vector<HeapChunk> &freedChunksBig = heapAllocator->getFreedChunksBig();

    void *ptrs[10];
    size_t sizes[10];

    // size smaller than threshold
    size_t sizeOfSmallAlloc = 2 * 4096;
    void *smallAlloc = nullptr;

    for (uint32_t i = 0; i < 10; i++) {
        sizes[i] = 4096;
        ptrs[i] = heapAllocator->allocate(sizes[i]);
    }

    EXPECT_EQ(0u, freedChunksSmall.size());
    EXPECT_EQ(0u, freedChunksBig.size());

    // Release 8 chunks
    for (uint32_t i = 0; i < 8; i++) {
        heapAllocator->free(ptrs[i], sizes[i]);
    }

    // Allocate small chunk, should be taken from freed list
    smallAlloc = heapAllocator->allocate(sizeOfSmallAlloc);

    EXPECT_NE(nullptr, smallAlloc);
    EXPECT_LE(upperLimitPtr - (8 * 4096), reinterpret_cast<uintptr_t>(smallAlloc));

    EXPECT_EQ(1u, freedChunksSmall.size());

    heapAllocator->free(smallAlloc, sizeOfSmallAlloc);

    // It should not go to freedBig list
    EXPECT_EQ(0u, freedChunksBig.size());

    // It should merge to freedSmall chunk
    EXPECT_EQ(1u, freedChunksSmall.size());

    // Release last 2 allocs
    for (uint32_t i = 8; i < 10; i++) {
        heapAllocator->free(ptrs[i], sizes[i]);
    }

    // In the end both lists should be empty
    EXPECT_EQ(0u, freedChunksSmall.size());
    EXPECT_EQ(0u, freedChunksBig.size());

    delete heapAllocator;
}

TEST(HeapAllocatorTest, Given10SmallAllocationsWhenMergedToBigAllocatedAsSmallNotSplittedAndReleasedThenItDoesNotGoToFreedBigChunksList) {
    void *ptrBase = reinterpret_cast<void *>(0);
    uintptr_t basePtr = 0;

    // Size for 10 small allocs plus one single 3 page plus some space
    size_t size = (10 + 3 + 1) * 4096;
    uintptr_t upperLimitPtr = basePtr + size;

    size_t threshold = 4 * 4096;

    HeapAllocatorUnderTest *heapAllocator = new HeapAllocatorUnderTest(ptrBase, size, threshold);

    std::vector<HeapChunk> &freedChunksSmall = heapAllocator->getFreedChunksSmall();
    std::vector<HeapChunk> &freedChunksBig = heapAllocator->getFreedChunksBig();

    void *ptrs[10];
    size_t sizes[10];

    // size smaller than threshold
    size_t sizeOfSmallAlloc = 3 * 4096;
    void *smallAlloc = nullptr;

    for (uint32_t i = 0; i < 10; i++) {
        sizes[i] = 4096;
        ptrs[i] = heapAllocator->allocate(sizes[i]);
    }

    EXPECT_EQ(0u, freedChunksSmall.size());
    EXPECT_EQ(0u, freedChunksBig.size());

    // Release 8 chunks
    for (uint32_t i = 0; i < 5; i++) {
        heapAllocator->free(ptrs[i], sizes[i]);
    }

    // Allocate small chunk, should be taken from freed list
    smallAlloc = heapAllocator->allocate(sizeOfSmallAlloc);

    EXPECT_NE(nullptr, smallAlloc);
    EXPECT_LE(upperLimitPtr - (5 * 4096), reinterpret_cast<uintptr_t>(smallAlloc));

    EXPECT_EQ(0u, freedChunksSmall.size());

    heapAllocator->free(smallAlloc, sizeOfSmallAlloc);

    // It should not go to freedBig list
    EXPECT_EQ(0u, freedChunksBig.size());

    // It should go to freedSmall chunk
    EXPECT_EQ(1u, freedChunksSmall.size());

    // Release remaining allocs
    for (uint32_t i = 5; i < 10; i++) {
        heapAllocator->free(ptrs[i], sizes[i]);
        if (i < 9) {
            // chunks should be merged to freedSmall chunk on list
            EXPECT_EQ(1u, freedChunksSmall.size());
        }
    }

    // In the end both lists should be empty
    EXPECT_EQ(0u, freedChunksSmall.size());
    EXPECT_EQ(0u, freedChunksBig.size());

    delete heapAllocator;
}