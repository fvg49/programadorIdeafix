/********************************************************************
*
* MODULE & VERSION : @(#)cpcodint.c	1.3
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
#include "operac.h"
#include "comerc.h"
#include "cpcodint.fmh"
#include "aurus.sch"
#include "comerc.sch"
#include "operac.sch"

#define _TIPOLIS_ALTA	'A'
#define _TIPOLIS_BAJA	'B'
//#define _DEBUG
#define _NO_TIENE_OT 	99


/* Declaraciones globales */
form fm0;
schema comerc, operac, ctascob, sue, prosegur, old;
FILE *archivo = (FILE *)NULL;
DATE fecini, fecfin, fecfinOT;
short tipser;
bool  error_ot=FALSE;

typedef struct s_crosspue {
	int	 emp;
	int  tipcomp;
	int  serie;
	char deleg[2];
	long nroot;
	int  tippto;
	int  nroreng;
	long cliente;
	int  objet;
	int  codint;
	int  newcodint;
	struct  s_crosspue *sig;
	struct  s_crosspue *ant;
} s_crosspue;

s_crosspue * inicio = NULL; //para lista de Altas

void CrearListaNCrossoVacia(s_crosspue **pnpuestos);
void FinListaNCross(s_crosspue **pnpuestos);
void GuardarListaNCross(s_crosspue **pnpuestos, int emp, int tipcomp, int serie, char deleg[3], long nroot, int tippto, int nroreng, long cliente, int objet, int codint, int newcodint);
void ActSegunCross(s_crosspue **pnpuestos, dbtable tabla, dbindex indice, dbfield campo);
void ActTabla(s_crosspue **pnpuestos, dbtable tabla, dbindex indice, dbfield campo, long nroleg, DATE fdesde, DATE fhasta);

void VerCross(s_crosspue **pnpuestos);
private bool OtEstaAprobada();

char auxi[150];
long totalcasos=0; 
/* Programa principal */
wcmd(cpcodint, 1.3 06/14/11)
{
	fm_cmd cmd;
	fm0 = OpenForm("cpcodint", FM_EABORT);
	dbcursor cnpuesto;
	dbcursor cpuestos;
	int cant=0;
	char sarch[30];


	operac  = OpenSchema("operac",   IO_EABORT);
	comerc  = OpenSchema("comerc",   IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();
		cant=0;
		totalcasos=0; 
		CrearListaNCrossoVacia(&inicio);

		cnpuesto = CreateCursor(comerc|NPUESTObyCLIENTE, IO_NOT_LOCK);
		cpuestos = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);

        // RECORRE NPUESTO
		SetCursorFrom(cnpuesto, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT, NULL_SHORT);
		if (FmIsNull(fm0, EMP))
			SetCursorTo(cnpuesto, MAX_SHORT, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		else {
			if (FmIsNull(fm0, CLIE)) 
				SetCursorTo(cnpuesto, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			else {
				if (FmIsNull(fm0, OBJET)) 
					SetCursorTo(cnpuesto, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), MAX_SHORT, MAX_SHORT, MAX_SHORT);
				else 
					SetCursorTo(cnpuesto, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MAX_SHORT, MAX_SHORT);
			}
		}
		while (FetchCursor(cnpuesto) != ERROR) {

			//Solo Lee Altas
		    if(*SFld(comerc|NPUESTO_COND) != _TIPOLIS_ALTA)
		    	continue;

			//Solo Ots Aprobadas
			error_ot=FALSE;
			SetKey(comerc|OTbyEMP, IFld(comerc|NPUESTO_EMP), IFld(comerc|NPUESTO_TIPCOMP), IFld(comerc|NPUESTO_SERIE), SFld(comerc|NPUESTO_DELEG), LFld(comerc|NPUESTO_NROOT));
			if (GetRecord(comerc|OTbyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR) {
				SetIFld(comerc|NPUESTO_HORAPT, 1);
				PutRecord(comerc|NPUESTO);
				error_ot=TRUE;		
			}
			else
				if(!OtEstaAprobada())
					continue;

  		 	if (((cant++)%100)==0) {  
				sprintf(auxi, "Lee Cliente %ld Objetivo %d", LFld(comerc|NPUESTO_CLIENTE), IFld(comerc|NPUESTO_OBJET));
		 		FmSetFld(fm0, COMENT, auxi);
		 		WiRefresh();
		 	}

	        // RECORRE PUESTOS 
			SetCursorFrom(cpuestos, LFld(comerc|NPUESTO_CLIENTE), IFld(comerc|NPUESTO_OBJET), IFld(comerc|NPUESTO_TIPPTO), NULL_SHORT);
			SetCursorTo  (cpuestos, LFld(comerc|NPUESTO_CLIENTE), IFld(comerc|NPUESTO_OBJET), IFld(comerc|NPUESTO_TIPPTO), MAX_SHORT);
			while (FetchCursor(cpuestos) != ERROR) {
				
				// No vuelvo a leer puestos que ya use 
			    if(IFld(operac|PUESTOS_NEWINT) == 1) continue;
				
				// Verifico que coincida el puesto
				if (IFld(comerc|NPUESTO_PUESTO) != IFld(operac|PUESTOS_PUESTO))	continue;
				if (TFld(comerc|NPUESTO_HINICIO) != TFld(operac|PUESTOS_HINICIO))	continue;
				if (TFld(comerc|NPUESTO_HFINAL) != TFld(operac|PUESTOS_HFINAL))	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 0), SFld(operac|PUESTOS_DIA1)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 1), SFld(operac|PUESTOS_DIA2)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 2), SFld(operac|PUESTOS_DIA3)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 3), SFld(operac|PUESTOS_DIA4)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 4), SFld(operac|PUESTOS_DIA5)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 5), SFld(operac|PUESTOS_DIA6)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_DIAS, 6), SFld(operac|PUESTOS_DIA7)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_REGIM), SFld(operac|PUESTOS_REGIM)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_CODFREC), SFld(operac|PUESTOS_CODFREC)) != 0)	continue;
				if (StrCmp(SFld(comerc|NPUESTO_TIPODIA), SFld(operac|PUESTOS_TIPODIA)) != 0)	continue;
			    if (DFld(comerc|OT_FINICIO) != DFld(operac|PUESTOS_FINICIO)) continue;
				if (DFld(comerc|OT_FFINAL) != NULL_DATE && DFld(comerc|OT_FFINAL)!=DFld(operac|PUESTOS_FFINAL)) continue;

   
					GuardarListaNCross(&inicio, IFld(comerc|NPUESTO_EMP), IFld(comerc|NPUESTO_TIPCOMP), IFld(comerc|NPUESTO_SERIE), SFld(comerc|NPUESTO_DELEG), 
									   LFld(comerc|NPUESTO_NROOT), IFld(comerc|NPUESTO_TIPPTO), IFld(comerc|NPUESTO_NRORENG), LFld(comerc|NPUESTO_CLIENTE), 
									   IFld(comerc|NPUESTO_OBJET), IFld(operac|PUESTOS_CODINT), IFld(comerc|NPUESTO_CODINT) );

   					totalcasos++; 

					SetIFld(operac|PUESTOS_NEWINT, 1);
					PutRecord(operac|PUESTOS);

			} 
	
		} 
		DeleteCursor(cnpuesto);
		DeleteCursor(cpuestos);
		
		
		sprintf(sarch, "cpcodint.%D-%T.txt", Today(), Hour());
		archivo=fopen(sarch, "w");

		VerCross(&inicio);

		ActSegunCross(&inicio, operac|PUESTOS,   operac|PUESTOSbyCLIENTE, operac|PUESTOS_CODINT);
		ActSegunCross(&inicio, operac|ASIG,      operac|ASIGbyPUESTO,     operac|ASIG_PUESTO);
		ActSegunCross(&inicio, operac|ASIGH,     operac|ASIGHbyEMP,       operac|ASIGH_PUESTO);


		fclose(archivo);

		EndTransaction();

 		FmSetFld(fm0, COMENT, NULL_STR);
 		WiRefresh();

		break;
	case FM_DELETE:
		break;
	case FM_IGNORE:
		FreeTable(comerc|OT);
		break;
	}
}


/* verifica que la lista no exista y si es asi manda a vaciarla */
void CrearListaNCrossoVacia(s_crosspue **pnpuestos)
{
	if ((*pnpuestos) != NULL)
		FinListaNCross(pnpuestos);
}

/* vacia la lista */
void FinListaNCross(s_crosspue **pnpuestos)
{
	s_crosspue *np, *npaux;

	for (np = (*pnpuestos); np != NULL; ) {
		npaux = np;
		np = np->sig;
		free(npaux);
	}

	(*pnpuestos) = NULL;
}

/* agrega un nodo a la lista */

void GuardarListaNCross(s_crosspue **pnpuestos, int emp, int tipcomp, int serie, char deleg[3], 
                        long nroot, int tippto, int nroreng, long cliente, int objet, int codint, int newcodint)

{
	s_crosspue *pnew;

	pnew = (s_crosspue *)Alloc(sizeof(s_crosspue));

	pnew->emp = emp;
	pnew->tipcomp = tipcomp;
	pnew->serie = serie;
	strncpy(pnew->deleg, deleg, 3);
	pnew->nroot = nroot;
	pnew->nroreng = nroreng;
	pnew->cliente = cliente;
	pnew->objet = objet;
	pnew->tippto = tippto;
	pnew->codint = codint; 
	pnew->newcodint = newcodint; 


	
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


/* recorrer lista para ver como queda armada la lista de puestos */
void VerCross(s_crosspue **pnpuestos)
{
	s_crosspue * aux;

	aux = (*pnpuestos); 

	if (aux == NULL)
		return;	

	fprintf(archivo, "emp\ttipcomp\tserie\tdeleg\tnroot\tnroreng\tcliente\tobjet\ttippto\tcodint\tcodint Marte\n"); 

	do	{
		fprintf(archivo, "%d\t%d\t%d\t%s\t%ld\t%d\t%ld\t%d\t%d\t%d\t%d\n", 
						aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng,aux->cliente, aux->objet, aux->tippto, aux->codint, aux->newcodint);
		aux = aux->ant;
	}	while ( aux != NULL );
}

/* lee la tabla operac.puestos validando con los datos de la lista, de ser iguales
	son marcados en la lista para ser borrados */

private bool OtEstaAprobada()
{
	 return	(IFld(comerc|OT_ESTVTA) && IFld(comerc|OT_ESTOPER) && IFld(comerc|OT_ESTADM) );
}

void ActSegunCross(s_crosspue **pnpuestos, dbtable tabla, dbindex indice, dbfield campo)
{

	s_crosspue * aux;
	dbcursor ctabla;
	char stabla[15], svalor[5];
	long caso=0;
	long v_nroleg=0;
	DATE v_diad=0, v_diah=0;

// PUESTOS   primary key(cliente, objet, tippto, codint)
// ASIG      index puesto(emp, cliente, objetivo, ptoser, puesto, nroint, nroleg)
// ASIGH     (emp, cliente, objetivo, ptoser, puesto, nroint, nroleg, fecbaj, fecalt),
// PARTE     leg(emp, cliente, objetivo, nroleg, dia)
// EXCEPCION primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo),

	ctabla = CreateCursor(indice, IO_NOT_LOCK);

	aux = (*pnpuestos); 

	sprintf(stabla,"%s",NULL_STR);

	if (aux == NULL)
		return;	
	caso=0;

	do	{

		if (tabla==(operac|PUESTOS)) {
			SetCursorFrom(ctabla, aux->cliente, aux->objet, aux->tippto, aux->codint);
			SetCursorTo  (ctabla, aux->cliente, aux->objet, aux->tippto, aux->codint);
			sprintf(stabla,"PUESTOS");
		}
		if (tabla==(operac|ASIG)) {
			SetCursorFrom(ctabla, aux->emp, aux->cliente, aux->objet, aux->tippto, aux->codint, MIN_SHORT, MIN_LONG);
			SetCursorTo  (ctabla, aux->emp, aux->cliente, aux->objet, aux->tippto, aux->codint, MAX_SHORT, MAX_LONG);
			sprintf(stabla,"ASIG");
		}
		if (tabla==(operac|ASIGH)) {
			SetCursorFrom(ctabla, aux->emp, aux->cliente, aux->objet, aux->tippto, aux->codint, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
			SetCursorTo  (ctabla, aux->emp, aux->cliente, aux->objet, aux->tippto, aux->codint, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
			sprintf(stabla,"ASIGH");

		}
		fprintf(archivo , "%s\t%d\t%d\t%d\t%s\t%ld\t%d\t%ld\t\t%d\t%d\t%d\t%d\n", 
		 stabla, aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng,aux->cliente, aux->objet, aux->tippto, aux->codint, aux->newcodint);

		sprintf(auxi, "Escribe Tabla %s Cliente %ld Objetivo %d %ld%%", stabla, aux->cliente, aux->objet, ((caso++)*100)/totalcasos );
 		FmSetFld(fm0, COMENT, auxi);
		WiRefresh();
		
		while (FetchCursor(ctabla) != ERROR) {

			DelRecord(tabla);			

			sprintf(svalor, "%d", IFld(campo));
			if (tabla==(operac|PUESTOS)) 
				SetIFld(operac|PUESTOS_PADREINT, IFld(campo));

			if (tabla==(operac|ASIG)) 
				SetFld(operac|ASIG_REGPTO, svalor);

			if (tabla==(operac|ASIGH)) 
				SetFld(operac|ASIGH_REGPTO, svalor);

			SetIFld(campo, aux->newcodint+500);
			PutRecord(tabla);

			if (tabla==(operac|ASIG) || tabla==(operac|ASIGH)) {
				if (tabla==(operac|ASIG)) {
					v_nroleg= LFld(operac|ASIG_NROLEG);
					v_diad  = DFld(operac|ASIG_FECASIG);
					v_diah  = DFld(operac|ASIG_FECHAS);
				}
				if (tabla==(operac|ASIGH)) {
					v_nroleg= LFld(operac|ASIGH_NROLEG);
					v_diad  = DFld(operac|ASIGH_FECALT);
					v_diah  = DFld(operac|ASIGH_FECBAJ);
				}


				ActTabla(&aux, operac|PARTE,     operac|PARTEbyLEG,       operac|PARTE_PUESTO,     v_nroleg, v_diad, v_diah);
				ActTabla(&aux, operac|EXCEPCION, operac|EXCEPCIONbyEMP,   operac|EXCEPCION_PUESTO, v_nroleg, v_diad, v_diah);

			} 
		} 

		aux = aux->ant;
	}	while ( aux != NULL );

	DeleteCursor(ctabla);

}

void ActTabla(s_crosspue **pnpuestos, dbtable tabla, dbindex indice, dbfield campo, long nroleg, DATE fdesde, DATE fhasta  )
{

	s_crosspue * aux;
	dbcursor ctabla;
	char stabla1[15];

// PARTE     leg(emp, cliente, objetivo, nroleg, dia)
// EXCEPCION primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo),

	ctabla = CreateCursor(indice, IO_NOT_LOCK);
	aux = (*pnpuestos); 
	sprintf(stabla1,"%s",NULL_STR);

	if (tabla==(operac|PARTE)) {
		SetCursorFrom(ctabla, aux->emp, aux->cliente, aux->objet, nroleg, fdesde);
		SetCursorTo  (ctabla, aux->emp, aux->cliente, aux->objet, nroleg, fhasta);
		sprintf(stabla1,"PARTE");

	} 
	if (tabla==(operac|EXCEPCION)) {
		SetCursorFrom(ctabla, aux->emp, aux->cliente, aux->objet, fdesde, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (ctabla, aux->emp, aux->cliente, aux->objet, fhasta, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		sprintf(stabla1,"EXCEPCION");

	} 
	fprintf(archivo , "\t%s\t%d\t%d\t%d\t%s\t%ld\t%d\t%ld\t%d\t%d\t%d\t%d\t%ld\t%.3D\t%.3D \n", 
	 stabla1,aux->emp, aux->tipcomp,aux->serie, aux->deleg, aux->nroot, aux->nroreng,aux->cliente, aux->objet, aux->tippto, aux->codint, aux->newcodint, nroleg, fdesde, fhasta);
	
	while (FetchCursor(ctabla) != ERROR) {

		if (tabla==(operac|EXCEPCION)) 
			if (nroleg!=LFld(operac|EXCEPCION_NROLEG))
				continue;

		if (tabla==(operac|PARTE)) 
			SetIFld(operac|PARTE_ASICBLUS, IFld(campo));

		if (IFld(campo)!=aux->codint)
			continue;

		SetIFld(campo, aux->newcodint+500);
		PutRecord(tabla);
	} 

	DeleteCursor(ctabla);

}

