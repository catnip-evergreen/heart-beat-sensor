// final code
// code for TMT project
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int pulsePin = A0;                 // Pulse Sensor connected to pin A0
int blinkPin = 13;                // pin to blink led at each beat detected, this ensures that we are able to differentiate when the readings are being taken and communicated via arduino
static boolean serialVisual = true;  //set to true to see the bpm on the serial monitor of the PC
// Volatile Variables,changes during the runtime
volatile int rate[10];                      // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P = 512;                      // used to find peak in pulse wave
volatile int T = 512 ;                    // used to find trough in pulse wave
volatile int thresh = 585;                // used to find instant moment of heart beat
volatile int amp = 100;                   // used to hold amplitude of pulse waveform
volatile boolean firstBeat = true;        // used to set rate array so a reasonable BPM is detected
volatile boolean secondBeat = false;      // used to seed rate array so a reasonable BPM is detected
volatile int BPM;                   // int that holds data in A0. updated every 2mS
volatile int Signal;                // holds the incoming data from pulse sensor
volatile int IBI = 600;             // int that holds the time interval between beats, interbeat interval
volatile boolean Pulse = false;     // True when User's live heartbeat is detected. False when not a live beat. 
volatile boolean QS = false;        // becomes true when a heartbeat is detected by arduino
void setup()
{
  pinMode(blinkPin,OUTPUT);         // the L pin on arduino will blink whenever the heartbeat is detected
  Serial.begin(115200);             // maximum baud rate for the highest possible accuracy
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  lcd.begin(16, 2);
  lcd.clear(); // initialises the LCD display
}

// main loop
void loop()
{
   
   
  if (QS == true) // when a heartbeat is detected
    {     
      // BPM and IBI are determined
      // QS is true when there is a heartbeat
      serialOutputWhenBeatHappens(); // A Beat Happened, Output that to serial.     
      QS = false; // reset the QS flag for next reading
    }
     
  delay(20); //introduce a delay
  //print the message to read the next reading
}


void interruptSetup()
{     
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;  // disables PWM on digital pins 3 and 11, goes into CTC mode
  TCCR2B = 0x06;  // prescaled to 256
  OCR2A = 0X7C;    // count set to 124 for sample rate of 500 Hz
  TIMSK2 = 0x02;   // enabled when TIMER2 = OCRA2
  sei();             // enable global interrupts     
} 



void serialOutputWhenBeatHappens()
{    
 if (serialVisual == true) //  Code to display the results
   {            
     Serial.print(" Heart-Beat Found ");  //displays the heartbeat on serial monitor of the computer
     Serial.print("BPM: ");
     Serial.println(BPM);
     lcd.print("Heart-Beat Found "); // displays the results on the lcd screen
     lcd.setCursor(1,1);
     lcd.print("BPM: ");
     lcd.setCursor(5,1);
     lcd.print(BPM);
     delay(300);
     lcd.clear();
   }
 else
   {// when there is no visualisation of data to be done
     sendDataToSerial('B',BPM);   // send heart rate with a 'B' prefix
     sendDataToSerial('Q',IBI);   // send time between beats with a 'Q' prefix
   }   
}

void sendDataToSerial(char symbol, int data )
{
   Serial.print(symbol);
   Serial.println(data);                
}

// used for generating interrupts

ISR(TIMER2_COMPA_vect) //triggered when Timer2 counts to 124
{  
  cli();                                      // disable interrupts 
  Signal = analogRead(pulsePin);              // read the Pulse Sensor 
  sampleCounter += 2;                         // to keep track of time
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to monitor the noise
  //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3) // avoid dichrotic noise by waiting 3/5 of last IBI The dicrotic pulse is an abnormal carotid pulse found in conjunction with certain conditions characterised by low cardiac output.
    {      
      if (Signal < T) // T is the trough
      {                        
        T = Signal; //  lowest point in pulse  
      }
    }

  if(Signal > thresh && Signal > P)
    {          // thresh condition used for avoiding noise
      P = Signal;                             // P is peak
    }                                        //  highest point in pulse 

 // to detect the hearteat
  if (N > 300)
  {                                   // avoid high frequency noise, determined through trial and error
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
      {        
        Pulse = true;                               // set the Pulse flag when there is a pulse
      digitalWrite(blinkPin,HIGH);                // turn on pin LED pin
        IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
        lastBeatTime = sampleCounter;               // time taken for next pulse
  
        if(secondBeat)
        {                        // if this is the second beat, if secondBeat == TRUE
          secondBeat = false;                  // clear secondBeat flag
          for(int i=0; i<=9; i++) // seed the running total for a realisitic BPM
          {             
            rate[i] = IBI;                      
          }
        }
  
        if(firstBeat) // t's the first time there is a beat, if firstBeat == TRUE
        {                         
          firstBeat = false;                   // clear firstBeat flag
          secondBeat = true;                   // set the second beat flag
          sei();                               // enable interrupts again
          return;                              // IBI value is unreliable , discarded
        }   
      // keep a running total of the last 10 IBI values
      word runningTotal = 0;                  // clear the runningTotal variable    

      for(int i=0; i<=8; i++)
        {                // shift data in the rate array
          rate[i] = rate[i+1];                  // and drop the oldest IBI value 
          runningTotal += rate[i];              // add up the 9 oldest IBI values
        }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;     
    
      QS = true;                              // set QS flag 
      // QS flag is not cleared here
    }                       
  }

  if (Signal < thresh && Pulse == true)
    {   // when the values are going down, the beat is over
    digitalWrite(blinkPin,LOW);            // turn off LED pin
      Pulse = false;                         // reset the Pulse flag 
      amp = P - T;                           // get amplitude of the pulse wave
      thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
      P = thresh;                            // reset these for next time
      T = thresh;
    }

  if (N > 2500)
    {                           // if 2.5 seconds go by without a beat
      thresh = 585;                          // set thresh default
      P = 512;                               // set P default
      T = 512;                               // set T default
      lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
      firstBeat = true;                      // to avoid noise 
      secondBeat = false;                    // when we get the heartbeat back
    }

  sei();                                   // enable interrupts when youre done!
}

