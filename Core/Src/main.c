#include "main.h"
#include "lcd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUTTON_COUNT        4
#define MAX_SEQUENCE_LENGTH 20

#define SHOW_MS        300
#define GAP_MS         120
#define PRESS_MS       150
#define FLASH_SCORE_ADDR  ((uint32_t)0x0807F800)

uint32_t maxScore = 0;

static GPIO_TypeDef* const btnPorts[BUTTON_COUNT] = {GPIOB, GPIOB, GPIOB, GPIOA};
static const uint16_t      btnPins [BUTTON_COUNT] = {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_3, GPIO_PIN_10};
static GPIO_TypeDef* const ledPorts[BUTTON_COUNT] = {GPIOA, GPIOA, GPIOA, GPIOB};
static const uint16_t      ledPins [BUTTON_COUNT] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_4, GPIO_PIN_0};
static GPIO_TypeDef* const clrPorts[BUTTON_COUNT] = {GPIOA, GPIOB, GPIOC, GPIOA};
static const uint16_t      clrPins [BUTTON_COUNT] = {GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_9};

#define RED_PORT   GPIOC
#define RED_PIN    GPIO_PIN_0
#define GREEN_PORT GPIOC
#define GREEN_PIN  GPIO_PIN_1
#define PIEZO_PORT GPIOB
#define PIEZO_PIN  GPIO_PIN_8

Lcd_HandleTypeDef lcd;
static uint8_t seq[MAX_SEQUENCE_LENGTH];
static uint8_t seqLen = 1;
volatile uint8_t gameEnabled = 0;
uint8_t allButtonsPressed(void);

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void Error_Handler(void);
uint32_t Flash_ReadScore(void);
void Flash_WriteScore(uint32_t score);

static inline void dly(uint32_t ms) {
    HAL_Delay(ms);
}

void beep(uint32_t ms, uint32_t freq) {
    uint32_t half = 1000000 / (2 * freq);
    uint32_t cycles = (ms * 1000UL) / (2 * half);
    while (cycles--) {
        HAL_GPIO_WritePin(PIEZO_PORT, PIEZO_PIN, GPIO_PIN_SET);
        for (volatile uint32_t i = 0; i < half / 5; i++) __NOP();
        HAL_GPIO_WritePin(PIEZO_PORT, PIEZO_PIN, GPIO_PIN_RESET);
        for (volatile uint32_t i = 0; i < half / 5; i++) __NOP();
    }
}

void beepStartMelody(void) {
    int melody[] = {659, 698, 784, 880, 988, 1047};
    for (int i = 0; i < 6; i++) {
        beep(120, melody[i]);
        HAL_Delay(40);
    }
}

void beepSuccess(void) {
	beep(100, 1200);  // Orta seviye tonda başla
	    dly(50);
	    beep(150, 1600);  // Daha yüksek tona çık
	    dly(50);
	    beep(200, 2000);  // En yüksek ton, tatmin edici bir bitiş
	}


void beepError(void) {
	 beep(200, 400);   // Kalın ve düşük bir ton
	    dly(50);
	    beep(200, 300);   // Daha da kalınlaşır
	    dly(50);
	    beep(300, 200);   // Son olarak en kalın ve uzun toay(50);
    beep(300, 262);
}

static void ledOn(uint8_t i) {
    HAL_GPIO_WritePin(ledPorts[i], ledPins[i], GPIO_PIN_SET);
}

static void ledOff(uint8_t i) {
    HAL_GPIO_WritePin(ledPorts[i], ledPins[i], GPIO_PIN_RESET);
}

static uint8_t waitBtn(void) {
    while (1) {
        for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
            if (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) == GPIO_PIN_RESET) {
                while (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) == GPIO_PIN_RESET);
                return i;
            }
        }
        dly(10);
    }
}

static void showSeq(void) {
    for (uint8_t i = 0; i < seqLen; ++i) {
        ledOn(seq[i]);
        dly(SHOW_MS);
        ledOff(seq[i]);
        dly(GAP_MS);
    }
}
void checkGameToggle(void) {
    static uint32_t pressStart = 0;
    uint8_t allPressed = 1;

    for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
        if (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) != GPIO_PIN_RESET) {
            allPressed = 0;
            break;
        }
    }

    if (allPressed) {
        if (pressStart == 0)
            pressStart = HAL_GetTick();
        else if (HAL_GetTick() - pressStart >= 1000) { // 1 saniye
            gameEnabled = !gameEnabled;
            pressStart = 0;

            // Geri bildirim
            if (gameEnabled) {
                beep(100, 1500);
                Lcd_clear(&lcd);
                Lcd_cursor(&lcd, 0, 0);
                Lcd_string(&lcd, "Game ON");
            } else {
                beep(200, 400);
                Lcd_clear(&lcd);
                Lcd_cursor(&lcd, 0, 0);
                Lcd_string(&lcd, "Game OFF");
            }

            dly(1000);
        }
    } else {
        pressStart = 0;
    }
}


static void showStartAnimation(void) {
    char msg1[17];
    uint8_t toggle = 0;

    Lcd_clear(&lcd);

    while (1) {
        if (toggle == 0) {
            sprintf(msg1, "High Score: %lu", maxScore);
            Lcd_cursor(&lcd, 0, 0);
            Lcd_string(&lcd, msg1);
            Lcd_cursor(&lcd, 1, 0);
            Lcd_string(&lcd, "Ready to play?");
        } else {
            Lcd_cursor(&lcd, 0, 0);
            Lcd_string(&lcd, "Press any key");
            Lcd_cursor(&lcd, 1, 0);
            Lcd_string(&lcd, "to start.....");
        }

        toggle = !toggle;

        for (uint16_t t = 0; t < 1000; t += 100) {
            for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
                uint8_t ledIndex = i;
                int8_t  btnIndex = BUTTON_COUNT - 1 - i;
                ledOn(ledIndex);
                HAL_GPIO_WritePin(clrPorts[btnIndex], clrPins[btnIndex], GPIO_PIN_SET);
                dly(100);
                ledOff(ledIndex);
                HAL_GPIO_WritePin(clrPorts[btnIndex], clrPins[btnIndex], GPIO_PIN_RESET);
            }

            for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
                if (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) == GPIO_PIN_RESET) {
                    while (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) == GPIO_PIN_RESET);
                    Lcd_clear(&lcd);
                    Lcd_cursor(&lcd, 0, 0);
                    Lcd_string(&lcd, "Get ready...");
                    dly(1500);
                    return;
                }
            }
        }
    }
}

static void playGame(void) {
    char levelStr[17];

    while (1) {
        showStartAnimation();
        beepStartMelody();
        seqLen = 1;

        while (1) {
            // 4 butona 1 saniye boyunca basılırsa oyunu sonlandır
            if (allButtonsPressed()) {
                uint32_t t0 = HAL_GetTick();
                while (allButtonsPressed()) {
                    if (HAL_GetTick() - t0 >= 1000) {
                        gameEnabled = 0;
                        Lcd_clear(&lcd);
                        Lcd_cursor(&lcd, 0, 0);
                        Lcd_string(&lcd, "Game Turned Off");
                        Lcd_cursor(&lcd, 1, 0);
                        Lcd_string(&lcd, "Release Buttons");
                        return;
                    }
                }
            }

            sprintf(levelStr, "Level: %d", seqLen);
            Lcd_clear(&lcd);
            Lcd_cursor(&lcd, 0, 0);
            Lcd_string(&lcd, levelStr);
            dly(1000);

            for (uint8_t i = 0; i < seqLen; ++i)
                seq[i] = rand() % BUTTON_COUNT;

            showSeq();

            uint8_t failed = 0;
            for (uint8_t i = 0; i < seqLen; ++i) {
                // Kapanış kontrolü yine burada da yapılmalı
                if (allButtonsPressed()) {
                    uint32_t t0 = HAL_GetTick();
                    while (allButtonsPressed()) {
                        if (HAL_GetTick() - t0 >= 1000) {
                            gameEnabled = 0;
                            Lcd_clear(&lcd);
                            Lcd_cursor(&lcd, 0, 0);
                            Lcd_string(&lcd, "Game Turned Off");
                            Lcd_cursor(&lcd, 1, 0);
                            Lcd_string(&lcd, "Release Buttons");
                            return;
                        }
                    }
                }

                uint8_t p = waitBtn();
                ledOn(p);
                HAL_GPIO_WritePin(clrPorts[p], clrPins[p], GPIO_PIN_SET);
                dly(PRESS_MS);
                ledOff(p);
                HAL_GPIO_WritePin(clrPorts[p], clrPins[p], GPIO_PIN_RESET);

                if (p != seq[i]) {
                    HAL_GPIO_WritePin(RED_PORT, RED_PIN, GPIO_PIN_SET);
                    beepError();
                    dly(1000);
                    HAL_GPIO_WritePin(RED_PORT, RED_PIN, GPIO_PIN_RESET);

                    Lcd_clear(&lcd);
                    Lcd_cursor(&lcd, 0, 0);
                    Lcd_string(&lcd, "Game Over!");
                    Lcd_cursor(&lcd, 1, 0);
                    Lcd_string(&lcd, "Try Again!");
                    dly(2000);
                    failed = 1;
                    break;
                }
            }

            if (failed) break;

            HAL_GPIO_WritePin(GREEN_PORT, GREEN_PIN, GPIO_PIN_SET);
            beepSuccess();
            dly(500);
            HAL_GPIO_WritePin(GREEN_PORT, GREEN_PIN, GPIO_PIN_RESET);

            uint8_t isNewRecord = 0;
            if (seqLen > maxScore) {
                maxScore = seqLen;
                Flash_WriteScore(maxScore);
                isNewRecord = 1;
            }

            Lcd_clear(&lcd);

            if (isNewRecord) {
                Lcd_cursor(&lcd, 0, 0);
                Lcd_string(&lcd, " New Record!");
                Lcd_cursor(&lcd, 1, 0);
                Lcd_string(&lcd, "Memory Legend!");

                beep(523, 200);  // C5
                dly(100);
                beep(659, 200);  // E5
                dly(100);
                beep(783, 300);  // G5
                dly(150);
                beep(1046, 500); // C6
                dly(2000);
            } else if (seqLen % 3 == 0) {
                const char* successMessages[] = {
                    "Nice memory!",
                    "Well played!",
                    "Awesome!",
                    "You're sharp!",
                    "Great job!",
                    "Memory Master!",
                    "You nailed it!",
                    "Champion!"
                };
                uint8_t msgIndex = rand() % (sizeof(successMessages) / sizeof(successMessages[0]));
                Lcd_cursor(&lcd, 0, 0);
                Lcd_string(&lcd, (char *)successMessages[msgIndex]);
                dly(1500);
            }

            if (seqLen < MAX_SEQUENCE_LENGTH)
                ++seqLen;

            dly(400);
        }
    }
}
uint8_t allButtonsPressed(void) {
    for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
        if (HAL_GPIO_ReadPin(btnPorts[i], btnPins[i]) == GPIO_PIN_SET)
            return 0;
    }
    return 1;
}



int main(void) {

	    HAL_Init();
	    SystemClock_Config();
	    MX_GPIO_Init();

	    Lcd_PortType ports[] = {GPIOA, GPIOA, GPIOB, GPIOC};
	    Lcd_PinType  pins[]  = {GPIO_PIN_12, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_6};
	    lcd = Lcd_create(ports, pins, GPIOC, GPIO_PIN_3, GPIOC, GPIO_PIN_2, LCD_4_BIT_MODE);

	    maxScore = Flash_ReadScore();
	    if (maxScore > 100) maxScore = 0;

	    beep(100, 800);
	    dly(50);
	    beep(100, 1000);
	    dly(50);
	    beep(150, 1200);

	    srand(HAL_GetTick());

	    while (1) {
	        checkGameToggle();

	        if (gameEnabled) {
	            playGame();
	        }

	        dly(50);
	    }


}

uint32_t Flash_ReadScore(void) {
    return *(uint32_t*)FLASH_SCORE_ADDR;
}

void Flash_WriteScore(uint32_t score) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef e = {0}; uint32_t err = 0;
    e.TypeErase = FLASH_TYPEERASE_SECTORS;
    e.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    e.Sector = FLASH_SECTOR_7;
    e.NbSectors = 1;
    HAL_FLASHEx_Erase(&e, &err);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SCORE_ADDR, score);
    HAL_FLASH_Lock();
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef O = {0}; RCC_ClkInitTypeDef C = {0};
    O.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    O.HSIState = RCC_HSI_ON;
    O.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&O);
    C.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1;
    C.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    C.AHBCLKDivider = RCC_SYSCLK_DIV1;
    C.APB1CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&C, FLASH_LATENCY_0);
}

void MX_GPIO_Init(void) {
    GPIO_InitTypeDef G = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    G.Mode = GPIO_MODE_OUTPUT_PP; G.Pull = GPIO_NOPULL; G.Speed = GPIO_SPEED_FREQ_LOW;
    G.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOA, &G);
    G.Pin = GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOB, &G);
    G.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOC, &G);

    G.Mode = GPIO_MODE_INPUT; G.Pull = GPIO_PULLUP;
    G.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOB, &G);
    G.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &G);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
