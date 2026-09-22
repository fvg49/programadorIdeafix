/********************************************************************
*
* MODULE & VERSION : @(#)cpcodint2.c	1.1
* DATE             : 11/06/21
* TIME             : 18:46:12
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
#include "cpcodint2.fmh"
#include "aurus.sch"
#include "comerc.sch"
#include "operac.sch"
#include "webinter.sch"

#define _TIPOLIS_ALTA	'A'
#define _TIPOLIS_BAJA	'B'
//#define _DEBUG
#define _NO_TIENE_OT 	99


/* Declaraciones globales */
form fm0;
schema comerc, operac, webinter;
FILE *archivo = (FILE *)NULL;
DATE fecini, fecfin, fecfinOT;
short tipser;
bool  error_ot=FALSE;


void ActualizaParte(int p_emp, long p_cliente, int p_objet, int p_tippto, long p_codant, long  p_codnue);
void ActualizaAsig(int p_emp, long p_cliente, int p_objet, int p_tippto, long  p_codant, long  p_codnue);
void ActualizaAsigh(int p_emp, long p_cliente, int p_objet, int p_tippto, long  p_codant, long  p_codnue);

char auxi[150];
int porccaso=0;
long totalcasos=0; 
/* Programa principal */
wcmd(cpcodint2, 1.1 06/21/11)
{
	fm_cmd cmd;
	fm0 = OpenForm("cpcodint2", FM_EABORT);
	dbcursor cnewcodint;
	int cant=0, caso=0;



	operac   = OpenSchema("operac"  ,   IO_EABORT);
	comerc   = OpenSchema("comerc"  ,   IO_EABORT);
	webinter = OpenSchema("webinter",   IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();
		cant=0;
		totalcasos=0; 

		cnewcodint = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

//		sprintf(sarch, "cpcodint2.%D-%T.txt", Today(), Hour());
//		archivo=fopen(sarch, "w");

        // RECORRE RELACION

		cnewcodint = CreateCursor(webinter|NEWCODINTbyEMP, IO_NOT_LOCK);
        // RECORRE NPUESTO
		SetCursorFrom(cnewcodint, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT, NULL_SHORT);
		if (FmIsNull(fm0, EMP))
			SetCursorTo(cnewcodint, MAX_SHORT, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		else {
			if (FmIsNull(fm0, CLIE)) 
				SetCursorTo(cnewcodint, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			else {
				if (FmIsNull(fm0, OBJET))
					SetCursorTo(cnewcodint, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), MAX_SHORT, MAX_SHORT, MAX_SHORT);
				else 
					SetCursorTo(cnewcodint, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MAX_SHORT, MAX_SHORT);
			}
		}
		while (FetchCursor(cnewcodint) != ERROR) {
			totalcasos++;
	 		FmSetFld(fm0, COMENT, "Verificando cantidad de casos");
	 		WiRefresh();
			
		} 

		MoveCursorFirst(cnewcodint); 
		caso=0;
		while (FetchCursor(cnewcodint) != ERROR) {

//			fprintf(stderr, "NEWCODINT\t%ld\t%d\t%d\t%ld\n", LFld(webinter|NEWCODINT_CLIENTE), IFld(webinter|NEWCODINT_OBJETIVO), IFld(webinter|NEWCODINT_TIPPTO), LFld(webinter|NEWCODINT_CODANT));

			porccaso=((caso++)*100)/totalcasos;
			sprintf(auxi, "Lee NEWCODINT Cliente %ld Objetivo %d %d%%", LFld(webinter|NEWCODINT_CLIENTE), IFld(webinter|NEWCODINT_OBJETIVO), porccaso);

	 		FmSetFld(fm0, COMENT, auxi);
	 		WiRefresh();

			ActualizaParte(IFld(webinter|NEWCODINT_EMP), LFld(webinter|NEWCODINT_CLIENTE), IFld(webinter|NEWCODINT_OBJETIVO),
			               IFld(webinter|NEWCODINT_TIPPTO), LFld(webinter|NEWCODINT_CODANT), LFld(webinter|NEWCODINT_CODNUE));

			ActualizaAsig (IFld(webinter|NEWCODINT_EMP), LFld(webinter|NEWCODINT_CLIENTE), IFld(webinter|NEWCODINT_OBJETIVO),
			               IFld(webinter|NEWCODINT_TIPPTO), LFld(webinter|NEWCODINT_CODANT), LFld(webinter|NEWCODINT_CODNUE));

			ActualizaAsigh(IFld(webinter|NEWCODINT_EMP), LFld(webinter|NEWCODINT_CLIENTE), IFld(webinter|NEWCODINT_OBJETIVO),
			               IFld(webinter|NEWCODINT_TIPPTO), LFld(webinter|NEWCODINT_CODANT), LFld(webinter|NEWCODINT_CODNUE));
			               	
			
			

		} 
		DeleteCursor(cnewcodint);
		
		
//		fclose(archivo);

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
  
void ActualizaParte(int p_emp, long p_cliente, int p_objet, int p_tippto, long p_codant, long  p_codnue)
{

	dbcursor ctabla;

	sprintf(auxi, "Escribe PARTE Cliente %ld Objetivo %d %d%%", p_cliente, p_objet, porccaso);
	FmSetFld(fm0, COMENT, auxi);
	WiRefresh();

	//rpuesto(emp, cliente, objetivo, ptoser, puesto, nroint, dia, nroleg);

	ctabla = CreateCursor(operac|PARTEbyRPUESTO, IO_NOT_LOCK);
	SetCursorFrom(ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, NULL_SHORT, NULL_DATE, NULL_LONG);
	SetCursorTo  (ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, MAX_SHORT,  MAX_DATE,  MAX_LONG);

	if (FmIFld(fm0, LIMPIA)) {
		while (FetchCursor(ctabla) != ERROR) {
			SetIFld(operac|PARTE_ASICBLUS, NULL_SHORT);
			PutRecord(operac|PARTE);
		} 
		MoveCursorFirst(ctabla); 
	}

	while (FetchCursor(ctabla) != ERROR) {

//		fprintf(stderr , "\tPARTE\t%d\t%ld\t%d\t%d\t%d\t%d\t%.3D\t%ld\n", p_emp, p_cliente, p_objet, p_tippto, p_codant, p_codnue, DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG));

		if (!IsNull(operac|PARTE_ASICBLUS))
			continue;

		DelRecord(operac|PARTE);

		SetIFld(operac|PARTE_PUESTO, p_codnue);
		SetIFld(operac|PARTE_ASICBLUS, p_codant);

		PutRecord(operac|PARTE);
	} 

	DeleteCursor(ctabla);

}
void ActualizaAsig(int p_emp, long p_cliente, int p_objet, int p_tippto, long  p_codant, long  p_codnue)

{
	dbcursor ctabla;
	char svalor[12];

	sprintf(auxi, "Escribe ASIG  Cliente %ld Objetivo %d %d%%", p_cliente, p_objet, porccaso);
	FmSetFld(fm0, COMENT, auxi);
	WiRefresh();

	ctabla = CreateCursor(operac|ASIGbyPUESTO, IO_NOT_LOCK);
	SetCursorFrom(ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, NULL_SHORT, NULL_LONG);
	SetCursorTo  (ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, MAX_SHORT,  MAX_LONG);

	if (FmIFld(fm0, LIMPIA)) {
		while (FetchCursor(ctabla) != ERROR) {
			SetFld(operac|ASIG_REGPTO, NULL_STR);
			PutRecord(operac|ASIG);
		} 
		MoveCursorFirst(ctabla); 
	}

	while (FetchCursor(ctabla) != ERROR) {
//		fprintf(stderr , "\tASIG\t%d\t%ld\t%d\t%d\t%d\t%d\t%.3D\t%ld\n", p_emp, p_cliente, p_objet, p_tippto, p_codant, p_codnue, DFld(operac|ASIG_FECASIG), LFld(operac|ASIG_NROLEG));

		if (!IsNull(operac|ASIG_REGPTO))
			continue;

		DelRecord(operac|ASIG);

		SetIFld(operac|ASIG_PUESTO, p_codnue);

		sprintf(svalor, "%d", p_codant);
		SetFld (operac|ASIG_REGPTO, svalor);

		PutRecord(operac|ASIG);
	} 

	DeleteCursor(ctabla);

}
void ActualizaAsigh(int p_emp, long p_cliente, int p_objet, int p_tippto, long  p_codant, long  p_codnue)
{

	dbcursor ctabla;
	char svalor[12];

	sprintf(auxi, "Escribe ASIGH Cliente %ld Objetivo %d %d%%", p_cliente, p_objet, porccaso);
	FmSetFld(fm0, COMENT, auxi);
	WiRefresh();
	
	ctabla = CreateCursor(operac|ASIGHbyPUESTO, IO_NOT_LOCK);
	SetCursorFrom(ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, NULL_SHORT, NULL_LONG, NULL_DATE, NULL_DATE);
	SetCursorTo  (ctabla, p_emp, p_cliente, p_objet, p_tippto, p_codant, MAX_SHORT,  MAX_LONG, MAX_DATE, MAX_DATE);

	if (FmIFld(fm0, LIMPIA)) {
		while (FetchCursor(ctabla) != ERROR) {
			SetFld(operac|ASIGH_REGPTO, NULL_STR);
			PutRecord(operac|ASIGH);
		} 
		MoveCursorFirst(ctabla); 
	}

	while (FetchCursor(ctabla) != ERROR) {
//		fprintf(stderr , "\tASIGH\t%d\t%ld\t%d\t%d\t%d\t%d\t%.3D\t%ld\n", p_emp, p_cliente, p_objet, p_tippto, p_codant, p_codnue, DFld(operac|ASIGH_FECALT), LFld(operac|ASIGH_NROLEG));

		if (!IsNull(operac|ASIGH_REGPTO))
			continue;

		DelRecord(operac|ASIGH);

		SetIFld(operac|ASIGH_PUESTO, p_codnue);

		sprintf(svalor, "%d", p_codant);
		SetFld (operac|ASIGH_REGPTO, svalor);

		PutRecord(operac|ASIGH);
	} 

	DeleteCursor(ctabla);

}

