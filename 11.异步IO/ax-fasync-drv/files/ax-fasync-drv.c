/** ===================================================== **
 *Author : ALINX Electronic Technology (Shanghai) Co., Ltd.
 *Website: http://www.alinx.com
 *Address: Room 202, building 18, 
           No.518 xinbrick Road, 
           Songjiang District, Shanghai
 *Created: 2020-3-2 
 *Version: 1.0
 ** ===================================================== **/

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
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <asm/mach/map.h>
#include <asm/io.h>
  
/* 设备节点名称 */  
#define DEVICE_NAME       "fasync_led"
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
    dev_t                devid;             //设备号
    struct cdev          cdev;              //字符设备
    struct class         *class;            //类
    struct device        *device;           //设备
    struct device_node   *nd;               //设备树的设备节点
    int                  alinx_key_gpio;    //gpio号
    atomic_t             key_sts;           //记录按键状态, 为1时被按下
    unsigned int         irq;               //中断号
    struct timer_list    timer;             //定时器
    wait_queue_head_t    wait_q_h;          //等待队列头
    struct fasync_struct *fasync;           //异步信号
};
/* 声明设备结构体 */
static struct alinx_char_dev alinx_char = {
    .cdev = {
        .owner = THIS_MODULE,
    },
};

/* 中断服务函数 */
static irqreturn_t key_handler(int irq, void *dev_in)
{
    /* 按键按下或抬起时会进入中断 */
    struct alinx_char_dev *dev = (struct alinx_char_dev *)dev_in;
    /* 开启50毫秒的定时器用作防抖动 */
    dev->timer.data = (volatile long)dev_in;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(50));
    return IRQ_RETVAL(IRQ_HANDLED);
}

/* 定时器服务函数 */
void timer_function(unsigned long arg)
{
    struct alinx_char_dev *dev = (struct alinx_char_dev *)arg;
    /* value用于获取按键值 */
    unsigned char value;
    /* 获取按键值 */
    value = gpio_get_value(dev->alinx_key_gpio);
    if(value == 0)
    {
        /* 按键按下, 状态置1 */
        atomic_set(&dev->key_sts, 1);
        /* fasync有没有初始化过 */
        if(dev->fasync)
        {
            /* 初始化过说明应用程序调用过 */
            kill_fasync(&dev->fasync, SIGIO, POLL_OUT);
        }
        else if((current->state & TASK_INTERRUPTIBLE) != 0)
        {
            /* 是等待队列，需要唤醒进程 */
            wake_up_interruptible(&dev->wait_q_h);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* 按键抬起 */
    }
}

/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int char_drv_open(struct inode *inode_p, struct file *file_p)  
{  
    printk("gpio_test module open\n"); 
    file_p->private_data = &alinx_char;
    return 0;  
}  
  
/* read函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t char_drv_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    unsigned int keysts = 0;
    int ret;
    
    struct alinx_char_dev *dev = (struct alinx_char_dev *)file_p->private_data;
    
    /* 读取key的状态 */
    keysts = atomic_read(&dev->key_sts);
    /* 判断文件打开方式 */
    if(file_p->f_flags & O_NONBLOCK)
    {
        /* 如果是非阻塞访问, 说明已满足读取条件 */
    }
    /* 判断当前按键状态 */
    else if(!keysts)
    {
        /* 按键未被按下(数据未准备好) */
        /* 以当前进程创建并初始化为队列项 */
        DECLARE_WAITQUEUE(queue_mem, current);
        /* 把当前进程的队列项添加到队列头 */
        add_wait_queue(&dev->wait_q_h, &queue_mem);
        /* 设置当前进成为可被信号打断的状态 */
        __set_current_state(TASK_INTERRUPTIBLE);
        /* 切换进程, 是当前进程休眠 */
        schedule();
        
        /* 被唤醒, 修改当前进程状态为RUNNING */
        set_current_state(TASK_RUNNING);
        /* 把当前进程的队列项从队列头中删除 */
        remove_wait_queue(&dev->wait_q_h, &queue_mem);
        
        /* 判断是否是被信号唤醒 */
        if(signal_pending(current))
        {
            /* 如果是直接返回错误 */
            return -ERESTARTSYS;
        }
        else
        {
            /* 被按键唤醒 */
        }
    }
    else
    {
        /* 按键被按下(数据准备好了) */
    }    
      
    /* 读取key的状态 */
    keysts = atomic_read(&dev->key_sts);
    /* 返回按键状态值 */
    ret = copy_to_user(buf, &keysts, sizeof(keysts));
    /* 清除按键状态 */
    atomic_set(&dev->key_sts, 0);
    return 0;  
}  

/* poll函数实现 */  
unsigned int char_drv_poll(struct file *file_p, struct poll_table_struct *wait)
{
    unsigned int ret = 0;
    
    struct alinx_char_dev *dev = (struct alinx_char_dev *)file_p->private_data;
    
    /* 将应用程序添添加到等待队列中 */
    poll_wait(file_p, &dev->wait_q_h, wait);
    
    /* 判断key的状态 */
    if(atomic_read(&dev->key_sts))
    {
        /* key准备好了, 返回数据可读 */
        ret = POLLIN;
    }
    else
    {
        
    }
    
    return ret;
}

/* fasync函数实现 */
static int char_drv_fasync(int fd,struct file *file_p, int mode)
{
    struct alinx_char_dev *dev = (struct alinx_char_dev *)file_p->private_data;
    return fasync_helper(fd, file_p, mode, &dev->fasync);
}

/* release函数实现, 对应到Linux系统调用函数的close函数 */  
static int char_drv_release(struct inode *inode_p, struct file *file_p)  
{  
    printk("gpio_test module release\n");
    return char_drv_fasync(-1, file_p, 0);
}
      
/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations ax_char_fops = {  
    .owner   = THIS_MODULE,  
    .open    = char_drv_open,  
    .read    = char_drv_read,   
    .poll    = char_drv_poll,  
    .fasync  = char_drv_fasync,
    .release = char_drv_release,   
};  
  
/* 模块加载时会调用的函数 */  
static int __init char_drv_init(void)  
{
    /* 用于接受返回值 */
    u32 ret = 0;
    
    /* 初始化原子变量 */
    atomic_set(&alinx_char.key_sts, 0);
    
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

    /* 获取中断号 */
    alinx_char.irq = gpio_to_irq(alinx_char.alinx_key_gpio);
    /* 申请中断 */
    ret = request_irq(alinx_char.irq,
                      key_handler,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      "alinxkey", 
                      &alinx_char);
    if(ret < 0)
    {
        printk("irq %d request failed\r\n", alinx_char.irq);
        return -EFAULT;
    }
    
    alinx_char.timer.function = timer_function;
    init_timer(&alinx_char.timer);
    
    init_waitqueue_head(&alinx_char.wait_q_h);
    
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
    /* 释放gpio */
    gpio_free(alinx_char.alinx_key_gpio);

    /* 释放中断 */
    free_irq(alinx_char.irq, NULL);

    /* 删除定时器 */   
    del_timer_sync(&alinx_char.timer);

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
MODULE_DESCRIPTION("FASYNC LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  













