#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include <avr/io.h>
#include <util/delay.h>
#include "LCD.h"
#include "spiAVR.h"
#include <stdio.h>  //debugging purposes
#include <stdlib.h>
#include "music.h"

// #define CLOCK_PRESCALE(n)   (CLKPR = 0x80, CLKPR = (n))
// #define BUZZER_CONFIG       (DDRB |= (1<<4)) 
// #define BUZZ_ON             (PORTB |= (1<<4))
// #define BUZZ_OFF            (PORTB &= ~(1<<4))

//fPWM = fclk / (N * (1+TOP))

#define NUM_TASKS 3 //TODO: Change to the number of tasks being used

// #define DATA_PIN 4
// #define SHCLK 3
// #define STCLK 0

#define SRCLK 3
#define STCLK 0
#define DATA_PIN 2

volatile unsigned char cnt = 1;
unsigned char curr_Row, rowDat, colDat, pattern, row, col;
unsigned int curr_note = 0;
unsigned int start;
unsigned int R, B, G, Y = 0;

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long TASK1_PERIOD = 100;
const unsigned long TASK2_PERIOD = 50;
const unsigned long TASK_GAME = 500;
const unsigned long GCD_PERIOD = 50;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks

const char letters[] = {'A', 'b', 'C', 'd', 'E', 'F'};
const char letterRank[6] = {
  0b01110111, // A
  0b01111100, // b
  0b00111001, // C
  0b01011110, // d
  0b01111001, // E
  0b01110001  // F
};

// void aaashiftOut(char data) {
    
//   for (int i = 0; i < 7; i++) {
//     serial_println(GetBit(data, i));
//     // Set the data pin to the correct bit
//     PORTB = SetBit(PORTB, DATA_PIN, GetBit(data, i));

//     PORTD = SetBit(PORTD, SHCLK, 1); //Shift (SH)
//     _delay_us(10);
//     PORTD = SetBit(PORTD, SHCLK, 0); //Shift (SH)
//     _delay_us(10);
//   }
//   PORTB = SetBit(PORTB, STCLK, 1); //Store (ST)
//   _delay_us(10);
//   PORTB = SetBit(PORTB, STCLK, 0); //Store (ST)
//   _delay_us(10);
// }
void shiftOut(char data) {
    
  for (int i = 7; i >= 0; i--) {
    //serial_println(GetBit(data, i));
    PORTB = SetBit(PORTB, DATA_PIN, GetBit(data, i));
    PORTD = SetBit(PORTD, SRCLK, 1);
    _delay_us(1);
    PORTD = SetBit(PORTD, SRCLK, 0);
  }
  //PORTB = SetBit(PORTB, SER, Q7); // For ult led3

  PORTB = SetBit(PORTB, STCLK, 1);
  _delay_us(1);
  PORTB = SetBit(PORTB, STCLK, 0);
}
// void displayLetter(int index) {
//   // Ensure the index is valid

//   // Get the segment code for the letter
//   char segments = letterRank[index];

//   // Shift out the bits
//   shiftOut(segments);
// }

void Send_Command(char data) {
  PORTB = SetBit(PORTB, PIN_SS, 0);
  PORTD = SetBit(PORTD, 7, 0);
  //serial_println("start send");
  SPI_SEND(data);
  //serial_println("end send");
}
void Send_Data(char data) {
  PORTB = SetBit(PORTB, PIN_SS, 0);
  PORTD = SetBit(PORTD, 7, 1);
  SPI_SEND(data);
}
void ST7735_init() {
  // Reset
  PORTD = SetBit(PORTD, 2, 0);
  //shiftOut(0x00);
  _delay_ms(200);
  PORTD = SetBit(PORTD, 2, 1);
  //shiftOut(0x80);
  _delay_ms(200);

  // Init
  Send_Command(0x01); // SW RESET
  _delay_ms(150);
  Send_Command(0x11); // SLPOUT
  _delay_ms(200);
  Send_Command(0x0C); // COLMOD
  Send_Data(0x06); //for 18 bit color mode. You can pick any color mode you want
  _delay_ms(10);
  Send_Command(0x29); // DISPON
  _delay_ms(200);
}

enum GAME_STATES{WAIT, PRESS, PLAY, PRESS_Wait};
enum GAME_BUTTONS{GAME_IDLE, GAME_PRESS};
enum GAME_SCORES{HIGH, LOW};

void SendCommand(char command) {

}
void SendData(char data) {

}
void Clear_Screen() {
  Send_Command(0x2A);
  Send_Data(0x00);
  Send_Data(0x00); //48
  Send_Data(0x00);
  Send_Data(0xFF); //80

  Send_Command(0x2B);
  Send_Data(0x00);
  Send_Data(0x00); //48
  Send_Data(0x00);
  Send_Data(0xFF); //80

  Send_Command(0x2C);
  for (unsigned int i = 0; i < 50000; i++) { // for each pixel?
    Send_Data(33);
    Send_Data(16);
    Send_Data(13);
  }
}
void Draw_Key(char c, char y) {
    char x = c * 25 + 10;
    Send_Command(0x2A);
    Send_Data(0);
    Send_Data(x); //48
    Send_Data(0);
    Send_Data(x+15); //80

    Send_Command(0x2B);
    Send_Data(0);
    Send_Data(y); //48
    Send_Data(0);
    Send_Data(y+15); //80

    Send_Command(0x2C);
    for (unsigned char i = 0; i < 224; i++) { // for each pixel?
        if (c == 0) {
            Send_Data(0);
            Send_Data(0);
            Send_Data(255);
        } else if (c == 1) {
            Send_Data(255);
            Send_Data(0);
            Send_Data(0);
        } else if (c == 2) {
            Send_Data(0);
            Send_Data(255);
            Send_Data(0);
        } else if (c == 3) {
            Send_Data(23);
            Send_Data(208);
            Send_Data(255);
        }
    }
}
void Clear_Key(char c, char y) {
    char x = c * 25 + 10;
    Send_Command(0x2A);
    Send_Data(0);
    Send_Data(x); //48
    Send_Data(0);
    Send_Data(x+15); //80

    Send_Command(0x2B);
    Send_Data(0);
    Send_Data(y); //48
    Send_Data(0);
    Send_Data(y+15); //80

    Send_Command(0x2C);
    for (unsigned int i = 0; i < 224; i++) { // for each pixel?
        Send_Data(33);
        Send_Data(16);
        Send_Data(13);
    }
}



// int TickFct_SR(int state) {
//     serial_println(GetBit(PINC, 4));
//     switch(state) {
//         case STOP:
//             if (start == 1) {state = RUN;}
//             else {state = STOP;}
//             break;
//         case RUN:
            
//             break;
//     }
//     switch(state) {
//         case STOP:
//             state = RUN;
//             break;
//         case RUN:
//             break;
//     }
//     return state;
// }

int Tick_Buttons(int state) {
    //serial_println("here");
    switch(state) {
        case GAME_IDLE:
            if (GetBit(PINC, 3) || GetBit(PINC, 2) || GetBit(PINC, 1) || GetBit(PINC, 0)) {
                state = GAME_PRESS;
            }
            else {state = GAME_IDLE;}
            break;
        
        case GAME_PRESS:
            if (!GetBit(PINC, 0))      {Y = 1; state = GAME_IDLE;}
            else if (!GetBit(PINC, 1)) {G = 1; state = GAME_IDLE;}
            else if (!GetBit(PINC, 2)) {B = 1; state = GAME_IDLE;}
            else if (!GetBit(PINC, 3)) {R = 1; state = GAME_IDLE;}
            else {state = GAME_IDLE;}
            break;
    }
    switch (state) {
        case GAME_IDLE:
            break;
        case GAME_PRESS:
            break;
    }
    return state;
}

int Tick_Game(int state) {
    static unsigned int i = 1;
    static unsigned int j = 0;
    static unsigned int delay = 0;
    //static unsigned int curr_note = 0;
    static unsigned int k = 3;
    //static unsigned int rank = 13;
    switch(state) {
        case WAIT:
            if (GetBit(PINC, 4)) {state = PRESS;} //restart
            // else if (GetBit(PINC, 3) || GetBit(PINC, 2)|| GetBit(PINC, 1) || GetBit(PINC, 0)) {cnt = 0; state = PRESS;}
            break;
        case PRESS:
        //serial_println("Press");
            if (!GetBit(PINC, 4)) {start = 1; delay = 0; i = 0; j = 0; state = PLAY;}
            else {state = PRESS;}
            break;
        case PLAY:
        //serial_println("play");
            if (GetBit(PINC, 4)) {state = PRESS_Wait;} 
            else {state = PLAY;}
            break;
        case PRESS_Wait:
        //serial_println("press wait");
            if (!GetBit(PINC, 4)) {start = 0; state = WAIT;}
            else {state = PRESS_Wait;}
            break;
    }
    switch(state) { //button state actions
        case WAIT:
            OCR1B = 0;
            shiftOut(0x00);
            //shiftOut(letterRank[0]);
            //outNum(8);
            //shiftOut(letterRank[0]);
            break;
        case PRESS:
            break;
        case PLAY:
            serial_println("here3");
            delay = durations[i-1] / 4;
            curr_note = getCol(notes[i]);
                serial_println("here4");
                if (curr_note == 0) { //correct button is pressed
                    cnt++; 
                    //serial_println(cnt);
                    if (R || B || G || Y) {
                        //Increment score here
                        R = 0;
                        B = 0;
                        G = 0;
                        Y = 0;
                        Clear_Key(getCol(notes[i]), 60);
                        i++; 
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    else {
                        //Decrement score here if you want
                        cnt--;
                        Clear_Key(getCol(notes[i]), 60);
                        i++;
                        k--; 
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    cnt += cnt;
                } 
                else if (curr_note == 1) { //correct button is pressed
                    cnt++;
                    serial_println(cnt);
                    if (R || B || G || Y) {
                        //Increment score here
                        R = 0;
                        B = 0;
                        G = 0;
                        Y = 0;
                        Clear_Key(getCol(notes[i]), 60);
                        i++;
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    else {
                        //Decrement score here if you want
                        cnt--;
                        Clear_Key(getCol(notes[i]), 60);
                        i++;
                        k--; 
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    cnt += cnt;
                } 
                else if (curr_note == 2) { //correct button is pressed
                    cnt++; 
                    serial_println(cnt);
                    if (R || B || G || Y) {
                        //Increment score here
                        R = 0;
                        B = 0;
                        G = 0;
                        Y = 0;
                        Clear_Key(getCol(notes[i]), 60);
                        i++; 
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    else {
                        //Decrement score here if you want
                        cnt--;
                        Clear_Key(getCol(notes[i]), 60);
                        i++;
                        k--;  
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    cnt += cnt;
                } 
                else if (curr_note == 3) { //correct button is pressed
                    cnt++;
                    if (R || B || G || Y) {
                        //Increment score here
                        R = 0;
                        B = 0;
                        G = 0;
                        Y = 0;
                        Clear_Key(getCol(notes[i]), 60);
                        i++; 
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    else {
                        //Decrement score here if you want
                        cnt--;
                        Clear_Key(getCol(notes[i]), 60);
                        i++; 
                        k--;
                        j = 0;
                        Draw_Key(notes[i] > 0 ? getCol(notes[i]) : 4, 60);
                    }
                    cnt += cnt;
                }
                if (j >= delay) {
                OCR1B = 0;
                } else {
                    unsigned int icr = getICR(notes[i]);
                    ICR1 = icr;
                    OCR1B = notes[i-1] > 0 ? icr/2 : 0;
                }
                j++;
                if (i == SONG_LENGTH) i = 0;
            // else if (cnt == 6) {
            //     shiftOut(letterRank[0]);
            //     PORTD = SetBit(PORTD, 4, 1);
            //     start = 0;
            //     state = WAIT;
            // }
            break;
    }
    return state;
}

int Tick_Score(int state) {
    static unsigned int k = 3;
    switch(state) {
        case LOW:
            if (cnt > 0) {state = HIGH;}
            break;
        case HIGH:
            if (cnt == 6) {cnt = 0; state = LOW;}
            break;
    }
    switch(state) {
        case LOW:
            k--;
            break;
        case HIGH:
            if (cnt > 200) {k++; shiftOut(letterRank[0]); PORTD = SetBit(PORTD, 4, 1); start = 0; start = 0; Clear_Screen();}
            else if (cnt == 2) {k++; shiftOut(letterRank[3]);}
            else if (cnt == 3) {k++; shiftOut(letterRank[2]);}
            else if (cnt == 4) {k++; shiftOut(letterRank[1]);}
            else if (cnt == 0) {shiftOut(letterRank[3]); PORTD = SetBit(PORTD, 5, 1); PORTD = SetBit(PORTD, 4, 0); start = 0; Clear_Screen();}
            break;
    }
    return state;
}

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    DDRD = 0xff; PORTD = 0x00; 
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0x00; PORTC = 0x00;

    //13, 12, 11, 7, 6, 5
    serial_init(9600);
    ADC_init();   // initializes ADC
    SPI_INIT();
    ST7735_init();
    unsigned char i = 0;

    TCCR1A |= (1 << WGM11) | (1 << COM1B1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8

    ICR1 = 39999; //20ms pwm period

    // TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM0A1 sets it to channel A
    // TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
    //WGM11, WGM12, WGM13 set timer to fast pwm mode


    //TODO: Initialize tasks here
    // e.g. 
    tasks[i].period = TASK1_PERIOD;
    tasks[i].state = GAME_IDLE;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Buttons;
    i++;
    tasks[i].period = TASK_GAME;
    tasks[i].state = WAIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Game;
    i++;
    tasks[i].period = TASK2_PERIOD;
    tasks[i].state = LOW;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Score;
    i++;
    // tasks[i].period = TASK1_PERIOD;
    // tasks[i].state = LCD_INIT;
    // tasks[i].elapsedTime = tasks[i].period;
    // tasks[i].TickFct = &TickFct_LCD;
    
    Clear_Screen();
    serial_println("here2");

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        //PORTD = SetBit(PORTD, 4, 1);
        //shiftOut(letterRank[0]);
        // shiftOut(255,0);
        // update_leds(i);
        // j++;
        // if (j > 100) {
        //     i++;
        //     j = 0;
        // }
        // if (i > 7) i = 0;
    }

    return 0;
}