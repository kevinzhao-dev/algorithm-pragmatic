#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace detail {

struct Entry {
    std::uint64_t key = 0;
    std::uint64_t value = 0;
};

// Keeps each stripe on its own cache line so unrelated stripes do not false-share.
template <typename Mutex>
struct alignas(64) PaddedMutex {
    Mutex mutex;
};

}  // namespace detail

template <typename Mutex = std::mutex>
class CoarseTable {
public:
    explicit CoarseTable(std::size_t capacity) : slots_(capacity) {}

    bool insert(std::uint64_t key, std::uint64_t value) {
        const std::lock_guard lock(mutex_);
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            if (slots_[slot].key == 0 || slots_[slot].key == key) {
                slots_[slot] = {key, value};
                return true;
            }
            slot = next(slot);
        }
        return false;
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        const std::lock_guard lock(mutex_);
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            if (slots_[slot].key == key) {
                return slots_[slot].value;
            }
            if (slots_[slot].key == 0) {
                return std::nullopt;
            }
            slot = next(slot);
        }
        return std::nullopt;
    }

private:
    std::size_t home(std::uint64_t key) const { return key & (slots_.size() - 1); }
    std::size_t next(std::size_t slot) const { return (slot + 1) & (slots_.size() - 1); }

    std::vector<detail::Entry> slots_;
    mutable Mutex mutex_;
};

template <typename Mutex = std::mutex>
class StripedTable {
public:
    StripedTable(std::size_t capacity, std::size_t stripes)
        : slots_(capacity), stripes_(stripes) {}

    bool insert(std::uint64_t key, std::uint64_t value) {
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            const std::lock_guard lock(stripe_of(slot).mutex);
            if (slots_[slot].key == 0 || slots_[slot].key == key) {
                slots_[slot] = {key, value};
                return true;
            }
            slot = next(slot);
        }
        return false;
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            const std::lock_guard lock(stripe_of(slot).mutex);
            if (slots_[slot].key == key) {
                return slots_[slot].value;
            }
            if (slots_[slot].key == 0) {
                return std::nullopt;
            }
            slot = next(slot);
        }
        return std::nullopt;
    }

    const void* stripe_address(std::size_t stripe) const { return &stripes_[stripe]; }

private:
    std::size_t home(std::uint64_t key) const { return key & (slots_.size() - 1); }
    std::size_t next(std::size_t slot) const { return (slot + 1) & (slots_.size() - 1); }
    detail::PaddedMutex<Mutex>& stripe_of(std::size_t slot) const {
        return stripes_[slot & (stripes_.size() - 1)];
    }

    std::vector<detail::Entry> slots_;
    mutable std::vector<detail::PaddedMutex<Mutex>> stripes_;
};

namespace detail {

// The shared body of the two coupled variants; they differ only in when the old lock is released.
template <typename Mutex, typename Advance>
class CoupledTable {
public:
    CoupledTable(std::size_t capacity, std::size_t stripes, Advance advance)
        : slots_(capacity), stripes_(stripes), advance_(advance) {}

    bool insert(std::uint64_t key, std::uint64_t value) {
        std::size_t slot = key & (slots_.size() - 1);
        lock_of(slot).lock();
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            if (slots_[slot].key == 0 || slots_[slot].key == key) {
                slots_[slot] = {key, value};
                lock_of(slot).unlock();
                return true;
            }
            slot = step(slot);
        }
        lock_of(slot).unlock();
        return false;
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        std::size_t slot = key & (slots_.size() - 1);
        lock_of(slot).lock();
        for (std::size_t probes = 0; probes < slots_.size(); ++probes) {
            if (slots_[slot].key == key) {
                const std::uint64_t value = slots_[slot].value;
                lock_of(slot).unlock();
                return value;
            }
            if (slots_[slot].key == 0) {
                lock_of(slot).unlock();
                return std::nullopt;
            }
            slot = step(slot);
        }
        lock_of(slot).unlock();
        return std::nullopt;
    }

private:
    std::size_t stripe_index(std::size_t slot) const { return slot & (stripes_.size() - 1); }
    Mutex& lock_of(std::size_t slot) const { return stripes_[stripe_index(slot)].mutex; }

    // Moves to the next slot, moving from the current stripe's lock to the next stripe's lock.
    std::size_t step(std::size_t slot) const {
        const std::size_t next = (slot + 1) & (slots_.size() - 1);
        const std::size_t from = stripe_index(slot);
        const std::size_t to = stripe_index(next);
        if (from != to) {
            advance_(stripes_[from].mutex, stripes_[to].mutex, from, to);
        }
        return next;
    }

    std::vector<Entry> slots_;
    mutable std::vector<PaddedMutex<Mutex>> stripes_;
    Advance advance_;
};

// Hand-over-hand: never gives up the old lock until holding the new one.
struct HandOverHand {
    template <typename Mutex>
    void operator()(Mutex& held, Mutex& next, std::size_t, std::size_t) const {
        next.lock();
        held.unlock();
    }
};

// Couples only along increasing stripe numbers, so a cycle of waiters cannot form.
struct OrderedAdvance {
    template <typename Mutex>
    void operator()(Mutex& held, Mutex& next, std::size_t from, std::size_t to) const {
        if (to > from) {
            next.lock();
            held.unlock();
        } else {
            held.unlock();
            next.lock();
        }
    }
};

}  // namespace detail

template <typename Mutex = std::mutex>
class FairTable {
public:
    FairTable(std::size_t capacity, std::size_t stripes) : table_(capacity, stripes, {}) {}

    bool insert(std::uint64_t key, std::uint64_t value) { return table_.insert(key, value); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { return table_.find(key); }

private:
    detail::CoupledTable<Mutex, detail::HandOverHand> table_;
};

template <typename Mutex = std::mutex>
class OrderedFairTable {
public:
    OrderedFairTable(std::size_t capacity, std::size_t stripes) : table_(capacity, stripes, {}) {}

    bool insert(std::uint64_t key, std::uint64_t value) { return table_.insert(key, value); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { return table_.find(key); }

private:
    detail::CoupledTable<Mutex, detail::OrderedAdvance> table_;
};
