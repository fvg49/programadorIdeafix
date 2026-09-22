/********************************************************************
*
* MODULE & VERSION : @(#)feriado.c	1.1 
* DATE             : 06/05/15 
* TIME             : 12:40:48 
*
* CREATED          : 11/06/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "feriado.fmh"
#include "operac.sch"
#include "sue.sch"

/* Funciones privadas */
static void Lectura(fm_cmd, find_mode);
static int validate1(void);
static void display1(char *buffer);
private fm_status HelpFecha(form fm, fmfield fno, int row);

/* Declaraciones globales */
int i;
form fm0;

int NLIN;
/* Programa principal */
wcmd(feriado, 1.1 05/15/06)
{
	fm_cmd cmd;
	fm0 = OpenForm("feriado", FM_EABORT);
	NLIN = FmFldLen(fm0, MULTI);

	FmOnKey(fm0, K_HELP, HelpFecha, FECHA, FECHA);
	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(FERIADO);
	case FM_UPDATE:
		BeginTransaction();
		FmToDb(fm0, 0, CONTROL_FLD, 0);
		SetFld(FERIADO_PROV, LOW_VALUE);
		while(GetRecord(FERIADObyFECHA, NEXT_KEY|PARTIAL_KEY,IO_NOT_LOCK, 2)!=ERROR)
			DelRecord(FERIADObyFECHA);
		FmToDb(fm0, 0, CONTROL_FLD, 0);
		for(i=0 ; i<NLIN && !FmIsNull(fm0, DESPAI, i) ; i++) {
			FmToDb(fm0, DESPAI, DESPROV, i);
			PutRecord(FERIADObyFECHA);
		}
		EndTransaction();
		break;
	case FM_DELETE:
		BeginTransaction();
		FmToDb(fm0, 0, PAIS1);
		SetFld(FERIADO_PROV, LOW_VALUE);
		while(GetRecord(FERIADObyFECHA, NEXT_KEY|PARTIAL_KEY,IO_NOT_LOCK, 2)!=ERROR)
			DelRecord(FERIADObyFECHA);
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(FERIADO);
		break;
	}
}

static void Lectura(fm_cmd cmd, find_mode mode)
{
	FmToDb(fm0, 0, FECHA);
	switch(cmd) {
	case FM_READ:
		SetFld(FERIADO_PAIS, LOW_VALUE);
		SetFld(FERIADO_PROV, LOW_VALUE);
		mode = NEXT_KEY|PARTIAL_KEY;
		break;
	case FM_READ_NEXT:
		SetFld(FERIADO_PAIS, LOW_VALUE);
		SetFld(FERIADO_PROV, HIGH_VALUE);
		mode = NEXT_KEY;
		break;
	case FM_READ_PREV:
		SetFld(FERIADO_PAIS, LOW_VALUE);
		SetFld(FERIADO_PROV, LOW_VALUE);
		if (GetRecord(FERIADObyFECHA, PREV_KEY, IO_NOT_LOCK)==ERROR) {
			FmSetStatus(fm0, FM_EOF);
			return;
		}
		SetFld(FERIADO_PAIS, LOW_VALUE);
		SetFld(FERIADO_PROV, LOW_VALUE);
		mode = NEXT_KEY|PARTIAL_KEY;
		break;
	}
	switch(GetRecord(FERIADObyFECHA, mode, IO_LOCK|IO_TEST, 1)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(FERIADObyFECHA, mode, 1);
		DbToFm(fm0, 0, FECHA);
		FmShowFlds(fm0, 0, FECHA);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	DbToFm(fm0, 0, DESCRIP, 0);
	i=0;
	do {
		DbToFm(fm0, PAIS1, DESPROV, i++);
	} while (GetRecord(FERIADObyFECHA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1)!=ERROR);
}

private fm_status HelpFecha(form fm, fmfield fno, int row)
{
	static dbcursor CUR = NULL;
	int n;

	CUR = CreateCursor(FERIADObyFECHA, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, MIN_DATE, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, MAX_DATE, MAX_SHORT, MAX_SHORT);

	n = PopUpDbMenu(10, 50, "Feriados", CUR, 1, validate1, display1);
	if (n >= 0) {
		FmSetDFld(fm, fno,     DFld(FERIADO_FECHA));
		FmSetFld (fm, DESCRIP, SFld(FERIADO_DESCRIP));
		FmShowFlds(fm, fno, fno);
	}
	DeleteCursor(CUR);
}

static int validate1(void)
{
	return TRUE;
}

static void display1(char *buffer)
{
	sprintf(buffer,"%.3D   %-25.25s", DFld(FERIADO_FECHA), SFld(FERIADO_DESCRIP));
}

