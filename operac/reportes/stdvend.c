/********************************************************************
*
* MODULE & VERSION : @(#)stdvend.c	1.2
* DATE             : 01/06/05
* TIME             : 15:40:01
* DESCRIPTION:
*               Listado de horas de servicio
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*

*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "stdvend.fmh"
#include "stdvend.rph"
#include "stdvend2.rph"
#include "bill.sch"
#undef SERVICIO
#undef SERVICIO_DESCRIP
#include "comerc.sch"
#include "operac.sch"

/* Estructuras */
struct scli {
	long cliente;
	int  objet;
	DATE fecha;
	long  svh;
	struct scli *sig;
} *scli_est = NULL;

/* Funciones privadas */
void GenerarReporte();
void ImprimirReporte();
void ImprimirReporteA();
struct scli* GuardarStd(long p_cli, short p_obj, DATE p_dia, long p_std, struct scli *corr);
private fm_status before(form fm, fmfield fno, int row);
private fm_status after(form fm, fmfield fno, int row);
bool FechasOk() ;
void AbrirSalida();

/* Declaraciones globales */
form fm0;
report rp0;
schema comerc, operac, bill;
FILE *fp;

/* Programa principal */
wcmd(stdvend, 1.2 06/05/01)
{
	fm_cmd cmd;
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

	fm0 = OpenForm  ("stdvend", FM_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
		case FM_UPDATE:
			GenerarReporte();
			if (*FmSFld(fm0, SALIDA) == 'A')
				ImprimirReporteA();
			else
				ImprimirReporte();

			FmSetFld (fm0, COMENT, NULL_STR);
			WiRefresh();
			return;
			break;
		case FM_IGNORE:
			break;
	}
}

void GenerarReporte()
{
	dbcursor curobj;
	DATE fdesde, fhasta, faux;
	long std;

	fdesde = FirstMonthDay(DMYToD(1, FmIFld(fm0, MESD), FmIFld(fm0, ANIOD)));
	fhasta = LastMonthDay (DMYToD(1, FmIFld(fm0, MESH), FmIFld(fm0, ANIOH)));

	curobj=CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom (curobj, FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
	SetCursorTo	  (curobj, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));

	while (FetchCursor(curobj) != ERROR) {
		char buffer[200];
		sprintf (buffer, "Procesando Puestos de Cliente %ld Objetivo %d", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		for (faux=fdesde; faux <= fhasta; faux ++) {
				std = StdHr(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), 
										  NULL_SHORT, faux, faux);

			scli_est = GuardarStd (LFld(comerc|OBJETIVO_CLIENTE),
						FmIFld(fm0, DETOBJ) ? IFld(comerc|OBJETIVO_OBJET) : NULL_SHORT,
						FmIFld(fm0, DETDIA) ? faux : NULL_DATE,
						std,
						scli_est);
		}
	}
	DeleteCursor(curobj);
}

struct scli* GuardarStd(long p_cli, short p_obj, DATE p_dia, long p_std, struct scli *corr)
{
	bool inserto = FALSE;
	struct scli *ant, *tope, *nuevo;
	ant = corr;
	tope = corr;
	
	while(!inserto && corr != NULL && corr->cliente <= p_cli) {
		// Es menor sigo recorriendo
		if (corr->cliente < p_cli ||
		   (corr->cliente == p_cli && corr->objet < p_obj) ||
		   (corr->cliente == p_cli && corr->objet == p_obj && corr->fecha < p_dia)) {
			ant = corr;
			corr = corr->sig;
		}
		else {
			if (corr->cliente == p_cli && corr->objet == p_obj && corr->fecha == p_dia){
				corr->svh += p_std;
				inserto = TRUE;
			}
		}
	}

	//No lo encontro o encontro la posicion que va
	if (!inserto) {
		nuevo = (struct scli *) malloc(sizeof(struct scli));
		nuevo->cliente = p_cli;
		nuevo->objet = p_obj;
		nuevo->fecha = p_dia;
		nuevo->svh = p_std;

		nuevo->sig = corr;
		if (ant == corr) {
			corr = nuevo;
		}
		else {
			ant->sig = nuevo;
			corr = tope;
		}
	}
	
	if (inserto) {
		corr = tope;
	}
	return corr;
}

void ImprimirReporte()
{
	struct scli *old_scli;
	
	AbrirSalida();
	for (;scli_est != NULL;) {

		RpSetLFld	(rp0, RCLI,		scli_est->cliente);
		RpSetFld	(rp0, RDCLI,	GetDescCli(scli_est->cliente));
		RpSetIFld	(rp0, ROBJ,		scli_est->objet);
		RpSetFld	(rp0, RDOBJ, 	GetObjDescrip(scli_est->cliente, scli_est->objet));
		RpSetLFld	(rp0, RSVH,		scli_est->svh);
		
		if (!FmIFld(fm0, DETOBJ) && !FmIFld(fm0, DETDIA))
			DoReport	(rp0, TITCLI_IMP);

		if (FmIFld(fm0, DETOBJ) && !FmIFld(fm0, DETDIA))
			DoReport	(rp0, TITOBJ_IMP);

		if (FmIFld(fm0, DETDIA)) {
            RpSetDFld(rp0, RFECHA, scli_est->fecha);
			DoReport	(rp0, LINFECHA);
		}

		//Borro el que ya lei y leo el siguiente
		old_scli = scli_est;
		scli_est = scli_est->sig;
		free(old_scli);
	} 
	CloseReport(rp0);
}


void ImprimirReporteA()
{
	struct scli *old_scli;

	AbrirSalida();
	for (;scli_est != NULL;) {

		RpClearZone(rp0, LINFECHA);
		RpSetLFld	(rp0, RCLI,		scli_est->cliente);
		RpSetFld	(rp0, RDCLI,	GetDescCli(scli_est->cliente));
		if (scli_est->objet != NULL_SHORT) {
			RpSetIFld	(rp0, ROBJ,		scli_est->objet);
			RpSetFld	(rp0, RDOBJ, 	GetObjDescrip(scli_est->cliente, scli_est->objet));
		}
		if (scli_est->fecha != NULL_DATE) {
            RpSetDFld(rp0, RFECHA, scli_est->fecha);
		}

		RpSetLFld	(rp0, RSVH,		scli_est->svh);
		DoReport	(rp0, LINFECHA);

		//Borro el que ya lei y leo el siguiente
		old_scli = scli_est;
		scli_est = scli_est->sig;
		free(old_scli);
	} 
	CloseReport(rp0);
}

private fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
		case ARCHIVO: 
			if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, ARCHIVO))
				FmSetFld(fm0, ARCHIVO, "stdvend.txt");
		break;
	}
	return FM_OK;
}

private fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case AGRUPF:
			if (!FechasOk()) {
				WiMsg ("Periodo DESDE debe ser menor o igual al HASTA");
				return FM_REDO;
			}
		break;
	}
	return FM_OK;
}

bool FechasOk() 
{
	DATE fechad, fechah;

	fechad = FirstMonthDay(DMYToD(1, FmIFld(fm0, MESD), FmIFld(fm0, ANIOD)));
	fechah = LastMonthDay (DMYToD(1, FmIFld(fm0, MESH), FmIFld(fm0, ANIOH)));

	return (fechad < fechah);
}

void AbrirSalida()
{
	char periodo[32], peraux[32];

	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("stdvend2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_FILE, FmSFld(fm0, ARCHIVO));
	}
	else {
		rp0 = OpenReport("stdvend", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, *FmSFld(fm0,SALIDA) == 'I' ? RP_IO_DEFAULT : RP_IO_TERM, NULL_STR);
	}
	BeginReport(rp0,1,NULL_STR);
	RpSetIFld(rp0, RDETFEC, FmIFld(fm0, DETDIA) ? 1 : 0);
	RpSetIFld(rp0, RDETOBJ, FmIFld(fm0, DETOBJ) ? 1 : 0);
	
	if (FmIFld(fm0, UNICPER)) {
		//Es el mismo periodo Desde Hasta
		sprintf(peraux, "%s %4.4d", MonthName(FmIFld(fm0, MESH)), FmIFld(fm0, ANIOH));
	}
	else {
		sprintf(peraux, "%3.3s %4.4d A %3.3s %4.4d", MonthName(FmIFld(fm0, MESD)), FmIFld(fm0, ANIOD),
		                                               MonthName(FmIFld(fm0, MESH)), FmIFld(fm0, ANIOH));
	}

 	sprintf(periodo, "%24.24s", peraux);

	RpSetFld(rp0, RPER, periodo);
}
