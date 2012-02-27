#ifndef __WORKLOAD_H__
#define __WORKLOAD_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHUNK_SLOTS 1024

class workload_gen
{
public:
	virtual off_t next_offset() = 0;
	virtual bool has_next() = 0;
};

class rand_permute
{
	off_t *offset;
	long num;
public:
	rand_permute(long num, int stride) {
		offset = (off_t *) valloc(num * sizeof(off_t));
		for (int i = 0; i < num; i++) {
			offset[i] = ((off_t) i) * stride;
		}

		for (int i = num - 1; i >= 1; i--) {
			int j = random() % i;
			off_t tmp = offset[j];
			offset[j] = offset[i];
			offset[i] = tmp;
		}
	}

	~rand_permute() {
		free(offset);
	}

	off_t get_offset(long idx) const {
		return offset[idx];
	}
};

class stride_workload: public workload_gen
{
	long first;	// the first entry
	long last;	// the last entry but it's not included in the range
	long curr;	// the current location
	long num;	// the number of entries we have visited
	int stride;
	int entry_size;
public:
	stride_workload(long first, long last, int entry_size) {
		this->first = first;
		this->last = last;
		curr = first;
		num = 0;
		this->entry_size = entry_size;
		stride = PAGE_SIZE / entry_size;
	}

	off_t next_offset() {
		off_t ret = curr;
		num++; 
		
		/*
		 * we stride with PAGE_SIZE.
		 * When we reach the end of the range,
		 * we start over but move one ahead from the last startover.
		 */
		curr += stride;
		if (curr >= last) {
			curr = first + (curr & (stride - 1));
			curr++;
		}
		ret *= entry_size;
		return ret;
	}

	bool has_next() {
		return num < (last - first);
	}
};

class local_rand_permute_workload: public workload_gen
{
	long start;
	long end;
	static const rand_permute *permute;
public:
	local_rand_permute_workload(long num, int stride, long start, long end) {
		if (permute == NULL) {
			permute = new rand_permute(num, stride);
		}
		this->start = start;
		this->end = end;
	}

	~local_rand_permute_workload() {
		if (permute) {
			delete permute;
			permute = NULL;
		}
	}

	off_t next_offset() {
		if (start >= end)
			return -1;
		return permute->get_offset(start++);
	}

	bool has_next() {
		return start < end;
	}
};

/* this class reads workload from a file. */
class file_workload: public workload_gen
{
	static off_t *offsets;

	long curr;
	long end;

	long swap_bytesl(long num) {
		long res;

		char *src = (char *) &num;
		char *dst = (char *) &res;
		for (unsigned int i = 0; i < sizeof(long); i++) {
			dst[sizeof(long) - 1 - i] = src[i];
		}
		return res;
	}

public:
	file_workload(const std::string &file, int nthreads) {
		static off_t file_size;
		static int remainings;
		static int shift = 0;
		static long start;
		static long end = 0;

		if (offsets == NULL) {
			int fd = open(file.c_str(), O_RDONLY);
			if (fd < 0) {
				perror("open");
				exit(1);
			}
			printf("%s's fd is %d\n", file.c_str(), fd);

			/* get the file size */
			struct stat stats;
			if (fstat(fd, &stats) < 0) {
				perror("fstat");
				exit(1);
			}
			file_size = stats.st_size;
			remainings = file_size / sizeof(off_t) % nthreads;
			assert(file_size % sizeof(off_t) == 0);

			offsets = (off_t *) malloc(file_size);
			/* read data of the file to a buffer */
			char *buf = (char *) offsets;
			long size = file_size;
			while (size > 0) {
				ssize_t ret = read(fd, buf, size);
				if (ret < 0) {
					perror("read");
					exit(1);
				}
				buf += ret;
				size -= ret;
			}
			close(fd);
		}

		/* the range in `offsets' */
		start = end;
		end = start + file_size / sizeof(off_t) / nthreads + (shift < remainings);
		this->curr = start;
		this->end = end;
		printf("start at %ld end at %ld\n", curr, end);
	}

	~file_workload() {
		free(offsets);
	}

	off_t next_offset() {
		/*
		 * the data in the file is generated by a Java program,
		 * its byte order is different from Intel architectures.
		 */
		return swap_bytesl(offsets[curr++]);
	}

	bool has_next() {
		return curr < end;
	}
};
off_t *file_workload::offsets;

class rand_workload: public workload_gen
{
	long start;
	long range;
	long num;
	off_t *offsets;
public:
	rand_workload(long start, long end, int stride) {
		this->start = start;
		this->range = end - start;
		num = 0;
		offsets = (off_t *) valloc(sizeof(*offsets) * range);
		for (int i = 0; i < range; i++) {
			offsets[i] = (start + random() % range) * stride;
		}
	}

	~rand_workload() {
		free(offsets);
	}

	off_t next_offset() {
		return offsets[num++];
	}

	bool has_next() {
		return num < range;
	}
};

class workload_chunk
{
public:
	virtual bool get_workload(off_t *, int num) = 0;
};

class stride_workload_chunk: public workload_chunk
{
	long first;	// the first entry
	long last;	// the last entry but it's not included in the range
	long curr;	// the current location
	int stride;
	int entry_size;
	pthread_spinlock_t _lock;
public:
	stride_workload_chunk(long first, long last, int entry_size) {
		pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE);
		this->first = first;
		this->last = last;
		this->entry_size = entry_size;
		printf("first: %ld, last: %ld\n", first, last);
		curr = first;
		stride = PAGE_SIZE / entry_size;
	}

	~stride_workload_chunk() {
		pthread_spin_destroy(&_lock);
	}

	bool get_workload(off_t *offsets, int num) {
		long start;
		long end;

		pthread_spin_lock(&_lock);
		start = curr;
		curr += stride * num;
		end = curr;
		/*
		 * if the chunk we try to get is in the range,
		 * get the chunk. 
		 */
		if (end < last + stride)
			goto unlock;

		/*
		 * the chunk is out of the range,
		 * let's start over but move the first entry forward.
		 */
		curr = first + (curr & (stride - 1));
		curr++;
		/*
		 * if the first entry is in the second page,
		 * it means we have accessed all pages, so no more work to do.
		 */
		if (curr == first + stride) {
			pthread_spin_unlock(&_lock);
			curr = end;
			return false;
		}
		start = curr;
		curr += stride * num;
		end = curr;
unlock:
		pthread_spin_unlock(&_lock);

		for (long i = 0; start < end; i++, start += stride)
			offsets[i] = start * entry_size;
		return true;
	}
};

class balanced_workload: public workload_gen
{
	off_t offsets[CHUNK_SLOTS];
	int curr;
	static workload_chunk *chunks;
public:
	balanced_workload(workload_chunk *chunks) {
		memset(offsets, 0, sizeof(offsets));
		curr = CHUNK_SLOTS;
		this->chunks = chunks;
	}

	~balanced_workload() {
		if (chunks) {
			delete chunks;
			chunks = NULL;
		}
	}

	off_t next_offset() {
		return offsets[curr++];
	}

	bool has_next() {
		if (curr < CHUNK_SLOTS)
			return true;
		else {
			bool ret = chunks->get_workload(offsets, CHUNK_SLOTS);
			curr = 0;
			return ret;
		}
	}
};
workload_chunk *balanced_workload::chunks;

#endif
