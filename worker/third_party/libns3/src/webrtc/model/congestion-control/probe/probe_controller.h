#pragma once
#include "ns3/time_delta.h"
#include "ns3/data_rate.h"
#include "ns3/common.h"
struct ProbeControllerConfig {
    explicit ProbeControllerConfig();
    ProbeControllerConfig(const ProbeControllerConfig&);
    ProbeControllerConfig& operator=(const ProbeControllerConfig&) = default;
    ~ProbeControllerConfig();

    // These parameters configure the initial probes. First we send one or two
    // probes of sizes p1 * start_bitrate_bps_ and p2 * start_bitrate_bps_.
    // Then whenever we get a bitrate estimate of at least further_probe_threshold
    // times the size of the last sent probe we'll send another one of size
    // step_size times the new estimate.
    double first_exponential_probe_scale;
    double second_exponential_probe_scale;
    double further_exponential_probe_scale;
    double further_probe_threshold;

    // Configures how often we send ALR probes and how big they are.
    TimeDelta alr_probing_interval;
    double alr_probe_scale;

    // Configures the probes emitted by changed to the allocated bitrate.
    double first_allocation_probe_scale;
    double second_allocation_probe_scale;
    bool allocation_allow_further_probing;
    RtcDataRate allocation_probe_max;
};

// This class controls initiation of probing to estimate initial channel
// capacity. There is also support for probing during a session when max
// bitrate is adjusted by an application.
class ProbeController {
public:
    ProbeController();
    ~ProbeController();

    std::vector<ProbeClusterConfig> SetBitrates(
        int64_t min_bitrate_bps,
        int64_t start_bitrate_bps,
        int64_t max_bitrate_bps,
        int64_t at_time_ms);

    // The total bitrate, as opposed to the max bitrate, is the sum of the
    // configured bitrates for all active streams.
    std::vector<ProbeClusterConfig>
        OnMaxTotalAllocatedBitrate(int64_t max_total_allocated_bitrate,
            int64_t at_time_ms);

    

    std::vector<ProbeClusterConfig> SetEstimatedBitrate(
        int64_t bitrate_bps,
        int64_t at_time_ms);

    void EnablePeriodicAlrProbing(bool enable);

    void SetAlrStartTimeMs(std::optional<int64_t> alr_start_time);
    void SetAlrEndedTimeMs(int64_t alr_end_time);

    std::vector<ProbeClusterConfig> RequestProbe(
        int64_t at_time_ms);

    // Sets a new maximum probing bitrate, without generating a new probe cluster.
    void SetMaxBitrate(int64_t max_bitrate_bps);

    // Resets the ProbeController to a state equivalent to as if it was just
    // created EXCEPT for `enable_periodic_alr_probing_`.
    void Reset(int64_t at_time_ms);

    std::vector<ProbeClusterConfig> Process(
        int64_t at_time_ms);

private:
    enum class State {
        // Initial state where no probing has been triggered yet.
        kInit,
        // Waiting for probing results to continue further probing.
        kWaitingForProbingResult,
        // Probing is complete.
        kProbingComplete,
    };

    std::vector<ProbeClusterConfig>
        InitiateExponentialProbing(int64_t at_time_ms);
    std::vector<ProbeClusterConfig> InitiateProbing(
        int64_t now_ms,
        std::vector<int64_t> bitrates_to_probe,
        bool probe_further);

    bool network_available_;
    State state_;
    int64_t min_bitrate_to_probe_further_bps_;
    int64_t time_last_probing_initiated_ms_;
    int64_t estimated_bitrate_bps_;
    int64_t start_bitrate_bps_;
    int64_t max_bitrate_bps_;
    int64_t last_bwe_drop_probing_time_ms_;
    std::optional<int64_t> alr_start_time_ms_;
    std::optional<int64_t> alr_end_time_ms_;
    bool enable_periodic_alr_probing_;
    int64_t time_of_last_large_drop_ms_;
    int64_t bitrate_before_last_large_drop_bps_;
    int64_t max_total_allocated_bitrate_;

    const bool in_rapid_recovery_experiment_;
    const bool limit_probes_with_allocateable_rate_;
    // For WebRTC.BWE.MidCallProbing.* metric.
    bool mid_call_probing_waiting_for_result_;
    int64_t mid_call_probing_bitrate_bps_;
    int64_t mid_call_probing_succcess_threshold_;
   

    int32_t next_probe_cluster_id_ = 1;
    ProbeControllerConfig config_;    
};