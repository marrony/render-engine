//
// Created by Marrony Neris on 11/10/15.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

class LinearAllocator {
public:
    LinearAllocator(int8_t* begin, size_t size) : begin(begin), current(begin), end(begin + size) { }

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
    HeapAllocator() : freeList(nullptr) { }

    ~HeapAllocator() {
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
            FreeList* header = (FreeList*) malloc(size + sizeof(FreeList));
            header->size = size;
            return header->data;
        }

        if(current == freeList)
            freeList = freeList->next;
        else
            previous->next = current->next;

        return current->data;
    }

    void* reallocate(void* ptr, size_t newSize) {
        FreeList* header = (FreeList*) ptr - 1;
        size_t oldSize = header->size;

        if (oldSize >= newSize)
            return ptr;

        deallocate(ptr);
        void* newPtr = allocate(newSize);
        memcpy(newPtr, ptr, oldSize);
        return newPtr;
    }

    void deallocate(void* ptr) {
        FreeList* newFreeList = (FreeList*) ptr - 1;
        newFreeList->next = freeList;
        freeList = newFreeList;
    }
private:
    struct FreeList {
        size_t size;
        FreeList* next;
        char data[];
    };

    FreeList* freeList;
};

template<int SIZE>
class PoolAllocator {
public:
private:
};

class BuddyAllocator {
};

#endif //ALLOCATOR_H
