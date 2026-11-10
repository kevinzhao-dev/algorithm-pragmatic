// Runs the racy quicksort so you can watch ThreadSanitizer report it. See the README.
#include <cstdio>
#include <random>
#include <vector>

#include "racy_quicksort.hpp"

int main() {
    practice::set_workers(8);
    std::mt19937_64 engine(7);
    std::vector<int> values(200'000);
    for (int& value : values) {
        value = static_cast<int>(engine() % 1'000'000);
    }
    racy_quicksort(std::span<int>(values));
    std::printf("done, first value %d\n", values.front());
}
