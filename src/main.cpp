#include <filesystem>
#include <iostream>

#include "rasterizer.hpp"

using namespace rasterizer;

using std::filesystem::path;

int main(int argc, const char *argv[])
{
    if (argc == 2)
    {
        Rasterizer rasterizer{640, 480, Model::from_obj(path{argv[1]})};
        rasterizer.run();
    }
    else
    {
        std::cout << "Usage: rasterizer model.obj" << std::endl;
    }
}
