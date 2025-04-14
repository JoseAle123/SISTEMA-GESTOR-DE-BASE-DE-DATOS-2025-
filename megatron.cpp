#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <direct.h>  // Para _mkdir en Windows
#include <io.h>      // Para _access

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
        // Verificar que la consulta comienza con '&' y termina con '#'
        if (consulta.front() != '&' || consulta.back() != '#') {
            std::cerr << "Consulta inválida. La consulta debe comenzar con '&' y terminar con '#'.\n";
            return;
        }
    
        // Remover los símbolos '&' y '#'
        std::string consultaLimpia = consulta.substr(1, consulta.size() - 2);
    
        // Dividir la consulta en partes
        std::stringstream ss(consultaLimpia);
        std::string palabraClave, campos, tabla;
    
        // Primer palabra clave: "SELECT"
        ss >> palabraClave;
        if (palabraClave != "SELECT") {
            std::cerr << "La consulta debe comenzar con 'SELECT'.\n";
            return;
        }
    
        // Leer los campos seleccionados (solo soporta "*")
        ss >> campos;
        if (campos != "*") {
            std::cerr << "Solo se soporta 'SELECT *' para seleccionar todos los campos.\n";
            return;
        }
    
        // Leer la palabra clave "FROM"
        std::string from;
        ss >> from;
        if (from != "FROM") {
            std::cerr << "La consulta debe contener 'FROM'.\n";
            return;
        }
    
        // Leer el nombre de la tabla
        ss >> tabla;
    
        // Comprobar si el archivo de esquema de la tabla existe
        std::ifstream esquema("esquemas\\esquema.txt");
        if (!esquema.is_open()) {
            std::cerr << "No se pudo abrir el archivo de esquema.\n";
            return;
        }
    
        bool tablaExistente = false;
        std::string linea;
        while (std::getline(esquema, linea)) {
            std::stringstream ssLinea(linea);
            std::string nombreTabla;
            
            // Leer solo el nombre de la tabla (hasta el primer '#')
            std::getline(ssLinea, nombreTabla, '#');  // Extrae solo el nombre de la tabla
    
            if (nombreTabla == tabla) {
                tablaExistente = true;
                break;
            }
        }
    
        if (!tablaExistente) {
            std::cerr << "La tabla '" << tabla << "' no existe en los esquemas.\n";
            esquema.close();
            return;
        }
    
        // Tabla encontrada, ahora leer el archivo de la tabla
        std::ifstream archivo("tablas\\" + tabla + ".txt");
        if (!archivo.is_open()) {
            std::cerr << "No se pudo abrir la tabla '" << tabla << "'.\n";
            return;
        }
    
        // Leer la primera línea para obtener los nombres de los campos
        std::getline(archivo, linea);
        std::stringstream ssCampos(linea);
        std::vector<std::string> camposTabla;
        std::string campo;
        while (std::getline(ssCampos, campo, '#')) {
            camposTabla.push_back(campo);
        }
    
        // Imprimir todos los campos
        std::cout << "Contenido de la tabla '" << tabla << "':\n";
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
            std::cout << "Realizar consulta ejemplo (&SELECT * FROM titanic#): ";
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
