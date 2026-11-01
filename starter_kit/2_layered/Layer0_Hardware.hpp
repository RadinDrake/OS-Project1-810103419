#pragma once

#include "../platform/VirtualHardware.hpp"
#include "../platform/Tracer.hpp"

namespace os_sim {
namespace layered {

// Layer 0: Hardware abstraction layer.
// Encapsulates raw physical storage and raw display device.
// Strict Rule: Layer 0 has NO knowledge of higher layers (Drivers, FS, or User API).
class Layer0_Hardware {
public:
    Layer0_Hardware(VirtualDisk& disk, IVirtualConsoleDevice& console, Tracer& tracer);

    bool raw_read_block(size_t block_id, std::array<uint8_t, DISK_BLOCK_SIZE>& buf);
    bool raw_write_block(size_t block_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& buf);
    void raw_write_console(const std::string& text);
    size_t get_total_blocks() const;

private:
    VirtualDisk& disk_;
    IVirtualConsoleDevice& console_;
    Tracer& tracer_;
};

} // namespace layered
} // namespace os_sim
