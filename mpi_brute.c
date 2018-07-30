#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

#include "mpi_brute.h"

void value_entry(char * data, void * arg )
{
    fprintf(stderr, "%s\n", data);
}


void count_entry(char * data, void * arg )
{
    unsigned long long int * cnt = (unsigned long long int*)arg;
    *cnt = *cnt +1;
    //fprintf(stderr, "%s\n", data);
}




void gen_combination( const char * choices,
                      int choice_len,
                      char * current,
                      int current_len,
                      int max_len,
                      void (*cb)(char * data, void *arg ),
                      void * arg )
{
    int i;

    for( i = 0 ; i < choice_len; i++)
    {
        current[current_len] = choices[i];
        current[current_len+1] = '\0';

        (cb)(current, arg);

        if( (current_len + 1) < max_len )
            gen_combination(choices, choice_len, current, current_len + 1, max_len, cb, arg);
    }
}


void gen_combination_for_seed( const char * choices,
                                int choice_len,
                                char * seed,
                                char * current,
                                int max_len,
                                void (*cb)(char * data, void *arg ),
                                void * arg )
{
    snprintf(current, max_len, "%s", seed);
    int seed_len = strlen(seed);

    fprintf(stderr, "Starting seed '%s' at len %d\n", seed, seed_len);
                 choice_len,
                 current,
    gen_combination(choices,
                    choice_len,
                    current,
                    seed_len,
                    max_len,
                    cb,
                    arg );


}



#define MAX_LEN 6


struct seed{
    char seed[MAX_LEN + 1];
    struct seed * next;
};


void push_seed(char * data, void * arg )
{
    struct seed ** seed = (struct seed **)arg;

    struct seed * current = *seed;

    if( current )
    {
        while(current->next)
        {
            current = current->next;
        }
    }        

    struct seed * n = (struct seed *)malloc(sizeof(struct seed));

    snprintf(n->seed, MAX_LEN + 1, data);
    n->next = NULL;

    if( current )
    {
        current->next = n;
    }
    else
    {
        *seed = n;
    }
}


void clear_seeds( struct seed ** s )
{

    if( *s == NULL )
        return;

    struct seed *current = *s;


    struct seed *to_free = NULL;

    while(current)
    {
        to_free = current;
        current = current->next;
        free(current);
    }

    *s = NULL;
}


struct seed * extract_local_seeds( struct seed * s , int rank , int size )
{
    struct seed * ret = NULL;

    int id = 0;

    while(s)
    {
        /* Is it for Me ? */
        if( (id % size) == rank )
        {
            /* Insert seed in local list */
            struct seed * n =  (struct seed *)malloc(sizeof(struct seed));
            snprintf(n->seed, MAX_LEN + 1, s->seed);
            n->next = ret;
            ret = n;
        }

        id++;
        s = s->next;
    }

    return ret;
}


int count_seeds( struct seed * s )
{
    int ret = 0;


    while(s)
    {
        ret++;
        s = s->next;
    }

    return ret;
}

struct seed * filter_len_seed( struct seed * s , int len,
                               void (*cb)(char * data, void *arg ),
                               void *arg )
{
    struct seed * ret = NULL;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while(s)
    {
        struct seed * next = s->next;

        if( strlen(s->seed) == len )
        {
           s->next = ret;
           ret = s;
        }
        else
        {
            if(rank == 0)
                (cb)(s->seed, arg);
            free(s);
        }
        s = next;
    }



    return ret;
}


struct seed * gen_seeds( const char * choice , int choice_len, int num_stream,
                         void (*cb)(char * data, void *arg ),
                         void *arg )
{
    int len = 0;
    char current[MAX_LEN + 1];
    struct seed * ret = NULL;

    do
    {
        len++;
        clear_seeds(&ret);
        gen_combination(choice, choice_len, current, 0, len, push_seed, (void *)&ret);


        //fprintf(stderr, "There are %d seeds for len %d\n", count_seeds(ret), len);

        /* We now remove seeds not matching the length */
        ret = filter_len_seed(ret, len, cb, arg);


    }while( (count_seeds(ret) < num_stream) && (len < MAX_LEN) );

    return ret;
}


void mpi_compute( const char * choices,
                  int max_len,
                  void (*cb)(char * data, void *arg ),
                  void * arg )
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank );
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    int choice_len = strlen(choices);
    char * current = (char *)malloc(sizeof(char) * (max_len + 1));


    /* First do Generate combination seeds to distribute the work */
    struct seed * global_seeds = gen_seeds( choices , choice_len, size, cb, arg );

    if( rank == 0)
    {
        fprintf(stderr, "There are %d GLOBAL seeds\n", count_seeds(global_seeds));

        struct seed * c = global_seeds;

        while(c)
        {
            fprintf(stderr, "SEED : '%s'\n", c->seed);
            c=c->next;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Here we do it distributed to avoid serializing it */
    struct seed * local_seeds = extract_local_seeds( global_seeds, rank , size );

    fprintf(stderr, "(%d/%d) Will compute %d LOCAL seeds\n", rank, size, count_seeds(local_seeds));

    /* We now have locals get rid of global seeds */
    clear_seeds( &global_seeds );

    struct seed * current_seed = local_seeds;
    
    while(current_seed)
    {
        (cb)(current_seed->seed, arg);
        gen_combination_for_seed(choices, choice_len, current_seed->seed, current, max_len, cb, arg );
        current_seed = current_seed->next;
    }

    free(current);
}


/*

int main(int argc, char ** argv )
{
    MPI_Init(&argc, &argv);

    const char * choices = "01";
    int choice_len = strlen(choices);


    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank );


    char current[MAX_LEN + 1];
    unsigned long long int cnt = 0;


    mpi_compute( choices,
                 MAX_LEN,
                 count_entry,
                 (void*)&cnt );

    unsigned long long int global_cnt = 0;
    MPI_Reduce(&cnt, &global_cnt, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if( rank == 0 )
    {
        fprintf(stderr, "COUNT : %llu", global_cnt);
    }

    MPI_Finalize();

    return 0;
}

*/
