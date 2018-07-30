MPI_Brute
---------

This a versatile MPI password breaking tool example.

MD5
---

You can generate MD5 tables for alphanumeric characters up to lenght 5 using:

``
make
mpirun -np XX ./rainbow
``

It should generate 25 GB of data.

TODO : Use distributed rainbow tables instead of raw dump.

Brute Forcing
-------------

The main interest of this small project is the single function parallel password
generation library. Using *mpi_brute.{c,h}* directly in your project you gain a
callback oriented password space generation routine with the following
footprint:

``
void mpi_brute_force( const char * choices,
                      int max_len,
                      void (*cb)(char * data, void *arg ),
                      void * arg );
``

- **choices** : list of characters to be explored
- **max_len** : maximum key length
- **cb** : callback to be called on each key
- **arg** : argument to be passed to each callback
