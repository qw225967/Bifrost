#pragma once
// This class enables pushback from congestion window directly to video encoder.
// When the congestion window is filling up, the video encoder target bitrate
// will be reduced accordingly to accommodate the network changes. To avoid
// pausing video too frequently, a minimum encoder target bitrate threshold is
// used to prevent video pause due to a full congestion window.
#include <stdint.h>
#include <optional>
#include "ns3/data_size.h"

using namespace rtc;
class CongestionWindowPushbackController {
public:
    CongestionWindowPushbackController();
    void UpdateOutstandingData(int64_t outstanding_bytes);
    void UpdatePacingQueue(int64_t pacing_bytes);
    uint32_t UpdateTargetBitrate(uint32_t bitrate_bps);
    void SetDataWindow(DataSize data_window);

private:
    const bool add_pacing_;
    const uint32_t min_pushback_target_bitrate_bps_;
    std::optional<DataSize> current_data_window_;
    int64_t outstanding_bytes_ = 0;
    int64_t pacing_bytes_ = 0;
    double encoding_rate_ratio_ = 1.0;
};