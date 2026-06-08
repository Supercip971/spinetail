
#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <string.h>
#include <unordered_map>
#include "records.hpp"

// Dear ImGui: standalone example application for SDL3 + SDL_GPU
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_sdlgpu3.cpp/.h in their own engine/app.
// - Unlike other backends, the user must call the function ImGui_ImplSDLGPU_PrepareDrawData() BEFORE issuing a SDL_GPURenderPass containing ImGui_ImplSDLGPU_RenderDrawData.
//   Calling the function is MANDATORY, otherwise the ImGui will not upload neither the vertex nor the index buffer for the GPU. See imgui_impl_sdlgpu3.cpp for more info.

#include <SDL3/SDL.h>
#include <stdio.h>  // printf, fprintf
#include <stdlib.h> // abort
#include <vector>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "ui.hpp"
#include "utils.hpp"

// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        mfprint("Usage: {} <filename>", argv[0]);
        return 1;
    }

    std::string path = argv[1];

    parse_records_from_file(path);
    RecordsManager::the().update_all_records();
    ui();

    return 0;
}
