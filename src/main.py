import numpy as np
import matplotlib.pyplot as plt

RUTACDD = "/dev/rpi_gpio"

def graficar_signal(muestras,opcion):
    print("Cadena recibida:", repr(muestras))
    # Convertir la cadena de bits en una lista de enteros
    signal = [int(bit) for bit in muestras]
    print("Señal como ints: ", signal)
    
    # Generar el eje de tiempo basado en el intervalo de 0.1 segundos
    time = np.arange(0, len(signal)*0.01, 0.01)

    # Crear figura nueva
    plt.figure()
    # Graficar la señal
    plt.step(time, signal, where='post') 
    plt.title("Señal Binaria " + opcion)
    plt.xlabel("Tiempo (s)")
    plt.ylabel("Amplitud")
    plt.grid()
    plt.savefig("grafica.png")
    plt.close()


def escribir_gpio(valor: str):
    try:
        with open("/dev/rpi_gpio", "w") as f:
            print("leyendo señal")
            f.write(valor)
            f.flush()   
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
    while True:
        opcion = input("¿Qué señal se desea sensar? (0, 1, exit): ")
        if opcion == "exit":
            break

        if opcion in ("0", "1"):
            muestras = recibir_signal(opcion)
            if muestras:  
                graficar_signal(muestras,opcion)
                print("Gráfica guardada en grafica.png")
            else:
                print("No se recibió ninguna muestra válida; no se guardó gráfica.")
        else:
            print("Opción inválida. Por favor, ingresa '0', '1' o 'exit'.")


if __name__ == "__main__":
    main()
