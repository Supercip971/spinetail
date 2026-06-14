#pragma once

#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <sstream>
#include <unordered_map>
typedef enum
{
    RECORD_TYPE_EVENT,
    RECORD_TYPE_SCHEDULER,
    RECORD_TYPE_SPANEVENT,
} RecordType;

struct Record
{
    RecordType type;
    bool is_begin;
    std::string name;
    float tick;
    long group;
    long pid;
    std::unordered_map<std::string, std::string> values;
};

bool record_parser_update_line(Record &record, const std::string &data);

using RecordsSubtick = std::vector<Record *>;
using RecordsByTicks = std::map<long, RecordsSubtick>;
using RecordsByGroups = std::map<long, RecordsByTicks>;

struct RecordsManager
{
private:
    RecordsManager() = default;

public:

    struct SpanEventStateRecording
    {
        std::vector<Record*> events;
        size_t pid;
    };


    std::map<size_t, SpanEventStateRecording> running_span_event; // BY GROUPS
    std::map<size_t, SpanEventStateRecording> saved_span_event; // BY PIDS
    bool start_reading = false;
    bool has_record = false;
    Record building_current_record;

    bool record_parser_update_from_line(std::string const &line, bool defer_sorting = false);

    size_t record_count = 0;
    RecordsByGroups records_by_group;
    std::vector<Record *> all_records;

    long last_tick()
    {

        if (all_records.size() <= 0)
            return 0;
        long width = 0;
        for (size_t cpu = 0; cpu < records_by_group.size(); cpu++)
        {
            width = std::max<long>(width, static_cast<long>(records_by_group[cpu].size()));
        }

        return std::max<long>(width, all_records[all_records.size() - 1]->tick) + 1.f;
    }

    Record *get_record_by_uid(size_t uid)
    {
        return all_records[uid];
    }
    static RecordsManager &the();

    // WARNING: don't add to all_records directly
    Record* _add_record(Record &record, bool defer_sorting = false);

    void add_record(Record &record, bool defer_sorting = false);

    void update_all_records();

    void update_providers();
};

void parse_records_from_file(const std::string &filename);
