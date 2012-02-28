// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/sip/sipcc_video_renderer.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "media/base/yuv_convert.h"
#include "sipcc_controller.h"

#include "base/logging.h"

using content::BrowserThread;


// The number of DIBs .
static const size_t kNoOfDIBS = 3;

struct SipccVideoRenderer::SharedDIB {
  SharedDIB(base::SharedMemory* ptr): shared_memory(ptr),
			              			references(0) {
  }

  ~SharedDIB() {}

  // The memory created to be shared with renderer processes.
  scoped_ptr<base::SharedMemory> shared_memory;

  // Number of renderer processes which hold this shared memory.
  // renderer process is represented by VidoeCaptureHost.
  int references;
};




SipccVideoRenderer::SipccVideoRenderer(
   int w, int h, std::string type)
    : width(w),
      height(h),
      init_done_(false),
      isLocal(false) 
{
	if(type.compare("remote") == 0)
	{
		LOG(INFO) << " Creating Video Renderer for Remote Capture ";
		isLocal = false;
	} else
    {
		LOG(INFO) << " Creating Video Renderer for Local Capture ";
		isLocal = true;
	}	  	

 	Init();

}

void  SipccVideoRenderer::Init() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(owned_dibs_.empty())
      << "Device is restarted without releasing shared memory.";
  LOG(INFO)<<" SipccVideoRenderer:: Init() Invoked : (isLocal ?? )" << isLocal;
  bool frames_created = true;
  const size_t needed_size = (width * height * 3) / 2;
  LOG(INFO) <<"SipccVideoRenderer::Init : needed_size " << needed_size;
  {
    base::AutoLock lock(lock_);
    for (size_t i = 1; i <= kNoOfDIBS; ++i) {
      base::SharedMemory* shared_memory = new base::SharedMemory();
      if (!shared_memory->CreateAndMapAnonymous(needed_size)) {
        frames_created = false;
        break;
      }
      SharedDIB* dib = new SharedDIB(shared_memory);
      owned_dibs_.insert(std::make_pair(i, dib));
    }
  }

  // Check whether all DIBs were created successfully.
  if (!frames_created) {
    LOG(ERROR) << " SipccVideoRenderer:: Allocation of Shared memory failed ";
    return;
  }
  
  SendFrameInfoAndBuffers(needed_size);
  LOG(INFO) << " SipccVideoRenderer:: Done Allocating Shared Memory ";

}

void SipccVideoRenderer::ReturnBuffer(int buffer_id)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  LOG(INFO) << "SipccVideoRenderer:: Return Buffer : " << buffer_id;
  DIBMap::iterator dib_it = owned_dibs_.find(buffer_id);
  // If this buffer is not held by this client, or this client doesn't exist
  // in controller, do nothing.
  if (dib_it == owned_dibs_.end())
  {
	LOG(INFO) << "SipccVideoRenderer:Return Buffer: No BUFFER TO RETURN";
    return;
  }

  {
    base::AutoLock lock(lock_);
    DCHECK_GT(dib_it->second->references, 0)
        << "The buffer is not used by renderer.";
    dib_it->second->references -= 1;
    if (dib_it->second->references > 0)
      return;
  }

}

SipccVideoRenderer::~SipccVideoRenderer() {
  LOG(INFO) << "SipccVideoRenderer:: Deleting Owned DIBs ";
  // Delete all DIBs.
  STLDeleteContainerPairSecondPointers(owned_dibs_.begin(),
                                      owned_dibs_.end());
}


///////////////////////////////////////////////////////////////////////////////
int SipccVideoRenderer::DeliverFrame(unsigned char* data,
                                     int length,
                                     unsigned int timestamp) {

  //post task to IO thread
  BrowserThread::PostTask(BrowserThread::IO,
                            FROM_HERE,
                            base::Bind(&SipccVideoRenderer::DeliverFrameOnIOThread,
                                       base::Unretained(this),
                                       data,length,timestamp));
  return 0;
 }

void SipccVideoRenderer::DeliverFrameOnIOThread(unsigned char* data,
                                     int length,
                                     unsigned int timestamp) {

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  int buffer_id = 0;
  base::SharedMemory* dib = NULL;
  {
    base::AutoLock lock(lock_);
    for (DIBMap::iterator dib_it = owned_dibs_.begin();
         dib_it != owned_dibs_.end(); dib_it++) {
	  LOG(INFO) << "SipccVideoRenderer::DeliverFrameOnIOThread: In  DIB loop ";
	  LOG(INFO) << "SipccVideoRenderer::DeliverFrameOnIOThread: references is "<<dib_it->second->references ;
      if (dib_it->second->references == 0) {
        buffer_id = dib_it->first;
        // Use special value "-1" in order to not be treated as buffer at
        // renderer side.
        dib_it->second->references = -1;
        dib = dib_it->second->shared_memory.get();
        break;
      }
    }
  }

  if (!dib) {
    LOG(ERROR) << " SipccVideoRenderer:: DeliverFrame: Not able to get memory handle";
    return 0;
  }

  uint8* target = static_cast<uint8*>(dib->memory());
  CHECK(dib->created_size() >= static_cast<size_t> (width *
                                                    height * 3) /
                                                    2);

  memcpy(target, static_cast<uint8*>(data), (width * height * 3) / 2);
  if(isLocal == true)
  {
  	SipccController::GetInstance()->OnCaptureBufferReady(buffer_id,timestamp);

  } else
  {
  	SipccController::GetInstance()->OnReceiveBufferReady(buffer_id,timestamp);
  }

  LOG(INFO) << "Deliver Frame Posted Task to IO Thread ";
  DCHECK_EQ(owned_dibs_[buffer_id]->references, -1);
  owned_dibs_[buffer_id]->references = 1;

  return 0;
}

int SipccVideoRenderer::FrameSizeChange(unsigned int width,
										unsigned int height,
										unsigned int numberOfStreams)
{

  LOG(INFO)<< " FrameSize called ";
  return 0;
}


///////////////////////////////////////////////////////////////////////

void SipccVideoRenderer::SendFrameInfoAndBuffers(int buffer_size) 
{

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  LOG(INFO) << " SipccVideoRenderer: SendFrameInfoAndBuffers: Buffer Size" << buffer_size;

  // the loop
  for (DIBMap::iterator dib_it = owned_dibs_.begin();
       dib_it != owned_dibs_.end(); dib_it++) {
    base::SharedMemory* shared_memory = dib_it->second->shared_memory.get();
    int index = dib_it->first;
    base::SharedMemoryHandle remote_handle;
    shared_memory->ShareToProcess(SipccController::GetInstance()->RenderProcessHandle(),
                                  &remote_handle);
	if (isLocal == true)
    {
    	SipccController::GetInstance()->OnCaptureBufferCreated(remote_handle,
				   											buffer_size,
				   											index);

	} else 
	{
    	SipccController::GetInstance()->OnReceiveBufferCreated(remote_handle,
				   											buffer_size,
				   											index);

	}
  } //end for

}
