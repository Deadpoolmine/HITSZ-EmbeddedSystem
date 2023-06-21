#include "math.h"
#include "sys.h"
#include "uart.h"

#define NULL 0

char flag = 0;

void count_delay(int count)
{
    for (; count; count--)
        ;
}

void function_Test(void)
{
    if (flag)
        flag = 0;
    else
        flag = 1;
    count_delay(1000);
}

#define TRUE        1
#define FALSE       0
#define MAX_CMD_LEN 128

struct cmd_wrapper {
    char input_buf[MAX_CMD_LEN];
    int input_pos;
};

void cmd_wrapper_init(struct cmd_wrapper *cmd_wrapper)
{
    cmd_wrapper->input_pos = 0;
}

void cmd_wrapper_push_char(struct cmd_wrapper *cmd_wrapper, char c)
{
    if (cmd_wrapper->input_pos < MAX_CMD_LEN) {
        cmd_wrapper->input_buf[cmd_wrapper->input_pos++] = c;
        cmd_wrapper->input_buf[cmd_wrapper->input_pos] = '\0';
    }
}

char *cmd_wrapper_pop_cmd(struct cmd_wrapper *cmd_wrapper)
{
    cmd_wrapper->input_pos = 0;
    return cmd_wrapper->input_buf;
}

int strcmp(char *s1, char *s2)
{
    while (*s1 && *s2) {
        if (*s1 != *s2)
            return 1;
        s1++;
        s2++;
    }

    if (*s1 || *s2)
        return 1;

    return 0;
}

GPIO_TypeDef *port_to_gpio(uint8_t port, char *id)
{
    switch (port) {
    case 0:
        if (id != NULL) {
            *id = 'A';
        }
        return GPIOA;
    case 1:
        if (id != NULL) {
            *id = 'B';
        }
        return GPIOB;
    case 2:
        if (id != NULL) {
            *id = 'C';
        }
        return GPIOC;
    case 3:
        if (id != NULL) {
            *id = 'D';
        }
        return GPIOD;
    case 4:
        if (id != NULL) {
            *id = 'E';
        }
        return GPIOE;
    case 5:
        if (id != NULL) {
            *id = 'F';
        }
        return GPIOF;
    case 6:
        if (id != NULL) {
            *id = 'G';
        }
        return GPIOG;
    default:
        return NULL;
    }
}

void port_timer_enable(uint8_t port)
{
    RCC->APB2ENR |= 1 << (port + 2);
}

enum WAV_MODE {
    SQUARE,
    SINE
};

#define MAX_PIN_NUM 16

void square_wav(GPIO_TypeDef *gpio, uint8_t pin, uint32_t delay, uint32_t num_period)
{
    sys_gpio_set(gpio, 1 << pin, SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);

    for (int i = 0; i < num_period; i++) {
        gpio->ODR ^= (1 << pin);
        count_delay(delay);
    }
}

void sin_wav(GPIO_TypeDef *gpio, uint8_t pin, uint32_t delay, uint32_t num_period)
{
    uint16_t phase = 0x0000ffff / 2;

    for (pin = 0; pin < MAX_PIN_NUM; pin++) {
        sys_gpio_set(gpio, 1 << pin, SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);
    }

    gpio->ODR = phase;
    for (int i = 0; i < num_period; i++) {
        gpio->ODR = (uint16_t)(phase + sin(i * 3.14 / 180) * (0x0000ffff / 2));
        count_delay(delay);
    }
}

void gen_wav(uint8_t port, uint8_t pin, uint32_t delay, uint32_t num_period, uint8_t mode)
{
    char id;

    /* PA口时钟使能 */
    port_timer_enable(port);
    GPIO_TypeDef *gpio = port_to_gpio(port, &id);

    if (mode == SQUARE) {
        putstring("GPIO(A, 0) Activated for Square Wave\n\r");
        square_wav(gpio, pin, delay, num_period);
    } else if (mode == SINE) {
        putstring("GPIOA Activated for Sine Wave\n\r");
        sin_wav(gpio, pin, delay, num_period);
    }
}

void extix_init(uint8_t port, uint8_t pin, uint8_t pprio, uint8_t sprio, uint8_t ch, uint8_t group)
{
    char id;
    port_timer_enable(port);
    GPIO_TypeDef *gpio = port_to_gpio(port, &id);

    sys_gpio_set(gpio, 1 << pin,
                 SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);
    sys_nvic_ex_config(gpio, 1 << pin, SYS_GPIO_RTIR); /* WKUP配置为上升沿触发中断 */
    sys_nvic_init(pprio, sprio, ch, group);            /* 中断优先级设置 */
}

#define WAVE_GPIO_PORT 0
#define WAVE_GPIO_PIN  4

#define IO_INT_GPIO_PORT  5
#define IO_INT_GPIO_PIN   0
#define IO_INT_HANDLER    EXTI0_IRQn
#define IO_INT_IRQHANDLER EXTI0_IRQHandler

void IO_INT_IRQHANDLER(void)
{
    EXTI->PR = 1 << IO_INT_GPIO_PIN; /* 清除KEY0所在中断线 的中断标志位 */
    putstring("Interrupted Received\n\r");
}

int main(void)
{
    struct cmd_wrapper cmd_wrapper;
    int exec = FALSE;

    uart_init();
    extix_init(IO_INT_GPIO_PORT, IO_INT_GPIO_PIN, 0, 2, IO_INT_HANDLER, 2);
    cmd_wrapper_init(&cmd_wrapper);

    putstring("[SIMPLE SHELL] >> ");

    while (1) {
        char c = getchar();
        if (c == '\r') {
            putchar('\n');
            exec = TRUE;
        }
        putchar(c);

        if (!exec) {
            cmd_wrapper_push_char(&cmd_wrapper, c);
        } else {
            char *cmd = cmd_wrapper_pop_cmd(&cmd_wrapper);
            if (strcmp(cmd, "square_wav") == 0) {
                gen_wav(WAVE_GPIO_PORT, WAVE_GPIO_PIN, 1000, 100, SQUARE);
            } else if (strcmp(cmd, "sin_wav") == 0) {
                gen_wav(WAVE_GPIO_PORT, WAVE_GPIO_PIN, 1000, 10000, SINE);
            }
            putstring("[SIMPLE SHELL] >> ");
        }

        exec = FALSE;
    }
}
