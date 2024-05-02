#include "writer.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include "../settings.h"

static inline constexpr int MAX_LINES_IN_BUFFER = 1000;
static inline constexpr std::chrono::steady_clock::duration MAX_TIME_BETWEEN_WRITES = std::chrono::seconds(1);

Writer::Writer(const std::filesystem::path &file_path) {
    if constexpr (!UPDATE_TABLE_FILE) {
        return;
    }

    this->file_thread = std::thread(&Writer::save_to_file, this, file_path);
}

Writer::~Writer() {
    assert(is_running);

    is_running = false;
    cond.notify_one();

    if (file_thread.joinable()) {
        file_thread.join();
    }
}

void Writer::add_line(const std::string &line) {
    if constexpr (!UPDATE_TABLE_FILE) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex);

    lines_in_active_buffer++;
    if (active_buffer == 0) {
        buffer0 << line << std::endl;
    } else {
        buffer1 << line << std::endl;
    }
    
    // Trigger a write to disk.
    if (should_write_to_disk()) {
        cond.notify_one();
    }
}

bool Writer::should_write_to_disk() const {
    return lines_in_active_buffer >= MAX_LINES_IN_BUFFER
        || std::chrono::steady_clock::now() - last_write > MAX_TIME_BETWEEN_WRITES;
}

void Writer::save_to_file(const std::filesystem::path &file_path) {
    std::ofstream file;
    file.open(file_path, std::ios::app);

    if (!file) {
        std::cerr << "Failed to open the file " << file_path << "." << std::endl;
        return;
    }

    std::unique_lock<std::mutex> lock(mutex);
    
    while (is_running) {
        // We avoid writing lines one by one, so wait until we have enough data to save.
        while (is_running && !should_write_to_disk()) {
            cond.wait(lock);
        }

        // Swap buffers and unlock so search threads are not blocked on writing to disk.
        active_buffer = 1 - active_buffer;
        lines_in_active_buffer = 0;
        last_write = std::chrono::steady_clock::now();
        lock.unlock();

        // Save the inactive buffer to disk.
        if (active_buffer == 0) {
            file << buffer1.str();
            buffer1.str(std::string());
        } else {
            file << buffer0.str();
            buffer0.str(std::string());
        }

        lock.lock();
    }
}
