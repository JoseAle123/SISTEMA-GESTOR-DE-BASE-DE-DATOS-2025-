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

// --- Función auxiliar para imprimir peso de cada campo ---
void imprimirPesoCampos(const char* linea, string tipos[], int numCampos) {
    const int TAM_BUFFER = 256;
    char campo[TAM_BUFFER];

    cout << "Peso por campo:\n";
    for (int i = 0; i < numCampos; ++i) {
        obtenerCampo(linea, i, campo, TAM_BUFFER);
        int pesoCampo = 0;

        if (tipos[i] == "int" || tipos[i] == "float") {
            pesoCampo = 4;
        } else if (tipos[i] == "char") {
            pesoCampo = 1;
        } else if (tipos[i] == "str") {
            int len = strlen(campo);
            if (len >= 2 && campo[0] == '"' && campo[len - 1] == '"') len -= 2;
            pesoCampo = len;
        }

        cout << "  Campo " << i << " (" << tipos[i] << "): " << pesoCampo << " bytes\n";
    }
}


int leerEsquemaParaArchivo(const string& nombreArchivo, string nombres[], string tipos[], int maxCampos) {
    ifstream esquema("esquemas/esquema.txt");
    if (!esquema.is_open()) {
        cerr << "No se pudo abrir esquemas.txt\n";
        return 0;
    }

    // Extraer solo nombre base (sin .txt) para comparar con esquema
    string nombreBase = nombreArchivo;
    size_t posExt = nombreArchivo.find(".txt");
    if (posExt != string::npos) {
        nombreBase = nombreArchivo.substr(0, posExt);
    }

    string linea;
    while (getline(esquema, linea)) {
        if (linea.find(nombreBase + "#") == 0) {
            // Cortamos la linea al nombreBase + "#"
            size_t pos = nombreBase.size() + 1;

            int count = 0;
            while (pos < linea.size() && count < maxCampos) {
                // Obtenemos nombre campo
                size_t posCampo = linea.find('#', pos);
                if (posCampo == string::npos) break;
                string campo = linea.substr(pos, posCampo - pos);

                pos = posCampo + 1;

                // Obtenemos tipo campo
                size_t posTipo = linea.find('#', pos);
                string tipo;
                if (posTipo == string::npos) {
                    tipo = linea.substr(pos);
                    pos = linea.size();
                } else {
                    tipo = linea.substr(pos, posTipo - pos);
                    pos = posTipo + 1;
                }

                nombres[count] = campo;
                tipos[count] = tipo;
                count++;
            }

            esquema.close();
            return count;
        }
    }

    cerr << "No se encontró esquema para el archivo: " << nombreArchivo << endl;
    esquema.close();
    return 0;
}



class Bloque {
    private:
        string nombreBloque;
        string nombreArchivoOriginal;
        ofstream archivo;
        int tamanioRegistro;
    
        int registrosTotales;  // cantidad de registros agregados desde que se creó el bloque
        int final;             // valor inicial fijo (ejemplo 2)
        int tamActual;
        const int tamTotal = 4 * 500; // 4 sectores de 500 bytes
      
    
        bool crearDirectorio(const string& nombre) {
            return mkdir(nombre.c_str(), 0777) == 0 || errno == EEXIST;
        }
    
        void escribirEncabezadoInicial() {
            archivo << nombreArchivoOriginal << "#" << final << "#-1#"
                    << tamanioRegistro << "#" << final << "#0#" << tamTotal << "\n";
        }
    
        void actualizarEncabezado() {
            archivo.close(); // cerramos para actualizar
            ifstream in(nombreBloque);
            string oldHeader;
            getline(in, oldHeader); // leemos encabezado viejo
            string contenido((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
            in.close();
    
            ofstream out(nombreBloque, ios::out | ios::trunc);
            out << nombreArchivoOriginal << "#" << (final + registrosTotales) << "#-1#"
                << tamanioRegistro << "#" << registrosTotales << "#"
                << tamActual << "#" << tamTotal << "\n";
            out << contenido;
            out.close();
    
            archivo.open(nombreBloque, ios::app); // reabrimos para seguir escribiendo
        }
    
    public:
        Bloque(int numeroBloque, const string& archivoOriginal, int tamanioRegistro)
            : nombreArchivoOriginal(archivoOriginal),
              tamanioRegistro(tamanioRegistro),
              registrosTotales(0),
              final(2),  // inicializamos final en 2
              tamActual(0)
        {
            crearDirectorio("bloques");
            nombreBloque = "bloques/bloque" + to_string(numeroBloque) + ".txt";
            archivo.open(nombreBloque, ios::out);
            if (!archivo.is_open()) {
                cerr << "Error al crear archivo: " << nombreBloque << endl;
            } else {
                escribirEncabezadoInicial();
            }
        }
    
        ~Bloque() {
            if (archivo.is_open()) archivo.close();
        }
    
        void registrarRegistroCompleto(const string& registro, int plato, int superficie, int pista, int sector) {
            if (!archivo.is_open()) return;
            archivo << registro 
                    << " | Pos: Plato " << plato 
                    << ", Superficie " << superficie 
                    << ", Pista " << pista 
                    << ", Sector " << sector << "\n";
    
            registrosTotales++;
            tamActual += tamanioRegistro;
            actualizarEncabezado();
        }
    };
    
    


class DiscoDuro {
private:
    int numPlatos, pistasPorSuperficie, sectoresPorPista;
    const int tamanioSector = 300; // tamanio del sector
    string rootDir;

    int plato = 0, superficie = 0, pista = 0, sector = 0;
    int bloqueActual = 0, sectoresUsadosEnBloque = 0;
    int espacioUsadoEnBloque = 0; // bytes usados en bloque actual
    int espacioUsadoEnSector = 0;
    int pesoFijoRegistro = -1; // peso fijo, -1 indica que no se ha calculado aún
    int espacioActualDisco = 0;
    Bloque* bloque;

public:
    DiscoDuro(int platos, int pistas, int sectores)
        : numPlatos(platos), pistasPorSuperficie(pistas), sectoresPorPista(sectores), bloque(nullptr) {
        rootDir = "DiscoDuroSimulado";
        crearEstructura();
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
                        string rutaSector = dirPista + "/Sector_" + to_string(sec) + ".txt";
                        ofstream archivo(rutaSector);
                        archivo << tamanioSector << "\n"; // primera línea: tamaño total sector
                        archivo << 0 << "\n";              // segunda línea: peso usado (inicial 0)
                        archivo.close();
                    }
                }
            }
        }
    }
    
    void actualizarPesoSector(int peso) {
        string pathSector = rootDir + "/Plato_" + to_string(plato)
            + "/Superficie_" + to_string(superficie)
            + "/Pista_" + to_string(pista)
            + "/Sector_" + to_string(sector) + ".txt";
    
        // Leer el contenido completo del archivo sector
        ifstream archivoLectura(pathSector);
        if (!archivoLectura.is_open()) {
            cerr << "No se pudo abrir sector para actualizar peso: " << pathSector << endl;
            return;
        }
    
        string linea1, linea2;
        getline(archivoLectura, linea1); // tamaño total sector
        getline(archivoLectura, linea2); // peso actual sector (a modificar)
    
        string restoContenido;
        string linea;
        while (getline(archivoLectura, linea)) {
            restoContenido += linea + "\n";
        }
        archivoLectura.close();
    
        // Reescribir el archivo sector con el nuevo peso en la segunda línea
        ofstream archivoEscritura(pathSector);
        if (!archivoEscritura.is_open()) {
            cerr << "No se pudo abrir sector para escribir peso: " << pathSector << endl;
            return;
        }
    
        archivoEscritura << linea1 << "\n";        // tamaño total sector (línea 1)
        archivoEscritura << peso << "\n";          // nuevo peso usado (línea 2)
        archivoEscritura << restoContenido;        // registros existentes
        archivoEscritura.close();
    }
    


    void escribirRegistro(const string& registro, int pesoRegistro, const string& nombreArchivo) {
        if (pesoRegistro > tamanioSector) {
            cerr << "Registro excede tamaño de sector\n";
            return;
        }
    
        // Si el bloque actual no existe (primer registro), se crea
        if (bloque == nullptr) {
            bloque = new Bloque(bloqueActual, nombreArchivo, pesoRegistro);
        }
    
        // Si el bloque actual ya no tiene espacio, crear uno nuevo
        if (espacioUsadoEnBloque + pesoRegistro > 4 * tamanioSector) {
            delete bloque;
            bloque = new Bloque(++bloqueActual, nombreArchivo, pesoRegistro);
            espacioUsadoEnBloque = 0;
            sectoresUsadosEnBloque = 0;
        }
    
        if (espacioUsadoEnSector + pesoRegistro > tamanioSector) {
            actualizarPesoSector(espacioUsadoEnSector);
            avanzarSector();
            espacioUsadoEnSector = 0;
        }
    
        string pathSector = rootDir + "/Plato_" + to_string(plato)
            + "/Superficie_" + to_string(superficie)
            + "/Pista_" + to_string(pista)
            + "/Sector_" + to_string(sector) + ".txt";
    
        int nuevoPeso = espacioUsadoEnSector + pesoRegistro;
        actualizarPesoSector(nuevoPeso);
    
        ofstream archivo(pathSector, ios::app);
        archivo << registro << endl;
        archivo.close();
    
        bloque->registrarRegistroCompleto(registro, plato, superficie, pista, sector);
    
        espacioUsadoEnSector += pesoRegistro;
        espacioUsadoEnBloque += pesoRegistro;
    
        if (espacioUsadoEnSector == pesoRegistro) {
            sectoresUsadosEnBloque++;
        }
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
    }
    

    void cargarArchivo(const string& nombreArchivo, string tipos[], int numCampos) {
        string rutaArchivo = nombreArchivo;
        if (nombreArchivo.find("tablas/") != 0) {
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
    
        int pesoFijoRegistro = -1;
        bool primerRegistroProcesado = false;
    
        while (getline(archivo, linea)) {
            if (linea.find_first_not_of(" \t\n\r") == string::npos) {
                continue; // Saltar líneas vacías o solo espacios
            }
    
            if (!primerRegistroProcesado) {
                pesoFijoRegistro = calcularPesoRegistro(linea.c_str(), tipos, numCampos);
    
                // Imprimir peso de cada campo del primer registro
                imprimirPesoCampos(linea.c_str(), tipos, numCampos);
                cout << "Peso total del primer registro: " << pesoFijoRegistro << " bytes\n";
    
                if (pesoFijoRegistro > tamanioSector) {
                    cerr << "Primer registro excede tamaño de sector, abortando\n";
                    archivo.close();
                    return;
                }
    
                primerRegistroProcesado = true;
            }
    
            escribirRegistro(linea, pesoFijoRegistro, nombreArchivo);
        }
    
        cout << "Datos del archivo '" << nombreArchivo << "' guardados correctamente en el disco." << endl;
        archivo.close();
    }

    void mostrarEspacioTotalDisco() const {
        int capacidadTotal = numPlatos * 2 * pistasPorSuperficie * sectoresPorPista * tamanioSector;
        cout << "Espacio total del disco: " << capacidadTotal << " bytes" << endl;
    }
    
    
    
    
    
};

int main() {
    DiscoDuro* disco = nullptr;
    int opcion;

    do {
        cout << "\n=== MENU PRINCIPAL ===\n";
        cout << "1. Crear disco\n";
        cout << "2. Cargar archivo\n";
        cout << "0. Salir\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1: {
                int platos, pistas, sectores;
                cout << "Ingrese el numero de platos: ";
                cin >> platos;
                cout << "Ingrese el numero de pistas: ";
                cin >> pistas;
                cout << "Ingrese el numero de sectores por pistas: ";
                cin >> sectores;

                delete disco;
                disco = new DiscoDuro(platos, pistas, sectores);
                disco->mostrarEspacioTotalDisco();
                break;
            }

            case 2: {
                if (!disco) {
                    cerr << "Primero debe crear el disco (opcion 1).\n";
                    break;
                }

                string archivoNombre;
                cout << "Ingrese el nombre del archivo (ej: tablas/titanic.txt): ";
                cin >> archivoNombre;

                string nombres[100];
                string tipos[100];
                int numCampos = leerEsquemaParaArchivo(archivoNombre, nombres, tipos, 100);

                if (numCampos == 0) {
                    cerr << "Error leyendo esquema para " << archivoNombre << endl;
                } else {
                    disco->cargarArchivo(archivoNombre, tipos, numCampos);
                }
                break;
            }

            case 0:
                cout << "Saliendo del programa.\n";
                break;

            default:
                cout << "Opcion invalida. Intente de nuevo.\n";
        }

    } while (opcion != 0);

    delete disco;
    return 0;
}