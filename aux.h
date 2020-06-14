
typedef struct lista {
	int id; 
	int order;
	int tipo;
	char *comando;
	struct lista *prox;
} *Lista;

typedef struct queue {
	int total;
	Lista lista;
} Queue;

Queue * novaQueue();

Lista newNode(int id,int order, int tipo,char *comando);

void addNode(Lista *l, int id,int order,int tipo, char *comando);
int getIdIndex(Lista l,int index);
int detectPiping(char *str);
int chooseExecute(char *str);
Lista removeFromId(Lista *l,int id );
int getTipoFromOrder(Lista l,int order);
int getPosFromOrder(Lista l,int order);
const char * geraMensagem(int i);
const char *geraAjuda1();
const char *geraAjuda2();
const char* imprimeLista(Lista *l);
const char* imprimeFinished(Lista *l);