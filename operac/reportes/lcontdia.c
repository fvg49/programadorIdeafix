/********************************************************************
*
* MODULE & VERSION : @(#)lcontdia.c	1.6 
* DATE             : 04/05/26 
* TIME             : 10:44:47 
*
* CREATED          : 21/12/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lcontdia.fmh"
#include "lcontdia.rph"
#include "sue.sch"   
#include "operac.sch"
#include "bill.sch"
#include "comerc.h"
#include "operac.h"
#include "opedef.h"
#include "filial.h"

#define ERR_ARCHI  "No se pudo abrir el archivo de salida."

/* Funciones privadas */
static void Proceso(void);
static bool ExisteParte(int emp, long nroleg, DATE dia, long cliente, int objet, int ptoser, int puesto,
						int nroint);
static bool TieneRetro(long nroleg, DATE dia);
static void AbrirReporte();
static void AbrirArchivo();
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema operac, sue, bill;
//tnlegxfil nodo_aux;
struct s_lisxusr_lib esta_lis;

/* Programa principal */
wcmd(lcontdia, 1.6 05/26/04)
{
	fm0    = OpenForm("lcontdia", FM_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
   	WiRefresh();

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	if (*FmSFld(fm0, SALIDA) == 'A')
		AbrirArchivo();
	else 
		AbrirReporte();

	Proceso();
	FinListaXusr();
}

static void Proceso(void)
{
	char buffer[100];
	dbcursor c_parte = (dbcursor) ERROR;

	c_parte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

	//c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	// Acumulo Legajos
	//InicLegajoXusr (FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), fm0, COMENT, TRUE, MAX_SHORT);
	//for (nodo_aux=inileg; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig) {
	//    if ((*nodo_aux).legxfil > FmLFld(fm0, NROLEGH) || (*nodo_aux).legxfil < FmLFld(fm0, NROLEGD))
	//    	continue;
    
    VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis))	{ 
		SetCursorFrom(c_parte, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_parte, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MIN_SHORT);
		//SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGD), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
		//SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGH), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_parte) != ERROR) {
	        if (LFld(operac|PARTE_NROLEG) < FmLFld(fm0, NROLEGD) || LFld(operac|PARTE_NROLEG) > FmLFld(fm0, NROLEGH))
	        	continue;
	        if (!ValidaFilial(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
	 
			sprintf (buffer, "Procesando Cliente %ld Objetivo %d", LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)))
				continue;
			if (TFld(operac|PARTE_HORAENT) != StrToT("00:00") || TFld(operac|PARTE_HORASAL) != StrToT("00:00"))
				continue;
			if (strcmp(SFld(operac|PARTE_CONDIC), "T"))
				continue;
			if (DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;
			if (ExisteParte(IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA),
							LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), IFld(operac|PARTE_PTOSER),
							IFld(operac|PARTE_PUESTO),  IFld(operac|PARTE_NROINT)))
				continue;
			if (TieneRetro(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA)))
				continue;
			SetKey(sue|PERbyEMP, IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG));
			(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);

			SetKey(bill|CLIENTEbyCLIENTE, LFld(operac|PARTE_CLIENTE));
			(void)GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
			
			if (*FmSFld(fm0, SALIDA) != 'A') {
				DbToRp(rp0, RNROLEG, RCOND);
				RpSetFld(rp0, RAPYNOM, SFld(sue|PER_APYNOM));
				RpSetFld(rp0, RNOMBRE, SFld(bill|CLIENTE_RAZSOC));
				RpSetFld(rp0, RDOBJET, GetObjDescrip(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)));
				DoReport(rp0, LINEA);
			}
			else {
				fprintf(fp, "%ld\t%.3D\t%30s\t%d\t%d\t%d\t%d\t%ld\t%30s\t%d\t%20s\t%s\t%d\t%d\n",
							LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), SFld(sue|PER_APYNOM),
							IFld(operac|PARTE_HSNOR), IFld(operac|PARTE_HS50), IFld(operac|PARTE_HS100F),
							IFld(operac|PARTE_HS100FE),	LFld(operac|PARTE_CLIENTE), SFld(bill|CLIENTE_RAZSOC),
							IFld(operac|PARTE_OBJETIVO), GetObjDescrip(LFld(operac|PARTE_CLIENTE),
							IFld(operac|PARTE_OBJETIVO)), SFld(operac|PARTE_CONDIC), NULL_SHORT, NULL_SHORT);
			}
		}
	}
	
	FinLegajoXusr();
}

static bool TieneRetro(long nroleg, DATE dia)
{
	dbcursor c_retro = (dbcursor) ERROR;
	bool existe = FALSE;
	
	c_retro = CreateCursor(operac|RETRObyREMPLE, IO_NOT_LOCK);

	SetCursorFrom(c_retro, FmIFld(fm0, EMP), nroleg, dia, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_retro, FmIFld(fm0, EMP), nroleg, dia, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_retro) != ERROR) {
		if (TFld(operac|RETRO_HORAENT) != StrToT("00:00") || TFld(operac|RETRO_HORASAL) != StrToT("00:00"))
			existe = TRUE;
		if (strcmp(SFld(operac|RETRO_CONDIC), "T"))
			existe = TRUE;
	}
	DeleteCursor(c_retro);
	return existe;
}

static void AbrirReporte()
{
	rp0 = OpenReport("lcontdia", RP_EABORT|RP_NOBEGIN);

	if (*FmSFld(fm0, SALIDA) == 'I') 
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

	if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

	BeginReport(rp0, 1, NULL_STR);
	RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));
}

static void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, ARCHIVO),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
	fprintf(fp, "Legajo\tFecha\tNombre\tHsNor\tHs50\tHs100\tHs100f\tCliente\t\tObjetivo\t\tEstado\tCond\tMot\n");		
}

static bool ExisteParte(int emp, long nroleg, DATE dia, long cliente, int objet, int ptoser, int puesto,
						int nroint)
{
	dbtable APARTE = (dbtable) ERROR;
	dbcursor c_aparte = (dbcursor) ERROR;
	bool esta = FALSE;

	APARTE   = CreateAlias(operac|PARTE);
	c_aparte = CreateCursor(APARTEbyEMPLE, IO_NOT_LOCK);

	SetCursorFrom(c_aparte, emp, nroleg, dia, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_aparte, emp, nroleg, dia, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_aparte) != ERROR) {
		if (IFld(APARTE_OBJETIVO) == objet  && LFld(APARTE_CLIENTE) == cliente &&
			IFld(APARTE_PTOSER)   == ptoser && IFld(APARTE_PUESTO)  == puesto  &&
			IFld(APARTE_NROINT)   == nroint)
			continue;
		if (TFld(APARTE_HORAENT) != StrToT("00:00") || TFld(APARTE_HORASAL) != StrToT("00:00")) {
			esta = TRUE;
			break;
		}

		if (strcmp(SFld(APARTE_CONDIC), "T")) {
			esta = TRUE;
			break;
		}
			continue;

//		esta = TRUE;
	}
	DeleteCursor(c_aparte);
	DeleteAlias(APARTE);
	return esta;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
    break;
    case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;				
}	

