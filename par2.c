#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "omp.h"
#include "papi.h"

#define SIZE 1<<15
#define MAX 1<<20
#define NUM_THREADS 8
#define BUCKET_SIZE 16
#define CHUNK 1024
#define RUNS 16
#define NUM_EVENTS 4
int Events[NUM_EVENTS] = { PAPI_TOT_CYC, PAPI_TOT_INS, PAPI_L1_DCM, PAPI_L2_DCM };
long long values[NUM_EVENTS], min_values[NUM_EVENTS];
int retval, EventSet=PAPI_NULL;
long long valuematrix[RUNS][NUM_EVENTS];


typedef struct lista
{
	int* list;
	int tamanho; // tamanho alocado
	int ocupado; // quanto já ocupado
}lista;

lista* create(int size)
{
	lista* lista = malloc(sizeof(struct lista));
	lista -> list = malloc(size*sizeof(int));
	lista -> tamanho = size;
	lista -> ocupado = 0;
	return lista;
}

void addElemento (lista* lista, int elemento)
{
	if (! ( lista -> ocupado < lista -> tamanho ))
	{
		lista->list = realloc( (lista->list) , (lista->tamanho)*2*sizeof(int));
		lista->tamanho *=2;
	}
	(lista -> list)[lista -> ocupado] = elemento;
	(lista -> ocupado)++;

}

void print(lista* lista)
{
	for (int i = 0; i < lista -> ocupado; i++)
		printf("%d\n",(lista -> list)[i]);
}

void freeL(lista* lista)
{
	free(lista -> list);
	free(lista);
}

int max (int elementos[], int N)
{
	int max = -1;
	for (int i = 0; i < N; i++)
		if (elementos[i] > max)
			max = elementos[i];

	return max;
}

// usada para o qsort
int cmpfunc (const void * a, const void * b) {
	return ( *(int*)a - *(int*)b );
}

void bucketSort (int elementos[],int N)
{
	int nrBuckets = (max(elementos,N) / BUCKET_SIZE) + 1;

	//printf("%d %d %d",N,BUCKET_SIZE,nrBuckets);
	//printf("line:%d\n",__LINE__);
	//fflush(stdout);
	lista* listas[nrBuckets];

	//printf("line:%d\n",__LINE__);
	//fflush(stdout);   
	omp_lock_t locks[nrBuckets];

	//printf("line:%d\n",__LINE__);
	//fflush(stdout);   
	for(int i=0;i<nrBuckets;i++){
		omp_init_lock(&locks[i]);

		//printf("line:%d\n",__LINE__);
		//fflush(stdout);
	}

	// criar cada bucket
	//printf("line:%d\n",__LINE__);
	//fflush(stdout);
#pragma omp parallel for
	for (int i = 0; i < nrBuckets; i++)
		listas[i] = create(BUCKET_SIZE);
	// inserir elementos no bucket correto
	//printf("line:%d\n",__LINE__);
	//fflush(stdout);
    
    //main loop que separa em buckets
#pragma omp parallel for schedule(static,CHUNK)
	for (int i = 0; i < N; i++)
	{
		int bucket = elementos[i] / BUCKET_SIZE;

		//addElemento(listas[bucket],elementos[i]);
		lista* lista = listas[bucket];

		omp_set_lock(&locks[bucket]);

		if (! ( lista -> ocupado < lista -> tamanho ))
		{
			//#pragma omp critical
			{
				// realocar memoria bla bla bla
				lista->list = realloc( (lista->list) , (lista->tamanho)*2*sizeof(int));
				lista->tamanho *=2;
			}
		}
		(lista -> list)[lista -> ocupado] = elementos[i];
		(lista -> ocupado)++;

		omp_unset_lock(&locks[bucket]);
	}
	//printf("line:%d\n",__LINE__);
	//fflush(stdout);
	// ordenar cada bucket
#pragma omp parallel for
	for (int i = 0; i < nrBuckets; i++)
		qsort(listas[i]->list,listas[i] -> ocupado, sizeof(int),cmpfunc);

	int pos[nrBuckets];
	pos[0]=0;

	for(int i = 1; i<nrBuckets ; i++){
		pos[i]=pos[i-1]+listas[i-1]->ocupado;
	}

	// reeordenar o array original com os elementos dos buckets ordenados
#pragma omp parallel for
	for (int i = 0; i < nrBuckets; i++)
	{
		memcpy(elementos+pos[i], listas[i]->list, listas[i]->ocupado * sizeof(int));
		freeL(listas[i]);
	}
}
//randomizes an array with ints with value up to max
int* randa(int n, int max){
	int* array = malloc( n*sizeof(int));
	for(int i =0 ; i<n; i++){
		array[i] = rand()%max;
	}
	return array;
}

int main (void)
{
	printf("config openmp\n");
	fflush(stdout);
	//OpenMP configs
	omp_set_num_threads(NUM_THREADS);
	omp_set_dynamic(0);

	printf("init PAPI\n");
	// Initialize PAPI
	fflush(stdout);
	int num_hwcntrs = 0;
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT) {
		fprintf(stderr,"PAPI library init error!\n");
		return 0;
	}

    /* create event set */
    if (PAPI_create_eventset(&EventSet) != PAPI_OK) {
        fprintf(stderr,"PAPI create event set error\n");
        return 0;
    }

    /* Get the number of hardware counters available */
    if ((num_hwcntrs = PAPI_num_hwctrs()) <= PAPI_OK)  {
        fprintf (stderr, "PAPI error getting number of available hardware counters!\n");
        return 0;
    }

    // We will be using at most NUM_EVENTS counters
    if (num_hwcntrs >= NUM_EVENTS) {
        num_hwcntrs = NUM_EVENTS;
    } else {
        fprintf (stderr, "Error: there aren't enough counters to monitor %d events!\n", NUM_EVENTS);
        return 0;
    }

    if (PAPI_add_events(EventSet,Events,NUM_EVENTS) != PAPI_OK)  {
        fprintf(stderr,"PAP I library add events error!\n");
        return 0;
    }
    long long start, end, elapsed, min=0L, timesum=0L;

    //runs the algo multiple times for consistency's sake
    for(int i=0 ; i<RUNS; i++){
        //printf("starting run number %d\n",i);
        //printf("line:%d\n",__LINE__);
        //fflush(stdout);
        start=PAPI_get_real_usec();
        if (PAPI_start(EventSet) != PAPI_OK) {
            fprintf (stderr, "PAPI error starting counters!\n");
            return 0;
        }

        //int k,cachelines=16;
        //cache warmups
        int* a = randa(SIZE,MAX);   
        //da load ao primeiro elemento do array para quando executar o algoritmo de sorting nao comecar logo com tudo cache misses
        // tbm da consistencia ao longo de varios testes pq no segundo ele ja na ia dar cache misses, agora nao da logo no primeiro
        //printf("line:%d\n",__LINE__);
        //fflush(stdout);

        //while(k<( cachelines/2 )){
            a[0]*=1; 
            //k=+BUCKET_SIZE;
        //}
        //printf("sorting...\n");
        //fflush(stdout);
        bucketSort(a,SIZE);
        //printf("finished sorting");
        //fflush(stdout);

        if (PAPI_stop(EventSet,values) != PAPI_OK) {
            fprintf (stderr, "PAPI error stoping counters!\n");
            return 0;
        }
        //printf("line:%d\n",__LINE__);
        //fflush(stdout);

        end=PAPI_get_real_usec();
        elapsed = end - start;
        timesum+=elapsed;

        if ((i==0) || (elapsed < min)) {
            min = elapsed;
            for (int j=0 ; j< NUM_EVENTS ; j++) min_values[j] = values [j];
        }
        for (int i=0 ; i< NUM_EVENTS ; i++) {
            char EventCodeStr[PAPI_MAX_STR_LEN];
            if (PAPI_event_code_to_name(Events[i], EventCodeStr) == PAPI_OK) {
                //printf ( "%s = %lld\n", EventCodeStr, values[i]);
            } else {
                //printf ( "PAPI UNKNOWN EVENT = %lld\n", values[i]);
            }
        }
        for (int j=0 ; j< NUM_EVENTS ; j++) valuematrix[i][j] = values [j];

    }
    printf ("\nWall clock time: %lld usecs\n", min);

    // output PAPI counters' values
    printf("best run:\n");
    for (int i=0 ; i< NUM_EVENTS ; i++) {
        char EventCodeStr[PAPI_MAX_STR_LEN];
        if (PAPI_event_code_to_name(Events[i], EventCodeStr) == PAPI_OK) {
            printf ( "%s = %lld\n", EventCodeStr, min_values[i]);
        } else {
            printf ( "PAPI UNKNOWN EVENT = %lld\n", min_values[i]);
        }
    }
    printf("\n\naverage:\n");
    printf("avg time: %lld\n",timesum/RUNS);
    for (int i=0 ; i< NUM_EVENTS ; i++) {
        long long avg=0L;
        for(int j=0; j < RUNS; j++){
            avg+=valuematrix[j][i];
        }
        avg=avg/(double)RUNS;
        char EventCodeStr[PAPI_MAX_STR_LEN];
        if (PAPI_event_code_to_name(Events[i], EventCodeStr) == PAPI_OK) {
            printf ( "%s = %lld\n", EventCodeStr, avg);
        } else {
            printf ( "PAPI UNKNOWN EVENT = %lld\n", avg);
        }
    }
    return 0;
}
