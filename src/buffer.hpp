#pragma once

#include <array>
#include <memory>

template <typename T> class Buffer
{

    std::unique_ptr<T[]> buffer;
    size_t width;
    size_t height;

  public:
    Buffer(std::size_t width, std::size_t height)
        : width{width}, height{height}, buffer{std::make_unique<T[]>(width *
                                                                     height)}
    {
    }

    T &operator()(std::size_t x, std::size_t y)
    {
        return buffer.get()[x + y * width];
    }

    void fill(T v)
    {
        std::fill(buffer.get(), buffer.get() + width * height, v);
    }
};