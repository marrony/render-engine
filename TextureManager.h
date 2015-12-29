//
// Created by Marrony Neris on 11/23/15.
//

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <type_traits>

struct Image {
    int width;
    int height;
    int format;
    uint8_t* pixels;
};

void loadImage(HeapAllocator& allocator, const char* filename, Image& image) {
    FILE* file = fopen(filename, "rb");

    assert(file != nullptr);

    fread(&image.width, sizeof(int), 1, file);
    fread(&image.height, sizeof(int), 1, file);
    fread(&image.format, sizeof(int), 1, file);
    image.pixels = (uint8_t*)allocator.allocate(image.width*image.height*image.format);
    fread(image.pixels, sizeof(uint8_t), image.width*image.height*image.format, file);

    fclose(file);
}

void saveImage(const char* filename, Image& image) {
    FILE* file = fopen(filename, "wb");

    assert(file != nullptr);

    fwrite(&image.width, sizeof(int), 1, file);
    fwrite(&image.height, sizeof(int), 1, file);
    fwrite(&image.format, sizeof(int), 1, file);
    fwrite(image.pixels, sizeof(uint8_t), image.width*image.height*image.format, file);

    fclose(file);
}

#include "TgaReader.h"
#include "JpegReader.h"

class TextureManager {
public:
    TextureManager(HeapAllocator& allocator, Device& device) : allocator(allocator), device(device) {
        linear = device.createSampler(GL_LINEAR, GL_LINEAR);
        nearest = device.createSampler(GL_NEAREST, GL_NEAREST);

        textureCount = 0;
        textureAllocated = 16;
        textures = (Resource*) allocator.allocate(textureAllocated * sizeof(Resource));
        memset(textures, 0, textureAllocated * sizeof(Resource));
    }

    ~TextureManager() {
        assert(textureCount == 0);

        allocator.deallocate(textures);

        device.destroySampler(linear);
        device.destroySampler(nearest);
    }

    Texture2D loadTexture(const char* filename) {
        uint32_t index;

        if(findTexture(filename, index)) {
            textures[index].refs++;
            return textures[index].texture;
        }

        if(textureCount >= textureAllocated) {
            textureAllocated = textureAllocated * 3 / 2;
            textures = (Resource*) allocator.reallocate(textures, textureAllocated * sizeof(Resource));
        }

        index = textureCount++;

        Image image;

        FILE* stream = fopen(filename, "rb");
        assert(stream != nullptr);

        if(strstr(filename, ".tga") != nullptr)
            readTga(allocator, stream, image);
        else if(strstr(filename, ".jpg") != nullptr)
            readJpeg(allocator, stream, image);

        fclose(stream);

        assert(image.format == 1 || image.format == 3 || image.format == 4);

        textures[index].refs = 1;
        textures[index].filename = filename;
        switch(image.format) {
        case 1:
            textures[index].texture = device.createRTexture(image.width, image.height, image.pixels);
            break;
        case 3:
            textures[index].texture = device.createRGBTexture(image.width, image.height, image.pixels);
            break;
        case 4:
            textures[index].texture = device.createRGBATexture(image.width, image.height, image.pixels);
            break;
        }

        allocator.deallocate(image.pixels);

        return textures[index].texture;
    }

    void unloadTexture(Texture2D texture) {
        uint32_t index;

        if (findTexture(texture, index)) {
            textures[index].refs--;

            if (textures[index].refs == 0) {
                destroy(&textures[index]);

                textureCount--;
                std::swap(textures[index], textures[textureCount]);
            }
        }
    }

    Sampler getLinear() {
        return linear;
    }

    Sampler getNearest() {
        return nearest;
    }
private:
    struct Resource {
        const char* filename;
        Texture2D texture;
        uint32_t refs;
    };

    void destroy(Resource* resource) {
        device.destroyTexture(resource->texture);
    }

    bool findTexture(Texture2D texture, uint32_t& index) {
        for(uint32_t i = 0; i < textureCount; i++) {
            if(textures[i].texture.id == texture.id) {
                index = i;
                return true;
            }
        }
        return false;
    }

    bool findTexture(const char* filename, uint32_t& index) {
        for(uint32_t i = 0; i < textureCount; i++) {
            if(textures[i].filename != nullptr && strcmp(textures[i].filename, filename) == 0) {
                index = i;
                return true;
            }
        }
        return false;
    }

    HeapAllocator& allocator;
    Device& device;

    Resource* textures;
    uint32_t textureCount;
    uint32_t textureAllocated;

    Sampler linear;
    Sampler nearest;
};

#endif //TEXTURE_MANAGER_H
