// tt.hpp — a lock-free transposition table.
//
// Each entry is two atomic words, key ^ data and data, so a torn entry (a reader racing a writer)
// decodes to a key mismatch and is discarded instead of being trusted.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "board.hpp"

enum class Bound : std::uint8_t { Exact, Lower, Upper };

struct TTEntry {
    std::uint64_t key = 0;
    int score = 0;
    int depth = 0;
    Bound bound = Bound::Exact;
    Move best = kNoMove;
};

class TranspositionTable {
public:
    explicit TranspositionTable(int log2_entries)
        : mask_((std::size_t{1} << log2_entries) - 1),
          slots_(std::make_unique<Slot[]>(mask_ + 1)) {}

    std::optional<TTEntry> probe(std::uint64_t key) const {
        const Slot& slot = slots_[static_cast<std::size_t>(key) & mask_];
        const std::uint64_t data = slot.data.load(std::memory_order_relaxed);
        const std::uint64_t checked = slot.key_xor_data.load(std::memory_order_relaxed);
        if ((checked ^ data) != key || (data & kOccupied) == 0) {
            return std::nullopt;
        }
        return unpack(key, data);
    }

    void store(const TTEntry& entry) {
        Slot& slot = slots_[static_cast<std::size_t>(entry.key) & mask_];
        const std::uint64_t data = pack(entry);
        slot.data.store(data, std::memory_order_relaxed);
        slot.key_xor_data.store(entry.key ^ data, std::memory_order_relaxed);
    }

    void clear() {
        for (std::size_t i = 0; i <= mask_; ++i) {
            slots_[i].data.store(0, std::memory_order_relaxed);
            slots_[i].key_xor_data.store(0, std::memory_order_relaxed);
        }
    }

    std::size_t size() const { return mask_ + 1; }

private:
    struct Slot {
        std::atomic<std::uint64_t> key_xor_data{0};
        std::atomic<std::uint64_t> data{0};
    };

    static constexpr std::uint64_t kOccupied = std::uint64_t{1} << 63;

    static std::uint64_t pack(const TTEntry& e) {
        const auto score = static_cast<std::uint16_t>(static_cast<std::int16_t>(e.score));
        return kOccupied | static_cast<std::uint64_t>(score) |
               (static_cast<std::uint64_t>(static_cast<std::uint8_t>(e.depth)) << 16) |
               (static_cast<std::uint64_t>(e.bound) << 24) |
               (static_cast<std::uint64_t>(static_cast<std::uint8_t>(e.best.from)) << 26) |
               (static_cast<std::uint64_t>(static_cast<std::uint8_t>(e.best.to)) << 34);
    }

    static TTEntry unpack(std::uint64_t key, std::uint64_t data) {
        TTEntry e;
        e.key = key;
        e.score = static_cast<std::int16_t>(static_cast<std::uint16_t>(data));
        e.depth = static_cast<std::int8_t>(static_cast<std::uint8_t>(data >> 16));
        e.bound = static_cast<Bound>((data >> 24) & 3);
        e.best.from = static_cast<std::int8_t>(static_cast<std::uint8_t>(data >> 26));
        e.best.to = static_cast<std::int8_t>(static_cast<std::uint8_t>(data >> 34));
        return e;
    }

    std::size_t mask_;
    std::unique_ptr<Slot[]> slots_;
};
