#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include "libacc.h"

#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"
// massima lunghezza accettata per il nome dei client e i KO dello store
#define MAX_LEN 50
// massima dimensione (in byte) di un blocco di dati da leggere/scrivere
#define MAX_RW 100

/* macro per il controllo degli errori con ripetizione dell'operazione
 * ogni secondo fino a 10 volte
 */
#define checkM1w(s,m) \
	{int j = 0; while ((s) == -1 && j < 10) {sleep(1); j++;} \
	if(j == 10) {perror(m); exit(errno);}}

int os_connect(char *name) {
	
	max = MAX_LEN + 20;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;
	char s[MAX_LEN+20] = "REGISTER ";
	strcat(s, name);
	strcat(s, " \n");
	// se si verifica un errore e t non viene modificata, sarà il messaggio d'errore
	char t[MAX_LEN] = "KO no response \n";

	checkM1w(skt = socket(AF_UNIX,SOCK_STREAM,0), "ERR: client creating socket")
	checkM1w(connect(skt,(struct sockaddr*)&sa,sizeof(sa)), "ERR: client connecting to socket")

	checkM1w(write(skt,s,max), "ERR: client writing on socket")
	checkM1w(read(skt,t,MAX_LEN), "ERR: client reading socket")
	
	if (strcmp(t,"OK \n") == 0)
		return 1;
	else {
		if (strncmp(t,"KO ",3) == 0)
			printf("%s", t);
		close(skt);
		return 0;
	}
}

int os_store(char *name, void *block, size_t len) {

	int n, m, i;
	char s[MAX_LEN+20] = "STORE ";
	// se si verifica un errore e t non viene modificata, sarà il messaggio d'errore
	char t[MAX_LEN] = "KO no response \n";
	strcat(s, name);
	strcat(s, " ");
	n = strlen(name);
	sprintf(s+n+7, "%zu \n ", len);
	// calcolo del numero di cicli da effettuare per scrivere l'intero blocco
	m = len / MAX_RW;
	if (len % MAX_RW != 0)
		m++;
	
	checkM1w(write(skt,s,max), "ERR: client writing on socket")
	for (i=0;i<m;i++) {
		checkM1w(write(skt,block,MAX_RW), "ERR: client writing on socket")
		block += MAX_RW;
	}

	checkM1w(read(skt,t,MAX_LEN), "ERR: client reading socket")

	if (strcmp(t,"OK \n") == 0)
		return 1;
	else {
		if (strncmp(t,"KO ",3)==0)
			printf("%s", t);
		close(skt);
		return 0;
	}
}

void *os_retrieve(char *name) {
	
	int len;
	char s[MAX_LEN+20] = "RETRIEVE ";
	// se si verifica un errore e t non viene modificata, sarà il messaggio d'errore
	char t[MAX_LEN] = "KO no response \n";
	strcat(s, name);
	strcat(s, " \n");
	
	checkM1w(write(skt,s,max), "ERR: client writing on socket")
	checkM1w(read(skt,t,MAX_LEN), "ERR: client reading socket")
	
	if(strncmp(t,"DATA ",5)==0 && strlen(t)>7) {
		void* block;
		void* temp_block;
		char* slen = t+5;
		char temp[MAX_RW];
		int i=0, m, r;
		slen = strtok(slen," ");
		len = atoi(slen);
		// block è il valore ritornato dalla funzione, e viene allocato della dimensione esatta dei dati
		block = (char*) malloc(len);
		temp_block = block;
		// calcolo del numero di cicli di lettura da eseguire
		m = len / MAX_RW;
		r = len % MAX_RW;
		if (r != 0)
			m++;
		while (i<m && read(skt,(void*)temp,MAX_RW)) {
			if(i == m-1) {
				memcpy(temp_block,temp,r);
			}
			else {
				memcpy(temp_block,temp,MAX_RW);
				temp_block += MAX_RW;
			}
			i++;
		}
		return block;
	}
	else {
		if(strncmp(t,"KO ",3) == 0)
			printf("%s", t);
		close(skt);
		return NULL;
	}
}

int os_delete(char *name) {
	
	int len = strlen(name);
	char s[MAX_LEN+20] = "DELETE ";
	// se si verifica un errore e t non viene modificata, sarà il messaggio d'errore
	char t[MAX_LEN] = "KO no response \n";
	strcpy(s+7, name);
	sprintf(s+7+len, " \n");

	checkM1w(write(skt,s,max), "ERR: client writing on socket")
	checkM1w(read(skt,t,MAX_LEN), "ERR: client reading socket")
	
	if (strcmp(t,"OK \n")==0)
		return 1;
	else {
		if(strncmp(t,"KO ",3) == 0)
			printf("%s", t);
		close(skt);
		return 0;
	}
}

int os_disconnect() {
	
	char t[5];
	
	checkM1w(write(skt,"LEAVE \n",8), "ERR: client writing on socket")
	checkM1w(read(skt,t,5), "ERR: client reading socket")
	checkM1w(close(skt), "ERR: client closing socket");
	
	if (strcmp(t,"OK \n") == 0)
		return 1;
	else
		return 0;
}