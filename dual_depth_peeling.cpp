//
// Created by Marrony Neris on 12/14/15.
//

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

int framebufferIndex = 9;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_RELEASE) {
        switch (key) {
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

const char* cube_shader_vert = STR(
void main() {
    //get the clipspace vertex position
    gl_Position = projection * view * instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);
}
);

const char* cube_shader_frag = STR(
layout(location = 0) out vec4 vFragColor; //output fragment colour

void main() {
    //just set the vColor uniform as the fragment colour
   vFragColor = vec4(1, 1, 1, 1);
}
);

const char* dual_init_frag = STR(
layout(location = 0) out vec4 vFragColor; //fragment shader output

void main() {
    //set the fragment colour as -fragment depth and fragment depth
    //in the red and green channel. This when combined with min/max
    //blending will help in peeling front and back layers simultaneously
    vFragColor.xy = vec2(-gl_FragCoord.z, gl_FragCoord.z);
}
);

const char* dual_peel_vert = STR(
out vec2 fragCoord;
out vec4 vColor;

void main() {
    //get the clipspace vertex position
    gl_Position = projection * view * instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);
    vColor = instanceData[gl_InstanceID].in_Color;
    fragCoord = in_Texture;
}
);

const char* dual_peel_frag = STR(
layout(location = 0) out vec4 vFragColor0;  //output to target 0
layout(location = 1) out vec4 vFragColor1;  //output to target 1
layout(location = 2) out vec4 vFragColor2;  //output to target 2

in vec2 fragCoord;
in vec4 vColor;

uniform sampler2D  depthBlenderTex; //depth blending output
uniform sampler2D  frontBlenderTex; //front blending output
uniform sampler2D  colorTex;

\n#define MAX_DEPTH 1.0\n   //max depth value to clear the depth with

void main() {
    vec2 fragCoord2 = gl_FragCoord.xy / vec2(1024, 768);

    //get the current fragment depth
    float fragDepth = gl_FragCoord.z;
    //get the depth value from the depth blending output
    vec2 depthBlender = texture(depthBlenderTex, fragCoord2.xy).xy;
    //get the front blending output
    vec4 forwardTemp = texture(frontBlenderTex, fragCoord2.xy);

    // Depths and 1.0-alphaMult always increase
    // so we can use pass-through by default with MAX blending
    vFragColor0.xy = depthBlender;

    // Front colors always increase (DST += SRC*ALPHA_MULT)
    // so we can use pass-through by default with MAX blending
    vFragColor1 = forwardTemp;

    // Because over blending makes color increase or decrease,
    // we cannot pass-through by default.
    // Each pass, only one fragment can a color greater than 0
    vFragColor2 = vec4(0.0);

    float nearestDepth = -depthBlender.x;
    float farthestDepth = depthBlender.y;
    float alphaMultiplier = 1.0 - forwardTemp.w;

    if (fragDepth < nearestDepth || fragDepth > farthestDepth) {
        // Skip this depth in the peeling algorithm
        vFragColor0.xy = vec2(-MAX_DEPTH);
        return;
    }

    if (fragDepth > nearestDepth && fragDepth < farthestDepth) {
        // This fragment needs to be peeled again
        vFragColor0.xy = vec2(-fragDepth, fragDepth);
        return;
    }

    // If we made it here, this fragment is on the peeled layer from last pass
    // therefore, we need to shade it, and make sure it is not peeled any farther
    vFragColor0.xy = vec2(-MAX_DEPTH);

    //if the fragment depth is the nearest depth, we blend the colour
    //to the second attachment
    vec3 vColor2 = texture(colorTex, fragCoord).rgb;
    //vColor2 *= vColor.rgb;
    float alpha = vColor.a;

    if (fragDepth == nearestDepth) {
        vFragColor1.xyz += vColor2.rgb * alpha * alphaMultiplier;
        vFragColor1.w = 1.0 - alphaMultiplier * (1.0 - alpha);
    } else {
        //otherwise we write to the thrid attachment
        vFragColor2 += vec4(vColor2.rgb, alpha);
    }
}
);

const char* blend_vert = STR(
out vec2 fragCoord;

void main() {
    //get the clip space position from the object space position
    gl_Position = vec4(in_Position, 1);
    fragCoord = in_Texture;
}
);

const char* blend_frag = STR(
uniform sampler2D tempTexture; //intermediate blending result

in vec2 fragCoord;

layout(location = 0) out vec4 vFragColor; //fragment shader output

void main() {
    //return the intermediate blending result
    vFragColor = texture(tempTexture, fragCoord.xy);

    //if the alpha is 0, we discard that fragment
    if(vFragColor.a == 0)
        discard;
}
);

const char* final_frag = STR(
uniform sampler2D depthBlenderTex;  //depth blending output
uniform sampler2D frontBlenderTex;  //front blending output
uniform sampler2D backBlenderTex;   //back blending output

in vec2 fragCoord;

layout(location = 0) out vec4 vFragColor; //fragment shader output

void main() {
    //get the front and back blender colors
    vec4 depthColor = texture(depthBlenderTex, fragCoord.xy);
    vec4 frontColor = texture(frontBlenderTex, fragCoord.xy);
    vec4 backColor = texture(backBlenderTex, fragCoord.xy);
    float alphaMultiplier = 1.0 - frontColor.a;

    // front + back
    //composite the front and back blending results
    //vFragColor.rgb = frontColor.rgb + backColor.rgb * frontColor.a;
    vFragColor.rgb = frontColor.rgb + backColor.rgb * alphaMultiplier;

    // front blender
    //vFragColor.rgb = frontColor.rgb + vec3(alphaMultiplier);

    // back blender
    //vFragColor.rgb = backColor.rgb;
}
);

int main() {
    if (!glfwInit())
            return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Dual Depth Peeling", NULL, NULL);

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

    Font fontRegular = textManager.loadFont("./fonts/OpenSans-Regular.ttf", 48);
    Font fontItalic = textManager.loadFont("./fonts/OpenSans-Italic.ttf", 48);

    int BINDING_POINT_INSTANCE_DATA = 0;
    int BINDING_POINT_FRAME_DATA = 1;
    int BINDING_POINT_LIGHT_DATA = 2;

    Model* sphereModel = modelManager.createSphere("sphere01", 1.0, 20);
    Model* quadModel = modelManager.createQuad("quad");

    Program cubeShader = device.createProgram(commonSource, cube_shader_vert, cube_shader_frag, nullptr);
    Program initShader = device.createProgram(commonSource, cube_shader_vert, dual_init_frag, nullptr);
    Program dualPeelShader = device.createProgram(commonSource, dual_peel_vert, dual_peel_frag, nullptr);
    Program blendShader = device.createProgram(commonSource, blend_vert, blend_frag, nullptr);
    Program finalShader = device.createProgram(commonSource, blend_vert, final_frag, nullptr);

    struct In_InstanceData {
        Matrix4 in_Rotation;
        Vector4 in_Color;
    };

    struct In_FrameData {
        Matrix4 projection;
        Matrix4 view;
    };

    const int NUMBER_SPHERES = 27;
    ConstantBuffer sphere27Instances = device.createConstantBuffer(NUMBER_SPHERES * sizeof(In_InstanceData));
    ConstantBuffer frameConstantBuffer = device.createConstantBuffer(sizeof(In_FrameData));

    device.setConstantBufferBindingPoint(cubeShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(cubeShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    device.setConstantBufferBindingPoint(initShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(initShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    device.setConstantBufferBindingPoint(dualPeelShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(dualPeelShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    device.setTextureBindingPoint(dualPeelShader, "depthBlenderTex", 0);
    device.setTextureBindingPoint(dualPeelShader, "frontBlenderTex", 1);
    device.setTextureBindingPoint(dualPeelShader, "colorTex", 2);

    device.setTextureBindingPoint(blendShader, "tempTexture", 0);

    device.setTextureBindingPoint(finalShader, "depthBlenderTex", 0);
    device.setTextureBindingPoint(finalShader, "frontBlenderTex", 1);
    device.setTextureBindingPoint(finalShader, "backBlenderTex", 2);

    Texture2D stained_glass = textureManager.loadTexture("images/stained_glass.tga");

    In_InstanceData instanceData[27];
    In_FrameData frameData;

    ModelInstance* modelInstance = modelManager.createModelInstance(sphereModel, NUMBER_SPHERES, sphere27Instances, BINDING_POINT_INSTANCE_DATA);

    RenderQueue renderQueue(device, heapAllocator);

    int index = 0;
    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            for(int z = -1; z <= 1; z++) {
                float tx[4] = {
                        (float)x * 2.5f,
                        (float)y * 2.5f,
                        (float)z * 2.5f,
                        1.0f
                };
                mnMatrix4Translate(tx, instanceData[index].in_Rotation.values);

                float alpha = 0.5;

#if 1
                instanceData[index].in_Color = {(float)x, (float)y, (float)z, alpha};
                mnVector3MulScalar(instanceData[index].in_Color.values, 0.5f, instanceData[index].in_Color.values);
                mnVector3AddScalar(instanceData[index].in_Color.values, 0.5f, instanceData[index].in_Color.values);
#else
                float r = index >= 0 && index < 9 ? 1 : 0;
                float g = index >= 9 && index < 18 ? 1 : 0;
                float b = index >= 18 && index < 27 ? 1 : 0;
                instanceData[index].in_Color = {r, g, b, alpha};
#endif

                index++;
            }
        }
    }

    const int WIDTH = 1024;
    const int HEIGHT = 768;

    Rect dualDepthPeelingViewport = {
            0, 0, WIDTH, HEIGHT
    };

    Texture2D frontTexId[2];
    Texture2D backTexId[2];
    Texture2D depthTexId[2];

    Framebuffer dualDepthPeelingFbo = device.createFramebuffer();
    for (int i = 0; i < 2; i++) {
        depthTexId[i] = device.createRG32FTexture(WIDTH, HEIGHT, nullptr);
        frontTexId[i] = device.createRGBAFTexture(WIDTH, HEIGHT, nullptr);
        backTexId[i] = device.createRGBAFTexture(WIDTH, HEIGHT, nullptr);
    }

    int attachID[7] = {0, 3};
    for (int i = 0; i < 2; i++) {
        device.bindTextureToFramebuffer(dualDepthPeelingFbo, depthTexId[i], attachID[i] + 0);
        device.bindTextureToFramebuffer(dualDepthPeelingFbo, frontTexId[i], attachID[i] + 1);
        device.bindTextureToFramebuffer(dualDepthPeelingFbo, backTexId[i], attachID[i] + 2);
    }

    Texture2D backBlenderTexId = device.createRGBFTexture(WIDTH, HEIGHT, nullptr);
    device.bindTextureToFramebuffer(dualDepthPeelingFbo, backBlenderTexId, 6);
    assert(device.isFramebufferComplete(dualDepthPeelingFbo));

    const int LAYERS = 4;

    CommandBuffer* clearColorBuffer = nullptr;
    CommandBuffer* stage1 = nullptr;
    CommandBuffer* stage2[LAYERS] = {nullptr};
    CommandBuffer* stage3[LAYERS] = {nullptr};
    CommandBuffer* stage4 = nullptr;

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

        angle += 0.1 * d;

        float fov = 45 * M_PI / 180.0f;
        float aspect = viewport.width / viewport.height;
        float znear = 0.01;
        float zfar = 30;
        mnMatrix4Perspective(fov, aspect, znear, zfar, frameData.projection.values);
        float eye[3] = {cosf(angle)*10, 0, sinf(angle)*10};
        float at[3] = {0, 0, 0};
        mnMatrix4LookAt(eye, at, frameData.view.values);

        device.copyConstantBuffer(sphere27Instances, instanceData, sizeof(instanceData));
        device.copyConstantBuffer(frameConstantBuffer, &frameData, sizeof(In_FrameData));

        //1. Initialize MIN-MAX buffer
        if (stage1 == nullptr) {
            stage1 = CommandBuffer::create(heapAllocator, 100);
            BindConstantBuffer::create(stage1, sphere27Instances, BINDING_POINT_INSTANCE_DATA);
            BindConstantBuffer::create(stage1, frameConstantBuffer, BINDING_POINT_FRAME_DATA);

            BindFramebuffer::create(stage1, {0});
            SetDrawBuffers::create(stage1, 0xffffffff);
            ClearColor::create(stage1, 0, 0.0f, 0.0f, 0.0f, 1.0f);

            BindFramebuffer::create(stage1, dualDepthPeelingFbo);
            SetViewport::create(stage1, 0, &dualDepthPeelingViewport);
            SetDrawBuffers::create(stage1, (1 << 1) | (1 << 2));
            ClearColor::create(stage1, 0, 0.0f, 0.0f, 0.0f, 0.0f);
            ClearColor::create(stage1, 1, 0.0f, 0.0f, 0.0f, 0.0f);
            SetDrawBuffers::create(stage1, (1 << 0));
            ClearColor::create(stage1, 0, -1.0f, -1.0f, 0.0f, 0.0f);
            SetDepthTest::disable(stage1);
            SetBlend::create(stage1, true, 0, GL_MAX, GL_NONE, GL_NONE);
            SetBlend::disable(stage1, 1);
            SetBlend::disable(stage1, 2);
            BindProgram::create(stage1, initShader);
        }

        ModelInstance::drawNoMaterial(modelInstance, 0, renderQueue, stage1);

        //2. Dual Depth Peeling + Blending
        const float bg[3] = {0.0f, 0.0f, 0.0f};
        if(clearColorBuffer == nullptr) {
            clearColorBuffer = CommandBuffer::create(heapAllocator, 20);
            SetDrawBuffers::create(clearColorBuffer, (1 << 6));
            ClearColor::create(clearColorBuffer, 0, bg[0], bg[1], bg[2], 0);
            BindProgram::create(clearColorBuffer, blendShader);
            BindTexture::create(clearColorBuffer, stained_glass, textureManager.getLinear(), 0);
        }

        Model::draw(quadModel, 0, renderQueue, clearColorBuffer);
        //renderQueue.submit(0, &clearColorBuffer, 1);

        int currId = 0;
        for (int layer = 1; layer < LAYERS; layer++) {
            currId = layer % 2;
            int prevId = 1 - currId;
            int bufId = currId * 3;

            if(stage2[layer] == nullptr) {
                stage2[layer] = CommandBuffer::create(heapAllocator, 100);

                SetDrawBuffers::create(stage2[layer], (1 << bufId) | (1 << (bufId+1)) | (1 << (bufId+2)));
                ClearColor::create(stage2[layer], 0, -1.0f, -1.0f, 0.0f, 0.0f);
                ClearColor::create(stage2[layer], 1, 0.0f, 0.0f, 0.0f, 0.0f);
                ClearColor::create(stage2[layer], 2, 0.0f, 0.0f, 0.0f, 0.0f);

                SetBlend::create(stage2[layer], true, 0, GL_MAX, GL_NONE, GL_NONE);
                SetBlend::create(stage2[layer], true, 1, GL_MAX, GL_NONE, GL_NONE);
                SetBlend::create(stage2[layer], true, 2, GL_MAX, GL_NONE, GL_NONE);

                BindProgram::create(stage2[layer], dualPeelShader);
                BindTexture::create(stage2[layer], depthTexId[prevId], textureManager.getNearest(), 0);
                BindTexture::create(stage2[layer], frontTexId[prevId], textureManager.getNearest(), 1);
                BindTexture::create(stage2[layer], stained_glass, textureManager.getLinear(), 2);
            }

            ModelInstance::drawNoMaterial(modelInstance, 0, renderQueue, stage2[layer]);

            //fullscreen pass
            if (stage3[layer] == nullptr) {
                stage3[layer] = CommandBuffer::create(heapAllocator, 100);
                SetDrawBuffers::create(stage3[layer], (1 << 6));
                SetBlend::create(stage3[layer], true, 0, GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                SetBlend::disable(stage3[layer], 1);
                SetBlend::disable(stage3[layer], 2);
                BindProgram::create(stage3[layer], blendShader);
                BindTexture::create(stage3[layer], backTexId[currId], textureManager.getNearest(), 0);
            }

            Model::draw(quadModel, 0, renderQueue, stage3[layer]);
        }

        //3. Final pass
        if (stage4 == nullptr) {
            stage4 = CommandBuffer::create(heapAllocator, 100);
            BindFramebuffer::create(stage4, {0});
            SetDrawBuffers::create(stage4, 0xffffffff);
            SetViewport::create(stage4, 0, &viewport);
            SetBlend::disable(stage4, 0);
            SetBlend::disable(stage4, 1);
            SetBlend::disable(stage4, 2);
            BindProgram::create(stage4, finalShader);
            BindTexture::create(stage4, depthTexId[currId], {0}, 0);
            BindTexture::create(stage4, frontTexId[currId], {0}, 1);
            BindTexture::create(stage4, backBlenderTexId, {0}, 2);
        }

        Model::draw(quadModel, 0, renderQueue, stage4);

        renderQueue.sendToDevice();

        if (framebufferIndex >= 0 && framebufferIndex <= 6) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, dualDepthPeelingFbo.id); CHECK_ERROR;
            glReadBuffer(GL_COLOR_ATTACHMENT0 + framebufferIndex); CHECK_ERROR;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); CHECK_ERROR;

            glBlitFramebuffer(
                    0, 0, WIDTH, HEIGHT,
                    0, 0, viewport.width, viewport.height,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST
            ); CHECK_ERROR;
        }

        const float color[3] = {1, 1, 1};
        textManager.printText(fontRegular, {0}, color, 10, 20, "Dual Depth Peeling");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    modelManager.destroyModel(sphereModel);
    modelManager.destroyModel(quadModel);
    modelManager.destroyModelInstance(modelInstance);

    textureManager.unloadTexture(stained_glass);

    device.destroyConstantBuffer(sphere27Instances);
    device.destroyConstantBuffer(frameConstantBuffer);

    device.destroyTexture(frontTexId[0]);
    device.destroyTexture(frontTexId[1]);
    device.destroyTexture(backTexId[0]);
    device.destroyTexture(backTexId[1]);
    device.destroyTexture(depthTexId[0]);
    device.destroyTexture(depthTexId[1]);
    device.destroyTexture(backBlenderTexId);

    device.destroyProgram(cubeShader);
    device.destroyProgram(initShader);
    device.destroyProgram(dualPeelShader);
    device.destroyProgram(blendShader);
    device.destroyProgram(finalShader);

    device.destroyFramebuffer(dualDepthPeelingFbo);

    CommandBuffer::destroy(heapAllocator, clearColorBuffer);
    CommandBuffer::destroy(heapAllocator, stage1);
    for (int i = 0; i < LAYERS; i++) {
        if (stage2[i])
            CommandBuffer::destroy(heapAllocator, stage2[i]);
    }
    for (int i = 0; i < LAYERS; i++) {
        if (stage3[i])
            CommandBuffer::destroy(heapAllocator, stage3[i]);
    }
    CommandBuffer::destroy(heapAllocator, stage4);

    return 0;
}
