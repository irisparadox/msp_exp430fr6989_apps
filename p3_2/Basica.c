
#include <msp430.h>
#include <driverlib.h>
#include <stdio.h>

volatile unsigned int number;
unsigned char LCD_Num[10] = {0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE0, 0xFF, 0xE7};
//**********************************************************
// Initializes the LCD_C module
// *** Source: Function obtained from MSP430FR6989’s Sample Code ***
void Initialize_LCD() {
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
}while (SFRIFG1 & OFIFG); // Test oscillator fault flag
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
return;
}

//***************function that displays any 16-bit unsigned integer************
void display_num_lcd(unsigned int n){
//initialize i to count though input paremter from main function, digit is used for while loop so as long as n is not
//0 the if statements will be executed.
int i;
int digit;
digit = n;
while(digit!=0){
digit = digit*10;
i++;
}
if(i>1000){
LCDM8 = LCD_Num[n%10]; // inputs the first(least significant digit) from the array onto the LCD output.
n=n/10;
i++;
}
if(i>100){
LCDM15 = LCD_Num[n%10]; // inputs the second(least significant digit) from the array onto the LCD output.
n=n/10;
i++;
}
if(i>10){
LCDM19 = LCD_Num[n%10]; // inputs the third(least significant digit) from the array onto the LCD output.
n=n/10;
i++;
}
if(i>1){
LCDM4 = LCD_Num[n%10]; // inputs the fourth(least significant digit) from the array onto the LCD output.
n=n/10;
i++;
}
if(i>0){
LCDM6 = LCD_Num[n%10]; // inputs the last (most significant digit) from the array onto the LCD output.
n=n/10;
i++;
}
}

//**********************************
// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() {
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


void main(void){

    // Detener el watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Configuración del LED rojo (P1.0)
 P1SEL0 &= ~(BIT0); 
 P1SEL1 &= ~(BIT0); 
 P1DIR |= BIT0;  
 P1OUT &= ~BIT0; // Apagar el LED inicialmente

 // Configuración del LED verde (P9.7)
 P9SEL0 &= ~(BIT7); 
 P9SEL1 &= ~(BIT7);
 P9DIR |= BIT7;  
 P9OUT &= ~BIT7; // Apagar el LED inicialmente

 // Configuración de los pulsadores (P1.1 y P1.2)
 P1DIR &= ~(BIT1 | BIT2); // Configurar como entrada
 P1REN |= (BIT1 | BIT2);  // Habilitar resistencias internas
 P1OUT |= (BIT1 | BIT2);  // Configurar como pull-up
 //Interrupciones para pulsadores
    P1IE |= (BIT1|BIT2);   // Habilitar interrupciones en P1.1
    P1IES |= (BIT1|BIT2);  // Interrupción en flanco de subida (cuando se suelta el botón), si lo pongo a 0 se queda en flanco de bajada
    P1IFG &= ~(BIT1|BIT2); // Limpiar la bandera de interrupción


// Configuración TIMER_A:
 // TimerA1, ACLK/1, modo up, reinicia TACLR
  TA1CTL = TASSEL_1 | TACLR | MC_1;
 // ACLK tiene una frecuencia de 32768 Hz
 // Carga cuenta en TA1CCR0 0.1seg TA1CCR=(0,1*32768)-1
    TA1CCR0 = 40000;
    TA1CCTL0 = CCIE; // Habilita interrupción (bit CCIE)


//incialización del LCD
Initialize_LCD();
config_ACLK_to_32KHz_crystal();

    number = 0;
    // Desbloquear los puertos de GPIO
    PMM_unlockLPM5();

    // Entrar en modo de bajo consumo y habilitar interrupciones globales
    __bis_SR_register(LPM4_bits + GIE);

    while(1);
}

// Rutina de interrupciones puerto 1
#pragma vector= PORT1_VECTOR
__interrupt void Port1(void){
    if (P1IFG & BIT1) { // Botón en P1.1
        P1OUT ^= BIT0; // Cambiar estado del LED rojo
        TA1CCTL0 ^= CCIE; // Habilitar/deshabilitar interrupción del Timer A1
        P1IFG &= ~BIT1; // Limpiar bandera de interrupción
    }
    if (P1IFG & BIT2) { // Botón en P1.2
        P9OUT ^= BIT7; // Cambiar estado del LED verde
        number = 0; // Reiniciar contador
        display_num_lcd(number); // Actualizar LCD
        P1IFG &= ~BIT2; // Limpiar bandera de interrupción
    }
}

// Rutina de interrupción de TIMER1_A0
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void){
 display_num_lcd(number);
 number++;
}
