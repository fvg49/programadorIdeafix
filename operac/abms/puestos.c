/********************************************************************
*
* MODULE & VERSION : @(#)puestos.c	1.2 
* DATE             : 07/05/16 
* TIME             : 12:47:56 
*
* CREATED          : 02/12/03
*
* DESCRIPTION:
*             Alta de puestos de Objetivos Brigadas.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comgral.h"
#include "comerc.h"
#include "puestos.fmh"
#include "operac.sch"

static fm_status before(form, fmfield, int);
static fm_status after (form, fmfield, int);
static void Lectura(fm_cmd, find_mode);
static int ProximoNroint(long cliente, int objet, int tippto);

/* Declaraciones globales */
form fm0;
bool existeregistro = FALSE;

/* Programa principal */
wcmd(puestos, 1.2 05/16/07)
{
	fm_cmd cmd;
	fm0 = OpenForm("puestos", FM_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(PUESTOS);
	case FM_UPDATE:
		BeginTransaction();
		FmToDb(fm0, 0, CONTROL_FLD, 0);
		PutRecord(PUESTOS);
		EndTransaction();

		FreeTable(PUESTOS);
		existeregistro = FALSE;
		break;
	case FM_DELETE:
		BeginTransaction();
		FmToDb(fm0, 0, CODINT);
		DelRecord(PUESTOS);
		EndTransaction();

		FreeTable(PUESTOS);
		existeregistro = FALSE;
		break;
	case FM_IGNORE:
		FreeTable(PUESTOS);
		existeregistro = FALSE;
  		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	FmToDb(fm0, 0, CODINT);
	switch(GetRecord(PUESTOSbyCLIENTE, mode, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(PUESTOSbyCLIENTE, mode);
		DbToFm(fm0, 0, CODINT);
		FmShowFlds(fm0, 0, CODINT);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	if (strcmp(SFld(PUESTOS_CODFREC), NULL_STR) || IFld(PUESTOS_HORAPT) >= 0)
		FmSetIFld(fm0, PTIME, TRUE);

	existeregistro = TRUE;

	DbToFm(fm0, 0, CONTROL_FLD, 0);
	FmShowFlds(fm0, 0, CODINT);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CODINT:
		FmSetIFld(fm, fno, ProximoNroint(FmLFld(fm, CLIENTE), FmIFld(fm, OBJET), FmIFld(fm, TIPPTO)));
		break;
	}

	//Si existe el registro, solo puede ser modificado la Fecha de Fin.
	if (existeregistro) {
		switch (fno) {   
			case AGRUPF : 
			case AGRUPH : 
			case FFINAL :
			case HFINAL :
			case CONTROL_FLD: 
				break;
			default :
				return FM_SKIP;
				break;
		}			
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
      	case OBJET :
		if (GetServicioObj(FmLFld(fm, CLIENTE), FmIFld(fm, fno)) != BRIGADA) {
			Warning("El Objetivo no es de Brigadas.");
			FmNextFld(fm, CLIENTE);
		}
		break;
	}
   	return FM_OK;
}

static int ProximoNroint(long cliente, int objet, int tippto)
{
	int proximonroint = 1;

	SetLFld(PUESTOS_CLIENTE, cliente);
	SetIFld(PUESTOS_OBJET,   objet);
	SetIFld(PUESTOS_TIPPTO,  tippto);
	SetIFld(PUESTOS_CODINT,  MAX_SHORT);
	if (GetRecord(PUESTOSbyCLIENTE, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
		return proximonroint = IFld(PUESTOS_CODINT) + 1;
	 
	return proximonroint;
}

