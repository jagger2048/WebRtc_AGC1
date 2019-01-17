// webrtc_agc.cpp: 定义控制台应用程序的入口点。
//
#include <iostream>
#include <vector>
#include "AudioFile.h"
#include "gain_control.h"
#include <vector>
#include "audio_util.h"
#include "my_splitting_filter.h"
using namespace std;
#define TEST_INIT_MODE

int main()
{
	AudioFile<float> af;
	af.load("speech with noise 32k.wav");
	size_t sample_rate = af.getSampleRate();
	//size_t sample_rate = 48000;
	mySplittingFilter splitting_filter(sample_rate);

#ifdef TEST_INIT_MODE
	//std::c
	void *hAGC = WebRtcAgc_Create();
	int minLevel = 0;
	int maxLevel = 255;
	int agcMode = kAgcModeFixedDigital;
	WebRtcAgc_Init(hAGC, minLevel, maxLevel, agcMode, sample_rate);

	WebRtcAgcConfig agcConfig;
	agcConfig.targetLevelDbfs = 12;			// 3 default
	agcConfig.compressionGaindB = 20;		// 9 default
	agcConfig.limiterEnable = 1;
	WebRtcAgc_set_config(hAGC, agcConfig);

	vector<float> input(af.samples[0].begin(), af.samples[0].end());
	const size_t nFrames = sample_rate / 100;	// sample_rate / 100;
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
		vector<int16_t> data_in_int16(nFrames);		// in int16 mod we use this format
		FloatToS16(&data_in_float[0], nFrames, &data_in_int16[0]);

		vector<int16_t> data_out_int16(nFrames);

		if (sample_rate == 16000)
		{
			int16_t *data_in_per_frame[1]{ &data_in_int16[0] };		// 1 = nBanks
			int16_t *data_out_per_frame[1]{ &data_out_int16[0] };		// 1 = nBanks
			int agcError = WebRtcAgc_Process(hAGC, data_in_per_frame, 1, 160, data_out_per_frame, inMicLevel, &outMicLevel, echo, &saturationWarning);
			if (agcError != 0)
			{
				std::cout << "Agc processes fail\n";
				return 0;
			}
			//inMicLevel = outMicLevel;
			vector<float> data_out_float(nFrames);
			S16ToFloat(&data_out_int16[0], nFrames, &data_out_float[0]);
			// your processing here

			for (auto &per_data : data_out_float) {
				data_out.push_back(per_data);
			}
		}
		else if (sample_rate == 32000)
		{
			int16_t tmp1[2][160]{};
			int16_t tmp2[2][160]{};
			int16_t tmp3[2][160]{};
			int16_t *data_out_per_frame[2] { tmp1[0],tmp1[1] };					// 2 = nBanks
			int16_t *bands_data_in[2]{ tmp2[0],tmp2[1]};							// 2 = nBanks
			int16_t *bands_data_out[2]{ tmp3[0],tmp3[1] };						// 2 = nBanks

																										//splitting_filter.Analysis( data_in_per_frame[0], bands_data_in );
			splitting_filter.Analysis(&data_in_int16[0], bands_data_in);
			// agc process
			int agcError = WebRtcAgc_Process(hAGC, bands_data_in, 2, 160, bands_data_out, inMicLevel, &outMicLevel, echo, &saturationWarning);
			if (agcError != 0)
			{
				std::cout << "Agc processes fail\n";
				return 0;
			}
			splitting_filter.Synthesis(bands_data_out, data_out_per_frame[0]);
			vector<float> data_out_float(nFrames);
			S16ToFloat(data_out_per_frame[0], nFrames, &data_out_float[0]);
			// your processing here

			for (auto &per_data : data_out_float) {
				data_out.push_back(per_data);
			}

		}
		iter += nFrames;
	}

#endif // TEST_INIT_MODE

#ifdef TEST_FLOAT_MODE
	vector<float> data_out;
	vector<float> input(af.samples[0].begin(), af.samples[0].end());
	const size_t frame_size = sample_rate / 100;	// sample_rate / 100;
	auto iter = input.begin();
	for (size_t n = 0; n < input.size() / frame_size; n++)
	{
		vector<float> frame(iter, iter + frame_size);
		float bands[3][160] = { 0 };
		float *bands_p[3] = { bands[0],bands[1],bands[2] };
		splitting_filter.Analysis(&frame[0], bands_p);

		vector<float> out(frame_size);
		splitting_filter.Synthesis(bands_p, &out[0]);
		for (auto per_data : out) {
			data_out.push_back(per_data);
		}
		iter += frame_size;
	}
#endif // TEST_FLOAT_MODE



	// output the processed signal
	vector<vector<float>> tmp;
	tmp.push_back(data_out);
	af.setAudioBuffer(tmp);
	af.save("simple test.wav");

    return 0;
}
