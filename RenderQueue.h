//
// Created by Marrony Neris on 11/7/15.
//

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <algorithm>
#include <functional>

struct RenderItem {
    uint64_t key;
    int commandBufferCount;
    CommandBuffer* commandBuffer[COMMAND_MAX];
};

bool operator<(const RenderItem& i0, const RenderItem& i1) {
    return i0.key < i1.key;
}

struct RenderGroup {
    CommandBuffer* commandBuffer;
    int itemsCount;
    RenderItem* items;
};

class RenderQueue {
public:
    RenderQueue(Device& device, HeapAllocator& allocator)
            : device(device), allocator(allocator), itemsCount(0) {
        items = (RenderItem*) allocator.allocate(sizeof(RenderItem) * 1024);
    }

    ~RenderQueue() {
        allocator.deallocate(items);
    }

    void submit(uint64_t key, CommandBuffer** commandBuffer, int commandBufferCount) {
        items[itemsCount].key = key;
        for (int i = 0; i < commandBufferCount; i++)
            items[itemsCount].commandBuffer[i] = commandBuffer[i];
        items[itemsCount].commandBufferCount = commandBufferCount;
        itemsCount++;
    }

    CommandBuffer* sendToCommandBuffer() {
        CommandBuffer* commandBuffer = CommandBuffer::create(allocator, 10);

        std::function<void(Command*)> exec = [&](Command* src) {
            if(commandBuffer->commandCount >= commandBuffer->maxCommands) {
                int maxCommands = commandBuffer->commandCount * 3 / 2;
                commandBuffer = CommandBuffer::realloc(allocator, commandBuffer, maxCommands);
            }

            Command* dst = CommandBuffer::getCommandAt(commandBuffer, commandBuffer->commandCount++);

            memcpy(dst, src, COMMAND_MAX_SIZE);
        };

        submit(exec);

        return commandBuffer;
    }

    void sendToDevice() {
        std::function<void(Command*)> exec = [this](Command* cmd) {
            invoke(cmd);
        };

        submit(exec);
    }

    int getSkippedCommands() {
        return skippedCommands;
    }

    int getExecutedCommands() {
        return executedCommands;
    }
private:
    void submit(std::function<void(Command*)> execute) {
        Command* previousCmd[COMMAND_MAX];

        memset(previousCmd, 0, sizeof(previousCmd));

        executedCommands = 0;
        skippedCommands = 0;

        std::sort(items, items+itemsCount);

        for (int i = 0; i < itemsCount; i++) {
            RenderItem& item = items[i];

            for (int j = 0; j < item.commandBufferCount; j++) {
                CommandBuffer* commandBuffer = item.commandBuffer[j];

                for (int k = 0; k < commandBuffer->commandCount; k++) {
                    Command* cmd = CommandBuffer::getCommandAt(commandBuffer, k);

                    extern const int sizeCommand[];

                    uint32_t id = cmd->id;

                    int size = sizeCommand[id];

                    if (isDrawCommand(id) || !previousCmd[id] || memcmp(previousCmd[id], cmd, size) != 0) {
                        execute(cmd);
                        previousCmd[id] = cmd;
                    } else {
                        skippedCommands++;
                    }
                }
            }
        }

        itemsCount = 0;
    }

    bool isDrawCommand(uint32_t id) {
        return id <= DRAW_COMMANDS_MAX;
    }

    void invoke(Command* cmd) {
        executedCommands++;
        Command::invoke(cmd, device);
    }

    Device& device;
    HeapAllocator& allocator;

    int itemsCount;
    RenderItem* items;
    int executedCommands;
    int skippedCommands;
};

#endif //RENDERQUEUE_H
