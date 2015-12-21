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

Rect viewport = {};

int framebufferIndex = 9;
float angle = 0;
bool autoAngle = true;

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
        case GLFW_KEY_0:
            framebufferIndex = 0;
            break;
        case GLFW_KEY_1:
            framebufferIndex = 1;
            break;
        case GLFW_KEY_2:
            framebufferIndex = 2;
            break;
        case GLFW_KEY_3:
            framebufferIndex = 3;
            break;
        case GLFW_KEY_4:
            framebufferIndex = 4;
            break;
        case GLFW_KEY_5:
            framebufferIndex = 5;
            break;
        case GLFW_KEY_6:
            framebufferIndex = 6;
            break;
        case GLFW_KEY_7:
            framebufferIndex = 7;
            break;
        case GLFW_KEY_8:
            framebufferIndex = 8;
            break;
        case GLFW_KEY_9:
            framebufferIndex = 9;
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

const char* depth_shader_vert = STR(
out float dist;

void main() {
    vec3 position = in_Position + in_Normal*0.05;

    gl_Position = projection * view * instanceData[gl_InstanceID].in_Rotation * vec4(position, 1);

    dist = length(view * instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1));
}
);

const char* depth_shader_frag = STR(
layout(location = 0) out vec4 vFragDepth;
layout(location = 1) out vec4 vFragColor;

in float dist;

void main() {
    vFragDepth = vec4(dist, -dist, 0, 0);
    vFragColor = vec4(1, 0, 1, 1);
}
);

const char* subsurface_shader_vert = STR(
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

const char* subsurface_shader_frag = STR(
layout(std140) uniform in_DepthData {
    mat4 depthProjection;
    mat4 depthView;
};

uniform sampler2D depthTexture;

in vec4 texCoord;
in vec3 normal;
in vec3 lightDirection;
in vec4 Plight;
in vec4 eye;

layout(location = 0) out vec4 fragColor;

float wrapLighting(float x, float wrap) {
    return (x + wrap) / (1.0 + wrap);
}

float scatterTint(float x, float scatterWidth) {
    return smoothstep(0.0, scatterWidth, x) * smoothstep(scatterWidth * 2.0, scatterWidth, x);
}

const float u_wrap = 1; //0..1
const float u_scatterWidth = 1;
const float u_shininess = 40;

const vec4 u_diffuseColor = vec4(0.8, 0.0, 0.0, 1.0);
const vec4 u_scatterColor = vec4(0.15, 0.0, 0.0, 1.0);
const vec4 u_specularColor = vec4(0.8, 0.8, 0.0, 1.0);

vec4 skinLut(vec2 p, vec3 scatterColor) {
    float NdotL = p.x * 2.0 - 1.0;
    float NdotH = p.y * 2.0 - 1.0;

    float NdotL_wrap = wrapLighting(NdotL, u_wrap);
    float diffuse = max(0, NdotL_wrap);
    float scatter = scatterTint(NdotL_wrap, u_scatterWidth);

    float specular = pow(NdotH, u_shininess);
    if(NdotL_wrap <= 0)
        specular = 0;

    return vec4(diffuse + scatter * scatterColor, specular);
}

vec3 shadeSkin(vec3 N, vec3 L, vec3 H, vec3 diffuseColor, vec3 specularColor, vec3 scatterColor) {
    float NdotL = dot(N, L);
    float NdotH = dot(N, H);
    vec2 p = vec2(NdotL, NdotH);

    vec4 light = skinLut(p * 0.5 + 0.5, scatterColor);
    return diffuseColor * light.rgb + specularColor * light.a;
}

const float u_scatterFalloff = 0.9;

float trace() {
    vec3 proj = texCoord.xyz / texCoord.www;
    float d_i = texture(depthTexture, proj.xy).x;
    float d_o = length(Plight);
    return (d_o - d_i);
}

vec3 sss(vec3 lightColor) {
    float s = trace();

    return lightColor * exp(-s * u_scatterFalloff);
}

void main() {
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDirection);
    vec3 V = normalize(eye.xyz);
    vec3 H = normalize(L + V);
    vec3 R = reflect(-V, N);

    fragColor.rgb = vec3(0);
    fragColor.rgb += sss(vec3(1, 0, 0));
//    fragColor.rgb += shadeSkin(N, L, H, vec3(1, 1, 1), vec3(0, 0, 0), vec3(1, 0, 0));
}
);

int main() {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Subsurface Scattering", NULL, NULL);

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

    Font fontRegular = textManager.loadFont("./fonts/OpenSans-Bold.ttf", 96);
    Texture2D lion = textureManager.loadTexture("images/stained_glass.tga");

    const int BINDING_POINT_INSTANCE_DATA = 0;
    const int BINDING_POINT_FRAME_DATA = 1;
    const int BINDING_POINT_DEPTH_DATA = 2;
    const int NUMBER_SPHERES = 5;
    const int WIDTH = 1024;
    const int HEIGHT = 1024;

    Model* sphereModel = modelManager.createSphere("sphere01", 1.0, 20);
    Model* quadModel = modelManager.createQuad("quad");

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

    In_InstanceData instanceData[NUMBER_SPHERES];
    In_FrameData frameData;
    In_FrameData depthData;

    ModelInstance* modelInstance = modelManager.createModelInstance(sphereModel, NUMBER_SPHERES, sphere4Instances, BINDING_POINT_INSTANCE_DATA);

    Program depthShader = device.createProgram(commonSource, depth_shader_vert, depth_shader_frag, nullptr);
    Program subsurfaceShader = device.createProgram(commonSource, subsurface_shader_vert, subsurface_shader_frag, nullptr);
    Program drawTexture = device.createProgram(commonSource, draw_texture_vert, draw_texture_frag, nullptr);

    device.setConstantBufferBindingPoint(depthShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(depthShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    device.setTextureBindingPoint(subsurfaceShader, "depthTexture", 0);

    device.setConstantBufferBindingPoint(subsurfaceShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(subsurfaceShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(subsurfaceShader, "in_DepthData", BINDING_POINT_DEPTH_DATA);

    device.setTextureBindingPoint(drawTexture, "in_Texture", 0);

    Rect depthViewport = {0, 0, WIDTH, HEIGHT};

    Texture2D depthTexture = device.createRG32FTexture(WIDTH, HEIGHT, nullptr);
    Texture2D color = device.createRGBATexture(WIDTH, HEIGHT, nullptr);
    Framebuffer depthFramebuffer = device.createFramebuffer();

    device.bindTextureToFramebuffer(depthFramebuffer, depthTexture, 0);
    device.bindTextureToFramebuffer(depthFramebuffer, color, 1);
    assert(device.isFramebufferComplete(depthFramebuffer));

    float sc[3] = {.6, 1, 1};
    Matrix4 scale;
    mnMatrix4Scale(sc, scale.values);

    float tx0[3] = {+.4, 0, 0};
    float tx1[3] = {-.4, 0, 0};
    float tx2[3] = {0, 1, +.2};
    float tx3[3] = {0, 1, -.2};
    mnMatrix4Translate(tx0, instanceData[0].in_Rotation.values);
    mnMatrix4Mul(instanceData[0].in_Rotation.values, scale.values, instanceData[0].in_Rotation.values);
    mnMatrix4Translate(tx1, instanceData[1].in_Rotation.values);
    mnMatrix4Mul(instanceData[1].in_Rotation.values, scale.values, instanceData[1].in_Rotation.values);
    mnMatrix4Translate(tx2, instanceData[2].in_Rotation.values);
    mnMatrix4Mul(instanceData[2].in_Rotation.values, scale.values, instanceData[2].in_Rotation.values);
    mnMatrix4Translate(tx3, instanceData[3].in_Rotation.values);
    mnMatrix4Mul(instanceData[3].in_Rotation.values, scale.values, instanceData[3].in_Rotation.values);

    CommandBuffer* depthPass = nullptr;
    CommandBuffer* scenePass = nullptr;
    CommandBuffer* quadPass = nullptr;

    RenderQueue renderQueue(device, heapAllocator);

    double current = glfwGetTime();
    double inc = 0;
    int fps = 0;
    int fps2 = 0;

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

        if (autoAngle) {
            angle += 0.3 * d;
        }

        float aspect = viewport.width / viewport.height;
        float at[3] = {0, 0, 0};
        float eye[3] = {cosf(angle)*10, 0, sinf(angle)*10};
        mnMatrix4Perspective(45 * M_PI / 180.0, aspect, 0.01, 30, frameData.projection.values);
        mnMatrix4LookAt(eye, at, frameData.view.values);

        float depthFar = 10;
        float light[3] = {8, 0, 0};
        mnMatrix4Perspective(45 * M_PI / 180.0, (float)WIDTH / (float)HEIGHT, 0.5, depthFar, depthData.projection.values);
        mnMatrix4LookAt(light, at, depthData.view.values);

        float sc2[3] = {0.1, 0.1, 0.1};
        mnMatrix4Scale(sc2, scale.values);
        mnMatrix4Translate(light, instanceData[4].in_Rotation.values);
        mnMatrix4Mul(instanceData[4].in_Rotation.values, scale.values, instanceData[4].in_Rotation.values);

        device.copyConstantBuffer(sphere4Instances, instanceData, sizeof(instanceData));
        device.copyConstantBuffer(frameConstantBuffer, &frameData, sizeof(In_FrameData));
        device.copyConstantBuffer(depthConstantBuffer, &depthData, sizeof(In_FrameData));

        if (depthPass == nullptr) {
            depthPass = CommandBuffer::create(heapAllocator, 100);

            BindConstantBuffer::create(depthPass, sphere4Instances, BINDING_POINT_INSTANCE_DATA);
            BindConstantBuffer::create(depthPass, depthConstantBuffer, BINDING_POINT_FRAME_DATA);

            BindFramebuffer::create(depthPass, depthFramebuffer);
            SetDrawBuffers::create(depthPass, (1 << 0) | (1 << 1));
            SetViewport::create(depthPass, 0, &depthViewport);

            ClearColor::create(depthPass, 0, depthFar, depthFar, 0, 0);
            ClearColor::create(depthPass, 1, 0, 0, 0, 1);

            SetDepthTest::disable(depthPass);

            SetBlend::create(depthPass, true, 0, GL_MIN, GL_NONE, GL_NONE);
            SetBlend::disable(depthPass, 1);

            SetCullFace::disable(depthPass);
            BindProgram::create(depthPass, depthShader);
        }

        ModelInstance::drawNoMaterial(modelInstance, 0, renderQueue, depthPass);

        if (scenePass == nullptr) {
            scenePass = CommandBuffer::create(heapAllocator, 100);

            //BindConstantBuffer::create(scenePass, sphere4Instances, BINDING_POINT_INSTANCE_DATA);
            BindConstantBuffer::create(scenePass, frameConstantBuffer, BINDING_POINT_FRAME_DATA);
            BindConstantBuffer::create(scenePass, depthConstantBuffer, BINDING_POINT_DEPTH_DATA);

            BindFramebuffer::create(scenePass, {0});
            SetDrawBuffers::create(scenePass, 0xffffffff);
            SetViewport::create(scenePass, 0, &viewport);
            ClearColor::create(scenePass, 0, 0.0, 0.0, 0.0, 1);
            SetBlend::disable(scenePass, 0);

#if RIGHT_HANDED
            ClearDepthStencil::create(scenePass, 1.0, 0x00);
            SetDepthTest::create(scenePass, true, GL_LEQUAL);
            SetCullFace::create(scenePass, true, GL_BACK, GL_CCW);
#else
            ClearDepthStencil::create(scenePass, 0.0, 0x00);
            SetDepthTest::create(scenePass, true, GL_GEQUAL);
            SetCullFace::create(scenePass, true, GL_BACK, GL_CW);
            glDepthRange(1, 0);
#endif

            BindProgram::create(scenePass, subsurfaceShader);
//            BindTexture::create(scenePass, lion, {0}, 0);
            BindTexture::create(scenePass, depthTexture, {0}, 0);
        }

        ModelInstance::drawNoMaterial(modelInstance, 0, renderQueue, scenePass);

#if 0
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
        textManager.printText(fontRegular, {0}, color, 10, 30, "Subsurface Scattering");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
