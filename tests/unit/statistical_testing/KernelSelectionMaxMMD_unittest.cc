/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (W) 2012-2013 Heiko Strathmann
 * Written (w) 2016 Soumyajit De
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the Shogun Development Team.
 */

#include <shogun/base/some.h>
#include <shogun/kernel/GaussianKernel.h>
#include <shogun/kernel/CustomKernel.h>
#include <shogun/features/DenseFeatures.h>
#include <shogun/features/streaming/generators/MeanShiftDataGenerator.h>
#include <shogun/mathematics/Statistics.h>
#include <shogun/mathematics/eigen3.h>
#include <shogun/mathematics/Math.h>
#include <shogun/statistical_testing/LinearTimeMMD.h>
#include <gtest/gtest.h>

using namespace shogun;

TEST(KernelSelectionMaxMMD, perform_test_permutation_biased_full)
{
	const index_t m=20;
	const index_t n=30;
	const index_t dim=2;
	const float64_t difference=0.5;
	const index_t num_kernels=10;

	// use fixed seed
	sg_rand->set_seed(12345);

	// streaming data generator for mean shift distributions
	auto gen_p=new CMeanShiftDataGenerator(0, dim, 0);
	auto gen_q=new CMeanShiftDataGenerator(difference, dim, 0);

	// create MMD instance, convienience constructor
	auto mmd=some<CLinearTimeMMD>(gen_p, gen_q);
	mmd->set_statistic_type(EStatisticType::BIASED_FULL);
	mmd->set_num_samples_p(m);
	mmd->set_num_samples_q(n);
	mmd->set_num_blocks_per_burst(1000);

	for (auto i=0; i<num_kernels; ++i)
	{
		// shoguns kernel width is different
		float64_t sigma=(i+1)*0.5;
		float64_t sq_sigma_twice=sigma*sigma*2;
		mmd->add_kernel(new CGaussianKernel(10, sq_sigma_twice));
	}

	mmd->select_kernel(EKernelSelectionMethod::MAXIMIZE_MMD);
	auto selected_kernel=static_cast<CGaussianKernel*>(mmd->get_kernel());
	EXPECT_NEAR(selected_kernel->get_width(), 0.5, 1E-10);

	// perform test with selected kernel
	index_t num_null_samples=10;
	mmd->set_num_null_samples(num_null_samples);
	mmd->set_null_approximation_method(ENullApproximationMethod::PERMUTATION);

	// compute p-value using permutation for null distribution and
	// assert against local machine computed result
	mmd->set_statistic_type(EStatisticType::BIASED_FULL);
	float64_t p_value=mmd->compute_p_value(mmd->compute_statistic());
//	EXPECT_NEAR(p_value, 0.0, 1E-10);
}

TEST(KernelSelectionMaxMMD, perform_test_permutation_unbiased_full)
{
	const index_t m=20;
	const index_t n=30;
	const index_t dim=2;
	const float64_t difference=0.5;
	const index_t num_kernels=10;

	// use fixed seed
	sg_rand->set_seed(12345);

	// streaming data generator for mean shift distributions
	auto gen_p=new CMeanShiftDataGenerator(0, dim, 0);
	auto gen_q=new CMeanShiftDataGenerator(difference, dim, 0);

	// create MMD instance, convienience constructor
	auto mmd=some<CLinearTimeMMD>(gen_p, gen_q);
	mmd->set_statistic_type(EStatisticType::UNBIASED_FULL);
	mmd->set_num_samples_p(m);
	mmd->set_num_samples_q(n);
	mmd->set_num_blocks_per_burst(1000);

	for (auto i=0; i<num_kernels; ++i)
	{
		// shoguns kernel width is different
		float64_t sigma=(i+1)*0.5;
		float64_t sq_sigma_twice=sigma*sigma*2;
		mmd->add_kernel(new CGaussianKernel(10, sq_sigma_twice));
	}

	mmd->select_kernel(EKernelSelectionMethod::MAXIMIZE_MMD);
	auto selected_kernel=static_cast<CGaussianKernel*>(mmd->get_kernel());
	EXPECT_NEAR(selected_kernel->get_width(), 0.5, 1E-10);

	// perform test with selected kernel
	index_t num_null_samples=10;
	mmd->set_num_null_samples(num_null_samples);
	mmd->set_null_approximation_method(ENullApproximationMethod::PERMUTATION);

	// compute p-value using permutation for null distribution and
	// assert against local machine computed result
	mmd->set_statistic_type(EStatisticType::BIASED_FULL);
	float64_t p_value=mmd->compute_p_value(mmd->compute_statistic());
//	EXPECT_NEAR(p_value, 0.0, 1E-10);
}

TEST(KernelSelectionMaxMMD, perform_test_permutation_unbiased_incomplete)
{
	const index_t m=20;
	const index_t n=20;
	const index_t dim=2;
	const float64_t difference=0.5;
	const index_t num_kernels=10;

	// use fixed seed
	sg_rand->set_seed(12345);

	// streaming data generator for mean shift distributions
	auto gen_p=new CMeanShiftDataGenerator(0, dim, 0);
	auto gen_q=new CMeanShiftDataGenerator(difference, dim, 0);

	// create MMD instance, convienience constructor
	auto mmd=some<CLinearTimeMMD>(gen_p, gen_q);
	mmd->set_statistic_type(EStatisticType::UNBIASED_INCOMPLETE);
	mmd->set_num_samples_p(m);
	mmd->set_num_samples_q(n);
	mmd->set_num_blocks_per_burst(1000);

	for (auto i=0; i<num_kernels; ++i)
	{
		// shoguns kernel width is different
		float64_t sigma=(i+1)*0.5;
		float64_t sq_sigma_twice=sigma*sigma*2;
		mmd->add_kernel(new CGaussianKernel(10, sq_sigma_twice));
	}

	mmd->select_kernel(EKernelSelectionMethod::MAXIMIZE_MMD);
	auto selected_kernel=static_cast<CGaussianKernel*>(mmd->get_kernel());
	EXPECT_NEAR(selected_kernel->get_width(), 0.5, 1E-10);

	// perform test with selected kernel
	index_t num_null_samples=10;
	mmd->set_num_null_samples(num_null_samples);
	mmd->set_null_approximation_method(ENullApproximationMethod::PERMUTATION);

	// compute p-value using permutation for null distribution and
	// assert against local machine computed result
	mmd->set_statistic_type(EStatisticType::BIASED_FULL);
	float64_t p_value=mmd->compute_p_value(mmd->compute_statistic());
//	EXPECT_NEAR(p_value, 0.0, 1E-10);
}
