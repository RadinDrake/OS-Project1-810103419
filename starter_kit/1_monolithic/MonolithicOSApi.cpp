#include "MonolithicOSApi.hpp"

namespace os_sim {
namespace monolithic {

MonolithicOSApi::MonolithicOSApi(MonolithicKernel& kernel) : kernel_(kernel) {}

int MonolithicOSApi::open(const std::string& path, OpenMode mode) {
    // Forward the open request through the syscall dispatcher.
    SyscallArgs args;
    args.path = path;
    args.mode = mode;
    return kernel_.syscall(SYS_OPEN, args);
}

int MonolithicOSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    // Forward the read request and resize the output buffer to the result.
    out_buffer.assign(max_bytes, 0);

    SyscallArgs args;
    args.handle = handle;
    args.buffer = out_buffer.data();
    args.count = max_bytes;

    int result = kernel_.syscall(SYS_READ, args);
    out_buffer.resize(result > 0 ? static_cast<size_t>(result) : 0);
    return result;
}

int MonolithicOSApi::write(int handle, const std::vector<uint8_t>& data) {
    // Forward the write request through the syscall dispatcher.
    SyscallArgs args;
    args.handle = handle;
    args.write_buffer = data.data();
    args.count = data.size();
    return kernel_.syscall(SYS_WRITE, args);
}

int MonolithicOSApi::close(int handle) {
    // Forward the close request through the syscall dispatcher.
    SyscallArgs args;
    args.handle = handle;
    return kernel_.syscall(SYS_CLOSE, args);
}

void MonolithicOSApi::console_write(const std::string& text) {
    // Forward console output through the syscall dispatcher.
    SyscallArgs args;
    args.text = text;
    kernel_.syscall(SYS_CONSOLE_WRITE, args);
}

std::string MonolithicOSApi::get_architecture_name() const {
    return "Linux-inspired Monolithic";
}

} // namespace monolithic
} // namespace os_sim
