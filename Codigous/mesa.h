#ifndef MESA_H
#define MESA_H

#include "mapa.h"
#include <pthread.h>

class Mesa {

    private:

    struct ContextoFicha {
        Mesa* mesa;
        int ficha;
    };

    int idMesa;
    Mapa mapa;
    pthread_t hilos[13];
    pthread_t hiloJuez;
    ContextoFicha pergaminos[13];
    pthread_mutex_t expansionDeDominio;
    pthread_cond_t avisoDelJuez;

    bool detener = false;
    int movimientosEjecutados = 0;

    static void* manifestarFicha(void* contexto);
    static void* invocarJuez(void* contexto);
    void arbitrarPartida();

    public:

    Mesa(int idMesa);
    ~Mesa();

    Mesa(const Mesa&) = delete;
    Mesa& operator=(const Mesa&) = delete;

    void iniciarExpansion();
};

#endif
