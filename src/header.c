#include "../inc/header.h"


void Config_chaves_leds(void) {

     // configura Chave S1 (P2.1).
    P2DIR &= ~BIT1;
    P2REN |= BIT1;
    P2OUT |= BIT1;
    // configura Chave S2 (P1.1).
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;

    // configura LED vermelho (P1.0).

    P1DIR |= BIT0; 
    P1OUT &= ~BIT0;
    
    // configura LED verde (P4.7).

    P4DIR |= BIT7;
    P4OUT &= ~BIT7;
    
}

int ConfigUARTModule0(uint8_t clock, uint16_t BRW, uint8_t BRF, uint8_t BRS) {

  P3DIR |= BIT3; /*Configura o pino P3.3 como transmissor da comunicação uart*/
  P3SEL |= BIT3;

  P3DIR &= ~BIT4; /*Configura o pino P3.4 como transmissor da comunicação uart*/
  P3SEL |= BIT4;

  
  UCA0CTL1 |= UCSWRST; // Seta o reset para poder configurar os registros 
  
  UCA0CTL0 = 0; // resetou todos os bits do registro.Ou seja, modo é uart , assincrono ,8 bit de dados e 1 bit de stop
  UCA0CTL1 |= UCSSEL__ACLK; // seleciona o clock aclk

  UCA0BRW = 3;

  UCA0MCTL = UCBRS_3; // seleciona o segundo estagio da modulação ver

  UCA0CTL1 &= ~UCSWRST;

  return 0;
  
}

int Debounce(void) {

    volatile int counter = 0xDFFF;

    while(counter != 0)
        counter--;

    return 0;

}

int DelayMicrosseconds(uint16_t number) {
	
	
	TA2CTL = TASSEL__SMCLK | MC_2 | TACLR;
	while(TA2R <= number);
	
	return 0;
	
}

int Delay40Microsseconds(uint16_t number) {
	
	TA2EX0 = 4;	// entrada que expande a divisão do clock por 5.
	
	                                       // ou seja, como smlkc-> 1mhz = 1 seg -> 1mhz / 5 = 200000/8 = 25000 representa 1 seg
    

    TA2CTL = TASSEL__SMCLK | MC_2 | TACLR | ID_3; // configura o clock por smclk , divide por 8
	while(TA2R <= number);
	
	TA2EX0 = 0;
	
	return 0;
	
}

int DelaySeconds(uint8_t number) {
	
	TA2EX0 = 7;	 // entrada que expande a divisão do clock por 6. 
	
	TA2CTL = TASSEL__ACLK | MC_2 | TACLR | ID_3; // configura o clock por aclk , divide o clock por 8
	if(number <= 127)
		while(TA2R <= (number << 9));

	else {
		while(TA2R == 0);						//assegura o inicio do timer
		while(TA2R != 0);						
		while(TA2R <= ((number & 0x7F) << 9));	
	}
	
	TA2EX0 = 0;	
	
	return 0;
	
}


  int UARTM0SendString(char *string ,uint16_t size){ // funcao que envia a string desejada pela uart
    int i = 0;
    while((string[i] != '\0')){ // enquanto não chegar no final da string, realize o loop
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = string[i];
        while(!(UCA0IFG & UCTXIFG));
        i++;
    }
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = '\0';
	
    return 0;
	}
