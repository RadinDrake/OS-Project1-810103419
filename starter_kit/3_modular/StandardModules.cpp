#include "StandardModules.hpp"

namespace os_sim {
namespace modular {

// ============================================================================
// StandardFSModule Implementation
// ============================================================================

StandardFSModule::StandardFSModule(VirtualDisk& disk, const std::string& name)
    : disk_(disk), name_(name),
      file_engine_(disk.get_total_blocks(),
                   [&disk](size_t id, std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.read_block(id, b); },
                   [&disk](size_t id, const std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.write_block(id, b); }) {}

std::string StandardFSModule::get_name() const { return name_; }

bool StandardFSModule::on_load() {
    is_loaded_ = true;
    return true;
}

void StandardFSModule::on_unload() {
    is_loaded_ = false;
    handles_.clear();
    active_handles_ = 0;
}

int StandardFSModule::open_file(const std::string& path, OpenMode mode) {
    if (!is_loaded_ || path.empty()) return ErrorCode::InvalidArgument;
    if (mode != OpenMode::Read && mode != OpenMode::Write) return ErrorCode::InvalidArgument;

    if (mode == OpenMode::Read) {
        if (!file_engine_.file_exists(path)) return ErrorCode::NotFound;
    } else if (mode == OpenMode::Write) {
        if (file_engine_.file_exists(path)) {
            file_engine_.truncate_file(path);
        } else {
            if (!file_engine_.create_file(path)) return ErrorCode::NoSpace;
        }
    }

    int h = allocate_handle();
    if (h < 0) return ErrorCode::NoSpace;
    ModularHandleEntry& entry = handles_[h];
    entry.path = path;
    entry.mode = mode;
    entry.offset = 0;
    entry.is_open = true;
    active_handles_++;
    return h;
}

int StandardFSModule::read_file(int handle, uint8_t* out_buf, size_t max_bytes) {
    if (!is_valid_handle(handle)) return ErrorCode::InvalidHandle;
    ModularHandleEntry& entry = handles_[handle];
    if (entry.mode == OpenMode::Write) return ErrorCode::PermissionDenied;
    if (max_bytes == 0) return 0;
    if (out_buf == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.read_bytes(entry.path, entry.offset, out_buf, max_bytes);
    if (bytes > 0) entry.offset += bytes;
    return bytes;
}

int StandardFSModule::write_file(int handle, const uint8_t* in_data, size_t count) {
    if (!is_valid_handle(handle)) return ErrorCode::InvalidHandle;
    ModularHandleEntry& entry = handles_[handle];
    if (entry.mode == OpenMode::Read) return ErrorCode::PermissionDenied;
    if (count == 0) return 0;
    if (in_data == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.write_bytes(entry.path, entry.offset, in_data, count);
    if (bytes > 0) entry.offset += bytes;
    return bytes;
}

int StandardFSModule::close_file(int handle) {
    if (!is_valid_handle(handle)) return ErrorCode::InvalidHandle;
    handles_[handle].is_open = false;
    if (active_handles_ > 0) active_handles_--;
    return ErrorCode::Success;
}

size_t StandardFSModule::get_active_handle_count() const {
    return active_handles_;
}

SimpleFileEngine& StandardFSModule::get_file_engine() {
    return file_engine_;
}

int StandardFSModule::allocate_handle() {
    for (size_t i = 0; i < handles_.size(); ++i) {
        if (!handles_[i].is_open) return static_cast<int>(i);
    }
    handles_.push_back(ModularHandleEntry{});
    return static_cast<int>(handles_.size() - 1);
}

bool StandardFSModule::is_valid_handle(int handle) const {
    if (handle < 0 || static_cast<size_t>(handle) >= handles_.size()) return false;
    return handles_[handle].is_open;
}

// ============================================================================
// ReadOnlyFSModule Implementation (Policy Module)
// ============================================================================

ReadOnlyFSModule::ReadOnlyFSModule(SimpleFileEngine& shared_engine, const std::string& name)
    : file_engine_(shared_engine), name_(name) {}

std::string ReadOnlyFSModule::get_name() const { return name_; }

bool ReadOnlyFSModule::on_load() {
    is_loaded_ = true;
    return true;
}

void ReadOnlyFSModule::on_unload() {
    is_loaded_ = false;
    handles_.clear();
    active_handles_ = 0;
}

int ReadOnlyFSModule::open_file(const std::string& path, OpenMode mode) {
    // Allow read-only access to existing files and track opened handles.
    if (!is_loaded_ || path.empty()) return ErrorCode::InvalidArgument;
    if (mode != OpenMode::Read) return ErrorCode::PermissionDenied;
    if (!file_engine_.file_exists(path)) return ErrorCode::NotFound;

    int h = allocate_handle();
    if (h < 0) return ErrorCode::NoSpace;
    ModularHandleEntry& entry = handles_[h];
    entry.path = path;
    entry.mode = OpenMode::Read;
    entry.offset = 0;
    entry.is_open = true;
    active_handles_++;
    return h;
}

int ReadOnlyFSModule::read_file(int handle, uint8_t* out_buf, size_t max_bytes) {
    if (!is_valid_handle(handle)) return ErrorCode::InvalidHandle;
    ModularHandleEntry& entry = handles_[handle];
    if (max_bytes == 0) return 0;
    if (out_buf == nullptr) return ErrorCode::InvalidArgument;
    int bytes = file_engine_.read_bytes(entry.path, entry.offset, out_buf, max_bytes);
    if (bytes > 0) entry.offset += bytes;
    return bytes;
}

int ReadOnlyFSModule::write_file(int handle, const uint8_t* in_data, size_t count) {
    // Reject every write request with PermissionDenied.
    (void)handle;
    (void)in_data;
    (void)count;
    return ErrorCode::PermissionDenied;
}

int ReadOnlyFSModule::close_file(int handle) {
    if (!is_valid_handle(handle)) return ErrorCode::InvalidHandle;
    handles_[handle].is_open = false;
    if (active_handles_ > 0) active_handles_--;
    return ErrorCode::Success;
}

size_t ReadOnlyFSModule::get_active_handle_count() const {
    return active_handles_;
}

int ReadOnlyFSModule::allocate_handle() {
    for (size_t i = 0; i < handles_.size(); ++i) {
        if (!handles_[i].is_open) return static_cast<int>(i);
    }
    handles_.push_back(ModularHandleEntry{});
    return static_cast<int>(handles_.size() - 1);
}

bool ReadOnlyFSModule::is_valid_handle(int handle) const {
    if (handle < 0 || static_cast<size_t>(handle) >= handles_.size()) return false;
    return handles_[handle].is_open;
}

// ============================================================================
// StandardConsoleModule Implementation
// ============================================================================

StandardConsoleModule::StandardConsoleModule(IVirtualConsoleDevice& console, const std::string& name)
    : console_(console), name_(name) {}

std::string StandardConsoleModule::get_name() const { return name_; }
bool StandardConsoleModule::on_load() { return true; }
void StandardConsoleModule::on_unload() {}
void StandardConsoleModule::write_console(const std::string& text) {
    console_.write(text);
}

// ============================================================================
// NullConsoleModule Implementation (Test 4: Extensibility)
// ============================================================================

NullConsoleModule::NullConsoleModule(const std::string& name)
    : name_(name) {}

std::string NullConsoleModule::get_name() const { return name_; }
bool NullConsoleModule::on_load() { return true; }
void NullConsoleModule::on_unload() {}
void NullConsoleModule::write_console(const std::string& text) {
    (void)text;
    discarded_count_++;
}

size_t NullConsoleModule::get_discarded_count() const {
    return discarded_count_;
}

} // namespace modular
} // namespace os_sim
