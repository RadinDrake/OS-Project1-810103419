#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cstddef>

namespace os_sim {

// Represents a single event in the execution trace flow.
struct TraceEvent {
    std::string from;
    std::string to;
    std::string action;
    std::string details;
};

// Records events in the educational architecture model.
//
// Tracks universal comparative metrics across all 3 architectures:
//  - domain_switches: User/kernel entries and returns
//  - data_copies: Annotated buffer transfers, not a count of actual memcpy calls
//  - max_call_depth: Manually declared illustrative depth, not a measured stack
//  - layer_transitions: Downward calls between adjacent layers
//  - module_dispatches: Calls routed through a loadable-module interface (modular kernel only)
//  - call_flow: Chronological trace of system events
class Tracer {
public:
    Tracer() = default;

    void record_domain_switch(const std::string& from, const std::string& to, const std::string& details = "");
    void record_data_copy(size_t bytes, const std::string& from_buf, const std::string& to_buf);
    void record_call_depth(size_t depth);
    void record_ipc(const std::string& sender, const std::string& receiver,
                    const std::string& action, size_t bytes, const std::string& details = "");
    void record_layer_transition(int from_layer, int to_layer, const std::string& operation, const std::string& details = "");
    void record_module_dispatch(const std::string& module_name, const std::string& action);
    void record_call(const std::string& from, const std::string& to, const std::string& action, const std::string& details = "");

    void reset();

    // Getters
    size_t get_domain_switches() const { return domain_switches_; }
    size_t get_data_copies() const { return data_copies_; }
    size_t get_data_bytes() const { return data_bytes_; }
    size_t get_max_call_depth() const { return max_call_depth_; }
    size_t get_ipc_messages() const { return ipc_messages_; }
    size_t get_ipc_bytes() const { return ipc_bytes_; }
    size_t get_layer_transitions() const { return layer_transitions_; }
    size_t get_module_dispatches() const { return module_dispatches_; }
    const std::vector<TraceEvent>& get_events() const { return events_; }

    void print_summary(const std::string& arch_name, std::ostream& os = std::cout) const;
    void print_call_flow(std::ostream& os = std::cout, size_t max_events = 40) const;

private:
    size_t domain_switches_{0};
    size_t data_copies_{0};
    size_t data_bytes_{0};
    size_t max_call_depth_{0};
    size_t ipc_messages_{0};
    size_t ipc_bytes_{0};
    size_t layer_transitions_{0};
    size_t module_dispatches_{0};
    std::vector<TraceEvent> events_;
};

} // namespace os_sim
