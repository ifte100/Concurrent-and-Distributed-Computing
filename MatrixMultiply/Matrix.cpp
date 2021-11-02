#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>

using namespace std;
using namespace std::chrono;

const int N = 100;

void multiplication(int A [N][N], int B [N][N], int output [N][N])
{
    int row, column, multiplying;

    for(row = 0; row < N; row++)
    {
        for(column = 0 ; column < N; column++)
        {
            output[row][column] = 0;

            for( multiplying= 0;multiplying< N; multiplying++)
            {
                output[row][column] += (A[row][multiplying]) * (B[multiplying][column]);
            }
        }
    }
}

int main()
{
    //to store the result of the multiplication
    int outputResult[N][N];

    int A [N][N];
    int B [N][N];

    //initialise random number generator
    srand(time(0));

    //store the start time
    auto start = high_resolution_clock::now();

    //randomly populating the arrays
    for(int i = 0; i < N; i++)
    {
        for(int j = 0; j < N; j++)
        {
            A[i][j] = rand() % 100;
        }
    }

     for(int i = 0; i < N; i++)
    {
        for(int j = 0; j < N; j++)
        {
            B[i][j] = rand() % 100;
        }
    }

    multiplication(A, B, outputResult);

    //store the time (stop time) in a variable
    auto stop = high_resolution_clock::now();

    //calculates the time difference (duration of execution)
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Time taken for multiplication: "
         << duration.count() << " microseconds" << endl;

    cout << "Multiplication result is: \n";

    ofstream file;
    file.open("outputFile.txt");

    for (int i = 0; i < N; i++) 
    {
        for (int j = 0; j < N; j++)
        {
            cout << outputResult[i][j] << " ";

            file << outputResult[i][j] << " ";
        }
        cout << "\n";
        file << "\n";
    }
    
    file.close();

}