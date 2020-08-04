//gcc -lpthread -o bf bf.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "bf.h"

pthread_mutex_t lock_offset;
unsigned long long global_offset = 0;

int next (char* o, char *k, 
		char *charset, int charset_len,
		int len){
	int i;
	for (i = 0; i < len; ++i){
		if (o[i] != charset_len - 1){
			++o[i];
			k[i] = charset[o[i]];
			return i;
		}
		o[i] = 0;
		k[i] = charset[o[i]];
	}
	return i;
}

int seed2char(int len_min, int len_max, 
		char *charset, int charset_len,
		unsigned long long seed, char *dst, char *offset)
{
	int i, len;
	unsigned long long n, x;

	n = seed;	
	for(i=len_min; i < len_max; ++i){
		x = pow(charset_len, (double)i);
		if (x > seed) { 
			n = x;
			break;
		}
	}
	len = i;

	if(seed != 0){
		n = seed;
	}
	memset(dst, charset[0], i); 
	memset(offset, 0, i); 

	lldiv_t res;
	//i = 0;
	//fprintf(stderr, "%d %lld %lld\n", i, n, seed);
	while( n > 1 ){
		res = lldiv(n, charset_len);
		n = res.quot;
		dst[len - i ] = charset[res.rem];
		offset[len - i ] = res.rem;
		//fprintf(stderr, "len: %d i: %d n: %lld char: %c\n", len, i, n, dst[len - i]);
		i--;
	}
	return len;
}

int nextlen(int len_min, int len_max, 
		int charset_len, unsigned long long seed)
{
	unsigned long long n, x;
	int i;

	for(i=len_min; i < len_max; ++i){
		x = pow(charset_len, (double)i);
		if (x > seed) { 
			break;
		}
	}
	return x;
}

void* worker_ (void* _ctx){
	bf_ctx* ctx = _ctx;
	unsigned long long cur_offset = 0, end_offset = 0, next_len_offset = 0;
	fprintf(stderr, "[%02d] start\n", ctx->thread_id);
	unsigned long long i;

	char data[ctx->len_max];
	char offset[ctx->len_max];

	clock_t start, end;
	float seconds;

	// fetch work offset
	pthread_mutex_lock(&lock_offset);
	cur_offset = global_offset;
	global_offset += ctx->work_size;
	pthread_mutex_unlock(&lock_offset); 	

	memset(data, 0, ctx->len_max);
	memset(offset, 0, ctx->len_max);
	ctx->len = seed2char(ctx->len_min, ctx->len_max, 
			ctx->charset, ctx->charset_len, 
			cur_offset, data, offset);
	next_len_offset = nextlen(ctx->len_min, ctx->len_max, 
			ctx->charset_len, cur_offset);

	while(cur_offset < ctx->possibility){
		fprintf(stderr, "[%02d] starting at %lld (%s)\n", ctx->thread_id, cur_offset, data);
		end_offset = cur_offset+ctx->work_size-1;
		if(end_offset > ctx->possibility){ end_offset = ctx->possibility-1; }
		//seed2char(ctx->len_min, ctx->len_max, end_offset, data, offset);
		//fprintf(stderr, "[%02d] ending at %lld (%s)\n", ctx->thread_id, end_offset, data) ;


		start = clock();
		for(i = cur_offset; i <= end_offset; ++i){
			ctx->process (data, ctx->len, ctx->process_ctx);
			//fprintf(stderr, "[%02d] (%d) %s\n", ctx->thread_id, ctx->len, data);

			if (i+1 == next_len_offset){
				++i;

				ctx->len++;
				// New len we reset data and offset
				memset(data, ctx->charset[0], ctx->len); 
				memset(offset, 0, ctx->len); 

				next_len_offset = nextlen(ctx->len_min, ctx->len_max, 
						ctx->charset_len, cur_offset);

				// We process the first elem of the new len
				ctx->process (data, ctx->len, ctx->process_ctx);
				//fprintf(stderr, "[%02d] (%d) %s\n", ctx->thread_id, ctx->len, data);
			}
			next(offset, data, 
					ctx->charset, ctx->charset_len, 
					ctx->len); 
		}
		end = clock();
		seconds = (float)(end - start) / CLOCKS_PER_SEC;
		fprintf(stderr, "[%02d] end at (%d) %s\n", ctx->thread_id, ctx->len, data);
		fprintf(stderr, "[%02d] %.2fs %.0fc/s\n", ctx->thread_id, seconds, (end_offset-cur_offset)/seconds);

		// fetch work offset
	    pthread_mutex_lock(&lock_offset);
		cur_offset = global_offset;
		global_offset += ctx->work_size;
	    pthread_mutex_unlock(&lock_offset); 	

		memset(data, 0, ctx->len_max);
		memset(offset, 0, ctx->len_max);

		ctx->len = seed2char(ctx->len_min, ctx->len_max, 
				ctx->charset, ctx->charset_len, 
				cur_offset, data, offset);
		next_len_offset = nextlen(ctx->len_min, ctx->len_max, 
				ctx->charset_len, cur_offset);

	}
	fprintf(stderr, "[%02d] end\n", ctx->thread_id);
}

void bf_init(unsigned long long seed){
	pthread_mutex_init(&lock_offset, NULL);
	global_offset = seed;
}

/*
void main(int argc, char** argv){
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	int len_min, len_max;
	int i, t;
	int thread_nb = 8;
	double possibility = 0;
	//unsigned long long work_size = 100000000;
	unsigned long long work_size = 5000000;


	if (argc < 3){
		printf ("usage %s len_min len_max\n", argv[0]);
		exit (0);
	}    

	len_min = atoi(argv[1]);
	len_max = atoi(argv[2]);

	if (argc > 3)
		thread_nb= atoi(argv[3]);

	fprintf(stderr, "len_min: %d len_max: %d, threads: %d\n", 
			len_min, len_max, thread_nb);

	for(i=len_min; i < len_max; ++i){
		possibility += pow((double)(MAX - MIN), (double)i);
	}

	fprintf(stderr, "charset_len: %d possibility: %.0lf\n", (MAX-MIN), possibility);

	pthread_mutex_init(&lock_offset, NULL);

	pthread_t thread[thread_nb];
	bf_ctx ctx[thread_nb];
	for (t = 0; t < thread_nb; ++t){
		ctx[t].process = test_;
		//ctx[t].process = crc_callback;
		ctx[t].process_ctx = NULL;
		ctx[t].thread_id = t;
		ctx[t].possibility = possibility;
		ctx[t].work_size = work_size;
		ctx[t].len_max = len_max;
		ctx[t].len_min = len_min;
		pthread_create (&thread[t], NULL, worker_, &ctx[t]);
	}

	for (t = 0; t < thread_nb; ++t){
		pthread_join (thread[t], NULL);
	}
}
*/
