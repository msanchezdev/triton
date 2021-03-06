#include <map>
#include <list>
#include <functional>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <yaml-cpp/yaml.h>
#include "app.hpp"
#include "utils.hpp"
#include "components/camera/camera.hpp"
#include "components/fps_controller/fps_controller.hpp"

using namespace triton;

App app;

App::App() {
    this->root.AddComponent(new FPSController());
}

App::~App() {
    debug("Shutting down the application");

    debug("Unloading images");
    this->images.clear();

    debug("Unregistering event handlers");
    events.Unsubscribe();

    debug("Destroying renderer");
    SDL_DestroyRenderer(this->renderer);

    debug("Closing main window");
    SDL_DestroyWindow(window);

    debug("Stopping image subsystem");
    IMG_Quit();

    debug("Stopping SDL subsystems");
    SDL_Quit();
}

void App::Initialize() {
    this->LoadConfig();

    this->InitializeVideo();
    this->InitializeGameWindow();
    this->InitializeImages();
    this->InitializeFonts();

    this->LoadResources();
}

int App::Start() {
    this->Loop();
    return 0;
}

void App::InitializeVideo() {
    debug("Initializing video subsystem");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        critical("Unable to initialize video subsystem: %s", SDL_GetError());
        throw new std::runtime_error("Unable to initialize video subsystem");
    }

    debug("Video subsystem initialized successfully");
}

void App::InitializeGameWindow() {
    debug("Creating game window");
    this->window = SDL_CreateWindow(
        "Triple T",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        this->resolution.width, this->resolution.height,
        this->fullscreen ? SDL_WINDOW_FULLSCREEN : 0
    );
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (this->window == nullptr) {
        critical("Unable to open game window: %s", SDL_GetError());
        throw new std::runtime_error("Unable to open game window");
    }

    debug("Creating main renderer");
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RendererFlags::SDL_RENDERER_ACCELERATED);

    if (this->renderer == nullptr) {
        critical("Unable to create main renderer: %s", SDL_GetError());
        throw new std::runtime_error("Unable to create main renderer");
    }

    debug("Game window opened successfully");
}

void App::InitializeImages() {
    debug("Initializing image subsystem");
    int formatLoaded = IMG_Init(IMG_INIT_PNG);

    if (!formatLoaded & IMG_INIT_PNG) {
        critical("Unable to initialize image subsystem: %s", IMG_GetError());
        throw new std::runtime_error("Unable to initialize image subsystem");
    }

    debug("Image subsystem initialized successfully");
}

void App::InitializeFonts() {
    debug("Initializing font subsystem");

    if (TTF_Init()) {
        critical("Unable to initialize font subsystem: %s", TTF_GetError());
        throw new std::runtime_error("Unable to initialize font subsystem");
    }

    debug("Font subsystem initialized successfully");
}

void App::Loop() {
    debug("Entering main loop");

    SDL_Event event;
    while (true) {
        this->Render();

        if (!SDL_PollEvent(&event)) continue;

        switch (event.type) {
        case SDL_EventType::SDL_QUIT:
            debug("Event Received: Quit");
            return;

        case SDL_EventType::SDL_KEYDOWN:
            if (event.key.keysym.sym == SDL_KeyCode::SDLK_ESCAPE) {
                debug("Event Received: Key down, Escape");
                debug("Pressed Escape, exitting");
                return;
            }
            break;

        case SDL_EventType::SDL_MOUSEMOTION:
            this->events.Emit(EventType::MouseMove, &event.motion);
            break;

        case SDL_EventType::SDL_MOUSEBUTTONDOWN:
            this->events.Emit(EventType::MouseButtonDown, &event.button);
            break;

        case SDL_EventType::SDL_MOUSEBUTTONUP:
            this->events.Emit(EventType::MouseButtonUp, &event.button);
            break;

        default:
            debug("Event Received: %#x", event.type);
            break;
        }
    }
}

void App::Render() {
    SDL_SetRenderDrawColor(app.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(this->renderer);
    // RenderEvent event;
    // this->events.Emit(EventType::Render, &event);

    // render cameras
    for (auto camera : this->cameras) {
        CameraRenderEvent camera_event {
            .camera = camera,
        };

        this->events.Emit(EventType::CameraRender, &camera_event);
    }

    SDL_RenderPresent(this->renderer);
}

void App::LoadConfig() {
    this->config = YAML::LoadFile("triton.yaml");

    // Set log level
    if (auto loglevel = this->config["loglevel"]) {
        string level_str = this->config["loglevel"].as<string>();
        SDL_LogPriority new_loglevel = SDL_LOG_PRIORITY_CRITICAL;

        if (level_str == "verbose") {
            new_loglevel = SDL_LOG_PRIORITY_VERBOSE;
        } else if (level_str == "debug") {
            new_loglevel = SDL_LOG_PRIORITY_DEBUG;
        } else if (level_str == "info") {
            new_loglevel = SDL_LOG_PRIORITY_INFO;
        } else if (level_str == "warn") {
            new_loglevel = SDL_LOG_PRIORITY_WARN;
        } else if (level_str == "error") {
            new_loglevel = SDL_LOG_PRIORITY_ERROR;
        } else if (level_str == "critical") {
            new_loglevel = SDL_LOG_PRIORITY_CRITICAL;
        } else {
            throw std::runtime_error("Invalid value for 'loglevel'. Valid levels are: verbose, debug, info, warn, error, critical");
        }

        SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, new_loglevel);
    }

    // set fullscreen
    if (auto fullscreen = this->config["fullscreen"]) {
        this->fullscreen = this->config["fullscreen"].as<bool>();
    }

    // Set resolution
    if (auto resolution = this->config["resolution"]) {
        auto resolution_str = this->config["resolution"].as<string>();
        auto resolution_split = strtok((char*)resolution_str.c_str(), "x");
        this->resolution.width = atoi(resolution_split);
        this->resolution.height = atoi(strtok(nullptr, "x"));
    }
}

void App::LoadResources() {
    if (auto assets = this->config["assets"]) {
        if (!assets.IsMap()) {
            throw std::runtime_error("Invalid value for 'assets', expected map");
        }

        if (auto images = assets["images"]) {
            if (!images.IsMap()) {
                throw std::runtime_error("Invalid 'assets.images' config, expected map");
            }

            for (auto image : images) {
                string name = image.first.as<string>();
                string path = image.second.as<string>();

                this->LoadImage(name, "assets/images/" + path);
            }
        }

        if (auto fonts = assets["fonts"]) {
            if (!fonts.IsMap()) {
                throw std::runtime_error("Invalid 'assets.fonts' config, expected map");
            }

            for (auto font : fonts) {
                string name = font.first.as<string>();
                string path = font.second.as<string>();

                this->LoadFont(name, "assets/fonts/" + path);
            }
        }
    }
}

void App::LoadImage(string name, string path) {
    this->images[name] = new ImageResource(this->renderer, name, path);
}

void App::LoadFont(string name, string path) {
    this->fonts[name] = new FontResource(name, path);
}

float App::GetPixelPerUnit() {
    return this->pixel_per_unit;
}

Camera* App::CreateCamera(string name) {
    auto camera_gameobject = new GameObject(name);
    auto camera = new Camera();
    camera_gameobject->AddComponent(camera);

    this->cameras.push_back(camera);
    return camera;
}

long double operator "" units(long double value) {
    return value * app.GetPixelPerUnit();
}

long double operator "" units(unsigned long long value) {
    return value * app.GetPixelPerUnit();
}
