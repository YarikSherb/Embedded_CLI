/*
 * hw_uart_driver.c
 *
 *  Created on: Jul 23, 2026
 *      Author: YarikSherb
 */
#include "main.h"

void hw_init(void *handler)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handler;

    if ((huart == NULL) || !IS_UART_INSTANCE(huart->Instance))
        return;

    HAL_UART_Init(huart);
}


void transmit_data_byte(void *handler, char byte)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handler;

    if ((huart == NULL) || !IS_UART_INSTANCE(huart->Instance))
        return;

    HAL_UART_Transmit(huart, (uint8_t *)&byte, 1, HAL_MAX_DELAY);
}

char IsActiveFlag_TX(void *handler)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handler;

    if ((huart == NULL) || !IS_UART_INSTANCE(huart->Instance))
        return 0;

    return (__HAL_UART_GET_FLAG(huart, UART_FLAG_TXE) != RESET);
}

char recive_data_byte(void *handler)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handler;
    uint8_t byte = 0;

    if ((huart == NULL) || !IS_UART_INSTANCE(huart->Instance))
        return 0;

    HAL_UART_Receive(huart, &byte, 1, HAL_MAX_DELAY);

    return (char)byte;
}

char IsActiveFlag_RX(void *handler)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handler;

    if ((huart == NULL) || !IS_UART_INSTANCE(huart->Instance))
        return 0;

    return (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET);
}
