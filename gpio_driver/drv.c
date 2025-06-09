#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DEVICE_NAME "rpi_gpio"
#define CLASS_NAME "my_gpio"
#define BUFFER_SIZE 100

static dev_t devnum;         // Número de dispositivo (major, minor)
static struct cdev c_dev;    // Estructura del dispositivo de caracteres
static struct class *cl;     // Clase para sysfs (/sys/class)

/* Variables para los GPIOs de entrada */
struct gpio_desc *in0, *in1;

/* Buffer para muestrear la señal */
static unsigned char sample_buf[BUFFER_SIZE];
static size_t buf_len = BUFFER_SIZE;

/* Declaraciones de funciones probe y remove para el platform driver */
static int my_driver_probe(struct platform_device *pdev);
static int my_driver_remove(struct platform_device *pdev);

/*
 * Tabla de compatibilidad Device Tree:
 * Asociamos el "compatible" definido en el DT con este driver.
 * Cuando el kernel encuentre un dispositivo con compatible = "gpio,los-debugueadores",
 * invocará la función probe de este driver.
 */
static const struct of_device_id my_drivers_ids[] = {
    { .compatible = "gpio,los-debugueadores" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_drivers_ids);

/*
 * Estructura del platform_driver:
 * Define nombre, tabla de compatibilidad y punteros a funciones probe/remove.
 * El kernel registra este driver y lo relaciona con dispositivos platform
 * definidos (e.g., en Device Tree) que coincidan con of_match_table.
 */
static struct platform_driver my_driver = {
    .driver = {
        .name = "my_gpio_driver",
        .of_match_table = my_drivers_ids,
    },
    .probe  = my_driver_probe,
    .remove = my_driver_remove,
};

/*
 * La función probe se llama cuando el driver se enlaza con un dispositivo:
 * - pdev contiene la información del dispositivo platform.
 * - Se obtienen los GPIOs definidos en el DT bajo "entrada-gpios".
 */
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

/*
 * La función remove se invoca al desacoplar el driver del dispositivo:
 * Con devm_*, el kernel libera automáticamente los recursos, así que no
 * se requiere código específico aquí.
 */
static int my_driver_remove(struct platform_device *pdev)
{
    return 0;
}

/*
 * Operaciones de archivo para comunicación con user-space:
 * - open: abre el dispositivo
 * - close/release: cierra el dispositivo
 * - read: lee datos desde el buffer del kernel hacia user-space
 * - write: recibe comando y muestrea GPIO
 */

/* release/close: Invocado cuando se cierra /dev/rpi_gpio */
static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO DEVICE_NAME": close()\n");
    return 0;
}

/* open: Invocado cuando se abre /dev/rpi_gpio en user-space */
static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO DEVICE_NAME": open()\n");
    return 0;
}

/* write: Recibe '0' o '1' desde user-space y llena el buffer sample_buf con lecturas GPIO */
static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    unsigned char user_val;
    struct gpio_desc *active_gpio;
    size_t i;

    /* esperamos 1 byte con '0' o '1' */
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
            return -EINVAL;
    }

    /* muestreamos BUFFER_SIZE veces */
    for (i = 0; i < BUFFER_SIZE; i++) {
        sample_buf[i] = gpiod_get_value(active_gpio) ? '1' : '0';
        usleep_range(10000, 11000);
    }

    return len;
}

/* read: Copia datos de sample_buf hacia user-space, soportando lecturas parciales */
static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    ssize_t to_copy = min_t(ssize_t, buf_len - *off, len);

    if (to_copy <= 0)
        return 0;

    if (copy_to_user(buf, sample_buf + *off, to_copy))
        return -EFAULT;

    *off += to_copy;
    return to_copy;
}

/* Asociamos las operaciones de archivo al driver de caracteres */
static struct file_operations pugs_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_close,
    .read    = my_read,
    .write   = my_write,
};

/*
 * Función de inicialización del módulo:
 * - Reserva region de major/minor
 * - Crea clase y device node en /dev
 * - Inicializa cdev
 * - Registra el platform driver para conectar con DT
 */
static int __init drv_init(void)
{
    int ret;
    struct device *dev_ret;

     /* Asignar major y minor (devnum) para 1 dispositivo */
    if ((ret = alloc_chrdev_region(&devnum, 0, 1, DEVICE_NAME)) < 0)
        return ret;

    /* Crear clase para el dispositivo: aparece en /sys/class/my_gpio */
    cl = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(cl)) {
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(cl);
    }

    /* Crear dispositivo: crea /dev/rpi_gpio con los números asignados */
    dev_ret = device_create(cl, NULL, devnum, NULL, DEVICE_NAME);
    if (IS_ERR(dev_ret)) {
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(dev_ret);
    }

    /* Inicializar cdev con nuestras operaciones de archivo */
    cdev_init(&c_dev, &pugs_fops);
    if ((ret = cdev_add(&c_dev, devnum, 1)) < 0) {
        device_destroy(cl, devnum);
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return ret;
    }

    /* Registro del platform driver: vincula probe/remove con el kernel */
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

/*
 * Función de salida del módulo:
 * - Elimina cdev, device y class
 * - Libera region de major/minor
 * - Desregistra el platform driver
 */
static void __exit drv_exit(void)
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
