/********************************************************************
*
* MODULE & VERSION : @(#)lugpag.c	1.6 
* DATE             : 08/08/12 
* TIME             : 16:14:38 
*
* CREATED          : 15/10/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lugpag.fmh"
#include "aurus.sch"
#include "comerc.sch"
#include "bill.sch"
#include "comerc.h"
#include "filial.h"
#include "operac.h"

/* Funciones privadas */
/* Funciones privadas */
static void displayLugPagXdel(char *buffer);
void HelpLugPagXdel(form fm, fmfield fno, int row);
static int validaLugPagXdel(void);

static fm_status before(form, fmfield, int), after(form, fmfield, int);

static void Lectura(fm_cmd, find_mode);

/* Declaraciones globales */
form fm0;
schema comerc, sue;
char denomlp[40];


/* Programa principal */
wcmd(lugpag, 1.6 08/12/08)
{
	fm_cmd cmd;
	comerc = OpenSchema("comerc", IO_EABORT);

	fm0 = OpenForm("lugpag", FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(OBJETIVO);
	case FM_UPDATE:
		BeginTransaction();
		FmToDb(fm0, 0, CONTROL_FLD, 0);
		
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(OBJETIVO_CLIENTE), IFld(OBJETIVO_OBJET))) {
			Warning("No tiene permisos sobre el Cliente %ld - Objetivo %d", LFld(OBJETIVO_CLIENTE), IFld(OBJETIVO_OBJET));
		}
		else
			PutRecord(OBJETIVO);

		EndTransaction();
		break;
	case FM_DELETE:
		BeginTransaction();
		FmToDb(fm0, 0, OBJET);
		SetLFld (OBJETIVO_OFPAG, NULL_LONG);
		PutRecord(OBJETIVO);
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(OBJETIVO);
		break;
	}
	
  	FinObjetivosXusr();
  	FinClientesXusr();
  	FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	FmToDb(fm0, 0, OBJET);
	switch(GetRecord(OBJETIVObyCLIENTE, mode, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(OBJETIVObyCLIENTE, mode);
		DbToFm(fm0, 0, OBJET);
		FmShowFlds(fm0, 0, OBJET);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}

	FmSetFld (fm0, DELEGA, SFld(OBJETIVO_DELEGA));
	FmSetFld (fm0, DDELEGA, GetDescDelega(SFld(OBJETIVO_DELEGA)));
	
	strcpy(denomlp, GetDescrLugPag (LFld(OBJETIVO_OFPAG)));
	FmSetFld(fm0, DESOF1, denomlp);

	DbToFm(fm0, 0, CONTROL_FLD, 0);

	FmShowFlds(fm0, 0, OBJET);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case CLIE:
  		InicClientesXusr();
		break;
	case OBJET:
		InicObjetivosXusr(FmLFld(fm, CLIE, row), FmIFld(fm, EMP, row));
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
		break;
	case CLIE:
	 	if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);

		if (ValidaClienteXusr(FmLFld(fm, CLIE, row)))
		  	FmSetFld(fm, DESCLI, GetDescCliente(FmLFld(fm, CLIE, row)), row);
		else {
  			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIE, row));
			FmSetLFld(fm, CLIE, NULL_LONG, row);
			FmSetFld(fm, DESCLI	, NULL_STR, row);
			return FM_REDO;
		}
	break;
	case OBJET:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIE, row));

		if (ValidaObjetivoXusr(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
			FmSetFld(fm, DESCRIP, GetObjDescrip(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row)), row);
		else	{
			Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row));
			FmSetIFld(fm, OBJET, NULL_SHORT, row);
			FmSetFld(fm, DESCRIP, NULL_STR, row);
			return FM_REDO;
   		}
	break;
	case OFPAG:
		if (FmKeyCode(fm) == K_HELP)
			HelpLugPagXdel(fm, fno, row);
		else {
			SetKey(comerc|LUPAXDELbyEMP, FmIFld(fm0, EMP), FmSFld(fm0, DELEGA), FmLFld(fm0, OFPAG));
			if (GetRecord(comerc|LUPAXDELbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				Warning("El Lugar de Pago %ld - %s no pertenece a la Delegación %s - %s", FmLFld(fm0, OFPAG), 
				               GetDescrLugPag (FmLFld(fm0, OFPAG)), FmSFld(fm0, DELEGA), GetDescDelega(FmSFld(fm0, DELEGA)));
				return FM_REDO;
			}	
			
			FmSetFld(fm0, DESOF1, GetDescrLugPag (LFld(comerc|LUPAXDEL_OFPAG)));
		}	
		
		break;
	}
	return FM_OK;
}

void HelpLugPagXdel(form fm, fmfield fno, int row)
{
	dbcursor c_lugpaxdel = (dbcursor) NULL;
	int n;
	
	c_lugpaxdel = CreateCursor(comerc|LUPAXDELbyEMP, IO_NOT_LOCK);
	
	SetCursorFrom(c_lugpaxdel, FmIFld(fm0, EMP), FmSFld(fm0, DELEGA), MIN_LONG);
	SetCursorTo(c_lugpaxdel, FmIFld(fm0, EMP), FmSFld(fm0, DELEGA), MAX_LONG);
	
    n = PopUpDbMenu(10, 100, "Lugar de Pago por Delegación", c_lugpaxdel, 0, validaLugPagXdel, displayLugPagXdel);
	
	if (n >= 0) {
		FmSetLFld(fm, fno, LFld(comerc|LUPAXDEL_OFPAG), row);
		FmSetFld(fm, fno+1, denomlp, row);
		FmShowFlds(fm, fno, fno+1, row);
	}	
	
	DeleteCursor(c_lugpaxdel);
}

static int validaLugPagXdel(void)
{
	return TRUE;
}

static void displayLugPagXdel(char *buffer)
{                     
   strcpy(denomlp, GetDescrLugPag (LFld(comerc|LUPAXDEL_OFPAG)));
            
   sprintf(buffer, "%ld - %s", LFld(comerc|LUPAXDEL_OFPAG), denomlp);
}                                                            
