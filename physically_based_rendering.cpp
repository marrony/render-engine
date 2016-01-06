//
// Created by Marrony Neris on 12/18/15.
//

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
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
#include "Wavefront.h"

Rect viewport = {};

float angle = 0;
bool autoAngle = true;

int baseColorIndex = 1;

Vector3 baseColors[] = {
        {0.560, 0.570, 0.580},
        {0.972, 0.960, 0.915},
        {0.913, 0.921, 0.925},
        {1.000, 0.766, 0.336},
        {0.955, 0.637, 0.538},
        {0.550, 0.556, 0.554},
        {0.660, 0.609, 0.526},
        {0.542, 0.497, 0.449},
        {0.662, 0.655, 0.634},
        {0.672, 0.637, 0.585},
};

const char* baseColorNames[] = {
        "Iron",
        "Silver",
        "Aluminum",
        "Gold",
        "Copper",
        "Chromium",
        "Nickel",
        "Titanium",
        "Cobalt",
        "Platinum",
};

struct In_MaterialData {
    Vector4 baseColor;
    float roughness;
    float metallic;
    float specularity;
    float ior;
    uint32_t useTBNMatrix;
    uint32_t approximationSpecular;
    uint32_t approximationDiffuse;
};

In_MaterialData materialData = {0};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (!autoAngle && action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:
            angle -= 0.01;
            break;
        case GLFW_KEY_RIGHT:
            angle += 0.01;
            break;
        }
    }

    if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_A:
            materialData.useTBNMatrix = !materialData.useTBNMatrix;
            break;
        case GLFW_KEY_S:
            materialData.approximationSpecular++;
            materialData.approximationSpecular %= 3;
            break;
        case GLFW_KEY_D:
            materialData.approximationDiffuse++;
            materialData.approximationDiffuse %= 2;
            break;
        case GLFW_KEY_SPACE:
            autoAngle = !autoAngle;
            break;
        case GLFW_KEY_0:
            baseColorIndex = 0;
            break;
        case GLFW_KEY_1:
            baseColorIndex = 1;
            break;
        case GLFW_KEY_2:
            baseColorIndex = 2;
            break;
        case GLFW_KEY_3:
            baseColorIndex = 3;
            break;
        case GLFW_KEY_4:
            baseColorIndex = 4;
            break;
        case GLFW_KEY_5:
            baseColorIndex = 5;
            break;
        case GLFW_KEY_6:
            baseColorIndex = 6;
            break;
        case GLFW_KEY_7:
            baseColorIndex = 7;
            break;
        case GLFW_KEY_8:
            baseColorIndex = 8;
            break;
        case GLFW_KEY_9:
            baseColorIndex = 9;
            break;

        case GLFW_KEY_UP:
            materialData.roughness += 0.05;
            break;
        case GLFW_KEY_DOWN:
            materialData.roughness -= 0.05;
            break;

        case GLFW_KEY_N:
            materialData.metallic -= 0.1;
            break;
        case GLFW_KEY_M:
            materialData.metallic += 0.1;
            break;

        case GLFW_KEY_K:
            materialData.specularity += 0.1;
            break;
        case GLFW_KEY_L:
            materialData.specularity -= 0.1;
            break;

        case GLFW_KEY_I:
            materialData.ior += 0.05;
            break;
        case GLFW_KEY_O:
            materialData.ior -= 0.05;
            break;
        }
    }

    materialData.roughness = std::max(0.0f, std::min(materialData.roughness, 1.0f));
    materialData.metallic = std::max(0.0f, std::min(materialData.metallic, 1.0f));
    materialData.specularity = std::max(0.0f, std::min(materialData.specularity, 1.0f));
    materialData.ior = std::max(0.0f, std::min(materialData.ior, 10.0f));
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    viewport.width = width;
    viewport.height = height;
}

#define STR(x) #x

const char* draw_texture_vert = STR(
out vec2 vtx_Texture;

void main() {
    gl_Position = projection * view * instanceData[0].in_Rotation * vec4(in_Position, 1);
    vtx_Texture = in_Texture;
}
);

const char* draw_texture_frag = STR(
in vec2 vtx_Texture;

uniform sampler2D in_Texture;

layout(location = 0) out vec4 out_FragColor;

void main() {
    vec4 value = texture(in_Texture, vtx_Texture);

    out_FragColor = value;
}
);

#undef STR

#include "PBRShaders.h"

int main() {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Physically Based Rendering", NULL, NULL);

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

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = w;
    viewport.height = h;

    HeapAllocator heapAllocator;

    Device device;

    ModelManager modelManager(heapAllocator, device);
    TextureManager textureManager(heapAllocator, device);
    TextManager textManager(heapAllocator, device);

    Font fontBig = textManager.loadFont("./fonts/OpenSans-Bold.ttf", 96);
    Font fontSmall = textManager.loadFont("./fonts/OpenSans-Bold.ttf", 48);

    const int BINDING_POINT_INSTANCE_DATA = 0;
    const int BINDING_POINT_FRAME_DATA = 1;
    const int BINDING_POINT_MATERIAL_DATA = 2;

    const int NUMBER_QUADS = 6;
    const int NUMBER_SPHERES = 4;
    const int WIDTH = 1024;
    const int HEIGHT = 1024;

    ImageCube irradianceImg;
    ImageCube prefilterEnvImg[8];
    Image integrateBRDFImg;
    ImageCube skyboxImg;

#define IMG_PATH "images/LancellottiChapel"

    loadCube(heapAllocator, IMG_PATH"/diffuse_irradiance.irr", irradianceImg);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_0.irr", prefilterEnvImg[0]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_1.irr", prefilterEnvImg[1]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_2.irr", prefilterEnvImg[2]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_3.irr", prefilterEnvImg[3]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_4.irr", prefilterEnvImg[4]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_5.irr", prefilterEnvImg[5]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_6.irr", prefilterEnvImg[6]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_7.irr", prefilterEnvImg[7]);

    loadImage(heapAllocator, IMG_PATH"/integrate_brdf.irr", integrateBRDFImg);

    readJpeg(heapAllocator, IMG_PATH"/posx.jpg", skyboxImg.faces[POSITIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/negx.jpg", skyboxImg.faces[NEGATIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/posy.jpg", skyboxImg.faces[POSITIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/negy.jpg", skyboxImg.faces[NEGATIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/posz.jpg", skyboxImg.faces[POSITIVE_Z]);
    readJpeg(heapAllocator, IMG_PATH"/negz.jpg", skyboxImg.faces[NEGATIVE_Z]);

    TextureCube skyboxIrradiance = createTextureCube(device, &irradianceImg, 1);
    TextureCube prefilterEnv = createTextureCube(device, prefilterEnvImg, 8);
    TextureCube skyboxCube = createTextureCube(device, &skyboxImg, 1);
    Texture2D integrateBRDF = createTexture2D(device, integrateBRDFImg);

    Texture2D skybox[6];

    for(int i = 0; i < 6; i++) {
        skybox[i] = device.createRGBTexture(skyboxImg.faces[i].width, skyboxImg.faces[i].height, skyboxImg.faces[i].pixels);

        heapAllocator.deallocate(skyboxImg.faces[i].pixels);
        heapAllocator.deallocate(irradianceImg.faces[i].pixels);
    }

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 6; j++)
            heapAllocator.deallocate(prefilterEnvImg[i].faces[j].pixels);
    }

    heapAllocator.deallocate(integrateBRDFImg.pixels);

    Texture2D normalTexture = textureManager.loadTexture("images/lion_ddn.tga");

    Model* sphereModel = modelManager.createSphere("sphere01", 1.0, 20);
    Model* quadModel = modelManager.createQuad("quad");
    Model* wavefront = modelManager.loadWavefront("models/venus.obj");

    struct In_InstanceData {
        Matrix4 in_Rotation;
        Vector4 in_Color;
    };

    struct In_FrameData {
        Matrix4 projection;
        Matrix4 view;
        Vector4 cameraPosition;
    };

    ConstantBuffer sphereConstantBuffer = device.createConstantBuffer(NUMBER_SPHERES * sizeof(In_InstanceData));
    ConstantBuffer frameConstantBuffer = device.createConstantBuffer(sizeof(In_FrameData));
    ConstantBuffer materialConstantBuffer = device.createConstantBuffer(sizeof(In_MaterialData));

    ConstantBuffer skyboxConstantBuffer[6];
    for(int i = 0; i < 6; i++)
        skyboxConstantBuffer[i] = device.createConstantBuffer(sizeof(In_InstanceData));

    In_InstanceData sphereData[NUMBER_SPHERES];
    In_InstanceData quadData[NUMBER_QUADS];
    In_FrameData frameData;

    ModelInstance* sphereInstances = modelManager.createModelInstance(sphereModel, NUMBER_SPHERES, sphereConstantBuffer, BINDING_POINT_INSTANCE_DATA);

    Program physicallyBasedShader = device.createProgram(commonSource, physically_based_shader_vert, physically_based_shader_frag, nullptr);
    Program drawTexture = device.createProgram(commonSource, draw_texture_vert, draw_texture_frag, nullptr);

    device.setTextureBindingPoint(physicallyBasedShader, "skyboxIrradiance", 0);
    device.setTextureBindingPoint(physicallyBasedShader, "skyboxCube", 1);
    device.setTextureBindingPoint(physicallyBasedShader, "normalTexture", 2);
    device.setTextureBindingPoint(physicallyBasedShader, "prefilterEnv", 3);
    device.setTextureBindingPoint(physicallyBasedShader, "integrateBRDFTex", 4);

    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_MaterialData", BINDING_POINT_MATERIAL_DATA);

    device.setTextureBindingPoint(drawTexture, "in_Texture", 0);

    device.setConstantBufferBindingPoint(drawTexture, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(drawTexture, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    Texture2D depthTexture = device.createRG32FTexture(WIDTH, HEIGHT, nullptr);
    Texture2D debugTexture = device.createRGBATexture(WIDTH, HEIGHT, nullptr);
    Framebuffer depthFramebuffer = device.createFramebuffer();

    device.bindTextureToFramebuffer(depthFramebuffer, depthTexture, 0);
    device.bindTextureToFramebuffer(depthFramebuffer, debugTexture, 1);
    assert(device.isFramebufferComplete(depthFramebuffer));

    float sc[3] = {1, 1, 1};
    Matrix4 scale;
    mnMatrix4Scale(sc, scale.values);

    float tx0[3] = {0, 0, -3};
    float tx1[3] = {0, 0, -1};
    float tx2[3] = {0, 0, +1};
    float tx3[3] = {0, 0, +3};
    mnMatrix4Translate(tx0, sphereData[0].in_Rotation.values);
    mnMatrix4Mul(sphereData[0].in_Rotation.values, scale.values, sphereData[0].in_Rotation.values);
    mnMatrix4Translate(tx1, sphereData[1].in_Rotation.values);
    mnMatrix4Mul(sphereData[1].in_Rotation.values, scale.values, sphereData[1].in_Rotation.values);
    mnMatrix4Translate(tx2, sphereData[2].in_Rotation.values);
    mnMatrix4Mul(sphereData[2].in_Rotation.values, scale.values, sphereData[2].in_Rotation.values);
    mnMatrix4Translate(tx3, sphereData[3].in_Rotation.values);
    mnMatrix4Mul(sphereData[3].in_Rotation.values, scale.values, sphereData[3].in_Rotation.values);

    CommandBuffer* scenePassCommon = nullptr;
    CommandBuffer* scenePass = nullptr;

    CommandBuffer* skyboxPass[6] = {nullptr};

    {
        float X_AXIS[3] = {1, 0, 0};
        float Y_AXIS[3] = {0, 1, 0};
        float Z_AXIS[3] = {0, 0, 1};
        float sc[3] = {10, 10, 10};

        {
            float tx[3] = {10, 0, 0};
            mnMatrix4Transformation(Y_AXIS, M_PI_2, tx, sc, quadData[POSITIVE_X].in_Rotation.values);
        }

        {
            float tx[3] = {-10, 0, 0};
            mnMatrix4Transformation(Y_AXIS, -M_PI_2, tx, sc, quadData[NEGATIVE_X].in_Rotation.values);
        }

        {
            float tx[3] = {0, 10, 0};
            mnMatrix4Transformation(X_AXIS, -M_PI_2, tx, sc, quadData[POSITIVE_Y].in_Rotation.values);
        }

        {
            float tx[3] = {0, -10, 0};
            mnMatrix4Transformation(X_AXIS, M_PI_2, tx, sc, quadData[NEGATIVE_Y].in_Rotation.values);
        }

        {
            float tx[3] = {0, 0, 10};
            mnMatrix4Transformation(Y_AXIS, 0, tx, sc, quadData[POSITIVE_Z].in_Rotation.values);
        }

        {
            float tx[3] = {0, 0, -10};
            mnMatrix4Transformation(Y_AXIS, M_PI, tx, sc, quadData[NEGATIVE_Z].in_Rotation.values);
        }
    }

    RenderQueue renderQueue(device, heapAllocator);

    double current = glfwGetTime();
    double inc = 0;
    int fps = 0;
    int fps2 = 0;

    float angleLight = 0;

    materialData.roughness = 0.5;
    materialData.metallic = 1.0;
    materialData.specularity = 0.5;
    materialData.ior = 0.0;
    materialData.approximationSpecular = 0;
    materialData.approximationDiffuse = 0;
    baseColorIndex = 0;

    while (!glfwWindowShouldClose(window)) {
        double c = glfwGetTime();
        double d = c - current;
        inc += d;
        current = c;
        fps++;

        if (inc > 1) {
            fps2 = fps;
            fps = 0;
            inc = 0;
        }

        angleLight += 0.3 * d;
        if (autoAngle) {
            angle += 0.3 * d;
        }

        float aspect = viewport.width / viewport.height;
        float at[3] = {0, 0, 0};
        float eye[3] = {cosf(angle)*8, cosf(angle)*5, sinf(angle)*8};
        float upCam[3] = {0, 1, 0};
        mnMatrix4Perspective(55 * M_PI / 180.0, aspect, 0.01, 30, frameData.projection.values);
        mnMatrix4LookAt(eye, at, upCam, frameData.view.values);

        frameData.cameraPosition.x = eye[0];
        frameData.cameraPosition.y = eye[1];
        frameData.cameraPosition.z = eye[2];

        materialData.baseColor.x = baseColors[baseColorIndex].x;
        materialData.baseColor.y = baseColors[baseColorIndex].y;
        materialData.baseColor.z = baseColors[baseColorIndex].z;

        device.copyConstantBuffer(sphereConstantBuffer, sphereData, sizeof(sphereData));
        device.copyConstantBuffer(frameConstantBuffer, &frameData, sizeof(In_FrameData));
        device.copyConstantBuffer(materialConstantBuffer, &materialData, sizeof(In_MaterialData));

        for(int i = 0; i < 6; i++)
            device.copyConstantBuffer(skyboxConstantBuffer[i], &quadData[i], sizeof(In_InstanceData));

        if (scenePassCommon == nullptr) {
            scenePassCommon = CommandBuffer::create(heapAllocator, 100);

            BindConstantBuffer::create(scenePassCommon, frameConstantBuffer, BINDING_POINT_FRAME_DATA);
            BindConstantBuffer::create(scenePassCommon, materialConstantBuffer, BINDING_POINT_MATERIAL_DATA);

            BindFramebuffer::create(scenePassCommon, {0});
            SetDrawBuffers::create(scenePassCommon, 0xffffffff);
            SetViewport::create(scenePassCommon, 0, &viewport);
            ClearColor::create(scenePassCommon, 0, 0.0, 0.0, 0.0, 1);
            SetBlend::disable(scenePassCommon, 0);

#if RIGHT_HANDED
            ClearDepthStencil::create(scenePassCommon, 1.0, 0x00);
            SetDepthTest::create(scenePassCommon, true, GL_LEQUAL);
            SetCullFace::create(scenePassCommon, true, GL_BACK, GL_CCW);
#else
            ClearDepthStencil::create(scenePassCommon, 0.0, 0x00);
            SetDepthTest::create(scenePassCommon, true, GL_GEQUAL);
            SetCullFace::create(scenePassCommon, true, GL_BACK, GL_CW);
            glDepthRange(1, 0);
#endif
        }

        if (scenePass == nullptr) {
            scenePass = CommandBuffer::create(heapAllocator, 10);

            BindProgram::create(scenePass, physicallyBasedShader);
            BindTexture::create(scenePass, skyboxIrradiance, {0}, 0);
            BindTexture::create(scenePass, skyboxCube, {0}, 1);
            BindTexture::create(scenePass, normalTexture, {0}, 2);
            BindTexture::create(scenePass, prefilterEnv, {0}, 3);
            BindTexture::create(scenePass, integrateBRDF, {0}, 4);
        }

        renderQueue.submit(0, &scenePassCommon, 1);
        ModelInstance::drawNoMaterial(sphereInstances, 0, renderQueue, scenePass);

        for(int i = 0; i < 6; i++) {
            if (skyboxPass[i] == nullptr) {
                skyboxPass[i] = CommandBuffer::create(heapAllocator, 100);

                BindConstantBuffer::create(skyboxPass[i], frameConstantBuffer, BINDING_POINT_FRAME_DATA);
                BindConstantBuffer::create(skyboxPass[i], skyboxConstantBuffer[i], BINDING_POINT_INSTANCE_DATA);

                BindProgram::create(skyboxPass[i], drawTexture);
                BindTexture::create(skyboxPass[i], skybox[i], {0}, 0);
            }

            Model::draw(quadModel, 0, renderQueue, skyboxPass[i]);
        }

        renderQueue.sendToDevice();

#if 0
        glBindFramebuffer(GL_READ_FRAMEBUFFER, depthFramebuffer.id); CHECK_ERROR;
        glReadBuffer(GL_COLOR_ATTACHMENT1); CHECK_ERROR;
//        glReadBuffer(GL_DEPTH_ATTACHMENT); CHECK_ERROR;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); CHECK_ERROR;

        glBlitFramebuffer(
                0, 0, WIDTH, HEIGHT,
                0, 0, viewport.width, viewport.height,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
        ); CHECK_ERROR;
#endif

        const float color[3] = {1, 1, 1};
        textManager.printText(fontSmall, {0}, color, 10, 540, "Bump: %s", materialData.useTBNMatrix ? "Yes" : "No");
        textManager.printText(fontSmall, {0}, color, 10, 480, "Specular: %s",
                (materialData.approximationSpecular == 0) ? "Real Time IBL" : ((materialData.approximationSpecular == 1) ? "Real Time Approx. IBL" : "Approx. IBL Tex"));
        textManager.printText(fontSmall, {0}, color, 10, 420, "Diffuse: %s",
                (materialData.approximationDiffuse == 0) ? "Real Time IBL" : "Approx. IBL Tex");
        textManager.printText(fontSmall, {0}, color, 10, 360, "Base color: %s", baseColorNames[baseColorIndex]);
        textManager.printText(fontSmall, {0}, color, 10, 300, "IOR: %.2f", materialData.ior);
        textManager.printText(fontSmall, {0}, color, 10, 240, "Specularity: %.2f", materialData.specularity);
        textManager.printText(fontSmall, {0}, color, 10, 180, "Metallic: %.2f", materialData.metallic);
        textManager.printText(fontSmall, {0}, color, 10, 120, "Roughness: %.2f", materialData.roughness);
        textManager.printText(fontBig, {0}, color, 10, 30, "Physically Based Rendering");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    textureManager.unloadTexture(normalTexture);

    modelManager.destroyModel(sphereModel);
    modelManager.destroyModel(quadModel);
    modelManager.destroyModel(wavefront);

    modelManager.destroyModelInstance(sphereInstances);

    device.destroyConstantBuffer(sphereConstantBuffer);
    device.destroyConstantBuffer(frameConstantBuffer);
    device.destroyConstantBuffer(materialConstantBuffer);
    for(int i = 0; i < 6; i++)
        device.destroyConstantBuffer(skyboxConstantBuffer[i]);

    device.destroyTexture(skyboxIrradiance);
    device.destroyTexture(prefilterEnv);
    device.destroyTexture(skyboxCube);
    device.destroyTexture(depthTexture);
    device.destroyTexture(debugTexture);
    device.destroyTexture(integrateBRDF);
    for(int i = 0; i < 6; i++)
        device.destroyTexture(skybox[i]);

    device.destroyFramebuffer(depthFramebuffer);

    device.destroyProgram(physicallyBasedShader);
    device.destroyProgram(drawTexture);

    heapAllocator.deallocate(scenePassCommon);
    heapAllocator.deallocate(scenePass);
    for(int i = 0; i < 6; i++)
        heapAllocator.deallocate(skyboxPass[i]);
}
