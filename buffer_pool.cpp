#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <sys/stat.h>  // mkdir
#include <dirent.h>    // opendir, closedir

using namespace std;

bool obtenerAtributos(const string& archivoConTxt, string atributos[], int& cantidad) {
    ifstream esquema("esquemas/esquema.txt");
    if (!esquema.is_open()) {
        cerr << "❌ No se pudo abrir esquemas/esquema.txt\n";
        return false;
    }

    string linea;
    while (getline(esquema, linea)) {
        stringstream ss(linea);
        string nombreArchivo;
        getline(ss, nombreArchivo, '#');

        if (nombreArchivo + ".txt" == archivoConTxt) {
            cantidad = 0;
            string token;
            int i = 0;
            while (getline(ss, token, '#')) {
                if (i % 2 == 0) atributos[cantidad++] = token;
                i++;
            }
            return true;
        }
    }
    return false;
}

bool insertarRegistroEnBloque(const string& archivoConTxt, const string& registro, int pesoRegistro, int& bloqueIndexOut) {
    int bloqueIndex = 0;
    char ruta[100];

    while (true) {
        sprintf(ruta, "bloques/bloque%d.txt", bloqueIndex);

        ifstream in(ruta);
        if (!in.is_open()) {
            cerr << "❌ No se encontró un bloque válido: " << ruta << endl;
            return false;
        }

        string cabecera;
        getline(in, cabecera);
        in.close();

        char nombreArchivo[50];
        int lineaInsertar, dummy, peso, numRegs, tamActual, tamTotal;

        sscanf(cabecera.c_str(), "%[^#]#%d#%d#%d#%d#%d#%d",
               nombreArchivo, &lineaInsertar, &dummy, &peso, &numRegs, &tamActual, &tamTotal);

        if (archivoConTxt != nombreArchivo || tamActual + pesoRegistro > tamTotal) {
            bloqueIndex++;
            continue;
        }

        // Leer contenido actual del bloque (sin cabecera)
        vector<string> lineas;
        ifstream in2(ruta);
        string temp;
        getline(in2, temp); // Cabecera
        while (getline(in2, temp)) lineas.push_back(temp);
        in2.close();

        int lineaFinal = lineas.size();  // Por defecto: agregar al final

        if (dummy == 0) {
            char rutaEliminado[100];
            sprintf(rutaEliminado, "registros_eliminados/bloque%d.txt", bloqueIndex);
            ifstream inEliminado(rutaEliminado);
            vector<string> lineasEliminadas;

            if (inEliminado.is_open()) {
                string lineaPrimera;
                getline(inEliminado, lineaPrimera); // Solo la primera línea

                string linea;
                while (getline(inEliminado, linea)) {
                    if (!linea.empty()) lineasEliminadas.push_back(linea);
                }
                inEliminado.close();

                if (!lineaPrimera.empty()) {
                    int dummy1, lineaBloque;
                    sscanf(lineaPrimera.c_str(), "%d#%d", &dummy1, &lineaBloque);
                    lineaFinal = lineaBloque - 2; // Ajuste (por la cabecera)
                }

                // Reescribir archivo de eliminados sin la primera línea
                ofstream outEliminado(rutaEliminado);
                for (const string& l : lineasEliminadas) outEliminado << l << "\n";
                outEliminado.close();

                // Si ya no hay más líneas, actualizar dummy = -1
                if (lineasEliminadas.empty()) {
                    dummy = -1;
                }
            }
        }

        // Insertar el registro en la línea correspondiente
        if (lineaFinal < (int)lineas.size()) {
            lineas[lineaFinal] = registro;
        } else {
            while ((int)lineas.size() < lineaFinal) lineas.push_back("");
            lineas.push_back(registro);
        }

        // Escribir archivo actualizado con nueva cabecera
        ofstream out(ruta);
        out << archivoConTxt << "#" << (lineaInsertar + 1) << "#" << dummy << "#" << peso << "#"
            << (numRegs + 1) << "#" << (tamActual + pesoRegistro) << "#" << tamTotal << "\n";
        for (const string& l : lineas) out << l << "\n";
        out.close();

        bloqueIndexOut = bloqueIndex;
        cout << "✅ Registro insertado en bloque[" << bloqueIndex << "], línea: " << (lineaFinal + 2) << endl;
        return true;
    }

    cout << "❌ No se encontró un bloque con espacio suficiente.\n";
    return false;
}



bool insertarRegistroEnSector(int bloqueIndex, const string& archivoConTxt, const string& registro, int pesoRegistro, string& direccionOut) {
    char rutaBloque[100];
    sprintf(rutaBloque, "bloques/bloque%d.txt", bloqueIndex);

    // Leer cabecera del bloque
    ifstream inBloque(rutaBloque);
    if (!inBloque.is_open()) {
        cerr << "❌ No se pudo abrir el bloque " << rutaBloque << endl;
        return false;
    }

    string cabecera;
    getline(inBloque, cabecera);
    inBloque.close();

    char nombreArchivo[50];
    int lineaInsertar, dummy, peso, numRegs, tamActual, tamTotal;
    sscanf(cabecera.c_str(), "%[^#]#%d#%d#%d#%d#%d#%d",
           nombreArchivo, &lineaInsertar, &dummy, &peso, &numRegs, &tamActual, &tamTotal);

    int lineaReuso = -1;
    if (dummy == 0) {
        // Leer solo la primera línea del archivo de registros eliminados
        char rutaEliminado[100];
        sprintf(rutaEliminado, "registros_eliminados/bloque%d.txt", bloqueIndex);
        ifstream inEliminado(rutaEliminado);
        if (inEliminado.is_open()) {
            string primeraLinea;
            getline(inEliminado, primeraLinea);
            inEliminado.close();

            if (!primeraLinea.empty()) {
                int ignorado1, ignorado2;
                sscanf(primeraLinea.c_str(), "%d#%d#%d", &ignorado1, &ignorado2, &lineaReuso);
            }
        }
    }

    // Continuar con la inserción normal en sectores
    char rutaSectores[100];
    sprintf(rutaSectores, "bloques_sectores/bloque%d.txt", bloqueIndex);
    ifstream listaSectores(rutaSectores);
    if (!listaSectores.is_open()) {
        cerr << "❌ No se encontró el archivo de sectores para bloque " << bloqueIndex << endl;
        return false;
    }

    string sectorPath;
    while (getline(listaSectores, sectorPath)) {
        ifstream sectorIn(sectorPath.c_str());
        if (!sectorIn.is_open()) continue;

        string nombreArchivoSector;
        int tamTotalSector = 0, tamActualSector = 0;
        string linea;

        getline(sectorIn, nombreArchivoSector);
        getline(sectorIn, linea); tamTotalSector = atoi(linea.c_str());
        getline(sectorIn, linea); tamActualSector = atoi(linea.c_str());

        vector<string> registros;
        while (getline(sectorIn, linea)) registros.push_back(linea);
        sectorIn.close();

        if (nombreArchivoSector == archivoConTxt && tamActualSector + pesoRegistro <= tamTotalSector) {
            if (lineaReuso != -1) {
                int index = lineaReuso - 4; // Las 3 primeras líneas son cabecera
                if (index >= 0 && index < (int)registros.size()) {
                    registros[index] = registro;
                } else {
                    while ((int)registros.size() < index) registros.push_back("");
                    registros.push_back(registro);
                }
            } else {
                registros.push_back(registro);
            }

            ofstream sectorOut(sectorPath.c_str());
            sectorOut << archivoConTxt << "\n";
            sectorOut << tamTotalSector << "\n";
            sectorOut << (tamActualSector + pesoRegistro) << "\n";
            for (const string& r : registros) sectorOut << r << "\n";
            sectorOut.close();

            // Obtener dirección física
            int plato = -1, superficie = -1, pista = -1, sector = -1;
            size_t pos;
            pos = sectorPath.find("Plato_"); if (pos != string::npos) plato = stoi(sectorPath.substr(pos + 6));
            pos = sectorPath.find("Superficie_"); if (pos != string::npos) superficie = stoi(sectorPath.substr(pos + 11));
            pos = sectorPath.find("Pista_"); if (pos != string::npos) pista = stoi(sectorPath.substr(pos + 6));
            pos = sectorPath.find("Sector_"); if (pos != string::npos) sector = stoi(sectorPath.substr(pos + 7));

            stringstream ss;
            ss << " | Pos: Plato " << plato << ", Superficie " << superficie << ", Pista " << pista << ", Sector " << sector;
            direccionOut = ss.str();
            return true;
        }
    }

    return false;
}


// Busca en todos los bloques que pertenezcan al archivo y que tengan espacio en sus sectores
bool insertarEnSectorEnBloquesArchivo(const string& archivoConTxt, const string& registro, int pesoRegistro, int& bloqueIndexOut, string& direccionOut) {
    if (pesoRegistro <= 0) {
        cerr << "❌ Peso de registro inválido: " << pesoRegistro << endl;
        return false;
    }

    int bloqueIndex = 0;
    char ruta[100];

    while (true) {
        sprintf(ruta, "bloques/bloque%d.txt", bloqueIndex);

        ifstream in(ruta);
        if (!in.is_open()) break; // No más bloques

        string cabecera;
        getline(in, cabecera);
        in.close();

        char nombreArchivo[50];
        int lineaInsertar, dummy, peso, numRegs, tamActual, tamTotal;

        sscanf(cabecera.c_str(), "%[^#]#%d#%d#%d#%d#%d#%d",
               nombreArchivo, &lineaInsertar, &dummy, &peso, &numRegs, &tamActual, &tamTotal);

        cout << "Revisando bloque: " << bloqueIndex << " con archivo: " << nombreArchivo << endl;

        // Solo bloques del archivo buscado
        if (archivoConTxt == nombreArchivo) {
            // Intentar insertar en sector dentro de este bloque
            if (insertarRegistroEnSector(bloqueIndex, archivoConTxt, registro, pesoRegistro, direccionOut)) {
                bloqueIndexOut = bloqueIndex;
                return true;
            }
        }

        bloqueIndex++;
    }

    cout << "❌ No hay ningún sector con espacio libre en los bloques de " << archivoConTxt << endl;
    return false;
}


bool crearCarpetaSiNoExiste(const char* ruta) {
    // Intentamos abrir el directorio
    DIR* dir = opendir(ruta);
    if (dir) {
        // El directorio existe
        closedir(dir);
        return true;
    } else if (ENOENT == errno) {
        // No existe, intentamos crear
        if (mkdir(ruta, 0777) == 0) {
            return true;
        } else {
            std::cerr << "Error al crear carpeta: " << ruta << std::endl;
            return false;
        }
    } else {
        // Otro error
        std::cerr << "Error al verificar carpeta: " << ruta << std::endl;
        return false;
    }
}

// Leer cabecera del bloque (línea 0)
bool leerCabeceraBloque(int bloqueIndex, string& cabeceraOut) {
    string ruta = "bloques/bloque" + to_string(bloqueIndex) + ".txt";
    ifstream in(ruta);
    if (!in.is_open()) {
        cerr << "No se pudo abrir bloque " << bloqueIndex << endl;
        return false;
    }
    getline(in, cabeceraOut);
    in.close();
    return true;
}

// Escribir cabecera bloque (línea 0)
bool escribirCabeceraBloque(int bloqueIndex, const string& nuevaCabecera) {
    string ruta = "bloques/bloque" + to_string(bloqueIndex) + ".txt";
    ifstream in(ruta);
    if (!in.is_open()) {
        cerr << "No se pudo abrir bloque " << bloqueIndex << " para lectura." << endl;
        return false;
    }
    vector<string> lineas;
    string linea;
    getline(in, linea); // cabecera vieja
    lineas.push_back(nuevaCabecera); // nueva cabecera
    while (getline(in, linea)) {
        lineas.push_back(linea);
    }
    in.close();

    ofstream out(ruta);
    if (!out.is_open()) {
        cerr << "No se pudo abrir bloque " << bloqueIndex << " para escritura." << endl;
        return false;
    }
    for (auto& l : lineas) {
        out << l << "\n";
    }
    out.close();
    return true;
}

bool actualizarCabeceraBloque(int bloqueIndex, int pesoNuevo, int numRegsNuevo) {
    string cabecera;
    if (!leerCabeceraBloque(bloqueIndex, cabecera)) return false;

    // Formato esperado: nombreArchivo#algo#algo#algo#numRegs#pesoActual#algo
    // Ejemplo: titanic.txt#56#-1#65#54#3510#4000
    // Queremos actualizar el campo numRegs (5to, index 4) y pesoActual (6to, index 5)
    // Además, si el tercer campo (índice 2) es "-1", cambiarlo a "0"
    vector<string> partes;
    size_t pos = 0, prev = 0;
    while ((pos = cabecera.find('#', prev)) != string::npos) {
        partes.push_back(cabecera.substr(prev, pos - prev));
        prev = pos + 1;
    }
    partes.push_back(cabecera.substr(prev));

    if (partes.size() < 7) {
        cerr << "Formato cabecera inválido" << endl;
        return false;
    }

    // Modificar si el tercer campo es -1
    if (partes[2] == "-1") {
        partes[2] = "0";
    }

    partes[4] = to_string(numRegsNuevo);
    partes[5] = to_string(pesoNuevo);

    string nuevaCabecera = partes[0];
    for (size_t i = 1; i < partes.size(); ++i) {
        nuevaCabecera += "#" + partes[i];
    }

    return escribirCabeceraBloque(bloqueIndex, nuevaCabecera);
}


// Actualizar peso actual en sector
bool actualizarPesoSector(const string& sectorPath, int nuevoPeso) {
    ifstream in(sectorPath);
    if (!in.is_open()) {
        cerr << "No se pudo abrir sector " << sectorPath << " para actualizar peso" << endl;
        return false;
    }
    string nombreArchivo;
    string tamTotalStr, tamActualStr;
    vector<string> registros;
    string linea;

    getline(in, nombreArchivo);
    getline(in, tamTotalStr);
    getline(in, tamActualStr);

    while (getline(in, linea)) {
        registros.push_back(linea);
    }
    in.close();

    tamActualStr = to_string(nuevoPeso);

    // Reescribir sector con nuevo peso y mismos registros (menos el que borramos)
    ofstream out(sectorPath);
    if (!out.is_open()) {
        cerr << "No se pudo abrir sector " << sectorPath << " para escritura" << endl;
        return false;
    }
    out << nombreArchivo << "\n";
    out << tamTotalStr << "\n";
    out << tamActualStr << "\n";
    for (auto& r : registros) {
        out << r << "\n";
    }
    out.close();
    return true;
}

bool eliminarRegistro(int bloqueIndex, const string& idEliminar) {
    string rutaSectores = "bloques_sectores/bloque" + to_string(bloqueIndex) + ".txt";
    ifstream listaSectores(rutaSectores);
    if (!listaSectores.is_open()) {
        cerr << "No se encontró archivo de sectores para bloque " << bloqueIndex << endl;
        return false;
    }

    string cabeceraBloque;
    if (!leerCabeceraBloque(bloqueIndex, cabeceraBloque)) return false;

    // Parsear cabecera para obtener peso actual y número de registros y peso registro (cuarto parámetro)
    vector<string> partes;
    size_t pos = 0, prev = 0;
    while ((pos = cabeceraBloque.find('#', prev)) != string::npos) {
        partes.push_back(cabeceraBloque.substr(prev, pos - prev));
        prev = pos + 1;
    }
    partes.push_back(cabeceraBloque.substr(prev));

    if (partes.size() < 7) {
        cerr << "Formato cabecera inválido" << endl;
        return false;
    }

    int pesoActualBloque = stoi(partes[5]);
    int numRegsBloque = stoi(partes[4]);
    int pesoRegistro = stoi(partes[3]);  // cuarto parámetro, índice 3

    int lineaEnBloque = -1;
    string rutaBloque = "bloques/bloque" + to_string(bloqueIndex) + ".txt";
    ifstream bloqueIn(rutaBloque);
    if (!bloqueIn.is_open()) {
        cerr << "No se pudo abrir bloque " << bloqueIndex << endl;
        return false;
    }
    vector<string> lineasBloque;
    string linea;
    while (getline(bloqueIn, linea)) {
        lineasBloque.push_back(linea);
    }
    bloqueIn.close();

    // Buscar registro en bloque (ignorando cabecera)
    for (size_t i = 1; i < lineasBloque.size(); i++) {
        string reg = lineasBloque[i];
        size_t posSep = reg.find('#');
        if (posSep != string::npos) {
            string id = reg.substr(0, posSep);
            if (id == idEliminar) {
                lineaEnBloque = (int)i;
                break;
            }
        }
    }
    if (lineaEnBloque == -1) {
        cout << "No se encontró registro con ID " << idEliminar << " en bloque " << bloqueIndex << endl;
        return false;
    }

    string sectorPath;
    int lineaEnSector = -1;

    crearCarpetaSiNoExiste("registros_eliminados");
    string pathEliminados = "registros_eliminados/bloque" + to_string(bloqueIndex) + ".txt";
    ofstream outEliminados(pathEliminados, ios::app);
    if (!outEliminados.is_open()) {
        cerr << "No se pudo abrir archivo para registros eliminados: " << pathEliminados << endl;
        return false;
    }

    bool eliminado = false;

    while (getline(listaSectores, sectorPath)) {
        ifstream sectorIn(sectorPath);
        if (!sectorIn.is_open()) {
            cerr << "No se pudo abrir sector: " << sectorPath << endl;
            continue;
        }

        string nombreArchivoSector;
        string tamTotalStr, tamActualStr;
        vector<string> registrosSector;
        getline(sectorIn, nombreArchivoSector);
        getline(sectorIn, tamTotalStr);
        getline(sectorIn, tamActualStr);

        while (getline(sectorIn, linea)) {
            registrosSector.push_back(linea);
        }
        sectorIn.close();

        for (size_t i = 0; i < registrosSector.size(); i++) {
            string reg = registrosSector[i];
            size_t posSep = reg.find('#');
            if (posSep != string::npos) {
                string idReg = reg.substr(0, posSep);
                if (idReg == idEliminar) {
                    lineaEnSector = (int)i + 4;  // línea real en sector contando cabecera (3 líneas)

                    // Vaciar línea en el sector
                    registrosSector[i] = "";

                    // Actualizar peso sector
                    int pesoActualSector = stoi(tamActualStr);
                    int nuevoPesoSector = pesoActualSector - pesoRegistro;
                    tamActualStr = to_string(nuevoPesoSector);

                    // Reescribir sector
                    ofstream sectorOut(sectorPath);
                    if (!sectorOut.is_open()) {
                        cerr << "No se pudo abrir sector para escritura: " << sectorPath << endl;
                        return false;
                    }
                    sectorOut << nombreArchivoSector << "\n" << tamTotalStr << "\n" << tamActualStr << "\n";
                    for (auto& r : registrosSector) {
                        sectorOut << r << "\n";
                    }
                    sectorOut.close();

                    // Vaciar línea en el bloque
                    lineasBloque[lineaEnBloque] = "";

                    // Reescribir bloque
                    ofstream bloqueOut(rutaBloque);
                    if (!bloqueOut.is_open()) {
                        cerr << "No se pudo abrir bloque para escritura: " << rutaBloque << endl;
                        return false;
                    }
                    for (const string& l : lineasBloque) {
                        bloqueOut << l << "\n";
                    }
                    bloqueOut.close();

                    // Actualizar cabecera del bloque
                    int nuevoPesoBloque = pesoActualBloque - pesoRegistro;
                    int nuevoNumRegs = numRegsBloque - 1;
                    if (!actualizarCabeceraBloque(bloqueIndex, nuevoPesoBloque, nuevoNumRegs)) return false;

                    // Guardar info de eliminación
                    outEliminados << idEliminar << "#" << lineaEnBloque + 1 << "#" << lineaEnSector << "\n";

                    eliminado = true;
                    break;
                }
            }
        }
        if (eliminado) break;
    }

    outEliminados.close();

    if (!eliminado) {
        cout << "No se encontró registro con ID " << idEliminar << " en sectores del bloque " << bloqueIndex << endl;
        return false;
    }

    cout << "Registro con ID " << idEliminar << " eliminado correctamente." << endl;
    return true;
}


void insertarRegistro() {
    string archivo;
    cout << "Ingrese el nombre del archivo (con .txt): ";
    cin >> archivo;

    string atributos[20];
    int cantidad = 0;
    if (!obtenerAtributos(archivo, atributos, cantidad)) {
        cerr << "❌ Atributos no encontrados para " << archivo << endl;
        return;
    }

    cin.ignore();
    string registro = "";
    for (int i = 0; i < cantidad; ++i) {
        string valor;
        cout << "Ingrese " << atributos[i] << ": ";
        getline(cin, valor);
        if (i > 0) registro += "#";
        registro += valor;
    }

    int pesoRegistro = 0;
    int tmpIndex = 0;
    char rutaBloqueTmp[100];
    while (true) {
        sprintf(rutaBloqueTmp, "bloques/bloque%d.txt", tmpIndex);
        ifstream in(rutaBloqueTmp);
        if (!in.is_open()) break;

        string cab;
        getline(in, cab);
        in.close();

        char nombreArchivo[50];
        int lineaInsertar, dummy, peso, numRegs, tamActual, tamTotal;
        sscanf(cab.c_str(), "%[^#]#%d#%d#%d#%d#%d#%d",
               nombreArchivo, &lineaInsertar, &dummy, &peso, &numRegs, &tamActual, &tamTotal);

        if (archivo == nombreArchivo) {
            pesoRegistro = peso;
            break;
        }
        tmpIndex++;
    }

    if (pesoRegistro == 0) {
        cerr << "❌ No se encontró peso de registro para " << archivo << endl;
        return;
    }

    int bloqueIndexSector = -1;
    string direccionFisica;

    if (!insertarEnSectorEnBloquesArchivo(archivo, registro, pesoRegistro, bloqueIndexSector, direccionFisica)) {
        cerr << "No se pudo insertar registro en sector\n";
        return;
    }

    string registroConDireccion = registro + direccionFisica;

    int bloqueIndexBloque = -1;
    if (!insertarRegistroEnBloque(archivo, registroConDireccion, pesoRegistro, bloqueIndexBloque)) {
        cerr << "No se pudo insertar registro en bloque\n";
        return;
    }

    cout << "✅ Registro insertado correctamente.\n";
}

void eliminarRegistroMenu() {
    int bloque;
    string idEliminar;

    cout << "Ingrese número de bloque para eliminar registro: ";
    cin >> bloque;
    cout << "Ingrese ID del registro a eliminar (primer campo antes de '#'): ";
    cin >> idEliminar;

    if (!eliminarRegistro(bloque, idEliminar)) {
        cout << "Error al eliminar registro." << endl;
    } else {
        cout << "✅ Registro eliminado correctamente." << endl;
    }
}


int main() {
    while (true) {
        cout << "\n----- Menú -----\n";
        cout << "1. Insertar registro\n";
        cout << "2. Eliminar registro\n";
        cout << "3. Salir\n";
        cout << "Seleccione una opción: ";

        int opcion;
        cin >> opcion;

        switch (opcion) {
            case 1:
                insertarRegistro();
                break;
            case 2:
                eliminarRegistroMenu();
                break;
            case 3:
                cout << "Saliendo...\n";
                return 0;
            default:
                cout << "Opción inválida, intente de nuevo.\n";
                break;
        }
    }
}
