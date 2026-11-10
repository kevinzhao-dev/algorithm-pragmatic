#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Tracks the true partition with a plain array; every operation here is O(n).
class ModelDsu {
public:
    explicit ModelDsu(std::size_t n) : label_(n) {
        for (std::size_t i = 0; i < n; ++i) {
            label_[i] = static_cast<int>(i);
        }
    }
    bool same(int a, int b) const { return label(a) == label(b); }
    int label(int x) const { return label_[static_cast<std::size_t>(x)]; }
    bool unite(int a, int b) {
        const int from = label(a);
        const int to = label(b);
        if (from == to) {
            return false;
        }
        for (int& l : label_) {
            if (l == from) {
                l = to;
            }
        }
        return true;
    }
    std::vector<int> members(int x) const {
        std::vector<int> out;
        for (std::size_t i = 0; i < label_.size(); ++i) {
            if (label_[i] == label(x)) {
                out.push_back(static_cast<int>(i));
            }
        }
        return out;
    }
    std::size_t set_count() const {
        return std::set<int>(label_.begin(), label_.end()).size();
    }

private:
    std::vector<int> label_;
};

std::vector<int> sorted(std::vector<int> values) {
    std::ranges::sort(values);
    return values;
}

// Merges singletons into equal halves, the worst case for the smaller-into-larger rule.
void balanced_unions(ListDsu& dsu, int n) {
    for (int width = 1; width < n; width *= 2) {
        for (int start = 0; start + width < n; start += 2 * width) {
            dsu.unite(start, start + width);
        }
    }
}

// Builds a binomial tree by repeatedly uniting equal-rank trees, without any compression help.
void binomial_unions(ForestDsu& dsu, int n) {
    for (int width = 1; width < n; width *= 2) {
        for (int start = 0; start + width < n; start += 2 * width) {
            dsu.unite(start, start + width);
        }
    }
}

}  // namespace

TEST(1, "every element starts alone") {
    const ListDsu dsu(3);
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(dsu.find(i), i);
        CHECK_EQ(dsu.set_size(i), 1);
    }
}

TEST(1, "an out of range element is rejected") {
    ListDsu dsu(2);
    CHECK_THROWS_AS(dsu.find(2), std::out_of_range);
    CHECK_THROWS_AS(dsu.find(-1), std::out_of_range);
    CHECK_THROWS_AS(dsu.unite(0, 2), std::out_of_range);
    CHECK_THROWS_AS(dsu.set_size(5), std::out_of_range);
    CHECK_THROWS_AS(dsu.members(5), std::out_of_range);
}

TEST(1, "uniting makes elements share a representative") {
    ListDsu dsu(4);
    CHECK(dsu.unite(0, 1));
    CHECK(dsu.find(0) == dsu.find(1));
    CHECK(dsu.find(0) != dsu.find(2));
    CHECK_EQ(dsu.set_size(1), 2);
}

TEST(1, "uniting two elements of the same set does nothing") {
    ListDsu dsu(2);
    CHECK(dsu.unite(0, 1));
    CHECK(!dsu.unite(1, 0));
    CHECK_EQ(dsu.set_size(0), 2);
}

TEST(1, "members lists exactly the set") {
    ListDsu dsu(6);
    dsu.unite(0, 1);
    dsu.unite(2, 3);
    dsu.unite(1, 3);
    CHECK_EQ(sorted(dsu.members(3)), (std::vector<int>{0, 1, 2, 3}));
    CHECK_EQ(dsu.members(4), std::vector<int>{4});
}

TEST(1, "random unions match a label array") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        INFO("trial ", trial);
        const int n = static_cast<int>(rng.uniform(1, 40));
        ListDsu dsu(static_cast<std::size_t>(n));
        ModelDsu model(static_cast<std::size_t>(n));
        for (int step = 0; step < 80; ++step) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            REQUIRE_EQ(dsu.unite(a, b), model.unite(a, b));
            const int q = static_cast<int>(rng.uniform(0, n - 1));
            REQUIRE_EQ(sorted(dsu.members(q)), model.members(q));
            REQUIRE_EQ(dsu.set_size(q), model.members(q).size());
            for (int i = 0; i < n; ++i) {
                REQUIRE_EQ(dsu.find(i) == dsu.find(q), model.same(i, q));
            }
        }
    }
}

TEST(1, "the smaller list is always the one relabeled") {
    const int n = 1 << 14;
    ListDsu dsu(static_cast<std::size_t>(n));
    balanced_unions(dsu, n);
    CHECK_EQ(dsu.set_size(0), static_cast<std::size_t>(n));
    CHECK_LE(dsu.rep_updates(), static_cast<std::uint64_t>(n) * 14 / 2 + static_cast<std::uint64_t>(n));
}

TEST(1, "relabeling cost stays within n log n on a random merge order") {
    Rng rng;
    const int n = 1 << 12;
    ListDsu dsu(static_cast<std::size_t>(n));
    auto order = rng.permutation(static_cast<std::size_t>(n));
    for (std::size_t i = 1; i < order.size(); ++i) {
        dsu.unite(order[i - 1], order[i]);
    }
    CHECK_LE(dsu.rep_updates(), static_cast<std::uint64_t>(n) * 12);
}

TEST(1, "a chain of unions onto one large set costs only linear relabeling") {
    const int n = 1 << 14;
    for (bool large_first : {true, false}) {
        INFO("large set is the first argument: ", large_first);
        ListDsu dsu(static_cast<std::size_t>(n));
        for (int i = 1; i < n; ++i) {
            if (large_first) {
                dsu.unite(0, i);
            } else {
                dsu.unite(i, 0);
            }
        }
        CHECK_LE(dsu.rep_updates(), static_cast<std::uint64_t>(n));
    }
}

TEST(2, "a default-constructed forest is empty and grows on demand") {
    ForestDsu dsu;
    CHECK_EQ(dsu.size(), 0);
    CHECK_EQ(dsu.count(), 0);
    CHECK_EQ(dsu.make_set(), 0);
    CHECK_EQ(dsu.make_set(), 1);
    CHECK_EQ(dsu.count(), 2);
    CHECK_EQ(dsu.size(), 2);
}

TEST(2, "an out of range element is rejected") {
    ForestDsu dsu(2);
    CHECK_THROWS_AS(dsu.find(2), std::out_of_range);
    CHECK_THROWS_AS(dsu.connected(0, 2), std::out_of_range);
    CHECK_THROWS_AS(dsu.unite(-1, 0), std::out_of_range);
}

TEST(2, "uniting reports whether anything changed and tracks the number of sets") {
    ForestDsu dsu(3);
    CHECK_EQ(dsu.count(), 3);
    CHECK(dsu.unite(0, 1));
    CHECK(!dsu.unite(0, 1));
    CHECK_EQ(dsu.count(), 2);
    CHECK(dsu.connected(1, 0));
    CHECK(!dsu.connected(1, 2));
    CHECK_EQ(dsu.set_size(1), 2);
    CHECK_EQ(dsu.set_size(2), 1);
}

TEST(2, "a long chain of unions leaves one set") {
    const int n = 200'000;
    ForestDsu dsu(static_cast<std::size_t>(n));
    for (int i = 1; i < n; ++i) {
        CHECK(dsu.unite(i - 1, i));
    }
    CHECK_EQ(dsu.count(), 1);
    CHECK_EQ(dsu.set_size(0), static_cast<std::size_t>(n));
    CHECK(dsu.connected(0, n - 1));
}

TEST(2, "random unions match a label array") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        INFO("trial ", trial);
        const int n = static_cast<int>(rng.uniform(1, 40));
        ForestDsu dsu(static_cast<std::size_t>(n));
        ModelDsu model(static_cast<std::size_t>(n));
        for (int step = 0; step < 80; ++step) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            REQUIRE_EQ(dsu.unite(a, b), model.unite(a, b));
            REQUIRE_EQ(dsu.count(), model.set_count());
            const int x = static_cast<int>(rng.uniform(0, n - 1));
            const int y = static_cast<int>(rng.uniform(0, n - 1));
            REQUIRE_EQ(dsu.connected(x, y), model.same(x, y));
            REQUIRE_EQ(dsu.set_size(x), model.members(x).size());
        }
    }
}

TEST(2, "tree depth stays logarithmic regardless of argument order") {
    const int n = 1 << 14;
    for (bool forward : {true, false}) {
        INFO("forward ", forward);
        ForestDsu dsu(static_cast<std::size_t>(n));
        for (int i = 1; i < n; ++i) {
            if (forward) {
                dsu.unite(i - 1, i);
            } else {
                dsu.unite(i, i - 1);
            }
        }
        int deepest = 0;
        for (int i = 0; i < n; ++i) {
            deepest = std::max(deepest, dsu.depth(i));
        }
        CHECK_LE(deepest, 14);
    }
}

TEST(2, "finds stay nearly constant on the worst forest") {
    const int n = 1 << 16;
    ForestDsu dsu(static_cast<std::size_t>(n));
    binomial_unions(dsu, n);
    const std::uint64_t before = dsu.hops();
    for (int i = 0; i < n; ++i) {
        dsu.find(i);
    }
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < n; ++i) {
            dsu.find(i);
        }
    }
    // Four rounds over n elements: compression must leave them only a step or two from the root.
    CHECK_LE(dsu.hops() - before, 6ULL * static_cast<std::uint64_t>(n));
}

TEST(2, "a mixed workload follows few parent links per operation") {
    Rng rng;
    const int n = 1 << 18;
    ForestDsu dsu(static_cast<std::size_t>(n));
    std::uint64_t operations = 0;
    for (int step = 0; step < 2 * n; ++step) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        dsu.unite(a, b);
        dsu.find(static_cast<int>(rng.uniform(0, n - 1)));
        operations += 2;
    }
    CHECK_LE(dsu.hops(), 6 * operations);
}

PERF(2, "two million mixed operations", 1500) {
    Rng rng;
    const int n = 1 << 20;
    ForestDsu dsu(static_cast<std::size_t>(n));
    for (int step = 0; step < n; ++step) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        dsu.unite(a, b);
        dsu.connected(static_cast<int>(rng.uniform(0, n - 1)), static_cast<int>(rng.uniform(0, n - 1)));
    }
    CHECK_LT(dsu.count(), static_cast<std::size_t>(n));
}

TEST(3, "members of a singleton is the element itself") {
    ForestDsu dsu(2);
    CHECK_EQ(dsu.members(1), std::vector<int>{1});
}

TEST(3, "members lists exactly the set after unions in any order") {
    ForestDsu dsu(8);
    dsu.unite(0, 1);
    dsu.unite(2, 3);
    dsu.unite(4, 5);
    dsu.unite(6, 7);
    dsu.unite(1, 3);
    dsu.unite(5, 7);
    dsu.unite(3, 7);
    CHECK_EQ(sorted(dsu.members(0)), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7}));
}

TEST(3, "members grows and merges with make_set") {
    ForestDsu dsu;
    const int a = dsu.make_set();
    const int b = dsu.make_set();
    const int c = dsu.make_set();
    dsu.unite(a, c);
    CHECK_EQ(sorted(dsu.members(a)), (std::vector<int>{a, c}));
    CHECK_EQ(dsu.members(b), std::vector<int>{b});
}

TEST(3, "random members match a label array") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        INFO("trial ", trial);
        const int n = static_cast<int>(rng.uniform(1, 30));
        ForestDsu dsu(static_cast<std::size_t>(n));
        ModelDsu model(static_cast<std::size_t>(n));
        for (int step = 0; step < 40; ++step) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            dsu.unite(a, b);
            model.unite(a, b);
            const int q = static_cast<int>(rng.uniform(0, n - 1));
            REQUIRE_EQ(sorted(dsu.members(q)), model.members(q));
        }
    }
}

TEST(3, "an empty index has no entities") {
    const DedupIndex index;
    CHECK_EQ(index.entity_count(), 0);
}

TEST(3, "records with no identifiers stay separate") {
    DedupIndex index;
    const int a = index.add_record({});
    const int b = index.add_record({});
    CHECK(!index.same_entity(a, b));
    CHECK_EQ(index.entity_count(), 2);
}

TEST(3, "records sharing an identifier become one entity") {
    DedupIndex index;
    const int a = index.add_record({"mail:x", "phone:1"});
    const int b = index.add_record({"phone:1"});
    const int c = index.add_record({"mail:y"});
    CHECK(index.same_entity(a, b));
    CHECK(!index.same_entity(a, c));
    CHECK_EQ(index.entity_count(), 2);
    CHECK_EQ(index.entity_members(b), (std::vector<int>{a, b}));
}

TEST(3, "a later record can bridge two existing entities") {
    DedupIndex index;
    const int a = index.add_record({"p"});
    const int b = index.add_record({"q"});
    CHECK_EQ(index.entity_count(), 2);
    const int c = index.add_record({"p", "q"});
    CHECK_EQ(index.entity_count(), 1);
    CHECK_EQ(index.entity_members(a), (std::vector<int>{a, b, c}));
}

TEST(3, "a record repeating an identifier does not double count") {
    DedupIndex index;
    const int a = index.add_record({"p", "p"});
    CHECK_EQ(index.entity_members(a), std::vector<int>{a});
    CHECK_EQ(index.entity_count(), 1);
}

TEST(3, "random records match a label array") {
    Rng rng;
    for (int trial = 0; trial < 200; ++trial) {
        INFO("trial ", trial);
        DedupIndex index;
        const int count = static_cast<int>(rng.uniform(1, 40));
        ModelDsu model(static_cast<std::size_t>(count));
        std::map<std::string, int> owner;
        for (int record = 0; record < count; ++record) {
            std::vector<std::string> identifiers;
            for (int i = static_cast<int>(rng.uniform(0, 3)); i > 0; --i) {
                identifiers.push_back("id" + std::to_string(rng.uniform(0, 12)));
            }
            REQUIRE_EQ(index.add_record(identifiers), record);
            for (const std::string& id : identifiers) {
                const auto [it, fresh] = owner.try_emplace(id, record);
                if (!fresh) {
                    model.unite(record, it->second);
                }
            }
        }
        CHECK_EQ(index.entity_count(), model.set_count());
        for (int q = 0; q < count; ++q) {
            REQUIRE_EQ(index.entity_members(q), model.members(q));
        }
    }
}

PERF(3, "querying members of many small entities", 1500) {
    Rng rng;
    DedupIndex index;
    const int count = 200'000;
    for (int record = 0; record < count; ++record) {
        index.add_record({"group" + std::to_string(record / 2)});
    }
    std::size_t total = 0;
    for (int record = 0; record < count; ++record) {
        total += index.entity_members(record).size();
    }
    CHECK_EQ(total, static_cast<std::size_t>(2 * count));
    CHECK_EQ(index.entity_count(), static_cast<std::size_t>(count / 2));
}
