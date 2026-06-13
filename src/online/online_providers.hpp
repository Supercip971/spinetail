#pragma once



#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include "records.hpp"


/*
 * A provider gives the record entry a line,
 * and can be queried for whether there is a next line.
 *
 * get_line: Calling get_line is expected to destroy the current buffer.
 * update: Calling update is expected to update the buffer with the next line, and returns true if we have a next line.
 */

class Provider
{
    public:
    virtual std::optional<std::string> get_data() = 0;
    virtual ~Provider() = default;
};


class EmptyProvider : public Provider
{
    public:
    std::optional<std::string> get_data() override { return std::nullopt; }
};

class FileWatcherProvider : public Provider
{
    size_t _last_seek = {};
    std::string _path = {};
    std::filesystem::file_time_type _last_edit = {};
    std::ifstream _file;


    public:
    FileWatcherProvider(const std::string& path);
    std::optional<std::string> get_data() override;
};
