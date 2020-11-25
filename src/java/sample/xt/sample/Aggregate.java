package xt.sample;

import xt.audio.Enums.XtSample;
import xt.audio.Enums.XtSetup;
import xt.audio.Enums.XtSystem;
import xt.audio.Structs.XtAggregateDeviceParams;
import xt.audio.Structs.XtAggregateStreamParams;
import xt.audio.Structs.XtBuffer;
import xt.audio.Structs.XtChannels;
import xt.audio.Structs.XtFormat;
import xt.audio.Structs.XtMix;
import xt.audio.Structs.XtStreamParams;
import xt.audio.XtAudio;
import xt.audio.XtDevice;
import xt.audio.XtPlatform;
import xt.audio.XtSafeBuffer;
import xt.audio.XtService;
import xt.audio.XtStream;

public class Aggregate {

    static void onXRun(int index, Object user) {
        System.out.println("XRun on device " + index + ".");
    }

    static void onBuffer(XtStream stream, XtBuffer buffer, Object user) throws Exception {
        XtSafeBuffer safe = XtSafeBuffer.get(stream);
        safe.lock(buffer);
        int count = buffer.frames * stream.getFormat().channels.inputs;
        System.arraycopy(safe.getInput(), 0, safe.getOutput(), 0, count);
        safe.unlock(buffer);
    }

    public static void main() throws Exception {

        XtAggregateStreamParams aggregateParams;
        XtMix mix = new XtMix(48000, XtSample.INT16);
        XtFormat inputFormat = new XtFormat(mix, new XtChannels(2, 0, 0, 0));
        XtFormat outputFormat = new XtFormat(mix, new XtChannels(0, 0, 2, 0));

        try(XtPlatform platform = XtAudio.init(null, null, null)) {
            XtSystem system = XtAudio.setupToSystem(XtSetup.SYSTEM_AUDIO);
            XtService service = platform.getService(system);
            if(service == null) return;

            try(XtDevice input = service.openDefaultDevice(false);
                XtDevice output = service.openDefaultDevice(true)) {
                if(input == null || !input.supportsFormat(inputFormat)) return;
                if(output == null || !output.supportsFormat(outputFormat)) return;

                XtAggregateDeviceParams[] deviceParams = new XtAggregateDeviceParams[2];
                deviceParams[0] = new XtAggregateDeviceParams(input, inputFormat.channels, 30.0);
                deviceParams[1] = new XtAggregateDeviceParams(output, outputFormat.channels, 30.0);
                XtStreamParams streamParams = new XtStreamParams(true, Aggregate::onBuffer, Aggregate::onXRun);
                aggregateParams = new XtAggregateStreamParams(streamParams, deviceParams, 2, mix, output);
                try(XtStream stream = service.aggregateStream(aggregateParams, null);
                    XtSafeBuffer safe = XtSafeBuffer.register(stream, true)) {
                    stream.start();
                    Thread.sleep(2000);
                    stream.stop();
                }
            }
        }
    }
}