#include "torneo.h"
#include <iostream> 
using namespace std;

Torneo::torneo(int jugadoresInvocados, int limiteDeDominios, const string& ReglasDelJuego) {
    this->jugadoresInvocados = jugadoresInvocados;
    this->limiteDeDominios = limiteDeDominios;
    this->ReglasDelJuego = ReglasDelJuego;
}

void torneo iniciarRitual() {
    cout << "Iniciando Ritual de Emparejamiento:" << endl;
    cout << "Jugadores Invocados: " << jugadoresInvocados << endl;
    cout << "Limite de Dominios Simultaneos: " << limiteDeDominios << endl;

    if(!ReglasDelJuego.empty()) {
        cout << "Se leyeron las reglas! son: " << ReglasDelJuego << endl;
    } else {
        cout << "No se están recibiendo las reglas, raaaro..." << endl;
    }

    cout << "Tamos Listos... Marichiwew!" << endl;
}