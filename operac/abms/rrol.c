/********************************************************************
*
* MODULE & VERSION : @(#)rrol.c	1.2 
* DATE             : 05/11/21 
* TIME             : 16:27:00 
*
* CREATED          : 01/08/01
*
* DESCRIPTION:
*             Carga de Roles.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "rrol.fmh"

/* Funciones privadas */
static fm_status before(form fm, fmfield fno, int row);
static void Lectura(fm_cmd, find_mode);

/* Declaraciones globales */
form fm0;
int i, j;

/* Programa principal */
wcmd(rrol, 1.2 11/21/05)
{
	fm_cmd cmd;

	fm0 = OpenForm("rrol", FM_EABORT);

	while ((cmd = DoForm(fm0, before, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:  Lectura(cmd,THIS_KEY); break;
	case FM_ADD :
		InitRecord(RROL);
	case FM_UPDATE:
		BeginTransaction();
		SetIFld(RROL_CODROL, FmIFld(fm0, CODROL));
		SetIFld(RROL_FILA,   MIN_SHORT);
		SetIFld(RROL_COLUM,  MIN_SHORT);
		while (GetRecord(RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1) != ERROR)
			DelRecord(RROL);

		for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, POS1, i); i++) {
			SetIFld(RROL_CODROL,  FmIFld(fm0, CODROL));
			SetIFld(RROL_FILA, i + 1);
			for (j = 0; !FmIsNull(fm0, POS1 + j, i); ) {
				SetFld (RROL_VALOR, FmSFld(fm0, POS1 + j, i));
				SetIFld(RROL_COLUM, ++j);
				PutRecord(RROL);
			}
		}
		EndTransaction();
		break;
	case FM_DELETE:
		BeginTransaction();
		SetIFld(RROL_CODROL, FmIFld(fm0, CODROL));
		SetIFld(RROL_FILA,   MIN_SHORT);
		SetIFld(RROL_COLUM,  MIN_SHORT);
		while (GetRecord(RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1) != ERROR)
			DelRecord(RROL);
		EndTransaction();
	case FM_IGNORE:
		FreeTable(RROL);
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	dbcursor c_RROL;
	char vigi[50];

	SetIFld(RROL_CODROL, FmIFld(fm0, CODROL));
	SetIFld(RROL_FILA,   MIN_SHORT);
	SetIFld(RROL_COLUM,  MIN_SHORT);
	switch(GetRecord(RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, 1);
		DbToFm(fm0, 0, DROL);
		FmShowFlds(fm0, 0, DROL);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	c_RROL = CreateCursor(RROLbyCODROL, IO_NOT_LOCK);

	for (i = 0; i < FmFldLen(fm0, MULTI); i++) {
		j = 0;
		SetCursorFrom(c_RROL, FmIFld(fm0, CODROL), i + 1, MIN_SHORT);
		SetCursorTo  (c_RROL, FmIFld(fm0, CODROL), i + 1, MAX_SHORT);
		while (FetchCursor(c_RROL) != ERROR) {
			sprintf(vigi, "Ciclo %d", i + 1);
			FmSetFld(fm0, VIGI,     vigi,             i);
			FmSetFld(fm0, POS1 + j, SFld(RROL_VALOR), i);
			j++;
		}
	}
	
	DeleteCursor(c_RROL);
}

static fm_status before(form fm, fmfield fno, int row)
{
	char vigi[50];

	switch(fno) {
	case VIGI :
		if (row == 0) {
			sprintf(vigi, "Ciclo %d", row + 1);
			FmSetFld(fm, VIGI, vigi, row);
		}
		if (row > 0 && !FmIsNull(fm, POS1, row - 1)) {
			sprintf(vigi, "Ciclo %d", row + 1);
			FmSetFld(fm, VIGI, vigi, row);
		}
	break;
	}
	return FM_OK;
}
