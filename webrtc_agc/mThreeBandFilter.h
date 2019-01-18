#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.141592654
#endif // !M_PI

const size_t kNumBands = 3;
const size_t kSparsity = 4;

typedef struct ThreeBandFilter
{
	float* in_buffer_;
	float* out_buffer_;
	SparseFIRFilter ** analysis_filters_;
	SparseFIRFilter ** synthesis_filters_;
	float dct_modulation_[kSparsity][kNumBands];// [kNumBands]
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

ThreeBandFilter * ThreeBandFilter_Init( unsigned int length) {

	ThreeBandFilter *handles = (ThreeBandFilter*)malloc(sizeof(ThreeBandFilter));

	handles->in_buffer_ = (float *)malloc(sizeof(float) * (length / kNumBands) ); //  (length / kNumBands),
	handles->out_buffer_ = (float*)malloc( sizeof(*(handles->in_buffer_)) );//out_buffer_(in_buffer_.size());

	// sparsity FIR LPF init
	for (size_t i = 0; i < kSparsity; ++i) {
		for (size_t j = 0; j < kNumBands; ++j) {
			analysis_filters_.push_back(
				std::unique_ptr<SparseFIRFilter>(new SparseFIRFilter(
					kLowpassCoeffs[i * kNumBands + j], kNumCoeffs, kSparsity, i)));
			synthesis_filters_.push_back(
				std::unique_ptr<SparseFIRFilter>(new SparseFIRFilter(
					kLowpassCoeffs[i * kNumBands + j], kNumCoeffs, kSparsity, i)));
		}
	}


	// 
	for (size_t i = 0; i < kSparsity; ++i) {
		for (size_t j = 0; j < kNumBands; ++j) {
			handles->dct_modulation_[i][j] =
				2.f * cos(2.f * M_PI * i * (2.f * j + 1.f) / (float)kSparsity);
		}
	}
	return handles;
}

// Downsamples |in| into |out|, taking one every |kNumbands| starting from
// |offset|. |split_length| is the |out| length. |in| has to be at least
// |kNumBands| * |split_length| long.
void Downsample(const float* in,
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
void Upsample(const float* in, size_t split_length, size_t offset, float* out) {
	for (size_t i = 0; i < split_length; ++i) {
		out[kNumBands * i + offset] += kNumBands * in[i];
	}
}

void ThreeBandFilter_Analysis(const float* in,
	size_t length,
	float* const* out) {
	//RTC_CHECK_EQ(in_buffer_.size(), rtc::CheckedDivExact(length, kNumBands));
	for (size_t i = 0; i < kNumBands; ++i) {
		memset(out[i], 0, in_buffer_.size() * sizeof(*out[i]));
	}
	for (size_t i = 0; i < kNumBands; ++i) {
		Downsample(in, in_buffer_.size(), kNumBands - i - 1, &in_buffer_[0]);
		for (size_t j = 0; j < kSparsity; ++j) {
			const size_t offset = i + j * kNumBands;
			analysis_filters_[offset]->Filter(&in_buffer_[0], in_buffer_.size(),
				&out_buffer_[0]);
			DownModulate(&out_buffer_[0], out_buffer_.size(), offset, out);
		}
	}
}

void ThreeBandFilter_Synthesis(const float* const* in,
	size_t split_length,
	float* out) {
	//RTC_CHECK_EQ(in_buffer_.size(), split_length);
	memset(out, 0, kNumBands * in_buffer_.size() * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < kSparsity; ++j) {
			const size_t offset = i + j * kNumBands;
			UpModulate(in, in_buffer_.size(), offset, &in_buffer_[0]);
			synthesis_filters_[offset]->Filter(&in_buffer_[0], in_buffer_.size(),
				&out_buffer_[0]);
			Upsample(&out_buffer_[0], out_buffer_.size(), i, out);
		}
	}
}

// Modulates |in| by |dct_modulation_| and accumulates it in each of the
// |kNumBands| bands of |out|. |offset| is the index in the period of the
// cosines used for modulation. |split_length| is the length of |in| and each
// band of |out|.
void ThreeBandFilter_DownModulate(const float* in,
	size_t split_length,
	size_t offset,
	float* const* out) {
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < split_length; ++j) {
			out[i][j] += dct_modulation_[offset][i] * in[j];
		}
	}
}

// Modulates each of the |kNumBands| bands of |in| by |dct_modulation_| and
// accumulates them in |out|. |out| is cleared before starting to accumulate.
// |offset| is the index in the period of the cosines used for modulation.
// |split_length| is the length of each band of |in| and |out|.
void ThreeBandFilter_UpModulate(const float* const* in,
	size_t split_length,
	size_t offset,
	float* out) {
	memset(out, 0, split_length * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < split_length; ++j) {
			out[j] += dct_modulation_[offset][i] * in[i][j];
		}
	}
}