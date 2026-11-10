#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

bool is_palindrome(std::string_view s) {
    return std::equal(s.begin(), s.begin() + static_cast<std::ptrdiff_t>(s.size() / 2), s.rbegin());
}

bool is_subsequence(std::string_view needle, std::string_view haystack) {
    std::size_t i = 0;
    for (char ch : haystack) {
        if (i < needle.size() && needle[i] == ch) {
            ++i;
        }
    }
    return i == needle.size();
}

std::size_t brute_lps(const std::string& s) {
    std::size_t best = 0;
    for (std::uint32_t mask = 0; mask < (1u << s.size()); ++mask) {
        std::string t;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (mask >> i & 1) {
                t.push_back(s[i]);
            }
        }
        if (is_palindrome(t)) {
            best = std::max(best, t.size());
        }
    }
    return best;
}

// Exponential recursion over every BST shape on keys i..j-1.
long long brute_bst(const std::vector<long long>& w, int i, int j) {
    if (i >= j) {
        return 0;
    }
    const long long sum = std::accumulate(w.begin() + i, w.begin() + j, 0LL);
    long long best = std::numeric_limits<long long>::max();
    for (int r = i; r < j; ++r) {
        best = std::min(best, brute_bst(w, i, r) + brute_bst(w, r + 1, j));
    }
    return best + sum;
}

void check_plan(const BstPlan& plan, const std::vector<long long>& w) {
    const int n = static_cast<int>(w.size());
    REQUIRE_EQ(plan.left.size(), n);
    REQUIRE_EQ(plan.right.size(), n);
    std::vector<int> inorder;
    long long cost = 0;
    const std::function<void(int, int)> walk = [&](int node, int depth) {
        if (node == -1) {
            return;
        }
        walk(plan.left[node], depth + 1);
        inorder.push_back(node);
        cost += w[node] * (depth + 1);
        walk(plan.right[node], depth + 1);
    };
    walk(plan.root, 0);
    std::vector<int> expected(n);
    std::iota(expected.begin(), expected.end(), 0);
    CHECK_EQ(inorder, expected);
    CHECK_EQ(cost, plan.cost);
}

long long brute_coins(const std::vector<long long>& v, int i, int j) {
    if (i > j) {
        return 0;
    }
    const long long total = std::accumulate(v.begin() + i, v.begin() + j + 1, 0LL);
    return total - std::min(brute_coins(v, i + 1, j), brute_coins(v, i, j - 1));
}

long long brute_tower(const std::vector<Block>& blocks) {
    std::vector<int> perm(blocks.size());
    std::iota(perm.begin(), perm.end(), 0);
    long long best = 0;
    do {
        long long height = 0;
        for (std::size_t i = 0; i < perm.size(); ++i) {
            const Block& upper = blocks[perm[i]];
            if (i > 0) {
                const Block& lower = blocks[perm[i - 1]];
                if (upper.length > lower.length || upper.width > lower.width) {
                    break;
                }
            }
            height += upper.height;
            best = std::max(best, height);
        }
    } while (std::next_permutation(perm.begin(), perm.end()));
    return best;
}

long long brute_true(const std::string& expr, bool want) {
    if (expr.size() == 1) {
        return (expr[0] == 'T') == want ? 1 : 0;
    }
    long long total = 0;
    for (std::size_t k = 1; k < expr.size(); k += 2) {
        const std::string left = expr.substr(0, k);
        const std::string right = expr.substr(k + 1);
        for (bool a : {true, false}) {
            for (bool b : {true, false}) {
                const char op = expr[k];
                const bool result = op == '&' ? (a && b) : op == '|' ? (a || b) : (a != b);
                if (result == want) {
                    total += brute_true(left, a) * brute_true(right, b);
                }
            }
        }
    }
    return total;
}

}  // namespace

TEST(1, "empty and single character strings") {
    CHECK_EQ(longest_palindromic_subsequence(""), "");
    CHECK_EQ(longest_palindromic_subsequence("x"), "x");
}

TEST(1, "a string with no repeats keeps one character") {
    const std::string out = longest_palindromic_subsequence("abcdef");
    CHECK_EQ(out.size(), 1);
    CHECK(is_subsequence(out, "abcdef"));
}

TEST(1, "an existing palindrome is returned whole") {
    CHECK_EQ(longest_palindromic_subsequence("level"), "level");
    CHECK_EQ(longest_palindromic_subsequence("noon"), "noon");
}

TEST(1, "characters can be skipped") {
    const std::string out = longest_palindromic_subsequence("palindromes");
    CHECK(is_palindrome(out));
    CHECK(is_subsequence(out, "palindromes"));
    CHECK_EQ(out.size(), brute_lps("palindromes"));
}

TEST(1, "random strings match the exponential oracle") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 12));
        const std::string s = rng.text(n, "abc");
        INFO("s=", s);
        const std::string out = longest_palindromic_subsequence(s);
        CHECK(is_palindrome(out));
        CHECK(is_subsequence(out, s));
        CHECK_EQ(out.size(), brute_lps(s));
    }
}

PERF(1, "long strings stay quadratic", 1000) {
    Rng rng;
    const std::string s = rng.text(4'000, "ab");
    const std::string out = longest_palindromic_subsequence(s);
    CHECK_GT(out.size(), 2000);
    CHECK(is_palindrome(out));
}

TEST(2, "no keys and a single key") {
    const BstPlan empty = optimal_bst(std::vector<long long>{});
    CHECK_EQ(empty.cost, 0);
    CHECK_EQ(empty.root, -1);
    const std::vector<long long> one{7};
    const BstPlan plan = optimal_bst(one);
    CHECK_EQ(plan.cost, 7);
    CHECK_EQ(plan.root, 0);
    check_plan(plan, one);
}

TEST(2, "greedy by largest weight is not optimal") {
    const std::vector<long long> w{2, 12, 9, 11};
    const BstPlan plan = optimal_bst(w);
    check_plan(plan, w);
    CHECK_EQ(plan.cost, 61);
}

TEST(2, "skewed weights put the heavy key at the root") {
    const std::vector<long long> w{1, 1, 1, 1000};
    const BstPlan plan = optimal_bst(w);
    check_plan(plan, w);
    CHECK_EQ(plan.root, 3);
}

TEST(2, "random weights match the exponential oracle") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        std::vector<long long> w(n);
        for (auto& x : w) {
            x = rng.uniform(1, 30);
        }
        INFO("round ", round);
        const BstPlan plan = optimal_bst(w);
        check_plan(plan, w);
        CHECK_EQ(plan.cost, brute_bst(w, 0, n));
    }
}

PERF(2, "cubic DP on hundreds of keys", 2000) {
    Rng rng;
    std::vector<long long> w(500);
    for (auto& x : w) {
        x = rng.uniform(1, 1000);
    }
    const BstPlan plan = optimal_bst(w);
    CHECK_GT(plan.cost, 0);
}

TEST(3, "no coins and one coin") {
    CHECK_EQ(first_player_value(std::vector<long long>{}), 0);
    CHECK_EQ(first_player_value(std::vector<long long>{42}), 42);
}

TEST(3, "looking one move ahead is not enough") {
    // Taking the 100 lets the opponent take the 200; taking the 1 keeps 200 for us.
    const std::vector<long long> v{1, 200, 100, 1};
    CHECK_EQ(first_player_value(v), 201);
}

TEST(3, "with an even count the first player never loses") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = 2 * static_cast<int>(rng.uniform(1, 8));
        std::vector<long long> v(n);
        long long odd = 0, even = 0;
        for (int i = 0; i < n; ++i) {
            v[i] = rng.uniform(0, 1000);
            (i % 2 == 0 ? odd : even) += v[i];
        }
        INFO("round ", round);
        CHECK_GE(first_player_value(v), std::max(odd, even));
    }
}

TEST(3, "random rows match minimax") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        std::vector<long long> v(n);
        for (auto& x : v) {
            x = rng.uniform(0, 1000);
        }
        INFO("round ", round);
        CHECK_EQ(first_player_value(v), brute_coins(v, 0, n - 1));
    }
}

PERF(3, "thousands of coins stay quadratic", 1000) {
    Rng rng;
    std::vector<long long> v(5000);
    for (auto& x : v) {
        x = rng.uniform(0, 1000);
    }
    CHECK_GT(first_player_value(v), 0);
}

TEST(4, "no blocks gives height zero") {
    CHECK_EQ(tallest_tower(std::vector<Block>{}), 0);
}

TEST(4, "equal footprints may stack") {
    const std::vector<Block> blocks{{3, 3, 5}, {3, 3, 7}};
    CHECK_EQ(tallest_tower(blocks), 12);
}

TEST(4, "blocks cannot be rotated") {
    const std::vector<Block> blocks{{5, 1, 10}, {1, 5, 20}};
    CHECK_EQ(tallest_tower(blocks), 20);
}

TEST(4, "random towers match permutation search") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        std::vector<Block> blocks(n);
        for (auto& b : blocks) {
            b = {rng.uniform(1, 6), rng.uniform(1, 6), rng.uniform(1, 20)};
        }
        INFO("round ", round);
        CHECK_EQ(tallest_tower(blocks), brute_tower(blocks));
    }
}

TEST(4, "boolean parenthesizations on a fixed expression") {
    CHECK_EQ(count_true("T"), 1);
    CHECK_EQ(count_true("F"), 0);
    CHECK_EQ(count_true("T^F"), 1);
    CHECK_EQ(count_true("T&F|T"), 2);
    CHECK_EQ(count_true("F|F|F"), 0);
}

TEST(4, "random expressions match the Catalan enumeration") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int operators = static_cast<int>(rng.uniform(1, 8));
        std::string expr = rng.text(1, "TF");
        for (int i = 0; i < operators; ++i) {
            expr += rng.text(1, "&|^") + rng.text(1, "TF");
        }
        INFO("expr=", expr);
        CHECK_EQ(count_true(expr), brute_true(expr, true) % 1'000'000'007);
    }
}

PERF(4, "hundreds of operands stay cubic", 2000) {
    Rng rng;
    std::string expr = rng.text(1, "TF");
    for (int i = 0; i < 450; ++i) {
        expr += rng.text(1, "&|^") + rng.text(1, "TF");
    }
    const long long ways = count_true(expr);
    CHECK_GE(ways, 0);
}
