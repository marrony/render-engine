//
// Created by Marrony Neris on 11/7/15.
//

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <algorithm>

struct RenderItem {
    uint64_t key;
    DrawCommand* cmd;
    int statesCount;
    State* states[COMMAND_MAX];
};

bool operator<(const RenderItem& i0, const RenderItem& i1) {
    return i0.key < i1.key;
}

struct RenderGroup {
    State* state;
    int itemsCount;
    RenderItem* items;
};

class RenderQueue {
public:
    RenderQueue(Device& device) : device(device), itemsCount(0) {
        items = new RenderItem[1024];
    }

    void submit(uint64_t key, DrawCommand* draw, State** states, int statesCount) {
        items[itemsCount].key = key;
        items[itemsCount].cmd = draw;
        for (int i = 0; i < statesCount; i++)
            items[itemsCount].states[i] = states[i];
        items[itemsCount].statesCount = statesCount;
        itemsCount++;
    }

    void submit() {
        bool statesSet[COMMAND_MAX];
        bool nonDefaultState[COMMAND_MAX];
        StateCommand* previousState[COMMAND_MAX];

        memset(previousState, 0, sizeof(previousState));
        memset(nonDefaultState, 0xff, sizeof(nonDefaultState));

        executedCommands = 0;
        skippedCommands = 0;

        std::sort(items, items+itemsCount);

        for (int i = 0; i < itemsCount; i++) {
            memset(statesSet, 0, sizeof(statesSet));

            RenderItem& item = items[i];

            for (int j = 0; j < item.statesCount; j++) {
                State* state = item.states[j];

                for (int k = 0; k < state->commandCount; k++) {
                    StateCommand* stateCmd = state->commands[k];

                    extern const int sizeCommand[];

                    uint32_t id = stateCmd->id;

                    int size = sizeCommand[id];

                    if (!statesSet[id] && (!previousState[id] || memcmp(previousState[id], stateCmd, size) != 0)) {
                        execute(&stateCmd->command);
                        statesSet[id] = true;
                        previousState[id] = stateCmd;
                    } else {
                        skippedCommands++;
                    }
                }
            }

            execute(&item.cmd->command);
        }

        itemsCount = 0;
    }

    int getSkippedCommands() {
        return skippedCommands;
    }

    int getExecutedCommands() {
        return executedCommands;
    }

private:
    void execute(Command* cmd) {
        extern const FnSubmitCommand submitCommand[];

        executedCommands++;
        submitCommand[cmd->id](device, cmd);
    }

    Device& device;
    int itemsCount;
    RenderItem* items;
    int executedCommands;
    int skippedCommands;

    StateCommand* defaults[COMMAND_MAX];
};

#endif //RENDERQUEUE_H
