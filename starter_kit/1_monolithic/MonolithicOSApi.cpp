#include "MonolithicOSApi.hpp"

namespace os_sim {
namespace monolithic {

MonolithicOSApi::MonolithicOSApi(MonolithicKernel& kernel) : kernel_(kernel) {}

int MonolithicOSApi::open(const std::string& path, OpenMode mode) {
    // TODO
    // Forward the open request through the syscall dispatcher.

    // YOUR CODE HERE
    (void)path;
    (void)mode;
    return ErrorCode::NotFound;
}

int MonolithicOSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    // TODO
    // Forward the read request and resize the output buffer to the result.

    // YOUR CODE HERE
    (void)handle;
    (void)out_buffer;
    (void)max_bytes;
    return ErrorCode::InvalidHandle;
}

int MonolithicOSApi::write(int handle, const std::vector<uint8_t>& data) {
    // TODO
    // Forward the write request through the syscall dispatcher.

    // YOUR CODE HERE
    (void)handle;
    (void)data;
    return ErrorCode::InvalidHandle;
}

int MonolithicOSApi::close(int handle) {
    // TODO
    // Forward the close request through the syscall dispatcher.

    // YOUR CODE HERE
    (void)handle;
    return ErrorCode::InvalidHandle;
}

void MonolithicOSApi::console_write(const std::string& text) {
    // TODO
    // Forward console output through the syscall dispatcher.

    // YOUR CODE HERE
    (void)text;
}

std::string MonolithicOSApi::get_architecture_name() const {
    return "Linux-inspired Monolithic";
}

} // namespace monolithic
} // namespace os_sim
