#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STR(x) #x

void check_error(const char* file, int line) {
    if (glGetError() != GL_NO_ERROR) {
        printf("glError() = %s:%d\n", file, line);
        exit(-1);
    }
}

#include "Allocator.h"
#include "Device.h"
#include "Commands.h"
#include "RenderQueue.h"
#include "Text.h"
#include "Model.h"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

const char* vertexSource = STR(
        layout(location = 0) in vec3 in_Position;
        layout(location = 1) in vec3 in_Normal;
        layout(location = 2) in vec2 in_Texture;
        layout(location = 3) in vec3 in_Color;

        layout(std140) uniform in_ShaderData {
            uniform mat3 in_Rotation;
            uniform vec4 in_Color2;
            uniform float in_Scale[4];
        };

        out vec3 vtx_Position;
        out vec3 vtx_Normal;
        out vec2 vtx_Texture;
        out vec4 vtx_Color;

        void main() {
            vtx_Position = in_Rotation * in_Position * in_Scale[gl_InstanceID];
            gl_Position = vec4(vtx_Position, 1);
            vtx_Texture = in_Texture;
            vtx_Normal = in_Normal;
            vtx_Color = vec4(in_Color, 1);
        }
);

const char* geometrySource = STR(
        layout (triangles) in;
        layout (triangle_strip, max_vertices=3) out;

        in vec3 vtx_Position[];
        in vec3 vtx_Normal[];
        in vec2 vtx_Texture[];
        in vec4 vtx_Color[];

        out vec3 geo_Position;
        out vec3 geo_Normal;
        out vec2 geo_Texture;
        out vec4 geo_Color;

        void main() {
            gl_Position = gl_in[0].gl_Position;
            geo_Position = vtx_Position[0];
            geo_Normal = vtx_Normal[0];
            geo_Texture = vtx_Texture[0];
            geo_Color = vtx_Color[0];
            EmitVertex();

            gl_Position = gl_in[1].gl_Position;
            geo_Position = vtx_Position[1];
            geo_Normal = vtx_Normal[1];
            geo_Texture = vtx_Texture[1];
            geo_Color = vtx_Color[1];
            EmitVertex();

            gl_Position = gl_in[2].gl_Position;
            geo_Position = vtx_Position[2];
            geo_Normal = vtx_Normal[2];
            geo_Texture = vtx_Texture[2];
            geo_Color = vtx_Color[2];
            EmitVertex();

            EndPrimitive();
        }
);

const char* fragmentSource = STR(
        in vec3 geo_Position;
        in vec3 geo_Normal;
        in vec2 geo_Texture;
        in vec4 geo_Color;

        layout(std140) uniform in_ShaderData {
            uniform mat3 in_Rotation;
            uniform vec4 in_Color2;
            uniform float in_Scale[4];
        };

        uniform sampler2D in_Sampler;

        layout(location = 0) out vec3 out_Position;
        layout(location = 1) out vec3 out_Normal;
        layout(location = 2) out vec4 out_Albedo;

        void main() {
            out_Position = geo_Position;
            out_Normal = normalize(geo_Normal);
            out_Albedo.rgb = (texture(in_Sampler, geo_Texture)).rgb; // * geo_Color * in_Color2).rgb;
            out_Albedo.w = 1;
        }
);

const char* quadVertexSource = STR(
        layout(location = 0) in vec3 in_Position;
        layout(location = 1) in vec2 in_Texture;

        out vec2 vtx_Texture;

        void main() {
            gl_Position = vec4(in_Position, 1);
            vtx_Texture = in_Texture;
        }
);

const char* quadGeometrySource = STR(
        layout (triangles) in;
        layout (triangle_strip, max_vertices=4) out;

        in vec2 vtx_Texture[];

        out vec2 geo_Texture;

        void main() {
            gl_Position = gl_in[0].gl_Position;
            geo_Texture = vtx_Texture[0];
            EmitVertex();

            gl_Position = gl_in[1].gl_Position;
            geo_Texture = vtx_Texture[1];
            EmitVertex();

            gl_Position = (gl_in[0].gl_Position + gl_in[2].gl_Position) * 0.5;
            geo_Texture = (vtx_Texture[0] + vtx_Texture[2]) * 0.5;
            EmitVertex();

            gl_Position = gl_in[2].gl_Position;
            geo_Texture = vtx_Texture[2];
            EmitVertex();

            EndPrimitive();
        }
);

const char* quadFragmentSource = STR(
        in vec2 geo_Texture;

        uniform sampler2D in_Position;
        uniform sampler2D in_Normal;
        uniform sampler2D in_Albedo;

        layout(location = 0) out vec4 out_FragColor;

        vec3 calculateLight(vec3 position, vec3 normal, vec4 albedo, vec3 lightPos, vec3 lightColor) {
            vec3 lightVec = lightPos - position;

            if(length(lightVec) < 1.5) {
                vec3 lightDir = normalize(lightVec);
                return max(dot(normal, lightDir), 0) * albedo.rgb * lightColor;
            }

            return vec3(0);
        }

        void main() {
            vec3 vewPos = vec3(0, 0, 0);
            vec3 lightPos0 = vec3(0, 1.2, 1);
            vec3 lightPos1 = vec3(+1.2, 0, 1);
            vec3 lightPos2 = vec3(-1.2, 0, 1);

            vec3 position = texture(in_Position, geo_Texture).rgb;
            vec3 normal = texture(in_Normal, geo_Texture).rgb;
            vec4 albedo = texture(in_Albedo, geo_Texture);

            vec3 lighting = albedo.rgb * 0.2;
            lighting += calculateLight(position, normal, albedo, lightPos0, vec3(1, 0, 0));
            lighting += calculateLight(position, normal, albedo, lightPos1, vec3(0, 1, 0));
            lighting += calculateLight(position, normal, albedo, lightPos2, vec3(0, 0, 1));

            out_FragColor.rgb = lighting;
            out_FragColor.a = 1;
        }
);

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

    const size_t size = 1024 * 1024;
    int8_t* data = new int8_t[size];
    LinearAllocator allocator(data, size);

    Device device;

    Font fontRegular(device, "./fonts/OpenSans-Regular.ttf");
    Font fontItalic(device, "./fonts/OpenSans-Italic.ttf");

    Program program = device.createProgram(vertexSource, fragmentSource, geometrySource);

    struct {
        float in_Rotation[12];
        float in_Color2[4];
        float in_Scale0[4];
        float in_Scale1[4];
        float in_Scale2[4];
        float in_Scale3[4];
    } in_vertexData = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            1, 1, 1, 1,
            1.00, 0, 0, 0,
            0.75, 0, 0, 0,
            0.50, 0, 0, 0,
            0.25, 0, 0, 0,
    };

    int bindingPoint = 0;
    device.setConstantBufferBinding(program, "in_ShaderData", bindingPoint);
    ConstantBuffer constantBuffer = device.createConstantBuffer(sizeof(in_vertexData), &in_vertexData, bindingPoint);

    const uint32_t black = 0xff000000;
    const uint32_t white = 0xffffffff;

    const uint32_t pixels0[] = {
            black, white, black, white,
            white, black, white, black,
            black, white, black, white,
            white, black, white, black,
    };
    Texture2D texture0 = device.createTexture(4, 4, pixels0);
    Sampler sampler0 = device.createSampler(GL_NEAREST);
    Material* material0 = Material::create(allocator, program, texture0, sampler0, device.getUniformLocation(program, "in_Sampler"));

    const uint32_t pixels1[] = {
            black, white, black, white, black, white, black, white,
            white, black, white, black, white, black, white, black,
            black, white, black, white, black, white, black, white,
            white, black, white, black, white, black, white, black,
            black, white, black, white, black, white, black, white,
            white, black, white, black, white, black, white, black,
            black, white, black, white, black, white, black, white,
            white, black, white, black, white, black, white, black,
    };
    Texture2D texture1 = device.createTexture(8, 8, pixels1);
    Sampler sampler1 = device.createSampler(GL_LINEAR);
    Material* material1 = Material::create(allocator, program, texture1, sampler1, device.getUniformLocation(program, "in_Sampler"));

    float vertexData[] = {
            -0.5, -0.5, 0.0,
            -0.5, +0.5, 0.0,
            +0.5, +0.5, 0.0,
            +0.5, -0.5, 0.0,
    };
    float normalData[] = {
            -1.0, -1.0, 0.5,
            -1.0, +1.0, 0.5,
            +1.0, +1.0, 0.5,
            +1.0, -1.0, 0.5,
    };
    float textureData[] = {
            0, 0,
            0, 1,
            1, 1,
            1, 0
    };
    float colorData[] = {
            1, 1, 1,
            1, 1, 1,
            1, 1, 1,
            1, 1, 1,
    };
    uint16_t indexData[] = {0, 1, 3, 3, 1, 2};

    VertexBuffer vertexBuffer = device.createStaticVertexBuffer(sizeof(vertexData), vertexData);
    VertexBuffer normalBuffer = device.createStaticVertexBuffer(sizeof(normalData), normalData);
    VertexBuffer textureBuffer = device.createStaticVertexBuffer(sizeof(textureData), textureData);
    VertexBuffer colorBuffer = device.createStaticVertexBuffer(sizeof(colorData), colorData);
    IndexBuffer indexBuffer = device.createIndexBuffer(sizeof(indexData), indexData);

    VertexDeclarationDesc vertexDeclaration[4] = {};
    vertexDeclaration[0].buffer = vertexBuffer;
    vertexDeclaration[0].format = VertexFloat3;
    vertexDeclaration[0].offset = 0;
    vertexDeclaration[0].stride = 0;//sizeof(float)*3;

    vertexDeclaration[1].buffer = normalBuffer;
    vertexDeclaration[1].format = VertexFloat3;
    vertexDeclaration[1].offset = 0;
    vertexDeclaration[1].stride = 0;//sizeof(float)*3;

    vertexDeclaration[2].buffer = textureBuffer;
    vertexDeclaration[2].format = VertexFloat2;
    vertexDeclaration[2].offset = 0;
    vertexDeclaration[2].stride = 0;//sizeof(float)*2;

    vertexDeclaration[3].buffer = colorBuffer;
    vertexDeclaration[3].format = VertexFloat3;
    vertexDeclaration[3].offset = 0;
    vertexDeclaration[3].stride = 0;//sizeof(float)*3;

    VertexArray vertexArray = device.createVertexArray(vertexDeclaration, 4, indexBuffer);

    Model* model0 = Model::create(allocator, vertexArray, 1);
    model0->meshes[0] = Mesh::create(allocator, 0, 3);
    ModelInstance* modelInstance0 = ModelInstance::create(allocator, model0);
    modelInstance0->materials[0] = material0;

    Model* model1 = Model::create(allocator, vertexArray, 1);
    model1->meshes[0] = Mesh::create(allocator, 3, 3);
    ModelInstance* modelInstance1 = ModelInstance::create(allocator, model1);
    modelInstance1->materials[0] = material1;

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    RenderQueue renderQueue(device);

    int wgbuffer = 512;
    int hgbuffer = 512;
    Framebuffer gBuffer = device.createFramebuffer();
    Texture2D position = device.createFloat32Texture(wgbuffer, hgbuffer, nullptr);
    Texture2D normal = device.createFloat32Texture(wgbuffer, hgbuffer, nullptr);
    Texture2D albedo = device.createTexture(wgbuffer, hgbuffer, nullptr);
    Renderbuffer depth = device.createRenderbuffer(wgbuffer, hgbuffer);

    device.bindTextureToFramebuffer(gBuffer, position, 0);
    device.bindTextureToFramebuffer(gBuffer, normal, 1);
    device.bindTextureToFramebuffer(gBuffer, albedo, 2);
    device.bindRenderbufferToFramebuffer(gBuffer, depth);

    if (!device.isFramebufferComplete(gBuffer))
        return -1;

    int targets[] = { 0, 1, 2 };
    device.setRenderTarget(gBuffer, targets, 3);

    Framebuffer nullFramebuffer = {0};

    float quadVertex[] = {
            -1.0, -1.0, 0.0,
            -1.0, +1.0, 0.0,
            +1.0, +1.0, 0.0,
            +1.0, -1.0, 0.0,
    };
    float quadTexture[] = {
            0, 0,
            0, 1,
            1, 1,
            1, 0
    };
    uint16_t quadIndex[] = {0, 1, 3, 3, 1, 2};

    VertexBuffer quadVertexBuffer = device.createStaticVertexBuffer(sizeof(quadVertex), quadVertex);
    VertexBuffer quadTextureBuffer = device.createStaticVertexBuffer(sizeof(quadTexture), quadTexture);
    IndexBuffer quadIndexBuffer = device.createIndexBuffer(sizeof(quadIndex), quadIndex);

    VertexDeclarationDesc vertexDeclarationQuad[2] = {};
    vertexDeclarationQuad[0].buffer = quadVertexBuffer;
    vertexDeclarationQuad[0].format = VertexFloat3;
    vertexDeclarationQuad[0].offset = 0;
    vertexDeclarationQuad[0].stride = 0;

    vertexDeclarationQuad[1].buffer = quadTextureBuffer;
    vertexDeclarationQuad[1].format = VertexFloat2;
    vertexDeclarationQuad[1].offset = 0;
    vertexDeclarationQuad[1].stride = 0;

    VertexArray quadVertexArray = device.createVertexArray(vertexDeclarationQuad, 2, quadIndexBuffer);
    Program quadProgram = device.createProgram(quadVertexSource, quadFragmentSource, quadGeometrySource);

    CommandBuffer* setGBuffer = CommandBuffer::create(allocator, 4);
    setGBuffer->commands[0] = CopyConstantBuffer::create(allocator, constantBuffer, &in_vertexData, sizeof(in_vertexData));
    setGBuffer->commands[1] = BindFramebuffer::create(allocator, gBuffer);
    setGBuffer->commands[2] = SetViewport::create(allocator, 0, 0, wgbuffer, hgbuffer);
    setGBuffer->commands[3] = ClearColor::create(allocator, 0.25, 0.25, 0.25, 1);

    CommandBuffer* drawQuad = CommandBuffer::create(allocator, 8);
    drawQuad->commands[0] = BindFramebuffer::create(allocator, nullFramebuffer);
    drawQuad->commands[1] = SetViewport::create(allocator, 0, 0, width, height);
    drawQuad->commands[2] = BindProgram::create(allocator, quadProgram);
    drawQuad->commands[3] = BindTexture::create(allocator, quadProgram, position, device.getUniformLocation(quadProgram, "in_Position"));
    drawQuad->commands[4] = BindTexture::create(allocator, quadProgram, normal, device.getUniformLocation(quadProgram, "in_Normal"));
    drawQuad->commands[5] = BindTexture::create(allocator, quadProgram, albedo, device.getUniformLocation(quadProgram, "in_Albedo"));
    drawQuad->commands[6] = BindVertexArray::create(allocator, quadVertexArray);
    drawQuad->commands[7] = DrawTriangles::create(allocator, 0, 6);

    float angle = 0;
    while (!glfwWindowShouldClose(window)) {
        float cos = cosf(angle);
        float sin = sinf(angle);

        angle += 0.005;

        //rotate z-axis
        in_vertexData.in_Rotation[0] = cos;
        in_vertexData.in_Rotation[1] = -sin;
        in_vertexData.in_Rotation[4] = sin;
        in_vertexData.in_Rotation[5] = cos;

        //rotate y-axis
//        in_vertexData.in_Rotation[0] = cos;
//        in_vertexData.in_Rotation[2] = sin;
//        in_vertexData.in_Rotation[7] = -sin;
//        in_vertexData.in_Rotation[9] = cos;

        in_vertexData.in_Color2[0] -= 0.00001;
        in_vertexData.in_Color2[1] -= 0.00002;
        in_vertexData.in_Color2[2] -= 0.00003;

        modelInstance0->draw(2, renderQueue, setGBuffer);
        modelInstance1->draw(1, renderQueue, setGBuffer);

        renderQueue.submit(3, &drawQuad, 1);

        renderQueue.submit();

        fontItalic.printText(10, 180, "My font example");
        fontRegular.printText(10, 130, "Memory used %ld bytes", allocator.memoryUsed());
        float totalCommands = renderQueue.getExecutedCommands() + renderQueue.getSkippedCommands();
        fontRegular.printText(10, 80, "Executed commands %d | %.2f%% executed",
                              renderQueue.getExecutedCommands(), renderQueue.getExecutedCommands() / totalCommands * 100);
        fontRegular.printText(10, 30, "Skipped commands %d | %.2f%% ignored",
                              renderQueue.getSkippedCommands(), renderQueue.getSkippedCommands() / totalCommands * 100);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
