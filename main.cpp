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

#include "Vector.h"
#include "Matrix.h"
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

    //SeparateProgram vertexProgram = device.createVertexProgram(commonSource, vertexSource);
    //SeparateProgram fragmentProgram = device.createFragmentProgram(commonSource, fragmentSource);

    Program programOpaque = device.createProgram(commonSource, vertexSource, fragmentSource, nullptr);
    Program programTransparent = device.createProgram(commonSource, vertexTransparencySource, fragmentTransparencySource, nullptr);

    struct In_VertexData {
        float in_Rotation[16];
        Vector4 in_Color;
    };

    struct In_FrameData {
        Matrix4 projection;
        Matrix4 view;
    };

    In_FrameData in_frameData;
    In_VertexData in_vertexData0[4];
    In_VertexData in_vertexData1[2];
    In_VertexData in_vertexData2[1];

    uint32_t pixel = 0x00ff0000;
    uint32_t pixels[] = {pixel, pixel, pixel, pixel};

    Texture2D texture0 = textureManager.loadTexture("images/lion.tga");
    Texture2D texture1 = textureManager.loadTexture("images/lion_ddn.tga");
    Texture2D texture2 = textureManager.loadTexture("images/stained_glass.tga");
    Texture2D texture3 = device.createRGBATexture(2, 2, pixels);

    MaterialBumpedDiffuse bumpedDiffuse;
    bumpedDiffuse.program = programOpaque;
    bumpedDiffuse.mainUnit = device.getUniformLocation(programOpaque, "in_MainTex");
    bumpedDiffuse.mainTex = texture0;
    bumpedDiffuse.mainSampler = textureManager.getLinear();
    bumpedDiffuse.bumpUnit = device.getUniformLocation(programOpaque, "in_BumpMap");
    bumpedDiffuse.bumpMap = texture1;
    bumpedDiffuse.bumpSampler = textureManager.getNearest();
    Material* material0 = Material::create(heapAllocator, &bumpedDiffuse);

    MaterialTransparency transparency;
    transparency.program = programTransparent;
    transparency.mainUnit = device.getUniformLocation(programTransparent, "in_MainTex");
    transparency.mainTex = texture2;
    transparency.mainSampler = textureManager.getLinear();
    transparency.bumpUnit = device.getUniformLocation(programTransparent, "in_BumpMap");
    transparency.bumpMap = texture1;
    transparency.bumpSampler = textureManager.getNearest();
    transparency.alpha = 0.5;
    //transparency.bindPoint = 2;
    Material* material1 = Material::create(heapAllocator, &transparency);

    MaterialBumpedDiffuse backgroundMaterial;
    backgroundMaterial.program = programOpaque;
    backgroundMaterial.mainUnit = device.getUniformLocation(programOpaque, "in_MainTex");
    backgroundMaterial.mainTex = texture2;
    backgroundMaterial.mainSampler = textureManager.getLinear();
    backgroundMaterial.bumpUnit = device.getUniformLocation(programOpaque, "in_BumpMap");
    backgroundMaterial.bumpMap = texture3;
    backgroundMaterial.bumpSampler = textureManager.getNearest();
    Material* material2 = Material::create(heapAllocator, &backgroundMaterial);

    int BINDING_POINT_INSTANCE_DATA = 0;
    int BINDING_POINT_FRAME_DATA = 1;
    int BINDING_POINT_LIGHT_DATA = 2;

    device.setConstantBufferBindingPoint(programOpaque, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(programOpaque, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(programTransparent, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(programTransparent, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(programTransparent, "in_LightData", BINDING_POINT_LIGHT_DATA);

    ConstantBuffer constantBuffer0 = device.createConstantBuffer(4 * sizeof(In_VertexData));
    ConstantBuffer constantBuffer1 = device.createConstantBuffer(2 * sizeof(In_VertexData));
    ConstantBuffer constantBuffer2 = device.createConstantBuffer(1 * sizeof(In_VertexData));
    ConstantBuffer frameDataBuffer = device.createConstantBuffer(sizeof(In_FrameData));

    Model* model = modelManager.createSphere("sphere01", 1.0, 20);

    ModelInstance* modelInstance0 = modelManager.createModelInstance(model, 4, constantBuffer0, in_vertexData0, 4 * sizeof(In_VertexData));
    ModelInstance::setMaterial(modelInstance0, 0, material0);

    ModelInstance* modelInstance1 = modelManager.createModelInstance(model, 2, constantBuffer1, in_vertexData1, 2 * sizeof(In_VertexData));
    ModelInstance::setMaterial(modelInstance1, 0, material1);

    Model* quadModel = modelManager.createQuad("quad");

    ModelInstance* modelInstance2 = modelManager.createModelInstance(quadModel, 1, constantBuffer2, in_vertexData2, 1 * sizeof(In_VertexData));
    ModelInstance::setMaterial(modelInstance2, 0, material2);

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
    DepthStencilTexture depth = device.createDepthStencilTexture(wgbuffer, hgbuffer);

    device.bindTextureToFramebuffer(gBuffer, position, 0);
    device.bindTextureToFramebuffer(gBuffer, normal, 1);
    device.bindTextureToFramebuffer(gBuffer, albedo, 2);
    device.bindDepthStencilTextureToFramebuffer(gBuffer, depth);

    if (!device.isFramebufferComplete(gBuffer))
        return -1;

    int targets[] = { 0, 1, 2 };
    device.setRenderTarget(gBuffer, targets, 3);

    Framebuffer transparentBuffer = device.createFramebuffer();
    Texture2D quadTexture = device.createRGBA32FTexture(wgbuffer, hgbuffer, nullptr);
    device.bindTextureToFramebuffer(transparentBuffer, quadTexture, 0);
    device.bindDepthStencilTextureToFramebuffer(transparentBuffer, depth);

    if (!device.isFramebufferComplete(transparentBuffer))
        return -1;

    int quadTarget = 0;
    device.setRenderTarget(transparentBuffer, &quadTarget, 1);

    Program quadProgram = device.createProgram(commonSource, quadVertexSource, quadFragmentSource, quadGeometrySource);
    Program copyProgram = device.createProgram(commonSource, quadVertexSource, copyFragmentSource, quadGeometrySource);

    Viewport gBufferViewport = {0, 0, wgbuffer, hgbuffer};
    CommandBuffer* setGBuffer = CommandBuffer::create(heapAllocator, 10);
    BindFramebuffer::create(setGBuffer, gBuffer);
    SetViewport::create(setGBuffer, &gBufferViewport);
    ClearColor::create(setGBuffer, 0, 0.00, 0.00, 0.00, 1); //position
    ClearColor::create(setGBuffer, 1, 0.00, 0.00, 0.00, 1); //normal
    ClearColor::create(setGBuffer, 2, 0.50, 0.50, 0.50, 0); //albedo
    ClearDepthStencil::create(setGBuffer, 1.0, 0x00);
    SetDepthTest::create(setGBuffer, true, GL_LEQUAL);
    CopyConstantBuffer::create(setGBuffer, frameDataBuffer, &in_frameData, sizeof(In_FrameData));
    BindConstantBuffer::create(setGBuffer, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    device.setConstantBufferBindingPoint(quadProgram, "in_LightData", BINDING_POINT_LIGHT_DATA);

    struct LightData {
        Vector4 position;
        Vector4 color;
    };

    LightData lightData[3] = {};
    ConstantBuffer lightPosConstantBuffer = device.createConstantBuffer(3 * sizeof(LightData));

    Framebuffer nullFramebuffer = {0};
    CommandBuffer* drawQuad = CommandBuffer::create(heapAllocator, 20);
    BindFramebuffer::create(drawQuad, transparentBuffer);
    SetViewport::create(drawQuad, &gBufferViewport);
    SetDepthTest::create(drawQuad, false, GL_NONE);
    BindProgram::create(drawQuad, quadProgram);
    BindTexture::create(drawQuad, quadProgram, position, device.getUniformLocation(quadProgram, "in_Position"));
    BindTexture::create(drawQuad, quadProgram, normal, device.getUniformLocation(quadProgram, "in_Normal"));
    BindTexture::create(drawQuad, quadProgram, albedo, device.getUniformLocation(quadProgram, "in_Albedo"));
    CopyConstantBuffer::create(drawQuad, lightPosConstantBuffer, lightData, 3 * sizeof(LightData));
    BindConstantBuffer::create(drawQuad, lightPosConstantBuffer, BINDING_POINT_LIGHT_DATA);
    CopyConstantBuffer::create(drawQuad, frameDataBuffer, &in_frameData, sizeof(In_FrameData));
    BindConstantBuffer::create(drawQuad, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    CommandBuffer* drawTransparent = CommandBuffer::create(heapAllocator, 10);
    BindFramebuffer::create(drawTransparent, transparentBuffer);
    SetViewport::create(drawTransparent, &gBufferViewport);
    SetDepthTest::create(setGBuffer, true, GL_LEQUAL);
    BindConstantBuffer::create(drawTransparent, lightPosConstantBuffer, BINDING_POINT_LIGHT_DATA);
    CopyConstantBuffer::create(drawTransparent, frameDataBuffer, &in_frameData, sizeof(In_FrameData));
    BindConstantBuffer::create(drawTransparent, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    RenderQueue renderQueue(device, heapAllocator);

    //deferred shading
    ModelInstance::draw(modelInstance0, 1, renderQueue, setGBuffer);
    ModelInstance::draw(modelInstance2, 2, renderQueue, setGBuffer);

    //light accumulation
    Model::draw(quadModel, 3, renderQueue, drawQuad);

    //transparent materials
    ModelInstance::draw(modelInstance1, 4, renderQueue, drawTransparent);

    //todo change to glBlitFramebuffer?
    CommandBuffer* copyCommand = CommandBuffer::create(heapAllocator, 10);
    BindFramebuffer::create(copyCommand, nullFramebuffer);
    SetDepthTest::create(copyCommand, false, GL_NONE);
    SetViewport::create(copyCommand, &viewport);
    BindProgram::create(copyCommand, copyProgram);
    BindTexture::create(copyCommand, copyProgram, quadTexture, 0);
    Model::draw(quadModel, 10, renderQueue, copyCommand);

    CommandBuffer* commandBuffer = renderQueue.sendToCommandBuffer();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

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

        lightData[0].position.x = sin * 2;
        lightData[0].position.y = 0;
        lightData[0].position.z = -1;
        lightData[0].color = {0.9, 0.5, 0.4, 1.0};

        lightData[1].position.x = 0;
        lightData[1].position.y = sin * 2;
        lightData[1].position.z = -1;
        lightData[1].color = {0.2, 0.9, 0.4, 1.0};

        lightData[2].position.x = cos * 2;
        lightData[2].position.y = sin * 2;
        lightData[2].position.z = -1;
        lightData[2].color = {0.1, 0.2, 0.8, 1.0};

        mnMatrix4Identity(in_frameData.projection.values);
        mnMatrix4Identity(in_frameData.view.values);

        float fov = 45.0f * M_PI / 180.0f;
        mnMatrix4Perspective(fov, viewport.width / (float)viewport.height, 0.001, 10, in_frameData.projection.values);
//        mnMatrix4Ortho(-1, +1, -1, +1, -10, +10, in_frameData.projection.values);

        float eye[3] = {sinf(angle)*2.5f, 0, cosf(angle)*2.5f};
        float center[3] = {0, 0, 0};
        mnMatrix4LookAt(eye, center, in_frameData.view.values);

        float axisX[3] = {1, 0, 0};
        float axisY[3] = {0, 1, 0};
        float axisZ[3] = {0, 0, 1};

        float offset0[4][3] = {
                -.0, -.0, 0,
                -.5, +.5, 0,
                +.5, +.5, 0,
                +.5, -.5, 0,
        };
        float scale0[3] = {0.35, 0.35, 0.35};
        mnMatrix4Transformation(axisX, angle, offset0[0], scale0, in_vertexData0[0].in_Rotation);
        mnMatrix4Transformation(axisY, angle, offset0[1], scale0, in_vertexData0[1].in_Rotation);
        mnMatrix4Transformation(axisZ, angle, offset0[2], scale0, in_vertexData0[2].in_Rotation);
        mnMatrix4Transformation(axisX, angle, offset0[3], scale0, in_vertexData0[3].in_Rotation);

        float offset1[2][3] = {
                -0.5, 0, 0,
                +0.5, 0, 0
        };
        float scale1[3] = {0.45, 0.45, 0.45};
        mnMatrix4Transformation(axisY, angle, offset1[0], scale1, in_vertexData1[0].in_Rotation);
        mnMatrix4Transformation(axisY, angle, offset1[1], scale1, in_vertexData1[1].in_Rotation);

        float offset2[3] = {0, 0, 0};
        float scale2[3] = {0.75, 0.75, 0.75};
        mnMatrix4Transformation(axisX, 0, offset2, scale2, in_vertexData2[0].in_Rotation);

        in_vertexData0[0].in_Color = {1, 1, 1, 1};
        in_vertexData0[1].in_Color = {1, 1, 1, 1};
        in_vertexData0[2].in_Color = {1, 1, 1, 1};
        in_vertexData0[3].in_Color = {1, 1, 1, 1};
        in_vertexData1[0].in_Color = {1, 1, 1, 1};
        in_vertexData1[1].in_Color = {1, 1, 1, 1};
        in_vertexData2[0].in_Color = {1, 1, 1, 1};

//        in_vertexData0.in_Color[0].x -= 0.00001;
//        in_vertexData0.in_Color[0].y -= 0.00002;
//        in_vertexData0.in_Color[0].z -= 0.00003;

        angle += 0.005;

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
    Material::destroy(heapAllocator, material2);
    CommandBuffer::destroy(heapAllocator, setGBuffer);
    CommandBuffer::destroy(heapAllocator, drawQuad);
    CommandBuffer::destroy(heapAllocator, commandBuffer);
    CommandBuffer::destroy(heapAllocator, drawTransparent);
    CommandBuffer::destroy(heapAllocator, copyCommand);

    modelManager.destroyModel(quadModel);
    modelManager.destroyModelInstance(modelInstance0);
    modelManager.destroyModelInstance(modelInstance1);
    modelManager.destroyModelInstance(modelInstance2);
    textureManager.unloadTexture(texture0);
    textureManager.unloadTexture(texture1);
    textureManager.unloadTexture(texture2);

    device.destroyTexture(position);
    device.destroyTexture(normal);
    device.destroyTexture(albedo);
    device.destroyTexture(depth);
    device.destroyTexture(quadTexture);
    device.destroyTexture(texture3);
    device.destroyFramebuffer(gBuffer);
    device.destroyFramebuffer(transparentBuffer);
    device.destroyConstantBuffer(lightPosConstantBuffer);
    device.destroyConstantBuffer(constantBuffer0);
    device.destroyConstantBuffer(constantBuffer1);
    device.destroyConstantBuffer(constantBuffer2);
    device.destroyConstantBuffer(frameDataBuffer);
    device.destroyProgram(programOpaque);
    device.destroyProgram(programTransparent);
    device.destroyProgram(quadProgram);
    device.destroyProgram(copyProgram);

    heapAllocator.dumpFreeList();

    glfwDestroyWindow(window);
    glfwTerminate();
}
