#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>


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


vector<Processo> lerProcessosDoArquivo(const string& nomeArquivo) {
    ifstream arquivo(nomeArquivo);
    vector<Processo> processos;

    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << nomeArquivo << endl;
        return processos;
    }

    string linha;
    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        Processo p;
        int bloqueioInt;

        // Lê os valores na ordem
        if (ss >> p.id >> p.tempoChegada >> p.exec1 >> bloqueioInt >> p.tempoEsperaIO >> p.exec2) {
            p.temBloqueio = (bloqueioInt != 0);  // Converte int para bool
            processos.push_back(p);
        } else {
            cerr << "Linha mal formatada: " << linha << std::endl;
        }
    }

    return processos;
}

void distribuirProcessos(const vector<Processo>& processos,
                         queue<Processo>& prontos,
                         queue<Processo>& aguardandoChegada) {
    for (const Processo& p : processos) {
        if (p.tempoChegada == 0) {
            prontos.push(p);
        } else {
            aguardandoChegada.push(p);
        }
    }
}
void imprimirFila(const std::string& nome, std::queue<Processo> fila) {
    std::cout << "\n== " << nome << " ==\n";

    if (fila.empty()) {
        std::cout << "(vazia)\n";
        return;
    }

    while (!fila.empty()) {
        const Processo& p = fila.front();
        std::cout << "ID: " << p.id
                  << " Chegada: " << p.tempoChegada
                  << " Exec1: " << p.exec1
                  << " Bloqueio: " << (p.temBloqueio ? "Sim" : "Nao")
                  << " EsperaIO: " << p.tempoEsperaIO
                  << " Exec2: " << p.exec2
                  << std::endl;
        fila.pop();
    }
}

int main() {
    int quantum = 10;
    int tempo_global = 0;
    queue<Processo> prontos;
    queue<Processo> aguardandoChegada;
    queue<Processo> bloqueados;
    queue<Processo> terminados;
    string nomeArquivo = "processos.txt";
    vector<Processo> processos = lerProcessosDoArquivo(nomeArquivo);
    distribuirProcessos(processos, prontos, aguardandoChegada);
    imprimirFila("Fila de Prontos", prontos);
    imprimirFila("Fila de Aguardando Chegada", aguardandoChegada);
    imprimirFila("Fila de Bloqueados", bloqueados);
    imprimirFila("Fila de Terminados", terminados);

  /*  while (!prontos.empty() || !aguardandoChegada.empty() || !bloqueados.empty()) {
        // Atualiza estado dos aguardandoChegada
        // Executa processo em CPU (Round Robin)
        // Verifica se algum processo termina ou bloqueia
        // Atualiza tempo de bloqueio
        // tempo++;
    } */

    return 0;
}