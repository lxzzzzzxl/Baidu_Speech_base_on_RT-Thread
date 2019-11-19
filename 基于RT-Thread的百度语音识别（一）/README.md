# 基于RT-Thread的百度语音识别（一）
## RT-Thread简介
RT-Thread是一个集实时操作系统（RTOS）内核、中间件组件和开发者社区于一体的技术平台，由熊谱翔先生带领并集合开源社区力量开发而成，RT-Thread也是一个组件完整丰富、高度可伸缩、简易开发、超低功耗、高安全性的物联网操作系统。RT-Thread具备一个IoT OS平台所需的所有关键组件，例如GUI、网络协议栈、安全传输、低功耗组件等等。经过11年的累积发展，RT-Thread已经拥有一个国内最大的嵌入式开源社区，同时被广泛应用于能源、车载、医疗、消费电子等多个行业，累积装机量超过2亿台，成为国人自主开发、国内最成熟稳定和装机量最大的开源RTOS。

RT-Thread拥有良好的软件生态，支持市面上所有主流的编译工具如GCC、Keil、IAR等，工具链完善、友好，支持各类标准接口，如POSIX、CMSIS、C++应用环境、Javascript执行环境等，方便开发者移植各类应用程序。商用支持所有主流MCU架构，如ARM Cortex-M/R/A, MIPS, X86, Xtensa, C-Sky, RISC-V，几乎支持市场上所有主流的MCU和Wi-Fi芯片。

得益于RT-thread丰富的组件以及一系列好用的软件包，作为应用开发者，我们不必过度关心底层的实现，而可以将更多的精力放在应用实现上，我将分享的百度语音识别便是如此。

## 百度AI简介
语音识别服务是百度AI众多服务中的一项，应用该服务，你可以将语音识别为文字，适用于手机应用语音交互、语音内容分析、智能硬件、呼叫中心智能客服等多种场景。我分享的百度语音识别就是将该服务应用于STM32上的一个案例。

## 一. 项目介绍
> 硬件：IoT Board（正点原子 - 潘多拉L475开发板）
> 平台：RT-Thread + 百度AI

1. 使用RT-Thread的 **stm32l475-atk-pandora BSP** 或 **RT-Thread IoT-Board SDK**；
2. 挂载elm FatFS文件系统，用于存放待识别音频；
3. 初始化板载WIFI模块 AP6181 或 使用AT组件+ESP8266，使开发板具备网络功能；
4. 使用Audio组件，实现录音功能，并将音频存入文件系统；
5. 使用webclient软件包，将文件系统中的音频上传到百度AI服务端，识别后返回Json数据；
6. 使用cJson软件包解析数据，根据解析出的数据作出响应动作（控制RGB灯）；
7. 将中文字库烧写进外部spi flash，使用SUFD+FAL软件包读写flash，实现LCD显示识别结果。

因为项目的重点以及难点在于百度语音识别，所以接下来的文章将着重讲解上述的4-7点，其他部分大家自行前往RT-Thread的文档中心学习。

## 二. 百度语音识别服务使用流程
在项目开始之前，我们需要先熟悉一遍百度语音服务的调用流程，不然直接写代码，你可能会一脸懵逼。
百度语音识别简单来说就是百度AI通过API的方式给开发者提供一个通用的HTTP接口，开发者通过这个接口上传音频文件，服务器返回识别结果，就这么简单，具体怎么做我们接着往下看；

### 1. 首先我们要注册一个百度开发者账号，然后创建一个语音识别的应用：

#### 1.1 搜索 “百度AI” ，进入如下页面，点击右上方控制台（未注册的需注册）：
![](https://ftp.bmp.ovh/imgs/2019/10/105cd8bd45d5fe3b.png)

#### 1.2 点击“语音技术”，进入如下页面，点击“创建应用”：
![](https://ftp.bmp.ovh/imgs/2019/10/702a5db744a2bb28.png)

#### 1.3 填写相关信息后点击创建，创建成功后可以在应用列表看到你新创建的应用：
![](https://ftp.bmp.ovh/imgs/2019/10/35856dd41c1c0f9d.png)

### 2. 语音识别过程
#### 2.1 获取 Access Token：
> 向授权服务地址 https://aip.baidubce.com/oauth/2.0/token 发送请求（推荐使用POST），并在URL中带上以下参数：
>
> **grant_type**： 必须参数，固定为**client_credentials**；
> **client_id**： 必须参数，应用的API Key；
> **client_secret**： 必须参数，应用的Secret Key；

* 使用浏览器获取Access Token：
```
https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=Gqt6jzFQDB1UrfVlkTBxr43k&client_secret=dsAQulSVgXEUq2xxyUGFegQOpUWVDpx2
```
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g93d3wtthnj21z40exgn0.jpg)

* 使用Postman获取Access Token:
> 这里给大家推荐一个好用的软件**Postman**，其是一款功能强大的网页调试与发送网页HTTP请求的接口测试神器，使用它你可以方便的体验百度语音识别的完整流程：

![](http://ww1.sinaimg.cn/large/93ef9e7bly1g93dip81suj20xr0mejtn.jpg)

#### 2.2 使用Access Token进行语音识别
有了Access Token，就可以开始进行语音识别了，我们采用raw方式，上传本地文件（使用官方提供的16k采样率pcm文件样例），还是用Postman来测试：
* 语音识别调用地址：https://vop.baidu.com/pro_api
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g93dyzlyzhj20u1027jrd.jpg)
* 语音数据的采样率和压缩格式在 HTTP-HEADER 里的Content-Type 表明，例：```Content-Type: audio/pcm;rate=16000```
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g93e0tobwvj20x80b50tk.jpg)
* 填写URL参数

字段名|可需|描述
--|--|--
cuid|必填|用户唯一标识，用来区分用户，计算UV值。建议填写能区分用户的机器 MAC 地址或 IMEI 码，长度为60字符以内。
token|必填|开放平台获取到的开发者[access_token]获取 Access Token "access_token")
dev_pid|选填|不填写lan参数生效，都不填写，默认80001，dev_pid参数见本节开头的表格

![](http://ww1.sinaimg.cn/large/93ef9e7bgy1g93eo4sb35j20wt0b90tp.jpg)
* 上传本地音频文件，发送识别，返回结果
![](http://ww1.sinaimg.cn/large/93ef9e7bgy1g93esv8or0j20x70gtjsh.jpg)

自此完整的百度语音识别流程就结束了，更多详细内容，大家参考百度AI文档中心的相关部分 https://ai.baidu.com/docs#/ASR-API/77e2b22e 。

## 三. 动手实践
实践开始之前，先确保你已经做了以下两件事：
1. 注册百度开发者账号，并创建了一个语音识别应用，而且成功获取了Access Token；
2. 建立一个基于你自己的STM32平台的RT-Thread工程，它必须具备Finsh控制台，文件系统，网络功能（不明白的参见RT-Thread文档中心，网络功能推荐使用AT组件+ESP8266，因为这是最简单快捷的方法）。

如果上面说的准备事项没有问题，那么请继续往下看：
这次我们将先跳过录音功能，而使用事先准备好的音频文件进行语音识别并控制板载RGB灯（项目介绍的5，6点），所以你还需要准备一些音频文件，比如“红灯开”，“蓝灯关”。。。我是用手机录的音频，然后使用ffmpeg工具将音频转为百度语音官方认为最适合的16k采样率pcm文件，最后将这些音频文件放进sd卡中，我们的文件系统也是挂载在sd卡上的。

本次工程会用到RT-Thread的两个软件包：webclient和cJson软件包，你需要使用ENV工具将这两个软件包添加进工程里。
* 添加webclient和cJson软件包
![微信截图_20191119165838.png](http://ww1.sinaimg.cn/large/93ef9e7bly1g93gdmma2ej20ws0jk3zv.jpg)

好了，开始写代码：
```C
/* bd_speech_rcg.c */

#include <rtthread.h>
#include <bd_speech_rcg.h>

#include <sys/socket.h> //网络功能需要的头文件
#include <webclient.h>  //webclient软件包头文件
#include <dfs_posix.h>  //文件系统需要的头文件
#include <cJSON.h>      //CJSON软件包头文件

/* 使用外设需要的头文件 */
#include <rtdevice.h>
#include <board.h>


/* 获取RGB灯对应的引脚编号 */
#define PIN_LED_R		GET_PIN(E,  7)
#define PIN_LED_G		GET_PIN(E,  8)
#define PIN_LED_B		GET_PIN(E,  9)

#define RES_BUFFER_SIZE     4096        //数据接收数组大小
#define HEADER_BUFFER_SIZE      2048    //最大支持的头部长度

/* URL */
#define POST_FILE_URL  "http://vop.baidu.com/server_api?dev_pid=1536&cuid=lxzzzzzxl&token=25.9119f50a60602866be9288f1f14a1059.315360000.1884092937.282335-15525116"
/* 头部数据（必需） */
char *form_data = "audio/pcm;rate=16000";

/* 预定义的指令 */
char *cmd1 = "打开红灯";
char *cmd2 = "关闭红灯";
char *cmd3 = "打开蓝灯";
char *cmd4 = "关闭蓝灯";
char *cmd5 = "打开绿灯";
char *cmd6 = "关闭绿灯";



/************************************************
函数名称 ： bd
功    能 ： 将音频文件发送到百度语音服务器，并接收响应数据
参    数 ： 音频文件名（注意在文件系统中的位置，默认根目录）
返 回 值 ： void
作    者 ： rtthread；霹雳大乌龙
*************************************************/
void bd(int argc, char **argv)
{	
    char *filename = NULL;
    unsigned char *buffer = RT_NULL;
    int content_length = -1, bytes_read = 0;
    int content_pos = 0;
    int ret = 0;
	
    /* 判断命令是否合法 */
    if(argc != 2)
    {
    	rt_kprintf("bd <filename>\r\n");
    	return;
    }
	
    /* 获取音频文件名 */
    filename = argv[1];

    /* 以只读方式打开音频文件 */
    int fd = open(filename, O_RDONLY, 0);
    if(fd < 0)
    {
    	rt_kprintf("open %d fail!\r\n", filename);
    	goto __exit;
    }
	
    /* 获取音频文件大小 */
    size_t length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
	
    /* 创建响应数据接收数据 */
    buffer = (unsigned char *) web_malloc(RES_BUFFER_SIZE);
    if(buffer == RT_NULL)
    {
    	rt_kprintf("no memory for receive response buffer.\n");
        ret = -RT_ENOMEM;
    	goto __exit;
    }
	
    /* 创建会话 */
    struct webclient_session *session = webclient_session_create(HEADER_BUFFER_SIZE);
    if(session == RT_NULL)
    {
    	ret = -RT_ENOMEM;
    	goto __exit;
    }

    /* 拼接头部数据 */
    webclient_header_fields_add(session, "Content-Length: %d\r\n", length);
    webclient_header_fields_add(session, "Content-Type: %s\r\n", form_data);
	
    /* 发送POST请求 */
    int rc = webclient_post(session, POST_FILE_URL, NULL);
    if(rc < 0)
    {
    	rt_kprintf("webclient post data error!\n");
        goto __exit;
    }else if (rc == 0)
    {
        rt_kprintf("webclient connected and send header msg!\n");
    }else
    {
        rt_kprintf("rc code: %d!\n", rc);
    }

    while(1)
    {
    	rt_memset(buffer, 0, RES_BUFFER_SIZE);
    	length = read(fd, buffer, RES_BUFFER_SIZE);
    	if(length <= 0)
    	{
    		break;
    	}
    	ret = webclient_write(session, buffer, length);
    	if(ret < 0)
    	{
    		rt_kprintf("webclient write error!\r\n");
    		break;
    	}	
    	rt_thread_mdelay(100);
    }
    close(fd);
    rt_kprintf("Upload voice data successfully\r\n");

    if(webclient_handle_response(session) != 200)
    {
        rt_kprintf("get handle resposne error!");
        goto __exit;
    }   

    /* 获取接收的响应数据长度 */
    content_length = webclient_content_length_get(session);
    rt_thread_delay(100);

    do
    {
    	bytes_read = webclient_read(session, buffer, 1024);
        if (bytes_read <= 0)
        {
            break;
        }

    	for(int index = 0; index < bytes_read; index++)
    	{
    		rt_kprintf("%c", buffer[index]);
    	}

        content_pos += bytes_read;
    	}while(content_pos < content_length);	

	/* 解析json数据 */
	bd_data_parse(buffer);
	
	__exit:
    			if(fd >= 0)
    				close(fd);
    			if(session != NULL)
    				webclient_close(session);
    			if(buffer != NULL)
    				web_free(buffer);
                    
    			return;
}

/* 导出为命令形式 */
MSH_CMD_EXPORT(bd, webclient post file);
```

```C
/* bd_speech_rcg.c */

/************************************************
函数名称 ： bd_data_parse
功    能 ： 解析json数据，并作出响应动作
参    数 ： data ------ 百度语音服务返回的数据（json格式）
返 回 值 ： void
作    者 ： RT-Thread；霹雳大乌龙
*************************************************/
void bd_data_parse(uint8_t *data)
{
    cJSON *root = RT_NULL, *object = RT_NULL, *item =RT_NULL;

    root = cJSON_Parse((const char *)data);
    if (!root)
    {
        rt_kprintf("No memory for cJSON root!\n");
        return;
    }
		
    object = cJSON_GetObjectItem(root, "result");

    item = object->child;

    rt_kprintf("\nresult	:%s \r\n", item->valuestring);
		
		rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
		rt_pin_mode(PIN_LED_G, PIN_MODE_OUTPUT);
		rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
		rt_pin_write(PIN_LED_R,1);
		rt_pin_write(PIN_LED_G,1);
		rt_pin_write(PIN_LED_B,1);
		
		if(strstr((char*)data, cmd1) != NULL)
		{
				/* 打开红灯 */
			rt_pin_write(PIN_LED_R,0);
		}		
		if(strstr((char*)data, cmd2) != NULL)
		{
				/* 关闭红灯 */
			rt_pin_write(PIN_LED_R,1);
		}
		if(strstr((char*)data, cmd3) != NULL)
		{
				/* 打开蓝灯 */
			rt_pin_write(PIN_LED_B,0);
		}
		if(strstr((char*)data, cmd4) != NULL)
		{
				/* 关闭蓝灯 */
			rt_pin_write(PIN_LED_B,1);
		}
		if(strstr((char*)data, cmd5) != NULL)
		{
				/* 打开绿灯 */
			rt_pin_write(PIN_LED_G,0);
		}
		if(strstr((char*)data, cmd6) != NULL)
		{
				/* 关闭绿灯 */
			rt_pin_write(PIN_LED_G,1);
		}
		

    if (root != RT_NULL)
        cJSON_Delete(root);
}
```

只需以上的代码（代码参考自论坛各位小伙伴），你就可以实现百度语音识别以及控制相应外设了。下面看看实际效果：
我使用的潘多拉开发板板载了stlink，且其为我们提供了一个虚拟串口，用usb数据线将开发板和电脑连接起来，将代码烧写进开发板后，我们利用这个虚拟串口，使用Xshell一类的终端软件，就可以看到如下的开机画面：
![](https://ftp.bmp.ovh/imgs/2019/10/bdfc1e5c5e29b5d8.png)
这便是RT-Thread提供的Finsh控制台组件，使用这个组件，我们可以方便地观察程序的运行状态，以命令行的形式调试运行程序，从图中我们可以看到，我们需要的文件系统和网络功能都已经初始化成功。

使用ls命令看看：
![](https://ftp.bmp.ovh/imgs/2019/10/aa9765bde9fb096e.png)
欸~，这便是我事先准备好的音频文件。

在上面的代码中，我们可以看到有这样一句：
```C
MSH_CMD_EXPORT(bd, webclient post file);
```
通过这行代码，我们就可以在Finsh控制台里使用bd这个命令，这个命令就是将音频文件发送到百度语音服务器，试试看：
![](https://ftp.bmp.ovh/imgs/2019/10/3b3f17044d530b3b.png)

看，我使用bd命令将greenon.pcm发送到百度语音服务器，正确识别出结果：“打开绿灯”；于此同时，rgb灯也亮起了绿色

![](https://ftp.bmp.ovh/imgs/2019/10/3298f9616fd1a0f9.jpg)

尝试其他音频文件，效果完美！！！
