/********************************************************************
*
* MODULE & VERSION : @(#)horfac.c	1.1 
* DATE             : 02/09/23 
* TIME             : 12:39:05 
*
* DESCRIPTION:
*               Generacion de archivo con hs extras para facturacion.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "horfac.fmh"
#include "horfac.rph"
#include "comerc.sch"
#include "operac.sch"

#define MAXCLI		80000

struct cliente {
	int  emp;
	long cli;
	int  obj;
	DATE fecha;
	int  horas;
} pcli[MAXCLI], *ucli = pcli, *ecli, *fincli, *ncli;

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static void AbrirSalida();
void ImprimirReporte();
void GenerarReporte();
void CargarCliObj(int emp, long cliente, int objetivo, DATE fecha, int hn, int h50, int h100);
private int compcli(struct cliente *a, struct cliente *b);

/* Declaraciones globales */
form fm0;
report rp0;
schema comerc, operac;
FILE *fp = NULL;

/* Programa principal */
wcmd(horfac, 1.20 07/19/02)
{
	comerc = OpenSchema("comerc", IO_EABORT);

	fm0 = OpenForm  ("horfac", FM_EABORT);

	while (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	InicioListaTipoExcepcion();
	AbrirSalida();
	GenerarReporte();
	ImprimirReporte();

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));
	}
	else {                                                             
		rp0 = OpenReport("horfac", RP_EABORT|RP_NOBEGIN);

		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}
}

void GenerarReporte()
{
	dbcursor cparte, cexc, cpto, cretro, cretroexc;
	char buffer[50];
	int puesto;
	short tipoexc;

	cexc   = CreateCursor(EXCEPCIONbyEMP, IO_NOT_LOCK);

	/*Recorro las excepciones */
	if (!FmIsNull(fm0, CLIDESDE)) {
		SetCursorFrom(cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIFld(fm0, OBJDESDE), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIHASTA), FmIFld(fm0, OBJHASTA), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		SetCursorFrom(cexc, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, MIN_DATE,
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cexc, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_DATE,
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while (FetchCursor(cexc) != ERROR) {
		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		SetKey(comerc|OBJETIVO, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC)
			continue;

		if (IFld(EXCEPCION_CONDIC) == ACARGO_EMP)
			continue;

		CargarCliObj(FmIFld(fm0, EMP), LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO), DFld(EXCEPCION_DIA),
					 IFld(EXCEPCION_HORAS), IFld(EXCEPCION_HS50), IFld(EXCEPCION_HS100));

		sprintf(buffer, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		FmSetFld(fm0, COMENT, buffer);
		WiRefresh();
	}
	DeleteCursor(cexc);
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
}

void CargarCliObj(int emp, long cliente, int objetivo, DATE fecha, int hn, int h50, int h100)
{
	for (ecli = pcli; ecli < ucli; ecli++) {
		if (ecli->emp == emp && ecli->cli == cliente && ecli->obj == objetivo && ecli->fecha == fecha) {
			ecli->horas += hn + h50 + h100;
			break;
		}
	}
	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI])
			Error("Tabla interna saturada. Max %d", MAXCLI);

		ucli->emp   = emp;
		ucli->cli   = cliente;
		ucli->obj   = objetivo;
		ucli->fecha = fecha;
		ucli->horas = hn + h50 + h100;
		ucli++;
	}
}

void ImprimirReporte()
{
	char buffer[50];

	qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	sprintf (buffer, "Generando reporte");
	FmSetFld (fm0, COMENT, buffer);
	WiRefresh();

	for (ecli = pcli; ecli < ucli; ecli++) {
		if (*FmSFld(fm0, SALIDA) == 'A')
		    fprintf(fp, "%ld\t%d\t%.3D\t%9.2f\n", ecli->cli, ecli->obj, ecli->fecha, (double)ecli->horas / 100.0);
		else {
			RpSetLFld(rp0, RCLI,  ecli->cli);
			RpSetFld (rp0, RDCLI, GetDescCli(ecli->cli));

			RpSetIFld(rp0, ROBJ,  ecli->obj);
			RpSetFld (rp0, RDOBJ, GetObjDescrip(ecli->cli, ecli->obj));

			RpSetDFld(rp0, RFCH,  ecli->fecha);
			RpSetLFld(rp0, SATOT, ecli->horas);
			DoReport(rp0, ZFCH);
		}
	}
}

private int compcli(struct cliente *a, struct cliente *b)
{
	return	a->emp    < b->emp    ? -1 : a->emp    > b->emp    ? 1 :
			a->cli    < b->cli    ? -1 : a->cli    > b->cli    ? 1 :
			a->obj    < b->obj    ? -1 : a->obj    > b->obj    ? 1 :
			a->fecha  < b->fecha  ? -1 : a->fecha  > b->fecha  ? 1 :
			0;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FECHAD:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'L')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "LUNES");
		#endif
	break;

	case FECHAH:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'D')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "DOMINGO");
		#endif
	break;
	}
	return FM_OK;
}	
