# SdC\_Practico-5

Este repositorio contiene el desarrollo de un trabajo práctico para la materia **Sistemas de Computacion**, centrado en la interacción con GPIOs y señales mediante un **driver de carácter** en el espacio de kernel y scripts en Python en el espacio de usuario.

## Estructura del proyecto

```
SdC_Practico-5/
│
├── README.md                
├── signals.py               ← Script en Python para manejar señales del sistema
│
├── src/                     ← Código fuente de alto nivel
│   ├── main.py              ← Script principal en Python para ejecutar el proyecto
│   └── grafica.png          ← Imagen de referencia o resultado gráfico del proyecto
│
├── gpio_driver/             ← Módulo del kernel para manejo de GPIOs
    ├── drv.c                ← Código fuente del driver de GPIO (driver de carácter)
    ├── Makefile             ← Makefile para compilar el módulo
    ├── dt_gpio.dts          ← Archivo de árbol de dispositivo para el módulo GPIO
    ├── dt_gpio.dtbo         ← Versión compilada del árbol de dispositivo
    ├── drv.ko               ← Módulo del kernel compilado
    └── *                    ← Archivos intermedios y objetos generados durante la compilación


```

## Descripción general

Este proyecto demuestra cómo interactuar con pines GPIO mediante un **driver de carácter** personalizado cargado como módulo del kernel. La interacción se realiza desde espacio de usuario mediante scripts en Python, que acceden al archivo de dispositivo correspondiente.

* El módulo en `gpio_driver/` permite controlar GPIOs mediante operaciones de lectura/escritura sobre un dispositivo de carácter.
* `signals.py` permite simular señales que llegan a los pines GPIOs en un entorno emulado y `src/main.py` permite usar estos recursos desde espacio de usuario.
* El árbol de dispositivo (`dt_gpio.dts`) configura el hardware en plataformas embebidas compatibles.

## Uso

### 1. Compilar el módulo del kernel

```bash
cd gpio_driver
make
```

### 2. Aplicar el Device Tree Overlay (`dt_overlay`)

Esto es útil para plataformas como Raspberry Pi que usan overlays para habilitar/controlar hardware desde el arranque.

#### Compilar el archivo `.dts` a `.dtbo` (si no está generado):

```bash
dtc -O dtb -o dt_gpio.dtbo -b 0 -@ dt_gpio.dts
```

#### Agregar el `.dtbo` al sistema:

```bash
sudo dtoverlay dt_gpio.dtbo 
```


### 3. Cargar el módulo del kernel:

```bash
sudo insmod drv.ko
```

Verificar con:

```bash
dmesg | tail
```

### 4. Ejecutar los scripts en Python

```bash
python3 src/main.py
```

## Requisitos

* Sistema Linux con soporte para carga de módulos (por ejemplo, Raspberry Pi)
* Python 3
* `device-tree-compiler` (`dtc`)
* Permisos de administrador

