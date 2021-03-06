#ifndef XT_SHARED_STRUCTS_HPP
#define XT_SHARED_STRUCTS_HPP

#include <xt/api/Structs.h>

#include <vector>
#include <atomic>
#include <cstdint>

struct XtLocation
{
  char const* file;
  char const* func;
  int32_t line;
};

struct XtBlockingParams 
{
  int32_t index;
  XtFormat format;
  double bufferSize;
  XtBool interleaved;
};

struct XtBuffers
{
  std::vector<uint8_t> interleaved;
  std::vector<void*> nonInterleaved;
  std::vector<std::vector<uint8_t>> channels;
};

struct XtIOBuffers
{
  XtBuffers input;
  XtBuffers output;
};

struct XtOnBufferParams
{
  int32_t index;
  bool emulated;
  bool interleaved;
  XtIOBuffers* buffers;
  XtFormat const* format;
  XtBuffer const* buffer;
};

struct XtAtomicInt
{
  std::atomic_int v;
  XtAtomicInt() = default;
  XtAtomicInt(XtAtomicInt const& i):
  v(i.v.load()) { }

  XtAtomicInt& operator=(XtAtomicInt const& i)
  { v = i.v.load(); return *this; }
};

template <class Rollback>
struct XtGuard
{
  bool committed;
  Rollback rollback;

  void Commit() { committed = true; }
  ~XtGuard() { if(!committed) rollback(); }
  XtGuard(Rollback rb): committed(false), rollback(rb) { }
  XtGuard(XtGuard&& g): committed(g.committed), rollback(g.rollback) { g.committed = true; }
};

#endif // XT_SHARED_STRUCTS_HPP