#pragma once
#include "ns3/data_rate.h"
#include <optional>
#include <queue>
#include "ns3/common.h"

using namespace rtc;
struct BitrateProberConfig {	
	BitrateProberConfig();
	BitrateProberConfig& operator=(const BitrateProberConfig&) = default;
	~BitrateProberConfig() = default;

	// The minimum number probing packets used.
	int min_probe_packets_sent;
	// A minimum interval between probes to allow scheduling to be feasible.
	TimeDelta min_probe_delta;
	// The minimum probing duration.
	TimeDelta min_probe_duration;
	// Maximum amount of time each probe can be delayed.
	TimeDelta max_probe_delay;
	// If NextProbeTime() is called with a delay higher than specified by
	// `max_probe_delay`, abort it.
	bool abort_delayed_probes;
};

class BitrateProber {
public:
	BitrateProber();
	~BitrateProber();

	void SetEnabled(bool enable);

	// Returns true if the prober is in a probing session, i.e., it currently
	// wants packets to be sent out according to the time returned by
	// TimeUntilNextProbe().
	bool is_probing() const { return probing_state_ == ProbingState::kActive; }

	// Initializes a new probing session if the prober is allowed to probe. Does
  // not initialize the prober unless the packet size is large enough to probe
  // with.
	void OnIncomingPacket(DataSize packet_size);

	// Create a cluster used to probe for `bitrate_bps` with `num_probes` number
	// of probes.
	void CreateProbeCluster(RtcDataRate bitrate, Timestamp now, int cluster_id);

	// Returns the time at which the next probe should be sent to get accurate
	// probing. If probing is not desired at this time, Timestamp::PlusInfinity()
	// will be returned.
	// TODO(bugs.webrtc.org/11780): Remove `now` argument when old mode is gone.
	Timestamp NextProbeTime(Timestamp now) const;

	// Information about the current probing cluster.
	std::optional<PacedPacketInfo> CurrentCluster(Timestamp now);

	// Returns the minimum number of bytes that the prober recommends for
	// the next probe, or zero if not probing.
	DataSize RecommendedMinProbeSize() const;

	// Called to report to the prober that a probe has been sent. In case of
	// multiple packets per probe, this call would be made at the end of sending
	// the last packet in probe. `size` is the total size of all packets in probe.
	void ProbeSent(Timestamp now, DataSize size);

private:
	enum class ProbingState {
		// Probing will not be triggered in this state at all times.
		kDisabled,
		// Probing is enabled and ready to trigger on the first packet arrival.
		kInactive,
		// Probe cluster is filled with the set of data rates to be probed and
		// probes are being sent.
		kActive,
		// Probing is enabled, but currently suspended until an explicit trigger
		// to start probing again.
		kSuspended,
	};

	// A probe cluster consists of a set of probes. Each probe in turn can be
	// divided into a number of packets to accommodate the MTU on the network.
	struct ProbeCluster {
		PacedPacketInfo pace_info;

		int sent_probes = 0;
		int sent_bytes = 0;
		Timestamp created_at = Timestamp::MinusInfinity();
		Timestamp started_at = Timestamp::MinusInfinity();
		int retries = 0;
	};

	Timestamp CalculateNextProbeTime(const ProbeCluster& cluster) const;

	ProbingState probing_state_;

	// Probe bitrate per packet. These are used to compute the delta relative to
	// the previous probe packet based on the size and time when that packet was
	// sent.
	std::queue<ProbeCluster> clusters_;

	// Time the next probe should be sent when in kActive state.
	Timestamp next_probe_time_;

	int total_probe_count_;
	int total_failed_probe_count_;

	BitrateProberConfig config_;
};