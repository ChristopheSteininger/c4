#include "os.h"

#include <iostream>

#include "../settings.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static void *windows_allocate_huge_pages(size_t count, size_t size) {
    size_t large_page_size = GetLargePageMinimum();
    if (!large_page_size) {
        std::cerr << "Error fetching large page size." << std::endl;
        return NULL;
    }

    LUID lock_memory_luid;
    if (!LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &lock_memory_luid)) {
        std::cerr << "Error fetching lock memory privilege." << std::endl;
        return NULL;
    }
    
    HANDLE token{};
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        std::cerr << "Error fetching process token." << std::endl;
        return NULL;
    }

    TOKEN_PRIVILEGES new_privileges{};
    new_privileges.PrivilegeCount = 1;
    new_privileges.Privileges[0].Luid = lock_memory_luid;
    new_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(token, false, &new_privileges, sizeof(new_privileges), NULL, NULL)) {
        CloseHandle(token);
        std::cerr << "Error adjusting token privileges." << std::endl;
        return NULL;
    }

    CloseHandle(token);

    DWORD last_error = GetLastError();
    if (last_error != ERROR_SUCCESS) {
        std::cerr << "Error adjusting token privileges: " << last_error << std::endl;
        return NULL;
    }

    // Round up to the nearest large page size.
    size_t allocate_size = (count * size + large_page_size - 1) & ~(large_page_size - 1);

    void *mem_with_huge_pages = VirtualAlloc(
            NULL, allocate_size, MEM_LARGE_PAGES | MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem_with_huge_pages) {
        std::cerr << "Error allocating huge pages." << std::endl;
        return NULL;
    }

    return mem_with_huge_pages;
}
#endif

void *allocate_huge_pages(size_t count, size_t size) {
    if (!ENABLE_HUGE_PAGES) {
        return calloc(count, size);
    }

#ifdef _WIN32
    void *mem_with_huge_pages = windows_allocate_huge_pages(count, size);
    if (!mem_with_huge_pages) {
        return calloc(count, size);
    }

    return mem_with_huge_pages;
#else
    std::cerr << "Error huge pages requested but not implemented." << std::endl;
    return calloc(count, size);
#endif
}

void free_huge_pages(void *memory) {
    if (!memory) {
        std::cerr << "Error memory already freed." << std::endl;
        return;
    }

    if (!ENABLE_HUGE_PAGES) {
        free(memory);
        return;
    }

#ifdef _WIN32
    if (!VirtualFree(memory, 0, MEM_RELEASE)) {
        std::cerr << "Error freeing memory." << std::endl;
    }
#else
    free(memory);
#endif
}

void set_thread_affinity(std::thread &thread, int id) {
    if (!ENABLE_AFFINITY) {
        return;
    }

#ifdef _WIN32
    SetThreadAffinityMask(thread.native_handle(), DWORD_PTR(1) << id);
#else
    std::cerr << "Error thread affinity requested but not implemented." << std::endl;
#endif
}

void os_prefetch(void *address) {
#ifdef _WIN32
    _mm_prefetch(static_cast<const char *>(address), _MM_HINT_T2);
#else
    // void __builtin_prefetch(const void *addr, int rw=0, int locality=3)
    // rw       = read/write flag. 0 for read, 1 for write & read/write.
    // locality = persistance in cache.
    __builtin_prefetch(address, 1, 3);
#endif
}
