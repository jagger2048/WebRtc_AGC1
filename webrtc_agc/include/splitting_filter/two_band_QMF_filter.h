#pragma once
#ifdef USING_WEBRTC_SPL
#include "signal_processing_library.h"
#endif // USING_WEBRTC_SPL
#include <stdlib.h>
#include <stdint.h>

#include "spl_inl.h"

#ifndef WEBRTC_SPL_SCALEDIFF32
// C + the 32 most significant bits of A * B
#define WEBRTC_SPL_SCALEDIFF32(A, B, C) \
  (C + (B >> 16) * A + (((uint32_t)(B & 0x0000FFFF) * A) >> 16))

#endif // !WEBRTC_SPL_SCALEDIFF32


///////////////////////////////////////////////////////////////////////////////////////////////
// WebRtcSpl_AllPassQMF(...)
//
// Allpass filter used by the analysis and synthesis parts of the QMF filter.
//
// Input:
//    - in_data             : Input data sequence (Q10)
//    - data_length         : Length of data sequence (>2)
//    - filter_coefficients : Filter coefficients (length 3, Q16)
//
// Input & Output:
//    - filter_state        : Filter state (length 6, Q10).
//
// Output:
//    - out_data            : Output data sequence (Q10), length equal to
//                            |data_length|
//
void WebRtcSpl_AllPassQMF(int32_t* in_data, size_t data_length,
	int32_t* out_data, const uint16_t* filter_coefficients,
	int32_t* filter_state);

void WebRtcSpl_AnalysisQMF(const int16_t* in_data,
	size_t in_data_length,
	int16_t* low_band,
	int16_t* high_band,
	int32_t* filter_state1,
	int32_t* filter_state2);

void WebRtcSpl_SynthesisQMF(const int16_t* low_band,
	const int16_t* high_band,
	size_t band_length,
	int16_t* out_data,
	int32_t* filter_state1,
	int32_t* filter_state2);

