#pragma once

namespace ugv {

struct SearchQueueEntry {
    int x = 0;
    int y = 0;

    double priority = 0.0;
};

// std::priority_queue is a max-heap by default.
// Reverse comparison so the smallest priority is processed first.
struct SearchQueueEntryCompare {
    bool operator()(
        const SearchQueueEntry& lhs,
        const SearchQueueEntry& rhs
    ) const noexcept {
        return lhs.priority > rhs.priority;
    }
};

}  // namespace ugv
