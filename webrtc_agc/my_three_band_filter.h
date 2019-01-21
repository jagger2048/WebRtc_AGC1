#pragma once
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "my_sparse_fir_fliter.h"

#ifndef M_PI
#define M_PI 3.141592654
#endif // !M_PI

const size_t kNumBands = 3;
const size_t kSparsity = 4;


typedef struct ThreeBandFilter
{
	float* in_buffer_;
	size_t buffer_len_;
	float* out_buffer_;

	float dct_modulation_[kSparsity*kNumBands][kNumBands];	// [kNumBands]
	mSparseFIRFilter *pAnalysis[kNumBands * kSparsity];		
	mSparseFIRFilter *pSynthesis[kNumBands * kSparsity];
};

// Factors to take into account when choosing |kNumCoeffs|:
//   1. Higher |kNumCoeffs|, means faster transition, which ensures less
//      aliasing. This is especially important when there is non-linear
//      processing between the splitting and merging.
//   2. The delay that this filter bank introduces is
//      |kNumBands| * |kSparsity| * |kNumCoeffs| / 2, so it increases linearly
//      with |kNumCoeffs|.
//   3. The computation complexity also increases linearly with |kNumCoeffs|.
const size_t kNumCoeffs = 4;

// The Matlab code to generate these |kLowpassCoeffs| is:
//
// N = kNumBands * kSparsity * kNumCoeffs - 1;
// h = fir1(N, 1 / (2 * kNumBands), kaiser(N + 1, 3.5));
// reshape(h, kNumBands * kSparsity, kNumCoeffs);
//
// Because the total bandwidth of the lower and higher band is double the middle
// one (because of the spectrum parity), the low-pass prototype is half the
// bandwidth of 1 / (2 * |kNumBands|) and is then shifted with cosine modulation
// to the right places.
// A Kaiser window is used because of its flexibility and the alpha is set to
// 3.5, since that sets a stop band attenuation of 40dB ensuring a fast
// transition.
const float kLowpassCoeffs[kNumBands * kSparsity][kNumCoeffs] = {
	{ -0.00047749f, -0.00496888f, +0.16547118f, +0.00425496f },
{ -0.00173287f, -0.01585778f, +0.14989004f, +0.00994113f },
{ -0.00304815f, -0.02536082f, +0.12154542f, +0.01157993f },
{ -0.00383509f, -0.02982767f, +0.08543175f, +0.00983212f },
{ -0.00346946f, -0.02587886f, +0.04760441f, +0.00607594f },
{ -0.00154717f, -0.01136076f, +0.01387458f, +0.00186353f },
{ +0.00186353f, +0.01387458f, -0.01136076f, -0.00154717f },
{ +0.00607594f, +0.04760441f, -0.02587886f, -0.00346946f },
{ +0.00983212f, +0.08543175f, -0.02982767f, -0.00383509f },
{ +0.01157993f, +0.12154542f, -0.02536082f, -0.00304815f },
{ +0.00994113f, +0.14989004f, -0.01585778f, -0.00173287f },
{ +0.00425496f, +0.16547118f, -0.00496888f, -0.00047749f } };

int ThreeBandFilter_Init(ThreeBandFilter *handles, size_t length) {

	handles->buffer_len_ = length / kNumBands ;										// 480 / 3 = 160
	handles->in_buffer_ = (float *)malloc(sizeof(float) * handles->buffer_len_);	
	handles->out_buffer_ = (float*)malloc( sizeof(float)*handles->buffer_len_);		

	// sparsity FIR LPF init
	for (size_t i = 0; i < kSparsity; ++i) {
		for (size_t j = 0; j < kNumBands; ++j) {
			SparseFIRFilter_Init(handles->pAnalysis[i * kNumBands + j], kLowpassCoeffs[i * kNumBands + j], kNumCoeffs, kSparsity, i );
			SparseFIRFilter_Init(handles->pSynthesis[i * kNumBands + j], kLowpassCoeffs[i * kNumBands + j], kNumCoeffs, kSparsity, i);
		}
	}

	// 12 * 3
	for (size_t i = 0; i < kSparsity*kNumBands; ++i) {
		for (size_t j = 0; j < kNumBands; ++j) {
			handles->dct_modulation_[i][j] =
				2.f * cos(2.f * M_PI * i * (2.f * j + 1.f) / (float)(kSparsity*kNumBands));
		}
	}
	return 0;
}
ThreeBandFilter* ThreeBandFilter_Create(size_t length) {

	// create and malloc memory
	ThreeBandFilter *handles = (ThreeBandFilter*)malloc(sizeof(ThreeBandFilter));
	if (handles != NULL)
	{
		for (size_t i = 0; i < kSparsity*kNumBands; i++)
		{
			handles->pAnalysis[i] = (mSparseFIRFilter *)malloc(sizeof(mSparseFIRFilter));
			handles->pSynthesis[i] = (mSparseFIRFilter *)malloc(sizeof(mSparseFIRFilter));
		}
		ThreeBandFilter_Init(handles, length);
	}

	return handles;
}
// Downsamples |in| into |out|, taking one every |kNumbands| starting from
// |offset|. |split_length| is the |out| length. |in| has to be at least
// |kNumBands| * |split_length| long.
void mDownsample(const float* in,
	size_t split_length,
	size_t offset,
	float* out) {
	for (size_t i = 0; i < split_length; ++i) {
		out[i] = in[kNumBands * i + offset];
	}
}

// Upsamples |in| into |out|, scaling by |kNumBands| and accumulating it every
// |kNumBands| starting from |offset|. |split_length| is the |in| length. |out|
// has to be at least |kNumBands| * |split_length| long.
void mUpsample(const float* in, size_t split_length, size_t offset, float* out) {
	for (size_t i = 0; i < split_length; ++i) {
		out[kNumBands * i + offset] += kNumBands * in[i];
	}
}
// Modulates |in| by |dct_modulation_| and accumulates it in each of the
// |kNumBands| bands of |out|. |offset| is the index in the period of the
// cosines used for modulation. |split_length| is the length of |in| and each
// band of |out|.

void ThreeBandFilter_DownModulate(
	ThreeBandFilter *handles,
	const float* in,
	size_t split_length,
	size_t offset,
	float* const* out) {
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < split_length; ++j) {
			out[i][j] += handles->dct_modulation_[offset][i] * in[j];
		}
	}
}

// Modulates each of the |kNumBands| bands of |in| by |dct_modulation_| and
// accumulates them in |out|. |out| is cleared before starting to accumulate.
// |offset| is the index in the period of the cosines used for modulation.
// |split_length| is the length of each band of |in| and |out|.
void ThreeBandFilter_UpModulate(
	ThreeBandFilter *handles,
	const float* const* in,
	size_t split_length,
	size_t offset,
	float* out) {
	memset(out, 0, split_length * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < split_length; ++j) {
			out[j] += handles->dct_modulation_[offset][i] * in[i][j];
		}
	}
}


void ThreeBandFilter_Analysis(ThreeBandFilter *handles,const float* in,
	size_t length,
	float* const* out) {
	//RTC_CHECK_EQ(in_buffer_.size(), rtc::CheckedDivExact(length, kNumBands));
	for (size_t i = 0; i < kNumBands; ++i) {
		memset(out[i], 0, handles->buffer_len_ * sizeof(*out[i]));
	}
	for (size_t i = 0; i < kNumBands; ++i) {
		mDownsample(in, handles->buffer_len_, kNumBands - i - 1, handles->in_buffer_);
		for (size_t j = 0; j < kSparsity; ++j) {
			const size_t offset = i + j * kNumBands;
			SparseFIRFilter_Filter(
				handles->pAnalysis[offset],
				handles->in_buffer_, 
				handles->buffer_len_,
				handles->out_buffer_);
			ThreeBandFilter_DownModulate(handles,handles->out_buffer_, handles->buffer_len_, offset, out);
		}
	}
}

void ThreeBandFilter_Synthesis(ThreeBandFilter *handles, const float* const* in,
	size_t split_length,
	float* out) {
	//RTC_CHECK_EQ(in_buffer_.size(), split_length);
	memset(out, 0, kNumBands * handles->buffer_len_ * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < kSparsity; ++j) {
			const size_t offset = i + j * kNumBands;
			ThreeBandFilter_UpModulate(handles,in, handles->buffer_len_, offset, handles->in_buffer_);
			SparseFIRFilter_Filter(
				handles->pSynthesis[offset],
				handles->in_buffer_,
				handles->buffer_len_,
				handles->out_buffer_);
			mUpsample(handles->out_buffer_, handles->buffer_len_, i, out);
		}
	}
}



int ThreeBandFilter_Destory(ThreeBandFilter *handles){

	if (handles !=NULL)
	{
		free(handles->in_buffer_);
		free(handles->out_buffer_);
		for (size_t i = 0; i < kNumBands * kSparsity; i++)
		{
			SparseFIRFilter_Destory(handles->pAnalysis[i]);
			SparseFIRFilter_Destory(handles->pSynthesis[i]);
		}
		return 0;
	}
	return -1;
}