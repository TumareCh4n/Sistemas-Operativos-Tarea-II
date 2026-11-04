
#include "matchmaker.h"
#include "mesa.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <csignal>

using namespace std;

Matchmaker::Matchmaker(int jugadoresInvocados, int limiteDeDominios, const Configuracion& configuracion) {
    this->jugadoresInvocados = jugadoresInvocados;
    this->limiteDeDominios = limiteDeDominios;
    this->configuracion = configuracion;

    if(pthread_mutex_init(&cerrojoDelDestino, nullptr) != 0){
        throw runtime_error("No se pudo proteger la fila del destino.");
    }
    if(pthread_cond_init(&campanaDelMatchmaker, nullptr) != 0){
        pthread_mutex_destroy(&cerrojoDelDestino);
        throw runtime_error("No se pudo instalar la campana del Matchmaker.");
    }
}

Matchmaker::~Matchmaker(){
    pthread_cond_destroy(&campanaDelMatchmaker);
    pthread_mutex_destroy(&cerrojoDelDestino);
}

void* Matchmaker::vigilarElTiempo(void* contexto){
    Matchmaker* servidor = static_cast<Matchmaker*>(contexto);

    sigset_t señales;
    sigemptyset(&señales);
    sigaddset(&señales, SIGINT);
    sigaddset(&señales, SIGUSR1); //Despierta al vigilante al cerrar normalmente

    int recibida = 0;
    if(sigwait(&señales, &recibida) == 0 && recibida == SIGINT){
        pthread_mutex_lock(&servidor->cerrojoDelDestino);
        servidor->zaWarudo.store(true);
        servidor->inscripcionesCerradas = true;
        pthread_cond_broadcast(&servidor->campanaDelMatchmaker);
        pthread_mutex_unlock(&servidor->cerrojoDelDestino);
        cout << "\n[ZA WAAARUDO! Ctrl+C recibido! No entraran mas jugadores." << endl;
    }

    return nullptr;
}

void Matchmaker::recibirJugador(int jugador){
    pthread_mutex_lock(&cerrojoDelDestino);
    if(!zaWarudo.load()){
        filaDeEspera.push_back(jugador);
        pthread_cond_broadcast(&campanaDelMatchmaker);
    }
    pthread_mutex_unlock(&cerrojoDelDestino);
}

void* Matchmaker::invocarJugador(void* contexto){
    PergaminoJugador* pergamino = static_cast<PergaminoJugador*>(contexto);
    pergamino->servidor->recibirJugador(pergamino->jugador);
    return nullptr;
}

void* Matchmaker::abrirDominio(void* contexto){
    PergaminoMesa* pergamino = static_cast<PergaminoMesa*>(contexto);
    pergamino->servidor->atenderMesa(pergamino->numeroMesa);
    return nullptr;
}

void Matchmaker::atenderMesa(int numeroMesa){
    while(true){
        pthread_mutex_lock(&cerrojoDelDestino);

        //Si no hay pareja completa, esperamos dormidos, no hacemos busy waiting.
        while(filaDeEspera.size() < 2 && !inscripcionesCerradas && !zaWarudo.load()){
            pthread_cond_wait(&campanaDelMatchmaker, &cerrojoDelDestino);
        }

        if(zaWarudo.load() || (filaDeEspera.size() < 2 && inscripcionesCerradas)){
            pthread_mutex_unlock(&cerrojoDelDestino);
            break;
        }

        int jugadorPuma = filaDeEspera.front();
        filaDeEspera.pop_front();
        int jugadorPerros = filaDeEspera.front();
        filaDeEspera.pop_front();
        ++mesasActivas;
        maximoObservado = max(maximoObservado, mesasActivas);
        pthread_mutex_unlock(&cerrojoDelDestino);

        cout << "[MATCHMAKER] Mesa " << numeroMesa << ": Jugador " << jugadorPuma
             << " sera Puma, Jugador " << jugadorPerros << " controlara los Perros." << endl;

        //Cada mesa construye un mapa nuevo y espera a su propio Juez.
        bool finalizada = false;
        try {
            Mesa mesa(numeroMesa, configuracion, zaWarudo);
            finalizada = mesa.iniciarExpansion();
        }
        catch(const exception& error){
            cout << "[MATCHMAKER] Mesa " << numeroMesa << " fallo: " << error.what() << endl;
        }

        pthread_mutex_lock(&cerrojoDelDestino);
        --mesasActivas;
        if(finalizada) ++partidasCompletadas;
        else ++partidasAbortadas;
        pthread_mutex_unlock(&cerrojoDelDestino);
        cout << "[MATCHMAKER] Mesa " << numeroMesa << " liberada. Puede recibir otra pareja." << endl;
    }
}

void Matchmaker::iniciarEmparejamiento(){
    int cantidadMesas = min(limiteDeDominios, jugadoresInvocados / 2);

    if(cantidadMesas == 0){
        cout << "[MATCHMAKER] No hay parejas suficientes para jugar." << endl;
        return;
    }

    //Bloqueamos señales ANTES de crear los hilos; todos heredan esta mascara.
    sigset_t señales;
    sigemptyset(&señales);
    sigaddset(&señales, SIGINT);
    sigaddset(&señales, SIGUSR1);
    if(pthread_sigmask(SIG_BLOCK, &señales, nullptr) != 0){
        throw runtime_error("No se pudo proteger SIGINT.");
    }

    if(pthread_create(&vigilanteSIGINT, nullptr, Matchmaker::vigilarElTiempo, this) != 0){
        throw runtime_error("No se pudo invocar al vigilante de SIGINT.");
    }

    vector<pthread_t> guardianes(cantidadMesas);
    vector<PergaminoMesa> dominios(cantidadMesas);
    int mesasCreadas = 0;

    for(int i = 0; i < cantidadMesas && !zaWarudo.load(); ++i){
        dominios[i] = {this, i + 1};
        int error = pthread_create(&guardianes[i], nullptr, Matchmaker::abrirDominio, &dominios[i]);
        if(error != 0){
            cout << "[MATCHMAKER] No pude abrir la mesa " << i + 1 << " (" << error << ")." << endl;
            break;
        }
        ++mesasCreadas;
    }

    //No hacemos 10.000 pthread_create simultaneos. Llegan por tandas de hasta 128.
    const int MAX_INVOCACIONES = 128;
    for(int primero = 1; primero <= jugadoresInvocados && mesasCreadas > 0 && !zaWarudo.load(); primero += MAX_INVOCACIONES){
        int cantidad = min(MAX_INVOCACIONES, jugadoresInvocados - primero + 1);
        vector<pthread_t> hilosLlegada;
        hilosLlegada.reserve(cantidad);
        vector<PergaminoJugador> invitaciones(cantidad);

        for(int i = 0; i < cantidad && !zaWarudo.load(); ++i){
            invitaciones[i] = {this, primero + i};
            pthread_t nuevoJugador;
            int error = pthread_create(&nuevoJugador, nullptr, Matchmaker::invocarJugador, &invitaciones[i]);
            if(error != 0){
                cout << "[MATCHMAKER] No pude invocar al jugador " << primero + i
                     << " como hilo (" << error << "). Lo registro directamente." << endl;
                recibirJugador(primero + i);
            }
            else {
                hilosLlegada.push_back(nuevoJugador);
            }
        }

        for(pthread_t hilo : hilosLlegada){
            pthread_join(hilo, nullptr);
        }
    }

    pthread_mutex_lock(&cerrojoDelDestino);
    inscripcionesCerradas = true;
    pthread_cond_broadcast(&campanaDelMatchmaker);
    pthread_mutex_unlock(&cerrojoDelDestino);

    for(int i = 0; i < mesasCreadas; ++i){
        pthread_join(guardianes[i], nullptr);
    }

    //SIGUSR1 se envia solo al vigilante, para que no quede bloqueado en sigwait().
    pthread_kill(vigilanteSIGINT, SIGUSR1);
    pthread_join(vigilanteSIGINT, nullptr);

    cout << "\n[MATCHMAKER] Ritual de emparejamiento terminado." << endl;
    cout << "[MATCHMAKER] Partidas atendidas: " << partidasCompletadas << endl;
    cout << "[MATCHMAKER] Partidas abortadas: " << partidasAbortadas << endl;
    cout << "[MATCHMAKER] Maximo simultaneo observado: " << maximoObservado
         << " de " << limiteDeDominios << " mesas permitidas." << endl;
    if(zaWarudo.load()){
        cout << "[ZAA WARUDOOO!] Se descartaron " << filaDeEspera.size()
             << " jugadores que todavia esperaban. Limpieza completa." << endl;
    }
    else if(!filaDeEspera.empty()){
        cout << "[MATCHMAKER] Jugador " << filaDeEspera.front()
             << " quedo sin pareja. F." << endl;
    }
}