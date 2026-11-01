#include "Tracer.hpp"
#include <iomanip>
#include <algorithm>

namespace os_sim {

void Tracer::record_domain_switch(const std::string& from, const std::string& to, const std::string& details) {
    domain_switches_++;
    events_.push_back({from, to, "DOMAIN_SWITCH", details});
}

void Tracer::record_data_copy(size_t bytes, const std::string& from_buf, const std::string& to_buf) {
    if (bytes == 0) return;
    data_copies_++;
    data_bytes_ += bytes;
    events_.push_back({from_buf, to_buf, "DATA_COPY", std::to_string(bytes) + " bytes"});
}

void Tracer::record_call_depth(size_t depth) {
    if (depth > max_call_depth_) {
        max_call_depth_ = depth;
    }
}

void Tracer::record_ipc(const std::string& sender, const std::string& receiver,
                        const std::string& action, size_t bytes, const std::string& details) {
    ipc_messages_++;
    ipc_bytes_ += bytes;
    // An IPC round-trip across process protection domains entails domain switches
    domain_switches_ += 2;
    std::string full_details = details.empty() ? "[" + std::to_string(bytes) + " bytes]"
                                               : details + " [" + std::to_string(bytes) + " bytes]";
    events_.push_back({sender, receiver, "IPC:" + action, full_details});
}

void Tracer::record_layer_transition(int from_layer, int to_layer, const std::string& operation, const std::string& details) {
    layer_transitions_++;
    std::string from_name = "Layer" + std::to_string(from_layer);
    std::string to_name = "Layer" + std::to_string(to_layer);
    events_.push_back({from_name, to_name, operation, details});
}

void Tracer::record_module_dispatch(const std::string& module_name, const std::string& action) {
    module_dispatches_++;
    events_.push_back({"KernelCore", module_name, "MODULE_DISPATCH", action});
}

void Tracer::record_call(const std::string& from, const std::string& to, const std::string& action, const std::string& details) {
    events_.push_back({from, to, action, details});
}

void Tracer::reset() {
    domain_switches_ = 0;
    data_copies_ = 0;
    data_bytes_ = 0;
    max_call_depth_ = 0;
    ipc_messages_ = 0;
    ipc_bytes_ = 0;
    layer_transitions_ = 0;
    module_dispatches_ = 0;
    events_.clear();
}

void Tracer::print_summary(const std::string& arch_name, std::ostream& os) const {
    os << "=========================================================\n";
    os << " Architectural Metric Summary: " << arch_name << "\n";
    os << "=========================================================\n";
    os << "  - Domain Switches    : " << domain_switches_ << "\n";
    os << "  - Modeled Transfers  : " << data_copies_ << "\n";
    os << "  - Modeled Bytes      : " << data_bytes_ << "\n";
    os << "  - Declared Depth     : " << max_call_depth_ << " (illustrative)\n";
    os << "  - Layer Transitions  : " << layer_transitions_ << "\n";
    os << "  - Module Dispatches  : " << module_dispatches_ << "\n";
    os << "  - Total Trace Events : " << events_.size() << "\n";
    os << "=========================================================\n";
}

void Tracer::print_call_flow(std::ostream& os, size_t max_events) const {
    os << "--- Call Flow Trace (first " << std::min(max_events, events_.size())
       << " of " << events_.size() << " events) ---\n";
    for (size_t i = 0; i < std::min(max_events, events_.size()); ++i) {
        const auto& e = events_[i];
        os << std::setw(3) << (i + 1) << ". [" << e.from << " -> " << e.to << "] "
           << e.action;
        if (!e.details.empty()) {
            os << " (" << e.details << ")";
        }
        os << "\n";
    }
    if (events_.size() > max_events) {
        os << "    ... [" << (events_.size() - max_events) << " events omitted] ...\n";
    }
    os << "---------------------------------------------------------\n";
}

} // namespace os_sim
