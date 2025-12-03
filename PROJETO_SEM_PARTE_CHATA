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

/* the directories where output files will be placed */
#define RESULT_IMAGE_DIR "./Result-image-dir/"
#define INPUT_IMAGE_DIR "./Input-image-dir/"
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
} posicao;

char * files[] = {"img-IST-0.jpeg", "img-IST-1.jpeg", "img-IST-2.jpeg", 
                  "img-IST-3.jpeg", "img-IST-4.jpeg", "img-IST-5.jpeg", 
                  "img-IST-6.jpeg", "img-IST-7.jpeg", "img-IST-8.jpeg", 
                  "img-IST-9.jpeg"};
int nn_files = 10;


 void *processar_imagens_threads(void *arg){

	gdImagePtr in_img;
	/* output images */
	gdImagePtr out_blur_img;
	gdImagePtr out_contrast_img;
	gdImagePtr out_thumb_img;
	gdImagePtr out_sepia_img;
	gdImagePtr out_gray_img;


	/* file name of the image created and to be saved on disk	 */
	char out_file_name[100];


    struct thread_dados *dados = (struct thread_dados *)arg;
    int inicio = dados->inicio;
    int fim = dados->fim;

for (int i = inicio; i < fim; i++){	
		char full_file_name[100];
		sprintf(full_file_name, "%s%s", INPUT_IMAGE_DIR, files[i]);
		printf("image %s\n", full_file_name);
		/* load of the input file */
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


int main(){
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec start_time_par, end_time_par;
    int n_threads;
    printf("Digite o numero de threads: \n");
    scanf("%d", &n_threads);



	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_seq);

	if (create_directory(RESULT_IMAGE_DIR) == 0){
        fprintf(stderr, "Impossível criar %s\n", RESULT_IMAGE_DIR);
        exit(-1);
    }


	 clock_gettime(CLOCK_MONOTONIC, &end_time_seq);
     clock_gettime(CLOCK_MONOTONIC, &start_time_par);
	
	
	pthread_t *thread_id = malloc(n_threads * sizeof(pthread_t));
		
	if (!thread_id) { 
		perror("malloc"); exit(1); 
	}
    
	struct thread_dados *posicao = malloc(n_threads * sizeof(struct thread_dados));
   for(int i = 0; i< n_threads; i++){
    
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

    free(thread_id);
    free(posicao);

	exit(0);
}

