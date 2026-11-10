// practice.hpp — the whole test harness for this repo, on purpose small enough to read in one go.
//
// In a test file:
//     #include "practice.hpp"
//     #include "solution.hpp"
//
//     TEST(1, "push then pop returns the same value") { ... CHECK_EQ(s.pop(), 7); }
//     PERF(2, "one million pushes", 2000 /* ms budget */) { ... }
//
// TEST(stage, name)              functional test; runs under sanitizers.
// PERF(stage, name, budget_ms)   runs only in optimized builds; fails if it exceeds the budget.
//
// In a stub:  int solve(int x) { PRACTICE_TODO(); }
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <ostream>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace practice {

// Thrown by PRACTICE_TODO(); the runner reports it as "not implemented yet" rather than a bug.
class NotImplemented final : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

#define PRACTICE_TODO()                                                                 \
    throw ::practice::NotImplemented(std::string(__func__) + " (" + __FILE__ + ":" +    \
                                     std::to_string(__LINE__) + ")")

struct TestCase {
    int stage;
    const char* name;
    void (*body)();
    const char* file;
    int line;
    long budget_ms;  // > 0 marks a performance test
};

namespace detail {

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(int stage, const char* name, void (*body)(), const char* file, int line,
              long budget_ms) {
        registry().push_back({stage, name, body, file, line, budget_ms});
    }
};

struct AbortTest {};

inline constexpr std::size_t kMaxPrintedElements = 24;
inline constexpr int kMaxPrintedFailures = 6;

template <typename T>
struct IsOptional : std::false_type {};
template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename T>
struct IsPair : std::false_type {};
template <typename A, typename B>
struct IsPair<std::pair<A, B>> : std::true_type {};

template <typename T>
struct IsTuple : std::false_type {};
template <typename... Ts>
struct IsTuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
concept Streamable = requires(std::ostream& os, const T& value) { os << value; };

template <typename T>
void print_value(std::ostream& os, const T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
        os << (value ? "true" : "false");
    } else if constexpr (std::is_same_v<U, char>) {
        os << '\'' << value << '\'';
    } else if constexpr (std::is_same_v<U, signed char> || std::is_same_v<U, unsigned char>) {
        os << static_cast<int>(value);
    } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
        os << '"' << std::string_view(value) << '"';
    } else if constexpr (IsOptional<U>::value) {
        if (value) {
            print_value(os, *value);
        } else {
            os << "nullopt";
        }
    } else if constexpr (IsPair<U>::value) {
        os << '(';
        print_value(os, value.first);
        os << ", ";
        print_value(os, value.second);
        os << ')';
    } else if constexpr (IsTuple<U>::value) {
        os << '(';
        std::size_t index = 0;
        std::apply(
            [&](const auto&... element) {
                ((os << (index++ ? ", " : ""), print_value(os, element)), ...);
            },
            value);
        os << ')';
    } else if constexpr (std::ranges::input_range<const U>) {
        using Element = std::ranges::range_value_t<const U>;
        os << '[';
        std::size_t index = 0;
        for (auto&& element : value) {
            if (index == kMaxPrintedElements) {
                os << ", ...";
            }
            if (index < kMaxPrintedElements) {
                if (index != 0) {
                    os << ", ";
                }
                if constexpr (std::is_same_v<Element, bool>) {
                    print_value(os, static_cast<bool>(element));
                } else {
                    print_value(os, element);
                }
            }
            ++index;
        }
        os << ']';
        if (index > kMaxPrintedElements) {
            os << " (size " << index << ')';
        }
    } else if constexpr (Streamable<U>) {
        os << value;
    } else {
        os << "<unprintable>";
    }
}

template <typename... Args>
std::string concat(const Args&... args) {
    std::ostringstream out;
    const auto append = [&](const auto& arg) {
        if constexpr (std::is_convertible_v<decltype(arg), std::string_view>) {
            out << std::string_view(arg);
        } else {
            print_value(out, arg);
        }
    };
    (append(args), ...);
    return out.str();
}

class InfoNode {
public:
    virtual ~InfoNode() = default;
    virtual std::string text() const = 0;
    InfoNode* previous = nullptr;
};

inline thread_local InfoNode* info_head = nullptr;

// Holds a lazily evaluated message that is only rendered when a check in its scope fails.
template <typename Make>
class ScopedInfo final : public InfoNode {
public:
    explicit ScopedInfo(Make make) : make_(std::move(make)) {
        previous = info_head;
        info_head = this;
    }
    ~ScopedInfo() override { info_head = previous; }
    ScopedInfo(const ScopedInfo&) = delete;
    ScopedInfo& operator=(const ScopedInfo&) = delete;
    std::string text() const override { return make_(); }

private:
    Make make_;
};

struct RunState {
    std::mutex mutex;
    std::atomic<int> failed_checks{0};
    std::string details;
};

inline RunState& run_state() {
    static RunState state;
    return state;
}

inline std::string_view base_name(std::string_view path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

inline void report_failure(const char* file, int line, const std::string& message) {
    RunState& state = run_state();
    const int count = state.failed_checks.fetch_add(1) + 1;
    if (count > kMaxPrintedFailures) {
        return;
    }
    std::ostringstream out;
    out << "    " << base_name(file) << ':' << line << ": " << message << '\n';
    for (const InfoNode* node = info_head; node != nullptr; node = node->previous) {
        out << "      where " << node->text() << '\n';
    }
    if (count == kMaxPrintedFailures) {
        out << "    (more failures in this test are hidden)\n";
    }
    const std::lock_guard lock(state.mutex);
    state.details += out.str();
}

inline bool check_true(bool passed, const char* text, const char* file, int line) {
    if (!passed) {
        report_failure(file, line, std::string("CHECK(") + text + ") is false");
    }
    return passed;
}

template <typename T>
inline constexpr bool kPlainInteger =
    std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char> &&
    !std::is_same_v<T, char8_t> && !std::is_same_v<T, char16_t> && !std::is_same_v<T, char32_t> &&
    !std::is_same_v<T, wchar_t>;

// Integer comparisons go through std::cmp_* so `CHECK_EQ(v.size(), 3)` needs no casts.
struct Equal {
    static constexpr const char* symbol = "==";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        if constexpr (kPlainInteger<A> && kPlainInteger<B>) {
            return std::cmp_equal(a, b);
        } else {
            return a == b;
        }
    }
};
struct NotEqual {
    static constexpr const char* symbol = "!=";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        return !Equal{}(a, b);
    }
};
struct Less {
    static constexpr const char* symbol = "<";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        if constexpr (kPlainInteger<A> && kPlainInteger<B>) {
            return std::cmp_less(a, b);
        } else {
            return a < b;
        }
    }
};
struct LessEqual {
    static constexpr const char* symbol = "<=";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        return !Less{}(b, a);
    }
};
struct Greater {
    static constexpr const char* symbol = ">";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        return Less{}(b, a);
    }
};
struct GreaterEqual {
    static constexpr const char* symbol = ">=";
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
        return !Less{}(a, b);
    }
};

template <typename Compare, typename A, typename B>
bool check_compare(const A& a, const B& b, const char* a_text, const char* b_text,
                   const char* file, int line) {
    if (Compare{}(a, b)) {
        return true;
    }
    std::ostringstream out;
    out << a_text << ' ' << Compare::symbol << ' ' << b_text << " failed\n      left : ";
    print_value(out, a);
    out << "\n      right: ";
    print_value(out, b);
    if constexpr (std::ranges::random_access_range<const A> &&
                  std::ranges::random_access_range<const B> &&
                  !std::is_convertible_v<const A&, std::string_view>) {
        const auto left_size = std::ranges::size(a);
        const auto right_size = std::ranges::size(b);
        for (std::size_t i = 0; i < std::min<std::size_t>(left_size, right_size); ++i) {
            if (!Equal{}(a[i], b[i])) {
                out << "\n      first difference at index " << i;
                break;
            }
        }
        if (left_size != right_size) {
            out << "\n      sizes: " << left_size << " vs " << right_size;
        }
    }
    report_failure(file, line, out.str());
    return false;
}

template <typename A, typename B, typename Tolerance>
bool check_near(const A& a, const B& b, const Tolerance& tolerance, const char* a_text,
                const char* b_text, const char* file, int line) {
    const auto difference = std::abs(static_cast<double>(a) - static_cast<double>(b));
    if (difference <= static_cast<double>(tolerance)) {
        return true;
    }
    report_failure(file, line,
                   concat(a_text, " is not within ", tolerance, " of ", b_text, "\n      left : ",
                          static_cast<double>(a), "\n      right: ", static_cast<double>(b)));
    return false;
}

}  // namespace detail

#define PRACTICE_JOIN_(a, b) a##b
#define PRACTICE_JOIN(a, b) PRACTICE_JOIN_(a, b)

#define PRACTICE_REGISTER_(fn, stage, name, budget)                                            \
    static void fn();                                                                          \
    static const ::practice::detail::Registrar PRACTICE_JOIN(fn, _registrar){                  \
        (stage), (name), &fn, __FILE__, __LINE__, (budget)};                                   \
    static void fn()

#define TEST(stage, name) \
    PRACTICE_REGISTER_(PRACTICE_JOIN(practice_test_, __LINE__), stage, name, 0)
#define PERF(stage, name, budget_ms) \
    PRACTICE_REGISTER_(PRACTICE_JOIN(practice_perf_, __LINE__), stage, name, budget_ms)

// CHECK records a failure and continues; REQUIRE ends the current test.
#define CHECK(...)                                                                             \
    ::practice::detail::check_true(static_cast<bool>(__VA_ARGS__), #__VA_ARGS__, __FILE__,     \
                                   __LINE__)
#define REQUIRE(...)                                                                           \
    do {                                                                                       \
        if (!CHECK(__VA_ARGS__)) throw ::practice::detail::AbortTest{};                        \
    } while (false)

// The expected value goes last, so it may contain commas: CHECK_EQ(v, std::vector{1, 2}).
#define PRACTICE_COMPARE_(cmp, a, ...)                                                         \
    ::practice::detail::check_compare<::practice::detail::cmp>((a), (__VA_ARGS__), #a,         \
                                                               #__VA_ARGS__, __FILE__, __LINE__)
#define CHECK_EQ(a, ...) PRACTICE_COMPARE_(Equal, a, __VA_ARGS__)
#define CHECK_NE(a, ...) PRACTICE_COMPARE_(NotEqual, a, __VA_ARGS__)
#define CHECK_LT(a, ...) PRACTICE_COMPARE_(Less, a, __VA_ARGS__)
#define CHECK_LE(a, ...) PRACTICE_COMPARE_(LessEqual, a, __VA_ARGS__)
#define CHECK_GT(a, ...) PRACTICE_COMPARE_(Greater, a, __VA_ARGS__)
#define CHECK_GE(a, ...) PRACTICE_COMPARE_(GreaterEqual, a, __VA_ARGS__)

#define REQUIRE_EQ(a, ...)                                                                     \
    do {                                                                                       \
        if (!CHECK_EQ(a, __VA_ARGS__)) throw ::practice::detail::AbortTest{};                  \
    } while (false)

#define CHECK_NEAR(a, b, tolerance)                                                            \
    ::practice::detail::check_near((a), (b), (tolerance), #a, #b, __FILE__, __LINE__)

#define CHECK_THROWS_AS(statement, ...)                                                        \
    do {                                                                                       \
        bool practice_caught_ = false;                                                         \
        try {                                                                                  \
            static_cast<void>(statement);                                                      \
        } catch (const ::practice::NotImplemented&) {                                          \
            throw;                                                                             \
        } catch (const __VA_ARGS__&) {                                                         \
            practice_caught_ = true;                                                           \
        } catch (...) {                                                                        \
            ::practice::detail::report_failure(                                                \
                __FILE__, __LINE__, #statement " threw the wrong exception type");             \
            practice_caught_ = true;                                                           \
        }                                                                                      \
        if (!practice_caught_) {                                                               \
            ::practice::detail::report_failure(__FILE__, __LINE__,                             \
                                               #statement " did not throw " #__VA_ARGS__);     \
        }                                                                                      \
    } while (false)

// Attaches context to any check that fails in this scope: INFO("seed=", seed, " n=", n);
#define INFO(...)                                                                              \
    const ::practice::detail::ScopedInfo PRACTICE_JOIN(practice_info_, __LINE__) {             \
        [&] { return ::practice::detail::concat(__VA_ARGS__); }                                \
    }

// ---------------------------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------------------------

// Deterministic by default so failures reproduce. Set PRACTICE_SEED to explore other inputs.
inline std::uint64_t default_seed() {
    if (const char* text = std::getenv("PRACTICE_SEED")) {
        return std::strtoull(text, nullptr, 10);
    }
    return 6006604661720ULL;
}

class Rng {
public:
    explicit Rng(std::uint64_t seed = default_seed()) : engine_(seed) {}

    // Uniform integer in the closed interval [low, high].
    std::int64_t uniform(std::int64_t low, std::int64_t high) {
        return std::uniform_int_distribution<std::int64_t>(low, high)(engine_);
    }
    double real() { return std::uniform_real_distribution<double>(0.0, 1.0)(engine_); }
    bool coin(double probability = 0.5) { return real() < probability; }

    std::vector<int> ints(std::size_t count, int low, int high) {
        std::vector<int> values(count);
        for (int& value : values) {
            value = static_cast<int>(uniform(low, high));
        }
        return values;
    }
    std::vector<int> permutation(std::size_t count) {
        std::vector<int> values(count);
        for (std::size_t i = 0; i < count; ++i) {
            values[i] = static_cast<int>(i);
        }
        shuffle(values);
        return values;
    }
    std::string text(std::size_t length, std::string_view alphabet = "abcdefghijklmnopqrstuvwxyz") {
        std::string result(length, ' ');
        for (char& ch : result) {
            ch = alphabet[static_cast<std::size_t>(
                uniform(0, static_cast<std::int64_t>(alphabet.size()) - 1))];
        }
        return result;
    }
    template <std::ranges::random_access_range Range>
    void shuffle(Range& range) {
        std::ranges::shuffle(range, engine_);
    }
    std::mt19937_64& engine() { return engine_; }

private:
    std::mt19937_64 engine_;
};

class Stopwatch {
public:
    Stopwatch() : start_(std::chrono::steady_clock::now()) {}
    double elapsed_ms() const {
        const auto delta = std::chrono::steady_clock::now() - start_;
        return std::chrono::duration<double, std::milli>(delta).count();
    }

private:
    std::chrono::steady_clock::time_point start_;
};

// Value wrapper that counts comparisons, for asserting complexity without relying on the clock.
template <typename T = int>
class Counted {
public:
    Counted() = default;
    explicit Counted(T value) : value_(std::move(value)) {}
    const T& value() const { return value_; }

    friend bool operator==(const Counted& a, const Counted& b) {
        ++comparisons();
        return a.value_ == b.value_;
    }
    friend bool operator<(const Counted& a, const Counted& b) {
        ++comparisons();
        return a.value_ < b.value_;
    }
    friend bool operator>(const Counted& a, const Counted& b) { return b < a; }
    friend bool operator<=(const Counted& a, const Counted& b) { return !(b < a); }
    friend bool operator>=(const Counted& a, const Counted& b) { return !(a < b); }
    friend std::ostream& operator<<(std::ostream& os, const Counted& c) { return os << c.value_; }

    static std::uint64_t& comparisons() {
        static std::uint64_t count = 0;
        return count;
    }

private:
    T value_{};
};

// Element type that tracks how many instances are alive, to catch leaks and double frees
// in containers you write. LeakSanitizer is not available on Apple Silicon.
class Tracked {
public:
    Tracked() : value_(0) { ++live_; }
    explicit Tracked(int value) : value_(value) { ++live_; }
    Tracked(const Tracked& other) : value_(other.value_) { ++live_; }
    Tracked(Tracked&& other) noexcept : value_(other.value_) { ++live_; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;
    ~Tracked() { --live_; }

    int value() const { return value_; }
    friend bool operator==(const Tracked& a, const Tracked& b) { return a.value_ == b.value_; }
    friend std::ostream& operator<<(std::ostream& os, const Tracked& t) { return os << t.value_; }

    static std::int64_t live() { return live_.load(); }

private:
    int value_;
    static inline std::atomic<std::int64_t> live_{0};
};

// Exponent k such that cost grows like n^k between two measurements.
inline double growth_exponent(double cost_small, double cost_large, double n_small,
                              double n_large) {
    return std::log(cost_large / cost_small) / std::log(n_large / n_small);
}

}  // namespace practice
