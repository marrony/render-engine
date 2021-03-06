//
// Created by Marrony Neris on 01/03/16.
//

#ifndef JPEGREADER_H
#define JPEGREADER_H

#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>

bool readJpeg(HeapAllocator& allocator, FILE* stream, Image& image) {
    struct jpeg_decompress_struct srcinfo;
    struct jpeg_error_mgr jsrcerr;

    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);
    jpeg_stdio_src( &srcinfo, stream );

//    jpeg_save_markers(&srcinfo, JPEG_COM, 0xFFFF);
//
//    for (int m = 0; m < 16; m++)
//        jpeg_save_markers(&srcinfo, JPEG_APP0 + m, 0xFFFF);

    jpeg_read_header( &srcinfo, TRUE );
    jpeg_start_decompress( &srcinfo );

    image.width = srcinfo.image_width;
    image.height = srcinfo.image_height;
    image.format = srcinfo.num_components;
    image.pixels = (uint8_t*)allocator.allocate( srcinfo.image_width * srcinfo.image_height * srcinfo.num_components );

    int row_size = srcinfo.num_components*srcinfo.image_width;

    while (srcinfo.output_scanline < srcinfo.image_height) {
        int row = srcinfo.image_height - srcinfo.output_scanline - 1;

        JSAMPROW row_ptr = &image.pixels[row*row_size];
        jpeg_read_scanlines(&srcinfo, &row_ptr, 1);
    }

    jpeg_finish_decompress( &srcinfo );
    jpeg_destroy_decompress( &srcinfo );

    return true;
}

void readJpeg(HeapAllocator& allocator, const char* filename, Image& image) {
    FILE* infile = fopen( filename, "rb" );
    readJpeg(allocator, infile, image);
    fclose(infile);
}

#endif //JPEGREADER_H
