/*
 * Copyright 2014 Open Connectome Project (http://openconnecto.me)
 * Written by Da Zheng (zhengda1936@gmail.com)
 *
 * This file is part of FlashMatrix.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>

#include "mem_dense_matrix.h"
#include "mem_matrix_store.h"
#include "sparse_matrix.h"
#include "mem_vector.h"

#include "fm_utils.h"
#include "rutils.h"

using namespace fm;

/*
 * Clean up a sparse matrix.
 */
static void fm_clean_SpM(SEXP p)
{
	object_ref<sparse_matrix> *ref
		= (object_ref<sparse_matrix> *) R_ExternalPtrAddr(p);
	delete ref;
}

/*
 * Clean up a dense matrix
 */
static void fm_clean_DM(SEXP p)
{
	object_ref<dense_matrix> *ref
		= (object_ref<dense_matrix> *) R_ExternalPtrAddr(p);
	delete ref;
}

SEXP create_FMR_matrix(sparse_matrix::ptr m, const std::string &name)
{
	Rcpp::List ret;
	ret["name"] = Rcpp::String(name);
	ret["type"] = Rcpp::String("sparse");

	object_ref<sparse_matrix> *ref = new object_ref<sparse_matrix>(m);
	SEXP pointer = R_MakeExternalPtr(ref, R_NilValue, R_NilValue);
	R_RegisterCFinalizerEx(pointer, fm_clean_SpM, TRUE);
	ret["pointer"] = pointer;

	Rcpp::LogicalVector sym(1);
	sym[0] = m->is_symmetric();
	ret["sym"] = sym;

	Rcpp::NumericVector nrow(1);
	nrow[0] = m->get_num_rows();
	ret["nrow"] = nrow;

	Rcpp::NumericVector ncol(1);
	ncol[0] = m->get_num_cols();
	ret["ncol"] = ncol;

	return ret;
}

SEXP create_FMR_matrix(dense_matrix::ptr m, const std::string &name)
{
	Rcpp::List ret;
	ret["name"] = Rcpp::String(name);
	ret["type"] = Rcpp::String("dense");

	object_ref<dense_matrix> *ref = new object_ref<dense_matrix>(m);
	SEXP pointer = R_MakeExternalPtr(ref, R_NilValue, R_NilValue);
	R_RegisterCFinalizerEx(pointer, fm_clean_SpM, TRUE);
	ret["pointer"] = pointer;

	Rcpp::NumericVector nrow(1);
	nrow[0] = m->get_num_rows();
	ret["nrow"] = nrow;

	Rcpp::NumericVector ncol(1);
	ncol[0] = m->get_num_cols();
	ret["ncol"] = ncol;

	return ret;
}

SEXP create_FMR_vector(detail::vec_store::const_ptr vec, const std::string &name)
{
	detail::matrix_store::const_ptr mat = vec->conv2mat(vec->get_length(),
			1, false);
	return create_FMR_vector(mem_dense_matrix::create(
				detail::mem_matrix_store::cast(mat)), name);
}

SEXP create_FMR_vector(dense_matrix::ptr m, const std::string &name)
{
	Rcpp::List ret;
	ret["name"] = Rcpp::String(name);
	ret["type"] = Rcpp::String("vector");

	object_ref<dense_matrix> *ref = new object_ref<dense_matrix>(m);
	SEXP pointer = R_MakeExternalPtr(ref, R_NilValue, R_NilValue);
	R_RegisterCFinalizerEx(pointer, fm_clean_DM, TRUE);
	ret["pointer"] = pointer;

	Rcpp::NumericVector len(1);
	if (m->get_num_cols() == 1)
		len[0] = m->get_num_rows();
	else
		len[0] = m->get_num_cols();
	ret["len"] = len;
	return ret;
}

vector::ptr get_vector(const Rcpp::List &vec)
{
	assert(is_vector(vec));
	object_ref<dense_matrix> *ref
		= (object_ref<dense_matrix> *) R_ExternalPtrAddr(vec["pointer"]);
	dense_matrix::ptr mat = ref->get_object();
	// This should be a column matrix.
	assert(mat->store_layout == matrix_layout_t::L_COL
			&& mat->get_num_cols() == 1);
	const detail::mem_col_matrix_store &col_mat
		= dynamic_cast<const detail::mem_col_matrix_store &>(mat->get_data());
	detail::vec_store::const_ptr store = col_mat.get_col_vec(0);
	assert(store);
	return mem_vector::create(detail::mem_vec_store::cast(store));
}
