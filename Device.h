//
// Created by Marrony Neris on 11/7/15.
//

#ifndef DEVICE_H_H
#define DEVICE_H_H

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void check_error(const char* file, int line);

#define CHECK_ERROR check_error(__FILE__, __LINE__)

struct Sampler {
    GLuint id;
};

struct Texture2D {
    GLuint id;
};

struct TextureCube {
    GLuint id;
};

union DepthTexture {
    GLuint id;
    Texture2D texture;
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

struct Rect {
    float x;
    float y;
    float width;
    float height;
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
    Device();

    ~Device();

    VertexBuffer createDynamicVertexBuffer(size_t size, const void* data);

    VertexBuffer createStaticVertexBuffer(size_t size, const void* data);

    IndexBuffer createIndexBuffer(size_t size, const void* data);

    ConstantBuffer createConstantBuffer(size_t size);

    void setConstantBufferBindingPoint(Program program, const char* blockName, int bindingPoint);

    void setTextureBindingPoint(Program program, const char* name, int bindingPoint);

    VertexArray createVertexArray(const VertexDeclaration* vertexDeclarations, int size, IndexBuffer indexBuffer);

    Sampler createSampler(int minFilter, int magFilter, int mipFilter = GL_NONE);

    Texture2D createRGB16FTexture(int width, int height, const void* pixels);

    Texture2D createRGBA16FTexture(int width, int height, const void* pixels);

    Texture2D createRGB32FTexture(int width, int height, const void* pixels);

    Texture2D createRGBA32FTexture(int width, int height, const void* pixels);

    Texture2D createRGBATexture(int width, int height, const void* pixels);

    Texture2D createRGBTexture(int width, int height, const void* pixels);

    Texture2D createRGBAFTexture(int width, int height, const void* pixels);

    Texture2D createRGBFTexture(int width, int height, const void* pixels);

    Texture2D createRTexture(int width, int height, const void* pixels);

    Texture2D createRG32FTexture(int width, int height, const void* pixels);

    DepthTexture createDepth32FTexture(int width, int height);

    DepthStencilTexture createDepth24Stencil8Texture(int width, int height);

    SeparateProgram createVertexProgram(const char* commonSource, const char* source);

    SeparateProgram createFragmentProgram(const char* commonSource, const char* source);

    Program createProgram(const char* commonSource, const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr);

    Framebuffer createFramebuffer();

    Renderbuffer createRenderbuffer(int width, int height);

    //////////////////////////////////////////////////

    void destroyTexture(Texture2D texture);

    void destroyTexture(DepthStencilTexture texture);

    void destroySampler(Sampler sampler);

    void destroyVertexBuffer(VertexBuffer vertexBuffer);

    void destroyIndexBuffer(IndexBuffer indexBuffer);

    void destroyConstantBuffer(ConstantBuffer constantBuffer);

    void destroyVertexArray(VertexArray vertexArray);

    void destroyProgram(Program program);

    void destroyRenderbuffer(Renderbuffer renderbuffer);

    void destroyFramebuffer(Framebuffer framebuffer);

    //////////////////////////////////////////////////

    void bindTextureToFramebuffer(Framebuffer framebuffer, Texture2D texture, int index);

    void bindDepthTextureToFramebuffer(Framebuffer framebuffer, DepthTexture texture);

    void bindDepthStencilTextureToFramebuffer(Framebuffer framebuffer, DepthStencilTexture texture);

    void bindRenderbufferToFramebuffer(Framebuffer framebuffer, Renderbuffer renderbuffer);

    void bindFramebuffer(Framebuffer framebuffer);

    void bindReadFramebuffer(Framebuffer framebuffer);

    void bindDrawFramebuffer(Framebuffer framebuffer);

    void setRenderTarget(Framebuffer framebuffer, int targets[], int count);

    bool isFramebufferComplete(Framebuffer framebuffer);

    void bindVertexArray(VertexArray vertexArray);

    void bindProgram(Program program);

    void copyConstantBuffer(ConstantBuffer constantBuffer, const void* data, size_t size);

    void bindTexture(Texture2D texture, int unit);

    //TODO remove this method
    void setValue(Program program, const char* name, float x, float y, float z);

    void bindSampler(Sampler sampler, int unit);

    void drawTriangles(int offset, int count);

    void drawTrianglesInstanced(int offset, int count, int instance);

    void drawArrays(int type, int first, int count);

    void drawArraysInstanced(int type, int first, int count, int instance);

    void updateVertexBuffer(VertexBuffer vertexBuffer, size_t offset, size_t size, const void* data);
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
