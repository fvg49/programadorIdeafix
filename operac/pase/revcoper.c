/********************************************************************
*
* MODULE & VERSION : @(#)revcoper.c	1.1 
* DATE             : 01/11/23 
* TIME             : 17:23:29 
*
* CREATED          : 05/01/99
*
* DESCRIPTION:
* Este proceso se encarga de realizar el cierre de operaciones. Cambia al
* PARTE de OPERAC el estado de horas normales y extras (al 50, 100 y feriado).
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "revcoper.fmh"

#define	ERR_PERI_BORR		"El periodo de referencia del Sist.Novia: %d / %d de la Empresa: %d no existe!!!"

#define	ERR_LIQ				"No existe la liquidaci\'{o}n: %ld"
#define	ERR_CUR				"No se pudo crear el cursor para borrar la informaci\'{o}n vieja"
#define	ERR_LEG				"No existe el empleado perteneciente a la Empresa: %d con Legajo : %ld"
#define	HORAS_EXTRAS_50		13  //Variables que definen las variables de novedad del empleado...
#define	HORAS_EXTRAS_100	14
#define	HORAS_EXTRAS_FER	116
#define	HORAS_NORMALES		59

// Funciones privadas.
static	fm_status	after(form fm, fmfield fn0, int row);


static	void	BorrarVAREMP(long nroiq, int vemp);
static	void	CambiarEstadoParte(int emp, DATE fdesnor, DATE fhasnor, DATE fdesext, DATE fhasext);
static	void	CamEstParteRetro(int emp, DATE fdesde, DATE fhasta);
static	void	GuardarDatos(int emp, long nroleg, double hsextras50, double hsextras100,
							double hsextrasfer, double hsnormales);
static	void	LeerLegajo(int emp, long nroleg);
static	void	AlmacenarHoras(long nroleg, int vemp, long ccosto, double valemp);
static	void	GuardarLog(int emp, int mesc, int anioc);


// Variables globales.
form	fm0;
schema	sue;

// Programa principal
wcmd(revcoper, 1.1 11/23/01) {

	int		emp,
			mesc,
			anioc;
	DATE	fdesnor,
			fhasnor,
			fdesext,
			fhasext;

	sue	=	OpenSchema("sue", IO_NOT_LOCK);

	fm0	=	OpenForm("revcoper", FM_EABORT);
	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;
	mesc	=	FmIFld(fm0, MESC);
	anioc	=	FmIFld(fm0, ANIOC);
	emp		=	FmIFld(fm0, EMP);
	fdesnor	=	FmDFld(fm0, FDESNOR);
	fhasnor	=	FmDFld(fm0, FHASNOR);
	fdesext	=	FmDFld(fm0, FDESEX);
	fhasext	=	FmDFld(fm0, FHASEX);

	BeginTransaction();
	GuardarLog(emp, mesc, anioc);
	CambiarEstadoParte(emp, fdesnor, fhasnor, fdesext, fhasext);
//	CamEstParteRetro(emp, fdesde, fhasta);
	EndTransaction();
	CloseAllSchemas();
}


static void CambiarEstadoParte(int emp, DATE fdesnor, DATE fhasnor,
								DATE fdesext, DATE fhasext) {

	dbcursor	c_parte;

	c_parte	=	CreateCursor(PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, emp, MIN_LONG, MIN_DATE, MIN_SHORT);
	SetCursorTo  (c_parte, emp, MAX_LONG, MAX_DATE, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		if ((DFld(PARTE_DIA) < fdesnor ||
			DFld(PARTE_DIA) > fhasnor) &&
			(DFld(PARTE_DIA) < fdesext ||
			DFld(PARTE_DIA) > fhasext)) {
				continue;
			}

		if (DFld(PARTE_DIA) >= fdesext &&
			DFld(PARTE_DIA) <= fhasext) {
			SetIFld(PARTE_CONFEX, CERRADO);
		}
		if (DFld(PARTE_DIA) >= fdesnor &&
			DFld(PARTE_DIA) <= fhasnor) {
			SetIFld(PARTE_CONFIR, CERRADO);
		}
		PutRecord(PARTE);
	}
}


static void CamEstParteRetro(int emp, DATE fdesde, DATE fhasta) {

}


static void GuardarLog(int emp, int mesc, int anioc) {

	SetKey(CIERREbyEMP, emp, mesc, anioc);
	if (GetRecord(CIERREbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		return Error(ERR_PERI_BORR, mesc, anioc, emp);
	}
	SetDFld(CIERRE_FECCIE,  Today());
	SetTFld(CIERRE_HORACIE, Hour());
	SetIFld(CIERRE_USUCIE,  GetUid());
	PutRecord(CIERRE);
}


static fm_status after(form fm, fmfield fno, int row) {

	int	mes,
		anio;

	switch (fno) {
		case EMP :
			// leo el periodo ingresado:
			SetKey(CIERREbyEMP, FmIFld(fm, fno), MAX_SHORT, MAX_SHORT);
			if (GetRecord(CIERREbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) == ERROR) {
				return FmErrMsg(fm, M_ERR_PERI_INEX);
			}
			if (IsNull(CIERRE_FECCIE)) {
				return FmErrMsg(fm, M_ERR_PERI_CERRADO);
			}
			DbToFm(fm, 0, CONTROL_FLD);
			break;
	}
	return FM_OK;
}

