
.. "Copyright (c) 2008 Robert B. Reese, Bryan A. Jones, J. W. Bruce ("AUTHORS")"
   All rights reserved.
   (R. Reese, reese_AT_ece.msstate.edu, Mississippi State University)
   (B. A. Jones, bjones_AT_ece.msstate.edu, Mississippi State University)
   (J. W. Bruce, jwbruce_AT_ece.msstate.edu, Mississippi State University)

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose, without fee, and without written agreement is
   hereby granted, provided that the above copyright notice, the following
   two paragraphs and the authors appear in all copies of this software.

   IN NO EVENT SHALL THE "AUTHORS" BE LIABLE TO ANY PARTY FOR
   DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
   OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE "AUTHORS"
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE "AUTHORS" SPECIFICALLY DISCLAIMS ANY WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
   AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
   ON AN "AS IS" BASIS, AND THE "AUTHORS" HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."

   Please maintain this header in its entirety when copying/modifying
   these files.

********************************************************************************************************
ledtoggle.c - Uses an FSM approach to toggle an LED whenever a pushbutton input is pressed and released.
********************************************************************************************************
A program that uses a finite state machine approach for toggling an LED whenever a pushbutton switch is pressed and released. Demonstrates the use of debounce delays when polling a switch input.

::

 // wokifvzohtdlm
 
 #include "pic24_all.h"
 
 // wokifvzohtdlm

LED1 configuration and access
=============================

::

 // wokifvzohtdlm
 #define CONFIG_LED1() CONFIG_RB14_AS_DIG_OUTPUT()
 #define LED1 (_LATB14)     //led1 state
 
 // wokifvzohtdlm

Pushbutton configuration and access
===================================

::

 // wokifvzohtdlm
 void config_pb()  {
 #if (HARDWARE_PLATFORM == EMBEDDED_C1)
   CONFIG_RB7_AS_DIG_INPUT();
   ENABLE_RB7_PULLUP();
 #else
   CONFIG_RB13_AS_DIG_INPUT();
   ENABLE_RB13_PULLUP();
 #endif
 // wokifvzohtdlm

..

 ..

  Give the pullup some time to take effect.

::

 // wokifvzohtdlm
   DELAY_US(1);
 }
 
 #if (HARDWARE_PLATFORM == EMBEDDED_C1)
 # define PB_PRESSED()   (_RB7 == 0)
 # define PB_RELEASED()  (_RB7 == 1)
 #else
 # define PB_PRESSED()   (_RB13 == 0)
 # define PB_RELEASED()  (_RB13 == 1)
 #endif
 
 // wokifvzohtdlm

State machine
=============
First, define the states, along with a human-readable version.

::

 // wokifvzohtdlm
 
 typedef enum  {
   STATE_PRESSED,
   STATE_RELEASED,
 } state_t;
 
 const char* apsz_state_names[] = {
   "STATE_PRESSED",
   "STATE_RELEASED",
 };
 
 // wokifvzohtdlm

Provide a convenient function to print out the state.

::

 // wokifvzohtdlm
 void print_state(state_t e_state) {
 // wokifvzohtdlm

..

 ..

  Force an initial print of the state

::

 // wokifvzohtdlm
   static state_t e_last_state = 0xFFFF;
 
 // wokifvzohtdlm

..

 ..

  Only print if the state changes.

::

 // wokifvzohtdlm
   if (e_state != e_last_state) {
     e_last_state = e_state;
 // wokifvzohtdlm

..

 ..

  ..

   ..

    Verify that the state has a string representation before printing it.

::

 // wokifvzohtdlm
     ASSERT(e_state <= N_ELEMENTS(apsz_state_names));
     outString(apsz_state_names[e_state]);
     outChar('\n');
   }
 }
 
 // wokifvzohtdlm

This function defines the state machine.

::

 // wokifvzohtdlm
 void update_state(void) {
   static state_t e_state = STATE_RELEASED;
 
   switch (e_state) {
     case STATE_RELEASED:
       if (PB_PRESSED()) {
         e_state = STATE_PRESSED;
         LED1 = !LED1;
       }
       break;
 
     case STATE_PRESSED:
       if (PB_RELEASED()) {
         e_state = STATE_RELEASED;
       }
       break;
 
     default:
       ASSERT(0);
   }
 
   print_state(e_state);
 }
 
 // wokifvzohtdlm

main
====
This code initializes the system, then runs the state machine above when
the pushbutton's value changes.

::

 // wokifvzohtdlm
 int main (void) {
 // wokifvzohtdlm

..

 ..

  Configure the hardware.

::

 // wokifvzohtdlm
   configBasic(HELLO_MSG);
   config_pb();
   CONFIG_LED1();
 
 // wokifvzohtdlm

..

 ..

  Initialize the state machine's extended state to its starting value.

::

 // wokifvzohtdlm
   LED1 = 0;
 
   while (1) {
     update_state();
 
 // wokifvzohtdlm

..

 ..

  ..

   ..

    Debounce the switch by waiting for bounces to die out.

::

 // wokifvzohtdlm
     DELAY_MS(DEBOUNCE_DLY);
 
 // wokifvzohtdlm

..

 ..

  ..

   ..

    Blink the heartbeat LED to confirm that the program is running.

::

 // wokifvzohtdlm
     doHeartbeat();
   }
 }
