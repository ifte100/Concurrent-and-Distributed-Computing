#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>


using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int size)
{
    for (int i = 0; i < size; i++)
    {
        //ToDo: Add Comment
        vector[i] = rand() % 100;
    }
}


int main(){

    unsigned long size = 100000000;

    srand(time(0));

    int *v1, *v2, *v3;

    //ToDo: Add Comment
    auto start = high_resolution_clock::now();

    //ToDo: Add Comment
    v1 = (int *) malloc(size * sizeof(int *));
    v2 = (int *) malloc(size * sizeof(int *));
    v3 = (int *) malloc(size * sizeof(int *));


    randomVector(v1, size);

    randomVector(v2, size);

    int totalSum = 0;
    //ToDo: Add Comment
    for (int i = 0; i < size; i++)
    {
        v3[i] = v1[i] + v2[i];
        totalSum += v3[i];
    }

    auto stop = high_resolution_clock::now();

    //ToDo: Add Comment
    auto duration = duration_cast<microseconds>(stop - start);


    cout << "Time taken by function: "
         << duration.count() << " microseconds" << " and the total sum is " << totalSum << endl;

    return 0;
}