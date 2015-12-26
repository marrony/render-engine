//
// Created by Marrony Neris on 11/10/15.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdio.h>
#include <assert.h>
#include <memory.h>

class LinearAllocator {
public:
    LinearAllocator(void* begin, size_t size) : begin((int8_t*)begin), current((int8_t*)begin), end((int8_t*)begin + size) { }

    void* allocate(size_t size) {
        if (current + size > end)
            return nullptr;

        void* data = current;
        current += size;
        return data;
    }

    size_t memoryUsed() {
        return current - begin;
    }

    void reset() {
        current = begin;
    }
private:
    int8_t* begin;
    int8_t* end;
    int8_t* current;
};

class HeapAllocator {
public:
    HeapAllocator() : freeList(nullptr), bytesAllocated(0), numberAllocations(0) { }

    ~HeapAllocator() {
        assert(numberAllocations == 0);
        assert(bytesAllocated == 0);

        clearMemory();
    }

    void clearMemory() {
        while(freeList) {
            void* ptr = freeList;

            freeList = freeList->next;

            free(ptr);
        }
    }

    void* allocate(size_t size) {
        FreeList* current = freeList;
        FreeList* previous = nullptr;

        while(current) {
            if(current->size >= size)
                break;

            previous = current;
            current = current->next;
        }

        if(!current) {
            size = roundSize(size);

            bytesAllocated += size;
            numberAllocations++;

            FreeList* header = (FreeList*) malloc(size + sizeof(FreeList));
            header->size = size;
            return header->data;
        }

        if(current == freeList)
            freeList = freeList->next;
        else
            previous->next = current->next;

        bytesAllocated += current->size;
        numberAllocations++;
        return current->data;
    }

    void* reallocate(void* ptr, size_t newSize) {
        if(ptr != nullptr) {
            FreeList* header = (FreeList*) ptr - 1;
            size_t oldSize = header->size;

            if (oldSize >= newSize)
                return ptr;

            deallocate(ptr);
            void* newPtr = allocate(newSize);
            memcpy(newPtr, ptr, oldSize);
            return newPtr;
        }

        return allocate(newSize);
    }

    void deallocate(void* ptr) {
        assert(ptr != nullptr);

        FreeList* node = (FreeList*) ptr - 1;

        assert(alreadyDeallocated(node) == false);

        node->next = freeList;
        freeList = node;

        numberAllocations--;
        bytesAllocated -= node->size;
    }

    size_t memoryUsed() {
        return bytesAllocated;
    }

    void dumpFreeList() {
        printf("numberAllocations: %ld\n", numberAllocations);
        printf("bytesAllocated: %ld\n", bytesAllocated);

        FreeList* f = freeList;
        while(f) {
            printf("free: %ld\n", f->size);
            f = f->next;
        }
    }
private:
    struct FreeList {
        size_t size;
        FreeList* next;
        char data[];
    };

    size_t roundSize(size_t size) {
        const size_t blockSize = 128;
        size_t blocks = size / blockSize;

        if((size % blockSize) != 0)
            blocks++;

        return blocks * blockSize;
    }

    bool alreadyDeallocated(FreeList* node) {
        FreeList* f = freeList;

        while(f != nullptr) {
            if(f == node)
                return true;

            f = f->next;
        }

        return false;
    }

    FreeList* freeList;
    size_t numberAllocations;
    size_t bytesAllocated;
};

template<int SIZE>
class PoolAllocator {
public:
private:
};

class BuddyAllocator {
};

#endif //ALLOCATOR_H
