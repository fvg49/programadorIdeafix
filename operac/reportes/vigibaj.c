/********************************************************************
*
* MODULE & VERSION : @(#)vigibaj.c	1.1
* DATE             : 04/07/15
* TIME             : 11:03:13
*
* CREATED          : 14/07/04
*
* DESCRIPTION:
*				Listado de Control de Vigiladores de Baja.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*
*********************************************************************/
#include <ideafix.h>
#include "vigibaj.fmh"
#include "vigibaj.rph"
#include "operac.sch"
#include "sue.sch"

#define INACTIVO 0

// Declaraciones globales
form   fm0;
schema operac, sue;
report rp0 = ERROR;

/* Programa principal */
wcmd(vigibaj, 1.1 07/15/04)
{
	dbcursor c_asig;

	sue    = OpenSchema("sue",     IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);
	fm0    = OpenForm  ("vigibaj", FM_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) == FM_EXIT) return;

	c_asig = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), MIN_LONG, MIN_SHORT,
																				  MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), MAX_LONG, MAX_SHORT,
																				  MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		SetIFld(sue|PER_EMP,    FmIFld(fm0, EMP));
		SetLFld(sue|PER_NROLEG, LFld(operac|ASIG_NROLEG));
		if ((GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) && IFld(sue|PER_ACTIVO) == INACTIVO) {
			if (rp0 == ERROR) {
		 		rp0 = OpenReport("vigibaj", RP_EABORT|RP_NOBEGIN);
				RpSetOutput(rp0, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
				BeginReport(rp0, 1, NULL_STR);
			}
			if (rp0 != ERROR) {
				RpSetLFld(rp0, R_LEGAJO,   LFld(operac|ASIG_NROLEG));
				RpSetFld (rp0, R_APENOM,   SFld(sue|PER_APYNOM));
				RpSetLFld(rp0, R_CLIENTE,  LFld(operac|ASIG_CLIENTE));
				RpSetIFld(rp0, R_OBJETIVO, IFld(operac|ASIG_OBJETIVO));
				RpSetDFld(rp0, R_FECEGR,   DFld(sue|PER_FECEGR));
				DoReport(rp0, LINEA);
			}
		}
	}
	DeleteCursor(c_asig);

	if (rp0 != ERROR)
		CloseReport(rp0);
	else
		Warning("No hay datos para emitir el listado.");
}
