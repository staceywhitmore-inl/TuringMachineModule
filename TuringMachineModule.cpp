//#include <TimerOne.h> // Can be used for SOFTware interrupts to call a f(n) at an interval set in microSeconds Use for Gate Length (Software Interrupt)
#include <SPI.h>
#include "MCP_DAC.h" // reference library for MCP4921 DAC IC
MCP4921 DAC;       // create DAC object of type MCP4921 

// This is an Adaptation of the LightRider sketch in Jeremy Blum's "Exploring Arduino" 
// which I've modified to take an external clock signal (for speed control) 
// and to produce a voltage via an MCP4921 DAC by mapping current 8-bit value in shift registers (SN74LS14N shift register IC)
// to 12-bit value passed to DAC and interpolated to millivolt increments 

/*
8v / 256 = 0.03125 0r 0.033 or 33 milliVolts 
multiply the 8-bit # the LEDs represent by 0.033 to get the volts/octave output
All LEDs lit add up to ~ 8.4 volts 
*/

const int CLK_PIN_IN = 0; // Interrupt 0 (pin 2 on Uno) for incoming clock source (Arturia Key Step sequencer "sync out" jack).

 // Arduino pins   | DAC pins
 //----------------|-----------: -----------------------------------------------
 // Pin D10 (SS)     <--  CS(2): Pin 10 declared in setup() by "DAC.begin(10);"    
 // Pin D11 (MOSI)   <-- SCK(3): Declared by Default by Library  
 // pin D13 (SCLCK)  <-- SCK(3): Declared by Default by Library  


//const uint32_t GATE_LENGTH = 500000; // .5 Sec = 1/2 million microseconds (500,000 uS)
// Button, Pot and Read INPUT pins
const int BTN_HIGH           = 2;
const int BTN_LOW            = 7;
const int RND_POT            = 0; // AO
const int OUT_BIT_READ_PIN   = 12; //D12

// Declare SHIFT REGister Pins
const int SER_DATA_PIN  =4; // TODO: change to 4 (was 8)
const int LATCH_PIN     =5; // "": " " 5 ~       (was 9)
const int CLK_PIN       =6; // "": " " 6 ~       (was 10)



// Declare associated Interrupt variables
volatile int cvOutput = 0;
volatile int currentBitIndex = 0;
volatile int dataPinState;

// TEST Sequence of LEDs
int currentValue = 212;

const int NUMBER_OF_BITS = 8;

void setup() {
 // Set Input Buttons (Not required) 
 pinMode(BTN_HIGH, INPUT); // 2
 pinMode(BTN_LOW, INPUT);  // 7 
  //Setup for Interrupts
 Serial.begin(230400); // 9600

  // Setup For Shift Reg
 pinMode(SER_DATA_PIN, OUTPUT);
 pinMode(LATCH_PIN, OUTPUT);
 pinMode(CLK_PIN, OUTPUT);

 
// DAC chip select (CS) signal comes from Arduino. Set Arduino SS out to pin D10
 DAC.begin(10);     // Initialize on Pin 10 

 // To make generated number more 'random', I seed it with noise generated an unassigned, floating analog pin. 
 randomSeed(analogRead(A5));

 attachInterrupt(CLK_PIN_IN, externalClkReceived, RISING);
}


// No loop is used 
void loop() {
  //Serial.print("Current v/oct binary value is : ");
  //Serial.println(currentBitIndex);
}



// Interrupt f(n) 
void externalClkReceived() {
  changeVoltageBits();
  
  currentBitIndex++;
  if (currentBitIndex >= NUMBER_OF_BITS){
    currentBitIndex = 0;
  }
}

// Timer Interrupt f(n)
void changeVoltageBits() {
  // 0.a) // Latch LOW to start sending
   digitalWrite(LATCH_PIN, LOW); 
   shiftOut(SER_DATA_PIN, CLK_PIN, currentValue); // Custom shiftBits to override built-in shiftBits() 
   changeVoltage(currentValue);
   digitalWrite(LATCH_PIN, HIGH);  // Latch HIGHT to SHOW the pattern (e.g. on the LEDs)   
}



// Called by Interrupt f(n): externalClkReceived
void changeVoltage(int val) {
  cvOutput = map(val, 0, 128, 0, 4095);
  cvOutput= constrain(cvOutput, 0, 4095);
 // Serial.print("[");
 // Serial.print(currentBitIndex); Serial.print("] = "); Serial.print(val); Serial.print(" ---> ");  Serial.println(cvOutput);
  DAC.analogWrite(cvOutput);   
}





// 
void shiftBits(int DATA_pin, int CLOCK_pin, byte DATA_out) { 


  pinMode(CLOCK_pin, OUTPUT);
  pinMode(DATA_pin, OUTPUT);
   // LATCH has already been set LOW prior to this call

  // 0.b) Clear everything out just in case to prepare shift register for bit shifting
  digitalWrite(DATA_pin, 0);  // DATA = 0 = LOW 
  digitalWrite(CLOCK_pin, 0); // CLOCK = 0 = LOW 
   // --[^BEFORE ALL]-----------------------------------------------


     dataPinState = getDataPinState(); // Det. by what is on DigitalRead pin 

    // 1.a) Then set DATA_pin HIGH(1) or LOW(0) depnding on resulting pinState 
    digitalWrite(DATA_pin, dataPinState); // DATA = HIGH(1) OR LOW(0)


     
    // 1.b) REGISTER shifts bits on UPstroke (RISING edge) of CLOCK_pin  (THIS TRIGGERS SHIFT of E/ bit one position)
    digitalWrite(CLOCK_pin, 1); // CLOCK = 1 = HIGH TO SHIFT BITS (adding what is on DATA pin(state) ^ above )
     
    // --[After EACH shift]------------------------------------------
   // 3.b) ZERO the DATA_pin AFTER shift to prevent bleed through
    digitalWrite(DATA_pin, 0); //DATA = 0 = LOW 
  // LATCH will be set HIGH After this call 


  // 9.) --[AFTER ALL (8) Shifts]------------------------------------------------
  //STOP shifting
  digitalWrite(CLOCK_pin, 0); // CLOCK BACK TO 0(LOW) to STOP Shifting 
  // 9.b) LATCH will be shifted HIGH again to SHOW the bits on the LEDs
}



int getDataPinState(){
  int rndPotVal = analogRead(RND_POT); // 10-bit resolution
  bool currentOutBitVal; 
  int randomNumber = random(0, 1023); 

  if(BTN_HIGH == HIGH) { // 2
    return 1;
  } else if(BTN_LOW == HIGH) {
    return 0;
  }

  // IF neither button is pressed the following code is executed: 
  if (randomNumber >= rndPotVal) {
     // then invert out shifted bit
    currentOutBitVal  = !digitalRead(OUT_BIT_READ_PIN);
  }
  else {
    // Leave the out shifted bit as it is.       
    currentOutBitVal = digitalRead(OUT_BIT_READ_PIN);
  }
     
  return (currentOutBitVal) ? 1 : 0;
}










