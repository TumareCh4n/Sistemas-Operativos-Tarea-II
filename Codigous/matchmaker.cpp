
#include "matchmaker.h"
#include "mesa.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>

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

void Matchmaker::recibirJugador(int jugador){
    pthread_mutex_lock(&cerrojoDelDestino);
    filaDeEspera.push_back(jugador);
    pthread_cond_broadcast(&campanaDelMatchmaker);
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
        while(filaDeEspera.size() < 2 && !inscripcionesCerradas){
            pthread_cond_wait(&campanaDelMatchmaker, &cerrojoDelDestino);
        }

        if(filaDeEspera.size() < 2 && inscripcionesCerradas){
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
        try {
            Mesa mesa(numeroMesa, configuracion);
            mesa.iniciarExpansion();
        }
        catch(const exception& error){
            cout << "[MATCHMAKER] Mesa " << numeroMesa << " fallo: " << error.what() << endl;
        }

        pthread_mutex_lock(&cerrojoDelDestino);
        --mesasActivas;
        ++partidasCompletadas;
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

    vector<pthread_t> guardianes(cantidadMesas);
    vector<PergaminoMesa> dominios(cantidadMesas);
    int mesasCreadas = 0;

    for(int i = 0; i < cantidadMesas; ++i){
        dominios[i] = {this, i + 1};
        int error = pthread_create(&guardianes[i], nullptr, Matchmaker::abrirDominio, &dominios[i]);
        if(error != 0){
            cout << "[MATCHMAKER] No pude abrir la mesa " << i + 1 << " (" << error << ")." << endl;
            break;
        }
        ++mesasCreadas;
    }

    if(mesasCreadas == 0){
        cout << "[MATCHMAKER] Ritual fallido: no hay mesas activas." << endl;
        return;
    }

    //No hacemos 10.000 pthread_create simultaneos. Llegan por tandas de hasta 128.
    const int MAX_INVOCACIONES = 128;
    for(int primero = 1; primero <= jugadoresInvocados; primero += MAX_INVOCACIONES){
        int cantidad = min(MAX_INVOCACIONES, jugadoresInvocados - primero + 1);
        vector<pthread_t> hilosLlegada;
        hilosLlegada.reserve(cantidad);
        vector<PergaminoJugador> invitaciones(cantidad);

        for(int i = 0; i < cantidad; ++i){
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

    cout << "\n[MATCHMAKER] Ritual de emparejamiento terminado." << endl;
    cout << "[MATCHMAKER] Partidas atendidas: " << partidasCompletadas << endl;
    cout << "[MATCHMAKER] Maximo simultaneo observado: " << maximoObservado
         << " de " << limiteDeDominios << " mesas permitidas." << endl;
    if(!filaDeEspera.empty()){
        cout << "[MATCHMAKER] Jugador " << filaDeEspera.front()
             << " quedo sin pareja. F." << endl;
    }
}