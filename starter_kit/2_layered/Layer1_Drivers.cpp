#include "Layer1_Drivers.hpp"

namespace os_sim {
namespace layered {

Layer1_Drivers::Layer1_Drivers(Layer0_Hardware& hw, Tracer& tracer)
    : hw_(hw), tracer_(tracer) {}

bool Layer1_Drivers::driver_read_sector(size_t sector_id, std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    // Read a sector through Layer 0 and trace the transition and data copy.
    tracer_.record_layer_transition(1, 0, "READ_SECTOR");
    bool ok = hw_.raw_read_block(sector_id, buf);
    if (ok) {
        tracer_.record_data_copy(DISK_BLOCK_SIZE, "Layer0_DiskBlock", "Layer1_DriverBuffer");
    }
    return ok;
}

bool Layer1_Drivers::driver_write_sector(size_t sector_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    // Write a sector through Layer 0 and trace the transition and data copy.
    tracer_.record_layer_transition(1, 0, "WRITE_SECTOR");
    bool ok = hw_.raw_write_block(sector_id, buf);
    if (ok) {
        tracer_.record_data_copy(DISK_BLOCK_SIZE, "Layer1_DriverBuffer", "Layer0_DiskBlock");
    }
    return ok;
}

void Layer1_Drivers::driver_write_display(const std::string& text) {
    // Forward display output to Layer 0 and trace the transition.
    tracer_.record_layer_transition(1, 0, "WRITE_DISPLAY");
    hw_.raw_write_console(text);
}

size_t Layer1_Drivers::get_capacity_sectors() const {
    return hw_.get_total_blocks();
}

} // namespace layered
} // namespace os_sim
