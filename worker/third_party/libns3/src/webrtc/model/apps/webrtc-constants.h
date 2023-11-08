/******************************************************************************
 * Copyright 2016-2017 Cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 *                                                                            *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/

/**
 * @file
 * Global constants for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */


#ifndef RMCAT_CONSTANTS_H
#define RMCAT_CONSTANTS_H

#include <stdint.h>

const uint32_t DEFAULT_PACKET_SIZE = 1000;
const uint32_t IPV4_HEADER_SIZE = 20;
const uint32_t UDP_HEADER_SIZE = 8;
const uint32_t IPV4_UDP_OVERHEAD = IPV4_HEADER_SIZE + UDP_HEADER_SIZE;
const uint64_t WEBRTC_FEEDBACK_PERIOD_US = 100 * 1000;
const uint64_t WEBRTC_ECHO_PERIOD_MS = 500;

// syncodec parameters
const uint32_t SYNCODEC_DEFAULT_FPS = 30;
enum SyncodecType {
    SYNCODEC_TYPE_PERFECT = 0,
    SYNCODEC_TYPE_FIXFPS,
    SYNCODEC_TYPE_STATS,
    SYNCODEC_TYPE_TRACE,
    SYNCODEC_TYPE_SHARING,
    SYNCODEC_TYPE_HYBRID
};

/**
 * Parameters for the rate shaping buffer as specified in draft-ietf-rmcat-nada
 * These are the default values according to the draft
 * The rate shaping buffer is currently implemented in the sender ns3
 * application (#ns3::RmcatSender ). For other congestion controllers
 * that do not need the rate shaping buffer, you can disable it by
 * setting USE_BUFFER to false.
 */
const bool USE_BUFFER = true;
const float BETA_V = 0.0;
const float BETA_S = 0.0;
const uint32_t MAX_QUEUE_SIZE_SANITY = 80 * 1000 * 1000; //bytes

/* topology parameters */
const uint32_t T_MAX_S = 500;  // maximum simulation duration  in seconds
const double T_TCP_LOG = 2;  // sample interval for log TCP flows

/* Default topology setting parameters */
const uint32_t WIFI_TOPO_MACQUEUE_MAXNPKTS = 1000;
const uint32_t WIFI_TOPO_ARPCACHE_ALIVE_TIMEOUT = 24 * 60 * 60; // 24 hours
const float WIFI_TOPO_2_4GHZ_PATHLOSS_EXPONENT = 3.0f;
const float WIFI_TOPO_2_4GHZ_PATHLOSS_REFLOSS = 40.0459f;
const uint32_t WIFI_TOPO_CHANNEL_WIDTH = 20;   // default channel width: 20MHz

#endif /* RMCAT_CONSTANTS_H */
