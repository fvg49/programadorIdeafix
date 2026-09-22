/********************************************************************
*
* MODULE & VERSION : @(#)rcierfac.c	1.2
* DATE             : 01/10/01
* TIME             : 18:01:02
*
* CREATED          : 05/01/99
*
* DESCRIPTION:
*	Este proceso se encarga de realizar el cierre de operaciones para facturacion.
	La idea es impedir la modificación del parte sin interferir en los pases de las horas.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "operac.h"
#include "rcierfac.fmh"

// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);
static void CambiarEstadoParte();
static void CamEstParteRetro();
bool ObjetivoValido (long cliente, short objetivo);

// Variables globales.
form fm0;

// Programa principal
wcmd(rcierfac, 1.2 10/01/01)
{

	fm0 = OpenForm("rcierfac", FM_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	BeginTransaction();
	CambiarEstadoParte();
	CamEstParteRetro  ();
	EndTransaction();
	CloseAllSchemas();
}

static void CambiarEstadoParte()
{
	DATE dia = NULL_DATE;
	dbcursor c_parte;

	c_parte = CreateCursor(PARTEbyDIA, IO_LOCK);
	SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (!ObjetivoValido (LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))) {
			continue;
		}

		if (dia != DFld(PARTE_DIA)) {
			FmSetDFld(fm0, COMENTARIO,  DFld(PARTE_DIA));
			FmSetFld (fm0, DCOMENTARIO, "Parte");
			WiRefresh();
	 		dia = DFld(PARTE_DIA);
		}
		if (IFld(PARTE_CONFIR) == CERRADO_FAC) {
			if (LFld(PARTE_LIQFAC) != NULL_LONG) {
				Error ("El cliente %ld %d dia %.3D ya se proceso en la liquidacion %ld\nSolucion: Revertir la liquidacion ",
					LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), DFld(PARTE_DIA), LFld(PARTE_LIQFAC));
			}
			SetIFld(PARTE_CONFIR, A_CONF);
			PutRecord(PARTE);
		}
	}
	FmSetDFld(fm0, COMENTARIO,  NULL_DATE);
	FmSetFld (fm0, DCOMENTARIO, NULL_STR);
	WiRefresh();
	FreeTable(PARTE);
}

static void CamEstParteRetro()
{
	DATE dia = NULL_DATE;
	dbcursor c_retro;

	c_retro = CreateCursor(RETRObyRDIA, IO_LOCK);
	SetCursorFrom(c_retro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_retro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_retro) != ERROR) {

		if (!ObjetivoValido (LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO))) {
			continue;
		}

		if (dia != DFld(RETRO_DIA)) {
			FmSetDFld(fm0, COMENTARIO,  DFld(RETRO_DIA));
			FmSetFld (fm0, DCOMENTARIO, "Retroactivos");
			WiRefresh();
	 		dia = DFld(RETRO_DIA);
		}

		if (IFld(RETRO_CONFIR) == CERRADO_FAC) {
			if (LFld(RETRO_LIQFAC) != NULL_LONG) {
				Error ("El cliente %ld %d dia %.3D ya se proceso en la liquidacion %ld\nSolucion: Revertir la liquidacion ",
					LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA), LFld(RETRO_LIQFAC));
			}
			SetIFld(RETRO_CONFIR, A_CONF);
			PutRecord(RETRO);
		}
	}
	FmSetDFld(fm0, COMENTARIO,  NULL_DATE);
	FmSetFld (fm0, DCOMENTARIO, NULL_STR);
	WiRefresh();
	FreeTable(RETRO);
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case CLID:
			break;
	}
	return FM_OK;
}

bool ObjetivoValido (long cliente, short objetivo)
{ 
	if (cliente < FmLFld(fm0, CLID))
		return FALSE;

	if (cliente > FmLFld(fm0, CLIH))
		return FALSE;

	if (cliente == FmLFld(fm0, CLID) && objetivo < FmIFld(fm0, OBJD))
		return FALSE;

	if (cliente == FmLFld(fm0, CLIH) && objetivo > FmIFld(fm0, OBJH))
		return FALSE;

	return TRUE;
}

