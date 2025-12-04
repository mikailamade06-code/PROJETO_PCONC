/******************************************************************************
 * Programacao Concorrente
 * MEEC 21/22
 *
 * Projecto - Parte1
 *                           serial-complexo.c
 * 
 * Compilacao: gcc serial-complexo -o serial-complex -lgd
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

#define RESULT_IMAGE_DIR "./Result-image-dir/"

static char *input_dir = NULL;

/******************************************************************************
 * main()
 *
 * Arguments: (none)
 * Returns: 0 in case of sucess, positive number in case of failure
 * Side-Effects: creates thumbnail, resized copy and watermarked copies
 *               of images
 *
 * Description: implementation of the complex serial version 
 *              This application only works for a fixed pre-defined set of files
 *
 *****************************************************************************/
struct thread_dados{
    char **imagens;
    int inicio; 
    int fim;
	char **files;
	int nn_files;
    char *INPUT_IMAGE_DIR;
} posicao;


char **collect_jpeg_files(const char *dirpath, int *out_count) {
    DIR *d = opendir(dirpath);
    if (!d) return NULL;

    struct dirent *entry;
    char **files = NULL;
    int capacidade = 0, cnt = 0;

    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 4) continue; 

        
        int is_jpeg = 0;
        if (len >= 5 && strcasecmp(name + len - 5, ".jpeg") == 0) is_jpeg = 1;
        if (!is_jpeg) continue;

        
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

void *processar_imagens_threads(void *arg){

	gdImagePtr in_img;
	/* output images */
	gdImagePtr out_blur_img;
	gdImagePtr out_contrast_img;
	gdImagePtr out_thumb_img;
	gdImagePtr out_sepia_img;
	gdImagePtr out_gray_img;


	char out_file_name[100];


    struct thread_dados *dados = (struct thread_dados *)arg;
    int inicio = dados->inicio;
    int fim = dados->fim;
	char **files = dados->files;
	int nn_files = dados->nn_files;
    char *INPUT_IMAGE_DIR = dados->INPUT_IMAGE_DIR;

for (int i = inicio; i < fim; i++){	
		char full_file_name[100];
		sprintf(full_file_name, "%s/%s", INPUT_IMAGE_DIR, files[i]);
		printf("image %s\n", full_file_name);
	
	    in_img = read_jpeg_file(full_file_name);
		if (in_img == NULL){
			fprintf(stderr, "Impossible to read %s image\n", files[i]); 
			continue;
		}



out_contrast_img = contrast_image(in_img);
		out_blur_img = blur_image(in_img);
		out_sepia_img = sepia_image(in_img); 
		out_thumb_img = thumb_image(in_img);
		out_gray_img = gray_image(in_img);

		/* save resized */ 
		sprintf(out_file_name, "%ssepia_%s", RESULT_IMAGE_DIR, files[i]);
		if(write_jpeg_file(out_sepia_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
		}
		sprintf(out_file_name, "%sblur_%s", RESULT_IMAGE_DIR, files[i]);
		if(write_jpeg_file(out_blur_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
		}
		sprintf(out_file_name, "%scontrast_%s", RESULT_IMAGE_DIR, files[i]);
		if(write_jpeg_file(out_contrast_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
		}
		sprintf(out_file_name, "%sthumb_%s", RESULT_IMAGE_DIR, files[i]);
		if(write_jpeg_file(out_thumb_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
		}
		sprintf(out_file_name, "%sgray_%s", RESULT_IMAGE_DIR, files[i]);
		if(write_jpeg_file(out_gray_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
		}
		 gdImageDestroy(out_blur_img);
         gdImageDestroy(out_sepia_img);
         gdImageDestroy(out_contrast_img);
         gdImageDestroy(out_gray_img);  
         gdImageDestroy(out_thumb_img);  
         gdImageDestroy(in_img);
}
return NULL;
 }

int comparar_file(const void *a, const void *b) {
	const char *file_a = *(const char **)a;
	const char *file_b = *(const char **)b;

	struct stat stat_a, stat_b;
	char path_a[256], path_b[256];

	snprintf(path_a, sizeof(path_a), "%s/%s", input_dir, file_a);
	snprintf(path_b, sizeof(path_b), "%s/%s", input_dir, file_b);

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



int main(int argc, char *argv[]){
	if (argc!=4) {
		 fprintf(stderr, "%s\n", argv[0]);
		
		return 1;
	}
    int n_threads = 0;
    n_threads = atoi(argv[2]);
    char *INPUT_IMAGE_DIR = NULL;
    INPUT_IMAGE_DIR = argv[1];
    input_dir = argv[1];

	int nn_files = 0;
	char **files = NULL;
    files = collect_jpeg_files(argv[1], &nn_files);
    if (!files || nn_files == 0) {
        exit(1);
        
    }

    
for(int i =0; i< nn_files; i++){
    printf("File %d: %s %ld bytes\n", i, files[i], sizeof(files[i]));
}

char *ordem = argv[3];

if (strcmp(ordem, "-name") == 0) {
    
    qsort(files, nn_files, sizeof(char *),
          comparar_nomes);
}
else if (strcmp(ordem, "-size") == 0) {
    
    qsort(files, nn_files, sizeof(char *), comparar_file);
}
else {
    fprintf(stderr, "%s\n", argv[0]);

    return 1;
}

	

    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec start_time_par, end_time_par;

	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_seq);

	if (create_directory(RESULT_IMAGE_DIR) == 0){
        fprintf(stderr, "Impossível criar %s\n", RESULT_IMAGE_DIR);
        exit(-1);
    }


	 clock_gettime(CLOCK_MONOTONIC, &end_time_seq);
     clock_gettime(CLOCK_MONOTONIC, &start_time_par);
	
	struct thread_dados *posicao = malloc(n_threads * sizeof(struct thread_dados));
	pthread_t *thread_id = malloc(n_threads * sizeof(pthread_t));
	if (!thread_id) { 
		 exit(1); 
	}

   for(int i = 0; i< n_threads; i++){
        posicao[i].INPUT_IMAGE_DIR = argv[1];
		posicao[i].files = files;
		posicao[i].nn_files = nn_files;
        posicao[i].inicio = i * (nn_files / n_threads);
		
		if(n_threads > nn_files) n_threads = nn_files;
        if(i == n_threads - 1){
            posicao[i].fim = nn_files;
		}
		else{
             posicao[i].fim = posicao[i].inicio + (nn_files / n_threads);
        } 
    }

    for(int j=0; j < n_threads; j++){
    pthread_create(&thread_id[j], NULL, processar_imagens_threads, &posicao[j]);

    }

for(int j=0; j< n_threads; j++){
    pthread_join(thread_id[j], NULL);
}


    clock_gettime(CLOCK_MONOTONIC, &end_time_par);
    clock_gettime(CLOCK_MONOTONIC, &end_time_total);

    struct timespec par_time = diff_timespec(&end_time_par, &start_time_par);
    struct timespec seq_time = diff_timespec(&end_time_seq, &start_time_seq);
    struct timespec total_time = diff_timespec(&end_time_total, &start_time_total);
    
    printf("\tseq \t %10jd.%09ld\n", seq_time.tv_sec, seq_time.tv_nsec);
    printf("\tpar \t %10jd.%09ld\n", par_time.tv_sec, par_time.tv_nsec);
    printf("total \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    

    // depois de join e antes de exit:
    for (int i = 0; i < nn_files; i++) free(files[i]);
    free(files);

    free(thread_id);
    free(posicao);

	exit(0);
}
