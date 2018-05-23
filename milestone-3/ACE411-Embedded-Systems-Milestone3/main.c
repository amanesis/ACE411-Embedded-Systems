/*

Embedded Systems
AVR STUDIO 7

Reading the sensor and showing the 
measurements in the terminal.

The communication is achieved by 
the use of USART_RS232 and I2C protocols

We defined register map of MPU6050 (MPU6050_res_define.h)

*/
#define F_CPU 8000000UL									/* Define CPU clock Frequency: 8MHz */
#include <avr/io.h>										/* Include Libraries and Defines*/
#include <util/delay.h>	
#include <avr/interrupt.h>								
#include <inttypes.h>									
#include <stdlib.h>										
#include <stdio.h>										
#include "MPU6050_res_define.h"							
#include "I2C_Master_H_file.h"							
#include "USART_RS232_H_file.h"							

float Acc_x,Acc_y,Acc_z,Temperature,Gyro_x,Gyro_y,Gyro_z;

void MPU6050_Init()										/* Gyro initialization function */
{
	_delay_ms(150);										
	I2C_Start_Wait(0xD0);								/* Start with device write address */
	I2C_Write(SMPLRT_DIV);								/* Write to sample rate register */
	I2C_Write(0x07);									/* 1KHz sample rate */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(PWR_MGMT_1);								/* Write to power management register */
	I2C_Write(0x01);									/* X axis gyroscope reference frequency */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(CONFIG);									/* Write to Configuration register */
	I2C_Write(0x00);									/* Fs = 8KHz */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(GYRO_CONFIG);								/* Write to Gyro configuration register */
	I2C_Write(0x18);									
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(INT_ENABLE);								/* Write to interrupt enable register */
	I2C_Write(0x01);
	I2C_Stop();
}

void MPU_Start_Loc()
{
	I2C_Start_Wait(0xD0);								/* I2C start with device write address */
	I2C_Write(ACCEL_XOUT_H);							/* Write start location address from where to read */ 
	I2C_Repeated_Start(0xD1);							/* I2C start with device read address */
}

void Read_RawValue()
{
	MPU_Start_Loc();									/* Read Gyro values */
	Acc_x = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Acc_y = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Acc_z = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_x = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_y = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_z = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Nack());
	I2C_Stop();
}

//initialize interrupts
void initInterrupts(){
	
	cli(); //disable interrupts while initializing
	
	GICR = (1<<INT0) ; //Set Bit6 of GICR to unmask INT0 interrupt. (GICR = 0x40)
	MCUCR |= 1<<ISC01 | 1<<ISC00; //Configuring MCUCR for rising edge interrupt for INT0
	
	GICR = (1<<INT1);  //Set Bit6 of GICR to unmask INT1 interrupt.
	MCUCR |= 1<<ISC11 | 1<<ISC10; //Configuring MCUCR for rising edge interrupt for INT1

	sei();   //Enable Global Interrupts
}

// interrupt service routines
ISR(INT0_vect) //for int0
{
	PORTB=0x00;
	_delay_ms(1000);
}

ISR(INT1_vect) //for int1
{
	
}


int main()
{
	//Configure TIMER1
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);		 	//NON Inverted PWM
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(0<<CS11)|(1<<CS10);	//PRESCALER=1 MODE 14(FAST PWM)

	ICR1=19999;	//fPWM=50Hz (Period = 20ms Standard).

	DDRD=0XFF;	//PWM Pins as Out
	PORTD &= ~(1<<2);
	PORTD &= ~(1<<3);
	DDRB=0xFF;
	PORTB=0xFF;
	
	char buffer[20], float_[10];
	float tmpxa,tmpya,tmpza;
	float Xa,Ya,Za;
	//float tmpxg=0,tmpyg=0,tmpzg=0;
	float Xg=0,Yg=0,Zg=0;
	
	initInterrupts();
	I2C_Init();											/* Initialize I2C */
	MPU6050_Init();										/* Initialize MPU6050 */
	USART_Init(9600);									/* Initialize USART with 9600 baud rate */
	
	Read_RawValue();
	
	tmpxa = Acc_x/16384.0;
	tmpya = Acc_y/16384.0;
	tmpza = Acc_z/16384.0;
	
	//tmpxg = Gyro_x/131;
	//tmpyg = Gyro_y/131;
	//tmpzg = Gyro_z/131;
	
	while(1)
	{
		//sei(); 

		//PORTB=0x00;
		//_delay_ms(200);
		Read_RawValue();
		
		/*
		Divide raw value by sensitivity scale factor to get real values
		*/
	
		Xa = Acc_x/16384.0;// - tmpxa;
		Ya = Acc_y/16384.0;// - tmpya;
		Za = Acc_z/16384.0;// - tmpza;
		
		Xg = Gyro_x/131;// - tmpxg;
		Yg = Gyro_y/131;// - tmpyg;
		Zg = Gyro_z/131;// - tmpzg;

		
		//Take values in buffer to send all parameters over USART
		
	
		dtostrf( Xa, 3, 2, float_ );
		sprintf(buffer," Ax = %s g\t",float_);
		USART_SendString(buffer);

		dtostrf( Ya, 3, 2, float_ );
		sprintf(buffer," Ay = %s g\t",float_);
		USART_SendString(buffer);
	
		dtostrf( Za, 3, 2, float_ );
		sprintf(buffer," Az = %s g\t",float_);
		USART_SendString(buffer);

		dtostrf( Xg, 3, 2, float_ );
		sprintf(buffer," Gx = %s%c/s\t",float_,0xF8);
		USART_SendString(buffer);

		dtostrf( Yg, 3, 2, float_ );
		sprintf(buffer," Gy = %s%c/s\t",float_,0xF8);
		USART_SendString(buffer);
	
		dtostrf( Zg, 3, 2, float_ );
		sprintf(buffer," Gz = %s%c/s\r\n",float_,0xF8);
		USART_SendString(buffer);
		
		
		
		//OCR1A=255;	//45 degree
		//_delay_ms(100);

		//OCR1A=1050;	//90 degree
		//_delay_ms(100);

		//OCR1A=1950;	//135 degree
		//_delay_ms(20);

		//OCR1A=2400;	//180 degree
		//_delay_ms(20);
		
		
		//while(Acc_z/16384.0<0.9 && Acc_y/16384.0>0){
		
		//OCR1A=200;	//45 degree
		
		//PORTB=0x00;
		//Read_RawValue();
		//Xa = Acc_x/16384.0;// - tmpxa;
		//Ya = Acc_y/16384.0;// - tmpya;
		//Za = Acc_z/16384.0;// - tmpza;
		
		//Xg = Gyro_x/131;// - tmpxg;
		//Yg = Gyro_y/131;// - tmpyg;
		//Zg = Gyro_z/131;// - tmpzg;
		//}
		
		while(Ya<tmpya+0.3 && Ya<0){
			PORTB=0x01;
			OCR1A=10;
			_delay_ms(1);
			
			Read_RawValue();
			Xa = Acc_x/16384.0;// - tmpxa;
			Ya = Acc_y/16384.0;// - tmpya;
			Za = Acc_z/16384.0;// - tmpza;
		}
		
		while(Ya>tmpya+0.3 && Ya>0){
			PORTB=0x40;
			OCR1A=1340;
			_delay_ms(1);
			
			Read_RawValue();
			Xa = Acc_x/16384.0;// - tmpxa;
			Ya = Acc_y/16384.0;// - tmpya;
			Za = Acc_z/16384.0;// - tmpza;

		}
		
		/*while(Ya>-0.2 && Ya<0){
			OCR1A=255;	//45 degree
			_delay_ms(1);
			PORTB=0x00;
			Read_RawValue();
			Xa = Acc_x/16384.0;// - tmpxa;
			Ya = Acc_y/16384.0;// - tmpya;
			Za = Acc_z/16384.0;// - tmpza;
			
			//Xg = Gyro_x/131;// - tmpxg;
			//Yg = Gyro_y/131;// - tmpyg;
			//Zg = Gyro_z/131;// - tmpzg;
		}*/
		
		OCR1A=1320;	
		_delay_ms(1);
		OCR1A=1050;	//90 degree
		_delay_ms(1);
		PORTB=0xFF;
		
		Read_RawValue();
		Xa = Acc_x/16384.0;// - tmpxa;
		Ya = Acc_y/16384.0;// - tmpya;
		Za = Acc_z/16384.0;// - tmpza;
		
		Xg = Gyro_x/131;// - tmpxg;
		Yg = Gyro_y/131;// - tmpyg;
		Zg = Gyro_z/131;// - tmpzg;
		
	}
}
