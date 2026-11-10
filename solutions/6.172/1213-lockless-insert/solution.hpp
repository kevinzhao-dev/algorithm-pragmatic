#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

template <template <typename> class Atomic = std::atomic>
class LocklessTable {
public:
    explicit LocklessTable(std::size_t capacity) : slots_(capacity) {}

    bool insert(std::uint64_t key, std::uint64_t value) {
        const std::size_t mask = slots_.size() - 1;
        std::size_t index = key & mask;
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            Slot& slot = slots_[index];
            std::uint64_t seen = 0;
            if (slot.key.compare_exchange_strong(seen, key, std::memory_order_acq_rel,
                                                 std::memory_order_acquire) ||
                seen == key) {
                slot.value.store(value, std::memory_order_release);
                return true;
            }
            index = (index + 1) & mask;
        }
        return false;
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        const std::size_t mask = slots_.size() - 1;
        std::size_t index = key & mask;
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            const Slot& slot = slots_[index];
            const std::uint64_t seen = slot.key.load(std::memory_order_acquire);
            if (seen == 0) {
                return std::nullopt;
            }
            if (seen == key) {
                // The key may be claimed before its value is published.
                const std::uint64_t value = slot.value.load(std::memory_order_acquire);
                return value == 0 ? std::nullopt : std::optional<std::uint64_t>(value);
            }
            index = (index + 1) & mask;
        }
        return std::nullopt;
    }

private:
    struct Slot {
        Atomic<std::uint64_t> key{0};
        Atomic<std::uint64_t> value{0};
    };

    std::vector<Slot> slots_;
};

template <template <typename> class Atomic = std::atomic>
class PublishedTable {
public:
    explicit PublishedTable(std::size_t capacity) : slots_(capacity) {}

    bool insert(std::uint64_t key, std::string text) {
        const std::size_t mask = slots_.size() - 1;
        std::size_t index = key & mask;
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            Slot& slot = slots_[index];
            std::uint64_t seen = 0;
            if (slot.key.compare_exchange_strong(seen, key, std::memory_order_acq_rel,
                                                 std::memory_order_acquire)) {
                slot.text = std::move(text);
                slot.ready.store(1, std::memory_order_release);
                return true;
            }
            if (seen == key) {
                return false;
            }
            index = (index + 1) & mask;
        }
        return false;
    }

    std::optional<std::string> find(std::uint64_t key) const {
        const std::size_t mask = slots_.size() - 1;
        std::size_t index = key & mask;
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            const Slot& slot = slots_[index];
            const std::uint64_t seen = slot.key.load(std::memory_order_acquire);
            if (seen == 0) {
                return std::nullopt;
            }
            if (seen == key) {
                if (slot.ready.load(std::memory_order_acquire) == 0) {
                    return std::nullopt;
                }
                return slot.text;
            }
            index = (index + 1) & mask;
        }
        return std::nullopt;
    }

private:
    struct Slot {
        Atomic<std::uint64_t> key{0};
        Atomic<std::uint32_t> ready{0};
        std::string text;
    };

    std::vector<Slot> slots_;
};
