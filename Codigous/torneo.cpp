#include "torneo.h"
#include "matchmaker.h"
#include "configuracion.h"
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

    Configuracion reglas = descifrarPergamino(ReglasDelJuego);
    cout << "[PERGAMINO] Reglas descifradas: delay " << reglas.minDelay << "-" << reglas.maxDelay
         << " ms, semilla " << reglas.semilla << ", Puma en " << reglas.posicionPuma << "." << endl;

    Matchmaker servidor(jugadoresInvocados, limiteDeDominios, reglas);
    servidor.iniciarEmparejamiento();
}