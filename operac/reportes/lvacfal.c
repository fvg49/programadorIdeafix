/********************************************************************
*
* MODULE & VERSION : @(#)lvacfal.c	1.2
* DATE             : 10/04/06
* TIME             : 17:48:55
*
* CREATED          : 24/02/05
*
* DESCRIPTION:
*       Listado que indica los Vigiladores que la faltan tomar vacaciones.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "filial.h"
#include "sue.sch"
#include "operac.sch"
#include "lvacfal.fmh"
#include "lvacfal.rph"

/* Funciones privadas */
static void PrintHead(void);
static void Imprimir();

/* Declaraciones globales */
FILE *fp1 = NULL;
form fm0;
report rp = ERROR;
schema sue, oper;
int anioingreso, periodo;
int  g_emp;

/* Programa principal */
wcmd(lvacfal, 1.4 02/13/07)
{
	dbcursor c_VAC = (dbcursor) ERROR;
	dbcursor c_PER = (dbcursor) ERROR;
	DATE fecierre;

	fm0  = OpenForm  ("lvacfal", FM_EABORT);
	sue  = OpenSchema("sue",     IO_EABORT);
	oper = OpenSchema("operac",  IO_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	g_emp= StrToI(getenv("emp"));
	fecierre  = GetFechaCierreOpe(g_emp) - 30;
	InicLegajoXusr (g_emp, fecierre, fm0, MENSAJE, TRUE, MAX_SHORT);

	/*
	
	g_emp= StrToI(getenv("emp"));
	fecierre  = GetFechaCierreOpe(g_emp);

	InicLegajoXusr (g_emp, fecierre, fm0, DPROCVIG, TRUE, MAX_SHORT);
	WiRefresh();
		
	*/
	
	
	
	
	
	c_PER = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	c_VAC = CreateCursor(oper|VACACbyPERIODO, IO_NOT_LOCK);

	if (*FmSFld(fm0, SALIDA) == 'A') {
		if ((fp1 = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
			Error("No se puede abrir el archivo de salida.");

		PrintHead();
	}
	else {
		if (rp == ERROR) {
			rp = OpenReport("lvacfal", RP_EABORT|RP_NOBEGIN);
			RpSetOutput(rp, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
			BeginReport(rp, 1, NULL_STR);
			PrintHead();
		}
	}

	if (FmIsNull(fm0, PERIODO))
		periodo = Year(Today());
	else
		periodo = FmIFld(fm0, PERIODO);

	SetCursorFrom(c_PER, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGD));
	SetCursorTo  (c_PER, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGH));
	while (FetchCursor(c_PER) != ERROR) {

		if(!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)) )
			continue;

		if (LFld(sue|PER_CODCCOS) >= 10000 && LFld(sue|PER_CODCCOS) < 100000)
			continue;

		if (!IsNull(sue|PER_FECEGR))
			continue;

		if (Year(DFld(sue|PER_FECING)) > periodo)
			continue;

		if (!ValidaLegajoXusr(LFld(sue|PER_NROLEG), MAX_DATE))
			continue;

		if (FmIsNull(fm0, PERIODO)) {
			//En el sistema se comenzo a cargar vacaciones a partir de 1999.
			if (Year(DFld(sue|PER_FECING)) <= 1998)
				anioingreso = 1999;
			else
				anioingreso = Year(DFld(sue|PER_FECING));
		}
		else
			anioingreso = FmIFld(fm0, PERIODO);

		for (; anioingreso <= periodo; anioingreso++) {
			SetIFld(oper|VACAC_EMP,     FmIFld(fm0, EMP));
			SetLFld(oper|VACAC_NROLEG,  LFld(sue|PER_NROLEG));
			SetIFld(oper|VACAC_PERIODO, anioingreso);
			SetDFld(oper|VACAC_FDESDE,  MAX_DATE);
			if (GetRecord(oper|VACAC_EMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR) {
				Imprimir();
			}
		}
	}
	if (rp == ERROR && fp1 == NULL)
		Warning("No hay datos para emitir el listado.");
}

static void PrintHead(void)
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp1, "Vigilador\tNombre\tFecha Ingreso\tPeriodo\n");
	}
	else {
		RpSetLFld(rp, RNROLEGD, FmLFld(fm0, NROLEGD));
		RpSetFld (rp, RAPYNOMD, FmSFld(fm0, APYNOMD));
		RpSetLFld(rp, RNROLEGH, FmLFld(fm0, NROLEGH));
		RpSetFld (rp, RAPYNOMH, FmSFld(fm0, APYNOMH));
	}
}

static void Imprimir()
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp1, "%ld\t%s\t%.3D\t%d\n", LFld(sue|PER_NROLEG), SFld(sue|PER_APYNOM),
											DFld(sue|PER_FECING), anioingreso);
	}
	else {
		RpSetLFld(rp, RNROLEG,   LFld(sue|PER_NROLEG));
		RpSetFld (rp, RAPYNOM,   SFld(sue|PER_APYNOM));
		RpSetIFld(rp, RPERIODO,  anioingreso);
		RpSetDFld(rp, RFINGRESO, DFld(sue|PER_FECING));
		RpSetDFld(rp, RFEGRESO,  DFld(sue|PER_FECEGR));
		DoReport (rp, ZLINEA);
	}
}
