#include "tone.hh"
Tone::Tone(double t_frequency, double t_duration,double t_sample_rate,double t_amplitude)  
     : frequency(t_frequency),
       duration(t_duration) ,
       sample_rate(t_sample_rate),
       amplitude(t_amplitude),
       sample_array(new double [(unsigned long int)(t_duration * t_sample_rate)]),
       PI(atan(1.0)*4)    {

      assert(t_duration*t_sample_rate < MAX_SAMPLES) ;
      unsigned int i =0;
      long double phase=0;
      unsigned long int num_samples=(unsigned long int)(duration * sample_rate) ;
      for(i=0;i<num_samples;i++)  {
        phase= ( i * frequency * 2 * PI ) / sample_rate ; 
        sample_array[i]=amplitude*sin(phase); 
      }
      
   }

void Tone::to_file(std::string file_name) {
     ofstream tone_stream(file_name.c_str()); 
     double time;
     unsigned int i =0; 
     unsigned long int num_samples=(unsigned long int)(duration * sample_rate) ;
     /* header information */ 
     tone_stream<<"; Sample Rate "<<sample_rate<<std::endl;
     tone_stream<<"; Channels 1"<<std::endl;
     for(i=0;i<num_samples;i++)  {
       time=(double)i/sample_rate;
       tone_stream<<time<<"\t"<<sample_array[i]<<std::endl;  
     }
     tone_stream.close();
}
