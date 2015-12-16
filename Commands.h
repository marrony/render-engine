//
// Created by Marrony Neris on 11/7/15.
//

#ifndef COMMANDS_H
#define COMMANDS_H

#include "assert.h"

enum CommandType {
    DRAW_TRIANGLES = 0,
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
    COPY_CONSTANT_BUFFER,
    DIRECT_COMMANDS_MAX = COPY_CONSTANT_BUFFER,
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
    int equation;
    int src;
    int dst;

    static const uint32_t TYPE = SET_BLEND0;

    static void create(CommandBuffer* commandBuffer, bool enable, int index, int equation, int src, int dst) {
        SetBlend* setBlend = getCommand<SetBlend>(commandBuffer);
        setBlend->command.id += index;
        setBlend->enable = enable;
        setBlend->equation = equation;
        setBlend->src = src;
        setBlend->dst = dst;
    }

    static void disable(CommandBuffer* commandBuffer, int index) {
        create(commandBuffer, false, index, GL_NONE, GL_NONE, GL_NONE);
    }

    static void submit(Device& device, SetBlend* cmd) {
        int index = cmd->command.id - SET_BLEND0;
        if(cmd->enable) {
            glEnablei(GL_BLEND, index);
            glBlendEquationSeparatei(index, cmd->equation, cmd->equation); CHECK_ERROR;
            glBlendFuncSeparatei(index, cmd->src, cmd->dst, cmd->src, cmd->dst); CHECK_ERROR;
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
    Texture2D texture;
    Sampler sampler;

    static const uint32_t TYPE = BIND_TEXTURE0;

    static void create(CommandBuffer* commandBuffer, Texture2D texture, Sampler sampler, int unit) {
        BindTexture* bindTexture = getCommand<BindTexture>(commandBuffer);
        bindTexture->command.id += unit;
        bindTexture->texture = texture;
        bindTexture->sampler = sampler;
    }

    static void submit(Device& device, BindTexture* cmd) {
        int unit = cmd->command.id - BIND_TEXTURE0;
        device.bindTexture(cmd->texture, unit);
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

const FnSubmitCommand submitCommand[] = {
        [DRAW_TRIANGLES] = FnSubmitCommand(DrawTriangles::submit),
        [DRAW_TRIANGLES_INSTANCED] = FnSubmitCommand(DrawTrianglesInstanced::submit),
        [CLEAR_COLOR0] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR1] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR2] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR3] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR4] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR5] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR6] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_COLOR7] = FnSubmitCommand(ClearColor::submit),
        [CLEAR_DEPTH_STENCIL] = FnSubmitCommand(ClearDepthStencil::submit),
        [BIND_FRAMEBUFFER] = FnSubmitCommand(BindFramebuffer::submit),
        [SET_VIEWPORT0] = FnSubmitCommand(SetViewport::submit),
        [SET_VIEWPORT1] = FnSubmitCommand(SetViewport::submit),
        [SET_VIEWPORT2] = FnSubmitCommand(SetViewport::submit),
        [SET_VIEWPORT3] = FnSubmitCommand(SetViewport::submit),
        [SET_SCISSOR0] = FnSubmitCommand(SetScissor::submit),
        [SET_SCISSOR1] = FnSubmitCommand(SetScissor::submit),
        [SET_SCISSOR2] = FnSubmitCommand(SetScissor::submit),
        [SET_SCISSOR3] = FnSubmitCommand(SetScissor::submit),
        [SET_DEPTH_TEST] = FnSubmitCommand(SetDepthTest::submit),
        [SET_CULL_FACE] = FnSubmitCommand(SetCullFace::submit),
        [SET_BLEND0] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND1] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND2] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND3] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND4] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND5] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND6] = FnSubmitCommand(SetBlend::submit),
        [SET_BLEND7] = FnSubmitCommand(SetBlend::submit),
        [SET_DRAWBUFFERS] = FnSubmitCommand(SetDrawBuffers::submit),
        [COPY_CONSTANT_BUFFER] = FnSubmitCommand(CopyConstantBuffer::submit),
        [BIND_CONSTANT_BUFFER] = FnSubmitCommand(BindConstantBuffer::submit),
        [BIND_VERTEX_ARRAY] = FnSubmitCommand(BindVertexArray::submit),
        [BIND_PROGRAM] = FnSubmitCommand(BindProgram::submit),
        [BIND_TEXTURE0] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE1] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE2] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE3] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE4] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE5] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE6] = FnSubmitCommand(BindTexture::submit),
        [BIND_TEXTURE7] = FnSubmitCommand(BindTexture::submit),
};

const int sizeCommand[] = {
        [DRAW_TRIANGLES] = sizeof(DrawTriangles),
        [DRAW_TRIANGLES_INSTANCED] = sizeof(DrawTrianglesInstanced),
        [CLEAR_COLOR0] = sizeof(ClearColor),
        [CLEAR_COLOR1] = sizeof(ClearColor),
        [CLEAR_COLOR2] = sizeof(ClearColor),
        [CLEAR_COLOR3] = sizeof(ClearColor),
        [CLEAR_COLOR4] = sizeof(ClearColor),
        [CLEAR_COLOR5] = sizeof(ClearColor),
        [CLEAR_COLOR6] = sizeof(ClearColor),
        [CLEAR_COLOR7] = sizeof(ClearColor),
        [CLEAR_DEPTH_STENCIL] = sizeof(ClearDepthStencil),
        [BIND_FRAMEBUFFER] = sizeof(BindFramebuffer),
        [SET_VIEWPORT0] = sizeof(SetViewport),
        [SET_VIEWPORT1] = sizeof(SetViewport),
        [SET_VIEWPORT2] = sizeof(SetViewport),
        [SET_VIEWPORT3] = sizeof(SetViewport),
        [SET_SCISSOR0] = sizeof(SetScissor),
        [SET_SCISSOR1] = sizeof(SetScissor),
        [SET_SCISSOR2] = sizeof(SetScissor),
        [SET_SCISSOR3] = sizeof(SetScissor),
        [SET_DEPTH_TEST] = sizeof(SetDepthTest),
        [SET_CULL_FACE] = sizeof(SetCullFace),
        [SET_BLEND0] = sizeof(SetBlend),
        [SET_BLEND1] = sizeof(SetBlend),
        [SET_BLEND2] = sizeof(SetBlend),
        [SET_BLEND3] = sizeof(SetBlend),
        [SET_BLEND4] = sizeof(SetBlend),
        [SET_BLEND5] = sizeof(SetBlend),
        [SET_BLEND6] = sizeof(SetBlend),
        [SET_BLEND7] = sizeof(SetBlend),
        [SET_DRAWBUFFERS] = sizeof(SetDrawBuffers),
        [COPY_CONSTANT_BUFFER] = sizeof(CopyConstantBuffer),
        [BIND_CONSTANT_BUFFER] = sizeof(BindConstantBuffer),
        [BIND_VERTEX_ARRAY] = sizeof(BindVertexArray),
        [BIND_PROGRAM] = sizeof(BindProgram),
        [BIND_TEXTURE0] = sizeof(BindTexture),
        [BIND_TEXTURE1] = sizeof(BindTexture),
        [BIND_TEXTURE2] = sizeof(BindTexture),
        [BIND_TEXTURE3] = sizeof(BindTexture),
        [BIND_TEXTURE4] = sizeof(BindTexture),
        [BIND_TEXTURE5] = sizeof(BindTexture),
        [BIND_TEXTURE6] = sizeof(BindTexture),
        [BIND_TEXTURE7] = sizeof(BindTexture),
};

#endif //COMMANDS_H
