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

#include <unordered_map>
#include <shogun/io/SGIO.h>
#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/SGVector.h>
#include <shogun/lib/GPUMatrix.h>
#include <shogun/mathematics/eigen3.h>
#include <shogun/mathematics/Math.h>
#include <shogun/statistical_testing/MMD.h>
#include <shogun/statistical_testing/internals/mmd/WithinBlockPermutation.h>
#include <shogun/statistical_testing/internals/mmd/BiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedIncomplete.h>

using namespace shogun;
using namespace internal;
using namespace mmd;

WithinBlockPermutation::WithinBlockPermutation(index_t nx, index_t ny, EStatisticType type)
: n_x(nx), n_y(ny), stype(type), terms()
{
	SG_SDEBUG("number of samples are %d and %d!\n", n_x, n_y);
	std::fill(&terms.term[0], &terms.term[2]+1, 0);
	std::fill(&terms.diag[0], &terms.diag[2]+1, 0);
}

void WithinBlockPermutation::add_term(float64_t val, index_t i, index_t j)
{
	if (i<n_x && j<n_x)
	{
		SG_SDEBUG("Adding KernelMatrix(%d,%d)=%f to term 1!\n", i, j, val);
		terms.term[0]+=val;
		if (i==j)
			terms.diag[0]+=val;
	}
	else if (i>=n_x && j>=n_x)
	{
		SG_SDEBUG("Adding KernelMatrix(%d,%d)=%f to term 2!\n", i, j, val);
		terms.term[1]+=val;
		if (i==j)
			terms.diag[1]+=val;
	}
	else if (i>=n_x && j<n_x)
	{
		SG_SDEBUG("Adding KernelMatrix(%d,%d)=%f to term 3!\n", i, j, val);
		terms.term[2]+=val;
		if (i-n_x==j)
			terms.diag[2]+=val;
	}
}

float64_t WithinBlockPermutation::operator()(SGMatrix<float64_t> km)
{
	SG_SDEBUG("Entering!\n");
	SGVector<index_t> permuted_inds(n_x+n_y);
	std::iota(permuted_inds.vector, permuted_inds.vector+permuted_inds.vlen, 0);
	CMath::permute(permuted_inds);

	std::unordered_map<index_t, index_t> inds;
	for (int i=0; i<permuted_inds.vlen; ++i)
		inds.insert(std::make_pair(permuted_inds[i], i));

	std::fill(&terms.term[0], &terms.term[2]+1, 0);
	std::fill(&terms.diag[0], &terms.diag[2]+1, 0);

	for (auto j=0; j<n_x+n_y; ++j)
	{
		for (auto i=0; i<n_x+n_y; ++i)
			add_term(km(i, j), inds.find(i)->second, inds.find(j)->second);
	}

	SG_SDEBUG("term_1 sum (with diagonal) = %f!\n", terms.term[0]);
	SG_SDEBUG("term_2 sum (with diagonal) = %f!\n", terms.term[1]);
	if (stype!=EStatisticType::BIASED_FULL)
	{
		terms.term[0]-=terms.diag[0];
		terms.term[1]-=terms.diag[1];
		SG_SDEBUG("term_1 sum (without diagonal) = %f!\n", terms.term[0]);
		SG_SDEBUG("term_2 sum (without diagonal) = %f!\n", terms.term[1]);
		terms.term[0]/=n_x*(n_x-1);
		terms.term[1]/=n_y*(n_y-1);
	}
	else
	{
		terms.term[0]/=n_x*n_x;
		terms.term[1]/=n_y*n_y;
	}
	SG_SDEBUG("term_1 (normalized) = %f!\n", terms.term[0]);
	SG_SDEBUG("term_2 (normalized) = %f!\n", terms.term[1]);

	SG_SDEBUG("term_3 sum (with diagonal) = %f!\n", terms.term[2]);
	if (stype==EStatisticType::UNBIASED_INCOMPLETE)
	{
		terms.term[2]-=terms.diag[2];
		SG_SDEBUG("term_3 sum (without diagonal) = %f!\n", terms.term[2]);
		terms.term[2]/=n_x*(n_x-1);
	}
	else
		terms.term[2]/=n_x*n_y;
	SG_SDEBUG("term_3 (normalized) = %f!\n", terms.term[2]);

	SG_SDEBUG("Leaving!\n");
	return terms.term[0]+terms.term[1]-2*terms.term[2];
}
