#include "Layer3_OSApi.hpp"

namespace os_sim {
namespace layered {

Layer3_OSApi::Layer3_OSApi(Layer2_FileSystem& layer2, Tracer& tracer)
    : layer2_(layer2), tracer_(tracer) {}

namespace {
// Declared (illustrative) depth of a request: Layer 3 -> 2 -> 1 -> 0.
constexpr size_t LAYERED_CALL_DEPTH = 3;
}

// Trace the Layer 3 -> Layer 2 transition, hand the block down, return its status.
// File-local helper so Layer3_OSApi.hpp stays unchanged.
static int submit_block(Layer2_FileSystem& layer2, Tracer& tracer, ParameterBlock& pb, const char* op_name) {
    tracer.record_call_depth(LAYERED_CALL_DEPTH);
    tracer.record_layer_transition(3, 2, op_name);
    layer2.process_request(pb);
    return pb.status;
}

int Layer3_OSApi::open(const std::string& path, OpenMode mode) {
    // Send a traced open request to Layer 2 using a ParameterBlock.
    ParameterBlock pb;
    pb.op = LayerOp::Open;
    pb.path = path;
    pb.mode = mode;
    return submit_block(layer2_, tracer_, pb, "OPEN");
}

int Layer3_OSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    // Send a traced read request to Layer 2 and update the output buffer.
    out_buffer.assign(max_bytes, 0);

    ParameterBlock pb;
    pb.op = LayerOp::Read;
    pb.handle = handle;
    pb.buffer = out_buffer.data();
    pb.length = max_bytes;

    int result = submit_block(layer2_, tracer_, pb, "READ");
    out_buffer.resize(result > 0 ? static_cast<size_t>(result) : 0);
    return result;
}

int Layer3_OSApi::write(int handle, const std::vector<uint8_t>& data) {
    // Send a traced write request to Layer 2 and return its status.
    ParameterBlock pb;
    pb.op = LayerOp::Write;
    pb.handle = handle;
    pb.write_buffer = data.data();
    pb.length = data.size();
    return submit_block(layer2_, tracer_, pb, "WRITE");
}

int Layer3_OSApi::close(int handle) {
    // Send a traced close request to Layer 2 and return its status.
    ParameterBlock pb;
    pb.op = LayerOp::Close;
    pb.handle = handle;
    return submit_block(layer2_, tracer_, pb, "CLOSE");
}

void Layer3_OSApi::console_write(const std::string& text) {
    // Send a traced console-output request to Layer 2.
    ParameterBlock pb;
    pb.op = LayerOp::ConsoleWrite;
    pb.text = text;
    submit_block(layer2_, tracer_, pb, "CONSOLE_WRITE");
}

std::string Layer3_OSApi::get_architecture_name() const {
    return "Strict Layered Approach";
}

} // namespace layered
} // namespace os_sim
