#include "tone.hh"
#include<assert.h>
#include <iomanip>
using namespace std;
Tone::Tone(double t_frequency, double t_duration,double t_sample_rate,double t_amplitude)  
     : frequency(t_frequency),
       duration(t_duration) ,
       sample_rate(t_sample_rate),
       amplitude(t_amplitude),
       sample_array(new double [(unsigned long int)round(t_duration * t_sample_rate)]),
       PI(atan(1.0)*4)    {

      assert(t_duration*t_sample_rate < MAX_SAMPLES) ;
      unsigned int i =0;
      long double phase=0;
      unsigned long int num_samples=round(duration * sample_rate) ;
      for(i=0;i<num_samples;i++)  {
        phase= ( i * frequency * 2 * PI ) / sample_rate ; 
        sample_array[i]=amplitude*sin(phase); 
      }
      
   }

double Tone::to_file(std::string file_name) {
     ofstream tone_stream(file_name.c_str()); 
     double time;
     unsigned int i =0; 
     unsigned long int num_samples=round(duration * sample_rate) ;
     /* header information */ 
     tone_stream<<"; Sample Rate "<<sample_rate<<std::endl;
     tone_stream<<"; Channels 1"<<std::endl;
     for(i=0;i<num_samples;i++)  {
       time=(double)(i+1)/sample_rate; /* no sample at t=0 */ 
       tone_stream<<std::setprecision(6)<<time<<"\t"<<std::setprecision(6)<<sample_array[i]<<std::endl;  
     }
     tone_stream.close();
     return time ; 
     /* return the last time stamp that you wrote for the next guy to use */
}

double* Tone::multiply(double* noise,unsigned long int length) {
     unsigned long int num_samples=round(duration * sample_rate) ;
     assert(length == num_samples) ; /* No point multiplying two non-equal arrays */
     double* shifted_noise =new double [num_samples];
     unsigned long int i;
     for(i=0;i<num_samples;i++)  {
       shifted_noise[i]=noise[i]*sample_array[i]; /* point wise multiplication */ 
     }
     return shifted_noise; 
}

double Tone::append_to_file(double start_time,std::string file_name)  {
     ofstream tone_stream(file_name.c_str(),ios::app); 
     double time;
     unsigned int i =0; 
     unsigned long int num_samples=round(duration * sample_rate) ;
     /* header information */ 
     for(i=0;i<num_samples;i++)  {
       time=(double)(i+1)/sample_rate+start_time; /* no sample at t=0 */
       tone_stream<<std::setprecision(6)<<time<<"\t"<<std::setprecision(6)<<sample_array[i]<<std::endl;  
     }
     tone_stream.close();
     return time;
}
