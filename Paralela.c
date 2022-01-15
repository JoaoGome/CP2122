#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "omp.h"

#define SIZE 24

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
    int nrBuckets = (max(elementos,N) / 10) + 1;

    lista* listas[nrBuckets];
    omp_lock_t locks[nrBuckets];
    for(int i=0;i<nrBuckets;i++){
        omp_init_lock(&locks[i]);
    }
    // criar cada bucket
    #pragma omp parallel for
    for (int i = 0; i < nrBuckets; i++)
        listas[i] = create(10);
    // inserir elementos no bucket correto
    #pragma omp parallel for
    for (int i = 0; i < N; i++)
    {
        int bucket = elementos[i] / 10;
        
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
int* randa(int n, int max){
    int* array = malloc( n*sizeof(int));
    for(int i =0 ; i<n; i++){
        array[i] = rand()%max;
    }
    return array;
}

int main (void)
{
    int m = 1000000;
    int max = 30000;
    int* a = randa(m,max);
    double avg =0;
    for(int i=0;i<10;i++){
        double start, end;
        double cpu_time_used;
        start = omp_get_wtime();
        bucketSort(a,m);
        end = omp_get_wtime();
        cpu_time_used = ((double) (end - start));
        printf("%fs to execute\n", cpu_time_used);
        avg+=cpu_time_used;
    }
    avg = avg/10;
    printf("%f \n",avg);
        for (int i = 0; i < m; i++){
        //printf("%d\n",a[i]);
        }

}
