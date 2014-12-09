/*
* Neri Lemus		(860977196)
* 
* Final Project
* Lab Section 021
* Code for the Thermostat Microcontroller
* 
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/
#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 

#include "usart_ATmega1284.h"

// Templates for 7-seg LED Display
unsigned char zero  = 0x77;
unsigned char one   = 0x14;
unsigned char two   = 0xB3;
unsigned char three = 0xB6;
unsigned char four  = 0xD4;
unsigned char five  = 0xE6;
unsigned char six   = 0xE7;
unsigned char seven = 0x34;
unsigned char eight = 0xF7;
unsigned char nine  = 0xF6;

// Global variables
unsigned char desired_temp;
unsigned char first_digit;
unsigned char second_digit;
unsigned char curr_temp;
unsigned char fan; // 1 = AC on, 2 = AC off, 3 = Heater on, 4 = heater off
bool send_byte;

int cnt;

// SM States
enum receiveState {rec_init, receive} rec_state;
enum tempState {temp_init, temp_increase, temp_decrease} temp_state;
enum fanState {fan_init, AC_message, cool_room, Heat_message, heat_room} fan_state;
enum sendState 	{send_init, send} send_state;

// Helper methods
unsigned char num_to_sevseg(unsigned char input)
{
	switch(input)
	{
		case 0:
			return 0x77;
		case 1:
			return 0x14;
		case 2:
			return 0xB3;
		case 3:
			return 0xB6;
		case 4:
			return 0xD4;
		case 5:
			return 0xE6;
		case 6:
			return 0xE7;
		case 7:
			return 0x34;
		case 8:
			return 0xF7;
		case 9:
			return 0xF6;
		default:
			return 0x77;
	}
}

void transmit_data(volatile uint8_t *port, unsigned char data, unsigned char reg) 
{
	int i;
	for (i = 7; i >= 0 ; --i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		if(reg == 1){*port = 0x08;}
		else{*port = 0x20;}
		// set SER = next bit of data to be sent.
		*port |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		*port |= 0x04;
	}
	// set RCLK = 1. Rising edge copies data from the ?Shift? register to the ?Storage? register
	if(reg == 1){*port |= 0x02;}
	else{*port |= 0x10;}
	// clears all lines in preparation of a new transmission
	*port = 0x00;
}

// State Machines
void receive_SM()
{
	// SM Transitions
	switch(rec_state)
	{
		case rec_init:
			if (USART_HasReceived(0)) {rec_state = receive;}
			else rec_state = rec_init;
			break;
		case receive:
			rec_state = rec_init;
			break;
		default:
			break;
	}
	
	// SM Actions
	switch(rec_state)
	{
		case rec_init:
			break;
		case receive:
			desired_temp = USART_Receive(0);
			first_digit = desired_temp / 10;
			second_digit = desired_temp % 10;
			transmit_data(&PORTB, num_to_sevseg(first_digit), 1);
			transmit_data(&PORTB, num_to_sevseg(second_digit), 2);
			break;
		default:
			break;
	}
}

void temp_SM()
{
	// Actions
	switch(temp_state)
	{
		case temp_init:
			break;
		case temp_increase:
			curr_temp = curr_temp + 1;
			//transmit_data(&PORTC, curr_temp, 1);
			break;
		case temp_decrease:
			curr_temp = curr_temp - 1;
			//transmit_data(&PORTC, curr_temp, 1);
			break;
		default:
			break;
	}

	// Transitions
	switch(temp_state)
	{
		case temp_init:
			if(~PINA & 0x01)
				temp_state = temp_increase;
			else if(~PINA & 0x02)
				temp_state = temp_decrease;
			else
				break;
			break;
		case temp_increase:
			if(~PINA & 0x01)
				temp_state = temp_increase;
			else if(~PINA & 0x02)
				temp_state = temp_decrease;
			else
				temp_state = temp_init;
			break;
		case temp_decrease:
			if(~PINA & 0x01)
				temp_state = temp_increase;
			else if(~PINA & 0x02)
				temp_state = temp_decrease;
			else
				temp_state = temp_init;
			break;
		default:
			break;
	}
}

void send_tick ()
{
	// Actions
	switch(send_state)
	{
		case send_init:
			break;
		case send:
			USART_Send(fan, 0);
			break;
		default:
			break;
	}

	// Transitions
	switch(send_state)
	{
		case send_init:
			if (USART_IsSendReady(0) && send_byte)
				send_state = send;
			else 
				send_state = send_init;
			break;
		case send:
			while(!USART_HasTransmitted(0)){}
			send_state = send_init;
			send_byte = false;
			break;
		default:
			break;
	}
}

void fan_SM()
{
	// Actions
	switch(fan_state)
	{
		case fan_init:
			cnt = 0;
			fan = 0;
			break;
		case AC_message:
			fan = 1;
			send_byte = true;
			PORTA = PORTA | 0x04;
			break;
		case cool_room:
			if(cnt == 20)
			{
				curr_temp = curr_temp - 1;
				cnt = 0;
			}
			break;
		case Heat_message:
			fan = 3;
			send_byte = true;
			PORTA = PORTA | 0x08;
			break;
		case heat_room:
			if(cnt == 20)
			{
				curr_temp = curr_temp + 1;
				cnt = 0;
			}
			break;
		default:
			break;
	}

	// Transitions
	switch(fan_state)
	{
		case fan_init:
			if(curr_temp >=(desired_temp + 2))
				fan_state = AC_message;
			else if(curr_temp <= (desired_temp - 2))
				fan_state = Heat_message;
			else
				break;
			break;
		case AC_message:
			fan_state = cool_room;
			break;
		case cool_room:
			if(curr_temp > desired_temp)
				cnt = cnt + 1;
			else
			{
				fan = 2;
				send_byte = true;
				fan_state = fan_init;
				PORTA = PORTA & 0x03;
			}
			break;
		case Heat_message:
			fan_state = heat_room;
			break;
		case heat_room:
			if(curr_temp < desired_temp)
				cnt = cnt + 1;
			else
			{
				fan = 4;
				send_byte = true;
				fan_state = fan_init;
				PORTA = PORTA & 0x03;
			}
			break;
		default:
			break;
	}
}

// Tasks
void RecTask() 
{
	rec_state = rec_init;
	first_digit = 7;
	second_digit = 2;
	transmit_data(&PORTB, num_to_sevseg(first_digit), 1);
	transmit_data(&PORTB, num_to_sevseg(second_digit), 2);
	
	initUSART(0);
	for (;;) 
	{
		receive_SM();
		vTaskDelay(90);
	}
}

void TempTask()
{
	curr_temp = 72;
	desired_temp = 72;
	temp_state = temp_init;
	transmit_data(&PORTC, curr_temp, 1);
	for(;;)
	{
		temp_SM();
		transmit_data(&PORTC, curr_temp, 1);
		vTaskDelay(250);
	}
}

void SendTask() 
{
	send_state = send_init;
	send_byte = false;
	initUSART(0);
	for ( ; ; ) 
	{
		send_tick();
		vTaskDelay(300);
	}
}

void FanTask()
{
	fan_state = fan_init;
	cnt = 0;
	for(;;)
	{
		fan_SM();
		vTaskDelay(500);
	}
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	//xTaskCreate(LedSecTask, (signed portCHAR *)"LedSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(RecTask, 	(signed portCHAR *)"RecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL);
	xTaskCreate(SendTask, 		(signed portCHAR *)"SendTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL);
	xTaskCreate(TempTask,	(signed portCHAR *)"TempTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(FanTask,	(signed portCHAR *)"FanTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	

int main(void) 
{ 
   DDRA = 0xFC; PORTA=0x03;
   DDRB = 0xFF; PORTB=0x00;
   DDRC = 0xFF; PORTC=0x00;
   
   //Start Tasks  
   StartSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}