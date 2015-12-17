//
// Created by Marrony Neris on 12/17/15.
//

#include "RenderQueue.h"

bool operator<(const RenderItem& i0, const RenderItem& i1) {
    return i0.key < i1.key;
}

RenderQueue::RenderQueue(Device& device, HeapAllocator& allocator)
        : device(device), allocator(allocator), itemsCount(0) {
    items = (RenderItem*) allocator.allocate(sizeof(RenderItem) * 1024);
}

RenderQueue::~RenderQueue() {
    allocator.deallocate(items);
}

void RenderQueue::submit(uint64_t key, CommandBuffer** commandBuffer, int commandBufferCount) {
    items[itemsCount].key = key;
    for (int i = 0; i < commandBufferCount; i++)
        items[itemsCount].commandBuffer[i] = commandBuffer[i];
    items[itemsCount].commandBufferCount = commandBufferCount;
    itemsCount++;
}

void RenderQueue::sort() {
    std::sort(items, items+itemsCount);
}

CommandBuffer* RenderQueue::sendToCommandBuffer() {
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

void RenderQueue::sendToDevice() {
    std::function<void(Command*)> exec = [this](Command* cmd) {
        invoke(cmd);
    };

    submit(exec);
}

int RenderQueue::getSkippedCommands() {
    return skippedCommands;
}

int RenderQueue::getExecutedCommands() {
    return executedCommands;
}

void RenderQueue::submit(std::function<void(Command*)> execute) {
    Command* previousCmd[COMMAND_MAX];

    memset(previousCmd, 0, sizeof(previousCmd));

    executedCommands = 0;
    skippedCommands = 0;

    for (int i = 0; i < itemsCount; i++) {
        RenderItem& item = items[i];

        for (int j = 0; j < item.commandBufferCount; j++) {
            CommandBuffer* commandBuffer = item.commandBuffer[j];

            for (int k = 0; k < commandBuffer->commandCount; k++) {
                Command* cmd = CommandBuffer::getCommandAt(commandBuffer, k);

                CommandType id = (CommandType)cmd->id;

                int size = sizeCommand[id];

                if (isDirectCommand(id) || !previousCmd[id] || memcmp(previousCmd[id], cmd, size) != 0) {
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

bool RenderQueue::isDirectCommand(uint32_t id) {
    return id <= DIRECT_COMMANDS_MAX;
}

void RenderQueue::invoke(Command* cmd) {
    executedCommands++;
    Command::invoke(cmd, device);
}
