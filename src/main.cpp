
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

// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.

typedef enum
{
    RECORD_TYPE_EVENT,
    RECORD_TYPE_SCHEDULER
} RecordType;

struct Record
{
    RecordType type;
    std::string name;
    float tick;
    long group;
    std::unordered_map<std::string, std::string> values;
};
template <typename... Args>
void mfprint(auto const &rt_fmt_str, Args &&...args)
{
    std::cout << std::vformat(rt_fmt_str, std::make_format_args(args)...) << std::endl;
}

using RecordsSubtick = std::vector<Record *>;
using RecordsByTicks = std::map<long, RecordsSubtick>;
using RecordsByGroups = std::map<long, RecordsByTicks>;

size_t record_count = 0;
RecordsByGroups records_by_group;
std::vector<Record *> all_records;

void add_record(Record &record)
{
    if (!records_by_group.contains(record.group))
    {
        records_by_group[record.group] = RecordsByTicks();
    }

    if (record.name == "")
    {
        record.name = "record(" + std::to_string(record.group) + "," + std::to_string(record_count) + ")";
    }

    Record *r = new Record();
    *r = record;

    if (records_by_group[record.group].contains(record.tick))
    {
        records_by_group[record.group][record.tick].push_back(r);
    }
    else
    {
        records_by_group[record.group][record.tick] = RecordsSubtick({r});
    }
    record_count++;
}

void add_all_records()
{
    // add records in order from tick to group
    //
    // EACH group has an iterator pointing to the current record

    for (auto &group : records_by_group)
    {
        for (auto &record : group.second)
        {
            all_records.insert(all_records.end(), record.second.begin(), record.second.end());
        }
    }
    std::sort(all_records.begin(), all_records.end(), [](Record *a, Record *b)
              {
        if (a->tick != b->tick)
        {
            return a->tick < b->tick;
        }
        return a->group < b->group; });
}

void parse_records(std::fstream &file)
{
    std::string line;
    bool start_reading = false;
    bool has_record = false;
    Record current_record;
    while (std::getline(file, line))
    {
        if (!start_reading && line.contains("@st:begin"))
        {
            start_reading = true;
            continue;
        }
        if (line.contains("@st:sched"))
        {
            if (has_record)
            {
                add_record(current_record);
                current_record = {};
            }

            current_record.type = RECORD_TYPE_SCHEDULER;
            has_record = true;
            continue;
        }
        if (line.contains("@st:event"))
        {
            if (has_record)
            {
                add_record(current_record);
                current_record = {};
            }

            current_record.type = RECORD_TYPE_EVENT;
            has_record = true;
            continue;
        }

        if (line[0] == '#')
        {

            if (line.starts_with("#name"))
            {
                current_record.name = line.substr(strlen("#name "));
                continue;
            }
            if (line.starts_with("#group"))
            {
                current_record.group = std::stol(line.substr(strlen("#group ")));
                continue;
            }
            if (line.starts_with("#tick"))
            {
                current_record.tick = std::stof(line.substr(strlen("#tick ")));
                continue;
            }
            if (line.starts_with("#"))
            {
                continue;
            }
        }
        else
        {
            // first word is key, the next until the end of theline are avlues
            auto end_first_word = line.find(' ');
            std::string key = line.substr(0, end_first_word);
            std::string value = line.substr(end_first_word + 1);
            current_record.values[key] = value;
        }
    }

    if (has_record)
    {
        add_record(current_record);
    }
    mfprint("records parsed", "");
}
// Main code

auto get_nth_element(auto const &group, size_t index)
{
    auto it = group.begin();
    for (size_t i = 0; i < index; ++i)
    {
        it = std::next(it);
    }
    return std::move(it);
}

const char *get_record(void *, int index)
{
    if ((size_t)index >= record_count)
    {
        return nullptr;
    }

    return all_records[index]->name.c_str();
}

auto get_selected_record(auto record_ref)
{
    return all_records[record_ref];
}

std::hash<std::string> hash_fn;

int record_ref = -1;
Record *selected_record = nullptr;

int display_tick(RecordsByTicks &group, size_t start_tick, long cpu, float rec_width, ImVec2 cursor)
{

    auto draw_list = ImGui::GetWindowDrawList();

    Record *last_record = nullptr;
    ImRect r = {};
    r.Min.y = 0.f + 200.f * cpu;
    r.Min.x = rec_width * start_tick;
    r.Max.x = r.Min.x;
    r.Max.y = 100.f + 200.f * cpu;

    draw_list->ChannelsSplit(2);

    for (size_t tick = start_tick; tick < start_tick + 21; tick++)
    {
        if (!group.contains(tick))
        {
            r.Max.x += rec_width;
            continue;
        }

        draw_list->ChannelsSetCurrent(0);
        auto r2 = r;
        auto records = group[tick];
        for (auto record : records)
        {
            if (record->type == RECORD_TYPE_EVENT)
            {
                continue;
            }
            r.Max.x = record->tick * rec_width;

            if (last_record == nullptr)
            {
                last_record = record;
            }

            ImColor col = hash_fn(last_record->name);

            col.Value.w = 1.0f;
            if (tick % 2 == 0)
            {
                col.Value.x += 0.03f;
                col.Value.y += 0.03f;
                col.Value.z += 0.03f;
            }
            if (selected_record == last_record)
            {
                col.Value.x += 0.05f;
                col.Value.y += 0.05f;
                col.Value.z += 0.05f;
            }

            draw_list->AddRectFilled(r.Min + cursor, r.Max + cursor,
                                     col);

            // clickable rect

            if (ImGui::IsMouseHoveringRect(r.Min + cursor, r.Max + cursor) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selected_record = last_record;
            }
            if (record->name != last_record->name)
            {
                bool use_dark_mode = false;
                if (col.Value.x + col.Value.y + col.Value.z > 1.5f)
                {
                    use_dark_mode = true;
                }
                draw_list->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize(),

                    r.Min + cursor + ImVec2{2, 2}, use_dark_mode ? 0xFF000000 : 0xFFFFFFFF, last_record->name.c_str(), nullptr,
                    rec_width);
            }
            last_record = record;
            r.Min.x = r.Max.x;
        }

        draw_list->AddRectFilled(r.Min + cursor, r.Max + cursor, hash_fn(last_record->name.c_str()) | 0xff000000);
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            r.Min + cursor + ImVec2{2, 2}, 0xFFFFFFFF, last_record->name.c_str(), nullptr,
            rec_width);

        draw_list->ChannelsSetCurrent(1);

        for (auto record : records)
        {
            if (record->type != RECORD_TYPE_EVENT)
            {
                continue;
            }

            printf("%s tick=%f\n", record->name.c_str(), record->tick);



            r2.Min.x = (record->tick) * rec_width;

            r2.Max.x = (record->tick +0.1f) * rec_width;

            ImColor col = hash_fn(record->name);

            col.Value.w = 1.0f;


            draw_list->AddRectFilled(r2.Min + cursor, r2.Max + cursor,
                                     col);


            // clickable rect

            if (ImGui::IsMouseHoveringRect(r2.Min + cursor, r2.Max + cursor) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selected_record = record;
            }
            draw_list->AddText(
                ImGui::GetFont(),
                ImGui::GetFontSize(),

                r2.Min + cursor + ImVec2{0, -ImGui::GetFontSize()}, 0xFFFFFFFF, record->name.c_str(), nullptr,
                rec_width);
        }
    }

    draw_list->ChannelsMerge();

    return 0;
}

int display_graph(float rec_width)
{

    ImGui::BeginChild("graph", {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted("- Graph -");
    // ImGui::Text("cursor: %f, %f", cursor.x, cursor.y);

    auto cursor = ImGui::GetCursorScreenPos();
    // show size
    //
    float width = 0.f;
    for (size_t cpu = 0; cpu < records_by_group.size(); cpu++)
    {
        width = std::max(width, rec_width * records_by_group[cpu].size());
        width = std::max(width, rec_width * records_by_group[cpu].end()->first);
    }
    ImGui::ItemSize(
        ImRect(0, 0, width, ImGui::GetWindowHeight()));
    size_t start_tick = std::max(0.f, ((-cursor.x) / rec_width) - 1);
    for (size_t cpu = 0; cpu < records_by_group.size(); cpu++)
    {
        auto &group = records_by_group[cpu];

        if (group.size() == 0)
            continue;

        display_tick(group, start_tick, cpu, rec_width, cursor);
    }
    //  draw_list->AddLine(cursor + ImVec2{50, 50}, cursor + ImVec2{1000, 1000}, 0xFFFFFFFF);
    ImGui::EndChild();
    return 0;
}

int ui()
{

    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Create SDL window graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL3+SDL_GPU example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Create GPU Device
    SDL_GPUDevice *gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB, true, nullptr);
    if (gpu_device == nullptr)
    {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }

    // Claim window for GPU Device
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
    {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetGPUSwapchainParameters(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale; // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                     // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // Our state

    // Main loop
    bool done = false;

    static float rec_width = 300.f;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        ImGui::Begin("records list");
        {
            //     IMGUI_API bool
            // ListBox(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int height_in_items = -1);

            ImGui::SliderFloat("record width", &rec_width, 100.f, 500.f);

            ImGui::ListBox(
                "##records",
                &record_ref,
                get_record,
                nullptr,
                record_count,
                -1);


            if (record_ref != -1)
            {
                selected_record = get_selected_record(record_ref);
            }
            if (selected_record != nullptr)
            {
                ImGui::Text("Name: %s", selected_record->name.c_str());
                ImGui::Text("Tick: %f", selected_record->tick);
                ImGui::Text("Group: %ld", selected_record->group);

                for (auto &[key, value] : selected_record->values)
                {
                    ImGui::Text("%s: %s", key.c_str(), value.c_str());
                }
            }
        }
        ImGui::End();

        ImGui::Begin("graph");
        {
            display_graph(rec_width);
        }
        ImGui::End();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).

        // 3. Show another simple window.

        // Rendering
        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device); // Acquire a GPU command buffer

        SDL_GPUTexture *swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr); // Acquire a swapchain texture

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            // Setup and start a render pass
            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{0, 0, 0, 1};
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;
            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }

        // Submit the command buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    SDL_WaitForGPUIdle(gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        mfprint("Usage: {} <filename>", argv[0]);
        return 1;
    }

    std::string path = argv[1];

    std::fstream file(path);
    if (!file.is_open())
    {
        mfprint("Failed to open file: {}", path);
        return 1;
    }

    parse_records(file);
    add_all_records();
    ui();

    return 0;
}
