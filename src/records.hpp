#pragma once

#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <unordered_map>
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

using RecordsSubtick = std::vector<Record *>;
using RecordsByTicks = std::map<long, RecordsSubtick>;
using RecordsByGroups = std::map<long, RecordsByTicks>;

struct RecordsManager
{

    size_t record_count = 0;
    RecordsByGroups records_by_group;
    std::vector<Record *> all_records;

    long last_tick()
    {
        long width = 0;
        for (size_t cpu = 0; cpu < records_by_group.size(); cpu++)
        {
            width = std::max<long>(width, static_cast<long>(records_by_group[cpu].size()));
            width = std::max<long>(width, static_cast<long>(records_by_group[cpu].end()->first));

        }
        return width;
    }

    Record* get_record_by_uid(size_t uid)
    {
        return all_records[uid];
    }
    static RecordsManager &the();

    // WARNING: don't add to all_records directly
    void add_record(Record &record);

    void update_all_records();

};

void parse_records_from_file(const std::string &filename);
