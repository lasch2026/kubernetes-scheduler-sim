Simulador de Escalonamento de PODs - Kubernetes

Trabalho prático da disciplina **Análise e Aplicações de Sistemas Operacionais** 
Universidade do Vale do Rio dos Sinos - UNISINOS 

Descrição

Simulação do escalonador de PODs do Kubernetes em C, utilizando **threads POSIX**, 
**mutex** e **semáforos**. O sistema cria um nó Master e três nós Workers, 
e escalona 15 PODs usando três métricas de alocação.

Arquitetura
MASTER (thread principal) 
-> Thread Escalonador (Algoritmo de score por recursos livres)
-> Worker-1: 8 CPU | 8192 MB RAM | 100 GB Disco
-> Worker-2: 4 CPU | 4096 MB RAM |  50 GB Disco (Worker-3: 16 CPU | 16384 MB RAM | 200 GB Disco)

Métricas de Escalonamento

O algoritmo calcula um **score** para cada Worker elegível:

score = (cpu_livre - cpu_req) / cpu_total
+ (mem_livre - mem_req) / mem_total
+ (disco_livre - disco_req) / disco_total

O Worker com maior score recebe o POD, isto favorece o worker 
com mais folga proporcional de recursos.

Técnicas Utilizadas

- **Threads POSIX** (`pthread_create`, `pthread_join`)
- **Mutex** (`pthread_mutex_lock/unlock`) — protege acesso aos recursos dos Workers
- **Semáforos** (`sem_wait`, `sem_post`) — sincroniza Master e Escalonador
- **Paradigma Produtor-Consumidor** — Master produz PODs, Escalonador consome

Como compilar -> Requisitos
- Linux (ou WSL no Windows)
- GCC instalado

Comparação com Kubernetes Padrão

O Kubernetes padrão aloca PODs considerando apenas **CPU e memória**. 
Esta solução adiciona **espaço em disco** como terceira métrica, evitando 
alocar PODs em Workers que teriam CPU e memória disponíveis mas 
estariam sem espaço em disco — situação real em clusters de produção.

Vídeo de Apresentação
[Assista no YouTube](https://youtu.be/_5VM91LS8nk)
