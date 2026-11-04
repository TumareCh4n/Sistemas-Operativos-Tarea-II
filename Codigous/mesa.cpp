#include "mesa.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std;

Mesa::Mesa(int idMesa, const Configuracion& configuracion, atomic<bool>& zaWarudo)
    : idMesa(idMesa), configuracion(configuracion), zaWarudo(&zaWarudo) {

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

    while(!mesa->detener.load() && !mesa->zaWarudo->load()){
        unsigned int intervalo = static_cast<unsigned int>(mesa->configuracion.maxDelay - mesa->configuracion.minDelay) + 1U;
        int espera = mesa->configuracion.minDelay + static_cast<int>(rand_r(&semilla) % intervalo);

        //Dormimos por trozos: Ctrl+C puede detenernos sin esperar un delay enorme.
        while(espera > 0 && !mesa->detener.load() && !mesa->zaWarudo->load()){
            int trozo = min(espera, 50);
            this_thread::sleep_for(chrono::milliseconds(trozo));
            espera -= trozo;
        }

        pthread_mutex_lock(&mesa->expansionDeDominio);

        if(mesa->detener.load() || mesa->zaWarudo->load() || mesa->mapa.obtenerGanador() != Ganador::NINGUNO){
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

    if(zaWarudo->load()) return;

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

    detener.store(false);
    movimientosEjecutados = 0;

    if(zaWarudo->load()) return;
    if(mapa.obtenerGanador() != Ganador::NINGUNO){
        cout << "[JUEZ] La partida ya tenia ganador al comenzar." << endl;
        resultadoValido = true;
        return;
    }

    int creados = 0;
    for(int i = 0; i < 13 && !zaWarudo->load(); ++i){
        pergaminos[i] = {this, i + 1};
        int error = pthread_create(&hilos[i], nullptr, Mesa::manifestarFicha, &pergaminos[i]);

        if(error != 0){
            cout << "[JUEZ] No se pudo manifestar la ficha " << i + 1 << " (" << error << ")" << endl;
            break;
        }
        ++creados;
    }

    bool falloDeInvocacion = creados != 13 && !zaWarudo->load();
    bool abandono = false;
    Ganador ganadorPorAbandono = Ganador::NINGUNO;

    //El cronometro solo sirve para el abandono simulado. Sin limite de partida.
    auto inicio = chrono::steady_clock::now();

    if(!falloDeInvocacion && creados == 13){
        pthread_mutex_lock(&expansionDeDominio);
        int ultimoResumen = 0;

        while(!detener.load() && !zaWarudo->load() && mapa.obtenerGanador() == Ganador::NINGUNO){
            timespec proximoAviso;
            clock_gettime(CLOCK_REALTIME, &proximoAviso);
            proximoAviso.tv_nsec += 100000000L;
            if(proximoAviso.tv_nsec >= 1000000000L){
                ++proximoAviso.tv_sec;
                proximoAviso.tv_nsec -= 1000000000L;
            }

            int respuesta = pthread_cond_timedwait(&avisoDelJuez, &expansionDeDominio, &proximoAviso);
            if(respuesta != 0 && respuesta != ETIMEDOUT){
                cout << "[JUEZ] Fallo esperando el aviso (" << respuesta << ")" << endl;
                falloDeInvocacion = true;
                break;
            }

            if(movimientosEjecutados - ultimoResumen >= 15){
                ultimoResumen = movimientosEjecutados;
                cout << "\n[JUEZ " << idMesa << "] Revision del tablero. Movimientos: " << movimientosEjecutados << endl;
                mapa.revelarNen();
            }

            auto transcurrido = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - inicio).count();

            //Simulacion controlada del abandono de un jugador (no una desconexion de red real).
            if(configuracion.abandonoSimulado != "NINGUNO" && transcurrido >= configuracion.abandonoMs){
                abandono = true;
                ganadorPorAbandono = (configuracion.abandonoSimulado == "PUMA") ? Ganador::PERROS : Ganador::PUMA;
                break;
            }

        }
        detener.store(true);
        pthread_cond_broadcast(&avisoDelJuez);
        pthread_mutex_unlock(&expansionDeDominio);
    }
    else {
        detener.store(true);
    }

    //Los 13 threads deben salir ANTES de liberar el tablero.
    for(int i = 0; i < creados; ++i){
        pthread_join(hilos[i], nullptr);
    }

    cout << "\n[JUEZ " << idMesa << "] Reuni todos los hilos. Movimientos: " << movimientosEjecutados << endl;
    mapa.revelarNen();

    if(zaWarudo->load()){
        cout << "[ZA WARUDO] Mesa " << idMesa << " cerrada por SIGINT." << endl;
    }
    else if(abandono){
        cout << "[JUEZ " << idMesa << "] Abandono del jugador " << configuracion.abandonoSimulado
             << ". Gana " << (ganadorPorAbandono == Ganador::PUMA ? "PUMA" : "PERROS") << " por abandono." << endl;
        resultadoValido = true;
    }
    else if(falloDeInvocacion){
        //El intento de crear un hilo fallido se interpreta como fallo de su lado.
        cout << "[JUEZ] Fallo interno: gana " << (creados == 0 ? "PERROS" : "PUMA") << " por abandono tecnico." << endl;
    }
    else if(mapa.obtenerGanador() == Ganador::PUMA){
        cout << "[JUEZ] Victoria del Puma!" << endl;
        resultadoValido = true;
    }
    else if(mapa.obtenerGanador() == Ganador::PERROS){
        cout << "[JUEZ] Victoria de los Perros!" << endl;
        resultadoValido = true;
    }

    cout << "[JUEZ " << idMesa << "] Mesa disponible otra vez." << endl;
}

bool Mesa::iniciarExpansion(){

    cout << "\n[MESA " << idMesa << "] EXPANSION DE DOMINIO!" << endl;

    if(zaWarudo->load()) return false;

    int error = pthread_create(&hiloJuez, nullptr, Mesa::invocarJuez, this);
    if(error != 0){
        cout << "[MESA] No se pudo crear el hilo Juez (" << error << ")" << endl;
        return false;
    }

    pthread_join(hiloJuez, nullptr);
    return resultadoValido;
}
