/********************************************************************
*
* MODULE & VERSION : %W% 
* DATE             : %E% 
* TIME             : %U% 
*
* CREATED          : 05/07/12
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "ccosto.fmh"
#include "sue.sch"

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);

static void Lectura(fm_cmd, find_mode);

/* Declaraciones globales */
form fm0;

/* Programa principal */
wcmd(ccosto, %I% %G%)
{
	fm_cmd cmd;
	fm0 = OpenForm("ccosto", FM_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(CCOSTO);
	case FM_UPDATE:
		BeginTransaction();
		FmToDb(fm0, 0, CONTROL_FLD, 0);
		PutRecord(CCOSTO);

		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(CCOSTO);
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	FmToDb(fm0, 0, COD);
	switch(GetRecord(CCOSTObyEMP, mode, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(CCOSTObyEMP, mode);
		DbToFm(fm0, 0, COD);
		FmShowFlds(fm0, 0, COD);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	DbToFm(fm0, 0, CONTROL_FLD, 0);
	FmShowFlds(fm0, 0, COD);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case COD:
		break;
	case DENOM:
		break;
	case ASIG:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case COD:
		break;
	case DENOM:
		break;
	case ASIG:
		break;
	}
	return FM_OK;
}
