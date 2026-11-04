
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

//Joder, por qué tengo que hacer esto? si el codigo con las funciones ahí mismo se veian de pana
#include "torneo.h"

int main(int cantidadPergaminos, char *pergaminos[]) {

    //invocacion:
    // ./torneo N M
    // ./torneo N M config.txt

    if(cantidadPergaminos != 3 && cantidadPergaminos != 4) {

        cout << "Ritual Fallido! Faltan Datos UwU!" << endl;

        return EXIT_FAILURE;
    }

    int JugadoresInvocados;
    int limiteDeDominios;

    try {
        JugadoresInvocados = stoi(pergaminos[1]);
        limiteDeDominios = stoi(pergaminos[2]);

    } catch (const exception&) {
        cout << "Ritual Fallido! Asegurate de que N y M sean numeros enteros OwO." << endl;
        return EXIT_FAILURE;
    }

    if(JugadoresInvocados < 1) {
        cout << "Ritual Fallido! No puede haber torneo sin jugadores ewe." << endl;
        return EXIT_FAILURE;
    }

    if(limiteDeDominios < 1) {
        cout << "Ritual Fallido! No puede haber torneo sin mesas xD." << endl;
        return EXIT_FAILURE;
    }

    string ReglasDelJuego;

    if(cantidadPergaminos == 4) {
        ReglasDelJuego = pergaminos[3];
    }
    cout<< "==============================" << endl << "     TORNEO DE KOMIKAN WAMOO    " << endl << "==============================" << endl;
    cout<< "Ritual exitoso! Se invocaran " << JugadoresInvocados << " jugadores y se jugaran en " << limiteDeDominios << " dominios." << endl;

    if(!ReglasDelJuego.empty()) {
        cout << "Se usara el pergamino de reglas: " << ReglasDelJuego << endl;
    } else {
        cout << "Se usaran las reglas por defecto." << endl;
    }

    Torneo torneo(JugadoresInvocados, limiteDeDominios, ReglasDelJuego);

    torneo.iniciarRitual();

    return EXIT_SUCCESS;
}
