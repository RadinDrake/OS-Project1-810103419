#pragma once

#include "Layer1_Drivers.hpp"
#include "LayerTypes.hpp"
#include "../platform/SimpleFileEngine.hpp"
#include "../platform/Tracer.hpp"
#include "../platform/OSApi.hpp"
#include <string>
#include <vector>

namespace os_sim {
namespace layered {

struct Layer2FileHandle {
    std::string path;
    OpenMode mode{OpenMode::Read};
    size_t offset{0};
    bool is_open{false};
};

// Layer 2: File System & System Services.
// Interacts ONLY with Layer 1 (Drivers).
// Processes Parameter Blocks from Layer 3.
class Layer2_FileSystem {
public:
    Layer2_FileSystem(Layer1_Drivers& drivers, Tracer& tracer);

    void process_request(ParameterBlock& pb);

private:
    int handle_open(const std::string& path, OpenMode mode);
    int handle_read(int handle, uint8_t* out_buffer, size_t length);
    int handle_write(int handle, const uint8_t* in_buffer, size_t length);
    int handle_close(int handle);
    int handle_console_write(const std::string& text);

    int allocate_handle();
    bool is_valid_handle(int handle) const;

    Layer1_Drivers& drivers_;
    Tracer& tracer_;
    SimpleFileEngine file_engine_;
    std::vector<Layer2FileHandle> handles_;
};

} // namespace layered
} // namespace os_sim
