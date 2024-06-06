/* Timer group-hardware timer example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_log.h"

#include "ds_timer.h"

static const char *TAG = "TIMER APP";

#define TIMER_DIVIDER         16  //  H硬件定时器时钟分割器
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER/1000)  // 将计数器值转换为毫秒
#define TIMER_INTERVAL0_SEC   (10) // 第一个计时器的样本测试间隔
#define TEST_WITH_RELOAD      1        // 测试将通过自动重新加载来完成

/*
*传递事件的示例结构。
*从定时器中断处理程序到主程序。
 */
typedef struct {
    uint64_t timer_minute_count;
    uint64_t timer_second_count;
} timer_event_t;

timer_event_t g_timer_event;

xQueueHandle timer_queue;

/*
 * 计时器组0 ISR处理程序。
*。
*注：
*我们这里不调用定时器接口，因为它们不是用IRAM_ATTR声明的。
*如果在禁用SPI闪存缓存的同时不为计时器IRQ提供服务，
*我们可以在不使用ESP_INTR_FLAG_IRAM标志的情况下分配该中断，并使用普通API。
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
    timer_spinlock_take(TIMER_GROUP_0);
    int timer_idx = (int) para;

    // /* 准备基本事件数据。
    //然后将被发送回主程序任务*/
    timer_event_t evt;

    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);

    /* 在警报被触发之后
      我们需要再次启用它，以便下次触发它 */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, timer_idx);

    /* 现在只需将事件数据发送回主程序任务 */
    xQueueSendFromISR(timer_queue, &evt, NULL);
    timer_spinlock_give(TIMER_GROUP_0);
}

/*
 * 初始化计时器组0的选定计时器。
*。
*TIMER_IDX-要初始化的定时器编号。
*AUTO_RELOAD-计时器是否应在报警时自动重新加载？
*TIMER_INTERVAL_SEC-要设置的报警间隔
 */
static void example_tg0_timer_init(int timer_idx,
                                   bool auto_reload, double timer_interval_sec)
{
    /* 选择并初始化定时器的基本参数 */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // 默认时钟源为APB
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* 计时器的计数器最初将从下面的值开始。
    此外，如果设置了AUTO_RELOAD，则该值将在报警时自动重新加载 */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* 配置报警值和报警中断。 */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                       (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

/*
 * 这个示例程序的主要任务是
 */
static void timer_example_evt_task(void *arg)
{
    while (1) {
        timer_event_t evt;
        xQueueReceive(timer_queue, &evt, portMAX_DELAY);
        g_timer_event.timer_minute_count ++;
        //60s 计算一次 刷新时间
        if(g_timer_event.timer_minute_count >= 6000){
            g_timer_event.timer_minute_count = 0;
        }
        g_timer_event.timer_second_count ++;
        //1s计算一次
        if(g_timer_event.timer_second_count >= 100){
            g_timer_event.timer_second_count = 0;
            ESP_LOGI(TAG, "1s run ");
        }
    }
}

/*
 * 在本例中，我们将测试定时器组0的硬件定时器0和定时器1。
 */
void ds_timer_init(void)
{
    g_timer_event.timer_minute_count = 0;
    g_timer_event.timer_second_count = 0;
    timer_queue = xQueueCreate(10, sizeof(timer_event_t));
    example_tg0_timer_init(TIMER_0, TEST_WITH_RELOAD, TIMER_INTERVAL0_SEC);
    xTaskCreate(timer_example_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);
}

