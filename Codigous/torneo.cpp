#include "torneo.h"
#include <iostream> 
using namespace std;

Torneo::Torneo(int jugadoresInvocados, int limiteDeDominios, const string& ReglasDelJuego) {
    this->jugadoresInvocados = jugadoresInvocados;
    this->limiteDeDominios = limiteDeDominios;
    this->ReglasDelJuego = ReglasDelJuego;
};

void Torneo::iniciarRitual() {
    cout << "Iniciando Ritual de Emparejamiento:" << endl;
    cout << "Jugadores Invocados: " << jugadoresInvocados << endl;
    cout << "Limite de Dominios Simultaneos: " << limiteDeDominios << endl;

    if(!ReglasDelJuego.empty()) {
        cout << "Reglas del Juego recibidas: " << ReglasDelJuego << endl;
    } else {
        cout << "No se están recibiendo las reglas, raaaro..." << endl;
    }

    cout << "Tamos Listos... Marichiwew!" << endl;
}