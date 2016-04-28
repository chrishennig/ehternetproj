//-----------------------------------------------------------------------------
// F04x_Ports_SwitchLED.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program demonstrates how to configure port pins as digital inputs
// and outputs.  The C8051F040 target board has one push-button switch 
// connected to a port pin and one LED.  The program constantly checks the 
// status of the switch and if it is pushed, it turns on the LED.
//
// The program also monitors P4.0.  If P4.0 is high, it sets P4.1 low.  If
// P4.0 is low, P4.1 is set high.  The purpose of this part of the program
// is to show how to access the higher ports.  Ports 4-7 are not available
// on all SFR pages, so the SFRPAGE register must be set correctly before 
// reading or writing these ports.
//
//
// How To Test:
//
// 1) Download code to a 'F040 target board
// 2) Ensure that the J1 and J3 headers are shorted
// 3) Push the button (P3.7) and see that the LED turns on 
// 4) Set P4.0 high and low using an external connection and check that
//    the output on P4.1 is the inverse of P4.0.
//
//
// FID:            04X000002
// Target:         C8051F04x
// Tool chain:     Keil C51 7.50 / Keil EVAL C51
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (GP)
//    -15 NOV 2005
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <c8051f040.h>                 // SFR declarations

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

char Tab7Seg[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};


sfr16 RCAP3    = 0xCA;                 // Timer3 reload value
sfr16 TMR3     = 0xCC;                 // Timer3 counter
sbit   LED= P1^6;
#define SYSCLK 3062500                   // approximate SYSCLK frequency in Hz
char   buffer = 0x00;
int x=0;
double counter=0;
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);           
void PORT_Init (void);
void Timer3_Init (int counts);
void Timer3_ISR (void);

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   WDTCN = 0xde;                       // Disable watchdog timer
   WDTCN = 0xad;

   SFRPAGE =CONFIG_PAGE;
   PORT_Init();                        // Initialize Port I/O
   //OSCILLATOR_Init ();                 // Initialize Oscillator
   SFRPAGE =TMR3_PAGE;
   Timer3_Init (SYSCLK/12/26);
   EA=1;
   SFRPAGE= LEGACY_PAGE;
   
   while (1)
   {
/*	counter++;

	if (counter < 2000) buffer= Tab7Seg[1];
	if (counter > 4000) buffer= Tab7Seg[2];
	if (counter > 6000) buffer= Tab7Seg[3];
	if (counter > 8000 )buffer= Tab7Seg[4];
	if (counter > 10000 )buffer= Tab7Seg[5];
	if (counter > 12000 )buffer= Tab7Seg[6];
	if (counter > 14000 )buffer= Tab7Seg[7];
	if (counter > 16000)buffer= Tab7Seg[8];
	if (counter > 18000)buffer= Tab7Seg[9];
	if (counter > 20000 )counter =0;

*/
   if (x==0)  
   {                             // clear TF3
   //buffer= Tab7Seg[7];
   buffer= Tab7Seg[0]; 
   x=1;
   }
    if (x==1) 
 {                            // clear TF3
   //buffer= Tab7Seg[1];
   buffer= Tab7Seg[1]; 
   x=0;
   }


    //SFRPAGE = CONFIG_PAGE;           // set SFR page before reading or writing
                                       // to P4 registers
   }                                   // end of while(1)
}                                      // end of main()

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// OSCILLATOR_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the internal oscillator
// at its maximum frequency.
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;              // set SFR page

   OSCICN |= 0x03;                     // Configure internal oscillator for
                                       // its maximum frequency (24.5 Mhz)

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and ports pins.
// 
// To configure a pin as a digital input, the pin is configured as digital
// and open-drain and the port latch should be set to a '1'.  The weak-pullups
// are used to pull the pins high.  Pressing the switch pulls the pins low.
//
// To configure a pin as a digital output, the pin is configured as digital
// and push-pull.  
//
// Some ports pins do not have the option to be configured as analog or digital,
// so it not necessary to explicitly configure them as digital.
//
// An output pin can also be configured to be an open-drain output if system
// requires it.  For example, if the pin is an output on a multi-device bus,
// it will probably be configured as an open-drain output instead of a 
// push-pull output.  For the purposes of this example, the pin is configured
// as push-pull output because the pin in only connected to an LED.
//

//-----------------------------------------------------------------------------
void PORT_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;              // set SFR page before writing to
                                       // registers on this page

   P3MDIN |= 0xff;                     // P3 is digital

   P1MDOUT = 0xff;                     // P1 is push-pull
   P3MDOUT = 0x00;                     // P3 is open-drain

   P3     |= 0xff;                     // Set P3 latch to '1'


   XBR2    = 0x40;                     // Enable crossbar and enable
                                       // weak pull-ups

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

void Timer3_Init (int counts)
{
   TMR3CN = 0x00;                      // Stop Timer3; Clear TF3;
                                       // use SYSCLK/12 as timebase
   RCAP3   = -counts;                  // Init reload values
   TMR3    = 0xffff;                   // set to reload immediately
   EIE2   |= 0x01;                     // enable Timer3 interrupts
   TR3 = 1;                            // start Timer3
}

void Timer3_ISR (void) interrupt 14
{
   TF3 = 0;
   P1=buffer;
   P3=buffer;
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------