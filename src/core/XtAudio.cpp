// Don't warn about posix-compliant strdup() and friends.
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

#include "XtPrivate.hpp"
#include <cassert>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <inttypes.h>

// ---- local ----

static void InitStreamBuffers(
  XtIntermediateBuffers& buffers, XtBool interleaved, XtBool nonInterleaved,
  const XtFormat* format, int32_t frames, int32_t sampleSize) {

  if(interleaved) {
    buffers.inputInterleaved = std::vector<char>(frames * format->channels.inputs * sampleSize, '\0');
    buffers.outputInterleaved = std::vector<char>(frames * format->channels.outputs * sampleSize, '\0');
  }

  if(nonInterleaved) {
    buffers.inputNonInterleaved = std::vector<void*>(format->channels.inputs, nullptr);
    buffers.outputNonInterleaved = std::vector<void*>(format->channels.outputs, nullptr);
    buffers.inputChannelsNonInterleaved = std::vector<std::vector<char>>(
      format->channels.inputs, std::vector<char>(frames * sampleSize, '\0'));
    buffers.outputChannelsNonInterleaved = std::vector<std::vector<char>>(
      format->channels.outputs, std::vector<char>(frames * sampleSize, '\0'));
    for(int32_t i = 0; i < format->channels.inputs; i++)
      buffers.inputNonInterleaved[i] = &(buffers.inputChannelsNonInterleaved[i][0]);
    for(int32_t i = 0; i < format->channels.outputs; i++)
      buffers.outputNonInterleaved[i] = &(buffers.outputChannelsNonInterleaved[i][0]);
  }
}

static XtError OpenStreamInternal(XtDevice* d, const XtStreamParams* params, bool secondary, void* user, XtStream** stream) {

  XtError error;
  XtFault fault;
  int32_t frames;
  XtBool supports;
  XtSystem system;
  XtBool canInterleaved;
  XtBool initInterleaved;
  XtBool canNonInterleaved;
  XtBool initNonInterleaved;

  XT_ASSERT(d != nullptr);
  XT_ASSERT(params != nullptr);
  XT_ASSERT(stream != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  XT_ASSERT(params->bufferSize > 0.0);
  XT_ASSERT(params->streamCallback != nullptr);
  XT_ASSERT(XtiValidateFormat(d->GetSystem(), params->format));

  double rate = params->format.mix.rate;
  uint64_t inMask = params->format.channels.inMask;
  int32_t inputs = params->format.channels.inputs;
  uint64_t outMask = params->format.channels.outMask;
  int32_t outputs = params->format.channels.outputs;
  XtSample sample = params->format.mix.sample;

  auto attributes = XtAudioGetSampleAttributes(sample);

  *stream = nullptr;
  system = d->GetSystem();  
  if((error = XtDeviceSupportsFormat(d, &params->format, &supports)) != 0)
    return error;
  if(!supports)
    return XtiCreateError(system, XtAudioGetService(system)->GetFormatFault());
  if((error = XtDeviceSupportsAccess(d, XtTrue, &canInterleaved)) != 0)
    return error;
  if((error = XtDeviceSupportsAccess(d, XtFalse, &canNonInterleaved)) != 0)
    return error;
  if((fault = d->OpenStream(params, secondary, user, stream)) != 0)
    return XtiCreateError(d->GetSystem(), fault);
  if((fault = (*stream)->GetFrames(&frames)) != 0) {
    XtStreamDestroy(*stream);
    return XtiCreateError(d->GetSystem(), fault);
  }

  (*stream)->user = user;
  (*stream)->format = params->format;
  (*stream)->aggregated = false;
  (*stream)->aggregationIndex = 0;
  (*stream)->interleaved = params->interleaved;
  (*stream)->xRunCallback = params->xRunCallback;
  (*stream)->sampleSize = attributes.size;
  (*stream)->streamCallback = params->streamCallback;
  (*stream)->canInterleaved = canInterleaved;
  (*stream)->canNonInterleaved = canNonInterleaved;
  initInterleaved = params->interleaved && !canInterleaved;
  initNonInterleaved = !params->interleaved && !canNonInterleaved;
  InitStreamBuffers((*stream)->intermediate, initInterleaved, initNonInterleaved, &params->format, frames, attributes.size);
  return 0;
}

// ---- print ----

const char* XT_CALL XtPrintCauseToString(XtCause cause) {
  switch(cause) {
  case XtCauseFormat: return "Format";
  case XtCauseGeneric: return "Generic";
  case XtCauseService: return "Service";
  case XtCauseUnknown: return "Unknown";
  case XtCauseEndpoint: return "Endpoint";
  default: assert(false); return nullptr;
  }
}

const char* XT_CALL XtPrintSetupToString(XtSetup setup) {
  switch(setup) {
  case XtSetupProAudio: return "ProAudio";
  case XtSetupSystemAudio: return "SystemAudio";
  case XtSetupConsumerAudio: return "ConsumerAudio";
  default: assert(false); return nullptr;
  }
}

const char* XT_CALL XtPrintSystemToString(XtSystem system) {
  switch(system) {
  case XtSystemALSA: return "ALSA";
  case XtSystemASIO: return "ASIO";
  case XtSystemJACK: return "JACK";
  case XtSystemWASAPI: return "WASAPI";
  case XtSystemPulseAudio: return "PulseAudio";
  case XtSystemDirectSound: return "DirectSound";
  default: assert(false); return nullptr;
  }
}

const char* XT_CALL XtPrintSampleToString(XtSample sample) {
  switch(sample) {
  case XtSampleUInt8: return "UInt8";
  case XtSampleInt16: return "Int16";
  case XtSampleInt24: return "Int24";
  case XtSampleInt32: return "Int32";
  case XtSampleFloat32: return "Float32";
  default: assert(false); return nullptr;
  }
}

const char* XT_CALL XtPrintCapabilitiesToString(XtCapabilities capabilities) {
  size_t i = 0;
  std::string result;
  if(capabilities == 0) return "None";
  static thread_local char buffer[128];
  if((capabilities & XtCapabilitiesTime) != 0) result += "Time, ";
  if((capabilities & XtCapabilitiesLatency) != 0) result += "Latency, ";
  if((capabilities & XtCapabilitiesFullDuplex) != 0) result += "FullDuplex, ";
  if((capabilities & XtCapabilitiesChannelMask) != 0) result += "ChannelMask, ";
  if((capabilities & XtCapabilitiesXRunDetection) != 0) result += "XRunDetection, ";
  memcpy(buffer, result.data(), result.size() - 2);
  buffer[result.size() - 2] = '\0';
  return buffer;
}

const char* XT_CALL XtPrintErrorInfoToString(const XtErrorInfo* info) {
  std::ostringstream stream;
  static thread_local char buffer[1024];
  std::memset(buffer, 0, sizeof(buffer));
  stream << XtPrintSystemToString(info->system);
  stream << " " << XtPrintCauseToString(info->cause);
  stream << " Error: " << info->fault << " (" << info->text << ")";
  auto result = stream.str();
  std::memcpy(buffer, result.c_str(), std::min(static_cast<size_t>(1023), result.size()));
  return buffer;
}

// ---- audio ----

XtVersion XT_CALL XtAudioGetVersion(void) {
  return { 1, 7 };
}

XtErrorInfo XT_CALL XtAudioGetErrorInfo(XtError error) {
  XtErrorInfo result;
  XT_ASSERT(error != 0);
  auto fault = XtiGetErrorFault(error);
  auto system = static_cast<XtSystem>((error & 0xFFFFFFFF00000000) >> 32ULL);
  auto service = XtAudioGetService(system);
  result.fault = fault;
  result.system = system;
  result.text = service->GetFaultText(fault);
  result.cause = service->GetFaultCause(fault);
  return result;
}

XtAttributes XT_CALL XtAudioGetSampleAttributes(XtSample sample) {
  XT_ASSERT(XtSampleUInt8 <= sample && sample <= XtSampleFloat32);
  XtAttributes result;
  result.isSigned = sample != XtSampleUInt8;
  result.isFloat = sample == XtSampleFloat32;
  result.count = sample == XtSampleInt24? 3: 1;
  switch(sample) {
  case XtSampleUInt8: result.size = 1; break;
  case XtSampleInt16: result.size = 2; break;
  case XtSampleInt24: result.size = 3; break;
  case XtSampleInt32: result.size = 4; break;
  case XtSampleFloat32: result.size = 4; break;
  default: XT_FAIL("Unknown sample"); break;
  }
  return result;
}

void XT_CALL XtAudioTerminate(void) {
  XT_ASSERT(XtiCalledOnMainThread());
  XtiTerminatePlatform();
  free(XtiId);
  XtiId = nullptr;
  XtiErrorCallback = nullptr;
}

void XT_CALL XtAudioInit(const char* id, void* window, XtErrorCallback callback) {
  XtiErrorCallback = callback;
  XtiId = id == nullptr || strlen(id) == 0? strdup("XT-Audio"): strdup(id);
  XtiInitPlatform(window);
}

// ---- service ----

XtCapabilities XT_CALL XtServiceGetCapabilities(const XtService* s) {
  XT_ASSERT(s != nullptr);
  return s->GetCapabilities();
}

XtError XT_CALL XtServiceGetDeviceCount(const XtService* s, int32_t* count) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(count != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *count = 0;
  return XtiCreateError(s->GetSystem(), s->GetDeviceCount(count));
}

XtError XT_CALL XtServiceOpenDevice(const XtService* s, int32_t index, XtDevice** device) {
  XT_ASSERT(index >= 0);
  XT_ASSERT(s != nullptr);
  XT_ASSERT(device != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *device = nullptr;
  return XtiCreateError(s->GetSystem(), s->OpenDevice(index, device));
}

XtError XT_CALL XtServiceOpenDefaultDevice(const XtService* s, XtBool output, XtDevice** device) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(device != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *device = nullptr;
  return XtiCreateError(s->GetSystem(), s->OpenDefaultDevice(output, device));
}

XtError XT_CALL XtServiceAggregateStream(const XtService* s, const XtAggregateParams* params, void* user, XtStream** stream) {

  XT_ASSERT(s != nullptr);
  XT_ASSERT(params != nullptr);
  XT_ASSERT(params->count > 0);
  XT_ASSERT(stream != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  XT_ASSERT(params->master != nullptr);
  XT_ASSERT(params->devices != nullptr);
  XT_ASSERT(params->channels != nullptr);
  XT_ASSERT(params->bufferSizes != nullptr);
  XT_ASSERT(params->streamCallback != nullptr);

  XtSystem system = s->GetSystem();
  auto attrs = XtAudioGetSampleAttributes(params->mix.sample);
  std::unique_ptr<XtAggregate> result(new XtAggregate);
  result->user = user;
  result->running = 0;
  result->system = system;
  result->masterIndex = -1;
  result->aggregated = false;
  result->aggregationIndex = -1;
  result->insideCallbackCount = 0;
  result->sampleSize = attrs.size;
  result->interleaved = params->interleaved;
  result->xRunCallback = params->xRunCallback;
  result->canInterleaved = params->interleaved;
  result->streamCallback = params->streamCallback;
  result->canNonInterleaved = !params->interleaved;
  result->inputRings = std::vector<XtRingBuffer>(params->count, XtRingBuffer());
  result->outputRings = std::vector<XtRingBuffer>(params->count, XtRingBuffer());
  result->contexts = std::vector<XtAggregateContext>(params->count, XtAggregateContext());

  XtError error;
  int32_t frames = 0;
  int32_t thisFrames;
  XtStream* thisStream;
  XtFormat format = { 0 };
  bool masterFound = false;
  XtFormat thisFormat = { 0 };
  XtStreamCallback thisCallback;

  format.mix = params->mix;
  for(int32_t i = 0; i < params->count; i++) {
    thisFormat.mix = params->mix;
    thisFormat.channels.inputs = params->channels[i].inputs;
    thisFormat.channels.inMask = params->channels[i].inMask;
    thisFormat.channels.outputs = params->channels[i].outputs;
    thisFormat.channels.outMask = params->channels[i].outMask;

    result->contexts[i].index = i;
    result->contexts[i].stream = result.get();
    result->channels.push_back(params->channels[i]);
    format.channels.inputs += params->channels[i].inputs;
    format.channels.outputs += params->channels[i].outputs;

    masterFound |= params->master == params->devices[i];
    if(params->master == params->devices[i])
      result->masterIndex = i;
    thisCallback = params->master == params->devices[i]? XtiMasterCallback: XtiSlaveCallback;
    XtStreamParams thisParams = { 0 };
    thisParams.bufferSize = params->bufferSizes[i];
    thisParams.format = thisFormat;
    thisParams.interleaved = params->interleaved;
    thisParams.streamCallback = thisCallback;
    thisParams.xRunCallback = params->xRunCallback;
    if((error = OpenStreamInternal(params->devices[i], &thisParams, params->master != params->devices[i], &result->contexts[i], &thisStream) != 0))
      return error;
    result->streams.push_back(std::unique_ptr<XtStream>(thisStream));
    thisStream->aggregated = true;
    thisStream->aggregationIndex = i;
    if((error = XtStreamGetFrames(thisStream, &thisFrames)) != 0)
      return error;
    frames = thisFrames > frames? thisFrames: frames;
  }
  XT_ASSERT(masterFound);

  result->format = format;
  result->frames = frames * 2;
  InitStreamBuffers(result->weave, params->interleaved, !params->interleaved, &format, frames, attrs.size);
  InitStreamBuffers(result->intermediate, params-> interleaved, !params->interleaved, &format, frames, attrs.size);
  for(int32_t i = 0; i < params->count; i++) {
    result->inputRings[i] = XtRingBuffer(params->interleaved != XtFalse, result->frames, params->channels[i].inputs, attrs.size);
    result->outputRings[i] = XtRingBuffer(params->interleaved != XtFalse, result->frames, params->channels[i].outputs, attrs.size);
  }

  *stream = result.release();
  return 0;
}

// ---- device ----

void XT_CALL XtDeviceDestroy(XtDevice* d) {
  XT_ASSERT(XtiCalledOnMainThread());
  delete d;
}

XtError XT_CALL XtDeviceShowControlPanel(XtDevice* d) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  return XtiCreateError(d->GetSystem(), d->ShowControlPanel());
}

XtError XT_CALL XtDeviceGetMix(const XtDevice* d, XtBool* valid, XtMix* mix) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(mix != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *valid = XtFalse;
  memset(mix, 0, sizeof(XtMix));
  return XtiCreateError(d->GetSystem(), d->GetMix(valid, mix));
}

XtError XT_CALL XtDeviceGetName(const XtDevice* d, char* buffer, int32_t* size) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  XT_ASSERT(size != nullptr && *size >= 0);
  return XtiCreateError(d->GetSystem(), d->GetName(buffer, size));
}

XtError XT_CALL XtDeviceGetChannelCount(const XtDevice* d, XtBool output, int32_t* count) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(count != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *count = 0;
  return XtiCreateError(d->GetSystem(), d->GetChannelCount(output, count));
}

XtError XT_CALL XtDeviceSupportsAccess(const XtDevice* d, XtBool interleaved, XtBool* supports) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(supports != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  *supports = XtFalse;
  return XtiCreateError(d->GetSystem(), d->SupportsAccess(interleaved, supports));
}

XtError XT_CALL XtDeviceGetBufferSize(const XtDevice* d, const XtFormat* format, XtBufferSize* size) {
  XtError error;
  XtBool supports;
  XtSystem system;
  XT_ASSERT(d != nullptr);
  XT_ASSERT(size != nullptr);
  XT_ASSERT(format != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  XT_ASSERT(XtiValidateFormat(d->GetSystem(), *format));

  system = d->GetSystem();
  memset(size, 0, sizeof(XtBufferSize));
  if((error = XtDeviceSupportsFormat(d, format, &supports)) != 0)
    return error;
  if(!supports)
    return XtiCreateError(system, XtAudioGetService(system)->GetFormatFault());
  return XtiCreateError(d->GetSystem(), d->GetBufferSize(format, size));
}

XtError XT_CALL XtDeviceGetChannelName(const XtDevice* d, XtBool output, int32_t index, char* buffer, int32_t* size) {
  XT_ASSERT(index >= 0);
  XT_ASSERT(d != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());  
  XT_ASSERT(size != nullptr && *size >= 0);
  return XtiCreateError(d->GetSystem(), d->GetChannelName(output, index, buffer, size));
}

XtError XT_CALL XtDeviceSupportsFormat(const XtDevice* d, const XtFormat* format, XtBool* supports) {
  XT_ASSERT(d != nullptr);
  XT_ASSERT(format != nullptr);
  XT_ASSERT(supports != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  XT_ASSERT(XtiValidateFormat(d->GetSystem(), *format));
  *supports = XtFalse;
  return XtiCreateError(d->GetSystem(), d->SupportsFormat(format, supports));
}

XtError XT_CALL XtDeviceOpenStream(XtDevice* d, const XtStreamParams* params, void* user, XtStream** stream) {

  return OpenStreamInternal(d, params, false, user, stream);
}

// ---- stream ----

void XT_CALL XtStreamDestroy(XtStream* s) { 
  XT_ASSERT(XtiCalledOnMainThread());
  delete s;
}

XtError XT_CALL XtStreamStop(XtStream* s) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  return XtiCreateError(s->GetSystem(), s->Stop());
}

XtError XT_CALL XtStreamStart(XtStream* s) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(XtiCalledOnMainThread());
  return XtiCreateError(s->GetSystem(), s->Start());
}

const XtFormat* XT_CALL XtStreamGetFormat(const XtStream* s) {
  XT_ASSERT(s != nullptr);
  return &s->format;
}

XtError XT_CALL XtStreamGetFrames(const XtStream* s, int32_t* frames) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(frames != nullptr);
  *frames = 0;
  return XtiCreateError(s->GetSystem(), s->GetFrames(frames));
}

XtError XT_CALL XtStreamGetLatency(const XtStream* s, XtLatency* latency) {
  XT_ASSERT(s != nullptr);
  XT_ASSERT(latency != nullptr);
  memset(latency, 0, sizeof(XtLatency));
  return XtiCreateError(s->GetSystem(), s->GetLatency(latency));
}