#include "mapa.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;

Mapa::Mapa() {}

int Mapa::obtenerID(int fila, int columna)const{
	
	if(fila < 0 || fila >= 5|| columna <0||columna >=5){
	return -1;
	}	 
	
return fila * 5 + columna; 
}

void Mapa::conectarPosiciones(int origen, int destino) {

posiciones[origen].vecinos.push_back(destino);

posiciones[destino].vecinos.push_back(origen);
}


void Mapa::despertarNen() {
    posiciones.clear();
    posiciones.reserve(28);

	posicionPuma = -1;
	perrosCapturados = 0;
	partidaIniciada = false;
	ganador = Ganador::NINGUNO;

	for (int i = 0; i < 28; ++i){
	posiciones.push_back({ i, Jugadores::VACIO,{}, 0 });
}

	//Posiciones de los cuadraos de abajo
	for(int f = 0; f < 5; ++f){
		for(int c = 0; c < 5; ++c){
			
			int actual = obtenerID(f,c); 
			if(c < 4) { //conectamos con la derecha
				conectarPosiciones(actual, obtenerID(f,c +1));
			}
			if(f < 4) { //Conectamo con abajo
				conectarPosiciones(actual, obtenerID(f + 1,c));
			}
		}
	}
			//conexiones del trono
			conectarPosiciones(REY, TopL);
			conectarPosiciones(REY, TopR);
			conectarPosiciones(REY, 2);
	
			conectarPosiciones(TopL, TopR);
			conectarPosiciones(TopL, 0);
			conectarPosiciones(TopL, 1);

			conectarPosiciones(TopR, 3);
			conectarPosiciones(TopR, 4);


	cout << "<MAPA> El Nen del mapa ha despertado! : " << posiciones.size() << " posiciones activas." << endl; }

	bool Mapa::posicionValida(int id) const{
			return id >= 0 && id < static_cast<int>(posiciones.size());
}

	bool Mapa::posicionLibre(int id) const{
	
	return posicionValida(id) && posiciones[id].jugadores == Jugadores:: VACIO;
}

	bool Mapa::sonAdyacentes(int origen, int destino) const {
		
		if(!posicionValida(origen) || !posicionValida(destino)) {
			return false;
		}


		const vector<int>& vecinos = posiciones[origen].vecinos;

		return find(vecinos.begin(),vecinos.end(),destino) != vecinos.end();
	}

	string Mapa::nombrePosicion(int id) const{

		if(id == TopL ) return "TopL";
		if(id == TopR ) return "TopR";
		if(id == REY ) return "REY";

		if (id < 0 || id >= 25) {
			return "INVALIDA";
		}

		ostringstream salida;

		salida << setw(2) << setfill('0') << id;

		return salida.str();
}

int Mapa::obtenerFicha(int posicion) const{
	if(!posicionValida(posicion)) return -1;
	return posiciones[posicion].idFicha;
}

int Mapa::obtenerPerrosCapturados() const{
	return perrosCapturados;
}

Ganador Mapa::obtenerGanador() const{
	return ganador;
}

bool Mapa::colocarFicha(int ficha, int destino){

	if(partidaIniciada || ficha < 1 || ficha > 13 || !posicionLibre(destino)) return false;

	//No queremos dos Pumas ni perros clonados, po
	for(const Posicion& posicion : posiciones){
		if(posicion.idFicha == ficha) return false;
	}

	posiciones[destino].idFicha = ficha;
	posiciones[destino].jugadores = (ficha == 1) ? Jugadores::PUMA : Jugadores::PERRO;
	if(ficha == 1) posicionPuma = destino;
	return true;
}

bool Mapa::comenzarPartida(){

	if(partidaIniciada) return false;

	int pumas = 0;
	int perros = 0;

	for(const Posicion& posicion : posiciones){
		if(posicion.jugadores == Jugadores::PUMA) ++pumas;
		if(posicion.jugadores == Jugadores::PERRO) ++perros;
	}

	if(pumas != 1 || perros != 12) return false;

	partidaIniciada = true;
	revisarVictoria();
	return true;
}

//No todos los caminos de dos casillas son saltos rectos.
bool Mapa::saltoValido(int origen, int intermedio, int destino) const{

	if(!sonAdyacentes(origen,intermedio) || !sonAdyacentes(intermedio,destino)) return false;

	//Dos pasos horizontales o verticales en la cuadricula
	if(origen < 25 && intermedio < 25 && destino < 25){
		int filaO = origen / 5, colO = origen % 5;
		int filaI = intermedio / 5, colI = intermedio % 5;
		int filaD = destino / 5, colD = destino % 5;

		return filaO + filaD == 2 * filaI && colO + colD == 2 * colI;
	}

	//Saltos del trono: solo las lineas que siguen rectas
	auto caminoRecto = [=](int a, int b, int c){
		return (origen == a && intermedio == b && destino == c) ||
		       (origen == c && intermedio == b && destino == a);
	};

	return caminoRecto(REY,TopL,0) || caminoRecto(REY,TopR,4) ||
	       caminoRecto(TopL,1,6) || caminoRecto(TopR,3,8) ||
	       caminoRecto(REY,2,7);
}

bool Mapa::pumaTieneMovimientos() const{

	if(!posicionValida(posicionPuma)) return false;

	for(int vecino : posiciones[posicionPuma].vecinos){
		if(posicionLibre(vecino)) return true;

		if(posiciones[vecino].jugadores == Jugadores::PERRO){
			for(int destino = 0; destino < 28; ++destino){
				if(posicionLibre(destino) && saltoValido(posicionPuma,vecino,destino)) return true;
			}
		}
	}

	return false;
}

void Mapa::revisarVictoria(){

	if(!partidaIniciada || ganador != Ganador::NINGUNO) return;

	if(perrosCapturados >= 6){
		ganador = Ganador::PUMA;
		return;
	}

	if(!pumaTieneMovimientos()) ganador = Ganador::PERROS;
}

ResultadoMovimiento Mapa::intentarMovimiento(int ficha, int origen, int destino){

	if(!partidaIniciada) return ResultadoMovimiento::NO_INICIADA;
	if(ganador != Ganador::NINGUNO) return ResultadoMovimiento::PARTIDA_TERMINADA;

	if(ficha < 1 || ficha > 13 || !posicionValida(origen) || !posicionValida(destino) || origen == destino){
		return ResultadoMovimiento::INVALIDO;
	}

	if(posiciones[origen].jugadores == Jugadores::VACIO){
		return ResultadoMovimiento::SIN_FICHA;
	}

	if(posiciones[origen].idFicha != ficha){
		return ResultadoMovimiento::FICHA_AJENA;
	}

	if(!posicionLibre(destino)){
		return ResultadoMovimiento::OCUPADO;
	}

	if(sonAdyacentes(origen,destino)){
		posiciones[destino].jugadores = posiciones[origen].jugadores;
		posiciones[destino].idFicha = ficha;
		posiciones[origen].jugadores = Jugadores::VACIO;
		posiciones[origen].idFicha = 0;
		if(ficha == 1) posicionPuma = destino;

		revisarVictoria();
		return ResultadoMovimiento::EXITOSO;
	}

	//Solo el Puma puede saltar y comerse a los Perros
	if(ficha == 1){
		for(int intermedio : posiciones[origen].vecinos){
			if(posiciones[intermedio].jugadores != Jugadores::PERRO) continue;
			if(!saltoValido(origen,intermedio,destino)) continue;

			posiciones[intermedio].jugadores = Jugadores::VACIO;
			posiciones[intermedio].idFicha = 0;
			posiciones[origen].jugadores = Jugadores::VACIO;
			posiciones[origen].idFicha = 0;
			posiciones[destino].jugadores = Jugadores::PUMA;
			posiciones[destino].idFicha = 1;
			posicionPuma = destino;
			++perrosCapturados;

			revisarVictoria();
			return ResultadoMovimiento::CAPTURA;
		}
	}

	return ResultadoMovimiento::INVALIDO;
}

void Mapa::revelarNen() const {
    cout << "[MAPA] El Mapa ha sido revelado!" << endl;

    if(posiciones.empty()) {
        cout << "[MAPA] Primero hay que despertar el Nen." << endl;
        return;
    }

    auto simbolo = [this](int id) -> string {
        int ficha = posiciones[id].idFicha;
        if(ficha == 0) return "..";
        if(ficha == 1) return "PU";

        ostringstream salida;
        salida << "D" << setw(2) << setfill('0') << ficha - 1;
        return salida.str();
    };

    cout << "              [REY:" << simbolo(REY) << "]" << endl;
    cout << "        [TopL:" << simbolo(TopL) << "] [TopR:" << simbolo(TopR) << "]" << endl << endl;

    for(int f = 0; f < 5; ++f){
        for(int c = 0; c < 5; ++c){
            int id = obtenerID(f,c);
            cout << "[" << nombrePosicion(id) << ":" << setw(3) << simbolo(id) << "]";
            if(c < 4) cout << "--";
        }
        cout << endl;
        if(f < 4) cout << "    |         |         |         |         |" << endl;
    }

    cout << "Perros capturados: " << perrosCapturados << "/6" << endl;

    if(ganador == Ganador::PUMA) cout << "[JUEZ] VICTORIA DEL PUMA!" << endl;
    if(ganador == Ganador::PERROS) cout << "[JUEZ] VICTORIA DE LOS PERROS!" << endl;
}

	//Encuentra en que casilla anda cada bicho.
int Mapa::buscarFicha(int ficha) const{

	for(const Posicion& posicion : posiciones){
		if(posicion.idFicha == ficha) return posicion.id;
	}

	return -1; //Se lo comieron o aun no aparece
}

//Movimientos posibles de una ficha. La Mesa debe llamar esto con el mutex tomado.
vector<int> Mapa::movimientosDisponibles(int ficha) const{

	vector<int> destinos;
	if(ficha < 1 || ficha > 13 || ganador != Ganador::NINGUNO || !partidaIniciada) return destinos;

	int origen = buscarFicha(ficha);
	if(!posicionValida(origen)) return destinos;

	for(int vecino : posiciones[origen].vecinos){
		if(posicionLibre(vecino)) destinos.push_back(vecino);
	}

	//Los perros no saltan. El Puma si puede comerse alguno.
	if(ficha == 1){
		for(int vecino : posiciones[origen].vecinos){
			if(posiciones[vecino].jugadores != Jugadores::PERRO) continue;

			for(int destino = 0; destino < 28; ++destino){
				if(posicionLibre(destino) && saltoValido(origen,vecino,destino)){
					destinos.push_back(destino);
				}
			}
		}
	}

	return destinos;
}
