#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace os_sim {

constexpr size_t DISK_BLOCK_SIZE = 64; // Raw 64-byte disk blocks as defined in project requirements

// Simulated raw disk hardware.
// Represents Layer 0 physical storage.
// Works strictly with block IDs (no concept of files, directories, or byte streams).
class VirtualDisk {
public:
    explicit VirtualDisk(size_t total_blocks = 512)
        : total_blocks_(total_blocks),
          storage_(total_blocks, std::array<uint8_t, DISK_BLOCK_SIZE>{}) {}

    size_t get_total_blocks() const {
        return total_blocks_;
    }

    bool read_block(size_t block_id, std::array<uint8_t, DISK_BLOCK_SIZE>& out_buffer) {
        if (block_id >= total_blocks_) {
            return false;
        }
        out_buffer = storage_[block_id];
        read_ops_++;
        return true;
    }

    bool write_block(size_t block_id, const std::array<uint8_t, DISK_BLOCK_SIZE>& in_buffer) {
        if (block_id >= total_blocks_) {
            return false;
        }
        storage_[block_id] = in_buffer;
        write_ops_++;
        return true;
    }

    void reset() {
        for (auto& block : storage_) {
            block.fill(0);
        }
        read_ops_ = 0;
        write_ops_ = 0;
    }

    size_t get_read_ops() const { return read_ops_; }
    size_t get_write_ops() const { return write_ops_; }

private:
    size_t total_blocks_;
    std::vector<std::array<uint8_t, DISK_BLOCK_SIZE>> storage_;
    size_t read_ops_{0};
    size_t write_ops_{0};
};

// Base interface for console output devices.
class IVirtualConsoleDevice {
public:
    virtual ~IVirtualConsoleDevice() = default;
    virtual void write(const std::string& text) = 0;
    virtual std::vector<std::string> get_history() const = 0;
    virtual void clear() = 0;
};

// Simulated console output device.
// Captures written text lines into history and optionally mirrors to stdout.
class VirtualConsole : public IVirtualConsoleDevice {
public:
    explicit VirtualConsole(bool echo_stdout = false) : echo_stdout_(echo_stdout) {}

    void write(const std::string& text) override {
        history_.push_back(text);
        if (echo_stdout_) {
            std::cout << "[CONSOLE OUTPUT] " << text << std::endl;
        }
    }

    std::vector<std::string> get_history() const override {
        return history_;
    }

    void clear() override {
        history_.clear();
    }

    std::string get_last_line() const {
        return history_.empty() ? "" : history_.back();
    }

private:
    bool echo_stdout_{false};
    std::vector<std::string> history_;
};

// Null device (discards all output).
// Used in Test 4 to evaluate architectural extensibility.
class NullConsoleDevice : public IVirtualConsoleDevice {
public:
    void write(const std::string& /*text*/) override {
        discard_count_++;
    }

    std::vector<std::string> get_history() const override {
        return {};
    }

    void clear() override {
        discard_count_ = 0;
    }

    size_t get_discard_count() const {
        return discard_count_;
    }

private:
    size_t discard_count_{0};
};

} // namespace os_sim
