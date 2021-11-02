#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mpi.h>
#include <CL/cl.h>

using namespace std;
using namespace std::chrono;

int SIZE = 500;
int length;
int *sorting_arr;
int *recv_arr;

cl_mem buf1;
cl_mem buf2;
int *output;//for opencl

//getting the devices on a platform
cl_device_id device_id;

//gettng the context-environment where kernels execute and memory management is done
cl_context context;

//storing a program object for the context
cl_program program;

//storing the kernel object
cl_kernel kernel;

//OpenCL objects such as memory, program and kernel objects are created using a context.
//Operations on these objects are performed using a command-queue
cl_command_queue queue;
cl_event event = NULL;

int err;

//creeating a device, actually accessing a specific device such as CPU or GPU
cl_device_id create_device();

//setting up OpenCL device, context, queue and Kernel
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);

//after program is created using clCreateProgramWithSource, this builds (compiles & links) a program executable from the program source
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);

//setting up kernel memory by creating buffer
void setup_kernel_memory();
void copy_kernel_args();
void free_memory();

void randomArray(int* &array, int size)
{   
    array = (int*)malloc(sizeof(int) * size);

    for (int i = 0; i < size; i++)
    {
        array[i] = rand() % 100;
    }
}

int main(int argc, char** argv) {
    
    srand(time(0));

    int numtasks, rank, name_len, dest, src, count; 
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status stat;
    // Initialize the MPI environment
    MPI_Init(NULL,NULL);

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    int scatter_size = SIZE / numtasks;
    
    randomArray(sorting_arr, SIZE);

    if(rank == 0)
    {
        cout << "Im in Head" <<endl;

        recv_arr = (int *)malloc(scatter_size * sizeof(int *));

        cout << "Before sorting: \n";
        for (int i = 0; i < SIZE; i++)
        {
            cout << sorting_arr[i] << " ";
        }

        auto start = high_resolution_clock::now();

        MPI_Scatter(sorting_arr, scatter_size, MPI_INT, recv_arr, scatter_size, MPI_INT, 0, MPI_COMM_WORLD);

        size_t global[1] = {(size_t)SIZE};

        setup_openCL_device_context_queue_kernel((char *)"./quickSort.cl", (char *)"quickSortCL");

        setup_kernel_memory();
        copy_kernel_args();

    /*  Enqueues a command to execute a kernel on a device
        clEnqueueNDRangeKernel (cl_command_queue command_queue,
            cl_kernel kernel,
            cl_uint work_dim,
            const size_t *global_work_offset,
            const size_t *global_work_size,
            const size_t *local_work_size,
            cl_uint num_events_in_wait_list,
            const cl_event *event_wait_list,
            cl_event *event) */
        clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
        clWaitForEvents(1, &event);

    /*  Enqueue commands to read from a buffer object
        clEnqueueReadBuffer (cl_command_queue command_queue,
            cl_mem buffer,
            cl_bool blocking_read,
            size_t offset,
            size_t size,
            void *ptr,
            cl_uint num_events_in_wait_list,
            const cl_event *event_wait_list,
            cl_event *event) */
        clEnqueueReadBuffer(queue, buf1, CL_TRUE, 0, SIZE * sizeof(int), &sorting_arr[0], 0, NULL, NULL);

        MPI_Gather(MPI_IN_PLACE, scatter_size, MPI_INT, recv_arr, scatter_size, MPI_INT, 0, MPI_COMM_WORLD);

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
        cout << "Im in node" <<endl;

        recv_arr = (int *)malloc(scatter_size * sizeof(int *));

        MPI_Scatter(NULL, scatter_size, MPI_INT, recv_arr, scatter_size, MPI_INT, 0, MPI_COMM_WORLD);
        
        size_t global[1] = {(size_t)SIZE};

        setup_openCL_device_context_queue_kernel((char *)"./quickSort.cl", (char *)"quickSortCL");

        setup_kernel_memory();
        copy_kernel_args();

        clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
        clWaitForEvents(1, &event);

        clEnqueueReadBuffer(queue, buf1, CL_TRUE, 0, SIZE * sizeof(int), &sorting_arr[0], 0, NULL, NULL); 

        MPI_Gather(recv_arr, scatter_size, MPI_INT, NULL, scatter_size, MPI_INT, 0, MPI_COMM_WORLD);
    }

    free_memory();
    MPI_Finalize();
}

void free_memory()
{
    //free the buffers
    clReleaseMemObject(buf1);
    clReleaseMemObject(buf2);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(sorting_arr);
}

void copy_kernel_args()
{
    //To execute a kernel, the kernel arguments must be set.
    //Parameters(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value)
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buf1);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buf2);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    buf1 = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(int), NULL, NULL);
    buf2 = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, buf1, CL_TRUE, 0, SIZE * sizeof(int), &sorting_arr[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf2, CL_TRUE, 0, SIZE * sizeof(int), &output[0], 0, NULL, NULL);
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

/* int partition(__global int *array, int low, int high) 
{
    //start from the rightmost element as pivot. We can also select leftmost or randomly.
    int pivot = array[high];
    int temp;

    //smaller element index
    int i = (low - 1);

    for(int elements = low; elements < high; elements++)
    {
        //if current is smaller than or equals to pivot we swap
        if(array[elements] <= pivot)
        {
            i++;
            temp = array[i];
            array[i] = array[elements];
            array[elements] = temp;
        }
      
    }
  //swapping with greeater element at i
    temp = array[i + 1];
    array[i + 1] = array[high];
    array[high] = temp;

    return (i + 1);
}

__kernel void iterativequicksort(__global int* array,__global int* stack, int low, int high) 
{
    int top = -1; 

    stack[++top] = low; 
    stack[++top] = high; 
  
    if(low <= high)
    {
        while (top >= 0) 
        { 
            high = stack[top--]; 
            low = stack[top--]; 

            int var_partition = partition(array, low, high); 

            if (var_partition - 1 > low) 
            { 
                stack[++top] = low; 
                stack[++top] = var_partition - 1; 
            } 

            if (var_partition + 1 < high) 
            { 
                stack[++top] = var_partition + 1; 
                stack[++top] = high; 
            } 
        }
    }
}

__kernel void quickSortCL(__global int* input, __global int* output) 
{
    const int totalIndex = get_global_id(0);

    iterativequicksort(input, output, 0, totalIndex);

    for(int i = 0; i <= totalIndex; i++)
        output[i] = input[i];
} */