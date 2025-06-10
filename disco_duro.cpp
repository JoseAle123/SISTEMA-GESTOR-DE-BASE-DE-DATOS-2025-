#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>

#include <dirent.h>    // para DIR*, opendir(), readdir(), closedir()
#include <sys/types.h> // para struct dirent y tipos relacionados


#include <set>   // Aquí se incluye set
#include <vector>
#include <sstream>
#include <algorithm>  // Para usar sort()

using namespace std; // para evitar escribir std::set, std::string, etc.




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
        int tamanioSector; // <-- nuevo campo
        int registrosTotales;
        int final;
        int tamActual;
        int tamTotal; // <-- calculado dinámicamente
    
        bool crearDirectorio(const string& nombre) {
            return mkdir(nombre.c_str(), 0777) == 0 || errno == EEXIST;
        }
    
        void escribirEncabezadoInicial() {
            archivo << nombreArchivoOriginal << "#" << final << "#-1#"
                    << tamanioRegistro << "#" << final << "#0#" << tamTotal << "\n";
        }
    
        void actualizarEncabezado() {
            archivo.close();
            ifstream in(nombreBloque);
            string oldHeader;
            getline(in, oldHeader);
            string contenido((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
            in.close();
    
            ofstream out(nombreBloque, ios::out | ios::trunc);
            out << nombreArchivoOriginal << "#" << (final + registrosTotales) << "#-1#"
                << tamanioRegistro << "#" << registrosTotales << "#"
                << tamActual << "#" << tamTotal << "\n";
            out << contenido;
            out.close();
    
            archivo.open(nombreBloque, ios::app);
        }
    
    public:
        Bloque(int numeroBloque, const string& archivoOriginal, int tamanioRegistro, int tamanioSector)
            : nombreArchivoOriginal(archivoOriginal),
              tamanioRegistro(tamanioRegistro),
              tamanioSector(tamanioSector),
              registrosTotales(0),
              final(2),
              tamActual(0),
              tamTotal(4 * tamanioSector) // <-- se calcula correctamente
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
    int tamanioSector = 1024; // tamanio del sector
    string rootDir;

    int plato = 0, superficie = 0, pista = 0, sector = 0;
    int bloqueActual = 0, sectoresUsadosEnBloque = 0;
    int espacioUsadoEnBloque = 0; // bytes usados en bloque actual
    int espacioUsadoEnSector = 0;
    int pesoFijoRegistro = -1; // peso fijo, -1 indica que no se ha calculado aún
    int espacioActualDisco = 0;
    int espacioDisponibleDisco = 0;
    int espacioTotalDisco = 0;
    Bloque* bloque;

    set<string> sectoresRegistrados; // ahora set está definido correctamente


public:
    DiscoDuro(int platos, int pistas, int sectores)
        : numPlatos(platos), pistasPorSuperficie(pistas), sectoresPorPista(sectores), bloque(nullptr) {
        rootDir = "DiscoDuroSimulado";
        crearEstructura();
        espacioTotalDisco = obtenerCapacidadTotal();
    }

    DiscoDuro() {} // Constructor sin argumentos

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
                        archivo << "SIN_NOMBRE_AUN\n";      // línea 1: nombre archivo pendiente
                        archivo << tamanioSector << "\n";   // línea 2: tamaño total sector
                        archivo << 0 << "\n";                // línea 3: peso usado
                        archivo.close();
                    }
                }
            }
        }
    }

        // Registrar sector en archivo bloqueX.txt dentro de bloques_sectores
    void registrarSectorEnBloque(int nroBloque, const string& pathSector) {
        string key = to_string(nroBloque) + "|" + pathSector; 
        if (sectoresRegistrados.find(key) != sectoresRegistrados.end()) {
            return; // Ya registrado para este bloque
        }

        sectoresRegistrados.insert(key);

        string nombreArchivo = "bloques_sectores/bloque" + to_string(nroBloque) + ".txt";
        ofstream archivo(nombreArchivo, ios::app);
        if (archivo.is_open()) {
            archivo << pathSector << "\n";
            archivo.close();
        } else {
            cerr << "Error al abrir archivo para bloque: " << nombreArchivo << endl;
        }
    }

    void actualizarNombreArchivoSector(const string& pathSector, const string& nombreArchivo) {
        ifstream archivoLectura(pathSector);
        if (!archivoLectura.is_open()) {
            cerr << "No se pudo abrir sector para actualizar nombre: " << pathSector << endl;
            return;
        }
    
        string linea1; // esta se reemplaza
        getline(archivoLectura, linea1);
    
        string restoContenido;
        string linea;
        while (getline(archivoLectura, linea)) {
            restoContenido += linea + "\n";
        }
        archivoLectura.close();
    
        ofstream archivoEscritura(pathSector);
        if (!archivoEscritura.is_open()) {
            cerr << "No se pudo abrir sector para escribir nombre: " << pathSector << endl;
            return;
        }
    
        archivoEscritura << nombreArchivo << "\n";  // línea 1 actualizada
        archivoEscritura << restoContenido;          // líneas restantes sin tocar
        archivoEscritura.close();
    }

    bool sectorSinNombre(const string& pathSector) {
        ifstream archivo(pathSector);
        if (!archivo.is_open()) {
            return false;  // o manejar error
        }
        string linea1;
        getline(archivo, linea1);
        archivo.close();
        return linea1 == "SIN_NOMBRE_AUN";
    }
    
    

    int contarArchivosEnBloques() {
        int contador = 0;
        std::string ruta = "bloques";  // Carpeta fija
    
        DIR* dir = opendir(ruta.c_str());
        if (dir == nullptr) {
            std::cerr << "No se pudo abrir la carpeta: " << ruta << '\n';
            return 0;
        }
    
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string nombre(entry->d_name);
            if (nombre == "." || nombre == "..") continue;
    
            std::string rutaCompleta = ruta + "/" + nombre;
            struct stat info;
            if (stat(rutaCompleta.c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
                contador++;
            }
        }
    
        closedir(dir);
        return contador;
    }

    bool archivoYaRegistrado(const std::string& nombreArchivo) {
        std::ifstream registro("ArchivosEnBloques/archivosEnBloques.txt");
        if (!registro.is_open()) {
            std::cerr << "No se pudo abrir ArchivosEnBloques/archivosEnBloques.txt\n";
            return false; // Si no existe, asumimos que no está registrado
        }
    
        std::string linea;
        while (std::getline(registro, linea)) {
            if (linea == nombreArchivo) {
                return true;
            }
        }
        return false;
    }
    
    void registrarNuevoArchivo(const std::string& nombreArchivo) {
        std::ofstream registro("ArchivosEnBloques/archivosEnBloques.txt", std::ios::app);
        if (!registro.is_open()) {
            std::cerr << "No se pudo abrir ArchivosEnBloques/archivosEnBloques.txt para escritura\n";
            return;
        }
        registro << nombreArchivo << '\n';
        registro.close();
    }
    
    bool verificarYRegistrarArchivo(const std::string& nombreArchivo) {
        if (archivoYaRegistrado(nombreArchivo)) {
            std::cout << "El archivo '" << nombreArchivo << "' ya se encuentra en el disco.\n";
            return true;  // Ya estaba registrado
        }
    
        // Si no está registrado, reinicia las variables y lo registra
        std::cout << "bloques actuales: " << contarArchivosEnBloques() << std::endl;
        bloqueActual = contarArchivosEnBloques();  // Comenzar en nuevo bloque
        std::cout << bloqueActual << " PROBANDO LA FUNCION" << std::endl;
    
        espacioUsadoEnBloque = 0;
        sectoresUsadosEnBloque = 0;
        espacioUsadoEnSector = 0;
    
        // 🔧 IMPORTANTE: Eliminar bloque anterior para evitar mezcla
        if (bloque != nullptr) {
            delete bloque;
            bloque = nullptr;
        }
    
        registrarNuevoArchivo(nombreArchivo);
        return false;  // Era nuevo, se acaba de registrar
    }
    
    

    
    void actualizarPesoSector(int peso) {
        string pathSector = rootDir + "/Plato_" + to_string(plato)
            + "/Superficie_" + to_string(superficie)
            + "/Pista_" + to_string(pista)
            + "/Sector_" + to_string(sector) + ".txt";
    
        ifstream archivoLectura(pathSector);
        if (!archivoLectura.is_open()) {
            cerr << "No se pudo abrir sector para actualizar peso: " << pathSector << endl;
            return;
        }
    
        string linea1, linea2, linea3;
        getline(archivoLectura, linea1); // nombre archivo
        getline(archivoLectura, linea2); // tamaño total sector
        getline(archivoLectura, linea3); // peso actual (a reemplazar)
    
        string restoContenido;
        string linea;
        while (getline(archivoLectura, linea)) {
            restoContenido += linea + "\n";
        }
        archivoLectura.close();
    
        ofstream archivoEscritura(pathSector);
        if (!archivoEscritura.is_open()) {
            cerr << "No se pudo abrir sector para escribir peso: " << pathSector << endl;
            return;
        }
    
        archivoEscritura << linea1 << "\n";  // nombre archivo (línea 1)
        archivoEscritura << linea2 << "\n";  // tamaño total sector (línea 2)
        archivoEscritura << peso << "\n";    // nuevo peso actual (línea 3)
        archivoEscritura << restoContenido;  // registros existentes
        archivoEscritura.close();
    }

    string construirPathSector(int plato, int superficie, int pista, int sector) {
        // Asumimos que rootDir es una variable miembro accesible (o global) con el directorio raíz del disco
        return rootDir + "/Plato_" + to_string(plato) +
               "/Superficie_" + to_string(superficie) +
               "/Pista_" + to_string(pista) +
               "/Sector_" + to_string(sector) + ".txt";
    }
    
    string leerPrimeraLineaArchivo(const string& rutaArchivo) {
        ifstream archivo(rutaArchivo);
        if (!archivo.is_open()) {
            cerr << "No se pudo abrir el archivo: " << rutaArchivo << endl;
            return "";
        }
        string primeraLinea;
        getline(archivo, primeraLinea);
        archivo.close();
        return primeraLinea;
    }


    void escribirRegistro(const string& registro, int pesoRegistro, const string& nombreArchivo) {
        if (pesoRegistro > tamanioSector) {
            cerr << "Registro excede tamaño de sector\n";
            return;
        }
    
        // Verificar si el disco ya está lleno
        if (espacioActualDisco + pesoRegistro > espacioTotalDisco) {
            cerr << "Disco lleno. No se puede escribir el registro.\n";
            return;
        }
    
        if (bloque == nullptr) {
            bloque = new Bloque(bloqueActual, nombreArchivo, pesoRegistro, tamanioSector);
        }
    
        while (true) {
            string pathSector = construirPathSector(plato, superficie, pista, sector);
            string nombreArchivoSector = leerPrimeraLineaArchivo(pathSector);
    
            if (nombreArchivoSector == "SIN_NOMBRE_AUN" || nombreArchivoSector == nombreArchivo) {
                if (nombreArchivoSector == "SIN_NOMBRE_AUN") {
                    actualizarNombreArchivoSector(pathSector, nombreArchivo);
                }
    
                if (espacioUsadoEnBloque + pesoRegistro > 4 * tamanioSector) {
                    delete bloque;
                    bloque = new Bloque(++bloqueActual, nombreArchivo, pesoRegistro, tamanioSector);
                    espacioUsadoEnBloque = 0;
                    sectoresUsadosEnBloque = 0;
                    espacioUsadoEnSector = 0;
                }
    
                if (espacioUsadoEnSector + pesoRegistro > tamanioSector) {
                    actualizarPesoSector(espacioUsadoEnSector);
                    avanzarSector();
                    espacioUsadoEnSector = 0;
                    continue;
                }
    
                ofstream archivo(pathSector, ios::app);
                archivo << registro << endl;
                archivo.close();
    
                registrarSectorEnBloque(bloqueActual, pathSector);
                bloque->registrarRegistroCompleto(registro, plato, superficie, pista, sector);
    
                espacioUsadoEnSector += pesoRegistro;
                espacioUsadoEnBloque += pesoRegistro;
                espacioActualDisco += pesoRegistro;
    
                if (espacioUsadoEnSector == pesoRegistro) {
                    sectoresUsadosEnBloque++;
                }
    
                actualizarPesoSector(espacioUsadoEnSector);
                break;
            } else {
                avanzarSector();
                espacioUsadoEnSector = 0;
            }
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
                        cout << "SIN SECTORES DISPONIBLES\n";
                        actualizarEspacioDisco();
                        guardarCaracteristicasEnArchivo();
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

    void mostrarArbol() {
        for (int p = 0; p < numPlatos; ++p) {
            cout << "Plato " << p << endl;
            for (int s = 0; s < 2; ++s) {  // 2 superficies por plato
                cout << "  Superficie " << s << endl;
                for (int pi = 0; pi < pistasPorSuperficie; ++pi) {
                    cout << "    Pista " << pi << endl;
                    for (int se = 0; se < sectoresPorPista; ++se) {
                        cout << "      Sector " << se << endl;
                    }
                }
            }
        }
    }

    
    int obtenerCapacidadTotal() const {
        return numPlatos * 2 * pistasPorSuperficie * sectoresPorPista * tamanioSector;
    }
    
    int obtenerCapacidadOcupada() const {
        return espacioActualDisco;
    }
    
    int obtenerCapacidadLibre() const {
        return obtenerCapacidadTotal() - obtenerCapacidadOcupada();
    }

    void actualizarEspacioDisco() {
        espacioTotalDisco = obtenerCapacidadTotal();
        espacioActualDisco = obtenerCapacidadOcupada(); // Este ya debe haber sido actualizado previamente con inserciones o eliminaciones
        espacioDisponibleDisco = obtenerCapacidadLibre();
    }
    
    void mostrarEspacioDisco() {
        actualizarEspacioDisco();
        cout << "Capacidad total del disco: " << espacioTotalDisco << " bytes" << endl;
        cout << "Espacio ocupado: " << espacioActualDisco << " bytes" << endl;
        cout << "Espacio libre: " << espacioDisponibleDisco << " bytes" << endl;
        cout << "Tamanio de sector " << tamanioSector << " bytes" << endl;
        cout << "Espacio por bloque " << tamanioSector * 4 << " bytes" << endl;
    }
    
    

    void guardarCaracteristicasEnArchivo() {
        actualizarEspacioDisco();
        ofstream archivo("discoCaracteristicas.txt");
        if (!archivo.is_open()) {
            cerr << "No se pudo crear discoCaracteristicas.txt\n";
            return;
        }
    
        archivo << "=== Características del Disco ===\n";
        archivo << "Numero de platos: " << numPlatos << "\n";
        archivo << "Superficies por plato: 2\n";
        archivo << "Pistas por superficie: " << pistasPorSuperficie << "\n";
        archivo << "Sectores por pista: " << sectoresPorPista << "\n";
        archivo << "Tamaño de sector: " << tamanioSector << " bytes\n";
        archivo << "Capacidad total: " << obtenerCapacidadTotal() << " bytes\n";
        archivo << "Capacidad ocupada: " << obtenerCapacidadOcupada() << " bytes\n";
        archivo << "Capacidad libre: " << obtenerCapacidadLibre() << " bytes\n";
    
        archivo.close();
        cout << "Características del disco guardadas en 'discoCaracteristicas.txt'.\n";
    }

    
   
};



int main() {
    DiscoDuro* disco = nullptr;
    int opcion;

    do {
        cout << "\n=== MENU PRINCIPAL ===\n";
        cout << "1. Crear disco\n";
        cout << "2. Cargar archivo\n";
        cout << "3. Mostrar estructura del disco (Árbol)\n";
        cout << "4. Mostrar caracteristicas del disco\n";
        cout << "5. Mostrar sectores ocupados y su ubicación\n";
        cout << "6. Mostrar informacion de los bloques\n";
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
                cout << "Ingrese el numero de sectores por pista: ";
                cin >> sectores;

                delete disco;
                disco = new DiscoDuro(platos, pistas, sectores);
                disco->guardarCaracteristicasEnArchivo();
                cout << "Disco creado y características guardadas.\n";
                break;
            }

            case 2: {
                if (!disco) {
                    cerr << "Debe crear el disco primero (opción 1) o cargar desde archivo (opción 5).\n";
                    break;
                }

                string archivoNombre;
                cout << "Ingrese el nombre del archivo (ej: tablas/titanic.txt): ";
                cin >> archivoNombre;

                if (disco->verificarYRegistrarArchivo(archivoNombre)) break;

                string nombres[100], tipos[100];
                int numCampos = leerEsquemaParaArchivo(archivoNombre, nombres, tipos, 100);

                if (numCampos == 0) {
                    cerr << "Error leyendo esquema para " << archivoNombre << endl;
                } else {
                    disco->cargarArchivo(archivoNombre, tipos, numCampos);
                }

                disco->actualizarEspacioDisco();
                disco->guardarCaracteristicasEnArchivo();
                break;
            }

            case 3: {
                if (!disco) {
                    cerr << "Debe crear el disco primero (opción 1) o cargar desde archivo (opción 5).\n";
                    break;
                }
                disco->mostrarArbol();
                break;
            }

            case 4: {
                if (!disco) {
                    cerr << "Debe crear el disco primero (opción 1) o cargar desde archivo (opción 5).\n";
                    break;
                }
                disco->mostrarEspacioDisco();
                break;
            }

            case 5: {
                const string carpetaBloquesSectores = "bloques_sectores";
                DIR* dir = opendir(carpetaBloquesSectores.c_str());
            
                if (!dir) {
                    cerr << "Error: No existe la carpeta 'bloques_sectores'.\n";
                    break;
                }
            
                struct dirent* archivo;
                vector<string> archivosBloques;
                
                // Primero recolectamos y ordenamos los nombres de los archivos
                while ((archivo = readdir(dir)) != nullptr) {
                    string nombreArchivo = archivo->d_name;
                    if (nombreArchivo == "." || nombreArchivo == "..") continue;
                    archivosBloques.push_back(nombreArchivo);
                }
                closedir(dir);
                
                // Ordenamos los archivos alfabéticamente (bloque0.txt, bloque1.txt, etc.)
                sort(archivosBloques.begin(), archivosBloques.end());
            
                int totalSectores = 0;
                cout << "\n=== SECTORES OCUPADOS POR BLOQUES (ORDENADOS) ===\n";
                
                for (const auto& nombreArchivo : archivosBloques) {
                    string rutaArchivo = carpetaBloquesSectores + "/" + nombreArchivo;
                    ifstream archivoBloque(rutaArchivo);
            
                    if (!archivoBloque.is_open()) {
                        cerr << "No se pudo abrir: " << rutaArchivo << "\n";
                        continue;
                    }
            
                    cout << "\n[ Bloque: " << nombreArchivo << " ]\n";
            
                    string linea;
                    int sectoresEnBloque = 0;
                    while (getline(archivoBloque, linea)) {
                        if (!linea.empty()) {
                            cout << "Sector " << ++sectoresEnBloque << " -> Dirección: " << linea << "\n";
                        }
                    }
            
                    totalSectores += sectoresEnBloque;
                    cout << "Total sectores en este bloque: " << sectoresEnBloque << "\n";
            
                    archivoBloque.close();
                }
            
                cout << "\n=== TOTAL GENERAL ===\n";
                cout << "Sectores en uso: " << totalSectores << "\n";
                break;
            }

            case 6: {
                const string carpetaBloques = "bloques";
                DIR* dir = opendir(carpetaBloques.c_str());
                
                if (!dir) {
                    cerr << "Error: No existe la carpeta 'bloques'.\n";
                    break;
                }
            
                // Contar bloques ocupados
                int totalBloques = 0;
                struct dirent* archivo;
                while ((archivo = readdir(dir)) != nullptr) {
                    if (string(archivo->d_name) != "." && string(archivo->d_name) != "..") {
                        totalBloques++;
                    }
                }
                closedir(dir);
            
                cout << "\n=== INFORMACIÓN DE BLOQUES ===\n";
                cout << "Bloques ocupados: " << totalBloques << "\n";
                cout << "No hay bloques vacíos.\n\n";
            
                // Pedir al usuario el número de bloque a mostrar
                int numBloque;
                cout << "Ingrese el número de bloque a inspeccionar (ej: 1, 2, ...): ";
                cin >> numBloque;
            
                string rutaBloque = carpetaBloques + "/bloque" + to_string(numBloque) + ".txt";
                ifstream archivoBloque(rutaBloque);
            
                if (!archivoBloque.is_open()) {
                    cerr << "Error: El bloque " << numBloque << " no existe.\n";
                    break;
                }
            
                // Leer cabecera
                string cabecera;
                getline(archivoBloque, cabecera);
            
                // Parsear cabecera (formato: nombre#final#-1#tamRegistro#registros#tamActual#tamTotal)
                vector<string> partes;
                stringstream ss(cabecera);
                string parte;
                while (getline(ss, parte, '#')) {
                    partes.push_back(parte);
                }
            
                // Mostrar metadatos
                cout << "\n[ BLOQUE " << numBloque << " ]\n";
                cout << "Archivo origen: " << partes[0] << "\n";
                cout << "Tamaño por registro: " << partes[3] << " bytes\n";
                cout << "Registros almacenados: " << partes[4] << "\n";
                cout << "Espacio usado: " << partes[5] << " bytes\n";
                cout << "Capacidad total: " << partes[6] << " bytes\n";
                cout << "Porcentaje ocupado: " 
                     << (stof(partes[5]) / stof(partes[6])) * 100 << "%\n";
            
                // Mostrar registros
                cout << "\n[ REGISTROS ]\n";
                string registro;
                int contador = 1;
                while (getline(archivoBloque, registro)) {
                    cout << contador++ << ": " << registro << "\n";
                }
            
                archivoBloque.close();
                break;
            }

             
            

            case 0:
                cout << "Saliendo del programa.\n";
                break;

            default:
                cout << "Opción inválida. Intente de nuevo.\n";
        }

    } while (opcion != 0);

    delete disco;
    return 0;
}
