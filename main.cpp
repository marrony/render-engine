#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

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

#define CHECK_ERROR check_error(__FILE__, __LINE__)

struct Vector2 {
    float x, y;
};

struct Vector3 {
    float x, y, z;
};

struct Vector4 {
    float x, y, z, w;
};

#include "Allocator.h"
#include "Device.h"
#include "Commands.h"
#include "RenderQueue.h"
#include "Text.h"
#include "Material.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Shaders.h"

Viewport viewport = {};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_UP /*&& action == GLFW_PRESS*/)
        viewport.height++;

    if (key == GLFW_KEY_DOWN /*&& action == GLFW_PRESS*/)
        viewport.height--;

    if (key == GLFW_KEY_RIGHT /*&& action == GLFW_PRESS*/)
        viewport.width++;

    if (key == GLFW_KEY_LEFT /*&& action == GLFW_PRESS*/)
        viewport.width--;
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    viewport.width = width;
    viewport.height = height;
}

int main(int argc, char* argv[]) {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);

    glfwMakeContextCurrent(window);

    glfwSwapInterval(0);

    if (gl3wInit()) {
        return -1;
    }

    HeapAllocator heapAllocator;

    Device device;

    ModelManager modelManager(heapAllocator, device);
    TextureManager textureManager(heapAllocator, device);
    TextManager textManager(heapAllocator, device);

    Font fontRegular = textManager.loadFont("./fonts/OpenSans-Regular.ttf", 48);
    Font fontItalic = textManager.loadFont("./fonts/OpenSans-Italic.ttf", 48);

    Program program = device.createProgram(vertexSource, fragmentSource, geometrySource);

    struct In_vertexData {
        Vector4 in_Rotation[3];
        Vector4 in_Color;
        Vector4 in_Offset_Scale;
    };

    In_vertexData in_vertexData0[4];
    In_vertexData in_vertexData1[1];

    Vector4 offsetScale[4] = {
            -.5, -.5, 0, 0.35, //offset/scale
            -.5, +.5, 0, 0.35,
            +.5, +.5, 0, 0.35,
            +.5, -.5, 0, 0.35,
    };

    for(int i = 0; i < 4; i++) {
        in_vertexData0[i].in_Rotation[0] = {1, 0, 0, 0};
        in_vertexData0[i].in_Rotation[1] = {0, 1, 0, 0};
        in_vertexData0[i].in_Rotation[2] = {0, 0, 1, 0};
        in_vertexData0[i].in_Color = {1, 1, 1, 1};
        in_vertexData0[i].in_Offset_Scale = offsetScale[i];
    }

    in_vertexData1[0].in_Rotation[0] = {1, 0, 0, 0};
    in_vertexData1[0].in_Rotation[1] = {0, 1, 0, 0};
    in_vertexData1[0].in_Rotation[2] = {0, 0, 1, 0};
    in_vertexData1[0].in_Color = {1, 1, 1, 1};
    in_vertexData1[0].in_Offset_Scale = {0, 0, 0, 0.35};

    Texture2D texture0 = textureManager.loadTexture("images/lion.tga");
    Texture2D texture1 = textureManager.loadTexture("images/lion_ddn.tga");

    MaterialBumpedDiffuse bumpedDiffuse;
    bumpedDiffuse.program = program;
    bumpedDiffuse.mainUnit = device.getUniformLocation(program, "in_MainTex");
    bumpedDiffuse.mainTex = texture0;
    bumpedDiffuse.mainSampler = textureManager.getLinear();
    bumpedDiffuse.bumpUnit = device.getUniformLocation(program, "in_BumpMap");
    bumpedDiffuse.bumpMap = texture1;
    bumpedDiffuse.bumpSampler = textureManager.getNearest();
    Material* material0 = Material::create(heapAllocator, &bumpedDiffuse);
    Material* material1 = Material::create(heapAllocator, &bumpedDiffuse);

    device.setConstantBufferBindingPoint(program, "in_InstanceData", 0);
    ConstantBuffer constantBuffer0 = device.createConstantBuffer(4 * sizeof(In_vertexData));
    ConstantBuffer constantBuffer1 = device.createConstantBuffer(1 * sizeof(In_vertexData));

    Model* model = modelManager.createSphere("sphere01", 1.0, 20);

    ModelInstance* modelInstance0 = modelManager.createModelInstance(model, 4, constantBuffer0, in_vertexData0, 4 * sizeof(In_vertexData));
    ModelInstance::setMaterial(modelInstance0, 0, material0);

    ModelInstance* modelInstance1 = modelManager.createModelInstance(model, 1, constantBuffer1, in_vertexData1, 1 * sizeof(In_vertexData));
    ModelInstance::setMaterial(modelInstance1, 0, material1);

    modelManager.destroyModel(model);

    viewport.x = 0;
    viewport.y = 0;
    glfwGetFramebufferSize(window, &viewport.width, &viewport.height);
    glViewport(0, 0, viewport.width, viewport.height);

    int wgbuffer = 1024;
    int hgbuffer = 768;
    Framebuffer gBuffer = device.createFramebuffer();
    Texture2D position = device.createRGB32FTexture(wgbuffer, hgbuffer, nullptr);
    Texture2D normal = device.createRGB32FTexture(wgbuffer, hgbuffer, nullptr);
    Texture2D albedo = device.createRGBATexture(wgbuffer, hgbuffer, nullptr);
    Renderbuffer depth = device.createRenderbuffer(wgbuffer, hgbuffer);

    device.bindTextureToFramebuffer(gBuffer, position, 0);
    device.bindTextureToFramebuffer(gBuffer, normal, 1);
    device.bindTextureToFramebuffer(gBuffer, albedo, 2);
    device.bindRenderbufferToFramebuffer(gBuffer, depth);

    if (!device.isFramebufferComplete(gBuffer))
        return -1;

    int targets[] = { 0, 1, 2 };
    device.setRenderTarget(gBuffer, targets, 3);

    Program quadProgram = device.createProgram(quadVertexSource, quadFragmentSource, quadGeometrySource);

    Viewport gBufferViewport = {0, 0, wgbuffer, hgbuffer};
    CommandBuffer* setGBuffer = CommandBuffer::create(heapAllocator, 7);
    BindFramebuffer::create(setGBuffer, gBuffer);
    SetViewport::create(setGBuffer, &gBufferViewport);
    ClearColor::create(setGBuffer, 0, 0.25, 0.25, 0.25, 1);
    ClearColor::create(setGBuffer, 1, 0.25, 0.25, 0.25, 1);
    ClearColor::create(setGBuffer, 2, 0.25, 0.25, 0.25, 1);
    ClearDepthStencil::create(setGBuffer, 1.0, 0x00);
    SetDepthTest::create(setGBuffer, true, GL_LEQUAL);

    device.setConstantBufferBindingPoint(quadProgram, "in_LightData", 0);

    Vector4 lightPos[3] = {};
    ConstantBuffer lightPosConstantBuffer = device.createConstantBuffer(sizeof(lightPos));

    Framebuffer nullFramebuffer = {0};
    CommandBuffer* drawQuad = CommandBuffer::create(heapAllocator, 11);
    BindFramebuffer::create(drawQuad, nullFramebuffer);
    SetViewport::create(drawQuad, &viewport);
    SetDepthTest::create(drawQuad, false, 0);
    BindProgram::create(drawQuad, quadProgram);
    BindTexture::create(drawQuad, quadProgram, position, device.getUniformLocation(quadProgram, "in_Position"));
    BindTexture::create(drawQuad, quadProgram, normal, device.getUniformLocation(quadProgram, "in_Normal"));
    BindTexture::create(drawQuad, quadProgram, albedo, device.getUniformLocation(quadProgram, "in_Albedo"));
    CopyConstantBuffer::create(drawQuad, lightPosConstantBuffer, lightPos, sizeof(lightPos));
    BindConstantBuffer::create(drawQuad, lightPosConstantBuffer, 0);

    RenderQueue renderQueue(device, heapAllocator);

    Model* quadModel = modelManager.createQuad("quad");

    ModelInstance::draw(modelInstance0, 2, renderQueue, setGBuffer);
    ModelInstance::draw(modelInstance1, 1, renderQueue, setGBuffer);
    Model::draw(quadModel, 3, renderQueue, drawQuad);

    CommandBuffer* commandBuffer = renderQueue.sendToCommandBuffer();

    glEnable(GL_CULL_FACE); CHECK_ERROR;
    glCullFace(GL_FRONT); CHECK_ERROR;

    double current = glfwGetTime();
    double inc = 0;
    int fps = 0;
    int fps2 = 0;

    float angle = 0;
    while (!glfwWindowShouldClose(window)) {
        double c = glfwGetTime();
        double d = c - current;
        inc += d;
        current = c;
        fps++;

        if(inc > 1) {
            fps2 = fps;
            fps = 0;
            inc = 0;
        }

        float cos = cosf(angle);
        float sin = sinf(angle);

        lightPos[0].x = sin * 2;
        lightPos[0].y = 0;
        lightPos[0].z = -1;

        lightPos[1].x = 0;
        lightPos[1].y = sin * 2;
        lightPos[1].z = -1;

        lightPos[2].x = cos * 2;
        lightPos[2].y = sin * 2;
        lightPos[2].z = -1;

        angle += 0.005;

        std::function<void(Vector4[])> rotateX = [cos, sin](Vector4 matrix[]) {
            matrix[1].y = cos;
            matrix[1].z = -sin;
            matrix[2].y = sin;
            matrix[2].z = cos;
        };

        std::function<void(Vector4[])> rotateY = [cos, sin](Vector4 matrix[]) {
            matrix[0].x = cos;
            matrix[0].z = sin;
            matrix[2].x = -sin;
            matrix[2].z = cos;
        };

        std::function<void(Vector4[])> rotateZ = [cos, sin](Vector4 matrix[]) {
            matrix[0].x = cos;
            matrix[0].y = -sin;
            matrix[1].x = sin;
            matrix[1].y = cos;
        };

        rotateX(in_vertexData0[0].in_Rotation);
        rotateY(in_vertexData0[1].in_Rotation);
        rotateZ(in_vertexData0[2].in_Rotation);
        rotateX(in_vertexData0[3].in_Rotation);

        rotateX(in_vertexData1[0].in_Rotation);

//        in_vertexData0.in_Color[0].x -= 0.00001;
//        in_vertexData0.in_Color[0].y -= 0.00002;
//        in_vertexData0.in_Color[0].z -= 0.00003;

        CommandBuffer::execute(commandBuffer, device);

        textManager.printText(fontItalic, 10, 230, "Fps: %d Angle: %f", fps2, angle);
        textManager.printText(fontRegular, 10, 180, "viewport: %d %d %d %d", viewport.x, viewport.y, viewport.width, viewport.height);
        textManager.printText(fontRegular, 10, 130, "Memory used %ld bytes", heapAllocator.memoryUsed());
        float totalCommands = renderQueue.getExecutedCommands() + renderQueue.getSkippedCommands();
        textManager.printText(fontRegular, 10, 80, "Executed commands %d | %.2f%% executed",
                              renderQueue.getExecutedCommands(), renderQueue.getExecutedCommands() / totalCommands * 100);
        textManager.printText(fontRegular, 10, 30, "Skipped commands %d | %.2f%% ignored",
                              renderQueue.getSkippedCommands(), renderQueue.getSkippedCommands() / totalCommands * 100);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Material::destroy(heapAllocator, material0);
    Material::destroy(heapAllocator, material1);
    CommandBuffer::destroy(heapAllocator, setGBuffer);
    CommandBuffer::destroy(heapAllocator, drawQuad);
    CommandBuffer::destroy(heapAllocator, commandBuffer);

    modelManager.destroyModel(quadModel);
    modelManager.destroyModelInstance(modelInstance0);
    modelManager.destroyModelInstance(modelInstance1);
    textureManager.unloadTexture(texture0);
    textureManager.unloadTexture(texture1);

    device.destroyTexture(position);
    device.destroyTexture(normal);
    device.destroyTexture(albedo);
    device.destoryRenderbuffer(depth);
    device.destroyFramebuffer(gBuffer);
    device.destroyConstantBuffer(lightPosConstantBuffer);
    device.destroyConstantBuffer(constantBuffer0);
    device.destroyConstantBuffer(constantBuffer1);
    device.destroyProgram(program);
    device.destroyProgram(quadProgram);

    heapAllocator.dumpFreeList();

    glfwDestroyWindow(window);
    glfwTerminate();
}
