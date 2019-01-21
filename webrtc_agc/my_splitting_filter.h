#pragma once
/*
	This calss wraps the webrtc splitting filter method.
*/
#include <cstring>
#include <memory>
#include <vector>

#include "splitting_filter.h"
#include "three_band_filter_bank.h"
#include "signal_processing_library.h"
#include "audio_util.h"

struct TwoBandsStates {
	TwoBandsStates() {
		memset(analysis_state1, 0, sizeof(analysis_state1));
		memset(analysis_state2, 0, sizeof(analysis_state2));
		memset(synthesis_state1, 0, sizeof(synthesis_state1));
		memset(synthesis_state2, 0, sizeof(synthesis_state2));
	}

	static const int kStateSize = 6;
	int analysis_state1[kStateSize];
	int analysis_state2[kStateSize];
	int synthesis_state1[kStateSize];
	int synthesis_state2[kStateSize];
};
//class AudioBuffer
class mySplittingFilter
{
	// TwoBandsFilter can work at 64khz
public:
	mySplittingFilter(size_t sample_rate) {
		if (sample_rate == 32000)
		{
			sample_rate_ = sample_rate;
		}
		else if (sample_rate == 48000)
		{
			sample_rate_ = 48000;
			three_bands_filter_48k = new ThreeBandFilterBank(480);
		}
	};
	int Analysis(const int16_t *data, int16_t* const* bands) {
		if (sample_rate_ == 32000)
		{
			WebRtcSpl_AnalysisQMF(data, 320, bands[0], bands[1], TwoBands.analysis_state1, TwoBands.analysis_state2);
		}
		else if (sample_rate_ == 48000)
		{
			// std::cout << "We don't suggest this process in 48khz, using flaot format function instead.\n";
			float data_in_float[480]{};
			float bands_p[3][160] = {};
			float *data_out_float[3]{ bands_p[0],bands_p[1],bands_p[2] };	// a block of continuous memory is sugested 
			S16ToFloat(data, 480, data_in_float);
			three_bands_filter_48k->Analysis(data_in_float, 480, data_out_float);
			FloatToS16(data_out_float[0], 160, bands[0]);
			FloatToS16(data_out_float[1], 160, bands[1]);
			FloatToS16(data_out_float[2], 160, bands[2]);
		}
	
		return 0;
	}
	int Synthesis(const int16_t* const* bands, int16_t *data) {
		//const float *data, float* const* bands
		if (sample_rate_ == 32000)
		{

			WebRtcSpl_SynthesisQMF(bands[0], bands[1], 160, data, TwoBands.synthesis_state1, TwoBands.synthesis_state2);
		}
		else if (sample_rate_ == 48000) {
			// it's better to use float format func instead of int16_t
			float tmp[3][160] = {};
			//float *data_in_float[3]{ new float[160]{},new float[160]{},new float[160]{} };
			float *data_in_float[3]{ tmp[0],tmp[1],tmp[2] };
			float data_out_float[480]{};
			S16ToFloat(bands[0], 160, data_in_float[0]);	// |bands|'s memory may not be continous,it's better to 
			S16ToFloat(bands[1], 160, data_in_float[1]);	// convert it from int16_t into float band by band.
			S16ToFloat(bands[2], 160, data_in_float[2]);
			three_bands_filter_48k->Synthesis(data_in_float, 160, data_out_float);
			FloatToS16(data_out_float, 480, data);
		}
		return 0;
	};
	int Analysis(const float *data, float* const* bands) {
		// data:  the pointer of the input frame, 32k is 320 48k is 480;
		// bands: a 2-d array, 32k: [ [band1][band2] ], 48k:[ [band1] [band2] [band2]]
		//			the length of per bands is 160
		if (sample_rate_ == 32000)
		{
			int16_t data_out_int16[2][160]{ };
			int16_t data_in_int16[320]{};
			FloatToS16(data, 320, data_in_int16);
			// analysis
			WebRtcSpl_AnalysisQMF(data_in_int16, 320, data_out_int16[0], data_out_int16[1], TwoBands.analysis_state1, TwoBands.analysis_state2);
			S16ToFloat(data_out_int16[0], 160, bands[0]);
			S16ToFloat(data_out_int16[1], 160, bands[1]);
		}
		else if (sample_rate_ == 48000) {
			three_bands_filter_48k->Analysis(data, 480, bands);
		}
		return 0;
	};
	int Synthesis(const float* const* bands, float *data) {
		if (sample_rate_ == 32000)
		{
			int16_t data_in_int16[2][160]{};
			int16_t data_out_int16[320]{};
			FloatToS16(bands[0], 320, data_in_int16[0]);
			WebRtcSpl_SynthesisQMF(data_in_int16[0], data_in_int16[1], 160, data_out_int16, TwoBands.synthesis_state1, TwoBands.synthesis_state2);
			S16ToFloat(data_out_int16, 320, data);
		}
		else if (sample_rate_ == 48000) {
			three_bands_filter_48k->Synthesis(bands, 160, data);
		}
		return 0;
	};

	virtual ~mySplittingFilter();
private:
	size_t sample_rate_ = 32000;
	size_t num_bands_ = 1;

	TwoBandsStates TwoBands;
	ThreeBandFilterBank *three_bands_filter_48k = 0;
	// audio buffer manager.
	void update();
	float * data_float_ = 0;
	int16_t * data_int16_ = 0;
	int16_t * bands_[3] = {};
};

mySplittingFilter::~mySplittingFilter()
{
}
