#ifndef CONTENT_RENDERER_MEDIA_SIPCC_VIDEO_DECODER_H_
#define CONTENT_RENDERER_MEDIA_SIPCC_VIDEO_DECODER_H_
#include <string>

#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "base/time.h"
#include "content/common/content_export.h"
#include "media/base/filters.h"
#include "media/base/video_frame.h"
#include "media/video/capture/video_capture.h"
#include "third_party/webrtc/video_engine/include/vie_render.h"



class MessageLoop;

class CONTENT_EXPORT SIPCCVideoDecoder
    : public media::VideoDecoder
{
 public:
  SIPCCVideoDecoder(MessageLoop* message_loop, const std::string& url);
  virtual ~SIPCCVideoDecoder();

  // Filter implementation.
  virtual void Play(const base::Closure& callback) OVERRIDE;
  virtual void Seek(base::TimeDelta time,
                    const media::FilterStatusCB& cb) OVERRIDE;
  virtual void Pause(const base::Closure& callback) OVERRIDE;
  virtual void Flush(const base::Closure& callback) OVERRIDE;
  virtual void Stop(const base::Closure& callback) OVERRIDE;

  // Decoder implementation.
  virtual void Initialize(
      media::DemuxerStream* demuxer_stream,
      const media::PipelineStatusCB& filter_callback,
      const media::StatisticsCallback& stat_callback) OVERRIDE;
  virtual void Read(const ReadCB& callback) OVERRIDE;
  virtual const gfx::Size& natural_size() OVERRIDE;

  //buffer: A pointer to the buffer containing the I420 video frame.
  void DeliverFrame( media::VideoCapture::VideoFrameBuffer* frame);

  MessageLoop* MyMessageLoop() {
	return message_loop_;
  }

 private:

  MessageLoop* message_loop_;
  gfx::Size visible_size_;
  std::string url_;
  ReadCB read_cb_;

  // Used for accessing |read_cb_| from another thread.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(SIPCCVideoDecoder);
};
#endif
