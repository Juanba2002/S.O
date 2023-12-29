#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



#define HORA_INICIAL_SIMULACION 7
#define HORA_FINAL_SIMULACION 19



#define MAX_SIZE 100

typedef struct {
    char Nombre_agente[MAX_SIZE];
    int hora;
    int personas;
} AgentRequest;

typedef struct {
    int horaInicio;
    int horaFinal;
    int segundoshora;
    int totalpersonas;
    char pipecrecibe[MAX_SIZE];
    sem_t semaforo;  // Semáforo para sincronización
    AgentRequest solicitud;  // Variable compartida para la solicitud del agente
} ControladorConfig;

// Estructura para almacenar información sobre las reservas
typedef struct {
    int reservas[24];                    // Número de reservas por hora
    int personasEnHora[24];              // Cantidad de personas por hora
    int solicitudesTotales[24];          // Número total de solicitudes por hora
    int solicitudesReprogramadas[24];    // Número de solicitudes reprogramadas por hora
} ControladorState;

// Estructura para pasar múltiples argumentos a los hilos
typedef struct {
    ControladorConfig config;
    ControladorState state;
} ThreadArgs;

// Funciones prototipo
void* agenteThread(void* args);
void* relojThread(void* args);
void inicializarControlador(ControladorConfig config, ControladorState* state);
void procesarPeticion(AgentRequest request, ControladorConfig config, ControladorState* state,int horaActual);
void imprimirReporte(ControladorConfig config,ControladorState state);

// Variables globales
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Función principal del controlador
int main(int argc, char *argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc != 11) {
        fprintf(stderr, "Uso: %s -i horaInicio -f horaFinal -s segundoshora -t totalpersonas -p pipecrecibe\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parsear argumentos de línea de comandos
    ControladorConfig config;
    strcpy(config.pipecrecibe, "");  // Inicializar a cadena vacía para evitar problemas
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-i") == 0) {
            config.horaInicio = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-f") == 0) {
            config.horaFinal = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            config.segundoshora = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            config.totalpersonas = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            strcpy(config.pipecrecibe, argv[i + 1]);
        }
    }
  
  if(config.horaFinal > 24){
   fprintf(stderr, "La hora debe ser menor o igual a 24", argv[0]);
        exit(EXIT_FAILURE);
  }
  
  if (unlink(config.pipecrecibe) == -1) {
        perror("Error al eliminar la tubería");
        exit(EXIT_FAILURE);
    }


  if (mkfifo(config.pipecrecibe, 0666) == -1) {
        perror("Error al crear la tubería");
        exit(EXIT_FAILURE);
    }
  
   printf("control");
    ControladorState state;
    inicializarControlador(config, &state);
     
    // Crear hilo para el reloj
    pthread_t reloj_thread;
    ThreadArgs reloj_args = {config, state};
    pthread_create(&reloj_thread, NULL, relojThread, (void*)&reloj_args);
   
    // Crear hilo para el agente
    

    // Esperar que el hilo del reloj termine
    pthread_join(reloj_thread, NULL);
  
 

    
    // Imprimir reporte al finalizar la simulación
    imprimirReporte(config, state);
   
  

    return 0;
}

int encontrarHoraDisponiblePosterior(int horaSolicitada, ControladorConfig config, ControladorState* state) {
    int hora = horaSolicitada + 1; // Comenzar a buscar desde la hora siguiente a la solicitada

    // Recorrer las horas posteriores en el rango de simulación
    while (hora <= config.horaFinal && hora <= 19) {
        // Verificar si hay cupo en la hora actual
        if (state->reservas[hora] < config.totalpersonas) {
            return hora; // Devolver la hora disponible con cupo
        }
        hora++;
    }

    return -1; // Devolver -1 si no se encuentra ninguna hora disponible posterior con cupo
}


// Función para inicializar el estado del controlador
void inicializarControlador(ControladorConfig config, ControladorState* state) {
    memset(state, 0, sizeof(ControladorState));

    // Inicializar el semáforo
    sem_init(&config.semaforo, 0, 0);
}

// Función para procesar una petición de un agente
void procesarPeticion(AgentRequest request, ControladorConfig config, ControladorState* state, int horaActual) {
    pthread_mutex_lock(&mutex);  // Bloquear el mutex para evitar concurrencia en la actualización del estado
    printf("Recibida solicitud de %s para la hora %d con %d personas.\n", request.Nombre_agente, request.hora, request.personas);
    // Verificar si la hora solicitada está dentro del período de simulación
    if (request.hora < config.horaInicio || request.hora > config.horaFinal) {
        printf("Reserva negada, debe volver otro día\n");
    } else {
        // Verificar si la hora solicitada ya pasó
        if (request.hora < horaActual) {
            // Si la hora ya pasó
            printf("Reserva negada, la hora de reserva no puede ser inferior a la hora actual de simulación.\n");
        } else {
            // Verificar si hay cupo en la hora solicitada
            if (state->reservas[request.hora] < config.totalpersonas) {
                printf("Reserva ok. La solicitud es aprobada, las personas pueden entrar en la playa por 2 horas a partir de la hora solicitada.\n");
                // Actualizar el estado del controlador con la nueva reserva
                state->reservas[request.hora]++;
                state->personasEnHora[request.hora] += request.personas;
                state->personasEnHora[request.hora + 1] += request.personas;
            } else {
                // Buscar una hora disponible posterior dentro del rango de la simulación
                int nuevaHora = encontrarHoraDisponiblePosterior(request.hora, config, state);
                if (nuevaHora != -1) {
                    printf("Reserva garantizada para otras horas. Se ha programado para la hora: %d\n", nuevaHora);
                    // Actualizar el estado del controlador con la nueva reserva
                    state->reservas[nuevaHora]++;
                    state->personasEnHora[nuevaHora] += request.personas;
                    state->personasEnHora[nuevaHora + 1] += request.personas;
                    state->solicitudesReprogramadas[nuevaHora]++;  // Corregir aquí
                } else {
                    printf("Reserva negada, debe volver otro día\n");
                }
            }
        }
    }

    pthread_mutex_unlock(&mutex);  // Desbloquear el mutex después de actualizar el estado
}


// Función para imprimir el reporte al finalizar la simulación
void imprimirReporte( ControladorConfig config, ControladorState state) {
    FILE *archivo = fopen("Datos.txt", "w");

    if (archivo == NULL) {
        perror("Error al abrir el archivo para guardar el reporte");
        return;
    }

    fprintf(archivo, "Reporte de la simulación:\n");

    // Inicializar variables para calcular estadísticas
    int maxPersonas = 0;
    int minPersonas = config.totalpersonas + 1; // Inicializar con un valor alto para encontrar el mínimo
    int horaMaxPersonas = -1;
    int horaMinPersonas = -1;
    int solicitudesAceptadas[24] = {0};  // Contador de solicitudes aceptadas por hora
    int solicitudesNegadas[24] = {0};   // Contador de solicitudes negadas por hora
    int solicitudesReprogramadas = 0;

    // Recorrer cada hora en el rango de simulación
    for (int hora = config.horaInicio; hora <= config.horaFinal; hora++) {
        // Calcular estadísticas de personas
        int personasEnHora = state.personasEnHora[hora];
        if (personasEnHora > maxPersonas) {
            maxPersonas = personasEnHora;
            horaMaxPersonas = hora;
        }
        if (personasEnHora < minPersonas) {
            minPersonas = personasEnHora;
            horaMinPersonas = hora;
        }

        // Contar solicitudes aceptadas y negadas
        solicitudesAceptadas[hora] = state.reservas[hora];
        solicitudesNegadas[hora] = state.solicitudesTotales[hora] - state.reservas[hora];

        // Contar solicitudes reprogramadas
        solicitudesReprogramadas += state.solicitudesReprogramadas[hora];

        // Escribir en el archivo
        fprintf(archivo, "Hora %d - Aceptadas: %d, Negadas: %d, Reprogramadas: %d\n",
                hora, solicitudesAceptadas[hora], solicitudesNegadas[hora], state.solicitudesReprogramadas[hora]);
    }

    // Imprimir estadísticas generales en el archivo
    fprintf(archivo, "Hora pico: %d con %d personas\n", horaMaxPersonas, maxPersonas);
    fprintf(archivo, "Hora con menos personas: %d con %d personas\n", horaMinPersonas, minPersonas);

    // Imprimir estadísticas globales en el archivo
    fprintf(archivo, "Total de solicitudes reprogramadas: %d\n", solicitudesReprogramadas);

    fclose(archivo);


    // Imprimir estadísticas generales
    printf("Hora pico: %d con %d personas\n", horaMaxPersonas, maxPersonas);
    printf("Hora con menos personas: %d con %d personas\n", horaMinPersonas, minPersonas);

    // Imprimir estadísticas por hora
    for (int hora = config.horaInicio; hora <= config.horaFinal; hora++) {
        printf("Hora %d - Aceptadas: %d, Negadas: %d, Reprogramadas: %d\n",
               hora, solicitudesAceptadas[hora], solicitudesNegadas[hora], state.solicitudesReprogramadas[hora]);
    }

    // Imprimir estadísticas globales
    printf("Total de solicitudes reprogramadas: %d\n", solicitudesReprogramadas);
}


// Función que representa el hilo del agente
void* agenteThread(void* args) {
    ThreadArgs* agente_args = (ThreadArgs*)args;
    ControladorConfig config = agente_args->config;

    // Abrir pipe para enviar solicitudes al controlador
    int fd_enviar = open(config.pipecrecibe, O_WRONLY);
    if (fd_enviar == -1) {
        perror("Error al abrir el pipe para enviar solicitudes");
        exit(EXIT_FAILURE);
    }

    // Enviar nombre al controlador para registro
    write(fd_enviar, config.solicitud.Nombre_agente, MAX_SIZE);
    close(fd_enviar);

    // Notificar al hilo del reloj que hay una nueva solicitud
    sem_post(&config.semaforo);

    return NULL;
}



// Función que representa el hilo del reloj
void* relojThread(void* args) {
    ThreadArgs* reloj_args = (ThreadArgs*)args;
    ControladorConfig config = reloj_args->config;
    ControladorState* state = &(reloj_args->state);
    int horaActualSimulacion = config.horaInicio;
    AgentRequest solicitud;

    // Abrir el pipe para lectura
    int fd_pipe = open(config.pipecrecibe, O_RDONLY);
    if (fd_pipe == -1) {
        perror("Error al abrir el pipe para lectura");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Leer la solicitud del pipe
       if (read(fd_pipe, &solicitud, sizeof(AgentRequest)) <= 0) {
            break;  // Fin de archivo
        }
      char respuesta[MAX_SIZE];
        sprintf(respuesta, "Respuesta para %s", solicitud.Nombre_agente);
      
      int fd_enviar = open(config.pipecrecibe, O_WRONLY);
        if (fd_enviar == -1) {
            perror("Error al abrir el pipe para enviar respuestas");
            exit(EXIT_FAILURE);}
        write(fd_enviar, respuesta, MAX_SIZE);
        close(fd_enviar);

        // Procesar la solicitud
        procesarPeticion(solicitud, config, state, horaActualSimulacion);

        // Imprimir la hora actual
        printf("Hora actual en la simulación: %d\n", horaActualSimulacion);

        // Dormir hasta la próxima "hora de simulación"
        sleep(config.segundoshora);
        horaActualSimulacion++;

        // Verificar la condición de terminación
        if (horaActualSimulacion > 19) {
            break;  // Salir del bucle si la hora actual es mayor a 19
        }
    }

    // Cerrar el pipe al final del hilo
    close(fd_pipe);

    return NULL;
}