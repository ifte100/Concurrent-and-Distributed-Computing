#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono;

#define PRINT 1

int SZ = 8;
//input vectors
int *v;
int *v2;
//output vectors
int *v3;



//Device has global memory. Global memory has memory objects.
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

//setting up the kernel arguments to execute a kernel
void copy_kernel_args();

//frees memory for device, kernel, queue, etc.
void free_memory();

void init(int *&A, int size);
void print(int *A, int size);

int main(int argc, char **argv)
{
    if (argc > 1)
        SZ = atoi(argv[1]);

    init(v, SZ);
    init(v2, SZ);

    v3 = (int *)malloc(sizeof(int) * size);

    auto start = high_resolution_clock::now();
    //to describe the total number of work-items in work_dim dimensions that will execute the kernel function
    size_t global[1] = {(size_t)SZ};

    //initial vector
    print(v, SZ);
    print(v2, SZ);

    setup_openCL_device_context_queue_kernel((char *)"./vector_ops.cl", (char *)"square_magnitude");

    setup_kernel_memory();
    copy_kernel_args();

    //Enqueues a command to execute a kernel on a device
    //clEnqueueNDRangeKernel (cl_command_queue command_queue,
        //cl_kernel kernel,
        //cl_uint work_dim,
        //const size_t *global_work_offset,
        //const size_t *global_work_size,
        //const size_t *local_work_size,
        //cl_uint num_events_in_wait_list,
        //const cl_event *event_wait_list,
        //cl_event *event)
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    //Enqueue commands to read from a buffer object
    //clEnqueueReadBuffer (cl_command_queue command_queue,
        //cl_mem buffer,
        //cl_bool blocking_read,
        //size_t offset,
        //size_t size,
        //void *ptr,
        //cl_uint num_events_in_wait_list,
        //const cl_event *event_wait_list,
        //cl_event *event)
    clEnqueueReadBuffer(queue, bufV, CL_TRUE, 0, SZ * sizeof(int), &v[0], 0, NULL, NULL);
    clEnqueueReadBuffer(queue, bufV2, CL_TRUE, 0, SZ * sizeof(int), &v2[0], 0, NULL, NULL);
    clEnqueueReadBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);
    
    high_resolution_clock::time_point stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken for vector addition: " << duration.count() << " ms" << endl;

    //result vector
    print(v, SZ);
    print(v2, SZ);
    print(v3, SZ);

    //frees memory for device, kernel, queue, etc.
    //you will need to modify this to free your own buffers
    free_memory();
}

void init(int *&A, int size)
{
    A = (int *)malloc(sizeof(int) * size);

    for (long i = 0; i < size; i++)
    {
        A[i] = rand() % 100; // any number less than 100
    }
}

void print(int *A, int size)
{
    if (PRINT == 0)
    {
        return;
    }

    if (PRINT == 1 && size > 15)
    {
        for (long i = 0; i < 5; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
        printf(" ..... ");
        for (long i = size - 5; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    printf("\n----------------------------\n");
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

    free(v);
    free(v2);
    free(v3);
}


void copy_kernel_args()
{
    //To execute a kernel, the kernel arguments must be set.
    //Parameters(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value)
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
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
    //A buffer object is created. Parameters(valid context, flags, size in bytes of buffer object, 
            //host_ptr-a pointer to buffer data already allocated by application, error code)
    //flags is a bit-field that is used to specify allocation and usage information such as the memory
            //arena that should be used to allocate the buffer object and how it will be used
    //If value specified for flags is 0, the default is used which is CL_MEM_READ_WRITE.
    bufV = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV2 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV3 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, bufV, CL_TRUE, 0, SZ * sizeof(int), &v[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, SZ * sizeof(int), &v2[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);
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