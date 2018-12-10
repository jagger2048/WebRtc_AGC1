# WebRtc Agc

This repository is webrtc agc module demo. We capture the files form audio_processing/ modules/agc/legacy and other dependent files. And the algorithm has not been wraped from the C-type in this demo,so you can modify it as you like.

The agc algorithm in this demo can only process 10ms per frame, and support 16 khz or 8 khz sample rate's signal, that is 160 or 80 points per frame.Actually this algorithm native support up to 48khz.  If you want to apply it to upper sample rate , you should add a splitting filter to split the input signal into 2 or 3 bands for 32khz or 48khz. The same processing can also  be found at webrtc noise suppression. Moreover, be careful for the data format of input  and output data,they are int16_t PCM format.

Note that webrtc has another agc algorithm module, agc2, which based RNN technology, located at modules/agc2/* .

Have fun.

Jagger, 2018-12-10

------

ps: 

Processing flow:

WebRtcAgc_Create() ->WebRtcAgc_Init() ->WebRtcAgc_set_config() ->  processing per frame { WebRtcAgc_Process() } 



You can find the codes in the webrtc_agc.cpp file. I will add 32khz 48khz support in the next update version.
