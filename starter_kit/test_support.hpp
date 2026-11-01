#pragma once

// Additional checks shared by the reference runner and the student runner.

class CountingApi : public OSApi {
public:
    explicit CountingApi(OSApi& api) : api_(api) {}
    size_t calls{0};

    int open(const std::string& path, OpenMode mode) override {
        calls++;
        return api_.open(path, mode);
    }
    int read(int handle, std::vector<uint8_t>& buffer, size_t count) override {
        calls++;
        return api_.read(handle, buffer, count);
    }
    int write(int handle, const std::vector<uint8_t>& data) override {
        calls++;
        return api_.write(handle, data);
    }
    int close(int handle) override {
        calls++;
        return api_.close(handle);
    }
    void console_write(const std::string& text) override {
        calls++;
        api_.console_write(text);
    }
    std::string get_architecture_name() const override {
        return api_.get_architecture_name();
    }

private:
    OSApi& api_;
};

bool balanced_domains(const Tracer& tracer) {
    std::string current_kernel;
    size_t transitions = 0;
    for (const TraceEvent& event : tracer.get_events()) {
        if (event.action != "DOMAIN_SWITCH") continue;
        transitions++;
        if (event.from == "UserSpace" && event.to != "UserSpace" && current_kernel.empty()) {
            current_kernel = event.to;
        } else if (event.to == "UserSpace" && event.from == current_kernel) {
            current_kernel.clear();
        } else {
            return false;
        }
    }
    return current_kernel.empty() && transitions == tracer.get_domain_switches();
}

void run_boundary_cases(OSApi& os, const std::string& name) {
    const size_t sizes[] = {0, 1, 63, 64, 65, 127, 128, 129, 327};
    const size_t chunks[] = {1, 17, 64, 100};
    for (size_t size : sizes) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; i++) data[i] = (i * 37 + size) % 256;
        int fd = os.open("binary_src.dat", OpenMode::Write);
        bool passed = fd >= 0 && os.write(fd, data) == int(size);
        if (fd >= 0) passed = os.close(fd) == 0 && passed;
        for (size_t chunk : chunks) {
            bool copied = copy_file(os, "binary_src.dat", "binary_dst.dat", chunk);
            bool equal = read_test_file(os, "binary_dst.dat") == bytes_to_string(data);
            passed = copied && equal && passed;
        }
        report_result(name + ": Binary copy at " + std::to_string(size) + " bytes", passed,
                      "chunks 1, 17, 64, 100");
    }
    bool seeded = write_test_file(os, "copy_guard.dat", "keep this data");
    bool same_path = !copy_file(os, "copy_guard.dat", "copy_guard.dat");
    bool zero_chunk = !copy_file(os, "copy_guard.dat", "guard_dst.dat", 0);
    report_result(name + ": Invalid copy requests preserve the source",
                  seeded && same_path && zero_chunk &&
                  read_test_file(os, "copy_guard.dat") == "keep this data");
}

void run_additional_file_checks(OSApi& os, const std::string& name) {
    report_result(name + ": Empty path rejected",
                  os.open("", OpenMode::Read) == ErrorCode::InvalidArgument &&
                  os.open("", OpenMode::Write) == ErrorCode::InvalidArgument);
    report_result(name + ": Reserved open modes rejected",
                  os.open("reserved.dat", OpenMode::Append) == ErrorCode::InvalidArgument &&
                  os.open("reserved.dat", OpenMode::ReadWrite) == ErrorCode::InvalidArgument);

    bool seeded = write_test_file(os, "contract.dat", "abcdef");
    int fd = os.open("contract.dat", OpenMode::Read);
    report_result(name + ": Contract-test setup", seeded && fd >= 0);
    if (!seeded || fd < 0) return;

    std::vector<uint8_t> buffer{99};
    int zero_read = os.read(fd, buffer, 0);
    bool empty_buffer = buffer.empty();
    int first_read = os.read(fd, buffer, 2);
    report_result(name + ": Zero-byte read preserves offset",
                  zero_read == 0 && empty_buffer && first_read == 2 &&
                  bytes_to_string(buffer) == "ab");
    int second_read = os.read(fd, buffer, 2);
    bool second_ok = second_read == 2 && bytes_to_string(buffer) == "cd";
    int short_read = os.read(fd, buffer, 64);
    report_result(name + ": Sequential and short reads",
                  second_ok && short_read == 2 && bytes_to_string(buffer) == "ef");
    report_result(name + ": EOF returns zero and clears the buffer",
                  os.read(fd, buffer, 64) == 0 && buffer.empty());
    report_result(name + ": Close and double close",
                  os.close(fd) == 0 && os.close(fd) == ErrorCode::InvalidHandle);
    std::vector<uint8_t> one_byte{90};
    report_result(name + ": Closed handles rejected",
                  os.read(fd, buffer, 1) == ErrorCode::InvalidHandle &&
                  os.write(fd, one_byte) == ErrorCode::InvalidHandle);

    int first = os.open("contract.dat", OpenMode::Read);
    int second = os.open("contract.dat", OpenMode::Read);
    bool independent = first >= 0 && second >= 0 && first != second;
    independent = os.read(first, buffer, 2) == 2 && bytes_to_string(buffer) == "ab" && independent;
    independent = os.read(second, buffer, 2) == 2 && bytes_to_string(buffer) == "ab" && independent;
    os.close(first);
    independent = os.read(second, buffer, 2) == 2 && bytes_to_string(buffer) == "cd" && independent;
    os.close(second);
    report_result(name + ": Independent open-file offsets", independent);

    int write_fd = os.open("contract.dat", OpenMode::Write);
    std::vector<uint8_t> empty;
    report_result(name + ": Zero-byte write succeeds",
                  write_fd >= 0 && os.write(write_fd, empty) == 0);
    report_result(name + ": Read from write-only handle rejected",
                  os.read(write_fd, buffer, 1) == ErrorCode::PermissionDenied);
    std::vector<uint8_t> replacement{'x', 'y'};
    bool overwritten = os.write(write_fd, replacement) == 2;
    os.close(write_fd);
    report_result(name + ": Write mode truncates previous contents",
                  overwritten && read_test_file(os, "contract.dat") == "xy");
}

void run_capacity_checks(OSApi& os, const std::string& name) {
    // The fixture provides three blocks, with block zero reserved.
    int fd = os.open("capacity.dat", OpenMode::Write);
    std::vector<uint8_t> too_large(129, 7);
    std::vector<uint8_t> full(128, 7);
    std::vector<uint8_t> one{90};
    report_result(name + ": Disk-full error code",
                  fd >= 0 && os.write(fd, too_large) == ErrorCode::NoSpace);
    int probe = os.open("probe.dat", OpenMode::Write);
    bool capacity_preserved = probe >= 0 && os.write(probe, one) == 1;
    os.close(probe);
    probe = os.open("probe.dat", OpenMode::Write);
    os.close(probe);
    report_result(name + ": Failed allocation preserves free capacity", capacity_preserved);
    bool filled = os.write(fd, full) == 128;
    os.close(fd);
    report_result(name + ": Exact disk capacity remains usable",
                  filled && read_test_file(os, "capacity.dat") == bytes_to_string(full));
    probe = os.open("probe.dat", OpenMode::Write);
    report_result(name + ": Full disk rejects an additional byte",
                  probe >= 0 && os.write(probe, one) == ErrorCode::NoSpace);
    os.close(probe);
    fd = os.open("capacity.dat", OpenMode::Write);
    bool reclaimed = fd >= 0 && os.write(fd, one) == 1;
    os.close(fd);
    report_result(name + ": Truncation reclaims blocks",
                  reclaimed && read_test_file(os, "capacity.dat") == "Z");
}

struct MetricSnapshot {
    std::string name;
    size_t crossings;
    size_t layers;
    size_t transfers;
    size_t modeled_bytes;
    size_t reads;
    size_t writes;
    size_t declared_depth;
    size_t module_calls;
};

MetricSnapshot run_metric_case(OSApi& os, VirtualDisk& disk, Tracer& tracer,
                               const std::string& name, bool layered_model, bool modular_model) {
    const std::string payload = "Standardized OS Workload for Architectural Comparison - UT 2026.\n";
    bool seeded = write_test_file(os, "bench_src.dat", payload);
    size_t reads_before = disk.get_read_ops();
    size_t writes_before = disk.get_write_ops();
    tracer.reset();
    CountingApi counted(os);
    bool copied = copy_file(counted, "bench_src.dat", "bench_dst.dat", 64);
    MetricSnapshot snapshot{name, tracer.get_domain_switches(), tracer.get_layer_transitions(),
                            tracer.get_data_copies(), tracer.get_data_bytes(),
                            disk.get_read_ops() - reads_before,
                            disk.get_write_ops() - writes_before, tracer.get_max_call_depth(),
                            tracer.get_module_dispatches()};
    bool balanced = balanced_domains(tracer);
    bool adjacent = true;
    for (const TraceEvent& event : tracer.get_events()) {
        if (event.from.find("Layer") != 0 || event.to.find("Layer") != 0) continue;
        if (event.action == "DATA_COPY") continue;
        bool allowed = (event.from == "Layer3" && event.to == "Layer2") ||
                       (event.from == "Layer2" && event.to == "Layer1") ||
                       (event.from == "Layer1" && event.to == "Layer0");
        adjacent = adjacent && allowed;
    }
    size_t expected_transfers = 4;
    size_t expected_bytes = 2 * payload.size();
    size_t expected_layers = 0;
    if (layered_model) {
        expected_transfers += snapshot.reads + snapshot.writes;
        expected_bytes += (snapshot.reads + snapshot.writes) * DISK_BLOCK_SIZE;
        expected_layers = counted.calls + 2 * (snapshot.reads + snapshot.writes + 2);
    }
    report_result(name + ": Identical benchmark completes correctly",
                  seeded && copied && read_test_file(os, "bench_dst.dat") == payload);
    report_result(name + ": One entry and return per API call",
                  balanced && snapshot.crossings == 2 * counted.calls && counted.calls == 11);
    report_result(name + ": Physical block-I/O accounting",
                  snapshot.reads == 3 && snapshot.writes == 2);
    report_result(name + ": Model-event contract",
                  adjacent && snapshot.layers == expected_layers &&
                  snapshot.transfers == expected_transfers && snapshot.modeled_bytes == expected_bytes);
    // Every API call of the modular kernel is routed to an active module; the other
    // architectures never go through a module interface.
    report_result(name + ": Module-dispatch contract",
                  snapshot.module_calls == (modular_model ? counted.calls : 0));
    return snapshot;
}

void run_dispatch_and_tracer_checks() {
    {
        VirtualDisk disk(8);
        VirtualConsole console;
        Tracer tracer;
        monolithic::MonolithicKernel kernel(disk, console, tracer);
        monolithic::SyscallArgs args;
        bool rejected = kernel.syscall(-1, args) == ErrorCode::InvalidArgument &&
                        kernel.syscall(999, args) == ErrorCode::InvalidArgument;
        report_result("Monolithic: Unknown syscall and balanced error return",
                      rejected && balanced_domains(tracer) && tracer.get_domain_switches() == 4);
        args.path = "null_buffer.dat";
        args.mode = OpenMode::Write;
        args.handle = kernel.syscall(monolithic::SYS_OPEN, args);
        args.count = 1;
        report_result("Monolithic: Positive write length requires an input buffer",
                      args.handle >= 0 && kernel.syscall(monolithic::SYS_WRITE, args) == ErrorCode::InvalidArgument);
        kernel.syscall(monolithic::SYS_CLOSE, args);
    }
    {
        VirtualDisk disk(8);
        VirtualConsole console;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, console, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::ParameterBlock pb{};
        pb.op = static_cast<layered::LayerOp>(999);
        l2.process_request(pb);
        report_result("Layered: Unknown operation and balanced error return",
                      pb.status == ErrorCode::InvalidArgument && balanced_domains(tracer) &&
                      tracer.get_domain_switches() == 2);
        pb.op = layered::LayerOp::Open;
        pb.path = "null_buffer.dat";
        pb.mode = OpenMode::Write;
        l2.process_request(pb);
        pb.handle = pb.status;
        pb.op = layered::LayerOp::Write;
        pb.length = 1;
        l2.process_request(pb);
        report_result("Layered: Positive write length requires an input buffer",
                      pb.handle >= 0 && pb.status == ErrorCode::InvalidArgument);
        pb.op = layered::LayerOp::Close;
        l2.process_request(pb);
        tracer.reset();
        std::array<uint8_t, DISK_BLOCK_SIZE> block{};
        bool failed = !l1.driver_read_sector(999, block) && !l1.driver_write_sector(999, block);
        report_result("Layered: Failed sector operations do not record transfers",
                      failed && tracer.get_data_copies() == 0 && tracer.get_data_bytes() == 0);
    }
    Tracer tracer;
    tracer.record_layer_transition(3, 2, "REQUEST");
    tracer.record_data_copy(0, "A", "B");
    report_result("Tracer: Layer calls are separate from domain crossings",
                  tracer.get_layer_transitions() == 1 && tracer.get_domain_switches() == 0 &&
                  tracer.get_data_copies() == 0);
    tracer.record_module_dispatch("fs", "open_file");
    report_result("Tracer: Module dispatches are separate from other counters",
                  tracer.get_module_dispatches() == 1 && tracer.get_domain_switches() == 0 &&
                  tracer.get_layer_transitions() == 1 && tracer.get_data_copies() == 0);
    tracer.record_data_copy(7, "A", "B");
    tracer.record_domain_switch("UserSpace", "Kernel");
    tracer.record_call_depth(4);
    tracer.reset();
    report_result("Tracer: Reset clears all model counters",
                  tracer.get_domain_switches() == 0 && tracer.get_layer_transitions() == 0 &&
                  tracer.get_data_copies() == 0 && tracer.get_data_bytes() == 0 &&
                  tracer.get_max_call_depth() == 0 && tracer.get_module_dispatches() == 0 &&
                  tracer.get_events().empty());
}

void run_additional_module_checks() {
    VirtualDisk first_disk(16);
    VirtualDisk second_disk(16);
    VirtualConsole console;
    Tracer tracer;
    modular::ModularKernel kernel(tracer);
    modular::ModularOSApi os(kernel);
    std::vector<uint8_t> buffer;
    const std::vector<uint8_t> data{1, 2, 3};
    bool unavailable = os.open("x", OpenMode::Read) == ErrorCode::ServiceUnavailable &&
                       os.read(0, buffer, 1) == ErrorCode::ServiceUnavailable &&
                       os.write(0, data) == ErrorCode::ServiceUnavailable &&
                       os.close(0) == ErrorCode::ServiceUnavailable;
    os.console_write("No console module");
    report_result("Modular: Missing services and balanced error returns",
                  unavailable && balanced_domains(tracer));
    auto first = std::make_shared<modular::StandardFSModule>(first_disk, "first");
    auto second = std::make_shared<modular::StandardFSModule>(second_disk, "second");
    auto output = std::make_shared<modular::StandardConsoleModule>(console, "output");
    bool registered = kernel.load_module(first) && kernel.load_module(second) && kernel.load_module(output);
    report_result("Modular: Invalid and duplicate registrations rejected",
                  registered && !kernel.load_module(nullptr) && !kernel.load_module(first));
    report_result("Modular: Unknown and wrong-type module selections rejected",
                  !kernel.set_active_fs_module("missing") && !kernel.set_active_fs_module("output") &&
                  !kernel.set_active_console_module("first") && !kernel.unload_module("missing"));
    int first_fd = os.open("one.dat", OpenMode::Write);
    int second_fd = os.open("two.dat", OpenMode::Write);
    bool two_open = first_fd >= 0 && second_fd >= 0 && first->get_active_handle_count() == 2;
    bool no_unload = !kernel.unload_module("first");
    bool no_switch = !kernel.set_active_fs_module("second");
    bool still_usable = os.write(first_fd, data) == 3;
    os.close(first_fd);
    bool one_open = first->get_active_handle_count() == 1 && !kernel.unload_module("first");
    os.close(second_fd);
    report_result("Modular: Busy module remains active and usable",
                  two_open && no_unload && no_switch && still_usable && one_open &&
                  first->get_active_handle_count() == 0);
    report_result("Modular: Switch after close and unload active module",
                  kernel.set_active_fs_module("second") && kernel.unload_module("second") &&
                  kernel.get_active_fs_module() == nullptr &&
                  os.open("x", OpenMode::Read) == ErrorCode::ServiceUnavailable);
    auto readonly = std::make_shared<modular::ReadOnlyFSModule>(first->get_file_engine(), "readonly");
    bool policy_loaded = kernel.load_module(readonly) && kernel.set_active_fs_module("readonly");
    bool denied = os.open("one.dat", OpenMode::Append) == ErrorCode::PermissionDenied &&
                  os.open("one.dat", OpenMode::ReadWrite) == ErrorCode::PermissionDenied;
    int read_fd = os.open("one.dat", OpenMode::Read);
    bool protected_handle = read_fd >= 0 && readonly->get_active_handle_count() == 1 &&
                            !kernel.unload_module("readonly") &&
                            os.read(read_fd, buffer, 8) == 3 && buffer == data;
    os.close(read_fd);
    report_result("Modular: Read-only policy and lifecycle share the same rules",
                  policy_loaded && denied && protected_handle &&
                  readonly->get_active_handle_count() == 0 && kernel.unload_module("readonly"));
    report_result("Modular: Successful unload removes registry entry",
                  !kernel.is_module_loaded("readonly") && !kernel.unload_module("readonly") &&
                  kernel.unload_module("first"));
}


// Test double: a file-system module whose services always fail with the given error code.
// reported_handles lets it pretend that it still owns open files, like a module that
// failed in the middle of its work.
class FaultyFSModule : public modular::IFileSystemModule {
public:
    FaultyFSModule(const std::string& name, int failure_code, size_t reported_handles = 0)
        : name_(name), failure_code_(failure_code), reported_handles_(reported_handles) {}

    std::string get_name() const override { return name_; }
    bool on_load() override { return true; }
    void on_unload() override { unloaded_ = true; }

    int open_file(const std::string&, OpenMode) override { return failure_code_; }
    int read_file(int, uint8_t*, size_t) override { return failure_code_; }
    int write_file(int, const uint8_t*, size_t) override { return failure_code_; }
    int close_file(int) override { return failure_code_; }
    size_t get_active_handle_count() const override { return reported_handles_; }

    bool was_unloaded() const { return unloaded_; }

private:
    std::string name_;
    int failure_code_;
    size_t reported_handles_;
    bool unloaded_{false};
};

void run_module_failure_checks() {
    VirtualDisk disk(16);
    VirtualConsole console;
    Tracer tracer;
    modular::ModularKernel kernel(tracer);
    modular::ModularOSApi os(kernel);
    auto healthy = std::make_shared<modular::StandardFSModule>(disk, "healthy_fs");
    auto broken = std::make_shared<FaultyFSModule>("broken_fs", ErrorCode::Busy);
    kernel.load_module(healthy);
    kernel.load_module(std::make_shared<modular::StandardConsoleModule>(console));
    kernel.load_module(broken);
    bool selected = kernel.set_active_fs_module("broken_fs");

    // 1. The kernel passes the module's error to the caller without changing it.
    tracer.reset();
    std::vector<uint8_t> buffer;
    const std::vector<uint8_t> data{1, 2, 3};
    bool propagated = selected &&
                      os.open("x", OpenMode::Write) == ErrorCode::Busy &&
                      os.read(0, buffer, 1) == ErrorCode::Busy && buffer.empty() &&
                      os.write(0, data) == ErrorCode::Busy &&
                      os.close(0) == ErrorCode::Busy;
    report_result("Modular fault: Module errors reach the caller unchanged", propagated);

    // 2. Failed calls still enter and leave the kernel once and move no data.
    report_result("Modular fault: Failed calls stay balanced and move no data",
                  balanced_domains(tracer) && tracer.get_domain_switches() == 8 &&
                  tracer.get_data_copies() == 0 && tracer.get_module_dispatches() == 4);

    // 3. Other services are not affected by the broken file-system module.
    os.console_write("still alive");
    report_result("Modular fault: Other services keep working",
                  console.get_last_line() == "still alive");

    // 4. The kernel recovers by switching to a healthy module and removing the broken one.
    bool switched = kernel.set_active_fs_module("healthy_fs");
    bool healthy_works = switched && write_test_file(os, "after_fault.dat", "ok") &&
                         read_test_file(os, "after_fault.dat") == "ok";
    bool removed = kernel.unload_module("broken_fs") && broken->was_unloaded() &&
                   !kernel.is_module_loaded("broken_fs");
    report_result("Modular fault: Recovery by switching to a healthy module and unloading the broken one",
                  healthy_works && removed);

    // 5. A failed module that still reports open handles is never removed by force.
    auto stuck = std::make_shared<FaultyFSModule>("stuck_fs", ErrorCode::Busy, 1);
    bool stuck_active = kernel.load_module(stuck) && kernel.set_active_fs_module("stuck_fs");
    report_result("Modular fault: Module with live handles is not removed or replaced by force",
                  stuck_active && !kernel.unload_module("stuck_fs") &&
                  !kernel.set_active_fs_module("healthy_fs") &&
                  kernel.get_active_fs_module() != nullptr &&
                  kernel.get_active_fs_module()->get_name() == "stuck_fs" &&
                  kernel.is_module_loaded("stuck_fs") && !stuck->was_unloaded());
}
