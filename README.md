# SdC\_Practico-5

Este repositorio contiene la implementación de la **segunda parte** del Trabajo Práctico 5 de la materia Sistemas de Computación (Año 2025). Se desarrolla un **Character Device Driver (CDD)** personalizado para sensar dos señales externas y una aplicación de usuario para seleccionar la señal a muestrear y graficar su evolución en el tiempo.

---

## Estructura del proyecto

```
SdC_Practico-5/
├── README.md                ← Este archivo
├── signals.py               ← Script usuario para simular/enviar señales a GPIOs
│
├── src/                     ← Aplicación de usuario
│   ├── main.py              ← Interacción con driver y graficación
│   └── grafica.png          ← Salida gráfica
│
├── gpio_driver/             ← Driver del kernel y overlay
│   ├── drv.c                ← Código fuente del driver
│   ├── Makefile             ← Compilación del módulo
│   ├── dt_gpio.dts          ← Device Tree Overlay fuente
│   ├── dt_gpio.dtbo         ← Overlay compilado
│   ├── drv.ko               ← Módulo del kernel compilado
│   └── <archivos intermedios>
└── .git/                    ← Carpeta interna de Git (no modificar)
```
## Descripción general

Este proyecto demuestra cómo interactuar con pines GPIO mediante un **driver de carácter** personalizado cargado como módulo del kernel. La interacción se realiza desde espacio de usuario mediante scripts en Python, que acceden al archivo de dispositivo correspondiente.

* El módulo en `gpio_driver/` permite controlar GPIOs mediante operaciones de lectura/escritura sobre un dispositivo de carácter.
* `signals.py` permite simular señales que llegan a los pines GPIOs en un entorno emulado y `src/main.py` permite usar estos recursos desde espacio de usuario.
* El árbol de dispositivo (`dt_gpio.dts`) configura el hardware en plataformas embebidas compatibles.

### Estructura del driver

#### Platform Driver

* Se vincula vía Device Tree (compatible = "gpio,los-debugueadores").
* En `probe()`: obtiene GPIOs con `devm_gpiod_get_index()`.

#### Character Device

* Interfaz en `/dev/rpi_gpio`.
* `my_write()`: selecciona GPIO a muestrear (0 o 1), inicia muestreo (100 lecturas).
* `my_read()`: entrega el contenido de `sample_buf` al usuario (por ej. "0110101101").
* `open()` y `release()`: operaciones básicas.

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

### Estructura del driver

#### Platform Driver

* Se vincula vía Device Tree (compatible = "gpio,los-debugueadores").
* En `probe()`: obtiene GPIOs con `devm_gpiod_get_index()`.

#### Character Device

* Interfaz en `/dev/rpi_gpio`.
* `my_write()`: selecciona GPIO a muestrear (0 o 1), inicia muestreo (10 lecturas con `msleep(100)`).
* `my_read()`: entrega el contenido de `sample_buf` al usuario (por ej. "0110101101").
* `open()` y `release()`: operaciones básicas.

### Overlay

* `dt_gpio.dts`: define nodo con GPIOs, por ejemplo:

  ```dts
  entrada-gpios = <&gpio 17 GPIO_ACTIVE_HIGH>, <&gpio 27 GPIO_ACTIVE_HIGH>;
  ```
* Compilar con:

  ```bash
  dtc -O dtb -o dt_gpio.dtbo -b 0 -@ dt_gpio.dts
  ```
* Copiar a `/boot/overlays/` y agregar en `config.txt`:

  ```ini
  dtoverlay=dt_gpio
  ```

### Compilación

```bash
cd gpio_driver
make
```

Genera `drv.ko`. Para cargar:

```bash
sudo insmod drv.ko
```

Verificación:

```bash
dmesg | tail
```

---

## Simulación y prueba

* Entorno: QEMU emulando Raspberry Pi.
* Acceso vía SSH.
* `signals.py`: genera patrones de señal en GPIOs.
* `main.py`: selecciona señal, recupera muestras, grafica.


## Flujo de uso sugerido

1. Iniciar Raspberry (emulada o real).
2. Copiar `drv.ko` y `dt_gpio.dtbo`.
3. Configurar Device Tree Overlay.
4. Cargar driver con `insmod`.
5. Ejecutar `signals.py` para generar señal.
6. Ejecutar `main.py` para muestrear y graficar.
7. Ver `grafica.png`.

## Requisitos

* Sistema Linux con soporte para carga de módulos (por ejemplo, Raspberry Pi)
* Python 3
* `device-tree-compiler` (`dtc`)
* Permisos de administrador



