//
// Created by Marrony Neris on 11/7/15.
//

#ifndef DEVICE_H_H
#define DEVICE_H_H

struct Sampler {
    GLuint sampler;
};

struct Texture2D {
    GLuint texId;
};

struct VertexBuffer {
    GLuint vbo;
};

struct IndexBuffer {
    GLuint ibo;
};

struct VertexArray {
    GLuint vao;
};

struct Program {
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
    VertexBuffer createVertexBuffer(size_t size, const void* data) {
        GLuint vbo;

        glGenBuffers(1, &vbo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        check_error(__FILE__, __LINE__);

        return { vbo };
    }

    IndexBuffer createIndexBuffer(size_t size, const void* data) {
        GLuint ibo;

        glGenBuffers(1, &ibo);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        check_error(__FILE__, __LINE__);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        check_error(__FILE__, __LINE__);

        return { ibo };
    }

    VertexArray createVertexArray(const VertexDeclarationDesc* vertexDeclarationDesc, int size, IndexBuffer indexBuffer) {
        GLuint vao;

        glGenVertexArrays(1, &vao);
        check_error(__FILE__, __LINE__);

        glBindVertexArray(vao);
        check_error(__FILE__, __LINE__);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.ibo);
        check_error(__FILE__, __LINE__);

        for(int i = 0; i < size; i++) {
            uint32_t* format = vertexDeclarationDesc[i].format;
            uint32_t stride = vertexDeclarationDesc[i].stride;
            void* offset = vertexDeclarationDesc[i].offset;
            VertexBuffer vertexBuffer = vertexDeclarationDesc[i].buffer;

            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.vbo);
            check_error(__FILE__, __LINE__);

            glVertexAttribPointer(i, format[0], format[1], GL_FALSE, stride, offset);
            check_error(__FILE__, __LINE__);
            glEnableVertexAttribArray(i);
            check_error(__FILE__, __LINE__);
        }

        glBindVertexArray(0);

        return { vao };
    }

    Sampler createSampler(int type) {
        GLuint sampler;

        glGenSamplers(1, &sampler);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, type);
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, type);

        return { sampler };
    }

    Texture2D createTexture(int width, int height, void* pixels) {
        GLuint texId;
        glGenTextures(1, &texId);

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GLfloat color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
        glBindTexture(GL_TEXTURE_2D, 0);

        return { texId };
    }

    Program createProgram(const char* vertexSource, const char* fragmentSource) {
        GLint status;

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
        if(!status) {
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
        if(!status) {
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
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(!status) {
            char info[1024];

            glGetProgramInfoLog(program, 1024, nullptr, info);
            printf("program: %s\n", info);
            exit(-1);
        }

        return { program };
    }

    void bindVertexArray(VertexArray vertexArray) {
        glBindVertexArray(vertexArray.vao);
        check_error(__FILE__, __LINE__);
    }

    void bindProgram(Program program) {
        glUseProgram(program.id);
        check_error(__FILE__, __LINE__);
    }

    void bindTexture(Program program, Texture2D texture, const char* name, int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);

        int index = glGetUniformLocation(program.id, name);
        if(index != -1) {
            glUniform1i(index, unit);
            check_error(__FILE__, __LINE__);

            glBindTexture(GL_TEXTURE_2D, texture.texId);
            check_error(__FILE__, __LINE__);
        }
    }

    void setMatrix(Program program, const char* name, float rotation) {
        float matrix[9] = {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1,
        };

        int index = glGetUniformLocation(program.id, name);
        if(index != -1) {
            glUniformMatrix3fv(index, 1, 0, matrix);
            check_error(__FILE__, __LINE__);
        }
    }

    void setValue(Program program, const char* name, float x, float y, float z) {
        int index = glGetUniformLocation(program.id, name);
        if(index != -1) {
            glUniform3f(index, x, y, z);
            check_error(__FILE__, __LINE__);
        }
    }

    void setValue(Program program, const char* name, float* value) {
        int index = glGetUniformLocation(program.id, name);
        if(index != -1) {
            glUniform4fv(index, 1, value);
            check_error(__FILE__, __LINE__);
        }
    }

    void bindSampler(Sampler sampler, int unit) {
        glBindSampler(unit, sampler.sampler);
        check_error(__FILE__, __LINE__);
    }

    void drawTriangles(int offset, int count) {
        void* _offset = (void*)(offset*sizeof(uint16_t));

        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, _offset);
        check_error(__FILE__, __LINE__);
    }
};

#endif //DEVICE_H_H
