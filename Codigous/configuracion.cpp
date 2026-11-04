
#include "configuracion.h"
#include "mapa.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <climits>
#include <cctype>

using namespace std;

//Quitamos espacios antes y despues de cada dato, no los de adentro.
static string quitarEspacios(const string& texto){
    size_t primero = texto.find_first_not_of(" \t\r\n");
    if(primero == string::npos) return "";
    size_t ultimo = texto.find_last_not_of(" \t\r\n");
    return texto.substr(primero, ultimo - primero + 1);
}

static int descifrarNumero(const string& texto, int linea){
    try {
        size_t leidos = 0;
        long long numero = stoll(texto, &leidos);
        if(leidos != texto.size() || numero < INT_MIN || numero > INT_MAX) throw invalid_argument("numero invalido");
        return static_cast<int>(numero);
    } catch(const exception&) {
        throw runtime_error("Pergamino corrupto en linea " + to_string(linea) + ": numero invalido ('" + texto + "').");
    }
}

static int descifrarPosicion(const string& texto, int linea){
    if(texto == "REY") return REY;
    if(texto == "TopL") return TopL;
    if(texto == "TopR") return TopR;

    int posicion = descifrarNumero(texto,linea);
    if(posicion < 0 || posicion >= 28){
        throw runtime_error("Pergamino corrupto en linea " + to_string(linea) + ": posicion fuera del mapa.");
    }
    return posicion;
}

Configuracion descifrarPergamino(const string& archivo){
    Configuracion reglas;
    if(archivo.empty()) return reglas;

    ifstream pergamino(archivo);
    if(!pergamino.is_open()) throw runtime_error("El pergamino '" + archivo + "' no existe o no se puede abrir.");

    set<string> llavesLeidas;
    string linea;
    int numeroLinea = 0;

    while(getline(pergamino,linea)){
        ++numeroLinea;
        size_t comentario = linea.find('#');
        if(comentario != string::npos) linea = linea.substr(0,comentario);
        linea = quitarEspacios(linea);
        if(linea.empty()) continue;

        size_t igual = linea.find('=');
        if(igual == string::npos || linea.find('=',igual + 1) != string::npos){
            throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": falta CLAVE=VALOR.");
        }

        string llave = quitarEspacios(linea.substr(0,igual));
        string valor = quitarEspacios(linea.substr(igual + 1));
        if(llave.empty() || valor.empty()){
            throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": clave o valor vacio.");
        }
        if(!llavesLeidas.insert(llave).second){
            throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": clave repetida '" + llave + "'.");
        }

        if(llave == "MIN_DELAY") reglas.minDelay = descifrarNumero(valor,numeroLinea);
        else if(llave == "MAX_DELAY") reglas.maxDelay = descifrarNumero(valor,numeroLinea);
    
        else if(llave == "SEED"){
            try {
                if(valor[0] == '-') throw invalid_argument("negativo");
                size_t leidos = 0;
                unsigned long long numero = stoull(valor,&leidos);
                if(leidos != valor.size() || numero > UINT_MAX) throw invalid_argument("fuera de rango");
                reglas.semilla = static_cast<unsigned int>(numero);
            } catch(const exception&) {
                throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": SEED invalida.");
            }
        }
        else if(llave == "ABANDONO"){
            if(valor != "NINGUNO" && valor != "PUMA" && valor != "PERROS"){
                throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": ABANDONO debe ser NINGUNO, PUMA o PERROS.");
            }
            reglas.abandonoSimulado = valor;
        }
        else if(llave == "ABANDONO_MS") reglas.abandonoMs = descifrarNumero(valor,numeroLinea);

        else if(llave == "PUMA") reglas.posicionPuma = descifrarPosicion(valor,numeroLinea);
        else if(llave == "PERROS"){
            if(valor.back() == ',') throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": PERROS tiene una coma extra.");
            stringstream datos(valor);
            string posicion;
            int contador = 0;
            while(getline(datos,posicion,',')){
                if(contador >= 12) throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": hay mas de 12 Perros.");
                posicion = quitarEspacios(posicion);
                reglas.posicionesPerros[contador++] = descifrarPosicion(posicion,numeroLinea);
            }
            if(contador != 12) throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": deben existir 12 Perros.");
        }
        else throw runtime_error("Pergamino corrupto en linea " + to_string(numeroLinea) + ": clave desconocida '" + llave + "'.");
    }

    if(reglas.minDelay < 1 || reglas.maxDelay < 1 || reglas.minDelay > reglas.maxDelay){
        throw runtime_error("MIN_DELAY y MAX_DELAY deben ser positivos y MIN_DELAY <= MAX_DELAY.");
    }

    if(reglas.abandonoMs < 0 || reglas.abandonoMs >= 5000){
        throw runtime_error("ABANDONO_MS debe estar entre 0 y 9999 para nuestras pruebas jeje.");
    }

    set<int> ocupadas;
    ocupadas.insert(reglas.posicionPuma);
    for(int posicion : reglas.posicionesPerros){
        if(!ocupadas.insert(posicion).second){
            throw runtime_error("Dos fichas intentan nacer en la misma posicion " + to_string(posicion) + ".");
        }
    }

    return reglas;
}