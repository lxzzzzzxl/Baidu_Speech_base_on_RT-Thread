#include <webclient.h>  /* 使用 HTTP 协议与服务器通信需要包含此头文件 */
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <netdb.h>
#include <cJSON.h>
#include <finsh.h>
#include <drv_lcd.h>
#include <cn_font.h>
#include <dfs_posix.h>
#include <mycc936.h>

#define GET_HEADER_BUFSZ        1024        //头部大小
#define GET_RESP_BUFSZ          1024        //响应缓冲区大小
#define GET_URL_LEN_MAX         256         //网址最大长度
#define GET_URI                 "http://api.seniverse.com/v3/weather/now.json?key=6dxwrt9yzsyj3vmr&location=guangzhou&language=zh-Hans&unit=c" //获取天气的 API
#define AREA_ID                 "101021300" //上海浦东地区 ID


/* 天气数据解析 */
void weather_data_parse(rt_uint8_t *data)
{
		lcd_clear(WHITE);

    /* 设置背景色和前景色 */
    lcd_set_color(WHITE,BLACK);

    /* 在LCD 上显示字符 */
    lcd_show_string(55, 5, 24, "RT-Thread");
		show_str(20, 35, 200, 32, (rt_uint8_t *)"天气预报", 32);
		show_str(20, 80, 200, 32, (rt_uint8_t *)"城市：", 32);
		show_str(20, 120, 200, 32, (rt_uint8_t *)"天气：", 32);
		show_str(20, 160, 200, 32, (rt_uint8_t *)"温度：", 32);
		show_str(120, 220, 200, 16, (rt_uint8_t *)"By 霹雳大乌龙", 16);
	
    cJSON *root = RT_NULL, *object = RT_NULL, *item = RT_NULL, *last = RT_NULL, *hh = RT_NULL;

    root = cJSON_Parse((const char *)data);
    if (!root)
    {
        rt_kprintf("No memory for cJSON root!\n");
        return;
    }
    object = cJSON_GetObjectItem(root, "results");
		
		item = object->child;
		last = cJSON_GetObjectItem(item, "location");
		
		hh = cJSON_GetObjectItem(last, "id");
    rt_kprintf("\nid    :%s \n", hh->valuestring);
		
		hh = cJSON_GetObjectItem(last, "name");
    rt_kprintf("\nname    :%s \n", hh->valuestring);
		
		
		unsigned short* buf = malloc(128);
		int size = Utf82Unicode(hh->valuestring, (char*)buf);
		unsigned char *buffer = malloc(size);
		unicode2gbk((uint8_t*)buf, buffer);
		show_str(100, 80, 200, 32, (uint8_t*)buffer, 32);
		
		
		
		hh = cJSON_GetObjectItem(last, "country");
    rt_kprintf("\ncountry    :%s \n", hh->valuestring);
		
		hh = cJSON_GetObjectItem(last, "path");
    rt_kprintf("\npath    :%s \n", hh->valuestring);
		
		hh = cJSON_GetObjectItem(last, "timezone");
    rt_kprintf("\ntimezone    :%s \n", hh->valuestring);
		
		hh = cJSON_GetObjectItem(last, "timezone_offset");
    rt_kprintf("\ntimezone_offset    :%s \n", hh->valuestring);
		
		
		last = cJSON_GetObjectItem(item, "now");
		
		/*天气*/
		hh = cJSON_GetObjectItem(last, "text");
    rt_kprintf("\ntext    :%s \n", hh->valuestring);
		size = Utf82Unicode(hh->valuestring, (char*)buf);
		unicode2gbk((uint8_t*)buf, buffer);
		show_str(100, 120, 200, 32, (uint8_t*)buffer, 32);
		
		
		/*code*/
		hh = cJSON_GetObjectItem(last, "code");
    rt_kprintf("\ncode    :%s \n", hh->valuestring);
		
		/*温度*/
		hh = cJSON_GetObjectItem(last, "temperature");
    rt_kprintf("\ntemperature    :%s \n", hh->valuestring);
		show_str(100, 160, 200, 32, (rt_uint8_t *)hh->valuestring, 32);
		
		
		last = cJSON_GetObjectItem(item, "last_update");
		rt_kprintf("\nlast_update    :%s \n", last->valuestring);
		
		

    if (root != RT_NULL)
        cJSON_Delete(root);
}
void weather(int argc, char **argv)
{
    rt_uint8_t *buffer = RT_NULL;
    int resp_status;
    struct webclient_session *session = RT_NULL;
    char *weather_url = RT_NULL;
    int content_length = -1, bytes_read = 0;
    int content_pos = 0;

    /* 为 weather_url 分配空间 */
    weather_url = rt_calloc(1, GET_URL_LEN_MAX);
    if (weather_url == RT_NULL)
    {
        rt_kprintf("No memory for weather_url!\n");
        goto __exit;
    }
    /* 拼接 GET 网址 */
    rt_snprintf(weather_url, GET_URL_LEN_MAX, GET_URI, AREA_ID);

    /* 创建会话并且设置响应的大小 */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        rt_kprintf("No memory for get header!\n");
        goto __exit;
    }

    /* 发送 GET 请求使用默认的头部 */
    if ((resp_status = webclient_get(session, weather_url)) != 200)
    {
        rt_kprintf("webclient GET request failed, response(%d) error.\n", resp_status);
        goto __exit;
    }

    /* 分配用于存放接收数据的缓冲 */
    buffer = rt_calloc(1, GET_RESP_BUFSZ);
    if(buffer == RT_NULL)
    {
        rt_kprintf("No memory for data receive buffer!\n");
        goto __exit;
    }

    content_length = webclient_content_length_get(session);
    if (content_length < 0)
    {
        /* 返回的数据是分块传输的. */
        do
        {
            bytes_read = webclient_read(session, buffer, GET_RESP_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }
						for(int index = 0; index < bytes_read; index++)
					  {
					  	rt_kprintf("%c", buffer[index]);
			  		}
        } while (1);
    }
    else
    {
        do
        {
            bytes_read = webclient_read(session, buffer, 
                    content_length - content_pos > GET_RESP_BUFSZ ?
                            GET_RESP_BUFSZ : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }
						for(int index = 0; index < bytes_read; index++)
					  {
					  	rt_kprintf("%c", buffer[index]);
			  		}
            content_pos += bytes_read;
        } while (content_pos < content_length);
    }

    /* 天气数据解析 */
    weather_data_parse(buffer);

__exit:
    /* 释放网址空间 */
    if (weather_url != RT_NULL)
        rt_free(weather_url);
    /* 关闭会话 */
    if (session != RT_NULL)
        webclient_close(session);
    /* 释放缓冲区空间 */
    if (buffer != RT_NULL)
        rt_free(buffer);
}

MSH_CMD_EXPORT(weather, Get weather by webclient);
