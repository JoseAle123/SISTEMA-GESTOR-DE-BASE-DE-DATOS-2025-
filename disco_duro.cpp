#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>  // mkdir

using namespace std;

// Función para crear directorio, devuelve true si se crea o ya existe
bool crearDirectorio(const std::string& path) {
    // modo 0755 (rwxr-xr-x)
    int resultado = mkdir(path.c_str(), 0755);
    if (resultado == 0) {
        return true; // creado correctamente
    } else if (resultado == -1) {
        // Si ya existe no es error
        // No usamos errno ni strerror para mantener las librerías indicadas
        return true;
    }
    return false;
}

class DiscoDuro {
private:
    int numPlatos;
    int pistasPorSuperficie;
    int sectoresPorPista;
    int tamanioDisco;

    int tamanioSector = 4096;
    std::string rootDir;

public:
    DiscoDuro(int platos, int pistas, int sectores)
        : numPlatos(platos), pistasPorSuperficie(pistas), sectoresPorPista(sectores) {
        rootDir = "DiscoDuroSimulado";
    }

    void crearEstructura() {
        if (!crearDirectorio(rootDir)) {
            std::cerr << "Error creando directorio raíz\n";
            return;
        }

        for (int p = 0; p < numPlatos; ++p) {
            std::string nombrePlato = rootDir + "/Plato_" + std::to_string(p);
            if (!crearDirectorio(nombrePlato)) {
                std::cerr << "Error creando directorio Plato\n";
                return;
            }

            for (int s = 0; s < 2; ++s) { // 2 superficies por plato
                std::string nombreSuperficie = nombrePlato + "/Superficie_" + std::to_string(s);
                if (!crearDirectorio(nombreSuperficie)) {
                    std::cerr << "Error creando directorio Superficie\n";
                    return;
                }

                for (int t = 0; t < pistasPorSuperficie; ++t) {
                    std::string nombrePista = nombreSuperficie + "/Pista_" + std::to_string(t);
                    if (!crearDirectorio(nombrePista)) {
                        std::cerr << "Error creando directorio Pista\n";
                        return;
                    }

                    for (int sec = 0; sec < sectoresPorPista; ++sec) {
                        std::string nombreSector = nombrePista + "/Sector_" + std::to_string(sec) + ".txt";
                        std::ofstream archivo(nombreSector);
                        if (!archivo) {
                            std::cerr << "Error creando archivo: " << nombreSector << "\n";
                            return;
                        }
                        archivo.close();
                    }
                }
            }
        }

        std::cout << "Estructura de disco duro creada exitosamente en '" << rootDir << "'\n";
    }

    // Función que calcula el tamaño total del disco y lo guarda en el atributo
    void calcularTamanioDisco() {
        // 2 superficies por plato
        tamanioDisco = numPlatos * 2 * pistasPorSuperficie * sectoresPorPista * tamanioSector;
    }

    // Getter para obtener el tamaño en bytes
    int getTamanioDisco() const {
        return tamanioDisco;
    }

};

int main() {
    int platos, pistas, sectores;

    std::cout << "Ingrese el número de platos: ";
    std::cin >> platos;

    std::cout << "Ingrese el número de pistas por superficie: ";
    std::cin >> pistas;

    std::cout << "Ingrese el número de sectores por pista: ";
    std::cin >> sectores;

    DiscoDuro disco(platos, pistas, sectores);
    disco.crearEstructura();

    cout << "tamanio del disco Duro" << disco.getTamanioDisco() << endl;

    return 0;
}
