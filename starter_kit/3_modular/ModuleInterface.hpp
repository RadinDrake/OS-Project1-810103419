#pragma once

#include "../platform/OSApi.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace os_sim {
namespace modular {

class IFileSystemModule;
class IConsoleModule;

// Base interface for all Loadable Kernel Modules (LKMs).
class IKernelModule {
public:
    virtual ~IKernelModule() = default;
    virtual std::string get_name() const = 0;
    virtual std::string get_type() const = 0; // "fs" or "console"
    virtual bool on_load() = 0;
    virtual void on_unload() = 0;

    // Convenient downcasting helpers (eliminates need for complex dynamic_pointer_cast)
    virtual IFileSystemModule* as_fs() { return nullptr; }
    virtual IConsoleModule* as_console() { return nullptr; }
};

// File System Module Interface.
// Allows pluggable file system policies (Separation of Mechanism and Policy).
class IFileSystemModule : public IKernelModule {
public:
    std::string get_type() const override { return "fs"; }
    IFileSystemModule* as_fs() override { return this; }

    virtual int open_file(const std::string& path, OpenMode mode) = 0;
    virtual int read_file(int handle, uint8_t* out_buf, size_t max_bytes) = 0;
    virtual int write_file(int handle, const uint8_t* in_data, size_t count) = 0;
    virtual int close_file(int handle) = 0;

    // Number of active open file descriptors.
    // Crucial for Module Lifecycle Safety: cannot unload while active_handle_count > 0.
    virtual size_t get_active_handle_count() const = 0;
};

// Console Device Module Interface.
class IConsoleModule : public IKernelModule {
public:
    std::string get_type() const override { return "console"; }
    IConsoleModule* as_console() override { return this; }

    virtual void write_console(const std::string& text) = 0;
};

} // namespace modular
} // namespace os_sim
