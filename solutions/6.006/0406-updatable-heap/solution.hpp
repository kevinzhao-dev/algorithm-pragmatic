#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class UpdatableHeap {
public:
    class Handle {
    public:
        Handle() = default;
        friend bool operator==(const Handle&, const Handle&) = default;

    private:
        friend class UpdatableHeap;
        Handle(std::uint32_t slot, std::uint32_t generation) : slot_(slot), generation_(generation) {}
        std::uint32_t slot_ = kNoSlot;
        std::uint32_t generation_ = 0;
    };

    explicit UpdatableHeap(Compare compare = Compare{}) : compare_(std::move(compare)) {}

    Handle push(T value) {
        const std::uint32_t slot = allocate_slot();
        heap_.push_back(Entry{std::move(value), slot});
        slots_[slot].position = heap_.size() - 1;
        sift_up(heap_.size() - 1);
        return Handle(slot, slots_[slot].generation);
    }

    const T& top() const {
        require_nonempty();
        return heap_.front().value;
    }

    Handle top_handle() const {
        require_nonempty();
        const std::uint32_t slot = heap_.front().slot;
        return Handle(slot, slots_[slot].generation);
    }

    T pop() {
        require_nonempty();
        return remove_at(0);
    }

    std::size_t size() const { return heap_.size(); }
    bool empty() const { return heap_.empty(); }

    bool contains(Handle handle) const { return is_live(handle); }

    const T& get(Handle handle) const { return heap_[position_of(handle)].value; }

    void update(Handle handle, T value) {
        const std::size_t position = position_of(handle);
        heap_[position].value = std::move(value);
        restore(position);
    }

    T erase(Handle handle) { return remove_at(position_of(handle)); }

    std::vector<Handle> build(std::vector<T> items) {
        heap_.clear();
        heap_.reserve(items.size());
        slots_.clear();
        free_.clear();
        slots_.resize(items.size());
        std::vector<Handle> handles;
        handles.reserve(items.size());
        for (std::size_t i = 0; i < items.size(); ++i) {
            const auto slot = static_cast<std::uint32_t>(i);
            heap_.push_back(Entry{std::move(items[i]), slot});
            slots_[slot].position = i;
            handles.push_back(Handle(slot, slots_[slot].generation));
        }
        for (std::size_t index = heap_.size() / 2; index-- > 0;) {
            sift_down(index);
        }
        return handles;
    }

private:
    static constexpr std::uint32_t kNoSlot = std::numeric_limits<std::uint32_t>::max();
    static constexpr std::size_t kFree = std::numeric_limits<std::size_t>::max();

    struct Entry {
        T value;
        std::uint32_t slot;
    };

    // A slot maps a stable handle to the entry's current index in the heap array.
    struct Slot {
        std::size_t position = kFree;
        std::uint32_t generation = 0;
    };

    void require_nonempty() const {
        if (heap_.empty()) {
            throw std::out_of_range("empty heap");
        }
    }

    std::uint32_t allocate_slot() {
        if (!free_.empty()) {
            const std::uint32_t slot = free_.back();
            free_.pop_back();
            return slot;
        }
        slots_.emplace_back();
        return static_cast<std::uint32_t>(slots_.size() - 1);
    }

    bool is_live(Handle handle) const {
        return handle.slot_ < slots_.size() && slots_[handle.slot_].generation == handle.generation_ &&
               slots_[handle.slot_].position != kFree;
    }

    std::size_t position_of(Handle handle) const {
        if (!is_live(handle)) {
            throw std::invalid_argument("stale or invalid handle");
        }
        return slots_[handle.slot_].position;
    }

    void place(std::size_t position, Entry entry) {
        slots_[entry.slot].position = position;
        heap_[position] = std::move(entry);
    }

    void swap_entries(std::size_t a, std::size_t b) {
        std::swap(heap_[a], heap_[b]);
        slots_[heap_[a].slot].position = a;
        slots_[heap_[b].slot].position = b;
    }

    void sift_up(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!compare_(heap_[parent].value, heap_[index].value)) {
                return;
            }
            swap_entries(parent, index);
            index = parent;
        }
    }

    void sift_down(std::size_t index) {
        while (true) {
            const std::size_t left = 2 * index + 1;
            if (left >= heap_.size()) {
                return;
            }
            std::size_t child = left;
            const std::size_t right = left + 1;
            if (right < heap_.size() && compare_(heap_[left].value, heap_[right].value)) {
                child = right;
            }
            if (!compare_(heap_[index].value, heap_[child].value)) {
                return;
            }
            swap_entries(index, child);
            index = child;
        }
    }

    // The entry may need to move in either direction; at most one of these does anything.
    void restore(std::size_t position) {
        if (position > 0 && compare_(heap_[(position - 1) / 2].value, heap_[position].value)) {
            sift_up(position);
        } else {
            sift_down(position);
        }
    }

    T remove_at(std::size_t position) {
        const std::uint32_t slot = heap_[position].slot;
        swap_entries(position, heap_.size() - 1);
        T value = std::move(heap_.back().value);
        heap_.pop_back();
        // Invalidate the handle before reusing the slot, so stale copies are detectable.
        slots_[slot].position = kFree;
        ++slots_[slot].generation;
        free_.push_back(slot);
        if (position < heap_.size()) {
            restore(position);
        }
        return value;
    }

    std::vector<Entry> heap_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_;
    Compare compare_;
};

class DeadlineScheduler {
public:
    bool submit(const std::string& name, std::int64_t deadline) {
        if (tasks_.contains(name)) {
            return false;
        }
        const Handle handle = heap_.push(Task{deadline, next_sequence_++, name});
        tasks_.emplace(name, handle);
        return true;
    }

    bool reschedule(const std::string& name, std::int64_t deadline) {
        const auto found = tasks_.find(name);
        if (found == tasks_.end()) {
            return false;
        }
        Task task = heap_.get(found->second);
        task.deadline = deadline;
        heap_.update(found->second, std::move(task));
        return true;
    }

    bool cancel(const std::string& name) {
        const auto found = tasks_.find(name);
        if (found == tasks_.end()) {
            return false;
        }
        heap_.erase(found->second);
        tasks_.erase(found);
        return true;
    }

    std::optional<std::string> next() {
        if (heap_.empty()) {
            return std::nullopt;
        }
        Task task = heap_.pop();
        tasks_.erase(task.name);
        return std::move(task.name);
    }

    std::optional<std::string> peek() const {
        if (heap_.empty()) {
            return std::nullopt;
        }
        return heap_.top().name;
    }

    std::size_t size() const { return heap_.size(); }

private:
    struct Task {
        std::int64_t deadline;
        std::uint64_t sequence;
        std::string name;
    };
    // The heap is a max-heap, so "later" means lower priority.
    struct LaterDeadline {
        bool operator()(const Task& a, const Task& b) const {
            return a.deadline != b.deadline ? a.deadline > b.deadline : a.sequence > b.sequence;
        }
    };
    using Heap = UpdatableHeap<Task, LaterDeadline>;
    using Handle = typename Heap::Handle;

    Heap heap_;
    std::unordered_map<std::string, Handle> tasks_;
    std::uint64_t next_sequence_ = 0;
};
