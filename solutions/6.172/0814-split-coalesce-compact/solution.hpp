#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>

enum class Coalesce { Never, Adjacent };

class TwoSizeHeap {
public:
    TwoSizeHeap(std::size_t lines, Coalesce policy) : policy_(policy) {
        for (std::size_t i = 0; i < lines; ++i) {
            free_large_.insert(i * kLine);
        }
    }

    std::optional<std::size_t> alloc_small() {
        if (!free_small_.empty()) {
            const std::size_t offset = *free_small_.begin();
            free_small_.erase(free_small_.begin());
            return offset;
        }
        if (free_large_.empty()) {
            return std::nullopt;
        }
        const std::size_t offset = *free_large_.begin();
        free_large_.erase(free_large_.begin());
        free_small_.insert(offset + kSmall);
        return offset;
    }

    std::optional<std::size_t> alloc_large() {
        if (free_large_.empty()) {
            return std::nullopt;
        }
        const std::size_t offset = *free_large_.begin();
        free_large_.erase(free_large_.begin());
        return offset;
    }

    void free_small(std::size_t offset) {
        const std::size_t buddy = offset ^ kSmall;
        if (policy_ == Coalesce::Adjacent && free_small_.erase(buddy) == 1) {
            free_large_.insert(offset & ~(kLine - 1));
        } else {
            free_small_.insert(offset);
        }
    }

    void free_large(std::size_t offset) { free_large_.insert(offset); }

    std::size_t free_bytes() const {
        return free_large_.size() * kLine + free_small_.size() * kSmall;
    }

private:
    static constexpr std::size_t kSmall = 32;
    static constexpr std::size_t kLine = 64;

    Coalesce policy_;
    std::set<std::size_t> free_large_;
    std::set<std::size_t> free_small_;
};

class CompactingHeap {
public:
    using Handle = std::size_t;

    explicit CompactingHeap(std::size_t lines)
        : arena_(lines * kLine), half_owner_(lines * 2, kNone), free_halves_(lines * 2) {}

    std::optional<Handle> alloc_small() { return place(1); }

    std::optional<Handle> alloc_large() {
        if (auto handle = place(2)) {
            return handle;
        }
        if (free_bytes() >= kLine) {
            compact();
            return place(2);
        }
        return std::nullopt;
    }

    void free(Handle handle) {
        Object& object = objects_[handle];
        for (std::size_t h = object.half; h < object.half + object.halves; ++h) {
            half_owner_[h] = kNone;
        }
        object.live = false;
        free_handles_.push_back(handle);
        free_halves_ += object.halves;
    }

    std::span<std::byte> deref(Handle handle) {
        const Object& object = objects_[handle];
        return {arena_.data() + object.half * kHalf, object.halves * kHalf};
    }

    void compact() {
        const std::size_t lines = half_owner_.size() / 2;
        std::vector<std::size_t> half_used;
        for (std::size_t line = 0; line < lines; ++line) {
            const bool low = half_owner_[line * 2] != kNone;
            const bool high = half_owner_[line * 2 + 1] != kNone;
            if (low != high) {
                half_used.push_back(line);
            }
        }
        for (std::size_t i = 0; i + 1 < half_used.size(); i += 2) {
            move_object(half_used[i + 1], half_used[i]);
        }
    }

    std::size_t free_bytes() const { return free_halves_ * kHalf; }

private:
    static constexpr std::size_t kHalf = 32;
    static constexpr std::size_t kLine = 64;
    static constexpr std::size_t kNone = ~std::size_t{0};

    struct Object {
        std::size_t half = 0;
        std::size_t halves = 0;
        bool live = false;
    };

    // Places an object of `halves` half-lines at the lowest position that can take it.
    std::optional<Handle> place(std::size_t halves) {
        for (std::size_t h = 0; h + halves <= half_owner_.size(); h += halves == 2 ? 2 : 1) {
            bool available = true;
            for (std::size_t i = h; i < h + halves; ++i) {
                available = available && half_owner_[i] == kNone;
            }
            if (!available) {
                continue;
            }
            const Handle handle = new_handle();
            objects_[handle] = Object{h, halves, true};
            for (std::size_t i = h; i < h + halves; ++i) {
                half_owner_[i] = handle;
            }
            free_halves_ -= halves;
            return handle;
        }
        return std::nullopt;
    }

    Handle new_handle() {
        if (!free_handles_.empty()) {
            const Handle handle = free_handles_.back();
            free_handles_.pop_back();
            return handle;
        }
        objects_.emplace_back();
        return objects_.size() - 1;
    }

    // Moves the single live small object on line `from` into the free half of line `to`.
    void move_object(std::size_t from, std::size_t to) {
        const std::size_t source_half = half_owner_[from * 2] != kNone ? from * 2 : from * 2 + 1;
        const std::size_t target_half = half_owner_[to * 2] == kNone ? to * 2 : to * 2 + 1;
        const Handle handle = half_owner_[source_half];
        std::memcpy(arena_.data() + target_half * kHalf, arena_.data() + source_half * kHalf, kHalf);
        half_owner_[target_half] = handle;
        half_owner_[source_half] = kNone;
        objects_[handle].half = target_half;
    }

    std::vector<std::byte> arena_;
    std::vector<std::size_t> half_owner_;  // half-line -> handle, or kNone when free
    std::vector<Object> objects_;
    std::vector<Handle> free_handles_;
    std::size_t free_halves_;
};
