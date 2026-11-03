#include "ModularKernel.hpp"

namespace os_sim {
namespace modular {

ModularKernel::ModularKernel(Tracer& tracer) : tracer_(tracer) {}

bool ModularKernel::load_module(std::shared_ptr<IKernelModule> mod) {
    if (!mod) return false;
    std::string name = mod->get_name();
    if (modules_.find(name) != modules_.end()) {
        return false;
    }

    if (!mod->on_load()) {
        return false;
    }

    modules_[name] = mod;

    IFileSystemModule* fs = mod->as_fs();
    if (fs != nullptr && active_fs_ == nullptr) {
        active_fs_ = fs;
    } else {
        IConsoleModule* console = mod->as_console();
        if (console != nullptr && active_console_ == nullptr) {
            active_console_ = console;
        }
    }

    tracer_.record_call("KernelCore", name, "LOAD_MODULE", "Type: " + mod->get_type());
    return true;
}

bool ModularKernel::unload_module(const std::string& name) {
    if (modules_.count(name) == 0) {
        return false;
    }

    auto mod = modules_[name];

    // Lifecycle safety: reject unloading while the module still has open file handles.
    IFileSystemModule* fs = mod->as_fs();
    if (fs != nullptr && fs->get_active_handle_count() > 0) {
        tracer_.record_call("KernelCore", name, "REJECTED_UNLOAD",
                            std::to_string(fs->get_active_handle_count()) + " open handle(s)");
        return false;
    }

    mod->on_unload();

    // Clear the active pointer if this module was the active service.
    if (fs != nullptr && active_fs_ == fs) {
        active_fs_ = nullptr;
    }
    IConsoleModule* console = mod->as_console();
    if (console != nullptr && active_console_ == console) {
        active_console_ = nullptr;
    }

    modules_.erase(name);

    tracer_.record_call("KernelCore", name, "SUCCESS_UNLOAD", "Type: " + mod->get_type());
    return true;
}

bool ModularKernel::set_active_fs_module(const std::string& name) {
    auto it = modules_.find(name);
    if (it == modules_.end()) return false;
    auto fs = it->second->as_fs();
    if (!fs) return false;
    if (active_fs_ != nullptr && active_fs_ != fs && active_fs_->get_active_handle_count() > 0) {
        return false;
    }
    active_fs_ = fs;
    return true;
}

bool ModularKernel::set_active_console_module(const std::string& name) {
    auto it = modules_.find(name);
    if (it == modules_.end()) return false;
    auto con = it->second->as_console();
    if (!con) return false;
    active_console_ = con;
    return true;
}

int ModularKernel::sys_open(const std::string& path, OpenMode mode) {
    tracer_.record_domain_switch("UserSpace", "ModularKernel", "sys_open");
    tracer_.record_call_depth(4);
    int result = ErrorCode::ServiceUnavailable;
    if (active_fs_) {
        tracer_.record_module_dispatch(active_fs_->get_name(), "open_file");
        result = active_fs_->open_file(path, mode);
    }
    tracer_.record_domain_switch("ModularKernel", "UserSpace", "syscall_return");
    return result;
}

int ModularKernel::sys_read(int handle, uint8_t* out_buf, size_t max_bytes) {
    tracer_.record_domain_switch("UserSpace", "ModularKernel", "sys_read");
    tracer_.record_call_depth(4);
    int bytes = ErrorCode::ServiceUnavailable;
    if (active_fs_) {
        tracer_.record_module_dispatch(active_fs_->get_name(), "read_file");
        bytes = active_fs_->read_file(handle, out_buf, max_bytes);
    }
    if (bytes > 0) {
        tracer_.record_data_copy(bytes, "ModuleBuffer", "UserBuffer");
    }
    tracer_.record_domain_switch("ModularKernel", "UserSpace", "syscall_return");
    return bytes;
}

int ModularKernel::sys_write(int handle, const uint8_t* in_data, size_t count) {
    tracer_.record_domain_switch("UserSpace", "ModularKernel", "sys_write");
    tracer_.record_call_depth(4);
    int bytes = ErrorCode::ServiceUnavailable;
    if (active_fs_) {
        tracer_.record_module_dispatch(active_fs_->get_name(), "write_file");
        bytes = active_fs_->write_file(handle, in_data, count);
    }
    if (bytes > 0) {
        tracer_.record_data_copy(bytes, "UserBuffer", "ModuleBuffer");
    }
    tracer_.record_domain_switch("ModularKernel", "UserSpace", "syscall_return");
    return bytes;
}

int ModularKernel::sys_close(int handle) {
    tracer_.record_domain_switch("UserSpace", "ModularKernel", "sys_close");
    tracer_.record_call_depth(4);
    int result = ErrorCode::ServiceUnavailable;
    if (active_fs_) {
        tracer_.record_module_dispatch(active_fs_->get_name(), "close_file");
        result = active_fs_->close_file(handle);
    }
    tracer_.record_domain_switch("ModularKernel", "UserSpace", "syscall_return");
    return result;
}

void ModularKernel::sys_console_write(const std::string& text) {
    tracer_.record_domain_switch("UserSpace", "ModularKernel", "sys_console_write");
    tracer_.record_call_depth(4);
    if (active_console_) {
        tracer_.record_module_dispatch(active_console_->get_name(), "write_console");
        active_console_->write_console(text);
    }
    tracer_.record_domain_switch("ModularKernel", "UserSpace", "syscall_return");
}

IFileSystemModule* ModularKernel::get_active_fs_module() const {
    return active_fs_;
}

IConsoleModule* ModularKernel::get_active_console_module() const {
    return active_console_;
}

bool ModularKernel::is_module_loaded(const std::string& name) const {
    return modules_.find(name) != modules_.end();
}

} // namespace modular
} // namespace os_sim
