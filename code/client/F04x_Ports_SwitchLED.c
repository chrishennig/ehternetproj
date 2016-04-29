// Includes

#include <c8051f040.h>                       // SFR declarations

// Function Prototypes

void OSCILLATOR_Init (void);           
void PORT_Init (void);
void Readtimer (void);
void Timer3_Init (int counts);
void Timer3_ISR (void);// interrupt 14

//Variablen

#define SYSCLK 3062500                       // approximate SYSCLK frequency in Hz

sfr16 RCAP3    = 0xCA;                       // Timer3 reload value
sfr16 TMR3     = 0xCC;                       // Timer3 counter
sbit   LED     = P1^6;

char Tab7Seg[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
int  reset=0;
int startrutine=0;
int toggle=1;
int framecounter = 1; 

//############## Ethernet Frame ############## 
char frame[32];
char praeambel[7] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char begin = 0xAB;
char ziel[6] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
char quelle[6] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
char typ[2] = {0xfe, 0xff};
char daten[5];
char pad = 0x00;
char crc[4] = {0x00, 0x00, 0x00, 0x00};

void main (void)
{

   WDTCN = 0xde;                             // Disable watchdog timer
   WDTCN = 0xad;

   SFRPAGE =CONFIG_PAGE;
   PORT_Init();                              // Initialize Port I/O
   OSCILLATOR_Init ();                       // Initialize Oscillator
   SFRPAGE =TMR3_PAGE;
   Timer3_Init (SYSCLK/12/1);     
   EA=1;
   SFRPAGE= LEGACY_PAGE;

   P1= 0x00;
  
   while (1)
   {
   P1=P3;
   P2= frame[20];
   }                                         // end of while(1)
}                                            // end of main()

void OSCILLATOR_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;              // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;                    // set SFR page

   OSCICN |= 0x03;                           // Configure internal oscillator for
                                             // its maximum frequency (24.5 Mhz)

   SFRPAGE = SFRPAGE_SAVE;                   // Restore SFR page
}

void PORT_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;              // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;                    // set SFR page before writing to
                                             // registers on this page

   P1MDOUT = 0xff;                           // P1 is push-pull
   P2MDOUT = 0xff;                           // P2 is push-pull
   P3MDOUT = 0x00;                           // P3 is open-drain  jetzt P2

   XBR2    = 0x40;                           // Enable crossbar and enable
                                             // weak pull-ups

   SFRPAGE = SFRPAGE_SAVE;                   // Restore SFR page
}

void Timer3_Init (int counts)
{
   TMR3CN = 0x00;                            // Stop Timer3; Clear TF3;
                                             // use SYSCLK/12 as timebase
   RCAP3   = -counts;                        // Init reload values
   TMR3    = 0xffff;                         // set to reload immediately
   EIE2   |= 0x01;                           // enable Timer3 interrupts
   TR3 = 1;                                  // start Timer3
}


void Timer3_ISR (void) interrupt 14
{
   TF3 = 0;
	 
	if (toggle==1)                             // hier wird der toggle zwischen 0 und einem wert behandelt
	{
	   if ( P3 == 0x00)
	   {
	   		reset=1;
			toggle=0;
	   }
	}
   if ( reset == 1)
   {
		if (P3 != 0x00)                         // Suche nach Preambel
	 	{     
			if (framecounter == 31)
			{
				framecounter=1;
			}
            toggle=1;
			frame[framecounter] = P3;            // hier wird in das Framefeld geschriben



			if ( P3 == 0x55)
				{ 
					startrutine++;
					if (startrutine == 7)          //  Abfrage ob 7 mal paeambel
					{
						framecounter = 7;
						startrutine = 0;
					}
				}
			framecounter++;
			reset=0;
      	}
   }
}