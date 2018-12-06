#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <x86intrin.h>

#include "hjbcrypt.h"

#define array_size(x) (sizeof(x) / sizeof(*(x)))

#ifdef HARDENED
extern uint64_t Te0[];
extern uint64_t Te1[];
#else
extern uint64_t Te0[];
#endif


/* * * * * * * * * * * * * * * * * */
/* * My code * * * * * * * * * * * */

#define PROBE_NUM 10000

unsigned long time_access_no_flush(const char *adrs) {
  volatile unsigned long time;
  asm __volatile__ (
    "  mfence             \n" // guarantees that every load and store instruction that precedes in program order the MFENCE instruction is globally visible
    "  lfence             \n" // LFENCE does not execute until all prior instructions have completed locally
    "  rdtsc              \n"
    "  lfence             \n"
    "  movl %%eax, %%esi  \n"
    "  movl (%1), %%eax   \n"
    "  lfence             \n"
    "  rdtsc              \n"
    "  subl %%esi, %%eax  \n"
    : "=a" (time)
    : "c" (adrs)
    :  "%esi", "%edx");
  return time;
}

unsigned int find_threshold(){
	const char *addr = malloc(8);
	uint64_t t1=0, t2=0, t_sum_flush=0, t_sum_no_flush=0;
	for(size_t i = 0; i < PROBE_NUM; i++){
		if(i%2==0){
			asm __volatile__ ("mfence\nclflush 0(%0)" : : "r" (addr) :);
		}
		
		if(i%2==0){
			t_sum_flush += time_access_no_flush(addr);
		}
		else{
			t_sum_no_flush += time_access_no_flush(addr);
		}
	}
	t_sum_flush = t_sum_flush / (PROBE_NUM/2);
	t_sum_no_flush = t_sum_no_flush / (PROBE_NUM/2);
	// printf("flush: %lu\nno flush: %lu\n", t_sum_flush, t_sum_no_flush);
	return (t_sum_flush + t_sum_no_flush)/2;
	 
}

void set_other_bytes(unsigned char* in, unsigned int reserved_byte){
	for(size_t i = 0; i < 8; i++){
		if(i == reserved_byte){
			continue;
		}
		in[i] = rand()%256;
	}
}

int main(void)
{
	unsigned char out[8];
	unsigned char in[8];

	unsigned long long threshold = find_threshold();
	// printf("threshold: %llu\n", threshold);
	
	// unsigned int results[8];
	unsigned int max_hits[8];
	char results[8];
	for(size_t i = 0; i < 8; i++){
		max_hits[i]=0;
	}
	


	for (unsigned int i=0; i<8; i++) {
		for (unsigned int val=0; val<256; val++) {
			unsigned int hits = 0;
			in[i] = val;
			for (unsigned int round=0; round < PROBE_NUM; round++) {
				set_other_bytes(in, i);
				asm __volatile__ ("mfence\nclflush 0(%0)" : : "r" (&Te0[0]) :);
				hjb_sign_data(out, in, 8);
				if(time_access_no_flush((const char *)&Te0[0]) < threshold){
					hits++;
				}
			}
			if(hits > max_hits[i]){
				max_hits[i] = hits;
				results[i] = (char)val;
			}
		}
	}

	for(size_t i = 0; i < 8; i++){
		printf("%hhx", (unsigned char)results[i]);
		if(i != 7){
			printf(":");
		}
	}
	printf(" (");
	for(size_t i = 0; i < 8; i++){
		if(isprint(results[i])){
			printf("%c", (char)results[i]);	
		}
		else{
			printf("?");
		}
	}
	printf(")\n");

	return 0;
}
