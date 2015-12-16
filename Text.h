//
// Created by Marrony Neris on 11/7/15.
//

#ifndef TEXT_H
#define TEXT_H

#include "Device.h"
#include "Allocator.h"

struct Font {
    int id;
};

class TextManager {
public:
    TextManager(HeapAllocator& allocator, Device& device);

    ~TextManager();

    void printText(Font fontId, Framebuffer framebuffer, const float color[3], float x, float y, const char* fmt, ...);

    Font loadFont(const char* fontface, int height);
private:
    struct Character {
        Texture2D texture;
        int size[2];
        int bearing[2];
        int advance;
    };

    struct FontFace {
        int height;
        const char* fontface;
        Character characters[128];
    };

    HeapAllocator& allocator;
    Device& device;

    VertexArray vertexArray;
    VertexBuffer vertexBuffer;
    Program program;

    FontFace* fonts[10];
    int fontCount;
};

#endif //TEXT_H
