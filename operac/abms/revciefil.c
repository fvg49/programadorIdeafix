/********************************************************************
* MODULE & VERSION : @(#)revciefil.c	1.1
* DATE             : 08/04/23
* TIME             : 12:46:58
*
* CREATED          : 23/04/98
*
* DESCRIPTION:
*      Reversion de Cierre por Filial
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.sch"
#include "operac.sch"
#include "revciefil.fmh"
#include "filial.h"
#include "bill.h"
#include "billpro.h"

/* Funciones privadas */
static void SetMulti();

/* Declaraciones globales */
schema comerc, operac;
form fm0;
int i;

/* Programa principal */
wcmd(revciefil, 1.1 04/23/08)
{
	fm_cmd cmd;
	fm0 = OpenForm("revciefil", FM_EABORT);

	operac = OpenSchema("operac", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	SetMulti();
	
	while ((cmd = DoForm(fm0, NULLFP, NULLFP))  != FM_EXIT)
	switch (cmd) {
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		
		for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, FFILIAL, i); i++) {
			if (FmIFld(fm0, REVIERTE) == TRUE) {
		    	
		    	//se lee el registro a modificar
		    	SetKey(operac|CIEFILbyACTIVA, FALSE, FmSFld(fm0, FFILIAL, i), MAX_DATE, MAX_DATE, MAX_TIME);
    			(void)GetRecord(operac|CIEFILbyACTIVA, PARTIAL_KEY|PREV_KEY, IO_NOT_LOCK, 2);

	    		SetIFld(operac|CIEFIL_REVERTIDO, TRUE);
	    		SetLFld(operac|CIEFIL_IDREVER, GetUid());
				SetDFld(operac|CIEFIL_FECREVER, Today());
				SetTFld(operac|CIEFIL_HORAREVER, Hour());
				
				PutRecord(operac|CIEFIL);
			}	
		}	 
		
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
    }
}

static void SetMulti()
{
	int uid;
	dbcursor c_usrxfil = (dbcursor) NULL;
	uid = GetUid();

	i = 0;
	
	//Se lee usuario por filial para ver si es valido
	c_usrxfil = CreateCursor(comerc|USRXFILbyUSUARIO, IO_NOT_LOCK);
	SetCursorFrom(c_usrxfil, uid);
	SetCursorTo(c_usrxfil, uid);
    while(FetchCursor(c_usrxfil) != ERROR && i < FmFldLen(fm0, MULTI)) {
    	//seteo fecha de ultimo cierre
    	SetKey(operac|CIEFILbyACTIVA, FALSE, SFld(comerc|USRXFIL_FILIAL), MAX_DATE, MAX_DATE, MAX_TIME);
    	if (GetRecord(operac|CIEFILbyACTIVA, PARTIAL_KEY|PREV_KEY, IO_NOT_LOCK, 2) != ERROR)
    		FmSetDFld(fm0, FECCIE, DFld(operac|CIEFIL_FECCIE), i);
    	else
            continue;

    	//seteo filial y descripcion de filial
    	FmSetFld(fm0, FFILIAL, SFld(comerc|USRXFIL_FILIAL), i);
    	FmSetFld(fm0, DESCFIL, GetDescFilial(SFld(comerc|USRXFIL_FILIAL)), i);
            
    	//seteo revierte como false por default
    	FmSetIFld(fm0, REVIERTE, FALSE, i);
    	
    	i++;
    }
	
	DeleteCursor(c_usrxfil);	
}

