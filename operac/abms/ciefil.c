/********************************************************************
* MODULE & VERSION : @(#)ciefil.c	1.2
* DATE             : 08/11/10
* TIME             : 14:46:30
*
* CREATED          : 22/04/98
*
* DESCRIPTION:
*      Cierre por Filial
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.sch"
#include "operac.sch"
#include "ciefil.fmh"
#include "filial.h"

/* Funciones privadas */
static void SetMulti();
static fm_status after(form fm, fmfield fno, int row);

/* Declaraciones globales */
schema comerc, operac;
form fm0;
int i;

/* Programa principal */
wcmd(ciefil, 1.2 11/10/08)
{
	fm_cmd cmd;
	fm0 = OpenForm("ciefil", FM_EABORT);

	operac = OpenSchema("operac", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	SetMulti();
	
	while ((cmd = DoForm(fm0, NULLFP, after))  != FM_EXIT)
	switch (cmd) {
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		
		for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, FFILIAL, i); i++) {
			if (FmIFld(fm0, CIERRA) == TRUE) {
				SetFld(operac|CIEFIL_FILIAL, FmSFld(fm0, FFILIAL, i));
				SetDFld(operac|CIEFIL_FECCIE, FmDFld(fm0, FECCIE, i));
				SetLFld(operac|CIEFIL_IDCARGA, (long)GetUid());

				SetDFld(operac|CIEFIL_FECCARGA, Today());
				SetTFld(operac|CIEFIL_HORACARGA, Hour());
				SetIFld(operac|CIEFIL_REVERTIDO, FALSE);
				
				SetLFld(operac|CIEFIL_IDREVER, NULL_LONG);
				SetDFld(operac|CIEFIL_FECREVER, NULL_DATE);
				SetTFld(operac|CIEFIL_HORAREVER, NULL_TIME);
				
				PutRecord(operac|CIEFIL);
			}	
		}	 
		
		EndTransaction();

		SetMulti();
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
    	//seteo filial y descripcion de filial
    	FmSetFld(fm0, FFILIAL, SFld(comerc|USRXFIL_FILIAL), i);
    	FmSetFld(fm0, DESCFIL, GetDescFilial(SFld(comerc|USRXFIL_FILIAL)), i);
    	
    	//seteo fecha de ultimo cierre
    	SetKey(operac|CIEFILbyACTIVA, FALSE, SFld(comerc|USRXFIL_FILIAL), MAX_DATE, MAX_DATE, MAX_TIME);
    	if (GetRecord(operac|CIEFILbyACTIVA, PARTIAL_KEY|PREV_KEY, IO_NOT_LOCK, 2) != ERROR)
    		FmSetDFld(fm0, FECULT, DFld(operac|CIEFIL_FECCIE), i);
    	else
            FmSetDFld(fm0, FECULT, NULL_DATE, i);

            
    	//seteo cierra como false por default, y fecha de cierre con fecha de hoy como default
    	FmSetIFld(fm0, CIERRA, FALSE, i);
    	FmSetDFld(fm0, FECCIE, Today(), i);
    	
    	i++;
    }
	
	DeleteCursor(c_usrxfil);	
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FECCIE:
		if (!FmIsNull(fm, FECULT, row) && FmDFld(fm, fno, row) < FmDFld(fm, FECULT, row)) {
			Warning("La Fecha de Cierre de la Filial %s debe ser Mayor a la última Fecha de Cierre", FmSFld(fm, FFILIAL, row));
			return FM_REDO;
		}	
		break;
	}
	return FM_OK;				
}

