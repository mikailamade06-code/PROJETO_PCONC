/******************************************************************************
 * Programacao Concorrente
 * MEEC 21/22
 *
 * Projecto - Parte1
 *                           serial-complexo.c
 * 
 * Compilacao: gcc serial-complexo -o serial-complex -lgd
 *       Mikail Amade 114847 
 *       José Pedro Ferreira 114087
 *           
 *****************************************************************************/

#include <gd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <time.h>
#include "image-lib.h"
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
static const char *dir_para_ordenar = NULL;
static char *input_dir = NULL;

typedef struct {
    char image_name[256];
    char input_dir[256];
    char result_dir[256];
    int termina;
} Task;

typedef struct {
    int thread_id;
    int pipe;
} ThreadData;

int task_pipe[2];
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int imagens_total = 0;
double tempo_total = 0.0;
int tasks_pendentes = 0;
pthread_cond_t  c = PTHREAD_COND_INITIALIZER;


char **guarda_jpeg(const char *dirpath, int *out_count) {
    DIR *d = opendir(dirpath);
    if (!d) return NULL;

    struct dirent *entry;
    char **files = NULL;
    int capacidade = 0, cnt = 0;
    
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 4) continue;

        int jpeg = 0;
        if (len >= 5 && strcasecmp(name + len - 5, ".jpeg") == 0) jpeg = 1;
        if (!jpeg) continue;

        if (cnt == capacidade) {
            if (capacidade) {
              capacidade = capacidade * 2;
            } else {
               capacidade = 16;
            }
            char **tmp = realloc(files, capacidade * sizeof(char *));
            if (!tmp) {
                for (int i = 0; i < cnt; i++) free(files[i]);
                free(files);
                closedir(d);
                return NULL;
            }
            files = tmp;
        }

        files[cnt] = strdup(name);
        if (!files[cnt]) {
            for (int i = 0; i < cnt; i++) free(files[i]);
            free(files);
            closedir(d);
            return NULL;
        }
        cnt++;
    }
    closedir(d);
    *out_count = cnt;
    return files;
}

void print_stats() {
    pthread_mutex_lock(&stats_mutex);
    printf("Numero total de imagens processadas – %d\n", imagens_total);
    if (imagens_total > 0) {
        printf("Tempo médio de processamento – %.2fs\n", tempo_total / imagens_total);
    } else {
        printf("Tempo médio de processamento – 0.0s\n");
    }
    pthread_mutex_unlock(&stats_mutex);
}

void *processar_imagens_threads(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int thread_id = data->thread_id;
    int pipe = data->pipe;

    while (1) {
        Task task;
        ssize_t tamanho_lido= read(pipe, &task, sizeof(Task));
        
        if (tamanho_lido!= sizeof(Task)) {
            break;
        }

        if (task.termina) {
            break;
        }

        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        char full_file_name[512];
        sprintf(full_file_name, "%s/%s", task.input_dir, task.image_name);

        gdImagePtr in_img = read_jpeg_file(full_file_name);
        if (in_img == NULL) {
          pthread_mutex_lock(&stats_mutex);

          tasks_pendentes--;
            if (tasks_pendentes == 0) {
              pthread_cond_signal(&c);
          }

         pthread_mutex_unlock(&stats_mutex);
         continue;
}

        gdImagePtr out_contrast_img = contrast_image(in_img);
        gdImagePtr out_blur_img = blur_image(in_img);
        gdImagePtr out_sepia_img = sepia_image(in_img);
        gdImagePtr out_thumb_img = thumb_image(in_img);
        gdImagePtr out_gray_img = gray_image(in_img);

        char out_file[1024];
        sprintf(out_file, "%scontrast_%s", task.result_dir, task.image_name);
        write_jpeg_file(out_contrast_img, out_file);
        sprintf(out_file, "%sblur_%s", task.result_dir, task.image_name);
        write_jpeg_file(out_blur_img, out_file);
        sprintf(out_file, "%ssepia_%s", task.result_dir, task.image_name);
        write_jpeg_file(out_sepia_img, out_file);
        sprintf(out_file, "%sthumb_%s", task.result_dir, task.image_name);
        write_jpeg_file(out_thumb_img, out_file);
        sprintf(out_file, "%sgray_%s", task.result_dir, task.image_name);
        write_jpeg_file(out_gray_img, out_file);

        gdImageDestroy(out_blur_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(out_contrast_img);
        gdImageDestroy(out_gray_img);
        gdImageDestroy(out_thumb_img);
        gdImageDestroy(in_img);

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double tempo = (end_time.tv_sec - start_time.tv_sec) + 
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        pthread_mutex_lock(&stats_mutex);
        imagens_total++;
        tempo_total += tempo;
        int total_atual = imagens_total;
        double media_atual = tempo_total / imagens_total;
        tasks_pendentes--;
        if (tasks_pendentes == 0) {
        pthread_cond_signal(&c);
       }
        pthread_mutex_unlock(&stats_mutex);

        printf("thread %d processou %s em %.2fs\n", thread_id, task.image_name, tempo);
        printf("Numero total de imagens processadas – %d\n", total_atual);
        printf("Tempo médio de processamento – %.2fs\n", media_atual);
    }
    return NULL;
}

int comparar_file(const void *a, const void *b) {
    const char *file_a = *(const char **)a;
    const char *file_b = *(const char **)b;

    struct stat stat_a, stat_b;
    char path_a[512], path_b[512];

  snprintf(path_a, sizeof(path_a), "%s/%s", dir_para_ordenar, file_a);
  snprintf(path_b, sizeof(path_b), "%s/%s", dir_para_ordenar, file_b);

    if (stat(path_a, &stat_a) != 0) return 0;
    if (stat(path_b, &stat_b) != 0) return 0;

    if (stat_a.st_size < stat_b.st_size) return -1;
    if (stat_a.st_size > stat_b.st_size) return 1;
    return 0;
}

int comparar_nomes(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n_threads> <-name|-size>\n", argv[0]);
        exit(1);
    }

    int n_threads = atoi(argv[1]);
    char *ordem = argv[2];

    if (strcmp(ordem, "-name") != 0 && strcmp(ordem, "-size") != 0) {
        fprintf(stderr, "Invalid order parameter. Use -name or -size\n");
        exit(1);
    }

    if (pipe(task_pipe) == -1) {
        exit(1);
    }

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    ThreadData *thread_data = malloc(n_threads * sizeof(ThreadData));
    pthread_mutex_lock(&stats_mutex);
   
    pthread_mutex_unlock(&stats_mutex);
    for (int i = 0; i < n_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].pipe = task_pipe[0];
        pthread_create(&threads[i], NULL, processar_imagens_threads, &thread_data[i]);
    }

    printf("Foram criadas %d threads\n", n_threads);

    char linha[256];
    char palavra_1[128], palavra_2[256];
    

    while (1) {
        printf("Qual o comando: ");
        fflush(stdout);

        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            break;
        }

        int n_palavras = sscanf(linha, "%s %s", palavra_1, palavra_2);

        if (n_palavras == 0) {
            continue;
        }

        if (strcmp(palavra_1, "QUIT") == 0) {
            
            break;
        } else if (strcmp(palavra_1, "STAT") == 0) {
            print_stats();
        } else if (strcmp(palavra_1, "DIR") == 0 && n_palavras == 2) {
          
            int nn_files = 0;
            char **files = guarda_jpeg(palavra_2, &nn_files);
            pthread_mutex_lock(&stats_mutex);
            tasks_pendentes += nn_files;
            pthread_mutex_unlock(&stats_mutex);

            if (!files || nn_files == 0) {
                printf("Nenhuma imagem encontrada na pasta %s\n", palavra_2);
                continue;
            }
             char dir_local[256];
             strncpy(dir_local, palavra_2, sizeof(dir_local));
             dir_local[sizeof(dir_local) - 1] = '\0';
            if (strcmp(ordem, "-name") == 0) {
                qsort(files, nn_files, sizeof(char *), comparar_nomes);
            } else {
                dir_para_ordenar = dir_local;
                qsort(files, nn_files, sizeof(char *), comparar_file);
                dir_para_ordenar = NULL;
            }
            dir_para_ordenar = NULL;
            printf("A %d imagens na pasta %s serão processadas pelas %d threads\n", 
                   nn_files, palavra_2, n_threads);

            char result_dir[512];
            sprintf(result_dir, "%s/Result-image-dir/", palavra_2);
            create_directory(result_dir);

            for (int i = 0; i < nn_files; i++) {
                Task task;
                strcpy(task.image_name, files[i]);
                strcpy(task.input_dir, palavra_2);
                strcpy(task.result_dir, result_dir);
                task.termina = 0;

                write(task_pipe[1], &task, sizeof(Task));
            }

            for (int i = 0; i < nn_files; i++) {
                free(files[i]);
            }
            free(files);
        } else {
            printf("comando inválido\n");
        }
    }
     pthread_mutex_lock(&stats_mutex);
    while (tasks_pendentes > 0) {
        pthread_cond_wait(&c, &stats_mutex);
    }
    pthread_mutex_unlock(&stats_mutex);
    Task termina_task;
    termina_task.termina = 1;
    for (int i = 0; i < n_threads; i++) {
        write(task_pipe[1], &termina_task, sizeof(Task));
    }

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    print_stats();

    close(task_pipe[0]);
    close(task_pipe[1]);
    free(threads);
    free(thread_data);

    exit(0);
}
