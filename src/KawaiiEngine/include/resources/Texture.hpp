#pragma once

namespace kawe {
    struct Texture {

        Texture(int w, int h, int c, unsigned char *d)
            : width    { w }
            , height   { h }
            , channels { c }
            , data     { d } {}

        ~Texture() = default;

        int width;
        int height;
        int channels;
        unsigned char *data;
    };
}