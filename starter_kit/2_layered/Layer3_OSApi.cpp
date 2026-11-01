#include "Layer3_OSApi.hpp"

namespace os_sim {
namespace layered {

Layer3_OSApi::Layer3_OSApi(Layer2_FileSystem& layer2, Tracer& tracer)
    : layer2_(layer2), tracer_(tracer) {}

int Layer3_OSApi::open(const std::string& path, OpenMode mode) {
    // TODO
    // Send a traced open request to Layer 2 using a ParameterBlock.

    // YOUR CODE HERE
    (void)path;
    (void)mode;
    return ErrorCode::NotFound;
}

int Layer3_OSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    // TODO
    // Send a traced read request to Layer 2 and update the output buffer.

    // YOUR CODE HERE
    (void)handle;
    (void)out_buffer;
    (void)max_bytes;
    return ErrorCode::InvalidHandle;
}

int Layer3_OSApi::write(int handle, const std::vector<uint8_t>& data) {
    // TODO
    // Send a traced write request to Layer 2 and return its status.

    // YOUR CODE HERE
    (void)handle;
    (void)data;
    return ErrorCode::InvalidHandle;
}

int Layer3_OSApi::close(int handle) {
    // TODO
    // Send a traced close request to Layer 2 and return its status.

    // YOUR CODE HERE
    (void)handle;
    return ErrorCode::InvalidHandle;
}

void Layer3_OSApi::console_write(const std::string& text) {
    // TODO
    // Send a traced console-output request to Layer 2.

    // YOUR CODE HERE
    (void)text;
}

std::string Layer3_OSApi::get_architecture_name() const {
    return "Strict Layered Approach";
}

} // namespace layered
} // namespace os_sim
