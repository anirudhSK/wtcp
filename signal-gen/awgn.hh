#include<cmath>
#include<string>
#include <assert.h>
#include <iostream>
#include <fstream>
using namespace std; 
class AwgnNoise { 

    private : 
      const double amplitude;
      const double bandwidth; 
      const double sample_rate ;
      const double duration;
      double*   sample_array;  
      double*   pre_interpolation_array; 


      class Gaussian {
            private :
             const double mu;
             const double sigma;
             const double PI;
            public : 
             Gaussian(double t_mean, double t_dev);
             double sample(void);
           } ; 
     
      const AwgnNoise::Gaussian* rng; 
         
    public : 
       AwgnNoise(double t_amplitude, double t_bandwidth,double t_sample_rate,double t_duration) ;
       void interpolate(double* pre_interpolation_array,double* post_interpolation_array) ;
       void to_file(std::string file_name) ;  
} ; 
