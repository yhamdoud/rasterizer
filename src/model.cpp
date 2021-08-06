#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "model.hpp"
#include "utils.hpp"
#include "vector.hpp"

using std::byte;
using std::clamp;
using std::istringstream;
using std::optional;
using std::string;
using std::string_view;
using std::vector;
using std::filesystem::path;

namespace rasterizer
{

Texture::Texture(int w, int h, int c, std::unique_ptr<uint8_t> d)
    : width{w}, height{h}, channel_count{c}, data{std::move(d)}
{
}

Color8 Texture::sample_bilinear(Vec2 tc) const
{
    // Translate to (floating point) texel indices.
    auto t_f = tc * Vec2{width, height} - Vec2{0.5f, 0.5f};

    float u_fract = utils::fract(t_f.u);
    float v_fract = utils::fract(t_f.v);

    IVec2 t{floor(t_f)};

    auto a = static_cast<Color>((*this)(t.x, t.y));
    auto b = static_cast<Color>((*this)(t.x + 1, t.y));
    auto c = static_cast<Color>((*this)(t.x, t.y + 1));
    auto d = static_cast<Color>((*this)(t.x + 1, t.y + 1));

    return static_cast<Color8>(
        lerp(lerp(a, b, u_fract), lerp(c, d, u_fract), v_fract));
}

Color8 Texture::sample_nearest(Vec2 tc) const
{
    // return (*this)(static_cast<IVec2>(round(tc * Vec2{width - 1, height -
    // 1})));
    return (*this)(
        static_cast<IVec2>(round(tc * Vec2{width, height} - Vec2{0.5f, 0.5f})));
}

Color8 Texture::operator()(Vec2 tc) const
{
    switch (sample)
    {
    case SampleMode::nearest:
        return sample_nearest(tc);
    case SampleMode::bilinear:
        return sample_bilinear(tc);
    };
}

Color8 Texture::operator()(int x, int y) const
{
    switch (wrap)
    {
    case WrapMode::repeat:
        x = abs(x % width);
        y = abs(y % height);
        break;
    case WrapMode::clamp:
        x = clamp(x, 0, width - 1);
        y = clamp(y, 0, height - 1);
        break;
    }

    uint8_t *pixel = data.get() + (y * width + x) * channel_count;

    switch (channel_count)
    {
    case 3:
        return Color8{pixel[0], pixel[1], pixel[2], 0};
    case 4:
        // TODO: reinterpret_cast or maybe return a reference?
        return Color8{pixel[0], pixel[1], pixel[2], pixel[3]};
    default:
        abort();
    }
}

Color8 Texture::operator()(IVec2 t) const { return (*this)(t.x, t.y); }

optional<Texture> Texture::from_file(const path &filename)
{
    int width, height, chan_count;

    auto *data = reinterpret_cast<uint8_t *>(
        stbi_load(filename.c_str(), &width, &height, &chan_count, 0));

    return data == nullptr
               ? std::nullopt
               : std::optional(Texture{width, height, chan_count,
                                       std::unique_ptr<uint8_t>{data}});
}

Model Model::from_obj(const std::filesystem::path &path)
{
    std::ifstream fs(path);
    if (!fs.is_open())
        throw std::runtime_error{"Error while opening file: " + path.string()};

    string line;

    vector<Vec3> positions;
    vector<Vec3> normals;
    vector<Vec2> uvs;

    vector<Vertex> vertices;

    while (std::getline(fs, line))
    {
        istringstream ss{line};

        string type;
        ss >> type;

        if (type == "v")
        {
            float x, y, z;
            ss >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }
        else if (type == "vt")
        {
            float u, v;
            ss >> u >> v;
            uvs.emplace_back(u, v);
        }
        else if (type == "vn")
        {
            float x, y, z;
            ss >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }
        else if (type == "f")
        {
            string ln;
            getline(ss, ln);

            std::array<int, 3> v, uv, n;

            if (sscanf(ln.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d\n", &v[0],
                       &uv[0], &n[0], &v[1], &uv[1], &n[1], &v[2], &uv[2],
                       &n[2]) == 9)
            {
                // OBJ uses zero based-indexing.
                for (auto i = 0; i < 3; i++)
                    vertices.push_back(Vertex{positions[--v[i]],
                                              normals[--n[i]], uvs[--uv[i]]});
            }
            else if (sscanf(ln.c_str(), "%d %d %d", &v[0], &v[1], &v[2]) == 3)
            {
                for (auto i = 0; i < 3; i++)
                    vertices.push_back(
                        Vertex{positions[--v[i]], Vec3{0}, Vec2{0}});
            }
            else
            {
                throw std::runtime_error{"Error while parsing line: " + line};
            }
        }

        if (ss.bad())
            throw std::runtime_error{"Error while parsing line: " + line};
    }

    if (fs.bad())
        throw std::runtime_error{"Error while reading file: " + path.string()};

    Model model{};
    model.mesh = std::make_unique<Mesh>(Mesh{vertices});
    return model;
}

} // namespace rasterizer