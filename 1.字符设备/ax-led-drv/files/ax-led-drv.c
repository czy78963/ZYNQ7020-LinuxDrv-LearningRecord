#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/ide.h>  
#include <linux/types.h>  
  
/* 驱动名称 */  
#define DEVICE_NAME       "gpio_leds"  
/* 驱动主设备号 */  
#define GPIO_LED_MAJOR    200  
  
/* gpio寄存器虚拟地址 */  
static unsigned int gpio_add_minor;  
/* gpio寄存器物理基地址 */  
#define GPIO_BASE         0xE000A000  
/* gpio寄存器所占空间大小 */  
#define GPIO_SIZE         0x1000  
/* gpio方向寄存器 */  
#define GPIO_DIRM_0       (unsigned int *)(0xE000A204 - GPIO_BASE + gpio_add_minor)  
/* gpio使能寄存器 */   
#define GPIO_OEN_0        (unsigned int *)(0xE000A208 - GPIO_BASE + gpio_add_minor)  
/* gpio控制寄存器 */  
#define GPIO_DATA_0       (unsigned int *)(0xE000A040 - GPIO_BASE + gpio_add_minor)  
  
/* 时钟使能寄存器虚拟地址 */  
static unsigned int clk_add_minor;  
/* 时钟使能寄存器物理基地址 */  
#define CLK_BASE          0xF8000000  
/* 时钟使能寄存器所占空间大小 */  
#define CLK_SIZE          0x1000  
/* AMBA外设时钟使能寄存器 */  
#define APER_CLK_CTRL     (unsigned int *)(0xF800012C - CLK_BASE + clk_add_minor)        
  
/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int gpio_leds_open(struct inode *inode_p, struct file *file_p)  
{  
	/* 把需要修改的物理地址映射到虚拟地址 */
	gpio_add_minor = (unsigned int)ioremap(GPIO_BASE, GPIO_SIZE);  
	clk_add_minor = (unsigned int)ioremap(CLK_BASE, CLK_SIZE); 

    /* MIO_0时钟使能 */  
    *APER_CLK_CTRL |= 0x00400000;  
    /* MIO_0设置成输出 */  
    *GPIO_DIRM_0 |= 0x00000001;  
    /* MIO_0使能 */  
    *GPIO_OEN_0 |= 0x00000001;  
      
    printk("gpio_test module open\n");  
      
    return 0;  
}  
  
  
/* write函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t gpio_leds_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    int rst;  
    char writeBuf[5] = {0};  
      
    printk("gpio_test module write\n");  
  
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
        printk("gpio_test ON\n");  
    }  
    else if(0 == writeBuf[0])  
    {  
        *GPIO_DATA_0 |= 0x00000001;  
        printk("gpio_test OFF\n");  
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
    /* MIO_0时钟失能 */  
    *APER_CLK_CTRL |= 0xFFBFFFFF;  
    /* MIO_0失能 */  
    *GPIO_OEN_0 &= 0xFFFFFFFE;  
       
    /* 释放对虚拟地址的占用 */  
    iounmap((unsigned int *)gpio_add_minor);  
    iounmap((unsigned int *)clk_add_minor);  
	
    printk("gpio_test module release\n");  
    return 0;  
}  
	  
/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations gpio_leds_fops = {  
    .owner   = THIS_MODULE,  
    .open    = gpio_leds_open,  
    .write   = gpio_leds_write,     
    .release = gpio_leds_release,   
};  
  
/* 模块加载时会调用的函数 */  
static int __init gpio_led_init(void)  
{  
    int ret;  
      
    /* 通过模块主设备号、名称、模块带有的功能函数(及file_operations结构体)来注册模块 */  
    ret = register_chrdev(GPIO_LED_MAJOR, DEVICE_NAME, &gpio_leds_fops);  
    if (ret < 0)   
    {  
        printk("gpio_led_dev_init_ng\n");  
        return ret;  
    }  
    else  
    {  
        /* 注册成功 */ 
        printk("gpio_led_dev_init_ok\n");  
    }  
    return 0;  
}  
  
/* 卸载模块 */  
static void __exit gpio_led_exit(void)  
{  
    /* 注销模块, 释放模块对这个设备号和名称的占用 */  
    unregister_chrdev(GPIO_LED_MAJOR, DEVICE_NAME);
	
    printk("gpio_led_dev_exit_ok\n");  
}  
  
/* 标记加载、卸载函数 */  
module_init(gpio_led_init);  
module_exit(gpio_led_exit);  
  
/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_led");  
MODULE_DESCRIPTION("GPIO LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  

