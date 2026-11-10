// racy_quicksort.hpp — a parallel quicksort with a real determinacy race. Reproduce it under
// ThreadSanitizer (see the README), then write the corrected version in solution.hpp.
#pragma once

#include <cstddef>
#include <span>
#include <utility>

#include "forkjoin.hpp"

template <typename T>
void racy_quicksort(std::span<T> a) {
    if (a.size() < 2) {
        return;
    }
    const T pivot = a[a.size() / 2];
    std::size_t i = 0;
    std::size_t j = a.size() - 1;
    for (;;) {
        while (a[i] < pivot) {
            ++i;
        }
        while (pivot < a[j]) {
            --j;
        }
        if (i >= j) {
            break;
        }
        std::swap(a[i], a[j]);
        ++i;
        --j;
    }
    // The two recursive calls both include position j, and both write to it.
    const std::size_t split = j;
    practice::fork2([&] { racy_quicksort(a.first(split + 1)); },
                    [&] { racy_quicksort(a.subspan(split)); });
}
