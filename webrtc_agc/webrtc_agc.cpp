// webrtc_agc.cpp: 定义控制台应用程序的入口点。
//
#define DR_WAV_IMPLEMENTATION

#include <iostream>
#include <vector>
#include "gain_control.h"
//#include "audio_util.h"
//#include "my_splitting_filter.h"
using namespace std;

#include "wavfile.h"
//#include "my_splitting_filter_c.h"

#include "WebrtcAGC.h"

int main()
{
	wav wavfile;
	//wavread("music_16k_0db.wav", &wavfile);
	wavread("speech with noise 32k.wav", &wavfile);
	//wavread("WAV48K16B-My Heart Will Go on.wav", &wavfile);
	size_t sample_rate = wavfile.sampleRate;

	const size_t input_size = wavfile.totalPCMFrameCount;
	int16_t *output_s16 = (int16_t*)calloc(input_size,sizeof(int16_t) );
	// AGC processing
	my_WebrtcAGC(wavfile.pDataS16[0], output_s16, input_size, sample_rate);	
	// output
	wavwrite_s16("agc_test 32k.wav", &output_s16, input_size, 1, sample_rate);

	free(output_s16);
    return 0;
}
