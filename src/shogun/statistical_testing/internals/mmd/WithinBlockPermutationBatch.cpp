/*
 * Restructuring Shogun's statistical hypothesis testing framework.
 * Copyright (C) 2014  Soumyajit De
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:/www.gnu.org/licenses/>.
 */

#include <array>
#include <shogun/io/SGIO.h>
#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/GPUMatrix.h>
#include <shogun/mathematics/Math.h>
#include <shogun/statistical_testing/MMD.h>
#include <shogun/statistical_testing/internals/mmd/WithinBlockPermutationBatch.h>
#include <shogun/statistical_testing/internals/mmd/BiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedIncomplete.h>

using namespace shogun;
using namespace internal;
using namespace mmd;

struct WithinBlockPermutationBatch::terms_t
{
	std::array<float32_t, 3> term{};
	std::array<float32_t, 3> diag{};
};

WithinBlockPermutationBatch::WithinBlockPermutationBatch(index_t nx, index_t ny, index_t N, EStatisticType type)
: n_x(nx), n_y(ny), num_null_samples(N), stype(type)
{
	SG_SDEBUG("number of samples are %d and %d!\n", n_x, n_y);
	SG_SDEBUG("number of null samples is %d!\n", num_null_samples);
	permuted_inds=SGVector<index_t>(n_x+n_y);
	inverted_permuted_inds.resize(num_null_samples);
	for (auto i=0; i<num_null_samples; ++i)
		inverted_permuted_inds[i].resize(permuted_inds.vlen);
}

void WithinBlockPermutationBatch::add_term(terms_t& t, float32_t val, index_t i, index_t j)
{
	if (i<n_x && j<n_x && i<=j)
	{
		SG_SDEBUG("Adding Kernel(%d,%d)=%f to term_0!\n", i, j, val);
		t.term[0]+=val;
		if (i==j)
			t.diag[0]+=val;
	}
	else if (i>=n_x && j>=n_x && i<=j)
	{
		SG_SDEBUG("Adding Kernel(%d,%d)=%f to term_1!\n", i, j, val);
		t.term[1]+=val;
		if (i==j)
			t.diag[1]+=val;
	}
	else if (i>=n_x && j<n_x)
	{
		SG_SDEBUG("Adding Kernel(%d,%d)=%f to term_2!\n", i, j, val);
		t.term[2]+=val;
		if (i-n_x==j)
			t.diag[2]+=val;
	}
}

SGVector<float32_t> WithinBlockPermutationBatch::operator()(SGMatrix<float32_t> km)
{
	SG_SDEBUG("Entering!\n");

	for (auto k=0; k<num_null_samples; ++k)
	{
		std::iota(permuted_inds.vector, permuted_inds.vector+permuted_inds.vlen, 0);
		CMath::permute(permuted_inds);
		for (auto i=0; i<permuted_inds.vlen; ++i)
			inverted_permuted_inds[k][permuted_inds[i]]=i;
	}

	SGVector<float32_t> result(num_null_samples);
#pragma omp parallel for
	for (auto k=0; k<num_null_samples; ++k)
	{
		terms_t t;
		for (auto j=0; j<n_x+n_y; ++j)
		{
			for (auto i=0; i<n_x+n_y; ++i)
				add_term(t, km(i, j), inverted_permuted_inds[k][i], inverted_permuted_inds[k][j]);
		}

		t.term[0]=2*(t.term[0]-t.diag[0]);
		t.term[1]=2*(t.term[1]-t.diag[1]);
		SG_SDEBUG("term_0 sum (without diagonal) = %f!\n", t.term[0]);
		SG_SDEBUG("term_1 sum (without diagonal) = %f!\n", t.term[1]);
		if (stype!=EStatisticType::BIASED_FULL)
		{
			t.term[0]/=n_x*(n_x-1);
			t.term[1]/=n_y*(n_y-1);
		}
		else
		{
			t.term[0]+=t.diag[0];
			t.term[1]+=t.diag[1];
			SG_SDEBUG("term_0 sum (with diagonal) = %f!\n", t.term[0]);
			SG_SDEBUG("term_1 sum (with diagonal) = %f!\n", t.term[1]);
			t.term[0]/=n_x*n_x;
			t.term[1]/=n_y*n_y;
		}
		SG_SDEBUG("term_0 (normalized) = %f!\n", t.term[0]);
		SG_SDEBUG("term_1 (normalized) = %f!\n", t.term[1]);

		SG_SDEBUG("term_2 sum (with diagonal) = %f!\n", t.term[2]);
		if (stype==EStatisticType::UNBIASED_INCOMPLETE)
		{
			t.term[2]-=t.diag[2];
			SG_SDEBUG("term_2 sum (without diagonal) = %f!\n", t.term[2]);
			t.term[2]/=n_x*(n_x-1);
		}
		else
			t.term[2]/=n_x*n_y;
		SG_SDEBUG("term_2 (normalized) = %f!\n", t.term[2]);

		result[k]=t.term[0]+t.term[1]-2*t.term[2];
		SG_SDEBUG("result[%d] = %f!\n", k, result[k]);
	}
	SG_SDEBUG("Leaving!\n");
	return result;
}
