#pragma once

#include "ModuleInterface.hpp"
#include "../platform/Tracer.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace os_sim {
namespace modular {

// Modular Monolithic Kernel Core.
// Provides Core Mechanisms:
//  - Module Registration, Loading, and Unloading at runtime.
//  - Lifecycle Safety: strictly rejects unloading a module while it has active resources (open files).
//  - Dispatching system calls to the currently active modules.
class ModularKernel {
public:
    explicit ModularKernel(Tracer& tracer);

    bool load_module(std::shared_ptr<IKernelModule> mod);
    bool unload_module(const std::string& name);
    bool set_active_fs_module(const std::string& name);
    bool set_active_console_module(const std::string& name);

    int sys_open(const std::string& path, OpenMode mode);
    int sys_read(int handle, uint8_t* out_buf, size_t max_bytes);
    int sys_write(int handle, const uint8_t* in_data, size_t count);
    int sys_close(int handle);
    void sys_console_write(const std::string& text);

    IFileSystemModule* get_active_fs_module() const;
    IConsoleModule* get_active_console_module() const;
    bool is_module_loaded(const std::string& name) const;

private:
    Tracer& tracer_;
    std::unordered_map<std::string, std::shared_ptr<IKernelModule>> modules_;
    IFileSystemModule* active_fs_{nullptr};
    IConsoleModule* active_console_{nullptr};
};

} // namespace modular
} // namespace os_sim
