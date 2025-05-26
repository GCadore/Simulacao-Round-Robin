#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

struct Processo {
    int id;
    int tempoChegada;
    int exec1;
    bool temBloqueio;
    int tempoEsperaIO;
    int exec2;

    // Controle da simulação
    int tempoRestante;       // total restante do trecho atual
    int quantumRestante;
    int estado;              // 0 = exec1, 1 = bloqueio, 2 = exec2, 3 = terminado
    int tempoBloqueado;
    int tempoInicioExecucao = -1;
    int tempoFinalizacao = -1;
    int trocasDeContexto = 0;

    int tempoEsperaTotal = 0;
    int ultimoTempoExecutado = -1;
};

vector<Processo> lerProcessosDoArquivo(const string& nomeArquivo) {
    ifstream arquivo(nomeArquivo);
    vector<Processo> processos;

    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir arquivo: " << nomeArquivo << endl;
        return processos;
    }

    string linha;
    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        Processo p;
        int bloqueioInt;
        if (ss >> p.id >> p.tempoChegada >> p.exec1 >> bloqueioInt >> p.tempoEsperaIO >> p.exec2) {
            p.temBloqueio = (bloqueioInt != 0);
            p.estado = 0;
            p.tempoRestante = p.exec1;
            p.quantumRestante = 0;
            p.tempoBloqueado = 0;
            p.trocasDeContexto = 0;
            p.tempoEsperaTotal = 0;
            processos.push_back(p);
        }
    }
    return processos;
}

const int QUANTUM_FIXO = 4;
const int NUM_NUCLEOS = 2;

void atualizarChegadas(queue<Processo>& aguardandoChegada, queue<Processo>& prontos, int tempo_global) {
    queue<Processo> temp;
    while (!aguardandoChegada.empty()) {
        Processo p = aguardandoChegada.front();
        aguardandoChegada.pop();
        if (p.tempoChegada <= tempo_global) {
            p.quantumRestante = QUANTUM_FIXO;
            prontos.push(p);
        } else {
            temp.push(p);
        }
    }
    aguardandoChegada = temp;
}

void imprimirEstado(int tempo_global, const queue<Processo>& prontos,
                    const queue<Processo>& bloqueados, Processo* executando[]) {
    cout << "Tempo: " << tempo_global << "\nProntos: ";
    queue<Processo> temp = prontos;
    while (!temp.empty()) {
        cout << temp.front().id << " ";
        temp.pop();
    }
    cout << "\nBloqueados: ";
    temp = bloqueados;
    while (!temp.empty()) {
        cout << temp.front().id << " ";
        temp.pop();
    }
    cout << "\nExecutando: ";
    for (int i = 0; i < NUM_NUCLEOS; i++) {
        if (executando[i]) cout << "(Nucleo " << i << ") P" << executando[i]->id << " ";
        else cout << "(Nucleo " << i << ") - ";
    }
    cout << "\n\n";
}

int main() {
    int tempo_global = 0;
    vector<Processo> processos = lerProcessosDoArquivo("processos.txt");

    queue<Processo> aguardandoChegada;
    queue<Processo> prontos;
    queue<Processo> bloqueados;
    queue<Processo> terminados;

    // Inicialmente todos na fila aguardando
    for (auto& p : processos) {
        aguardandoChegada.push(p);
    }

    Processo* executando[NUM_NUCLEOS] = {nullptr};

    while (true) {
        atualizarChegadas(aguardandoChegada, prontos, tempo_global);

        // Atualiza bloqueados
        int bloqueadosSize = (int)bloqueados.size();
        for (int i = 0; i < bloqueadosSize; i++) {
            Processo p = bloqueados.front();
            bloqueados.pop();
            p.tempoBloqueado--;
            if (p.tempoBloqueado <= 0) {
                p.estado = 2; // vai para exec2
                p.tempoRestante = p.exec2;
                p.quantumRestante = QUANTUM_FIXO;
                prontos.push(p);
            } else {
                bloqueados.push(p);
            }
        }

        // Atualiza núcleos
        for (int i = 0; i < NUM_NUCLEOS; i++) {
            Processo* p = executando[i];
            if (p == nullptr) {
                if (!prontos.empty()) {
                    Processo novo = prontos.front();
                    prontos.pop();
                    novo.trocasDeContexto++;
                    if (novo.tempoInicioExecucao == -1) novo.tempoInicioExecucao = tempo_global;
                    novo.quantumRestante = QUANTUM_FIXO;
                    executando[i] = new Processo(novo); // aloca nova cópia para execução
                }
            } else {
                // Executa 1 unidade de tempo
                p->tempoRestante--;
                p->quantumRestante--;
                // Incrementa espera para processos prontos (não executando)
                // (Pode fazer fora do loop principal)
                // Se terminar o trecho atual
                if (p->tempoRestante <= 0) {
                    if (p->estado == 0 && p->temBloqueio) {
                        // Vai para bloqueio
                        p->estado = 1;
                        p->tempoBloqueado = p->tempoEsperaIO;
                        bloqueados.push(*p);
                        delete executando[i];
                        executando[i] = nullptr;
                    } else if (p->estado == 2 || (p->estado == 0 && !p->temBloqueio)) {
                        // Processo terminou
                        p->estado = 3;
                        p->tempoFinalizacao = tempo_global + 1;
                        terminados.push(*p);
                        delete executando[i];
                        executando[i] = nullptr;
                    } else if (p->estado == 1) {
                        // Se estivesse bloqueado (não deve ocorrer aqui)
                    }
                } else if (p->quantumRestante <= 0) {
                    // Quantum acabou, troca processo
                    prontos.push(*p);
                    delete executando[i];
                    executando[i] = nullptr;
                }
            }
        }

        // Incrementa tempo_global
        tempo_global++;

        imprimirEstado(tempo_global, prontos, bloqueados, executando);

        // Condição de parada: tudo terminado?
        if (aguardandoChegada.empty() && prontos.empty() && bloqueados.empty()) {
            bool nucleoVazio = true;
            for (int i = 0; i < NUM_NUCLEOS; i++) {
                if (executando[i] != nullptr) nucleoVazio = false;
            }
            if (nucleoVazio) break;
        }
    }

    // Aqui pode calcular as métricas e imprimir resultados finais

    cout << "Simulacao finalizada em tempo: " << tempo_global << "\n";

    return 0;
}
