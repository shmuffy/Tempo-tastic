# ♭ **Tempo-tastic** ♮

## Introduction
Inspired by games like Guitar Hero, Dance Dance Revolution, and Friday Night Funkin. The game is designed to test a player's rhythmic timing skills. Tempo-tastic challenges players to press buttons in sync with music to match the rhythm. Music is played through the piezo-electric passive buzzer when you press start button and when pressed again turns the game off. A 1-Digit-7-Segment Display shows your current rank as in Guitar Hero. The game offers a fun and engaging way to improve hand-eye coordination and rhythmic testing. 

## Table of Contents
 - [Hardware Components](#hardware-components)
 - [Functionality](#functionality)
 - [Challenges](#challenges)

## Hardware Components 
- **Microcontroller**
  - Arduino Uno R3
  - ATmega 2560 (Modifiable and adds more preffered complexity)
  - STM32F7 (Modifiable and adds much more preffered complexity)
- **Inputs**
  - Custom RGBY Buttons
  - 74HC595N Shift Register
- **Outputs**
  - 128 x 128 LCD Display
    - Respectively displays square colors according to the button colors(RGBY)
  - 1-Digit-7-Segment LED
    - Displays (starting | current | ending) Rank
  - Green/RED LEDs
    - (Green = Win | Red = Lose)
  - Piezo Passive Buzzer

# Functionality
* Start the game -> Press Black Button
  * Song plays -> Game timer starts counting down
    * Press accordingly and timer will start increasing
    * Successful hits will accumulate points and display a Rank from (A -> D)
    * If you maintain a Rank of (A || B) after the song ends you win!
      * Otherwise you lose...
    * Result = Result ? Win : Lose
* To increase difficulty you can tweak the frequency in the music.h file.

# Challenges
* Using ATmega2560 would've been a much better choice than Arduino UNO R3 as the piezo-electric buzzer requires specific pins (pin 9 and 10) for Timer1 yet occupied by LCD screen and the shift register.
* Broken hardware results in replacing stuff frequently. 4-Digit-7-Segment Display had a faulty G segment section.
* Previous shift register's data pin ended up defective therefore had to get a replacement.
* LED Matrix had to be changed to 128x128 LCD due to malfunctions. 
