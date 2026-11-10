#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

bool is_subsequence(std::string_view needle, std::string_view haystack) {
    std::size_t at = 0;
    for (const char ch : haystack) {
        if (at < needle.size() && needle[at] == ch) {
            ++at;
        }
    }
    return at == needle.size();
}

// Tries every subset of `a` as a candidate common subsequence.
std::size_t exhaustive_lcs(const std::string& a, const std::string& b) {
    std::size_t best = 0;
    for (std::size_t mask = 0; mask < (std::size_t{1} << a.size()); ++mask) {
        std::string candidate;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if ((mask >> i) & 1) {
                candidate += a[i];
            }
        }
        if (candidate.size() > best && is_subsequence(candidate, b)) {
            best = candidate.size();
        }
    }
    return best;
}

template <typename A, typename B>
void check_alignment(const A& a, const B& b, const Alignment& alignment) {
    for (std::size_t k = 0; k < alignment.size(); ++k) {
        const auto [i, j] = alignment[k];
        REQUIRE(i < a.size());
        REQUIRE(j < b.size());
        REQUIRE(a[i] == b[j]);
        if (k > 0) {
            REQUIRE(alignment[k - 1].first < i);
            REQUIRE(alignment[k - 1].second < j);
        }
    }
}

std::vector<Counted<char>> counted(const std::string& text) {
    std::vector<Counted<char>> out;
    out.reserve(text.size());
    for (const char ch : text) {
        out.emplace_back(ch);
    }
    return out;
}

std::vector<DiffLine> filtered(const std::vector<DiffLine>& diff, DiffKind drop) {
    std::vector<DiffLine> out;
    for (const DiffLine& line : diff) {
        if (line.kind != drop) {
            out.push_back(line);
        }
    }
    return out;
}

std::vector<std::string> texts_of(const std::vector<DiffLine>& lines) {
    std::vector<std::string> out;
    for (const DiffLine& line : lines) {
        out.emplace_back(line.text);
    }
    return out;
}

std::size_t keep_count(const std::vector<DiffLine>& diff) {
    return static_cast<std::size_t>(std::ranges::count_if(
        diff, [](const DiffLine& line) { return line.kind == DiffKind::Keep; }));
}

}  // namespace

TEST(1, "an empty input has no common subsequence") {
    CHECK_EQ(lcs_length(std::string{}, std::string{"abc"}), 0);
    CHECK_EQ(lcs_length(std::string{"abc"}, std::string{}), 0);
    CHECK_EQ(lcs_length(std::string{}, std::string{}), 0);
}

TEST(1, "identical inputs match completely") {
    const std::string text = "dynamic";
    CHECK_EQ(lcs_length(text, text), text.size());
}

TEST(1, "no shared element gives zero") {
    CHECK_EQ(lcs_length(std::string{"abc"}, std::string{"xyz"}), 0);
}

TEST(1, "order matters, not just membership") {
    CHECK_EQ(lcs_length(std::string{"abcd"}, std::string{"dcba"}), 1);
    CHECK_EQ(lcs_length(std::string{"abc"}, std::string{"cab"}), 2);
}

TEST(1, "elements can be whole lines") {
    const std::vector<std::string> a{"int x;", "int y;", "return x;"};
    const std::vector<std::string> b{"int x;", "int z;", "int y;", "return y;"};
    CHECK_EQ(lcs_length(a, b), 2);
}

TEST(1, "random small strings match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const std::string a = rng.text(static_cast<std::size_t>(rng.uniform(0, 10)), "abc");
        const std::string b = rng.text(static_cast<std::size_t>(rng.uniform(0, 10)), "abc");
        INFO("a=", a, " b=", b);
        REQUIRE_EQ(lcs_length(a, b), exhaustive_lcs(a, b));
    }
}

TEST(1, "comparisons stay within a constant factor of the table size") {
    Rng rng;
    const auto a = counted(rng.text(400, "abcd"));
    const auto b = counted(rng.text(250, "abcd"));
    Counted<char>::comparisons() = 0;
    CHECK_GT(lcs_length(a, b), 0);
    CHECK_LE(Counted<char>::comparisons(), 2 * a.size() * b.size());
}

PERF(1, "six thousand by six thousand", 600) {
    Rng rng;
    const std::string a = rng.text(6000, "abcd");
    const std::string b = rng.text(6000, "abcd");
    CHECK_GT(lcs_length(a, b), 3000);
}

TEST(2, "an empty input gives an empty alignment") {
    CHECK(lcs_alignment(std::string{}, std::string{"abc"}).empty());
    CHECK(lcs_alignment(std::string{"abc"}, std::string{}).empty());
}

TEST(2, "identical inputs align position by position") {
    const std::string text = "aligned";
    const Alignment expected{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}};
    CHECK_EQ(lcs_alignment(text, text), expected);
}

TEST(2, "a single common element is found wherever it is") {
    CHECK_EQ(lcs_alignment(std::string{"xxqxx"}, std::string{"yyyq"}), Alignment{{2, 3}});
}

TEST(2, "alignments on whole lines compare the strings") {
    const std::vector<std::string> a{"keep", "drop", "keep too"};
    const std::vector<std::string> b{"keep", "keep too"};
    CHECK_EQ(lcs_alignment(a, b), Alignment{{0, 0}, {2, 1}});
}

TEST(2, "random alignments are valid and as long as the LCS") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const std::string a = rng.text(static_cast<std::size_t>(rng.uniform(0, 30)), "abc");
        const std::string b = rng.text(static_cast<std::size_t>(rng.uniform(0, 30)), "abc");
        INFO("a=", a, " b=", b);
        const Alignment alignment = lcs_alignment(a, b);
        check_alignment(a, b, alignment);
        REQUIRE_EQ(alignment.size(), lcs_length(a, b));
    }
}

TEST(3, "lcs_length keeps O(min(n, m)) memory on balanced inputs") {
    Rng rng;
    const std::string a = rng.text(2000, "abcd");
    const std::string b = rng.text(1200, "abcd");
    heap_meter::Scope scope;
    const std::size_t length = lcs_length(a, b);
    CHECK_LE(scope.peak_bytes(), 64 * b.size());
    CHECK_GT(length, 0);
}

TEST(3, "lcs_length rolls over the shorter input") {
    Rng rng;
    const std::string a = rng.text(4000, "abcd");
    const std::string b = rng.text(16, "abcd");
    {
        heap_meter::Scope scope;
        CHECK_GT(lcs_length(a, b), 0);
        CHECK_LE(scope.peak_bytes(), 8192);
    }
    {
        heap_meter::Scope scope;
        CHECK_GT(lcs_length(b, a), 0);
        CHECK_LE(scope.peak_bytes(), 8192);
    }
}

TEST(3, "rolling does not change any answers") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const std::string a = rng.text(static_cast<std::size_t>(rng.uniform(0, 40)), "ab");
        const std::string b = rng.text(static_cast<std::size_t>(rng.uniform(0, 40)), "ab");
        INFO("a=", a, " b=", b);
        REQUIRE_EQ(lcs_length(a, b), lcs_alignment(a, b).size());
        REQUIRE_EQ(lcs_length(a, b), lcs_length(b, a));
    }
}

TEST(4, "linear alignment handles empty and single-element inputs") {
    CHECK(lcs_alignment_linear(std::string{}, std::string{"ab"}).empty());
    CHECK(lcs_alignment_linear(std::string{"ab"}, std::string{}).empty());
    CHECK(lcs_alignment_linear(std::string{"a"}, std::string{"b"}).empty());
    CHECK_EQ(lcs_alignment_linear(std::string{"a"}, std::string{"xxa"}), Alignment{{0, 2}});
    CHECK_EQ(lcs_alignment_linear(std::string{"xxa"}, std::string{"a"}), Alignment{{2, 0}});
}

TEST(4, "linear alignment is valid and optimal on random inputs") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const std::string a = rng.text(static_cast<std::size_t>(rng.uniform(0, 50)), "abc");
        const std::string b = rng.text(static_cast<std::size_t>(rng.uniform(0, 50)), "abc");
        INFO("a=", a, " b=", b);
        const Alignment alignment = lcs_alignment_linear(a, b);
        check_alignment(a, b, alignment);
        REQUIRE_EQ(alignment.size(), lcs_length(a, b));
    }
}

TEST(4, "linear alignment keeps the heap linear in both inputs") {
    Rng rng;
    const std::string a = rng.text(2400, "abcd");
    const std::string b = rng.text(1600, "abcd");
    heap_meter::Scope scope;
    const Alignment alignment = lcs_alignment_linear(a, b);
    CHECK_LE(scope.peak_bytes(), 72 * (a.size() + b.size()));
    CHECK_GT(alignment.size(), 0);
}

PERF(4, "time stays quadratic and memory stays linear on five thousand elements", 1500) {
    Rng rng;
    const std::string a = rng.text(5000, "abcd");
    const std::string b = rng.text(5000, "abcd");
    heap_meter::Scope scope;
    const Alignment alignment = lcs_alignment_linear(a, b);
    CHECK_LE(scope.peak_bytes(), 72 * (a.size() + b.size()));
    check_alignment(a, b, alignment);
}

TEST(5, "identical files are all Keep") {
    const std::vector<std::string> lines{"a", "b", "c"};
    const auto diff = diff_lines(lines, lines);
    REQUIRE_EQ(diff.size(), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        CHECK(diff[i].kind == DiffKind::Keep);
        CHECK_EQ(diff[i].text, lines[i]);
    }
}

TEST(5, "a completely changed file removes everything before adding") {
    const std::vector<std::string> before{"a", "b"};
    const std::vector<std::string> after{"x", "y", "z"};
    const auto diff = diff_lines(before, after);
    REQUIRE_EQ(diff.size(), 5);
    CHECK(diff[0].kind == DiffKind::Remove);
    CHECK(diff[1].kind == DiffKind::Remove);
    CHECK(diff[2].kind == DiffKind::Add);
    CHECK(diff[4].kind == DiffKind::Add);
}

TEST(5, "empty files on either side") {
    const std::vector<std::string> none;
    const std::vector<std::string> some{"a", "b"};
    CHECK(diff_lines(none, none).empty());
    const auto removed = diff_lines(some, none);
    CHECK_EQ(removed.size(), 2);
    CHECK(removed[0].kind == DiffKind::Remove);
    const auto added = diff_lines(none, some);
    CHECK_EQ(added.size(), 2);
    CHECK(added[1].kind == DiffKind::Add);
}

TEST(5, "the text views point into the caller's lines") {
    const std::vector<std::string> before{"shared line"};
    const auto diff = diff_lines(before, before);
    REQUIRE_EQ(diff.size(), 1);
    CHECK(diff[0].text.data() == before[0].data());
}

TEST(5, "random diffs replay both files and are minimal") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        std::vector<std::string> before(static_cast<std::size_t>(rng.uniform(0, 25)));
        std::vector<std::string> after(static_cast<std::size_t>(rng.uniform(0, 25)));
        for (auto& line : before) {
            line = rng.text(1, "abcd");
        }
        for (auto& line : after) {
            line = rng.text(1, "abcd");
        }
        INFO("trial ", trial);
        const auto diff = diff_lines(before, after);
        REQUIRE_EQ(texts_of(filtered(diff, DiffKind::Add)), before);
        REQUIRE_EQ(texts_of(filtered(diff, DiffKind::Remove)), after);
        REQUIRE_EQ(keep_count(diff), lcs_length(before, after));

        bool saw_add = false;
        for (const DiffLine& line : diff) {
            if (line.kind == DiffKind::Keep) {
                saw_add = false;
            } else if (line.kind == DiffKind::Add) {
                saw_add = true;
            } else {
                REQUIRE(!saw_add);
            }
        }
    }
}
