#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "libacc.h"

// massima lunghezza accettata per il nome dei client
#define MAX_LEN 50
#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"

int perfop, succop;

/* test 1
 * i dati da memorizzare per il test sono sequenze di "ciao" di dimensione crescente.
 */
void store () {
	int i, j = 0;
	int dim = 100;
	char temp[100000];
	char name[3];
	for(i=0;i<20;i++) {
		while (j+4<dim) {
			strncpy(temp+j,"ciao",4);
			j+=4;
		}
		temp[j] = '\0';
		sprintf(name,"%hhu%hhu", i/10, i%10);
		perfop++;
		succop += os_store(name,temp,j+1);
		dim += 5257;
	}
}

// test 2 (recupera i file creati con il test 1 e ne controlla l' integrità)
void* retrieve () {
	int i, j = 0;
	int dim = 100;
	char temp[100000];
	char name[3];
	char* ret;
	for(i=0;i<20;i++) {
		while (j+4<dim) {
			strncpy(temp+j,"ciao",4);
			j+=4;
		}
		temp[j] = '\0';
		sprintf(name,"%hhu%hhu", i/10, i%10);
		perfop++;
		ret = os_retrieve(name);
		if (strcmp(ret,temp) == 0)
			succop++;
		dim += 5257;
	}
	free(ret);
}

// test 3 (cancella i file creati con il test 1)
void delete () {
	int i;
	char name[3];
	for(i=0;i<20;i++) {
		sprintf(name,"%hhu%hhu", i/10, i%10);
		perfop++;
		succop += os_delete(name);
	}
}

int main (int argc, char* argv[]) {
	int test;
	char name[MAX_LEN] = "\0";
	sscanf(argv[1],"%s",name);
	sscanf(argv[2],"%d",&test);
	// operazioni effettuate
	perfop = 0;
	// operazioni riuscite
	succop = 0;

	if (strlen(name) == 0 || test < 1 || test > 3) {
		printf("ERR: usage = client [NAME] [TESTCODE], where 0 < TESTCODE:int < 4\n");
		return 1;
	}

	// tentativi di connessione allo store (al massimo 10)
	while(perfop<10 && !os_connect(name))
		perfop++;

	if (perfop == 10) {
		printf("ERR: error during connection: make sure a store is running\n");
		return 1;
	}

	perfop++;
	succop++;
	
	switch (test) {
		case 1:
			store();
			break;
		case 2:
			retrieve();
			break;
		case 3:
			delete();
			break;
	}

	perfop++;
	succop += os_disconnect();

	printf("\nTEST %d : %d successful operations on %d\n", test, succop, perfop);
	fflush(stdout);

	return 0;
}