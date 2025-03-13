#include <driverlib.h>
#include <msp430.h>

#define TRUE  1
#define FALSE 0

unsigned char LCD_Num[10] = {0xfc, 0x60, 0xdb, 0xf3, 0x67, 0xb7, 0xbf, 0xe0, 0xff, 0xe7};

void lcd_display_int(unsigned int n);
void initialize_LCD(void);
void config_ACLK_to_32KHz_crystal(void);

// mailbox data structure to isolate timer's logic and button's logic
struct mbox {
    volatile unsigned int count;
    volatile unsigned char play_flag;
    volatile unsigned char clear_flag;
} counter_mbox;

int main(void) {
    // Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    initialize_LCD();
    config_ACLK_to_32KHz_crystal();

    counter_mbox.play_flag  = TRUE;  // play set to true
    counter_mbox.clear_flag = FALSE; // clear flag set to false
    counter_mbox.count = 0;          // count initialized to 0

    lcd_display_int(counter_mbox.count); // we display the count for the first time

    P1DIR |= BIT0;
    P1DIR &= ~(BIT1 | BIT2); // p1.1 and p1.2 set to input
    P1OUT |= BIT1 | BIT2;    // pull-up resistors for p1.1 and p1.2

    P1SEL0 &= ~(BIT0);  // Port1 configured as I/O
    P1SEL1 &= ~(BIT0);

    P1REN |= BIT1 | BIT2; // enable p1.1 and p1.2 resistors

    P1IES |= BIT1 | BIT2; // falling_edge interrupt signal
    P1IE  |= BIT1 | BIT2; // enable p1.1 and p1.2 interrupt
    P1IFG &= ~(BIT1 | BIT2); // clear p1.1 and p1.2 interrupt flags
    _BIS_SR(GIE); // enable global interrupt

    TA0CTL = TASSEL_1 | TACLR | MC_1;
    TA1CTL = TASSEL_1 | TACLR | MC_1;
    TA0CCR0 = 40000;
    TA1CCR0 = 40000;
    TA0CCTL0 = CCIE;
    TA1CCTL0 = CCIE;

    __low_power_mode_0();
    __no_operation();

}

#pragma vector=PORT1_VECTOR
__interrupt void rti_p1(void) {
    if(P1IFG & BIT2) {
        counter_mbox.play_flag ^= 1;
        P1IFG &= ~BIT2;
    }

    if(P1IFG & BIT1) {
        counter_mbox.clear_flag = TRUE;
        P1IFG &= ~BIT1;
    }
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
    if(counter_mbox.play_flag)
        ++counter_mbox.count;

    if(counter_mbox.clear_flag) {
        counter_mbox.count = 0;
        counter_mbox.clear_flag = FALSE;
    }
    lcd_display_int(counter_mbox.count);
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void) {
    if(!(counter_mbox.count % 4))
        P1OUT |= BIT0;
    else
        P1OUT &= ~(BIT0);
}

void lcd_display_int(unsigned int n) {
    unsigned int n_digits = 0;
    unsigned int digit = n;
    while(digit) {
        digit /= 10;
        ++n_digits;
    }
    switch(n_digits) {
    case 5:
        LCDM6  = LCD_Num[n / 10000];
        LCDM4  = LCD_Num[(n / 1000) % 10];
        LCDM19 = LCD_Num[(n / 100) % 10];
        LCDM15 = LCD_Num[(n / 10) % 10];
        LCDM8  = LCD_Num[n % 10];
        break;
    case 4:
        LCDM4  = LCD_Num[n / 1000];
        LCDM19 = LCD_Num[(n / 100) % 10];
        LCDM15 = LCD_Num[(n / 10) % 10];
        LCDM8  = LCD_Num[n % 10];
        break;
    case 3:
        LCDM19 = LCD_Num[(n / 100)];
        LCDM15 = LCD_Num[(n / 10) % 10];
        LCDM8  = LCD_Num[n % 10];
        break;
    case 2:
        LCDM15 = LCD_Num[(n / 10)];
        LCDM8  = LCD_Num[n % 10];
        break;
    case 1:
        LCDM8  = LCD_Num[n];
        break;
    default:
        LCDM4  = 0x0;
        LCDM6  = 0x0;
        LCDM8  = LCD_Num[0];
        LCDM15 = 0x0;
        LCDM19 = 0x0;
        break;
    }
}

void initialize_LCD(void) {
    PJSEL0 = BIT4 | BIT5; // For LFXT

    // Initialize LCD segments 0 - 21; 26 - 43
    LCDCPCTL0 = 0xFFFF;
    LCDCPCTL1 = 0xFC3F;
    LCDCPCTL2 = 0x0FFF;

    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8; // Unlock CS registers
    CSCTL4 &= ~LFXTOFF; // Enable LFXT

    do {
        CSCTL5 &= ~LFXTOFFG; // Clear LFXT fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG); // Test oscillator fault flag
    CSCTL0_H = 0; // Lock CS registers

    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;

    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
    LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled
    LCDCMEMCTL = LCDCLRM; // Clear LCD memory

    //Turn LCD on
    LCDCCTL0 |= LCDON;
}

//**********************************
// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal(void) {
    // By default, ACLK runs on LFMODCLK at 5MHz/128 = 39 KHz
    // Reroute pins to LFXIN/LFXOUT functionality
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;

    // Wait until the oscillator fault flags remain cleared
    CSCTL0 = CSKEY; // Unlock CS registers
    do {
        CSCTL5 &= ~LFXTOFFG; // Local fault flag
        SFRIFG1 &= ~OFIFG; // Global fault flag
    } while((CSCTL5 & LFXTOFFG) != 0);

    CSCTL0_H = 0; // Lock CS registers
    return;
}
