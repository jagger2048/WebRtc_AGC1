#pragma once 
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

typedef struct mSparseFIRFilter
{
	size_t sparsity_;
	size_t offset_;

	float * nonzero_coeffs_;
	size_t nozero_coeffs_len_;
	float * state_;
	size_t state_len_;
}mSparseFIRFilter;
int  SparseFIRFilter_Init(
	mSparseFIRFilter * handles,
	const float* nonzero_coeffs,
	size_t num_nonzero_coeffs,
	size_t sparsity,
	size_t offset);
void SparseFIRFilter_Filter(mSparseFIRFilter *handles, const float* in, size_t length, float* out);

int SparseFIRFilter_Destory(mSparseFIRFilter *handles);