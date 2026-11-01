#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace os_sim {

// File open access modes.
enum class OpenMode {
    Read,       // Open existing file for reading
    Write,      // Open file for writing (truncate if exists, create if not)
    Append,     // Reserved; not supported in this exercise
    ReadWrite   // Reserved; not supported in this exercise
};

// Standardized error codes for OSApi operations.
namespace ErrorCode {
    constexpr int Success = 0;
    constexpr int NotFound = -1;
    constexpr int InvalidHandle = -2;
    constexpr int PermissionDenied = -3;
    constexpr int NoSpace = -4;
    constexpr int ServiceUnavailable = -5;
    constexpr int Busy = -6;
    constexpr int InvalidArgument = -7;
}

// Common Abstract Operating System API interface (API Portability).
// User programs (e.g. copy_file) are written exclusively against this interface
// and execute unchanged across all 3 OS structural architectures.
class OSApi {
public:
    virtual ~OSApi() = default;

    // Open a file at the given path with specified mode.
    // path: File path.
    // mode: Access mode (Read, Write, etc.).
    // Returns: Non-negative integer file handle on success, negative ErrorCode on failure.
    virtual int open(const std::string& path, OpenMode mode) = 0;

    // Read up to max_bytes from an open file handle into out_buffer.
    // handle: File handle returned by open().
    // out_buffer: Output byte vector filled with read data.
    // max_bytes: Maximum bytes to read in this call.
    // Returns: Number of bytes actually read (0 on EOF), or negative ErrorCode on failure.
    virtual int read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) = 0;

    // Write data bytes to an open file handle.
    // handle: File handle returned by open().
    // data: Byte vector containing data to write.
    // Returns: Number of bytes written, or negative ErrorCode on failure.
    virtual int write(int handle, const std::vector<uint8_t>& data) = 0;

    // Close an open file handle.
    // handle: File handle returned by open().
    // Returns: 0 on success, negative ErrorCode on failure.
    virtual int close(int handle) = 0;

    // Write text to the system console.
    // text: Text string to display.
    virtual void console_write(const std::string& text) = 0;

    // Return the architectural name of this OS implementation.
    virtual std::string get_architecture_name() const = 0;
};

// Helper functions for easy conversion between std::string and byte vectors.
inline std::vector<uint8_t> string_to_bytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

inline std::string bytes_to_string(const std::vector<uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

} // namespace os_sim
