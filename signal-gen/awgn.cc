#include"awgn.hh"
AwgnNoise::AwgnNoise(double t_amplitude,double t_bandwidth,double t_sample_rate,double t_duration)
        :amplitude(t_amplitude),
         bandwidth(t_bandwidth),
         sample_rate(t_sample_rate),
         duration(t_duration),
         sample_array(new double[(unsigned long int)(sample_rate*duration)]) ,
         pre_interpolation_array(new double[(unsigned long int)(2*bandwidth*duration)])  ,
          rng(new AwgnNoise::Gaussian(0,amplitude)) {

        /* generate AWGN noise bandlimited to bandwidth around origin */ 
        /* Need to generate for duration seconds */
        unsigned long int pre_interp_sample_count=(2*bandwidth*duration);
        unsigned long int i=0; 
     
        /* Create a Gaussian Random number generator */  
        Gaussian rng_normal(0.0,amplitude); 
        for(i=0; i< pre_interp_sample_count; i++)  {
             pre_interpolation_array[i]=rng_normal.sample(); 
        }
       
        /* TODO: Interpolate to new frequency */ 
        
}

void AwgnNoise::to_file(std::string file_name) {
     ofstream noise_stream(file_name.c_str()); 
     double time;
     unsigned int i =0; 
     unsigned long int num_samples=(unsigned long int)(duration * sample_rate) ;
     /* header information */ 
     noise_stream<<"; Sample Rate "<<sample_rate<<std::endl;
     noise_stream<<"; Channels 1"<<std::endl;
     for(i=0;i<num_samples;i++)  {
       time=(double)i/sample_rate;
       noise_stream<<time<<"\t"<<pre_interpolation_array[i]<<std::endl;  
     }
     noise_stream.close();

}
void AwgnNoise::interpolate() {
       //unsigned long int upsampled_length=(unsigned long int)sample_rate*duration;
       //unsigned long int current_length=(unsigned long int)(2*bandwidth*duration); 
       assert(0<1);
       /* Do an FFT of the current set of samples */
       /* TODO Fill this in later, for now, just copy */ 
}

double AwgnNoise::Gaussian::sample() {
      double u1=(double)rand()/RAND_MAX; 
      double u2=(double)rand()/RAND_MAX;

      /* Box Muller transform */ 
      double n1 = sqrt(-1 * 2 * log(u1) ) * cos (2 * PI * u2) ; 
      //double n2 = sqrt(-1 * 2 * log(u2) ) * cos (2 * PI * u1) ; 

      /* cribs if I generate both variates but use only one. */ 
//      std::cout<<"Printing out variables u1 and u2 "<<u1<<","<<u2<<std::endl;
      return ( ( sigma * n1 ) + mu ); 

}

AwgnNoise::Gaussian::Gaussian(double t_mean, double t_variance) 
   : mu(t_mean),
     sigma(t_variance) ,
     PI(atan(1)*4) {
   srand(0);
}
