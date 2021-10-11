#pragma once

#include <queue>
#include <variant>
#include <chrono>
#include <thread>
#include <mutex>

#include <Magick++.h>

class Saver {
    public: enum LoopState {
        NoLoop,
        SlowLoop,
        Loop,
    };

    public:
        Saver();
        ~Saver();

    public:
        Magick::Image screen() const;
        Magick::Geometry bounds(const Magick::Image &image) const;
        Magick::Geometry bounds(const std::vector<Magick::Image> &image) const;
        Magick::Geometry bounds(const std::vector<Magick::Geometry> &image) const;
    
    public:
        void save(const std::string &name, const Magick::Image &image, const Magick::Geometry &bound="0x0+0+0");
        void save(const std::string &name, const std::vector<Magick::Image> &images, const Magick::Geometry &bound="-1x-1+0+0", LoopState looping=LoopState::Loop);

    private:
        std::thread t;

        std::queue<std::pair<std::variant<Magick::Image, std::vector<Magick::Image>>, std::string>> _images;
        std::mutex _mutex;
};