#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>

using namespace std;
using namespace std::chrono;

const int SIZE = 10000000;


void randomArray(int array[], int size)
{
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
int partition(int array[], int low, int high)
{
    //start from the rightmost element as pivot. We can also select leftmost or randomly.
    int pivot = array[high];

    //smaller element index
    int i = (low - 1);

    for(int elements = low; elements < high; elements++)
    {
        //if current is smaller than or equals to pivot we swap
        if(array[elements] <= pivot)
        {
            i++;
            swapping(&array[i], &array[elements]);
        }
      
    }
  //swapping with greeater element at i
    swapping(&array[i + 1], &array[high]);
    return (i + 1);
}

//need low as the starting of array and high as the ending
void quickSort(int array[], int low, int high)
{
    //for first iteration it will be checking if the last element is greater than the first, then swap, and so on...we find the pivot
    if (low < high)
    {
        int var_partition = partition(array, low, high);

        //total no. of threads will be equal to total no. of calls to the function
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                //call on the left of pivot to sort
                quickSort(array, low, var_partition - 1);
            }
            #pragma omp section
            {
                //call on the right of pivot to sort
                quickSort(array, var_partition + 1, high);
            }
        }
    }
}

int main() {

    int data[SIZE];

    randomArray(data, SIZE);

    // cout << "Before sorting: \n";
    // for (int i = 0; i < SIZE; i++)
    // {
    //     cout << data[i] << " ";
    //     cout << endl;
    // }

    srand(time(0));

    auto start = high_resolution_clock::now();

    quickSort(data, 0, SIZE - 1);

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop - start);

    // cout << "After sorting: \n";
    // for (int i = 0; i < SIZE; i++)
    // {
    //     cout << data[i] << " ";
    //     cout << endl;
    // }

    cout << "Time taken by function: "
         << duration.count() << " microseconds" << endl;

}
