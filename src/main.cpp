#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

#include "rasterizer.hpp"

using namespace rasterizer;

using std::filesystem::path;

int help(const std::string_view msg)
{
    std::cout << msg << "\nUsage: rasterizer model.obj [diffuse.png]"
              << std::endl;
    return 1;
}

int main(int argc, const char *argv[])
{
    if (argc >= 2)
    {
        auto model = Model::from_obj(path{argv[1]});

        if (argc == 3)
        {
            auto diffuse = Texture::from_file(path{argv[2]});
            if (diffuse.has_value())
                model.diffuse_texture =
                    std::make_unique<Texture>(std::move(*diffuse));
            else
                return help("Invalid diffuse texture provided.");
        }

        Rasterizer rasterizer{640, 480, std::move(model)};
        rasterizer.run();

        return 0;
    }
    else
    {
        return help("No model provided.");
    }
}