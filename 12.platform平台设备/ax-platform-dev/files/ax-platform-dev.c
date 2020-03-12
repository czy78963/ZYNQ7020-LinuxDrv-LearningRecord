/** ===================================================== **
 *Author : ALINX Electronic Technology (Shanghai) Co., Ltd.
 *Website: http://www.alinx.com
 *Address: Room 202, building 18, 
           No.518 xinbrick Road, 
           Songjiang District, Shanghai
 *Created: 2020-3-2 
 *Version: 1.0
 ** ===================================================== **/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 寄存器首地址 */
/* gpio方向寄存器 */ 
#define GPIO_DIRM_0       0xE000A204
/* gpio使能寄存器 */ 
#define GPIO_OEN_0        0xE000A208
/* gpio控制寄存器 */ 
#define GPIO_DATA_0       0xE000A040
/* AMBA外设时钟使能寄存器 */  
#define APER_CLK_CTRL     0xF800012C
/* 寄存器大小 */
#define REGISTER_LENGTH   4

/* 删除设备时会执行此函数 */
static void led_release(struct device *dev)
{
    printk("led device released\r\n");
}

/* 初始化LED的设备信息, 即寄存器信息 */
static struct resource led_resources[] = 
{
    {
        .start = GPIO_DIRM_0,
        .end   = GPIO_DIRM_0 + REGISTER_LENGTH - 1,
        /* 寄存器当作内存处理 */
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO_OEN_0,
        .end   = GPIO_OEN_0 + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO_DATA_0,
        .end   = GPIO_DATA_0 + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = APER_CLK_CTRL,
        .end   = APER_CLK_CTRL + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
};

/* 声明并初始化platform_device */
static struct platform_device led_device = 
{
    /* 名字和driver中的name一致 */
    .name = "alinx-led",
    /* 只有一个设备 */
    .id = -1,
    .dev = {
        /* 设置release函数 */
        .release = &led_release,
     },
    /* 设置资源个数 */
    .num_resources = ARRAY_SIZE(led_resources),
    /* 设置资源信息 */
    .resource = led_resources,
};

/* 入口函数 */
static int __init led_device_init(void)
{
    /* 在入口函数中调用platform_driver_register, 注册platform驱动 */
    return platform_device_register(&led_device);
}

/* 出口函数 */
static void __exit led_device_exit(void)
{
    /* 在出口函数中调用platform_driver_register, 卸载platform驱动 */
    platform_device_unregister(&led_device);
}

/* 标记加载、卸载函数 */ 
module_init(led_device_init);
module_exit(led_device_exit);

/* 驱动描述信息 */
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_led");  
MODULE_DESCRIPTION("PLATFORM LED device");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL"); 




























































