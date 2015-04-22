#include <stdio.h>
#include <eigen3/Eigen/Dense>

#include "common.h"

#include "mem_dense_matrix.h"

using namespace fm;

class set_col_operate: public type_set_operate<double>
{
	size_t num_cols;
public:
	set_col_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(double *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = (row_idx + i) * num_cols + col_idx;
		}
	}
};

class set_row_operate: public type_set_operate<double>
{
	size_t num_cols;
public:
	set_row_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(double *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = row_idx * num_cols + col_idx + i;
		}
	}
};

template<class Type>
void test_eigen(size_t nrow, size_t ncol, size_t right_ncol)
{
	struct timeval start, end;

	printf("test eigen: M(%ld x %ld) * M(%ld %ld)\n", nrow, ncol, ncol, right_ncol);
	gettimeofday(&start, NULL);
	Eigen::Matrix<Type, Eigen::Dynamic, Eigen::Dynamic> eigen_m1(nrow, ncol);
#pragma omp parallel for
	for (size_t i = 0; i < nrow; i++) {
		for (size_t j = 0; j < ncol; j++)
			eigen_m1(i, j) = i * ncol + j;
	}
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input Eigen matrix\n",
			time_diff(start, end));

	Eigen::Matrix<Type, Eigen::Dynamic, Eigen::Dynamic> eigen_m2(ncol, right_ncol);
	for (size_t i = 0; i < ncol; i++) {
		for (size_t j = 0; j < right_ncol; j++)
			eigen_m2(i, j) = i * right_ncol + j;
	}

	start = end;
	Eigen::Matrix<Type, Eigen::Dynamic, Eigen::Dynamic> eigen_res = eigen_m1 * eigen_m2;
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply Eigen matrix\n",
			time_diff(start, end));
}

/*
 * This multiplies a tall column-wise matrix with a small column-wise matrix.
 */
template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MM1(size_t nrow, size_t ncol,
		size_t right_ncol)
{
	struct timeval start, end;

	printf("inner product of tall col-wise matrix: M(%ld x %ld) * M(%ld %ld)\n",
			nrow, ncol, ncol, right_ncol);
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input column matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, right_ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));

	gettimeofday(&start, NULL);
	dense_matrix::ptr res1 = m1->get_matrix()->multiply(*m2->get_matrix());
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply column matrix in parallel\n",
			time_diff(start, end));
	assert(res1->get_num_rows() == m1->get_num_rows());
	assert(res1->get_num_cols() == m2->get_num_cols());
	printf("The result matrix has %ld rows and %ld columns\n",
			res1->get_num_rows(), res1->get_num_cols());

	return type_mem_dense_matrix<Type>::create(mem_dense_matrix::cast(res1));
}

/*
 * This multiplies a tall row-wise matrix with a small column-wise matrix.
 */
template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MM1_5(size_t nrow, size_t ncol,
		size_t right_ncol)
{
	struct timeval start, end;

	printf("inner product of tall row-wise matrix: M(%ld x %ld) * M(%ld %ld)\n",
			nrow, ncol, ncol, right_ncol);
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_ROW, set_row_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input row matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, right_ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));

	gettimeofday(&start, NULL);
	dense_matrix::ptr res1 = m1->get_matrix()->multiply(*m2->get_matrix());
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply row matrix in parallel\n",
			time_diff(start, end));
	assert(res1->get_num_rows() == m1->get_num_rows());
	assert(res1->get_num_cols() == m2->get_num_cols());
	printf("The result matrix has %ld rows and %ld columns\n",
			res1->get_num_rows(), res1->get_num_cols());

	return type_mem_dense_matrix<Type>::create(mem_dense_matrix::cast(res1));
}

/*
 * This multiplies a large (tall and narrow) row-wise matrix with a small
 * column-wise matrix. It should give us the best performance.
 */
template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MM2(size_t nrow, size_t ncol,
		size_t right_ncol)
{
	struct timeval start, end;

	printf("test tall row-wise matrix (best case): M(%ld x %ld) * M(%ld %ld)\n",
			nrow, ncol, ncol, right_ncol);
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_ROW, set_row_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input row matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, right_ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));
	
	typename type_mem_dense_matrix<Type>::ptr res_m
		= type_mem_dense_matrix<Type>::create(nrow, right_ncol,
				matrix_layout_t::L_ROW);

	gettimeofday(&start, NULL);
	const detail::mem_row_matrix_store &row_m
		= (const detail::mem_row_matrix_store &) m1->get_matrix()->get_data();
	const detail::mem_col_matrix_store &col_m
		= (const detail::mem_col_matrix_store &) m2->get_matrix()->get_data();
	const detail::mem_row_matrix_store &res_row_m
		= (const detail::mem_row_matrix_store &) res_m->get_matrix()->get_data();
#pragma omp parallel for
	for (size_t i = 0; i < nrow; i++) {
		const Type *in_row = (const Type *) row_m.get_row(i);
		Type *out_row = (Type *) res_row_m.get_row(i);
		for (size_t j = 0; j < right_ncol; j++) {
			const Type *in_col = (const Type *) col_m.get_col(j);
			out_row[j] = 0;
			for (size_t k = 0; k < ncol; k++)
				out_row[j] += in_row[k] * in_col[k];
		}
	}
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply row matrix in parallel\n",
			time_diff(start, end));
	return res_m;
}

/*
 * This multiplies a large column-wise matrix with a small column-wise matrix
 * directly. So this implementation potentially generates many CPU cache misses.
 */
template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MM3(size_t nrow, size_t ncol,
		size_t right_ncol)
{
	struct timeval start, end;

	printf("test tall col-wise matrix (bad impl): M(%ld x %ld) * M(%ld %ld)\n",
			nrow, ncol, ncol, right_ncol);
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input column matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, right_ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));
	
	typename type_mem_dense_matrix<Type>::ptr res_m
		= type_mem_dense_matrix<Type>::create(nrow, right_ncol,
				matrix_layout_t::L_COL);

	gettimeofday(&start, NULL);
	const detail::mem_col_matrix_store &left_m
		= (const detail::mem_col_matrix_store &) m1->get_matrix()->get_data();
	const detail::mem_col_matrix_store &right_m
		= (const detail::mem_col_matrix_store &) m2->get_matrix()->get_data();
	const detail::mem_col_matrix_store &res_col_m
		= (const detail::mem_col_matrix_store &) res_m->get_matrix()->get_data();
#pragma omp parallel for
	for (size_t i = 0; i < nrow; i++) {
		for (size_t j = 0; j < right_ncol; j++) {
			const Type *right_col = (const Type *) right_m.get_col(j);
			Type *out_col = (Type *) res_col_m.get_col(j);
			out_col[i] = 0;
			for (size_t k = 0; k < ncol; k++) {
				const Type *left_col = (const Type *) left_m.get_col(k);
				out_col[i] += left_col[i] * right_col[k];
			}
		}
	}
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply column matrix in parallel\n",
			time_diff(start, end));
	return res_m;
}

template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MV1(size_t nrow, size_t ncol)
{
	printf("test a tall col-wise matrix: M(%ld x %ld) * v(%ld)\n",
			nrow, ncol, ncol);

	struct timeval start, end;
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_COL, set_col_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input column matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, 1,
				matrix_layout_t::L_COL, set_col_operate(ncol));

	gettimeofday(&start, NULL);
	dense_matrix::ptr res1 = m1->get_matrix()->multiply(*m2->get_matrix());
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply column matrix in parallel\n",
			time_diff(start, end));
	assert(res1->get_num_rows() == m1->get_num_rows());
	assert(res1->get_num_cols() == m2->get_num_cols());
	printf("The result matrix has %ld rows and %ld columns\n",
			res1->get_num_rows(), res1->get_num_cols());
	return type_mem_dense_matrix<Type>::create(mem_dense_matrix::cast(res1));
}

template<class Type>
typename type_mem_dense_matrix<Type>::ptr test_MV2(size_t nrow, size_t ncol)
{
	printf("test a wide row-wise matrix: M(%ld x %ld) * v(%ld)\n",
			nrow, ncol, ncol);

	struct timeval start, end;
	gettimeofday(&start, NULL);
	typename type_mem_dense_matrix<Type>::ptr m1
		= type_mem_dense_matrix<Type>::create(nrow, ncol,
				matrix_layout_t::L_ROW, set_row_operate(ncol));
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to construct input row matrix\n",
			time_diff(start, end));
	typename type_mem_dense_matrix<Type>::ptr m2
		= type_mem_dense_matrix<Type>::create(ncol, 1,
				matrix_layout_t::L_COL, set_col_operate(ncol));

	gettimeofday(&start, NULL);
	dense_matrix::ptr res1 = m1->get_matrix()->multiply(*m2->get_matrix());
	gettimeofday(&end, NULL);
	printf("It takes %.3f seconds to multiply row matrix in parallel\n",
			time_diff(start, end));
	assert(res1->get_num_rows() == m1->get_num_rows());
	assert(res1->get_num_cols() == m2->get_num_cols());
	printf("The result matrix has %ld rows and %ld columns\n",
			res1->get_num_rows(), res1->get_num_cols());
	return type_mem_dense_matrix<Type>::create(mem_dense_matrix::cast(res1));
}

template<class Type>
void check_result(typename type_mem_dense_matrix<Type>::ptr m1,
		typename type_mem_dense_matrix<Type>::ptr m2)
{
	assert(m1->get_num_rows() == m2->get_num_rows());
	assert(m1->get_num_cols() == m2->get_num_cols());
#pragma omp parallel for
	for (size_t i = 0; i < m1->get_num_rows(); i++) {
		for (size_t j = 0; j < m1->get_num_cols(); j++) {
			assert(m1->get(i, j) == m2->get(i, j));
		}
	}
}

void matrix_mul_tests()
{
	size_t nrow = 1024 * 1024 * 124;
	size_t ncol = 20;
	printf("Multiplication of a large and tall matrix and a small square matrix\n");
	test_eigen<double>(nrow, ncol, ncol);
	D_mem_dense_matrix::ptr res1;
	D_mem_dense_matrix::ptr res2;
	res1 = test_MM1<double>(nrow, ncol, ncol);
	res2 = test_MM1_5<double>(nrow, ncol, ncol);
	check_result<double>(res1, res2);
	res2 = test_MM2<double>(nrow, ncol, ncol);
	check_result<double>(res1, res2);
	res2 = test_MM3<double>(nrow, ncol, ncol);
	check_result<double>(res1, res2);
}

void matrix_vec_mul_tests()
{
	size_t nrow = 1024 * 1024 * 124;
	size_t ncol = 120;
	printf("Multiplication of a large (tall/wide) matrix and a vector\n");
	test_MV1<double>(nrow, ncol);
	test_MV2<double>(ncol, nrow);
}

int main()
{
	matrix_vec_mul_tests();
	printf("\n\n");
	matrix_mul_tests();
}
