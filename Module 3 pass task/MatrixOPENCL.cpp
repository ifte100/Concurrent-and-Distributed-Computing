#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mpi.h>
#include <omp.h>
#include <CL/cl.h>

using namespace std;
using namespace std::chrono;

const int size = 100;
//to store the result of the multiplication
int *outputResult;

int *A;
int *B;

//These memory object use OpenCL type cl_mem. Holds data used in an OpenCL program.
cl_mem bufV;
cl_mem bufV2;
cl_mem bufV3;

//getting the devices on a platform
cl_device_id device_id;

//gettng the context-environment where kernels execute and memory management is done
cl_context context;

//storing a program object for the context
cl_program program;

//storing the kernel object
cl_kernel kernel;

cl_command_queue queue;
cl_event event = NULL;

int err;

//creating a device, actually accessing a specific device such as CPU or GPU
cl_device_id create_device();

//setting up OpenCL device, context, queue and Kernel
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);

//after program is created using clCreateProgramWithSource, this builds (compiles & links) a program executable from the program source
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);

//setting up kernel memory by creating buffer
void setup_kernel_memory();

//setting up the kernel arguments to execute a kernel
void copy_kernel_args();

//frees memory for device, kernel, queue, etc.
void free_memory();

//OpenCL
void openCL();

void randomArray(int *array)
{
    for(int i = 0; i < size * size; i++)
    {
        array[i] = rand() % 100;
    }
}

int main(int argc, char** argv)
{
    //initialise random number generator
    srand(time(0));

    //store the start time
    auto start = high_resolution_clock::now();

    //openMP set number of threads
    //omp_set_num_threads(NO_OF_THREADS);

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
/*NOTE THAT COULD NOT WORK WITH 2D ARRAYS IN OPENCL SO INSTEAD,
WORKING WITH A 1D ARRAY THAT IS TRANSFORMED FROM 2D*/
    if(rank == 0) 
    {
        A = (int *)malloc(size * size * sizeof(int *));
        B = (int *)malloc(size * size * sizeof(int *));
        outputResult = (int *)malloc(size * size * sizeof(int *));
    
        randomArray(A);
        randomArray(B);
    
        MPI_Scatter(&A[0], scatter_len, MPI_INT, &A, 0, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&B[0], broadcast_len, MPI_INT, 0, MPI_COMM_WORLD);
        openCL();
        MPI_Gather(MPI_IN_PLACE, scatter_len, MPI_INT, &outputResult[0], scatter_len, MPI_INT, 0, MPI_COMM_WORLD);

        
        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(stop - start);


        cout << "Time taken by function: "
                << duration.count() << " microseconds "<<endl;

        //cout<<"IM IN HEAD!"<<endl;    
        
        ofstream file;
        file.open("outputFile.txt");

        for (int i = 0; i < size; i++) 
        {
            for (int j = 0; j < size; j++)
            {
                cout << outputResult[i] << " ";

                file << outputResult[i] << " ";
            }
            cout << "\n";
            file << "\n";
        }
        
        file.close();

        //frees memory for device, kernel, queue, etc.
        free_memory();
    }
    else if(rank != 0)
    {
        A = (int *)malloc(size * size * sizeof(int *));
        B = (int *)malloc(size * size * sizeof(int *));
        outputResult = (int *)malloc(size * size * sizeof(int *));

        randomArray(A);
        randomArray(B);
        
        MPI_Scatter(NULL, scatter_len, MPI_INT, &A[0], scatter_len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&B[0], broadcast_len, MPI_INT, 0, MPI_COMM_WORLD);
        openCL();
        MPI_Gather(&outputResult[0], scatter_len, MPI_INT, NULL, scatter_len, MPI_INT, 0, MPI_COMM_WORLD);

        //Print(outputResult);
        //  for (int i = 0; i < size; i++) 
        // {
        //     for (int j = 0; j < size; j++)
        //     {
        //         cout << outputResult[i] << " ";
        //     }
        //     cout << "\n";
        // }
        //cout<<"IM IN NODE!"<<endl;
        //frees memory for device, kernel, queue, etc.
        free_memory();
    }

    if(rank = 0)
    {
        MPI_Finalize();
    }
}

void openCL()
{
     //to describe the total number of work-items in work_dim dimensions that will execute the kernel function
    size_t global[3] = {(size_t)size, (size_t)size, (size_t)size};

    setup_openCL_device_context_queue_kernel((char *)"./multiply.cl", (char *)"multiplication");

    setup_kernel_memory();
    copy_kernel_args();

    clEnqueueNDRangeKernel(queue, kernel, 3, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    //Enqueue commands to read from a buffer object
    clEnqueueReadBuffer(queue, bufV, CL_TRUE, 0, size * size * sizeof(int), &A[0], 0, NULL, NULL);
    clEnqueueReadBuffer(queue, bufV2, CL_TRUE, 0, size * size * sizeof(int), &B[0], 0, NULL, NULL);
    clEnqueueReadBuffer(queue, bufV3, CL_TRUE, 0, size * size * sizeof(int), &outputResult[0], 0, NULL, NULL);
}

void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufV);
    clReleaseMemObject(bufV2);
    clReleaseMemObject(bufV3);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(A);
    free(B);
    free(outputResult);
}

void copy_kernel_args()
{
    //To execute a kernel, the kernel arguments must be set.
    //Parameters(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value)
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&size);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufV2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufV3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    //clCreateBuffer function creates a buffer object 
    bufV = clCreateBuffer(context, CL_MEM_READ_WRITE, size * size * sizeof(int), NULL, NULL);
    bufV2 = clCreateBuffer(context, CL_MEM_READ_WRITE, size * size * sizeof(int), NULL, NULL);
    bufV3 = clCreateBuffer(context, CL_MEM_READ_WRITE, size * size * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, bufV, CL_TRUE, 0, size * size * sizeof(int), &A[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, size * size * sizeof(int), &B[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV3, CL_TRUE, 0, size * size * sizeof(int), &outputResult[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    //creating an OpenCL context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    //creates a host or device command-queue on a specific device.
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };


    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    //Creates a program for a context. 
    //clCreateProgramWithSource(valid context, count of programs, array of count pointers, array with number of chars in each string, error code)
    program = clCreateProgramWithSource(ctx, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program 
   The fourth parameter accepts options that configure the compilation. 
   These are similar to the flags used by gcc. For example, you can 
   define a macro with the option -DMACRO=VALUE and turn off optimization 
   with -cl-opt-disable.
   */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {
        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}