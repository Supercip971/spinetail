

#pragma once
#include "online_providers.hpp"
#include "records.hpp"
class ProviderParser
{
    std::string building_buffer = "";

public:
    Provider *_provider;
    ProviderParser(Provider *provider) : _provider(provider) {}

    std::optional<Record> update()
    {
        std::optional<std::string> data = _provider->get_data();
        if (!data.has_value())
            return std::nullopt;
        building_buffer += *data;

        std::stringstream ss(building_buffer);
        std::string line = "";
        auto old_tellg = ss.tellg();
        while (std::getline(ss, line))
        {
            RecordsManager::the().record_parser_update_from_line(line, false);

            printf("line: %s\n", line.c_str());
            printf("tellg: %ld\n", (long)ss.tellg());
            old_tellg = ss.tellg();
        }

        building_buffer = building_buffer.substr(old_tellg);
        return std::nullopt;
    }
};
