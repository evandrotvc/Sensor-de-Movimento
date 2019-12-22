#include <msp430.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../inc/header.h"


#define discrepancia_medida 100
uint8_t hora=0 , min=0 , sec =0;

typedef enum{
    livre,
    Armando,
    Armado,
    Disparado
} State;

//
uint8_t analisa_distancias(uint16_t, uint16_t);
void Mede_distancias(void);


uint8_t good_read = 0, ocupado = 0, falhou_armacao = 0;
uint16_t distancia = 0, start = 0, end = 0;


void main(void) {


    RTCCTL01 |= RTCHOLD;// para o RTC(usa o hold),

   // RTCCTL01 |= RTCTEVIE; // RTC Event IE

    RTCCTL01 = RTCSSEL_2 | RTCTEV_0 | RTCHOLD| RTCMODE; //  Modo calendario,



    RTCSEC = 0x00;
    RTCMIN = 50;
    RTCHOUR = 0x0E; // Define a hora inicial como-> 14:50:00

    RTCCTL01 &= ~RTCHOLD; // inicia a contagem do rtc lipando o bit do hold



    State CurrentState = livre;

    uint16_t i, vetor_medidas[4], media;

    WDTCTL = WDTPW | WDTHOLD;    // pare watchdog timer



    /* Port usadas:
        P1.0 -> LED 1
        P1.1 -> botao 2
        P1.2 -> sensor de proximidade  trigger
        P2.0 -> sensor de proximidade echo (TA1.1)
        P2.1 -> botao 1
        P3.3 -> Bluetooth RX
        P3.4 -> Bluetooth TX
        P4.7 -> LED 2
    */


    Config_chaves_leds();

    // Configure Timer A0 para controlar os leds quando disparado


        TA0CTL = TASSEL__ACLK | MC_0 | TACLR; // timer começa do 0 e em modo espera(hold)
        TA0CCR0 = 8191; // frequencia = 4 Hertz.
        TA0CCTL0 |= CCIE; // habilita interrupcao para esse canal

    /* Configuracao do sensor de proximidade HC-SR04:*/

        P1DIR |= BIT2;
        P1OUT &= ~BIT2;
        P2DIR &= ~BIT0;
        P2SEL |= BIT0;



        TA1CCTL1 = CAP | CM_3| CCIS_0 | CCIE ; // configura mdo de captura do hc-sr04


        TA1CTL = TASSEL__SMCLK | MC_2 | TACLR ; // configura o timer SMCLK , modo continuos e contagem inicia do 0

    // configuracao modulo bluetooth:

        ConfigUARTModule0(1, 104, 1, 0);








        __enable_interrupt();



        while(1)
        {

            switch(CurrentState)
            {

                case livre:

                    /*Nesta etapa, o msp aguarda o usuario apertar a chave s1, ou seja, está livre*/


                    P1OUT |= BIT0;    // Liga o LED1.
                    P4OUT &= ~BIT7;  // Desliga o LED2.


                    if( (P2IN & BIT1) == 0  )
                    {
                        Debounce(); // aguarda um momento devido a possibilidade de gerar ruido apos o acionamento da chave
                        CurrentState = Armando;

                        while( (P2IN & BIT1) == 0 ); // enquanto está pressionado, fique
                        Debounce();
                    }

                    break;

                case Armando:

                    /*Nesta etapa, o msp tem que fazer 4 medidas de distancias, para assegurar que
                    todas as medidas não tenham altas discrepancias,após fazer as 4 medidas ,tire a media aritmetica entre as medidas
                    e compare com a variavel discrepancia_medida.
                    Alguma medida foi discrepante? Volte pro estado livre, já que a armação falhou!
                    */

                    media = 0;
                    falhou_armacao = 0;

                    for(i=0; i<4; i++)
                    {


                        // Ambos os leds são ligados, só pra indicar que as medidas estão sendo feitas
                        P1OUT |= BIT0;
                        P4OUT |= BIT7;

                        Mede_distancias();
                        while(ocupado);
                        vetor_medidas[i] = distancia;
                        media += distancia;

                        Delay40Microsseconds(18750);    // espera 0,75 segundos de acordo com as configuracoes do header

                        P1OUT &= ~BIT0; // Desliga o LED1.
                        P4OUT &= ~BIT7; // Desliga o LED2.

                        Delay40Microsseconds(6250);     // epsera 0.25 secondos.

                    }

                    media >>= 2; // Pegue a media e divida por 4 (desconsiderando parte fracionaria)

                    // Analisa cada distancia medida

                    for(i=0; i<4; i++) {

                        if(!analisa_distancias(media, vetor_medidas[i])){
                            falhou_armacao = 1;
                            break;
                        }

                    }

                    if(falhou_armacao == 0) // todas medidas estão ok? Arme o alarme!
                        CurrentState = Armado;

                    else
                        CurrentState = livre;

                    break;

                case Armado:

                    /* Etapa no qual o msp está armado, ou seja, ele sempre está verificando as medidas
                    do objeto. A interrupcão pegas as medidas tanto da borda de subida e descida e assim,
                    analisa se tais medidas estão discrepantes da media(valor calculado na etapa anterior).
                    Se deu valor discrepante, significa que houve movimento, logo Dispare o alarme!

                    */

                    P1OUT &= ~BIT0; // Desliga o LED1.
                    P4OUT |= BIT7; // Liga o LED2.

                    DelaySeconds(1);

                    UARTM0SendString("Alarme armado!\n", 11); // envia a mensagem via bluetooth

                    while(CurrentState == Armado)
                    {


                        if( (P1IN & BIT1) == 0 ) // botao s2 foi pressionado? resete o alarme
                        {
                            Debounce();
                            CurrentState = livre;

                            while( (P1IN & BIT1) == 0 ); // aguarde este mesmo botao ser solto
                            Debounce();
                            UARTM0SendString("Alarme reiniciado!\n", 13);
                        }

                        else {

                            Mede_distancias(); // faca medidas
                            while(ocupado); // trava que aguarda o terminío da interrupcao, para ter o valor da distancia

                            DelayMicrosseconds(1000);

                            if(!analisa_distancias(media, distancia)) // |media - distancia | > discrepancia_medida ??
                                CurrentState = Disparado; // se sim, dispare o alarme

                        }

                    }

                    break;

                case Disparado:

                    /* Etapa no qual o msp avisa usuario de que o alarme foi disparado.
                    enquanto não for acionado o botao s2 de reset, fique piscando alternado
                    os leds. reset pressionado? volte pra etapa livre

                    */

                    // timer controla os LEDs para piscarem alternadamente


                    TA0CTL |= MC__UP; // inicie timer dos leds
                    TA0CCTL0 |= CCIE;


                    char buf1[10], buf2[10], buf3[10], horario[50];

                    //pega a hora , minuto e segundo
                    sec = RTCSEC;
                    min = RTCMIN;
                    hora = RTCHOUR;
                    // converte um numero inteiro para uma string
                            sprintf(buf1, "%d", sec);
                            sprintf(buf2, "%d", min);
                            sprintf(buf3, "%d", hora);

                            strcpy(horario,buf3);
                            strcat(horario,":");
                            strcat(horario,buf2);
                            strcat(horario,":");
                            strcat(horario,buf1);


                    UARTM0SendString(horario, 17); // envia mensagem para o usuario
                    UARTM0SendString("Alarme Disparado!\n", 17); // envia mensagem para o usuario


                    while( (P1IN & BIT1) != 0 );  // aguarda a chave s2 ser pressionada, assim os leds ficam alternando

                    // VOLTE ao estadolivre.

                    Debounce();
                    CurrentState = livre;

                    while( (P1IN & BIT1) == 0 ); // aguarda a chave ser solta
                    Debounce();

                    UARTM0SendString("Alarme reiniciado!\n", 13); // avisa o usuario que foi reiniciado o alarme!

                    TA0CCTL0 &= ~CCIE;
                    // Pare o timer
                    TA0CTL &= ~MC__UP;


                    break;

                }

        }

}


void Mede_distancias(void) {

    P1OUT |= BIT2; // envia trigger
    DelayMicrosseconds(12);

    P1OUT &= ~BIT2; // limpa o pino para reenviar o trigger novamente
    ocupado = 1;
}

uint8_t analisa_distancias(uint16_t media, uint16_t distancia) {

    if(abs(media - distancia) < discrepancia_medida)
        return 1;

    return 0;

}


#pragma vector = TA0_CCR0_INT
__interrupt void TA0_CCR0_ISR()
{
      P1OUT ^= BIT0; // inverte estado do led1
      P4OUT ^= BIT7; // inverte estado do led2
      TA0CCTL0 &= ~CCIFG; // Nesse modo de interrupcao, sempre tem que limpar a flag de interrupção.
}

#pragma vector = TA1_CCRN_INT
__interrupt void TA1_CCRN_ISR()
{

        switch(TA1IV)
        {
        // interrupcao do sensor de proximidade
            case 0x02:

            // pega a distancia na borda de subida

                if((TA1CCTL1 & CCI) == CCI)
                {
                    start = TA1CCR1;
                    good_read = 1;

                }

            // pega a distancia na borda de descida

                if((TA1CCTL1 & CCI) == 0)
                {

                            end = TA1CCR1;
                            distancia = (end - start); // subtracao para comparar com o valor da media, calculada no estagio armando
                            good_read = 0;
                            ocupado = 0;

                }

            break;
        }

}


/*
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void){
    switch(RTCIV)
    {
        case 0x04:              // RTCEVIFG
           // if(RTCSEC <10)
           //   P1OUT ^= BIT0;

           // min = RTCMIN;
            break;
        default: break;
    }
}
*/
