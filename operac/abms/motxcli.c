/********************************************************************
* MODULE & VERSION : @(#)motxcli.c	1.2
* DATE             : 01/01/02
* TIME             : 09:11:06
*
* CREATED          : 17/08/2000
*
* DESCRIPTION:
*      Hay motivos de excepcion que no se pueden cargar para todos los cliente-objetivos
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "motxcli.fmh"
#include "operac.sch"
#include "comerc.sch"

/* Funciones privadas */
static void Lectura(fm_cmd, find_mode);
private void Grabardatos();

/* Declaraciones globales */
form fm0;
schema comerc, operac;

/* Programa principal */
wcmd(motxcli, 1.2 01/02/01)
{
	fm_cmd cmd;
	fm0 = OpenForm("motxcli", FM_EABORT);

	operac = OpenSchema("operac", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ: Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		Grabardatos();
		EndTransaction();
		break;
	case FM_IGNORE:
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
	i=0;
	while (GetRecord(comerc|OBJETIVObyCLIENTE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
		if (!IFld(comerc|OBJETIVO_ACTIVO)) {
			continue;
		}

		if (i == FmFldLen(fm0, MULTIS)) {
			Error ("No se pueden mostrar tantos renglones");
		}
		
		FmSetIFld(fm0, OBJET,    IFld(comerc|OBJETIVO_OBJET), i);
		FmSetFld (fm0, DOBJET,   GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), LFld(comerc|OBJETIVO_OBJET)), i);
		
		SetKey (operac|MOTXCLIbyCODCOND, FmIFld (fm0, CODCOND), FmIFld (fm0, CODMOT), LFld(comerc|OBJETIVO_CLIENTE), LFld(comerc|OBJETIVO_OBJET));
		if (GetRecord (operac|MOTXCLIbyCODCOND, THIS_KEY, IO_NOT_LOCK) == ERROR)
			FmSetIFld(fm0, VALCLI, FALSE, i);
		else			
			FmSetIFld(fm0, VALCLI, TRUE,  i);

		i ++;
	}
}

private void Grabardatos()
{
	int  i;

	SetKey (operac|MOTXCLIbyCODCOND, FmIFld (fm0, CODCOND), FmIFld (fm0, CODMOT), FmLFld (fm0, CLIE), MIN_SHORT);
	while (GetRecord (operac|MOTXCLIbyCODCOND, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		DelRecord (operac|MOTXCLI);
	}

	for (i = 0; i < FmFldLen(fm0, MULTIS) && !FmIsNull(fm0, OBJET, i); i++) {
		if (!FmIFld (fm0, VALCLI, i))
			continue;
			
		FmToDb (fm0, CODCOND, OBJET, i);
		PutRecord (operac|MOTXCLI);
	}
}

