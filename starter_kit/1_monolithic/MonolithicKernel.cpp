#include "MonolithicKernel.hpp"

namespace os_sim {
namespace monolithic {

MonolithicKernel::MonolithicKernel(VirtualDisk& disk, IVirtualConsoleDevice& console, Tracer& tracer)
    : disk_(disk), console_(console), tracer_(tracer),
      file_engine_(disk.get_total_blocks(),
                   [&disk](size_t id, std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.read_block(id, b); },
                   [&disk](size_t id, const std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.write_block(id, b); }) {
    setup_syscall_table();
}

int MonolithicKernel::syscall(int syscall_num, const SyscallArgs& args) {
    // TODO
    // Trace entry into the kernel and record call depth.
    // TODO
    // Dispatch the syscall and handle invalid numbers.
    // TODO
    // Trace the return to user space and return the result.

    // YOUR CODE HERE
    (void)syscall_num;
    (void)args;
    return ErrorCode::InvalidArgument;
}

void MonolithicKernel::setup_syscall_table() {
    // TODO
    // Register each syscall number with its matching kernel handler.

    // YOUR CODE HERE
}

int MonolithicKernel::handle_open(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_open(args.path, args.mode);
}

int MonolithicKernel::handle_read(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_read(args.handle, args.buffer, args.count);
}

int MonolithicKernel::handle_write(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_write(args.handle, args.write_buffer, args.count);
}

int MonolithicKernel::handle_close(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_close(args.handle);
}

int MonolithicKernel::handle_console_write(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_console_write(args.text);
}

int MonolithicKernel::sys_open(const std::string& path, OpenMode mode) {
    // TODO
    // Validate the path, apply the open mode, and allocate a file descriptor.

    // YOUR CODE HERE
    (void)path;
    (void)mode;
    return ErrorCode::NotFound;
}

int MonolithicKernel::sys_read(int handle, uint8_t* out_buf, size_t max_bytes) {
    // TODO
    // Validate read access, read data, and update the offset and copy trace.

    // YOUR CODE HERE
    (void)handle;
    (void)out_buf;
    (void)max_bytes;
    return ErrorCode::InvalidHandle;
}

int MonolithicKernel::sys_write(int handle, const uint8_t* in_data, size_t count) {
    // TODO
    // Validate write access, write data, and update the offset and copy trace.

    // YOUR CODE HERE
    (void)handle;
    (void)in_data;
    (void)count;
    return ErrorCode::InvalidHandle;
}

int MonolithicKernel::sys_close(int handle) {
    // TODO
    // Validate and close the file descriptor.

    // YOUR CODE HERE
    (void)handle;
    return ErrorCode::InvalidHandle;
}

int MonolithicKernel::sys_console_write(const std::string& text) {
    console_.write(text);
    return ErrorCode::Success;
}

int MonolithicKernel::allocate_fd() {
    for (size_t i = 0; i < fd_table_.size(); ++i) {
        if (!fd_table_[i].is_open) {
            return static_cast<int>(i);
        }
    }
    fd_table_.push_back(KernelFileDesc{});
    return static_cast<int>(fd_table_.size() - 1);
}

bool MonolithicKernel::is_valid_fd(int handle) const {
    if (handle < 0 || static_cast<size_t>(handle) >= fd_table_.size()) {
        return false;
    }
    return fd_table_[handle].is_open;
}

} // namespace monolithic
} // namespace os_sim
