#include <mpi.h>
#include <stdio.h>
#include <string>

int main(int argc, char** argv) {
    int numtasks, rank, name_len, tag=1, dest, src, count; 
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status stat;
    // Initialize the MPI environment
    MPI_Init(&argc,&argv);

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    // Print off a hello world message
    char outmessage[] = "Hello World workers!!";
    char inmessage[22];

    if(rank == 0)
    {
        dest = 1;
        src = 1; 
        MPI_Bcast(&outmessage, numtasks, MPI_CHAR, 0, MPI_COMM_WORLD);
        //MPI_Recv(&inmessage, numtasks, MPI_CHAR, src, tag, MPI_COMM_WORLD, &stat);
        printf("Sending...");
    }
     else if(rank == 1)
    {
        dest = 0;
        src = 0;
        //MPI_Recv(&inmessage, numtasks, MPI_CHAR, src, tag, MPI_COMM_WORLD, &stat);
        MPI_Bcast(&outmessage, numtasks, MPI_CHAR, 0, MPI_COMM_WORLD);
        printf("Receiving...");
    }
        //printf("Hello SIT315. You get this message from %s, rank %d out of %d\n",
           //name, rank, numtasks);
    MPI_Get_count(&stat, MPI_CHAR, &count);
    printf("Task %d: received %d char(s) from task %d with tag %d which is %s \n",
    rank, count, stat.MPI_SOURCE, stat.MPI_TAG, outmessage);

    // Finalize the MPI environment
    MPI_Finalize();
}