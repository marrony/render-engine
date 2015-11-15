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
    RenderQueue(Device& device) : device(device), itemsCount(0) {
        items = new RenderItem[1024];
    }

    void submit(uint64_t key, CommandBuffer** commandBuffer, int commandBufferCount) {
        items[itemsCount].key = key;
        for (int i = 0; i < commandBufferCount; i++)
            items[itemsCount].commandBuffer[i] = commandBuffer[i];
        items[itemsCount].commandBufferCount = commandBufferCount;
        itemsCount++;
    }

    CommandBuffer* sendToCommandBuffer(HeapAllocator& allocator) {
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
        bool statesSet[COMMAND_MAX];
        bool nonDefaultState[COMMAND_MAX];
        Command* previousState[COMMAND_MAX];

        memset(previousState, 0, sizeof(previousState));
        memset(nonDefaultState, 0xff, sizeof(nonDefaultState));

        executedCommands = 0;
        skippedCommands = 0;

        std::sort(items, items+itemsCount);

        for (int i = 0; i < itemsCount; i++) {
            memset(statesSet, 0, sizeof(statesSet));

            RenderItem& item = items[i];

            for (int j = 0; j < item.commandBufferCount; j++) {
                CommandBuffer* commandBuffer = item.commandBuffer[j];

                for (int k = 0; k < commandBuffer->commandCount; k++) {
                    Command* cmd = CommandBuffer::getCommandAt(commandBuffer, k);

                    extern const int sizeCommand[];

                    uint32_t id = cmd->id;

                    int size = sizeCommand[id];

                    if (!statesSet[id] && (!previousState[id] || memcmp(previousState[id], cmd, size) != 0)) {
                        execute(cmd);
                        statesSet[id] = true;
                        previousState[id] = cmd;
                    } else {
                        skippedCommands++;
                    }
                }
            }
        }

        itemsCount = 0;
    }

    void invoke(Command* cmd) {
        executedCommands++;
        Command::invoke(cmd, device);
    }

    Device& device;
    int itemsCount;
    RenderItem* items;
    int executedCommands;
    int skippedCommands;

    Command* defaults[COMMAND_MAX];
};

#endif //RENDERQUEUE_H
