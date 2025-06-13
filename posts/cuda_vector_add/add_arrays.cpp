#include <iostream>
#include <cmath>

/* Function to add idx-th element from two arrays (x and y)
 and store it in sum. Written a la CUDA. */
void add_arr(int N, float *sum, float *x, float *y, int idx)
{
    if (idx < N)
      sum[idx] = y[idx] + x[idx];
}

int main() {
  int N = 1<<20; // About 1M elements
  size_t size = N * sizeof(float);


  float *x = (float *) malloc(size);
  float *y = (float *) malloc(size);
  float *sum = (float *) malloc(size);

  for (int i = 0; i < N; i++) {
    x[i] = 1.0f;
    y[i] = 2.0f;
  }

  // Run kernel
  for (int idx = 0; idx < N; idx++)
    add_arr(N, sum, x, y, idx);

  float max_err = 0.0f;
  //Check if all values in sum are 3
  for (int i = 0; i < N; i++) {
    max_err = std::fmax(max_err, std::fabs(3.0 - sum[i])); 
  }

  std::cout << "max error = " << max_err << std::endl;

  free(x);
  free(y);
  free(sum);

  return 0;
}
