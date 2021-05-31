#include "resources/ResourceLoader.hpp"

template<>
auto kawe::ResourceLoader::load([[maybe_unused]]const std::string &filepath) -> std::shared_ptr<kawe::Texture> {
    // return _textureLoader.load(filepath);
    return nullptr;
}
