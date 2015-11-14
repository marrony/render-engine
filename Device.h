//
// Created by Marrony Neris on 11/7/15.
//

#ifndef DEVICE_H_H
#define DEVICE_H_H

struct Sampler {
    GLuint sampler;
};

struct Texture2D {
    GLuint id;
};

struct DepthStencilTexture {
    GLuint id;
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

struct Framebuffer {
    GLuint id;
};

struct Renderbuffer {
    GLuint id;
};

uint32_t VertexFloat2[] = {2, GL_FLOAT};
uint32_t VertexFloat3[] = {3, GL_FLOAT};
uint32_t VertexFloat4[] = {4, GL_FLOAT};

struct VertexDeclarationDesc {
    uint32_t* format;
    uint32_t stride;
    void* offset;
    VertexBuffer buffer;
};

class Device {
public:
    VertexBuffer createDynamicVertexBuffer(size_t size, const void* data) {
        GLuint vbo;

        glGenBuffers(1, &vbo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check_error(__FILE__, __LINE__);

        return {vbo};
    }

    VertexBuffer createStaticVertexBuffer(size_t size, const void* data) {
        GLuint vbo;

        glGenBuffers(1, &vbo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check_error(__FILE__, __LINE__);

        return {vbo};
    }

    IndexBuffer createIndexBuffer(size_t size, const void* data) {
        GLuint ibo;

        glGenBuffers(1, &ibo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        check_error(__FILE__, __LINE__);

        return {ibo};
    }

    ConstantBuffer createConstantBuffer(size_t size, const void* data, int bindingPoint) {
        GLuint cbo;

        glGenBuffers(1, &cbo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_UNIFORM_BUFFER, cbo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
        check_error(__FILE__, __LINE__);

        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, cbo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        check_error(__FILE__, __LINE__);

        return {cbo};
    }

    void setConstantBufferBinding(Program program, const char* blockName, int bindingPoint) {
        int index = glGetUniformBlockIndex(program.id, blockName);
        glUniformBlockBinding(program.id, index, bindingPoint);
    }

    VertexArray createVertexArray(const VertexDeclarationDesc* vertexDeclarationDesc, int size,
                                  IndexBuffer indexBuffer) {
        GLuint vao;

        glGenVertexArrays(1, &vao);
        check_error(__FILE__, __LINE__);

        glBindVertexArray(vao);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
        check_error(__FILE__, __LINE__);

        for (int i = 0; i < size; i++) {
            uint32_t* format = vertexDeclarationDesc[i].format;
            uint32_t stride = vertexDeclarationDesc[i].stride;
            void* offset = vertexDeclarationDesc[i].offset;
            VertexBuffer vertexBuffer = vertexDeclarationDesc[i].buffer;

            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id);
            check_error(__FILE__, __LINE__);

            glVertexAttribPointer(i, format[0], format[1], GL_FALSE, stride, offset);
            check_error(__FILE__, __LINE__);
            glEnableVertexAttribArray(i);
            check_error(__FILE__, __LINE__);
        }

        glBindVertexArray(0);

        return {vao};
    }

    Sampler createSampler(int type) {
        GLuint sampler;

        glGenSamplers(1, &sampler);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, type);
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, type);

        return {sampler};
    }

    Texture2D createFloat16Texture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId);

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        return {texId};
    }

    Texture2D createFloat32Texture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId);

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        return {texId};
    }

    Texture2D createTexture(int width, int height, const void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId);

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GLfloat color[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
        glBindTexture(GL_TEXTURE_2D, 0);

        return {texId};
    }

    DepthStencilTexture createDepthStencilTexture(int width, int height) {
        GLuint texId;
        glGenTextures(1, &texId);

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        return {texId};
    }

    Program createProgram(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr) {
        GLint status;

        const char* geometrySource2[] = {
                "#version 410 core\n", geometrySource
        };

        GLuint geometryShader = 0;

        if(geometrySource != nullptr) {
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
            check_error(__FILE__, __LINE__);
            glShaderSource(geometryShader, 2, geometrySource2, nullptr);
            check_error(__FILE__, __LINE__);
            glCompileShader(geometryShader);
            check_error(__FILE__, __LINE__);
            glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &status);
            if (!status) {
                char info[1024];

                glGetShaderInfoLog(geometryShader, 1024, nullptr, info);
                printf("vertexShader: %s\n", info);
                exit(-1);
            }
        }

        const char* vertexSource2[] = {
                "#version 410 core\n", vertexSource
        };

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        check_error(__FILE__, __LINE__);
        glShaderSource(vertexShader, 2, vertexSource2, nullptr);
        check_error(__FILE__, __LINE__);
        glCompileShader(vertexShader);
        check_error(__FILE__, __LINE__);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char info[1024];

            glGetShaderInfoLog(vertexShader, 1024, nullptr, info);
            printf("vertexShader: %s\n", info);
            exit(-1);
        }

        const char* fragmentSource2[] = {
                "#version 410 core\n", fragmentSource
        };

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        check_error(__FILE__, __LINE__);
        glShaderSource(fragmentShader, 2, fragmentSource2, nullptr);
        check_error(__FILE__, __LINE__);
        glCompileShader(fragmentShader);
        check_error(__FILE__, __LINE__);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char info[1024];

            glGetShaderInfoLog(fragmentShader, 1024, nullptr, info);
            printf("vertexShader: %s\n", info);
            exit(-1);
        }

        GLuint program = glCreateProgram();
        check_error(__FILE__, __LINE__);
        glAttachShader(program, vertexShader);
        check_error(__FILE__, __LINE__);
        glAttachShader(program, fragmentShader);
        check_error(__FILE__, __LINE__);
        if(geometryShader != 0) {
            glAttachShader(program, geometryShader);
            check_error(__FILE__, __LINE__);
        }
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char info[1024];

            glGetProgramInfoLog(program, 1024, nullptr, info);
            printf("program: %s\n", info);
            exit(-1);
        }

        return {program};
    }

    Framebuffer createFramebuffer() {
        GLuint id;

        glGenFramebuffers(1, &id);
        //glBindFramebuffer(GL_FRAMEBUFFER, id);

        return {id};
    }

    void bindTextureToFramebuffer(Framebuffer framebuffer, Texture2D texture, int index) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture.id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindDepthStencilTextureToFramebuffer(Framebuffer framebuffer, DepthStencilTexture texture) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Renderbuffer createRenderbuffer(int width, int height) {
        GLuint id;

        glGenRenderbuffers(1, &id);
        glBindRenderbuffer(GL_RENDERBUFFER, id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        return {id};
    }

    void bindRenderbufferToFramebuffer(Framebuffer framebuffer, Renderbuffer renderbuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.id);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
        check_error(__FILE__, __LINE__);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id);
        check_error(__FILE__, __LINE__);
    }

    void bindReadFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
        check_error(__FILE__, __LINE__);
    }

    void bindDrawFramebuffer(Framebuffer framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id);
        check_error(__FILE__, __LINE__);
    }

    void setRenderTarget(Framebuffer framebuffer, int targets[], int count) {
        GLenum _targets[GL_COLOR_ATTACHMENT31 - GL_COLOR_ATTACHMENT0] = { };

        for(int i = 0; i < count; i++)
            _targets[i] = GL_COLOR_ATTACHMENT0 + targets[i];

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id);
        glDrawBuffers(count, _targets);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }


    bool isFramebufferComplete(Framebuffer framebuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);

        GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    void bindVertexArray(VertexArray vertexArray) {
        glBindVertexArray(vertexArray.id);
        check_error(__FILE__, __LINE__);
    }

    void bindProgram(Program program) {
        glUseProgram(program.id);
        check_error(__FILE__, __LINE__);
    }

    int getUniformLocation(Program program, const char* name) {
        return glGetUniformLocation(program.id, name);
    }

    void copyConstantBuffer(ConstantBuffer constantBuffer, const void* data, size_t size) {
        glBindBuffer(GL_UNIFORM_BUFFER, constantBuffer.id);
        check_error(__FILE__, __LINE__);

        void* buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
        check_error(__FILE__, __LINE__);

        memcpy(buffer, data, size);

        glUnmapBuffer(GL_UNIFORM_BUFFER);
        check_error(__FILE__, __LINE__);
    }

    void bindTexture(Program program, Texture2D texture, int index) {
        glActiveTexture(GL_TEXTURE0 + index);
        glUniform1i(index, index);
        check_error(__FILE__, __LINE__);

        glBindTexture(GL_TEXTURE_2D, texture.id);
        check_error(__FILE__, __LINE__);
    }

    void setValue(Program program, const char* name, float x, float y, float z) {
        int index = glGetUniformLocation(program.id, name);
        if (index != -1) {
            glUniform3f(index, x, y, z);
            check_error(__FILE__, __LINE__);
        }
    }

    void bindSampler(Sampler sampler, int unit) {
        glBindSampler(unit, sampler.sampler);
        check_error(__FILE__, __LINE__);
    }

    void drawTriangles(int offset, int count) {
        void* _offset = (void*) (offset * sizeof(uint16_t));

        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset);
        check_error(__FILE__, __LINE__);
    }

    void drawTrianglesInstanced(int offset, int count, int instance) {
        void* _offset = (void*) (offset * sizeof(uint16_t));

        glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset, instance);
        check_error(__FILE__, __LINE__);
    }
};

#endif //DEVICE_H_H
