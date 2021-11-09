__kernel void vector_add(global const float * restrict x, 
                         global const float * restrict y, 
                         global float * restrict z,
						 const int num_elements)
{
    int index;

    // add the vector elements
	#pragma unroll 16
	for (index = 0; index < num_elements; index++)
		z[index] = x[index] + y[index];
}

