// Instrumented helpers shared by the 6.006 sequence and sorting exercises.
//
// Complexity claims in these exercises are about how many array writes or comparisons an
// algorithm performs, so the tests count those operations instead of timing them.
#pragma once

#include <cstdint>
#include <ostream>
#include <utility>

namespace practice6006 {

// Element type that records every copy, move and comparison performed on it. A container's
// "array writes" are its copies plus moves; a sort's comparisons are counted as well.
// Counters are static per instantiation, so call reset() before the region being measured.
template <typename T = int>
class Probe {
public:
    struct Stats {
        std::uint64_t copies = 0;
        std::uint64_t moves = 0;
        std::uint64_t compares = 0;
    };

    Probe() = default;
    explicit Probe(T value) : value_(std::move(value)) {}

    Probe(const Probe& other) : value_(other.value_) { ++stats().copies; }
    Probe(Probe&& other) noexcept : value_(std::move(other.value_)) { ++stats().moves; }
    Probe& operator=(const Probe& other) {
        ++stats().copies;
        value_ = other.value_;
        return *this;
    }
    Probe& operator=(Probe&& other) noexcept {
        ++stats().moves;
        value_ = std::move(other.value_);
        return *this;
    }

    const T& value() const { return value_; }

    friend bool operator==(const Probe& a, const Probe& b) {
        ++stats().compares;
        return a.value_ == b.value_;
    }
    friend bool operator<(const Probe& a, const Probe& b) {
        ++stats().compares;
        return a.value_ < b.value_;
    }
    friend bool operator>(const Probe& a, const Probe& b) { return b < a; }
    friend bool operator<=(const Probe& a, const Probe& b) { return !(b < a); }
    friend bool operator>=(const Probe& a, const Probe& b) { return !(a < b); }
    friend std::ostream& operator<<(std::ostream& os, const Probe& probe) {
        return os << probe.value_;
    }

    static Stats& stats() {
        static Stats instance;
        return instance;
    }
    static void reset() { stats() = Stats{}; }
    static std::uint64_t copies() { return stats().copies; }
    static std::uint64_t moves() { return stats().moves; }
    static std::uint64_t compares() { return stats().compares; }
    // Total writes of an element into a slot, whichever way they happened.
    static std::uint64_t writes() { return stats().copies + stats().moves; }

private:
    T value_{};
};

}  // namespace practice6006
