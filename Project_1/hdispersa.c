#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    int inicio;
    int fin;
    int *matriz;
    int N;
    int ceros;
  int id;
} DatosHilo;

void *contarCeros(void *arg) {
    DatosHilo *datos = (DatosHilo *)arg;
    int ceros = 0;

    for (int i = datos->inicio; i < datos->fin; i++) {
        for (int j = 0; j < datos->N; j++) {
            if (datos->matriz[i * datos->N + j] == 0) {
                ceros++;
            }
        }
    }

    datos->ceros = ceros;
   printf("Hilo %d: \n", datos->id);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    int M = 0, N = 0, nprocesos = 0, porcentaje = 0;
    char *archivo = NULL;

    // ... (código de argumentos y lectura de matriz)
  // Parsear los argumentos de entrada
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
        if (strcmp(argv[i], "-f") == 0) {
            M = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-c") == 0) {
            N = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-a") == 0) {
            archivo = argv[++i];

        } else if (strcmp(argv[i], "-n") == 0) {
            nprocesos = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-p") == 0) {
            porcentaje = atoi(argv[++i]);

        }
      
    }
 
  
    if (M == 0 || N == 0 || archivo == NULL || nprocesos == 0 || porcentaje == 0) {
        fprintf(stderr, "Debe proporcionar todos los argumentos requeridos: -f, -c, -a, -n, -p.\n");
        exit(1);
    }
  FILE *file;
    int m_original = 0; // Número de filas en la matriz original
    int n_original = 0; // Número de columnas en la matriz original

    // Abre el archivo de texto
    file = fopen(archivo, "r");

    if (file == NULL) {
        printf("No se pudo abrir el archivo.\n");
        return 1;
    }

    // Lee el archivo línea por línea para determinar las dimensiones
    char line[1000]; // Asumiendo que cada línea tiene menos de 1000 caracteres
    while (fgets(line, sizeof(line), file) != NULL) {
        m_original++; // Incrementa el número de filas
        if (n_original == 0) {
            // Contar el número de elementos en la primera fila para determinar el número de columnas
            char *token = strtok(line, " ");
            while (token != NULL) {
                n_original++;
                token = strtok(NULL, " ");
            }
        }
    }

    // Regresa al inicio del archivo
    fseek(file, 0, SEEK_SET);

    // Compara m y n con las dimensiones originales
    if (M > m_original || N > n_original) {
        printf("Las dimensiones especificadas son mayores que las dimensiones originales.\n");
        fclose(file);
        return 1;
    }

    // Crea una nueva matriz de tamaño m x n
    int matriz[M][N];

    // Lee y almacena los elementos de la matriz original en la nueva matriz
    for (int i = 0; i < M; i++) {
        if (fgets(line, sizeof(line), file) == NULL) {
            printf("Error al leer datos desde el archivo.\n");
            fclose(file);
            return 1;
        }
        char *token = strtok(line, " ");
        for (int j = 0; j < N; j++) {
            if (token == NULL) {
                printf("Error: no hay suficientes elementos en la fila %d.\n", i + 1);
                fclose(file);
                return 1;
            }
            matriz[i][j] = atoi(token);
            token = strtok(NULL, " ");
        }
    }
  fclose(file);
    // Imprime la matriz
    printf("Matriz resultante:\n");
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", matriz[i][j]);
          
        }
        printf("\n");
    }

    // Crear los hilos y verificar la matriz
    pthread_t *hilos = malloc(nprocesos * sizeof(pthread_t));
    DatosHilo *datosHilos = malloc(nprocesos * sizeof(DatosHilo));

    for (int i = 0; i < nprocesos; i++) {
        datosHilos[i].inicio = i * M / nprocesos;
        datosHilos[i].fin = (i + 1) * M / nprocesos;
        datosHilos[i].matriz = matriz;
        datosHilos[i].N = N;
        datosHilos[i].id= i;
             
      

        pthread_create(&hilos[i], NULL, contarCeros, &datosHilos[i]);
      
        
    }

    // Esperar a que todos los hilos terminen
    int totalCeros = 0;
    
    for (int i = 0; i < nprocesos; i++) {
        pthread_join(hilos[i], NULL);
      
        totalCeros += datosHilos[i].ceros;
    }

    // Calcular el porcentaje de ceros
    double porcentajeCeros = totalCeros * 100.0 / (M * N);

    printf("La matriz en el archivo %s tiene un total de %d ceros (%.2f%%), ", archivo, totalCeros, porcentajeCeros);

    if (porcentajeCeros >= porcentaje) {
        printf("por tanto, se considera dispersa.\n");
    } else {
        printf("por tanto, no se considera dispersa.\n");
    }

   free(matriz);
   free(hilos);
   free(datosHilos);

    return 0;
}