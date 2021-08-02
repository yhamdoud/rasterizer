#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "model.hpp"
#include "vector.hpp"

using std::istringstream;
using std::string;
using std::string_view;
using std::vector;

namespace rasterizer
{

Model::Model(Mesh &&mesh) : mesh{mesh} {}

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

            std::array<int, 3> vi, uvi, ni;

            if (sscanf(ln.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vi[0],
                       &uvi[0], &ni[0], &vi[1], &uvi[1], &ni[1], &vi[2],
                       &uvi[2], &ni[2]) == 9)
            {
                // OBJ uses zero based-indexing.
                for (auto i = 0; i < 3; i++)
                    vertices.push_back(Vertex{positions[--vi[i]],
                                              normals[--ni[i]], uvs[--uvi[i]]});
            }
            else if (sscanf(ln.c_str(), "%d %d %d", &vi[0], &vi[1], &vi[2]) ==
                     3)
            {
                for (auto i = 0; i < 3; i++)
                    vertices.push_back(
                        Vertex{positions[--vi[i]], Vec3{0}, Vec2{0}});
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

    return Model{Mesh{vertices}};
}

} // namespace rasterizer