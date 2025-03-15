#ifndef WRITER_H_
#define WRITER_H_

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// Allows search threads to save important results to disk.
// Thread safe.
class Writer {
   public:
    Writer(const std::filesystem::path &file_path);
    ~Writer();

    void add_line(const std::string &line);

   private:
    std::mutex mutex;
    std::condition_variable cond;

    // Start of data shared between threads.
    bool is_running{true};
    int active_buffer_index{0};
    int lines_in_active_buffer{0};
    std::chrono::steady_clock::time_point last_write{std::chrono::steady_clock::now()};

    // One buffer is used by search threads to save new data and the other
    // buffer is used by the file thread to write to disk. The buffers are
    // swapped before each save to disk.
    std::stringstream buffer0{};
    std::stringstream buffer1{};
    // End of shared data.

    std::thread file_thread;

    bool should_write_to_disk() const;
    void save_to_file(const std::filesystem::path &file_path);
};

#endif
