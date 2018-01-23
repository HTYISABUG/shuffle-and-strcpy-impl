#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline __attribute__((always_inline))
void get_cycles(unsigned *high, unsigned *low)
{
	asm volatile ("CPUID\n\t"
	              "RDTSC\n\t"
	              "mov %%edx, %0\n\t"
	              "movl %%eax, %1\n\t": "=r" (*high), "=r" (*low)::"%rax","%rbx","%rcx","%rdx"
	             );
}

static inline __attribute__((always_inline))
void get_cycles_end(unsigned *high, unsigned *low)
{
	asm volatile("RDTSCP\n\t"
	             "mov %%edx, %0\n\t"
	             "mov %%eax, %1\n\t"
	             "CPUID\n\t": "=r" (*high), "=r" (*low)::"%rax","%rbx","%rcx","%rdx"
	            );
}

static inline __attribute__((always_inline))
uint64_t diff_in_cycles(unsigned high1, unsigned low1,
                        unsigned high2, unsigned low2)
{
	uint64_t start,end;
	start = (((uint64_t) high1 << 32) | low1);
	end = (((uint64_t) high2 << 32) | low2);
	return end - start;
}

void shuffle(uint16_t *array, uint16_t n)
{
	unsigned int seed = time(NULL);
	uint16_t tmp;
	int idx;

	while (n) {
		idx = rand_r(&seed) % (n--);
		tmp = array[idx];
		array[idx] = array[n];
		array[n] = tmp;
	}
}

void top_in_shuffle(uint16_t *array, uint16_t n)
{
    uint16_t *tmp, idx;
    const uint64_t round = (uint64_t)(n * log(n) + n);
    unsigned int seed = time(NULL);

    tmp = malloc(n * sizeof(uint16_t));

    for (uint64_t i = 0; i < round; i++) {
        memcpy(tmp, array, n * sizeof(uint16_t));

        idx = rand_r(&seed) % n;
        tmp[0] = array[idx];

        memcpy(&tmp[1], array, idx * sizeof(uint16_t));
        memcpy(array, tmp, n * sizeof(uint16_t));
    }

    free(tmp);
}

void riffle_shuffle(uint16_t *array, uint16_t n)
{
    uint16_t *tmp, *p;
    unsigned int seed = time(NULL);

    tmp = malloc(n * sizeof(uint16_t));
    p = tmp;

    for (int i = 0; i < 7; ++i) {
        uint16_t cut = n >= 20 ? rand_r(&seed) % 20 + n / 2 - 10 : n / 2;
        uint16_t left = 0, right = cut, fall;

        while (p != tmp + n) {
            if (left != cut) {
                fall = rand_r(&seed) % 3;
                fall = left + fall < cut ? fall : 1;

                for (int i = 0; i < fall; ++i) {
                    *p++ = array[left++];
                }
            }

            if (right != n) {
                fall = rand_r(&seed) % 3;
                fall = right + fall < n ? fall : 1;

                for (int i = 0; i < fall; ++i) {
                    *p++ = array[right++];
                }
            }
        }

        memcpy(array, tmp, n * sizeof(uint16_t));
        p = tmp;
    }
}

#define ALIGNED(X) \
    !((uintptr_t)X & (sizeof(uint64_t) - 1))

#define DETECTNULL(X) \
    (((X) - 0x01010101) & ~(X) & 0x80808080)

char *strcpy_orig(char *dest, const char *src)
{
	char *rtn = dest;

	while ((*dest++ = *src++));

	return rtn;
}

char *strcpy_opt(char *dest, const char *src)
{
	if (ALIGNED(src) & ALIGNED(dest)) {
		uint64_t *asrc = (uint64_t *)src;
		uint64_t *adest = (uint64_t *)dest;

		while (!DETECTNULL(*asrc)) {
			*adest++ = *asrc++;
		}

		src = (char *)asrc;
		dest = (char *)adest;
	}

	while ((*dest++ = *src++));

	return dest;
}

#define K 1024
#define M 1048576
#define G 1073741824
#define PAGE_SIZE 4096 // 4K
#define BUFFER_SIZE 16777216 // 16M
#define RANDOM_BUFFER_SIZE 256
#define NUM_TEST 18

typedef uint32_t word;

static char *src,  *src__page;
static char *dest, *dest_page;
static const int test_byte[NUM_TEST] = {4, 8, 16, 32, 64, 128, 256, 512, K, 4096, 32768, 65536, 256 * K, M, 3, 17, 29, 137};
static int  *random_buffer_1K, *random_buffer_1M;

static void init()
{
	src  = malloc(BUFFER_SIZE);
	dest = malloc(BUFFER_SIZE * 2);

	src__page = src  + ((PAGE_SIZE - ((uintptr_t)src  & (PAGE_SIZE - 1))) &
	                    (PAGE_SIZE - 1));
	dest_page = dest + ((PAGE_SIZE - ((uintptr_t)dest & (PAGE_SIZE - 1))) &
	                    (PAGE_SIZE - 1));

	srand(time(NULL));
	for (int i = 0; i < BUFFER_SIZE; ++i) {
		src[i] = '0' + rand() % 10;
	}
	src[BUFFER_SIZE-1] = '\0';

	random_buffer_1K = malloc(RANDOM_BUFFER_SIZE * sizeof(int));
	random_buffer_1M = malloc(RANDOM_BUFFER_SIZE * sizeof(int));

	for (int i = 0; i < RANDOM_BUFFER_SIZE; ++i) {
		random_buffer_1K[i] = rand() % K;
		random_buffer_1M[i] = rand() % M;
	}
}

#define ROUND 10
#define ARRAY_SIZE 65535

void shuffle_bench()
{
	uint32_t high1, low1;
	uint32_t high2, low2;
	uint64_t cycles;
    uint16_t array[ARRAY_SIZE];
    uint16_t length[9] = {1, 4, 16, 64, 256, 1024, 4096, 16384, 65535};

    for (uint16_t i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = i;
    }

    FILE *top_infp = fopen("top_in.txt", "w");
    FILE *rifflefp = fopen("riffle.txt", "w");

    for (int i = 0; i < 9; ++i) {
        cycles = 0;
        for (int j = 0; j < ROUND; j++) {
            get_cycles(&high1, &low1);
            top_in_shuffle(array, length[i]);
            get_cycles_end(&high2, &low2);
            cycles += diff_in_cycles(high1, low1, high2, low2);
        }
        fprintf(top_infp, "%hu %lf\n", length[i], (double)cycles / ROUND);

        cycles = 0;
        for (int j = 0; j < ROUND; j++) {
            get_cycles(&high1, &low1);
            riffle_shuffle(array, length[i]);
            get_cycles_end(&high2, &low2);
            cycles += diff_in_cycles(high1, low1, high2, low2);
        }
        fprintf(rifflefp, "%hu %lf\n", length[i], (double)cycles / ROUND);
    }

    fclose(top_infp);
    fclose(rifflefp);
}

void strcpy_bench()
{
	char *(*_strcpy)(char *, const char *) = strcpy_orig;

	uint32_t high1, low1;
	uint32_t high2, low2;
	uint64_t cycles;
	FILE *na1k = fopen("na1k_orig.txt", "w");
	FILE *na1m = fopen("na1m_orig.txt", "w");
	FILE *a1k  = fopen("a1k_orig.txt",  "w");
	FILE *a1m  = fopen("a1m_orig.txt",  "w");
    FILE *average = fopen("average_orig.txt", "w");

	for (uint64_t i = 0; i < NUM_TEST; ++i) {
		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1K[i*ROUND+j];
			char *dest_ptr = dest_page + random_buffer_1K[i*ROUND+j+1];
			char tmp;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(na1k, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1M[i*ROUND+j];
			char *dest_ptr = dest_page + random_buffer_1M[i*ROUND+j+1];
			char tmp;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(na1m, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1K[i*ROUND+j] * 4;
			char *dest_ptr = dest_page + random_buffer_1K[i*ROUND+j+1] * 4;
			char tmp;

			if (BUFFER_SIZE > random_buffer_1K[j*2] * 4 + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(a1k, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1M[i*ROUND+j] * 4;
			char *dest_ptr = dest_page + random_buffer_1M[i*ROUND+j+1] * 4;
			char tmp;

			if (BUFFER_SIZE > random_buffer_1M[j*2] * 4 + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(a1m, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf\n", test_byte[i], (double)cycles / NUM_TEST);
	}

	fclose(na1k);
	fclose(na1m);
	fclose(a1k);
	fclose(a1m);
    fclose(average);

	_strcpy = strcpy_opt;

	na1k = fopen("na1k_opt.txt", "w");
	na1m = fopen("na1m_opt.txt", "w");
	a1k  = fopen("a1k_opt.txt",  "w");
	a1m  = fopen("a1m_opt.txt",  "w");
    average = fopen("average_opt.txt", "w");

	for (uint64_t i = 0; i < NUM_TEST; ++i) {
		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1K[i*ROUND+j];
			char *dest_ptr = dest_page + random_buffer_1K[i*ROUND+j+1];
			char tmp;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(na1k, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1M[i*ROUND+j];
			char *dest_ptr = dest_page + random_buffer_1M[i*ROUND+j+1];
			char tmp;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(na1m, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1K[i*ROUND+j] * 4;
			char *dest_ptr = dest_page + random_buffer_1K[i*ROUND+j+1] * 4;
			char tmp;

			if (BUFFER_SIZE > random_buffer_1K[j*2] * 4 + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1K[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(a1k, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf ", test_byte[i], (double)cycles / NUM_TEST);

		cycles = 0;

		for (int j = 0; j < ROUND; j++) {
			char *src_ptr  = src__page + random_buffer_1M[i*ROUND+j] * 4;
			char *dest_ptr = dest_page + random_buffer_1M[i*ROUND+j+1] * 4;
			char tmp;

			if (BUFFER_SIZE > random_buffer_1M[j*2] * 4 + test_byte[i]) {
				tmp = src_ptr[test_byte[i]];
				src_ptr[test_byte[i]] = '\0';
			}

			get_cycles(&high1, &low1);
			_strcpy(dest_ptr, src_ptr);
			get_cycles_end(&high2, &low2);

			assert(!strcmp(src_ptr, dest_ptr));

			uint64_t c = diff_in_cycles(high1, low1, high2, low2);
			cycles += c;

			if (BUFFER_SIZE > random_buffer_1M[j*2] + test_byte[i]) {
				src_ptr[test_byte[i]] = tmp;
			}

			fprintf(a1m, "%d %ld\n", test_byte[i], c);
		}

		fprintf(average, "%d %lf\n", test_byte[i], (double)cycles / NUM_TEST);
	}

	fclose(na1k);
	fclose(na1m);
	fclose(a1k);
	fclose(a1m);
    fclose(average);
}

int main()
{
	init();

    shuffle_bench();
    /*strcpy_bench();*/

	return 0;
}
