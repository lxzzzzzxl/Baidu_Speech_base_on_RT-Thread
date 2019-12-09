## 一. 前言
在前面的2篇连载中我们已经讲解了百度语音识别的流程，如何使用webclient软件包进行语音识别，如何使用CJson软件包进行数据解析，如何在LCD上显示识别结果，如何通过语音识别控制外设。这一切的一切的首要前提，就是语音，那我们前面使用的是事先录制好的音频，而本次连载，我们终于要来实现录音功能了，有了录音，你想怎么识别就可以怎么识别，是不是很棒。<br>
我将采用RT-Thread的Audio设备框架，下面我会简单介绍该框架。但在那之前，我建议你先去看看正点原子教程的 “音乐播放器” 及 “录音机” 两个例程，确保你对WAVE文件，音频编解码芯片，SAI等知识点有一定的了解。
## 二. Audio设备框架
Audio（音频）设备是嵌入式系统中非常重要的一个组成部分，负责音频数据的采样和输出。如下图所示：

![undefined](http://ww1.sinaimg.cn/large/93ef9e7bly1g86yuo2s8kj20i208qaaa.jpg)

RT-Thread的Audio设备驱动框架为我们提供了标准 device 接口(open/close/read/control)，只要我们对接好设备框架，就可以在我们的应用代码里直接使用这些标准接口，对设备进行操作。（RT-Thread其他设备框架实现原理都是如此）<br>

详细介绍见[RT-Thread文档中心](https://www.rt-thread.org/document/site/programming-manual/device/audio/audio/#audio_3)

本篇我们不具体讲解Audio设备框架的对接，因为我使用的潘多拉开发板是官方支持的板子，所以底层驱动，框架对接这部分已经有相应的支持了。这里只简单提一下设备框架的对接方法，RT-Thread所有的设备框架的对接，基本上都是两大步骤：
> 1. 准备好相应的设备驱动，实现对应框架的ops函数<br>
> 2. 进行设备注册

想要弄懂这两步，RT-Thread的文档中心需要多看，还有就是去看框架源码。
## 三. 动手实践
#### 1.打开Audio设备（使能录音功能）
![微信截图_20191209153541.png](http://ww1.sinaimg.cn/large/93ef9e7bgy1g9qidcptcej20ws0jk3z5.jpg)
![微信截图_20191209152617.png](http://ww1.sinaimg.cn/large/93ef9e7bgy1g9qi9tojkyj20ws0jkaak.jpg)
#### 2.录音功能实现
```c
/* wav_record.c */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#define RECORD_TIME_MS      5000
#define RECORD_SAMPLERATE   8000
#define RECORD_CHANNEL      2
#define RECORD_CHUNK_SZ     ((RECORD_SAMPLERATE * RECORD_CHANNEL * 2) * 20 / 1000)

#define SOUND_DEVICE_NAME    "mic0"      /* Audio 设备名称 */
static rt_device_t mic_dev;              /* Audio 设备句柄 */

struct wav_header
{
    char  riff_id[4];              /* "RIFF" */
    int   riff_datasize;           /* RIFF chunk data size,exclude riff_id[4] and riff_datasize,total - 8 */
    char  riff_type[4];            /* "WAVE" */
    char  fmt_id[4];               /* "fmt " */
    int   fmt_datasize;            /* fmt chunk data size,16 for pcm */
    short fmt_compression_code;    /* 1 for PCM */
    short fmt_channels;            /* 1(mono) or 2(stereo) */
    int   fmt_sample_rate;         /* samples per second */
    int   fmt_avg_bytes_per_sec;   /* sample_rate * channels * bit_per_sample / 8 */
    short fmt_block_align;         /* number bytes per sample, bit_per_sample * channels / 8 */
    short fmt_bit_per_sample;      /* bits of each sample(8,16,32). */
    char  data_id[4];              /* "data" */
    int   data_datasize;           /* data chunk size,pcm_size - 44 */
};

static void wavheader_init(struct wav_header *header, int sample_rate, int channels, int datasize)
{
    memcpy(header->riff_id, "RIFF", 4);
    header->riff_datasize = datasize + 44 - 8;
    memcpy(header->riff_type, "WAVE", 4);
    memcpy(header->fmt_id, "fmt ", 4);
    header->fmt_datasize = 16;
    header->fmt_compression_code = 1;
    header->fmt_channels = channels;
    header->fmt_sample_rate = sample_rate;
    header->fmt_bit_per_sample = 16;
    header->fmt_avg_bytes_per_sec = header->fmt_sample_rate * header->fmt_channels * header->fmt_bit_per_sample / 8;
    header->fmt_block_align = header->fmt_bit_per_sample * header->fmt_channels / 8;
    memcpy(header->data_id, "data", 4);
    header->data_datasize = datasize;
}

int wavrecord_sample(int argc, char **argv)
{
    int fd = -1;
    uint8_t *buffer = NULL;
    struct wav_header header;
    struct rt_audio_caps caps = {0};
    int length, total_length = 0;

    if (argc != 2)
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("wavrecord_sample file.wav\n");
        return -1;
    }

    fd = open(argv[1], O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        rt_kprintf("open file for recording failed!\n");
        return -1;
    }
    write(fd, &header, sizeof(struct wav_header));

    buffer = rt_malloc(RECORD_CHUNK_SZ);
    if (buffer == RT_NULL)
        goto __exit;

    /* 根据设备名称查找 Audio 设备，获取设备句柄 */
    mic_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (mic_dev == RT_NULL)
        goto __exit;

    /* 以只读方式打开 Audio 录音设备 */
    rt_device_open(mic_dev, RT_DEVICE_OFLAG_RDONLY);

    /* 设置采样率、通道、采样位数等音频参数信息 */
    caps.main_type               = AUDIO_TYPE_INPUT;                            /* 输入类型（录音设备 ）*/
    caps.sub_type                = AUDIO_DSP_PARAM;                             /* 设置所有音频参数信息 */
    caps.udata.config.samplerate = RECORD_SAMPLERATE;                           /* 采样率 */
    caps.udata.config.channels   = RECORD_CHANNEL;                              /* 采样通道 */
    caps.udata.config.samplebits = 16;                                          /* 采样位数 */
    rt_device_control(mic_dev, AUDIO_CTL_CONFIGURE, &caps);

    while (1)
    {
        /* 从 Audio 设备中，读取 20ms 的音频数据  */
        length = rt_device_read(mic_dev, 0, buffer, RECORD_CHUNK_SZ);

        if (length)
        {
            /* 写入音频数据到到文件系统 */
            write(fd, buffer, length);
            total_length += length;
        }

        if ((total_length / RECORD_CHUNK_SZ) >  (RECORD_TIME_MS / 20))
            break;
    }

    /* 重新写入 wav 文件的头 */
    wavheader_init(&header, RECORD_SAMPLERATE, RECORD_CHANNEL, total_length);
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(struct wav_header));
    close(fd);

    /* 关闭 Audio 设备 */
    rt_device_close(mic_dev);

__exit:
    if (fd >= 0)
        close(fd);

    if (buffer)
        rt_free(buffer);

    return 0;
}
MSH_CMD_EXPORT(wavrecord_sample, record voice to a wav file);
```
以上代码其实就是文档中心的示例程序，搬过来就能直接使用，很方便~

注意这里：
```c
#define RECORD_TIME_MS      5000
#define RECORD_SAMPLERATE   8000
#define RECORD_CHANNEL      2
```
录音时间固定为每次5s，音频采样率设置为8000，通道数设置为2，因为百度语音要求的是16000（8000*2）的采样率。

同样的，这里导出了wavrecord_sample这个命令，使用方式：
在finsh控制台输入:
```c
wavrecord_sample bd.wav
```
录音功能便开启了，程序里设置的录音时间是5s，音频将存放于bd.wav中。再用之前实现的bd命令进行语音识别，发现效果还是非常不错的。
至此，我们的录音功能就算实现了。说实话，Audio设备还是挺复杂的，需要反复学习，所以我可能讲的不好（好吧，我基本没讲），需要大家多去看看框架的源码。
#### 3.IPC使用
整个项目的各部分功能我们都已经实现了，接下来就要用IPC将它们串接起来，形成一个完整的项目，设计效果是这样的：
> 按下按键，开始录音，录音结束后自动将音频发送到百度服务器端，返回识别结果，进行数据解析，显示结果（控制外设）。

那么我们大致可以将以上功能划分为三个线程，分别是：按键线程，录音线程以及识别线程。具体怎么做呢？
前面我已经把各部分功能拆分成了多个源文件，这样使整个工程看起来更加简洁：
* **main.c** ---> 初始化，创建线程...
* **bd_speech_rcg.c** ---> 语音识别
* **wav_record.c** ---> 录音 
* 按键我这里只是简单的读IO状态，就放在main.c就好了

bd_speech_rcg.c和wav_record.c里都是前面已经实现的功能函数，这里我就不做讲解，主要来看看main.c：
```c
/* main.c */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <dfs_posix.h>
#include <string.h>
#include <fal.h>
#include <drv_lcd.h>
#include <cn_font.h>

/* 函数声明 */	
extern int wavrecord_sample();
extern void bd();

/* 线程参数 */
#define THREAD_PRIORITY			25      //优先级
#define THREAD_STACK_SIZE		1024    //线程栈大小
#define THREAD_TIMESLICE		10      //时间片

/* 线程句柄 */
static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;
static rt_thread_t tid3 = RT_NULL;

/* 指向信号量的指针 */
static rt_sem_t dynamic_sem = RT_NULL;

/* 邮箱控制块 */
static struct rt_mailbox mb;
/* 用于放邮件的内存池 */
static char mb_pool[128];

/* 录音线程 tid1 入口函数 */
static void thread1_entry(void *parameter)
{
	static rt_err_t result;
    while(1)
    {
        result = rt_sem_take(dynamic_sem, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            rt_kprintf("take a dynamic semaphore, failed.\n");
            rt_sem_delete(dynamic_sem);
            return;
        }
        else
        {
			rt_kprintf("take a dynamic semaphore, success.\n");
            wavrecord_sample();         //获取到信号量，开始录音
			rt_mb_send(&mb, NULL);      //录音结束，发送邮件
        }
		rt_thread_mdelay(100);
    }		
}

/* 语音识别线程 tid2 入口函数 */
static void thread2_entry(void *parameter)
{
    while (1)
    {
        rt_kprintf("try to recv a mail\n");
        /* 从邮箱中收取邮件 */
        if (rt_mb_recv(&mb, NULL, RT_WAITING_FOREVER) == RT_EOK)
        {
			show_str(20, 40, 200, 32, (rt_uint8_t *)"百度语音识别", 32);
			show_str(20, 100, 200, 32, (rt_uint8_t *)"识别结果：", 32);
            rt_kprintf("get a mail from mailbox!");
            bd();       //收到邮件，进行语音识别
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
                rt_sem_release(dynamic_sem);        //当按键被按下，释放一个信号量
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
	
    /* 创建一个动态信号量，初始值是 0 */
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

    /* 创建线程 */	
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
```
通过上面的源码可以看到，我采用了信号量+邮箱的通讯机制；按键线程不断读取IO状态，当按键被按下时，释放一个信号量；录音线程处于永久等待信号量的状态，当接收到一个信号量时开始录音，录音结束后发送一封邮件到邮箱中；识别线程不停尝试获取邮件，当接收到邮件时，进行语音识别，后续的解析，显示。。。都是我们之前讲过的了。

## 四. 总结
自此整个百度语音识别项目就完整结束了，或许在某些功能实现上并不是很完美，或许某些地方会有错误，也希望大家批评指正，但大致流程应该是没问题的，效果亲测也OK。我会把完整工程放到我的GitHub上，后续随着我对RT-Thread的进一步学习，我会对项目进行优化，功能拓展啥的，大家也可以在GitHub上看到，
项目地址：https://github.com/lxzzzzzxl/Baidu_Speech_base_on_RT-Thread

谢谢！
                                                                                            By 霹雳大乌龙！