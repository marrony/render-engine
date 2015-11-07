//
// Created by Marrony Neris on 11/7/15.
//

#ifndef COMMANDS_H
#define COMMANDS_H

enum CommandType {
    DRAW_TRIANGLES = 0,
    BIND_VERTEX_ARRAY,
    BIND_PROGRAM,
    BIND_TEXTURE,
    BIND_SAMPLER,
    BIND_CONSTANT4,
    BIND_MATRIX3,
    COMMAND_MAX
};

struct StCommand {
    uint8_t id;
};

union StStateCommand {
    StCommand command;
    uint8_t id;
};

union StDrawCommand {
    StCommand command;
    uint8_t id;
};

struct State {
    int commandCount;
    StStateCommand* commands[];

    static State* create(Allocator& allocator, int commandCount) {
        size_t nbytes = sizeof(State) + commandCount*sizeof(StStateCommand*);

        State* state = (State*)allocator.allocate(nbytes);
        state->commandCount = commandCount;

        return state;
    }
};

template<typename T>
T* createCommand(Allocator& allocator) {
    T* command = (T*)allocator.allocate(sizeof(T));
    command->command.id = T::TYPE;
    return command;
}

struct BindVertexArray {
    StStateCommand command;
    VertexArray vertexArray;

    static const uint8_t TYPE = BIND_VERTEX_ARRAY;

    static StStateCommand* create(Allocator& allocator, VertexArray vertexArray) {
        BindVertexArray* bindVertexArray = createCommand<BindVertexArray>(allocator);
        bindVertexArray->vertexArray = vertexArray;
        return &bindVertexArray->command;
    }

    static void submit(Device& device, BindVertexArray* cmd) {
        device.bindVertexArray(cmd->vertexArray);
    }
};

struct BindProgram {
    StStateCommand command;
    Program program;

    static const uint8_t TYPE = BIND_PROGRAM;

    static StStateCommand* create(Allocator& allocator, Program program) {
        BindProgram* bindProgram = createCommand<BindProgram>(allocator);
        bindProgram->program = program;
        return &bindProgram->command;
    }

    static void submit(Device& device, BindProgram* cmd) {
        device.bindProgram(cmd->program);
    }
};

struct BindConstant4 {
    StStateCommand command;
    Program program;
    const char* name;
    float* value;

    static const uint8_t TYPE = BIND_CONSTANT4;

    static StStateCommand* create(Allocator& allocator, Program program, const char* name, float* value) {
        BindConstant4* bindConstant = createCommand<BindConstant4>(allocator);
        bindConstant->program = program;
        bindConstant->name = name;
        bindConstant->value = value;
        return &bindConstant->command;
    }

    static void submit(Device& device, BindConstant4* cmd) {
        device.setValue(cmd->program, cmd->name, cmd->value);
    }
};

struct BindMatrix3 {
    StStateCommand command;
    Program program;
    const char* name;
    float* value;

    static const uint8_t TYPE = BIND_MATRIX3;

    static StStateCommand* create(Allocator& allocator, Program program, const char* name, float* value) {
        BindMatrix3* bindMatrix = createCommand<BindMatrix3>(allocator);
        bindMatrix->program = program;
        bindMatrix->name = name;
        bindMatrix->value = value;
        return &bindMatrix->command;
    }

    static void submit(Device& device, BindMatrix3* cmd) {
        device.setMatrix(cmd->program, cmd->name, *cmd->value);
    }
};

struct BindSampler {
    StStateCommand command;
    Program program;
    Sampler sampler;
    int unit;

    static const uint8_t TYPE = BIND_SAMPLER;

    static StStateCommand* create(Allocator& allocator, Program program, Sampler sampler, int unit) {
        BindSampler* bindSampler = createCommand<BindSampler>(allocator);
        bindSampler->program = program;
        bindSampler->sampler = sampler;
        bindSampler->unit = unit;
        return &bindSampler->command;
    }

    static void submit(Device& device, BindSampler* cmd) {
        device.bindSampler(cmd->sampler, cmd->unit);
    }
};

struct BindTexture {
    StStateCommand command;
    Program program;
    const char* name;
    Texture2D texture;
    int unit;

    static const uint8_t TYPE = BIND_TEXTURE;

    static StStateCommand* create(Allocator& allocator, Program program, const char* name, Texture2D texture, int unit) {
        BindTexture* bindTexture = createCommand<BindTexture>(allocator);
        bindTexture->program = program;
        bindTexture->name = name;
        bindTexture->texture = texture;
        bindTexture->unit = unit;
        return &bindTexture->command;
    }

    static void submit(Device& device, BindTexture* cmd) {
        device.bindTexture(cmd->program, cmd->texture, cmd->name, cmd->unit);
    }
};

struct DrawTriangles {
    StDrawCommand command;
    int offset;
    int count;

    static const uint8_t TYPE = DRAW_TRIANGLES;

    static StDrawCommand* create(Allocator& allocator, int offset, int count) {
        DrawTriangles* drawTriangles = createCommand<DrawTriangles>(allocator);
        drawTriangles->offset = offset;
        drawTriangles->count = count;
        return &drawTriangles->command;
    }

    static void submit(Device& device, DrawTriangles* cmd) {
        device.drawTriangles(cmd->offset, cmd->count);
    }
};

typedef void (*FnSubmitCommand)(Device& device, StCommand* command);

FnSubmitCommand submitCommand[] = {
        [DRAW_TRIANGLES] = FnSubmitCommand(DrawTriangles::submit),
        [BIND_VERTEX_ARRAY] = FnSubmitCommand(BindVertexArray::submit),
        [BIND_PROGRAM] = FnSubmitCommand(BindProgram::submit),
        [BIND_TEXTURE] = FnSubmitCommand(BindTexture::submit),
        [BIND_SAMPLER] = FnSubmitCommand(BindSampler::submit),
        [BIND_CONSTANT4] = FnSubmitCommand(BindConstant4::submit),
        [BIND_MATRIX3] = FnSubmitCommand(BindMatrix3::submit),
};

int sizeCommand[] = {
        [DRAW_TRIANGLES] = sizeof(DrawTriangles),
        [BIND_VERTEX_ARRAY] = sizeof(BindVertexArray),
        [BIND_PROGRAM] = sizeof(BindProgram),
        [BIND_TEXTURE] = sizeof(BindTexture),
        [BIND_SAMPLER] = sizeof(BindSampler),
        [BIND_CONSTANT4] = sizeof(BindConstant4),
        [BIND_MATRIX3] = sizeof(BindMatrix3),
};

#endif //COMMANDS_H