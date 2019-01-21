#pragma once
#include "gain_control.h"
#include "my_splitting_filter_c.h"
#include <stdlib.h>
int my_WebrtcAGC(int16_t *data_in,int16_t *data_out,size_t kSamples,size_t sample_rate) {

	int16_t *pCurIn = data_in;
	int16_t *pCurOut = data_out;

	// AGC initialize
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
	// initialize the splitting filter 
	mSplittingFilter *splitting_filter_c = SplittingFilter_Create(sample_rate);

	// 
	int32_t outMicLevel = 0;
	int32_t inMicLevel = 0;
	int16_t echo = 0;
	uint8_t saturationWarning = 0;

	const size_t nFrames = sample_rate / 100;		// 160 320 480

	int16_t tmp_in[3][160]{};
	int16_t tmp_out[3][160]{};
	int16_t *data_in_band[3];
	int16_t *data_out_band[3];

	data_in_band[0] = tmp_in[0];
	data_out_band[0] = tmp_out[0];
	//for (size_t kBands = 0; kBands < 3; kBands++)
	//{
	//	data_in_band[kBands] = (int16_t*)calloc(160, sizeof(int16_t));
	//	data_out_band[kBands] = (int16_t*)calloc(160, sizeof(int16_t));
	//}

	for (size_t n = 0; n < kSamples / nFrames; n++)
	{
		if (nFrames == 160)
		{
			// 16000 hz
			memcpy(pCurIn,data_in_band[0],sizeof(int16_t)*nFrames);
			int agcError = WebRtcAgc_Process(hAGC, data_in_band, 1, nFrames, data_out_band, inMicLevel, &outMicLevel, echo, &saturationWarning);
			if (agcError != 0)
			{
				printf("Agc processes fail\n");
				return 0;
			}
			memcpy(pCurOut, data_out_band, nFrames * sizeof(int16_t));
		}
		else if (nFrames == 320)
		{
			SplittingFilter_Analysis_s16(splitting_filter_c, pCurIn, data_in_band);
			int agcError = WebRtcAgc_Process(hAGC, data_in_band, 2, nFrames, data_out_band, inMicLevel, &outMicLevel, echo, &saturationWarning);
			if (agcError != 0)
			{
				printf("Agc processes fail\n");
				return 0;
			}
			SplittingFilter_Synthesis_s16(splitting_filter_c, data_out_band, pCurOut);
		}
		else if (nFrames == 480)
		{
			SplittingFilter_Analysis_s16(splitting_filter_c, pCurIn, data_in_band);
			int agcError = WebRtcAgc_Process(hAGC, data_in_band, 3, nFrames, data_out_band, inMicLevel, &outMicLevel, echo, &saturationWarning);
			if (agcError != 0)
			{
				printf("Agc processes fail\n");
				return 0;
			}
			SplittingFilter_Synthesis_s16(splitting_filter_c, data_out_band, pCurOut);

		}
		pCurOut += n;
		pCurIn += n;
	}
}


