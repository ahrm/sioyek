#include "page_layout.h"

#include <algorithm>

int clamp_page_number(int page, int num_pages) {
    if (num_pages <= 0) {
        return -1;
    }
    return std::max(0, std::min(page, num_pages - 1));
}

int two_page_spread_start_for_page(int page, int num_pages, bool cover_offset) {
    page = clamp_page_number(page, num_pages);
    if (page < 0) {
        return -1;
    }
    if (cover_offset) {
        if (page == 0) {
            return 0;
        }
        return ((page - 1) / 2) * 2 + 1;
    }
    return (page / 2) * 2;
}

int last_two_page_spread_start(int num_pages, bool cover_offset) {
    if (num_pages <= 0) {
        return -1;
    }
    if (cover_offset) {
        if (num_pages == 1) {
            return 0;
        }
        return ((num_pages - 2) / 2) * 2 + 1;
    }
    return ((num_pages - 1) / 2) * 2;
}

int next_two_page_spread_start(int spread_start, int num_pages, bool cover_offset) {
    if (num_pages <= 0) {
        return -1;
    }

    spread_start = two_page_spread_start_for_page(spread_start, num_pages, cover_offset);
    if (spread_start < 0) {
        return -1;
    }

    int next_start = spread_start + 2;
    if (cover_offset && spread_start == 0) {
        next_start = 1;
    }

    return std::min(next_start, last_two_page_spread_start(num_pages, cover_offset));
}

int previous_two_page_spread_start(int spread_start, int num_pages, bool cover_offset) {
    if (num_pages <= 0) {
        return -1;
    }

    spread_start = two_page_spread_start_for_page(spread_start, num_pages, cover_offset);
    if (spread_start < 0) {
        return -1;
    }

    int previous_start = spread_start - 2;
    if (cover_offset && spread_start <= 1) {
        previous_start = 0;
    }

    return std::max(0, previous_start);
}

TwoPageSpread two_page_spread_for_page(int page, int num_pages, bool cover_offset) {
    TwoPageSpread spread;
    spread.first_page = two_page_spread_start_for_page(page, num_pages, cover_offset);
    if (spread.first_page < 0) {
        return spread;
    }

    if (cover_offset && spread.first_page == 0) {
        spread.second_page = -1;
    }
    else if (spread.first_page + 1 < num_pages) {
        spread.second_page = spread.first_page + 1;
    }

    return spread;
}

bool is_right_page_in_two_page_spread(int page, bool cover_offset) {
    return ((page + (cover_offset ? 1 : 0)) % 2) == 1;
}
