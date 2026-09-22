/*********************************************************************
* MODULO & VERSION : @(#)paseaur.c	1.1
* DATE             : 00/12/12
* TIME             : 17:02:03
*
* DESCRIPCION      : Pase de horas a Administración.
*
** TABLA         |  OPERACION
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "paseaur.fmh"
#include "prosegur.sch"
#include "sue.sch"
#include "operac.sch"

#define ERR_LEG            "No existe el empleado perteneciente a la Empresa: %d con Legajo : %ld"
#define WAR_PERI_PASADO    "El periodo asociado a esta liquidación ya fue pasado.\nNo se puede volver a generar."
#define ERR_PERI_BORR      "El periodo de referencia del Sist. Novia: %d / %d de la Empresa: %d no existe."
#define ERR_NO_EXISTE_PERI "No existe cierre en Novia correspondiente al mes-año de la liquidación ingresada."
#define ERR_NO_PASO_PERI_ANT "Falta procesar el periodo anterior."

/* ++ Prototypes ++ */
private fm_status after(form fm, fmfield fno, int row);
static void CargarPaseCCte(int emp, int mesc, int anioc, DATE fdesnor, DATE fhasnor, DATE fdesext,
						   DATE fhasext);
static void GuardarDatos(int emp, int mes, int ano, long nroleg, double hsextras50, double hsextras100,
						 double hsextrasfer, double hsnormales, short diasvaca, long cliente, int objet);
static void LeerLegajo(int emp, long nroleg);
static void GuardarLog(int emp, int nrocier, int mesc, int anioc);

/* Declaraciones globales */
schema prosegur, operac, sue;
form fm;
dbcursor curparte;
private int lec_modo;

wcmd(paseaur, 1.1 12/12/00)
{
	fm_cmd cmd;
	long nroliq;
	int  emp, mesc, anioc;
	DATE fdesnor, fhasnor, fdesext, fhasext;

	fm       = OpenForm("paseaur",    FM_EABORT);
	sue      = OpenSchema("sue",      IO_EABORT);
	operac   = OpenSchema("operac",   IO_EABORT);
	prosegur = OpenSchema("prosegur", IO_EABORT);
	curparte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);

	while ((cmd = DoForm(fm, NULLFP, after)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			continue;

		nroliq  = FmLFld(fm, NROLIQ);
		mesc    = FmIFld(fm, I_MESC);
		anioc   = FmIFld(fm, I_ANIOC);
		emp     = FmIFld(fm, I_EMP);
		fdesnor = FmDFld(fm, FDNORM);
		fhasnor = FmDFld(fm, FHNORM);
		fdesext = FmDFld(fm, FDEXTR);
		fhasext = FmDFld(fm, FHEXTR);

		BeginTransaction();
		GuardarLog(emp, FmIFld(fm, I_NROCIER), mesc, anioc);
		CargarPaseCCte(emp, mesc, anioc, fdesnor, fhasnor, fdesext, fhasext);
		EndTransaction();
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	int mes, anio;

	switch (fno) {
		case NROLIQ :
			// si la Liq está confirmada ya no se puede hacer nada!!!:
/*			if (FmIFld(fm, I_CONF)) {
				FmErrMsg(fm, M_LIQ_CONF);
			}
*/
			// leo el periodo ingresado:
			SetKey(operac|CIERREbyEMP, FmIFld(fm, I_EMP), MAX_SHORT);
			if (GetRecord(operac|CIERREbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) == ERROR) {
				return FmErrMsg(fm, M_ERR_PERI_INEX);
			}

			SetKey(operac|CIERREbyEMP, FmIFld(fm, I_EMP), MIN_SHORT);
			while (GetRecord(operac|CIERREbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR &&
																!IsNull(operac|CIERRE_FECIMPA)) {
				continue;
			}
			if (IFld(operac|CIERRE_TIPOCIER) == DEFINITIVO && !IsNull(operac|CIERRE_FECIMPA)) {
				Warning(WAR_PERI_PASADO);
				Stop(0);
			}
			if (IFld(operac|CIERRE_MESC)  < Month(FmDFld(fm, I_FLIQ)) ||
				IFld(operac|CIERRE_ANIOC) < Year(FmDFld(fm, I_FLIQ))) {
				WiMsg(ERR_NO_PASO_PERI_ANT);
				Stop(0);
			}
			if (IFld(operac|CIERRE_MESC)  > Month(FmDFld(fm, I_FLIQ)) ||
				IFld(operac|CIERRE_ANIOC) > Year(FmDFld(fm, I_FLIQ))) {
				WiMsg(ERR_NO_EXISTE_PERI);
				Stop(0);
			}
			DbToFm(fm, 0, CONTROL_FLD);
			break;
	}
	return FM_OK;
}

static void CargarPaseCCte(int emp, int mesc, int anioc, DATE fdesnor, DATE fhasnor, DATE fdesext, DATE fhasext)
{
	double hsextras50 = 0.0, hsextras100 = 0.0, hsextrasfer = 0.0, hsnormales = 0.0;
	long   cliente;
	int    objetivo, diasvaca;

	// si no hay errores vuelvo a procesar todo pero lleno paseccte.
	SetCursorFrom(curparte, emp, MIN_LONG, MIN_DATE, MIN_SHORT);
	SetCursorTo  (curparte, emp, MAX_LONG, MAX_DATE, MAX_SHORT);
	while (FetchCursor(curparte) != ERROR) {

		// no proceso los partes que no entren en alguno de los rangos de los
		// periodos:
		if ((DFld(operac|PARTE_DIA) < fdesnor || DFld(operac|PARTE_DIA) > fhasnor) &&
			(DFld(operac|PARTE_DIA) < fdesext || DFld(operac|PARTE_DIA) > fhasext))
			continue;

		if (DFld(operac|PARTE_DIA) >= fdesext && DFld(operac|PARTE_DIA) <= fhasext) {
			hsextras50  = FFld(operac|PARTE_HS50);
			hsextras100 = FFld(operac|PARTE_HS100F);
			hsextrasfer = FFld(operac|PARTE_HS100FE);
		}
		if (DFld(operac|PARTE_DIA) >= fdesnor && DFld(operac|PARTE_DIA) <= fhasnor) {
			hsnormales =  FFld(operac|PARTE_HSNOR);
		}
		diasvaca = Vacaciones(emp, LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));

		GuardarDatos(emp, mesc, anioc, LFld(operac|PARTE_NROLEG), hsextras50, hsextras100, hsextrasfer,
					 hsnormales, diasvaca, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
		hsextras50  = 0.0;
		hsextras100 = 0.0;
		hsextrasfer = 0.0;
		hsnormales  = 0.0;
	}
}

static void GuardarDatos(int emp, int mesc, int anioc, long nroleg, double hsextras50, double hsextras100,
						 double hsextrasfer, double hsnormales, short diasvaca, long cliente, int objet)
{
	LeerLegajo(emp, nroleg);

	SetKey(TMPOPERAbyNROLIQ, FmLFld(fm, NROLIQ), nroleg, cliente, objet);
	if (GetRecord(TMPOPERAbyNROLIQ, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		SetLFld(TMPOPERA_NORMALES,   LFld(TMPOPERA_NORMALES)   + hsnormales);
		SetLFld(TMPOPERA_EXTRAS50,   LFld(TMPOPERA_EXTRAS50)   + hsextras50);
		SetLFld(TMPOPERA_EXTRAS100,  LFld(TMPOPERA_EXTRAS100)  + hsextras100);
		SetLFld(TMPOPERA_FERIADO,    LFld(TMPOPERA_FERIADO)    + hsextrasfer);
		SetIFld(TMPOPERA_VACACIONES, IFld(TMPOPERA_VACACIONES) + diasvaca);
	}
	else {
		SetLFld(TMPOPERA_NORMALES,   hsnormales);
		SetLFld(TMPOPERA_EXTRAS50,   hsextras50);
		SetLFld(TMPOPERA_EXTRAS100,  hsextras100);
		SetLFld(TMPOPERA_FERIADO,    hsextrasfer);
		SetIFld(TMPOPERA_VACACIONES, diasvaca);
	}
	SetLFld(TMPOPERA_CCOSTO,  LFld(sue|PER_CODCCOS));
	SetLFld(TMPOPERA_NROCCTE, ArmarCCosto(cliente, objet));
	PutRecord(TMPOPERA);
}

static void LeerLegajo(int emp, long nroleg)
{
	SetKey(sue|PERbyEMP, emp, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
		Warning(ERR_LEG, emp, nroleg);
}

static void GuardarLog(int emp, int nrocier, int mesc, int anioc)
{
	SetKey(operac|CIERREbyEMP, emp, nrocier);
	if (GetRecord(operac|CIERREbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		return Error(ERR_PERI_BORR, mesc, anioc, emp);
	}
	SetDFld(operac|CIERRE_FECIMPA,  Today());
	SetTFld(operac|CIERRE_HORAIMPA, Hour());
	SetIFld(operac|CIERRE_USUIMPA,  GetUid());
	PutRecord(operac|CIERRE);
}
