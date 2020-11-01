package com.xtaudio.xt;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.xtaudio.xt.NativeTypes.*;

import java.util.Arrays;

import static com.xtaudio.xt.XtNative.*;

public final class XtAudio implements XtCloseable {

    static XtErrorCallback _errorCallback;
    public XtAudio(String id, Pointer window, XtErrorCallback error) {
        _errorCallback = error;
        XtNative.init();
        XtAudioInit(id, window, error);
    }

    @Override public void close() { XtAudioTerminate(); }
    public static XtVersion getVersion() { return XtAudioGetVersion(); }
    public static XtErrorInfo getErrorInfo(long error) { return XtAudioGetErrorInfo(error); }
    public static XtSystem setupToSystem(XtSetup setup) { return XtAudioSetupToSystem(setup); }
    public static XtAttributes getSampleAttributes(XtSample sample) { return XtAudioGetSampleAttributes(sample); }

    public static XtService getService(XtSystem system) {
        Pointer service = XtAudioGetService(system);
        return service == Pointer.NULL? null: new XtService(service);
    }

    public static XtSystem[] getSystems() {
        var mapper = new XtTypeMapper();
        var size = new IntByReference();
        XtNative.XtAudioGetSystems(null, size);
        var result = new int[size.getValue()];
        XtNative.XtAudioGetSystems(result, size);
        var converter = mapper.getFromNativeConverter(XtSystem.class);
        return Arrays.stream(result).mapToObj(s -> converter.fromNative(s, null)).toArray(XtSystem[]::new);
    }
}