#include <filesystem>

#include "rasterizer.hpp"

using namespace rasterizer;

using std::filesystem::path;

int main()
{
    Rasterizer rasterizer{640, 480,
                          Model::from_obj(path{"../models/suzanne.obj"})};
    rasterizer.run();
}
