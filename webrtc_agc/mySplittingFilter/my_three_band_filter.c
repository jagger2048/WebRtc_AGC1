#include "my_three_band_filter.h"

int ThreeBandFilter_Init(ThreeBandFilter * handles, size_t length) {

	handles->buffer_len_ = length / kNumBands;										// 480 / 3 = 160
	handles->in_buffer_ = (float *)malloc(sizeof(float) * handles->buffer_len_);
	handles->out_buffer_ = (float*)malloc(sizeof(float)*handles->buffer_len_);

	// sparsity FIR LPF init
	for (size_t i = 0; i < kSparsity; ++i) {
		for (size_t j = 0; j < kNumBands; ++j) {
			SparseFIRFilter_Init(handles->pAnalysis[i * kNumBands + j], kLowpassCoeffs[i * kNumBands + j], kNumCoeffs, kSparsity, i);
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

ThreeBandFilter * ThreeBandFilter_Create(size_t length) {

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
void mDownsample(const float * in, size_t split_length, size_t offset, float * out) {
	for (size_t i = 0; i < split_length; ++i) {
		out[i] = in[kNumBands * i + offset];
	}
}

// Upsamples |in| into |out|, scaling by |kNumBands| and accumulating it every
// |kNumBands| starting from |offset|. |split_length| is the |in| length. |out|
// has to be at least |kNumBands| * |split_length| long.
void mUpsample(const float * in, size_t split_length, size_t offset, float * out) {
	for (size_t i = 0; i < split_length; ++i) {
		out[kNumBands * i + offset] += kNumBands * in[i];
	}
}

void ThreeBandFilter_DownModulate(ThreeBandFilter * handles, const float * in, size_t split_length, size_t offset, float * const * out) {
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
void ThreeBandFilter_UpModulate(ThreeBandFilter * handles, const float * const * in, size_t split_length, size_t offset, float * out) {
	memset(out, 0, split_length * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < split_length; ++j) {
			out[j] += handles->dct_modulation_[offset][i] * in[i][j];
		}
	}
}

void ThreeBandFilter_Analysis(ThreeBandFilter * handles, const float * in, size_t length, float * const * out) {
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
			ThreeBandFilter_DownModulate(handles, handles->out_buffer_, handles->buffer_len_, offset, out);
		}
	}
}

void ThreeBandFilter_Synthesis(ThreeBandFilter * handles, const float * const * in, size_t split_length, float * out) {
	//RTC_CHECK_EQ(in_buffer_.size(), split_length);
	memset(out, 0, kNumBands * handles->buffer_len_ * sizeof(*out));
	for (size_t i = 0; i < kNumBands; ++i) {
		for (size_t j = 0; j < kSparsity; ++j) {
			const size_t offset = i + j * kNumBands;
			ThreeBandFilter_UpModulate(handles, in, handles->buffer_len_, offset, handles->in_buffer_);
			SparseFIRFilter_Filter(
				handles->pSynthesis[offset],
				handles->in_buffer_,
				handles->buffer_len_,
				handles->out_buffer_);
			mUpsample(handles->out_buffer_, handles->buffer_len_, i, out);
		}
	}
}

int ThreeBandFilter_Destory(ThreeBandFilter * handles) {

	if (handles != NULL)
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
