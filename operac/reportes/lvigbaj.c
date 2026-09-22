/********************************************************************
* MODULE & VERSION : @(#)lvigbaj.c
* DATE             :
* TIME             :
*
* CREATED          :
*
* DESCRIPTION:
*
*********************************************************************/
#include <ideafix.h>
#include "lvigbaj.fmh"
#include "lvigbaj.rph"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "bill.sch"

// Declaraciones globales
form   fm0;
schema operac, bill, sue;
report rp0 = ERROR;

/* Programa principal */
wcmd(lvigbaj, 1.12  23/09/98)
{
	fm_status cmd;
	dbcursor c_asig;

	bill   = OpenSchema("bill",    IO_EABORT);
	sue    = OpenSchema("sue",     IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);
	fm0    = OpenForm  ("lvigbaj", FM_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	rp0 = OpenReport("lvigbaj", RP_EABORT|RP_NOBEGIN);

		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
/*	if (*FmSFld(fm0, SALIDA) == 'I')
	if (*FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
*/
	BeginReport(rp0, 1, NULL_STR);

	c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), MIN_LONG, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), MAX_LONG, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		SetIFld(sue|PER_EMP,    FmIFld(fm0, EMP));
		SetLFld(sue|PER_NROLEG, LFld(operac|ASIG_NROLEG));
		if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR && IFld(sue|PER_ACTIVO) == 0) {
			RpSetLFld(rp0, R_LEGAJO, LFld(operac|ASIG_NROLEG));
			RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG)));
			RpSetLFld(rp0, R_CLI,    LFld(operac|ASIG_CLIENTE));
			RpSetIFld(rp0, R_OBJ,    IFld(operac|ASIG_OBJETIVO));
			RpSetFld (rp0, R_DOBJ,   GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
			RpSetDFld(rp0, R_FECBAJ, DFld(sue|PER_FECEGR)); 

			SetKey(bill|CLIENTEbyCLIENTE, LFld(operac|ASIG_CLIENTE));
			GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
			RpSetFld(rp0, R_DCLI, SFld(bill|CLIENTE_RAZSOC));
			DoReport(rp0, LINEA);
		}
	}
}
