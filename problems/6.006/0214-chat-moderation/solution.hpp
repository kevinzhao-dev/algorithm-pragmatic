#pragma once

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

template <typename Viewer, typename Message>
class ChatRoom {
public:
    // Stage 1 (and Stage 3's per_viewer_limit)
    explicit ChatRoom(std::vector<Viewer> viewers,
                      std::size_t per_viewer_limit = std::numeric_limits<std::size_t>::max()) {
        PRACTICE_TODO();
    }

    bool send(const Viewer& viewer, Message message) { PRACTICE_TODO(); }
    std::vector<Message> recent(std::size_t k) const { PRACTICE_TODO(); }
    std::size_t message_count() const { PRACTICE_TODO(); }

    // Stage 2
    bool ban(const Viewer& viewer) { PRACTICE_TODO(); }
    bool is_banned(const Viewer& viewer) const { PRACTICE_TODO(); }
};
