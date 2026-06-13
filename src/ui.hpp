#pragma once

#include <cstdint>
#include <string>
#include "records.hpp"
int ui();
int ui_init();
void ui_deinit();

Record*  get_selected_record();

void set_selected_record(Record* record);

int display_graph(float& rec_width);
uint64_t try_hash(std::string const &str);
