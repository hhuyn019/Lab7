#include <avr/io.h>
#include "io.h"
#define inputs (~PINA & 0x03)
unsigned char count = 0x00;
unsigned char c = '0';

// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;

//Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks

void TimerOn()
{
	// AVR timer/counter controller register TCCR1
	// bit3 = 0; CTC mode (clear timer on compare)
	// bit2bit1bit0 = 011: prescaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 / 64 = 125,000 ticks/s
	// THUS, TCNT! register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	
	//AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT! == OCR1A
	// We want a 1 ms tick. 0.001S * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// ! ms has passed. Thus, we compare to 125.
	OCR1A = 125;
	// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff()
{
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR()
{
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect)
{
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0)
	{
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
void Tick();
enum States {BEGIN, INIT, INC, DEC, WAIT, RESET}state;

int main(void)
{
    DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00; //LCD data lines
	DDRD = 0xFF; PORTD = 0x00; //LCD control lines
	unsigned char button1 = ~PINA & 0x01;
	unsigned char button2 = ~PINA & 0x02;
	
	//Initializes the LCD display
	LCD_init();
	TimerSet(500);
	TimerOn();
	state = BEGIN;
    while(1)
    {
	    
	//LCD_WriteData('0');
	Tick();
        while (!TimerFlag);
        TimerFlag = 0;
	//LCD_WriteData('1');
    }    
}

void Tick()
{
	switch(state) 
	{ 
		case BEGIN:
		{
			LCD_Cursor(1);
			LCD_WriteData(count + '0');
			state = INIT; 
			break;
		}
		
		case INIT:
		{
			if(inputs == 0x01)
			{
				state = INC; break;
			}
			else if(inputs == 0x02)
			{
				state = DEC; break;
			}
			else if(inputs == 0x03)
			{
				state = RESET; break;
			}
			else
			{
				state = INIT; break;
			}
		}
		
		case INC:
		{
			state = WAIT; break;
		}
		
		case DEC:
		{
			state = WAIT; break;
		}
		
		case WAIT:
		{
			if(inputs == 0x01)
			{
				state = INC; break;
			}
			else if(inputs == 0x02)
			{
				state = DEC; break;
			}
			else if(inputs == 0x03)
			{
				state = RESET; break;
			}
			else if(inputs == 0x00)
			{
				state = INIT; break;
			}
			else 
			{
				state = WAIT; break;
			}
		}
		
		case RESET:
		{
			if(inputs == 0x00)
			{
				state = INIT; break;
			}
			else
			{
				state = RESET; break;
			}
		}
		
		default:
			break;
	}
	
	switch(state) 
	{
		case BEGIN:
			break;
			
		case INIT:
			break;
			
		case INC:
		{
			if(count >= 9)
			{
				count = 9;
			}
			else
			{
				++count;
			}
			LCD_Cursor(1);
			LCD_WriteData(count + '0');
			break;
		}
		
		case DEC:
		{
			if(count <= 0)
			{
				count = 0;
			}
			else
			{
				--count;
			}
			LCD_Cursor(1);
			LCD_WriteData(count + '0');
			break;
		}
		
		case WAIT:
			break;
			
		case RESET:
		{
			count = 0; 
			LCD_Cursor(1);
			LCD_WriteData(count + '0');
			break;
		}
	}
}
