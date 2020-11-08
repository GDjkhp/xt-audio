package com.xtaudio.xt.sample;

import com.sun.jna.*;
import com.xtaudio.xt.*;
import static com.xtaudio.xt.NativeTypes.*;

public class RenderSimple {

    static float _phase = 0.0f;
    static final float FREQUENCY = 440.0f;
    static final XtMix MIX = new XtMix(44100, XtSample.FLOAT32);
    static final XtChannels CHANNELS = new XtChannels(0, 0, 1, 0);
    static final XtFormat FORMAT = new XtFormat(MIX, CHANNELS);

    static float nextSample() {
        _phase += FREQUENCY / FORMAT.mix.rate;
        if(_phase >= 1.0f) _phase = -1.0f;
        return (float)Math.sin(2.0 * _phase * Math.PI);
    }

    static void render(Pointer stream, XtBuffer buffer, Pointer user) {
        XtAdapter adapter = XtAdapter.get(stream);
        adapter.lockBuffer(buffer);
        float[] output = (float[])adapter.getOutput();
        for(int f = 0; f < buffer.frames; f++) output[f] = nextSample();
        adapter.unlockBuffer(buffer);
    }

    public static void main(String[] args) throws Exception {
        try(XtAudio audio = new XtAudio(null, null, null)) {
            XtSystem system = XtAudio.setupToSystem(XtSetup.CONSUMER_AUDIO);
            XtService service = XtAudio.getService(system);
            if(service == null) return;
            try(XtDevice device = service.openDefaultDevice(true)) {
                if(device == null || !device.supportsFormat(FORMAT)) return;
                XtBufferSize size = device.getBufferSize(FORMAT);
                try(XtStream stream = device.openStream(FORMAT, true, size.current, RenderSimple::render, null);
                    XtAdapter adapter = XtAdapter.register(stream, true, null)) {
                    stream.start();
                    Thread.sleep(1000);
                    stream.stop();
                }
            }
        }
    }
}