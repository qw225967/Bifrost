#include "probe_bitrate_estimator.h"

namespace {
    // The minumum number of probes we need to receive feedback about in percent
    // in order to have a valid estimate.
    constexpr double kMinReceivedProbesRatio = .80;

    // The minumum number of bytes we need to receive feedback about in percent
    // in order to have a valid estimate.
    constexpr double kMinReceivedBytesRatio = .80;

    // The maximum |receive rate| / |send rate| ratio for a valid estimate.
    constexpr float kMaxValidRatio = 2.0f;

    // The minimum |receive rate| / |send rate| ratio assuming that the link is
    // not saturated, i.e. we assume that we will receive at least
    // kMinRatioForUnsaturatedLink * |send rate| if |send rate| is less than the
    // link capacity.
    constexpr float kMinRatioForUnsaturatedLink = 0.9f;

    // The target utilization of the link. If we know true link capacity
    // we'd like to send at 95% of that rate.
    constexpr float kTargetUtilizationFraction = 0.95f;

    // The maximum time period over which the cluster history is retained.
    // This is also the maximum time period beyond which a probing burst is not
    // expected to last.
    constexpr TimeDelta kMaxClusterHistory = TimeDelta::Seconds(1);

    // The maximum time interval between first and the last probe on a cluster
    // on the sender side as well as the receive side.
    constexpr TimeDelta kMaxProbeInterval = TimeDelta::Seconds(1);

}  // namespace

ProbeBitrateEstimator::ProbeBitrateEstimator(){}

ProbeBitrateEstimator::~ProbeBitrateEstimator() = default;

std::optional<RtcDataRate> ProbeBitrateEstimator::HandleProbeAndEstimateBitrate(
    const PacketResult& packet_feedback) {
    int cluster_id = packet_feedback.sent_packet.pacing_info.probe_cluster_id;
    assert(cluster_id != PacedPacketInfo::kNotAProbe);

    EraseOldClusters(packet_feedback.receive_time);

    AggregatedCluster* cluster = &clusters_[cluster_id];

    if (packet_feedback.sent_packet.send_time < cluster->first_send) {
        cluster->first_send = packet_feedback.sent_packet.send_time;
    }
    if (packet_feedback.sent_packet.send_time > cluster->last_send) {
        cluster->last_send = packet_feedback.sent_packet.send_time;
        cluster->size_last_send = packet_feedback.sent_packet.size;
    }
    if (packet_feedback.receive_time < cluster->first_receive) {
        cluster->first_receive = packet_feedback.receive_time;
        cluster->size_first_receive = packet_feedback.sent_packet.size;
    }
    if (packet_feedback.receive_time > cluster->last_receive) {
        cluster->last_receive = packet_feedback.receive_time;
    }
    cluster->size_total += packet_feedback.sent_packet.size;
    cluster->num_probes += 1;

    assert(
        packet_feedback.sent_packet.pacing_info.probe_cluster_min_probes > 0);
    assert(packet_feedback.sent_packet.pacing_info.probe_cluster_min_bytes >
        0);

    int min_probes =
        packet_feedback.sent_packet.pacing_info.probe_cluster_min_probes *
        kMinReceivedProbesRatio;
    DataSize min_size =
        DataSize::Bytes(
            packet_feedback.sent_packet.pacing_info.probe_cluster_min_bytes) *
        kMinReceivedBytesRatio;
    if (cluster->num_probes < min_probes || cluster->size_total < min_size)
        return std::nullopt;

    TimeDelta send_interval = cluster->last_send - cluster->first_send;
    TimeDelta receive_interval = cluster->last_receive - cluster->first_receive;

    if (send_interval <= TimeDelta::Zero() || send_interval > kMaxProbeInterval ||
        receive_interval <= TimeDelta::Zero() ||
        receive_interval > kMaxProbeInterval) {        
        return std::nullopt;
    }
    // Since the `send_interval` does not include the time it takes to actually
    // send the last packet the size of the last sent packet should not be
    // included when calculating the send bitrate.
    assert(cluster->size_total > cluster->size_last_send);
    DataSize send_size = cluster->size_total - cluster->size_last_send;
    RtcDataRate send_rate = send_size / send_interval;

    // Since the `receive_interval` does not include the time it takes to
    // actually receive the first packet the size of the first received packet
    // should not be included when calculating the receive bitrate.
    assert(cluster->size_total > cluster->size_first_receive);
    DataSize receive_size = cluster->size_total - cluster->size_first_receive;
    RtcDataRate receive_rate = receive_size / receive_interval;

    double ratio = receive_rate / send_rate;
    if (ratio > kMaxValidRatio) {
        return std::nullopt;
    }
    RtcDataRate res = std::min(send_rate, receive_rate);
    // If we're receiving at significantly lower bitrate than we were sending at,
    // it suggests that we've found the true capacity of the link. In this case,
    // set the target bitrate slightly lower to not immediately overuse.
    if (receive_rate < kMinRatioForUnsaturatedLink * send_rate) {
        assert(send_rate > receive_rate);
        res = kTargetUtilizationFraction * receive_rate;
    }    
    estimated_data_rate_ = res;
    return estimated_data_rate_;
}

std::optional<RtcDataRate>
ProbeBitrateEstimator::FetchAndResetLastEstimatedBitrate() {
    std::optional<RtcDataRate> estimated_data_rate = estimated_data_rate_;
    estimated_data_rate_.reset();
    return estimated_data_rate;
}

void ProbeBitrateEstimator::EraseOldClusters(Timestamp timestamp) {
    for (auto it = clusters_.begin(); it != clusters_.end();) {
        if (it->second.last_receive + kMaxClusterHistory < timestamp) {
            it = clusters_.erase(it);
        }
        else {
            ++it;
        }
    }
}