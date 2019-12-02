/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <dfs_posix.h>
#include <string.h>
#include <fal.h>
#include <drv_lcd.h>
#include <cn_font.h>
#include <board.h>

extern void show_str(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *str, uint8_t size);	
extern int wavrecord_sample();
extern void bd();

#define THREAD_PRIORITY			25
#define THREAD_STACK_SIZE		1024
#define THREAD_TIMESLICE		10

static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;
static rt_thread_t tid3 = RT_NULL;

static rt_sem_t dynamic_sem = RT_NULL;

static struct rt_mailbox mb;
static char mb_pool[128];

/* 录音线程 tid1 入口函数 */
static void thread1_entry(void *parameter)
{
		static rt_err_t result;
    while(1)
    {
        /* 永久方式等待信号量，获取到信号量，则执行 number 自加的操作 */
        result = rt_sem_take(dynamic_sem, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            rt_kprintf("t2 take a dynamic semaphore, failed.\n");
            rt_sem_delete(dynamic_sem);
            return;
        }
        else
        {
					rt_kprintf("t2 take a dynamic semaphore, success.\n");
           wavrecord_sample();
					 rt_mb_send(&mb, NULL);
        }
				rt_thread_mdelay(100);
    }
		
}


/* 语音识别线程 tid2 入口函数 */
static void thread2_entry(void *parameter)
{

    while (1)
    {
        rt_kprintf("thread1: try to recv a mail\n");

        /* 从邮箱中收取邮件 */
        if (rt_mb_recv(&mb, NULL, RT_WAITING_FOREVER) == RT_EOK)
        {
						show_str(20, 40, 200, 32, (rt_uint8_t *)"百度语音识别", 32);
						show_str(20, 100, 200, 32, (rt_uint8_t *)"识别结果：", 32);
            rt_kprintf("get a mail from mailbox!");
            bd();
            rt_thread_mdelay(100);
        }
    }
    /* 执行邮箱对象脱离 */
    rt_mb_detach(&mb);
}

/* 按键线程 tid3 入口函数 */
static void thread3_entry(void *parameter)
{
		unsigned int count = 1;
		while(count > 0)
		{
			if(rt_pin_read(KEY0) == 0)
			{
					rt_kprintf("release a dynamic semaphore.\n");
          rt_sem_release(dynamic_sem);
			}
			rt_thread_mdelay(100);
		}
}

int main(void)
{
	
		fal_init();
		rt_pin_mode(KEY0, PIN_MODE_INPUT);
		rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
		rt_pin_mode(PIN_LED_G, PIN_MODE_OUTPUT);
		rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
		rt_pin_write(PIN_LED_R,1);
		rt_pin_write(PIN_LED_G,1);
		rt_pin_write(PIN_LED_B,1);
		
    /* 清屏 */
    lcd_clear(WHITE);

    /* 设置背景色和前景色 */
    lcd_set_color(WHITE,BLACK);

    /* 在LCD 上显示字符 */
    lcd_show_string(55, 5, 24, "RT-Thread");
		
		show_str(120, 220, 200, 16, (rt_uint8_t *)"By 霹雳大乌龙", 16);
	
		dynamic_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);
    if (dynamic_sem == RT_NULL)
    {
        rt_kprintf("create dynamic semaphore failed.\n");
        return -1;
    }
    else
    {
        rt_kprintf("create done. dynamic semaphore value = 0.\n");
    }
	
		rt_err_t result;

    /* 初始化一个 mailbox */
    result = rt_mb_init(&mb,
                        "mbt",                      /* 名称是 mbt */
                        &mb_pool[0],                /* 邮箱用到的内存池是 mb_pool */
                        sizeof(mb_pool) / 4,        /* 邮箱中的邮件数目，因为一封邮件占 4 字节 */
                        RT_IPC_FLAG_FIFO);          /* 采用 FIFO 方式进行线程等待 */
    if (result != RT_EOK)
    {
        rt_kprintf("init mailbox failed.\n");
        return -1;
    }
		
		tid1 = rt_thread_create("thread1",
                            thread1_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
		
		
		tid2 = rt_thread_create("thread2",
                            thread2_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);
		
		tid3 = rt_thread_create("thread3",
                            thread3_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid3 != RT_NULL)
        rt_thread_startup(tid3);
	
    return 0;
}

