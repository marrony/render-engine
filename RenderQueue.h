//
// Created by Marrony Neris on 11/7/15.
//

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <algorithm>
#include <functional>

#include "Allocator.h"
#include "Commands.h"
#include "Device.h"

struct RenderItem {
    uint64_t key;
    int commandBufferCount;
    CommandBuffer* commandBuffer[16];
};

struct RenderGroup {
    CommandBuffer* commandBuffer;
    int itemsCount;
    RenderItem* items;
};

class RenderQueue {
public:
    RenderQueue(Device& device, HeapAllocator& allocator);

    ~RenderQueue();

    void submit(uint64_t key, CommandBuffer** commandBuffer, int commandBufferCount);

    void sort();

    CommandBuffer* sendToCommandBuffer();

    void sendToDevice();

    int getSkippedCommands();

    int getExecutedCommands();
private:
    void submit(std::function<void(Command*)> execute);

    bool isDirectCommand(uint32_t id);

    void invoke(Command* cmd);

    Device& device;
    HeapAllocator& allocator;

    int itemsCount;
    RenderItem* items;
    int executedCommands;
    int skippedCommands;
};

#endif //RENDERQUEUE_H
