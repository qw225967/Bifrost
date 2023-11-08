#pragma once
#include <optional>
#include "ns3/common.h"
#include "ns3/data_rate.h"
#include <map>

class ProbeBitrateEstimator {
public:
    ProbeBitrateEstimator();
    ~ProbeBitrateEstimator();

    // Should be called for every probe packet we receive feedback about.
    // Returns the estimated bitrate if the probe completes a valid cluster.
    std::optional<RtcDataRate> HandleProbeAndEstimateBitrate(
        const PacketResult& packet_feedback);

    std::optional<RtcDataRate> FetchAndResetLastEstimatedBitrate();

private:
    struct AggregatedCluster {
        int num_probes = 0;
        Timestamp first_send = Timestamp::PlusInfinity();
        Timestamp last_send = Timestamp::MinusInfinity();
        Timestamp first_receive = Timestamp::PlusInfinity();
        Timestamp last_receive = Timestamp::MinusInfinity();
        DataSize size_last_send = DataSize::Zero();
        DataSize size_first_receive = DataSize::Zero();
        DataSize size_total = DataSize::Zero();
    };

    // Erases old cluster data that was seen before `timestamp`.
    void EraseOldClusters(Timestamp timestamp);

    std::map<int, AggregatedCluster> clusters_;    
    std::optional<RtcDataRate> estimated_data_rate_;
};