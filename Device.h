//
// Created by Marrony Neris on 11/7/15.
//

#ifndef DEVICE_H_H
#define DEVICE_H_H

struct Sampler {
    GLuint id;
};

struct Texture2D {
    GLuint id;
};

struct TextureCube {
    GLuint id;
};

union DepthStencilTexture {
    GLuint id;
    Texture2D texture;
};

struct VertexBuffer {
    GLuint id;
};

struct IndexBuffer {
    GLuint id;
};

struct ConstantBuffer {
    GLuint id;
};

struct VertexArray {
    GLuint id;
};

struct Program {
    GLuint id;
};

struct SeparateProgram {
    GLuint id;
};

struct Framebuffer {
    GLuint id;
};

struct Renderbuffer {
    GLuint id;
};

struct Viewport {
    int x;
    int y;
    int width;
    int height;
};

struct VertexFormat {
    uint32_t size : 16;
    uint32_t type : 16;
};

const VertexFormat VertexFloat1 = {1, GL_FLOAT};
const VertexFormat VertexFloat2 = {2, GL_FLOAT};
const VertexFormat VertexFloat3 = {3, GL_FLOAT};
const VertexFormat VertexFloat4 = {4, GL_FLOAT};

struct VertexDeclaration {
    VertexFormat format;
    uint32_t stride;
    void* offset;
    VertexBuffer buffer;
};

class Device {
public:
    Device() {
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

    ~Device() {
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

    VertexBuffer createDynamicVertexBuffer(size_t size, const void* data) {
        GLuint vbo;

        glGenBuffers(1, &vbo); CHECK_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_ERROR;
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW); CHECK_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;

        vertexBufferCount++;

        return {vbo};
    }

    VertexBuffer createStaticVertexBuffer(size_t size, const void* data) {
        GLuint vbo;

        glGenBuffers(1, &vbo); CHECK_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_ERROR;
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); CHECK_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;

        vertexBufferCount++;

        return {vbo};
    }

    IndexBuffer createIndexBuffer(size_t size, const void* data) {
        GLuint ibo;

        glGenBuffers(1, &ibo); CHECK_ERROR;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); CHECK_ERROR;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); CHECK_ERROR;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_ERROR;

        indexBufferCount++;

        return {ibo};
    }

    ConstantBuffer createConstantBuffer(size_t size) {
        GLuint cbo;

        glGenBuffers(1, &cbo); CHECK_ERROR;

        glBindBuffer(GL_UNIFORM_BUFFER, cbo); CHECK_ERROR;
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW); CHECK_ERROR;

        glBindBuffer(GL_UNIFORM_BUFFER, 0); CHECK_ERROR;

        constantBufferCount++;

        return {cbo};
    }

    void setConstantBufferBindingPoint(Program program, const char* blockName, int bindingPoint) {
        int index = glGetUniformBlockIndex(program.id, blockName);
        glUniformBlockBinding(program.id, index, bindingPoint); CHECK_ERROR;
    }

    VertexArray createVertexArray(const VertexDeclaration* vertexDeclarations, int size,
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

            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id); CHECK_ERROR;

            glVertexAttribPointer(i, format.size, format.type, GL_FALSE, stride, offset); CHECK_ERROR;
            glEnableVertexAttribArray(i); CHECK_ERROR;
        }

        glBindVertexArray(0);

        vertexArrayCount++;

        return {vao};
    }

    Sampler createSampler(int minFilter, int magFilter, int mipFilter = GL_NONE) {
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

    Texture2D createRGB16FTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRGBA16FTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRGB32FTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR; CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRGBA32FTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR; CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRGBATexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRGBTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    Texture2D createRTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels); CHECK_ERROR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    DepthStencilTexture createDepthStencilTexture(int width, int height) {
        GLuint texId;
        glGenTextures(1, &texId); CHECK_ERROR;

        glBindTexture(GL_TEXTURE_2D, texId); CHECK_ERROR;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr); CHECK_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0);

        textureCount++;

        return {texId};
    }

    SeparateProgram createVertexProgram(const char* commonSource, const char* source) {
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

    SeparateProgram createFragmentProgram(const char* commonSource, const char* source) {
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

    Program createProgram(const char* commonSource, const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr) {
        GLint status;

        const char* geometrySource2[] = {
                "#version 410 core\n",
                commonSource,
                geometrySource
        };

        GLuint geometryShader = 0;

        if(geometrySource != nullptr) {
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER); CHECK_ERROR;
            glShaderSource(geometryShader, 3, geometrySource2, nullptr); CHECK_ERROR;
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
                commonSource,
                vertexSource
        };

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); CHECK_ERROR;
        glShaderSource(vertexShader, 3, vertexSource2, nullptr); CHECK_ERROR;
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
                commonSource,
                fragmentSource
        };

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); CHECK_ERROR;
        glShaderSource(fragmentShader, 3, fragmentSource2, nullptr); CHECK_ERROR;
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

    Framebuffer createFramebuffer() {
        GLuint id;

        glGenFramebuffers(1, &id); CHECK_ERROR;

        framebufferCount++;

        return {id};
    }

    Renderbuffer createRenderbuffer(int width, int height) {
        GLuint id;

        glGenRenderbuffers(1, &id); CHECK_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, id); CHECK_ERROR;
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); CHECK_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        renderbufferCount++;

        return {id};
    }

    //////////////////////////////////////////////////

    void destroyTexture(Texture2D texture) {
        if (texture.id == 0) return;

        glDeleteTextures(1, &texture.id);

        textureCount--;
    }

    void destroyTexture(DepthStencilTexture texture) {
        if (texture.texture.id == 0) return;

        glDeleteTextures(1, &texture.id);

        textureCount--;
    }

    void destroySampler(Sampler sampler) {
        if (sampler.id == 0) return;

        glDeleteSamplers(1, &sampler.id);

        samplerCount--;
    }

    void destroyVertexBuffer(VertexBuffer vertexBuffer) {
        if (vertexBuffer.id == 0) return;

        glDeleteBuffers(1, &vertexBuffer.id);

        vertexBufferCount--;
    }

    void destroyIndexBuffer(IndexBuffer indexBuffer) {
        if (indexBuffer.id == 0) return;

        glDeleteBuffers(1, &indexBuffer.id);

        indexBufferCount--;
    }

    void destroyConstantBuffer(ConstantBuffer constantBuffer) {
        if (constantBuffer.id == 0) return;

        glDeleteBuffers(1, &constantBuffer.id);

        constantBufferCount--;
    }

    void destroyVertexArray(VertexArray vertexArray) {
        if (vertexArray.id == 0) return;

        glDeleteVertexArrays(1, &vertexArray.id);

        vertexArrayCount--;
    }

    void destroyProgram(Program program) {
        if (program.id == 0) return;

        glDeleteProgram(program.id);

        programCount--;
    }

    void destroyRenderbuffer(Renderbuffer renderbuffer) {
        if (renderbuffer.id == 0) return;

        glDeleteRenderbuffers(1, &renderbuffer.id);

        renderbufferCount--;
    }

    void destroyFramebuffer(Framebuffer framebuffer) {
        if (framebuffer.id == 0) return;

        glDeleteFramebuffers(1, &framebuffer.id);

        framebufferCount--;
    }

    //////////////////////////////////////////////////

    void bindTextureToFramebuffer(Framebuffer framebuffer, Texture2D texture, int index) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture.id, 0); CHECK_ERROR;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindDepthStencilTextureToFramebuffer(Framebuffer framebuffer, DepthStencilTexture texture) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0); CHECK_ERROR;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindRenderbufferToFramebuffer(Framebuffer framebuffer, Renderbuffer renderbuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.id); CHECK_ERROR;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    }

    void bindReadFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    }

    void bindDrawFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
    }

    void setRenderTarget(Framebuffer framebuffer, int targets[], int count) {
        GLenum _targets[GL_COLOR_ATTACHMENT31 - GL_COLOR_ATTACHMENT0] = { };

        for(int i = 0; i < count; i++)
            _targets[i] = GL_COLOR_ATTACHMENT0 + targets[i];

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id); CHECK_ERROR;
        glDrawBuffers(count, _targets); CHECK_ERROR;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    bool isFramebufferComplete(Framebuffer framebuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);

        GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER); CHECK_ERROR;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    void bindVertexArray(VertexArray vertexArray) {
        glBindVertexArray(vertexArray.id); CHECK_ERROR;
    }

    void bindProgram(Program program) {
        glUseProgram(program.id); CHECK_ERROR;
    }

    int getUniformLocation(Program program, const char* name) {
        return glGetUniformLocation(program.id, name);
    }

    void copyConstantBuffer(ConstantBuffer constantBuffer, const void* data, size_t size) {
        glBindBuffer(GL_UNIFORM_BUFFER, constantBuffer.id); CHECK_ERROR;

        void* buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY); CHECK_ERROR;

        memcpy(buffer, data, size);

        glUnmapBuffer(GL_UNIFORM_BUFFER); CHECK_ERROR;
    }

    void bindTexture(Program program, Texture2D texture, int unit) {
        glActiveTexture(GL_TEXTURE0 + unit); CHECK_ERROR;
        glUniform1i(unit, unit); CHECK_ERROR;
        glBindTexture(GL_TEXTURE_2D, texture.id); CHECK_ERROR;
    }

    //TODO remove this method
    void setValue(Program program, const char* name, float x, float y, float z) {
        int index = glGetUniformLocation(program.id, name);
        if (index != -1) {
            glUniform3f(index, x, y, z); CHECK_ERROR;
        }
    }

    void bindSampler(Sampler sampler, int unit) {
        glBindSampler(unit, sampler.id); CHECK_ERROR;
    }

    void drawTriangles(int offset, int count) {
        void* _offset = (void*) (offset * sizeof(uint16_t));

        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset); CHECK_ERROR;
    }

    void drawTrianglesInstanced(int offset, int count, int instance) {
        void* _offset = (void*) (offset * sizeof(uint16_t));

        glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset, instance); CHECK_ERROR;
    }

    void drawArraysTriangleStrip(int first, int count) {
        glDrawArrays(GL_TRIANGLE_STRIP, first, count); CHECK_ERROR;
    }

    void updateVertexBuffer(VertexBuffer vertexBuffer, size_t offset, size_t size, const void* data) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id); CHECK_ERROR;
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data); CHECK_ERROR;
        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_ERROR;
    }

private:
    uint32_t vertexBufferCount;
    uint32_t indexBufferCount;
    uint32_t constantBufferCount;
    uint32_t vertexArrayCount;
    uint32_t textureCount;
    uint32_t samplerCount;
    uint32_t programCount;
    uint32_t vertexProgramCount;
    uint32_t fragmentProgramCount;
    uint32_t framebufferCount;
    uint32_t renderbufferCount;
};

#endif //DEVICE_H_H
