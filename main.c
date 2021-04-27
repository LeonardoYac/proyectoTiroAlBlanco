//*** librerias obligatorias ***
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
// *** librerias adicionales ***
#include "driverlib/sysctl.h" // configuracion iniciales del procesador
#include "driverlib/gpio.h"
// *** otras librerias adicionales ***
#include "inc/tm4c123gh6pm.h" //#include "inc/tm4c123gh6pm.h" >> // para uso de registros exclusivos para dicho procesador
// *** libreria para las interrupciones ***
#include "driverlib/interrupt.h"

// *** variables ***
int moduloLDR;                                      // variable para detectar los modulos.
int numbers[10] = {64,121,36,48,25,18,2,120,0,24};  // datos del display. 0, 1, 2, 3, 4, 5, 6, 7, 8, 9.
int before=1;                                       // variable para bloqueo al sensar el ldr.
int CONT;                                           // variable conteo para mostrar en el display.
int pos1, pos2, pos3, pos4;                         // variable para coordinar secuencia. [LEDs]
int miTiempo = 0;                                   // varialbe para indicar el tiempo [1 seg]

//********************************************************************************************************************
// **** Metodos ***
//********************************************************************************************************************

void GPIO(void){ // configuracion inicial
    // Habilitar los perifericos
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);    // LED INDICADORES
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);    // DISPLAY
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);    // LDR
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);    // CONMUTADOR DISPLAY
    // para desblockear el PF0 SW2
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R = 0x0f;

    // definir entradas o salidas
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0); // BTN
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6  | GPIO_PIN_7 ); // ldr

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); // leds
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2  | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6); // display
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);    // conmutador

    // configuraciones pullUp o pullDown [ruidos]
    GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6  | GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // puetos, pines a utilizar, corrientes, PULLUP O PULLDOWN WPU|WPD
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // RESET
}

void Delay2(void){                  // tiempo para la multiplexacion.
    unsigned long volatile time;
    time = 727240*200/91000;        // 0.1 ms
    while(time){
        time--;
    }
}

void Display(void){     // funcion para la muestra del display.
    int UNI,DEC;
    // **realizar el split
    DEC = CONT/10;      // descenas
    UNI = CONT%10;      // unidades.

    //GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 , 1); // apagados
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, numbers[DEC]); // enviar decenas
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 , 3); // habilitar decenas.
    Delay2();           // tiempo

    //GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 , 1); // apagados
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, numbers[UNI]); // envia unidades
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 , 0); // habilitar. unidad
    Delay2();           // tiempo

}

void conteoInicio(void){

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, 48); // 3
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 , 0); // habilitar. unidad
    SysCtlDelay(80000000/3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, 36); // 2
    SysCtlDelay(80000000/3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, 121); // 1
    SysCtlDelay(80000000/3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, 63); // gion.

}

void conteo(void){      // para mostrar en el display contador.
    if(CONT <= 98){
       CONT++;
       before = 1;
    }
    else{
       CONT = 0;
    }
}

void moduloUno(void){           //** modulo UNO-LDR.
    int uno = 0;
    while(pos1 <  miTiempo){                                                                    // ciclo de aprox 1s. para detectar la senyal.
        moduloLDR = GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);  // lectura del modulo fisio ldr.

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 2);    // muestra el led indicador del blanco.
        pos1++;                                                                                 // pos1 incrementa en uno.

        if(moduloLDR == 224 && before == 0 && uno == 0){                                                    // si es el primer modulo y begore = 0 ingresa.
           conteo();                                                                            // evalua el valor para enviarlo al display.
           uno++;
        }
        if(moduloLDR == 240){                                                                   // si todos los modulos dan 1.
           before = 0;                                                                          // variable before regresa a 0 para ya no aceptar valores.
        }

        Display();                                                                              // muestra el valor en el display.
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 0);        // se apaga el indicador del blanco.
    pos1 = 0;                                                                                   // variable pas1 regresa a 0.
}

void moduloDos(void){           //** modulo DOS-LDR.
    int uno = 0;
    while(pos2 < miTiempo){
        moduloLDR = GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 4);
        pos2++;

        if(moduloLDR == 208 && before == 0 && uno == 0){ // MOD 2
           conteo();
           uno++;
        }
        if(moduloLDR == 240){
           before = 0;
        }

        Display();
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 0);
    pos2 = 0;
}

void moduloTres(void){          //** modulo TRES-LDR.
    int uno = 0;
    while(pos3 < miTiempo){
        moduloLDR = GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 8);
        pos3++;

        if(moduloLDR == 176 && before == 0 && uno == 0){ // MOD 3
           conteo();
           uno++;
        }
        if(moduloLDR == 240){
           before = 0;
        }

        Display();
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 0);
    pos3 = 0;
}

void moduloCuatro(void){            //** modulo CUATRO-LDR.
    int uno = 0;
    while(pos4 < miTiempo){
        moduloLDR = GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 16);
        pos4++;

        if(moduloLDR == 112 && before == 0 && uno == 0){ // MOD 4
           conteo();
           uno++;
        }
        if(moduloLDR == 240){
           before = 0;
        }

        Display();
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, 0);
    pos4 = 0;
}

void secuenciaUno(void){            // secuencia UNO para el tiro al blanco.
    int j;                          // se declara la variable j.
    for(j = 0; j < 3; j++){         // ciclo for de 3 ciclos. para completar los 12.
        moduloUno();                // se llama al moduloUno.
        moduloDos();                // se llama al moduloDos.
        moduloTres();               // se llama al moduloTres.
        moduloCuatro();             // se llama al moduloCuatro.
    }
}

void secuenciaDos(void){            // secuencia DOS para el tiro al blanco.
    int j;
    for(j = 0; j < 3; j++){
        moduloTres();
        moduloDos();
        moduloUno();
        moduloCuatro();
    }
}

void secuenciaTres(void){           // secuencia TRES para el tiro al blanco.
    int j;
    for(j = 0; j < 3; j++){
        moduloCuatro();
        moduloUno();
        moduloTres();
        moduloDos();
    }
}

int main (void){
    // *** configuracion del reloj del dispositivo ***
    SysCtlClockSet(SYSCTL_OSC_MAIN| SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_SYSDIV_5); // configurando el proyecto a 40 MHz
    GPIO();                                                     // configuracion principal puertos.
    bool ciclo = true;

    conteoInicio();                                             // se llama al metodo para la cuenta regresiva. 3>>2>>1.
    miTiempo = 1000;                                            // aprox 1 segundo.
    CONT = 0;                                                   // Se inicia las variables con valor 0.
    pos1 = 0;
    pos2 = 0;
    pos3 = 0;
    pos4 = 0;


    while(true){
        // *** secuencia uno
        secuenciaUno();                                             // se llama al metodo secuenciUno.
        int espera;                                                 // al finalizar la secuencia y se espera 3-4 seg aprox.
        for(espera = 0; espera <= 4000; espera++){                  // se muestra el valor acumulado en el display.
            Display();
        }

        // *** secuencia dos
        secuenciaDos();
        for(espera = 0; espera <= 4000; espera++){
            Display();
        }
        // *** secuencia tres.
        secuenciaTres();
        ciclo = true;
        while(ciclo){                                                   // al finalizar la secuencia el ciclo muestra el display con el valor final.
            Display();
            int btn = 0;
            btn = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
            if(btn != 1){
                conteoInicio();
                miTiempo = 1000;
                CONT = 0;
                pos1 = 0;
                pos2 = 0;
                pos3 = 0;
                pos4 = 0;
                ciclo = false;
            }
        }
    }


// fallo btn-inicio solo al final -- implementar reset.  |interrupciones|
}
