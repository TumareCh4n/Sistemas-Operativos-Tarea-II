#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cerrno>
using namespace std;

//Joder, por qué tengo que hacer esto? si el codigo con las funciones ahí mismo se veian de pana
#include "torneo.h"

int main(int cantidadPergaminos, char *pergaminos[]) {

    //invocacion:
    // ./torneo N M
    // ./torneo N M config.txt

    if(cantidadPergaminos != 3 && cantidadPergaminos != 4) {

        cout << "Ritual Fallido! (Pusiste " << cantidadPergaminos << " )" << endl;

        return EXIT_FAILURE;
    }

    int JugadoresInvocados;
    int limiteDeDominios;

    try {
        JugadoresInvocados = stoi(pergaminos[1]);
        limiteDeDominios = stoi(pergaminos[2]);

    } catch (const exception&) {
        cout << "Ritual Fallido! Asegurate de que N y M sean numeros enteros." << endl;
        return EXIT_FAILURE;
    }

    if(jugadoresInvocados < 1) {
        cout << "Ritual Fallido! No puede haber torneo sin jugadores." << endl;
        return EXIT_FAILURE;
    }

    if(limiteDeDominios < 1) {
        cout << "Ritual Fallido! No puede haber torneo sin mesas." << endl;
        return EXIT_FAILURE;
    }

    string ReglasDelJuego;

    if(cantidadPergaminos == 4) {
        ReglasdelJuego = pergaminos[3];
    }
    cout<< "==============================" << endl << "     TORNEO DE KOMIKAN WAMOO    " << endl << "==============================" << endl;
    cout<< "Ritual exitoso! Se invocaran " << jugadoresInvocados << " jugadores y se jugaran en " << limiteDeDominios << " dominios." << endl;

    if(!ReglasDelJuego.empty()) {
        cout << "Se usara el pergamino de reglas: " << ReglasDelJuego << endl;
    } else {
        cout << "Se usaran las reglas por defecto." << endl;
    }

    Torneo torneo(jugadoresInvocados, limiteDeDominios, ReglasDelJuego);

    torneo.iniciarRitual();

    return EXIT_SUCCESS;
}
