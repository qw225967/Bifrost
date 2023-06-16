/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/event.h"

#if defined(WEBRTC_WIN)
#include <windows.h>
#elif defined(WEBRTC_POSIX)
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#else
#error "Must define either WEBRTC_WIN or WEBRTC_POSIX."
#endif

#include "absl/types/optional.h"
#include "rtc_base/checks.h"
#include "rtc_base/synchronization/yield_policy.h"

namespace rtc {}  // namespace rtc
