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

class Allocator {
public:
    Allocator(uint8_t* begin, size_t size) : begin(begin), end(begin + size), current(begin) { }

    void* allocate(size_t size) {
        if (current + size > end)
            return nullptr;

        void* data = current;
        current += size;
        return data;
    }

    size_t memoryUsed() {
        return current - begin;
    }

private:
    uint8_t* begin;
    uint8_t* end;
    uint8_t* current;
};

#include "Device.h"
#include "Commands.h"
#include "RenderQueue.h"
#include "Text.h"

struct Mesh {
    State* state;
    StDrawCommand* draw;

    static Mesh* create(Allocator& allocator, int offset, int count) {
        Mesh* mesh = (Mesh*) allocator.allocate(sizeof(Mesh));

        mesh->state = State::create(allocator, 0);
        mesh->draw = DrawTriangles::create(allocator, offset, count);

        return mesh;
    }
};

struct Model {
    State* state;
    int meshCount;
    Mesh* meshes[];

    static Model* create(Allocator& allocator, VertexArray vertexArray, int meshCount) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh*));

        model->state = State::create(allocator, 1);
        model->state->commands[0] = BindVertexArray::create(allocator, vertexArray);
        model->meshCount = meshCount;

        return model;
    }
};

struct Material {
    State* state;

    static Material* create(Allocator& allocator, Program program, Texture2D texture, Sampler sampler, ConstantBuffer constantBuffer, const void* data, size_t size) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = State::create(allocator, 4);
        material->state->commands[0] = BindProgram::create(allocator, program);
        material->state->commands[1] = BindTexture::create(allocator, program, "in_Sampler", texture, 0);
        material->state->commands[2] = BindSampler::create(allocator, program, sampler, 0);
        material->state->commands[3] = CopyConstantBuffer::create(allocator, program, constantBuffer, data, size);

        return material;
    }
};

struct ModelInstance {
    State* state;
    Model* model;
    Material* materials[];

    void draw(RenderQueue& renderQueue) {
        for (int i = 0; i < model->meshCount; i++) {
            Mesh* mesh = model->meshes[i];
            Material* material = materials[i];

            State* states[] = {
                    state,
                    model->state,
                    material->state,
                    mesh->state,
            };

            renderQueue.submit(mesh->draw, states, 4);
        }
    }

    static ModelInstance* create(Allocator& allocator, Model* model) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(Material*);
        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = State::create(allocator, 0);
        modelInstance->model = model;

        return modelInstance;
    }
};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

const char* vertexSource = STR(
        layout(location = 0) in vec3 in_Position;
        layout(location = 1) in vec2 in_Texture;
        layout(location = 2) in vec3 in_Color;

        layout(std140) uniform in_ShaderData {
            uniform mat3 in_Rotation;
            uniform vec4 in_Color2;
        };

        out vec2 var_Texture;
        out vec4 var_Color;

        void main() {
            gl_Position = vec4(in_Rotation * in_Position, 1);
            var_Texture = in_Texture;
            var_Color = vec4(in_Color, 1);
        }
);

const char* fragmentSource = STR(
        in vec2 var_Texture;
        in vec4 var_Color;

        layout(std140) uniform in_ShaderData {
            uniform mat3 in_Rotation;
            uniform vec4 in_Color2;
        };

        uniform sampler2D in_Sampler;

        out vec4 out_FragColor;

        void main() {
            out_FragColor = texture(in_Sampler, var_Texture) * var_Color * in_Color2;
        }
);

int main(int argc, char* argv[]) {
    char buffer[1000];
    getcwd(buffer, 1000);

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
    uint8_t* data = new uint8_t[size];
    Allocator allocator(data, size);

    Device device;

    Font fontRegular(device, "./fonts/OpenSans-Regular.ttf");
    Font fontItalic(device, "./fonts/OpenSans-Italic.ttf");

    Program program = device.createProgram(vertexSource, fragmentSource);

    struct {
        float in_Rotation[12];
        float in_Color2[4];
    } in_vertexData = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            1, 1, 1, 1
    };

    int bindingPoint = 0;
    int index = glGetUniformBlockIndex(program.id, "in_ShaderData");
    glUniformBlockBinding(program.id, index, bindingPoint);

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
    Material* material0 = Material::create(allocator, program, texture0, sampler0, constantBuffer, &in_vertexData, sizeof(in_vertexData));

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
    Material* material1 = Material::create(allocator, program, texture1, sampler1, constantBuffer, &in_vertexData, sizeof(in_vertexData));

    float vertexData[] = {
            -0.5, -0.5, 0.0,
            -0.5, +0.5, 0.0,
            +0.5, +0.5, 0.0,
            +0.5, -0.5, 0.0,
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

    VertexBuffer vertexBuffer = device.createVertexBuffer(sizeof(vertexData), vertexData);
    VertexBuffer textureBuffer = device.createVertexBuffer(sizeof(textureData), textureData);
    VertexBuffer colorBuffer = device.createVertexBuffer(sizeof(colorData), colorData);
    IndexBuffer indexBuffer = device.createIndexBuffer(sizeof(indexData), indexData);

    VertexDeclarationDesc vertexDeclaration[3] = {};
    vertexDeclaration[0].buffer = vertexBuffer;
    vertexDeclaration[0].format = VertexFloat3;
    vertexDeclaration[0].offset = 0;
    vertexDeclaration[0].stride = 0;//sizeof(float)*3;
    vertexDeclaration[1].buffer = textureBuffer;
    vertexDeclaration[1].format = VertexFloat2;
    vertexDeclaration[1].offset = 0;
    vertexDeclaration[1].stride = 0;//sizeof(float)*2;
    vertexDeclaration[2].buffer = colorBuffer;
    vertexDeclaration[2].format = VertexFloat3;
    vertexDeclaration[2].offset = 0;
    vertexDeclaration[2].stride = 0;//sizeof(float)*3;
    VertexArray vertexArray = device.createVertexArray(vertexDeclaration, 3, indexBuffer);

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

    StStateCommand* setViewport0 = SetViewport::create(allocator, 0.5, 0.5, 1.0, 1.0);
    StStateCommand* setViewport1 = SetViewport::create(allocator, 0.0, 0.0, 0.5, 0.5);
    StStateCommand* setViewport2 = SetViewport::create(allocator, 0.0, 0.5, 0.5, 1.0);
    StStateCommand* setViewport3 = SetViewport::create(allocator, 0.5, 0.0, 1.0, 0.5);
    StStateCommand* setViewport4 = SetViewport::create(allocator, 0.25, 0.25, 0.75, 0.75);
    StStateCommand* setViewport5 = SetViewport::create(allocator, 0, 0, 1, 1);

    StStateCommand* clearColor = ClearColor::create(allocator, 0.15, 0.15, 0.15, 1);
    StStateCommand* clearColor2 = ClearColor::create(allocator, 0, 1, 0.15, 1);

    RenderQueue renderQueue(device);

    float angle = 0;
    while (!glfwWindowShouldClose(window)) {
        float cos = cosf(angle);
        float sin = sinf(angle);

        angle += 0.005;

        in_vertexData.in_Rotation[0] = cos;
        in_vertexData.in_Rotation[1] = -sin;
        in_vertexData.in_Rotation[4] = sin;
        in_vertexData.in_Rotation[5] = cos;

        in_vertexData.in_Color2[0] -= 0.00001;
        in_vertexData.in_Color2[1] -= 0.00002;
        in_vertexData.in_Color2[2] -= 0.00003;

        renderQueue.submit(&setViewport0->command);
        renderQueue.submit(&clearColor->command);
        modelInstance0->draw(renderQueue);

        renderQueue.submit(&setViewport1->command);
        renderQueue.submit(&clearColor->command);
        modelInstance1->draw(renderQueue);

        renderQueue.submit(&setViewport2->command);
        renderQueue.submit(&clearColor->command);
        modelInstance1->draw(renderQueue);

        renderQueue.submit(&setViewport3->command);
        renderQueue.submit(&clearColor->command);
        modelInstance0->draw(renderQueue);

        renderQueue.submit(&setViewport4->command);
        renderQueue.submit(&clearColor2->command);
        modelInstance0->draw(renderQueue);
        modelInstance1->draw(renderQueue);

        renderQueue.submit(&setViewport5->command);

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
