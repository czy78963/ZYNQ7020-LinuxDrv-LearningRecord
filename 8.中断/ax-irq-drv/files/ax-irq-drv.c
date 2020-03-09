#include <linux/module.h>  
#include <linux/kernel.h>
#include <linux/init.h>  
#include <linux/ide.h>  
#include <linux/types.h>  
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/mach/map.h>
#include <asm/io.h>
  
/* 设备节点名称 */  
#define DEVICE_NAME       "interrupt_led"
/* 设备号个数 */  
#define DEVID_COUNT       1
/* 驱动个数 */  
#define DRIVE_COUNT       1
/* 主设备号 */
#define MAJOR_U
/* 次设备号 */
#define MINOR_U           0

/* 把驱动代码中会用到的数据打包进设备结构体 */
struct alinx_char_dev {
/** 字符设备框架 **/
    dev_t              devid;             //设备号
    struct cdev        cdev;              //字符设备
    struct class       *class;            //类
    struct device      *device;           //设备
    struct device_node *nd;               //设备树的设备节点
/** 并发处理 **/
    spinlock_t         lock;              //自旋锁变量
/** gpio **/    
    int                alinx_key_gpio;    //gpio号
    int                key_sts;           //记录按键状态, 为1时被按下
/** 中断 **/
    unsigned int       irq;               //中断号
/** 定时器 **/
    struct timer_list  timer;             //定时器
};
/* 声明设备结构体 */
static struct alinx_char_dev alinx_char = {
    .cdev = {
        .owner = THIS_MODULE,
    },
};

/** 回掉 **/
/* 中断服务函数 */
static irqreturn_t key_handler(int irq, void *dev)
{
    /* 按键按下或抬起时会进入中断 */
    /* 开启50毫秒的定时器用作防抖动 */
    mod_timer(&alinx_char.timer, jiffies + msecs_to_jiffies(50));
    return IRQ_RETVAL(IRQ_HANDLED);
}

/* 定时器服务函数 */
void timer_function(unsigned long arg)
{
    unsigned long flags;
    /* 获取锁 */
    spin_lock_irqsave(&alinx_char.lock, flags);

    /* value用于获取按键值 */
    unsigned char value;
    /* 获取按键值 */
    value = gpio_get_value(alinx_char.alinx_key_gpio);
    if(value == 0)
    {
        /* 按键按下, 状态置1 */
        alinx_char.key_sts = 1;
    }
    else
    {
        /* 按键抬起 */
    }
    
    /* 释放锁 */
    spin_unlock_irqrestore(&alinx_char.lock, flags);
}

/** 系统调用实现 **/
/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int char_drv_open(struct inode *inode_p, struct file *file_p)  
{  
    printk("gpio_test module open\n");  
    return 0;  
}  
  
  
/* read函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t char_drv_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    unsigned long flags;
    int ret;
    /* 获取锁 */
    spin_lock_irqsave(&alinx_char.lock, flags);
    
    /* keysts用于读取按键状态 */
    /* 返回按键状态值 */
    ret = copy_to_user(buf, &alinx_char.key_sts, sizeof(alinx_char.key_sts));
    /* 清除按键状态 */
    alinx_char.key_sts = 0;
    
    /* 释放锁 */
    spin_unlock_irqrestore(&alinx_char.lock, flags);
    return 0;  
}  
  
/* release函数实现, 对应到Linux系统调用函数的close函数 */  
static int char_drv_release(struct inode *inode_p, struct file *file_p)  
{  
    printk("gpio_test module release\n");
    return 0;  
}  
      
/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations ax_char_fops = {  
    .owner   = THIS_MODULE,  
    .open    = char_drv_open,  
    .read   = char_drv_read,     
    .release = char_drv_release,   
};  
  
/* 模块加载时会调用的函数 */  
static int __init char_drv_init(void)  
{
    /* 用于接受返回值 */
    u32 ret = 0;
    
/** 并发处理 **/
    /* 初始化自旋锁 */
    spin_lock_init(&alinx_char.lock);
    
/** gpio框架 **/   
    /* 获取设备节点 */
    alinx_char.nd = of_find_node_by_path("/alinxkey");
    if(alinx_char.nd == NULL)
    {
        printk("alinx_char node not find\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alinx_char node find\r\n");
    }
    
    /* 获取节点中gpio标号 */
    alinx_char.alinx_key_gpio = of_get_named_gpio(alinx_char.nd, "alinxkey-gpios", 0);
    if(alinx_char.alinx_key_gpio < 0)
    {
        printk("can not get alinxkey-gpios");
        return -EINVAL;
    }
    printk("alinxkey-gpio num = %d\r\n", alinx_char.alinx_key_gpio);
    
    /* 申请gpio标号对应的引脚 */
    ret = gpio_request(alinx_char.alinx_key_gpio, "alinxkey");
    if(ret != 0)
    {
        printk("can not request gpio\r\n");
        return -EINVAL;
    }
    
    /* 把这个io设置为输入 */
    ret = gpio_direction_input(alinx_char.alinx_key_gpio);
    if(ret < 0)
    {
        printk("can not set gpio\r\n");
        return -EINVAL;
    }

/** 中断 **/
    /* 获取中断号 */
    alinx_char.irq = gpio_to_irq(alinx_char.alinx_key_gpio);
    /* 申请中断 */
    ret = request_irq(alinx_char.irq,
                      key_handler,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      "alinxkey", 
                      NULL);
    if(ret < 0)
    {
        printk("irq %d request failed\r\n", alinx_char.irq);
        return -EFAULT;
    }
    
/** 定时器 **/
    alinx_char.timer.function = timer_function;
    init_timer(&alinx_char.timer);

/** 字符设备框架 **/    
    /* 注册设备号 */
    alloc_chrdev_region(&alinx_char.devid, MINOR_U, DEVID_COUNT, DEVICE_NAME);
    
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

/* 卸载模块 */  
static void __exit char_drv_exit(void)  
{  
/** gpio **/
    /* 释放gpio */
    gpio_free(alinx_char.alinx_key_gpio);

/** 中断 **/
    /* 释放中断 */
    free_irq(alinx_char.irq, NULL);

/** 定时器 **/
    /* 删除定时器 */   
    del_timer_sync(&alinx_char.timer);

/** 字符设备框架 **/
    /* 注销字符设备 */
    cdev_del(&alinx_char.cdev);
    
    /* 注销设备号 */
    unregister_chrdev_region(alinx_char.devid, DEVID_COUNT);
    
    /* 删除设备节点 */
    device_destroy(alinx_char.class, alinx_char.devid);
    
    /* 删除类 */
    class_destroy(alinx_char.class);
    
    printk("timer_led_dev_exit_ok\n");  
}  
  
/* 标记加载、卸载函数 */  
module_init(char_drv_init);  
module_exit(char_drv_exit);  
  
/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("alinx char");  
MODULE_DESCRIPTION("INTERRUPT LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  











