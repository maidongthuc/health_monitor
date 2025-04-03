#pragma once
#include <stdarg.h>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class OneClassSVM {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        float kernels[9] = { 0 };
                        kernels[0] = compute_kernel(x,   85.0  , 99.0  , 36.8  , 80.0  , 73.0 );
                        kernels[1] = compute_kernel(x,   98.0  , 97.0  , 37.8  , 107.0  , 68.0 );
                        kernels[2] = compute_kernel(x,   95.0  , 99.0  , 37.0  , 125.0  , 69.0 );
                        kernels[3] = compute_kernel(x,   87.0  , 98.0  , 36.7  , 80.0  , 73.0 );
                        kernels[4] = compute_kernel(x,   67.0  , 96.0  , 36.7  , 105.0  , 74.0 );
                        kernels[5] = compute_kernel(x,   97.0  , 99.0  , 36.6  , 87.0  , 69.0 );
                        kernels[6] = compute_kernel(x,   95.0  , 99.0  , 37.3  , 92.0  , 55.0 );
                        kernels[7] = compute_kernel(x,   69.0  , 96.0  , 36.8  , 90.0  , 68.0 );
                        kernels[8] = compute_kernel(x,   79.0  , 98.0  , 36.4  , 122.0  , 65.0 );
                        float decision = -3.307890052526 - ( + kernels[0] * 0.495044413195  + kernels[1] * 0.060129527527  + kernels[2]   + kernels[3]   + kernels[4]   + kernels[5] * 0.248709362591  + kernels[6]   + kernels[7] * 0.546116696686  + kernels[8]  );

                        return decision > 0 ? 0 : 1;
                    }

                protected:
                    /**
                    * Compute kernel between feature vector and support vector.
                    * Kernel type: rbf
                    */
                    float compute_kernel(float *x, ...) {
                        va_list w;
                        va_start(w, 5);
                        float kernel = 0.0;

                        for (uint16_t i = 0; i < 5; i++) {
                            kernel += pow(x[i] - va_arg(w, double), 2);
                        }

                        return exp(-0.001 * kernel);
                    }
                };
            }
        }
    }