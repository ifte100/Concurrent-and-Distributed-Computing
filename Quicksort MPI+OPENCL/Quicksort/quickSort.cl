int partition(__global int *array, int low, int high) 
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
}
