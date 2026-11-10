// planted_bugs.hpp — small functions with deliberate memory and undefined-behaviour bugs.
//
// Read them, reproduce each one under the tools, then write the corrected versions in
// solution.hpp. Everything is noinline so frames in sanitizer reports stay readable.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace planted {

// Adds `extra` to the old first element, read after the vector may have grown.
[[gnu::noinline]] inline int use_after_realloc(std::vector<int>& values, int extra) {
    const int* first = values.data();
    values.push_back(extra);  // can reallocate and free the block `first` points into
    return *first + extra;
}

// Clones the first n elements of src.
[[gnu::noinline]] inline int* clone_off_by_one(const int* src, std::size_t n) {
    int* copy = new int[n];
    for (std::size_t i = 0; i <= n; ++i) {  // walks one element too far
        copy[i] = src[i];
    }
    return copy;
}

// Copies a NUL-terminated string into a local buffer with no bound check.
[[gnu::noinline]] inline int copy_into_stack(const char* text) {
    char buffer[8];
    char* out = buffer;
    while (*text != '\0') {
        *out++ = *text++;
    }
    *out = '\0';
    return static_cast<int>(std::strlen(buffer));
}

// Owns an array but relies on the compiler-generated (shallow) copy.
struct ShallowBuffer {
    int* data;
    explicit ShallowBuffer(std::size_t n) : data(new int[n]()) {}
    ~ShallowBuffer() { delete[] data; }
};

[[gnu::noinline]] inline int double_release() {
    ShallowBuffer a(4);
    ShallowBuffer b = a;  // both destructors now delete the same pointer
    return b.data[0];
}

// Adds up values in an int that the caller assumes cannot overflow.
[[gnu::noinline]] inline int sum_values(const std::vector<int>& values) {
    int total = 0;
    for (const int value : values) {
        total += value;
    }
    return total;
}

// A mask with the low `count` bits set. Correct for every count except the widest one.
[[gnu::noinline]] inline std::uint32_t low_bits_mask(int count) {
    return (std::uint32_t{1} << count) - 1;
}

// Reads a little-endian u32 out of a byte buffer as if every offset were aligned.
[[gnu::noinline]] inline std::uint32_t read_u32(const std::uint8_t* buffer, std::size_t offset) {
    return *reinterpret_cast<const std::uint32_t*>(buffer + offset);
}

struct ListNode {
    int value;
    ListNode* next;
};

// The second value in the list, assuming the caller always passes at least two nodes.
[[gnu::noinline]] inline int second_value(const ListNode* head) {
    return head->next->value;
}

// Bumps a small histogram without checking the bucket index.
[[gnu::noinline]] inline int record_bucket(int bucket) {
    int counts[4] = {0, 0, 0, 0};
    counts[bucket] += 1;
    return counts[0];
}

}  // namespace planted
