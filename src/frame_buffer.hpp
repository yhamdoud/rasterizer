#pragma once

#include <array>
#include <memory>

template <typename T> class FrameBuffer
{
    using type = T;

    size_t width;
    size_t height;
    std::unique_ptr<T[]> buffer;

  public:
    FrameBuffer(std::size_t width, std::size_t height)
        : width{width}, height{height}, buffer{std::make_unique<T[]>(width *
                                                                     height)}
    {
    }

    T &operator()(std::size_t x, std::size_t y)
    {
        return buffer.get()[x + y * width];
    }

    T *get() { return buffer.get(); }

    void fill(T v)
    {
        std::fill(buffer.get(), buffer.get() + width * height, v);
    }
};