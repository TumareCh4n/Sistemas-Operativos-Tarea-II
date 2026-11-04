
#include "mesa.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>
#include <chrono>
#include <thread>

using namespace std;

Mesa::Mesa(int idMesa, const Configuracion& configuracion) : idMesa(idMesa), configuracion(configuracion) {

    if(pthread_mutex_init(&expansionDeDominio, nullptr) != 0){
        throw runtime_error("No se pudo iniciar la expansion de dominio.");
    }
    if(pthread_cond_init(&avisoDelJuez, nullptr) != 0){
        pthread_mutex_destroy(&expansionDeDominio);
        throw runtime_error("No se pudo invocar el aviso del Juez.");
    }
}

Mesa::~Mesa(){
    pthread_cond_destroy(&avisoDelJuez);
    pthread_mutex_destroy(&expansionDeDominio);
}

void* Mesa::manifestarFicha(void* contexto){

    ContextoFicha* pergamino = static_cast<ContextoFicha*>(contexto);
    Mesa* mesa = pergamino->mesa;
    int ficha = pergamino->ficha;

    //Cada bicho tira su propio dado. Nada de compartir rand().
    unsigned int semilla = mesa->configuracion.semilla
                        ^ static_cast<unsigned int>(ficha * 7919 + mesa->idMesa * 97);

    while(true){
        unsigned int intervalo = static_cast<unsigned int>(mesa->configuracion.maxDelay - mesa->configuracion.minDelay) + 1U;
        int espera = mesa->configuracion.minDelay + static_cast<int>(rand_r(&semilla) % intervalo);
        this_thread::sleep_for(chrono::milliseconds(espera));

        pthread_mutex_lock(&mesa->expansionDeDominio);

        if(mesa->detener || mesa->mapa.obtenerGanador() != Ganador::NINGUNO){
            pthread_mutex_unlock(&mesa->expansionDeDominio);
            break;
        }

        int origen = mesa->mapa.buscarFicha(ficha);
        if(origen == -1){ //El Puma se lo comio, F
            pthread_mutex_unlock(&mesa->expansionDeDominio);
            break;
        }

        vector<int> posibles = mesa->mapa.movimientosDisponibles(ficha);

        if(!posibles.empty()){
            int destino = posibles[rand_r(&semilla) % posibles.size()];
            ResultadoMovimiento resultado = mesa->mapa.intentarMovimiento(ficha,origen,destino);

            if(resultado == ResultadoMovimiento::EXITOSO || resultado == ResultadoMovimiento::CAPTURA){
                ++mesa->movimientosEjecutados;

                cout << "[MESA " << mesa->idMesa << "] "
                     << (ficha == 1 ? "PUMA" : "PERRO " + to_string(ficha - 1))
                     << ": " << mesa->mapa.nombrePosicion(origen)
                     << " -> " << mesa->mapa.nombrePosicion(destino);

                if(resultado == ResultadoMovimiento::CAPTURA) cout << " [CAPTURA]";
                cout << endl;

                //Avisamos al Juez, no hace falta que este preguntando sin parar.
                pthread_cond_signal(&mesa->avisoDelJuez);
            }
        }

        pthread_mutex_unlock(&mesa->expansionDeDominio);
    }

    return nullptr;
}

void* Mesa::invocarJuez(void* contexto){

    Mesa* mesa = static_cast<Mesa*>(contexto);
    mesa->arbitrarPartida();
    return nullptr;
}

void Mesa::arbitrarPartida(){

    cout << "\n[JUEZ " << idMesa << "] Preparando campo de batalla..." << endl;
    mapa.despertarNen();

    if(!mapa.colocarFicha(1, configuracion.posicionPuma)){
        cout << "[JUEZ] No pude colocar al Puma, tremendo ritual fallido." << endl;
        return;
    }

    //La formacion la dicta el pergamino, nada de perros naciendo donde quieran
    for(int perro = 1; perro <= 12; ++perro){
        if(!mapa.colocarFicha(perro + 1, configuracion.posicionesPerros[perro - 1])){
            cout << "[JUEZ] Fallo al colocar al Perro " << perro << endl;
            return;
        }
    }

    if(!mapa.comenzarPartida()){
        cout << "[JUEZ] La partida no pudo comenzar." << endl;
        return;
    }

    cout << "[JUEZ " << idMesa << "] Partida inaugurada con 13 fichas." << endl;
    mapa.revelarNen();

    detener = false;
    movimientosEjecutados = 0;

    if(mapa.obtenerGanador() != Ganador::NINGUNO){
        cout << "[JUEZ] La partida ya tenia ganador al comenzar." << endl;
        return;
    }

    int creados = 0;
    for(int i = 0; i < 13; ++i){
        pergaminos[i] = {this, i + 1};
        int error = pthread_create(&hilos[i], nullptr, Mesa::manifestarFicha, &pergaminos[i]);

        if(error != 0){
            cout << "[JUEZ] No se pudo manifestar la ficha " << i + 1 << " (" << error << ")" << endl;
            break;
        }
        ++creados;
    }

    bool falloDeInvocacion = creados != 13;
    bool tiempoAgotado = false;

    if(!falloDeInvocacion){
        //Cinco segundos SOLO durante nuestras pruebas, no es una regla del juego.
        timespec horaLimite;
        clock_gettime(CLOCK_REALTIME, &horaLimite);
        horaLimite.tv_sec += 5;

        pthread_mutex_lock(&expansionDeDominio);
        int ultimoResumen = 0;

        while(!detener && mapa.obtenerGanador() == Ganador::NINGUNO){
            int respuesta = pthread_cond_timedwait(&avisoDelJuez, &expansionDeDominio, &horaLimite);

            if(movimientosEjecutados - ultimoResumen >= 15){
                ultimoResumen = movimientosEjecutados;
                cout << "\n[JUEZ " << idMesa << "] Revision del tablero. Movimientos: " << movimientosEjecutados << endl;
                mapa.revelarNen();
            }

            if(respuesta == ETIMEDOUT){
                tiempoAgotado = true;
                break;
            }
            if(respuesta != 0){
                cout << "[JUEZ] Fallo esperando el aviso (" << respuesta << ")" << endl;
                falloDeInvocacion = true;
                break;
            }
        }

        detener = true;
        pthread_cond_broadcast(&avisoDelJuez);
        pthread_mutex_unlock(&expansionDeDominio);
    }
    else {
        pthread_mutex_lock(&expansionDeDominio);
        detener = true;
        pthread_cond_broadcast(&avisoDelJuez);
        pthread_mutex_unlock(&expansionDeDominio);
    }

    //Los 13 threads deben salir ANTES de liberar el tablero.
    for(int i = 0; i < creados; ++i){
        pthread_join(hilos[i], nullptr);
    }

    cout << "\n[JUEZ " << idMesa << "] Reuni todos los hilos. Movimientos: " << movimientosEjecutados << endl;
    mapa.revelarNen();

    if(falloDeInvocacion){
        cout << "[JUEZ] Partida abortada por fallo al iniciar o supervisar los hilos." << endl;
    }
    else if(mapa.obtenerGanador() == Ganador::PUMA){
        cout << "[JUEZ] Victoria del Puma!" << endl;
    }
    else if(mapa.obtenerGanador() == Ganador::PERROS){
        cout << "[JUEZ] Victoria de los Perros!" << endl;
    }
    else if(tiempoAgotado){
        cout << "[JUEZ] Tiempo de prueba agotado, sin ganador." << endl;
    }

    cout << "[JUEZ " << idMesa << "] Mesa disponible otra vez." << endl;
}

void Mesa::iniciarExpansion(){

    cout << "\n[MESA " << idMesa << "] EXPANSION DE DOMINIO!" << endl;

    int error = pthread_create(&hiloJuez, nullptr, Mesa::invocarJuez, this);
    if(error != 0){
        cout << "[MESA] No se pudo crear el hilo Juez (" << error << ")" << endl;
        return;
    }

    pthread_join(hiloJuez, nullptr);
}
