#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

//Constantes
#define NUM_WORKERS 3
#define NUM_PODS    15
#define PENDING     -1

typedef struct {
    int    id;
    char   nome[20];
    float  cpu_total;     //Nucleos de CPU
    float  mem_total;     //Memoria em MB
    float  disco_total;   //Disco em GB
    float  cpu_livre;
    float  mem_livre;
    float  disco_livre;
    int    num_pods;      //Numero de pods que foram alocados
} Worker;

//POD com seus requisitos
typedef struct {
    int    id;
    char   nome[20];
    float  cpu_req;       //CPU necessaria
    float  mem_req;       //Memoria necessaria
    float  disco_req;     //Disco necessario
    int    worker_alocado; 
} Pod;

//Variaveis Globais
Worker workers[NUM_WORKERS];
Pod pods[NUM_PODS];

//Mutex
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;

//Semaforo: Master sinaliza que ha POD pronto para alocar
sem_t sem_pod_pronto;

//Semaforo: Escalonador sinaliza que alocou o POD
sem_t sem_pod_alocado;

//Indice do POD atual (Escalonado)
int pod_atual = 0;

void inicializar_workers() {
    //Worker 0
    workers[0].id = 0;
    strcpy(workers[0].nome, "Worker-1");
    workers[0].cpu_total = 8.0;
    workers[0].mem_total = 8192.0;  
    workers[0].disco_total = 100.0;  
    workers[0].cpu_livre = workers[0].cpu_total;
    workers[0].mem_livre = workers[0].mem_total;
    workers[0].disco_livre = workers[0].disco_total;
    workers[0].num_pods = 0;

    //Worker 1
    workers[1].id          = 1;
    strcpy(workers[1].nome, "Worker-2");
    workers[1].cpu_total   = 4.0;
    workers[1].mem_total   = 4096.0;  
    workers[1].disco_total = 50.0;
    workers[1].cpu_livre   = workers[1].cpu_total;
    workers[1].mem_livre   = workers[1].mem_total;
    workers[1].disco_livre = workers[1].disco_total;
    workers[1].num_pods    = 0;

    //Worker 2
    workers[2].id          = 2;
    strcpy(workers[2].nome, "Worker-3");
    workers[2].cpu_total   = 16.0;
    workers[2].mem_total   = 16384.0; 
    workers[2].disco_total = 200.0;
    workers[2].cpu_livre   = workers[2].cpu_total;
    workers[2].mem_livre   = workers[2].mem_total;
    workers[2].disco_livre = workers[2].disco_total;
    workers[2].num_pods    = 0;
}

void inicializar_pods() {
    //id, nome, cpu, mem(MB), disco(GB)
    float dados[NUM_PODS][3] = {
        {1.0,  512.0,  5.0},
        {2.0, 1024.0, 10.0},
        {0.5,  256.0,  2.0},
        {4.0, 2048.0, 20.0},
        {1.0,  768.0,  8.0},
        {3.0, 1536.0, 15.0},
        {0.5,  128.0,  1.0},
        {2.0,  512.0,  5.0},
        {8.0, 4096.0, 40.0},
        {1.0,  256.0,  3.0},
        {2.0, 1024.0, 12.0},
        {4.0, 2048.0, 25.0},
        {1.0,  512.0,  6.0},
        {0.5,  256.0,  2.0},
        {3.0, 1024.0, 10.0}
    };

    for (int i = 0; i < NUM_PODS; i++) {
        pods[i].id = i + 1;
        snprintf(pods[i].nome, 20, "Pod-%02d", i + 1);
        pods[i].cpu_req = dados[i][0];
        pods[i].mem_req = dados[i][1];
        pods[i].disco_req = dados[i][2];
        pods[i].worker_alocado = PENDING;
    }
}

//Retorna o worker escolhido, ou PENDING
int escalonar(Pod *pod) {
    int   melhor_worker = PENDING;
    float melhor_score  = -1.0;

    for (int i = 0; i < NUM_WORKERS; i++) {
        //Verifica se o worker tem recursos suficientes
        if (workers[i].cpu_livre >= pod->cpu_req  &&
            workers[i].mem_livre >= pod->mem_req  &&
            workers[i].disco_livre >= pod->disco_req) {

    //Score vai somar o percentual de recursos livres apos alocar. Quanto maior o score, mais "folga" o worker tem
            float score =
                (workers[i].cpu_livre   - pod->cpu_req)  / workers[i].cpu_total  +
                (workers[i].mem_livre   - pod->mem_req)  / workers[i].mem_total  +
                (workers[i].disco_livre - pod->disco_req)/ workers[i].disco_total;

            if (score > melhor_score) {
                melhor_score  = score;
                melhor_worker = i;
            }
        }
    }
    return melhor_worker;
}

void *thread_escalonador(void *arg) {
    for (int i = 0; i < NUM_PODS; i++) {
        sem_wait(&sem_pod_pronto);

        pthread_mutex_lock(&mutex_workers);

        int w = escalonar(&pods[i]);
        pods[i].worker_alocado = w;

        if (w != PENDING) {
            workers[w].cpu_livre   -= pods[i].cpu_req;
            workers[w].mem_livre   -= pods[i].mem_req;
            workers[w].disco_livre -= pods[i].disco_req;
            workers[w].num_pods++;
            printf("  [ESCALONADOR] %s alocado em %s\n",
                   pods[i].nome, workers[w].nome);
        } else {
            printf("  [ESCALONADOR] %s ficou PENDENTE (sem recursos)\n",
                   pods[i].nome);
        }

        pthread_mutex_unlock(&mutex_workers);

        sem_post(&sem_pod_alocado);
    }
    return NULL;
}

void exibir_workers() {
    printf("ESTADO DOS WORKERS:\n");

    for (int i = 0; i < NUM_WORKERS; i++) {
        printf("%s\n", workers[i].nome);
        printf("CPU: %.1f / %.1f nucleos livres\n",
               workers[i].cpu_livre, workers[i].cpu_total);
        printf("MEMO: %.0f / %.0f MB livres\n",
               workers[i].mem_livre, workers[i].mem_total);
        printf("DISCO: %.1f / %.1f GB livres\n",
               workers[i].disco_livre, workers[i].disco_total);
        printf("PODs alocados: %d\n", workers[i].num_pods);
    }
}

void exibir_pods() {
    printf("ALOCACAO FINAL DOS PODS: \n");
    printf("%-10s %-6s %-10s %-8s %-15s \n", "POD", "CPU", "MEMO(MB)", "DISCO", "WORKER");
    
    for (int i = 0; i < NUM_PODS; i++) {
        char destino[20];
        if (pods[i].worker_alocado == PENDING)
            strcpy(destino, "PENDENTE");
        else
            strcpy(destino, workers[pods[i].worker_alocado].nome);

        printf("%-10s %-6.1f %-10.0f %-8.1f %-15s \n",
               pods[i].nome,
               pods[i].cpu_req,
               pods[i].mem_req,
               pods[i].disco_req,
               destino);
    }
}

void exibir_estatisticas() {
    int pendentes = 0;
    for (int i = 0; i < NUM_PODS; i++)
        if (pods[i].worker_alocado == PENDING) pendentes++;

    printf("ESTATISTICAS: \n");
    printf("Total de PODs criados: %d\n", NUM_PODS);
    printf("PODs alocados: %d\n", NUM_PODS - pendentes);
    printf("PODs pendentes: %d\n", pendentes);

    for (int i = 0; i < NUM_WORKERS; i++) {
        float uso_cpu   = (1.0 - workers[i].cpu_livre  /workers[i].cpu_total)  *100;
        float uso_mem   = (1.0 - workers[i].mem_livre  /workers[i].mem_total)  *100;
        float uso_disco = (1.0 - workers[i].disco_livre/workers[i].disco_total)*100;
        printf("║  %s: CPU %.1f%% | MEM %.1f%% | DISCO %.1f%% | %d PODs\n",
               workers[i].nome, uso_cpu, uso_mem, uso_disco, workers[i].num_pods);
    }

    printf("COMPARACAO COM KUBERNETES PADRAO:\n");
    printf("Kubernetes usa apenas CPU + MEMO para escalonar.\n");
    printf("Esta solucao usa CPU + MEMO + DISCO, evitando alocar\n");
    printf("PODs em workers sem espaco em disco suficiente.\n");
}

int main() {
    printf("\n SIMULADOR DE ESCALONAMENTO KUBERNETES \n\n");

    inicializar_workers();
    inicializar_pods();
    sem_init(&sem_pod_pronto,  0, 0);
    sem_init(&sem_pod_alocado, 0, 0);

    printf("[MASTER] Iniciando Workers...\n");
    exibir_workers();

    //Thread do escalonador
    pthread_t tid_escalonador;
    pthread_create(&tid_escalonador, NULL, thread_escalonador, NULL);

    //Master envia os PODs um a um para o escalonador
    printf("\n[MASTER] Enviando PODs para escalonamento...\n\n");
    for (int i = 0; i < NUM_PODS; i++) {
        printf("[MASTER] Submetendo %s (CPU:%.1f MEM:%.0fMB DISCO:%.1fGB)\n",
               pods[i].nome,
               pods[i].cpu_req,
               pods[i].mem_req,
               pods[i].disco_req);

        sem_post(&sem_pod_pronto); 
        sem_wait(&sem_pod_alocado); 
    }

    pthread_join(tid_escalonador, NULL);

    exibir_pods();
    exibir_workers();
    exibir_estatisticas();

    pthread_mutex_destroy(&mutex_workers);
    sem_destroy(&sem_pod_pronto);
    sem_destroy(&sem_pod_alocado);

    printf("\n[MASTER] Escalonamento concluido.\n\n");
    return 0;
}
