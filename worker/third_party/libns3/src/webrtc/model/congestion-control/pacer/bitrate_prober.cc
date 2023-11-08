#include "bitrate_prober.h"
#include "ns3/data_size.h"
#include "ns3/time_delta.h"
#include "ns3/timestamp.h"
// The min probe packet size is scaled with the bitrate we're probing at.
// This defines the max min probe packet size, meaning that on high bitrates
// we have a min probe packet size of 200 bytes.
constexpr DataSize kMinProbePacketSize = DataSize::Bytes(200);
constexpr TimeDelta kProbeClusterTimeout = TimeDelta::Seconds(5);

BitrateProberConfig::BitrateProberConfig()
    : min_probe_packets_sent(5),
    min_probe_delta(TimeDelta::Millis(1)),
    min_probe_duration(TimeDelta::Millis(15)),
    max_probe_delay(TimeDelta::Millis(10)),
    abort_delayed_probes(true) {   
}

//BitrateProber只是为了维持一段时间和数据量内发送的网速不低于某一比特率
BitrateProber::BitrateProber()
:probing_state_(ProbingState::kDisabled),
next_probe_time_(Timestamp::PlusInfinity()),
total_probe_count_(0),
total_failed_probe_count_(0)
{
    SetEnabled(true);
}

BitrateProber::~BitrateProber() {

}

void BitrateProber::SetEnabled(bool enable) {
    if (enable) {
        if (probing_state_ == ProbingState::kDisabled) {
            probing_state_ = ProbingState::kInactive;
            std::cout << "Bandwidth probing enabled, set to inactive";
        }
    }
    else {
        probing_state_ = ProbingState::kDisabled;
        std::cout << "Bandwidth probing disabled";
    }
}
//进入的media packet可以作为探测包发送？
void BitrateProber::OnIncomingPacket(DataSize packet_size) {
    // Don't initialize probing unless we have something large enough to start
    // probing.
    if (probing_state_ == ProbingState::kInactive && !clusters_.empty() &&
        packet_size >= std::min(RecommendedMinProbeSize(), kMinProbePacketSize)) {
        // Send next probe right away.
        //立即发送（探测报文）
        next_probe_time_ = Timestamp::MinusInfinity();
        probing_state_ = ProbingState::kActive; //激活(clusters_非空）
    }
}

void BitrateProber::CreateProbeCluster(RtcDataRate bitrate,
    Timestamp now,
    int cluster_id) {
    assert(probing_state_ != ProbingState::kDisabled);
    assert(bitrate > RtcDataRate::Zero());

    total_probe_count_++;
    while (!clusters_.empty() &&
        now - clusters_.front().created_at > kProbeClusterTimeout) {
        clusters_.pop();
        total_failed_probe_count_++;
    }

    ProbeCluster cluster;
    cluster.created_at = now;
    cluster.pace_info.probe_cluster_min_probes = config_.min_probe_packets_sent;
    cluster.pace_info.probe_cluster_min_bytes =
        (bitrate * config_.min_probe_duration).bytes();
    assert(cluster.pace_info.probe_cluster_min_bytes >= 0);
    cluster.pace_info.send_bitrate_bps = bitrate.bps();
    cluster.pace_info.probe_cluster_id = cluster_id;
    clusters_.push(cluster);

    std::cout << "Probe cluster (bitrate:min bytes:min packets): ("
        << cluster.pace_info.send_bitrate_bps << ":"
        << cluster.pace_info.probe_cluster_min_bytes << ":"
        << cluster.pace_info.probe_cluster_min_probes << ")";
    // If we are already probing, continue to do so. Otherwise set it to
    // kInactive and wait for OnIncomingPacket to start the probing.
    if (probing_state_ != ProbingState::kActive)
        probing_state_ = ProbingState::kInactive;
}

Timestamp BitrateProber::NextProbeTime(Timestamp now) const {
    // Probing is not active or probing is already complete.
    if (probing_state_ != ProbingState::kActive || clusters_.empty()) {
        return Timestamp::PlusInfinity();
    }

    // Legacy behavior, just warn about late probe and return as if not probing.
    if (!config_.abort_delayed_probes && next_probe_time_.IsFinite() &&
        now - next_probe_time_ > config_.max_probe_delay) {
        std::cout << "Probe delay too high"
            " (next_ms:"
            << next_probe_time_.ms() << ", now_ms: " << now.ms()
            << ")";
        return Timestamp::PlusInfinity();
    }

    return next_probe_time_;
}

std::optional<PacedPacketInfo> BitrateProber::CurrentCluster(Timestamp now) {
    if (clusters_.empty() || probing_state_ != ProbingState::kActive) {
        return std::nullopt;
    }

    if (config_.abort_delayed_probes && next_probe_time_.IsFinite() &&
        now - next_probe_time_ > config_.max_probe_delay) {
        std::cout << "Probe delay too high"
            " (next_ms:"
            << next_probe_time_.ms() << ", now_ms: " << now.ms()
            << "), discarding probe cluster.";
        clusters_.pop();
        if (clusters_.empty()) {
            probing_state_ = ProbingState::kSuspended;
            return std::nullopt;
        }
    }

    PacedPacketInfo info = clusters_.front().pace_info;
    info.probe_cluster_bytes_sent = clusters_.front().sent_bytes;
    return info;
}

// Probe size is recommended based on the probe bitrate required. We choose
// a minimum of twice `kMinProbeDeltaMs` interval to allow scheduling to be
// feasible.
//根据配置，计算并返回它期待的下次媒体数据包发送的最小数据量，这通常是一个固定值：
DataSize BitrateProber::RecommendedMinProbeSize() const {
    if (clusters_.empty()) {
        return DataSize::Zero();
    }
    RtcDataRate send_rate =
        RtcDataRate::BitsPerSec(clusters_.front().pace_info.send_bitrate_bps);
    return 2 * send_rate * config_.min_probe_delta;
}

void BitrateProber::ProbeSent(Timestamp now, DataSize size) {
    assert(probing_state_ == ProbingState::kActive);
    assert(!size.IsZero());

    if (!clusters_.empty()) {
        ProbeCluster* cluster = &clusters_.front();
        if (cluster->sent_probes == 0) {
            assert(cluster->started_at.IsInfinite());
            cluster->started_at = now;
        }
        cluster->sent_bytes += size.bytes<int>(); //该探测已经发送的字节数
        cluster->sent_probes += 1; //该探测已经发送的报文数
        next_probe_time_ = CalculateNextProbeTime(*cluster);
        //探测是否完毕？
        if (cluster->sent_bytes >= cluster->pace_info.probe_cluster_min_bytes &&
            cluster->sent_probes >= cluster->pace_info.probe_cluster_min_probes) {       
            clusters_.pop();
        }
        //没有探测需求，over
        if (clusters_.empty()) {
            probing_state_ = ProbingState::kSuspended;
        }
    }
}

Timestamp BitrateProber::CalculateNextProbeTime(
    const ProbeCluster& cluster) const {
    assert(cluster.pace_info.send_bitrate_bps > 0);
    assert(cluster.started_at.IsFinite());

    // Compute the time delta from the cluster start to ensure probe bitrate stays
    // close to the target bitrate. Result is in milliseconds.
    DataSize sent_bytes = DataSize::Bytes(cluster.sent_bytes);
    RtcDataRate send_bitrate =
        RtcDataRate::BitsPerSec(cluster.pace_info.send_bitrate_bps);
    TimeDelta delta = sent_bytes / send_bitrate;
    return cluster.started_at + delta;
}
