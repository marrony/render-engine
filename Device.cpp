//
// Created by Marrony Neris on 12/15/15.
//

#include "Device.h"

void check_error(const char* file, int line) {
    switch (glGetError()) {
        case GL_NO_ERROR:
            break;
        case GL_INVALID_ENUM:
            printf("glError(GL_INVALID_ENUM) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_INVALID_VALUE:
            printf("glError(GL_INVALID_VALUE) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_INVALID_OPERATION:
            printf("glError(GL_INVALID_OPERATION) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            printf("glError(GL_INVALID_FRAMEBUFFER_OPERATION) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_OUT_OF_MEMORY:
            printf("glError(GL_OUT_OF_MEMORY) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_STACK_UNDERFLOW:
            printf("glError(GL_STACK_UNDERFLOW) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        case GL_STACK_OVERFLOW:
            printf("glError(GL_STACK_OVERFLOW) = %s:%d\n", file, line);
            exit(EXIT_FAILURE);
            break;
        default:
            break;
    }
}

Device::Device() {
    vertexBufferCount = 0;
    indexBufferCount = 0;
    constantBufferCount = 0;
    vertexArrayCount = 0;
    textureCount = 0;
    samplerCount = 0;
    programCount = 0;
    vertexProgramCount = 0;
    fragmentProgramCount = 0;
    framebufferCount = 0;
    renderbufferCount = 0;
}

Device::~Device() {
    assert(vertexBufferCount == 0);
    assert(indexBufferCount == 0);
    assert(constantBufferCount == 0);
    assert(vertexArrayCount == 0);
    assert(textureCount == 0);
    assert(samplerCount == 0);
    assert(programCount == 0);
    assert(vertexProgramCount == 0);
    assert(fragmentProgramCount == 0);
    assert(framebufferCount == 0);
    assert(renderbufferCount == 0);
}

VertexBuffer Device::createDynamicVertexBuffer(size_t size, const void* data) {
    GLuint vbo;

    glGenBuffers(1, &vbo); CHECK_ERROR;

    glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_ERROR;
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW); CHECK_ERROR;

    glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;

    vertexBufferCount++;

    return {vbo};
}

VertexBuffer Device::createStaticVertexBuffer(size_t size, const void* data) {
    GLuint vbo;

    glGenBuffers(1, &vbo); CHECK_ERROR;

    glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_ERROR;
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); CHECK_ERROR;

    glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;

    vertexBufferCount++;

    return {vbo};
}

IndexBuffer Device::createIndexBuffer(size_t size, const void* data) {
    GLuint ibo;

    glGenBuffers(1, &ibo); CHECK_ERROR;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); CHECK_ERROR;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); CHECK_ERROR;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_ERROR;

    indexBufferCount++;

    return {ibo};
}

ConstantBuffer Device::createConstantBuffer(size_t size) {
    GLuint cbo;

    glGenBuffers(1, &cbo); CHECK_ERROR;

    glBindBuffer(GL_UNIFORM_BUFFER, cbo); CHECK_ERROR;
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW); CHECK_ERROR;

    glBindBuffer(GL_UNIFORM_BUFFER, 0); CHECK_ERROR;

    constantBufferCount++;

    return {cbo};
}

void Device::setConstantBufferBindingPoint(Program program, const char* blockName, int bindingPoint) {
    int index = glGetUniformBlockIndex(program.id, blockName);
    glUniformBlockBinding(program.id, index, bindingPoint); CHECK_ERROR;
}

void Device::setTextureBindingPoint(Program program, const char* name, int bindingPoint) {
    int index = glGetUniformLocation(program.id, name);
    if (index != -1) {
        glUseProgram(program.id);
        glUniform1i(index, bindingPoint);
    }
}

VertexArray Device::createVertexArray(const VertexDeclaration* vertexDeclarations, int size,
                              IndexBuffer indexBuffer) {
    GLuint vao;

    glGenVertexArrays(1, &vao); CHECK_ERROR;

    glBindVertexArray(vao); CHECK_ERROR;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id); CHECK_ERROR;

    for (int i = 0; i < size; i++) {
        VertexFormat format = vertexDeclarations[i].format;
        uint32_t stride = vertexDeclarations[i].stride;
        void* offset = vertexDeclarations[i].offset;
        VertexBuffer vertexBuffer = vertexDeclarations[i].buffer;

        if (vertexBuffer.id != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id); CHECK_ERROR;
            glVertexAttribPointer(i, format.size, format.type, GL_FALSE, stride, offset); CHECK_ERROR;
            glEnableVertexAttribArray(i); CHECK_ERROR;
        } else {
            glDisableVertexAttribArray(i); CHECK_ERROR;
        }
    }

    glBindVertexArray(0);

    vertexArrayCount++;

    return {vao};
}

Sampler Device::createSampler(int minFilter, int magFilter, int mipFilter) {
    assert(minFilter == GL_NEAREST || minFilter == GL_LINEAR);
    assert(magFilter == GL_NEAREST || magFilter == GL_LINEAR);
    assert(mipFilter == GL_NEAREST || mipFilter == GL_LINEAR || mipFilter == GL_NONE);

    GLuint sampler;

    glGenSamplers(1, &sampler); CHECK_ERROR;
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;

    if(minFilter == GL_NEAREST && mipFilter == GL_NEAREST)
        minFilter = GL_NEAREST_MIPMAP_NEAREST;

    if(minFilter == GL_NEAREST && mipFilter == GL_LINEAR)
        minFilter = GL_NEAREST_MIPMAP_LINEAR;

    if(minFilter == GL_LINEAR && mipFilter == GL_NEAREST)
        minFilter = GL_LINEAR_MIPMAP_NEAREST;

    if(minFilter == GL_LINEAR && mipFilter == GL_LINEAR)
        minFilter = GL_LINEAR_MIPMAP_LINEAR;

    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, minFilter); CHECK_ERROR;
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, magFilter); CHECK_ERROR;

    samplerCount++;

    return {sampler};
}

struct TextureBinder {
    GLint data;

    TextureBinder() {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &data); CHECK_ERROR;
    }

    ~TextureBinder() {
        glBindTexture(GL_TEXTURE_2D, data); CHECK_ERROR;
    }
};

Texture2D createTexture(uint32_t& textureCount, int internalFormat, int width, int height, int format, int type, const void* pixels) {
    TextureBinder binder;

    GLuint texId;
    glGenTextures(1, &texId); CHECK_ERROR;

    glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, pixels); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;

    textureCount++;

    return {texId};
}

struct TextureCubeBinder {
    GLint data;

    TextureCubeBinder() {
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &data); CHECK_ERROR;
    }

    ~TextureCubeBinder() {
        glBindTexture(GL_TEXTURE_CUBE_MAP, data); CHECK_ERROR;
    }
};

TextureCube createTextureCube(uint32_t& textureCount, int internalFormat, int width, int height, int format, int type, void* pixels[6]) {
    TextureCubeBinder binder;

    GLuint texId;
    glGenTextures(1, &texId); CHECK_ERROR;

    glBindTexture(GL_TEXTURE_CUBE_MAP, texId); CHECK_ERROR;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, width, height, 0, format, type, pixels[POSITIVE_X]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, width, height, 0, format, type, pixels[NEGATIVE_X]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, width, height, 0, format, type, pixels[POSITIVE_Y]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, width, height, 0, format, type, pixels[NEGATIVE_Y]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, width, height, 0, format, type, pixels[POSITIVE_Z]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, width, height, 0, format, type, pixels[NEGATIVE_Z]);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;

    textureCount++;

    return {texId};
}

Texture2D Device::createRGB16FTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGB16F, width, height, GL_RGB, GL_FLOAT, pixels);
}

Texture2D Device::createRGBA16FTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGBA16F, width, height, GL_RGBA, GL_FLOAT, pixels);
}

Texture2D Device::createRGB32FTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGB32F, width, height, GL_RGB, GL_FLOAT, pixels);
}

Texture2D Device::createRGBA32FTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT, pixels);
}

Texture2D Device::createRGBATexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

Texture2D Device::createRGBTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

Texture2D Device::createRGBAFTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGBA, width, height, GL_RGBA, GL_FLOAT, pixels);
}

Texture2D Device::createRGBFTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RGB, width, height, GL_RGB, GL_FLOAT, pixels);
}

Texture2D Device::createRTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RED, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

Texture2D Device::createRG32FTexture(int width, int height, const void* pixels) {
    return createTexture(textureCount, GL_RG32F, width, height, GL_RGB, GL_FLOAT, pixels);
}

TextureCube Device::createRGBCubeTexture(int width, int height, void* pixels[6]) {
    return createTextureCube(textureCount, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

DepthTexture Device::createDepth32FTexture(int width, int height) {
    Texture2D texture = createTexture(textureCount, GL_DEPTH_COMPONENT32F, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return {texture.id};
}

DepthStencilTexture Device::createDepth24Stencil8Texture(int width, int height) {
    Texture2D texture = createTexture(textureCount, GL_DEPTH24_STENCIL8, width, height, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    return {texture.id};
}

SeparateProgram Device::createVertexProgram(const char* commonSource, const char* source) {
    const char* source2[] = {
            "#version 410 core\n",
            "#define SEPARATE_VERTEX_SHADER\n",
            commonSource,
            source
    };

    GLint status;
    GLuint program = glCreateShaderProgramv(GL_VERTEX_SHADER, 4, source2);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char info[1024];

        glGetProgramInfoLog(program, 1024, nullptr, info);
        printf("program: %s\n", info);
        exit(-1);
    }

    vertexProgramCount++;

    return {program};
}

SeparateProgram Device::createFragmentProgram(const char* commonSource, const char* source) {
    const char* source2[] = {
            "#version 410 core\n",
            "#define SEPARATE_FRAGMENT_SHADER\n",
            commonSource,
            source
    };

    GLint status;
    GLuint program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 4, source2);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char info[1024];

        glGetProgramInfoLog(program, 1024, nullptr, info);
        printf("program: %s\n", info);
        exit(-1);
    }

    fragmentProgramCount++;

    return {program};
}

Program Device::createProgram(const char* commonSource, const char* vertexSource, const char* fragmentSource, const char* geometrySource) {
    GLint status;

    const char* geometrySource2[] = {
            "#version 410 core\n",
            "#define GEOMETRY_SHADER\n",
            commonSource,
            geometrySource
    };

    GLuint geometryShader = 0;

    if(geometrySource != nullptr) {
        geometryShader = glCreateShader(GL_GEOMETRY_SHADER); CHECK_ERROR;
        glShaderSource(geometryShader, 4, geometrySource2, nullptr); CHECK_ERROR;
        glCompileShader(geometryShader); CHECK_ERROR;
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char info[1024];

            glGetShaderInfoLog(geometryShader, 1024, nullptr, info);
            printf("geometryShader: %s\n", info);
            exit(-1);
        }
    }

    const char* vertexSource2[] = {
            "#version 410 core\n",
            "#define VERTEX_SHADER\n",
            commonSource,
            vertexSource
    };

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); CHECK_ERROR;
    glShaderSource(vertexShader, 4, vertexSource2, nullptr); CHECK_ERROR;
    glCompileShader(vertexShader); CHECK_ERROR;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char info[1024];

        glGetShaderInfoLog(vertexShader, 1024, nullptr, info);
        printf("vertexShader: %s\n", info);
        exit(-1);
    }

    const char* fragmentSource2[] = {
            "#version 410 core\n",
            "#define FRAGMENT_SHADER\n",
            commonSource,
            fragmentSource
    };

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); CHECK_ERROR;
    glShaderSource(fragmentShader, 4, fragmentSource2, nullptr); CHECK_ERROR;
    glCompileShader(fragmentShader); CHECK_ERROR;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char info[1024];

        glGetShaderInfoLog(fragmentShader, 1024, nullptr, info);
        printf("fragmentShader: %s\n", info);
        exit(-1);
    }

    GLuint program = glCreateProgram(); CHECK_ERROR;
    glAttachShader(program, vertexShader); CHECK_ERROR;
    glAttachShader(program, fragmentShader); CHECK_ERROR;
    if(geometryShader != 0) {
        glAttachShader(program, geometryShader); CHECK_ERROR;
    }
    glLinkProgram(program); CHECK_ERROR;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char info[1024];

        glGetProgramInfoLog(program, 1024, nullptr, info);
        printf("program: %s\n", info);
        exit(-1);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if(geometryShader != 0) {
        glDeleteShader(geometryShader);
    }

    programCount++;

    return {program};
}

Framebuffer Device::createFramebuffer() {
    GLuint id;

    glGenFramebuffers(1, &id); CHECK_ERROR;

    framebufferCount++;

    return {id};
}

Renderbuffer Device::createRenderbuffer(int width, int height) {
    GLuint id;

    glGenRenderbuffers(1, &id); CHECK_ERROR;
    glBindRenderbuffer(GL_RENDERBUFFER, id); CHECK_ERROR;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); CHECK_ERROR;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    renderbufferCount++;

    return {id};
}

//////////////////////////////////////////////////

void Device::destroyTexture(Texture2D texture) {
    if (texture.id == 0) return;

    glDeleteTextures(1, &texture.id);

    textureCount--;
}

void Device::destroyTexture(TextureCube texture) {
    if (texture.id == 0) return;

    glDeleteTextures(1, &texture.id);

    textureCount--;
}

void Device::destroyTexture(DepthStencilTexture texture) {
    if (texture.texture.id == 0) return;

    glDeleteTextures(1, &texture.id);

    textureCount--;
}

void Device::destroySampler(Sampler sampler) {
    if (sampler.id == 0) return;

    glDeleteSamplers(1, &sampler.id);

    samplerCount--;
}

void Device::destroyVertexBuffer(VertexBuffer vertexBuffer) {
    if (vertexBuffer.id == 0) return;

    glDeleteBuffers(1, &vertexBuffer.id);

    vertexBufferCount--;
}

void Device::destroyIndexBuffer(IndexBuffer indexBuffer) {
    if (indexBuffer.id == 0) return;

    glDeleteBuffers(1, &indexBuffer.id);

    indexBufferCount--;
}

void Device::destroyConstantBuffer(ConstantBuffer constantBuffer) {
    if (constantBuffer.id == 0) return;

    glDeleteBuffers(1, &constantBuffer.id);

    constantBufferCount--;
}

void Device::destroyVertexArray(VertexArray vertexArray) {
    if (vertexArray.id == 0) return;

    glDeleteVertexArrays(1, &vertexArray.id);

    vertexArrayCount--;
}

void Device::destroyProgram(Program program) {
    if (program.id == 0) return;

    glDeleteProgram(program.id);

    programCount--;
}

void Device::destroyRenderbuffer(Renderbuffer renderbuffer) {
    if (renderbuffer.id == 0) return;

    glDeleteRenderbuffers(1, &renderbuffer.id);

    renderbufferCount--;
}

void Device::destroyFramebuffer(Framebuffer framebuffer) {
    if (framebuffer.id == 0) return;

    glDeleteFramebuffers(1, &framebuffer.id);

    framebufferCount--;
}

//////////////////////////////////////////////////

void Device::bindTextureToFramebuffer(Framebuffer framebuffer, Texture2D texture, int index) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture.id, 0); CHECK_ERROR;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Device::bindDepthTextureToFramebuffer(Framebuffer framebuffer, DepthTexture texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0); CHECK_ERROR;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Device::bindDepthStencilTextureToFramebuffer(Framebuffer framebuffer, DepthStencilTexture texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0); CHECK_ERROR;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Device::bindRenderbufferToFramebuffer(Framebuffer framebuffer, Renderbuffer renderbuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.id); CHECK_ERROR;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Device::bindFramebuffer(Framebuffer framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
}

void Device::bindReadFramebuffer(Framebuffer framebuffer) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
}

void Device::bindDrawFramebuffer(Framebuffer framebuffer) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
}

void Device::setRenderTarget(Framebuffer framebuffer, int targets[], int count) {
    GLenum _targets[GL_COLOR_ATTACHMENT31 - GL_COLOR_ATTACHMENT0] = { };

    for(int i = 0; i < count; i++)
        _targets[i] = GL_COLOR_ATTACHMENT0 + targets[i];

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    glDrawBuffers(count, _targets); CHECK_ERROR;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

bool Device::isFramebufferComplete(Framebuffer framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);

    GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER); CHECK_ERROR;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void Device::bindVertexArray(VertexArray vertexArray) {
    glBindVertexArray(vertexArray.id); CHECK_ERROR;
}

void Device::bindProgram(Program program) {
    glUseProgram(program.id); CHECK_ERROR;
}

void Device::copyConstantBuffer(ConstantBuffer constantBuffer, const void* data, size_t size) {
    glBindBuffer(GL_UNIFORM_BUFFER, constantBuffer.id); CHECK_ERROR;
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data); CHECK_ERROR;
}

void Device::bindTexture(Texture2D texture, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit); CHECK_ERROR;
    glBindTexture(GL_TEXTURE_2D, texture.id); CHECK_ERROR;
}

void Device::bindTexture(TextureCube texture, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit); CHECK_ERROR;
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id); CHECK_ERROR;
}

//TODO remove this method
void Device::setValue(Program program, const char* name, float x, float y, float z) {
    int index = glGetUniformLocation(program.id, name);
    if (index != -1) {
        glUniform3f(index, x, y, z); CHECK_ERROR;
    }
}

void Device::bindSampler(Sampler sampler, int unit) {
    glBindSampler(unit, sampler.id); CHECK_ERROR;
}

void Device::drawTriangles(int offset, int count) {
    void* _offset = (void*) (offset * sizeof(uint16_t));

    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset); CHECK_ERROR;
}

void Device::drawTrianglesInstanced(int offset, int count, int instance) {
    void* _offset = (void*) (offset * sizeof(uint16_t));

    glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset, instance); CHECK_ERROR;
}

void Device::drawArrays(int type, int first, int count) {
    glDrawArrays(type, first, count); CHECK_ERROR;
}

void Device::drawArraysInstanced(int type, int first, int count, int instance) {
    glDrawArraysInstanced(type, first, count, instance); CHECK_ERROR;
}

void Device::updateVertexBuffer(VertexBuffer vertexBuffer, size_t offset, size_t size, const void* data) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id); CHECK_ERROR;
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data); CHECK_ERROR;
    glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;
}
