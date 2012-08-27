wtcp
====

Wireless transmission control

Audio latency setup :
This documents all the steps for a reproducible measurement of audio latency between two machines running Skype. 

There are 3 machines that are of interest throughout :

1. The Audio Measurement Machine: This is the one resposnsible for playing out a specific sound pattern and measuring the mouth-to-ear latency.
2. The Caller : The "mouth" of the audio latency test.
3. The Callee : The "ear" of the audio latency test.

The Caller and Callee need have no relationship to the machine who initiated the Skype call because once the call is initiated, both machines are peers. Hence, we can freely pick the caller and callee and could potentially measure two sets of latencies.

The short version of this setup is as follows :
AMM speaker on 100%, mic on about 50 %. Caller mic in on 48, gain 0.0dB. Caller speaker out can be anything so long as a headphone is plugged in. Callee mics completely diabled. Callee speaker out is at 81. Callee volume on Skype is 75. 

Detailed Configuration settings :

Audio Measurement Machine (AMM)
--------------------------------------------------------

DELL PRECISION Core2Duo that plays out a specific sound pattern and measures mouth-to-ear latency.
A RadioShack splitter connects into the FRONT PANEL headphone out of the AMM (i.e. next to the power button). 
Another RadioShack splitter connects into the FRONT PANEL mic-in of the AMM.
TODO : Put a mark on the splitter to distinguish the two sides. 


When looking straight at the front panel, the headphone out is on top of the mic in. To setup the measurement, 
 -Take the WHITE cable and connect it to the left side of the mic-in splitter and the left side of the headphone out splitter. 
 -Take the male end of the Audio isolator and connect it into the right side of the head phone out splitter.
 -Take the female end of the Audio isolator and connect the short black cable to it. Connect the other end of the short black cable into the mic-in of the caller.
 -Take the long black cable and connect one end of it into the right side of the mic-in splitter of the AMM. Connect the other end of this long black cable into the head phone out of the callee. 

Volume settings : (Click the sound icon on the top right on Ubuntu)
 Make sure neither speaker nor mic is muted. 
 Make sure the speaker is on 100% on the AMM (not any more than  100 %). 
 On the Output tab :
  Output should be set to "Internal Audio Analog Stereo" (remove any other USB / other mics).
  Balance of left and right should be 0 (i.e. completely balanced).
 On the Input tab :
  Input should be set to Microphone 1 and again "Internal Audio Analog Stereo" must be checked. 
  Mic volume (or should I say gain should be about 50 %). I cannnot say exactly because Linux does not display numbers. 
 On the Hardware tab :
  Profile should say Analog Stereo Duplex. 
 On the Sound Effects tab :
  Make sure Alert volume is set to zero and muted. 
  Sound theme should be set to "No sounds"

Caller machine :
=================================================================
Lenovo Think Centre with MAC address 44:37:e6:a4:16:fe. 
NOTE: You could interchange this machine with the callee machine and swap their roles as caller and callee, BUT due to some clock drift the swapped arrangement results in latencies going all the way to 300 ms. So, stick with this.

Insert the other end of the short black cable from above into the mic in of this machine. Open Sound preferences by typing Sound in the windows 7 command bar.

SYSTEM SETTINGS :
------------------------------------------------------------------
Go to the Playback tab first :
 -Make sure Front Panel 3.5mm Jack is checked. 
 -Go to the Levels tab inside the Playback tab. 
    -Set the speaker level to anything you want to, and make sure to plug in a headphone into the headphone out of the caller. This is just for debugging by looping back the mic in on the caller into its headphone out, so it does not matter what this speaker level is SO LONG AS there is a headphone plugged into it. I usually set it to 94.
    -Make sure on the levels tab, the Microphone and Line In are both turned OFF (i.e. mute them by pressing the sound icon next to them). These two are micrphones and Line In associated with the Playback device, and not the recording device which we will use to feed in sound. Having these on cause ground noise in some cases. So make sure they are off. 
    -The Balance on the Speaker should be equal on both left and right.
    -Center, Subwoofer,rear and front should all be 100. Rear and Front should both be balanced. 
 -Go to the Enhancements tab and ensure that Immediate Mode alone is checked, everything else is unchecked and Setting is set to None. 
 -Go to the Advanced tab and make sure 24bit,48000Hz (Studio Quality) is selected. Also ensure both boxes under "Exclusive Mode" are checked. 
 -Press Ok to save changes

Go to the Recording tab :
 -Double Click on the microphone that says "Realtek High Definition Audio". Make sure this one shows a green tick next to it, and make sure nothing else does. If anything else does, disable it by right clicking.
 -On the General tab, make sure Fron Panel 3.5mm Jack is checked and "Use this device(enable)" is selected. 
 -On the Listen tab, 
   -Check "Listen to this device", to hear through the headphone.
   -There is some stuff on Battery Power, where it says continue running when on battery power, and check this just to be consistent. 
   -Also check Do not prompt when this device is plugged in
 -On the level tab, IMPORTANT
   -Set microphone level to 48 and make sure it is NOT muted.
   -Set the Microphone Boost to 0.0 dB (lowest). 
 -On the Enhancements tab, make sure "Immediate Mode" and "DC Offset Cancellation" are both checked to be true. Everything else should be unchecked. 
 -On the Advanced tab, make sure, 2 channel , 16 bit, 44100 Hz (CD Quality) is set and also ensure both boxes under "Exclusive Mode" are checked.
 -Press Ok to save changes

On The Sounds tab :
 -Leave the theme at Windows Default. "Play startup sound" should be checked.

On the Communications tab :
 -Make sure "Reduce the volume of other sounds by 80 %" is checked.

Skype Settings :
--------------------------------------------------------------------
Go to Tools->Options->Audio Settings :
  MAKE SURE BOTH "Autommatically adjust speaker settings" and "Automatically adjust microphone settings" are unchecked. This can be a big headache otherwise. 
  Set Speaker Volume at 70%. (This is in addition to the System Sound settings)
  Leave the Mic setting at wherever it is (It mirrors the System mic settings)
  Make sure Speaker output is set to "Speakers (Realtek High Definition Audio)".
  Make sure Microphone input is set to "Microphone (Realtek High Definition Audio)". 
  This above step is especially important AFTER you have connected a webcam because it will switch to using the webcam mic, which will screw all results. 

Callee machine :
=================================================================
Lenovo Think Centre with MAC address 44:37:e6:a4:2c:71
Read note above on interchanging caller and callee. 
Connect other end of long black cable into headphone out of this machine. The first end goes into mic in of AMM.

SYSTEM SETTINGS :
------------------------------------------------------------------
Go to the Playback tab first :
 -Make sure Front Panel 3.5mm Jack is checked. 
 -Go to the Levels tab inside the Playback tab. 
   -IMPORTANT: Set the speaker level to 81 and make sure left and right are balanced. 
   -Make sure on the levels tab, the Microphone and Line In are both turned OFF (i.e. mute them by pressing the sound icon next to them). These two are micrphones and Line In associated with the Playback device. Having these on cause ground noise in some cases. So make sure they are off. 
   -Center, Subwoofer,rear and front should all be 100. Rear and Front should both be balanced. 
   -Go to the Enhancements tab and ensure that Immediate Mode alone is checked, everything else is unchecked and Setting is set to None. 
   -Go to the Advanced tab and make sure 24bit,48000Hz (Studio Quality) is selected. Also ensure both boxes under "Exclusive Mode" are checked. 
   -Press Ok to save changes

Go to the Recording tab next :
  -Make sure no microphones are checked as active (i.e. with a green tick next to them). If you have plugged in a webcam whose mic shows up here, disable it. You can always re-enable it  if required.

On The Sounds tab :
 -Leave the theme at Windows Default. "Play startup sound" should be checked.

On the Communications tab :
 -Make sure "Reduce the volume of other sounds by 80 %" is checked. 

Skype Settings :
--------------------------------------------------------------------
Go to Tools->Options->Audio Settings :
  MAKE SURE BOTH "Autommatically adjust speaker settings" and "Automatically adjust microphone settings" are unchecked. This can be a big headache otherwise. 
  Set Speaker Volume at 75%. (This is in addition to the System Sound settings)
  Leave the Mic setting at wherever it is (It mirrors the System mic settings)
  Make sure Speaker output is set to "Speakers (Realtek High Definition Audio)".
  Make sure Microphone input is set to "None". 
  The above step is especially important AFTER you have connected a webcam because it will switch to using the webcam mic, which will screw all results. 

