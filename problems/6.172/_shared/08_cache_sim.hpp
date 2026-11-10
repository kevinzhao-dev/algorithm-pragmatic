// SPOILER: a complete cache simulator, which answers 6.172/1101 and much of 1103. Finish those first.
//
// 08_cache_sim.hpp — a deterministic set-associative LRU cache model and traced containers.
//
// Stands in for cachegrind in the 6.172 cache exercises: the code under test reads and writes
// through TracedMatrix / TracedArray, and every access updates one or more simulated caches.
// Simulated addresses are independent of real heap layout, so miss counts are fully repeatable.
#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace practice::cachesim {

class Cache {
public:
    Cache(std::size_t capacity_bytes, std::size_t line_bytes, std::size_t ways)
        : capacity_bytes_(capacity_bytes), line_bytes_(line_bytes), ways_(ways) {
        if (capacity_bytes == 0 || !std::has_single_bit(line_bytes) || ways == 0 ||
            capacity_bytes % (line_bytes * ways) != 0 ||
            !std::has_single_bit(capacity_bytes / (line_bytes * ways))) {
            throw std::invalid_argument("capacity must be line_bytes * ways * a power of two");
        }
        line_shift_ = std::countr_zero(line_bytes);
        sets_ = capacity_bytes / (line_bytes * ways);
        set_mask_ = sets_ - 1;
        tags_.assign(sets_ * ways_, 0);
        fill_.assign(sets_, 0);
    }

    std::size_t capacity_bytes() const { return capacity_bytes_; }
    std::size_t line_bytes() const { return line_bytes_; }
    std::size_t ways() const { return ways_; }

    // One line's worth of access; true on a hit.
    bool access(std::uint64_t address) {
        const std::uint64_t line = address >> line_shift_;
        const std::size_t set = static_cast<std::size_t>(line) & set_mask_;
        std::uint64_t* const first = tags_.data() + set * ways_;
        std::size_t& filled = fill_[set];  // the first `filled` ways are valid, MRU first
        ++accesses_;

        std::uint64_t* found = first;
        std::uint64_t* const end = first + filled;
        while (found != end && *found != line) {
            ++found;
        }
        const bool hit = found != end;
        if (!hit) {
            ++misses_;
            if (filled < ways_) {
                ++filled;
            }
            found = first + filled - 1;  // a free way, or the LRU line to evict
        }
        std::move_backward(first, found, found + 1);
        *first = line;
        return hit;
    }

    // Touches each line overlapping [address, address + bytes) exactly once.
    void access_range(std::uint64_t address, std::size_t bytes) {
        if (bytes == 0) {
            return;
        }
        const std::uint64_t first_line = address >> line_shift_;
        const std::uint64_t last_line = (address + (bytes - 1)) >> line_shift_;
        // Written so the loop also ends when last_line is the very top of the address space.
        for (std::uint64_t line = first_line;; ++line) {
            access(line << line_shift_);
            if (line == last_line) {
                break;
            }
        }
    }

    std::uint64_t accesses() const { return accesses_; }
    std::uint64_t misses() const { return misses_; }
    std::uint64_t hits() const { return accesses_ - misses_; }

    void reset_counters() {
        accesses_ = 0;
        misses_ = 0;
    }

    void flush() {
        std::fill(fill_.begin(), fill_.end(), 0);
        reset_counters();
    }

private:

    std::size_t capacity_bytes_;
    std::size_t line_bytes_;
    std::size_t ways_;
    int line_shift_ = 0;
    std::size_t sets_ = 0;
    std::size_t set_mask_ = 0;
    std::vector<std::uint64_t> tags_;
    std::vector<std::size_t> fill_;
    std::uint64_t accesses_ = 0;
    std::uint64_t misses_ = 0;
};

// A simulated address space feeding one or more caches at once. Regions are separated by a
// small stagger so different arrays do not line up on the same sets by coincidence.
class Memory {
public:
    explicit Memory(std::vector<Cache*> caches) : caches_(std::move(caches)) {}
    explicit Memory(Cache& cache) : caches_{&cache} {}

    std::uint64_t allocate(std::size_t bytes) {
        const std::uint64_t base = next_;
        next_ = align_up(next_ + bytes + kStagger, 64);
        return base;
    }

    void touch(std::uint64_t address, std::size_t bytes) {
        for (Cache* cache : caches_) {
            cache->access_range(address, bytes);
        }
    }

    const std::vector<Cache*>& caches() const { return caches_; }

private:
    static constexpr std::size_t kStagger = 17 * 64;

    static std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
        return (value + alignment - 1) / alignment * alignment;
    }

    std::vector<Cache*> caches_;
    std::uint64_t next_ = std::uint64_t{1} << 32;
};

// Row-major storage; get and set report one element access to the simulated caches.
template <typename T = double>
class TracedMatrix {
public:
    TracedMatrix(Memory& memory, std::size_t rows, std::size_t cols, T fill = T{})
        : memory_(&memory),
          rows_(rows),
          cols_(cols),
          base_(memory.allocate(rows * cols * sizeof(T))),
          data_(rows * cols, fill) {}

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }

    T get(std::size_t row, std::size_t col) const {
        const std::size_t index = row * cols_ + col;
        memory_->touch(base_ + index * sizeof(T), sizeof(T));
        return data_[index];
    }

    void set(std::size_t row, std::size_t col, const T& value) {
        const std::size_t index = row * cols_ + col;
        memory_->touch(base_ + index * sizeof(T), sizeof(T));
        data_[index] = value;
    }

    const std::vector<T>& values() const { return data_; }

private:
    Memory* memory_;
    std::size_t rows_;
    std::size_t cols_;
    std::uint64_t base_;
    std::vector<T> data_;
};

template <typename T = double>
class TracedArray {
public:
    TracedArray(Memory& memory, std::size_t size, T fill = T{})
        : memory_(&memory), base_(memory.allocate(size * sizeof(T))), data_(size, fill) {}

    std::size_t size() const { return data_.size(); }

    T get(std::size_t index) const {
        memory_->touch(base_ + index * sizeof(T), sizeof(T));
        return data_[index];
    }

    void set(std::size_t index, const T& value) {
        memory_->touch(base_ + index * sizeof(T), sizeof(T));
        data_[index] = value;
    }

    const std::vector<T>& values() const { return data_; }

private:
    Memory* memory_;
    std::uint64_t base_;
    std::vector<T> data_;
};

// The same matrix interface without any simulation, for functional tests and timing.
template <typename T = double>
class PlainMatrix {
public:
    PlainMatrix(std::size_t rows, std::size_t cols, T fill = T{})
        : rows_(rows), cols_(cols), data_(rows * cols, fill) {}

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
    T get(std::size_t row, std::size_t col) const { return data_[row * cols_ + col]; }
    void set(std::size_t row, std::size_t col, const T& value) { data_[row * cols_ + col] = value; }
    const std::vector<T>& values() const { return data_; }

private:
    std::size_t rows_;
    std::size_t cols_;
    std::vector<T> data_;
};

// The same array interface without any simulation.
template <typename T = double>
class PlainArray {
public:
    explicit PlainArray(std::size_t size, T fill = T{}) : data_(size, fill) {}
    std::size_t size() const { return data_.size(); }
    T get(std::size_t index) const { return data_[index]; }
    void set(std::size_t index, const T& value) { data_[index] = value; }
    const std::vector<T>& values() const { return data_; }

private:
    std::vector<T> data_;
};

}  // namespace practice::cachesim
