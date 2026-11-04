
#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <array>
#include <string>

struct Configuracion {

    int minDelay = 50;
    int maxDelay = 300;
    unsigned int semilla = 12345;

    int posicionPuma = 27; // REY
    std::array<int,12> posicionesPerros = {13,14,15,16,17,18,19,20,21,22,23,24};
};

Configuracion descifrarPergamino(const std::string& archivo);

#endif