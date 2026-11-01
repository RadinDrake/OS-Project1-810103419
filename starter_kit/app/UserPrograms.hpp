#pragma once

#include "../platform/OSApi.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace os_sim {

// User-level program: copy_file (Example System Call Sequence).
//
// Notice that copy_file is NOT a system call itself; it is a user-space application
// that orchestrates the 5 fundamental system services provided by OSApi:
//   1. console_write
//   2. open (read mode)
//   3. open (write mode)
//   4. read / write loop
//   5. close
//
// This code demonstrates API Portability:
// It executes unchanged across all 3 OS structural architectures.
inline bool copy_file(OSApi& os,
                      const std::string& source_path,
                      const std::string& destination_path,
                      size_t chunk_size = 64) {
    if (chunk_size == 0 || source_path == destination_path) {
        os.console_write("CopyFile Error: Invalid chunk size or identical paths.");
        return false;
    }
    // 1. Output prompts / status to console
    os.console_write("CopyFile: Initiating copy from '" + source_path + "' to '" + destination_path + "'");

    // 2. Open source file
    int in_fd = os.open(source_path, OpenMode::Read);
    if (in_fd < 0) {
        os.console_write("CopyFile Error: Failed to open source file '" + source_path + "' (Code: " + std::to_string(in_fd) + ")");
        return false;
    }

    // 3. Open/create destination file
    int out_fd = os.open(destination_path, OpenMode::Write);
    if (out_fd < 0) {
        os.console_write("CopyFile Error: Failed to open destination file '" + destination_path + "' (Code: " + std::to_string(out_fd) + ")");
        os.close(in_fd);
        return false;
    }

    // 4. Copy loop: read block, write block until EOF
    size_t total_copied = 0;
    bool success = true;

    while (true) {
        std::vector<uint8_t> buffer;
        int bytes_read = os.read(in_fd, buffer, chunk_size);
        if (bytes_read < 0) {
            os.console_write("CopyFile Error: Read error encountered during copy.");
            success = false;
            break;
        }
        if (bytes_read == 0) {
            // End of File (EOF)
            break;
        }

        int bytes_written = os.write(out_fd, buffer);
        if (bytes_written < 0 || static_cast<size_t>(bytes_written) != buffer.size()) {
            os.console_write("CopyFile Error: Write error or short write encountered.");
            success = false;
            break;
        }

        total_copied += bytes_written;
    }

    // 5. Close handles
    os.close(in_fd);
    os.close(out_fd);

    if (success) {
        os.console_write("CopyFile: Successfully copied " + std::to_string(total_copied) + " bytes.");
    } else {
        os.console_write("CopyFile: Copy aborted with errors.");
    }

    return success;
}

// Helper user-level utility: Create and write text to a file.
inline bool write_test_file(OSApi& os, const std::string& path, const std::string& content) {
    int fd = os.open(path, OpenMode::Write);
    if (fd < 0) {
        return false;
    }
    std::vector<uint8_t> data(content.begin(), content.end());
    int written = os.write(fd, data);
    os.close(fd);
    return written == static_cast<int>(data.size());
}

// Helper user-level utility: Read full text content from a file.
inline std::string read_test_file(OSApi& os, const std::string& path) {
    int fd = os.open(path, OpenMode::Read);
    if (fd < 0) {
        return "";
    }
    std::string result;
    std::vector<uint8_t> buffer;
    while (true) {
        int n = os.read(fd, buffer, 64);
        if (n <= 0) break;
        result.append(reinterpret_cast<const char*>(buffer.data()), n);
    }
    os.close(fd);
    return result;
}

} // namespace os_sim
