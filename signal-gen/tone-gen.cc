#include<iostream>
#include"tone.hh"
#include"awgn.hh"
#include<getopt.h>
using namespace std;


const float SAMPLE_RATE=44100;
int main(int argc,char ** argv) {

   int center_freq = 0 ;
   int bandwidth   = 0 ;
   float duration  = 0 ; 
   float amplitude = 0 ; 

   static struct option long_options[] = {
    /* These options don't set a flag.
       We distinguish them by their indices. */
    {"center_freq",required_argument, 0, 'c'},
    {"bandwidth"  ,required_argument, 0, 'b'},
    {"duration"   ,required_argument, 0, 'd'},
    {"amplitude"  ,required_argument, 0, 'a'},
    {0, 0, 0, 0}
   };
   /* getopt_long stores the option index here. */
   int option_index = 0;
   int c=0;
   while ((c=getopt_long (argc, argv, "c:b:d:a:",long_options, &option_index)) != -1) {
     
     switch (c) {
       case 'c':
         center_freq=atoi(optarg);
         break;
     
       case 'b':
         bandwidth=atoi(optarg);
         break;
     
       case 'd':
         duration=atof(optarg);
         break;
     
       case 'a':
         amplitude=atof(optarg);
         break;
       
       default:
         printf("Usage : ./tonegen --center_freq F --bandwidth B --duration D --amplitude A \n");
         exit(1);
     }
   }

   /* Print usage  */
   if (center_freq <= 0 || bandwidth < 0 || duration <= 0 || amplitude < 0) { /* amplitude 0 is allowed to gen. silence, bandwidth can be 0 to generate no noise at all  */
         printf("Invalid or incomplete arguments\nUsage : ./tonegen --center_freq F --bandwidth B --duration D --amplitude A \n");
         exit(1);
   }

   /* generate 2000 Hz tone wave */ 
   /* for 0.3 seconds */ 
   /* at 44.1 KHZ*/
   /* Amplitude of 0.9 to avoid saturation */  
   Tone test_tone(center_freq,duration,SAMPLE_RATE,amplitude); 
//   test_tone.to_file("output.dat"); 

   /* Test case 2 : AWGN noise, no interpolation  */
   /* BW of 100 Hz  */
   /* Duration of 0.3 second */ 
   /* Sample_rate of 44.1KHZ */
   /* amplitude of 0.9  */
   
//   AwgnNoise white_noise1(0.9,22000,44000,5); 
//   white_noise1.to_file("noise1.dat");

   /* Test case 3 : AWGN noise, with interpolation  */
   /* BW of 100 Hz  */
   /* Duration of 0.3 second */ 
   /* Sample_rate of 44.1KHZ */
   /* amplitude of 0.9  */

   if(bandwidth!=0) {   
     AwgnNoise white_noise2(bandwidth,duration,SAMPLE_RATE,amplitude); 
  
     /* Test case 4: Shift noise to elsewhere on the spectrum */ 
   
     /* get noise samples generated earlier */ 
     double* noise=white_noise2.get_noise();  
      
     /* multiply them with tone to 'shift' them */
     double *shifted_samples=test_tone.multiply(noise,(long int)SAMPLE_RATE*(duration)); 
  
     /* create a new AwgnNoise wrapper */
     AwgnNoise shifted_noise(white_noise2,shifted_samples);
   
     /* dump new samples */ 
     shifted_noise.to_file("noiseShifted.dat");
   }
   else {
     test_tone.to_file("noiseShifted.dat"); 
   }
}
