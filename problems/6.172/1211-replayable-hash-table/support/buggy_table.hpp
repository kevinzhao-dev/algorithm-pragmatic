// A small open-addressing table that forgets to wrap the probe around the end of the array.
// Real code would corrupt memory; this model records the out-of-bounds probe in corrupted().
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class BuggyTable {
public:
    explicit BuggyTable(std::size_t capacity) : slots_(capacity) {}

    void insert(std::uint64_t key, std::uint64_t value) {
        std::size_t slot = home(key);
        while (true) {
            if (slot == slots_.size()) {
                corrupted_ = true;  // the probe ran past the last slot
                return;
            }
            if (slots_[slot].key == 0) {
                slots_[slot] = {key, value};
                return;
            }
            ++slot;
        }
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        for (std::size_t slot = home(key); slot < slots_.size() && slots_[slot].key != 0; ++slot) {
            if (slots_[slot].key == key) {
                return slots_[slot].value;
            }
        }
        return std::nullopt;
    }

    bool corrupted() const { return corrupted_; }

private:
    struct Slot {
        std::uint64_t key = 0;
        std::uint64_t value = 0;
    };

    std::size_t home(std::uint64_t key) const {
        return static_cast<std::size_t>((key * 0x9E3779B97F4A7C15ULL) >> 32) % slots_.size();
    }

    std::vector<Slot> slots_;
    bool corrupted_ = false;
};
