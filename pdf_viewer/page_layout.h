#pragma once

struct TwoPageSpread {
    int first_page = -1;
    int second_page = -1;
};

int clamp_page_number(int page, int num_pages);
int two_page_spread_start_for_page(int page, int num_pages, bool cover_offset);
int last_two_page_spread_start(int num_pages, bool cover_offset);
int next_two_page_spread_start(int spread_start, int num_pages, bool cover_offset);
int previous_two_page_spread_start(int spread_start, int num_pages, bool cover_offset);
TwoPageSpread two_page_spread_for_page(int page, int num_pages, bool cover_offset);
bool is_right_page_in_two_page_spread(int page, bool cover_offset);
