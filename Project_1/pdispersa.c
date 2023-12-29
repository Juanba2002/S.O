#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int M = 0, N = 0, nprocesos = 0, porcentaje = 0;
    char *archivo = NULL;

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

    // Crear los procesos y verificar la matriz
    for (int i = 0; i < nprocesos; i++) {
         if (fork() == 0) {
            // Código del proceso hijo
            int inicio = i * M / nprocesos;
            int fin = (i + 1) * M / nprocesos;
            int ceros = 0;
            int id=i;
          printf("Proceso hijo (ID %d)\n", id);
            for (int i = inicio; i < fin; i++) {
                for (int j = 0; j < N; j++) {
                    if (matriz[i][j] == 0) {
                        ceros++;
                    }
                }
            }

            exit(ceros);
        }
    }

    // Esperar a que todos los procesos hijos terminen
    int totalCeros = 0;
    for (int i = 0; i < nprocesos; i++) {
        int status;
        wait(&status);
        totalCeros += WEXITSTATUS(status);
    }

    // Calcular el porcentaje de ceros
    double porcentajeCeros = totalCeros * 100.0 / (M * N);
    printf("La matriz en el archivo %s tiene un total de %d ceros (%.2f%%), ", archivo, totalCeros, porcentajeCeros);
    
    if(porcentajeCeros >= porcentaje){
        printf("por tanto, se considera dispersa.\n");
    }else{
        printf("por tanto, no se considera dispersa.\n");
    }
  

    
    return 0;
}