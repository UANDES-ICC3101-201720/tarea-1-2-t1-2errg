#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include "types.h"
#include "const.h"
#include "util.h"

// TODO: implement
int partition(UINT* A, int lo, int hi){
    int pivot, i = lo;
    pivot = A[hi];
    for (int j = lo;j<hi;j++){
        if (A[j] < pivot){
            A[i] = A[j];
            i++;
        }
    }
    A[i] = A[hi];
    return i;

}
int quicksort(UINT* A, int lo, int hi) {
    if (lo < hi){
        int p;
        p = partition(A,lo,hi);
        quicksort(A,lo,p-1);
        quicksort(A,p+1,hi);
    }
    return 0;
}

// TODO: implement
int parallel_quicksort(UINT* A, int lo, int hi) {
    return 0;
}

int main(int argc, char** argv) {
    printf("[quicksort] Starting up...\n");

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* TODO: parse arguments with getopt */
    char* Tvalue;
    int Evalue = 0;
    int index, c;

    opterr = 0;

	while ((c = getopt (argc, argv, "E:T:")) != -1)
		switch (c){
			case 'E':
				Evalue = atoi(optarg);
				break;
			case 'T':
				Tvalue = optarg;
				break;
			case '?':
				if (optopt == 'T' || optopt == 'E')
				  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				  fprintf (stderr,
					   "Unknown option character `\\x%x'.\n",
					   optopt);
				return 1;
			default:
				abort ();
		}
	printf ("E = %d, T = %d\n",Evalue, atoi(Tvalue));

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);


    /* TODO: start datagen here as a child process. */

    pid_t pid;
	pid = fork();
	if (pid == -1){   
		fprintf(stderr, "Error en fork\n");
		exit(EXIT_FAILURE);
	}
	if (pid == 0){
		execlp("./datagen","./datagen",NULL);
	}

    /* Create the domain socket to talk to datagen. */
    struct sockaddr_un addr;
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("[quicksort] Socket error.\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("[quicksort] connect error.\n");
        close(fd);
        exit(-1);
    }

    /* DEMO: request two sets of unsorted random numbers to datagen */
    for (int i = 0; i < Evalue; i++) {
        /* T value 3 hardcoded just for testing. */
        char palabra[15];
        strcpy(palabra,"BEGIN U");
        strcat(palabra, Tvalue);
        char *begin = palabra;
        int rc = strlen(begin);

        /* Request the random number stream to datagen */
        if (write(fd, begin, strlen(begin)) != rc) {
            if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
            else {
                perror("[quicksort] write error.\n");
                exit(-1);
            }
        }

        /* validate the response */
        char respbuf[10];
        read(fd, respbuf, strlen(DATAGEN_OK_RESPONSE));
        respbuf[strlen(DATAGEN_OK_RESPONSE)] = '\0';

        if (strcmp(respbuf, DATAGEN_OK_RESPONSE)) {
            perror("[quicksort] Response from datagen failed.\n");
            close(fd);
            exit(-1);
        }

        UINT readvalues = 0;
        size_t numvalues = pow(10, 3);
        size_t readbytes = 0;

        UINT *readbuf = malloc(sizeof(UINT) * numvalues);

        while (readvalues < numvalues) {
            /* read the bytestream */
            readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
            readvalues += readbytes / 4;
        }

        /* Print out the values obtained from datagen */
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u\n", *pv);
        }

        free(readbuf);
    }

    /* Issue the END command to datagen */
    int rc = strlen(DATAGEN_END_CMD);
    if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) != rc) {
        if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
        else {
            perror("[quicksort] write error.\n");
            close(fd);
            exit(-1);
        }
    }

    close(fd);
    exit(0);
}