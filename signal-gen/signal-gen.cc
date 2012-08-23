#include<iostream>
#include"tone.hh"
#include"awgn.hh"
#include<getopt.h>
#include<stdlib.h> // for the random freq selection
using namespace std;


int get_random_tone(int last_freq) {
   /* get a random tone making sure it's differen from the last one. */
   int current_freq;
   do {   
       current_freq=floor(((double)rand()/RAND_MAX)*10) + 1 ; 
       /* steps of 100 Hz , so multiply by 100 */
       current_freq=current_freq*100; 
   }
   while(current_freq==last_freq) ;
   return current_freq;
}
const float SAMPLE_RATE=44100;
int main(int argc,char ** argv) {

   int num_batches        = 0;
   float amplitude        = 0; 
   int pulses_per_batch   = 0;
   float pulse_duration   = 0; 
   float batch_separation = 0; 

   static struct option long_options[] = {
    /* These options don't set a flag.
       We distinguish them by their indices. */
    {"num_batches"     ,required_argument, 0, 'n'},
    {"amplitude"       ,required_argument, 0, 'a'},
    {"pulses_per_batch",required_argument, 0, 'p'},
    {"pulse_duration"  ,required_argument, 0, 'd'},
    {"batch_separation",required_argument, 0, 's'},
    {0, 0, 0, 0}
   };
   /* getopt_long stores the option index here. */
   int option_index = 0;
   int c=0;
   while ((c=getopt_long (argc, argv, "n:a:p:d:s",long_options, &option_index)) != -1) {
     
     switch (c) {
       case 'n':
         num_batches=atoi(optarg);
         break;
     
       case 'a':
         amplitude=atof(optarg);
         break;
       
       case 'p':
         pulses_per_batch=atoi(optarg);
         break;

       case 'd':
         pulse_duration=atof(optarg);
         break;

       case 's':
         batch_separation=atof(optarg);
         break;

       default:
         printf("Invalid or incomplete arguments\nUsage : ./signal-gen --num_batches B --amplitude A --pulses_per_batch P --pulse_duration D --batch_separation S \nFrequencies are randomly selected between 100Hz and 1000Hz in steps of 1000Hz\nThere is not separation between pulses\n");
         exit(1);
     }
   }

   /* Print usage  */
   if (  num_batches < 1 || amplitude < 0 || pulses_per_batch < 1 || pulse_duration <= 0 || batch_separation < 0  ) { /* amplitude 0 is allowed to gen. silence, bandwidth can be 0 to generate no noise at all  */
         printf("Invalid or incomplete arguments\nUsage : ./signal-gen --num_batches B --amplitude A --pulses_per_batch P --pulse_duration D --batch_separation S \nFrequencies are randomly selected between 100Hz and 1000Hz in steps of 1000Hz\nThere is not separation between pulses\n");
         exit(1);
   }

   /* randomise frequencies */
   srand(0); 

   int i=0; 
   int j=0; 
   double last_time;
   int last_freq =0;
   int current_freq =0;  
   for (i=0;i<num_batches;i++)  {
    for (j=0;j<pulses_per_batch;j++) {
        current_freq=get_random_tone(last_freq); 
        std::cout<<"Batch "<<i<<" Pulse "<<j<<" Frequency "<<current_freq<<" at time "<<last_time<<"\n";
        Tone current_tone(current_freq,pulse_duration,SAMPLE_RATE,amplitude); 
        if ((i==0)  && (j==0) ) {
            last_time=current_tone.to_file("signal.dat"); 
        }
        else {
            last_time=current_tone.append_to_file(last_time,"signal.dat");
        }
        last_freq=current_freq;
     }
     /* inter batch silence to allow Skype time to adapt */
     Tone current_tone(current_freq,batch_separation,SAMPLE_RATE,0.0);
     std::cout<<"Silence at time "<<last_time<<"\n";
     last_time=current_tone.append_to_file(last_time,"signal.dat");
   }
}
