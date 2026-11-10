#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class ListDsu {
public:
    explicit ListDsu(std::size_t n) : rep_(n), next_(n, -1), tail_(n), size_(n, 1) {
        std::iota(rep_.begin(), rep_.end(), 0);
        std::iota(tail_.begin(), tail_.end(), 0);
    }

    int find(int x) const { return rep_[check(x)]; }

    std::size_t set_size(int x) const { return size_[check(rep_[check(x)])]; }

    bool unite(int x, int y) {
        int a = find(x);
        int b = find(y);
        if (a == b) {
            return false;
        }
        if (size_[index(a)] < size_[index(b)]) {
            std::swap(a, b);
        }
        // Relabel only the smaller list, then splice it onto the end of the larger one.
        for (int member = b; member != -1; member = next_[index(member)]) {
            rep_[index(member)] = a;
            ++rep_updates_;
        }
        next_[index(tail_[index(a)])] = b;
        tail_[index(a)] = tail_[index(b)];
        size_[index(a)] += size_[index(b)];
        return true;
    }

    std::vector<int> members(int x) const {
        std::vector<int> out;
        out.reserve(set_size(x));
        for (int member = find(x); member != -1; member = next_[index(member)]) {
            out.push_back(member);
        }
        return out;
    }

    std::uint64_t rep_updates() const { return rep_updates_; }

private:
    static std::size_t index(int x) { return static_cast<std::size_t>(x); }

    std::size_t check(int x) const {
        if (x < 0 || index(x) >= rep_.size()) {
            throw std::out_of_range("element out of range");
        }
        return index(x);
    }

    std::vector<int> rep_;
    std::vector<int> next_;
    std::vector<int> tail_;
    std::vector<std::size_t> size_;
    std::uint64_t rep_updates_ = 0;
};

class ForestDsu {
public:
    explicit ForestDsu(std::size_t n = 0) {
        for (std::size_t i = 0; i < n; ++i) {
            make_set();
        }
    }

    int make_set() {
        const int id = static_cast<int>(parent_.size());
        parent_.push_back(id);
        rank_.push_back(0);
        size_.push_back(1);
        next_.push_back(id);
        ++components_;
        return id;
    }

    int find(int x) {
        check(x);
        int root = x;
        while (parent_[index(root)] != root) {
            root = parent_[index(root)];
            ++hops_;
        }
        while (parent_[index(x)] != root && x != root) {
            x = std::exchange(parent_[index(x)], root);
        }
        return root;
    }

    bool unite(int x, int y) {
        int a = find(x);
        int b = find(y);
        if (a == b) {
            return false;
        }
        if (rank_[index(a)] < rank_[index(b)]) {
            std::swap(a, b);
        }
        parent_[index(b)] = a;
        size_[index(a)] += size_[index(b)];
        if (rank_[index(a)] == rank_[index(b)]) {
            ++rank_[index(a)];
        }
        // Two circular lists fuse by swapping one next pointer in each.
        std::swap(next_[index(a)], next_[index(b)]);
        --components_;
        return true;
    }

    bool connected(int x, int y) { return find(x) == find(y); }

    std::size_t set_size(int x) { return size_[index(find(x))]; }

    std::size_t count() const { return components_; }

    std::size_t size() const { return parent_.size(); }

    int depth(int x) const {
        check(x);
        int links = 0;
        while (parent_[index(x)] != x) {
            x = parent_[index(x)];
            ++links;
        }
        return links;
    }

    std::vector<int> members(int x) {
        const std::size_t total = set_size(x);
        std::vector<int> out;
        out.reserve(total);
        int member = x;
        for (std::size_t i = 0; i < total; ++i) {
            out.push_back(member);
            member = next_[index(member)];
        }
        return out;
    }

    std::uint64_t hops() const { return hops_; }

private:
    static std::size_t index(int x) { return static_cast<std::size_t>(x); }

    void check(int x) const {
        if (x < 0 || index(x) >= parent_.size()) {
            throw std::out_of_range("element out of range");
        }
    }

    std::vector<int> parent_;
    std::vector<int> rank_;
    std::vector<std::size_t> size_;
    std::vector<int> next_;
    std::size_t components_ = 0;
    std::uint64_t hops_ = 0;
};

class DedupIndex {
public:
    int add_record(const std::vector<std::string>& identifiers) {
        const int record = dsu_.make_set();
        for (const std::string& id : identifiers) {
            const auto [it, inserted] = owner_.try_emplace(id, record);
            if (!inserted) {
                dsu_.unite(record, it->second);
            }
        }
        return record;
    }

    bool same_entity(int a, int b) { return dsu_.connected(a, b); }

    std::size_t entity_count() const { return dsu_.count(); }

    std::vector<int> entity_members(int record) {
        std::vector<int> members = dsu_.members(record);
        std::ranges::sort(members);
        return members;
    }

private:
    ForestDsu dsu_;
    std::unordered_map<std::string, int> owner_;
};
