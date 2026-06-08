#pragma once

#include <iostream>
template <typename... Args>
inline void mfprint(auto const &rt_fmt_str, Args &&...args)
{
    std::cout << std::vformat(rt_fmt_str, std::make_format_args(args)...) << std::endl;
}
