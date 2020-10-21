package com.xtaudio.xt;

import com.sun.jna.Callback;
import com.sun.jna.DefaultTypeMapper;
import com.sun.jna.FromNativeContext;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.ToNativeContext;
import com.sun.jna.TypeConverter;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import com.sun.jna.win32.StdCallFunctionMapper;
import com.sun.jna.win32.StdCallLibrary;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class XtNative {

    private static boolean initialized = false;

    private XtNative() {
    }

    public static class Mix extends Structure {

        public int rate;
        public int sample;

        Mix() {
        }

        Mix(Pointer value) {
            super(value);
            read();
        }

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("rate", "sample");
        }

        XtMix fromNative() {
            XtMix result = new XtMix();
            result.rate = rate;
            result.sample = XtSample.class.getEnumConstants()[sample];
            return result;
        }

        static Mix toNative(XtMix mix) {
            Mix result = new Mix();
            result.rate = mix.rate;
            result.sample = mix.sample.ordinal();
            return result;
        }
    }

    static class EnumConverter<E extends Enum<E>> implements TypeConverter {

        final int base;
        final Class<E> type;

        EnumConverter(Class<E> type, int base) {
            this.base = base;
            this.type = type;
        }

        @Override
        public Class<Integer> nativeType() {
            return Integer.class;
        }

        @Override
        public Object toNative(Object o, ToNativeContext tnc) {
            return o == null ? 0 : ((Enum<E>) o).ordinal() + base;
        }

        @Override
        public Object fromNative(Object o, FromNativeContext fnc) {
            return type.getEnumConstants()[((int) o) - base];
        }
    }

    static class TypeMapper extends DefaultTypeMapper {

        TypeMapper() {
            addTypeConverter(XtSample.class, new EnumConverter<>(XtSample.class, 0));
        }
    }

    public static class Format extends Structure {

        public int rate;
        public int sample;
        public int inputs;
        public long inMask;
        public int outputs;
        public long outMask;

        Format() {
        }

        Format(Pointer value) {
            super(value);
            read();
        }

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("rate", "sample", "inputs", "inMask", "outputs", "outMask");
        }

        XtFormat fromNative() {
            XtFormat result = new XtFormat();
            result.mix.rate = rate;
            result.mix.sample = XtSample.values()[sample];
            result.channels.inputs = inputs;
            result.channels.inMask = inMask;
            result.channels.outputs = outputs;
            result.channels.outMask = outMask;
            return result;
        }

        static Format toNative(XtFormat format) {
            Format result = new Format();
            result.rate = format.mix.rate;
            result.sample = format.mix.sample.ordinal();
            result.inputs = format.channels.inputs;
            result.inMask = format.channels.inMask;
            result.outputs = format.channels.outputs;
            result.outMask = format.channels.outMask;
            return result;
        }
    }

    public static class ChannelsByValue extends XtChannels implements Structure.ByValue {
    }

    static interface XRunCallback extends Callback {

        void callback(int index, Pointer user);
    }

    static interface TraceCallback extends Callback {

        void callback(int level, String message);
    }

    static interface StreamCallback extends Callback {

        void callback(Pointer stream, Pointer input, Pointer output, int frames,
                double time, long position, boolean timeValid, long error, Pointer user);
    }

    static void init() {
        if (initialized) {
            return;
        }
        boolean isX64 = Native.POINTER_SIZE == 8;
        System.setProperty("jna.encoding", "UTF-8");
        boolean isWin32 = System.getProperty("os.name").contains("Windows");
        Map<String, Object> options = new HashMap<>();
        if (isWin32 && !isX64) {
            options.put(Library.OPTION_FUNCTION_MAPPER, new StdCallFunctionMapper());
            options.put(Library.OPTION_CALLING_CONVENTION, StdCallLibrary.STDCALL_CONVENTION);
        }
        if (isWin32 && !isX64) {
            Native.register(NativeLibrary.getInstance("win32-x86/xt-core.dll", options));
        } else if (isWin32 && isX64) {
            Native.register(NativeLibrary.getInstance("win32-x64/xt-core.dll", options));
        } else if (!isWin32 && !isX64) {
            Native.register(NativeLibrary.getInstance("linux-x86/libxt-core.so", options));
        } else {
            Native.register(NativeLibrary.getInstance("linux-x64/libxt-core.so", options));
        }
        initialized = true;
    }

    static void handleError(long error) {
        if (error != 0) {
            throw new XtException(error);
        }
    }

    static String wrapAndFreeString(Pointer p) {
        String result = p.getString(0);
        XtAudioFree(p);
        return result;
    }

    static native int XtAudioGetErrorCause(long error);

    static native int XtAudioGetErrorFault(long error);

    static native int XtAudioGetErrorSystem(long error);

    static native String XtAudioGetErrorText(long error);

    static native Pointer XtAudioPrintCapabilitiesToString(int capabilities);

    static native void XtStreamDestroy(Pointer s);

    static native long XtStreamStop(Pointer s);

    static native long XtStreamStart(Pointer s);

    static native int XtStreamGetSystem(Pointer s);

    static native long XtStreamGetFrames(Pointer s, IntByReference frames);

    static native long XtStreamGetLatency(Pointer s, XtLatency latency);

    static native Pointer XtStreamGetFormat(Pointer s);

    static native boolean XtStreamIsInterleaved(Pointer s);

    static native int XtServiceGetSystem(Pointer s);

    static native String XtServiceGetName(Pointer s);

    static native int XtServiceGetCapabilities(Pointer s);

    static native long XtServiceGetDeviceCount(Pointer s, IntByReference count);

    static native long XtServiceOpenDevice(Pointer s, int index, PointerByReference device);

    static native long XtServiceOpenDefaultDevice(Pointer s, boolean output, PointerByReference device);

    static native long XtServiceAggregateStream(Pointer s, Pointer devices,
            Pointer channels, double[] bufferSizes, int count, Mix mix,
            boolean interleaved, Pointer master, StreamCallback streamCallback,
            XRunCallback xRunCallback, Pointer user, PointerByReference stream);

    static native boolean XtAudioIsWin32();

    static native void XtAudioTerminate();

    static native void XtAudioFree(Pointer p);

    static native int XtAudioGetVersionMajor();

    static native int XtAudioGetVersionMinor();

    static native int XtAudioGetServiceCount();

    static native Pointer XtAudioGetServiceByIndex(int index);

    static native Pointer XtAudioGetServiceBySetup(int setup);

    static native Pointer XtAudioGetServiceBySystem(int system);

    static native XtAttributes XtAudioGetSampleAttributes(int sample);

    static native void XtAudioInit(String id, Pointer window, TraceCallback trace, XtFatalCallback fatal);

    static native void XtDeviceDestroy(Pointer d);

    static native long XtDeviceShowControlPanel(Pointer d);

    static native int XtDeviceGetSystem(Pointer d);

    static native long XtDeviceGetName(Pointer d, PointerByReference name);

    static native long XtDeviceGetBuffer(Pointer d, Format format, XtBuffer buffer);

    static native long XtDeviceGetMix(Pointer d, IntByReference valid, XtMix mix);

    static native long XtDeviceGetChannelCount(Pointer d, boolean output, IntByReference count);

    static native long XtDeviceSupportsFormat(Pointer d, Format format, IntByReference supports);

    static native long XtDeviceSupportsAccess(Pointer d, boolean interleaved, IntByReference supports);

    static native long XtDeviceGetChannelName(Pointer d, boolean output, int index, PointerByReference name);

    static native long XtDeviceOpenStream(Pointer d, Format format, boolean interleaved, double bufferSize, StreamCallback streamCallback, XRunCallback xRunCallback, Pointer user, PointerByReference stream);
}
