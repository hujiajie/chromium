// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/rtcp/rtcp_defines.h"

#include "media/cast/logging/logging_defines.h"

namespace media {
namespace cast {

RtcpCastMessage::RtcpCastMessage(uint32_t ssrc)
    : remote_ssrc(ssrc), ack_frame_id(0u), target_delay_ms(0) {}
RtcpCastMessage::RtcpCastMessage()
    : remote_ssrc(0), ack_frame_id(0u), target_delay_ms(0) {}
RtcpCastMessage::~RtcpCastMessage() {}

RtcpPliMessage::RtcpPliMessage(uint32_t ssrc) : remote_ssrc(ssrc) {}
RtcpPliMessage::RtcpPliMessage() : remote_ssrc(0) {}

RtcpReceiverEventLogMessage::RtcpReceiverEventLogMessage()
    : type(UNKNOWN), packet_id(0u) {}
RtcpReceiverEventLogMessage::~RtcpReceiverEventLogMessage() {}

RtcpReceiverFrameLogMessage::RtcpReceiverFrameLogMessage(RtpTimeTicks timestamp)
    : rtp_timestamp_(timestamp) {}
RtcpReceiverFrameLogMessage::RtcpReceiverFrameLogMessage(
    const RtcpReceiverFrameLogMessage& other) = default;
RtcpReceiverFrameLogMessage::~RtcpReceiverFrameLogMessage() {}

RtcpReceiverReferenceTimeReport::RtcpReceiverReferenceTimeReport()
    : remote_ssrc(0u), ntp_seconds(0u), ntp_fraction(0u) {}
RtcpReceiverReferenceTimeReport::~RtcpReceiverReferenceTimeReport() {}

RtcpEvent::RtcpEvent() : type(UNKNOWN), packet_id(0u) {}
RtcpEvent::~RtcpEvent() {}

RtpReceiverStatistics::RtpReceiverStatistics() :
    fraction_lost(0),
    cumulative_lost(0),
    extended_high_sequence_number(0),
    jitter(0) {}

SendRtcpFromRtpReceiver_Params::SendRtcpFromRtpReceiver_Params()
    : ssrc(0),
      sender_ssrc(0) {}

SendRtcpFromRtpReceiver_Params::~SendRtcpFromRtpReceiver_Params() {}

}  // namespace cast
}  // namespace media
