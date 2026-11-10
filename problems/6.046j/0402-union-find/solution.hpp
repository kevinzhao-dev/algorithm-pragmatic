#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "practice.hpp"

// Stage 1
class ListDsu {
public:
    explicit ListDsu(std::size_t n) { PRACTICE_TODO(); }

    int find(int x) const { PRACTICE_TODO(); }
    bool unite(int x, int y) { PRACTICE_TODO(); }
    std::size_t set_size(int x) const { PRACTICE_TODO(); }
    std::vector<int> members(int x) const { PRACTICE_TODO(); }

    // Cost model used by the tests: the number of representative pointers rewritten.
    std::uint64_t rep_updates() const { PRACTICE_TODO(); }
};

// Stage 2
class ForestDsu {
public:
    explicit ForestDsu(std::size_t n = 0) { PRACTICE_TODO(); }

    int make_set() { PRACTICE_TODO(); }
    int find(int x) { PRACTICE_TODO(); }
    bool unite(int x, int y) { PRACTICE_TODO(); }
    bool connected(int x, int y) { PRACTICE_TODO(); }
    std::size_t set_size(int x) { PRACTICE_TODO(); }
    std::size_t count() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }

    // Parent links from x to its root, without compressing anything.
    int depth(int x) const { PRACTICE_TODO(); }

    // Cost model used by the tests: the number of parent links followed by find.
    std::uint64_t hops() const { PRACTICE_TODO(); }

    // Stage 3
    std::vector<int> members(int x) { PRACTICE_TODO(); }
};

// Stage 3
class DedupIndex {
public:
    int add_record(const std::vector<std::string>& identifiers) { PRACTICE_TODO(); }
    bool same_entity(int a, int b) { PRACTICE_TODO(); }
    std::size_t entity_count() const { PRACTICE_TODO(); }
    std::vector<int> entity_members(int record) { PRACTICE_TODO(); }
};
