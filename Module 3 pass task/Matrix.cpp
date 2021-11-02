#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mpi.h>

using namespace std;
using namespace std::chrono;

const int size = 4;
//to store the result of the multiplication
int **outputResult;

int **A;
int **B;

void randomArray(int **array)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            array[i][j] = rand() % 100;
        }
    }
}


void multiplication(int **A, int **B, int **output)
{
    int row, column, multiplying;

    for(row = 0; row < size; row++)
    {
        for(column = 0 ; column < size; column++)
        {
            output[row][column] = 0;

            for( multiplying= 0;multiplying< size; multiplying++)
            {
                output[row][column] += (A[row][multiplying]) * (B[multiplying][column]);
            }
        }
    }
}

int main(int argc, char** argv)
{
    //initialise random number generator
    srand(time(0));

    //store the start time
    auto start = high_resolution_clock::now();

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

    int length = size/numtasks;
    int broadcast_len = (size * size);
    int scatter_len = (size * size) / numtasks;

 
//initialising our vectors
//executed by all the processes but only master initialises.
    if(rank == 0) 
    {
        A = (int **)malloc(size * size * sizeof(int *));
        B = (int **)malloc(size * size * sizeof(int *));
        outputResult = (int **)malloc(size * size * sizeof(int *));

        //allocation each row
        for(int i = 0; i < size; i++)
        {
            A[i] = (int *)malloc(size * sizeof(int));
        }
        for(int i = 0; i < size; i++)
        {
            B[i] = (int *)malloc(size * sizeof(int));
        }
        for(int i = 0; i < size; i++)
        {
            outputResult[i] = (int *)malloc(size * sizeof(int));
        }

        randomArray(A);
        randomArray(B);
    
        // for (int i = 0; i < size; i++) 
        // {
        //     for (int j = 0; j < size; j++)
        //     {
        //         cout << A[i][j] << " ";
        //     }
        //     cout << "\n";
        // }

        MPI_Scatter(&A[0][0], scatter_len, MPI_INT, &A, 0, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&B[0][0], broadcast_len, MPI_INT, 0, MPI_COMM_WORLD);
        multiplication(A, B, outputResult);
        MPI_Gather(MPI_IN_PLACE, scatter_len, MPI_INT, &outputResult[0][0], scatter_len, MPI_INT, 0, MPI_COMM_WORLD);

        
        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(stop - start);


        cout << "Time taken by function: "
                << duration.count() << " microseconds "<<endl;

        ofstream file;
        file.open("outputFile.txt");

        for (int i = 0; i < size; i++) 
        {
            for (int j = 0; j < size; j++)
            {
                cout << outputResult[i][j] << " ";

                file << outputResult[i][j] << " ";
            }
            cout << "\n";
            file << "\n";
        }
        
        file.close();
    }

    else
    {
        A = (int **)malloc(size * size * sizeof(int *));
        B = (int **)malloc(size * size * sizeof(int *));
        outputResult = (int **)malloc(size * size * sizeof(int *));

        //allocation each row
        for(int i = 0; i < size; i++)
        {
            A[i] = (int *)malloc(size * sizeof(int));
        }
        for(int i = 0; i < size; i++)
        {
            B[i] = (int *)malloc(size * sizeof(int));
        }
        for(int i = 0; i < size; i++)
        {
            outputResult[i] = (int *)malloc(size * sizeof(int));
        }

        randomArray(A);
        randomArray(B);
        
        // for (int i = 0; i < size; i++) 
        // {
        //     for (int j = 0; j < size; j++)
        //     {
        //         cout << A[i][j] << " ";
        //     }
        //     cout << "\n";
        // }
        MPI_Scatter(NULL, scatter_len, MPI_INT, &A[0][0], scatter_len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&B[0][0], broadcast_len, MPI_INT, 0, MPI_COMM_WORLD);
        multiplication(A, B, outputResult);
        MPI_Gather(&outputResult[0][0], scatter_len, MPI_INT, NULL, scatter_len, MPI_INT, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}