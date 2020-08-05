#include "data.h"
#include "bf.h"
#include "crypto/aes-ni.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __MINGW32__ 

#ifndef _LIBC
# define __builtin_expect(expr, val)   (expr)
#endif

#undef memmem

/* Return the first occurrence of NEEDLE in HAYSTACK.  */
void *
memmem (const void *haystack, size_t haystack_len, const void *needle,
	size_t needle_len)
{
  const char *begin;
  const char *const last_possible
    = (const char *) haystack + haystack_len - needle_len;

  if (needle_len == 0)
    /* The first occurrence of the empty string is deemed to occur at
       the beginning of the string.  */
    return (void *) haystack;

  /* Sanity check, otherwise the loop might search through the whole
     memory.  */
  if (__builtin_expect (haystack_len < needle_len, 0))
    return NULL;

  for (begin = (const char *) haystack; begin <= last_possible; ++begin)
    if (begin[0] == ((const char *) needle)[0] &&
	!memcmp ((const void *) &begin[1],
		 (const void *) ((const char *) needle + 1),
		 needle_len - 1))
      return (void *) begin;

  return NULL;
}
#endif

int find_pe(char* buffer, int len, void* _ctx){
	uint8_t key[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t iv[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	char magic[] = "This program cannot be run in DOS mode";

	int i;
	uint8_t *pos;
	char data[enc_data_len];

	memcpy(key, buffer, len);
	
	__m128i key_schedule[20];

	aes128_load_key(key_schedule, key);

		
	for(i = 0; i < enc_data_len; i+=16){

		aes128_dec(key_schedule, enc_data + i, data + i);
		XOR64(data + i , iv);
		memcpy(iv, enc_data + i , 16);

	}
	
	pos = memmem(data, enc_data_len, magic, sizeof(magic)-1);
	if(pos != NULL){
		printf("match, key 0x%08llx (%s)\n", *(uint64_t*)key, buffer);
	}
	return 0;
}

int find_zero(char* buffer, int len, void* _ctx){
	uint8_t key[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t iv[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t magic[] = {
		0x00, 0x00, 0x00, 0x00, 0x00
	};


	int i;
	uint8_t *pos;
	char data[enc_data_len];

	memcpy(key, buffer, len);
	
	__m128i key_schedule[20];

	aes128_load_key(key_schedule, key);

		
	for(i = 0; i < enc_data_len; i+=16){
		aes128_dec(key_schedule, enc_data + i, data + i);
		XOR64(data + i , iv);
		memcpy(iv, enc_data + i , 16);

	}
	
	pos = memmem(data, enc_data_len, magic, sizeof(magic));
	if(pos != NULL){
		printf("match, key 0x%08llx (%s)\n", *(uint64_t*)key, buffer);
	}
	return 0;
}

void main(int argc, char** argv){
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	int len_min, len_max;
	int i, t;
	int thread_nb = 8;
	double possibility = 0;
	unsigned long long work_size = 10000000;
	unsigned long long seed = 0;

	static char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789-_.";
	char charset_len = sizeof(charset) - 1;

	if (argc < 3){
		printf ("usage %s len_min len_max\n", argv[0]);
		exit (0);
	}    

	len_min = atoi(argv[1]);
	len_max = atoi(argv[2]);

	if (argc > 3)
		thread_nb= atoi(argv[3]);

	if (argc > 4)
		seed = atoll(argv[4]);

	fprintf(stderr, "len_min: %d len_max: %d, threads: %d\n", 
			len_min, len_max, thread_nb);

	for(i=len_min; i < len_max; ++i){
		possibility += pow((double)(charset_len), (double)i);
	}
	//possibility = pow((double)(charset_len), (double)len_max);

	fprintf(stderr, "charset_len: %d possibility: %.0lf seed: %lld\n", 
			charset_len, possibility, seed);


	bf_init(seed);
	pthread_t thread[thread_nb];
	bf_ctx ctx[thread_nb];
	for (t = 0; t < thread_nb; ++t){

		ctx[t].thread_id = t;
		ctx[t].possibility = possibility;
		ctx[t].work_size = work_size;

		ctx[t].len_max = len_max;
		ctx[t].len_min = len_min;

		ctx[t].charset = charset;
		ctx[t].charset_len = charset_len;

		ctx[t].process = find_zero;
		//ctx[t].process = find_pe;
		ctx[t].process_ctx = NULL;


		pthread_create (&thread[t], NULL, worker_, &ctx[t]);
	}

	for (t = 0; t < thread_nb; ++t){
		pthread_join (thread[t], NULL);
	}
}
