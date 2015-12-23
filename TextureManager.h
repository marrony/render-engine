//
// Created by Marrony Neris on 11/23/15.
//

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

struct Image {
    int width;
    int height;
    int format;
    char* pixels;
};

#include "TgaReader.h"

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

        Image tgaImage;

        FILE* stream = fopen(filename, "rb");
        assert(stream != nullptr);
        readTga(allocator, stream, tgaImage);
        fclose(stream);

        assert(tgaImage.format == 1 || tgaImage.format == 3 || tgaImage.format == 4);

        textures[index].refs = 1;
        textures[index].filename = filename;
        switch(tgaImage.format) {
        case 1:
            textures[index].texture = device.createRTexture(tgaImage.width, tgaImage.height, tgaImage.pixels);
            break;
        case 3:
            textures[index].texture = device.createRGBTexture(tgaImage.width, tgaImage.height, tgaImage.pixels);
            break;
        case 4:
            textures[index].texture = device.createRGBATexture(tgaImage.width, tgaImage.height, tgaImage.pixels);
            break;
        }

        allocator.deallocate(tgaImage.pixels);

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
