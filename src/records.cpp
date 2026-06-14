
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
    auto added = _add_record(record, defer_sorting);
    if (record.type == RECORD_TYPE_SPANEVENT)
    {
        if (record.is_begin)
        {
            running_span_event[record.group].events.push_back(added);
        }
        else
        {
            auto &open_records = running_span_event[record.group].events;

            ssize_t it = -1;
            for (ssize_t i = 0; i < (ssize_t)open_records.size(); i++)
            {
                if (open_records[i]->name == record.name)
                {
                    it = i;
                    break;
                }
            }
            if (it != -1)
            {
                open_records.erase(open_records.begin() + it);
            }
            else
            {
                printf("WARN: ending event that never started\n");
            }
        }
    }
    // if we schedule, stop span event, and start new ones
    else if (record.type == RECORD_TYPE_SCHEDULER)
    {

        Record *previous = nullptr;
        ssize_t tick = record.tick;

        // get the last scheduler record before this tick
        while (tick >= 0)
        {
            if (records_by_group[record.group].contains(tick))
            {
                for (auto &r2 : records_by_group[record.group][tick])
                {
                    if (r2->type == RECORD_TYPE_SCHEDULER && r2->tick < record.tick)
                    {
                        previous = r2;
                        tick = 0;
                        break;
                    }
                }
            }
            tick--;
        }

        if (previous != nullptr)
        {

            if (previous->pid == record.pid)
            {
                return;
            }

            // save
            if (!saved_span_event.contains(previous->pid))
            {
                saved_span_event[previous->pid] = {};
            }
            if (!running_span_event.contains(previous->group))
            {
                running_span_event[previous->group] = {};
            }

            for (auto r : running_span_event[previous->group].events)
            {
                saved_span_event[previous->pid].events.push_back(r);

                Record copied = *r;
                copied.is_begin = false;
                copied.group = previous->group;
                copied.tick = record.tick - 0.0001f;
                _add_record(copied, defer_sorting);
            }
            running_span_event[previous->group].events.clear();
            return;
        }

        // load
        if (!saved_span_event.contains(record.pid))
        {
            saved_span_event[record.pid] = {};
        }
        running_span_event[record.group].events = saved_span_event[record.pid].events;
        for (auto r : running_span_event[record.group].events)
        {
            Record copied = *r;
            copied.group = record.group;
            copied.tick = record.tick;

            _add_record(copied, defer_sorting);
        }

        saved_span_event[record.pid].events.clear();
    }
}

Record *RecordsManager::_add_record(Record &record, bool defer_sorting)
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

    // if we start a span event, save it for context switch
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

    return r;
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
    else if (line.contains("@st:event:begin"))
    {
        printf("⁼== adding record = %s\n", building_current_record.name.c_str());
        if (has_record)
        {
            this->add_record(building_current_record, defer_sorting);
            building_current_record = {};
        }

        building_current_record.type = RECORD_TYPE_SPANEVENT;
        building_current_record.is_begin = true;
        has_record = true;
        return true;
    }
    else if (line.contains("@st:event:end"))
    {
        printf("⁼== adding record = %s\n", building_current_record.name.c_str());
        if (has_record)
        {
            this->add_record(building_current_record, defer_sorting);
            building_current_record = {};
        }

        building_current_record.type = RECORD_TYPE_SPANEVENT;
        building_current_record.is_begin = false;
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
        if (line.starts_with("#pid"))
        {
            building_current_record.pid = std::stol(line.substr(strlen("#pid ")));
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
    while (std::getline(file, line))
    {
        records.record_parser_update_from_line(line, false);
    }

    mfprint("file parsed: {}", filename);
}
