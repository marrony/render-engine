//
// Created by Marrony Neris on 12/15/15.
//

#include "Text.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define STR(x) #x

const char* vertexFontSource = STR(
        layout(location = 0) in vec4 in_Vertex;

        out vec2 var_Texture;

        void main() {
            gl_Position = vec4(in_Vertex.xy, 0.0, 1.0);
            var_Texture = in_Vertex.zw;
        }
);

const char* fragmentFontSource = STR(
        in vec2 var_Texture;

        uniform sampler2D in_Sampler;
        uniform vec3 in_Color;

        out vec4 out_FragColor;

        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(in_Sampler, var_Texture).r);
            out_FragColor = vec4(in_Color, 1.0) * sampled;
        }
);

const char* commonFontSource = "";

#undef STR

TextManager::TextManager(HeapAllocator& allocator, Device& device)
        :  allocator(allocator), device(device), fontCount(0) {
    vertexBuffer = device.createDynamicVertexBuffer(sizeof(float) * 4 * 4, nullptr);

    VertexDeclaration vertexDeclaration[1] = {};
    vertexDeclaration[0].buffer = vertexBuffer;
    vertexDeclaration[0].format = VertexFloat4;
    vertexDeclaration[0].offset = 0;
    vertexDeclaration[0].stride = 0;

    IndexBuffer indexBuffer = {0};
    vertexArray = device.createVertexArray(vertexDeclaration, 1, indexBuffer);
    program = device.createProgram(commonFontSource, vertexFontSource, fragmentFontSource);
}

TextManager::~TextManager() {
    for(int i = 0; i < fontCount; i++) {
        FontFace* font = fonts[i];

        for(int ch = 0; ch < 128; ch++)
            device.destroyTexture(font->characters[ch].texture);

        allocator.deallocate(font);
    }

    device.destroyVertexBuffer(vertexBuffer);
    device.destroyVertexArray(vertexArray);
    device.destroyProgram(program);
}

void TextManager::printText(Font fontId, Framebuffer framebuffer, const float color[3], float x, float y, const char* fmt, ...) {
    char text[1024];

    va_list list;
    va_start(list, fmt);
    vsnprintf(text, 1024, fmt, list);
    va_end(list);

    size_t size = strlen(text);

    device.bindProgram(program);
    device.setValue(program, "in_Color", color[0], color[1], color[2]); //TODO remove this call
    device.bindVertexArray(vertexArray);

    int width = 0;
    int height = 0;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetFramebufferSize(window, &width, &height);

    device.bindDrawFramebuffer(framebuffer);
    glViewport(0, 0, width, height);

    float invw = 1.0f / width;
    float invh = 1.0f / height;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Sampler sampler = {0};
    device.bindSampler(sampler, 0);

    int in_Sampler = 0;
    device.setTextureBindingPoint(program, "in_Sampler", in_Sampler);

    FontFace* font = fonts[fontId.id];

    float scale = 1.0;
    for (size_t i = 0; i < size; i++) {
        Character& ch = font->characters[text[i]];

        GLfloat xpos = x + ch.bearing[0] * scale;
        GLfloat ypos = y - (ch.size[1] - ch.bearing[1]) * scale;

        GLfloat w = ch.size[0] * scale;
        GLfloat h = ch.size[1] * scale;

        GLfloat vertices[4][4] = {
                {xpos,     ypos,     0.0, 1.0},
                {xpos,     ypos + h, 0.0, 0.0},

                {xpos + w, ypos,     1.0, 1.0},
                {xpos + w, ypos + h, 1.0, 0.0},
        };

        for (int i = 0; i < 6; i++) {
            vertices[i][0] = (vertices[i][0] * invw * 2.0f) - 1.0f;
            vertices[i][1] = (vertices[i][1] * invh * 2.0f) - 1.0f;
        }

        device.updateVertexBuffer(vertexBuffer, 0, sizeof(vertices), vertices);

        device.bindTexture(ch.texture, in_Sampler);

        device.drawArraysTriangleStrip(0, 4);

        x += ch.advance * scale;
    }

    glDisable(GL_BLEND);
}

Font TextManager::loadFont(const char* fontface, int height) {
    for(int i = 0; i < fontCount; i++) {
        if(strcmp(fontface, fonts[i]->fontface) == 0 && height == fonts[i]->height)
            return {i};
    }

    FontFace* font = (FontFace*) allocator.allocate(sizeof(FontFace));
    int fontId = fontCount++;

    fonts[fontId] = font;

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        printf("Could not init FreeType Library\n");
        exit(-1);
    }

    FT_Face face;
    if (FT_New_Face(ft, fontface, 0, &face)) {
        printf("Failed to load font\n");
        exit(-1);
    }

    FT_Set_Pixel_Sizes(face, 0, height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    font->fontface = fontface;
    font->height = height;

    for (int ch = 0; ch < 128; ch++) {
        if (FT_Load_Char(face, ch, FT_LOAD_RENDER)) {
            printf("Failed to load Glyph\n");
            continue;
        }

        Texture2D texture = device.createRTexture(face->glyph->bitmap.width,
                                                  face->glyph->bitmap.rows,
                                                  face->glyph->bitmap.buffer);
        font->characters[ch].texture = texture;
        font->characters[ch].size[0] = face->glyph->bitmap.width;
        font->characters[ch].size[1] = face->glyph->bitmap.rows;
        font->characters[ch].bearing[0] = face->glyph->bitmap_left;
        font->characters[ch].bearing[1] = face->glyph->bitmap_top;
        font->characters[ch].advance = face->glyph->advance.x >> 6;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return {fontId};
}
