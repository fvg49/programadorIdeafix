/********************************************************************
*
* MODULE & VERSION : @(#)ctrlasis.c	1.3 
* DATE             : 01/11/20 
* TIME             : 16:06:37 
*
* CREATED          : 22/05/2000
*
* DESCRIPTION:
*      Busca errores en el parte
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "ctrlasis.fmh"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"
#include "asist.sch"

/* Defines */

/* Funciones privadas */
void Mensaje (char *msg);
void ControlarAsisten();
//void ControlarLicen();
void ControlarParte();

/* Declaraciones globales */
form   fm0;
schema comerc, operac, asist;
char abuffer[50];


wcmd(ctrlasis, 1.3 11/20/01)
{
	fm_cmd cmd;
	DATE fecha;
	int i = 0;

	fm0    = OpenForm  ("ctrlasis", FM_EABORT);
	comerc = OpenSchema("comerc",  IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);
	asist  = OpenSchema("asist",  IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT) {
		switch (cmd) {
			case FM_ADD:
			case FM_UPDATE:
				ControlarAsisten();
//				ControlarLicen();
				ControlarParte();
				FmSetFld (fm0, COMENT, NULL_STR);
			break;
		case FM_IGNORE:
			break;
		}
	}
}


void ControlarAsisten()
{
	bool encontro=FALSE;
	dbcursor c_ASIS;
	
	c_ASIS = CreateCursor(asist|ASISTENbyEMPRE, IO_NOT_LOCK);
	SetCursorFrom(c_ASIS, FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_ASIS, FmIFld(fm0, EMP), FmDFld(fm0, FHASTA), MAX_LONG, MAX_SHORT);
	
	while (FetchCursor(c_ASIS) != ERROR) {
		encontro=FALSE;
		sprintf(abuffer, "Procesando ASISTEN %.3D", DFld(asist|ASISTEN_FECHA));
		Mensaje(abuffer);

		SetKey(operac|PARTEbyEMPLE, FmIFld(fm0, EMP), LFld(asist|ASISTEN_NROLEG), DFld(asist|ASISTEN_FECHA), MIN_LONG, MIN_SHORT);
		while (GetRecord(operac|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			
			if (LFld(operac|PARTE_CLIENTE) < FmLFld(fm0, CLIMIN)) {
				continue;
			}
			
			if (IFld(operac|PARTE_HSNOR)   > FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS50)    > FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS100F)  > FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS100FE) > FmIFld(fm0, CANTMIN)) {
					encontro=TRUE;			
					fprintf (stderr, "ASISTEN - El legajo %ld %.3D %ld %d horas %d %d %d %d \n", LFld(asist|ASISTEN_NROLEG), DFld(asist|ASISTEN_FECHA),
										LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
										IFld(operac|PARTE_HSNOR),IFld(operac|PARTE_HS50),
										IFld(operac|PARTE_HS100F), IFld(operac|PARTE_HS100FE));
			}
		}

//        if (encontro && FmIFld(fm0, BORRA)) {
//			DelRecord(asist|ASISTEN);
//		}
	}
	
}

/*

void ControlarLicen()
{
	bool encontro=FALSE;
	dbcursor c_LICENC, c_PARTE;

	c_LICENC = CreateCursor(asist|LICENbyEMP, IO_NOT_LOCK);
	SetCursorFrom(c_LICENC, FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), MIN_SHORT, MIN_LONG);
	SetCursorTo  (c_LICENC, FmIFld(fm0, EMP), MAX_DATE, MAX_SHORT, MAX_LONG);

	c_PARTE = CreateCursor(operac|PARTEbyEMPLE,IO_NOT_LOCK);

	while (FetchCursor(c_LICENC) != ERROR) {
		encontro=FALSE;

		sprintf(abuffer, "Procesando LICEN %.3D", DFld(asist|LICEN_FECHAD));
		Mensaje(abuffer);

		SetCursorFrom (c_PARTE, FmIFld(fm0, EMP), LFld(asist|LICEN_NROLEG), DFld(asist|LICEN_FECHAD), MIN_LONG, MIN_SHORT);
		SetCursorTo   (c_PARTE, FmIFld(fm0, EMP), LFld(asist|LICEN_NROLEG), DFld(asist|LICEN_FECHAH), MIN_LONG, MIN_SHORT);

		while (FetchCursor(c_PARTE) != ERROR) {
			if (LFld(operac|PARTE_CLIENTE) < FmLFld(fm0, CLIMIN)) {
				continue;
			}

			if (IFld(operac|PARTE_HSNOR) > FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS50) >  FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS100F) > FmIFld(fm0, CANTMIN) ||
				IFld(operac|PARTE_HS100FE) > FmIFld(fm0, CANTMIN)) {
					encontro=TRUE;			
					fprintf (stderr, "LICEN - El legajo %ld lic %.3D %.3D - parte %.3D %ld %d horas %d %d %d %d \n", LFld(operac|PARTE_NROLEG), 
										DFld(asist|LICEN_FECHAD),
										DFld(asist|LICEN_FECHAH),
										DFld(operac|PARTE_DIA),
										LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
										IFld(operac|PARTE_HSNOR),IFld(operac|PARTE_HS50),
										IFld(operac|PARTE_HS100F), IFld(operac|PARTE_HS100FE));
			}
		}
//      if (encontro && FmIFld(fm0, BORRA)) {
//			DelRecord(asist|LICEN);
//		}
		
	}
}
 
*/ 
 
void Mensaje (char *msg)
{
	FmSetFld(fm0, COMENT, msg);
	WiRefresh();
}

void ControlarParte()
{
	bool error=FALSE;
	dbcursor c_LICENC, c_PARTE;

	c_LICENC = CreateCursor(asist|LICENbyFECHA, IO_NOT_LOCK);

	c_PARTE = CreateCursor(operac|PARTEbyDIA,IO_NOT_LOCK);
	SetCursorFrom (c_PARTE, FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
	SetCursorTo   (c_PARTE, FmIFld(fm0, EMP), FmDFld(fm0, FHASTA), MAX_LONG, MAX_SHORT);

	while (FetchCursor(c_PARTE) != ERROR) {
		sprintf(abuffer, "Procesando PARTE %.3D", DFld(operac|PARTE_DIA));
		Mensaje(abuffer);

		if (LFld(operac|PARTE_CLIENTE) < FmLFld(fm0, CLIMIN)) {
			continue;
		}

		error=FALSE;

		if (*SFld(operac|PARTE_CONDIC) == 'A'){
			SetKey(asist|ASISTENbyINDLEG,FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG),
						 DFld(operac|PARTE_DIA), MIN_SHORT);
			if (GetRecord(asist|ASISTENbyINDLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR) {
					error=TRUE;
			}

//			SetCursorFrom(c_LICENC, FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), MIN_DATE, MIN_SHORT);
//			SetCursorTo  (c_LICENC, FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), MAX_DATE, MAX_SHORT);
//			while (FetchCursor(c_LICENC) != ERROR) {
//				if (DFld(operac|PARTE_DIA) > DFld(asist|LICEN_FECHAH) ||
//					DFld(operac|PARTE_DIA) < DFld(asist|LICEN_FECHAD)) continue;
//					
//					error=FALSE; //Esta cargado en licencia
//			} 
		}

		if (error) {			
			fprintf (stderr, "PARTE - El legajo %ld dia %.3D esta ausente en el parte pero no esta en ausentismo cli %ld obj %d \n",
					LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
		}
	}
}

