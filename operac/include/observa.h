
#define MAXOBS       100

struct comentario {
	int  cod;
	char sobs[150];
};

bool ProximoComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento);
void PrincipioComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento);
void InicializoComentario(struct comentario *principio, struct comentario **ultimo);
void CargarComentario(struct comentario *principio, struct comentario **ultimo, short scod, char *scoment);
bool HayComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento);
int compobs_lib(struct comentario *a, struct comentario *b);
double Standard(short emp, long cliente, short objetivo, DATE fini);
double CalcularStd(short emp, long cliente, short objet, DATE foini, DATE fofin, 
                   DATE fini, DATE ffin, DATE maxfec, int paso, 
                   bool gcomen, struct comentario *principio, struct comentario **ultimo);
DATE UltimaOt(short emp, long cliente, short objet, DATE maxfec);

