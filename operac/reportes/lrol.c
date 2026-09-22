/********************************************************************
*
* MODULE & VERSION : @(#)lrol.c	1.2 
* DATE             : 05/12/01 
* TIME             : 11:08:11 
*
* CREATED          : 06/08/2001
*
* DESCRIPTION:
*              Impresión de Roles
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "operac.sch"
#include "lrol.fmh"
#include "lrol.rph"

/* Funciones privadas */
void GenerarReporte();
void ImprimirReporte();
private void AbrirReporte();

/* Declaraciones globales */
form fm0;
report rp0;

/* Programa principal */
wcmd(lrol, 1.2 12/01/05)
{
	fm_cmd cmd;

	fm0 = OpenForm("lrol", FM_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();
		GenerarReporte();
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
}

void GenerarReporte()
{
	AbrirReporte();
	ImprimirReporte();
	CloseReport(rp0);
}

void ImprimirReporte()
{
	int i = 0, j = 0, t;
	long campo;
	bool primeravez = TRUE;
	char vigi[14], val[32], aux[2];
	dbcursor c_rrol;

	c_rrol = CreateCursor(RROLbyCODROL, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(c_rrol, FmIFld(fm0, ROLD), MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_rrol, FmIFld(fm0, ROLH), MAX_SHORT, MAX_SHORT);
	while ((campo = FetchCursor(c_rrol)) != ERROR) {
		if (campo <= 2 && !primeravez) {
			for (t = i; t < 31; ) {
				for (j = 0; j < i && t < 31; j++, t++) {
					sprintf(aux,"%c", val[j]);
					RpSetFld(rp0, C1 + t, aux);
				}
			}
			DoReport(rp0, LIN);
			RpClearZone(rp0, LIN);
			i = 0;
		}
		primeravez = FALSE;
		RpSetIFld(rp0, R_ROL,  IFld(RROL_CODROL));
		RpSetFld (rp0, R_DROL, GetRolDesc(IFld(RROL_CODROL)));

		sprintf(vigi, "Ciclo %d", IFld(RROL_FILA));
		RpSetFld(rp0, R_VIGI, vigi);

		RpSetFld(rp0, C1 + i, SFld(RROL_VALOR));
		val[i] = *SFld(RROL_VALOR);

		i++;
	}
	for (t = i; t < 31; ) {
		for (j = 0; j < i && t < 31; j++, t++) {
			sprintf(aux,"%c", val[j]);
			RpSetFld(rp0, C1 + t, aux);
		}
	}
	DoReport(rp0, LIN);
	RpClearZone(rp0, LIN);
}

private void AbrirReporte()
{
	rp0 = OpenReport("lrol", RP_EABORT|RP_NOBEGIN);
	RpSetOutput(rp0, *FmSFld(fm0,SALIDA) == 'I' ? RP_IO_DEFAULT : RP_IO_TERM, NULL_STR);
	BeginReport(rp0, 1, NULL_STR);
}
