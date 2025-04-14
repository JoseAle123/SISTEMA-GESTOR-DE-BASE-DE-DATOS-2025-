#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <direct.h>  // Para _mkdir en Windows
#include <io.h>      // Para _access
#include <algorithm>

class Tabla {
public:
    static void crearEsquema(const std::string& nombreArchivoTabla) {
        std::ifstream archivo("tablas\\" + nombreArchivoTabla);
        if (!archivo.is_open()) {
            std::cerr << "No se pudo abrir la tabla: " << nombreArchivoTabla << std::endl;
            return;
        }

        std::string linea;
        if (!std::getline(archivo, linea)) {
            std::cerr << "El archivo está vacío." << std::endl;
            archivo.close();
            return;
        }

        archivo.close();

        std::stringstream ss(linea);
        std::string campo;
        std::vector<std::string> campos;

        while (std::getline(ss, campo, '#')) {
            campos.push_back(campo);
        }

        std::string nombreTabla = nombreArchivoTabla.substr(0, nombreArchivoTabla.find('.'));
        std::ofstream esquema("esquemas\\esquema.txt", std::ios::app);  // app: para no sobrescribir

        if (!esquema.is_open()) {
            std::cerr << "No se pudo abrir el archivo de esquema." << std::endl;
            return;
        }

        esquema << nombreTabla;
        for (const auto& c : campos) {
            std::string tipo;
            std::cout << "Ingresa el tipo de dato para \"" << c << "\" (por ejemplo: str, int): ";
            std::cin >> tipo;
            esquema << "#" << c << "#" << tipo;
        }
        esquema << "\n";

        esquema.close();
        std::cout << "Esquema guardado exitosamente en tablas\\esquema.txt\n";
    }

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
        crearEsquema(nombreArchivoSalida);
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

    static void realizarConsulta(const std::string& consulta) {
        if (consulta.front() != '&' || consulta.back() != '#') {
            std::cerr << "Consulta inválida. La consulta debe comenzar con '&' y terminar con '#'.\n";
            return;
        }
    
        std::string consultaLimpia = consulta.substr(1, consulta.size() - 2);  // sin & ni #
        
        // Buscar posiciones de SELECT y FROM
        size_t posSelect = consultaLimpia.find("SELECT");
        size_t posFrom = consultaLimpia.find("FROM");
    
        if (posSelect == std::string::npos || posFrom == std::string::npos) {
            std::cerr << "La consulta debe contener 'SELECT' y 'FROM'.\n";
            return;
        }
    
        std::string camposStr = consultaLimpia.substr(posSelect + 6, posFrom - (posSelect + 6));
        std::string tabla = consultaLimpia.substr(posFrom + 4);
    
        // Limpiar espacios
        camposStr.erase(std::remove_if(camposStr.begin(), camposStr.end(), ::isspace), camposStr.end());
        tabla.erase(std::remove_if(tabla.begin(), tabla.end(), ::isspace), tabla.end());
    
        // Leer esquema
        std::ifstream esquema("esquemas\\esquema.txt");
        if (!esquema.is_open()) {
            std::cerr << "No se pudo abrir el archivo de esquema.\n";
            return;
        }
    
        bool tablaExistente = false;
        std::string linea;
        std::vector<std::string> camposTabla;
        while (std::getline(esquema, linea)) {
            std::stringstream ssLinea(linea);
            std::string nombreTabla;
            std::getline(ssLinea, nombreTabla, '#');
    
            if (nombreTabla == tabla) {
                tablaExistente = true;
                std::string campo;
                while (std::getline(ssLinea, campo, '#')) {
                    camposTabla.push_back(campo);
                }
                break;
            }
        }
    
        if (!tablaExistente) {
            std::cerr << "La tabla '" << tabla << "' no existe en los esquemas.\n";
            return;
        }
    
        // Procesar campos seleccionados
        std::vector<std::string> camposSeleccionados;
        if (camposStr == "*") {
            camposSeleccionados = camposTabla;
        } else {
            std::stringstream ssCampos(camposStr);
            std::string campo;
            while (std::getline(ssCampos, campo, ',')) {
                auto it = std::find(camposTabla.begin(), camposTabla.end(), campo);
                if (it == camposTabla.end()) {
                    std::cerr << "El campo '" << campo << "' no existe en el esquema de la tabla '" << tabla << "'.\n";
                    return;
                }
                camposSeleccionados.push_back(campo);
            }
        }
    
        // Abrir la tabla
        std::ifstream archivo("tablas\\" + tabla + ".txt");
        if (!archivo.is_open()) {
            std::cerr << "No se pudo abrir la tabla '" << tabla << "'.\n";
            return;
        }
    
        // Leer cabecera
        std::getline(archivo, linea);
        std::stringstream ssHeader(linea);
        std::vector<std::string> header;
        while (std::getline(ssHeader, linea, '#')) {
            header.push_back(linea);
        }
    
        // Obtener índices de los campos seleccionados
        std::vector<int> indices;
        for (const auto& campo : camposSeleccionados) {
            auto it = std::find(header.begin(), header.end(), campo);
            if (it != header.end()) {
                indices.push_back(std::distance(header.begin(), it));
            }
        }
    
        // Imprimir resultado
        std::cout << "Contenido de la tabla '" << tabla << "':\n";
        for (const auto& campo : camposSeleccionados) {
            std::cout << campo << "\t";
        }
        std::cout << "\n";
    
        while (std::getline(archivo, linea)) {
            std::stringstream ssLinea(linea);
            std::vector<std::string> valores;
            std::string valor;
            while (std::getline(ssLinea, valor, '#')) {
                valores.push_back(valor);
            }
    
            for (int idx : indices) {
                if (idx < valores.size()) {
                    std::cout << valores[idx] << "\t";
                } else {
                    std::cout << "NULL\t";
                }
            }
            std::cout << "\n";
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
        std::cout << "3. Realizar una consultae\n";
        std::cout << "0. Salir\n";
        std::cout << "Seleccione una opcion: ";
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
        else if (opcion == 3) {
            std::string consulta;
            std::cout << "Realizar consulta ejemplo (&SELECT * FROM titanic#) o (&SELECT Name, Age FROM titanic#): ";
            std::getline(std::cin, consulta);
            Tabla::realizarConsulta(consulta);
        }
        else if (opcion == 0) {
            std::cout << "Saliendo del programa.\n";
        }
        else {
            std::cout << "Opcion inválida. Intente nuevamente.\n";
        }

    } while (opcion != 0);

    return 0;
}
