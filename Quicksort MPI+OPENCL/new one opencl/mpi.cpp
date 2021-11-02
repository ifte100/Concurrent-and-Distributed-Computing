#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mpi.h>

using namespace std;
using namespace std::chrono;

int SIZE = 10000;
int length;
int *sorting_arr;

void randomArray(int* &array, int size)

{   
    array = (int*)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        array[i] = rand() % 100;
    }
}

void swapping(int* array1, int* array2)
{
    int temp = *array1;
    *array1 = *array2;
    *array2 = temp;
}

//rearranging array using partition
int partition(int low, int high)

{
    //start from the rightmost element as pivot. We can also select leftmost or randomly.
    int pivot = sorting_arr[high];
    //smaller element index
    int i = (low - 1);

    for(int elements = low; elements < high; elements++)
    {
        //if current is smaller than or equals to pivot we swap
        if(sorting_arr[elements] <= pivot)
        {
            i++;
            swapping(&sorting_arr[i], &sorting_arr[elements]);
        }    

    }
  //swapping with greeater element at i
    swapping(&sorting_arr[i + 1], &sorting_arr[high]);
    return (i + 1);
}

void quickSortSeq(int low, int high)
{
    //for first iteration it will be checking if the last element is greater than the first, then swap, and so on...we find the pivot
    if (low < high)
    {
        int var_partition = partition(low, high);
        //call on the left of pivot to sort
        quickSortSeq(low, var_partition - 1);
        //call on the right of pivot to sort
        quickSortSeq(var_partition + 1, high);
    }
}

void quickSortMP(int low, int high, int rank, int numtasks, int depth)
{
    int child_rank = rank + (1 << depth);
    //normal sequential quicksort
    if(child_rank >= numtasks)

    {
        //cout<<"Im in if"<<endl;
        quickSortSeq(low, high);
    }

    else if (low < high)
    {
        //cout<<"Im in else if"<<endl;
        int var_partition = partition(low, high);

        /* basically we are sorting the array in two partitions. The higher partition will be sorted in one node
        and the lower partition will be sent to a different child node. Each then makes a recursive call and 
        sorts itself    
        */
        if( (var_partition - low) > (high - var_partition - 1) )
        {
            MPI_Send(&sorting_arr[var_partition], high-var_partition, MPI_INT, child_rank, 1, MPI_COMM_WORLD);
            quickSortMP(low, var_partition, rank, numtasks, depth+1);
            MPI_Recv(&sorting_arr[var_partition], high-var_partition, MPI_INT, child_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            MPI_Send(&sorting_arr[low], var_partition-low, MPI_INT, child_rank, 1, MPI_COMM_WORLD);
            quickSortMP(var_partition + 1, high, rank, numtasks, depth+1);
            MPI_Recv(&sorting_arr[low], var_partition-low, MPI_INT, child_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

    }
}

int main(int argc, char** argv) 
{
    int numtasks, rank, name_len, dest, src, count; 
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

    if(rank == 0)
    {
        randomArray(sorting_arr, SIZE);

        cout << "Before sorting: \n";

        for (int i = 0; i < SIZE; i++)
        {

            cout << sorting_arr[i] << " ";

        }
        auto start = high_resolution_clock::now();

        quickSortMP(0, SIZE, rank, numtasks, 0);

        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(stop - start);

        cout <<"\n"<<endl;
        cout << "After sorting: \n";
        for (int i = 0; i < SIZE; i++)

        {
            cout << sorting_arr[i] << " ";
        }
        cout <<"\n"<<endl;
        cout << "Time taken by function: "
            << duration.count() << " microseconds" << endl;

    }
    else
    {
        int depth=0, parent=0;
        while( 1 << depth <= rank)
        {
            depth++;
        }    

        //we need to use PRobe to get size
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

        MPI_Get_count(&stat, MPI_INT, &length);

        parent = stat.MPI_SOURCE;

        sorting_arr = (int*)malloc(sizeof(int) * length);

        MPI_Recv(sorting_arr, length, MPI_INT, parent, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        quickSortMP(0, length, rank, numtasks, depth);
        MPI_Send(sorting_arr, length, MPI_INT, parent, 1, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}

