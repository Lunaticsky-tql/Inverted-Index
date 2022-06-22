//
// Created by LENOVO on 2022/6/22.
//
#include <cstdio>
#if defined  __aarch64__
printf("this is aarch64\n");
#elif defined  __x86_64__
#include <mpi.h>
#endif


int main (int argc, char **argv)
{
    int rank, size;

    MPI_Init (&argc, &argv);  /* starts MPI */

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);    /* get current process id */
    MPI_Comm_size (MPI_COMM_WORLD, &size);    /* get number of processes */

    printf( "Hello world from process %d of %d\n", rank, size );

    MPI_Finalize();

    return 0;
}
