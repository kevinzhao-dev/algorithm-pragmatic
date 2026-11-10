#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../0406-updatable-heap/solution.hpp"

class CapacityPlanner {
public:
    using MachineId = std::uint64_t;

    struct Machine {
        MachineId id;
        std::uint64_t capacity;
    };

    explicit CapacityPlanner(std::span<const Machine> machines) {
        std::vector<Free> items;
        items.reserve(machines.size());
        for (const Machine& machine : machines) {
            items.push_back(Free{machine.capacity, machine.id});
        }
        const auto handles = free_.build(std::move(items));
        machines_.reserve(machines.size() * 2);
        for (std::size_t i = 0; i < machines.size(); ++i) {
            machines_.emplace(machines[i].id, State{handles[i], machines[i].capacity, {}});
        }
    }

    std::optional<MachineId> place(const std::string& task, std::uint64_t demand) {
        if (tasks_.contains(task)) {
            throw std::invalid_argument("task already placed: " + task);
        }
        if (free_.empty() || free_.top().available < demand) {
            return std::nullopt;
        }
        const MachineId machine = free_.top().machine;
        assign(task, demand, machine);
        return machine;
    }

    void release(const std::string& task) {
        const auto found = tasks_.find(task);
        if (found == tasks_.end()) {
            throw std::out_of_range("unknown task: " + task);
        }
        const auto [machine, demand] = found->second;
        tasks_.erase(found);
        State& state = machines_.at(machine);
        state.tasks.erase(task);
        adjust_available(state, +static_cast<std::int64_t>(demand));
    }

    std::vector<std::string> tasks_on(MachineId machine) const {
        const State& state = machines_.at(machine);
        return std::vector<std::string>(state.tasks.begin(), state.tasks.end());
    }

    std::uint64_t available(MachineId machine) const {
        return free_.get(machines_.at(machine).handle).available;
    }

    // Moves every task to the other machines, or changes nothing at all.
    bool drain(MachineId machine) {
        State& state = machines_.at(machine);
        if (state.tasks.empty()) {
            return true;
        }
        const Free cordoned = free_.erase(state.handle);

        struct Move {
            std::string task;
            std::uint64_t demand;
            MachineId moved_to;
        };
        std::vector<Move> moved;
        bool failed = false;
        for (const std::string& task : state.tasks) {
            const std::uint64_t demand = tasks_.at(task).demand;
            if (free_.empty() || free_.top().available < demand) {
                failed = true;
                break;
            }
            const MachineId target = free_.top().machine;
            assign_on_other(task, demand, target);
            moved.push_back({task, demand, target});
        }

        if (failed) {
            for (const Move& move : moved) {
                State& other = machines_.at(move.moved_to);
                other.tasks.erase(move.task);
                adjust_available(other, +static_cast<std::int64_t>(move.demand));
                tasks_.at(move.task) = TaskInfo{machine, move.demand};
            }
            state.handle = free_.push(cordoned);
            return false;
        }

        state.tasks.clear();
        state.handle = free_.push(Free{state.capacity, machine});
        return true;
    }

private:
    struct Free {
        std::uint64_t available;
        MachineId machine;
    };
    struct MoreFreeFirst {
        bool operator()(const Free& a, const Free& b) const { return a.available < b.available; }
    };
    using Heap = UpdatableHeap<Free, MoreFreeFirst>;
    using Handle = Heap::Handle;

    struct State {
        Handle handle;
        std::uint64_t capacity;
        std::unordered_set<std::string> tasks;
    };
    struct TaskInfo {
        MachineId machine;
        std::uint64_t demand;
    };

    void adjust_available(State& state, std::int64_t delta) {
        Free entry = free_.get(state.handle);
        entry.available = static_cast<std::uint64_t>(static_cast<std::int64_t>(entry.available) + delta);
        free_.update(state.handle, entry);
    }

    void assign(const std::string& task, std::uint64_t demand, MachineId machine) {
        State& state = machines_.at(machine);
        adjust_available(state, -static_cast<std::int64_t>(demand));
        state.tasks.insert(task);
        tasks_.emplace(task, TaskInfo{machine, demand});
    }

    // Used during drain: the task's record already exists and only its location changes.
    void assign_on_other(const std::string& task, std::uint64_t demand, MachineId machine) {
        State& state = machines_.at(machine);
        adjust_available(state, -static_cast<std::int64_t>(demand));
        state.tasks.insert(task);
        tasks_.at(task) = TaskInfo{machine, demand};
    }

    Heap free_;
    std::unordered_map<MachineId, State> machines_;
    std::unordered_map<std::string, TaskInfo> tasks_;
};
