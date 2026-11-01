#pragma once

#include "Layer0_Hardware.hpp"
#include "../platform/Tracer.hpp"
#include <string>
#include <array>
#include <cstdint>

namespace os_sim {
namespace layered {

// Layer 1: Device Drivers.
// Strict Layering: Interacts ONLY with Layer 0.
// Knowledge Separation: Knows sector IDs, but NEVER filenames or directories.
class Layer1_Drivers {
public:
    Layer1_Drivers(Layer0_Hardware& hw, Tracer& tracer);

    bool driver_read_sector(size_t sector_id, std::array<uint8_t, DISK_BLOCK_SIZE>& buf);
    bool driver_write_sector(size_t sector_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& buf);
    void driver_write_display(const std::string& text);
    size_t get_capacity_sectors() const;

private:
    Layer0_Hardware& hw_;
    Tracer& tracer_;
};

} // namespace layered
} // namespace os_sim
