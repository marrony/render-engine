//
// Created by Marrony Neris on 11/7/15.
//

#ifndef COMMANDS_H
#define COMMANDS_H

#include "assert.h"

enum CommandType {
    DRAW_TRIANGLES = 0,
    DRAW_TRIANGLES_INSTANCED,
    DRAW_COMMANDS_MAX = DRAW_TRIANGLES_INSTANCED,
    BIND_FRAMEBUFFER,
    SET_VIEWPORT,
    SET_DEPTH_TEST,
    CLEAR_COLOR,
    COPY_CONSTANT_BUFFER,
    BIND_VERTEX_ARRAY,
    BIND_PROGRAM,
    BIND_TEXTURE0,
    BIND_TEXTURE1,
    BIND_TEXTURE2,
    BIND_SAMPLER,
    COMMAND_MAX
};

const int COMMAND_MAX_SIZE = 24;

typedef void (* FnSubmitCommand)(Device& device, class Command* command);

struct Command {
    uint32_t id;

    static void invoke(Command* cmd, Device& device) {
        extern const FnSubmitCommand submitCommand[];

        submitCommand[cmd->id](device, cmd);
    }
};

struct CommandBuffer {
    int commandCount;
    int maxCommands;
    char commands[];

    static CommandBuffer* create(HeapAllocator& allocator, int maxCommands) {
        size_t nbytes = sizeof(CommandBuffer) + maxCommands * COMMAND_MAX_SIZE;

        CommandBuffer* commandBuffer = (CommandBuffer*) allocator.allocate(nbytes);
        commandBuffer->commandCount = 0;
        commandBuffer->maxCommands = maxCommands;

        return commandBuffer;
    }

    static CommandBuffer* realloc(HeapAllocator& allocator, CommandBuffer* commandBuffer, int maxCommands) {
        size_t nbytes = sizeof(CommandBuffer) + maxCommands * COMMAND_MAX_SIZE;

        commandBuffer = (CommandBuffer*) allocator.reallocate(commandBuffer, nbytes);
        commandBuffer->maxCommands = maxCommands;

        return commandBuffer;
    }

    static void destroy(HeapAllocator& allocator, CommandBuffer* commandBuffer) {
        allocator.deallocate(commandBuffer);
    }

    static Command* getCommandAt(CommandBuffer* commandBuffer, int index) {
        return (Command*) &commandBuffer->commands[index * COMMAND_MAX_SIZE];
    }

    static void execute(CommandBuffer* commandBuffer, Device& device) {
        for(int i = 0; i < commandBuffer->commandCount; i++) {
            Command* command = getCommandAt(commandBuffer, i);

            Command::invoke(command, device);
        }
    }
};

template<typename T>
T* getCommand(CommandBuffer* commandBuffer) {
    static_assert(sizeof(T) <= COMMAND_MAX_SIZE, "Size of command should be less than or equal to COMMAND_MAX_SIZE");

    assert(commandBuffer->commandCount < commandBuffer->maxCommands);

    int index = commandBuffer->commandCount++;
    int offset = index * COMMAND_MAX_SIZE;
    T* command = (T*) &commandBuffer->commands[offset];
    command->command.id = T::TYPE;

    return command;
}

struct BindFramebuffer {
    Command command;
    Framebuffer framebuffer;

    static const uint32_t TYPE = BIND_FRAMEBUFFER;

    static void create(CommandBuffer* commandBuffer, Framebuffer framebuffer) {
        BindFramebuffer* bindFramebuffer = getCommand<BindFramebuffer>(commandBuffer);
        bindFramebuffer->framebuffer = framebuffer;
    }

    static void submit(Device& device, BindFramebuffer* cmd) {
        device.bindFramebuffer(cmd->framebuffer);
    }
};

struct ClearColor {
    Command command;
    float r, g, b, a;

    static const uint32_t TYPE = CLEAR_COLOR;

    static void create(CommandBuffer* commandBuffer, float r, float g, float b, float a) {
        ClearColor* clearColor = getCommand<ClearColor>(commandBuffer);
        clearColor->r = r;
        clearColor->g = g;
        clearColor->b = b;
        clearColor->a = a;
    }

    static void submit(Device& device, ClearColor* cmd) {
        glClearColor(cmd->r, cmd->g, cmd->b, cmd->a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
};

struct SetViewport {
    Command command;
    Viewport* viewport;

    static const uint32_t TYPE = SET_VIEWPORT;

    static void create(CommandBuffer* commandBuffer, Viewport* viewport) {
        SetViewport* setViewport = getCommand<SetViewport>(commandBuffer);
        setViewport->viewport = viewport;
    }

    static void submit(Device& device, SetViewport* cmd) {
        Viewport* viewport = cmd->viewport;

        glEnable(GL_SCISSOR_TEST);
        glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
        glScissor(viewport->x, viewport->y, viewport->width, viewport->height);
    }
};

struct SetDepthTest {
    Command command;
    bool enable;
    int function;

    static const uint32_t TYPE = SET_DEPTH_TEST;

    static void create(CommandBuffer* commandBuffer, bool enable, int function) {
        SetDepthTest* setDepthTest = getCommand<SetDepthTest>(commandBuffer);
        setDepthTest->enable = enable;
        setDepthTest->function = function;
    }

    static void submit(Device& device, SetDepthTest* cmd) {
        if(cmd->enable) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(cmd->function);

            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glDisable(GL_DEPTH_TEST);

            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
};

struct CopyConstantBuffer {
    Command command;
    ConstantBuffer constantBuffer;
    const void* data;
    size_t size;

    static const uint32_t TYPE = COPY_CONSTANT_BUFFER;

    static void create(CommandBuffer* commandBuffer, ConstantBuffer constantBuffer, const void* data, size_t size) {
        CopyConstantBuffer* copyConstantBuffer = getCommand<CopyConstantBuffer>(commandBuffer);
        copyConstantBuffer->constantBuffer = constantBuffer;
        copyConstantBuffer->data = data;
        copyConstantBuffer->size = size;
    }

    static void submit(Device& device, CopyConstantBuffer* cmd) {
        device.copyConstantBuffer(cmd->constantBuffer, cmd->data, cmd->size);
    }
};

struct BindVertexArray {
    Command command;
    VertexArray vertexArray;

    static const uint32_t TYPE = BIND_VERTEX_ARRAY;

    static void create(CommandBuffer* commandBuffer, VertexArray vertexArray) {
        BindVertexArray* bindVertexArray = getCommand<BindVertexArray>(commandBuffer);
        bindVertexArray->vertexArray = vertexArray;
    }

    static void submit(Device& device, BindVertexArray* cmd) {
        device.bindVertexArray(cmd->vertexArray);
    }
};

struct BindProgram {
    Command command;
    Program program;

    static const uint32_t TYPE = BIND_PROGRAM;

    static void create(CommandBuffer* commandBuffer, Program program) {
        BindProgram* bindProgram = getCommand<BindProgram>(commandBuffer);
        bindProgram->program = program;
    }

    static void submit(Device& device, BindProgram* cmd) {
        device.bindProgram(cmd->program);
    }
};

struct BindSampler {
    Command command;
    Program program;
    Sampler sampler;
    int unit;

    static const uint32_t TYPE = BIND_SAMPLER;

    static void create(CommandBuffer* commandBuffer, Program program, Sampler sampler, int unit) {
        BindSampler* bindSampler = getCommand<BindSampler>(commandBuffer);
        bindSampler->program = program;
        bindSampler->sampler = sampler;
        bindSampler->unit = unit;
    }

    static void submit(Device& device, BindSampler* cmd) {
        device.bindSampler(cmd->sampler, cmd->unit);
    }
};

struct BindTexture {
    Command command;
    Program program;
    Texture2D texture;

    static const uint32_t TYPE = BIND_TEXTURE0;

    static void create(CommandBuffer* commandBuffer, Program program, Texture2D texture, int unit) {
        BindTexture* bindTexture = getCommand<BindTexture>(commandBuffer);
        bindTexture->command.id += unit;
        bindTexture->program = program;
        bindTexture->texture = texture;
    }

    static void submit(Device& device, BindTexture* cmd) {
        int unit = cmd->command.id - BIND_TEXTURE0;
        device.bindTexture(cmd->program, cmd->texture, unit);
    }
};

struct DrawTriangles {
    Command command;
    int offset;
    int count;

    static const uint32_t TYPE = DRAW_TRIANGLES;

    static void create(CommandBuffer* commandBuffer, int offset, int count) {
        DrawTriangles* drawTriangles = getCommand<DrawTriangles>(commandBuffer);
        drawTriangles->offset = offset;
        drawTriangles->count = count;
    }

    static void submit(Device& device, DrawTriangles* cmd) {
        device.drawTriangles(cmd->offset, cmd->count);
    }
};

struct DrawTrianglesInstanced {
    Command command;
    int offset;
    int count;
    int instances;

    static const uint32_t TYPE = DRAW_TRIANGLES_INSTANCED;

    static void create(CommandBuffer* commandBuffer, int offset, int count, int instances) {
        DrawTrianglesInstanced* drawTrianglesInstanced = getCommand<DrawTrianglesInstanced>(commandBuffer);
        drawTrianglesInstanced->offset = offset;
        drawTrianglesInstanced->count = count;
        drawTrianglesInstanced->instances = instances;
    }

    static void submit(Device& device, DrawTrianglesInstanced* cmd) {
        device.drawTrianglesInstanced(cmd->offset, cmd->count, cmd->instances);
    }
};

const FnSubmitCommand submitCommand[] = {
        [DRAW_TRIANGLES] = FnSubmitCommand(DrawTriangles::submit),
        [DRAW_TRIANGLES_INSTANCED] = FnSubmitCommand(DrawTrianglesInstanced::submit),
        [CLEAR_COLOR] = FnSubmitCommand(ClearColor::submit),
        [BIND_FRAMEBUFFER] = FnSubmitCommand(BindFramebuffer::submit),
        [SET_VIEWPORT] = FnSubmitCommand(SetViewport::submit),
        [SET_DEPTH_TEST] = FnSubmitCommand(SetDepthTest::submit),
        [COPY_CONSTANT_BUFFER] = FnSubmitCommand(CopyConstantBuffer::submit),
        [BIND_VERTEX_ARRAY] = FnSubmitCommand(BindVertexArray::submit),
        [BIND_PROGRAM] = FnSubmitCommand(BindProgram::submit),
        [BIND_TEXTURE0] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE1] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE2] = FnSubmitCommand(BindTexture::submit),
        [BIND_SAMPLER] = FnSubmitCommand(BindSampler::submit),
};

const int sizeCommand[] = {
        [DRAW_TRIANGLES] = sizeof(DrawTriangles),
        [DRAW_TRIANGLES_INSTANCED] = sizeof(DrawTrianglesInstanced),
        [CLEAR_COLOR] = sizeof(ClearColor),
        [BIND_FRAMEBUFFER] = sizeof(BindFramebuffer),
        [SET_VIEWPORT] = sizeof(SetViewport),
        [SET_DEPTH_TEST] = sizeof(SetDepthTest),
        [COPY_CONSTANT_BUFFER] = sizeof(CopyConstantBuffer),
        [BIND_VERTEX_ARRAY] = sizeof(BindVertexArray),
        [BIND_PROGRAM] = sizeof(BindProgram),
        [BIND_TEXTURE0] = sizeof(BindTexture),
        [BIND_TEXTURE1] = sizeof(BindTexture),
        [BIND_TEXTURE2] = sizeof(BindTexture),
        [BIND_SAMPLER] = sizeof(BindSampler),
};

#endif //COMMANDS_H
