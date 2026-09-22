/********************************************************************
*
* MODULE & VERSION : @(#)lnotrab.c	1.5 
* DATE             : 04/03/04 
* TIME             : 18:19:57 
*
* CREATED          : 15/10/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lnotrab.fmh"
#include "lnotrab.rph"
#include "sue.sch"
#include "operac.sch"

#define VAL_ACTIVO 1
/* Funciones privadas */
static fm_status before(form, fmfield, int);
static fm_status after (form, fmfield, int);
void AbrirReporte();

/* Declaraciones globales */
form fm0;
report rp0;
schema sue, operac;
fm_cmd cmd;

/* Programa principal */
wcmd(lnotrab, 1.5 03/04/04)
{
	dbcursor c_per, c_parte;
	bool encontro, encontro_algo;
	fm0 = OpenForm("lnotrab", FM_EABORT);
	sue    = OpenSchema("sue", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	
	while((cmd=DoForm(fm0, before, after)) != FM_EXIT) {
		if (cmd!=FM_UPDATE)
			continue;

		encontro_algo=FALSE;
		c_per=CreateCursor(sue|PERbyACT, IO_NOT_LOCK);
		SetCursorFrom(c_per, FmIFld(fm0, EMP), VAL_ACTIVO, FmLFld(fm0, LEGD));
		SetCursorTo  (c_per, FmIFld(fm0, EMP), VAL_ACTIVO, FmLFld(fm0, LEGH));
		while(FetchCursor(c_per)!=ERROR) {

			if (LFld(sue|PER_CODCCOS)>=10000 && LFld(sue|PER_CODCCOS)<=99000)
				continue;

			if (DFld(sue|PER_FECING)<FirstMonthDay(FmDFld(fm0, FECDES)))
				continue;
			if (DFld(sue|PER_FECING)>FmDFld(fm0, FECHAS))
				continue;

//			fprintf(stderr, "%ld\n",  LFld(sue|PER_NROLEG));

			encontro=FALSE;
			/* Revizo que no tenga horas cargadas*/
			c_parte=CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
			SetCursorFrom(c_parte, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FECDES), NULL_LONG, NULL_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FECHAS), MAX_LONG, MAX_SHORT);
			while(FetchCursor(c_parte)!=ERROR) {

// 				fprintf(stderr, "1-ENTRO %ld\n",  LFld(sue|PER_NROLEG));

				if(LFld(operac|PARTE_HSNOR)==0 && LFld(operac|PARTE_HS50)==0 &&
				   LFld(operac|PARTE_HS100F)==0  && LFld(operac|PARTE_HS100FE)==0)
					continue;

// 				fprintf(stderr, "2-ENTRO %ld\n",  LFld(sue|PER_NROLEG));

				encontro=TRUE;
				break;
			} 
			if (encontro)
				continue; 
			
			if (!encontro_algo)
				AbrirReporte();

			encontro_algo=TRUE;
				
//			fprintf(stderr, "3-ENTRO %ld\n",  LFld(sue|PER_NROLEG));

			DbToRp(rp0, RLEG, RFECING);
			DoReport(rp0, LINEA);
		}

		if(!encontro_algo)
			WiDialog(WD_OK, WD_OK, "Aviso", "No hay legajos[1m [0mque cumplan con las condiciones ingresadas");
		else
			CloseReport(rp0);
	}
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case FECDES:
		if(FmIsNull(fm, fno))
			FmSetDFld(fm, fno, FirstMonthDay(Today()) );
		break;
	case FECHAS:
		if(FmIsNull(fm, fno))
			FmSetDFld(fm, fno, LastMonthDay(Today()) );
		break;
	case SALIDA:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case SALIDA:
		break;
	}
	return FM_OK;
}
void AbrirReporte()
{
	rp0 = OpenReport("lnotrab", RP_EABORT|RP_NOBEGIN);  
	RpSetOutput(rp0, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
	BeginReport(rp0, 1, NULL_STR);     

	RpSetIFld(rp0, REMP,    FmIFld(fm0, EMP));
	RpSetFld (rp0, RDEMP,   FmSFld(fm0, DEMP));
	RpSetDFld(rp0, RFECDES, FmDFld(fm0, FECDES));
	RpSetDFld(rp0, RFECHAS, FmDFld(fm0, FECHAS));
	RpSetLFld(rp0, RLEGD,   FmLFld(fm0, LEGD));
	RpSetFld (rp0, RDLEGD,  FmSFld(fm0, DLEGD));
	RpSetLFld(rp0, RLEGH,   FmLFld(fm0, LEGH));
	RpSetFld (rp0, RDLEGH,  FmSFld(fm0, DLEGH));

}




