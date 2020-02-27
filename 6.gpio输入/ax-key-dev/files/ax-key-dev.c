#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
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
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/mach/map.h>
#include <asm/io.h>
  
/* 设备节点名称 */  
#define DEVICE_NAME       "gpio_key"
/* 设备号个数 */  
#define DEVID_COUNT       1
/* 驱动个数 */  
#define DRIVE_COUNT       1
/* 主设备号 */
#define MAJOR1
/* 次设备号 */
#define MINOR1            0

/* 把驱动代码中会用到的数据打包进设备结构体 */
struct alinx_char_dev{
    dev_t              devid;             //设备号
    struct cdev        cdev;              //字符设备
    struct class       *class;            //类
    struct device      *device;           //设备
    struct device_node *nd;               //设备树的设备节点
    int                alinx_key_gpio;    //gpio号
};
/* 声明设备结构体 */
static struct alinx_char_dev alinx_char = {
    .cdev = {
        .owner = THIS_MODULE,
    },
};

/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int gpio_key_open(struct inode *inode_p, struct file *file_p)  
{
    /* 设置私有数据 */
    file_p->private_data = &alinx_char;
    printk("gpio_test module open\n");  
    return 0;  
}  
  
  
/* write函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t gpio_key_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    int ret = 0;
    /* 返回按键的值 */
	unsigned int key_value = 0;
    /* 获取私有数据 */
    struct alinx_char_dev *dev = file_p->private_data;
  
    /* 检查按键是否被按下 */
    if(0 == gpio_get_value(dev->alinx_key_gpio))
    {
        /* 按键被按下 */
        /* 防抖 */
        mdelay(50);
        /* 等待按键抬起 */
        while(!gpio_get_value(dev->alinx_key_gpio));
        key_value = 1;
    }
    else
    {
        /* 按键未被按下 */
    }
    /* 返回按键状态 */
    ret = copy_to_user(buf, &key_value, sizeof(key_value));
    
    return ret;  
}  
  
/* release函数实现, 对应到Linux系统调用函数的close函数 */  
static int gpio_key_release(struct inode *inode_p, struct file *file_p)  
{  
    printk("gpio_test module release\n");  
    return 0;  
}  
      
/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations ax_char_fops = {  
    .owner   = THIS_MODULE,  
    .open    = gpio_key_open,  
    .read    = gpio_key_read,     
    .release = gpio_key_release,   
};  
  
/* 模块加载时会调用的函数 */  
static int __init gpio_key_init(void)  
{
    /* 用于接受返回值 */
    u32 ret = 0;
    
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
    
    /* 注册设备号 */
    alloc_chrdev_region(&alinx_char.devid, MINOR1, DEVID_COUNT, DEVICE_NAME);
    
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
static void __exit gpio_key_exit(void)  
{  
    /* 释放gpio */
    gpio_free(alinx_char.alinx_key_gpio);

    /* 注销字符设备 */
    cdev_del(&alinx_char.cdev);
    
    /* 注销设备号 */
    unregister_chrdev_region(alinx_char.devid, DEVID_COUNT);
    
    /* 删除设备节点 */
    device_destroy(alinx_char.class, alinx_char.devid);
    
    /* 删除类 */
    class_destroy(alinx_char.class);
       
    printk("gpio_key_dev_exit_ok\n");  
}  
  
/* 标记加载、卸载函数 */  
module_init(gpio_key_init);  
module_exit(gpio_key_exit);  
  
/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_key");  
MODULE_DESCRIPTION("GPIO OUT driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  










