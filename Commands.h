//
// Created by Marrony Neris on 11/7/15.
//

#ifndef COMMANDS_H
#define COMMANDS_H

enum CommandType {
    DRAW_TRIANGLES = 0,
    DRAW_TRIANGLES_INSTANCED,
    SET_VIEWPORT,
    CLEAR_COLOR,
    COPY_CONSTANT_BUFFER,
    BIND_VERTEX_ARRAY,
    BIND_PROGRAM,
    BIND_TEXTURE,
    BIND_SAMPLER,
    COMMAND_MAX
};

struct Command {
    uint8_t id;
};

union StateCommand {
    Command command;
    uint8_t id;
};

union StDrawCommand {
    Command command;
    uint8_t id;
};

struct State {
    int commandCount;
    StateCommand* commands[];

    static State* create(Allocator& allocator, int commandCount) {
        size_t nbytes = sizeof(State) + commandCount * sizeof(StateCommand*);

        State* state = (State*) allocator.allocate(nbytes);
        state->commandCount = commandCount;

        return state;
    }
};

template<typename T>
T* createCommand(Allocator& allocator) {
    T* command = (T*) allocator.allocate(sizeof(T));
    command->command.id = T::TYPE;
    return command;
}

struct ClearColor {
    StateCommand command;
    float r, g, b, a;

    static const uint8_t TYPE = CLEAR_COLOR;

    static StateCommand* create(Allocator& allocator, float r, float g, float b, float a) {
        ClearColor* clearColor = createCommand<ClearColor>(allocator);
        clearColor->r = r;
        clearColor->g = g;
        clearColor->b = b;
        clearColor->a = a;
        return &clearColor->command;
    }

    static void submit(Device& device, ClearColor* cmd) {
        glClearColor(cmd->r, cmd->g, cmd->b, cmd->a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
};

struct SetViewport {
    StateCommand command;
    float x;
    float y;
    float width;
    float height;

    static const uint8_t TYPE = SET_VIEWPORT;

    static StateCommand* create(Allocator& allocator, float x, float y, float width, float height) {
        SetViewport* setViewport = createCommand<SetViewport>(allocator);
        setViewport->x = x;
        setViewport->y = y;
        setViewport->width = width;
        setViewport->height = height;
        return &setViewport->command;
    }

    static void submit(Device& device, SetViewport* cmd) {
        GLFWwindow* window = glfwGetCurrentContext();

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);

        int x = w * cmd->x;
        int y = h * cmd->y;
        int width = (w * cmd->width) - x;
        int height = (h * cmd->height) - y;

        glEnable(GL_SCISSOR_TEST);
        glViewport(x, y, width, height);
        glScissor(x, y, width, height);
    }
};

struct CopyConstantBuffer {
    StateCommand command;
    ConstantBuffer constantBuffer;
    const void* data;
    size_t size;

    static const uint8_t TYPE = COPY_CONSTANT_BUFFER;

    static StateCommand* create(Allocator& allocator, ConstantBuffer constantBuffer, const void* data, size_t size) {
        CopyConstantBuffer* copyConstantBuffer = createCommand<CopyConstantBuffer>(allocator);
        copyConstantBuffer->constantBuffer = constantBuffer;
        copyConstantBuffer->data = data;
        copyConstantBuffer->size = size;
        return &copyConstantBuffer->command;
    }

    static void submit(Device& device, CopyConstantBuffer* cmd) {
        device.copyConstantBuffer(cmd->constantBuffer, cmd->data, cmd->size);
    }
};

struct BindVertexArray {
    StateCommand command;
    VertexArray vertexArray;

    static const uint8_t TYPE = BIND_VERTEX_ARRAY;

    static StateCommand* create(Allocator& allocator, VertexArray vertexArray) {
        BindVertexArray* bindVertexArray = createCommand<BindVertexArray>(allocator);
        bindVertexArray->vertexArray = vertexArray;
        return &bindVertexArray->command;
    }

    static void submit(Device& device, BindVertexArray* cmd) {
        device.bindVertexArray(cmd->vertexArray);
    }
};

struct BindProgram {
    StateCommand command;
    Program program;

    static const uint8_t TYPE = BIND_PROGRAM;

    static StateCommand* create(Allocator& allocator, Program program) {
        BindProgram* bindProgram = createCommand<BindProgram>(allocator);
        bindProgram->program = program;
        return &bindProgram->command;
    }

    static void submit(Device& device, BindProgram* cmd) {
        device.bindProgram(cmd->program);
    }
};

struct BindSampler {
    StateCommand command;
    Program program;
    Sampler sampler;
    int unit;

    static const uint8_t TYPE = BIND_SAMPLER;

    static StateCommand* create(Allocator& allocator, Program program, Sampler sampler, int unit) {
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
    StateCommand command;
    Program program;
    const char* name;
    Texture2D texture;
    int unit;

    static const uint8_t TYPE = BIND_TEXTURE;

    static StateCommand* create(Allocator& allocator, Program program, const char* name, Texture2D texture,
                                  int unit) {
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

struct DrawTrianglesInstanced {
    StDrawCommand command;
    int offset;
    int count;
    int instances;

    static const uint8_t TYPE = DRAW_TRIANGLES_INSTANCED;

    static StDrawCommand* create(Allocator& allocator, int offset, int count, int instances) {
        DrawTrianglesInstanced* drawTrianglesInstanced = createCommand<DrawTrianglesInstanced>(allocator);
        drawTrianglesInstanced->offset = offset;
        drawTrianglesInstanced->count = count;
        drawTrianglesInstanced->instances = instances;
        return &drawTrianglesInstanced->command;
    }

    static void submit(Device& device, DrawTrianglesInstanced* cmd) {
        device.drawTrianglesInstanced(cmd->offset, cmd->count, cmd->instances);
    }
};

typedef void (* FnSubmitCommand)(Device& device, Command* command);

FnSubmitCommand submitCommand[] = {
        [DRAW_TRIANGLES] = FnSubmitCommand(DrawTriangles::submit),
        [DRAW_TRIANGLES_INSTANCED] = FnSubmitCommand(DrawTrianglesInstanced::submit),
        [CLEAR_COLOR] = FnSubmitCommand(ClearColor::submit),
        [SET_VIEWPORT] = FnSubmitCommand(SetViewport::submit),
        [COPY_CONSTANT_BUFFER] = FnSubmitCommand(CopyConstantBuffer::submit),
        [BIND_VERTEX_ARRAY] = FnSubmitCommand(BindVertexArray::submit),
        [BIND_PROGRAM] = FnSubmitCommand(BindProgram::submit),
        [BIND_TEXTURE] = FnSubmitCommand(BindTexture::submit),
        [BIND_SAMPLER] = FnSubmitCommand(BindSampler::submit),
};

int sizeCommand[] = {
        [DRAW_TRIANGLES] = sizeof(DrawTriangles),
        [DRAW_TRIANGLES_INSTANCED] = sizeof(DrawTrianglesInstanced),
        [CLEAR_COLOR] = sizeof(ClearColor),
        [SET_VIEWPORT] = sizeof(SetViewport),
        [COPY_CONSTANT_BUFFER] = sizeof(CopyConstantBuffer),
        [BIND_VERTEX_ARRAY] = sizeof(BindVertexArray),
        [BIND_PROGRAM] = sizeof(BindProgram),
        [BIND_TEXTURE] = sizeof(BindTexture),
        [BIND_SAMPLER] = sizeof(BindSampler),
};

#endif //COMMANDS_H