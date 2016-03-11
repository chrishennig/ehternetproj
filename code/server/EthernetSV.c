
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <c8051F040.h>                 // SFR declarations
#include <stdio.h>                     

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F04x
//-----------------------------------------------------------------------------

sfr16 ADC0     = 0xbe;                 // ADC0 data
sfr16 RCAP2    = 0xca;                 // Timer2 capture/reload
sfr16 RCAP3    = 0xca;                 // Timer3 capture/reload
sfr16 TMR2     = 0xcc;                 // Timer2
sfr16 TMR3     = 0xcc;                 // Timer3

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define BAUDRATE     115200            // Baud rate of UART in bps
#define SYSCLK       24500000          // System Clock
#define SAMPLE_RATE  50000             // Sample frequency in Hz
#define INT_DEC      256               // Integrate and decimate ratio
#define SAR_CLK      2500000           // Desired SAR clock speed
#define SAMPLE_DELAY 500                // Delay in ms before taking sample

#define mask 0x04c11DB7 				// frame mask for CRC calculation


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);           
void PORT_Init (void);
void UART1_Init (void);
void ADC0_Init (void);
void TIMER3_Init (int counts);
void ADC0_ISR (void);
void Wait_MS (unsigned int ms);
// ############## Chris und Tim functions ##############
void crcFunc();

void sendFrame();
void nextFrame(char char1,char char2,char char3,char char4,char char5);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

long Result;                           // ADC0 decimated value
char Tab7Seg[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F}; // dies ist die umrechnung von zahlen in byte
char other[2];							// hier koennen zusaetliche informationen aus dem frame gespeichert werden
//char framebuffer[25];
					

//############## Ethernet Frame ############## 
char praeambel[7] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char begin = 0xAB;
char ziel[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
char quelle[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
char typ[2] = {0x00, 0x00};
char daten[5];
char pad = 0x00;
char crc[4] = {0x00, 0x00, 0x00, 0x01};

char frame[32];

int globalcounter = 0;
int newframe = 0;




//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   // local variabels
   long measurement;                   // Measured voltage in mV
   long result_redux;															// bitreduzierter ADC Wert
   long rest;
   char s1;																		// Tausendstel
   char s10;																	// Hunderstel
   char s100;																	// Zehntel
   char s1000;	                                                // Einer


   // configuration
   WDTCN = 0xde;                       // Disable watchdog timer
   WDTCN = 0xad;
   OSCILLATOR_Init ();                 // Initialize oscillator
   PORT_Init ();                       // Initialize crossbar and GPIO
   UART1_Init ();                      // Initialize UART1
   TIMER3_Init (SYSCLK/SAMPLE_RATE);   // Initialize Timer3 to overflow at sample rate
   ADC0_Init ();                       // Init ADC
   SFRPAGE = ADC0_PAGE;
   AD2EN = 1;                          // Enable ADC
   EA = 1;                             // Enable global interrupts

   while (1)
   {

	   EA = 0;                          // Disable interrupts !!

      measurement =  Result * 2430 / 4095; // dies hier wird ohne unterbrechung ausgefuehrt

      EA = 1;                          // Re-enable interrupts !!
	  
     result_redux = measurement;			//Stellenzuweisung (/1; /10; /100)

	  s1000 = (char)(result_redux/1000);						// Typumwandlung (Uncasten)
	  rest = result_redux % 1000;								// % nimmt Rest nach Division
	  s100 = (char)(rest/100);
	  rest = rest % 100;
	  s10 = (char)(rest/10);
	  rest = rest % 10;
	  s1 = (char)rest;

		
     //########## Verschiedene alternativen ##########
     sendFrame();
     nextFrame(Tab7Seg[s1],Tab7Seg[s10],Tab7Seg[s100],Tab7Seg[s1000],0x00);

      Wait_MS(SAMPLE_DELAY);           // Wait 50 milliseconds before taking another sample
   }
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;              // Set SFR page

   OSCICN = 0x83;                      // Set internal oscillator to run
                                       // at its maximum frequency

   CLKSEL = 0x00;                      // Select the internal osc. as
                                       // the SYSCLK source

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

void PORT_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;              // set SFR page

   XBR0     = 0x00;
   XBR1     = 0x00;
   XBR2     = 0x44;                    // Enable crossbar and weak pull-up
                                       // Enable UART1

   P3MDOUT  = 0x00;					   // Port 3 open drain
   P3MDIN   = 0xff;					   // Port 3 digital
   P1MDOUT  = 0xff;					   // Port 1 LED push pull

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}


void UART1_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = UART1_PAGE;
   SCON1   = 0x10;                     // SCON1: mode 0, 8-bit UART, enable RX

   SFRPAGE = TIMER01_PAGE;
   TMOD   &= ~0xF0;
   TMOD   |=  0x20;                    // TMOD: timer 1, mode 2, 8-bit reload


   if (SYSCLK/BAUDRATE/2/256 < 1) {
      TH1 = -(SYSCLK/BAUDRATE/2);
      CKCON |= 0x10;                   // T1M = 1; SCA1:0 = xx
   } else if (SYSCLK/BAUDRATE/2/256 < 4) {
      TH1 = -(SYSCLK/BAUDRATE/2/4);
      CKCON &= ~0x13;                  // Clear all T1 related bits
      CKCON |=  0x01;                  // T1M = 0; SCA1:0 = 01
   } else if (SYSCLK/BAUDRATE/2/256 < 12) {
      TH1 = -(SYSCLK/BAUDRATE/2/12);
      CKCON &= ~0x13;                  // T1M = 0; SCA1:0 = 00
   } else {
      TH1 = -(SYSCLK/BAUDRATE/2/48);
      CKCON &= ~0x13;                  // Clear all T1 related bits
      CKCON |=  0x02;                  // T1M = 0; SCA1:0 = 10
   }

   TL1 = TH1;                          // Initialize Timer1
   TR1 = 1;                            // Start Timer1

   SFRPAGE = UART1_PAGE;
   TI1 = 1;                            // Indicate TX1 ready

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page

}



void ADC0_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = ADC0_PAGE;

   ADC0CN = 0x04;                      // ADC0 disabled; normal tracking
                                       // mode; ADC0 conversions are initiated
                                       // on overflow of Timer3; ADC0 data is
                                       // right-justified

   REF0CN = 0x03;                      // Enable on-chip VREF,
                                       // and VREF output buffer
   AMX0CF = 0x00;                      // AIN inputs are single-ended (default)
   AMX0SL = 0x01;                      // Select AIN2.1 pin as ADC mux input
   ADC0CF = (SYSCLK/SAR_CLK) << 3;     // ADC conversion clock = 2.5MHz, Gain=1
   EIE2 |= 0x02;                       // enable ADC interrupts
   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

void TIMER3_Init (int counts)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = TMR3_PAGE;

   TMR3CN = 0x00;                      // Stop Timer3; Clear TF3;
   TMR3CF = 0x08;                      // use SYSCLK as timebase

   RCAP3   = -counts;                  // Init reload values
   TMR3    = RCAP3;                    // Set to reload immediately
   EIE2   &= ~0x01;                    // Disable Timer3 interrupts
   TR3     = 1;                        // start Timer3

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------


void ADC0_ISR (void) interrupt 15
{
   static unsigned int_dec=INT_DEC;    // Integrate/decimate counter
                                       // we post a new result when
                                       // int_dec = 0
   static long accumulator=0L;         // Here's where we integrate the
                                       // ADC samples
   AD0INT = 0;                         // Clear ADC conversion complete
                                       // indicator
   accumulator += ADC0;                // Read ADC value and add to running
                                       // total
   int_dec--;                          // Update decimation counter
   if (int_dec == 0)                   // If zero, then post result
   {
      int_dec = INT_DEC;               // Reset counter
      Result = accumulator >> 8;
      accumulator = 0L;                // Reset accumulator
   }
}

//-----------------------------------------------------------------------------
// Support Subroutines
//-----------------------------------------------------------------------------


void Wait_MS(unsigned int ms)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = TMR2_PAGE;

   TMR2CN = 0x00;                      // Stop Timer3; Clear TF3;
   TMR2CF = 0x00;                      // use SYSCLK/12 as timebase

   RCAP2 = -(SYSCLK/10000);          // Timer 2 overflows at 1 kHz
   TMR2 = RCAP2;

   ET2 = 0;                            // Disable Timer 2 interrupts

   TR2 = 1;                            // Start Timer 2

   while(ms)
   {
      TF2 = 0;                         // Clear flag to initialize
      while(!TF2);                     // Wait until timer overflows
      ms--;                            // Decrement ms
   }

   TR2 = 0;                            // Stop Timer 2

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFRPAGE
}


//############## numern berechnen ##############
/*
int  number(int einer){
   int numbercalc;
	return einer;	
}
int  number(int einer, int zehner){
   int numbercalc;
	numbercalc = einer + zehner * 10;
   return numbercalc;
}
int  number(int einer, int zehner, int hunderter){
   int numbercalc;
   numbercalc = einer + zehner * 10 + hunderter * 100;
   return numbercalc;
}
int  number(int einer, int zehner, int hunderter, int tausender){
   int numbercalc;
   numbercalc = einer + zehner * 10 + hunderter * 100 + tausender * 1000;
   return numbercalc;   
}
*/


//############## CRC ############## 
void crcFunc(){
   /*
   int bitstrom[] = praeambel + begin + ziel + quelle + typ + daten + pad;
   int bitzahl = sizeof(bitstrom);
   int register = 0;
   int i;
   for(i; i < bitzahl; i++){
      if(((register >> 31) & 1) != bitstrom[i])
         register = (register << 1) ^ mask;
      else
         register = (register << 1);
   }
   */
}



void sendFrame(){
   if(newframe = 1){
      if(globalcounter <= 32){
         P3 = frame[globalcounter];
         globalcounter++;
      }
      else{
         newframe = 0;
      }
   }
   else{
      //do nothing
   }
}

void nextFrame(char char1,char char2,char char3,char char4,char char5){
   if(char1==frame[21] ||char2==frame[22] ||char3==frame[23] ||char4==frame[24] ||char5==frame[25]){
      //do nothing
   } 
   else{
      if (newframe = 0){
         frame[0] = praeambel[0]; // globale variablen
         frame[1] = praeambel[1];
         frame[2] = praeambel[2];
         frame[3] = praeambel[3];
         frame[4] = praeambel[4];
         frame[5] = praeambel[5];
         frame[6] = praeambel[6];
         frame[7] = begin;
         frame[8] = ziel[0];
         frame[9] = ziel[1];
         frame[10] = ziel[2];
         frame[11] = ziel[3];
         frame[12] = ziel[4];
         frame[13] = ziel[5];
         frame[14] = quelle[0];
         frame[15] = quelle[1];
         frame[16] = quelle[2];
         frame[17] = quelle[3];
         frame[18] = quelle[4];
         frame[19] = quelle[5];
         frame[20] = typ[0];
         frame[21] = typ[1];
         frame[22] = char1; // lokale variablen aus dem Kopf
         frame[23] = char2;
         frame[24] = char3;
         frame[25] = char4;
         frame[26] = char5;
         frame[27] = pad;

         crcFunc():        // hier wird die crc neu berechnet

         frame[28] = crc[0];
         frame[29] = crc[1];
         frame[30] = crc[2];
         frame[31] = crc[3];
         newframe = 1;
      }
   }
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------







