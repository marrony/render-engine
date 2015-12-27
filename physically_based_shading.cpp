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
        {1.00, 0.71, 0.29},
        {0.95, 0.93, 0.88},
        {0.95, 0.64, 0.54},
        {0.56, 0.57, 0.58},
        {0.91, 0.92, 0.92}
};

const char* baseColorNames[] = {
        "gold",
        "silver",
        "copper",
        "iron",
        "aluminum"
};

struct In_MaterialData {
    Vector4 baseColor;
    float roughness;
    float metallic;
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
        case GLFW_KEY_SPACE:
            autoAngle = !autoAngle;
            break;
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

        case GLFW_KEY_UP:
            materialData.roughness += 0.05;
            break;
        case GLFW_KEY_DOWN:
            materialData.roughness -= 0.05;
            break;

        case GLFW_KEY_N:
            materialData.metallic += 0.05;
            break;
        case GLFW_KEY_M:
            materialData.metallic -= 0.05;
            break;
        }
    }
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    viewport.width = width;
    viewport.height = height;
}

#define STR(x) #x

const char* draw_texture_vert = STR(
out vec2 vtx_Texture;

void main() {
    gl_Position = vec4(in_Position, 1);
    vtx_Texture = in_Texture;
}
);

const char* draw_texture_frag = STR(
in vec2 vtx_Texture;

uniform sampler2D in_Texture;

layout(location = 0) out vec4 out_FragColor;

void main() {
    vec4 value = texture(in_Texture, vtx_Texture);
    float a = value.x;
    float b = value.y;

//    out_FragColor = vec4(a/10);
    out_FragColor = vec4(1);
    if(b < 10)
        b = -b;

    out_FragColor = vec4(b - a) / 10;
}
);

const char* physically_based_shader_vert = STR(
layout(std140) uniform in_DepthData {
    mat4 depthProjection;
    mat4 depthView;
};

out vec4 texCoord;
out vec3 normal;
out vec3 lightDirection;
out vec4 Plight;
out vec4 eye;

mat4 bias = mat4(
        vec4(0.5,   0,   0, 0),
        vec4(  0, 0.5,   0, 0),
        vec4(  0,   0, 0.5, 0),
        vec4(0.5, 0.5, 0.5, 1)
);

const vec3 u_lightPosition = vec3(8, 0, 0);

void main() {
    vec4 vertexWorld = instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);

    //normal = normalize(mat3(view * instanceData[gl_InstanceID].in_Rotation) * in_Normal);
    normal = normalize(mat3(view) * in_Normal);

    texCoord = bias * depthProjection * depthView * vertexWorld;
    Plight = depthView * vertexWorld;
    eye = -(view * vertexWorld);
    gl_Position = projection * view * vertexWorld;

    lightDirection = mat3(view) * normalize(u_lightPosition);
}
);

const char* physically_based_shader_frag = STR(
layout(std140) uniform in_DepthData {
    mat4 depthProjection;
    mat4 depthView;
};

layout(std140) uniform in_MaterialData {
    vec4 baseColor;
    float roughness;
    float metallic;
};

uniform sampler2D depthTexture;

in vec4 texCoord;
in vec3 normal;
in vec3 lightDirection;
in vec4 Plight;
in vec4 eye;

layout(location = 0) out vec4 fragColor;

const float M_PI = 3.14159265358979323846264338327950288;

vec3 lerp(vec3 x, vec3 y, float a) {
    return x + (y - x)*a;
}

void main() {
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDirection);
    vec3 V = normalize(eye.xyz);
    vec3 H = normalize(L + V);
    vec3 R = reflect(-V, N);

    float NdotL = max(0, dot(N, L));
    float NdotV = max(1e-5, dot(N, V));
    float LdotH = max(0, dot(L, H));
    float NdotH = max(0, dot(N, H));
    float VdotH = max(0, dot(V, H));
    vec2 p = vec2(NdotL, NdotH);

    vec3 diffuseColor = baseColor.rgb - baseColor.rgb*metallic;
    vec3 specularColor = lerp(vec3(0.08), baseColor.rgb, metallic);

    float alpha = roughness*roughness;
    float n = 2 / alpha - 2;
    float D = ((n + 2) / 2*M_PI) * pow(NdotH, n); //Dp

    float G = 1.0 / (4*VdotH*VdotH);

//    float k = roughness*roughness*0.5;
//    float _v = NdotV * (1 - k) + k;
//    float _l = NdotL * (1 - k) + k;
//    float G = 0.25 / (_v * _l);

//    float G = 1 / (4 * max(NdotL, NdotV));

//    float a = roughness*roughness;
//    float a2 = a*a;
//    float _v = NdotV + sqrt(NdotV * (NdotV - NdotV * a2) + a2);
//    float _l = NdotL + sqrt(NdotL * (NdotL - NdotL * a2) + a2);
//    float G = 1 / (_v * _l);

    //schlick
    float Fc = pow(1 - VdotH, 5);
    vec3 F = mix(specularColor, vec3(1), Fc);

    vec3 diffuse = diffuseColor / M_PI;

//    float FD90 = 0.5 + 2 * VdotH * VdotH * roughness;
//    float _v = 1 + (FD90 - 1) * pow(1 - NdotV, 5);
//    float _l = 1 + (FD90 - 1) * pow(1 - NdotL, 5);
//    vec3 diffuse = diffuseColor * ((1/M_PI) * _v * _l);

    vec3 specular = (D * G) * F;

    fragColor.rgb = (diffuse + specular) * NdotL;
}
);

int main() {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Physically Based Shading", NULL, NULL);

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
    const int BINDING_POINT_DEPTH_DATA = 2;
    const int BINDING_POINT_MATERIAL_DATA = 3;

    const int NUMBER_SPHERES = 4;
    const int WIDTH = 1024;
    const int HEIGHT = 1024;

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
    };

    ConstantBuffer sphere4Instances = device.createConstantBuffer(NUMBER_SPHERES * sizeof(In_InstanceData));
    ConstantBuffer frameConstantBuffer = device.createConstantBuffer(sizeof(In_FrameData));
    ConstantBuffer depthConstantBuffer = device.createConstantBuffer(sizeof(In_FrameData));
    ConstantBuffer materialConstantBuffer = device.createConstantBuffer(sizeof(In_MaterialData));

    In_InstanceData instanceData[NUMBER_SPHERES];
    In_FrameData frameData;
    In_FrameData depthData;

    ModelInstance* modelInstance = modelManager.createModelInstance(wavefront, NUMBER_SPHERES, sphere4Instances, BINDING_POINT_INSTANCE_DATA);

    Program physicallyBasedShader = device.createProgram(commonSource, physically_based_shader_vert, physically_based_shader_frag, nullptr);
    Program drawTexture = device.createProgram(commonSource, draw_texture_vert, draw_texture_frag, nullptr);

    device.setTextureBindingPoint(physicallyBasedShader, "depthTexture", 0);

    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_DepthData", BINDING_POINT_DEPTH_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_MaterialData", BINDING_POINT_MATERIAL_DATA);

    device.setTextureBindingPoint(drawTexture, "in_Texture", 0);

    Rect depthViewport = {0, 0, WIDTH, HEIGHT};

    Texture2D depthTexture = device.createRG32FTexture(WIDTH, HEIGHT, nullptr);
    Texture2D debugTexture = device.createRGBATexture(WIDTH, HEIGHT, nullptr);
    Framebuffer depthFramebuffer = device.createFramebuffer();

    device.bindTextureToFramebuffer(depthFramebuffer, depthTexture, 0);
    device.bindTextureToFramebuffer(depthFramebuffer, debugTexture, 1);
    assert(device.isFramebufferComplete(depthFramebuffer));

    float sc[3] = {.001, .001, .001};
    Matrix4 scale;
    mnMatrix4Scale(sc, scale.values);

    float tx0[3] = {0, -1, -3};
    float tx1[3] = {0, -1, -1};
    float tx2[3] = {0, -1, +1};
    float tx3[3] = {0, -1, +3};
    mnMatrix4Translate(tx0, instanceData[0].in_Rotation.values);
    mnMatrix4Mul(instanceData[0].in_Rotation.values, scale.values, instanceData[0].in_Rotation.values);
    mnMatrix4Translate(tx1, instanceData[1].in_Rotation.values);
    mnMatrix4Mul(instanceData[1].in_Rotation.values, scale.values, instanceData[1].in_Rotation.values);
    mnMatrix4Translate(tx2, instanceData[2].in_Rotation.values);
    mnMatrix4Mul(instanceData[2].in_Rotation.values, scale.values, instanceData[2].in_Rotation.values);
    mnMatrix4Translate(tx3, instanceData[3].in_Rotation.values);
    mnMatrix4Mul(instanceData[3].in_Rotation.values, scale.values, instanceData[3].in_Rotation.values);

    CommandBuffer* scenePassCommon = nullptr;
    CommandBuffer* scenePass = nullptr;
    CommandBuffer* quadPass = nullptr;

    RenderQueue renderQueue(device, heapAllocator);

    double current = glfwGetTime();
    double inc = 0;
    int fps = 0;
    int fps2 = 0;

    float angleLight = 0;

    materialData.roughness = 0.5;
    materialData.metallic = 0.5;

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
        float eye[3] = {cosf(angle)*8, 3, sinf(angle)*8};
        float upCam[3] = {0, 1, 0};
        mnMatrix4Perspective(55 * M_PI / 180.0, aspect, 0.01, 30, frameData.projection.values);
        mnMatrix4LookAt(eye, at, upCam, frameData.view.values);

        float depthFar = 20;
#if 0
        float light[3] = {cosf(angleLight)*8, sinf(angleLight)*8, 0};
        float upLight[3] = {-sinf(angleLight), cosf(angleLight), 0};
#else
        float light[3] = {8, 0, 0};
        float upLight[3] = {0, 1, 0};
#endif
        mnMatrix4Perspective(55 * M_PI / 180.0, (float)WIDTH / (float)HEIGHT, 0.01, depthFar, depthData.projection.values);
        mnMatrix4LookAt(light, at, upLight, depthData.view.values);

        materialData.baseColor.x = baseColors[baseColorIndex-1].x;
        materialData.baseColor.y = baseColors[baseColorIndex-1].y;
        materialData.baseColor.z = baseColors[baseColorIndex-1].z;

        device.copyConstantBuffer(sphere4Instances, instanceData, sizeof(instanceData));
        device.copyConstantBuffer(frameConstantBuffer, &frameData, sizeof(In_FrameData));
        device.copyConstantBuffer(depthConstantBuffer, &depthData, sizeof(In_FrameData));
        device.copyConstantBuffer(materialConstantBuffer, &materialData, sizeof(In_MaterialData));

        if (scenePassCommon == nullptr) {
            scenePassCommon = CommandBuffer::create(heapAllocator, 100);

            //BindConstantBuffer::create(scenePass, sphere4Instances, BINDING_POINT_INSTANCE_DATA);
            BindConstantBuffer::create(scenePassCommon, frameConstantBuffer, BINDING_POINT_FRAME_DATA);
            BindConstantBuffer::create(scenePassCommon, depthConstantBuffer, BINDING_POINT_DEPTH_DATA);
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
            BindTexture::create(scenePass, depthTexture, {0}, 0);
        }

        renderQueue.submit(0, &scenePassCommon, 1);
        ModelInstance::drawNoMaterial(modelInstance, 0, renderQueue, scenePass);

        if (quadPass == nullptr) {
            quadPass = CommandBuffer::create(heapAllocator, 100);

            BindFramebuffer::create(quadPass, {0});
            SetDrawBuffers::create(quadPass, 0xffffffff);
            SetViewport::create(quadPass, 0, &viewport);
            SetDepthTest::disable(quadPass);
            SetCullFace::disable(quadPass);
            ClearColor::create(quadPass, 0, 0, 0, 0, 1);
            SetBlend::disable(quadPass, 0);

            BindProgram::create(quadPass, drawTexture);
            BindTexture::create(quadPass, depthTexture, {0}, 0);
        }

#if 0
        Model::draw(quadModel, 0, renderQueue, quadPass);
#endif

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
        textManager.printText(fontSmall, {0}, color, 10, 240, "Base color: %s", baseColorNames[baseColorIndex-1]);
        textManager.printText(fontSmall, {0}, color, 10, 180, "Metallic: %.2f", materialData.metallic);
        textManager.printText(fontSmall, {0}, color, 10, 120, "Roughness: %.2f", materialData.roughness);
        textManager.printText(fontBig, {0}, color, 10, 30, "Physically Based Shading");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    modelManager.destroyModel(sphereModel);
    modelManager.destroyModel(quadModel);
    modelManager.destroyModel(wavefront);

    modelManager.destroyModelInstance(modelInstance);

    device.destroyConstantBuffer(sphere4Instances);
    device.destroyConstantBuffer(frameConstantBuffer);
    device.destroyConstantBuffer(depthConstantBuffer);
    device.destroyConstantBuffer(materialConstantBuffer);

    device.destroyTexture(depthTexture);
    device.destroyTexture(debugTexture);
    device.destroyFramebuffer(depthFramebuffer);

    device.destroyProgram(physicallyBasedShader);
    device.destroyProgram(drawTexture);

    heapAllocator.deallocate(scenePassCommon);
    heapAllocator.deallocate(scenePass);
    heapAllocator.deallocate(quadPass);
}
