#include "Layer0_Hardware.hpp"

namespace os_sim {
namespace layered {

Layer0_Hardware::Layer0_Hardware(VirtualDisk& disk, IVirtualConsoleDevice& console, Tracer& tracer)
    : disk_(disk), console_(console), tracer_(tracer) {}

bool Layer0_Hardware::raw_read_block(size_t block_id, std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    return disk_.read_block(block_id, buf);
}

bool Layer0_Hardware::raw_write_block(size_t block_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& buf) {
    return disk_.write_block(block_id, buf);
}

void Layer0_Hardware::raw_write_console(const std::string& text) {
    console_.write(text);
}

size_t Layer0_Hardware::get_total_blocks() const {
    return disk_.get_total_blocks();
}

} // namespace layered
} // namespace os_sim
