#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <openssl/md5.h>

#include "pw_brute.h"


FILE * out_file = NULL;
size_t cnt = 0;



void md5sum( char * string, char hash[2*MD5_DIGEST_LENGTH + 1] )
{
    unsigned char md5[MD5_DIGEST_LENGTH] = {0};

    MD5((const char *)string, strlen(string), md5);

    int i;

    for( i = 0 ; i < MD5_DIGEST_LENGTH; i++ )
    {
        sprintf(hash + 2*i, "%02x", md5[i]);
    }
}


void compute_hash(char * data, void * arg )
{
    char hash[2*MD5_DIGEST_LENGTH + 1];
    md5sum(data, hash);

    fprintf(out_file, "%s %s\n", data, hash);
}


int main(int argc, char ** argv )
{
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank );

    char out[50];
    sprintf(out, "%d.dat", rank);

    out_file = fopen(out, "w");


    mpi_compute( "01",
                 2,
                 compute_hash,
                 (void*)NULL );

    size_t global_cnt = 0;
    MPI_Reduce(&cnt, &global_cnt, 1, MPI_COUNT, MPI_SUM, 0, MPI_COMM_WORLD);

    if( rank == 0 )
    {
        fprintf(stderr, "COUNT : %llu", global_cnt);
    }

    MPI_Finalize();

    return 0;
}