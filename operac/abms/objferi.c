/********************************************************************
* MODULE & VERSION : @(#)objferi.c	1.1
* DATE             : 03/01/07
* TIME             : 17:20:03
*
* CREATED          : 07/01/2003
*
* DESCRIPTION:
*      Se indica los Objetivos que no se consideran los dias feriados
*      para realizar el calculo de horas legales semanales.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "objferi.fmh"
#include "comerc.sch"
#include "bill.sch"

/* Funciones privadas */
static void Lectura(fm_cmd, find_mode);
private void PutInObjetivo();

/* Declaraciones globales */
form fm0;
schema comerc;

/* Programa principal */
wcmd(objferi, 1.1 01/07/03)
{
	fm_cmd cmd;
	fm0 = OpenForm("objferi", FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ: Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		PutInObjetivo();
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(OBJETIVO);
		break;
	}
}

static void Lectura(fm_cmd cmd, find_mode mode)
{
	int i;

	SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm0, CLIE));
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	switch(GetRecord(comerc|OBJETIVObyCLIENTE, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(comerc|OBJETIVObyCLIENTE, NEXT_KEY|PARTIAL_KEY, 1);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm0, CLIE));
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	for (i = 0; GetRecord(comerc|OBJETIVObyCLIENTE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR &&
			i < FmFldLen(fm0, MULTIS); i++ ) {
		if (!IFld(comerc|OBJETIVO_ACTIVO)) {
			i--;
			continue;
		}
		FmSetIFld(fm0, OBJET,    IFld(comerc|OBJETIVO_OBJET), i);
		FmSetFld (fm0, DOBJET,   GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), LFld(comerc|OBJETIVO_OBJET)), i);
		FmSetIFld(fm0, CONSFER, IFld(comerc|OBJETIVO_CONSFER), i);
	}
}

private void PutInObjetivo()
{
	int  i;

	for (i = 0; i < FmFldLen(fm0, MULTIS) && !FmIsNull(fm0, OBJET, i); i++) {
	 	SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm0, CLIE));
		SetIFld(comerc|OBJETIVO_OBJET,   FmLFld(fm0, OBJET, i));
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			SetLFld(comerc|OBJETIVO_CONSFER, FmIFld(fm0, CONSFER, i));
			PutRecord(comerc|OBJETIVO);
		}
	}
}
