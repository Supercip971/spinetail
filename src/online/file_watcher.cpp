#include <filesystem>
#include <fstream>
#include "online_providers.hpp"
FileWatcherProvider::FileWatcherProvider(const std::string &path)
    : _last_seek(0), _path(path)
{
    _file.open(_path, std::ios::in);

}

std::optional<std::string> FileWatcherProvider::get_data()
{
    std::filesystem::path p = _path;
    if(!std::filesystem::exists(p))
    {
        printf("file not found: %s\n", _path.c_str());
        _file.close();
        return std::nullopt;
    }

    auto t = std::filesystem::last_write_time(p);

    if (t != _last_edit)
    {
        _file.clear();
        printf(" === file changed! === \n");

        printf("seek to: %ld\n", (long)_last_seek);

        _file.seekg(0, std::ios::end);
        auto end = _file.tellg();

        _file.seekg(_last_seek, std::ios::beg);
        std::string line = "";
        std::string data = "";

        while(_file.tellg() < end)
        {
            auto c = _file.get();
            line += c;
            if(c == '\n')
            {
                data += line;
                line = "";
                _last_seek = _file.tellg(); // save seek position of the last successful read line
            }
        }

        printf("read %ld bytes\n", (long)data.length());

        if(_file.tellg() == -1){ // end reached before newline
            _file.clear();
            _file.seekg(0, std::ios::end);
        }
        _last_edit = t;

        if (data.length() == 0)
            return std::nullopt;

        return data;
    }
    return std::nullopt;
}
