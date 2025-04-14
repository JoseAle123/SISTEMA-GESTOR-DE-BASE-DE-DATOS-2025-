#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <direct.h>  // Para _mkdir en Windows
#include <io.h>      // Para _access

class Tabla {
public:
    static void convertirCSV(const std::string& archivoCSV, const std::string& nombreArchivoSalida) {
        if (_access("tablas", 0) == -1) {
            _mkdir("tablas");
        }

        std::ifstream entrada(archivoCSV);
        std::ofstream salida("tablas\\" + nombreArchivoSalida);

        if (!entrada.is_open() || !salida.is_open()) {
            std::cerr << "Error al abrir los archivos." << std::endl;
            return;
        }

        std::string linea;
        while (std::getline(entrada, linea)) {
            std::stringstream ss(linea);
            std::string campo;
            bool primerCampo = true;

            while (std::getline(ss, campo, '\t')) {
                if (!primerCampo) {
                    salida << "#";
                }
                salida << campo;
                primerCampo = false;
            }
            salida << "\n";
        }

        entrada.close();
        salida.close();

        std::cout << " Archivo convertido guardado en: tablas\\" << nombreArchivoSalida << "\n";
    }

    static void imprimirTabla(const std::string& nombreArchivo) {
        std::ifstream archivo("tablas\\" + nombreArchivo);
        if (!archivo.is_open()) {
            std::cerr << "No se pudo abrir la tabla: " << nombreArchivo << std::endl;
            return;
        }

        std::cout << "\n Contenido de " << nombreArchivo << ":\n\n";

        std::string linea;
        while (std::getline(archivo, linea)) {
            std::cout << linea << std::endl;
        }

        archivo.close();
    }
    
};

int main() {
    int opcion;
    do {
        std::cout << "\n=== Menu de Tablas ===\n";
        std::cout << "1. Crear una tabla desde archivo CSV\n";
        std::cout << "2. Imprimir una tabla por nombre\n";
        std::cout << "0. Salir\n";
        std::cout << "Seleccione una opción: ";
        std::cin >> opcion;

        std::cin.ignore();  // Limpiar buffer del enter

        if (opcion == 1) {
            std::string archivoCSV, nombreSalida;
            std::cout << "Ingrese el nombre del archivo CSV (ej. titanic.csv): ";
            std::getline(std::cin, archivoCSV);
            std::cout << "Ingrese el nombre de salida (ej. titanic.txt): ";
            std::getline(std::cin, nombreSalida);
            Tabla::convertirCSV(archivoCSV, nombreSalida);
        }
        else if (opcion == 2) {
            std::string nombreTabla;
            std::cout << "Ingrese el nombre de la tabla a imprimir (ej. titanic.txt): ";
            std::getline(std::cin, nombreTabla);
            Tabla::imprimirTabla(nombreTabla);
        }
        else if (opcion == 0) {
            std::cout << "Saliendo del programa.\n";
        }
        else {
            std::cout << "Opción inválida. Intente nuevamente.\n";
        }

    } while (opcion != 0);

    return 0;
}
