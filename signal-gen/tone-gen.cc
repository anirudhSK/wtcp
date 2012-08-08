#include<iostream>
#include"tone.hh"
#include"awgn.hh"
using namespace std;



int main() {
   /* generate 1000 Hz tone wave */ 
   /* for 2 seconds */ 
   /* at 44100 Hz */
   /* Amplitude of 0.5 to avoid saturation */  
   Tone test_tone(250,0.3,44100,0.9); 
//   test_tone.to_file("output.dat"); 

   /* Test case 2 : AWGN noise, no interpolation  */
   /* anplitude of 0.5  */
   /* BW of 22 KHz  */
   /* Sample_rate of 44KHZ */
   /* Duration of 1 second */ 
   
   AwgnNoise white_noise1(0.9,22000,44000,5); 
   white_noise1.to_file("noise1.dat");

   /* Test case 2 : AWGN noise, with interpolation  */
   /* anplitude of 0.5  */
   /* BW of 900 Hz  */
   /* Sample_rate of 44KHZ */
   /* Duration of 1 second */ 
   
   AwgnNoise white_noise2(0.9,900,44000,5); 
   white_noise2.to_file("noise.dat");

}
