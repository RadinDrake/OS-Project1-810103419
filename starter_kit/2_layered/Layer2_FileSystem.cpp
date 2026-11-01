#include "Layer2_FileSystem.hpp"

namespace os_sim {
namespace layered {

Layer2_FileSystem::Layer2_FileSystem(Layer1_Drivers& drivers, Tracer& tracer)
    : drivers_(drivers), tracer_(tracer),
      file_engine_(drivers.get_capacity_sectors(),
                   [this](size_t sector, std::array<uint8_t, DISK_BLOCK_SIZE>& b) {
                       tracer_.record_layer_transition(2, 1, "READ_SECTOR");
                       // Strict Layered: Layer 2 delegates to Layer 1 (NEVER directly to Layer 0)
                       return drivers_.driver_read_sector(sector, b);
                   },
                   [this](size_t sector, const std::array<uint8_t, DISK_BLOCK_SIZE>& b) {
                       tracer_.record_layer_transition(2, 1, "WRITE_SECTOR");
                       return drivers_.driver_write_sector(sector, b);
                   }) {}

void Layer2_FileSystem::process_request(ParameterBlock& pb) {
    tracer_.record_domain_switch("UserSpace", "LayeredKernel", "request");
    // TODO
    // Dispatch the requested operation and store its result in pb.status.

    // YOUR CODE HERE
    pb.status = ErrorCode::InvalidArgument;
    tracer_.record_domain_switch("LayeredKernel", "UserSpace", "request_return");
}

int Layer2_FileSystem::handle_open(const std::string& path, OpenMode mode) {
    if (path.empty()) return ErrorCode::InvalidArgument;
    if (mode != OpenMode::Read && mode != OpenMode::Write) return ErrorCode::InvalidArgument;

    if (mode == OpenMode::Read) {
        if (!file_engine_.file_exists(path)) {
            return ErrorCode::NotFound;
        }
    } else if (mode == OpenMode::Write) {
        if (file_engine_.file_exists(path)) {
            file_engine_.truncate_file(path);
        } else {
            if (!file_engine_.create_file(path)) {
                return ErrorCode::NoSpace;
            }
        }
    }

    int handle = allocate_handle();
    if (handle < 0) return ErrorCode::NoSpace;

    Layer2FileHandle& entry = handles_[handle];
    entry.path = path;
    entry.mode = mode;
    entry.offset = 0;
    entry.is_open = true;
    return handle;
}

int Layer2_FileSystem::handle_read(int handle, uint8_t* out_buffer, size_t length) {
    if (!is_valid_handle(handle)) {
        return ErrorCode::InvalidHandle;
    }
    Layer2FileHandle& entry = handles_[handle];
    if (entry.mode == OpenMode::Write) {
        return ErrorCode::PermissionDenied;
    }
    if (length == 0) return 0;
    if (out_buffer == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.read_bytes(entry.path, entry.offset, out_buffer, length);
    if (bytes > 0) {
        entry.offset += bytes;
        tracer_.record_data_copy(bytes, "Layer2_FSBuffer", "Layer3_UserBuffer");
    }
    return bytes;
}

int Layer2_FileSystem::handle_write(int handle, const uint8_t* in_buffer, size_t length) {
    if (!is_valid_handle(handle)) {
        return ErrorCode::InvalidHandle;
    }
    Layer2FileHandle& entry = handles_[handle];
    if (entry.mode == OpenMode::Read) {
        return ErrorCode::PermissionDenied;
    }
    if (length == 0) return 0;
    if (in_buffer == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.write_bytes(entry.path, entry.offset, in_buffer, length);
    if (bytes > 0) {
        entry.offset += bytes;
        tracer_.record_data_copy(bytes, "Layer3_UserBuffer", "Layer2_FSBuffer");
    }
    return bytes;
}

int Layer2_FileSystem::handle_close(int handle) {
    if (!is_valid_handle(handle)) {
        return ErrorCode::InvalidHandle;
    }
    handles_[handle].is_open = false;
    return ErrorCode::Success;
}

int Layer2_FileSystem::handle_console_write(const std::string& text) {
    tracer_.record_layer_transition(2, 1, "WRITE_DISPLAY");
    drivers_.driver_write_display(text);
    return ErrorCode::Success;
}

int Layer2_FileSystem::allocate_handle() {
    for (size_t i = 0; i < handles_.size(); ++i) {
        if (!handles_[i].is_open) return static_cast<int>(i);
    }
    handles_.push_back(Layer2FileHandle{});
    return static_cast<int>(handles_.size() - 1);
}

bool Layer2_FileSystem::is_valid_handle(int handle) const {
    if (handle < 0 || static_cast<size_t>(handle) >= handles_.size()) {
        return false;
    }
    return handles_[handle].is_open;
}

} // namespace layered
} // namespace os_sim
