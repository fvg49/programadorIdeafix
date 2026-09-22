/********************************************************************
* MODULE & VERSION : @(#)cierpar.c	1.2
* DATE             : 07/04/10
* TIME             : 16:18:38
*
* CREATED          : 08/03/07
*
* DESCRIPTION:
*             Realiza cierre parciales del parte para no permitir
*             cargas de novedades.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "billpro.h"
#include "billpro.sch"
#include "cierpar.fmh"

// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);

//Variables globales
schema billp;
form fm0;
char fecpar[15];

// Programa principal
wcmd(cierpar, 1.2 04/10/07)
{
	fm0   = OpenForm("cierpar", FM_EABORT);
	billp = OpenSchema("billpro", IO_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	if (FmIFld(fm0, EMP) == _EMP_PSA) {
		DToStr(FmDFld(fm0, FECHA), fecpar, DFMT_SEPAR);
		PutValParam(MOD_VIGI, FEC_CIERRE_OPERAC_PARCIAL, fecpar);
	}
	else {
		DToStr(FmDFld(fm0, FECHA), fecpar, DFMT_SEPAR);
		PutValParam(MOD_VIGI, FEC_CIERRE_OPERAC_SAPE_PARCIAL, fecpar);
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	int param;
	DATE fecha;

	switch (fno) {
	case EMP :
		if (FmIFld(fm0, EMP) == _EMP_PSA) 
			param = FEC_CIERRE_OPERAC_PARCIAL;
		else
			param = FEC_CIERRE_OPERAC_SAPE_PARCIAL;

		SetIFld(PARPRO_CODMOD, MOD_VIGI);
		SetIFld(PARPRO_CODPAR, param);
		if (GetRecord(PARPRObyCODMOD, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			fecha = StrToD(SFld(PARPRO_VALOR));
			FmSetDFld(fm, FECHA, fecha);
		}
	break;
	}
	return FM_OK;
}
