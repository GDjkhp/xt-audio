#ifndef XT_JACK_SHARED_HPP
#define XT_JACK_SHARED_HPP
#if XT_ENABLE_JACK
#include <xt/private/Device.hpp>
#include <xt/private/Stream.hpp>
#include <xt/private/Service.hpp>
#include <xt/private/DeviceList.hpp>
#include <xt/backend/jack/Private.hpp>

#include <jack/jack.h>
#include <vector>

struct JackService:
public XtService 
{
  JackService();
  ~JackService();
  XT_IMPLEMENT_SERVICE(JACK);
};

struct JackDevice:
public XtDevice
{
  XtJackClient _jc;
  XT_IMPLEMENT_DEVICE(JACK);
  XT_IMPLEMENT_DEVICE_STREAM();
  JackDevice(XtJackClient&& jc);
};

struct JackDeviceList:
public XtDeviceList
{
  XT_IMPLEMENT_DEVICE_LIST(JACK);
};

struct JackStream:
public XtStream
{
  XtJackClient _jc;
  std::atomic_int _running;
  std::atomic_int _insideCallback;
  std::vector<XtJackPort> _inputs;
  std::vector<XtJackPort> _outputs;
  std::vector<void*> _inputChannels;
  std::vector<void*> _outputChannels;
  std::vector<XtJackConnection> _connections;

  JackStream(XtJackClient&& jc);
  XT_IMPLEMENT_STREAM();
  XT_IMPLEMENT_STREAM_BASE();
  XT_IMPLEMENT_STREAM_BASE_SYSTEM(JACK);

  static int 
  XRunCallback(void* arg);
  static int 
  ProcessCallback(jack_nframes_t frames, void* arg);
};

#endif // XT_ENABLE_JACK
#endif // XT_JACK_SHARED_HPP