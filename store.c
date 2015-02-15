#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

// massima lunghezza accettata per il nome dei client e i KO dello store
#define MAX_LEN 50
// massima dimensione (in byte) di un blocco di dati da leggere/scrivere
#define MAX_RW 100
// massimo numero di client accettati
#define MAX_CONN 500
#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"

/* macro per il controllo degli errori con ripetizione dell'operazione
 * ogni secondo fino a 10 volte; in checkM1we è compreso anche un settaggio
 * della variabile error per favorire il riconoscimento dell'errore da
 * parte del chiamante
 */
#define checkM1w(s,m) \
	{int j = 0; while ((s) == -1 && j < 10) {sleep(1); j++;} \
	if(j == 10) {perror(m); exit(errno);}}

#define checkM1we(s,m) \
	{int j = 0; while ((s) == -1 && j < 10) {sleep(1); j++;} \
	if(j == 10) {perror(m); *error=1; return 1;}}

/* massima lunghezza dei messaggi inviati ai / ricevuti dai client,
 * funzione della lunghezza dei loro nomi (MAX_LEN) e inizializzata
 * nel main
 */
int max;

/* funzione che si occupa della memorizzazione degli oggetti ricevuti
 * dai client, chiamata in conseguenza alla ricezione di un messaggio
 * del tipo "STORE name len \n data"
 */
int store (int cs, char* client, int nl, int* error) {
	char* temp;
	char temparr[MAX_RW], templen[MAX_RW];
	int len, n, m, r, i = 0;
	
	n = read(cs, temparr, max-1);
	temp=temparr;
	if (n==-1||n==0) {
		checkM1we(write(cs,"KO error during transmission \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}
	else if (strncmp(temp,"TORE ",5)!=0 || strlen(temp)<11) {
		checkM1we(write(cs,"KO invalid store request \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}

	strcpy(templen,temparr);
	char* obj;
	char* slen = templen;
	char path[MAX_LEN+20];
	int ol;
	FILE *fd;
	obj = temp + 5;
	obj = strtok(obj, " ");
	
	strcpy(path,client);
	path[nl]='/';
	strcpy(path+nl+1,obj);
	
	ol = strlen(obj);
	slen = templen + ol + 6;
	slen = strtok(slen, " ");

	len = atoi(slen);
	if ((fd=fopen(path,"w"))==NULL) {
		perror("worker creating file");
		*error=1;
		return 1;
	}
	m = len / MAX_RW;
	r = len % MAX_RW;
	if (r != 0)
		m++;
	
	while(i<m && read(cs,temp,MAX_RW)) {
		if (i == m-1)
			checkM1we(fwrite(temp,1,r,fd), "worker writing on file")
		else
			checkM1we(fwrite(temp,1,MAX_RW,fd), "worker writing on file")
		i++;
	}

	checkM1we(fclose(fd), "worker closing file")

	checkM1we(write(cs,"OK \n", MAX_LEN), "worker writing on socket")
	return 0;
}

/* funzione che si occupa del recupero degli oggetti precedentemente
 * memorizzati dallo store, chiamata in conseguenza alla ricezione di
 * un messaggio del tipo "RETRIEVE name \n"
 */
int retrieve (int cs, char* client, int nl, int* error) {
	char* temp;
	char temparr[MAX_LEN+20];
	int n;
	
	n = read(cs, temparr, max-1);
	temp=temparr;
	if (n==-1||n==0) {
		checkM1we(write(cs,"KO error during transmission \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}
	else if (strncmp(temp,"ETRIEVE ",8)!=0 || strlen(temp)<11) {
		checkM1we(write(cs,"KO invalid retrieve request \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}
	char* obj;
	char path[MAX_LEN+20];
	char block[MAX_RW];
	FILE *fd;
	int i, sz, m, r;
	
	obj = temp + 8;
	obj = strtok(obj, " ");
	strcpy(path,client);
	path[nl]='/';
	strcpy(path+nl+1,obj);
	
	if ((fd=fopen(path,"r"))==NULL) {
		perror("worker reading file");
		*error=1;
		return 1;
	}
	fseek(fd, 0L, SEEK_END);
	sz = ftell(fd);
	fseek(fd, 0L, SEEK_SET);
	m = sz / MAX_RW;
	r = sz % MAX_RW;
	if (r != 0)
		m++;
	
	strcpy(temparr,"DATA ");
	sprintf(temparr+5, "%d \n ", sz);
	checkM1we(write(cs,temparr, MAX_LEN), "worker writing on socket")
	for(i=0;i<m;i++) {
		if (i==m-1) {
			fread(block,1,r,fd);
			checkM1we(write(cs,block,r), "worker writing on socket")
		}
		else {
			fread(block,1,MAX_RW,fd);
			checkM1we(write(cs,block,MAX_RW), "worker writing on socket")
		}
	}
	checkM1we(fclose(fd), "worker closing file")

	return 0;
}

/* funzione che si occupa dell'eliminazione degli oggetti precedentemente
 * memorizzati dallo store, chiamata in conseguenza alla ricezione di
 * un messaggio del tipo "RETRIEVE name \n"
 */
int delete (int cs, char* client, int nl, int* error) {
	
	char temparr[MAX_LEN+20];
	int n;
	
	n = read(cs, temparr, max-1);
	if (n==-1||n==0) {
		checkM1we(write(cs,"KO error during transmission \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}
	else if (strncmp(temparr,"ELETE ",6)!=0 || strlen(temparr)<9) {
		checkM1we(write(cs,"KO invalid retrieve request \n",MAX_LEN), "worker writing on socket")
		*error = 1;
		return 1;
	}
	
	char* obj;
	char path[MAX_LEN+20];
	obj = temparr + 6;
	obj = strtok(obj, " ");
	strcpy(path,client);
	path[nl]='/';
	strcpy(path+nl+1,obj);

	checkM1we(unlink(path), "worker deleting file")
	checkM1we(write(cs,"OK \n", MAX_LEN), "worker writing on socket")
	
	return 0;
}

/* funzione che si occupa di terminare la comunicazione con i client,
 * chiamata in conseguenza alla ricezione di un messaggio del tipo
 * "LEAVE \n"
 */
int leave (int cs, int* error, int* eoc) {
	
	char temp[8];
	checkM1we(read(cs,temp,8), "worker reading socket")
	
	if (strcmp(temp,"EAVE \n")==0){
		checkM1we(write(cs,"OK \n",5), "worker writing on socket")
		*eoc = 1;
		return 0;
	}
	else {
		*error = 1;
		return 1;
	}
}

/* codice eseguito dai threads creati nella funzione dispatcher, con
 * lo scopo di creare, mantenere e gestire un canale di comunicazione
 * con uno ed un solo client.
 */
static void* worker (void*arg) {
	// descrittore ottenuto dopo la accept()
	int cs = *((int*) arg);
	int n, len;
	char temp[MAX_LEN+20];
	
	n = read(cs, temp, max);
	if (n==-1 || n==0) {
		checkM1w(write(cs,"KO error during transmission \n",MAX_LEN), "worker writing on socket")
		pthread_exit(NULL);
	}
	else if (strncmp(temp,"REGISTER ",9)!=0 || strlen(temp)<12) {
		checkM1w(write(cs,"KO invalid registration message \n",MAX_LEN), "worker writing on socket")
		pthread_exit(NULL);
	}
	
	char* name;
	name = temp + 9;
	name = strtok(name," ");
	// creazione della cartella dedicata al client, se non esiste già
	if (mkdir(name,0777) == -1) {
		if (errno != EEXIST) {
			perror("worker creating directory");
			pthread_exit(NULL);
		}
	}
	
	checkM1w(write(cs,"OK \n", MAX_LEN), "worker writing on socket")
	
	len = strlen(name);
	char c;
	int error = 0, eoc = 0;
	
	/* decisione del comportamento da mantenere a seconda del tipo
	 * di messaggio arrivato dal client (basta la prima lettera);
	 * uscita dal loop a seguito di errore o messaggio "LEAVE \n"
	 */
	while (read(cs,&c,1) && !error && !eoc) {
		switch (c) {
			case 'S':
				store(cs,name,len,&error);
				break;
			case 'R':
				retrieve(cs,name,len,&error);
				break;
			case 'D':
				delete(cs,name,len,&error);
				break;
			case 'L':
				leave(cs,&error,&eoc);
				break;
		}
	}

	if (error)
		printf("ERR: worker exiting because of an error during the communication\n");
	
	close(cs);
	pthread_exit(NULL);
}

// dispatcher che cicla indeterminatamente fino a che non si verifica un errore
int dispatcher (int skt) {
	
	pthread_t t;
	int cs[MAX_CONN];
	int err;
	int* error = &err;
	int i=0;

	while (!err && i<MAX_CONN) {
		checkM1we(cs[i] = accept(skt,NULL,NULL), "ERR: dispatcher accepting connection")
		pthread_create(&t,NULL,&worker,cs+i);
		i++;
	}

	return 0;
}

int main (int argc, char* argv[]) {
	
	int skt;
	max = MAX_LEN+20;
	struct sockaddr_un sa;

	strncpy(sa.sun_path,SOCKNAME,UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	skt = socket(AF_UNIX,SOCK_STREAM,0);
	bind(skt,(struct sockaddr *)&sa,sizeof(sa));
	listen(skt,SOMAXCONN);

	mkdir("data",0777);
	chdir("data");

	dispatcher(skt);

	return 0;
}