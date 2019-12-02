# Chinese font library
## 1.简介
Chinese font library用于解决嵌入式开发中需要在LCD上显示中文的问题，该软件包基于FAL软件包的功能实现，通过访问存储在外部spi flash的字库文件，实现汉字的显示。同时提供 utf-8 -> unicode -> gbk 的转换函数接口，以应用于网络数据场景。更多应用场景(例：GSM中文短信等)有待实现。
### 1.1目录结构
名称 | 说明   
:----------|:----------
src | 源码 
inc | 头文件 
examples | 示例 

### 1.2许可证

### 1.3依赖
* RT-Thread SUFD组件
* RT-Thread FAL软件包
## 2.获取方式

## 3.使用方式
### 3.1 标准API
#### 在指定位置显示字符串
```c
void show_str(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *str, uint8_t size);
```
参数 | 描述   
:----------|:----------
x | 字符串起始x坐标
y | 字符串起始y坐标 
width | 字符串所占宽度 
height | 字符串所占高度 
*str | 字符串指针
size | 字体大小
**返回** | **描述**   
void | 无

### 3.2 网络应用场景API(编码转换)
#### utf-8转unicode
```c
int utf82unicode(char* pInput, char* pOutput)
```
参数 | 描述   
:----------|:----------
*pInput | utf8字符指针
*pOutput | unicode字符指针 
**返回** | **描述**   
int | unicode字符大小

#### unicode转gbk
```c
void unicode2gbk(uint8_t *pInput, uint8_t *pOutput);
```
参数 | 描述   
:----------|:----------
*pInput | unicode字符指针
*pOutput | gbk字符指针 
**返回** | **描述**   
void | 无

## 4.注意事项
**Chinese_font_library**基于fal软件包实现，使用之前需要做一些准备工作：
* 因为要使用fal读外部Flash，所以要对Flash进行移植工作，参考：[Flash 设备及分区移植示例](https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/README.md)
* 在fal_cfg.h的分区表中为外部Flash划出"font"分区，例：
```c
{FAL_PART_MAGIC_WROD,       "font",    "nor_flash",            (512 + 1024 + 512) * 1024,  7 * 1024 * 1024, 0}
```
* 将字库文件烧写进"font"分区。
* 使用前需调用fal_init()初始化fal库。

## 5.联系方式
* 维护：lxzzzzzxl
* 主页：https://github.com/lxzzzzzxl/Chinese_font_library