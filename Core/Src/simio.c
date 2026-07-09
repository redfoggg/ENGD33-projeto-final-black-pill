#include "simio.h"

extern UART_HandleTypeDef huart2;

void sim_printf(const char *fmt, ...)
{
#if DEBUG_RENODE
    char buffer[160];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 50);
#else
    (void)fmt;
#endif
}
