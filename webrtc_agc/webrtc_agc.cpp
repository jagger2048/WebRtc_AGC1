// webrtc_agc.cpp: 定义控制台应用程序的入口点。
//
#include <iostream>
#include <vector>
#include "stdafx.h"
#include "AudioFile.h"
#include "gain_control.h"
#include <vector>
#include "audio_util.h"
using namespace std;

int main()
{
	AudioFile<float> af;
	af.load("speech with noise 32k.wav");

	//void *agcHandle = NULL;
	void *hAGC = WebRtcAgc_Create();
	int minLevel = 0;
	int maxLevel = 255;
	int agcMode = kAgcModeFixedDigital;
	size_t sample_rate = af.getSampleRate();
	WebRtcAgc_Init(hAGC, minLevel, maxLevel, agcMode, sample_rate);

	WebRtcAgcConfig agcConfig;
	agcConfig.targetLevelDbfs = 12; // 3 default
	agcConfig.compressionGaindB = 20; // 9 default
	agcConfig.limiterEnable = 1;
	WebRtcAgc_set_config(hAGC, agcConfig);

	vector<float> input(af.samples[0].begin(), af.samples[0].end());
	const size_t nFrames = 160;	// sample_rate / 100;
	auto iter = input.begin();

	int32_t outMicLevel = 0;
	int32_t inMicLevel = 0;
	int16_t echo = 0;
	uint8_t saturationWarning = 0;
	vector<float> data_out;
	for (size_t n = 0; n < input.size() / nFrames; n++)
	{
		// your processing here
		vector<float> data_in_float(iter, iter + nFrames);
		vector<int16_t> data_in_int16(nFrames);
		webrtc::FloatToS16(&data_in_float[0],nFrames, &data_in_int16[0]);

		vector<int16_t> data_out_int16(iter, iter + nFrames);
		int16_t *data_in_per_frame[1]{&data_in_int16[0]}; // 1 = nBanks
		int16_t *data_out_per_frame[1]{&data_out_int16[0]}; // 1 = nBanks
		int agcError = WebRtcAgc_Process(hAGC, data_in_per_frame, 1, nFrames,data_out_per_frame, inMicLevel, &outMicLevel, echo , &saturationWarning);
		if (agcError !=0)
		{
			std::cout << "Agc processes fail\n";
			return 0;
		}
		//inMicLevel = outMicLevel;
		vector<float> data_out_float(nFrames);
		webrtc::S16ToFloat(&data_out_int16[0], nFrames, &data_out_float[0]);

		// your processing here

		for (auto &per_data : data_out_float) {
			data_out.push_back(per_data);
		}
		iter += nFrames;
	}
	vector<vector<float>> tmp;
	tmp.push_back(data_out);
	af.setAudioBuffer(tmp);
	af.save("after agc  20db - 01.wav");

    return 0;
}
