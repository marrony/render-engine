//
// Created by Marrony Neris on 11/18/15.
//

#ifndef TGAREADER_H
#define TGAREADER_H

#include <stdio.h>

/*
 A TGA file has a header that consists of 12 fields. These are:
 # 0x00 - 0x00 - id (uchar)
 # 0x01 - 0x01 - colour map type (uchar)
 # 0x02 - 0x02 - image type (uchar)
 # 0x03 - 0x04 - colour map first entry (short int)
 # 0x05 - 0x06 - colour map length (short int)
 # 0x07 - 0x07 - colour map depth (uchar)
 # 0x08 - 0x09 - horizontal origin (short int)
 # 0x0a - 0x0b - vertical origin (short int)
 # 0x0c - 0x0d - width (short int)
 # 0x0e - 0x0f - height (short int)
 # 0x10 - 0x10 - pixel depth (uchar)
 # 0x11 - 0x11 - image descriptor (uchar)

 Some possible values for the image type are:

 # 1 - colour map image
 # 2 - RGB(A) uncompressed
 # 3 - greyscale uncompressed
 # 9 - greyscale RLE (compressed)
 # 10 - RGB(A) RLE (compressed)
 */
struct TGAHeader {
    uint8_t id;
    uint8_t colour_map_type;
    uint8_t image_type;
    uint16_t colour_map_first_entry;
    uint16_t colour_map_length;
    uint8_t colour_map_depth;
    uint16_t horizontal_origin;
    uint16_t vertical_origin;
    uint16_t width;
    uint16_t height;
    uint8_t pixel_depth;
    uint8_t image_descriptor;
};

bool readTga(HeapAllocator& allocator, FILE* stream, Image& image) {
    TGAHeader header;

    fread(&header.id, sizeof(header.id), 1, stream);
    fread(&header.colour_map_type, sizeof(header.colour_map_type), 1, stream);
    fread(&header.image_type, sizeof(header.image_type), 1, stream);
    fread(&header.colour_map_first_entry, sizeof(header.colour_map_first_entry), 1, stream);
    fread(&header.colour_map_length, sizeof(header.colour_map_length), 1, stream);
    fread(&header.colour_map_depth, sizeof(header.colour_map_depth), 1, stream);
    fread(&header.horizontal_origin, sizeof(header.horizontal_origin), 1, stream);
    fread(&header.vertical_origin, sizeof(header.vertical_origin), 1, stream);
    fread(&header.width, sizeof(header.width), 1, stream);
    fread(&header.height, sizeof(header.height), 1, stream);
    fread(&header.pixel_depth, sizeof(header.pixel_depth), 1, stream);
    fread(&header.image_descriptor, sizeof(header.image_descriptor), 1, stream);

    if(header.image_type != 2 && header.image_type != 3 && header.image_type != 10) {
        return false;
    }

    if(header.pixel_depth != 8 && header.pixel_depth != 24 && header.pixel_depth != 32) {
        return false;
    }

    if(header.width == 0 || header.height == 0) {
        return false;
    }

    int pixel_size = header.pixel_depth / 8;
    int total_bytes = header.width * header.height * pixel_size;
    char* data = (char*)allocator.allocate(total_bytes);

    if(data == nullptr) {
        return false;
    }

    if(header.image_type == 10) {
        int bytes_to_process = total_bytes;

        char* dst = data;

        while(bytes_to_process > 0) {
            int byte_read = fgetc(stream);

            int bytes_to_read = (byte_read & 0x7f) + 1;

            bytes_to_process -= bytes_to_read * pixel_size;

            if(byte_read & 0x80) {
                char pixels[8];

                fread(pixels, sizeof(char), pixel_size, stream);

                do {
                    memcpy(dst, pixels, pixel_size);
                    dst += pixel_size;
                } while(--bytes_to_read);
            } else {
                bytes_to_read *= pixel_size;
                assert(fread(dst, sizeof(char), bytes_to_read, stream) == bytes_to_read);
                dst += bytes_to_read;
            }
        }
    } else {
        assert(fread(data, sizeof(char), total_bytes, stream) == total_bytes);
    }

    if(pixel_size >= 3) {
        for(int i = 0; i < total_bytes; i += pixel_size)
            std::swap(data[i + 0], data[i + 2]);
    }

    switch(pixel_size) {
        case 1:
            image.width = header.width;
            image.height = header.height;
            image.format = 1;
            image.pixels = data;
            break;

        case 3:
            image.width = header.width;
            image.height = header.height;
            image.format = 3;
            image.pixels = data;
            break;

        case 4:
            image.width = header.width;
            image.height = header.height;
            image.format = 4;
            image.pixels = data;
            break;

        default:
            break;
    }

    return true;
}

#endif //TGAREADER_H
