#include "platform/VirtualHardware.hpp"
#include "platform/Tracer.hpp"
#include "platform/OSApi.hpp"
#include "platform/SimpleFileEngine.hpp"
#include "app/UserPrograms.hpp"

#include "1_monolithic/MonolithicKernel.hpp"
#include "1_monolithic/MonolithicOSApi.hpp"

#include "2_layered/Layer0_Hardware.hpp"
#include "2_layered/Layer1_Drivers.hpp"
#include "2_layered/Layer2_FileSystem.hpp"
#include "2_layered/Layer3_OSApi.hpp"

#include "3_modular/ModularKernel.hpp"
#include "3_modular/StandardModules.hpp"
#include "3_modular/ModularOSApi.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <memory>

using namespace os_sim;

// Test helper: ANSI colors for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

void report_result(const std::string& test_name, bool passed, const std::string& note = "") {
    if (passed) {
        std::cout << "  [" COLOR_GREEN "PASS" COLOR_RESET "] " << test_name;
        if (!note.empty()) std::cout << " (" << note << ")";
        std::cout << "\n";
        g_tests_passed++;
    } else {
        std::cout << "  [" COLOR_RED "FAIL" COLOR_RESET "] " << test_name;
        if (!note.empty()) std::cout << " (" << note << ")";
        std::cout << "\n";
        g_tests_failed++;
    }
}

#include "test_support.hpp"

// ============================================================================
// TEST 1: File Copy Correctness across all 3 Architectures
// ============================================================================
void run_test_1() {
    std::cout << "\n" COLOR_BOLD COLOR_CYAN
              << "=========================================================================\n"
              << " TEST 1: File Copy Correctness across all 3 Architectures\n"
              << "========================================================================="
              << COLOR_RESET "\n";

    std::string test_data_multiblock = 
        "Line 1: OS Structures Tutorial - Operating System Structures.\n"
        "Line 2: Monolithic vs Layered vs Modular Monolithic.\n"
        "Line 3: Copying file across block boundaries (64-byte blocks in VirtualDisk).\n"
        "Line 4: Testing multi-block write, read, and verify integrity.\n"
        "Line 5: Dr. Kargahi OS Course TA Assignment verification payload 2026.\n";

    // 1. Monolithic Test
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        monolithic::MonolithicKernel kernel(disk, console, tracer);
        monolithic::MonolithicOSApi os(kernel);

        write_test_file(os, "source_mono.txt", test_data_multiblock);
        bool copy_ok = copy_file(os, "source_mono.txt", "dest_mono.txt", 64);
        std::string read_back = read_test_file(os, "dest_mono.txt");

        bool pass = copy_ok && (read_back == test_data_multiblock);
        report_result("Monolithic: File Copy Multi-block Integrity", pass,
                      "Copied " + std::to_string(read_back.size()) + " bytes");
        run_boundary_cases(os, os.get_architecture_name());
    }

    // 2. Strict Layered Test
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, console, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::Layer3_OSApi os(l2, tracer);

        write_test_file(os, "source_lay.txt", test_data_multiblock);
        bool copy_ok = copy_file(os, "source_lay.txt", "dest_lay.txt", 64);
        std::string read_back = read_test_file(os, "dest_lay.txt");

        bool pass = copy_ok && (read_back == test_data_multiblock);
        report_result("Strict Layered: File Copy Multi-block Integrity", pass,
                      "Copied " + std::to_string(read_back.size()) + " bytes");
        run_boundary_cases(os, os.get_architecture_name());
    }

    // 3. Modular Monolithic Test
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        modular::ModularKernel kernel(tracer);
        auto fs_mod = std::make_shared<modular::StandardFSModule>(disk);
        auto con_mod = std::make_shared<modular::StandardConsoleModule>(console);
        kernel.load_module(fs_mod);
        kernel.load_module(con_mod);

        modular::ModularOSApi os(kernel);

        write_test_file(os, "source_mod.txt", test_data_multiblock);
        bool copy_ok = copy_file(os, "source_mod.txt", "dest_mod.txt", 64);
        std::string read_back = read_test_file(os, "dest_mod.txt");

        bool pass = copy_ok && (read_back == test_data_multiblock);
        report_result("Modular Monolithic: File Copy Multi-block Integrity", pass,
                      "Copied " + std::to_string(read_back.size()) + " bytes");
        run_boundary_cases(os, os.get_architecture_name());
    }
}

// ============================================================================
// TEST 2: Architectural Trace Flow & Metric Comparison Matrix
// ============================================================================
void run_test_2() {
    std::cout << "\nTEST 2: Trace contracts and identical-workload accounting\n";
    std::vector<MetricSnapshot> metrics;
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        monolithic::MonolithicKernel kernel(disk, console, tracer);
        monolithic::MonolithicOSApi os(kernel);
        metrics.push_back(run_metric_case(os, disk, tracer, "Monolithic", false, false));
    }
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, console, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::Layer3_OSApi os(l2, tracer);
        metrics.push_back(run_metric_case(os, disk, tracer, "Strict Layered", true, false));
    }
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        modular::ModularKernel kernel(tracer);
        kernel.load_module(std::make_shared<modular::StandardFSModule>(disk));
        kernel.load_module(std::make_shared<modular::StandardConsoleModule>(console));
        modular::ModularOSApi os(kernel);
        metrics.push_back(run_metric_case(os, disk, tracer, "Modular Monolithic", false, true));
    }
    std::cout << "\nSource preparation and destination verification are excluded.\n"
              << "Architecture        | Crossings | Layer calls | Transfers | Model bytes | Disk R | Disk W | Module calls | Declared depth\n";
    for (const MetricSnapshot& metric : metrics) {
        std::cout << std::left << std::setw(20) << metric.name << "| "
                  << std::right << std::setw(9) << metric.crossings << " | "
                  << std::setw(11) << metric.layers << " | "
                  << std::setw(9) << metric.transfers << " | "
                  << std::setw(11) << metric.modeled_bytes << " | "
                  << std::setw(6) << metric.reads << " | "
                  << std::setw(6) << metric.writes << " | "
                  << std::setw(12) << metric.module_calls << " | " << metric.declared_depth << "\n";
    }
    std::cout << "Transfers are annotated model events, not measured memcpy calls.\n"
              << "Declared depth is illustrative and is not a grading or speed metric.\n";
    run_dispatch_and_tracer_checks();
}

// ============================================================================
// TEST 3: Invalid Input & Error Code Handling
// ============================================================================
void run_test_3() {
    std::cout << "\n" COLOR_BOLD COLOR_CYAN
              << "=========================================================================\n"
              << " TEST 3: Invalid Input & Error Code Handling (System Call Robustness)    \n"
              << "========================================================================="
              << COLOR_RESET "\n";

    auto test_arch = [](OSApi& os, const std::string& arch_name) {
        // 1. Open non-existent file for read
        int handle = os.open("non_existent_file.txt", OpenMode::Read);
        bool not_found_pass = (handle == ErrorCode::NotFound);

        // 2. Read from invalid handle
        std::vector<uint8_t> buf;
        int read_res = os.read(-1, buf, 64);
        bool invalid_read_pass = (read_res == ErrorCode::InvalidHandle);

        int read_res2 = os.read(999, buf, 64);
        bool invalid_read_pass2 = (read_res2 == ErrorCode::InvalidHandle);

        // 3. Write to invalid handle
        std::vector<uint8_t> dummy{1, 2, 3};
        int write_res = os.write(-1, dummy);
        bool invalid_write_pass = (write_res == ErrorCode::InvalidHandle);

        // 4. Close invalid handle
        int close_res = os.close(-5);
        bool invalid_close_pass = (close_res == ErrorCode::InvalidHandle);

        // 5. Open in Read mode and attempt to Write
        write_test_file(os, "ro_test.txt", "hello world");
        int ro_handle = os.open("ro_test.txt", OpenMode::Read);
        int write_ro_res = os.write(ro_handle, dummy);
        os.close(ro_handle);
        bool perm_denied_pass = (write_ro_res == ErrorCode::PermissionDenied);

        bool all_pass = not_found_pass && invalid_read_pass && invalid_read_pass2
                        && invalid_write_pass && invalid_close_pass && perm_denied_pass;

        report_result(arch_name + ": Robust Error Handling", all_pass);
    };

    // Test on all 3 architectures
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        monolithic::MonolithicKernel k(disk, console, tracer);
        monolithic::MonolithicOSApi os(k);
        test_arch(os, "Monolithic");
        run_additional_file_checks(os, "Monolithic");
    }

    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, console, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::Layer3_OSApi os(l2, tracer);
        test_arch(os, "Strict Layered");
        run_additional_file_checks(os, "Strict Layered");
    }

    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        modular::ModularKernel k(tracer);
        k.load_module(std::make_shared<modular::StandardFSModule>(disk));
        k.load_module(std::make_shared<modular::StandardConsoleModule>(console));
        modular::ModularOSApi os(k);
        test_arch(os, "Modular Monolithic");
        run_additional_file_checks(os, "Modular Monolithic");
    }
    {
        VirtualDisk disk(3);
        VirtualConsole console;
        Tracer tracer;
        monolithic::MonolithicKernel kernel(disk, console, tracer);
        monolithic::MonolithicOSApi os(kernel);
        run_capacity_checks(os, "Monolithic");
    }
    {
        VirtualDisk disk(3);
        VirtualConsole console;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, console, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::Layer3_OSApi os(l2, tracer);
        run_capacity_checks(os, "Strict Layered");
    }
    {
        VirtualDisk disk(3);
        VirtualConsole console;
        Tracer tracer;
        modular::ModularKernel kernel(tracer);
        kernel.load_module(std::make_shared<modular::StandardFSModule>(disk));
        kernel.load_module(std::make_shared<modular::StandardConsoleModule>(console));
        modular::ModularOSApi os(kernel);
        run_capacity_checks(os, "Modular Monolithic");
    }
}

// ============================================================================
// TEST 4: Extensibility (NullDevice / Custom Driver)
// ============================================================================
void run_test_4() {
    std::cout << "\n" COLOR_BOLD COLOR_CYAN
              << "=========================================================================\n"
              << " TEST 4: Extensibility (NullDevice / Custom Driver Integration)          \n"
              << "========================================================================="
              << COLOR_RESET "\n";

    // 1. Layered Extensibility: Connect NullConsoleDevice at Layer 0
    {
        VirtualDisk disk(256);
        NullConsoleDevice null_dev;
        Tracer tracer;
        layered::Layer0_Hardware l0(disk, null_dev, tracer);
        layered::Layer1_Drivers l1(l0, tracer);
        layered::Layer2_FileSystem l2(l1, tracer);
        layered::Layer3_OSApi os(l2, tracer);

        os.console_write("This message goes to NullDevice");
        os.console_write("Another discarded message");

        bool pass = (null_dev.get_discard_count() == 2);
        report_result("Layered Extensibility: Swapped NullDevice at Layer 0", pass,
                      "Discard count: " + std::to_string(null_dev.get_discard_count()));
    }

    // 2. Modular Extensibility: Dynamically load NullConsoleModule at runtime
    {
        VirtualDisk disk(256);
        VirtualConsole console;
        Tracer tracer;
        modular::ModularKernel k(tracer);
        k.load_module(std::make_shared<modular::StandardFSModule>(disk));
        auto std_con = std::make_shared<modular::StandardConsoleModule>(console);
        auto null_con = std::make_shared<modular::NullConsoleModule>("null_console");
        k.load_module(std_con);
        k.load_module(null_con);

        modular::ModularOSApi os(k);

        // Initially writing to standard console
        os.console_write("Visible in standard console");
        size_t std_history_before = console.get_history().size();

        // Dynamically switch active console module to null_console
        k.set_active_console_module("null_console");
        os.console_write("Silently discarded by null_console");
        os.console_write("Also discarded");

        size_t std_history_after = console.get_history().size();
        size_t null_discarded = null_con->get_discarded_count();

        bool pass = (std_history_before == 1) && (std_history_after == 1) && (null_discarded == 2);
        report_result("Modular Extensibility: Runtime Module Switching (NullConsole)", pass,
                      "Discard count: " + std::to_string(null_discarded));
    }
}

// ============================================================================
// TEST 5: Module Lifecycle Safety & Mechanism vs Policy (Modular Monolithic)
// ============================================================================
void run_test_5() {
    std::cout << "\n" COLOR_BOLD COLOR_CYAN
              << "=========================================================================\n"
              << " TEST 5: Module Lifecycle Safety & Policy Enforcement (Modular Monolithic)\n"
              << "========================================================================="
              << COLOR_RESET "\n";

    VirtualDisk disk(256);
    VirtualConsole console;
    Tracer tracer;
    modular::ModularKernel kernel(tracer);

    auto std_fs = std::make_shared<modular::StandardFSModule>(disk, "std_fs");
    auto std_con = std::make_shared<modular::StandardConsoleModule>(console);
    kernel.load_module(std_fs);
    kernel.load_module(std_con);

    modular::ModularOSApi os(kernel);

    // --- Part A: Lifecycle Safety (Reference Counting / Busy Module) ---
    int fd = os.open("locked_file.txt", OpenMode::Write);
    bool open_ok = (fd >= 0);

    // Module has 1 active handle: Unloading MUST be rejected!
    bool unload_attempt_blocked = open_ok && !kernel.unload_module("std_fs");
    report_result("Modular Lifecycle Safety: Unload rejected while file handle is open",
                  unload_attempt_blocked, open_ok ? ("Active handles: " + std::to_string(std_fs->get_active_handle_count())) : "File open failed");

    // Write some data and close handle
    std::vector<uint8_t> sample_data{'O', 'S', '2', '0', '2', '6'};
    os.write(fd, sample_data);
    os.close(fd);

    // Now active handles == 0: Unload MUST succeed!
    bool unload_succeeded = kernel.unload_module("std_fs");
    report_result("Modular Lifecycle Safety: Unload succeeded after closing all handles", unload_succeeded);

    // --- Part B: Mechanism vs Policy ---
    // Load ReadOnlyFSModule sharing the same underlying SimpleFileEngine
    auto ro_fs = std::make_shared<modular::ReadOnlyFSModule>(std_fs->get_file_engine(), "ro_fs");
    kernel.load_module(ro_fs);
    kernel.set_active_fs_module("ro_fs");

    // Policy Check 1: Opening for writing is strictly forbidden by Policy
    int write_open_res = os.open("new_file.txt", OpenMode::Write);
    bool write_forbidden = (write_open_res == ErrorCode::PermissionDenied);

    // Policy Check 2: Opening existing file for reading is allowed by Policy
    int read_open_res = os.open("locked_file.txt", OpenMode::Read);
    bool read_allowed = (read_open_res >= 0);

    // Policy Check 3: Calling write on the open read handle is rejected
    int write_attempt = os.write(read_open_res, sample_data);
    bool write_rejected = (write_attempt == ErrorCode::PermissionDenied);

    os.close(read_open_res);

    bool policy_pass = write_forbidden && read_allowed && write_rejected;
    report_result("Modular Policy Enforcement: ReadOnly policy strictly enforced without modifying kernel mechanism", policy_pass);
    run_additional_module_checks();
    run_module_failure_checks();
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main() {
    std::cout << COLOR_BOLD
              << "=========================================================================\n"
              << "      OPERATING SYSTEM STRUCTURES ARCHITECTURAL SIMULATION & TEST SUITE   \n"
              << "          University of Tehran - Dr. Kargahi OS Course \n"
              << "=========================================================================\n"
              << COLOR_RESET;

    run_test_1();
    run_test_2();
    run_test_3();
    run_test_4();
    run_test_5();

    std::cout << "\n" COLOR_BOLD
              << "=========================================================================\n"
              << " FINAL EVALUATION SUMMARY: "
              << COLOR_GREEN << g_tests_passed << " Passed" << COLOR_RESET << " / "
              << (g_tests_failed > 0 ? COLOR_RED : COLOR_GREEN) << g_tests_failed << " Failed" << COLOR_RESET << "\n"
              << "=========================================================================\n"
              << COLOR_RESET;

    return g_tests_failed == 0 ? 0 : 1;
}
