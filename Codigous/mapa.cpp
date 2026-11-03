#include "mapa.h"
#include <iostream>
using namespace std;

Mapa::Mapa() {}

void Mapa::despertarNen() {
    cout << "[MAPA] El nen del mapa ha despertado!" << endl;
}

void Mapa::revelarNen() const {
    cout << "[MAPA] El Mapa ha sido revelado!" << endl;
}