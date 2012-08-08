#include"awgn.hh"
#include <fftw3.h>
AwgnNoise::AwgnNoise(double t_amplitude,double t_bandwidth,double t_sample_rate,double t_duration)
        :amplitude(t_amplitude),
         bandwidth(t_bandwidth),
         sample_rate(t_sample_rate),
         duration(t_duration),
         sample_array(new double[(unsigned long int)(sample_rate*duration)]) ,
         pre_interpolation_array(new double[(unsigned long int)(2*bandwidth*duration)])  ,
         rng(new AwgnNoise::Gaussian(0,amplitude)) {

             
        /* Create a Gaussian Random number generator */  
        Gaussian rng_normal(0.0,amplitude); 

        /* generate AWGN noise bandlimited to 'bandwidth' around origin */ 
        /* Need to generate for 'duration' seconds */
        unsigned long int i=0; 
        unsigned long int pre_interp_sample_count =(2*bandwidth*duration);

        for(i=0; i< pre_interp_sample_count; i++)  {
             pre_interpolation_array[i]=rng_normal.sample();   /* white noise samples */  
        }
        /* now interpolate */
        interpolate(pre_interpolation_array,sample_array); 
        
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
       noise_stream<<time<<"\t"<<sample_array[i]<<std::endl;  
     }
     noise_stream.close();

}
void AwgnNoise::interpolate(double* pre_interpolation_array,double* post_interpolation_array) {

       /* input pre_interpolation_array , output sample_array */ 

       unsigned long int pre_interp_sample_count =(2*bandwidth*duration);
       unsigned long int post_interp_sample_count=sample_rate*duration;
       unsigned long int i=0; 

       /* Interpolate to higher sampling rate */ 
       fftw_complex *input_signal, *input_fft, *interp_signal, *interp_fft;
       fftw_plan p;
       
       /* Allocate sizes for input array and its fft */ 
       input_signal  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * pre_interp_sample_count);
       input_fft     = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * pre_interp_sample_count);

       /* plan for fft */
       p = fftw_plan_dft_1d(pre_interp_sample_count, input_signal,input_fft, FFTW_FORWARD, FFTW_ESTIMATE);

       /* init. arrays */
       for(i=0; i< pre_interp_sample_count; i++)  {
            input_signal[i][0]=pre_interpolation_array[i];
            input_signal[i][1]=0; /* imaginary part is 0 */ 
       }

       /* compute fft */  
       fftw_execute(p); 

       /* initialise new fft size for upsampling/interpolating  */
       interp_fft    = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * post_interp_sample_count);
       interp_signal = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * post_interp_sample_count);

       /* plan for new ifft */
       p = fftw_plan_dft_1d(post_interp_sample_count, interp_fft, interp_signal, FFTW_BACKWARD, FFTW_ESTIMATE);

       /* just ensure it is divisable by 2 */
       assert(pre_interp_sample_count % 2 == 0);          

       /* init. arrays to all 0*/
       for(i=0; i< post_interp_sample_count; i++)  {
            interp_fft[i][0]=0; /* real part is 0 */ 
            interp_fft[i][1]=0; /* imaginary part is 0 */ 
       }

       /* initialise the first half to first half of the original input_fft */
       for(i=0;i<pre_interp_sample_count/2;i++) {
            interp_fft[i][0]=input_fft[i][0];
            interp_fft[i][1]=input_fft[i][1];
       }

       /* initialise the last  half to last  half of the original input_fft for conjugate symmetry */ 
       for(i=1;i<pre_interp_sample_count/2;i++) {
            interp_fft[post_interp_sample_count - pre_interp_sample_count/2 + i][0]=input_fft[pre_interp_sample_count/2 + i ][0] ;
            interp_fft[post_interp_sample_count - pre_interp_sample_count/2 + i][1]=input_fft[pre_interp_sample_count/2 + i ][1] ;
       }
       
       /* split the middle signal across the two ends ; further improves interpolation accuracy */ 
       interp_fft[pre_interp_sample_count/2][0]                           =input_fft[pre_interp_sample_count/2][0]/2;
       interp_fft[pre_interp_sample_count/2][1]                           =input_fft[pre_interp_sample_count/2][1]/2;

       interp_fft[post_interp_sample_count - pre_interp_sample_count/2][0]=input_fft[pre_interp_sample_count/2][0]/2; 
       interp_fft[post_interp_sample_count - pre_interp_sample_count/2][1]=input_fft[pre_interp_sample_count/2][1]/2; 

       /* compute ifft */  
       fftw_execute(p); 
     
       /* copy into sample array */
       double scale_factor = 1/sqrt(pre_interp_sample_count*post_interp_sample_count) ;
       for(i=0;i<post_interp_sample_count;i++) {
           post_interpolation_array[i]=scale_factor*interp_signal[i][0]; /* ignore the imaginary part, it should be zero or close to it, due to roundoff errors */ 
       }
        
       /* free up space of all intermediate arrays */ 
       fftw_destroy_plan(p);
       fftw_free(input_signal);fftw_free(input_fft);fftw_free(interp_fft);fftw_free(interp_signal);
}

double AwgnNoise::Gaussian::sample() {
      double u1=(double)rand()/RAND_MAX; 
      double u2=(double)rand()/RAND_MAX;

      /* Box Muller transform */ 
      double n1 = sqrt(-1 * 2 * log(u1) ) * cos (2 * PI * u2) ; 
      //double n2 = sqrt(-1 * 2 * log(u2) ) * cos (2 * PI * u1) ; 

      /* cribs if I generate both variates but use only one. */ 
      return ( ( sigma * n1 ) + mu ); 

}

AwgnNoise::Gaussian::Gaussian(double t_mean, double t_dev) 
   : mu(t_mean),
     sigma(t_dev) ,
     PI(atan(1)*4) {
   srand(0);
}
