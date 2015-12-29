//
// Created by Marrony Neris on 12/18/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <chrono>
#include <thread>

#include "Vector.h"
#include "Matrix.h"
#include "Device.h"
#include "Allocator.h"
#include "TextureManager.h"

void imageGet3(Image& image, int x, int y, uint8_t pixels[3]) {
    int offsety = y*image.format*image.width;
    int offsetx = x*image.format;

    pixels[0] = image.pixels[offsety + offsetx + 0];
    pixels[1] = image.pixels[offsety + offsetx + 1];
    pixels[2] = image.pixels[offsety + offsetx + 2];
}

void imageSet3(Image& image, int x, int y, uint8_t pixels[3]) {
    int offsety = y*image.format*image.width;
    int offsetx = x*image.format;

    image.pixels[offsety + offsetx + 0] = pixels[0];
    image.pixels[offsety + offsetx + 1] = pixels[1];
    image.pixels[offsety + offsetx + 2] = pixels[2];
}

void imageSet3(Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t pixels[3] = {r, g, b};
    imageSet3(image, x, y, pixels);
}

void sampleFace(int face, Image input[6], float sc, float tc, float ma, float color[3]) {
    ma = fabs(ma);

    float s = (sc/ma + 1) * 0.5;
    float t = (tc/ma + 1) * 0.5;

//    printf("[%f %f] %f [%f %f]\n", sc, tc, ma, s, t);

    int x = s * (input[face].width - 1);
    int y = t * (input[face].height - 1);

    assert(x < input[face].width);
    assert(y < input[face].height);

    uint8_t pixels[3];
    imageGet3(input[face], x, y, pixels);

    //printf("%d [%d %d] [%d %d %d] [%d %d %d]\n", face, xx, yy, r, g, b, pixels[r], pixels[1], pixels[2]);

    color[0] = float(pixels[0]) / 255.0f;
    color[1] = float(pixels[1]) / 255.0f;
    color[2] = float(pixels[2]) / 255.0f;
}

void sampleCube(Image input[6], float vector[3], float color[3]) {
    float x = fabs(vector[0]);
    float y = fabs(vector[1]);
    float z = fabs(vector[2]);

    float temp[3];
    float count = 0;

    mnVector3MulScalar(color, 0, color);

    if(x >= y && x >= z) {
        if(vector[0] >= 0)
            sampleFace(POSITIVE_X, input, -vector[2], +vector[1], vector[0], temp);
        else
            sampleFace(NEGATIVE_X, input, +vector[2], +vector[1], vector[0], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    if(y >= x && y >= z) {
        if(vector[1] >= 0)
            sampleFace(POSITIVE_Y, input, +vector[0], -vector[2], vector[1], temp);
        else
            sampleFace(NEGATIVE_Y, input, +vector[0], +vector[2], vector[1], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    if(z >= x && z >= y) {
        if(vector[2] >= 0)
            sampleFace(POSITIVE_Z, input, +vector[0], +vector[1], vector[2], temp);
        else
            sampleFace(NEGATIVE_Z, input, -vector[0], +vector[1], vector[2], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    mnVector3MulScalar(color, 1.0 / count, color);
}

float* tangentToWorld( const float v[3], const float tangentZ[3], float out[3] ) {
    float tangentX[3];
    float tangentY[3];

    if(fabs(tangentZ[2]) < 0.999) {
        float up[3] = {0,0,1};
        mnVector3Cross(up, tangentZ, tangentX);
    } else {
        float up[3] = {1,0,0};
        mnVector3Cross(up, tangentZ, tangentX);
    }

    mnVector3Normalize(tangentX, tangentX);
    mnVector3Cross(tangentZ, tangentX, tangentY);

    float temp[3];
    mnVector3MulScalar(tangentX, v[0], temp);
    mnVector3MulAddScalar(tangentY, v[1], temp);
    mnVector3MulAddScalar(tangentZ, v[2], temp);

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
    return out;
}

uint32_t bitfieldReverse(uint32_t bits) {
    bits = ( bits << 16) | ( bits >> 16);
    bits = ( (bits & 0x00ff00ff) << 8 ) | ( (bits & 0xff00ff00) >> 8 );
    bits = ( (bits & 0x0f0f0f0f) << 4 ) | ( (bits & 0xf0f0f0f0) >> 4 );
    bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
    bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
    return bits;
}

void hammersley2D( uint32_t i, uint32_t N, float out[2] ) {
    float E1 = float(i) / float(N);
    float E2 = float(bitfieldReverse(i)) * 2.3283064365386963e-10;

    out[0] = E1;
    out[1] = E2;
}

void randomizeLinear(uint32_t i, uint32_t N, float out[2]) {
    float s = sqrtf(float(N));

    float x = float(i) / s;
    float y = fmodf(i, s);

    out[0] = x / s;
    out[1] = y / s;
}

void generateRandom(uint32_t i, uint32_t N, float out[2]) {
    out[0] = (rand() % N) / (float)N;
    out[1] = (rand() % N) / (float)N;
}

void generateSamplePoint(uint32_t i, uint32_t N, float out[2]) {
    hammersley2D(i, N, out);
//    randomizeLinear(i, N, out);
//    generateRandom(i, N, out);
}

float* uniformSampleHemisphere( float E[2], float Out[4] ) {
    float Phi = 2 * M_PI * E[0];
    float CosTheta = E[1];
    float SinTheta = sqrtf( 1 - CosTheta * CosTheta );

    Out[0] = SinTheta * cosf( Phi );
    Out[1] = SinTheta * sinf( Phi );
    Out[2] = CosTheta;
    Out[3] = CosTheta / M_PI;

    return Out;
}

float* cosineSampleHemisphere( float E[2], float Out[4] ) {
    float Phi = 2 * M_PI * E[0];
    float CosTheta = sqrtf( E[1] );
    float SinTheta = sqrtf( 1 - CosTheta * CosTheta );

    Out[0] = SinTheta * cosf( Phi );
    Out[1] = SinTheta * sinf( Phi );
    Out[2] = CosTheta;
    Out[3] = CosTheta / M_PI;

    return Out;
}

void computePixelIrradiance(int face, Image input[6], float s, float t, float irradiance[3]) {
    float normal[3];

    float x = s * 2 - 1;
    float y = t * 2 - 1;

    if(face == POSITIVE_Y) {
        float temp[3] = {x,  1, -y};
        mnVector3Normalize(temp, normal);
    } else if(face == NEGATIVE_Y) {
        float temp[3] = {x, -1,  y};
        mnVector3Normalize(temp, normal);
    } else if(face == POSITIVE_X) {
        float temp[3] = {1, y, -x};
        mnVector3Normalize(temp, normal);
    } else if(face == NEGATIVE_X) {
        float temp[3] = {-1, y, x};
        mnVector3Normalize(temp, normal);
    } else if(face == POSITIVE_Z) {
        float temp[3] = {x, y, 1};
        mnVector3Normalize(temp, normal);
    } else if(face == NEGATIVE_Z) {
        float temp[3] = {-x, y, -1};
        mnVector3Normalize(temp, normal);
    }

    float sampledColour[3] = {0, 0, 0};

    int NumSamples = 128;
    for(int i = 0; i < NumSamples; i++) {
        float Xi[2];
        float L[4];
        float H[3];

        generateSamplePoint(i, NumSamples, Xi);
        cosineSampleHemisphere(Xi, L);
//        uniformSampleHemisphere(Xi, L);
        tangentToWorld(L, normal, L);

#if 0
        mnVector3Add(L, normal, L);
        mnVector3Normalize(L, L);
#endif

        float NdotL = mnVector3Dot(normal, L);
        if(NdotL > 0) {
            float Li[3];

            sampleCube(input, L, Li);

            mnVector3Add(sampledColour, Li, sampledColour);
//            mnVector3MulAddScalar(Li, NdotL, sampledColour);
        }
    }

    mnVector3MulScalar(sampledColour, 1 / (float)NumSamples, irradiance);
}

void computeFaceIrradiance(int face, Image input[6], int width, int height, Image& output) {
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            float irradiance[3];

            float s = x / (float)(width-1);
            float t = y / (float)(height-1);
            computePixelIrradiance(face, input, s, t, irradiance);

            //printf("%d %d %d %f %f %f\n", i, j, img, irradiance[0], irradiance[1], irradiance[2]);

            imageSet3(output, x, y, irradiance[0]*255, irradiance[1]*255, irradiance[2]*255);
        }
    }
}

void computeIrradiance(HeapAllocator& allocator, Image input[6], int width, int height, Image output[6]) {
    for(int face = 0; face < 6; face++) {
        output[face].width = width;
        output[face].height = height;
        output[face].format = input[face].format;
        output[face].pixels = (uint8_t*)allocator.allocate(3 * width * height);
        memset(output[face].pixels, 0, 3 * width * height);
    }

    auto start = std::chrono::high_resolution_clock::now();

#if 1
    for(int face = 0; face < 6; face++) {
        printf("Calculating irradiance for face %d\n", face);

        auto start = std::chrono::high_resolution_clock::now();

        computeFaceIrradiance(face, input, width, height, output[face]);

        auto end = std::chrono::high_resolution_clock::now();

        long long milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        printf("Took %lld millis\n", milli);
    }
#else
    std::thread tr[6];

    for(int face = 0; face < 6; face++) {
        tr[face] = std::thread([=]() {
            computeFaceIrradiance(face, input, width, height, output[face]);
        });
    }

    for(int face = 0; face < 6; face++)
        tr[face].join();
#endif

    auto end = std::chrono::high_resolution_clock::now();
    long long milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf("Total time: %lld millis\n", milli);
}

int main() {
    srand(time(nullptr));

    HeapAllocator heapAllocator;

    Image images[6];
    Image output[6];

#define IMG_PATH "images/LancellottiChapel"

    readJpeg(heapAllocator, IMG_PATH"/posx.jpg", images[POSITIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/negx.jpg", images[NEGATIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/posy.jpg", images[POSITIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/negy.jpg", images[NEGATIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/posz.jpg", images[POSITIVE_Z]);
    readJpeg(heapAllocator, IMG_PATH"/negz.jpg", images[NEGATIVE_Z]);

//    for(int i = 0; i < 6; i++) {
//        int byte = 0;
//
//        if(i == POSITIVE_Y)
//            byte = 0xff;
//
//        int size = images[i].format*images[i].width*images[i].height;
//        memset(images[i].pixels, byte, size);
//    }

    computeIrradiance(heapAllocator, images, 128, 128, output);

    const char* filename[] = {
        IMG_PATH"/irradiance_posx.irr",
        IMG_PATH"/irradiance_negx.irr",
        IMG_PATH"/irradiance_posy.irr",
        IMG_PATH"/irradiance_negy.irr",
        IMG_PATH"/irradiance_posz.irr",
        IMG_PATH"/irradiance_negz.irr",
    };

    for(int i = 0; i < 6; i++) {
        saveImage(filename[i], output[i]);

        heapAllocator.deallocate(output[i].pixels);
        heapAllocator.deallocate(images[i].pixels);
    }

    return 0;
}
