# STM32 Simon Says Memory Game

A hardware-based Simon Says memory game developed on the STM32F411RE microcontroller.  
The project is implemented in Embedded C using the STM32 HAL library and follows fundamental embedded systems design principles.

---

## Project Overview

The system generates a random LED sequence and visually presents it to the user.  
The user must replicate the exact sequence using physical push buttons.  

- Correct input advances the level.
- Incorrect input ends the game.
- The LCD displays level information and system messages.
- A buzzer provides audible feedback for start, success, and error states.
- The highest score is stored in internal Flash memory and preserved after reset.

---

## Technical Specifications

- **Microcontroller:** STM32F411RE  
- **Language:** Embedded C  
- **Development Environment:** STM32CubeIDE  
- **Peripheral Interface:** HAL GPIO  
- **Display:** 16x2 LCD (HD44780 compatible, 4-bit mode)  
- **Audio Output:** GPIO-driven buzzer  
- **Persistent Storage:** Internal Flash memory (High Score)

---

## System Architecture

### Game Logic
- A random sequence is generated using pseudo-random number logic.
- LEDs display the sequence with controlled timing intervals.
- Button inputs are validated against the generated sequence.
- Each successful round increases the difficulty level.
- On incorrect input, the system triggers a failure state and resets the session.

### LCD Interface
The LCD operates in 4-bit parallel mode.  
Commands and data are transmitted in two nibbles.  

The display is used to:
- Show the current level
- Display system messages (Game On, Game Over, New Record)
- Provide user feedback during gameplay

### Flash Memory Management
The highest score is stored at a predefined Flash memory address.  

When a new record is achieved:
- The corresponding Flash sector is erased
- The updated score is programmed using HAL Flash APIs

At startup, the stored value is read and loaded into the system.

### Input Control Mechanism
Buttons are configured with pull-up resistors.  
A long-press detection mechanism allows enabling and disabling the game.

---

## Project Scope

This project demonstrates practical implementation of:

- GPIO input/output control
- Timing and delay management
- Pseudo-random number generation
- Parallel LCD communication (4-bit)
- Buzzer signal generation
- Internal Flash memory programming
- Embedded state-based game logic

---

This project represents a complete embedded system application combining hardware interaction, memory management, and interactive logic design on the STM32 platform.
