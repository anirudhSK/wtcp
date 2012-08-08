#include<iostream>
#include"tone.hh"
#include"awgn.hh"
using namespace std;



int main() {
   /* generate 2000 Hz tone wave */ 
   /* for 0.3 seconds */ 
   /* at 44.1 KHZ*/
   /* Amplitude of 0.9 to avoid saturation */  
   Tone test_tone(2000,0.3,44100,0.9); 
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
   
   AwgnNoise white_noise2(100,0.3,44100,0.9); 
   white_noise2.to_file("noise.dat");

   /* Test case 4: Shift noise to elsewhere on the spectrum */ 
 
   /* get noise samples generated earlier */ 
   double* noise=white_noise2.get_noise();  
    
   /* multiply them with tone to 'shift' them */
   double *shifted_samples=test_tone.multiply(noise,44100*0.3); 

   /* create a new AwgnNoise wrapper */
   AwgnNoise shifted_noise(white_noise2,shifted_samples);
   
   /* dumpo new samples */ 
   shifted_noise.to_file("noiseShifted.dat");
}
