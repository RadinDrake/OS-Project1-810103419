#pragma once

#include "Layer2_FileSystem.hpp"
#include "LayerTypes.hpp"
#include "../platform/OSApi.hpp"
#include "../platform/Tracer.hpp"

namespace os_sim {
namespace layered {

// Layer 3: Application Interface / User System Calls.
// Interacts ONLY with Layer 2 (File System & Services).
// Packages parameters into a Parameter Block and passes to Layer 2.
class Layer3_OSApi : public OSApi {
public:
    Layer3_OSApi(Layer2_FileSystem& layer2, Tracer& tracer);

    int open(const std::string& path, OpenMode mode) override;
    int read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) override;
    int write(int handle, const std::vector<uint8_t>& data) override;
    int close(int handle) override;
    void console_write(const std::string& text) override;
    std::string get_architecture_name() const override;

private:
    Layer2_FileSystem& layer2_;
    Tracer& tracer_;
};

} // namespace layered
} // namespace os_sim
