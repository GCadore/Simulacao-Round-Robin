#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <limits> // Necessário para limpar buffer do cin

using namespace std;

// Enum para clareza dos estados
enum EstadoProcesso { AGUARDANDO, PRONTO, EXECUTANDO_P1, BLOQUEADO, EXECUTANDO_P2, TERMINADO };

struct Processo {
    int id;
    int tempoChegada;
    int exec1;
    bool temBloqueio;
    int tempoEsperaIO;
    int exec2;
    int prioridade = 1; // Prioridade (default = 1, maior = mais prioritário -> maior quantum)

    // Controle da simulação
    int tempoRestanteExecAtual;
    int quantumAtual;
    int quantumRestante;
    EstadoProcesso estado;
    int tempoBloqueadoRestante;

    // Métricas
    int tempoInicioExecucao = -1;
    int tempoFinalizacao = -1;
    int trocasDeContexto = 0;
    int tempoEsperaTotal = 0;
    int tempoExecucaoTotal;
    int tempoTurnaround = 0;

    // Auxiliar
    int ultimoTempoPronto = -1;
};

// Estrutura para registrar eventos do Gantt
struct GanttEntry {
    int tempoInicio;
    int tempoFim;
    int processoId;
    int nucleoId;
};

// Função auxiliar para max
int maxInt(int a, int b) {
    return (a > b) ? a : b;
}

// --- Configurações da Simulação (serão definidas via cin) ---
int QUANTUM_BASE = 4; // Mantido como base, mas modo é escolhido
int NUM_NUCLEOS = 2;
bool USA_QUANTUM_DINAMICO = false;

// Função para ler processos do arquivo (lê prioridade opcionalmente)
vector<Processo> lerProcessosDoArquivo(const string& nomeArquivo, bool lerPrioridade) {
    ifstream arquivo(nomeArquivo);
    vector<Processo> processos;

    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir arquivo: " << nomeArquivo << endl;
        return processos;
    }

    string linha;
    string cabecalho;
    getline(arquivo, cabecalho); // Lê (e ignora) cabeçalho

    int linhaNum = 1;
    while (getline(arquivo, linha)) {
        linhaNum++;
        if (linha.empty() || linha[0] == '#') continue;

        stringstream ss(linha);
        Processo p;
        int bloqueioInt;
        char pipe = '|';
        int prioridadeLida = 1;

        // Tenta ler as colunas básicas
        if (!(ss >> p.id >> pipe >> p.tempoChegada >> pipe >> p.exec1 >> pipe >> bloqueioInt >> pipe >> p.tempoEsperaIO >> pipe >> p.exec2)) {
            cerr << "Erro ao ler colunas basicas na linha " << linhaNum << ": " << linha << endl;
            continue;
        }

        // Se modo dinâmico, tenta ler prioridade
        if (lerPrioridade) {
            if (ss.peek() == EOF || !(ss >> pipe >> prioridadeLida)) {
                 cerr << "Erro: Modo quantum dinamico ativo, mas coluna Prioridade nao encontrada ou invalida na linha " << linhaNum << ": " << linha << endl;
                 cerr << "       Certifique-se que o arquivo contem a coluna extra \"| Prioridade\" neste modo." << endl;
                 processos.clear();
                 return processos;
            } else {
                 if (prioridadeLida <= 0) {
                    cerr << "Aviso: Prioridade invalida (<= 0) na linha " << linhaNum << ". Usando prioridade 1." << endl;
                    p.prioridade = 1;
                 } else {
                    p.prioridade = prioridadeLida;
                 }
            }
        } else {
             p.prioridade = 1; // Default no modo fixo
        }

        p.temBloqueio = (bloqueioInt != 0);
        p.estado = AGUARDANDO;
        p.tempoRestanteExecAtual = p.exec1;
        p.quantumAtual = 0;
        p.quantumRestante = 0;
        p.tempoBloqueadoRestante = 0;
        p.tempoExecucaoTotal = p.exec1 + (p.temBloqueio ? p.exec2 : 0);
        p.ultimoTempoPronto = p.tempoChegada;
        processos.push_back(p);
    }
    arquivo.close();
    return processos;
}

// --- Variáveis Globais da Simulação ---
int tempo_global = 0;
vector<Processo> processos_originais;
queue<Processo*> aguardandoChegada;
queue<Processo*> prontos;
queue<Processo*> bloqueados;
vector<Processo> terminados;
vector<Processo*> executando_vec;
vector<GanttEntry> gantt_log;
int total_trocas_contexto = 0;
double tempo_total_ocupado_cpu = 0;

// Função para calcular o quantum (baseado em prioridade se dinâmico)
int calcularQuantum(Processo* p) {
    if (!USA_QUANTUM_DINAMICO) {
        return QUANTUM_BASE;
    } else {
        // Quantum Dinâmico: Base * Prioridade (mínimo 1)
        return maxInt(1, QUANTUM_BASE * p->prioridade);
    }
}

// Função para adicionar entrada ao Log do Gantt
void registrarGantt(int nucleo, int tempoInicio, int tempoFim, int processoId) {
    if (tempoInicio < tempoFim) {
        gantt_log.push_back({tempoInicio, tempoFim, processoId, nucleo});
    }
}

// Atualiza processos que chegaram
void atualizarChegadas() {
    queue<Processo*> temp;
    while (!aguardandoChegada.empty()) {
        Processo* p = aguardandoChegada.front();
        aguardandoChegada.pop();
        if (p->tempoChegada <= tempo_global) {
            p->estado = PRONTO;
            p->quantumAtual = calcularQuantum(p);
            p->quantumRestante = p->quantumAtual;
            p->ultimoTempoPronto = tempo_global;
            p->tempoEsperaTotal += maxInt(0, tempo_global - p->tempoChegada);
            prontos.push(p);
        } else {
            temp.push(p);
        }
    }
    aguardandoChegada = temp;
}

// Atualiza processos bloqueados
void atualizarBloqueados() {
    int size = bloqueados.size();
    for (int i = 0; i < size; ++i) {
        Processo* p = bloqueados.front();
        bloqueados.pop();
        p->tempoBloqueadoRestante--;
        if (p->tempoBloqueadoRestante <= 0) {
            p->estado = PRONTO;
            p->tempoRestanteExecAtual = p->exec2;
            p->quantumAtual = calcularQuantum(p);
            p->quantumRestante = p->quantumAtual;
            p->ultimoTempoPronto = tempo_global;
            prontos.push(p);
        } else {
            bloqueados.push(p);
        }
    }
}

// Incrementa tempo de espera
void atualizarTempoEspera() {
    int size = prontos.size();
    for (int i = 0; i < size; ++i) {
        Processo* p = prontos.front();
        prontos.pop();
        p->tempoEsperaTotal++;
        prontos.push(p);
    }
}

// Função principal de simulação
void simular() {
    for (auto& p : processos_originais) {
        aguardandoChegada.push(&p);
    }

    int processos_ativos = processos_originais.size();
    vector<int> inicio_exec_burst(NUM_NUCLEOS, -1);

    while (processos_ativos > 0) {
        atualizarChegadas();
        atualizarBloqueados();
        atualizarTempoEspera();

        for (int i = 0; i < NUM_NUCLEOS; ++i) {
            Processo* p_atual = executando_vec[i];

            if (p_atual == nullptr) {
                if (!prontos.empty()) {
                    p_atual = prontos.front();
                    prontos.pop();
                    executando_vec[i] = p_atual;
                    total_trocas_contexto++;
                    p_atual->trocasDeContexto++;
                    if (p_atual->tempoInicioExecucao == -1) {
                        p_atual->tempoInicioExecucao = tempo_global;
                    }
                    inicio_exec_burst[i] = tempo_global;
                }
            }

            if (p_atual != nullptr) {
                tempo_total_ocupado_cpu++;
                p_atual->tempoRestanteExecAtual--;
                p_atual->quantumRestante--;

                if (p_atual->tempoRestanteExecAtual <= 0) {
                    registrarGantt(i, inicio_exec_burst[i], tempo_global + 1, p_atual->id);
                    inicio_exec_burst[i] = -1;

                    // Verifica se estava em Exec1 ou Exec2 para decidir próximo estado
                    bool estavaEmExec1 = (p_atual->tempoExecucaoTotal - p_atual->tempoRestanteExecAtual <= p_atual->exec1);

                    if (estavaEmExec1 && p_atual->temBloqueio) {
                        p_atual->estado = BLOQUEADO;
                        p_atual->tempoBloqueadoRestante = p_atual->tempoEsperaIO;
                        bloqueados.push(p_atual);
                        executando_vec[i] = nullptr;
                    } else {
                        p_atual->estado = TERMINADO;
                        p_atual->tempoFinalizacao = tempo_global + 1;
                        p_atual->tempoTurnaround = p_atual->tempoFinalizacao - p_atual->tempoChegada;
                        terminados.push_back(*p_atual);
                        executando_vec[i] = nullptr;
                        processos_ativos--;
                    }
                } else if (p_atual->quantumRestante <= 0) {
                    registrarGantt(i, inicio_exec_burst[i], tempo_global + 1, p_atual->id);
                    inicio_exec_burst[i] = -1;

                    p_atual->estado = PRONTO;
                    p_atual->quantumAtual = calcularQuantum(p_atual);
                    p_atual->quantumRestante = p_atual->quantumAtual;
                    p_atual->ultimoTempoPronto = tempo_global + 1;
                    prontos.push(p_atual);
                    executando_vec[i] = nullptr;
                }
            }
        }

        tempo_global++;

        // Condição de parada: não há mais processos ativos
        if (processos_ativos <= 0) {
             bool nucleos_ociosos = true;
             for(int k=0; k<NUM_NUCLEOS; ++k) {
                 if (executando_vec[k] != nullptr) {
                     nucleos_ociosos = false;
                     // Registra burst final se algum núcleo ainda estava ativo
                     if(inicio_exec_burst[k] != -1) {
                         registrarGantt(k, inicio_exec_burst[k], tempo_global, executando_vec[k]->id);
                     }
                 }
             }
             // Verifica se realmente acabou tudo (importante se houver chegadas tardias)
             if (nucleos_ociosos && aguardandoChegada.empty() && prontos.empty() && bloqueados.empty()) {
                 break;
             }
        }

        // Limite de segurança
        if (tempo_global > 30000) {
            cerr << "Erro: Simulacao excedeu limite de tempo (30000)!" << endl;
             for(int k=0; k<NUM_NUCLEOS; ++k) {
                 if (executando_vec[k] != nullptr && inicio_exec_burst[k] != -1) {
                     registrarGantt(k, inicio_exec_burst[k], tempo_global, executando_vec[k]->id);
                 }
             }
            break;
        }
    }
}

// Calcula e imprime as métricas finais
void calcularEImprimirMetricas() {
    if (terminados.empty()) {
        cout << "Nenhum processo terminou." << endl;
        return;
    }

    double soma_turnaround = 0;
    double soma_espera = 0;

    cout << "\n--- Metricas de Desempenho ---" << endl;
    cout << "PID\tCheg\tExec1\tBlk?\tEspIO\tExec2";
    if (USA_QUANTUM_DINAMICO) cout << "\tPrio";
    cout << "\tFinal\tTurn\tEspTot\tTrocas" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------" << endl;

    // Imprime na ordem de término
    for (int i = 0; i < terminados.size(); ++i) {
        const Processo& p = terminados[i];
        soma_turnaround += p.tempoTurnaround;
        soma_espera += p.tempoEsperaTotal;

        cout << p.id << "\t"
             << p.tempoChegada << "\t"
             << p.exec1 << "\t"
             << (p.temBloqueio ? "S" : "N") << "\t"
             << (p.temBloqueio ? p.tempoEsperaIO : 0) << "\t"
             << (p.temBloqueio ? p.exec2 : 0) << "\t";
        if (USA_QUANTUM_DINAMICO) cout << p.prioridade << "\t";
        cout << p.tempoFinalizacao << "\t"
             << p.tempoTurnaround << "\t"
             << p.tempoEsperaTotal << "\t"
             << p.trocasDeContexto << endl;
    }
    cout << "-------------------------------------------------------------------------------------------------------------" << endl;

    double turnaround_medio = terminados.empty() ? 0 : soma_turnaround / terminados.size();
    double espera_media = terminados.empty() ? 0 : soma_espera / terminados.size();
    double utilizacao_cpu = (tempo_global > 0 && NUM_NUCLEOS > 0) ? (tempo_total_ocupado_cpu / (double)(NUM_NUCLEOS * tempo_global)) * 100.0 : 0.0;

    cout << "\nResultados Gerais:" << endl;
    cout << "- Quantum Utilizado: " << (USA_QUANTUM_DINAMICO ? "Dinamico (Base: " : "Fixo (Base: ") << QUANTUM_BASE << ", Prio based)" << endl;
    cout << "- Tempo Total de Simulacao: " << tempo_global << endl;
    cout << "- Turnaround Medio: " << turnaround_medio << endl;
    cout << "- Tempo de Espera Medio: " << espera_media << endl;
    cout << "- Utilizacao Media da CPU: " << (int)(utilizacao_cpu * 100) / 100.0 << "%" << endl;
    cout << "- Numero Total de Trocas de Contexto: " << total_trocas_contexto << endl;
}

// Imprime o Gantt Chart textual (VISUAL MELHORADO)
void imprimirGantt() {
    cout << "\n--- Linha do Tempo (Gantt Chart Textual) ---" << endl;

    if (gantt_log.empty() && tempo_global == 0) {
         cout << "Simulacao nao executou (tempo global 0)." << endl;
         return;
    }

    int max_tempo = tempo_global;
    if (max_tempo == 0) return;

    // Cria representações textuais para cada núcleo
    vector<string> gantt_linhas(NUM_NUCLEOS);
    int labelWidth = 8; // Largura para "Nucleo X: "
    for (int i = 0; i < NUM_NUCLEOS; ++i) {
        stringstream ss_label;
        ss_label << "Nucleo " << i << ": ";
        gantt_linhas[i] = ss_label.str();
        while(gantt_linhas[i].length() < labelWidth) gantt_linhas[i] += " ";
        for(int t=0; t<max_tempo; ++t) gantt_linhas[i] += " - "; // 3 chars por tempo
    }

    // Preenche as linhas do Gantt
    for (int j = 0; j < gantt_log.size(); ++j) {
        const GanttEntry& entry = gantt_log[j];
        if (entry.nucleoId >= 0 && entry.nucleoId < NUM_NUCLEOS) {
            for (int t = entry.tempoInicio; t < entry.tempoFim; ++t) {
                int pos = labelWidth + t * 3;
                if (pos + 2 < gantt_linhas[entry.nucleoId].length()) {
                    string pid_str = "P" + std::to_string(entry.processoId);
                    gantt_linhas[entry.nucleoId][pos] = pid_str[0];
                    gantt_linhas[entry.nucleoId][pos + 1] = (pid_str.length() > 1) ? pid_str[1] : ' ';
                    if (pid_str.length() > 2) gantt_linhas[entry.nucleoId][pos + 1] = '*';
                }
            }
        }
    }

    // Imprime as linhas do Gantt
    for (int i = 0; i < NUM_NUCLEOS; ++i) {
        cout << gantt_linhas[i] << endl;
    }

    // Imprime escala de tempo
    cout << string(labelWidth, ' ');
    for(int t = 0; t < max_tempo; ++t) {
        cout << (t % 10 == 0 ? " T" : "  ");
    }
    cout << endl;

    cout << string(labelWidth, ' ');
    for(int t = 0; t < max_tempo; ++t) {
        stringstream ss_time;
        ss_time << t;
        string time_str = ss_time.str();
        if (t % 10 == 0) {
             cout << time_str[0] << (time_str.length() > 1 ? time_str[1] : ' ') << (time_str.length() > 2 ? time_str[2] : ' ');
        } else if (t % 5 == 0) {
             cout << " + ";
        } else {
             cout << " . ";
        }
    }
    cout << endl;
    cout << string(labelWidth, ' ') << "Legenda: P<ID> (Processo), - (Ocioso), + (Marca 5s), T<Tempo> (Marca 10s)" << endl;
}

// Função para limpar o buffer do cin
void limparBufferCin() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() { // Removido argc, argv
    string nomeArquivo;
    char escolhaQuantum;
    int escolhaNucleos;

    // --- Coleta de Entrada Interativa ---
    cout << "--- Configuracao da Simulacao ---" << endl;

    // Nome do arquivo
    cout << "Digite o nome do arquivo de processos (ex: processos.txt): ";
    cin >> nomeArquivo;
    limparBufferCin();

    // Modo Quantum
    while (true) {
        cout << "Usar quantum dinamico baseado em prioridade? (s/n): ";
        cin >> escolhaQuantum;
        limparBufferCin();
        escolhaQuantum = tolower(escolhaQuantum);
        if (escolhaQuantum == 's' || escolhaQuantum == 'n') {
            USA_QUANTUM_DINAMICO = (escolhaQuantum == 's');
            break;
        } else {
            cout << "Entrada invalida. Digite 's' para sim ou 'n' para nao." << endl;
        }
    }

    // Número de Núcleos
    while (true) {
        cout << "Digite o numero de nucleos (2 ou 4): ";
        cin >> escolhaNucleos;
        if (cin.fail()) { // Verifica se a entrada foi um número
            cout << "Entrada invalida. Por favor, digite um numero." << endl;
            cin.clear(); // Limpa o estado de erro do cin
            limparBufferCin();
        } else if (escolhaNucleos == 2 || escolhaNucleos == 4) {
            NUM_NUCLEOS = escolhaNucleos;
            limparBufferCin();
            break;
        } else {
            cout << "Numero de nucleos invalido. Escolha 2 ou 4." << endl;
            limparBufferCin();
        }
    }

    // --- Fim da Coleta de Entrada ---

    // Inicializa o vetor de execução com base no NUM_NUCLEOS
    executando_vec.resize(NUM_NUCLEOS, nullptr);

    // Lê o arquivo de processos, passando a flag se deve ler prioridade
    processos_originais = lerProcessosDoArquivo(nomeArquivo, USA_QUANTUM_DINAMICO);
    if (processos_originais.empty()) {
        cerr << "Erro ao ler processos ou arquivo \"" << nomeArquivo << "\" vazio/invalido." << endl;
        return 1;
    }

    cout << "\nIniciando Simulacao Round Robin" << endl;
    cout << "Arquivo de Processos: " << nomeArquivo << endl;
    cout << "Numero de Nucleos: " << NUM_NUCLEOS << endl;
    cout << "Modo Quantum: " << (USA_QUANTUM_DINAMICO ? "Dinamico (Prioridade)" : "Fixo") << " (Base: " << QUANTUM_BASE << ")" << endl;
    cout << "----------------------------------------" << endl;

    simular();

    cout << "\n----------------------------------------" << endl;
    cout << "Simulacao Finalizada." << endl;

    calcularEImprimirMetricas();
    imprimirGantt();

    return 0;
}
