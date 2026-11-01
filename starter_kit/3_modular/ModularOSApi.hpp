#pragma once

#include "ModularKernel.hpp"
#include "../platform/OSApi.hpp"

namespace os_sim {
namespace modular {

// User-Space API implementation for Modular Monolithic OS.
// Forwards requests into the ModularKernel, which routes to dynamically loaded modules.
class ModularOSApi : public OSApi {
public:
    explicit ModularOSApi(ModularKernel& kernel);

    int open(const std::string& path, OpenMode mode) override;
    int read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) override;
    int write(int handle, const std::vector<uint8_t>& data) override;
    int close(int handle) override;
    void console_write(const std::string& text) override;
    std::string get_architecture_name() const override;

private:
    ModularKernel& kernel_;
};

} // namespace modular
} // namespace os_sim
