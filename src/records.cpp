
#include "records.hpp"
#include <algorithm>
#include <sstream>
#include "utils.hpp"

RecordsManager &RecordsManager::the()
{
    static RecordsManager instance = RecordsManager();
    return instance;
}

void RecordsManager::add_record(Record &record, bool defer_sorting)
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

    if (!defer_sorting)
    {
        all_records.insert(
            std::upper_bound(all_records.begin(), all_records.end(), r, [](Record *a, Record *b)
                             {
                if (a->tick != b->tick)
                {
                    return a->tick < b->tick;
                }
                return a->group < b->group; }),
            r);
    }
}

void RecordsManager::update_all_records()
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

bool RecordsManager::record_parser_update_from_line(std::string const &line, bool defer_sorting)
{
    if (line.length() == 0)
        return true;
    if (!start_reading && line.contains("@st:begin"))
    {
        start_reading = true;
        return true;
    }
    else if (line.contains("@st:sched"))
    {
        if (has_record)
        {
            printf("== adding record = %s\n", building_current_record.name.c_str());
            this->add_record(building_current_record, defer_sorting);
            building_current_record = {};
        }

        building_current_record.type = RECORD_TYPE_SCHEDULER;
        has_record = true;
        return true;
    }
    else if (line.contains("@st:event"))
    {
        printf("⁼== adding record = %s\n", building_current_record.name.c_str());
        if (has_record)
        {
            this->add_record(building_current_record, defer_sorting);
            building_current_record = {};
        }

        building_current_record.type = RECORD_TYPE_EVENT;
        has_record = true;
        return true;
    }
    else if (line[0] == '#')
    {

        if (line.starts_with("#name"))
        {
            building_current_record.name = line.substr(strlen("#name "));
            return true;
        }
        if (line.starts_with("#group"))
        {
            building_current_record.group = std::stol(line.substr(strlen("#group ")));
            return true;
        }
        if (line.starts_with("#tick"))
        {
            building_current_record.tick = std::stof(line.substr(strlen("#tick ")));
            return true;
        }
        if (line.starts_with("#"))
        {
            return true;
        }
    }
    else
    {
        // first word is key, the next until the end of theline are avlues
        auto end_first_word = line.find(' ');
        std::string key = line.substr(0, end_first_word);
        std::string value = line.substr(end_first_word + 1);
        building_current_record.values[key] = value;
    }

    return true;
}

void parse_records_from_file(const std::string &filename)
{
    mfprint("file parsing: {}", filename);
    RecordsManager &records = RecordsManager::the();
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return;
    }

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
                records.add_record(current_record);
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
                records.add_record(current_record);
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
        records.add_record(current_record);
    }
    mfprint("file parsed: {}", filename);
}
