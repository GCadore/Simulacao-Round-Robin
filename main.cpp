#include <iostream>
#include <fstream>


using namespace std;
struct Processo {
    int id;                 //
    int tempoChegada;       // Quando chega no sistema
    int exec1;              // Duração do primeiro trecho de CPU
    bool temBloqueio;       // true se houver bloqueio após exec1
    int tempoEsperaIO;      // Duração do bloqueio (espera)
    int exec2;              // Duração da execução após retorno


    // Campos auxiliares
    int tempoRestante;
    int quantumRestante = 0;
    int estado = 0;
    int tempoBloqueado = 0;
    int tempoDeInicioExecucao = -1;
    int tempoFinalizacao = -1;
    int trocasDeContexto = 0;
};

int main() {
    Processo p1;

    return 0;
}