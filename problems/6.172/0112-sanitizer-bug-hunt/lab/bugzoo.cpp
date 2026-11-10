// Runs one planted bug so you can reproduce it under the tools. Build and run it by hand:
// see the README of this exercise. Not part of the graded build.
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>

#include "planted_bugs.hpp"

// There is no LeakSanitizer on Apple Silicon. Run this one with: leaks --atExit -- ./bugzoo leak
[[gnu::noinline]] static int forgotten_allocation() {
    int* block = new int[32]();
    block[0] = 42;
    return block[0];
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr,
                     "usage: bugzoo <use-after-realloc|off-by-one|stack|double-free|overflow|"
                     "shift|misaligned|null|bounds|leak>\n");
        return 2;
    }
    const std::string_view scenario = argv[1];
    if (scenario == "use-after-realloc") {
        std::vector<int> values = {1, 2, 3};
        std::printf("%d\n", planted::use_after_realloc(values, 9));
    } else if (scenario == "off-by-one") {
        const int source[] = {1, 2, 3, 4, 5};
        delete[] planted::clone_off_by_one(source, 4);
    } else if (scenario == "stack") {
        std::printf("%d\n", planted::copy_into_stack("much too long for eight bytes"));
    } else if (scenario == "double-free") {
        std::printf("%d\n", planted::double_release());
    } else if (scenario == "overflow") {
        std::printf("%d\n", planted::sum_values({INT_MAX, 1}));
    } else if (scenario == "shift") {
        std::printf("%u\n", planted::low_bits_mask(32));
    } else if (scenario == "misaligned") {
        std::uint8_t bytes[16] = {1, 2, 3, 4, 5, 6, 7, 8};
        std::printf("%u\n", planted::read_u32(bytes, 1));
    } else if (scenario == "null") {
        const planted::ListNode only{7, nullptr};
        std::printf("%d\n", planted::second_value(&only));
    } else if (scenario == "bounds") {
        std::printf("%d\n", planted::record_bucket(4));
    } else if (scenario == "leak") {
        std::printf("%d\n", forgotten_allocation());
    } else {
        std::fprintf(stderr, "unknown scenario\n");
        return 2;
    }
    return 0;
}
