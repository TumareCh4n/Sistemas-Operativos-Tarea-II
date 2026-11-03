#ifndef TORNEO_H
#define TORNEO_H
using namespace std;
#include <string>

class torneo {
    private:
        int jugadoresInvocados;
        int limiteDeDominios;
        string ReglasDelJuego;

    public:
        torneo(int jugadoresInvocados, int limiteDeDominios, string ReglasDelJuego);
        void iniciarRitual();
};