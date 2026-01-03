#include "fft.hpp"
#include <iostream>
#include <omp.h>
#include <vector>
#include <print>

int main()
{
    std::vector<int> d{0,1,2,3,4,5,6,7};

    fft::fft2(d.begin(), d.end());

    std::println("{}", d);

    #pragma omp parallel
    {
        printf("Hello World from thread = %d\n", omp_get_thread_num());
    }
    return 0;
}
