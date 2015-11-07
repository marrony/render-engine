//
// Created by Marrony Neris on 11/7/15.
//

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

class RenderQueue {
public:
    RenderQueue(Device& device) : device(device), itemsCount(0) { }

    void submit(StDrawCommand* draw, State** states, int count) {
        items[itemsCount].draw = draw;
        for(int i = 0; i < count; i++)
            items[itemsCount].states[i] = states[i];
        items[itemsCount].count = count;
        itemsCount++;
    }

    void submit() {
        bool nonDefaultState[COMMAND_MAX];
        StStateCommand* previousState[COMMAND_MAX];

        memset(previousState, 0, sizeof(previousState));
        memset(nonDefaultState, 0xff, sizeof(nonDefaultState));

        numberCommands = 0;
        skippedCommands = 0;

        for(int i = 0; i < itemsCount; i++) {
            Item& item = items[i];

            bool statesSet[COMMAND_MAX];
            memset(statesSet, 0, sizeof(statesSet));

            for(int j = 0; j < item.count; j++) {
                State* state = item.states[j];

                for(int k = 0; k < state->commandCount; k++) {
                    StStateCommand* stateCmd = state->commands[k];

                    extern int sizeCommand[];

                    uint8_t id = stateCmd->id;

                    int size = sizeCommand[id];

                    if(!statesSet[id] && (!previousState[id] || memcmp(previousState[id], stateCmd, size) != 0)) {
                        submit(&stateCmd->command);
                        statesSet[id] = true;
                        previousState[id] = stateCmd;
                    } else {
                        skippedCommands++;
                    }
                }
            }

            submit(&item.draw->command);
        }

        itemsCount = 0;
    }

    int getSkippedCommands() {
        return skippedCommands;
    }

    int getNumberCommands() {
        return numberCommands;
    }
private:
    void submit(StCommand* cmd) {
        extern FnSubmitCommand submitCommand[];

        numberCommands++;
        submitCommand[cmd->id](device, cmd);
    }

    struct Item {
        StDrawCommand* draw;
        State* states[8];
        int count;
    };

    Device& device;
    int itemsCount;
    Item items[100];
    int numberCommands;
    int skippedCommands;

    StStateCommand* defaults[COMMAND_MAX];
};

#endif //RENDERQUEUE_H
