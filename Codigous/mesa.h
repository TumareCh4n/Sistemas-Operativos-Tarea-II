#ifndef MESA_H
#define MESA_H

#include "mapa.h"
#include "configuracion.h"
#include <pthread.h>
#include <atomic>

class Mesa {

    private:

    struct ContextoFicha {
        Mesa* mesa;
        int ficha;
    };

    int idMesa;
    Configuracion configuracion;
    std::atomic<bool>* zaWarudo; //El aviso de interrupcion del torneo entero
    Mapa mapa;
    pthread_t hilos[13];
    pthread_t hiloJuez;
    ContextoFicha pergaminos[13];
    pthread_mutex_t expansionDeDominio;
    pthread_cond_t avisoDelJuez;

    std::atomic<bool> detener{false};
    int movimientosEjecutados = 0;
    bool resultadoValido = false;

    static void* manifestarFicha(void* contexto);
    static void* invocarJuez(void* contexto);
    void arbitrarPartida();

    public:

    Mesa(int idMesa, const Configuracion& configuracion, std::atomic<bool>& zaWarudo);
    ~Mesa();

    Mesa(const Mesa&) = delete;
    Mesa& operator=(const Mesa&) = delete;

    bool iniciarExpansion();
};

#endif