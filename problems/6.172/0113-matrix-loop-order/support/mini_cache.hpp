// A small fixed set-associative LRU cache, used only to compare address traces.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class MiniCache {
public:
    MiniCache(std::size_t line_bytes, std::size_t sets, std::size_t ways)
        : line_bytes_(line_bytes), sets_(sets), ways_(ways), tags_(sets * ways, kEmpty) {}

    void access(std::uint64_t address) {
        const std::uint64_t line = address / line_bytes_;
        std::uint64_t* const set = tags_.data() + (line % sets_) * ways_;
        std::size_t way = 0;
        while (way < ways_ && set[way] != line) {
            ++way;
        }
        const bool hit = way < ways_;
        misses_ += hit ? 0 : 1;
        // Move to front; the last slot is the least recently used.
        const std::size_t from = hit ? way : ways_ - 1;
        for (std::size_t i = from; i > 0; --i) {
            set[i] = set[i - 1];
        }
        set[0] = line;
    }

    std::uint64_t misses() const { return misses_; }

private:
    static constexpr std::uint64_t kEmpty = ~std::uint64_t{0};
    std::size_t line_bytes_;
    std::size_t sets_;
    std::size_t ways_;
    std::vector<std::uint64_t> tags_;
    std::uint64_t misses_ = 0;
};
