#include "ModularOSApi.hpp"

namespace os_sim {
namespace modular {

ModularOSApi::ModularOSApi(ModularKernel& kernel) : kernel_(kernel) {}

int ModularOSApi::open(const std::string& path, OpenMode mode) {
    return kernel_.sys_open(path, mode);
}

int ModularOSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    out_buffer.resize(max_bytes);
    int res = kernel_.sys_read(handle, out_buffer.data(), max_bytes);
    if (res > 0) {
        out_buffer.resize(res);
    } else {
        out_buffer.clear();
    }
    return res;
}

int ModularOSApi::write(int handle, const std::vector<uint8_t>& data) {
    return kernel_.sys_write(handle, data.data(), data.size());
}

int ModularOSApi::close(int handle) {
    return kernel_.sys_close(handle);
}

void ModularOSApi::console_write(const std::string& text) {
    kernel_.sys_console_write(text);
}

std::string ModularOSApi::get_architecture_name() const {
    return "Modular Monolithic (Dynamic Modules)";
}

} // namespace modular
} // namespace os_sim
