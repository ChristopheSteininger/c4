#ifndef RESULT_H_
#define RESULT_H_

#include <condition_variable>
#include <mutex>

#include "../search.h"

// A thread safe wrapper for the result of a search.
class SearchResult {
   public:
    void reset();
    bool notify_result(int result);
    int wait_for_result();

   private:
    int score{SEARCH_STOPPED};
    bool found{false};

    std::mutex mutex;
    std::condition_variable cond;
};

#endif
