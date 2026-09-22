/********************************************************************
*
* MODULE & VERSION : @(#)cieroper.c	1.7
* DATE             : 05/03/30
* TIME             : 16:34:49
*
* CREATED          : 05/01/99
*
* DESCRIPTION:
*             Este proceso se encarga de realizar el cierre de operaciones.
*             Cambia al PARTE de OPERAC el estado de horas normales y extras
*             (al 50, 100 y feriado).
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "billpro.h"
#include "billpro.sch"
#include "operac.sch"
#include "cieroper.fmh"

#define ERR_PAR_PERI "No se han cargado los periodos para Hs.Normales y Hs.Extras."
#define ERR_SUP      "El periodo que se quiere cerrar se superpone con el cierre anterior!.\nDebe redefinir los días de los rangos. Para %s\nperiodo nuevo: %D - %D / periodo cerrado: %D - %D"
#define ERR_HUECO    "Entre el periodo que se quiere cerrar y el periodo anterior existe un\nrango de días que no pertenece a ningún cierre.\nDebe redefinir los días de los rangos."

// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);
static void ArmarFechas(int mesc, int anioc, DATE *fdnor, DATE *fhnor, DATE *fdex, DATE *fhex, DATE *fdret,
						DATE *fhret);
static void GuardarLog(int emp, int mesc, int anioc);
static void ValidarSuperposicion(DATE fdhnor, DATE fhhnor, DATE fdhex, DATE fhhex);
static bool Superpos(DATE fdesde, DATE fhasta, DATE fecdes, DATE fechas);

// Variables globales.
form fm0;
int  NROCIERRE;
DATE hoy, fdesde, fhasta, fdesnor, fhasnor, fdesext, fhasext;
TIME hora;

// Programa principal
wcmd(cieroper, 1.7 03/30/05)
{
	int  emp, mesc, anioc;
	DATE fecactual;

	fm0 = OpenForm("cieroper", FM_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	mesc    = FmIFld(fm0, MESC);
	anioc   = FmIFld(fm0, ANIOC);
	emp     = FmIFld(fm0, EMP);
	fdesnor = FmDFld(fm0, FECDNOR);
	fhasnor = FmDFld(fm0, FECHNOR);
	fdesext = FmDFld(fm0, FECDEX);
	fhasext = FmDFld(fm0, FECHEX);

	fdesde = fdesnor < fdesext ? fdesnor : fdesext;
	fhasta = fhasnor < fhasext ? fhasext : fhasnor;

	fecactual = GetFechaCierreOpe(emp);
	if (fhasta < fecactual && fecactual != NULL_DATE) {
		Error ("No se puede cerrar con fecha menor a la del ultimo cierre -%.3D-", fecactual);
	}

	if (FmIFld(fm0, ULTSEM)) {
		FmSetFld(fm0, COMENTARIO,  "Cerrando Operaciones Ultima Semana ...");
	}
	else {
		FmSetFld(fm0, COMENTARIO,  "Cerrando Operaciones ...");
	}

	WiRefresh();
	sleep(5);

	BeginTransaction();
	PutFechaCierreOpe(emp, fhasta);
	GuardarLog(emp, mesc, anioc);
	EndTransaction();

	CloseAllSchemas();
}

static void GuardarLog(int emp, int mesc, int anioc)
{
	FmToDb(fm0, EMP, FECHRET);
	SetDFld(CIERRE_FECCIE,   hoy);
	SetTFld(CIERRE_HORACIE,  hora);
	SetIFld(CIERRE_USUCIE,   GetUid());
	SetDFld(CIERRE_FECIMP,   NULL_DATE);
	SetTFld(CIERRE_HORAIMP,  NULL_TIME);
	SetLFld(CIERRE_USUIMP,   NULL_LONG);
	SetDFld(CIERRE_FECIMPA,  NULL_DATE);
	SetTFld(CIERRE_HORAIMPA, NULL_TIME);
	SetLFld(CIERRE_USUIMPA,  NULL_LONG);

	PutRecord(CIERRE);
	FreeTable(CIERRE);
}

static fm_status after(form fm, fmfield fno, int row)
{
	int  mesc = NULL_SHORT, anioc = NULL_SHORT, nrocier = NULL_SHORT;
	DATE fdhnor, fhhnor, fdhex, fhhex, fdhret, fhhret;

	switch (fno) {
		case ULTSEM :
			hoy  = Today();
			hora = Hour();
			SetKey(CIERREbyEMP, FmIFld(fm0, EMP), MAX_SHORT);
			if (GetRecord(CIERREbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
				/* No permite correr otro cierre, si el último corrido
				   es definitivo pero personal no importó los datos. */
				if (IFld(CIERRE_TIPOCIER) == DEFINITIVO) {
					if (IsNull(CIERRE_FECIMP)) {
						return FmErrMsg(fm, M_ERR_PERI_CERRADO, IFld(CIERRE_MESC), IFld(CIERRE_ANIOC));
					}
					else {
						mesc  = IFld(CIERRE_MESC) == 12 ? 1 : IFld(CIERRE_MESC) + 1;
						anioc = IFld(CIERRE_MESC) == 12 ? IFld(CIERRE_ANIOC) + 1 : IFld(CIERRE_ANIOC);
					}
				}
				else {
					mesc  = IFld(CIERRE_MESC);
					anioc = IFld(CIERRE_ANIOC);
				}
				nrocier = IFld(CIERRE_NROCIER) + 1;
			}
			else {
				nrocier = 1;
				mesc    = Month(hoy) == 1 ? 12 : Month(hoy);
				anioc   = Month(hoy) == 1 ? Year(hoy) - 1 : Year(hoy);
			}
			FmSetIFld(fm0, MESC,  mesc);
			FmSetIFld(fm0, ANIOC, anioc);
			FmSetIFld(fm0, I_NROCIER, nrocier);

			ArmarFechas(mesc, anioc, &fdhnor, &fhhnor, &fdhex, &fhhex, &fdhret, &fhhret);
			ValidarSuperposicion(fdhnor, fhhnor, fdhex, fhhex);
			FmSetDFld(fm0, FDESNOR,   fdhnor);
			FmSetDFld(fm0, FHASNOR,   fhhnor);
			FmSetDFld(fm0, FDESEX,    fdhex);
			FmSetDFld(fm0, FHASEX,    fhhex);
			FmSetDFld(fm0, I_FDESRET, fdhex);
			FmSetDFld(fm0, I_FHASRET, fhhex);
			break;
		case TIPOCIER :
			FmSetDFld(fm, FECDNOR, FmDFld(fm, FDESNOR));
			FmSetDFld(fm, FECHNOR, FmDFld(fm, FHASNOR));
			FmSetDFld(fm, FECDEX,  FmDFld(fm, FDESEX));
			FmSetDFld(fm, FECHEX,  FmDFld(fm, FHASEX));
			FmSetDFld(fm, FECDRET, FmDFld(fm, I_FDESRET));
			FmSetDFld(fm, FECHRET, FmDFld(fm, I_FHASRET));
	}
	return FM_OK;
}

static void ArmarFechas(int mesc, int anioc, DATE *fdnor, DATE *fhnor, DATE *fdex, DATE *fhex,
						DATE *fdret, DATE *fhret)
{
	// leo los parámetros que definen los rangos para hs.normales y extras.
	SetKey(PERIODOSbyEMP, FmIFld(fm0, EMP));
	if (GetRecord(PERIODOSbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		WiMsg(ERR_PAR_PERI);
		Stop(0);
	}
	// PARA HORAS NORMALES:
	// si eligió periodo completo, toma del 1 al último día del mes, del mes y año elegidos.
	if (IFld(PERIODOS_MESCOMPN)) {
		(*fdnor) = DMYToD(1, mesc, anioc);
		(*fhnor) = LastMonthDay((*fdnor));
	}
	else {
		int mesn, anion;
		mesn     = ((mesc - 1) == 0) ? 12 : mesc - 1;
		anion    = ((mesc - 1) == 0) ? anioc - 1 : anioc;
		(*fdnor) = DMYToD(IFld(PERIODOS_DDESNOR), mesn, anion);

		// lo sgte está por si pusieron un día que no todos los meses tienen. En consecuencia
		// DMYToD devuelve NULL_DATE => pongo el último día del mes-año.
		// esto es lo que quiere decir el desc de periodos.fm.
		if ((*fdnor) == NULL_DATE)
			(*fdnor) = LastMonthDay(DMYToD(1, mesn, anion));

		if (!FmIFld(fm0, ULTSEM))
			(*fhnor) = DMYToD(IFld(PERIODOS_DHASNOR), mesc, anioc);
		else
			(*fhnor) = LastMonthDay(DMYToD(1, mesn, anion));

		if ((*fhnor) == NULL_DATE)
			(*fhnor) = LastMonthDay(DMYToD(1, mesc, anioc));
	}

	// PARA HORAS EXTRAS:
	// si eligió periodo completo, toma del 1 al último día del mes, del mes y año elegidos.
	if (IFld(PERIODOS_MESCOMPE)) {
		(*fdex) = DMYToD(1, mesc, anioc);
		(*fhex) = LastMonthDay((*fdex));
	}
	else {
		// si no eligió periodo completo, toma el día desde de la tabla pero del mes y año anterior
		// al solicitado por el form., para la fecha desde.
		// para la fecha hasta, toma el día hasta de la tabla pero con el mes y año pedidos por el form.
		int	mesn, anion;
		mesn    = ((mesc - 1) == 0) ? 12 : mesc - 1;
		anion   = ((mesc - 1) == 0) ? anioc - 1 : anioc;
		(*fdex) = DMYToD(IFld(PERIODOS_DDESEX), mesn, anion);
		if ((*fdex) == NULL_DATE)
			(*fdex) = LastMonthDay(DMYToD(1, mesn, anion));

		(*fhex) = DMYToD(IFld(PERIODOS_DHASEX), mesc, anioc);

		if (!FmIFld(fm0, ULTSEM))
			(*fhex) = DMYToD(IFld(PERIODOS_DHASNOR), mesc, anioc);
		else
			(*fhex) = LastMonthDay(DMYToD(1, mesn, anion));

		if ((*fhex) == NULL_DATE)
			(*fhex) = LastMonthDay(DMYToD(1, mesc, anioc));
	}

	// PARA HORAS RETROACTIVAS:
	// si eligió periodo completo, toma del 1 al último día del mes, del mes y año elegidos.
	if (IFld(PERIODOS_MESCOMPN)) {
		(*fdret) = DMYToD(1, mesc, anioc);
		(*fhret) = LastMonthDay((*fdret));
	}
	else {
		int mesn, anion;
		mesn     = ((mesc - 1) == 0) ? 12 : mesc - 1;
		anion    = ((mesc - 1) == 0) ? anioc - 1 : anioc;
		(*fdret) = DMYToD(IFld(PERIODOS_DDESEX), mesn, anion);
		if ((*fdret) == NULL_DATE)
			(*fdret) = LastMonthDay(DMYToD(1, mesn, anion));

		(*fhret) = DMYToD(IFld(PERIODOS_DHASEX), mesc, anioc);

		if (!FmIFld(fm0, ULTSEM))
			(*fhret) = DMYToD(IFld(PERIODOS_DHASNOR), mesc, anioc);
		else
			(*fhret) = LastMonthDay(DMYToD(1, mesn, anion));

		if ((*fhret) == NULL_DATE)
			(*fhret) = LastMonthDay(DMYToD(1, mesc, anioc));
	}
}

static void ValidarSuperposicion(DATE fdhnor, DATE fhhnor, DATE fdhex, DATE fhhex)
{
	bool encontre = FALSE;

	// leo los parámetros que definen los rangos para hs.normales y extras.
	SetKey(CIERREbyEMP, FmIFld(fm0, EMP), MAX_SHORT);
	while (!encontre &&	GetRecord(CIERREbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
		if (IFld(CIERRE_TIPOCIER) == DEFINITIVO) {
			encontre = TRUE;
		}
	}
	if (!encontre) return;

	// llega aca solo por cierres definitivos y pasados a denarius!
	if (Superpos(fdhnor, fhhnor, DFld(CIERRE_FDESNOR), DFld(CIERRE_FHASNOR))) {
		WiMsg(ERR_SUP, "Horas Normales", fdhnor, fhhnor, DFld(CIERRE_FDESNOR), DFld(CIERRE_FHASNOR));
		Stop(0);
	}
	if (Superpos(fdhex,  fhhex,  DFld(CIERRE_FDESEX),  DFld(CIERRE_FHASEX))) {
		WiMsg(ERR_SUP, "Horas Extras", fdhex,  fhhex,  DFld(CIERRE_FDESEX),  DFld(CIERRE_FHASEX));
		Stop(0);
	}
	if (fdhnor != (DFld(CIERRE_FHASNOR) + 1) ||	fdhex != (DFld(CIERRE_FHASEX) + 1)) {
		WiMsg(ERR_HUECO);
		Stop(0);
	}
}

static bool Superpos(DATE fdesde, DATE fhasta, DATE fecdes, DATE fechas)
{
	/* valido si la fecha desde esté dentro del rango de fechas existente.
	   ej.: rango del cierre de alta: 5-10 o bien 5-6 |  cierre anterior: 4-9 (<-expresado en dias) */
	if (fdesde >= fecdes && fdesde <= fechas) {
		return TRUE;
	}
	/* valido si la fecha hasta esté dentro del rango de fechas existente.
	   ej.: rango del cierre de alta: 5-10 o bien 7-9 |  cierre anterior: 6-12 (<-expresado en dias) */
	if (fhasta >= fecdes && fhasta <= fechas) {
		return TRUE;
	}
	/* valido si la fecha desde del cierre existente esté dentro del rango de fechas existente.
	   ej.: rango del cierre de alta: 2-15 |  cierre anterior: 4-9 (<-expresado en dias) */
	if (fecdes >= fdesde && fecdes <= fhasta) {
		return TRUE;
	}
	return FALSE;
}

