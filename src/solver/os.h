#ifndef OS_H_
#define OS_H_

#include <cstdint>
#include <thread>

void *allocate_huge_pages(size_t count, size_t size);

void free_huge_pages(void *memory);

void set_thread_affinity(std::thread &thread, int id);

#endif
