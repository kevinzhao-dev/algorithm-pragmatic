#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

struct Slot {
    std::int64_t base;
    std::int64_t burst;
};

inline std::int64_t max_throughput(std::span<const Slot> slots, std::size_t cooldown = 2) {
    const std::size_t n = slots.size();
    cooldown = std::min(cooldown, n);
    // ring[j % (cooldown + 1)] holds x(j), the best total for slots j..n-1, for the most
    // recent cooldown + 1 values of j (zero beyond the end).
    std::vector<std::int64_t> ring(cooldown + 1, 0);
    std::int64_t window_base = 0;  // base of slots i+1 .. i+cooldown
    for (std::size_t i = n; i-- > 0;) {
        const std::int64_t next = ring[(i + 1) % (cooldown + 1)];
        const std::int64_t after_cooldown = ring[i % (cooldown + 1)];
        const std::int64_t best =
            std::max(slots[i].base + next, slots[i].burst + window_base + after_cooldown);
        ring[i % (cooldown + 1)] = best;
        if (cooldown > 0) {
            window_base += slots[i].base;
            if (i + cooldown < n) {
                window_base -= slots[i + cooldown].base;
            }
        }
    }
    return n == 0 ? 0 : ring[0];
}

inline bool can_meet_quota(std::span<const Slot> slots, std::int64_t quota,
                           std::size_t cooldown = 2) {
    return max_throughput(slots, cooldown) >= quota;
}

inline std::vector<std::size_t> burst_plan(std::span<const Slot> slots, std::size_t cooldown = 2) {
    const std::size_t n = slots.size();
    cooldown = std::min(cooldown, n);
    std::vector<std::int64_t> sum_base(n + 1, 0);
    for (std::size_t i = 0; i < n; ++i) {
        sum_base[i + 1] = sum_base[i] + slots[i].base;
    }
    std::vector<std::int64_t> x(n + cooldown + 2, 0);
    for (std::size_t i = n; i-- > 0;) {
        const std::size_t resume = std::min(i + cooldown + 1, n);
        x[i] = std::max(slots[i].base + x[i + 1],
                        slots[i].burst + (sum_base[resume] - sum_base[i + 1]) + x[resume]);
    }

    std::vector<std::size_t> plan;
    for (std::size_t i = 0; i < n;) {
        if (x[i] == slots[i].base + x[i + 1]) {
            ++i;
        } else {
            plan.push_back(i);
            i += cooldown + 1;
        }
    }
    return plan;
}

inline std::int64_t max_throughput_limited(std::span<const Slot> slots, std::size_t cooldown,
                                           std::size_t max_bursts) {
    const std::size_t n = slots.size();
    cooldown = std::min(cooldown, n);
    max_bursts = std::min(max_bursts, n);
    std::vector<std::int64_t> sum_base(n + 1, 0);
    for (std::size_t i = 0; i < n; ++i) {
        sum_base[i + 1] = sum_base[i] + slots[i].base;
    }
    // previous[i]: best total for slots i..n-1 with one fewer burst available.
    std::vector<std::int64_t> previous(n + 1, 0);
    for (std::size_t i = 0; i <= n; ++i) {
        previous[i] = sum_base[n] - sum_base[i];
    }
    std::vector<std::int64_t> current = previous;
    for (std::size_t budget = 1; budget <= max_bursts; ++budget) {
        for (std::size_t i = n; i-- > 0;) {
            const std::size_t resume = std::min(i + cooldown + 1, n);
            current[i] = std::max(
                slots[i].base + current[i + 1],
                slots[i].burst + (sum_base[resume] - sum_base[i + 1]) + previous[resume]);
        }
        std::swap(previous, current);
    }
    return previous[0];
}
