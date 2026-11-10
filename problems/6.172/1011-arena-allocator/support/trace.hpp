// trace.hpp — deterministic allocation traces. Same seed, same trace.
//
// A trace is a list of malloc / free / realloc operations on numbered slots, each slot holding at
// most one live block. peak_live is the most requested bytes ever live at once.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace trace {

enum class Kind : std::uint8_t { Malloc, Free, Realloc };

struct Op {
    Kind kind;
    std::uint32_t slot;
    std::uint32_t size;
};

struct Trace {
    std::string name;
    std::vector<Op> ops;
    std::size_t slots = 0;
    std::uint64_t peak_live = 0;
    std::uint64_t total_requested = 0;
};

class SplitMix {
public:
    explicit SplitMix(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Uniform in the closed interval [low, high].
    std::uint32_t range(std::uint32_t low, std::uint32_t high) {
        return low + static_cast<std::uint32_t>(next() % (std::uint64_t{high} - low + 1));
    }

    bool coin(std::uint32_t percent) { return range(0, 99) < percent; }

private:
    std::uint64_t state_;
};

// Records operations on behalf of a generator and keeps the bookkeeping honest.
class Builder {
public:
    Builder(std::string name, std::uint64_t seed) : rng(seed) { trace_.name = std::move(name); }

    SplitMix rng;

    std::uint32_t malloc_block(std::uint32_t size) {
        std::uint32_t slot;
        if (recycled_.empty()) {
            slot = static_cast<std::uint32_t>(sizes_.size());
            sizes_.push_back(0);
            live_slots_positions_.push_back(0);
        } else {
            slot = recycled_.back();
            recycled_.pop_back();
        }
        sizes_[slot] = size;
        live_ += size;
        live_slots_positions_[slot] = static_cast<std::uint32_t>(live_slots_.size());
        live_slots_.push_back(slot);
        trace_.total_requested += size;
        trace_.peak_live = std::max(trace_.peak_live, live_);
        trace_.ops.push_back({Kind::Malloc, slot, size});
        return slot;
    }

    void free_block(std::uint32_t slot) {
        trace_.ops.push_back({Kind::Free, slot, 0});
        live_ -= sizes_[slot];
        sizes_[slot] = 0;
        const std::uint32_t position = live_slots_positions_[slot];
        const std::uint32_t last = live_slots_.back();
        live_slots_[position] = last;
        live_slots_positions_[last] = position;
        live_slots_.pop_back();
        recycled_.push_back(slot);
    }

    void realloc_block(std::uint32_t slot, std::uint32_t size) {
        trace_.total_requested += size;
        live_ += size;
        live_ -= sizes_[slot];
        sizes_[slot] = size;
        trace_.peak_live = std::max(trace_.peak_live, live_);
        trace_.ops.push_back({Kind::Realloc, slot, size});
    }

    std::size_t live_count() const { return live_slots_.size(); }
    std::uint32_t size_of(std::uint32_t slot) const { return sizes_[slot]; }

    std::uint32_t random_live() {
        return live_slots_[rng.range(0, static_cast<std::uint32_t>(live_slots_.size()) - 1)];
    }

    Trace finish() {
        while (!live_slots_.empty()) {
            free_block(live_slots_.back());
        }
        trace_.slots = sizes_.size();
        return std::move(trace_);
    }

private:
    Trace trace_;
    std::vector<std::uint32_t> sizes_;
    std::vector<std::uint32_t> recycled_;
    std::vector<std::uint32_t> live_slots_;
    std::vector<std::uint32_t> live_slots_positions_;
    std::uint64_t live_ = 0;
};

namespace detail {

// Small requests are the common case in real programs; a few sizes are much more frequent.
inline std::uint32_t small_size(SplitMix& rng) {
    static constexpr std::uint32_t common[] = {8, 16, 24, 32, 48, 64};
    if (rng.coin(50)) {
        return common[rng.range(0, 5)];
    }
    return rng.range(1, 256);
}

inline std::uint32_t mixed_size(SplitMix& rng) {
    const std::uint32_t pick = rng.range(0, 99);
    if (pick < 70) {
        return rng.range(8, 256);
    }
    if (pick < 95) {
        return rng.range(512, 8192);
    }
    return rng.range(16 * 1024, 128 * 1024);
}

}  // namespace detail

// Small objects with random lifetimes around a steady population.
inline Trace small_churn(std::uint64_t seed, std::size_t scale) {
    Builder b("small-churn", seed);
    const std::size_t population = 600 * scale;
    const std::size_t steps = 8000 * scale;
    for (std::size_t step = 0; step < steps; ++step) {
        const bool grow = b.live_count() < population / 2 ||
                          (b.live_count() < population && b.rng.coin(55));
        if (grow) {
            b.malloc_block(detail::small_size(b.rng));
        } else {
            b.free_block(b.random_live());
        }
    }
    return b.finish();
}

// A skewed mix of small, medium and occasional large requests.
inline Trace mixed_sizes(std::uint64_t seed, std::size_t scale) {
    Builder b("mixed-sizes", seed);
    const std::size_t population = 300 * scale;
    const std::size_t steps = 6000 * scale;
    for (std::size_t step = 0; step < steps; ++step) {
        if (b.live_count() < population && (b.live_count() < population / 2 || b.rng.coin(55))) {
            b.malloc_block(detail::mixed_size(b.rng));
        } else {
            b.free_block(b.random_live());
        }
    }
    return b.finish();
}

// Allocate a big batch, free most of it in random order, leave survivors behind, repeat.
inline Trace sawtooth(std::uint64_t seed, std::size_t scale) {
    Builder b("sawtooth", seed);
    const std::size_t batch = 400 * scale;
    for (int round = 0; round < 12; ++round) {
        for (std::size_t i = 0; i < batch; ++i) {
            b.malloc_block(b.rng.range(64, 1024));
        }
        const std::size_t survivors = b.live_count() / 10;
        while (b.live_count() > survivors) {
            b.free_block(b.random_live());
        }
    }
    return b.finish();
}

// Fills memory with neighbours, frees every other one, frees the rest, then asks for blocks too
// big for any single hole. The heap stays flat only if neighbouring holes really merged.
inline Trace checkerboard(std::uint64_t seed, std::size_t scale) {
    Builder b("checkerboard", seed);
    const std::size_t rows = 500 * scale;
    for (int round = 0; round < 6; ++round) {
        const auto small = b.rng.range(40, 72);
        std::vector<std::uint32_t> even;
        std::vector<std::uint32_t> odd;
        for (std::size_t i = 0; i < rows; ++i) {
            even.push_back(b.malloc_block(small));
            odd.push_back(b.malloc_block(small));
        }
        for (const std::uint32_t slot : even) {
            b.free_block(slot);
        }
        for (const std::uint32_t slot : odd) {
            b.free_block(slot);
        }
        // Each request covers several of the old holes: no single hole can serve it.
        const auto large = static_cast<std::uint32_t>(small * 32);
        std::vector<std::uint32_t> big;
        for (std::size_t i = 0; i < rows / 32; ++i) {
            big.push_back(b.malloc_block(large));
        }
        for (const std::uint32_t slot : big) {
            b.free_block(slot);
        }
    }
    return b.finish();
}

// Buffers growing a little at a time, interleaved with short-lived allocations.
inline Trace realloc_growth(std::uint64_t seed, std::size_t scale) {
    Builder b("realloc-growth", seed);
    const std::size_t buffers = 8;
    std::vector<std::uint32_t> slots;
    for (std::size_t i = 0; i < buffers; ++i) {
        slots.push_back(b.malloc_block(64));
    }
    const std::size_t steps = 3000 * scale;
    std::vector<std::uint32_t> transient;
    for (std::size_t step = 0; step < steps; ++step) {
        const std::uint32_t slot = slots[b.rng.range(0, buffers - 1)];
        const std::uint32_t grown = b.size_of(slot) + b.rng.range(16, 64);
        b.realloc_block(slot, grown);
        if (b.rng.coin(30)) {
            transient.push_back(b.malloc_block(b.rng.range(16, 64)));
        }
        if (transient.size() > 32) {
            b.free_block(transient.front());
            transient.erase(transient.begin());
        }
        // Occasionally a buffer is trimmed, as a string or vector would be after a shrink.
        if (b.rng.coin(2)) {
            const std::uint32_t victim = slots[b.rng.range(0, buffers - 1)];
            b.realloc_block(victim, std::max<std::uint32_t>(32, b.size_of(victim) / 2));
        }
    }
    return b.finish();
}

// Strictly last-in first-out lifetimes, as call-scoped buffers are.
inline Trace stack_lifo(std::uint64_t seed, std::size_t scale) {
    Builder b("stack-lifo", seed);
    std::vector<std::uint32_t> stack;
    const std::size_t steps = 8000 * scale;
    const std::size_t depth = 400 * scale;
    for (std::size_t step = 0; step < steps; ++step) {
        if (stack.size() < depth && (stack.empty() || b.rng.coin(52))) {
            stack.push_back(b.malloc_block(detail::mixed_size(b.rng) % 2048 + 1));
        } else {
            b.free_block(stack.back());
            stack.pop_back();
        }
    }
    return b.finish();
}

// Long-lived objects allocated gradually among many short-lived ones, the classic source of
// fragmentation in servers: each long-lived block can pin a hole between short-lived ones.
inline Trace long_lived(std::uint64_t seed, std::size_t scale) {
    Builder b("long-lived", seed);
    const std::size_t steps = 8000 * scale;
    std::vector<std::uint32_t> short_lived;
    for (std::size_t step = 0; step < steps; ++step) {
        if (b.rng.coin(5)) {
            b.malloc_block(b.rng.range(128, 1024));
        }
        short_lived.push_back(b.malloc_block(detail::small_size(b.rng)));
        if (short_lived.size() > 64) {
            const std::uint32_t index = b.rng.range(0, static_cast<std::uint32_t>(short_lived.size()) - 1);
            b.free_block(short_lived[index]);
            short_lived[index] = short_lived.back();
            short_lived.pop_back();
        }
    }
    return b.finish();
}

inline std::vector<Trace> all_shapes(std::uint64_t seed, std::size_t scale) {
    std::vector<Trace> traces;
    traces.push_back(small_churn(seed, scale));
    traces.push_back(mixed_sizes(seed, scale));
    traces.push_back(sawtooth(seed, scale));
    traces.push_back(checkerboard(seed, scale));
    traces.push_back(realloc_growth(seed, scale));
    traces.push_back(stack_lifo(seed, scale));
    traces.push_back(long_lived(seed, scale));
    return traces;
}

}  // namespace trace
