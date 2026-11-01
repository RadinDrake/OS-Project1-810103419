#pragma once

#include "../platform/OSApi.hpp"
#include <cstdint>
#include <cstddef>
#include <string>

namespace os_sim {
namespace layered {

// Operation types passed within the Parameter Block.
enum class LayerOp {
    Open,
    Read,
    Write,
    Close,
    ConsoleWrite
};

// Parameter Block structure (Passing parameters in a block/table in memory).
//
// In strict layered architectures and traditional operating systems:
// When parameters exceed register count, or when crossing layer boundaries cleanly,
// arguments are packaged into a memory block and the pointer is passed across the boundary.
struct ParameterBlock {
    LayerOp op;
    std::string path;
    OpenMode mode{OpenMode::Read};
    int handle{-1};
    uint8_t* buffer{nullptr};
    const uint8_t* write_buffer{nullptr};
    size_t length{0};
    std::string text;
    int status{ErrorCode::Success}; // Output / Return code populated by receiving layer
};

} // namespace layered
} // namespace os_sim
