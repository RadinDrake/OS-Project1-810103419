#pragma once

#include "MonolithicKernel.hpp"
#include "../platform/OSApi.hpp"

namespace os_sim {
namespace monolithic {

// User-space Standard Library Wrapper (e.g. glibc wrapper) for Monolithic Kernel.
// Translates high-level API functions to kernel.syscall(...) invocations.
class MonolithicOSApi : public OSApi {
public:
    explicit MonolithicOSApi(MonolithicKernel& kernel);

    int open(const std::string& path, OpenMode mode) override;
    int read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) override;
    int write(int handle, const std::vector<uint8_t>& data) override;
    int close(int handle) override;
    void console_write(const std::string& text) override;

    std::string get_architecture_name() const override;

private:
    MonolithicKernel& kernel_;
};

} // namespace monolithic
} // namespace os_sim
