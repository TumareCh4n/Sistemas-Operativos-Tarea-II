
#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <pthread.h>
#include <deque>
#include "configuracion.h"

class Matchmaker {
    private:

    struct PergaminoJugador {
        Matchmaker* servidor;
        int jugador;
    };

    struct PergaminoMesa {
        Matchmaker* servidor;
        int numeroMesa;
    };

    int jugadoresInvocados;
    int limiteDeDominios;
    Configuracion configuracion;
    std::deque<int> filaDeEspera;
    pthread_mutex_t cerrojoDelDestino;
    pthread_cond_t campanaDelMatchmaker;
    bool inscripcionesCerradas = false;

    int mesasActivas = 0;
    int maximoObservado = 0;
    int partidasCompletadas = 0;

    static void* invocarJugador(void* contexto);
    static void* abrirDominio(void* contexto);
    void recibirJugador(int jugador);
    void atenderMesa(int numeroMesa);

    public:

    Matchmaker(int jugadoresInvocados, int limiteDeDominios, const Configuracion& configuracion);
    ~Matchmaker();
    Matchmaker(const Matchmaker&) = delete;
    Matchmaker& operator=(const Matchmaker&) = delete;

    void iniciarEmparejamiento();
};

#endif