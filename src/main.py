import numpy as np
import matplotlib.pyplot as plt
import os 
from time import sleep
RUTACDD = "/dev/rpi_gpio"
TIEMPO_SENSADO = 5
def graficar_signal(muestras):

     # Convertir la cadena de bits en una lista de enteros
    signal = [int(bit) for bit in muestras]
    
    # Generar el eje de tiempo basado en el intervalo de 0.1 segundos
    time = np.linspace(0, 0.1 * len(signal), len(signal))
    
    # Graficar la señal
    plt.step(time, signal, where='post') 
    plt.title("Señal Binaria")
    plt.xlabel("Tiempo (s)")
    plt.ylabel("Amplitud")
    plt.grid()
    plt.savefig("grafica.png")

def elegir_signal(opcion):
    os.system("echo " + "'" + opcion + "'" +  "> "+ RUTACDD)

def escribir_gpio(valor: str):
    try:
        with open("/dev/rpi_gpio", "w") as f:
            print("leyendo señal")
            f.write(valor)
            f.flush()   # asegurar que se manda al dispositivo
    except PermissionError:
        print("¡Permisos insuficientes! Prueba con sudo o ajusta los permisos de /dev/rpi_gpio")

def leer_gpio() -> str:
    try:
        with open("/dev/rpi_gpio", "r") as f:
            estado = f.read().strip()
        return estado
    except FileNotFoundError:
        print("No existe /dev/rpi_gpio. ¿Está cargado tu driver?")
        return ""
    except PermissionError:
        print("¡Permisos insuficientes para leer /dev/rpi_gpio!")
        return ""

def recibir_signal(opcion):
    escribir_gpio(opcion)
    muestras = leer_gpio()  
    return muestras

def main():
    opcion = input("¿Qué señal se desea sensar? (0, 1, exit): ")
    while opcion != "exit":
        if opcion in ("0", "1"):
            #elegir_signal(opcion)
            graficar_signal(recibir_signal(opcion))
            print("Gráfica guardada en grafica.png")
        else:
            print("Opción inválida. Por favor, ingresa '0', '1' o 'exit'.")
        
        opcion = input("¿Qué señal quieres sensar? (0, 1, exit): ")


if __name__ == "__main__":
    main()
