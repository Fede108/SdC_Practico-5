#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/uaccess.h>


#define DEVICE_NAME "rpi_gpio"
#define CLASS_NAME "my_gpio"
#define BUFFER_SIZE 64

static dev_t devnum; 		// Global variable for the first device number
static struct cdev c_dev; 	// Global variable for the character device structure
static struct class *cl; 	// Global variable for the device class


//class_create() /* creates a class for your devices visible in /sys/class/ */
//class_destroy() /* removes the class */
//device_create() /* creates a device node in the /dev directory */
//device_destroy() /* removes a device node in the /dev directory */

/* gpios variables*/
struct gpio_desc *in0, *in1;


/* Buffer para muestrear la señal */
static unsigned char sample_buf[BUFFER_SIZE];
static size_t buf_len = BUFFER_SIZE;


/* declarations probe and remove fuctions */
static int my_driver_probe(struct platform_device *pdev);
static int my_driver_remove(struct platform_device *pdev);

static const struct of_device_id my_drivers_ids[] = {
    { .compatible = "gpio,los-debugueadores" },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, my_drivers_ids);


static struct platform_driver my_driver = {
    .driver = {
        .name = "my_gpio_driver",
        .of_match_table = my_drivers_ids,
    },
    .probe  = my_driver_probe,
    .remove = my_driver_remove,
};


/* probe and remove fuctions */
static int my_driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    /* obtener GPIO[0] (primer pin en entrada-gpios) */
    in0 = devm_gpiod_get_index(dev, "entrada", 0, GPIOD_IN);
    if (IS_ERR(in0)) {
        dev_err(dev, "No se pudo obtener entrada 0\n");
        return PTR_ERR(in0);
    }

    /* obtener GPIO[1] (segundo pin en entrada-gpios) */
    in1 = devm_gpiod_get_index(dev, "entrada", 1, GPIOD_IN);
    if (IS_ERR(in1)) {
        dev_err(dev, "No se pudo obtener entrada 1\n");
        return PTR_ERR(in1);
    }

    dev_info(dev, "GPIOs de entrada inicializados correctamente\n");
    return 0;
}

static int my_driver_remove(struct platform_device *pdev)
{
    /* Con devm_gpiod_get_index el kernel libera todo al remover, no hace falta nada */
    return 0;
}


static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO DEVICE_NAME": close()\n");
    return 0;
}

static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO DEVICE_NAME": open()\n");
    return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    unsigned char user_val;
    struct gpio_desc *active_gpio;
    size_t i;

    /* esperamos 1 byte con 0 o 1 */
    if (len < 1)
        return -EINVAL;

    if (copy_from_user(&user_val, buf, 1))
        return -EFAULT;

    switch (user_val) {
        case '0':
            active_gpio = in0;
            break;
        case '1':
            active_gpio = in1;
            break;
        default:
            /* valor no válido, lo ignoramos o devolvemos error */
            return -EINVAL;
    }

    /* Muestreamos BUFFER_SIZE veces rápidamente */
    for (i = 0; i < BUFFER_SIZE; i++) {
        sample_buf[i] = gpiod_get_value(active_gpio) ? '1' : '0';
        //udelay(10);
    }

    return len;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    ssize_t to_copy = min(buf_len - *off, len);

    if (to_copy <= 0)
        return 0;

    /* read data from kernel buffer to user buffer */
    if (copy_to_user(buf, sample_buf + *off, to_copy))
        return -EFAULT;

    *off += to_copy;
    return to_copy;
}


static struct file_operations pugs_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init drv_init(void)
{
    int ret;
    struct device *dev_ret;

    if ((ret = alloc_chrdev_region(&devnum, 0, 1, DEVICE_NAME)) < 0)
        return ret;

    cl = class_create(CLASS_NAME);
    if (IS_ERR(cl)) {
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(cl);
    }

    dev_ret = device_create(cl, NULL, devnum, NULL, DEVICE_NAME);
    if (IS_ERR(dev_ret)) {
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &pugs_fops);
    if ((ret = cdev_add(&c_dev, devnum, 1)) < 0) {
        device_destroy(cl, devnum);
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return ret;
    }

    if ((ret = platform_driver_register(&my_driver)) < 0) {
        cdev_del(&c_dev);
        device_destroy(cl, devnum);
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return ret;
    }

    printk(KERN_INFO DEVICE_NAME ": Registrado exitosamente\n");
    return 0;
}

static void __exit drv_exit(void) /* Destructor */
{
    cdev_del(&c_dev);
    device_destroy(cl, devnum);
    class_destroy(cl);
    unregister_chrdev_region(devnum, 1);
    platform_driver_unregister(&my_driver);
    printk(KERN_INFO "Rpi_gpio_driver: dice Adios mundo cruel..!!\n");
}

module_init(drv_init);
module_exit(drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Los debugueadores");
MODULE_DESCRIPTION("Nuestro primer gpio driver");