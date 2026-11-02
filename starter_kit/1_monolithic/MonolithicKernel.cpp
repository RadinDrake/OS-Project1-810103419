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
    // Trace entry into the kernel and record call depth.
    tracer_.record_domain_switch("UserSpace", "MonolithicKernel", "syscall " + std::to_string(syscall_num));
    tracer_.record_call_depth(3); // declared (illustrative) depth, not a measured stack

    // Dispatch the syscall and handle invalid numbers.
    int result = ErrorCode::InvalidArgument;
    auto it = syscall_table_.find(syscall_num);
    if (it != syscall_table_.end()) {
        result = it->second(*this, args);
    }

    // Trace the return to user space and return the result.
    tracer_.record_domain_switch("MonolithicKernel", "UserSpace", "syscall_return");
    return result;
}

void MonolithicKernel::setup_syscall_table() {
    // Register each syscall number with its matching kernel handler.
    syscall_table_[SYS_OPEN]          = &MonolithicKernel::handle_open;
    syscall_table_[SYS_READ]          = &MonolithicKernel::handle_read;
    syscall_table_[SYS_WRITE]         = &MonolithicKernel::handle_write;
    syscall_table_[SYS_CLOSE]         = &MonolithicKernel::handle_close;
    syscall_table_[SYS_CONSOLE_WRITE] = &MonolithicKernel::handle_console_write;
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
    // Validate the path, apply the open mode, and allocate a file descriptor.
    if (path.empty()) {
        return ErrorCode::InvalidArgument;
    }

    switch (mode) {
        case OpenMode::Read:
            if (!file_engine_.file_exists(path)) {
                return ErrorCode::NotFound;
            }
            break;
        case OpenMode::Write:
            // Truncate if it exists, create if it does not.
            if (file_engine_.file_exists(path)) {
                file_engine_.truncate_file(path);
            } else if (!file_engine_.create_file(path)) {
                return ErrorCode::NoSpace;
            }
            break;
        default:
            // Append / ReadWrite are not supported in this exercise.
            return ErrorCode::InvalidArgument;
    }

    int fd = allocate_fd();
    KernelFileDesc& desc = fd_table_[fd];
    desc.path = path;
    desc.mode = mode;
    desc.offset = 0;
    desc.is_open = true;
    return fd;
}

int MonolithicKernel::sys_read(int handle, uint8_t* out_buf, size_t max_bytes) {
    // Validate read access, read data, and update the offset and copy trace.
    if (!is_valid_fd(handle)) {
        return ErrorCode::InvalidHandle;
    }
    KernelFileDesc& desc = fd_table_[handle];
    if (desc.mode != OpenMode::Read) {
        return ErrorCode::PermissionDenied;
    }
    if (max_bytes == 0) {
        return 0;
    }
    if (out_buf == nullptr) {
        return ErrorCode::InvalidArgument;
    }

    int n = file_engine_.read_bytes(desc.path, desc.offset, out_buf, max_bytes);
    if (n < 0) {
        return n;
    }
    desc.offset += static_cast<size_t>(n);
    // Modeled copy from kernel space to the user buffer.
    tracer_.record_data_copy(static_cast<size_t>(n), "KernelBuffer", "UserBuffer");
    return n;
}

int MonolithicKernel::sys_write(int handle, const uint8_t* in_data, size_t count) {
    // Validate write access, write data, and update the offset and copy trace.
    if (!is_valid_fd(handle)) {
        return ErrorCode::InvalidHandle;
    }
    KernelFileDesc& desc = fd_table_[handle];
    if (desc.mode != OpenMode::Write) {
        return ErrorCode::PermissionDenied;
    }
    if (count == 0) {
        return 0;
    }
    if (in_data == nullptr) {
        return ErrorCode::InvalidArgument;
    }

    // Modeled copy from the user buffer into kernel space.
    int n = file_engine_.write_bytes(desc.path, desc.offset, in_data, count);
    if (n < 0) {
        return n;
    }
    desc.offset += static_cast<size_t>(n);
    tracer_.record_data_copy(static_cast<size_t>(n), "UserBuffer", "KernelBuffer");
    return n;
}

int MonolithicKernel::sys_close(int handle) {
    // Validate and close the file descriptor.
    if (!is_valid_fd(handle)) {
        return ErrorCode::InvalidHandle;
    }
    KernelFileDesc& desc = fd_table_[handle];
    desc.is_open = false;
    desc.path.clear();
    desc.offset = 0;
    desc.mode = OpenMode::Read;
    return ErrorCode::Success;
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
