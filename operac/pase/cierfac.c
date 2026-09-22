/********************************************************************
*
* MODULE & VERSION : @(#)cierfac.c	1.4
* DATE             : 03/01/09
* TIME             : 11:00:55
*
* CREATED          : 05/01/99
*
* DESCRIPTION:
*	Este proceso se encarga de realizar el cierre de operaciones para facturacion.
	La idea es impedir la modificación del parte sin interferir en los pases de las horas.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* COMENTARIO: Si se modifica funcionalidad actualizar cierfac.hlp
*********************************************************************/
#include <ideafix.h>
#include "comerc.sch"
#include "operac.sch"
#include "operac.h"
#include "webinter.h"
#include "comerc.h"
#include "cierfac.fmh"

FILE *fp1 = NULL;

// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);
static void CambiarEstadoParte();
static void CamEstParteRetro();
bool ObjetivoValido (long cliente, short objetivo);

// Variables globales.
form fm0;
schema com;

// Programa principal
wcmd(cierfac, 1.4 01/09/03)
{
	com = OpenSchema("comerc", IO_EABORT);
	fm0 = OpenForm("cierfac", FM_EABORT);

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ingresar al programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

  	if ((fp1 = fopen("cierfac.log", "a+")) == NULL)
  		Error("No se pudo crear el archivo de log cierfac.log");

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ejecutar el programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}


	fprintf(fp1, "\nCIERRE DE FACTURACION: Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fprintf(fp1, "Cliente desde %ld Hasta %ld - Objetivo Desde %d Hasta %d - Fecha Desde %.3D Hasta %.3D \n",
				FmLFld(fm0, CLID), FmLFld(fm0, CLIH), FmIFld(fm0, OBJD), FmIFld(fm0, OBJH),
				FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH));

	BeginTransaction();
	CambiarEstadoParte();
	CamEstParteRetro  ();
	EndTransaction();
	CloseAllSchemas();
	fclose(fp1);
}

static void CambiarEstadoParte()
{
	DATE dia = NULL_DATE;
	dbcursor c_parte, c_obj;

	c_obj = CreateCursor(com|OBJETIVObyEMP, IO_NOT_LOCK);
	c_parte = CreateCursor(PARTEbyEMP, IO_LOCK);

	SetCursorFrom(c_obj, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
	SetCursorTo  (c_obj, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
	while (FetchCursor(c_obj) != ERROR) {

		SetCursorFrom(c_parte, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_parte, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_parte) != ERROR) {
			if (!ObjetivoValido (LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))) {
				continue;
			}

			if (dia != DFld(PARTE_DIA)) {
				FmSetLFld(fm0, COMCLI,  LFld(PARTE_CLIENTE));
				FmSetIFld(fm0, COMOBJ,  IFld(PARTE_OBJETIVO));
				FmSetDFld(fm0, COMENTARIO,  DFld(PARTE_DIA));
				FmSetFld (fm0, DCOMENTARIO, "Parte");
				WiRefresh();
		 		dia = DFld(PARTE_DIA);
			}
			if (IFld(PARTE_CONFIR) != CERRADO_FAC && IFld(PARTE_CONFIR) != CERRADO) {
				SetIFld(PARTE_CONFIR, CERRADO_FAC);
				PutRecord(PARTE);
			}
		}
	}
	FmSetLFld(fm0, COMCLI,  NULL_LONG);
	FmSetIFld(fm0, COMOBJ,  NULL_SHORT);
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

		if (IFld(RETRO_CONFIR) != CERRADO_FAC && IFld(RETRO_CONFIR) != CERRADO) {
			SetIFld(RETRO_CONFIR, CERRADO_FAC);
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

