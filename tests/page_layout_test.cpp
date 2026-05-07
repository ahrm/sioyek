#include "page_layout.h"

#include <iostream>
#include <string>

static void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "page_layout_test failed: " << message << "\n";
        std::exit(1);
    }
}

static void expect_spread(TwoPageSpread spread, int first, int second, const std::string& message) {
    expect(spread.first_page == first, message + " first page");
    expect(spread.second_page == second, message + " second page");
}

int main() {
    expect_spread(two_page_spread_for_page(0, 5, false), 0, 1, "no cover page 1");
    expect_spread(two_page_spread_for_page(1, 5, false), 0, 1, "no cover page 2");
    expect_spread(two_page_spread_for_page(2, 5, false), 2, 3, "no cover page 3");
    expect_spread(two_page_spread_for_page(4, 5, false), 4, -1, "no cover odd last page");

    expect_spread(two_page_spread_for_page(0, 5, true), 0, -1, "cover page alone");
    expect_spread(two_page_spread_for_page(1, 5, true), 1, 2, "cover offset pages 2-3");
    expect_spread(two_page_spread_for_page(2, 5, true), 1, 2, "cover offset page 3");
    expect_spread(two_page_spread_for_page(3, 5, true), 3, 4, "cover offset pages 4-5");

    expect(two_page_spread_start_for_page(-10, 6, false) == 0, "negative page clamps to first spread");
    expect(two_page_spread_start_for_page(99, 6, false) == 4, "large page clamps to last spread");
    expect(two_page_spread_start_for_page(99, 6, true) == 5, "large page clamps to last cover-offset spread");

    expect(next_two_page_spread_start(0, 6, false) == 2, "next no-cover spread");
    expect(previous_two_page_spread_start(2, 6, false) == 0, "previous no-cover spread");
    expect(next_two_page_spread_start(0, 6, true) == 1, "next from cover");
    expect(previous_two_page_spread_start(3, 6, true) == 1, "previous cover-offset spread");
    expect(next_two_page_spread_start(5, 6, true) == 5, "next clamps at last spread");

    expect(is_right_page_in_two_page_spread(0, true), "cover page is visually right");
    expect(!is_right_page_in_two_page_spread(1, true), "page 2 is visually left with cover offset");
    expect(is_right_page_in_two_page_spread(2, true), "page 3 is visually right with cover offset");
    expect(!is_right_page_in_two_page_spread(0, false), "page 1 is visually left without cover offset");
    expect(is_right_page_in_two_page_spread(1, false), "page 2 is visually right without cover offset");

    std::cout << "page_layout_test passed\n";
    return 0;
}
