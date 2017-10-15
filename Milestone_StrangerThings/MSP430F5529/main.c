/*
 * Mitchell Hay and Shani Thapa
 * Milestone 1 - Stranger Things Light Wall
 * RU09342
 * MSP430F5529
 */

#include <msp430.h>

volatile unsigned int i = 0;
volatile unsigned int j = 0;
volatile unsigned int r = 0;
volatile unsigned int g = 0;
volatile unsigned int b = 0;
volatile unsigned int byteCnt = 0;
volatile unsigned int numBytes = 0;
int message[80];
int rx;

void main(void) {
	// Clock Setup
	WDTCTL = WDTPW + WDTHOLD;                 	// Stop WDT
	TA0CCR0 = 256 - 1;                        	// PWM Period
	TA0CTL = TASSEL_2 + MC_1 + TACLR;         	// SMCLK, up mode, clear TAR

	// PWM Setup
	P1DIR |= BIT2 + BIT3 + BIT4;				// Set up Pins 1.2,3, and 4 as outputs
	P1SEL |= BIT2 + BIT3 + BIT4;				// Set Pins 1.2,3, and 4 to Timer A0 CCRx
	TA0CCTL1 = OUTMOD_7;                      	// CCR1 reset/set
	TA0CCR1 = r;                            	// CCR1 PWM duty cycle, red LED
	TA0CCTL2 = OUTMOD_7;                      	// CCR2 reset/set,
	TA0CCR2 = g;                            	// CCR2 PWM duty cycle, green LED
	TA0CCTL3 = OUTMOD_7;                      	// CCR3 reset/set
	TA0CCR3 = b;                            	// CCR3 PWM duty cycle, blue LED

	// UART Setup
	P3SEL |= BIT3 + BIT4;                       // P3.3,4 = USCI_A0 TXD/RXD
	UCA0CTL1 |= UCSWRST;                      	// **Put state machine in reset**
	UCA0CTL1 |= UCSSEL_1;                     	// ACLK
	UCA0BR0 = 3;                        		// 32726 MHz/3 = 9600 (see User's Guide)
	UCA0BR1 = 0;                              	// 1MHz 3
	UCA0MCTL |= UCBRS_3 + UCBRF_0;            	// Modulation UCBRSx=1, UCBRFx=0
	UCA0CTL1 &= ~UCSWRST;                   	// **Initialize USCI state machine**
	UCA0IE |= UCRXIE;                         	// Enable USCI_A0 RX interrupt

	__enable_interrupt();						// Enable Interrupts
	__bis_SR_register(LPM0 + GIE);       		// Enter LPM0, interrupts enabled
}

// Transmit and Receive Interrupts
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
	switch (__even_in_range(UCA0IV, 4)) {
	case 0:
		break;                             		// Vector 0 - no interrupt
	case 2:                                   	// Vector 2 - RXIFG
		while (!(UCA0IFG & UCTXIFG))
			;
		rx = UCA0RXBUF;							// Hold received character

		// Check first number of message
		if (byteCnt == 0) {
			numBytes = rx;
			byteCnt++;
			message[i] = numBytes - 3;
			i++;
		}

		// Assign RGB values to CCRs
		else if (byteCnt >= 1 && byteCnt <= 3) {
			switch (byteCnt) {
			case 1:
				r = rx;							// Set red to the 2nd int received
				TA0CCR1 = r;                   	// CCR1 PWM duty cycle, red
				byteCnt++;						// Increment byte count
				break;
			case 2:
				g = rx;							// Set green to 3rd int received
				TA0CCR2 = g;                    // CCR2 PWM duty cycle, green
				byteCnt++;						// Increment byte count
				break;
			case 3:
				b = rx;							// Set blue to 4th int received
				TA0CCR3 = b;                    // CCR3 PWM duty cycle, blue
				byteCnt++;						// Increment byte count
				break;
			}
		}

		// Write all of the other values
		else if (byteCnt < numBytes)
				{
			message[i] = rx;
			i++;
			byteCnt++;
			// If the message is over, transmit to next person
			if (byteCnt == numBytes) {
				while (message[j] != 0x0D) {
					while (!(UCA0IFG & UCTXIFG))
						;
					UCA0TXBUF = message[j];
					j++;
				}
				while (!(UCA0IFG & UCTXIFG))
					;
				UCA0TXBUF = 0x0D;				// Add to end of message
			}
		}
		break;
	case 4:
		break;
	default:
		break;
	}
}
