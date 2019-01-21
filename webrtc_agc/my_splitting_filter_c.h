#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <memory>

#include "signal_processing_library.h"	// two_band_qmf
#include "my_three_band_filter.h"		// three band filter
//#include "audio_util.h"					// f32 <--> s16

void f32_to_s16(const float* pIn, size_t sampleCount, int16_t* pOut )
{
	int r;
	for (size_t i = 0; i < sampleCount; ++i) {
		float x = pIn[i];
		float c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int)(c * 32767.5f);
		r = r - 32768;
		pOut[i] = (short)r;
	}
};
void s16_to_f32(const int16_t* pIn, size_t sampleCount, float* pOut)
{
	if (pOut == NULL || pIn == NULL) {
		return;
	}

	for (size_t i = 0; i < sampleCount; ++i) {
		*pOut++ = pIn[i] / 32768.0f;
	}
}

typedef struct mSplittingFilter{
	size_t sample_rate_;	// 32000,48000
	static const int kTwoBandStateSize = 6;
	int two_band_analysis_state[2][kTwoBandStateSize];
	int two_band_synthesis_state[2][kTwoBandStateSize];

	ThreeBandFilter* three_band_filter_48k;
	float data_f32[480];
	int16_t data_s16[480];
	float *three_band_f32[3];
	int16_t *three_band_s16[3];
};
mSplittingFilter* SplittingFilter_Create(size_t sample_rate) {
	
	mSplittingFilter*handles = (mSplittingFilter*)malloc(sizeof(mSplittingFilter));
	if (handles !=NULL)
	{
		handles->sample_rate_ = sample_rate;
		if (handles->sample_rate_ == 32000)
		{
			// Two band mode
			memset(handles->two_band_analysis_state, 0, sizeof(handles->two_band_analysis_state));
			memset(handles->two_band_synthesis_state, 0, sizeof(handles->two_band_synthesis_state));
		}
		else if (handles->sample_rate_ == 48000)
		{
			// Three band mode.

			handles->three_band_filter_48k = ThreeBandFilter_Create(480);
			if (handles->three_band_filter_48k == NULL)
			{
				printf("Three-band splitting filter initialize fail \n");
				free(handles);
				return nullptr;
			}
			memset(handles->data_f32,0,sizeof(handles->data_f32));
			memset(handles->data_s16,0,sizeof(handles->data_s16));
			for (size_t kBands = 0; kBands < 3; kBands++)
			{
				handles->three_band_f32[kBands] = (float*)calloc(160, sizeof(float));
				handles->three_band_s16[kBands] = (int16_t*)calloc(160, sizeof(int16_t));
			}
		}
		else
		{
			printf("Only support 32khz or 48 khz\n");
			free(handles);
			return nullptr;
		}
	}

	return handles;
}
int SplittingFilter_Analysis_s16(mSplittingFilter *handles,const int16_t *data, int16_t* const* bands) {
	if (handles->sample_rate_ == 32000)
	{
		WebRtcSpl_AnalysisQMF(
			data, 
			320, 
			bands[0], bands[1], 
			handles->two_band_analysis_state[0], handles->two_band_analysis_state[1]);
	}
	else 
	{
		//float data_in_float[480]{};
		//float bands_p[3][160] = {};
		//float *data_out_float[3]{ bands_p[0],bands_p[1],bands_p[2] };	// a block of continuous memory is sugested 
		//S16ToFloat(data, 480, data_in_float);

		s16_to_f32(data, 480, handles->data_f32);

		//float data_f32[480];
		//int16_t data_s16[480];
		//float *three_band_f32[3];
		//int16_t *three_band_s16[3];

		//ThreeBandFilter_Analysis(handles->three_band_filter_48k, data_in_float, 480, data_out_float);
		ThreeBandFilter_Analysis(handles->three_band_filter_48k, handles->data_f32, 480, handles->three_band_f32);

		for (size_t kBands = 0; kBands < 3; kBands++)
		{
			f32_to_s16(handles->three_band_f32[kBands], 160, bands[kBands]);
		}
		//FloatToS16(data_out_float[0], 160, bands[0]);
		//FloatToS16(data_out_float[1], 160, bands[1]);
		//FloatToS16(data_out_float[2], 160, bands[2]);
	}
	return 0;
}
int SplittingFilter_Synthesis_s16(mSplittingFilter *handles, const int16_t* const* bands, int16_t *data) {
	if (handles->sample_rate_ == 32000)
	{
		WebRtcSpl_SynthesisQMF(
			bands[0], bands[1], 
			160, 
			data, 
			handles->two_band_synthesis_state[0], handles->two_band_synthesis_state[0]);
	}
	else{
		// it's better to use float format func instead of int16_t
		//float tmp[3][160] = {};
		//float *data_in_float[3]{ tmp[0],tmp[1],tmp[2] };
		//float data_out_float[480]{};
		//S16ToFloat(bands[0], 160, data_in_float[0]);	// |bands|'s memory may not be continous,it's better to 
		//S16ToFloat(bands[1], 160, data_in_float[1]);	// convert it from int16_t into float band by band.
		//S16ToFloat(bands[2], 160, data_in_float[2]);

		for (size_t kBands = 0; kBands < 3; kBands++)
		{
			s16_to_f32(bands[kBands], 160, handles->three_band_f32[kBands]);
		}
		//ThreeBandFilter_Synthesis(handles->three_band_filter_48k,data_in_float, 160, data_out_float);
		ThreeBandFilter_Synthesis(handles->three_band_filter_48k, handles->three_band_f32, 160, handles->data_f32);
		f32_to_s16(handles->data_f32, 480, data);
		//FloatToS16(data_out_float, 480, data);
	}
	return 0;
}
int SplittingFilter_Destory(mSplittingFilter *handles) {
	if (handles !=NULL)
	{
		if (handles->sample_rate_ == 48000)
		{
			if (handles->three_band_filter_48k!=NULL)
			{
				free(handles->three_band_filter_48k);
				for (size_t kBands = 0; kBands < 3; kBands++)
				{
					free(handles->three_band_f32[kBands]);
					free(handles->three_band_s16[kBands]);
				}
			}

		}
	}
	return -1;
}