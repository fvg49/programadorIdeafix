/********************************************************************
* MODULE & VERSION : @(#)lptime.c	1.1
* DATE             : 00/09/14
* TIME             : 10:31:57
*
* CREATED          :
*
* DESCRIPTION:
*      Listado de Asignación de Vigiladores Part Time.
*
*********************************************************************/
#include <ideafix.h>
#include "lptime.fmh"
#include "lptime.rph"
#include "operac.sch"
#include "operac.h"
#include "comerc.h"

#define LISTACLI 1
#define LISTAVIG 2

// Declaraciones de Funciones
static fm_status before(form fm, fmfield fno, int row),
				 after(form, fmfield, int);
static void Proceso();
void AbrirReporte();

// Declaraciones globales
form   fm0;
report rp0 = ERROR;

/* Programa principal */
wcmd(lptime, 1.1 09/14/00)
{
	fm_status cmd;

	fm0 = OpenForm("lptime", FM_EABORT);

	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	AbrirReporte();
	Proceso();
}

static void Proceso()
{
	dbcursor casig;
	int      ultdia;
	char     buffer[50];

	if (FmIFld(fm0, LISTAPOR) == LISTACLI) {
		casig = CreateCursor(ASIGbyEMP, IO_NOT_LOCK);
		SetCursorFrom(casig, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIsNull(fm0, OBJDESDE) ? MIN_SHORT : FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (casig, FmIFld(fm0, EMP), FmLFld(fm0, CLIHASTA), FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		casig = CreateCursor(ASIGbyNROLEG, IO_NOT_LOCK);
		SetCursorFrom(casig, FmIFld(fm0, EMP), FmLFld(fm0, VIGDESDE), MIN_LONG, MIN_SHORT);
		SetCursorTo  (casig, FmIFld(fm0, EMP), FmLFld(fm0, VIGHASTA), MAX_LONG, MAX_SHORT);
	} 
	while (FetchCursor(casig) != ERROR) {
		if (strcmp(SFld(ASIG_VIGIL), PARTTIME))
			continue;

		ultdia = NULL_DATE;

		SetIFld(DIASPTIME_EMP,      FmIFld(fm0, EMP));
		SetLFld(DIASPTIME_CLIENTE,  LFld(ASIG_CLIENTE));
		SetIFld(DIASPTIME_OBJETIVO, IFld(ASIG_OBJETIVO));
		SetLFld(DIASPTIME_NROLEG,   LFld(ASIG_NROLEG));
		SetIFld(DIASPTIME_TIPPTO,   IFld(ASIG_PTOSER));
		SetIFld(DIASPTIME_PUESTO,   IFld(ASIG_PUESTO));
		SetIFld(DIASPTIME_NROINT,   IFld(ASIG_NROINT));
		SetDFld(DIASPTIME_DIA,      MAX_DATE);
		if (GetRecord(DIASPTIMEbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR)
			ultdia = DFld(DIASPTIME_DIA);
					
		RpSetLFld(rp0, R_CLI,     LFld(ASIG_CLIENTE));
		RpSetIFld(rp0, R_OBJ,     IFld(ASIG_OBJETIVO));
		RpSetFld (rp0, R_DOBJ,    GetObjDescrip(LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO)));
		RpSetLFld(rp0, R_LEGAJO,  LFld(ASIG_NROLEG));
		RpSetFld (rp0, R_APENOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(ASIG_NROLEG)));
		RpSetFld (rp0, R_EF,      InDescr(ASIG_EFECT, SFld(ASIG_EFECT)));
		RpSetFld (rp0, R_REGIM,   SFld(ASIG_REGIM));
		RpSetIFld(rp0, R_PTOSER,  IFld(ASIG_PTOSER));
		RpSetIFld(rp0, R_PUESTO,  IFld(ASIG_PUESTO));
		RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(ASIG_PTOSER)));
		RpSetFld (rp0, R_DIA1,    SFld(ASIG_DIA1));
		RpSetFld (rp0, R_DIA2,    SFld(ASIG_DIA2));
		RpSetFld (rp0, R_DIA3,    SFld(ASIG_DIA3));
		RpSetFld (rp0, R_DIA4,    SFld(ASIG_DIA4));
		RpSetFld (rp0, R_DIA5,    SFld(ASIG_DIA5));
		RpSetFld (rp0, R_DIA6,    SFld(ASIG_DIA6));
		RpSetFld (rp0, R_DIA7,    SFld(ASIG_DIA7));
		RpSetTFld(rp0, R_HSENT,   TFld(ASIG_HSENT));
		RpSetTFld(rp0, R_HSSAL,   TFld(ASIG_HSSAL));
		RpSetDFld(rp0, R_FECASIG, DFld(ASIG_FECASIG));
		RpSetDFld(rp0, R_FECHAS,  DFld(ASIG_FECHAS));
		RpSetDFld(rp0, R_ULTDIA,  ultdia);
		DoReport(rp0, LINVIG);
		DoReport(rp0, LINCLI);
		if (FmIFld(fm0, LISTAPOR) == 1)
			sprintf(buffer, "Procesando Puestos de Cliente %ld Objetivo %d", LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO));
		else
			sprintf(buffer, "Procesando Vigilador %ld", LFld(ASIG_NROLEG));
		FmSetFld(fm0, COMENT, buffer);
		WiRefresh();
	}
	FmSetFld(fm0, COMENT, NULL_STR);
	WiRefresh();
}

static fm_status before(form fm, fmfield fno, int row)
{

	switch (fno) {
	case NOMARCH:
		if (FmIsNull(fm, fno) && *FmSFld(fm, SALIDA) == 'A')
			FmSetFld(fm, fno, "lptime.txt");
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case LISTAPOR :
			switch(FmIFld(fm, fno)) {
				case 1 :
					FmSetFld(fm0, DVIGD, NULL_STR);
					FmSetFld(fm0, DVIGH, NULL_STR);
					break;
				case 2 :
					FmSetFld(fm0, DCLID, NULL_STR);
					FmSetFld(fm0, DCLIH, NULL_STR);
					FmSetFld(fm0, DOBJD, NULL_STR);
					FmSetFld(fm0, DOBJH, NULL_STR);
					break;
			}
			break;
	}
	return FM_OK;
}

void AbrirReporte ()
{
	//Si la salida es Impresora
	if (*FmSFld(fm0, SALIDA) == 'I') {
		rp0 = OpenReport("lptime", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	}

	//Si la salida es Terminal
	if (*FmSFld(fm0, SALIDA) == 'T') {
		rp0 = OpenReport("lptime", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	}

	//Si la salida es Archivo
	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("lptime1", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0,  RP_IO_FILE, FmSFld(fm0, NOMARCH));
	}
	BeginReport(rp0, 1, NULL_STR);

	if (FmIFld(fm0, LISTAPOR) == 1)
		RpSetIFld(rp0, R_LISTAPOR, LISTACLI);
	else
		RpSetIFld(rp0, R_LISTAPOR, LISTAVIG);
	DoReport(rp0, LIST);
}

