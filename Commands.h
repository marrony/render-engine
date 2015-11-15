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
    SET_VIEWPORT_REL,
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

struct Command {
    uint32_t id;
};

struct CommandBuffer {
    int commandCount;
    int maxCommands;
    char commands[];

    static CommandBuffer* create(LinearAllocator& allocator, int maxCommands) {
        size_t nbytes = sizeof(CommandBuffer) + maxCommands * COMMAND_MAX_SIZE;

        CommandBuffer* commandBuffer = (CommandBuffer*) allocator.allocate(nbytes);
        commandBuffer->commandCount = 0;
        commandBuffer->maxCommands = maxCommands;

        return commandBuffer;
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

Command* getCommandAt(CommandBuffer* commandBuffer, int index) {
    return (Command*) &commandBuffer->commands[index * COMMAND_MAX_SIZE];
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

//todo remove this?
struct SetViewportRelative {
    Command command;
    float x;
    float y;
    float width;
    float height;

    static const uint32_t TYPE = SET_VIEWPORT_REL;

    static void create(CommandBuffer* commandBuffer, float x, float y, float width, float height) {
        SetViewportRelative* setViewport = getCommand<SetViewportRelative>(commandBuffer);
        setViewport->x = x;
        setViewport->y = y;
        setViewport->width = width;
        setViewport->height = height;
    }

    static void submit(Device& device, SetViewportRelative* cmd) {
        GLFWwindow* window = glfwGetCurrentContext();

        int w = 0;
        int h = 0;

        GLint framebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);

        if(framebuffer != 0) {
            GLint type;
            GLint object;

            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &object);

            if(type == GL_TEXTURE) {
                GLint tex;

                glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);

                glBindTexture(GL_TEXTURE_2D, object);

                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

                glBindTexture(GL_TEXTURE_2D, tex);
            } else if(type == GL_RENDERBUFFER) {
                glBindRenderbuffer(GL_RENDERBUFFER, object);

                glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w);
                glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h);

                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }
        } else {
            glfwGetFramebufferSize(window, &w, &h);
        }

        int x = w * cmd->x;
        int y = h * cmd->y;
        int width = (w * cmd->width) - x;
        int height = (h * cmd->height) - y;

        glEnable(GL_SCISSOR_TEST);
        glViewport(x, y, width, height);
        glScissor(x, y, width, height);
    }
};

struct SetViewport {
    Command command;
    int x;
    int y;
    int width;
    int height;

    static const uint32_t TYPE = SET_VIEWPORT;

    static void create(CommandBuffer* commandBuffer, int x, int y, int width, int height) {
        SetViewport* setViewport = getCommand<SetViewport>(commandBuffer);
        setViewport->x = x;
        setViewport->y = y;
        setViewport->width = width;
        setViewport->height = height;
    }

    static void submit(Device& device, SetViewport* cmd) {
        glEnable(GL_SCISSOR_TEST);
        glViewport(cmd->x, cmd->y, cmd->width, cmd->height);
        glScissor(cmd->x, cmd->y, cmd->width, cmd->height);
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

    static void create(CommandBuffer* commandBuffer, Program program, Texture2D texture, int index) {
        BindTexture* bindTexture = getCommand<BindTexture>(commandBuffer);
        bindTexture->command.id += index;
        bindTexture->program = program;
        bindTexture->texture = texture;
    }

    static void submit(Device& device, BindTexture* cmd) {
        int index = cmd->command.id - BIND_TEXTURE0;
        device.bindTexture(cmd->program, cmd->texture, index);
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

typedef void (* FnSubmitCommand)(Device& device, Command* command);

const FnSubmitCommand submitCommand[] = {
        [DRAW_TRIANGLES] = FnSubmitCommand(DrawTriangles::submit),
        [DRAW_TRIANGLES_INSTANCED] = FnSubmitCommand(DrawTrianglesInstanced::submit),
        [CLEAR_COLOR] = FnSubmitCommand(ClearColor::submit),
        [BIND_FRAMEBUFFER] = FnSubmitCommand(BindFramebuffer::submit),
        [SET_VIEWPORT_REL] = FnSubmitCommand(SetViewportRelative::submit),
        [SET_VIEWPORT] = FnSubmitCommand(SetViewport::submit),
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
        [SET_VIEWPORT_REL] = sizeof(SetViewportRelative),
        [SET_VIEWPORT] = sizeof(SetViewport),
        [COPY_CONSTANT_BUFFER] = sizeof(CopyConstantBuffer),
        [BIND_VERTEX_ARRAY] = sizeof(BindVertexArray),
        [BIND_PROGRAM] = sizeof(BindProgram),
        [BIND_TEXTURE0] = sizeof(BindTexture),
        [BIND_TEXTURE1] = sizeof(BindTexture),
        [BIND_TEXTURE2] = sizeof(BindTexture),
        [BIND_SAMPLER] = sizeof(BindSampler),
};

#endif //COMMANDS_H
