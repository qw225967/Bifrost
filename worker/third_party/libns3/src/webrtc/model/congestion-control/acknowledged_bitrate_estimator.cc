#include "acknowledged_bitrate_estimator.h"

AcknowledgedBitrateEstimator::AcknowledgedBitrateEstimator():
    in_alr_(false),
    bitrate_estimator_(new BitrateEstimator())
{
    
}

AcknowledgedBitrateEstimator::~AcknowledgedBitrateEstimator() {}


void AcknowledgedBitrateEstimator::IncomingPacketFeedbackVector(
    const std::vector<PacketResult>& packet_feedback_vector) {
    assert(std::is_sorted(packet_feedback_vector.begin(),
        packet_feedback_vector.end(),
        PacketResult::ReceiveTimeOrder()));
    for (const auto& packet : packet_feedback_vector) {
        if (alr_ended_time_ && packet.sent_packet.send_time > *alr_ended_time_) {
            bitrate_estimator_->ExpectFastRateChange();
            alr_ended_time_.reset();
        }
        DataSize acknowledged_estimate = packet.sent_packet.size;
        acknowledged_estimate += packet.sent_packet.prior_unacked_data;
        bitrate_estimator_->Update(packet.receive_time, acknowledged_estimate,
            in_alr_);
    }
}

std::optional<RtcDataRate> AcknowledgedBitrateEstimator::bitrate() const {
    return bitrate_estimator_->bitrate();
}

std::optional<RtcDataRate> AcknowledgedBitrateEstimator::PeekRate() const {
    return bitrate_estimator_->PeekRate();
}

void AcknowledgedBitrateEstimator::SetAlrEndedTime(Timestamp alr_ended_time) {
    alr_ended_time_.emplace(alr_ended_time);
}

void AcknowledgedBitrateEstimator::SetAlr(bool in_alr) {
    in_alr_ = in_alr;
}