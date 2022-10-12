# Turing Machine Module

An embedded software implementation of a Turing Machine module for a Eurorack system  using an Arduino Uno, a shift register and a DAC. 
[Here is an example of an all-hardware based Turing Machine module made by Music Thing Modular](https://www.thonk.co.uk/shop/turingmkii/)

[... and a video explaing how they work](https://www.youtube.com/watch?v=va2XAdFtmeU&feature=emb_imp_woyt&themeRefresh=1)

This started out as an adaptation of the LightRider sketch in Jeremy Blum's "Exploring Arduino"
and then I thought to myself, "I could make musical sequence with this.."
It turns out somebody alread has. What I've built here is know by modular synthesizer enthusiasts as a Turing Machine Module.
A better explanation of what they do and how they work can be found here: 
https://www.thonk.co.uk/shop/turingmkii/

I thought to take this rite of passage myself and I've modified the aforementioned LightRider LED sketch to take an external clock signal (for speed control)
and to produce a voltage via an MCP4921 DAC by mapping current 8-bit value in shift registers (SN74LS14N shift register IC)
to 12-bit value passed to DAC and interpolated to millivolt increments.
This volts-per-octave signal can then be fed to a Voltage controlled  oscillator to generate notes.
Of course, they are random and not musical (unless ran through a quantizer to assign them to the closest note in a given musical scale. [This will be a separate project])

8v / 256 = 0.03125 0r 0.033 or 33 milliVolts
multiply the 8-bit # the LEDs represent by 0.033 to get the volts/octave output
All LEDs lit would add up to ~ 8.4 volts; however, because the MCP4921 is only rated for a maximum 5v VCC (which it uses as a reference voltage) 
the max voltage output is just under 5VDC. (I later plan to transition this project from being more software-oriented to more of a hardware implementation 
(e.g., more similar to the DIY kits mentioned in the video above) which will use a DAC0800L CN chip cable of reaching the upper octaves [but those higher octaves are shrill so this shoudl do for now.])
I will add a voltage divider circuit (with an adjustable trim pot) at the DAC's output to further attenuate the voltage and limit the output to lower notes if desired.

Because I do have the needed DPDT, 3-way switch for manually assigning 1 or 0 bits to the sequence, I've created a software implementation that utizies two momentary tactile buttons
for this purpose and 10K trim pot to adjust the probability of 'randomly' inverting the carried bit. 


## RESOURCES
MCP4xxx family DAC info:
from https://cyberblogspot.com/how-to-use-mcp4921-dac-with-arduino/#:~:text=The%20MCP4921%20is%20a%20Digital,the%20MCP4922%20is%20also%20available.  

Turing Machine Module DIY kit and resources
https://www.thonk.co.uk/shop/turingmkii/

Turing Machine [Module] Explained video:
https://www.youtube.com/watch?v=va2XAdFtmeU&feature=emb_imp_woyt&themeRefresh=1