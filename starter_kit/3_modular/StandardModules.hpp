#pragma once

#include "ModuleInterface.hpp"
#include "../platform/VirtualHardware.hpp"
#include "../platform/SimpleFileEngine.hpp"
#include <unordered_map>
#include <string>
#include <vector>

namespace os_sim {
namespace modular {

struct ModularHandleEntry {
    std::string path;
    OpenMode mode{OpenMode::Read};
    size_t offset{0};
    bool is_open{false};
};

// Standard Read-Write File System Module.
class StandardFSModule : public IFileSystemModule {
public:
    explicit StandardFSModule(VirtualDisk& disk, const std::string& name = "standard_fs");

    std::string get_name() const override;
    bool on_load() override;
    void on_unload() override;

    int open_file(const std::string& path, OpenMode mode) override;
    int read_file(int handle, uint8_t* out_buf, size_t max_bytes) override;
    int write_file(int handle, const uint8_t* in_data, size_t count) override;
    int close_file(int handle) override;

    size_t get_active_handle_count() const override;
    SimpleFileEngine& get_file_engine();

private:
    int allocate_handle();
    bool is_valid_handle(int handle) const;

    VirtualDisk& disk_;
    std::string name_;
    SimpleFileEngine file_engine_;
    bool is_loaded_{false};
    size_t active_handles_{0};
    std::vector<ModularHandleEntry> handles_;
};

// Policy Module: Read-Only File System Module (Policy vs Mechanism).
class ReadOnlyFSModule : public IFileSystemModule {
public:
    explicit ReadOnlyFSModule(SimpleFileEngine& shared_engine, const std::string& name = "readonly_fs");

    std::string get_name() const override;
    bool on_load() override;
    void on_unload() override;

    int open_file(const std::string& path, OpenMode mode) override;
    int read_file(int handle, uint8_t* out_buf, size_t max_bytes) override;
    int write_file(int handle, const uint8_t* in_data, size_t count) override;
    int close_file(int handle) override;

    size_t get_active_handle_count() const override;

private:
    int allocate_handle();
    bool is_valid_handle(int handle) const;

    SimpleFileEngine& file_engine_;
    std::string name_;
    bool is_loaded_{false};
    size_t active_handles_{0};
    std::vector<ModularHandleEntry> handles_;
};

// Standard Console Module.
class StandardConsoleModule : public IConsoleModule {
public:
    explicit StandardConsoleModule(IVirtualConsoleDevice& console, const std::string& name = "standard_console");

    std::string get_name() const override;
    bool on_load() override;
    void on_unload() override;
    void write_console(const std::string& text) override;

private:
    IVirtualConsoleDevice& console_;
    std::string name_;
};

// Null Console Module (Test 4: Extensibility).
class NullConsoleModule : public IConsoleModule {
public:
    explicit NullConsoleModule(const std::string& name = "null_console");

    std::string get_name() const override;
    bool on_load() override;
    void on_unload() override;
    void write_console(const std::string& text) override;
    size_t get_discarded_count() const;

private:
    std::string name_;
    size_t discarded_count_{0};
};

} // namespace modular
} // namespace os_sim
