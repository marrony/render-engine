#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

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

Rect viewport = {};

enum ProjectionType {
    PERSPECTIVE,
    PERSPECTIVE_INFINITE,
    ORTHO
};

ProjectionType projection = PERSPECTIVE;

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

    if (key == GLFW_KEY_1 && action == GLFW_RELEASE)
        projection = PERSPECTIVE;

    if (key == GLFW_KEY_2 && action == GLFW_RELEASE)
        projection = PERSPECTIVE_INFINITE;

    if (key == GLFW_KEY_3 && action == GLFW_RELEASE)
        projection = ORTHO;
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

    int BINDING_POINT_INSTANCE_DATA = 0;
    int BINDING_POINT_FRAME_DATA = 1;
    int BINDING_POINT_LIGHT_DATA = 2;

    Program programOpaque = device.createProgram(commonSource, vertexSource, fragmentSource, nullptr);
    Program programTransparent = device.createProgram(commonSource, vertexTransparencySource, fragmentTransparencySource, nullptr);
    Program quadProgram = device.createProgram(commonSource, quadVertexSource, quadFragmentSource, quadGeometrySource);
    Program copyProgram = device.createProgram(commonSource, quadVertexSource, copyFragmentSource, quadGeometrySource);

    device.setTextureBindingPoint(programOpaque, "in_MainTex", 0);
    device.setTextureBindingPoint(programOpaque, "in_BumpMap", 1);
    device.setConstantBufferBindingPoint(programOpaque, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(programOpaque, "in_FrameData", BINDING_POINT_FRAME_DATA);

    device.setTextureBindingPoint(programTransparent, "in_MainTex", 0);
    device.setTextureBindingPoint(programTransparent, "in_BumpMap", 1);
    device.setConstantBufferBindingPoint(programTransparent, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(programTransparent, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(programTransparent, "in_LightData", BINDING_POINT_LIGHT_DATA);

    device.setTextureBindingPoint(quadProgram, "in_Buffer0", 0);
    device.setTextureBindingPoint(quadProgram, "in_Buffer1", 1);
    device.setTextureBindingPoint(quadProgram, "in_Buffer2", 2);
    device.setConstantBufferBindingPoint(quadProgram, "in_LightData", BINDING_POINT_LIGHT_DATA);

    device.setTextureBindingPoint(copyProgram, "in_Texture", 0);

    struct In_InstanceData {
        Matrix4 in_Rotation;
        Vector4 in_Color;
    };

    struct In_FrameData {
        Matrix4 projection;
        Matrix4 view;
    };

    struct In_LightData {
        Vector4 position;
        Vector4 color;
    };

    In_FrameData in_frameData;
    In_InstanceData in_sphere4Instances[4];
    In_InstanceData in_sphere2Instances[2];
    In_InstanceData in_plane1Instance[1];
    In_InstanceData in_planeTranspInstance[1];

    uint32_t pixel = 0x00ff0000;
    uint32_t pixels[] = {pixel, pixel, pixel, pixel};

    Texture2D texture0 = textureManager.loadTexture("images/lion.tga");
    Texture2D texture1 = textureManager.loadTexture("images/lion_ddn.tga");
    Texture2D texture2 = textureManager.loadTexture("images/stained_glass.tga");
    Texture2D texture3 = device.createRGBATexture(2, 2, pixels);

    MaterialBumpedDiffuse bumpedDiffuse;
    bumpedDiffuse.program = programOpaque;
    bumpedDiffuse.mainUnit = 0;
    bumpedDiffuse.mainTex = texture0;
    bumpedDiffuse.mainSampler = textureManager.getLinear();
    bumpedDiffuse.bumpUnit = 1;
    bumpedDiffuse.bumpMap = texture1;
    bumpedDiffuse.bumpSampler = textureManager.getNearest();
    Material* diffuseMaterial = Material::create(heapAllocator, &bumpedDiffuse);

    MaterialTransparency transparency;
    transparency.program = programTransparent;
    transparency.mainUnit = 0;
    transparency.mainTex = texture2;
    transparency.mainSampler = textureManager.getLinear();
    transparency.bumpUnit = 1;
    transparency.bumpMap = texture3;
    transparency.bumpSampler = textureManager.getNearest();
    transparency.alpha = 0.5;
    Material* transparentMaterial = Material::create(heapAllocator, &transparency);

    MaterialBumpedDiffuse bumpedDiffuse2;
    bumpedDiffuse2.program = programOpaque;
    bumpedDiffuse2.mainUnit = 0;
    bumpedDiffuse2.mainTex = texture2;
    bumpedDiffuse2.mainSampler = textureManager.getLinear();
    bumpedDiffuse2.bumpUnit = 1;
    bumpedDiffuse2.bumpMap = texture3;
    bumpedDiffuse2.bumpSampler = textureManager.getNearest();
    Material* backgroundMaterial = Material::create(heapAllocator, &bumpedDiffuse2);

    ConstantBuffer sphere4Instances = device.createConstantBuffer(4 * sizeof(In_InstanceData));
    ConstantBuffer sphere2Instances = device.createConstantBuffer(2 * sizeof(In_InstanceData));
    ConstantBuffer plane1Instance = device.createConstantBuffer(1 * sizeof(In_InstanceData));
    ConstantBuffer planeTranspInstance = device.createConstantBuffer(1 * sizeof(In_InstanceData));
    ConstantBuffer frameDataBuffer = device.createConstantBuffer(sizeof(In_FrameData));

    Model* sphereModel = modelManager.createSphere("sphere01", 1.0, 20);

    ModelInstance* modelInstance0 = modelManager.createModelInstance(sphereModel, 4, sphere4Instances, BINDING_POINT_INSTANCE_DATA);
    ModelInstance::setMaterial(modelInstance0, 0, diffuseMaterial);

    ModelInstance* modelInstance1 = modelManager.createModelInstance(sphereModel, 2, sphere2Instances, BINDING_POINT_INSTANCE_DATA);
    ModelInstance::setMaterial(modelInstance1, 0, transparentMaterial);

    Model* quadModel = modelManager.createQuad("quad");

    ModelInstance* modelInstance2 = modelManager.createModelInstance(quadModel, 1, plane1Instance, BINDING_POINT_INSTANCE_DATA);
    ModelInstance::setMaterial(modelInstance2, 0, backgroundMaterial);

    ModelInstance* modelInstance3 = modelManager.createModelInstance(quadModel, 1, planeTranspInstance, BINDING_POINT_INSTANCE_DATA);
    ModelInstance::setMaterial(modelInstance3, 0, transparentMaterial);

    modelManager.destroyModel(sphereModel);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = w;
    viewport.height = h;

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

    assert(device.isFramebufferComplete(gBuffer));

//    int targets[] = { 0, 1, 2 };
//    device.setRenderTarget(gBuffer, targets, 3);

    Framebuffer transparentBuffer = device.createFramebuffer();
    Texture2D quadTexture = device.createRGBA32FTexture(wgbuffer, hgbuffer, nullptr);
    device.bindTextureToFramebuffer(transparentBuffer, quadTexture, 0);
    device.bindDepthStencilTextureToFramebuffer(transparentBuffer, depth);

    assert(device.isFramebufferComplete(transparentBuffer));

//    int quadTarget = 0;
//    device.setRenderTarget(transparentBuffer, &quadTarget, 1);

    Rect gBufferViewport = {0, 0, (float)wgbuffer, (float)hgbuffer};

    In_LightData lightData[3] = {};
    ConstantBuffer lightPosConstantBuffer = device.createConstantBuffer(3 * sizeof(In_LightData));

    CommandBuffer* drawQuadLight = CommandBuffer::create(heapAllocator, 20);
    BindFramebuffer::create(drawQuadLight, transparentBuffer);
    SetDrawBuffers::create(drawQuadLight, 0x1);
    SetViewport::create(drawQuadLight, 0, &gBufferViewport);
    SetDepthTest::disable(drawQuadLight);
    SetCullFace::disable(drawQuadLight);
    BindProgram::create(drawQuadLight, quadProgram);
    BindTexture::create(drawQuadLight, position, textureManager.getNearest(), 0);
    BindTexture::create(drawQuadLight, normal, textureManager.getNearest(), 1);
    BindTexture::create(drawQuadLight, albedo, textureManager.getNearest(), 2);
    BindConstantBuffer::create(drawQuadLight, lightPosConstantBuffer, BINDING_POINT_LIGHT_DATA);
    BindConstantBuffer::create(drawQuadLight, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    CommandBuffer* drawTransparent = CommandBuffer::create(heapAllocator, 10);
    BindFramebuffer::create(drawTransparent, transparentBuffer);
    SetDrawBuffers::create(drawTransparent, 0x1);
    SetViewport::create(drawTransparent, 0, &gBufferViewport);
#if RIGHT_HANDED
    SetDepthTest::create(drawTransparent, true, GL_LEQUAL);
#else
    SetDepthTest::create(drawTransparent, true, GL_GEQUAL);
#endif
    BindConstantBuffer::create(drawTransparent, lightPosConstantBuffer, BINDING_POINT_LIGHT_DATA);
    BindConstantBuffer::create(drawTransparent, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    RenderQueue renderQueue(device, heapAllocator);

    CommandBuffer* setupGBuffer = CommandBuffer::create(heapAllocator, 11);
    BindFramebuffer::create(setupGBuffer, gBuffer);
    SetDrawBuffers::create(setupGBuffer, 0x7);
    SetViewport::create(setupGBuffer, 0, &gBufferViewport);
    ClearColor::create(setupGBuffer, 0, 0.00, 0.00, 0.00, 0); //position
    ClearColor::create(setupGBuffer, 1, 0.00, 0.00, 0.00, 0); //normal
    ClearColor::create(setupGBuffer, 2, 0.50, 0.50, 0.50, 0); //albedo
#if RIGHT_HANDED
    ClearDepthStencil::create(setupGBuffer, 1.0, 0x00);
    SetDepthTest::create(setupGBuffer, true, GL_LEQUAL);
    SetCullFace::create(setupGBuffer, true, GL_BACK, GL_CCW);
#else
    ClearDepthStencil::create(setupGBuffer, 0.0, 0x00);
    SetDepthTest::create(setupGBuffer, true, GL_GEQUAL);
    SetCullFace::create(setupGBuffer, true, GL_BACK, GL_CW);
    glDepthRange(1, 0);
#endif
    BindConstantBuffer::create(setupGBuffer, frameDataBuffer, BINDING_POINT_FRAME_DATA);

    renderQueue.submit(0, &setupGBuffer, 1);

    CommandBuffer empty = {0};

    //deferred shading
    ModelInstance::draw(modelInstance0, 1, renderQueue, &empty);
    ModelInstance::draw(modelInstance2, 2, renderQueue, &empty);

    //light accumulation
    Model::draw(quadModel, 3, renderQueue, drawQuadLight);

    //transparent materials
    ModelInstance::draw(modelInstance1, 4, renderQueue, drawTransparent);
    ModelInstance::draw(modelInstance3, 5, renderQueue, drawTransparent);

    //todo change to glBlitFramebuffer?
    Framebuffer nullFramebuffer = {0};
    CommandBuffer* copyCommand = CommandBuffer::create(heapAllocator, 10);
    BindFramebuffer::create(copyCommand, nullFramebuffer);
    SetDrawBuffers::create(copyCommand, 0xffffffff);
    SetDepthTest::disable(copyCommand);
    SetCullFace::disable(copyCommand);
    SetViewport::create(copyCommand, 0, &viewport);
    BindProgram::create(copyCommand, copyProgram);
    BindTexture::create(copyCommand, quadTexture, textureManager.getNearest(), 0);
    Model::draw(quadModel, 10, renderQueue, copyCommand);

    CommandBuffer* commandBuffer = renderQueue.sendToCommandBuffer();

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
        float aspect = viewport.width / viewport.height;
        switch (projection) {
        case PERSPECTIVE:
            mnMatrix4Perspective(fov, aspect, 0.001, 10, in_frameData.projection.values);
            break;
        case PERSPECTIVE_INFINITE:
            mnMatrix4InfinitePerspective(fov, aspect, 0.001, in_frameData.projection.values);
            break;
        case ORTHO:
            mnMatrix4Ortho(-1*aspect, +1*aspect, -1, +1, -10, +10, in_frameData.projection.values);
            break;
        }

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
        mnMatrix4Transformation(axisX, angle, offset0[0], scale0, in_sphere4Instances[0].in_Rotation.values);
        mnMatrix4Transformation(axisY, angle, offset0[1], scale0, in_sphere4Instances[1].in_Rotation.values);
        mnMatrix4Transformation(axisZ, angle, offset0[2], scale0, in_sphere4Instances[2].in_Rotation.values);
        mnMatrix4Transformation(axisX, angle, offset0[3], scale0, in_sphere4Instances[3].in_Rotation.values);

        float offset1[2][3] = {
                -0.5, 0, 0,
                +0.5, 0, 0
        };
        float scale1[3] = {0.45, 0.45, 0.45};
        mnMatrix4Transformation(axisY, angle, offset1[0], scale1, in_sphere2Instances[0].in_Rotation.values);
        mnMatrix4Transformation(axisY, angle, offset1[1], scale1, in_sphere2Instances[1].in_Rotation.values);

        float offset2[3] = {0, 0, 0};
        float scale2[3] = {0.75, 0.75, 0.75};
        mnMatrix4Transformation(axisX, 0, offset2, scale2, in_plane1Instance[0].in_Rotation.values);

        float offset3[3] = {0, 0, -0.5};
        float scale3[3] = {0.5, 0.5, 0.5};
        mnMatrix4Transformation(axisX, 0, offset3, scale3, in_planeTranspInstance[0].in_Rotation.values);

        in_sphere4Instances[0].in_Color = {1, 1, 1, 1};
        in_sphere4Instances[1].in_Color = {1, 1, 1, 1};
        in_sphere4Instances[2].in_Color = {1, 1, 1, 1};
        in_sphere4Instances[3].in_Color = {1, 1, 1, 1};
        in_sphere2Instances[0].in_Color = {1, 1, 1, 1};
        in_sphere2Instances[1].in_Color = {1, 1, 1, 1};
        in_plane1Instance[0].in_Color = {1, 1, 1, 1};

//        in_vertexData0.in_Color[0].x -= 0.00001;
//        in_vertexData0.in_Color[0].y -= 0.00002;
//        in_vertexData0.in_Color[0].z -= 0.00003;

        angle += 0.005;

        device.copyConstantBuffer(lightPosConstantBuffer, lightData, 3 * sizeof(In_LightData));
        device.copyConstantBuffer(frameDataBuffer, &in_frameData, sizeof(In_FrameData));
        device.copyConstantBuffer(sphere4Instances, in_sphere4Instances, 4 * sizeof(In_InstanceData));
        device.copyConstantBuffer(sphere2Instances, in_sphere2Instances, 2 * sizeof(In_InstanceData));
        device.copyConstantBuffer(plane1Instance, in_plane1Instance, 1 * sizeof(In_InstanceData));
        device.copyConstantBuffer(planeTranspInstance, in_planeTranspInstance, 1 * sizeof(In_InstanceData));

        CommandBuffer::execute(commandBuffer, device);

#if 0
//        glBindFramebuffer(GL_READ_FRAMEBUFFER, transparentBuffer.id); CHECK_ERROR;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.id); CHECK_ERROR;
        glReadBuffer(GL_COLOR_ATTACHMENT0); CHECK_ERROR;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); CHECK_ERROR;

        glBlitFramebuffer(
                0, 0, wgbuffer, hgbuffer,
                0, 0, viewport.width, viewport.height,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
        ); CHECK_ERROR;
#endif

        const char* str;

        switch (projection) {
        case PERSPECTIVE:
            str = "Perspective";
            break;
        case PERSPECTIVE_INFINITE:
            str = "Perspective Infinite";
            break;
        case ORTHO:
            str = "Orthographic";
            break;
        }

        const float white[3] = {1, 1, 1};
        textManager.printText(fontItalic, nullFramebuffer, white, 10, 230, "Fps: %d Angle: %f", fps2, angle);
        textManager.printText(fontRegular, nullFramebuffer, white, 10, 180, "viewport: %.2f %.2f %.2f %.2f", viewport.x, viewport.y, viewport.width, viewport.height);
        textManager.printText(fontRegular, nullFramebuffer, white, 10, 130, "Memory used %ld bytes", heapAllocator.memoryUsed());
        float totalCommands = renderQueue.getExecutedCommands() + renderQueue.getSkippedCommands();
        textManager.printText(fontRegular, nullFramebuffer, white, 10, 80, "Executed commands %d | %.2f%% executed",
                              renderQueue.getExecutedCommands(), renderQueue.getExecutedCommands() / totalCommands * 100);
        textManager.printText(fontRegular, nullFramebuffer, white, 10, 30, "Skipped commands %d | %.2f%% ignored",
                              renderQueue.getSkippedCommands(), renderQueue.getSkippedCommands() / totalCommands * 100);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Material::destroy(heapAllocator, diffuseMaterial);
    Material::destroy(heapAllocator, transparentMaterial);
    Material::destroy(heapAllocator, backgroundMaterial);
    CommandBuffer::destroy(heapAllocator, setupGBuffer);
    CommandBuffer::destroy(heapAllocator, drawQuadLight);
    CommandBuffer::destroy(heapAllocator, commandBuffer);
    CommandBuffer::destroy(heapAllocator, drawTransparent);
    CommandBuffer::destroy(heapAllocator, copyCommand);

    modelManager.destroyModel(quadModel);
    modelManager.destroyModelInstance(modelInstance0);
    modelManager.destroyModelInstance(modelInstance1);
    modelManager.destroyModelInstance(modelInstance2);
    modelManager.destroyModelInstance(modelInstance3);
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
    device.destroyConstantBuffer(sphere4Instances);
    device.destroyConstantBuffer(sphere2Instances);
    device.destroyConstantBuffer(plane1Instance);
    device.destroyConstantBuffer(planeTranspInstance);
    device.destroyConstantBuffer(frameDataBuffer);
    device.destroyProgram(programOpaque);
    device.destroyProgram(programTransparent);
    device.destroyProgram(quadProgram);
    device.destroyProgram(copyProgram);

    heapAllocator.dumpFreeList();

    glfwDestroyWindow(window);
    glfwTerminate();
}
