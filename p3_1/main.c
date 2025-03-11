#include <driverlib.h>
#include <msp430.h>

int main(void) {
    // Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    P1DIR |= BIT0;
    P1DIR &= ~(BIT1 | BIT2);
    P1OUT |= BIT0 | BIT1 | BIT2;

    P1SEL0 &= ~(BIT0);
    P1SEL1 &= ~(BIT0);

    P1REN |= BIT1 | BIT2;

    P1IES |= BIT1 | BIT2;
    P1IE  |= BIT1 |BIT2;
    P1IFG &= ~(BIT1 | BIT2);
    _BIS_SR(GIE);

    TA0CTL = TASSEL_1 | TACLR | MC_1;
    TA0CCR0 = 40000;
    TA0CCTL0 = CCIE;

    __low_power_mode_0();
    __no_operation();

}

#pragma vector=PORT1_VECTOR
__interrupt void rti_p1(void) {
    if(P1IFG & BIT1) {
        TA0CTL ^= (1 << 4);
        P1IFG &= ~BIT1;
    }

    if(P1IFG & BIT2) {
        TA0R = 0x0;
        P1IFG &= ~BIT2;
    }
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
    P1OUT ^= 1;
}
