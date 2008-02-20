/* LiteOS Version 0.3 */
/*
The following is the license of LiteOS.

This file is part of LiteOS.
Copyright Qing Cao, 2007-2008, University of Illinois , qcao2@uiuc.edu

LiteOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LiteOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LiteOS.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#include <avr\io.h>
#include <avr/interrupt.h>
#define xtal 8000000L
#define baud 57600

#include "serialprint.h"
#include "../../platform/micaz/realmain.h"
#include "../../platform/avr/avrhardware.h"
#include "../../system/stdserialhandler.h"


static char cFlag;
static uint8_t receivebuffer[ 33 ];
static uint8_t previous;
static uint8_t status;


//No need to be called from main 
void initUSART() {
   UBRR0H = 0;
   UBRR0L = 15;
   UCSR0A |= _BV( U2X0 ); // double speed
   UCSR0C |= _BV( UCSZ01 ) | _BV( UCSZ00 ); // 8 data-bits, 1 stop-bit
   UCSR0B |= _BV( RXCIE0 ) | _BV( RXEN0 ) | _BV( TXEN0); // Enable recieve/transmit/interrupts
   cFlag = 1;
   //	  currentindex = 0; 
   previous = 0;
   status = 0;
}

//-------------------------------------------------------------------------
void usartPrint( uint8_t c )
 {
   if ( cFlag == 0 ) {
      initUSART();
   } 
   // Loop until UDR register is empty
   while (( UCSR0A& ( 1 << UDRE0 )) == 0 )
      ;
   // Transmit letter a
   UDR0 = c;
}

//-------------------------------------------------------------------------
void printString( char *p ) {
   //usartPrint(0);
   while (( *p ) != '\0' ) {
      usartPrint( *p );
      p ++;
   }
}

//-------------------------------------------------------------------------
void printStringN( char *p, uint8_t n ) {
   uint8_t i;
   for ( i = 0; i < n; i ++ ) {
      usartPrint( *p );
      p ++;
   }
}
//This function prints the value of uint8_t using '0','1',...'f'
void usartPutChipHex( uint8_t cChip ) {
   if ( cChip > 9 ) {
      usartPrint( 'a' + cChip - 10 );
   } else {
      usartPrint( '0' + cChip );
   }
}
//this function outputs the value of c
void usartPutHex( uint8_t c ) {
   usartPutChipHex( c >> 4 );
   usartPutChipHex( c& 0xf );
}
//this function outputs the value of l . note avr is small-endian so the result is from the right-hand start 
void usartPutLong( uint32_t l ) {
   uint8_t *pcByte = (( uint8_t* )( &l )) + 3;
   usartPrint( '0' );
   usartPrint( 'x' );
   usartPutHex( *pcByte );
   pcByte --;
   usartPutHex( *pcByte );
   pcByte --;
   usartPutHex( *pcByte );
   pcByte --;
   usartPutHex( *pcByte );
}

//-------------------------------------------------------------------------
void printInteger( int32_t a, int32_t b, int32_t c, int32_t d ) {
   usartPrint( 1 );
   usartPutLong( *( uint32_t* )( &a ));
   usartPutLong( *( uint32_t* )( &b ));
   usartPutLong( *( uint32_t* )( &c ));
   usartPutLong( *( uint32_t* )( &d ));
}


/*
int main()
{
printString("hello, world! Here ai amdfasdsafffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\n");
printInteger(-32, 32, 0,0);
printInteger(-332, 2232, -232,0);
printInteger(332, 1132, 340,0);
printInteger(432, 111132, 3430,0);
printInteger(532, 343432, 0,45);
while(1);
return 0; 
}
 */
/*
SIGNAL(SIG_UART0_RECV)
{
uint8_t dummy = UDR0; 
currentindex ++; 
if (currentindex ==3)
{  printString("hello, reply\n\0"); 
currentindex = 0;
}
} 
 */






///Must get two sync to begin record. 
///Receive two sync to stop record. ad then use the commandHandle to handle it. 
///Serves two goals:
///1 the initilization starts with z 2 the comm where two starting bytes tell the port and the message length 


SIGNAL( SIG_UART0_RECV ) {
   uint8_t dummy = UDR0;
   uint8_t currentindex = 0;
   uint8_t portnum, length;
   uint8_t i;
   if ( dummy == 'a' ) {
      _atomic_t _atomic = _atomic_start();
	   while (  ! ( UCSR0A& ( 1 << RXC0 )))
            ;
	   portnum= UDR0;
	   portnum = portnum - 48; 

       while (  ! ( UCSR0A& ( 1 << RXC0 )))
            ;
	   length = UDR0;
	   length = length - 48; 

	   
	   
      for ( i = 0; i < length; i ++ ) {
         while (  ! ( UCSR0A& ( 1 << RXC0 )))
            ;
         receivebuffer[ currentindex ++ ] = UDR0;
      }
      _atomic_end( _atomic );
     
      //commandHandle(receivebuffer, currentindex); 
      //initCommandReceived(receivebuffer); 
      serialDeliverPacket((( uint16_t )portnum ), (( uint8_t )length ), &receivebuffer[ 0 ] );
	  currentindex = 0; 
   	}
    else if ( dummy == 'z' ) {
      _atomic_t _atomic = _atomic_start();
      for ( i = 0; i < 33; i ++ ) {
         while (  ! ( UCSR0A& ( 1 << RXC0 )))
            ;
         receivebuffer[ currentindex ++ ] = UDR0;
      }
      _atomic_end( _atomic );
      initCommandReceived( receivebuffer );
	  currentindex = 0; 
     
   }
  
}