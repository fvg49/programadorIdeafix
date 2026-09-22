/********************************************************************
*
* MODULE & VERSION : @(#)dasiprov.c	1.1 
* DATE             : 06/05/16 
* TIME             : 09:46:58 
*
* CREATED          : 
*
* DESCRIPTION:
*             Desasignación de Vigiladores Provisorios.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "operac.sch"
#include "dasiprov.fmh"

/* Declaraciones globales */
form fm0;
dbcursor c_asig;
int emp;

/* Programa principal */
wcmd(dasiprov, 1.1 05/16/06)
{
	fm_cmd cmd;

	fm0    = OpenForm("dasiprov", FM_EABORT);
	c_asig = CreateCursor(ASIGbyEMP, IO_NOT_LOCK);

	emp = StrToI(ReadEnv("Nroempresa"));

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();

//		SetCursorFrom(c_asig, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, MIN_LONG);
//		SetCursorTo  (c_asig, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_LONG);

		SetCursorFrom(c_asig, emp, MIN_LONG, MIN_SHORT, MIN_LONG);
		SetCursorTo  (c_asig, emp, MAX_LONG, MAX_SHORT, MAX_LONG);

		while (FetchCursor(c_asig) != ERROR) {
			if (!strcmp(SFld(ASIG_EFECT), EFECTIVO))
				continue;
			if (DFld(ASIG_FECHAS) > FmDFld(fm0, FECHA))
				continue;

			InitRecord(ASIGH);
			SetIFld(ASIGH_EMP,      IFld(ASIG_EMP));
			SetLFld(ASIGH_CLIENTE,  LFld(ASIG_CLIENTE));
			SetIFld(ASIGH_OBJETIVO, IFld(ASIG_OBJETIVO));
			SetIFld(ASIGH_PTOSER,   IFld(ASIG_PTOSER));
			SetIFld(ASIGH_PUESTO,   IFld(ASIG_PUESTO));
			SetLFld(ASIGH_NROLEG,   LFld(ASIG_NROLEG));
			SetFld (ASIGH_VIGIL,    SFld(ASIG_VIGIL));
			SetFld (ASIGH_EFECT,    SFld(ASIG_EFECT));
			SetDFld(ASIGH_FECALT,   DFld(ASIG_FECASIG));
			SetDFld(ASIGH_FECBAJ,   DFld(ASIG_FECHAS));
			SetTFld(ASIGH_HSENT,    TFld(ASIG_HSENT));
			SetTFld(ASIGH_HSSAL,    TFld(ASIG_HSSAL));
			SetFld (ASIGH_DIA1,     SFld(ASIG_DIA1));
			SetFld (ASIGH_DIA2,     SFld(ASIG_DIA2));
			SetFld (ASIGH_DIA3,     SFld(ASIG_DIA3));
			SetFld (ASIGH_DIA4,     SFld(ASIG_DIA4));
			SetFld (ASIGH_DIA5,     SFld(ASIG_DIA5));
			SetFld (ASIGH_DIA6,     SFld(ASIG_DIA6));
			SetFld (ASIGH_DIA7,     SFld(ASIG_DIA7));
			SetIFld(ASIGH_MOTIVO,   DESAUT);
			SetFld (ASIGH_REGIM,    SFld(ASIG_REGIM));
			SetDFld(ASIGH_FECHAS,   DFld(ASIG_FECHAS));
			SetDFld(ASIGH_FFRANCO,  DFld(ASIG_FFRANCO));
			PutRecord(ASIGH);

			DelRecord(ASIG);
		}
		DeleteCursor(c_asig);
		EndTransaction();
		break;
	}
}
