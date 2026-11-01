#pragma once

#include "../platform/VirtualHardware.hpp"
#include "../platform/SimpleFileEngine.hpp"
#include "../platform/Tracer.hpp"
#include "../platform/OSApi.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace os_sim {
namespace monolithic {

// Standard System Call Numbers
constexpr int SYS_OPEN          = 1;
constexpr int SYS_READ          = 2;
constexpr int SYS_WRITE         = 3;
constexpr int SYS_CLOSE         = 4;
constexpr int SYS_CONSOLE_WRITE = 5;

// Arguments passed to a system call dispatcher.
struct SyscallArgs {
    std::string path;
    OpenMode mode{OpenMode::Read};
    int handle{-1};
    uint8_t* buffer{nullptr};
    const uint8_t* write_buffer{nullptr};
    size_t count{0};
    std::string text;
};

// Kernel File Descriptor entry maintained inside the kernel space.
struct KernelFileDesc {
    std::string path;
    OpenMode mode{OpenMode::Read};
    size_t offset{0};
    bool is_open{false};
};

// Monolithic Kernel implementation.
// All subsystems reside in a single kernel address space with static linking.
// User applications enter via the syscall() dispatcher table.
class MonolithicKernel {
public:
    using SyscallHandler = std::function<int(MonolithicKernel&, const SyscallArgs&)>;

    MonolithicKernel(VirtualDisk& disk, IVirtualConsoleDevice& console, Tracer& tracer);

    int syscall(int syscall_num, const SyscallArgs& args);

private:
    void setup_syscall_table();

    // Ready-made adapters used by the syscall table.
    static int handle_open(MonolithicKernel& kernel, const SyscallArgs& args);
    static int handle_read(MonolithicKernel& kernel, const SyscallArgs& args);
    static int handle_write(MonolithicKernel& kernel, const SyscallArgs& args);
    static int handle_close(MonolithicKernel& kernel, const SyscallArgs& args);
    static int handle_console_write(MonolithicKernel& kernel, const SyscallArgs& args);

    int sys_open(const std::string& path, OpenMode mode);
    int sys_read(int handle, uint8_t* out_buf, size_t max_bytes);
    int sys_write(int handle, const uint8_t* in_data, size_t count);
    int sys_close(int handle);
    int sys_console_write(const std::string& text);

    int allocate_fd();
    bool is_valid_fd(int handle) const;

    VirtualDisk& disk_;
    IVirtualConsoleDevice& console_;
    Tracer& tracer_;
    SimpleFileEngine file_engine_;
    std::unordered_map<int, SyscallHandler> syscall_table_;
    std::vector<KernelFileDesc> fd_table_;
};

} // namespace monolithic
} // namespace os_sim
