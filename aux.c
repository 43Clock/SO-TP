#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "aux.h"

#define MAX_SIZE 1024

Queue * novaQueue() {
	Queue *q = malloc(sizeof(Queue));
	q->total = 0;
	q->lista = NULL;
	return q;
}

Lista newNode(int id,int order,int tipo,char *comando) {
	Lista new = malloc(sizeof(struct lista));
	new->id = id;
	new->order = order;
	new->tipo = tipo;
	new->comando = comando;
	new-> prox = NULL;
	return new;
}

void addNode(Lista *l, int id,int order,int tipo, char *comando) {
	Lista new = newNode(id, order,tipo,comando);
	Lista aux = *l;
	if (*l == NULL) {
		*l = new;
	}
	else {
		while (aux -> prox != NULL) {
			aux = aux->prox;
		}
		aux -> prox = new;
	}
}

int getIdIndex(Lista l, int index) {
	int i = 0;
	Lista aux = l;
	while (i < index) {
		aux = aux->prox;
		i++;
	}
	if (aux) return aux->id;
	return -1;
}

int getTipoFromOrder(Lista l,int order){
	Lista aux = l;
	int res = -1;
	while(aux && aux->order != order)
		aux = aux->prox;
	if(aux) res = aux->tipo;
	return res;
}

int getPosFromOrder(Lista l,int order){
	int res = 0;
	Lista aux = l;
	while(aux && aux->order !=order){
		res++;
		aux = aux->prox;
	}
	return res;
}

Lista removeFromId(Lista *l,int id ){
	Lista temp = *l,prev,res;
	Lista next = (*l)->prox;
	while(temp != NULL && temp->id == id){
		res = *l;
		*l = next;
		return res;
	}
	while (temp != NULL && temp->id != id){
		prev = temp;
		temp = temp->prox;
	}
	res = temp;
	if(temp == NULL) return NULL;
	prev->prox = temp->prox;
	//free(temp);
	return res;
}

int howManyDigits(int n){
	int r = (n>0)?0:1;
	while(n!=0){
		n /= 10;
		r++;
	}
	return r;
}

const char* imprimeLista(Lista l){
	if(l==NULL) return "";
	Lista aux = l;
	int tam = 0;
	while(aux){
		tam+=strlen(aux->comando)+3+howManyDigits(aux->order);
		aux=aux->prox;
	}
	tam++;
	char *buf = malloc(sizeof(char)*tam);
	aux = l;
	while(aux){
		sprintf(buf+strlen(buf),"#%d: %s\n",aux->order,aux->comando);
		aux = aux->prox;
	}
	return buf;
}

const char* imprimeFinished(Lista l){
	if(l == NULL) return "";
	const char* t0 = "concluida:";
	const char* t1 = "max execução:";
	const char* t2 = "max inactividade:";
	const char* t3 = "terminada:";
	const char* t4 = "something went wrong:";
	Lista aux = l;
	int tam = 0;
	while(aux){
		tam += strlen(aux->comando)+4+howManyDigits(aux->order);
		if(aux->tipo == 0) tam += strlen(t0);
		else if(aux->tipo == 1) tam += strlen(t1);
		else if(aux->tipo == 2) tam += strlen(t2);
		else if(aux->tipo == 3) tam += strlen(t3);
		else tam += strlen(t4);
		aux = aux->prox;
	}
	tam++;
	char *buf = malloc(sizeof(char)*tam);
	aux = l;
	while(aux){
		if(aux->tipo == 0) sprintf(buf+strlen(buf),"#%d: %s %s\n",aux->order,t0,aux->comando);
		else if(aux->tipo == 1) sprintf(buf+strlen(buf),"#%d: %s %s\n",aux->order,t1,aux->comando);
		else if(aux->tipo == 2) sprintf(buf+strlen(buf),"#%d: %s %s\n",aux->order,t2,aux->comando);
		else if(aux->tipo == 3) sprintf(buf+strlen(buf),"#%d: %s %s\n",aux->order,t3,aux->comando);
		else tam += sprintf(buf+strlen(buf),"#%d: %s %s\n",aux->order,t4,aux->comando);
		aux = aux->prox;
	}
	return buf;
}


int detectPiping(char *str) {
	int flag = 0;
	for (int i = 0; str[i] && !flag; i++)
		if (str[i] == '|') flag = 1;
	return flag;
}

int chooseExecute(char *str) {
	if (strcmp(str, "tempo-inactividade") == 0 || strcmp(str, "-i") == 0) return 1;
	if (strcmp(str, "tempo-execucao") == 0 || strcmp(str, "-m") == 0) return 2;
	if (strcmp(str, "executar") == 0 || strcmp(str, "-e") == 0) return 3;
	if (strcmp(str, "listar") == 0 || strcmp(str, "-l") == 0) return 4;
	if (strcmp(str, "terminar") == 0 || strcmp(str, "-t") == 0) return 5;
	if (strcmp(str, "historico") == 0 || strcmp(str, "-r") == 0) return 6;
	if (strcmp(str, "ajuda") == 0 || strcmp(str, "-h") == 0) return 7;
	if (strcmp(str, "output") == 0 || strcmp(str, "-o") == 0) return 8;
	return 0;
}

char* itoa(int i, char b[]) {
	char const digit[] = "0123456789";
	char* p = b;
	if (i < 0) {
		*p++ = '-';
		i *= -1;
	}
	int shifter = i;
	do {
		++p;
		shifter = shifter / 10;
	} while (shifter);
	*p = '\0';
	do {
		*--p = digit[i % 10];
		i = i / 10;
	} while (i);
	return b;
}


const char * geraMensagem(int i) {
	char texto[MAX_SIZE] = "nova tarefa #";
	char buf[MAX_SIZE];
	itoa(i, buf);
	strcat(texto, buf);
	strcat(texto,"\n");
	char *text = malloc(sizeof(char)*(strlen(texto)+1));
	strcpy(text, texto);
	return text;
}

const char *geraAjuda1(){
	return "tempo-inactividade segs\ntempo-execucao segs\nexecutar 'p1'\nexecutar 'p1 | p2 ... | pn'\nlistar\nterminar num_ordem\nhistorico\najuda\nquit\n";
}

const char *geraAjuda2(){
	return "-i segs\n-m segs\n-e 'p1'\n-e 'p1 | p2 ... | pn'\n-l\n-t num_ordem\n-r\n-h\n";
}

