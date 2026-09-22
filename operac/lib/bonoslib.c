/********************************************************************
*
* MODULE & VERSION : @(#)bonos.c	1.2
* DATE             : 02/05/30
* TIME             : 19:33:28
*
* CREATED          : 27/05/2002
*
* DESCRIPTION:	
*	Modulo para el calculo de Bonos
*
* ROUTINE       |  PURPOSE
*---------------+-------------------------------------------------------------------------------------------
*
* ListaBonos es un lista que contiene informacion referente a los bonos asociados a un vigilador por
* cliente/objetivo.
* Esta implementado como un vector de campos de longitud fija, con MAXLEG posiciones.
* Tiene tres punteros asociados: 
* - pvig, que apunta al primer elemento de la lista.
* - fvig, que apunta al fin de la lista, al proximo elemento vacio de la lista.
* - cvig, que apunta al elemento actual de la lista. Se utiliza como cursor, para recorrer la lista.
* Nota: el primer elemento no se utiliza (Esto es para implementar la funcion Proximo).
* 
* // Funciones Públicas
* void InicializarListaBonos	| Inicializa la lista de bonos.
* void VolverInicioListaBonos	| Se posiciona en el primer elmento de la lista.
* bool ProximoListaBonos		| Se posiciona en el proximo elemento de la lista.
* void OrdPorCliObjListaBonos	| Ordena la lista de bonos por bono/cliente/objetivo/legajo.
* void OrdPorLegajoListaBonos	| Ordena la lista de bonos por bono/legajo/cliente/objetivo.
* void CargarListaBonos 		| Carga la informacion de bonos en la lista.
* bool EstaVaciaListaBonos	    | Devuelve true si la lista esta vacia
*
* // Funciones Privadas
* bool EsFinalListaBonos		| Indica si es el final de la lista de bonos.
* int compvig1					| Comparaciones para ordenamiento por bono/cliente/objetivo/legajo.
* int compvig2					| Comparaciones para ordenamiento por bono/legajo/cliente/objetivo.
* void CalcularBono1			| Realiza calculos para el bono 1 (Nocturno) y actualiza la lista con los resultados.
* void CalcularBono2			| Realiza calculos para el bono 2 y actualiza la lista con los resultados.
* void CalcularBono3            | Realiza calculos para el bono 3 y actualiza la lista con los resultados.
* void CargarBonos				| Busca los bonos del vigilador en un cliente/objetivo y los carga en la lista.
* bool EsParteConfirmado		| Indica si el parte esta confirmado.
* void CargarVigil				| Carga un vigilador/cliente/objetivo en la lista.
* void CopiarVigil				| Copiar un elemento de la lista: los datos de un vigilador.
*
********************************************************************/
#include <ideafix.h>
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "comerc.h"
#include "bonos.h"

//#define DEBUG	1

// Funciones Privadas
static bool EsParteConfirmado(schema operac);
static int compvig1(SVigil *a, SVigil *b);
static int compvig2(SVigil *a, SVigil *b);
static void CargarBonos(int emp, long cliente, int objet, long nroleg, char * condic, int bonod, int bonoh,
						bool hor_nocturno, DATE fechah, DATE fecha);
static void CargarVigil(long nroleg, int emp, long cliente, int objet, char * condic, int bono, long cant,
						long precio, long vigvalor, bool hor_nocturno);
static void CalcularBono1();
static void CalcularBono2();
static void CalcularBono3();
static bool EsFinalListaBonos();
static void CopiarVigil(const SVigil *a, SVigil *b);
static bool TieneBonos(long cliente, int objetivo, int bonod, int bonoh);
static bool ObjetivoActivo(long cliente, int objetivo, DATE fechad);
static long CantNodosListaBonos();

// Variables Públicas
static SVigil pvig[MAXLEG];

// Variables privadas
static SVigil *fvig, *cvig;
static SBono  pbon[MAXLEG], *ubon= pbon;
private struct s_bono_lib estbono;


/*************************
* Funciones Públicas 
**************************/
void InicializarListaBonos()
{
	int i;
	
	for (i=0; i<MAXLEG ; i++) {
	    pvig[i].emp=0;
	    pvig[i].cli=0;
	    pvig[i].obj=0;
	    pvig[i].bono=0;
	    pvig[i].bvalor=0;
	    pvig[i].vigvalor=0;
	    pvig[i].bcant=0;
		pvig[i].nroleg=0;
		pvig[i].cdiasT=0;
		pvig[i].cdiasL=0;
		pvig[i].cdiasA=0;
		pvig[i].cdiasF=0;
		pvig[i].cdiasV=0;
		pvig[i].diasnoc=0;
        //
		pvig[i].qbase=0;
		pvig[i].ibase=0;
		pvig[i].pbase=0;
		pvig[i].impfinal=0;
        //
		pbon[i].emp=0;
		pbon[i].cli=0;
		pbon[i].obj=0;
		pbon[i].bono=0;
	    pbon[i].bcant=0;
		pbon[i].bvalor=0;
	    pbon[i].vigvalor=0;
	}
	fvig=pvig;
	fvig++; // la primera posicion de la lista no se usa, esto es por la implementacion de Proximo
	VolverInicioListaBonos();
    //
    ubon=pbon;
}

void VolverInicioListaBonos()
{
	cvig=pvig;
}   

/* Se posiciona en el proximo elemento de la lista y devuelve 1 (OK), caso contrario
devuelve ERROR, sin cambiar de posicion.
*/
bool ProximoListaBonos(SVigil *s)
{
	// chequea que no sea el final de la lista (no olvidar que la primera posicion esta vacia).
	if (EsFinalListaBonos())
		return ERROR;  // -1

	// adelanta el cursor, cvig
  	cvig++;
	
	// chequea que no sea el final de la lista (no olvidar que la primera posicion esta vacia).
	if (EsFinalListaBonos()) 
		return ERROR;  // -1

	// 3ro. copia los datos
	CopiarVigil(cvig, s);

	return 1;
}

void OrdPorCliObjListaBonos()
{
	qsort((char *)pvig, (unsigned)(fvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)compvig1 );
}   

void OrdPorLegajoListaBonos()
{
	qsort((char *)pvig, (unsigned)(fvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)compvig2 );
}

void CargarListaBonos(int bonod, int bonoh, int listapor, int emp, long clid, long clih, int objd, int objh, DATE fechad, DATE fechah, long vigd, long vigh)
{
	dbcursor cparte= (dbcursor)NULL;
	int objet;
	long nroleg, cliente;
	DATE fecha;
	char condic[2];
	schema act, operac;
	bool hor_nocturno = FALSE;

 	act=CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(act);
	
#ifdef DEBUG
fprintf(stderr, "CargarListaBonos() - bonos (%d  %d) emp %d, clid %ld, clih %ld, objd %d, objh %d, fecd %.3D, fech %.3D, legd %ld, legh %ld\n",
	bonod, bonoh, emp, clid, clih, objd, objh, fechad, fechah, vigd, vigh);
#endif

	if (listapor== O_XCLIOBJ) {
		// primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
		cparte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
		SetCursorFrom(cparte, emp, clid, objd, fechad, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, emp, clih, objh, fechah, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else if (listapor== O_XLEGAJO) {
		// index emple(emp, nroleg, dia, cliente, objetivo),
		cparte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
		SetCursorFrom(cparte, emp, vigd, fechad, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cparte, emp, vigh, fechah, MAX_LONG, MAX_SHORT);
	}
	while (FetchCursor(cparte) != ERROR) {
		nroleg  = LFld(operac|PARTE_NROLEG);
		emp     = IFld(operac|PARTE_EMP);
		cliente = LFld(operac|PARTE_CLIENTE);
		objet   = IFld(operac|PARTE_OBJETIVO);
		fecha   = DFld(operac|PARTE_DIA);
		sprintf(condic, "%s", SFld(operac|PARTE_CONDIC));

		// Filtro fechas de partes no solicitadas en el form
		if( DFld(operac|PARTE_DIA) < fechad ||	DFld(operac|PARTE_DIA) > fechah )
			continue;

		//Filtro horario nocturno para Bono Nocturno.
		if (bonod == BONO_1_NOC && bonoh == BONO_1_NOC) {
			if (TFld(operac|PARTE_HORASAL) < StrToT(NOCT_SALIDA))
				hor_nocturno = FALSE;
			else {
				if (TFld(operac|PARTE_HORAENT) <= TFld(operac|PARTE_HORASAL)) {
					if (TFld(operac|PARTE_HORAENT) <= StrToT(NOCT_ENTRADA))
						hor_nocturno = TRUE;
				}
				else {
					// Si la salida es >= a 0:30 significa que entro antes. Es valido.
					if (TFld(operac|PARTE_HORASAL) >= StrToT(NOCT_ENTRADA))
						hor_nocturno = TRUE;
				}
			}
		}

	  	// Filtro objetivos inactivos
	  	if(!ObjetivoActivo(cliente, objet, fechad))
	  		continue;

		if (TieneLic(emp, nroleg, fecha))
			strcpy(condic, "L");

		// Filtro partes T no confirmados
		if(strcmp(condic,"T")==0 && !EsParteConfirmado(operac))
			continue;

	  	// Filtro clientes/objetivos sin bonos que pertenezcan al rango bonod-bonoh
	  	if(!TieneBonos(cliente, objet, bonod, bonoh) && bonod != BONO_1_NOC && bonoh != BONO_1_NOC)
	  		continue;

	  	// carga condiciones A, F, V y T confirmada.
	  	CargarBonos(emp, cliente, objet, nroleg, condic, bonod, bonoh, hor_nocturno, fechah, fecha);
	}
	// ya se recorrio la tabla PARTE, Dicha informacion está en memoria
	// ahora se la utiliza para calcular los diferentes bonos.
	CalcularBono1();
	CalcularBono2();
	CalcularBono3();
	DeleteCursor(cparte);
}

bool EstaVaciaListaBonos()
{
	return (CantNodosListaBonos()==0? TRUE:FALSE);
}

/*********************************************************************************************************/

/*************************
* Funciones Privadas
**************************/

static int compvig1(SVigil *a, SVigil *b)
{
	return  a->bono   > b->bono   ? 1 : a->bono   < b->bono   ? -1 :
			a->cli    > b->cli    ? 1 : a->cli    < b->cli    ? -1 :
			a->obj    > b->obj    ? 1 : a->obj    < b->obj    ? -1 :
			a->nroleg > b->nroleg ? 1 : a->nroleg < b->nroleg ? -1 :
			0;
}
/*********************************************************************************************************/
static int compvig2(SVigil *a, SVigil *b)
{
	return  a->bono   > b->bono   ? 1 : a->bono   < b->bono   ? -1 :
			a->nroleg > b->nroleg ? 1 : a->nroleg < b->nroleg ? -1 :
			a->cli    > b->cli    ? 1 : a->cli    < b->cli    ? -1 :
			a->obj    > b->obj    ? 1 : a->obj    < b->obj    ? -1 :
			0;
}
/*********************************************************************************************************/
static bool EsParteConfirmado(schema operac)
{
	if(strcmp(SFld(operac|PARTE_CONDIC),"A")==0)
		return TRUE;
		
	if(	IFld(operac|PARTE_HSNOR)>0 || IFld(operac|PARTE_HS50)>0 || 
	   		IFld(operac|PARTE_HS100F)>0 || IFld(operac|PARTE_HS100FE)>0 )
		return TRUE;   		
	return FALSE;
}
/*********************************************************************************************************/
static void CargarBonos(int emp, long cliente, int objet, long nroleg, char * condic, int bonod, int bonoh,
						bool hor_nocturno, DATE fechah, DATE fecha)
{
	SBono * aux;
	long vigvalor;
    _SParam_PVivo parhora;

	// Buscar bonos para el cliente/objetivo
  	for (aux = pbon; 
		 aux<ubon && !(aux->emp==emp && aux->cli==cliente && aux->obj==objet);
		 aux++)
		;
		
	if (aux == ubon) { // No existe el cliente/objetivo => buscar bonos y cargarlos en ubon

		if (bonod == BONO_1_NOC && bonoh == BONO_1_NOC) {
			ubon->emp      = emp;
			ubon->cli      = cliente;
			ubon->obj      = objet;
			ubon->bono     = bonod;
			ubon->bcant    = 0;
			ubon->bvalor   = 0;
			ubon->vigvalor = 0;
			ubon++;
		}
		else {
			// Me devuelve los bonos vendidos al cliente/objetivo 
			InicPuestosVivos(	emp,
							cliente,
							objet,
							NULL_SHORT,
							NULL_SHORT,       //Modelo OT
							fechah,
							fechah,
							FALSE,            //Calcule tarifario
							TRUE,             //Calcule Bonosf
							_VALIDAR_FECINI,  //Considera fecha de inicio de la ot
							NULL_SHORT,       //Todo tipo de OT
							TRUE,
							FALSE,
							parhora);			  //OT aprobada?
 
			// Cargar estructura
			VolverInicioPuestosVivos();

			while (ProximoBonoVivo(&estbono)) {

				// Filtro Bonos no solicitados en el form
				if (estbono.bono < bonod ||	estbono.bono > bonoh )
					continue;
			
				if (ubon == &pbon[MAXLEG])
					Error("Tabla interna de Bonos saturada. Max %d", MAXLEG);

				vigvalor = PrecioRemun(cliente, objet, estbono.bono, fechah);

				ubon->emp      = emp;
				ubon->cli      = cliente;
				ubon->obj      = objet;
				ubon->bono     = estbono.bono;
				ubon->bcant    = estbono.bcant;
				ubon->bvalor   = estbono.bprecio;
				ubon->vigvalor = vigvalor;
				ubon++;
			}
		}
	}

	// Tengo un legajo en emp,cliente,objetivo con una condic !!!
	
	// Encontrar los bonos (emp,cliente,objetivo) del legajo actual
	for (aux=pbon; aux<ubon; aux++) {
		if (aux->emp==emp && aux->cli==cliente && aux->obj==objet)
	 		CargarVigil(nroleg, emp, cliente, objet, condic, aux->bono, aux->bcant, aux->bvalor, aux->vigvalor,
	 					hor_nocturno);
	}

}
/*********************************************************************************************************/
static void CargarVigil(long nroleg, int emp, long cliente, int objet, char * condic, int bono, long cant,
						long precio, long vigvalor, bool hor_nocturno)
{
	SVigil * aux;

#ifdef DEBUG
fprintf(stderr,"CargarVigil() - emp %d, cli %ld, obj %d, leg %ld, bon %d, cant %ld, pre %ld vigval %ld condic %s\n", 
		emp, cliente, objet, nroleg, bono, cant, precio, vigvalor, condic);
#endif
		
	for (aux=pvig; 
		 aux<fvig && !(aux->bono==bono && aux->nroleg==nroleg && aux->cli==cliente && aux->obj==objet); 
		 aux++)
		;

	if (aux<fvig) { // ya existe 
		// Se cargan en aux->cdiasT tanto los dias T como F para considerarlos trabajados y llegar
		// al total de 16 o 21 dias dependiendo el caso.
		if(strcmp(condic,"T")==0 || strcmp(condic,"F")==0) aux->cdiasT++;
		if(strcmp(condic,"L")==0)						   aux->cdiasL++;
		if(strcmp(condic,"A")==0)						   aux->cdiasA++;
		if(strcmp(condic,"F")==0)						   aux->cdiasF++;
		if(strcmp(condic,"V")==0)						   aux->cdiasV++;
		if(hor_nocturno)								   aux->diasnoc++;
	}
	else {	// no existe
		if (fvig == &pvig[MAXLEG])
			Error("Tabla interna de Vigiladores saturada. Max %d", MAXLEG);
		fvig->emp	    = emp;
		fvig->cli	    = cliente;
		fvig->obj	    = objet;
		fvig->bono      = bono;
		fvig->bvalor    = precio;
		fvig->vigvalor  = vigvalor;
		fvig->bcant     = cant;
		fvig->nroleg    = nroleg;
		fvig->cdiasT    = 0;
		fvig->cdiasL    = 0;
		fvig->cdiasA    = 0;
		fvig->cdiasF    = 0;
		fvig->cdiasV    = 0;
		fvig->diasnoc   = 0;
		if(strcmp(condic,"T")==0 || strcmp(condic,"F")==0) fvig->cdiasT  = 1;
		if(strcmp(condic,"L")==0)						   fvig->cdiasL  = 1;
		if(strcmp(condic,"A")==0)						   fvig->cdiasA  = 1;
		if(strcmp(condic,"F")==0)						   fvig->cdiasF  = 1;
		if(strcmp(condic,"V")==0)						   fvig->cdiasV  = 1;
		if(hor_nocturno)								   fvig->diasnoc = 1;
		fvig++;
	}
}

static void CalcularBono1()
{
	SVigil * aux;
	
	// No requiere ordenamiento previo, pasa x todos
	//OrdPorCliObjListaBonos();

	for (aux=pvig; aux<fvig ; aux++) {
        if(aux->bono!=BONO_1_NOC)
        	continue;

		//Cantidad de dias que debe trabajar para cobrar el Bono Nocturno.
		if (aux->diasnoc < 20)
			continue;

		aux->qbase    = aux->diasnoc * 100;
		aux->ibase    = 0;
		aux->pbase    = 0;
		aux->impfinal = aux->qbase * aux->vigvalor;
	}		
}

/*********************************************************************************************************/
/***
 Es un bono que se paga por cliente/objetivo/legajo
***/
static void CalcularBono2()
{
	SVigil * aux;
	
	// No requiere ordenamiento previo, pasa x todos
	//OrdPorCliObjListaBonos();

	for (aux=pvig; aux<fvig ; aux++) {
        if(aux->bono!=BONO_2)
        	continue;

		aux->qbase    = aux->cdiasT*100;
		aux->ibase    = aux->qbase  * aux->vigvalor /100;
		aux->pbase    = 100;
		aux->impfinal = aux->ibase * aux->pbase / 100;
	}		
}
/*********************************************************************************************************/
/***
 Es un bono que se paga por vigilador 
***/
static void CalcularBono3()
{
	SVigil * aux;
	SVigil * auxvig;
    long legajo_ant=0;
    bool primercambiolegajo=TRUE;
	int  cmaydiasT=0;	// indica la mayor cantidad de dias trabajados en un solo objetivo.
	long vmaydiasT=0;	// indica el valor del bono de mayor cantidad de dias trabajados en un solo objetivo.

	int  cdiasobj=0; 	// indica la cantidad de dias trabajados, considerando todos los objetivos.
	long vmaybono=0;	// indica el mayor valor de bono.

	int  cfaltas=0;    	// indica la cantidad de faltas, considerando todos los objetivos.
	int  clic=0;    	// indica la cantidad de dias de licencias, considerando todos los objetivos.
	int  ctardes=0;		// indica la cantidad de llegadas tardes, considerando todos los objetivos.
	int  pbase;

	// ordenar por bono, legajo, cliente, objetivo
	OrdPorLegajoListaBonos();

	auxvig=pvig; 
	for (aux=pvig; aux<fvig; aux++) {
	
        if(aux->bono!=BONO_3)
        	continue;
 
	   	if(aux->nroleg!=legajo_ant && !primercambiolegajo)	{ 	// marca el cambio de legajo

			// calcular y actualizar porcentajes
			for (; auxvig<aux; auxvig++) {
                
                if(auxvig->cdiasT==cmaydiasT && auxvig->vigvalor==vmaydiasT) {  

	                auxvig->qbase    = 0;
	                auxvig->ibase    = 0;
	                auxvig->pbase    = 0;
	                auxvig->impfinal = 0;
	                
	                // importe base 
	                if(cdiasobj >= MIN_DIAS_N_OBJ)  	  //regla 3, vale regla 2 sobre regla 1
					 	auxvig->ibase  = vmaydiasT;		  //regla 2
					else if(cmaydiasT >= MIN_DIAS_1_OBJ)
						auxvig->ibase  = vmaydiasT;		  //regla 1
					// 
					pbase=100; 

					if (cfaltas==1 || ctardes==2)   	  //regla 4. Si falta 1 dia o llega tarde 2 se paga el 50% del bono.
						pbase=50; 
					if (cfaltas>1 || ctardes>=3)    	  //regla 5. Si falta mas de 1 dia o llega tarde mas de 2 no se paga bono.
						pbase=0; 
						
					auxvig->qbase    = 100;
					auxvig->pbase    = pbase; 
					auxvig->impfinal = auxvig->ibase * auxvig->pbase / 100 ;
				}
			}
			cmaydiasT=0;
			vmaydiasT=0;
			cdiasobj=0;
			vmaybono=0;
			cfaltas=0;
			ctardes=0;
	   	}                            
		
		// el objetivo de mayor cantidad de dias trabajados y el valor del bono de ese objetivo.
		if (aux->cdiasT > cmaydiasT) {
			cmaydiasT = aux->cdiasT;
			vmaydiasT = aux->vigvalor;
		}
		else if(aux->cdiasT == cmaydiasT && aux->vigvalor > vmaydiasT) {   // desempate por el mayor valor de bono
			cmaydiasT = aux->cdiasT;
			vmaydiasT = aux->vigvalor;
		}
		
		// el mayor valor de bono. 		// Por ahora "no" se usa !!!
		if (aux->vigvalor > vmaybono) {
			vmaybono  = aux->vigvalor;
		}

		// cantidad de dias trabajados en todos los objetivos.
		cdiasobj += aux->cdiasT;

		// cantidad de faltas.
		cfaltas += aux->cdiasA;
		clic    += aux->cdiasL;

		// llegadas tardes
		ctardes  = 0;	  			// ver como obtener llegadas tardes

		primercambiolegajo=FALSE;
		legajo_ant = aux->nroleg;

	} // fin for principal

	// calcular y actualizar porcentajes
	for (; auxvig<aux; auxvig++) {
                
    	if(auxvig->cdiasT==cmaydiasT && auxvig->vigvalor==vmaydiasT) {  

			auxvig->qbase    = 0;
            auxvig->ibase    = 0;
            auxvig->pbase    = 0;
            auxvig->impfinal = 0;
               
            // importe base 
            if(cdiasobj >= MIN_DIAS_N_OBJ)  		// regla 3, vale regla 2 sobre regla 1
				auxvig->ibase  = vmaydiasT;			// regla 2
			else if(cmaydiasT >= MIN_DIAS_1_OBJ)  
				auxvig->ibase  = vmaydiasT;			// regla 1
			// 
			pbase=100; 
			if (cfaltas==1 || ctardes==2)   		// regla 4
				pbase=50; 
			if (cfaltas>1 || ctardes>=3)    		// regla 5
				pbase=0; 
			
			auxvig->qbase    = 100;
			auxvig->pbase    = pbase;
			auxvig->impfinal = auxvig->ibase * auxvig->pbase / 100;
		}
	}
}
/*********************************************************************************************************/
static bool EsFinalListaBonos()
{
	return (cvig==fvig? TRUE:FALSE);	
}
/*********************************************************************************************************/
static void CopiarVigil(const SVigil *a, SVigil *b)
{
	b->emp		= a->emp;
    b->cli		= a->cli;
	b->obj		= a->obj;
    b->bono		= a->bono;
    b->bvalor	= a->bvalor;
    b->vigvalor	= a->vigvalor;
    b->bcant	= a->bcant;
	b->nroleg	= a->nroleg;
	b->cdiasT   = a->cdiasT;
	b->cdiasL   = a->cdiasL;
	b->cdiasA   = a->cdiasA;
	b->cdiasF   = a->cdiasF;
	b->cdiasV   = a->cdiasV;
	b->diasnoc  = a->diasnoc;
	//
	b->qbase  	= a->qbase;
	b->ibase  	= a->ibase;
	b->pbase	= a->pbase;
	b->impfinal	= a->impfinal;
}
/*********************************************************************************************************/
// chequea si tiene algun bono 
static bool TieneBonos(long cliente, int objetivo, int bonod, int bonoh)
{
	schema comerc, act;
	dbcursor cur;
	bool tiene=FALSE;
	act = CurrentSchema();
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(act);
	cur=CreateCursor(comerc|LISTABONbyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(cur, cliente, objetivo, bonod, MIN_DATE);
	SetCursorTo(  cur, cliente, objetivo, bonoh, MAX_DATE);
	if(FetchCursor(cur)!=ERROR)	
		tiene=TRUE;
	DeleteCursor(cur);
	return tiene;
}
/*********************************************************************************************************/
static bool ObjetivoActivo(long cliente, int objetivo, DATE fechad)
{
	schema comerc, act;
	act = CurrentSchema();
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(act);
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if(GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK)==ERROR)
		return FALSE;
	if ( !IFld(comerc|OBJETIVO_ACTIVO) && !IsNull(comerc|OBJETIVO_FECHAF) && DFld(comerc|OBJETIVO_FECHAF)<fechad )
		return FALSE;
	return TRUE;
}
/*********************************************************************************************************/
/* Recordar que un Nodo es un legajo/cliente/objetivo
*/
static long CantNodosListaBonos()
{
	SVigil * aux;
	int i=0;
	for (aux=pvig; aux<fvig; aux++)
		i++;
	return i-1;	// el primero no cuenta: la primera posicion de la lista no se usa, esto es por la implementacion de Proximo
}
/*********************************************************************************************************/
