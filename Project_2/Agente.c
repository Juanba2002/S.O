#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_SIZE 100

typedef struct {
    char nombre[MAX_SIZE];
    int hora;
    int personas;
} SolicitudReserva;

typedef struct {
    char nombre[MAX_SIZE];
    char archivoSolicitudes[MAX_SIZE];
    char pipeCrecibe[MAX_SIZE];
} AgenteConfig;

int main(int argc, char *argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc != 7) {
        fprintf(stderr, "Uso: %s -s nombre -a archivosolicitudes -p pipecrecibe\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parsear argumentos de línea de comandos para el agente
    AgenteConfig config;
    strcpy(config.nombre, "");
    strcpy(config.archivoSolicitudes, "");
    strcpy(config.pipeCrecibe, "");

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-s") == 0) {
            strcpy(config.nombre, argv[i + 1]);
        } else if (strcmp(argv[i], "-a") == 0) {
            strcpy(config.archivoSolicitudes, argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            strcpy(config.pipeCrecibe, argv[i + 1]);
        }
    }

  
    // Abrir pipe para enviar solicitudes al controlador
    int fd_enviar = open(config.pipeCrecibe, O_WRONLY);
    if (fd_enviar == -1) {
        perror("Error al abrir el pipe para enviar solicitudes");
        exit(EXIT_FAILURE);
    }
   

    // Enviar nombre al controlador para registro
    write(fd_enviar, config.nombre, MAX_SIZE);
    close(fd_enviar);
    
    // Abrir pipe para recibir respuestas del controlador
    int fd_recibir = open(config.pipeCrecibe, O_RDONLY);
    if (fd_recibir == -1) {
        perror("Error al abrir el pipe para recibir respuestas");
        exit(EXIT_FAILURE);
    }

    // Abrir el archivo de solicitudes en el agente
    FILE *archivo = fopen(config.archivoSolicitudes, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de solicitudes");
        exit(EXIT_FAILURE);
    }

    while (1) {
        SolicitudReserva solicitud;
        // Leer una línea del archivo
        if (fscanf(archivo, "%[^,],%d,%d\n", solicitud.nombre, &solicitud.hora, &solicitud.personas) != 3) {
            break;  // Fin de archivo
        }
        printf("Solicitud enviada");

        // Enviar la solicitud al controlador
        write(fd_enviar, &solicitud, sizeof(SolicitudReserva));

        // Esperar respuesta del controlador
        char respuesta[MAX_SIZE];
        read(fd_recibir, respuesta, MAX_SIZE);
        printf("Respuesta recibida");
        printf("Respuesta para %s: %s\n", solicitud.nombre, respuesta);

        // Simular un intervalo de tiempo entre envíos de solicitudes (puedes ajustar esto según tus necesidades)
        sleep(1);
    }

    // Cerrar el archivo
    fclose(archivo);

    // Cerrar el pipe para recibir respuestas
    close(fd_recibir);

    // Mensaje de finalización
    printf("Agente %s termina.\n", config.nombre);

    return 0;
}



