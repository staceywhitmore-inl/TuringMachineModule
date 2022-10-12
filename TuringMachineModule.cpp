//#include <TimerOne.h> // Can be used for SOFTware interrupts to call a f(n) at an interval set in microSeconds Use for Gate Length (Software Interrupt)
#include <SPI.h>
#include "MCP_DAC.h" // reference library for MCP4921 DAC IC
MCP4921 DAC;         // create DAC object of type MCP4921

// This started out as an adaptation of the LightRider sketch in Jeremy Blum's "Exploring Arduino"
// and then I thought to myself, "I could make musical sequence with this.."
// It turns out somebody alread has. What I've built here is know by modular synthesizer enthusiasts as a Turing Machine Module.
// A better explanation of what they do and how they work can be found here: 
// https://www.thonk.co.uk/shop/turingmkii/

// I thought to take this rite of passage myself and I've modified the aforementioned LightRider LED sketch to take an external clock signal (for speed control)
// and to produce a voltage via an MCP4921 DAC by mapping current 8-bit value in shift registers (SN74LS14N shift register IC)
// to 12-bit value passed to DAC and interpolated to millivolt increments.
// This volts-per-octave signal can then be fed to a Voltage controlled  oscillator to generate notes.
// Of course, they are random and not musical (unless ran through a quantizer to assign them to the closest note in a given musical scale. [This will be a separate project])

/*
8v / 256 = 0.03125 0r 0.033 or 33 milliVolts
multiply the 8-bit # the LEDs represent by 0.033 to get the volts/octave output
All LEDs lit would add up to ~ 8.4 volts; however, because the MCP4921 is only rated for a maximum 5v VCC (which it uses as a reference voltage) 
the max voltage output is just under 5VDC. (I later plan to transition this project from being more software-oriented to more of a hardware implementation 
(e.g., more similar to the DIY kits mentioned in the video above) which will use a DAC0800L CN chip cable of reaching the upper octaves [but those higher octaves are shrill so this should do for now.])
I will add a voltage divider circuit (with an adjustable trim pot) at the DAC's output to further attenuate the voltage and limit the output to lower notes if desired.

Because I do have the needed DPDT, 3-way switch for manually assigning 1 or 0 bits to the sequence, I've created a software implementation that utizies two momentary tactile buttons
for this purpose and 10K trim pot to adjust the probability of 'randomly' inverting the carried bit. 
*/

// Arduino pins   | DAC pins
//----------------|-----------: -----------------------------------------------
// Pin D10 (SS)     <--  CS(2): Pin 10 declared in setup() by "DAC.begin(10);"
// pin D13 (SCLCK)  <-- SCK(3): Declared by Default by Library  (SDI = Serial Data Interface) (*Note, I twisted these 2 wires on wire harness (yellow&orange wires)
// Pin D11 (MOSI)   <-- SDI(4): Declared by Default by Library  (MOSI = Master Out Slave In)  (*)

// const uint32_t GATE_LENGTH = 500000; // .5 Sec = 1/2 million microseconds (500,000 uS)

const int CLK_INTRPT_PIN = 0; // Interrupt 0 (I.e., PIN 2) (actually uses pin 2 on Uno) for incoming clock source (Arturia Key Step sequencer "sync out" jack).

// Declare SHIFT REGISTER Pins
const int SER_DATA_PIN = 4;
const int LATCH_PIN = 5;
const int CLK_PIN = 6;

// Other Input Pins
const int BTN_HIGH = 7;
const int BTN_LOW = 8;
const int RND_POT = A1;          // [A]O used for random seed.
const int OUT_BIT_READ_PIN = 12; // N The bit shifted out from shift register is sent to this pin on the Arduino.

// Declare (VOLATILE) associated Interrupt variables.
volatile int cvOutput = 0;
// volatile int  currentIteration = 0;//TODO: REMOVE. No longer needed.  WAS currentBitIndex = 0;
volatile int dataPinState;
volatile byte currentByte;
volatile int rndPotValue = 0;
volatile int rndInt;

const int NUMBER_OF_BITS_IN_SEQ = 8; // Const for now but would become assignable should a rotary selector is added later.

void setup()
{ 
  //Serial.begin(230400);// Comment(ed) out when not displaying debugging messages to free up resources.
  // Pin Setup For SHIFT REGISTER
  pinMode(SER_DATA_PIN, OUTPUT); // 4
  pinMode(LATCH_PIN, OUTPUT);    // 5
  pinMode(CLK_PIN, OUTPUT);      // 6
  //Button inputs
  pinMode(OUT_BIT_READ_PIN, INPUT); // 12
  pinMode(BTN_HIGH, INPUT);         // 7
  pinMode(BTN_LOW, INPUT);          // 8
  pinMode(RND_POT, INPUT);          // A1 (A0 used to seed random())

  // DAC chip select (CS) signal comes from Arduino. Set Arduino SS out to pin D10
  DAC.begin(10); // Initialize on Pin 10

  attachInterrupt(CLK_INTRPT_PIN, externalClkReceived, RISING); // PIN 2 of Arduino UNO

  // Ground LATCH_PIN and hold LOW for as long as transmitting
  digitalWrite(LATCH_PIN, 0);

  firstShift(SER_DATA_PIN, CLK_PIN, 0b00000000); // (4, 6, 0b10000001)

  // Return the latch pin HIGH to signal chip that it no longer needs to listen for information  (Output vals from shiftReg)
  digitalWrite(LATCH_PIN, 1);
  randomSeed(analogRead(0)); // Noise on analog pin A0 will cause randomSeed()to generate different seed numbers each time the program is run to make it 'more' "random".
}

// TODO: REMOVE WHEN NO LONGER NEEDED
void loop()
{
  //Serial.print("Current v/oct binary value is : ");
  //Serial.println(currentBitIndex);
}

void shiftBits(int DATA_pin, int CLOCK_pin, byte DATA_out)
{
  // Shift 8 bits out MSB first, on clock's rising edge
  // Had to add 10K PullDown resistor to clock input to keep LEDs from random flickers when clock cable is unplugged

  currentByte = DATA_out; // N

  rndPotValue = analogRead(RND_POT); // Pin A1 (value will be 0 - 1023)
  // nndPotValue = map(); // Would map 1023 to 255 but it seems to already by mapped to 255 (8-bit) by default (rather than 1023 (10-bit)
  rndInt = random(0, 1024); // Random number from 0 to 1023

  // Clear E/thing out, just in case, to prepare shift register for bit shifting
  digitalWrite(DATA_pin, 0);  // DATA = 0 = LOW
  digitalWrite(CLOCK_pin, 0); // CLOCK = 0 = LOW

  digitalWrite(CLOCK_pin, 0); // start [INTERNAL]CLOCK = 0 = LOW ..again (for e/iteration)

  // I realize there is a chance these if-else statements may not be executed at the time
  // a button is being held; HOWEVER, after testing this I found the behavior to be satisfactory.
  // So, I decided not to add any additional interrupts to button presses etc. 
  // Mostly, I didn't want to add this feature because the current behavior requiers 
  // the musician/operator to press to the button(s) at the opportune moment in order to change the bit
  // about to be carried over. If an interrupt occured whenever a button were pressed,
  // it would likely introduce undesired bx; although, I've yet to test this. 
  
  // As you can see from the following statements, if both buttons are pressed at the same time, 
  // by default, the carried bit is converted to 1 (not a 0).
  if (digitalRead(BTN_HIGH) == HIGH)
  { 
    dataPinState = 1;
    currentByte = (currentByte << 1) | (currentByte >> (NUMBER_OF_BITS_IN_SEQ - 1)) | 0b00000001; // | "Rotate through carry circular shift" (only unsigned so 0b11111111 = 129 [Not -127]).
  }
  else if (digitalRead(BTN_LOW) == HIGH)
  { 
    dataPinState = 0;
    currentByte = ~(~(currentByte << 1) | (currentByte >> (NUMBER_OF_BITS_IN_SEQ - 1)) | 0b00000001);
  }
  else if (rndInt < rndPotValue)
  { 
    dataPinState = !digitalRead(OUT_BIT_READ_PIN); 

    if (dataPinState)    
      currentByte = (currentByte << 1) | (currentByte >> (NUMBER_OF_BITS_IN_SEQ - 1)) | 0b00000001;    
    else     
      currentByte = ~(~(currentByte << 1) | (currentByte >> (NUMBER_OF_BITS_IN_SEQ - 1)) | 0b00000001);    
  }
  else
  {
    dataPinState = digitalRead(OUT_BIT_READ_PIN);
    currentByte = (currentByte << 1) | (currentByte >> (NUMBER_OF_BITS_IN_SEQ - 1)); // (sizeof(currentByte) - 1)  );
  }

  digitalWrite(DATA_pin, dataPinState); // Set DATA pin (pin 4)to HIGH(1) OR LOW(0)  --> TO pin D4 Ard
  digitalWrite(CLOCK_pin, 1);           // CLOCK = 1 = HIGH TO SHIFT BITS (adding what is on DATA pin(state) ^ above )
  digitalWrite(DATA_pin, 0);            // DATA pin (Pin 4) = 0 = LOW

  // STOP shifting
  digitalWrite(CLOCK_pin, 0); // CLOCK BACK TO 0 to STOP Shifting                           
  // Serial.println("***********************");
  // // Serial.print("["); Serial.print(currentIteration); Serial.print("]");
  // Serial.print("Current Byte is : ");
  // Serial.println(currentByte);
  outputVoltage(currentByte);
}

// INTERRUPT f(n)
void externalClkReceived()
{
  // ground LATCH_PIN and hold LOW for as long as transmitting
  digitalWrite(LATCH_PIN, 0);

  shiftBits(SER_DATA_PIN, CLK_PIN, currentByte); 
                                                 // Return the latch pin HIGH to signal chip that it no longer needs to listen for information
  digitalWrite(LATCH_PIN, 1);
}

// Called once to setup initial state from Setup()
void firstShift(int DATA_pin, int CLOCK_pin, byte DATA_out)
{
  // Shift 8 bits out MSB first, on clock's rising edge.
  // Clear everything out just in case to prepare shift register for bit shifting.
  digitalWrite(DATA_pin, 0);  // DATA = 0 = LOW
  digitalWrite(CLOCK_pin, 0); // CLOCK = 0 = LOW

  // Check and set e/ digit/bit space 1-by-1 and toggle it ON or OFF
  for (int i = 7; i >= 0; i--)
  {
    digitalWrite(CLOCK_pin, 0); // Start [INTERNAL]CLOCK = 0 = LOW ..again (for e/iteration).

    if (DATA_out & (1 << i)) // Any resulting # that is NOT 0 ---> TRUE
      dataPinState = 1;      // 1 = ON = HIGH
    else                     // IF bitmask result is 0 then ---> FALSE
      dataPinState = 0;      // 0 = OFF = LOW

    digitalWrite(DATA_pin, dataPinState); // Set DATA pin (pin 4)to HIGH(1) OR LOW(0)  --> TO pin D4 Ard
    digitalWrite(CLOCK_pin, 1);           // CLOCK = 1 = HIGH TO SHIFT BITS (adding what is on DATA pin(state) ^ above )
    digitalWrite(DATA_pin, 0);            // DATA pin (Pin 4) = 0 = LOW
  }                                       //*********** close FOR-loop *********************************
  // STOP shifting
  digitalWrite(CLOCK_pin, 0); // CLOCK BACK TO 0 to STOP Shifting

  currentByte = DATA_out;
  outputVoltage(currentByte);
}

// Called at the end of shiftBits()
void outputVoltage(int val)
{
  // Start at 10 to avoid -neg voltages at low values (perhaps a rectifier diode instead but adds a voltage drop. A (buffered) OpAmp p(x)ly best.)
  cvOutput = map(val, 0, 256, 5, 4095);
  cvOutput = constrain(cvOutput, 0, 4095);
  // Serial.print("[");
  // Serial.print(val);
  // Serial.print(" (10-bit value) maps to----> ");
  // Serial.print(cvOutput);
  // Serial.println(" (12-bit [mapped] Value)");
  DAC.analogWrite(cvOutput);
}

/*
outoutVoltage() results (recorded by serial outout and voltmeter[while using MCP4921 DAC with a max output of 5VDC]):
  // 1 (10-bit value) maps to----> 15 (mapped to 12-bit) (---> 0.013v)
  // 255 (10-bit) --maps to----> 4079 (12-bit) (---> ~4.945v)
*/

/*
References:
MCP4xxx family DAC info:
https://cyberblogspot.com/how-to-use-mcp4921-dac-with-arduino/#:~:text=The%20MCP4921%20is%20a%20Digital,the%20MCP4922%20is%20also%20available

Turing Machine Module DIY kit and resources
https://www.thonk.co.uk/shop/turingmkii/

Turing Machine [Module] Explained video:
https://www.youtube.com/watch?v=va2XAdFtmeU&feature=emb_imp_woyt&themeRefresh=1
*/