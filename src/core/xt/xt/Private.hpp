#ifndef XT_PRIVATE_HPP
#define XT_PRIVATE_HPP

#include <xt/XtAudio.h>
#include <xt/api/private/Device.hpp>
#include <xt/api/private/Stream.hpp>
#include <xt/api/private/Service.hpp>
#include <xt/private/BlockingStream.hpp>
#include <xt/private/BlockingStreamWin32.hpp>
#include <xt/private/BlockingStreamLinux.hpp>
#include <xt/private/Shared.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdarg>

// ---- internal ----

#define XT_VERIFY_ON_BUFFER(expr) \
  VerifyOnBuffer({__FILE__,  __func__, __LINE__}, (expr), #expr)
// ---- forward ----

#define XT_IMPLEMENT_CALLBACK_OVER_BLOCKING_STREAM() \
  XtFault Stop() override;                           \
  XtFault Start() override;                          \
  void RequestStop() override;

#define XT_IMPLEMENT_BLOCKING_STREAM(system)             \
  void StopStream() override;                            \
  void StartStream() override;                           \
  void ProcessBuffer(bool prefill) override;             \
  XtFault GetFrames(int32_t* frames) const override;     \
  XtFault GetLatency(XtLatency* latency) const override; \
  XtSystem GetSystem() const override { return XtSystem ## system; }

// ---- internal ----

struct XtAggregate;

struct XtAggregateContext {
  int32_t index;
  XtAggregate* stream;
};

struct XtRingBuffer {
  int32_t end;
  int32_t full;
  int32_t begin;
  int32_t frames;
  int32_t channels;
  bool interleaved;
  int32_t sampleSize;
  mutable int32_t locked;
  std::vector<std::vector<uint8_t>> blocks;

  XtRingBuffer();
  XtRingBuffer(bool interleaved, int32_t frames, 
    int32_t channels, int32_t sampleSize);

  void Clear();
  void Lock() const;
  void Unlock() const;
  int32_t Full() const;
  int32_t Read(void* target, int32_t frames);
  int32_t Write(const void* source, int32_t frames);
};

struct XtAggregate: public XtStream {
  int32_t frames;
  XtSystem system;
  int32_t masterIndex;
  volatile int32_t running;
  XtIOBuffers _weave;
  std::vector<XtChannels> channels;
  volatile int32_t insideCallbackCount;
  std::vector<XtRingBuffer> inputRings; 
  std::vector<XtRingBuffer> outputRings;
  std::vector<XtAggregateContext> contexts;
  std::vector<std::unique_ptr<XtBlockingStream>> streams;

  virtual ~XtAggregate();
  virtual XtFault Stop();
  virtual XtFault Start();
  virtual XtSystem GetSystem() const;
  virtual XtFault GetFrames(int32_t* frames) const;
  virtual XtFault GetLatency(XtLatency* latency) const;
};

void XT_CALLBACK XtiOnSlaveBuffer(const XtStream* stream, const XtBuffer* buffer, void* user);
void XT_CALLBACK XtiOnMasterBuffer(const XtStream* stream, const XtBuffer* buffer, void* user);

#endif // XT_PRIVATE_HPP