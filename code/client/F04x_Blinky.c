//------------------------------------------------------------------------------------
// F04x_Blinky.c
//------------------------------------------------------------------------------------
// Copyright (C) 2007 Silicon Laboratories, Inc.
//
// AUTH: BD
// DATE: 15 MAR 2002
//
// This program flashes the green LED on the C8051F040 target board about five times
// a second using the interrupt handler for Timer3.
// Target: C8051F04x
//
// Tool chain: KEIL Eval 'c'
//

//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f040.h>                    // SFR declarations

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F04x
//-----------------------------------------------------------------------------

sfr16 RCAP3    = 0xCA;                 // Timer3 reload value
sfr16 TMR3     = 0xCC;                 // Timer3 counter

//------------------------------------------------------------------------------------
// Global CONSTANTS
//------------------------------------------------------------------------------------

#define SYSCLK 3062500                    // approximate SYSCLK frequency in Hz

sbit  LED = P2^6;                         // green LED: '1' = ON; '0' = OFF

//------------------------------------------------------------------------------------
// Function PROTOTYPES
//------------------------------------------------------------------------------------
void PORT_Init (void);
void Timer3_Init (int counts);
void Timer3_ISR (void);

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main (void) {

   // disable watchdog timer
   WDTCN = 0xde;
   WDTCN = 0xad;

   SFRPAGE = CONFIG_PAGE;                 // Switch to configuration page
   PORT_Init ();

   SFRPAGE = TMR3_PAGE;                   // Switch to Timer 3 page
   Timer3_Init (SYSCLK / 12 / 100);        // Init Timer3 to generate interrupts
                                          // at a 10 Hz rate.
   EA = 1;											// enable global interrupts

   SFRPAGE = LEGACY_PAGE;                 // Page to sit in for now

   while (1) {                            // spin forever
   
   		P1=P3;

   }
}

//------------------------------------------------------------------------------------
// PORT_Init
//------------------------------------------------------------------------------------
//
// Configure the Crossbar and GPIO ports
//
void PORT_Init (void)
{
   XBR2    = 0x40;                     // Enable crossbar and weak pull-ups
   P1MDOUT = 0x40;                    // enable P1.6 (LED) as push-pull output
   P2MDOUT = 0x00;
   P3MDOUT = 0x00;
}

//------------------------------------------------------------------------------------
// Timer3_Init
//------------------------------------------------------------------------------------
//
// Configure Timer3 to auto-reload and generate an interrupt at interval
// specified by <counts> using SYSCLK/12 as its time base.
//
//
void Timer3_Init (int counts)
{
   TMR3CN = 0x00;                      // Stop Timer3; Clear TF3;
                                       // use SYSCLK/12 as timebase
   RCAP3   = -counts;                  // Init reload values
   TMR3    = 0xffff;                   // set to reload immediately
   EIE2   |= 0x01;                     // enable Timer3 interrupts
   TR3 = 1;                            // start Timer3
}

//------------------------------------------------------------------------------------
// Interrupt Service Routines
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// Timer3_ISR
//------------------------------------------------------------------------------------
// This routine changes the state of the LED whenever Timer3 overflows.
//
// NOTE: The SFRPAGE register will automatically be switched to the Timer 3 Page
// When an interrupt occurs.  SFRPAGE will return to its previous setting on exit
// from this routine.
//
void Timer3_ISR (void) interrupt 14
{
   TF3 = 0;                               // clear TF3
   LED = ~LED;                            // change state of LED
   
}