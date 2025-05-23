#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>

using namespace std;

bool crearDirectorio(const string& nombre) {
    return mkdir(nombre.c_str(), 0777) == 0 || errno == EEXIST;
}

// Funciones auxiliares
int contarCampos(const char* linea) {
    int count = 1;
    for (int i = 0; linea[i] != '\0'; ++i)
        if (linea[i] == '#') count++;
    return count;
}

void obtenerCampo(const char* linea, int indice, char* buffer, int tamBuffer) {
    int campoActual = 0, j = 0;
    for (int i = 0; linea[i] != '\0'; ++i) {
        if (linea[i] == '#') {
            if (campoActual == indice) {
                buffer[j] = '\0';
                return;
            }
            campoActual++;
            if (campoActual > indice) break;
            j = 0;
        } else if (campoActual == indice && j < tamBuffer - 1) {
            buffer[j++] = linea[i];
        }
    }
    buffer[j] = '\0';
}

int calcularPesoRegistro(const char* linea, string tipos[], int numCampos) {
    int pesoTotal = 0;
    const int TAM_BUFFER = 256;
    char campo[TAM_BUFFER];

    for (int i = 0; i < numCampos; ++i) {
        obtenerCampo(linea, i, campo, TAM_BUFFER);
        if (tipos[i] == "int" || tipos[i] == "float") {
            pesoTotal += 4;
        } else if (tipos[i] == "char") {
            pesoTotal += 1;
        } else if (tipos[i] == "str") {
            int len = strlen(campo);
            if (len >= 2 && campo[0] == '"' && campo[len - 1] == '"') len -= 2;
            pesoTotal += len;
        } else {
            cerr << "Tipo desconocido: " << tipos[i] << endl;
        }
    }
    return pesoTotal;
}

int leerTiposDesdeEsquema(const string& nombreArchivo, string tipos[], int maxCampos) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo de esquema.\n";
        return 0;
    }

    string linea;
    getline(archivo, linea);
    archivo.close();

    int count = 0;
    size_t inicio = 0, pos;

    while ((pos = linea.find('#', inicio)) != string::npos && count < maxCampos) {
        tipos[count++] = linea.substr(inicio, pos - inicio);
        inicio = pos + 1;
    }
    if (inicio < linea.size() && count < maxCampos) {
        tipos[count++] = linea.substr(inicio);
    }

    return count;
}


class Bloque {
    private:
        string nombreBloque;
        ofstream archivo;
    
        bool crearDirectorio(const string& nombre) {
            return mkdir(nombre.c_str(), 0777) == 0 || errno == EEXIST;
        }
    
    public:
        Bloque(int numeroBloque) {
            // Crear carpeta "bloques" si no existe
            crearDirectorio("bloques");
    
            // Construir ruta completa
            nombreBloque = "bloques/bloque" + to_string(numeroBloque) + ".txt";
    
            // Abrir archivo (sobrescribir)
            archivo.open(nombreBloque, ios::out);
            if (!archivo.is_open()) {
                cerr << "Error al crear archivo: " << nombreBloque << endl;
            }
        }
    
        ~Bloque() {
            if (archivo.is_open()) archivo.close();
        }
    
        void registrarPrimerCampo(const string& registro) {
            if (!archivo.is_open()) return;
    
            size_t pos = registro.find('#');
            string primerCampo = (pos != string::npos) ? registro.substr(0, pos) : registro;
            archivo << primerCampo << endl;
        }
    };



class DiscoDuro {
private:
    int numPlatos, pistasPorSuperficie, sectoresPorPista;
    const int tamanioSector = 4096;
    string rootDir;

    int plato = 0, superficie = 0, pista = 0, sector = 0;
    int bloqueActual = 0, sectoresUsadosEnBloque = 0;
    Bloque* bloque;

public:
    DiscoDuro(int platos, int pistas, int sectores)
        : numPlatos(platos), pistasPorSuperficie(pistas), sectoresPorPista(sectores), bloque(nullptr) {
        rootDir = "DiscoDuroSimulado";
        crearEstructura();
        bloque = new Bloque(bloqueActual);
    }

    ~DiscoDuro() {
        delete bloque;
    }

    void crearEstructura() {
        crearDirectorio(rootDir);
        for (int p = 0; p < numPlatos; ++p) {
            string dirPlato = rootDir + "/Plato_" + to_string(p);
            crearDirectorio(dirPlato);
            for (int s = 0; s < 2; ++s) {
                string dirSuperficie = dirPlato + "/Superficie_" + to_string(s);
                crearDirectorio(dirSuperficie);
                for (int t = 0; t < pistasPorSuperficie; ++t) {
                    string dirPista = dirSuperficie + "/Pista_" + to_string(t);
                    crearDirectorio(dirPista);
                    for (int sec = 0; sec < sectoresPorPista; ++sec) {
                        ofstream archivo(dirPista + "/Sector_" + to_string(sec) + ".txt");
                        archivo.close();
                    }
                }
            }
        }
    }

    void escribirRegistro(const string& registro) {
        if ((int)registro.length() > tamanioSector) {
            cerr << "Registro excede tamaño de sector\n";
            return;
        }

        string pathSector = rootDir + "/Plato_" + to_string(plato)
            + "/Superficie_" + to_string(superficie)
            + "/Pista_" + to_string(pista)
            + "/Sector_" + to_string(sector) + ".txt";

        ofstream archivo(pathSector, ios::app);
        archivo << registro << endl;
        archivo.close();

        if (bloque != nullptr) bloque->registrarPrimerCampo(registro);
        sectoresUsadosEnBloque++;

        avanzarSector();
    }

    void avanzarSector() {
        sector++;
        if (sector >= sectoresPorPista) {
            sector = 0; pista++;
            if (pista >= pistasPorSuperficie) {
                pista = 0; superficie++;
                if (superficie >= 2) {
                    superficie = 0; plato++;
                    if (plato >= numPlatos) {
                        cout << "Disco lleno\n";
                        exit(0);
                    }
                }
            }
        }

        if (sectoresUsadosEnBloque == 4) {
            delete bloque;
            bloque = new Bloque(++bloqueActual);
            sectoresUsadosEnBloque = 0;
        }
    }

    void cargarArchivo(const string& nombreArchivo, string tipos[], int numCampos) {
        // Construir la ruta completa agregando "tablas/" si no está presente
        string rutaArchivo = nombreArchivo;
        if (nombreArchivo.find("tablas/") != 0) {  // si no empieza con "tablas/"
            rutaArchivo = "tablas/" + nombreArchivo;
        }
    
        ifstream archivo(rutaArchivo);
        if (!archivo.is_open()) {
            cerr << "Error abriendo archivo: " << rutaArchivo << endl;
            return;
        }
    
        string linea;
        // Leer línea de cabecera y descartarla
        if (!getline(archivo, linea)) {
            cerr << "Archivo vacío o sin cabecera: " << rutaArchivo << endl;
            archivo.close();
            return;
        }
    
        while (getline(archivo, linea)) {
            // Ignorar líneas vacías o solo espacios
            if (linea.find_first_not_of(" \t\n\r") == string::npos) {
                continue;
            }
    
            int peso = calcularPesoRegistro(linea.c_str(), tipos, numCampos);
            if (peso <= tamanioSector) {
                escribirRegistro(linea);
            } else {
                cerr << "Registro omitido (excede " << tamanioSector << " bytes): " 
                     << linea.substr(0, 50) << "...\n";
            }
        }
    
        archivo.close();
    }
};

int main() {
    DiscoDuro disco(1, 3, 5);
    string archivoNombre;
    cout << "Ingrese el nombre del archivo (ej: tablas/titanic.txt): ";
    cin >> archivoNombre;

    string tipos[100];
    int numCampos = leerTiposDesdeEsquema("esquemas/esquema.txt", tipos, 100);
    disco.cargarArchivo(archivoNombre, tipos, numCampos);

    return 0;
}
