#include <iostream>
#include <fstream>


using namespace std;
class Processo {
public:
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

    Processo(int  id_, int chegada, int e1, bool bloqueia, int espera, int e2)
       : id(id_), tempoChegada(chegada), exec1(e1),
         temBloqueio(bloqueia), tempoEsperaIO(espera), exec2(e2)
    {
        tempoRestante = exec1 + (bloqueia ? e2 : 0);
    }

};
int main() {


    return 0;
}