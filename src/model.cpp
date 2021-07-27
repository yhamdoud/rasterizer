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
            for (int i = 0; i < 3; i++)
            {
                string tmp;
                size_t idx, uv_idx, nrm_idx;

                getline(ss, tmp, '/');
                idx = std::stoi(tmp);

                getline(ss, tmp, '/');
                uv_idx = std::stoi(tmp);

                getline(ss, tmp, ' ');
                nrm_idx = std::stoi(tmp);

                // OBJ uses zero based-indexing.
                vertices.push_back(Vertex{positions[--idx], normals[--nrm_idx],
                                          uvs[--uv_idx]});
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