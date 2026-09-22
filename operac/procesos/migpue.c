/********************************************************************
*
* MODULE & VERSION : @(#)migpue.c	1.3
* DATE             : 11/06/14
* TIME             : 17:24:26
*
* CREATED          : 06/06/2011
*
* DESCRIPTION:
*      Migrador de Codint para puestos: toma codint de puesto de operac.puestos y lo graba en comerc.npuesto
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "bill.h"
#include "operac.h"
#include "comerc.h"
#include "migpue.fmh"
#include "aurus.sch"
#include "comerc.sch"
#include "operac.sch"
#include "billpro.sch"

#define _TIPOLIS_ALTA	'A'
#define _TIPOLIS_BAJA	'B'
#define _DEBUG
#define _NO_TIENE_OT 	99


/* Funciones privadas */
static void Lectura(fm_cmd, find_mode);

/* Declaraciones globales */
form fm0;
schema comerc, bill, operac, ctascob, sue, prosegur, old;
FILE *archivo = (FILE *)NULL;
DATE fecini, fecfin, fecfinOT;
short tipser;
bool  error_ot=FALSE;

typedef struct s_npuestos {
	int  emp;
	int  tipcomp;
	int  serie;
	char deleg[3];
	long nroot;
	int  tippto;
	int  nroreng;
	DATE finicio;
	DATE ffinal;
	long cliente;
	int  objet;
	int  codint;
    bool actualiza;
	int puesto;
	char regim[9];
	TIME hinicio;
	TIME hfinal;
	char diaL[2];
	char diaM[2];
	char diaX[2];
	char diaJ[2];
	char diaV[2];
	char diaS[2];
	char diaD[2];
	char codfrec[3];
	char tipodia[2];
	char condic;
	struct  s_npuestos *sig;
	struct  s_npuestos *ant;
} s_npuestos;

s_npuestos * p_npuestosA = NULL; //para lista de Altas
s_npuestos * p_npuestosB = NULL; //para lista de Altas

void CrearListaNPuestosVacia(s_npuestos **pnpuestos);
void FinListaNPuestos(s_npuestos **pnpuestos);
void GuardarListaNPuestos(s_npuestos **pnpuestos, int emp, int tipcomp, int serie, char deleg[3], long nroot, int nroreng, DATE finicio, DATE ffinal, long cliente, int objet, int tippto, 
							int puesto, char regim[9], TIME hinicio, TIME hfinal, char diaL[2], 
							char diaM[2], char diaX[2], char diaJ[2], char diaV[2], 
							char diaS[2], char diaD[2], char codfrec[3], char tipodia[2], char condic);
void LeerNPuestos();
bool LeerValidarPuestos(s_npuestos **pnpuestos, char aob);
void GuardarLogPuestos(s_npuestos **pnpuestos, char aob);
void VerListaNPuestos(s_npuestos **pnpuestos);
private bool OtEstaAprobada();

/* Programa principal */
wcmd(migpue, 1.3 06/14/11)
{
	fm_cmd cmd;
	fm0 = OpenForm("migpue", FM_EABORT);

	bill    = OpenSchema("bill",   IO_EABORT);
	operac  = OpenSchema("operac",   IO_EABORT);
	comerc  = OpenSchema("comerc",   IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:      Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT: Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV: Lectura(cmd, PREV_KEY); break;
	case FM_UPDATE:
		BeginTransaction();
		CrearListaNPuestosVacia(&p_npuestosA);
		CrearListaNPuestosVacia(&p_npuestosB);

		LeerNPuestos();

		#ifdef _DEBUG
		fprintf(stderr, "ANTES\n");
		VerListaNPuestos(&p_npuestosA);
		VerListaNPuestos(&p_npuestosB);
        #endif
        
		LeerValidarPuestos(&p_npuestosA, _TIPOLIS_ALTA);
		LeerValidarPuestos(&p_npuestosB, _TIPOLIS_BAJA);
		
 		#ifdef _DEBUG 
		fprintf(stderr, "DESPUES\n");
        VerListaNPuestos(&p_npuestosA);
		VerListaNPuestos(&p_npuestosB);
        #endif

	  	GuardarLogPuestos(&p_npuestosA, _TIPOLIS_ALTA);
        GuardarLogPuestos(&p_npuestosB, _TIPOLIS_BAJA);

		FinListaNPuestos(&p_npuestosA);
		FinListaNPuestos(&p_npuestosB);

 		FmSetFld(fm0, COMENT, NULL_STR);
 		WiRefresh();

		EndTransaction();
		break;
	case FM_DELETE:
		break;
	case FM_IGNORE:
		FreeTable(comerc|OT);
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd p_cmd, find_mode mode)
{
	FmToDb(fm0, 0, OBJET);

	switch(GetRecord(comerc|OBJETIVObyCLIENTE, mode, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(comerc|OBJETIVObyCLIENTE, mode);
		DbToFm(fm0, 0, OBJET);
		FmShowFlds(fm0, 0, OBJET);
		return;
	case ERROR:
		FmSetStatus(fm0, p_cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
}



/* verifica que la lista no exista y si es asi manda a vaciarla */
void CrearListaNPuestosVacia(s_npuestos **pnpuestos)
{
	if ((*pnpuestos) != NULL)
		FinListaNPuestos(pnpuestos);
}

/* vacia la lista */
void FinListaNPuestos(s_npuestos **pnpuestos)
{
	s_npuestos *np, *npaux;

	for (np = (*pnpuestos); np != NULL; ) {
		npaux = np;
		np = np->sig;
		free(npaux);
	}

	(*pnpuestos) = NULL;
}

/* agrega un nodo a la lista */
void GuardarListaNPuestos(s_npuestos **pnpuestos, int emp, int tipcomp, int serie, char deleg[3],
							long nroot, int nroreng, DATE finicio, DATE ffinal, long cliente, int objet, int tippto, 
							int puesto, char regim[9], TIME hinicio, TIME hfinal, char diaL[2], 
							char diaM[2], char diaX[2], char diaJ[2], char diaV[2], 
							char diaS[2], char diaD[2], char codfrec[3], char tipodia[2], char condic)

{
	s_npuestos *pnew;

	pnew = (s_npuestos *)Alloc(sizeof(s_npuestos));

	pnew->emp = emp;
	pnew->tipcomp = tipcomp;
	pnew->serie = serie;
	strncpy(pnew->deleg, deleg, 3);
	pnew->nroot = nroot;
	pnew->nroreng = nroreng;
	pnew->finicio = finicio;
	pnew->ffinal = ffinal;
	pnew->cliente = cliente;
	pnew->objet = objet;
	pnew->tippto = tippto;
	pnew->puesto = puesto;
	strncpy(pnew->regim, regim, 8);
	pnew->hinicio = hinicio;
	pnew->hfinal = hfinal;
	strncpy(pnew->diaL, diaL, 1);
	strncpy(pnew->diaM, diaM, 1);
	strncpy(pnew->diaX, diaX, 1);
	strncpy(pnew->diaJ, diaJ, 1);
	strncpy(pnew->diaV, diaV, 1);
	strncpy(pnew->diaS, diaS, 1);
	strncpy(pnew->diaD, diaD, 1);
	strncpy(pnew->codfrec, codfrec, 2);
	strncpy(pnew->tipodia, tipodia, 1);
	pnew->condic = condic;
	pnew->sig = NULL;
	pnew->codint = NULL_SHORT; //guarda el codigo interno de registro para encontrarlo mas facil al borrar
	pnew->actualiza = FALSE;


	if (error_ot) { // Si no encontro OT le pone codint 99 para identificarla 
	   	pnew->codint = _NO_TIENE_OT;
		pnew->actualiza = TRUE;
		
	}
	
	//primer nodo de la lista
	if ((*pnpuestos) == NULL ) {
		pnew->ant = NULL;
		(*pnpuestos) = pnew;
	}
	else {
		pnew->ant = (*pnpuestos);
		(*pnpuestos)->sig = pnew;

		(*pnpuestos) = pnew;
	}	
}

/* lee la tabla comerc.npuesto y crea las listas con los datos a verificar 
	contra operac.puestos */
void LeerNPuestos()
{
	int 	cant=0,
			partial=0,
			modo=NEXT_KEY;
	char 	auxi[150];
	


	if (FmIsNull(fm0, EMP)) {
		SetKey(comerc|NPUESTObyCLIENTE, NULL_SHORT, NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		partial=0,
		modo=NEXT_KEY;
	}
	else {
		if (FmIsNull(fm0, CLIE)) {
			SetKey(comerc|NPUESTObyCLIENTE, FmIFld(fm0, EMP), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			partial=1,
			modo=NEXT_KEY|PARTIAL_KEY;
			
		}
		else {
			if (FmIsNull(fm0, OBJET)) {
				SetKey(comerc|NPUESTObyCLIENTE, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), NULL_SHORT, NULL_SHORT, NULL_SHORT);
				partial=2,
				modo=NEXT_KEY|PARTIAL_KEY;

				
			}
			else {
				SetKey(comerc|NPUESTObyCLIENTE, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT, NULL_SHORT);
				partial=3,
				modo=NEXT_KEY|PARTIAL_KEY;
				
			}
		}
	}
	while (GetRecord(comerc|NPUESTObyCLIENTE, modo, IO_NOT_LOCK, partial) != ERROR) {
	 	if(IFld(comerc|NPUESTO_CODINT) != NULL_SHORT)
	 		continue; 

	 	if (((cant++)%100)==0) {
			sprintf(auxi, "Lee Cliente %ld Objetivo %d", LFld(comerc|NPUESTO_CLIENTE), IFld(comerc|NPUESTO_OBJET));
	 		FmSetFld(fm0, COMENT, auxi);
	 		WiRefresh();
	 	}

		error_ot=FALSE;
		SetKey(comerc|OTbyEMP, IFld(comerc|NPUESTO_EMP), IFld(comerc|NPUESTO_TIPCOMP), IFld(comerc|NPUESTO_SERIE), SFld(comerc|NPUESTO_DELEG), LFld(comerc|NPUESTO_NROOT));
		if (GetRecord(comerc|OTbyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR) {
			error_ot=TRUE;		
		}
		else
			if(!OtEstaAprobada())
				continue;


	 	//Armo lista con Altas las Bajas no cuentan 
		if (StrCmp(SFld(comerc|NPUESTO_COND), _ALTA_OT) == 0) {

			GuardarListaNPuestos(&p_npuestosA,
									IFld(comerc|NPUESTO_EMP),
									IFld(comerc|NPUESTO_TIPCOMP),
									IFld(comerc|NPUESTO_SERIE),
									SFld(comerc|NPUESTO_DELEG),
									LFld(comerc|NPUESTO_NROOT),
									IFld(comerc|NPUESTO_NRORENG),
									DFld(comerc|OT_FINICIO),
									DFld(comerc|OT_FFINAL),
									LFld(comerc|NPUESTO_CLIENTE),
									IFld(comerc|NPUESTO_OBJET), 
									IFld(comerc|NPUESTO_TIPPTO),
									IFld(comerc|NPUESTO_PUESTO),
									SFld(comerc|NPUESTO_REGIM),
									TFld(comerc|NPUESTO_HINICIO),
									TFld(comerc|NPUESTO_HFINAL),
									SFld(comerc|NPUESTO_DIAS, 0),
									SFld(comerc|NPUESTO_DIAS, 1),
									SFld(comerc|NPUESTO_DIAS, 2),
									SFld(comerc|NPUESTO_DIAS, 3),
									SFld(comerc|NPUESTO_DIAS, 4),
									SFld(comerc|NPUESTO_DIAS, 5),
									SFld(comerc|NPUESTO_DIAS, 6),
									SFld(comerc|NPUESTO_CODFREC),
									SFld(comerc|NPUESTO_TIPODIA),
									*SFld(comerc|NPUESTO_COND));
		}
		
		//Armo lista con Bajas
		if (StrCmp(SFld(comerc|NPUESTO_COND), _BAJA_OT) == 0) {
			GuardarListaNPuestos(&p_npuestosB,
									IFld(comerc|NPUESTO_EMP),
									IFld(comerc|NPUESTO_TIPCOMP),
									IFld(comerc|NPUESTO_SERIE),
									SFld(comerc|NPUESTO_DELEG),
									LFld(comerc|NPUESTO_NROOT),
									IFld(comerc|NPUESTO_NRORENG),
									NULL_DATE,
									DFld(comerc|OT_FINICIO),
									LFld(comerc|NPUESTO_CLIENTE),
									IFld(comerc|NPUESTO_OBJET), 
									IFld(comerc|NPUESTO_TIPPTO),
									IFld(comerc|NPUESTO_PUESTO),
									SFld(comerc|NPUESTO_REGIM),
									TFld(comerc|NPUESTO_HINICIO),
									TFld(comerc|NPUESTO_HFINAL),
									SFld(comerc|NPUESTO_DIAS, 0),
									SFld(comerc|NPUESTO_DIAS, 1),
									SFld(comerc|NPUESTO_DIAS, 2),
									SFld(comerc|NPUESTO_DIAS, 3),
									SFld(comerc|NPUESTO_DIAS, 4),
									SFld(comerc|NPUESTO_DIAS, 5),
									SFld(comerc|NPUESTO_DIAS, 6),
									SFld(comerc|NPUESTO_CODFREC),
									SFld(comerc|NPUESTO_TIPODIA),
									*SFld(comerc|NPUESTO_COND));
		}
	
	}
}

/* recorrer lista para ver como queda armada la lista de puestos */
void VerListaNPuestos(s_npuestos **pnpuestos)
{
	s_npuestos * aux;

	aux = (*pnpuestos); 

	if (aux == NULL)
		return;	

	do	{
		fprintf(stderr, "emp %d tipcomp %d serie %d deleg %s nroot %ld nroreng %d finicio %.3D ffinal %.3D cliente %ld - objet %d - tippto %d - puesto %d - regim %s - hinicio %T - hfinal %T - diaL %s - diaM %s - diaX %s - diaJ %s - diaV %s - diaS %s - diaD %s - codfrec %s - tipodia %s -  codint %d - actualiza %B - cond %c\n", 
						aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng, aux->finicio, aux->ffinal, aux->cliente, aux->objet, aux->tippto, aux->puesto, aux->regim, aux->hinicio, aux->hfinal, aux->diaL, aux->diaM, aux->diaX, aux->diaJ, aux->diaV, aux->diaS, aux->diaD, aux->codfrec, aux->tipodia, aux->codint, aux->actualiza, aux->condic);
		aux = aux->ant;
	}	while ( aux != NULL );
}

/* lee la tabla operac.puestos validando con los datos de la lista, de ser iguales
	son marcados en la lista para ser borrados */
bool LeerValidarPuestos(s_npuestos **pnpuestos, char aob)
{
	bool encontro, cab = FALSE;
	//int capuxnpuA = 0, capuxnpuB = 0, capuxnpuAB = 0 ; // cantidad de puestos x npuesto si es mayor a uno no se actualiza 
	               
    //error se usa si encuentra mas de un puesto para un npuesto en ese caso no se actualiza npuesto
                   
	s_npuestos * aux;

	aux = (*pnpuestos);

	if (aux == NULL)
	 	return TRUE;	

	do	{
		
		encontro = FALSE;
		//capuxnpuA = 0;
		//capuxnpuB = 0;
		//capuxnpuAB = 0;

		//esto es por si hay un error en la OT, atualiza solo npuestos con codint 99
        if (aux->actualiza==TRUE) {
			aux = aux->ant;
        	continue;
        }

//		fprintf(stderr, "EMP %d tipcomp %d serie %d deleg %s nroot %ld nroreng %d finicio %.3D ffinal %.3D cliente %ld - objet %d - tippto %d - puesto %d - regim %s - hinicio %T - hfinal %T - diaL %s - diaM %s - diaX %s - diaJ %s - diaV %s - diaS %s - diaD %s - codfrec %s - tipodia %s -  codint %d - actualiza %B - cond %c\n", 
//						aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng, aux->finicio, aux->ffinal, aux->cliente, aux->objet, aux->tippto, aux->puesto, aux->regim, aux->hinicio, aux->hfinal, aux->diaL, aux->diaM, aux->diaX, aux->diaJ, aux->diaV, aux->diaS, aux->diaD, aux->codfrec, aux->tipodia, aux->codint, aux->actualiza, aux->condic);

		// es un while porque el indice no es unico 
		SetKey(operac|PUESTOSbyCLIENTE, aux->cliente, aux->objet, aux->tippto, NULL_SHORT);
		while(GetRecord(operac|PUESTOSbyCLIENTE, PARTIAL_KEY|NEXT_KEY, IO_NOT_LOCK, 3) != ERROR)	{


			if (aux->puesto != IFld(operac|PUESTOS_PUESTO))	continue;
			if (aux->hinicio != TFld(operac|PUESTOS_HINICIO))	continue;
			if (aux->hfinal != TFld(operac|PUESTOS_HFINAL))	continue;
			if (StrCmp(aux->diaL, SFld(operac|PUESTOS_DIA1)) != 0)	continue;
			if (StrCmp(aux->diaM, SFld(operac|PUESTOS_DIA2)) != 0)	continue;
			if (StrCmp(aux->diaX, SFld(operac|PUESTOS_DIA3)) != 0)	continue;
			if (StrCmp(aux->diaJ, SFld(operac|PUESTOS_DIA4)) != 0)	continue;
			if (StrCmp(aux->diaV, SFld(operac|PUESTOS_DIA5)) != 0)	continue;
			if (StrCmp(aux->diaS, SFld(operac|PUESTOS_DIA6)) != 0)	continue;
			if (StrCmp(aux->diaD, SFld(operac|PUESTOS_DIA7)) != 0)	continue;
			if (StrCmp(aux->regim, SFld(operac|PUESTOS_REGIM)) != 0)	continue;
			if (StrCmp(aux->codfrec, SFld(operac|PUESTOS_CODFREC)) != 0)	continue;
			if (StrCmp(aux->tipodia, SFld(operac|PUESTOS_TIPODIA)) != 0)	continue;


		    if(IFld(operac|PUESTOS_NEWINT) == 1 && aob == _TIPOLIS_ALTA)
		    	continue;


		    if(IFld(operac|PUESTOS_PADREINT) == 1 && aob == _TIPOLIS_BAJA)
		    	continue;


		    //fprintf(stderr, "%.3T %.3T %.3T %.3T\n", aux->hinicio, aux->hfinal, TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL));
		    //fprintf(stderr, "%s %s\n", aux->regim, SFld(operac|PUESTOS_REGIM));
		    //fprintf(stderr, "-%.3D - %.3D -\n", aux->finicio, DFld(operac|PUESTOS_FINICIO));
		    //fprintf(stderr, "-%.3D - %.3D o %.3D -\n", aux->ffinal, DFld(operac|PUESTOS_FFINAL), (DFld(operac|PUESTOS_FFINAL)-1));
		    
			switch (aob) {
				case _TIPOLIS_ALTA:
//				    fprintf(stderr, "inicio NPUESTO %.3D - Inicio PUESTOS%.3D -\n", aux->finicio, DFld(operac|PUESTOS_FINICIO));
		            if (aux->finicio == DFld(operac|PUESTOS_FINICIO)) {
						if (aux->ffinal==DFld(operac|PUESTOS_FFINAL) || aux->ffinal==NULL_DATE ){//alta con fecha de inicio pero ffinal nula
						   	aux->codint = IFld(operac|PUESTOS_CODINT); //guardo codint para usar despues la PK
							aux->actualiza = TRUE;
							encontro = TRUE;
							SetIFld(operac|PUESTOS_NEWINT, 1);
							PutRecord(operac|PUESTOS);
							//capuxnpuA++; 
		            	}
		            }
					break;
				case _TIPOLIS_BAJA:
					if (DFld(operac|PUESTOS_FFINAL) == aux->ffinal || DFld(operac|PUESTOS_FFINAL) == (aux->ffinal-1))	{
						aux->codint = IFld(operac|PUESTOS_CODINT); //guardo codint para usar despues la PK
						aux->actualiza = TRUE;
						encontro = TRUE;
						//capuxnpuB++; 
						SetIFld(operac|PUESTOS_PADREINT, 1);
						PutRecord(operac|PUESTOS);

					}
					break;
			}
			
			if (encontro)
				break;
			
		}
	           
        // si no encuentra un puesto o encuentra mas de uno no debe actualizar dicho npuesto
        if (encontro == FALSE)	{   // || capuxnpuA > 1 || capuxnpuB > 1 || capuxnpuAB > 1) {
        	
        	if (cab == FALSE )	{
	        	fprintf(stderr, "ERROR\temp\ttipcomp\tserie\tdeleg\tnroot\tnroreng\tfinicio\tffinal\tcliente\tobjet\ttippto\tpuesto\tregim\thinicio\thfinal\tdiaL\tdiaM\tdiaX\tdiaJ\tdiaV\tdiaS\tdiaD\tcodfrec\ttipodia\tcodint\tactualiza\tcondic\n"); 
        		cab = TRUE;
        	}	
        	
        	aux->codint = 0;
        	aux->actualiza = TRUE;

        	fprintf(stderr, "%s\t%d\t%d\t%d\t%s\t%ld\t%d\t%.3D\t%.3D\t%ld\t%d\t%d\t%d\t%s\t%T\t%T\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%B\t%c\n", 
					"ERROR", aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng, aux->finicio, aux->ffinal, aux->cliente, aux->objet, aux->tippto, aux->puesto, aux->regim, aux->hinicio, aux->hfinal, aux->diaL, aux->diaM, aux->diaX, aux->diaJ, aux->diaV, aux->diaS, aux->diaD, aux->codfrec, aux->tipodia, aux->codint, aux->actualiza, aux->condic);

        }
        
		aux = aux->ant;
	}	while ( aux != NULL );
	
	return TRUE;
}

/* guarda en un archivo los registros de la tabla operac.puesto que seran borrados 
	y luego los borra */
void GuardarLogPuestos(s_npuestos **pnpuestos, char aob)
{
	s_npuestos * aux;
	int cant=0 ;
	char 	auxi[150];

	char nomarch[50] = {'\0'};
	bool archOK = FALSE, impcab = FALSE;

	//nombre del archivo donde se exportan los registros a ser borrados
	sprintf(nomarch, "%s/logs/comerc.npuesto.actualizado.log", ReadEnv("COMERC"));

	aux = (*pnpuestos);

	if (aux == NULL)
		return;

	do	{
 		if (archOK == FALSE)	{
 			if ((archivo = fopen(nomarch, "a+")) == (FILE*)NULL)
 				Error("No se pudo generar el archivo %s", nomarch);
 			archOK = TRUE;
 		}

 		if (impcab == FALSE)	{
 			impcab = TRUE;
 			fprintf(archivo, "Usuario %d %s - Fecha %D - Hora %T \n", GetUid(), getenv("USER"), Today(), Hour());
		}

	 	if (((cant++)%100)==0) {
			sprintf(auxi, "Escribe Cliente %ld Objetivo %d", aux->cliente, aux->objet);
	 		FmSetFld(fm0, COMENT, auxi);
	 		WiRefresh();
	 	}

		if (aux->actualiza == TRUE) {
        
			//seteo clave
			SetIFld(comerc|NPUESTO_EMP, aux->emp);
			SetIFld(comerc|NPUESTO_TIPCOMP, aux->tipcomp);
			SetIFld(comerc|NPUESTO_SERIE, aux->serie);
			SetFld (comerc|NPUESTO_DELEG, aux->deleg);
			SetLFld(comerc|NPUESTO_NROOT, aux->nroot);
			SetIFld(comerc|NPUESTO_TIPPTO, aux->tippto);
			SetIFld(comerc|NPUESTO_NRORENG, aux->nroreng);
            (void)(GetRecord(comerc|NPUESTObyEMP, THIS_KEY, IO_NOT_LOCK));
            
            //seteo actualizacion
            SetIFld(comerc|NPUESTO_CODINT, aux->codint);

            //actualizo
            PutRecord(comerc|NPUESTO);
        }
		
		
		
		aux = aux->ant;
	}	while ( aux != NULL );

	if (archOK == TRUE)
		fclose(archivo);
}
private bool OtEstaAprobada()
{
	 return	(IFld(comerc|OT_ESTVTA) && IFld(comerc|OT_ESTOPER) && IFld(comerc|OT_ESTADM) );
}

