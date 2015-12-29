//
// Created by Marrony Neris on 11/7/15.
//

#ifndef COMMANDS_H
#define COMMANDS_H

#include <assert.h>

#include "Allocator.h"
#include "Device.h"

enum CommandType {
    DRAW_ARRAYS,
    DRAW_ARRAYS_INSTANCED,
    DRAW_TRIANGLES,
    DRAW_TRIANGLES_INSTANCED,
    CLEAR_COLOR0,
    CLEAR_COLOR1,
    CLEAR_COLOR2,
    CLEAR_COLOR3,
    CLEAR_COLOR4,
    CLEAR_COLOR5,
    CLEAR_COLOR6,
    CLEAR_COLOR7,
    CLEAR_DEPTH_STENCIL,
    DIRECT_COMMANDS_MAX = CLEAR_DEPTH_STENCIL,
    SET_VIEWPORT0,
    SET_VIEWPORT1,
    SET_VIEWPORT2,
    SET_VIEWPORT3,
    SET_SCISSOR0,
    SET_SCISSOR1,
    SET_SCISSOR2,
    SET_SCISSOR3,
    SET_DEPTH_TEST,
    SET_CULL_FACE,
    SET_BLEND0,
    SET_BLEND1,
    SET_BLEND2,
    SET_BLEND3,
    SET_BLEND4,
    SET_BLEND5,
    SET_BLEND6,
    SET_BLEND7,
    SET_DRAWBUFFERS,
    COPY_CONSTANT_BUFFER,
    BIND_CONSTANT_BUFFER,
    BIND_FRAMEBUFFER,
    BIND_VERTEX_ARRAY,
    BIND_PROGRAM,
    BIND_TEXTURE0,
    BIND_TEXTURE1,
    BIND_TEXTURE2,
    BIND_TEXTURE3,
    BIND_TEXTURE4,
    BIND_TEXTURE5,
    BIND_TEXTURE6,
    BIND_TEXTURE7,
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
    float color[4];

    static const uint32_t TYPE = CLEAR_COLOR0;

    static void create(CommandBuffer* commandBuffer, int index, float r, float g, float b, float a) {
        ClearColor* clearColor = getCommand<ClearColor>(commandBuffer);
        clearColor->command.id += index;
        clearColor->color[0] = r;
        clearColor->color[1] = g;
        clearColor->color[2] = b;
        clearColor->color[3] = a;
    }

    static void create(CommandBuffer* commandBuffer, int index, const float color[4]) {
        create(commandBuffer, index, color[0], color[1], color[2], color[3]);
    }

    static void submit(Device& device, ClearColor* cmd) {
        int index = cmd->command.id - CLEAR_COLOR0;
        glClearBufferfv(GL_COLOR, index, cmd->color); CHECK_ERROR;
    }
};

struct ClearDepthStencil {
    Command command;
    float depth;
    int stencil;

    static const uint32_t TYPE = CLEAR_DEPTH_STENCIL;

    static void create(CommandBuffer* commandBuffer, float depth, int stencil) {
        ClearDepthStencil* clearDepthStencil = getCommand<ClearDepthStencil>(commandBuffer);
        clearDepthStencil->depth = depth;
        clearDepthStencil->stencil = stencil;
    }

    static void submit(Device& device, ClearDepthStencil* cmd) {
        glClearBufferfi(GL_DEPTH_STENCIL, 0, cmd->depth, cmd->stencil); CHECK_ERROR;
    }
};

struct SetViewport {
    Command command;
    Rect* viewport;

    static const uint32_t TYPE = SET_VIEWPORT0;

    static void create(CommandBuffer* commandBuffer, int index, Rect* viewport) {
        SetViewport* setViewport = getCommand<SetViewport>(commandBuffer);
        setViewport->command.id += index;
        setViewport->viewport = viewport;
    }

    static void submit(Device& device, SetViewport* cmd) {
        int index = cmd->command.id - SET_VIEWPORT0;
        Rect* viewport = cmd->viewport;

        glViewportIndexedf(index, viewport->x, viewport->y, viewport->width, viewport->height);
    }
};

struct SetScissor {
    Command command;
    Rect* viewport;
    bool enable;

    static const uint32_t TYPE = SET_SCISSOR0;

    static void create(CommandBuffer* commandBuffer, int index, bool enable, Rect* viewport) {
        SetScissor* setScissor = getCommand<SetScissor>(commandBuffer);
        setScissor->command.id += index;
        setScissor->viewport = viewport;
        setScissor->enable = enable;
    }

    static void submit(Device& device, SetScissor* cmd) {
        int index = cmd->command.id - SET_SCISSOR0;
        Rect* viewport = cmd->viewport;

        if(cmd->enable) {
            glEnable(GL_SCISSOR_TEST);
            glScissorIndexed(index, viewport->x, viewport->y, viewport->width, viewport->height);
        } else {
            glEnable(GL_SCISSOR_TEST);
        }
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

    static void disable(CommandBuffer* commandBuffer) {
        create(commandBuffer, false, GL_NONE);
    }

    static void submit(Device& device, SetDepthTest* cmd) {
        if(cmd->enable) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(cmd->function); CHECK_ERROR;
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }
};

struct SetCullFace {
    Command command;
    bool enable;
    int cullFace;
    int frontFace;

    static const uint32_t TYPE = SET_CULL_FACE;

    static void create(CommandBuffer* commandBuffer, bool enable, int cullFace, int frontFace) {
        SetCullFace* setCullFace = getCommand<SetCullFace>(commandBuffer);
        setCullFace->enable = enable;
        setCullFace->cullFace = cullFace;
        setCullFace->frontFace = frontFace;
    }

    static void disable(CommandBuffer* commandBuffer) {
        create(commandBuffer, false, GL_NONE, GL_NONE);
    }

    static void submit(Device& device, SetCullFace* cmd) {
        if(cmd->enable) {
            glEnable(GL_CULL_FACE);
            glCullFace(cmd->cullFace); CHECK_ERROR;
            glFrontFace(cmd->frontFace); CHECK_ERROR;
        } else {
            glDisable(GL_CULL_FACE);
        }
    }
};

struct SetBlend {
    Command command;
    bool enable;
    uint16_t equationColor;
    uint16_t srcColor;
    uint16_t dstColor;
    uint16_t equationAlpha;
    uint16_t srcAlpha;
    uint16_t dstAlpha;

    static const uint32_t TYPE = SET_BLEND0;

    static void create(CommandBuffer* commandBuffer, bool enable, int index, int equationColor, int srcColor, int dstColor, int equationAlpha, int srcAlpha, int dstAlpha) {
        SetBlend* setBlend = getCommand<SetBlend>(commandBuffer);
        setBlend->command.id += index;
        setBlend->enable = enable;
        setBlend->equationAlpha = equationAlpha;
        setBlend->srcColor = srcColor;
        setBlend->dstColor = dstColor;
        setBlend->equationColor = equationColor;
        setBlend->srcAlpha = srcAlpha;
        setBlend->dstAlpha = dstAlpha;
    }

    static void create(CommandBuffer* commandBuffer, bool enable, int index, int equation, int src, int dst) {
        create(commandBuffer, enable, index, equation, src, dst, equation, src, dst);
    }

    static void disable(CommandBuffer* commandBuffer, int index) {
        create(commandBuffer, false, index, GL_NONE, GL_NONE, GL_NONE);
    }

    static void submit(Device& device, SetBlend* cmd) {
        int index = cmd->command.id - SET_BLEND0;
        if(cmd->enable) {
            glEnablei(GL_BLEND, index);
            glBlendEquationSeparatei(index, cmd->equationColor, cmd->equationAlpha); CHECK_ERROR;
            glBlendFuncSeparatei(index, cmd->srcColor, cmd->dstColor, cmd->srcAlpha, cmd->dstAlpha); CHECK_ERROR;
        } else {
            glDisablei(GL_BLEND, index);
        }
    }
};

struct SetDrawBuffers {
    Command command;
    uint32_t mask;

    static const uint32_t TYPE = SET_DRAWBUFFERS;

    static void create(CommandBuffer* commandBuffer, uint32_t mask) {
        SetDrawBuffers* setDrawBuffers = getCommand<SetDrawBuffers>(commandBuffer);
        setDrawBuffers->mask = mask;
    }

    static void submit(Device& device, SetDrawBuffers* cmd) {
        uint32_t mask = cmd->mask;

        //todo find a way to change back to default draw buffer
        if(mask == 0xffffffff) {
            glDrawBuffer(GL_BACK_LEFT);
            return;
        }

        int count = 0;
        GLenum buffers[32] = {};

        for (int i = 0; i < 32; i++) {
            if (mask & (1 << i)) {
                buffers[count] = GL_COLOR_ATTACHMENT0 + i;
                count++;
            }
        }

        if (count > 0) {
            glDrawBuffers(count, buffers);
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

struct BindConstantBuffer {
    Command command;
    ConstantBuffer constantBuffer;
    int bindingPoint;

    static const uint32_t TYPE = BIND_CONSTANT_BUFFER;

    static void create(CommandBuffer* commandBuffer, ConstantBuffer constantBuffer, int bindingPoint) {
        BindConstantBuffer* bindConstantBuffer = getCommand<BindConstantBuffer>(commandBuffer);
        bindConstantBuffer->constantBuffer = constantBuffer;
        bindConstantBuffer->bindingPoint = bindingPoint;
    }

    static void submit(Device& device, BindConstantBuffer* cmd) {
        glBindBufferBase(GL_UNIFORM_BUFFER, cmd->bindingPoint, cmd->constantBuffer.id); CHECK_ERROR;
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

struct BindTexture {
    Command command;
    bool isCube;
    union {
        Texture2D texture2D;
        TextureCube textureCube;
    };
    Sampler sampler;

    static const uint32_t TYPE = BIND_TEXTURE0;

    static void create(CommandBuffer* commandBuffer, Texture2D texture, Sampler sampler, int unit) {
        BindTexture* bindTexture = getCommand<BindTexture>(commandBuffer);
        bindTexture->command.id += unit;
        bindTexture->isCube = false;
        bindTexture->texture2D = texture;
        bindTexture->sampler = sampler;
    }

    static void create(CommandBuffer* commandBuffer, TextureCube texture, Sampler sampler, int unit) {
        BindTexture* bindTexture = getCommand<BindTexture>(commandBuffer);
        bindTexture->command.id += unit;
        bindTexture->isCube = true;
        bindTexture->textureCube = texture;
        bindTexture->sampler = sampler;
    }

    static void submit(Device& device, BindTexture* cmd) {
        int unit = cmd->command.id - BIND_TEXTURE0;

        if (cmd->isCube)
            device.bindTexture(cmd->textureCube, unit);
        else
            device.bindTexture(cmd->texture2D, unit);

        device.bindSampler(cmd->sampler, unit);
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

struct DrawArrays {
    Command command;
    int type;
    int offset;
    int count;

    static const uint32_t TYPE = DRAW_ARRAYS;

    static void create(CommandBuffer* commandBuffer, int type, int offset, int count) {
        DrawArrays* drawArrays = getCommand<DrawArrays>(commandBuffer);
        drawArrays->type = type;
        drawArrays->offset = offset;
        drawArrays->count = count;
    }

    static void submit(Device& device, DrawArrays* cmd) {
        device.drawArrays(cmd->type, cmd->offset, cmd->count);
    }
};

struct DrawArraysInstanced {
    Command command;
    int type;
    int offset;
    int count;
    int instances;

    static const uint32_t TYPE = DRAW_ARRAYS_INSTANCED;

    static void create(CommandBuffer* commandBuffer, int type, int offset, int count, int instances) {
        DrawArraysInstanced* drawArrays = getCommand<DrawArraysInstanced>(commandBuffer);
        drawArrays->type = type;
        drawArrays->offset = offset;
        drawArrays->count = count;
        drawArrays->instances = instances;
    }

    static void submit(Device& device, DrawArraysInstanced* cmd) {
        device.drawArraysInstanced(cmd->type, cmd->offset, cmd->count, cmd->instances);
    }
};

extern const FnSubmitCommand submitCommand[];
extern const int sizeCommand[];

#endif //COMMANDS_H
