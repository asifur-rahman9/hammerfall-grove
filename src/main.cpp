// Hammerfall Grove - entry point: parses the command line and locates assets.
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "app/App.hpp"

#ifndef EW_SOURCE_DIR
#define EW_SOURCE_DIR "."
#endif
#ifndef EW_FONT_DIR
#define EW_FONT_DIR ""
#endif

namespace {

bool looksLikeAssetRoot(const std::filesystem::path& p) { return std::filesystem::exists(p / "shaders" / "scene.vert"); }

std::string findAssetRoot(const std::string& requested) {
    if (!requested.empty()) return requested;
    if (const char* env = std::getenv("HAMMERFALL_GROVE_ASSETS")) return env;
    if (looksLikeAssetRoot(std::filesystem::current_path())) return std::filesystem::current_path().string();
    return EW_SOURCE_DIR;  // fall back to the source tree the binary was built from
}

void printUsage() {
    std::cout << "Hammerfall Grove\n\n"
                 "Usage: hammerfall_grove [options]\n"
                 "  --assets DIR        directory containing shaders/, models/, textures/\n"
                 "  --size WxH          window size (default 1280x720)\n"
                 "  --seed N            random seed for ember placement\n"
                 "  --no-vsync          uncapped frame rate\n";
}

}  // namespace

int main(int argc, char** argv) {
    app::AppOptions opt;
    std::string assets;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << a << '\n';
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--assets") assets = next();
        else if (a == "--seed") opt.seed = static_cast<std::uint32_t>(std::stoul(next()));
        else if (a == "--no-vsync") opt.vsync = false;
        else if (a == "--size") {
            const std::string v = next();
            const auto x = v.find('x');
            if (x == std::string::npos) {
                std::cerr << "--size expects WxH\n";
                return 2;
            }
            opt.width = std::stoi(v.substr(0, x));
            opt.height = std::stoi(v.substr(x + 1));
        } else if (a == "-h" || a == "--help") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown option " << a << "\n\n";
            printUsage();
            return 2;
        }
    }
    opt.assetRoot = findAssetRoot(assets);
    opt.fontDirectory = EW_FONT_DIR;
    if (!looksLikeAssetRoot(opt.assetRoot)) {
        std::cerr << "Could not find game assets in '" << opt.assetRoot << "'. Run from the repository root or pass --assets.\n";
        return 1;
    }

    try {
        app::App application(opt);
        return application.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
