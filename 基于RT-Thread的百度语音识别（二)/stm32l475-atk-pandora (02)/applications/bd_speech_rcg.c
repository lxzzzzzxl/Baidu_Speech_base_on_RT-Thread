/* bd_speech_rcg.c */

#include <rtthread.h>
#include <bd_speech_rcg.h>

#include <sys/socket.h> //网络功能需要的头文件
#include <webclient.h>  //webclient软件包头文件
#include <dfs_posix.h>  //文件系统需要的头文件
#include <cJSON.h>      //CJSON软件包头文件
#include <cn_font.h>
#include <mycc936.h>

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
		unsigned short* buf = malloc(128);
		rt_memset(buf,0,128);
		
		int size = Utf8ToUnicode(item->valuestring, (char*)buf);
		
		unsigned char *buffer = malloc(size);
		rt_memset(buffer,0,size);
		unicode2gbk((uint8_t*)buf,(uint8_t*)buffer);
	
		show_str(20, 100, 200, 32, (uint8_t*)buffer, 32);
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
