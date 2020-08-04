#ifndef BF_H_
#define BF_H_

#include <pthread.h>

typedef struct _bf_ctx{
	int thread_id;
	unsigned long long work_size;
	double possibility;

	int len;
	int len_min;
	int len_max;

	char *charset;
	int charset_len;

	int (*process)(char* key, int len, void* _ctx);
	void * process_ctx;
} bf_ctx;


void* worker_ (void* _ctx);
void bf_init(unsigned long long seed);

#endif
