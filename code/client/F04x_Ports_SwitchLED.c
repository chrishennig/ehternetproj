//-----------------------------------------------------------------------------
// F04x_Ports_SwitchLED.c
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <c8051f040.h>                 // SFR declarations

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);           
void PORT_Init (void);
void Readtimer (void);
void updateNumbers (void);

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

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   WDTCN = 0xde;                       // Disable watchdog timer
   WDTCN = 0xad;

   PORT_Init();                        // Initialize Port I/O
   OSCILLATOR_Init ();                 // Initialize Oscillator

   P1= 0x00;
  
   while (1)
   {

	P1=P3;
	
   }                                   // end of while(1)
}                                      // end of main()

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------
//############## Read Timer xy ############## 
/*
void Readtimer (void)
{
	int recive = 0;
	int framecounter = 0;
	char framebuffer[25];

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
      //updateNumbers;
   }
}

//void updateNumbers(void)
//{
   				
 //  P1 = framebuffer[17];
   
//}

*/
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
// P1.6   digital   push-pull     LED1
// P3.7   digital   open-drain    Switch 1
// P4.0   digital   open-drain    Input 1
// P4.1   digital   push-pull     Output 1
//-----------------------------------------------------------------------------
void PORT_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;              // set SFR page before writing to
                                       // registers on this page

   //P1MDIN |= 0xff;                     // P1 is digital  muss 1
   //P3MDIN |= 0xff;  

   P1MDOUT = 0xff;                     // P1 is push-pull
   P3MDOUT = 0x00;                     // P3 is open-drain  jetzt P2

   //P3     |= 0x00;                     // Set P3 latch to '1'   soll raus

   //P4MDOUT = 0x02;                     // P4.0 is open-drain; P4.1 is push-pull
   //P4      = 0x01;                     // Set P4.1 latch to '1'


   XBR2    = 0x40;                     // Enable crossbar and enable
                                       // weak pull-ups

   SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------