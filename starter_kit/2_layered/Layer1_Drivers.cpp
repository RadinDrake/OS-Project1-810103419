#include "Layer1_Drivers.hpp"

namespace os_sim {
namespace layered {

Layer1_Drivers::Layer1_Drivers(Layer0_Hardware& hw, Tracer& tracer)
    : hw_(hw), tracer_(tracer) {}

bool Layer1_Drivers::driver_read_sector(size_t sector_id, std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    // TODO
    // Read a sector through Layer 0 and trace the transition and data copy.

    // YOUR CODE HERE
    (void)sector_id;
    (void)buf;
    return false;
}

bool Layer1_Drivers::driver_write_sector(size_t sector_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    // TODO
    // Write a sector through Layer 0 and trace the transition and data copy.

    // YOUR CODE HERE
    (void)sector_id;
    (void)buf;
    return false;
}

void Layer1_Drivers::driver_write_display(const std::string& text) {
    // TODO
    // Forward display output to Layer 0 and trace the transition.

    // YOUR CODE HERE
    (void)text;
}

size_t Layer1_Drivers::get_capacity_sectors() const {
    return hw_.get_total_blocks();
}

} // namespace layered
} // namespace os_sim
