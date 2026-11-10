// replay.hpp — runs a trace against an allocator and verifies every result from the outside.
//
// validate() is the black-box checker every stage is graded with. It writes a slot-specific byte
// pattern into each block and re-reads all of it before the block is released or moved, so it
// notices blocks that overlap, that leave the heap, that are misaligned, or that lost data.
// run_ops() performs the same operations with no checking, for timing.
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "alloc_counter.hpp"
#include "trace.hpp"

namespace trace {

struct Report {
    std::string error;  // empty when every operation was valid
    std::uint64_t peak_live = 0;
    std::uint64_t heap_bytes = 0;
    std::size_t operations = 0;

    double utilization() const {
        return heap_bytes == 0 ? 0.0 : static_cast<double>(peak_live) / static_cast<double>(heap_bytes);
    }
};

namespace detail {

inline std::uint8_t pattern_byte(std::uint64_t seed, std::size_t index) {
    return static_cast<std::uint8_t>((seed * 0x9E3779B97F4A7C15ULL ^ index * 0xC2B2AE3D27D4EB4FULL) >> 56);
}

inline void fill(std::byte* at, std::size_t size, std::uint64_t seed) {
    for (std::size_t i = 0; i < size; ++i) {
        at[i] = static_cast<std::byte>(pattern_byte(seed, i));
    }
}

inline std::size_t first_mismatch(const std::byte* at, std::size_t size, std::uint64_t seed) {
    for (std::size_t i = 0; i < size; ++i) {
        if (at[i] != static_cast<std::byte>(pattern_byte(seed, i))) {
            return i;
        }
    }
    return size;
}

struct LiveBlock {
    std::byte* payload = nullptr;
    std::size_t size = 0;
    std::uint64_t seed = 0;
};

// The live byte ranges of all current blocks, to answer "does this new range overlap one?".
class RangeSet {
public:
    bool insert_if_free(std::byte* begin, std::size_t size) {
        const auto lo = reinterpret_cast<std::uintptr_t>(begin);
        const std::uintptr_t hi = lo + size;
        auto next = ranges_.lower_bound(lo);
        if (next != ranges_.end() && next->first < hi) {
            return false;
        }
        if (next != ranges_.begin()) {
            auto previous = std::prev(next);
            if (previous->second > lo) {
                return false;
            }
        }
        ranges_.emplace_hint(next, lo, hi);
        return true;
    }

    void erase(std::byte* begin) { ranges_.erase(reinterpret_cast<std::uintptr_t>(begin)); }

private:
    std::map<std::uintptr_t, std::uintptr_t> ranges_;
};

}  // namespace detail

// check_every: run allocator.check() after every n-th operation (0 disables it).
template <typename Allocator>
Report validate(Allocator& allocator, const Trace& t, std::size_t check_every = 1) {
    Report report;
    report.peak_live = t.peak_live;
    std::vector<detail::LiveBlock> slots(t.slots);
    detail::RangeSet ranges;

    std::size_t index = 0;
    const auto where = [&] {
        std::ostringstream out;
        out << t.name << " op " << index << ": ";
        return out.str();
    };
    const auto fail = [&](const std::string& message) {
        report.error = where() + message;
        return report;
    };

    const auto accept_block = [&](void* raw, std::size_t size, detail::LiveBlock& block) -> std::string {
        if (raw == nullptr) {
            return "allocation returned nullptr although the heap has room";
        }
        auto* payload = static_cast<std::byte*>(raw);
        if (reinterpret_cast<std::uintptr_t>(payload) % 16 != 0) {
            return "misaligned pointer (must be 16-byte aligned)";
        }
        const auto heap_low = reinterpret_cast<std::uintptr_t>(allocator.arena().base());
        const auto heap_high = heap_low + allocator.arena().used();
        const auto low = reinterpret_cast<std::uintptr_t>(payload);
        if (low < heap_low || low + size > heap_high) {
            return "block lies outside the heap";
        }
        if (!ranges.insert_if_free(payload, size)) {
            return "block overlaps a live block";
        }
        block.payload = payload;
        block.size = size;
        return {};
    };

    // Runs one allocator call and remembers whether it touched the C++ heap.
    bool used_operator_new = false;
    const auto guarded = [&](auto&& call) {
        const std::uint64_t before = alloc_counter::operator_new_calls();
        call();
        used_operator_new |= alloc_counter::operator_new_calls() != before;
    };

    for (; index < t.ops.size(); ++index) {
        const Op op = t.ops[index];
        detail::LiveBlock& slot = slots[op.slot];
        switch (op.kind) {
            case Kind::Malloc: {
                void* raw = nullptr;
                guarded([&] { raw = allocator.malloc(op.size); });
                if (auto message = accept_block(raw, op.size, slot); !message.empty()) {
                    return fail(message);
                }
                slot.seed = static_cast<std::uint64_t>(index) + 1;
                detail::fill(slot.payload, slot.size, slot.seed);
                break;
            }
            case Kind::Free: {
                if (const std::size_t at = detail::first_mismatch(slot.payload, slot.size, slot.seed); at != slot.size) {
                    return fail("lost data: a live block was overwritten before it was freed");
                }
                guarded([&] { allocator.free(slot.payload); });
                ranges.erase(slot.payload);
                slot = {};
                break;
            }
            case Kind::Realloc: {
                const std::size_t old_size = slot.size;
                const std::uint64_t old_seed = slot.seed;
                void* raw = nullptr;
                guarded([&] { raw = allocator.realloc(slot.payload, op.size); });
                ranges.erase(slot.payload);
                if (auto message = accept_block(raw, op.size, slot); !message.empty()) {
                    return fail(message);
                }
                const std::size_t preserved = std::min<std::size_t>(old_size, op.size);
                if (detail::first_mismatch(slot.payload, preserved, old_seed) != preserved) {
                    return fail("lost data: realloc did not preserve the old contents");
                }
                slot.seed = static_cast<std::uint64_t>(index) + 1;
                detail::fill(slot.payload, slot.size, slot.seed);
                break;
            }
        }
        if (check_every != 0 && index % check_every == 0 && !allocator.check()) {
            return fail("check() reported a corrupt heap");
        }
        if (used_operator_new) {
            return fail("the allocator called operator new; its metadata must live in the arena");
        }
    }
    if (check_every != 0 && !allocator.check()) {
        report.error = "check() reported a corrupt heap at the end of " + t.name;
    }
    report.heap_bytes = allocator.heap_bytes();
    report.operations = t.ops.size();
    return report;
}

// Executes the trace with no checking. The caller times it.
template <typename Allocator>
void run_ops(Allocator& allocator, const Trace& t, std::vector<void*>& scratch) {
    scratch.assign(t.slots, nullptr);
    for (const Op& op : t.ops) {
        switch (op.kind) {
            case Kind::Malloc:
                scratch[op.slot] = allocator.malloc(op.size);
                *static_cast<volatile char*>(scratch[op.slot]) = 1;
                break;
            case Kind::Free:
                allocator.free(scratch[op.slot]);
                break;
            case Kind::Realloc:
                scratch[op.slot] = allocator.realloc(scratch[op.slot], op.size);
                *static_cast<volatile char*>(scratch[op.slot]) = 1;
                break;
        }
    }
}

}  // namespace trace
