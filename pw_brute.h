#ifndef PW_BRUTE_H
#define PW_BRUTE_H


void mpi_brute_force( const char * choices,
                      int max_len,
                      void (*cb)(char * data, void *arg ),
                      void * arg );


#endif