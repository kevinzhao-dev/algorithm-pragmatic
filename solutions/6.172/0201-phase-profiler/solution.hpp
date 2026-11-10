#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class Profiler {
public:
    using Clock = std::function<std::uint64_t()>;

    struct Entry {
        std::string name;
        std::uint64_t calls = 0;
        std::uint64_t inclusive = 0;
        std::uint64_t self = 0;
    };

    explicit Profiler(Clock clock) : clock_(std::move(clock)) { nodes_.push_back(Node{}); }

    void enter(std::string_view name) {
        const std::size_t parent = stack_.empty() ? 0 : stack_.back().node;
        stack_.push_back(Frame{find_or_add_child(parent, name), clock_(), 0});
    }

    void exit() {
        if (stack_.empty()) {
            throw std::logic_error("exit without matching enter");
        }
        const std::uint64_t now = clock_();
        const Frame frame = stack_.back();
        stack_.pop_back();
        const std::uint64_t elapsed = now - frame.start;
        Node& node = nodes_[frame.node];
        node.calls += 1;
        node.total += elapsed;
        node.self += elapsed - frame.nested;
        if (!stack_.empty()) {
            stack_.back().nested += elapsed;
        }
    }

    std::size_t depth() const { return stack_.size(); }

    std::vector<Entry> report() const {
        std::map<std::string, Entry, std::less<>> entries;
        walk([&](const Node& node, std::span<const std::size_t> path) {
            if (node.calls == 0) {
                return;
            }
            Entry& entry = entries[node.name];
            entry.name = node.name;
            entry.calls += node.calls;
            entry.self += node.self;
            // Only the outermost activation of a name on this path contributes inclusive time.
            bool outermost = true;
            for (const std::size_t ancestor : path) {
                outermost = outermost && nodes_[ancestor].name != node.name;
            }
            if (outermost) {
                entry.inclusive += node.total;
            }
        });
        return sorted_entries(std::move(entries));
    }

    std::vector<std::string> folded() const {
        std::vector<std::string> lines;
        walk([&](const Node& node, std::span<const std::size_t> path) {
            if (node.self == 0) {
                return;
            }
            std::string line;
            for (const std::size_t ancestor : path) {
                line += nodes_[ancestor].name;
                line += ';';
            }
            line += node.name;
            line += ' ';
            line += std::to_string(node.self);
            lines.push_back(std::move(line));
        });
        std::sort(lines.begin(), lines.end());
        return lines;
    }

private:
    struct Node {
        std::string name;
        std::uint64_t calls = 0;
        std::uint64_t total = 0;
        std::uint64_t self = 0;
        std::vector<std::pair<std::string, std::size_t>> children;
    };

    struct Frame {
        std::size_t node;
        std::uint64_t start;
        std::uint64_t nested;
    };

    std::size_t find_or_add_child(std::size_t parent, std::string_view name) {
        for (const auto& [child_name, index] : nodes_[parent].children) {
            if (child_name == name) {
                return index;
            }
        }
        const std::size_t index = nodes_.size();
        nodes_.push_back(Node{});
        nodes_.back().name = std::string(name);
        nodes_[parent].children.emplace_back(std::string(name), index);
        return index;
    }

    template <typename Visit>
    void walk(Visit&& visit) const {
        std::vector<std::size_t> path;
        for (const auto& [name, child] : nodes_[0].children) {
            walk_node(child, path, visit);
        }
    }

    template <typename Visit>
    void walk_node(std::size_t index, std::vector<std::size_t>& path, Visit& visit) const {
        visit(nodes_[index], path);
        path.push_back(index);
        for (const auto& [name, child] : nodes_[index].children) {
            walk_node(child, path, visit);
        }
        path.pop_back();
    }

    static std::vector<Entry> sorted_entries(std::map<std::string, Entry, std::less<>> entries) {
        std::vector<Entry> result;
        result.reserve(entries.size());
        for (auto& [name, entry] : entries) {
            result.push_back(std::move(entry));
        }
        std::stable_sort(result.begin(), result.end(), [](const Entry& a, const Entry& b) {
            return a.self > b.self;
        });
        return result;
    }

    friend std::vector<Profiler::Entry> report_from_samples(
        std::span<const std::vector<std::string>> stacks);

    Clock clock_;
    std::vector<Node> nodes_;
    std::vector<Frame> stack_;
};

class ScopedPhase {
public:
    ScopedPhase(Profiler& profiler, std::string_view name) : profiler_(profiler) {
        profiler_.enter(name);
    }
    ~ScopedPhase() {
        try {
            profiler_.exit();
        } catch (...) {  // a destructor must never throw
        }
    }

    ScopedPhase(const ScopedPhase&) = delete;
    ScopedPhase& operator=(const ScopedPhase&) = delete;

private:
    Profiler& profiler_;
};

inline std::vector<Profiler::Entry> report_from_samples(
    std::span<const std::vector<std::string>> stacks) {
    std::map<std::string, Profiler::Entry, std::less<>> entries;
    for (const std::vector<std::string>& stack : stacks) {
        if (stack.empty()) {
            continue;
        }
        Profiler::Entry& leaf = entries[stack.back()];
        leaf.name = stack.back();
        leaf.self += 1;
        std::vector<std::string_view> seen;
        seen.reserve(stack.size());
        for (const std::string& frame : stack) {
            if (std::find(seen.begin(), seen.end(), std::string_view(frame)) == seen.end()) {
                seen.emplace_back(frame);
                Profiler::Entry& entry = entries[frame];
                entry.name = frame;
                entry.inclusive += 1;
            }
        }
    }
    return Profiler::sorted_entries(std::move(entries));
}
