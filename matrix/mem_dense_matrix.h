#ifndef __MEM_DENSE_MATRIX_H__
#define __MEM_DENSE_MATRIX_H__

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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "common.h"
#include "dense_matrix.h"
#include "bulk_operate.h"
#include "matrix_config.h"
#include "matrix_header.h"

namespace fm
{

class mem_dense_matrix: public dense_matrix
{
protected:
	mem_dense_matrix(size_t nrow, size_t ncol,
			const scalar_type &type): dense_matrix(nrow, ncol, type, true) {
	}

	virtual bool verify_inner_prod(const dense_matrix &m,
		const bulk_operate &left_op, const bulk_operate &right_op) const;
public:
	typedef std::shared_ptr<mem_dense_matrix> ptr;

	static ptr cast(dense_matrix::ptr m);

	virtual const char *get(size_t row, size_t col) const = 0;

	/*
	 * We need to provide a set of serial versions of the methods below.
	 * The EM matrix uses customized threads to parallelize the computation
	 * and overlap computation and I/O, and uses the IM matrix to perform
	 * computation. Therefore, the IM matrix need to expose the interface
	 * for serial implementations of the methods.
	 */

	virtual void serial_reset_data() = 0;
	virtual void serial_set_data(const set_operate &op) = 0;

	virtual dense_matrix::ptr serial_inner_prod(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op) const = 0;
};

class mem_row_dense_matrix;
class mem_col_dense_matrix;

/*
 * This class defines an in-memory dense matrix with data organized in rows.
 */
class mem_row_dense_matrix: public mem_dense_matrix
{
	struct deleter {
		void operator()(char *p) const{
			free(p);
		}
	};

	std::shared_ptr<char> data;

	void inner_prod_wide(const dense_matrix &m, const bulk_operate &left_op,
			const bulk_operate &right_op, mem_row_dense_matrix &res) const;
	void inner_prod_tall(const dense_matrix &m, const bulk_operate &left_op,
			const bulk_operate &right_op, mem_row_dense_matrix &res) const;
	void serial_inner_prod_wide(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op,
			mem_row_dense_matrix &res) const;
	void serial_inner_prod_tall(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op,
			mem_row_dense_matrix &res) const;
	virtual bool verify_inner_prod(const dense_matrix &m,
		const bulk_operate &left_op, const bulk_operate &right_op) const;

	mem_row_dense_matrix(size_t nrow, size_t ncol,
			const scalar_type &type): mem_dense_matrix(nrow, ncol, type) {
		if (nrow * ncol > 0) {
			data = std::shared_ptr<char>(
					(char *) memalign(PAGE_SIZE, nrow * ncol * type.get_size()),
					deleter());
			assert(data);
		}
	}

	std::shared_ptr<mem_col_dense_matrix> t_mat;
	/*
	 * This method returns a column-wise matrix on the same data, so we can
	 * use the column-wise matrix to access the data and perform computation.
	 * None of the methods in this class can change the metadata (nrow,
	 * ncol, etc), so we can cache the column-wise matrix.
	 */
	mem_col_dense_matrix &get_t_mat();
	const mem_col_dense_matrix &get_t_mat() const;

protected:
	mem_row_dense_matrix(size_t nrow, size_t ncol, const scalar_type &type,
			std::shared_ptr<char> data): mem_dense_matrix(nrow, ncol,
				type) {
		this->data = data;
	}
public:
	typedef std::shared_ptr<mem_row_dense_matrix> ptr;

	static ptr create(size_t nrow, size_t ncol, const scalar_type &type) {
		return ptr(new mem_row_dense_matrix(nrow, ncol, type));
	}
	static ptr create(size_t nrow, size_t ncol, const scalar_type &type, FILE *f);
	static ptr cast(dense_matrix::ptr);
	static ptr cast(mem_dense_matrix::ptr);

	~mem_row_dense_matrix() {
	}

	/*
	 * This method converts this row-marjor dense matrix to a column-major
	 * dense matrix. Nothing else is changed.
	 */
	std::shared_ptr<mem_col_dense_matrix> get_col_store() const;

	virtual bool write2file(const std::string &file_name) const;

	virtual dense_matrix::ptr shallow_copy() const;
	virtual dense_matrix::ptr deep_copy() const;
	virtual dense_matrix::ptr conv2(size_t nrow, size_t ncol, bool byrow) const;
	virtual dense_matrix::ptr transpose() const;

	virtual void reset_data();
	virtual void set_data(const set_operate &op);
	virtual void serial_reset_data();
	virtual void serial_set_data(const set_operate &op);

	virtual dense_matrix::ptr inner_prod(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op) const;
	virtual dense_matrix::ptr serial_inner_prod(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op) const;
	virtual bool aggregate(const bulk_operate &op, scalar_variable &res) const;
	virtual dense_matrix::ptr mapply2(const dense_matrix &m,
			const bulk_operate &op) const;
	virtual dense_matrix::ptr sapply(const bulk_uoperate &op) const;
	virtual dense_matrix::ptr apply(apply_margin margin, const arr_apply_operate &op) const;

	virtual char *get_row(size_t row) {
		return data.get() + row * get_num_cols() * get_entry_size();
	}

	virtual const char *get_row(size_t row) const {
		return data.get() + row * get_num_cols() * get_entry_size();
	}

	virtual char *get(size_t row, size_t col) {
		return get_row(row) + col * get_entry_size();
	}

	virtual const char *get(size_t row, size_t col) const {
		return get_row(row) + col * get_entry_size();
	}

	virtual matrix_layout_t store_layout() const {
		return matrix_layout_t::L_ROW;
	}

	friend class mem_col_dense_matrix;
};

/*
 * This class defines an in-memory dense matrix with data organized in columns.
 */
class mem_col_dense_matrix: public mem_dense_matrix
{
	struct deleter {
		void operator()(char *p) const{
			free(p);
		}
	};

	std::shared_ptr<char> data;

	mem_col_dense_matrix(std::shared_ptr<char> data, size_t nrow, size_t ncol,
			const scalar_type &type): mem_dense_matrix(nrow, ncol, type) {
		this->data = data;
	}

	mem_col_dense_matrix(size_t nrow, size_t ncol,
			const scalar_type &type): mem_dense_matrix(nrow, ncol, type) {
		if (nrow * ncol > 0) {
			data = std::shared_ptr<char>(
					(char *) memalign(PAGE_SIZE, nrow * ncol * type.get_size()),
					deleter());
			assert(data);
		}
	}

	// This method constructs the specified row in a preallocated array.
	// It is used internally, so the array should have the right length
	// to keep a row.
	void get_row(size_t idx, char *arr) const;

protected:
	mem_col_dense_matrix(size_t nrow, size_t ncol, const scalar_type &type,
			std::shared_ptr<char> data): mem_dense_matrix(nrow, ncol,
				type) {
		this->data = data;
	}
public:
	typedef std::shared_ptr<mem_col_dense_matrix> ptr;

	static ptr create(std::shared_ptr<char> data, size_t nrow, size_t ncol,
			const scalar_type &type) {
		return ptr(new mem_col_dense_matrix(data, nrow, ncol, type));
	}

	static ptr create(size_t nrow, size_t ncol, const scalar_type &type) {
		return ptr(new mem_col_dense_matrix(nrow, ncol, type));
	}
	static ptr create(size_t nrow, size_t ncol, const scalar_type &type, FILE *f);

	static ptr cast(dense_matrix::ptr);
	static ptr cast(mem_dense_matrix::ptr);

	~mem_col_dense_matrix() {
	}

	/*
	 * This method converts this column-marjor dense matrix to a row-major
	 * dense matrix. Nothing else is changed.
	 */
	std::shared_ptr<mem_row_dense_matrix> get_row_store() const;

	virtual bool write2file(const std::string &file_name) const;

	virtual dense_matrix::ptr shallow_copy() const;
	virtual dense_matrix::ptr deep_copy() const;
	virtual dense_matrix::ptr conv2(size_t nrow, size_t ncol, bool byrow) const;
	virtual dense_matrix::ptr transpose() const;

	virtual void reset_data();
	virtual void set_data(const set_operate &op);
	virtual void serial_reset_data();
	virtual void serial_set_data(const set_operate &op);

	virtual dense_matrix::ptr inner_prod(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op) const;
	virtual dense_matrix::ptr serial_inner_prod(const dense_matrix &m,
			const bulk_operate &left_op, const bulk_operate &right_op) const;
	virtual bool aggregate(const bulk_operate &op, scalar_variable &res) const;
	virtual dense_matrix::ptr mapply2(const dense_matrix &m,
			const bulk_operate &op) const;
	virtual dense_matrix::ptr sapply(const bulk_uoperate &op) const;
	virtual dense_matrix::ptr apply(apply_margin margin, const arr_apply_operate &op) const;

	virtual bool set_cols(const mem_col_dense_matrix &m,
			const std::vector<off_t> &idxs);
	virtual bool set_col(const char *buf, size_t size, size_t col);

	virtual dense_matrix::ptr get_cols(const std::vector<off_t> &idxs) const;

	virtual char *get_col(size_t col) {
		return data.get() + col * get_num_rows() * get_entry_size();
	}

	virtual const char *get_col(size_t col) const {
		return data.get() + col * get_num_rows() * get_entry_size();
	}

	virtual char *get(size_t row, size_t col) {
		return get_col(col) + row * get_entry_size();
	}

	virtual const char *get(size_t row, size_t col) const {
		return get_col(col) + row * get_entry_size();
	}

	virtual matrix_layout_t store_layout() const {
		return matrix_layout_t::L_COL;
	}

	friend class mem_row_dense_matrix;
};

template<class EntryType>
class type_mem_dense_matrix
{
	mem_dense_matrix::ptr m;

	type_mem_dense_matrix(size_t nrow, size_t ncol, matrix_layout_t layout) {
		if (layout == matrix_layout_t::L_COL)
			m = mem_col_dense_matrix::create(nrow, ncol,
					get_scalar_type<EntryType>());
		else if (layout == matrix_layout_t::L_ROW)
			m = mem_row_dense_matrix::create(nrow, ncol,
					get_scalar_type<EntryType>());
		else
			assert(0);
	}

	type_mem_dense_matrix(size_t nrow, size_t ncol, matrix_layout_t layout,
			const type_set_operate<EntryType> &op, bool parallel) {
		if (layout == matrix_layout_t::L_COL)
			m = mem_col_dense_matrix::create(nrow, ncol,
					get_scalar_type<EntryType>());
		else if (layout == matrix_layout_t::L_ROW)
			m = mem_row_dense_matrix::create(nrow, ncol,
					get_scalar_type<EntryType>());
		else
			assert(0);
		if (parallel)
			m->set_data(op);
		else
			m->serial_set_data(op);
	}

	type_mem_dense_matrix(mem_dense_matrix::ptr m) {
		this->m = m;
	}
public:
	typedef std::shared_ptr<type_mem_dense_matrix<EntryType> > ptr;

	static ptr create(size_t nrow, size_t ncol, matrix_layout_t layout) {
		return ptr(new type_mem_dense_matrix<EntryType>(nrow, ncol, layout));
	}

	static ptr create(size_t nrow, size_t ncol, matrix_layout_t layout,
			const type_set_operate<EntryType> &op, bool parallel = false) {
		return ptr(new type_mem_dense_matrix<EntryType>(nrow, ncol, layout,
					op, parallel));
	}

	static ptr create(mem_dense_matrix::ptr m) {
		assert(m->get_type() == get_scalar_type<EntryType>());
		return ptr(new type_mem_dense_matrix<EntryType>(m));
	}

	size_t get_num_rows() const {
		return m->get_num_rows();
	}

	size_t get_num_cols() const {
		return m->get_num_cols();
	}

	void set(size_t row, size_t col, const EntryType &v) {
		*(EntryType *) m->get(row, col) = v;
	}

	EntryType get(size_t row, size_t col) const {
		return *(EntryType *) m->get(row, col);
	}

	const mem_dense_matrix::ptr get_matrix() const {
		return m;
	}
};

typedef type_mem_dense_matrix<int> I_mem_dense_matrix;
typedef type_mem_dense_matrix<double> D_mem_dense_matrix;

template<class LeftType, class RightType, class ResType>
mem_dense_matrix::ptr multiply(mem_col_dense_matrix &m1, mem_dense_matrix &m2)
{
	basic_ops_impl<LeftType, RightType, ResType> ops;
	return mem_dense_matrix::cast(
			m1.serial_inner_prod(m2, ops.get_multiply(), ops.get_add()));
}

template<class LeftType, class RightType, class ResType>
mem_dense_matrix::ptr par_multiply(mem_col_dense_matrix &m1, mem_dense_matrix &m2)
{
	basic_ops_impl<LeftType, RightType, ResType> ops;
	return mem_dense_matrix::cast(m1.inner_prod(m2, ops.get_multiply(),
				ops.get_add()));
}

template<class LeftType, class RightType, class ResType>
typename type_mem_dense_matrix<ResType>::ptr  multiply(
		type_mem_dense_matrix<LeftType> &m1,
		type_mem_dense_matrix<RightType> &m2)
{
	basic_ops_impl<LeftType, RightType, ResType> ops;
	return type_mem_dense_matrix<ResType>::create(mem_dense_matrix::cast(
				m1.get_matrix()->serial_inner_prod(*m2.get_matrix(), ops.get_multiply(),
					ops.get_add())));
}

template<class LeftType, class RightType, class ResType>
typename type_mem_dense_matrix<ResType>::ptr par_multiply(
		type_mem_dense_matrix<LeftType> &m1,
		type_mem_dense_matrix<RightType> &m2)
{
	basic_ops_impl<LeftType, RightType, ResType> ops;
	return type_mem_dense_matrix<ResType>::create(mem_dense_matrix::cast(
				m1.get_matrix()->inner_prod(*m2.get_matrix(), ops.get_multiply(),
					ops.get_add())));
}

}

#endif
