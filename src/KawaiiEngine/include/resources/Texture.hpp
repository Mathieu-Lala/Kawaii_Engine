#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "spdlog/spdlog.h"

namespace kawe {
    class Texture {
    public:
        Texture(const std::string &filepath)
            : _data { stbi_load(filepath.c_str(), &_width, &_height, &_channels, 0) }
        {
            if (!_data)
                spdlog::error("couldn't load texture at '%s'.", filepath);
        }
        ~Texture() = default;

        auto is_initialised() const -> bool {
            return !!_data;
        }

    private:
        int _width;
        int _height;
        int _channels;
        unsigned char *_data;
    };
}