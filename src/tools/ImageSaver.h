#pragma once

#include <queue>
#include <variant>
#include <chrono>
#include <thread>
#include <mutex>

#include <glm/ext/vector_int2.hpp>
#include <Magick++.h>

class Saver {
    public: enum LoopState {
        NoLoop,
        SlowLoop,
        Loop,
    };
    using Image = Magick::Image;
    using Images = std::vector<Image>;
    using NamedImg = std::pair<std::variant<Image,Images>, std::string>;

    public:
        Saver();
        ~Saver();

    public:
        Image screen(glm::ivec2 size={0,0}) const;
        Magick::Geometry bounds(const Image &image) const;
        Magick::Geometry bounds(const Images &images) const;
        Magick::Geometry bounds(const std::vector<Magick::Geometry> &images, const Magick::Geometry &maxSize) const;

    public:
        void save(const std::string &name, const Image &image, const Magick::Geometry &bound="0x0+0+0");
        void save(const std::string &name, const Images &images, const Magick::Geometry &bound="-1x-1+0+0", LoopState looping=LoopState::Loop);

    private:
        std::thread t;

        std::queue<NamedImg> _images;
        std::mutex _mutex;
};