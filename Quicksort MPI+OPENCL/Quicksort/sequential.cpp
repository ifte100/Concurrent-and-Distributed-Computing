#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mpi.h>
#include <CL/cl.h>

using namespace std;
using namespace std::chrono;

int SIZE = 1000;
int length;
int *sorting_arr;

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
void setup_kernel_memory(int low, int openCLPartition, int* output);
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

void quickSortOpenCL(int low, int high)
{
    int openCLPartition = high - low;
    randomArray(output, openCLPartition);

    size_t global[1] = {(size_t)openCLPartition};
    size_t local[1] = {(size_t)openCLPartition};

    setup_openCL_device_context_queue_kernel((char *)"./quickSort.cl", (char *)"quickSortCL");

    setup_kernel_memory(low, openCLPartition, output);
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
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, local, 0, NULL, &event);
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
    clEnqueueReadBuffer(queue, buf2, CL_TRUE, 0, openCLPartition * sizeof(int), &sorting_arr[low], 0, NULL, NULL);
}

void quickSortMP(int low, int high, int rank, int numtasks, int depth)
{

    int child_rank = rank + (1 << depth);

    //normal sequential quicksort when the array is too small for child nodes to exist
    if(child_rank >= numtasks)
    {
        //cout<<"Im in if"<<endl;
        quickSortOpenCL(low, high);
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

int main(int argc, char** argv) {

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

        cout << "Time taken by function: "
            << duration.count() << " microseconds" << endl;
    }
    else
    {
        int depth=0, parent=0;
        //depth increases by 1 every 2,4,8 so on..rank
        while( 1 << depth <= rank)
        {
            depth++;
        } 
        
        //we need to use PRobe to get size
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        MPI_Get_count(&stat, MPI_INT, &length);
        //get the parent node
        parent = stat.MPI_SOURCE;

        sorting_arr = (int*)malloc(sizeof(int) * length);

        //receiving the data from node
        MPI_Recv(sorting_arr, length, MPI_INT, parent, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        quickSortMP(0, length, rank, numtasks, depth);
        //return sorting_arr to the parent node
        MPI_Send(sorting_arr, length, MPI_INT, parent, 1, MPI_COMM_WORLD);

    }

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

void setup_kernel_memory(int low, int openCLPartition, int* output)
{
    buf1 = clCreateBuffer(context, CL_MEM_READ_WRITE, openCLPartition * sizeof(int), NULL, NULL);
    buf2 = clCreateBuffer(context, CL_MEM_READ_WRITE, openCLPartition * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, buf1, CL_TRUE, 0, openCLPartition * sizeof(int), &sorting_arr[low], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf2, CL_TRUE, 0, openCLPartition * sizeof(int), &output, 0, NULL, NULL);
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