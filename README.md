# Simualção Round Robin
Implementação em c++ do escalonador RR e simulação em threads concorrentes


O QUE FAZER:

- criar a struct processos e fila
- ler o txt e criar um vetor de processos com base na leitura
- ordenar o vetor por tempo de entrada pra n dar B.O dps
- criar as seguintes filas: 4 filas representando 4 núcleos de CPU, uma fila de E/S, uma fila de terminados
- entrar num loop while. a condição de parada é a fila de terminados ter como tamnaho a quantidade de processos lidos do txt (ent precisa de uma variável contando a quantidade de processos lidos do txt)
- a cada ciclo:
    - 1 Thread vai ficar responsável por tirar um processo do vetor e jogar numa fila de CPU (se der o tempo)
    - 4 (ou duas) Threads ficam responsáveis por puxar um processo da fila, decrementar o tempo de execução restante e jogar para o final da fila, fila de E/S ou fila de terminados
    - 1 Thread fica respondável por gerenciar a fila de E/S e jogar os processos que terminaram a espera (bloqueados) para a fila de CPU (prontos)
    - 1 Thread fica responsável por incrementar a variável de tempo em 1 ciclo
- é importante usar mutex para garantir que as Threads não tentem acessar as filas ao mesmo tempo
- Exibir os gráficos de saída (não faço ideia de como fazer ainda)

