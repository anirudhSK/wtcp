#include<cmath>
#include<string>
#include <assert.h>
#include <iostream>
#include <fstream>
using namespace std; 
class Tone {
   
  private : 
    const double frequency; 
    const double duration ; 
    const double sample_rate; 
    const double amplitude;
    double* sample_array;    
    static const int MAX_SAMPLES=1000000; // MAXIMUM SIZE OF SAMPLE ARRAY
    const double PI;  
  public : 
 
   Tone(double t_frequency, double t_duration,double t_sample_rate,double t_amplitude); 
   void to_file(std::string file_name) ;  
   double* multiply(double* noise,unsigned long int length); /* multiply with AWGn to "move" noise */
} ; 
