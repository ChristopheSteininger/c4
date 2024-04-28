#include "result.h"

#include <cassert>

#include "../search.h"

void SearchResult::reset() {
    std::unique_lock<std::mutex> lock(mutex);

    score = SEARCH_STOPPED;
    found = false;
}

bool SearchResult::notify_result(int result) {
    std::unique_lock<std::mutex> lock(mutex);

    // Do nothing if another thread already found the solution.
    if (found) {
        return false;
    }

    score = result;
    found = true;

    lock.unlock();
    cond.notify_all();

    return true;
}

int SearchResult::wait_for_result() {
    std::unique_lock<std::mutex> lock(mutex);

    while (!found) {
        cond.wait(lock);
    }

    assert(score != SEARCH_STOPPED);
    return score;
}
