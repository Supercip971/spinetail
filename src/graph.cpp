#include "imgui.h"
#include "records.hpp"
#include "ui.hpp"

int display_tick(RecordsByTicks &group, size_t start_tick, long cpu, float rec_width, ImVec2 cursor)
{

    auto draw_list = ImGui::GetWindowDrawList();

    Record *last_record = nullptr;
    ImRect r = {};
    r.Min.y = 0.f + 200.f * cpu;
    r.Min.x = rec_width * start_tick;
    r.Max.x = r.Min.x;
    r.Max.y = 100.f + 200.f * cpu;

    Record *selected_record = get_selected_record();
    draw_list->ChannelsSplit(2);

    float window_width = ImGui::GetWindowWidth();
    int end = ceilf(window_width / rec_width) + 5;
    for (size_t tick = start_tick; tick < start_tick + end; tick++)
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

            ImColor col = try_hash(last_record->name);

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
                set_selected_record(last_record);
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

        draw_list->AddRectFilled(r.Min + cursor, r.Max + cursor, try_hash(last_record->name.c_str()) | 0xff000000);
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

            r2.Max.x = (record->tick + 0.1f) * rec_width;

            ImColor col = try_hash(record->name);

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

int display_graph(float &rec_width)
{

    ImGui::BeginChild("graph", {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);

    // ImGui::Text("cursor: %f, %f", cursor.x, cursor.y);

    auto cursor = ImGui::GetCursorScreenPos();
    float width = RecordsManager::the().last_tick() * rec_width;
    ImGui::ItemSize(
        ImRect(0, 0, width, ImGui::GetWindowHeight() - 16.f));
    size_t start_tick = std::max(0.f, ((-cursor.x) / rec_width) - 1);


    // check if it is hovered
    bool hovered = ImGui::IsWindowHovered();
    if (hovered)
    {
        auto &io = ImGui::GetIO(); // Pick new zoom from mouse wheel

        if (!io.KeyShift)
        {
            if (io.MouseWheel > 0.0f)
            {
                rec_width += rec_width * 0.02f * io.MouseWheel;
                //        zoom_changed = true;
            }
            else if (io.MouseWheel < 0.0f)
            {
                rec_width -= rec_width * (0.02f * -io.MouseWheel);
                //        zoom_changed = true;
            }
        }
    }

  //  printf("mouse wheel: %f, rec_width: %f\n", io.MouseWheel, rec_width);
    auto &records_by_group = RecordsManager::the().records_by_group;
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
