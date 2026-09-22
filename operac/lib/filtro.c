/*
** @(#)filtro.c	1.4  96/02/15  14:36:01
** 1.4  2/15/96
** Vanesa Bigio 
** Funciones de selección de postulantes según filtros deseados.
*/
#include <ideafix.h>
#include "emple.sch"
#include "emple.h"
#include "sue.sch"
#include "filtro.h"
#include "sirh.h"
#include "Icapac.h"
#include "Iplcarr.h"
#include "Ievades.h"

/* Declaraciones globales */
struct legemple { /* lista de legajos incorporados en CABPOST */
	long nroleg;
	struct legemple * sig;
} *CablegE=NULL;

long puntos=0;

static struct Resultado * NuevoNodoResultado ( int emp, char * coddoc,  
    long nrodoc, long nroleg, char * intext, DATE fpost,  
	bool asignado, int orden, char * apynom, int edad,  
	bool entreapr, long puntaje, char tipord);

void ObtenerTipo_y_NroDoc(int emp, Int legajo, char * tipdoc, Int * nrodoc);


bool PostConEntrev(int emp,int req,char * coddoc,long nrodoc) {
	/* tomara la ultima de las entrevistas */
	SetKey(ENTREVbyEMP,emp,req,coddoc,nrodoc,MAX_DATE,MAX_TIME);
	if (GetRecord(ENTREV, PREV_KEY|PARTIAL_KEY,IO_NOT_LOCK,4) != ERROR)
		if (*SFld(ENTREV_RESULT)=='R')
			return FALSE;
		else {
			SetKey(ENTINTbyEMP,emp,req,coddoc,nrodoc, MAX_DATE);
			if (GetRecord(ENTINT,PREV_KEY|PARTIAL_KEY,IO_NOT_LOCK,4) == ERROR
					|| *SFld(ENTINT_RESULT)!='R')
				return TRUE;
		}
	return FALSE;
}

static int EdadActual(DATE fecha) {
	short diat, mest, aniot, dnac, mnac, anac, edad;
	DToDMY(Today(), &diat, &mest, &aniot);
	DToDMY(fecha, &dnac, &mnac, &anac);
	edad = aniot - anac;
	if (mnac > mest) edad --;
	else
		if ((mnac == mest) && (dnac > diat)) edad --;
	return edad;
}

/* funcion que compara nodos de 2 postulantes, para ordenar la lista */
static int CmpNodos(struct Resultado *p1, struct Resultado *p2, char tipord)
{
	char nom1[SIZE_APYNOM], nom2[SIZE_APYNOM];
	
	switch(tipord) { /* verifico tipo de ordenamiento */
	case 'A':
		strcpy(nom1, p1->apynom);
		strcpy(nom2, p2->apynom);
		StrToUpper(nom1);
		StrToUpper(nom2);
		return strcmp(nom1, nom2);
		break;

	case 'L':
		if (p1->nroleg < p2->nroleg)
			return -1;
		if (p1->nroleg > p2->nroleg)
			return 1;
		return 0;
		break;
	case 'P': 
		if (p1->puntaje > p2->puntaje)
			return -1;
		if (p1->puntaje < p2->puntaje)
			return 1;
		
		strcpy(nom1, p1->apynom);
		strcpy(nom2, p2->apynom);
		StrToUpper(nom1);
		StrToUpper(nom2);
		return strcmp(nom1, nom2);
		break;
	default : /* caso de documentos. Ingresaria 'D'*/ 
		if (strcmp(p1->intext, p2->intext) < 0)	return 1;
		if (strcmp(p1->intext, p2->intext) > 0) return -1;
		if (strcmp(p1->coddoc, p2->coddoc) < 0) return -1;
		if (strcmp(p1->coddoc, p2->coddoc) > 0) return 1;
		if (p1->nrodoc < p2->nrodoc) return -1;
		return 1;
		break;
	} 
}





/*
** agrega un postulante (que cumpla las condiciones solicitadas) a la
** lista resultado de la consulta.
** Se ordena con CmpNodos(...)
*/

static struct Resultado * Agregar(struct Resultado * result,int emp,  
	char * coddoc,long nrodoc, long nroleg, char * intext, DATE fpost,  
	bool asignado,int orden, char * apynom, int edad,  
	bool entreapr, long puntaje, char tipord)
{
	struct Resultado *pnew, *p, *ant;

	pnew = NuevoNodoResultado( emp, coddoc, nrodoc, nroleg, intext, fpost,  
	          asignado, orden, apynom, edad, entreapr, puntaje, tipord);


	if (result == NULL)  // primer nodo
		result = pnew;

	else {

		for (ant = NULL, p = result; p; ant = p, p = p->next) {
			if (CmpNodos(p, pnew, tipord) < 1) continue; //mientras p < pnew //

			// pnew < p 

			pnew->next = p;
			if (ant == NULL) 
				result = pnew;
			else 
				ant->next = pnew;
			break;

		} // cuando se agrega en el final 
		if (ant != NULL)
			ant->next = pnew;
	}
	return result;
}






/*
** agrega un postulante (que cumpla las condiciones solicitadas) a la
** lista resultado de la consulta.
** Se ordena con CmpNodos(...)
*/
/*
static struct Resultado * Agregar(struct Resultado * result,int emp,  
	char * coddoc,long nrodoc, long nroleg, char * intext, DATE fpost,  
	bool asignado,int orden, char * apynom, int edad,  
	bool entreapr, char tipord)
{
	struct Resultado *pnew, *p, *ant;

	pnew = NuevoNodoResultado( emp, coddoc, nrodoc, nroleg, intext, fpost,  
	          asignado, orden, apynom, edad, entreapr, tipord)


	if (result == NULL)  // primer nodo
		result = pnew;

	else {

		for (ant = NULL, p = result; p; ant = p, p = p->next) {
			if (CmpNodos(p, pnew, tipord) < 1) continue; //mientras p < pnew //

			// pnew < p 

			pnew->next = p;
			if (ant == NULL) 
				result = pnew;
			else 
				ant->next = pnew;
			break;

		} // cuando se agrega en el final 
		if (ant != NULL)
			ant->next = pnew;
	}
	return result;
}
*/



static struct Resultado * NuevoNodoResultado ( int emp, char * coddoc,  
    long nrodoc, long nroleg, char * intext, DATE fpost,  
	bool asignado, int orden, char * apynom, int edad,  
	bool entreapr, long puntaje, char tipord)

{
	struct Resultado *pnew;
	
	pnew = (struct Resultado *) Alloc (sizeof(struct Resultado));
	if (pnew == NULL)
		Error("\n   No se pudo obtener suficiente memoria   \n");

	pnew->emp = emp;
	pnew->nrodoc = nrodoc;
	pnew->asignado = asignado;
	pnew->fecha = fpost;
	pnew->nroleg = nroleg;
	pnew->orden = orden;
	pnew->edad = edad;
	pnew->entreapr = entreapr;
	strcpy (pnew->coddoc, coddoc);
	strcpy (pnew->intext, intext);
	strcpy (pnew->apynom, apynom);
	pnew->puntaje= puntaje;
	pnew->next = NULL;

	return pnew;
	
}


/* ve si el postulante alcaza (o supera) el nivel requerido */
static bool CmpNivel(char reque, char postu) {
	switch (reque) {
	case 'M': /* muy bueno */
		if (postu == 'B' || postu == 'R') return FALSE;	break;
	case 'B': /* bueno */
		if (postu == 'R') return FALSE;	break;
	case 'R': /* regular */
		break;
	}
	return TRUE;
}

/*
** Funcion que verifica que el postulante analizado tenga los estudios
** requeridos.
** Retorna TRUE si los cumple o si no se han requerido estudios (lista
** de estudios nula).
*/
bool ValEst(struct Estudios * estud){
	struct Estudios * p = estud;
	if (estud == NULL)
		return TRUE;
	while (p != NULL) {
		SetKey(NIVELPOSTbyEMP, IFld(CABPOST_EMP), SFld(CABPOST_CODDOC),
						LFld(CABPOST_NRODOC), p->estud, p->tit);
		if (p->tit ==NULL_LONG) {						
			if (GetRecord(NIVELPOSTbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 4) == ERROR) {
				if (p->obli)
					return FALSE;
			}
			else 
				if (p->puntos!=NULL_LONG)
					puntos+=p->puntos;
		}
		else  {
			if (GetRecord(NIVELPOSTbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				if (p->obli)
					return FALSE;
			}
			else
				if (p->puntos!=NULL_LONG)
					puntos += p->puntos;
		}
		p = p->next;
	}
	return TRUE;
}

/*
** Idem ValEst(...) pero para los idiomas.
*/
bool ValIdio(struct Idiomas * idio){
	struct Idiomas * p = idio;
	if (idio == NULL)
		return TRUE;
	while (p != NULL) {
		SetKey(IDIOPERbyEMP, IFld(CABPOST_EMP),  SFld(CABPOST_CODDOC),
					LFld(CABPOST_NRODOC), p->idio);
		if (GetRecord(IDIOPERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			if (p->oblil || p->oblih || p->oblie)
				return FALSE;
		}
		else {
			if (!CmpNivel(*p->habla, *SFld(IDIOPER_HABLA))) {
				if (p->oblih)  				
					return FALSE; 
			}
			else  {
				if (p->puntosh!=NULL_LONG)
					puntos+=p->puntosh;				
    		}
			if (!CmpNivel(*p->lee, *SFld(IDIOPER_LEE))) {
				if (p->oblil)
					return FALSE; 
			}
			else {
				if (p->puntosl!=NULL_LONG)
		 			puntos+=p->puntosl;
			}
			if (!CmpNivel(*p->escribe, *SFld(IDIOPER_ESCRIBE)))  {
				if (p->oblie)
					return FALSE;
			}
			else {
				if (p->puntose!=NULL_LONG)
					puntos+=p->puntose;					
			}					
		}           
		p = p->next;
	}
	return TRUE;
}

/*
** Idem ValEst(...) pero para pais - provincia - localidad.
*/
bool ValLoc(struct Local * loc, bool externo) {
	struct Local * p = loc;
	schema sue = FindSchema("sue");
	if (loc == NULL)
		return TRUE;
	while (p != NULL) {
		if ( (externo && 
			(p->pais == NULL_SHORT || IFld(POSTU_PAIS) == p->pais) &&
			(p->prov == NULL_SHORT || IFld(POSTU_PROV) == p->prov) && 
			(p->loc == NULL_LONG || LFld(POSTU_LOCAL) == p->loc)) ||
			(!externo && 
			(p->pais == NULL_SHORT || IFld(sue|PER_CODPAIS) == p->pais) &&
			(p->prov == NULL_SHORT || IFld(sue|PER_PROV) == p->prov) && 
			(p->loc == NULL_LONG || LFld(sue|PER_LOCAL) == p->loc)))
			break;
		p = p->next;
	}
	return (p != NULL);
}

/*
** Idem ValEst(...) pero para los perfiles, debe cumplir todos los
** perfiles solicitados, si se indica sólo uno, devuelve también el
** orden del perfile para el postulante.
*/
bool ValPerf(struct Perfiles * perfil, int * orden) {
	int cant = 0;
	long lect;
	struct Perfiles * p = perfil;
	if (perfil == NULL)
		return TRUE;
	while (p != NULL) {
		SetKey(PERFILPbyEMP, IFld(CABPOST_EMP),  SFld(CABPOST_CODDOC),
					LFld(CABPOST_NRODOC), NULL_SHORT);
		while ((lect = GetRecord(PERFILP,NEXT_KEY|PARTIAL_KEY,IO_NOT_LOCK,3))
				!= ERROR && LFld(PERFILP_PUESTO) != p->puesto)
			;
		if (lect == ERROR)
			return FALSE;
		cant++;
		*orden = IFld(PERFILP_ORDEN) + 1;
		p = p->next;
	}
	if (cant > 1) *orden = 0;
	return TRUE;
}


/*
** Idem ValEst(...) pero para los conocimientos.
*/
bool ValCon(struct Conocim * conoc){

	struct Conocim * p = conoc;
	if (conoc == NULL)
		return TRUE;

	while (p != NULL) {
		SetKey(POSCONbyEMP, IFld(CABPOST_EMP), SFld(CABPOST_CODDOC), 
				LFld(CABPOST_NRODOC), p->conoc);
		if (GetRecord(POSCONbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			if (p->obli)
				return FALSE;
		}
		else
			if (p->puntos!=NULL_LONG)
				puntos += p->puntos;
		p = p->next;
	}
	return TRUE;
}

/*
** Idem ValEst(...) pero para los cursos.
*/
bool ValCur(struct Curso * curso){
	struct Curso * p = curso;
	if (curso == NULL)
		return TRUE;
	while (p != NULL) {
		SetKey(POSCURbyEMP, IFld(CABPOST_EMP), SFld(CABPOST_CODDOC), 
				LFld(CABPOST_NRODOC), p->curso);
		if (GetRecord(POSCURbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			if (p->obli)
			 	return FALSE;
		}
		else
			if (p->puntos!=NULL_LONG)
				puntos += p->puntos;
		p = p->next;
	}
	return TRUE;
}

/*
** Idem ValEst(...) pero para los cursos.
*/
bool ValCurInt(struct Curso * curso, int emp, Int legajo){
	struct Curso * p = curso;

	if (!InstCapac())
		return TRUE;

	if (curso == NULL)
		return TRUE;

	while (p != NULL) {
		if (legajo == NULL_LONG && p->obli) {
			return FALSE;
		}			
		else {
			if (!HizoCurso(emp, legajo, p->curso)) {
				// Funcion de libreria del Modulo de Capacitacion
				if (p->obli)
					return FALSE;
			}
			else
				if (p->puntos!=NULL_LONG)
					puntos+= p->puntos;
		}
		p = p->next;
	}

	return TRUE;
}

/*
** Idem ValEst(...) pero para los Programas.
*/
bool ValPgm(struct Programas * pgm, int emp, Int legajo){
	struct Programas * p = pgm;
	if (!InstPlcar())
		return TRUE;

	if (pgm == NULL)
		return TRUE;


	while (p != NULL) {
		if (legajo == NULL_LONG && p->obli)
			return FALSE;
    	else  {
			if ( ! HizoPgm(emp, legajo, p->cod)) {
				// Funcion de libreria del Modulo de Plan de Carrera
				if (p->obli)
					return FALSE;
			}
			else
				if (p->puntos!=NULL_LONG)
					puntos += p->puntos;
		}
		p = p->next;
	}
	return TRUE;
}
/*
** Idem ValEst(...) pero para los Programas.
*/
bool ValProy(struct Proyectos * proy, int emp, Int legajo){
	struct Proyectos * p = proy;
	if (!InstPlcar())
		return TRUE;

	if (proy == NULL)
		return TRUE;

		
	while (p != NULL) {
		if (legajo == NULL_LONG && p->obli)
			return FALSE;
		else {
			if ( ! HizoProy(emp, legajo, p->cod)) {	// Libreria de Plan de Carrera
				if (p->obli)
					return FALSE;
			}
			else
				if (p->puntos!=NULL_LONG)
					puntos += p->puntos;
		}
		p = p->next;
	}
	return TRUE;
}

/*
** Idem ValEst(...) pero para las evaluaciones de desempenio.
*/
bool ValEva(struct Evades * eva, int emp, Int legajo){
	struct Evades * e = eva;
	if (!InstEvade())
		return TRUE;

	if (eva == NULL)
		return TRUE;

		
	while (e != NULL) {
		if (legajo == NULL_LONG && e->obli)
			return FALSE;
		else {
			if  (!HizoEvade(emp, legajo, e->cod, e->result)) {
				if (e->obli)
					return FALSE;
			}
			else
				if (e->puntos!=NULL_LONG)
					puntos += e->puntos;
		}			
		e = e->next;
	}
	return TRUE;
}

bool ValResultTotEva(int emp, Int legajo, long result){
	if (result != NULL_LONG)
		return ResultTotEva(emp, legajo, result);
	return TRUE;		
}

bool ValResultUltEva(int emp, Int legajo, long result){
	if (result != NULL_LONG)
		return ResultUltEva(emp, legajo, result);
	return TRUE;		
}

bool ValPasantia(bool pasint, bool pasext, int emp, Int legajo){

	char tipdoc[10];
	Int nrodoc;

	if (!InstPlcar())
		return TRUE;
		
	if (pasint)  {
		if (legajo == NULL_LONG)
			return FALSE;
		else
			return HizoPasInterna(emp, legajo);
	}

	if (pasext) {
		if (legajo != NULL_LONG) {
			return HizoPasExterna(emp, SFld(PER_CODDOC,0), LFld(PER_NRODOC,0));
		}
		else
			return HizoPasExterna(IFld(CABPOST_EMP),
								SFld(CABPOST_CODDOC), LFld(CABPOST_NRODOC) );
	}

	return TRUE;
}


static void AddlegE(long nroleg)
{	
	struct legemple * p;
	p = (struct legemple *) Alloc (sizeof(struct legemple));
	p->nroleg = nroleg;
	p->sig = CablegE;
	CablegE = p;
}

static bool Estaleg(struct legemple * cab, long nroleg ) 
{
	struct legemple * p;
	for (p = cab; p && p->nroleg != nroleg; p=p->sig)
		;
	return (p != NULL);
}

/*
** Parámetros:
** emp : empresa que estoy considerando.
** req : si indico nro.de req.no considero los postulantes que estén
**       asociados a otro (sólo los asociados al mismo o sin asociación).
** sex : sexo que me interesa seleccionar, 0 indistinto.
** estciv : estado civil, 0 indistinto.
** emin: edad mínima.
** emax: edad máxima.
** conoc: puntero a una lista con los datos de los conocimientos requeridos.
** estud: puntero a lista con los estudios requeridos.
** idio : puntero a lista con los idiomas requeridos.
** local: puntero a lista con las localidades requeridas.
** perfil: puntero a lista con los perfiles requeridos.
** result: puntero a la lista que resulta luego de la selección.
** Los punteros a lista pueden ser nulos, en cuyo caso no considero el item
** para la selección.
** Retorna el puntero a la lista Resultado.
*/

struct Resultado * SelectPostu(int emp, int req, int sex, int estciv,  
	int emin, int emax, struct Conocim * conoc,  
	struct Curso * curso, struct Curso * cursoint, struct Estudios * estud,  
	struct Idiomas * idio,  
	struct Local * loc, struct Perfiles * perfil, 
	struct Programas * pgm, struct Proyectos * proy, struct Evades *eva,
    bool pasint, bool pasext,  struct Resultado * result,  
	char tipopost, char tipord, long puesto, char * codestr, long codubi, long ccosto, 
	long restoteva, long resulteva)
{
	char apynom[SIZE_APYNOM], int_ext[2];
	int edad, orden;
	bool entreapr;
	bool perint, asignado;
	dbcursor cCabpost;
	schema sue, emple, old;

    strcpy(int_ext, NULL_STR);				
	old = CurrentSchema();
	sue = OpenSchema("sue", IO_EABORT);
	emple = OpenSchema("emple", IO_EABORT);
	SwitchToSchema(emple);

	/* verifico si considera empleados internos en forma automatica */
	SetIFld(PARAME_EMP, emp);
	SetIFld(PARAME_NROPAR, PAR_PERINT);

	if (GetRecord(PARAMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR 
					&& ( !strcmp (SFld(PARAME_VAL), "S") ) )
		perint = TRUE;
	else
		perint= FALSE;

	cCabpost = CreateCursor(CABPOSTbyEMP, IO_NOT_LOCK);
	/* SetCursorFlds: sólo trae los campos que necesito */
	SetCursorFrom(cCabpost, emp, LOW_VALUE, MIN_LONG);
	SetCursorTo  (cCabpost, emp, HIGH_VALUE, MAX_LONG);	

	while (FetchCursor(cCabpost) != ERROR) {
		orden = 0;
		apynom[0]=0;
		puntos=0;
		if (IsNull(CABPOST_NROLEG)) {  //el registro de CABPOST es EXTERNO

			if (tipopost =='I' || tipopost == 'P')
				continue;

			// si se piden condiciones de cursos internos o pasantias o
			// proyectos especiales continuar

//			if (cursoint!=NULL || pgm!=NULL || proy!=NULL)
//				continue; 

			if (restoteva != NULL_LONG || resulteva != NULL_LONG)
				continue;

			SetKey(POSTUbyEMP,emp,SFld(CABPOST_CODDOC),LFld(CABPOST_NRODOC));
			if (GetRecord(POSTU,THIS_KEY,IO_NOT_LOCK) == ERROR) {
				continue;
			}    // si ya se paso a Denarius continuo

 			edad = EdadActual(DFld(POSTU_FECNAC));

			if ((estciv != 0 && IFld(POSTU_ESTCIV) != estciv)
				|| (sex != 0 && sex != IFld(POSTU_SEXO))
				|| (edad < emin || edad > emax)
				|| !ValCon(conoc) || !ValCur(curso)
				|| !ValEst(estud) || !ValCurInt(cursoint, IFld(CABPOST_EMP), NULL_LONG) 
				|| !ValPgm(pgm, IFld(CABPOST_EMP), NULL_LONG) || !ValProy(proy, IFld(CABPOST_EMP), NULL_LONG)				
				|| !ValPasantia(pasint, pasext, NULL_SHORT, NULL_LONG)
				|| !ValIdio(idio) || !ValLoc(loc,TRUE) || !ValEva(eva, IFld(CABPOST_EMP), NULL_LONG)
				|| !ValPerf(perfil, &orden) ) {
				continue;
			}

			strcpy(apynom, SFld(POSTU_APELL));
			strcpy(apynom+strlen(apynom), ", ");
			strncpy(apynom+strlen(apynom), SFld(POSTU_NOMBRE), (SIZE_APYNOM -1) - strlen(apynom));
			apynom[SIZE_APYNOM-1]='\0';
			strcpy(int_ext, "E");
		}  // fin cabpost.nroleg == null


		else { // el registro de CABPOST es un postulante INTERNO

			if (tipopost == 'E')
				continue;


			SetKey(sue|PERbyEMP,emp,LFld(CABPOST_NROLEG));
			if (GetRecord(sue|PERbyEMP,THIS_KEY,IO_NOT_LOCK) == ERROR ||
						IFld(sue|PER_ACTIVO) == 0){
				continue;

			 } 

			 // si tipopost == pendientes de reubicacion chequeo que tenga
			 // alguna accion pendiente en el Modulo de Plan de Carrera. Si vino con 'P' == > instalado
			 // TieneReubic : del lib de plan de carrera

			 if (tipopost == 'P') {
			 	if (!TieneReubic(emp, LFld(CABPOST_NROLEG), puesto, codestr, codubi, ccosto)) {
			 		continue; 
			 	}
				strcpy(int_ext, "P");			 	  	
			  }
			  else
			  	strcpy(int_ext, "I");  

			edad = EdadActual(DFld(sue|PER_FECNAC));

			if ((estciv != 0 &&	IFld(sue|PER_ESTCIV) != estciv)
				|| (sex != 0 && sex != IFld(sue|PER_SEXO))
		 		|| (edad < emin || edad > emax)
				|| !ValCon(conoc) || !ValCur(curso)
				|| !ValCurInt(cursoint, emp, LFld(CABPOST_NROLEG)) 
				|| !ValPgm(pgm, emp, LFld(CABPOST_NROLEG))
				|| !ValProy(proy, emp, LFld(CABPOST_NROLEG)) 
				|| !ValPasantia(pasint, pasext, emp, LFld(CABPOST_NROLEG))
				|| !ValEst(estud) 
				|| !ValIdio(idio) || !ValLoc(loc,FALSE) 
				|| !ValPerf(perfil, &orden) || !ValEva(eva, emp, LFld(CABPOST_NROLEG))
				|| !ValResultTotEva(emp, LFld(CABPOST_NROLEG), restoteva) 
				|| !ValResultUltEva(emp, LFld(CABPOST_NROLEG), resulteva)) {
				continue;
			}

			AddlegE(LFld(sue|PER_NROLEG)); // arma una lista de internos
			strncpy(apynom, SFld(sue|PER_APYNOM), SIZE_APYNOM -1);
			apynom[SIZE_APYNOM-1]='\0';

		} // fin lectura postulante INTERNO

//TODOS los postulantes de CABPOST

		SetIFld(HISREQ_EMP, emp);
		SetIFld(HISREQ_NROREQ, req);
		SetFld(HISREQ_CODDOC, SFld(CABPOST_CODDOC));
		SetLFld(HISREQ_NRODOC, LFld(CABPOST_NRODOC));
		SetLFld(HISREQ_NRODOC, LFld(CABPOST_NRODOC));
		asignado = (GetRecord(HISREQbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR);
		entreapr = PostConEntrev(emp, req,
				SFld(CABPOST_CODDOC),LFld(CABPOST_NRODOC));


		result = Agregar(result, emp, SFld(CABPOST_CODDOC),
		    LFld(CABPOST_NRODOC), LFld(CABPOST_NROLEG), int_ext,
		    DFld(CABPOST_FECHA), asignado, orden, apynom, edad,
		    entreapr, puntos, tipord);

	} // fin CABPOST

	DeleteCursor(cCabpost);


/* SI ESTA ACTIVADO EL PARAMETRO Y NO HAY CONDICIONES USO PER */

	if ( perint == TRUE && (tipopost == 'I' || tipopost == 'A' ||
	     tipopost == 'P') &&
	   ( conoc == NULL && curso == NULL && estud == NULL &&  
	     idio == NULL && perfil == NULL ))  // NO hay condiciones de CURRICULUM

	{

		dbcursor cper= CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);

		SetCursorFrom(cper, emp, NULL_LONG);
		SetCursorTo(cper, emp, MAX_LONG);

		while (FetchCursor(cper) != ERROR) {



			if (IFld(sue|PER_ACTIVO) == 0)
				continue;


			if ( Estaleg(CablegE, LFld(sue|PER_NROLEG)) ) 
				continue;

			 if (tipopost == 'P') {
			 	if (!TieneReubic(emp, LFld(sue|PER_NROLEG), puesto, codestr,
			 	 codubi, ccosto)) {
			 		continue; 
			 	}
				strcpy(int_ext, "P");			 	  	
			  }
			  else
			  	strcpy(int_ext, "I");  


			edad = EdadActual(DFld(sue|PER_FECNAC));
			if ((estciv != 0 &&	IFld(sue|PER_ESTCIV) != estciv)
				|| (sex != 0 && sex != IFld(sue|PER_SEXO))
		 		|| (edad < emin || edad > emax)
		 		|| !ValLoc(loc,FALSE) ) {
		 		continue;
		 	}



			if (!ValCurInt(cursoint, emp, LFld(sue|PER_NROLEG)) ||
			    !ValPgm(pgm, emp, LFld(sue|PER_NROLEG))         ||
			    !ValProy(proy, emp, LFld(sue|PER_NROLEG))       ||
			    !ValPasantia(pasint, pasext, emp, LFld(sue|PER_NROLEG)) ||
			    !ValEva(eva, emp, LFld(sue|PER_NROLEG))) {
				continue;
			}

			
			strncpy(apynom, SFld(sue|PER_APYNOM), SIZE_APYNOM -1);
			apynom[SIZE_APYNOM-1]='\0';

			result = Agregar(result, emp, SFld(sue|PER_CODDOC, 0),
			    LFld(sue|PER_NRODOC, 0), LFld(sue|PER_NROLEG), int_ext,
			    NULL_DATE, FALSE, 0, apynom, edad, FALSE, puntos, tipord);
	
		} 
	} 
	SwitchToSchema(old);
	return result;
}






/*
** Armo la lista con los conocimientos que son requeridos en la selección,
** se arma directamente con los valores indicados en el subform.pasado
** como parámetro.
** Devuelve puntero a la Lista de concoc.requeridos, nulo si no hay.
*/

struct Conocim * CargoConoc(struct Conocim * conoc, form fm,  
	int multi, fmfield fn1, fmfield fn2, fmfield fn3) {
	int i;
	struct Conocim * aux = conoc, *paux;

	for (i=0; i < multi && !FmIsNull(fm,fn1,i); i++) {

		paux = (struct Conocim *) malloc (sizeof(struct Conocim));

		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");

		paux->conoc  = FmIFld(fm,fn1,i);

		paux->obli = (fn2 != NULL_SHORT && 
							!strcmp(FmSFld(fm, fn2, i), "O")) ? TRUE : FALSE;

		paux->puntos = (fn3 != NULL_SHORT) ? FmLFld(fm, fn3, i) : NULL_LONG;

		paux->next   = NULL;

		if (conoc == NULL) {

			conoc = paux;
			aux = paux;
		}
		else {

			aux->next = paux;
			aux = paux;
		}

	}

	return conoc;
}



struct Curso * CargoCurso(struct Curso * curso, form fm,  
	int multi, fmfield fn1, fmfield fn2, fmfield fn3) {
	int i;
	struct Curso * aux = curso, *paux;
	for (i=0; i < multi && !FmIsNull(fm,fn1,i); i++) {
		paux = (struct Curso *) malloc (sizeof(struct Curso));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->curso  = FmIFld(fm,fn1,i);
		
		paux->obli = (fn2 != NULL_SHORT && 
						!strcmp(FmSFld(fm, fn2, i), "O")) ? TRUE : FALSE;
						
		paux->puntos = (fn3 != NULL_SHORT) ? FmLFld(fm, fn3, i) : NULL_LONG;
		
		paux->next   = NULL;
		if (curso == NULL) {
			curso = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return curso;
}




struct Estudios * CargoEstud(struct Estudios * estud, form fm,  
	int multi, fmfield fn1, fmfield fn2, fmfield fn3, fmfield fn4) {
	int i;
	struct Estudios * aux = estud, *paux;
	for (i=0; i < multi && !FmIsNull(fm,fn1,i); i++) {
		paux = (struct Estudios *) malloc (sizeof(struct Estudios));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->estud = FmIFld(fm,fn1,i);
		paux->tit	= FmLFld(fm, fn2, i);
		paux->obli	= (fn3 != NULL_SHORT 
						&& !strcmp(FmSFld(fm, fn3, i), "O")) ? TRUE : FALSE;
		paux->puntos= (fn4 != NULL_SHORT) ? FmLFld(fm, fn4, i) : NULL_LONG;
		paux->next  = NULL;
		if (estud == NULL) {
			estud = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return estud;
}




struct Idiomas * CargoIdio(struct Idiomas * idio, form fm,  
	int multi, fmfield idi, fmfield lee, fmfield esc, fmfield hab, 
	fmfield oblih, fmfield oblil, fmfield oblie, fmfield puntosh, 
	fmfield puntosl, fmfield puntose) {
	int i;
	struct Idiomas * aux = idio, *paux;
	for (i=0; i < multi && !FmIsNull(fm,idi,i); i++) {
		paux = (struct Idiomas *) malloc (sizeof(struct Idiomas));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->idio    = FmIFld(fm,idi,i);
		strcpy(paux->lee, FmSFld(fm,lee,i));
		strcpy(paux->escribe, FmSFld(fm,esc,i));
		strcpy(paux->habla, FmSFld(fm,hab,i));
		paux->oblil	   = (!strcmp(FmSFld(fm, oblil, i), "O"))? TRUE: FALSE;
		paux->oblih	   = (!strcmp(FmSFld(fm, oblih, i), "O"))? TRUE: FALSE;
		paux->oblie	   = (!strcmp(FmSFld(fm, oblie, i), "O"))? TRUE: FALSE;
		paux->puntosl  = FmLFld(fm, puntosl, i);
		paux->puntose  = FmLFld(fm, puntose, i);
		paux->puntosh  = FmLFld(fm, puntosh, i);
		paux->next  = NULL;
		if (idio == NULL) {
			idio = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return idio;
}




struct Local * CargoLocal(struct Local * loc, form fm, int multi,  
	fmfield pais,fmfield prov, fmfield local) {
	int i;
	struct Local * aux = loc, *paux;
	for (i=0; i < multi && !FmIsNull(fm,pais,i); i++) {
		paux = (struct Local *) malloc (sizeof(struct Local));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->pais = FmIFld(fm,pais,i);
		paux->prov = FmIFld(fm,prov,i);
		paux->loc  = FmLFld(fm,local,i);
		paux->next   = NULL;
		if (loc == NULL) {
			loc = paux;
			aux = paux;
		}         
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return loc;
}




struct Perfiles * CargoPerfiles(struct Perfiles * perfil, form fm,  
	int multi, fmfield pos) {
	int i;
	struct Perfiles * aux = perfil, *paux;

	for (i=0; i < multi && !FmIsNull(fm,pos,i); i++) {

		paux = (struct Perfiles *) malloc (sizeof(struct Perfiles));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");

		paux->puesto = FmLFld(fm,pos,i);

		paux->next   = NULL;
		if (perfil == NULL) {

			perfil = paux;
			aux = paux;
		}
		else {

			aux->next = paux;
			aux = paux;
		}
	}

	return perfil;
}




struct Programas * CargoPgm(struct Programas * pgm, form fm,  
	int multi, fmfield pos, fmfield fn2, fmfield fn3) {
	int i;
	struct Programas * aux = pgm, *paux;
	for (i=0; i < multi && !FmIsNull(fm,pos,i); i++) {
		paux = (struct Programas *) malloc (sizeof(struct Programas));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->cod = FmLFld(fm,pos,i);
		paux->obli = (!strcmp(FmSFld(fm, fn2, i), "O"))? TRUE: FALSE;
		paux->puntos = FmLFld(fm, fn3, i);
		paux->next   = NULL;
		if (pgm == NULL) {
			pgm = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return pgm;
}




struct Proyectos * CargoProy( struct Proyectos * proy, form fm,  
	int multi, fmfield pos, fmfield fn2, fmfield fn3) {
	int i;
	struct Proyectos * aux = proy, *paux;
	for (i=0; i < multi && !FmIsNull(fm,pos,i); i++) {
		paux = (struct Proyectos *) malloc (sizeof(struct Proyectos));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->cod = FmLFld(fm,pos,i);
		paux->obli = (!strcmp(FmSFld(fm, fn2, i), "O"))? TRUE: FALSE;
		paux->puntos = FmLFld(fm, fn3, i);
		paux->next   = NULL;
		if (proy == NULL) {
			proy = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return proy;
}

struct Evades * CargoEva(struct Evades * eva, form fm,  
	int multi, fmfield pos, fmfield pos2, fmfield fn3, fmfield fn4) {
	int i;
	struct Evades * aux = eva, *paux;
	for (i=0; i < multi && !FmIsNull(fm,pos,i); i++) {
		paux = (struct Evades *) malloc (sizeof(struct Evades));
		if (paux == NULL)
			Error("\n   No se pudo obtener suficiente memoria   \n");
		paux->cod = FmLFld(fm,pos,i);
		paux->obli = (!strcmp(FmSFld(fm, fn3, i), "O"))? TRUE: FALSE;
		paux->puntos = FmLFld(fm, fn4, i);
		paux->result = FmLFld(fm, pos, i);
		paux->next   = NULL;
		if (eva == NULL) {
			eva = paux;
			aux = paux;
		}
		else {
			aux->next = paux;
			aux = paux;
		}
	}
	return eva;
}




/* Libera el espacio de la lista Resultado */
void FreeResult(struct Resultado *head)
{
	if (head == NULL) 	return;
	else {
		FreeResult(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Conocim */
void FreeConoc(struct Conocim *head)
{
	if (head == NULL) 	return;
	else {
		FreeConoc(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Curso */
void FreeCurso(struct Curso *head)
{
	if (head == NULL) 	return;
	else {
		FreeCurso(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Estudios */
void FreeEstud(struct Estudios *head)
{
	if (head == NULL) 	return;
	else {
		FreeEstud(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Idiomas */
void FreeIdio(struct Idiomas *head)
{
	if (head == NULL) 	return;
	else {
		FreeIdio(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Local */
void FreeLocal(struct Local *head)
{
	if (head == NULL) 	return;
	else {
		FreeLocal(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Perfiles */
void FreePerfiles(struct Perfiles *head)
{
	if (head == NULL) 	return;
	else {
		FreePerfiles(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Programas */
void FreePgm(struct Programas *head)
{
	if (head == NULL) 	return;
	else {
		FreePgm(head->next);
		free(head);
	}
}



/* Libera el espacio de la lista Proyectos */
void FreeProy(struct Proyectos *head)
{
	if (head == NULL) 	return;
	else {
		FreeProy(head->next);
		free(head);
	}
}

/* Libera el espacio de la lista Evades */
void FreeEva(struct Evades *head)
{
	if (head == NULL) 	return;
	else {
		FreeEva(head->next);
		free(head);
	}
}


/*void ObtenerTipo_y_NroDoc(int emp, Int legajo, char * tipdoc, Int * nrodoc) {

// ya tengo leido el registro de PER

	if (!IsNull(PER_NRODOC[0])) {
			strcpy(tipdoc, SFld(PER_CODDOC[0]));
			* nrodoc = LFld(PER_NRODOC[0])
	} else {   // si el documento es un pasaporte
			strcpy(tipdoc, "PAS");   // LEER EL EQUIVALENTE EN SIRH
			* nrodoc = SFld(PER_NRODOC[0])
	}  		
}
*/ 
