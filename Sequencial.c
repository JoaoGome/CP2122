#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    if (lista -> ocupado < lista -> tamanho)
    {
        (lista -> list)[lista -> ocupado] = elemento;
        (lista -> ocupado)++;
    }
    else
    {
        // realocar memoria bla bla bla
    }
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

    // criar cada bucket
    for (int i = 0; i < nrBuckets; i++)
        listas[i] = create(10);

    // inserir elementos no bucket correto
    for (int i = 0; i < N; i++)
    {
        int bucket = elementos[i] / 10;
        addElemento(listas[bucket],elementos[i]);
    }

    // ordenar cada bucket
    for (int i = 0; i < nrBuckets; i++)
        qsort(listas[i]->list,listas[i] -> ocupado, sizeof(int),cmpfunc);


    int contador = 0;

    // reeordenar o array original com os elementos dos buckets ordenados
    for (int i = 0; i < nrBuckets; i++)
    {
        for (int j = 0; j < listas[i] -> ocupado; j++)
        {
            elementos[contador] = (listas[i]->list)[j];
            contador++;
        }

        freeL(listas[i]);
    }
}


int main (void)
{
    int a[] = {29,25,3,49,9,37,21,43,1,2,67,87,56,90,876,345,234,4,5,6,2,45,26,346};
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    bucketSort(a,SIZE);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    for (int i = 0; i < SIZE; i++)
        printf("%d\n",a[i]);

    printf("%fs to execute\n", cpu_time_used);

        
}