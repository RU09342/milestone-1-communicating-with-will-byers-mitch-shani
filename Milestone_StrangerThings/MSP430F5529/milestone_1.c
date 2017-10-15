#include <msp430.h>

volatile unsigned int i = 0;
volatile unsigned int j = 0;
volatile unsigned int r = 0;
volatile unsigned int g = 0;
volatile unsigned int b = 0;
volatile unsigned int byteCnt = 0;
volatile unsigned int numBytes = 0;
char message[80];
char greeting[20] = "Type s, m, f, or o:"; // Initial Greeting you should see upon properly connecting your Launchpad

void main(void) {
	WDTCTL = WDTPW + WDTHOLD;                 	// Stop WDT

	TA0CCR0 = 256 - 1;                        	// PWM Period
	TA0CTL = TASSEL_2 + MC_1 + TACLR;         	// SMCLK, up mode, clear TAR

	P1DIR |= BIT2 + BIT3 + BIT4;
	P1SEL |= BIT2 + BIT3 + BIT4;
	TA0CCTL1 = OUTMOD_7;                      	// CCR1 reset/set
	TA0CCR1 = r;                            	// CCR1 PWM duty cycle
	TA0CCTL2 = OUTMOD_7;                      	// CCR2 reset/set
	TA0CCR2 = g;                            	// CCR2 PWM duty cycle
	TA0CCTL3 = OUTMOD_7;                      	// CCR3 reset/set
	TA0CCR3 = b;                            	// CCR3 PWM duty cycle

	P3SEL |= BIT3 + BIT4;                       // P3.3,4 = USCI_A0 TXD/RXD
	UCA0CTL1 |= UCSWRST;                      	// **Put state machine in reset**
	UCA0CTL1 |= UCSSEL_1;                     	// ACLK
	UCA0BR0 = 3;                              	// 32726 MHz/3 = 9600 (see User's Guide)
	UCA0BR1 = 0;                              	// 1MHz 3
	UCA0MCTL |= UCBRS_3 + UCBRF_0;            	// Modulation UCBRSx=1, UCBRFx=0
	UCA0CTL1 &= ~UCSWRST;                   	// **Initialize USCI state machine**
	UCA0IE |= UCRXIE;                         	// Enable USCI_A0 RX interrupt

	while (greeting[i] != '\0') {
			while (!(UCA0IFG & UCTXIFG))
				;           // USCI_A0 TX buffer ready?
			{
				UCA0TXBUF = greeting[i];                  // TX -> RXed character
				i++;
			}
		}


	__enable_interrupt();
	__bis_SR_register(LPM0 + GIE);       		// Enter LPM0, interrupts enabled
}

// Echo back RXed character, confirm TX buffer is ready first
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
		break;                             // Vector 0 - no interrupt
	case 2:                                   // Vector 2 - RXIFG
		while (!(UCA0IFG & UCTXIFG));
		if (byteCnt == 0)
		{
			numBytes = UCA0RXBUF;
			byteCnt++;
			message[i] = numBytes - 3;
			i++;
		}
		else if (byteCnt >= 1 && byteCnt <= 3)
		{
			switch(byteCnt) {
				case 1:
					r = UCA0RXBUF;
					byteCnt++;
					break;
				case 2:
					g = UCA0RXBUF;
					byteCnt++;
					break;
				case 3:
					b = UCA0RXBUF;
					byteCnt++;
					break;
			}
		}
		else if (byteCnt < numBytes)
		{
			message[i] = UCA0RXBUF;
			i++;
			byteCnt++;
		}

		while (message[j] != 0x0D)
		{
			while (!(UCA0IFG & UCTXIFG));
			UCA0TXBUF = message[j];
			j++;
		}
		break;
	case 4:
		break;
	default:
		break;
	}
}

