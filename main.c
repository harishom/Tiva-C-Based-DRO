/* Copyright 2013 Harish Mehta.  
  *  
  *  
  *  
  * This project is free software; you can redistribute it and/or  
  * modify it under the terms of the GNU Lesser General Public  
  * License as published by the Free Software Foundation; either  
  * version 2.1 of the License, or (at your option) any later version.  
  *  
  * This library is distributed in the hope that it will be useful,  
  * but WITHOUT ANY WARRANTY; without even the implied warranty of  
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
  * Lesser General Public License for more details.  
  *  
  * You should have received a copy of the GNU Lesser General Public  
  * License along with this library; if not, write to the Free Software  
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  
  * USA.  
  */  
 // Author: Harish Mehta  
 // Date: 08-20-2013  
 //  
 // Discription: This program reads three iGaging "remote DRO" digital scales  
 // and sends the position to the host via hardware UART  
 //  
 #include <stdint.h>  
 #include <stdbool.h>  
 #include "inc/hw_ints.h"  
 #include "inc/hw_memmap.h"  
 #include "inc/hw_types.h"  
 #include "driverlib/sysctl.h"  
 #include "driverlib/interrupt.h"  
 #include "driverlib/gpio.h"  
 #include "driverlib/timer.h"  
 #include "driverlib/uart.h"  
 #include "driverlib/pin_map.h"  
 #include "driverlib/fpu.h"  
 #define           short_interval           2000                         //Generates 10Khz Clock signal for DROs  
 #define           long_interval            400000  
 #ifdef DEBUG  
 void  
 __error__(char *pcFilename, uint32_t ui32Line)  
 {  
 }  
 #endif  
 volatile unsigned char rxPacket;  
 volatile char rxBuf[15];  
 volatile unsigned long x_bit_values = 0;  
 volatile unsigned long y_bit_values = 0;  
 volatile unsigned long z_bit_values = 0;  
 unsigned char pause = 0;  
 unsigned int count = 0;  
 char buffer[60];  
 unsigned short i;  
 volatile unsigned int red_blink_counter = 0;  
 int main(void)  
 {  
           FPULazyStackingEnable();  
           FPUEnable();  
           SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);            // System clock set to 40 Mhz.  
        initPeripherals();  
           initWTIMER0();  
           pause =1;  
           IntMasterEnable();  
           while(1);                               // Interrupts handle all data.  
 }  
 void initWTIMER0()  
 {  
                TimerConfigure(WTIMER0_BASE, TIMER_CFG_PERIODIC);  
                TimerLoadSet(WTIMER0_BASE, TIMER_A, short_interval);  
                IntRegister(INT_WTIMER0A, WTIMER0IntHandler);  
                IntEnable(INT_WTIMER0A);  
                TimerIntEnable(WTIMER0_BASE, TIMER_TIMA_TIMEOUT);  
                TimerEnable(WTIMER0_BASE, TIMER_A);  
             GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);                    //Turn on blue LED  
 }  
 void WTIMER0IntHandler(void)  
 {  
           TimerIntClear(WTIMER0_BASE, TIMER_TIMA_TIMEOUT);  
           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);                                             //Turn off blue LED  
        if (pause)  
        {  
              pause = 0;  
              TimerLoadSet(WTIMER0_BASE, TIMER_A, short_interval);  
              GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2, 0 );  //P1OUT &= ~clock_out;  
              return;  
        }  
        if(!GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_2))                                          //P1OUT ^= clock_out;  
            GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2 );  
        else GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2, 0 );  
        if ((GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_2)) ==0)          //we are on the "low" portion of the out pulse  
        {  
             if (count <= 21)  
             {  
                GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2, 0 );    //P1OUT &= ~clock_out;  
                unsigned char on;  
                on = ((uint8_t)GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_3));    //on = (P2IN & x_axis_in) == x_axis_in;  
                x_bit_values |= ((long) (on ? 1 : 0) << count);  
                on = ((uint8_t)GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_4));           //on = (P2IN & y_axis_in) == y_axis_in;  
                y_bit_values |= ((long) (on ? 1 : 0) << count);  
                on = ((uint8_t)GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_5));                // on = (P2IN & z_axis_in) == z_axis_in;  
                z_bit_values |= ((long) (on ? 1 : 0) << count);  
             }  
            count++;  
             }  
        if (count > 21)  
        {  
                       TimerDisable(WTIMER0_BASE, TIMER_A);    //TACCTL0 = 0x00;               //stop the timer  
                        if (x_bit_values & ((long) 1 << 20)) {  
                  x_bit_values |= ((long) 0x7ff << 21);  
                         }  
                         if (y_bit_values & ((long) 1 << 20)) {  
                  y_bit_values |= ((long) 0x7ff << 21);  
                         }  
                         if (z_bit_values & ((long) 1 << 20)) {  
                  z_bit_values |= ((long) 0x7ff << 21);  
                         }  
                    if (++red_blink_counter > 20)  
                    {  
             if(!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2))  
               GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2 );  
             else GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, 0 );  
             red_blink_counter = 0;  
             if (x_bit_values == 0 && y_bit_values == 0 && z_bit_values == 0) {  
                    GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1 );  
                } else {  
                    GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_1, 0 );  
                }  
                         }  
                         FinishRead();  
                         x_bit_values = 0;  
                         y_bit_values = 0;  
                         z_bit_values = 0;  
                         count = 0;  
                         TimerEnable(WTIMER0_BASE, TIMER_A);  
           }  
 }  
 void FinishRead()  
 {  
          char temp[25];  
          char * c;  
          int position = 0;  
          memset(buffer, '\0', 60);  
          memset(temp, '\0', 25);  
     for(i=0;i<15;i++)  
     {  
          buffer[position++] = temp[i];  
          position++;  
     }  
     buffer[position++] = ',';  
       //X axis  
        memset(temp, '\0', 25);  
     itoa(x_bit_values, temp);  
     c = temp;  
     buffer[position++] = 'x';  
     while (*c != '\0') {  
         buffer[position++] = *c;  
         c++;  
     }  
     buffer[position++] = ',';  
     //Y axis  
        memset(temp, '\0', 25);  
     itoa(y_bit_values, temp);  
     c = temp;  
     buffer[position++] = 'y';  
     while (*c != '\0') {  
         buffer[position++] = *c;  
         c++;  
     }  
     buffer[position++] = ',';  
     //Z axis  
        memset(temp, '\0', 25);  
     itoa(z_bit_values, temp);  
     c = temp;  
     buffer[position++] = 'z';  
     while (*c != '\0') {  
         buffer[position++] = *c;  
         c++;  
     }  
     buffer[position++] = '\0';  
     buffer[position++] = '\n';  
     int idx;  
     for(idx=0; idx<sizeof buffer; idx++)  
          UARTCharPut(UART0_BASE, buffer[idx]);  
     pause = 1;  
     TimerLoadSet(WTIMER0_BASE, TIMER_A, long_interval);  
 }  
 static char *i2a(unsigned i, char *a, unsigned r) {  
     if (i / r > 0)  
         a = i2a(i / r, a, r);  
     *a = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % r];  
     return a + 1;  
 }  
 char *itoa(int i, char *a) {  
     int r = 10;  
     if (i < 0) {  
         *a = '-';  
         *i2a(-(unsigned) i, a + 1, r) = 0;  
     } else  
         *i2a(i, a, r) = 0;  
     return a;  
 }  
 void initPeripherals()  
 {  
        SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);  
        SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);  
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  
        // Enable port PA1 for UART0 U0TX  
        //  
        GPIOPinConfigure(GPIO_PA1_U0TX);  
        GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1);  
        //  
        // Enable port PA0 for UART0 U0RX  
        //  
        GPIOPinConfigure(GPIO_PA0_U0RX);  
        GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0);  
        GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);    //Handle LED outputs  
        //DRO Clock Output  
        GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2 );  
        //DRO Input data  
        GPIODirModeSet(GPIO_PORTD_BASE, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_DIR_MODE_IN);  
        GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);  
        UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,     (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |     UART_CONFIG_PAR_NONE));  
        UARTEnable(UART0_BASE);  
        UARTFIFOEnable(UART0_BASE);  
        UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX7_8, UART_FIFO_RX7_8);  
        UARTIntClear(UART0_BASE, UART_INT_RX);  
 }  
