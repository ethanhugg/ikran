// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// VideoCaptureController is the glue between VideoCaptureHost,
// VideoCaptureManager and VideoCaptureDevice.
// It provides functions for VideoCaptureHost to start a VideoCaptureDevice and
// is responsible for keeping track of shared DIBs and filling them with I420
// video frames for IPC communication between VideoCaptureHost and
// VideoCaptureMessageFilter.
// It implements media::VideoCaptureDevice::EventHandler to get video frames
// from a VideoCaptureDevice object and do color conversion straight into the
// shared DIBs to avoid a memory copy.
// It serves multiple VideoCaptureControllerEventHandlers.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_CONTROLLER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_CONTROLLER_H_
#pragma once

#include <list>
#include <map>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/process.h"
#include  "base/shared_memory.h"
#include "base/synchronization/lock.h"
#include "third_party/webrtc/common_types.h"
#include "third_party/webrtc/video_engine/include/vie_base.h"
#include "third_party/webrtc/video_engine/include/vie_capture.h"
#include "third_party/webrtc/video_engine/include/vie_codec.h"
#include "third_party/webrtc/video_engine/include/vie_render.h"


class SipccVideoRenderer: public webrtc::ExternalRenderer 
{

public:
    SipccVideoRenderer(int w, int h,std::string type);
    ~SipccVideoRenderer();
  	void ReturnBuffer(int buffer_id);


protected:

   virtual  int FrameSizeChange(
        unsigned int width, 
        unsigned int height, 
        unsigned int numberOfStreams) OVERRIDE;

    //buffer: A pointer to the buffer containing the I420 video frame.
    virtual int DeliverFrame(unsigned char* buffer, 
                             int bufferSize, 
                             unsigned int timestamp) OVERRIDE;

  	// should always be on IO thread
  	void Init();
 	void  DeliverFrameOnIOThread(unsigned char* buffer,
                             int bufferSize,
                             unsigned int timestamp);

private:
  void SendFrameInfoAndBuffers(int buffer_size);
  int width;
  int height;
  bool init_done_;
  bool isLocal;
  // Lock to protect free_dibs_ and owned_dibs_.
  base::Lock lock_;

  struct SharedDIB;
  typedef std::map<int /*buffer_id*/, SharedDIB*> DIBMap;
  // All DIBs created by this object.
  // It's modified only on IO thread.
  DIBMap owned_dibs_;


};

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_CONTROLLER_H_
