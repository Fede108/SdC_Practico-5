import numpy as np
import matplotlib.pyplot as plt
import os 
from time import sleep
RUTACDD = "./cdd.txt"
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

def recibir_signal():
    try:
        with open(RUTACDD, "r") as file:
            muestras = file.read().strip()  # Leer el contenido y eliminar espacios en blanco
        return muestras
    except FileNotFoundError:
        print(f"El archivo {RUTACDD} no existe.")
        return None

def main():
    opcion = input("¿Qué señal se desea sensar? (0, 1, exit): ")
    while opcion != "exit":
        if opcion in ("0", "1"):
            #elegir_signal(opcion)
            graficar_signal(recibir_signal())
            print("Gráfica guardada en grafica.png")
        else:
            print("Opción inválida. Por favor, ingresa '0', '1' o 'exit'.")
        
        opcion = input("¿Qué señal quieres sensar? (0, 1, exit): ")



if __name__ == "__main__":
    main()
