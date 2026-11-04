#ifndef TORNEO_H
#define TORNEO_H

#include <string>

class Torneo {

    private:

        int jugadoresInvocados;
        int limiteDeDominios;
        std::string ReglasDelJuego;

    public:

        Torneo(
            int jugadoresInvocados,
            int limiteDeDominios,
            const std::string& ReglasDelJuego
        );

        void iniciarRitual();
};

#endif