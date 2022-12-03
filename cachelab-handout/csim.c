
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "cachelab.h"

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

int v_flag = 0;
int s,E,b,S;
char file[1000];

typedef struct{
    int valid_bit;
    int tag;
    int timing;
}cache_line;

// 做一个二维数组
cache_line** cache = NULL;


void print_help()
{
      printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void init_cache()
{
    S = (1 << s);
    cache = (cache_line**)malloc(sizeof(cache_line*)*S);
    for (int i =0 ;i<S ;i++)
    {
        cache[i] = (cache_line*)malloc(sizeof(cache_line) * E);
        for (int j =0 ;j<E;j++)
        {
            cache[i][j].valid_bit = 0;
            cache[i][j].tag = -1;
            cache[i][j].timing = 1;
        }
    }
}

void update(unsigned int address)
{
    int set_index = (address >> b) & ((-1U) >> (64 - s));
	int tag = address >> (b + s);

    for (int i=0;i<E;i++)
    {
        if (cache[set_index][i].tag == tag)
        {
            // hit
            cache[set_index][i].timing = 0;
            if (v_flag == 1){
                printf(" hit");
            }
            hit_count++;
            return;
        }
    }

    for (int i = 0;i<E;i++)
    {
        if (cache[set_index][i].valid_bit == 0)
        {
            cache[set_index][i].timing = 0;
            cache[set_index][i].valid_bit = 1;
            cache[set_index][i].tag = tag;
            miss_count++;
            if (v_flag == 1){
                printf(" miss");
            }
            return;
        }
    }

    // 开始驱逐
    int max_timing_index = 0;
    int max_timing = cache[set_index][0].timing;
    for(int i=0;i<E;i++)
    {
        if (cache[set_index][i].timing > max_timing)
        {
            max_timing = cache[set_index][i].timing;
            max_timing_index = i;
        }
    }
    eviction_count++;
    miss_count++;
    cache[set_index][max_timing_index].timing = 0;
    cache[set_index][max_timing_index].tag = tag;
    if (v_flag == 1){
        printf(" miss eviction");
    }
    return;
}


void update_timing()
{
    for(int i =0 ;i<S;i++)
    {
        for(int j = 0;j<E;j++)
        {
            if(cache[i][j].valid_bit == 1)
            {
                cache[i][j].timing++;
            }
        }
    }
}

void open_trace_file()
{
    FILE* fp = fopen(file, "r"); 
	if(fp == NULL)
	{
		printf("open error");
		exit(-1);
	}
    char operation;
    unsigned int address;
    int size;
    while(fscanf(fp, " %c %xu,%d\n", &operation, &address, &size) > 0)
    {
        switch(operation)
        {
            case 'I':
                continue;
            case 'L':
                if (v_flag == 1){
                    printf("%c %x,%d",operation,address,size);
                }
                update(address);
                break;
            case 'M':
                if (v_flag == 1){
                    printf("%c %x,%d",operation,address,size);
                }
                update(address);
                update(address);
                break;
            case 'S':
                if (v_flag == 1){
                    printf("%c %x,%d",operation,address,size);
                }
                update(address);
        }
        if (v_flag == 1){
            printf("\n");
        }
        update_timing();
    }
    for (int i = 0;i<E;i++)
    {
        free(cache[i]);
    }
    free(cache);
}



int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc,argv,"hvs:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 'h':
                print_help();
                break;
            case 'v':
                // print_help();
                v_flag = 1;
                break;
            case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(file, optarg);
				break;
			default:
                print_help();
				break;
        }
    }
    init_cache();
    open_trace_file();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
