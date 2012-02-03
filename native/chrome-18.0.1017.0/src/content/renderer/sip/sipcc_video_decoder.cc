// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/sip/sipcc_video_decoder.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop.h"
#include "media/base/demuxer.h"
#include "media/base/filter_host.h"
#include "media/base/filters.h"
#include "media/base/limits.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "base/logging.h"

using media::CopyUPlane;
using media::CopyVPlane;
using media::CopyYPlane;
using media::DemuxerStream;
using media::FilterStatusCB;
using media::kNoTimestamp;
using media::PIPELINE_OK;
using media::StatisticsCallback;
using media::VideoDecoder;
using media::VideoFrame;

SIPCCVideoDecoder::SIPCCVideoDecoder(MessageLoop* message_loop,
                                 const std::string& url)
    : message_loop_(message_loop),
      visible_size_(640, 480),
      url_(url) {
}

SIPCCVideoDecoder::~SIPCCVideoDecoder() {}

void SIPCCVideoDecoder::Initialize(DemuxerStream* demuxer_stream,
                                 const media::PipelineStatusCB& filter_callback,
                                 const StatisticsCallback& stat_callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(
        FROM_HERE,
        base::Bind(&SIPCCVideoDecoder::Initialize, this,
                   make_scoped_refptr(demuxer_stream),
                   filter_callback, stat_callback));
    return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO) << "SIPCCVideoDecoder: Initialize() ";
  filter_callback.Run(PIPELINE_OK);

  // TODO(acolwell): Implement stats.
}

void SIPCCVideoDecoder::Play(const base::Closure& callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(FROM_HERE,
                            base::Bind(&SIPCCVideoDecoder::Play,
                                       this, callback));
    return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);

  LOG(INFO) << "SIPCCVideoDecoder: Play() ";
  VideoDecoder::Play(callback);
}

void SIPCCVideoDecoder::Pause(const base::Closure& callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(FROM_HERE,
                            base::Bind(&SIPCCVideoDecoder::Pause,
                                       this, callback));
    return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO) << "SIPCCVideoDecoder: Pause() ";


  VideoDecoder::Pause(callback);
}

void SIPCCVideoDecoder::Flush(const base::Closure& callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(FROM_HERE,
                            base::Bind(&SIPCCVideoDecoder::Flush,
                                       this, callback));
    return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO) << " SIPCCVideoDecoder:: Flush ";
  ReadCB read_cb;
  {
    base::AutoLock auto_lock(lock_);
    if (!read_cb_.is_null()) {
      std::swap(read_cb, read_cb_);
    }
  }

  if (!read_cb.is_null()) {
    scoped_refptr<media::VideoFrame> video_frame =
        media::VideoFrame::CreateBlackFrame(visible_size_.width(),
                                            visible_size_.height());
    read_cb.Run(video_frame);
  }

  VideoDecoder::Flush(callback);
}

void SIPCCVideoDecoder::Stop(const base::Closure& callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(FROM_HERE,
                            base::Bind(&SIPCCVideoDecoder::Stop,
                                       this, callback));
    return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO) << "SIPCCVideoDecoder: Stop ";


  VideoDecoder::Stop(callback);
}

void SIPCCVideoDecoder::Seek(base::TimeDelta time, const FilterStatusCB& cb) {
  if (MessageLoop::current() != message_loop_) {
     message_loop_->PostTask(FROM_HERE,
                             base::Bind(&SIPCCVideoDecoder::Seek, this,
                                        time, cb));
     return;
  }

  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO)<<" SIPCCVideoDecoder:: Seek ";
  cb.Run(PIPELINE_OK);
}

void SIPCCVideoDecoder::Read(const ReadCB& callback) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(
        FROM_HERE,
        base::Bind(&SIPCCVideoDecoder::Read, this, callback));
    return;
  }
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  LOG(INFO) <<" SIPCCVideoDecoder: Read ";
  base::AutoLock auto_lock(lock_);
  CHECK(read_cb_.is_null());
  read_cb_ = callback;
}

const gfx::Size& SIPCCVideoDecoder::natural_size() {
  // TODO(vrk): Return natural size when aspect ratio support is implemented.
  return visible_size_;
}

void SIPCCVideoDecoder::DeliverFrame( media::VideoCapture::VideoFrameBuffer* frame) {
 DCHECK(frame);
  LOG(INFO) << "SIPCCVideoDecoder: Deliver Frame ";
  ReadCB read_cb;
  {
    LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame Checking READ CALLBACK ";
    base::AutoLock auto_lock(lock_);
    if (read_cb_.is_null()) {
      LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame READ CALLBACK IS NULL";
      return ;
    }
    std::swap(read_cb, read_cb_);
  }

  // Always allocate a new frame.
  //
  // TODO(scherkus): migrate this to proper buffer recycling.
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame Creating new Video Frame ";
  scoped_refptr<media::VideoFrame> video_frame =
      VideoFrame::CreateFrame(VideoFrame::YV12,
                              visible_size_.width(),
                              visible_size_.height(),
                              host()->GetTime(),
                              base::TimeDelta::FromMilliseconds(30));
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame Creating Video Frame created ";

  int yPitch = 640;
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame yPitch " << yPitch;
  int uPitch = (640+ 1)/2;
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame yPitch " << uPitch;
  int vPitch = (640 + 1)/2;
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame yPitch " << vPitch;
  int uv_size = (641/2) * (481/2);
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame yPitch " << uv_size;

  int y_rows = 640;
  int uv_rows = 640/2;  // YV12 format.

  CopyYPlane(frame->memory_pointer,yPitch, y_rows, video_frame);
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame CopyYPlane" ;


  CopyUPlane((frame->memory_pointer + (640 * 480)), uPitch, uv_rows, video_frame);
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame CopyUPlane" ;

  CopyVPlane((frame->memory_pointer + (640 * 480 + uv_size)), vPitch, uv_rows, video_frame);
  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame CopyVPlane" ;

  LOG(INFO) << "SIPCCVideoDecoder: DeliverFrame Invoking Pipeline" ;
  read_cb.Run(video_frame);
  LOG(INFO) << " SIPCCVideoDecoder: Delivered Frame to Pipleline";
  return true;

}

