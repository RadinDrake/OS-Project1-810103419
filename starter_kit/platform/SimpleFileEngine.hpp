#pragma once

#include "VirtualHardware.hpp"
#include "OSApi.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace os_sim {

// Metadata tracking for a file.
// Kept in memory to allow students to focus on OS structure without
// needing to implement complex on-disk directory structures or B-trees.
struct FileEntry {
    std::string name;
    size_t size{0};
    std::vector<size_t> block_ids;
};

// Lightweight file management engine.
// Maps files and byte streams to raw 64-byte disk blocks.
// Interacts with block storage through callbacks, allowing it to be driven
// by direct hardware access (Monolithic), through a driver layer (Layered),
// or dynamic loadable modules (Modular Monolithic).
class SimpleFileEngine {
public:
    using ReadBlockFn = std::function<bool(size_t block_id, std::array<uint8_t, DISK_BLOCK_SIZE>&)>;
    using WriteBlockFn = std::function<bool(size_t block_id, const std::array<uint8_t, DISK_BLOCK_SIZE>&)>;

    SimpleFileEngine(size_t total_blocks, ReadBlockFn read_fn, WriteBlockFn write_fn)
        : total_blocks_(total_blocks),
          read_block_(std::move(read_fn)),
          write_block_(std::move(write_fn)),
          block_free_(total_blocks, true) {
        // Reserve block 0 for superblock / metadata simulation
        if (total_blocks_ > 0) {
            block_free_[0] = false;
        }
    }

    bool file_exists(const std::string& path) const {
        return files_.find(path) != files_.end();
    }

    bool create_file(const std::string& path) {
        if (file_exists(path)) {
            return false;
        }
        files_[path] = FileEntry{path, 0, {}};
        return true;
    }

    bool remove_file(const std::string& path) {
        auto it = files_.find(path);
        if (it == files_.end()) {
            return false;
        }
        for (size_t b_id : it->second.block_ids) {
            if (b_id < total_blocks_) {
                block_free_[b_id] = true;
            }
        }
        files_.erase(it);
        return true;
    }

    size_t get_file_size(const std::string& path) const {
        auto it = files_.find(path);
        if (it == files_.end()) {
            return 0;
        }
        return it->second.size;
    }

    void truncate_file(const std::string& path) {
        auto it = files_.find(path);
        if (it == files_.end()) {
            return;
        }
        for (size_t b_id : it->second.block_ids) {
            if (b_id < total_blocks_) {
                block_free_[b_id] = true;
            }
        }
        it->second.block_ids.clear();
        it->second.size = 0;
    }

    // Read data from file at specified offset into buffer.
    // Translates offset/length into raw block reads.
    int read_bytes(const std::string& path, size_t offset, uint8_t* out_buffer, size_t length) {
        auto it = files_.find(path);
        if (it == files_.end()) {
            return ErrorCode::NotFound;
        }
        const FileEntry& entry = it->second;
        if (offset >= entry.size) {
            return 0; // EOF
        }

        size_t bytes_to_read = std::min(length, entry.size - offset);
        size_t bytes_read = 0;

        while (bytes_read < bytes_to_read) {
            size_t current_offset = offset + bytes_read;
            size_t block_index = current_offset / DISK_BLOCK_SIZE;
            size_t block_offset = current_offset % DISK_BLOCK_SIZE;
            size_t chunk = std::min(bytes_to_read - bytes_read, DISK_BLOCK_SIZE - block_offset);

            if (block_index >= entry.block_ids.size()) {
                break;
            }
            size_t phys_block = entry.block_ids[block_index];

            std::array<uint8_t, DISK_BLOCK_SIZE> temp_block{};
            if (!read_block_(phys_block, temp_block)) {
                return ErrorCode::ServiceUnavailable;
            }

            std::memcpy(out_buffer + bytes_read, temp_block.data() + block_offset, chunk);
            bytes_read += chunk;
        }

        return static_cast<int>(bytes_read);
    }

    // Write data bytes to file at specified offset.
    // Allocates new blocks as needed and writes blocks via the block writer.
    int write_bytes(const std::string& path, size_t offset, const uint8_t* in_buffer, size_t length) {
        auto it = files_.find(path);
        if (it == files_.end()) {
            if (!create_file(path)) {
                return ErrorCode::NoSpace;
            }
            it = files_.find(path);
        }
        FileEntry& entry = it->second;

        size_t required_size = offset + length;
        size_t required_blocks = (required_size + DISK_BLOCK_SIZE - 1) / DISK_BLOCK_SIZE;

        // Check capacity before reserving any blocks.
        if (required_blocks > entry.block_ids.size()) {
            size_t extra_blocks = required_blocks - entry.block_ids.size();
            if (extra_blocks > get_free_block_count()) {
                return ErrorCode::NoSpace;
            }
        }

        while (entry.block_ids.size() < required_blocks) {
            size_t free_block = allocate_free_block();
            if (free_block == 0) {
                return ErrorCode::NoSpace;
            }
            entry.block_ids.push_back(free_block);
        }

        size_t bytes_written = 0;
        while (bytes_written < length) {
            size_t current_offset = offset + bytes_written;
            size_t block_index = current_offset / DISK_BLOCK_SIZE;
            size_t block_offset = current_offset % DISK_BLOCK_SIZE;
            size_t chunk = std::min(length - bytes_written, DISK_BLOCK_SIZE - block_offset);

            size_t phys_block = entry.block_ids[block_index];
            std::array<uint8_t, DISK_BLOCK_SIZE> temp_block{};

            // If we are doing a partial block write, read existing block first
            if (block_offset != 0 || chunk < DISK_BLOCK_SIZE) {
                if (!read_block_(phys_block, temp_block)) {
                    return ErrorCode::ServiceUnavailable;
                }
            }

            std::memcpy(temp_block.data() + block_offset, in_buffer + bytes_written, chunk);
            if (!write_block_(phys_block, temp_block)) {
                return ErrorCode::ServiceUnavailable;
            }

            bytes_written += chunk;
        }

        if (offset + bytes_written > entry.size) {
            entry.size = offset + bytes_written;
        }

        return static_cast<int>(bytes_written);
    }

    std::vector<std::string> list_files() const {
        std::vector<std::string> names;
        names.reserve(files_.size());
        for (const auto& [name, entry] : files_) {
            names.push_back(name);
        }
        return names;
    }

    size_t get_free_block_count() const {
        size_t count = 0;
        for (bool is_free : block_free_) {
            if (is_free) count++;
        }
        return count;
    }

private:
    size_t allocate_free_block() {
        for (size_t i = 1; i < total_blocks_; ++i) {
            if (block_free_[i]) {
                block_free_[i] = false;
                return i;
            }
        }
        return 0; // 0 indicates out of blocks
    }

    size_t total_blocks_;
    ReadBlockFn read_block_;
    WriteBlockFn write_block_;
    std::vector<bool> block_free_;
    std::unordered_map<std::string, FileEntry> files_;
};

} // namespace os_sim
