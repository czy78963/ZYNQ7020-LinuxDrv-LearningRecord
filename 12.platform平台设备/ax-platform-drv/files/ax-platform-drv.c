/** ===================================================== **
 *Author : ALINX Electronic Technology (Shanghai) Co., Ltd.
 *Website: http://www.alinx.com
 *Address: Room 202, building 18, 
           No.518 xinbrick Road, 
           Songjiang District, Shanghai
 *Created: 2020-3-2 
 *Version: 1.0
 ** ===================================================== **/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
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

/* 设备节点名称 */  
#define DEVICE_NAME       "gpio_leds"
/* 设备号个数 */  
#define DEVID_COUNT       1
/* 驱动个数 */  
#define DRIVE_COUNT       1
/* 主设备号 */
#define MAJOR
/* 次设备号 */
#define MINOR             0
 
/* gpio寄存器虚拟地址 */  
static u32 *GPIO_DIRM_0; 
/* gpio使能寄存器 */   
static u32 *GPIO_OEN_0;
/* gpio控制寄存器 */  
static u32 *GPIO_DATA_0;
/* AMBA外设时钟使能寄存器 */  
static u32 *APER_CLK_CTRL;

/* 把驱动代码中会用到的数据打包进设备结构体 */
struct alinx_char_dev{
    dev_t              devid;      //设备号
    struct cdev        cdev;       //字符设备
    struct class       *class;     //类
    struct device      *device;    //设备
};
/* 声明设备结构体 */
static struct alinx_char_dev alinx_char = {
    .cdev = {
        .owner = THIS_MODULE,
    },
};

/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int gpio_leds_open(struct inode *inode_p, struct file *file_p)  
{  
    /* 设置私有数据 */
    file_p->private_data = &alinx_char;    
      
    return 0;  
}  

/* write函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t gpio_leds_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    int rst;  
    char writeBuf[5] = {0};  
  
    rst = copy_from_user(writeBuf, buf, len);  
    if(0 != rst)  
    {  
        return -1;    
    }  
      
    if(1 != len)  
    {  
        printk("gpio_test len err\n");  
        return -2;  
    }  
    if(1 == writeBuf[0])  
    {  
        *GPIO_DATA_0 &= 0xFFFFFFFE;
    }  
    else if(0 == writeBuf[0])  
    {  
        *GPIO_DATA_0 |= 0x00000001;
    }  
    else  
    {  
        printk("gpio_test para err\n");  
        return -3;  
    }  
      
    return 0;  
}  

/* release函数实现, 对应到Linux系统调用函数的close函数 */  
static int gpio_leds_release(struct inode *inode_p, struct file *file_p)  
{   
    return 0;  
}  

/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations ax_char_fops = {  
    .owner   = THIS_MODULE,  
    .open    = gpio_leds_open,  
    .write   = gpio_leds_write,     
    .release = gpio_leds_release,   
};

/* probe函数实现, 驱动和设备匹配时会被调用 */
static int gpio_leds_probe(struct platform_device *dev)
{
    /* 资源大小 */
    int regsize[4];
    /* 资源信息 */
    struct resource *led_source[4];
    
    int i;
    for(i = 0; i < 4; i ++)
    {
        /* 获取dev中的IORESOURCE_MEM资源 */
        led_source[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
        /* 返回NULL获取资源失败 */
        if(!led_source[i])
        {
            dev_err(&dev->dev, "get resource %d failed\r\n", i);
            return -ENXIO;
        }
        /* 获取当前资源大小 */
        regsize[i] = resource_size(led_source[i]);
    }
    
    /* 把需要修改的物理地址映射到虚拟地址 */
    GPIO_DIRM_0   = ioremap(led_source[0]->start, regsize[0]);  
    GPIO_OEN_0    = ioremap(led_source[1]->start, regsize[1]);
    GPIO_DATA_0   = ioremap(led_source[2]->start, regsize[2]);
    APER_CLK_CTRL = ioremap(led_source[3]->start, regsize[3]);
 
	/* MIO_0时钟使能 */  
    *APER_CLK_CTRL |= 0x00400000; 
    /* MIO_0设置成输出 */  
    *GPIO_DIRM_0 |= 0x00000001;  
    /* MIO_0使能 */  
    *GPIO_OEN_0 |= 0x00000001; 
    
    /* 注册设备号 */
    alloc_chrdev_region(&alinx_char.devid, MINOR, DEVID_COUNT, DEVICE_NAME);
    
    /* 初始化字符设备结构体 */
    cdev_init(&alinx_char.cdev, &ax_char_fops);
    
    /* 注册字符设备 */
    cdev_add(&alinx_char.cdev, alinx_char.devid, DRIVE_COUNT);
    
    /* 创建类 */
    alinx_char.class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(alinx_char.class)) 
    {
        return PTR_ERR(alinx_char.class);
    }
    
    /* 创建设备节点 */
    alinx_char.device = device_create(alinx_char.class, NULL, 
                                      alinx_char.devid, NULL, 
                                      DEVICE_NAME);
    if (IS_ERR(alinx_char.device)) 
    {
        return PTR_ERR(alinx_char.device);
    }
    
    return 0;
}

static int gpio_leds_remove(struct platform_device *dev)
{
    /* 注销字符设备 */
    cdev_del(&alinx_char.cdev);
    
    /* 注销设备号 */
    unregister_chrdev_region(alinx_char.devid, DEVID_COUNT);
    
    /* 删除设备节点 */
    device_destroy(alinx_char.class, alinx_char.devid);
    
    /* 删除类 */
    class_destroy(alinx_char.class);
   
    /* 释放对虚拟地址的占用 */  
    iounmap(GPIO_DIRM_0);
    iounmap(GPIO_OEN_0);
    iounmap(GPIO_DATA_0);
    return 0;
}

/* 声明并初始化platform驱动 */
static struct platform_driver led_driver = {
    .driver = {
        /* 将会用name字段和设备匹配, 这里name命名为alinx-led */
        .name = "alinx-led",
    },
    .probe  = gpio_leds_probe,
    .remove = gpio_leds_remove,
};

/* 驱动入口函数 */
static int __init gpio_led_drv_init(void)
{
    /* 在入口函数中调用platform_driver_register, 注册platform驱动 */
    return platform_driver_register(&led_driver);
}

/* 驱动出口函数 */
static void __exit gpio_led_dev_exit(void)
{
    /* 在出口函数中调用platform_driver_register, 卸载platform驱动 */
    platform_driver_unregister(&led_driver);
}

/* 标记加载、卸载函数 */ 
module_init(gpio_led_drv_init);
module_exit(gpio_led_dev_exit);

/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_led");  
MODULE_DESCRIPTION("PLATFORM LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL"); 









































