/*
 * ================================================================================
 * @文件名称: tracking.c
 * @功能描述: 八路红外循迹模块串口驱动（USART3）
 * @所属模块: Components/tracking
 * @依赖: tracking.h, y_usart.h, y_timer.h
 * @协议说明:
 *   主控发送: $0,0,1#  -> 关闭校准/关闭模拟量/开启数字量
 *   模块返回: $D,x1:0,x2:0,...,x8:0#
 *   x1~x8 从左到右，0=检测到黑线，1=白底/未检测到
 * ================================================================================
 */

#include "tracking/tracking.h"

#include <string.h>

#include "y_timer/y_timer.h"
#include "y_usart/y_usart.h"

#define TRACKING_FRAME_MAX_LENGTH 96U
#define TRACKING_REQUEST_PERIOD_MS 500U
#define TRACKING_ONLINE_TIMEOUT_MS 300U

static uint8_t g_tracking_digits[TRACKING_CHANNEL_COUNT] = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};
static char g_tracking_frame[TRACKING_FRAME_MAX_LENGTH];
static uint8_t g_tracking_frame_index;
static uint8_t g_tracking_receiving;
static uint8_t g_tracking_data_valid;
static uint32_t g_tracking_last_update_ms;
static uint32_t g_tracking_last_request_ms;

static void tracking_request_digital_stream(void)
{
    static char request[] = "$0,0,1#";

    uart3_send_str(request);
    g_tracking_last_request_ms = millis();
}

static uint8_t tracking_parse_digital_frame(const char *frame)
{
    uint8_t digits[TRACKING_CHANNEL_COUNT];
    uint8_t index;
    const char *cursor;

    if ((frame == 0) || (frame[0] != '$') || (frame[1] != 'D'))
    {
        return 0U;
    }

    cursor = frame + 2;
    for (index = 0; index < TRACKING_CHANNEL_COUNT; index++)
    {
        cursor = strchr(cursor, ':');
        if ((cursor == 0) || ((cursor[1] != '0') && (cursor[1] != '1')))
        {
            return 0U;
        }

        digits[index] = (uint8_t)(cursor[1] - '0');
        cursor += 2;
    }

    memcpy(g_tracking_digits, digits, sizeof(g_tracking_digits));
    g_tracking_data_valid = 1U;
    g_tracking_last_update_ms = millis();
    return 1U;
}

void tracking_init(void)
{
    uint8_t index;

    g_tracking_frame_index = 0U;
    g_tracking_receiving = 0U;
    g_tracking_data_valid = 0U;
    g_tracking_last_update_ms = 0U;
    g_tracking_last_request_ms = 0U;

    for (index = 0; index < TRACKING_CHANNEL_COUNT; index++)
    {
        g_tracking_digits[index] = 1U;
    }

    memset(g_tracking_frame, 0, sizeof(g_tracking_frame));
    tracking_request_digital_stream();
}

void tracking_run(void)
{
    uint32_t now = millis();

    if ((now - g_tracking_last_request_ms) < TRACKING_REQUEST_PERIOD_MS)
    {
        return;
    }

    if ((!g_tracking_data_valid) || ((now - g_tracking_last_update_ms) > TRACKING_ONLINE_TIMEOUT_MS))
    {
        tracking_request_digital_stream();
    }
}

void tracking_uart_rx_byte(uint8_t rx_byte)
{
    if (rx_byte == '$')
    {
        g_tracking_receiving = 1U;
        g_tracking_frame_index = 0U;
        g_tracking_frame[g_tracking_frame_index++] = (char)rx_byte;
        return;
    }

    if (!g_tracking_receiving)
    {
        return;
    }

    if (g_tracking_frame_index >= (TRACKING_FRAME_MAX_LENGTH - 1U))
    {
        g_tracking_receiving = 0U;
        g_tracking_frame_index = 0U;
        return;
    }

    g_tracking_frame[g_tracking_frame_index++] = (char)rx_byte;

    if (rx_byte == '!')
    {
        g_tracking_receiving = 0U;
        g_tracking_frame_index = 0U;
        return;
    }

    if (rx_byte != '#')
    {
        return;
    }

    g_tracking_frame[g_tracking_frame_index] = '\0';
    g_tracking_receiving = 0U;
    (void)tracking_parse_digital_frame(g_tracking_frame);
    g_tracking_frame_index = 0U;
}

uint8_t tracking_is_data_valid(void)
{
    return g_tracking_data_valid;
}

uint8_t tracking_is_online(void)
{
    if (!g_tracking_data_valid)
    {
        return 0U;
    }

    return (uint8_t)((millis() - g_tracking_last_update_ms) <= TRACKING_ONLINE_TIMEOUT_MS);
}

uint8_t tracking_copy_digital(uint8_t *buffer, uint8_t buffer_len)
{
    if ((buffer == 0) || (buffer_len < TRACKING_CHANNEL_COUNT) || (!g_tracking_data_valid))
    {
        return 0U;
    }

    memcpy(buffer, g_tracking_digits, sizeof(g_tracking_digits));
    return 1U;
}

uint8_t tracking_get_digital_mask(void)
{
    uint8_t index;
    uint8_t mask = 0U;

    for (index = 0; index < TRACKING_CHANNEL_COUNT; index++)
    {
        if (g_tracking_digits[index] == 0U)
        {
            mask |= (uint8_t)(1U << index);
        }
    }

    return mask;
}


