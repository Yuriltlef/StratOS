#include "myDelay.h"
#include "st7786_spi.h"
#include "stm32f10x_gpio.h"
#include "test_1.h"

void gpioConfig(void) {
    GPIO_InitTypeDef GPIO_Config;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_Config.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_Config.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Config.GPIO_Pin   = GPIO_Pin_1;
    GPIO_Init(GPIOA, &GPIO_Config);

    GPIO_Config.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &GPIO_Config);

    GPIO_Config.GPIO_Pin  = GPIO_Pin_13;
    GPIO_Config.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_Config);
}

typedef enum {
    Nothing,
    KEY1_SHORT_DOWN,
    KEY2_SHORT_DOWN,
    KEY1_LONG_DOWN,
    KEY2_LONG_DOWN
} KeyState;

typedef struct {
    KeyState state;
    uint32_t dowmTime1;
    uint32_t dowmTime2;
} NowState;

void update_state(NowState* keyState) {
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
        myDelay(10);
        keyState->dowmTime1 += 10;
        return;
    }

    if (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
        if (keyState->dowmTime1 == 0) {
            keyState->state = Nothing;
        }
        if (keyState->dowmTime1 < 500 && keyState->dowmTime1 > 0) {
            keyState->state = KEY1_SHORT_DOWN;
        }
        if (keyState->dowmTime1 >= 500) {
            keyState->state = KEY1_LONG_DOWN;
        }
        keyState->dowmTime1 = 0;
    }
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2)) {
        myDelay(10);
        keyState->dowmTime2 += 10;
        return;
    }

    if (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2)) {
        if (keyState->state == Nothing) {
            if (keyState->dowmTime2 == 0) {
                keyState->state = Nothing;
            }
            if (keyState->dowmTime2 < 500 && keyState->dowmTime2 > 0) {
                keyState->state = KEY2_SHORT_DOWN;
            }
            if (keyState->dowmTime2 >= 500) {
                keyState->state = KEY2_LONG_DOWN;
            }
            keyState->dowmTime2 = 0;
        }
    }
}

static NowState nowState = {Nothing, 0, 0};
int main(void) {

    myDelayInit();
    St7786Spi4Color18 red = {63, 30, 10};

    st7789Init(&St7789Init);
    St7786Rect rect1 = {0, 0, 240, 138};
    ST_BLACK(BL);
    ST_CYAN(CY);
    st7789RectSet(&rect1, 0, 0, 240, 138);
    st7789ColorSet(&red, 63, 30, 10);
    st7789DMAFillRect(&rect1, &red);

    st7789RectSet(&rect1, 0, 138, 240, 2);
    st7789ColorSet(&red, 0, 0, 0);
    st7789DMAFillRect(&rect1, &red);

    st7789RectSet(&rect1, 0, 140, 240, 140);
    st7789ColorSet(&red, 0, 63, 63);
    st7789DMAFillRect(&rect1, &red);

    st7789ColorSet(&red, 63, 30, 10);
    st7789SpiShowStr("Hello!", -5, 10, 150, &BL, &CY, MID);
    st7789SpiShowChar('X', 44, 20, &BL, &red, MID);
    while (1) {

        for (unsigned char i = 32; i <= 126; i++) {
            st7789SpiShowChar(i, 20, 20, &BL, &red, MID);
            // myDelay(100);
        }
    }
}
