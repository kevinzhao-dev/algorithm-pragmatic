#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

inline std::uint64_t binomial(std::uint64_t n, std::uint64_t k) {
    if (k > n) {
        return 0;
    }
    k = std::min(k, n - k);
    // result is C(n - k + i, i) after step i, so each division is exact; 128 bits hold the
    // product before dividing.
    unsigned __int128 result = 1;
    for (std::uint64_t i = 1; i <= k; ++i) {
        result = result * (n - k + i) / i;
    }
    return static_cast<std::uint64_t>(result);
}

template <typename Domain, typename F>
std::vector<std::decay_t<std::invoke_result_t<F&, std::ranges::range_reference_t<const Domain>>>>
set_of(const Domain& domain, F f) {
    using Value = std::decay_t<std::invoke_result_t<F&, std::ranges::range_reference_t<const Domain>>>;
    std::vector<Value> values;
    for (auto&& x : domain) {
        values.push_back(f(x));
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

template <typename T>
std::vector<T> intersect(std::span<const T> a, std::span<const T> b) {
    std::vector<T> out;
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            ++i;
        } else if (b[j] < a[i]) {
            ++j;
        } else {
            out.push_back(a[i]);
            ++i;
            ++j;
        }
    }
    return out;
}

template <typename T>
std::size_t union_size(std::span<const T> a, std::span<const T> b) {
    std::size_t common = 0;
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            ++i;
        } else if (b[j] < a[i]) {
            ++j;
        } else {
            ++common;
            ++i;
            ++j;
        }
    }
    return a.size() + b.size() - common;
}

template <typename T>
std::size_t difference_size(std::span<const T> a, std::span<const T> b) {
    return union_size(a, b) - b.size();
}

template <typename T>
std::vector<T> gallop_intersect(std::span<const T> a, std::span<const T> b) {
    if (a.size() > b.size()) {
        return gallop_intersect(b, a);
    }
    std::vector<T> out;
    std::size_t start = 0;  // every element of b before `start` is known to be below the next query
    for (const T& x : a) {
        std::size_t step = 1;
        std::size_t high = start;
        while (high < b.size() && b[high] < x) {
            start = high + 1;
            high = start + step - 1;
            step *= 2;
        }
        // b[start - 1] < x (or start == 0) and b[high] >= x (or high is past the end).
        high = std::min(high + 1, b.size());
        while (start < high) {
            const std::size_t middle = start + (high - start) / 2;
            if (b[middle] < x) {
                start = middle + 1;
            } else {
                high = middle;
            }
        }
        if (start < b.size() && !(x < b[start])) {
            out.push_back(x);
            ++start;
        }
    }
    return out;
}

class PostingIndex {
public:
    void add(std::string term, std::vector<std::int64_t> doc_ids) {
        lists_[std::move(term)] = std::move(doc_ids);
    }

    std::vector<std::int64_t> query(std::span<const std::string> must,
                                    std::span<const std::string> must_not = {}) const {
        if (must.empty()) {
            return {};
        }
        std::vector<const std::vector<std::int64_t>*> required;
        required.reserve(must.size());
        for (const std::string& term : must) {
            const auto found = lists_.find(term);
            if (found == lists_.end()) {
                return {};
            }
            required.push_back(&found->second);
        }
        std::sort(required.begin(), required.end(),
                  [](const auto* a, const auto* b) { return a->size() < b->size(); });

        std::vector<std::int64_t> result = *required.front();
        for (std::size_t i = 1; i < required.size(); ++i) {
            result = gallop_intersect<std::int64_t>(result, *required[i]);
        }
        for (const std::string& term : must_not) {
            if (const auto found = lists_.find(term); found != lists_.end()) {
                result = subtract(result, found->second);
            }
        }
        return result;
    }

private:
    // Galloping set difference: result ∖ exclude, walking only `result` element by element.
    static std::vector<std::int64_t> subtract(const std::vector<std::int64_t>& result,
                                              const std::vector<std::int64_t>& exclude) {
        const auto hits = gallop_intersect<std::int64_t>(result, exclude);
        std::vector<std::int64_t> kept;
        kept.reserve(result.size() - hits.size());
        std::size_t next_hit = 0;
        for (const std::int64_t value : result) {
            if (next_hit < hits.size() && hits[next_hit] == value) {
                ++next_hit;
            } else {
                kept.push_back(value);
            }
        }
        return kept;
    }

    std::unordered_map<std::string, std::vector<std::int64_t>> lists_;
};
