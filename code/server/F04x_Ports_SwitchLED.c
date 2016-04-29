// Includes

#include <c8051f040.h>                    // SFR declarations

// Pin Declarations

char Tab7Seg[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F}; // 7-Segment auf Hex codetabelle


sfr16 RCAP3    = 0xCA;                    // Timer3 reload value
sfr16 TMR3     = 0xCC;                    // Timer3 counter
sbit   LED     = P1^6;

#define SYSCLK 3062500                    // approximate SYSCLK frequency in Hz
#define mask 0x04c11DB7                   // frame mask for CRC calculation

int globalcounter = 0;
int ticktock = 0;
int zaehler = 0;
int zaehler2 = 0;
char frame[32];


//############## Ethernet Frame ############## 
char praeambel[7] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char begin = 0xAB;
char ziel[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
char quelle[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
char typ[2] = {0x00, 0x00};
char daten[5];
char pad = 0x00;
char crc[4] = {0x00, 0x00, 0x00, 0x01};
 

 


// Funktionsprototypen

void OSCILLATOR_Init (void);           
void PORT_Init (void);
void Timer3_Init (int counts);
void Timer3_ISR (void);

void crcFunc(void);
 
void sendFrame(void);
void nextFrame (void);



void main (void)
{
   WDTCN = 0xde;                          // Disable watchdog timer
   WDTCN = 0xad;

   SFRPAGE =CONFIG_PAGE;
   PORT_Init();                           // Initialize Port I/O
   SFRPAGE =TMR3_PAGE;
   Timer3_Init (SYSCLK/12/5);     
   EA=1;
   SFRPAGE= LEGACY_PAGE;
   
   while (1)
   {
   daten[0]=Tab7Seg[zaehler2     ];
   daten[1]=Tab7Seg[zaehler2+1   ];
   daten[2]=Tab7Seg[zaehler2+2   ];
   daten[3]=Tab7Seg[zaehler2+3   ];
   daten[4]=Tab7Seg[zaehler2+4   ];
   nextFrame();
   }                                      // end of while(1)
}                                         // end of main()

void OSCILLATOR_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;           // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;                 // set SFR page

   OSCICN |= 0x03;                        // Configure internal oscillator for
                                          // its maximum frequency (24.5 Mhz)

   SFRPAGE = SFRPAGE_SAVE;                // Restore SFR page
}


void PORT_Init (void)
{
   char SFRPAGE_SAVE = SFRPAGE;           // Save Current SFR page

   SFRPAGE = CONFIG_PAGE;                 // set SFR page before writing to
                                          // registers on this page

   P3MDIN |= 0xff;                        // P3 is digital

   P1MDOUT = 0xff;                        // P1 is push-pull
   P3MDOUT = 0x00;                        // P3 is open-drain

   P3     |= 0xff;                        // Set P3 latch to '1'


   XBR2    = 0x40;                        // Enable crossbar and enable
                                          // weak pull-ups

   SFRPAGE = SFRPAGE_SAVE;                // Restore SFR page
}

void Timer3_Init (int counts)
{
   TMR3CN = 0x00;                         // Stop Timer3; Clear TF3;
                                          // use SYSCLK/12 as timebase
   RCAP3   = -counts;                     // Init reload values
   TMR3    = 0xffff;                      // set to reload immediately
   EIE2   |= 0x01;                        // enable Timer3 interrupts
   TR3 = 1;                               // start Timer3
}

void Timer3_ISR (void) interrupt 14
{
   TF3 = 0;
   sendFrame();
}

void sendFrame(void){

if(ticktock==0){
  
    if(globalcounter <= 32)
   {
   P1 = frame[globalcounter];
    P3 = frame[globalcounter];
    globalcounter++;
   }
   else 
   {
      globalcounter = 0;
      zaehler2++;                         //Dieser Zaehler laest die Daten regelmaessig steigen
      if(zaehler2>4){
         zaehler2=0;
      }
    }
   ticktock=1;
}else{                                    //Alles wird zurueckgesetzt
   P1 = 0x00;
   P3 = 0x00;
   ticktock=0;
}

}
 
void nextFrame(void){                     //das Frame aus seinen Bestandteilen zusammenbauen
         frame[0] = praeambel[0]; 
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
         frame[22] = daten[0]; 
         frame[23] = daten[1];
         frame[24] = daten[2];
         frame[25] = daten[3];
         frame[26] = daten[4];
         frame[27] = pad;
 
         //crcFunc();                     // hier kann die crc neu berechnet werden
 
         frame[28] = 0x51;// crc[0];      // feste Werte zum debuggen
         frame[29] = 0x52;// crc[1];
         frame[30] = 0x53;// crc[2];
         frame[31] = 0x54;// crc[3];
}


void crcFunc(void){
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