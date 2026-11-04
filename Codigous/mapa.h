
#ifndef MAPA_H
#define MAPA_H

#include <vector>
#include <string>

enum PosicionesDeArriba {
	TopL = 25,
	TopR = 26,
	REY = 27
};

enum class Jugadores{VACIO,PUMA,PERRO};

enum class ResultadoMovimiento{EXITOSO,CAPTURA,INVALIDO,OCUPADO,SIN_FICHA,FICHA_AJENA,NO_INICIADA,PARTIDA_TERMINADA};

enum class Ganador{NINGUNO,PUMA,PERROS};

struct Posicion{
	int id;
	Jugadores jugadores;
	std::vector<int> vecinos;
	int idFicha; // 0 = vacio, 1 = Puma, 2 al 13 = Perros
};

class Mapa {
	private:
	std::vector<Posicion> posiciones;

	int posicionPuma = -1;
	int perrosCapturados = 0;
	bool partidaIniciada = false;
	Ganador ganador = Ganador::NINGUNO;

	void conectarPosiciones(int origen, int destino);
	bool saltoValido(int origen, int intermedio, int destino) const;
	bool pumaTieneMovimientos() const;
	void revisarVictoria();

    public:
      
    Mapa();

    void despertarNen();
    void revelarNen() const;

	bool posicionValida(int Posicion) const;
	bool posicionLibre(int Posicion) const;
	bool sonAdyacentes(int origen, int destino) const;
	int obtenerID(int fila, int columna) const;

	std:: string nombrePosicion(int id) const;

	bool colocarFicha(int ficha, int destino);
	bool comenzarPartida();
	ResultadoMovimiento intentarMovimiento(int ficha, int origen, int destino);
	int obtenerFicha(int posicion) const;
	int obtenerPerrosCapturados() const;
	Ganador obtenerGanador() const;

	int buscarFicha(int ficha) const;
	std::vector<int> movimientosDisponibles(int ficha) const;
};

#endif
