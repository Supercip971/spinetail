
#include "records.hpp"
#include "utils.hpp"

RecordsManager &RecordsManager::the()
{
    static RecordsManager instance;
    return instance;
}

void RecordsManager::add_record(Record &record)
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
