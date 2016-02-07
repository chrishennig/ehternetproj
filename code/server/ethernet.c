
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
#define SAMPLE_DELAY 50                // Delay in ms before taking sample

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
void updateFrame();
void crcFunc();
void updateNumbers();
char int_to_bin(int k);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

long Result;                           // ADC0 decimated value
char Tab7Seg[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F}; // dies ist die umrechnung von zahlen in byte
char other[2];							// hier koennen zusaetliche informationen aus dem frame gespeichert werden
char framebuffer[25];					

//############## Ethernet Frame ############## 
char praeambel[7] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char begin = 0xAB;
char ziel[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
char quelle[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
char typ[2] = {0x00, 0x00};
char daten[5];
char pad = 0x00;
char crc[4] = {0x00, 0x00, 0x00, 0x01};

char frame[];


//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   // local variabels
   long measurement;                   // Measured voltage in mV
   long result_redux;															// bitreduzierter ADC Wert
   long rest;
   //int adc_0;
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

      P1 = s10;

		//framebuffer[15] = Tab7Seg[s1];
		//framebuffer[16] = Tab7Seg[s10];
		//framebuffer[17] = Tab7Seg[s10];
		//framebuffer[18] = Tab7Seg[s1000];
		//updateNumbers();
		
		//P0 = int_to_bin(measurement);

      //Wait_MS(SAMPLE_DELAY);           // Wait 50 milliseconds before taking another sample
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

   P0MDOUT |= 0x00;                    // Set TX1 pin to push-pull
   P1MDOUT |= 0x00;                     // Set P1.6(LED) to push-
   P2MDOUT |= 0x00; 
   P3MDOUT |= 0x00; 

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

   RCAP2 = -(SYSCLK/1000/12);          // Timer 2 overflows at 1 kHz
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


void updateNumbers(){
   					//framebuffer[17];
   P1 = framebuffer[17];
   
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------

/*




############## CRC ############## 



void crcFunc(){
   int bitstrom[] = praeambel + begin + ziel + quelle + typ + dauen + pad;
   int bitzahl = sizeof(bitstrom);
   int register = 0;
   int i;
   for(i; i < bitzahl; i++){
      if(((register >> 31) & 1) != bitstrom[i])
         register = (register << 1) ^ mask;
      else
         register = (register << 1);
   }
}

##############  ############## 
void updateFrame(){
   if(frame[] ==  praeambel + begin + ziel + quelle + typ + dauen + pad + crc)
      // mache nix 
   else{
      crc();
      frame[] =  praeambel + begin + ziel + quelle + typ + dauen + pad + crc;
      frameUpdate = 1;
   }
}

############## Timer XY ############## 
call ever x ms
int frameNumber = 0; // im Header
int frameUpdate = 0; // 0 == keine Änderung 1 == Änderung 

if(frameNumber < sizeof(frame) && frameUpdate == 1){
   P1 = frame[frameNumber];   // hier wird in den Port geschrieben
   frameNumber++;          // und beim nächsten Aufruf das nächste byte
}
else{
   frameNumber = 0;     // hier wird alles zurück gesetzt
   frameUpdate = 0;
}
 

############## main ############## 

int s1;
int s10;
int s100;
int s1000;





############## main ##############  


while(1){
 ...

updateFrame();
}

############## Read Timer xy ############## 
int recive = 0;
int framecounter = 0;
char framebuffer[25]

if (recive == 0){
   if (P1 == 0x55)// schauen ob die praeambel gesetzt wird
   {
      framecounter++;
      if (framecounter >= 7) 
      {
         recive = 1;// nach 7 mal 0x55 wird begonnen die Nachricht zu empfangen
      }
   }
   else{
      framecounter = 0;// wenn die praeambel unvollstaendig ist dann wird wieder von vorne begonnen
   }
}
else{
   if (framecounter < 26)
   {
      framecounter++;
      framebuffer[framecounter-7] = P1; // hier wird der inhalt in den buffer geschrieben
   }
   else{
      framecounter = 0;
      recive = 0;
      updateNumbers();
   } 
}

//test
*/



