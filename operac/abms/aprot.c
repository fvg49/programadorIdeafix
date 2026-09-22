/********************************************************************
* MODULE & VERSION : @(#)aprot.c	1.25
* DATE             : 09/06/09
* TIME             : 16:02:55
*
* CREATED          : 17/09/98
*
* DESCRIPTION:
*      Aprobación de Ordenes de Trabajo.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "aprot.fmh"
#include "comerc.sch"
#include "operac.sch"
#include "filial.h"
#include "motrech.fmh"

/* Defines */
#define PENDIENTE		0
#define APROBADO		1
#define RECHAZADO		2
#define MAX_CLIENTE		300
#define MAX_OBJET		300
#define ESPORADICO		2

/* Funciones privadas */
static bool SetMulti();
static void SetMultiEspor(), SetMultiRiff(), StoreInOt(), ClearMulti();
static void GraboPuesto(int p_emp, int tipcomp, int serie, char * deleg, long nroot, long cliente, int objet,
						 DATE finicio, DATE ffinal,  TIME hiniot, TIME hfinot);
static fm_status after(form fm, fmfield fno, int row);
bool ValidoEstadoOt ( short modelo, bool estadm, bool estoper, bool estvta);
bool ValidoEstadoOtc ( bool estadm, bool estoper, bool estvta);
bool ValidoEstadoOto ( bool estadm, bool estoper, bool estvta);
short ModeloComprobante (short tipcomp);
bool SetearEstadoOtc (int i);
bool SetearEstadoOto (int i);
void BajarCliente (long cliente, short objetivo);

/* Declaraciones globales */
int tope, pais, emp;
form fm0, fm1;
schema comerc, operac;
dbcursor c_OT;
bool comercial, operaciones, admvigi;

/* Programa principal */
wcmd(aprot, 1.25 06/09/09)
{
	fm_cmd cmd;

	operac = OpenSchema("operac", IO_EABORT | IO_SYMBOLS);
	comerc = OpenSchema("comerc", IO_EABORT | IO_SYMBOLS);

	comercial   = FALSE;
	admvigi     = FALSE;
	operaciones = FALSE;
	tope = 0;
	pais = StrToI(ReadEnv("PAIS"));

	emp = StrToI(ReadEnv("EMP"));

	InicListaXusr(StrToI(ReadEnv("EMP")));

	fm0 = OpenForm("aprot", FM_EABORT);
	 
	if (!SetMulti())
		Error("El Usuario no pertenece a un Grupo autorizado para realizar Aprobaciones");
	SetMultiRiff();
	SetMultiEspor();

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT)
	switch (cmd) {
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		StoreInOt();
		ClearMulti();
		SetMultiRiff();
		SetMultiEspor();
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
	
	FinListaXusr();
}

static bool SetMulti()
{
	int ok = 0;    /* No lectura */

	if (UsrInGrupo(GRPADM, GetUid())) {
		c_OT = CreateCursor(comerc|OTbyAPROBC, IO_NOT_LOCK);
		admvigi = TRUE;
		ok = 1;
	}
	else {
		if (UsrInGrupo(GRPOPER, GetUid()) || UsrInGrupo(GRPSUPOPER, GetUid())) {
			c_OT = CreateCursor(comerc|OTbyAPROBO, IO_NOT_LOCK);
			operaciones = TRUE;
			ok = 1;
		}
		else {
			if (UsrInGrupo(GRPCOMER, GetUid())) {
				c_OT = CreateCursor(comerc|OTbyAPROBV, IO_NOT_LOCK);
				comercial = TRUE;
				ok = 1;
			}
		}
	}
	if (!ok) 
		return FALSE;
	return TRUE;
}

static void SetMultiEspor()
{
	SetCursorFrom(c_OT, PENDIENTE, MIN_SHORT, MIN_SHORT, MIN_SHORT, LOW_VALUE,  MIN_LONG);
	SetCursorTo  (c_OT, PENDIENTE, MAX_SHORT, MAX_SHORT, MAX_SHORT, HIGH_VALUE, MAX_LONG);
	for (; tope < FmFldLen(fm0, MULTI) && FetchCursor(c_OT) != ERROR;) {
		short modelo;
		modelo = ModeloComprobante (IFld(comerc|OT_TIPCOMP));

		//si elijo una ot por formulario
		if (!FmIsNull(fm0, POSOT) && LFld(comerc|OT_NROOT) != FmLFld(fm0, POSOT))
			continue;

		if (modelo != MODOTC && modelo != MODOTO)
			continue;

		if (IFld(comerc|OT_TIPSER) != ESPORADICO )
			continue;

		if (IFld(comerc|OT_ANULADO) != NULL_SHORT && IFld(comerc|OT_ANULADO)) {
			continue;
		}

		if (operaciones && IFld(comerc|OT_ESTADM) == PENDIENTE)
			continue;
		
		if (!ValidoEstadoOt (modelo, IFld(comerc|OT_ESTADM), IFld(comerc|OT_ESTOPER), IFld(comerc|OT_ESTVTA)))
			continue;
  
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OT_CLIENTE), IFld(comerc|OT_OBJET)))
  	       	continue;

		if (IFld(comerc|OT_EMP) != emp)
			continue;

		FmSetIFld(fm0, I_EMP,     IFld(comerc|OT_EMP),     tope);
		FmSetIFld(fm0, I_TIPCOMP, IFld(comerc|OT_TIPCOMP), tope);
		FmSetIFld(fm0, I_SERIE,   IFld(comerc|OT_SERIE),   tope);

		SetIFld(comerc|NCBTES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		GetRecord(comerc|NCBTESbyTIPCOMP, THIS_KEY, IO_NOT_LOCK);

		SetIFld(comerc|NSERIES_EMP,     IFld(comerc|OT_EMP));
		SetIFld(comerc|NSERIES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		SetIFld(comerc|NSERIES_SERIE,   IFld(comerc|OT_SERIE));
		GetRecord(comerc|NSERIESbyEMP, THIS_KEY, IO_NOT_LOCK);

		switch(IFld(comerc|OT_EMP)) {
			case _EMP_PSA:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPOT), tope);
				break;
			case _EMP_SAPE:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPSAPE), tope);
				break;
			case _EMP_PSEG:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPPSEG), tope);
				break;
			default :
				FmSetFld(fm0, EMP, "NODEF", tope);
				break;			
		}	
		
		FmSetFld (fm0, TIPCOMP, SFld(comerc|NSERIES_DESCOR),  tope);
		FmSetFld (fm0, SERIE,   SFld(comerc|NCBTES_DESCOR),   tope);
		FmSetFld (fm0, DELEG,   SFld(comerc|OT_DELEG),        tope);
		FmSetLFld(fm0, NROOT,   LFld(comerc|OT_NROOT),        tope);
		FmSetFld (fm0, ABM,     SFld(comerc|OT_ABM),          tope);
		FmSetLFld(fm0, CLIENTE, LFld(comerc|OT_CLIENTE),      tope);
		FmSetIFld(fm0, OBJET,   IFld(comerc|OT_OBJET),        tope);
		FmSetDFld(fm0, FECREG,  DFld(comerc|OT_FECREG),       tope);
		FmSetFld (fm0, TIPSER,  "E",                          tope);
		FmSetFld (fm0, DTIPSER, "Esporádico",                 tope);

		if (admvigi)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTADM), tope);
		if (operaciones)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTOPER), tope);
		if (comercial)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTVTA), tope);

		tope++;
	}
}

static void SetMultiRiff()
{
	SetCursorFrom(c_OT, PENDIENTE, MIN_SHORT, MIN_SHORT, MIN_SHORT, LOW_VALUE,  MIN_LONG);
	SetCursorTo  (c_OT, PENDIENTE, MAX_SHORT, MAX_SHORT, MAX_SHORT, HIGH_VALUE, MAX_LONG);
	for (tope = 0; tope < FmFldLen(fm0, MULTI) && FetchCursor(c_OT) != ERROR;) {
		short modelo;
		modelo = ModeloComprobante (IFld(comerc|OT_TIPCOMP));
 
		//si elijo una ot por formulario
		if (!FmIsNull(fm0, POSOT) && LFld(comerc|OT_NROOT) != FmLFld(fm0, POSOT))
			continue;

		if (modelo != MODOTC && modelo != MODOTO)
			continue;
 
		if (IFld(comerc|OT_TIPSER) == ESPORADICO)
			continue;
 
		if (IFld(comerc|OT_ANULADO) != NULL_SHORT && IFld(comerc|OT_ANULADO)) {
			continue;
		}
 
		if (operaciones && IFld(comerc|OT_ESTADM) == PENDIENTE)
			continue;

		if (!ValidoEstadoOt ( modelo, IFld(comerc|OT_ESTADM), IFld(comerc|OT_ESTOPER), IFld(comerc|OT_ESTVTA)))
			continue;
           
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OT_CLIENTE), IFld(comerc|OT_OBJET)))
	       	continue;
		
		if (IFld(comerc|OT_EMP) != emp)
  			continue;

		FmSetIFld(fm0, I_EMP, 	  IFld(comerc|OT_EMP), 	   tope);
		FmSetIFld(fm0, I_TIPCOMP, IFld(comerc|OT_TIPCOMP), tope);
		FmSetIFld(fm0, I_SERIE,   IFld(comerc|OT_SERIE),   tope);

		SetIFld(comerc|NCBTES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		GetRecord(comerc|NCBTESbyTIPCOMP, THIS_KEY, IO_NOT_LOCK);

		SetIFld(comerc|NSERIES_EMP,     IFld(comerc|OT_EMP));
		SetIFld(comerc|NSERIES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		SetIFld(comerc|NSERIES_SERIE,   IFld(comerc|OT_SERIE));
		GetRecord(comerc|NSERIESbyEMP, THIS_KEY, IO_NOT_LOCK);

		switch(IFld(comerc|OT_EMP)) {
			case _EMP_PSA:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPOT), tope);
				break;
			case _EMP_SAPE:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPSAPE), tope);
				break;
			case _EMP_PSEG:
				FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPPSEG), tope);
				break;
			default :
				FmSetFld(fm0, EMP, "NODEF", tope);
				break;			
		}	

		FmSetFld (fm0, TIPCOMP, SFld(comerc|NSERIES_DESCOR),  tope);
		FmSetFld (fm0, SERIE,   SFld(comerc|NCBTES_DESCOR),   tope);
		FmSetFld (fm0, DELEG,   SFld(comerc|OT_DELEG),        tope);
		FmSetLFld(fm0, NROOT,   LFld(comerc|OT_NROOT),        tope);
		FmSetFld (fm0, ABM,     SFld(comerc|OT_ABM),          tope);
		FmSetLFld(fm0, CLIENTE, LFld(comerc|OT_CLIENTE),      tope);
		FmSetIFld(fm0, OBJET,   LFld(comerc|OT_OBJET),        tope);
		FmSetDFld(fm0, FECREG,  DFld(comerc|OT_FECREG),       tope);
		FmSetFld (fm0, TIPSER,  "R",                          tope);
		FmSetFld (fm0, DTIPSER, "Rif",                        tope);

		if (admvigi)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTADM), tope);
		if (operaciones)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTOPER), tope);
		if (comercial)
			FmSetIFld(fm0, APRUEBA, IFld(comerc|OT_ESTVTA), tope);

		tope++;
	}
}

static void StoreInOt()
{
	int i;
	short modelo;
	DATE fcierre=NULL_DATE;

	for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, EMP, i); i++) {
		if (FmIFld(fm0, APRUEBA, i) == 0)
			continue;
	

		fm1 = UseSubform(fm0, APRUEBA, 0, i);

		SetIFld(comerc|OT_EMP,     FmIFld(fm0, I_EMP,     i));
		SetIFld(comerc|OT_TIPCOMP, FmIFld(fm0, I_TIPCOMP, i));
		SetIFld(comerc|OT_SERIE,   FmIFld(fm0, I_SERIE,   i));
		SetFld (comerc|OT_DELEG,   FmSFld(fm0, DELEG,     i));
		SetLFld(comerc|OT_NROOT,   FmLFld(fm0, NROOT,     i));
		if (GetRecord( comerc|OTbyEMP, THIS_KEY, IO_LOCK) != ERROR) {


			// Si se quiere aprobar, valido ot contra fecha de cierre de operaciones

/*          CIERRE anule este control hasta que solo funcione cuando toca el esquema operativo
			fcierre=GetFechaCierreOpe(IFld(comerc|OT_EMP));

			if (FmIFld(fm0, APRUEBA, i) == APROBADO ) {
				if (!IsNull(comerc|OT_FINICIO)){
					if (DFld(comerc|OT_FINICIO)<=fcierre) {
						WiDialog(WD_OK, WD_OK, "Error", 
						"La OT %d-%d-%d-%s-%ld tiene fecha de inicio %.3D\nanterior a la fecha de [1mcierre de operaciones %.3D[0m\nNo se puede aprobar esta OT",	
						IFld(comerc|OT_EMP), IFld(comerc|OT_TIPCOMP), IFld(comerc|OT_SERIE), SFld (comerc|OT_DELEG), LFld(comerc|OT_NROOT), DFld(comerc|OT_FINICIO), fcierre);

						continue;
					}
				}
				else {
					if (DFld(comerc|OT_FFINAL)<=fcierre) {
						WiDialog(WD_OK, WD_OK, "Error", 
						"La OT %d-%d-%d-%s-%ld tiene fecha de fin %.3D\nanterior a la fecha de [1mcierre de operaciones %.3D[0m\nNo se puede aprobar esta OT",	
						IFld(comerc|OT_EMP), IFld(comerc|OT_TIPCOMP), IFld(comerc|OT_SERIE), SFld (comerc|OT_DELEG), LFld(comerc|OT_NROOT), DFld(comerc|OT_FFINAL), fcierre);

						continue;
					}
				}
			}
*/
			modelo = ModeloComprobante (FmIFld(fm0, I_TIPCOMP, i));

			if (modelo != MODOTC && modelo != MODOTO) {
				WiMsg("Comprobante %d no se sabe procesar", FmIFld(fm0, I_TIPCOMP, i));
				continue;
			}
			if (modelo == MODOTC) {       
				//Si grabo en puesto y en una baja veo si es la baja del objetivo
				if (SetearEstadoOtc(i) && *SFld (comerc|OT_ABM) == 'B')
					BajarCliente (LFld (comerc|OT_CLIENTE), IFld (comerc|OT_OBJET));
			}

			if (modelo == MODOTO) {
				//Si grabo en puesto y en una baja veo si es la baja del objetivo
				if (SetearEstadoOto(i) && *SFld (comerc|OT_ABM) == 'B')
					BajarCliente (LFld (comerc|OT_CLIENTE), IFld (comerc|OT_OBJET));
			}

			PutRecord(comerc|OT);
			FreeTable(comerc|OT);
		}
	}
}

static void ClearMulti()
{
	int i;
	for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, EMP, i); i++) {
		FmSetIFld(fm0, I_EMP, 	  NULL_SHORT, i);
		FmSetIFld(fm0, I_TIPCOMP, NULL_SHORT, i);
		FmSetIFld(fm0, I_SERIE,   NULL_SHORT, i);
		FmSetFld (fm0, EMP,       NULL_STR,   i);
		FmSetFld (fm0, TIPCOMP,   NULL_STR,   i);
		FmSetFld (fm0, SERIE,     NULL_STR,   i);
		FmSetFld (fm0, DELEG,     NULL_STR,   i);
		FmSetLFld(fm0, NROOT,     NULL_LONG,  i);
		FmSetFld (fm0, ABM,       NULL_STR,   i);
		FmSetLFld(fm0, CLIENTE,   NULL_LONG,  i);
		FmSetIFld(fm0, OBJET,     NULL_SHORT, i);
		FmSetDFld(fm0, FECREG,    NULL_DATE,  i);
		FmSetIFld(fm0, APRUEBA,   NULL_SHORT, i);
		FmSetFld (fm0, DAPRUEB,   NULL_STR,   i);
		FmSetFld (fm0, TIPSER,    NULL_STR,   i);
		FmSetFld (fm0, DTIPSER,   NULL_STR,   i);

		fm1 =UseSubform(fm0, APRUEBA, 0, i);
		FmSetFld (fm1, OBS,   NULL_STR,   i);

	}
}

static void GraboPuesto(int p_emp, int tipcomp, int serie, char * deleg, long nroot, long cliente, int objet,
								  DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot)
{
	int i;
	SetIFld(comerc|PTOSER_EMP,     p_emp);
	SetIFld(comerc|PTOSER_TIPCOMP, tipcomp);
	SetIFld(comerc|PTOSER_SERIE,   serie);	
	SetFld (comerc|PTOSER_DELEG,   deleg);
	SetLFld(comerc|PTOSER_NROOT,   nroot);
	SetIFld(comerc|PTOSER_TIPPTO,  MIN_SHORT);
	while (GetRecord(comerc|PTOSERbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 5) != ERROR) {
		SetIFld(comerc|NPUESTO_EMP,     p_emp);
		SetIFld(comerc|NPUESTO_TIPCOMP, tipcomp);
		SetIFld(comerc|NPUESTO_SERIE,   serie);
		SetFld (comerc|NPUESTO_DELEG,   deleg);
		SetLFld(comerc|NPUESTO_NROOT,   nroot);
		SetIFld(comerc|NPUESTO_TIPPTO,  IFld(comerc|PTOSER_TIPPTO));
		SetIFld(comerc|NPUESTO_NRORENG, MIN_SHORT);
		while (GetRecord(comerc|NPUESTObyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR) {
			i = 0;
			AsigCodInOperac(cliente, objet, IFld(comerc|PTOSER_TIPPTO), IFld(comerc|NPUESTO_PUESTO),
							TFld(comerc|NPUESTO_HINICIO),   TFld(comerc|NPUESTO_HFINAL),
							SFld(comerc|NPUESTO_DIAS, 0), SFld(comerc|NPUESTO_DIAS, 1),
							SFld(comerc|NPUESTO_DIAS, 2), SFld(comerc|NPUESTO_DIAS, 3),
							SFld(comerc|NPUESTO_DIAS, 4), SFld(comerc|NPUESTO_DIAS, 5),
							SFld(comerc|NPUESTO_DIAS, 6),   SFld(comerc|NPUESTO_REGIM),
							SFld(comerc|NPUESTO_TIPODIA),
							IFld(comerc|NPUESTO_CANTPUE),   IFld(comerc|NPUESTO_CANTVIG),
							SFld(comerc|NPUESTO_COND),      SFld(comerc|NPUESTO_CODFREC),
							IFld(comerc|NPUESTO_HORAPT),    finicio, ffinal, hiniot, hfinot, IFld(comerc|NPUESTO_CODINT));
		}
	}
}

static fm_status after(form fm, fmfield fno, int row)
{

	switch(fno) {
	case APRUEBA:
		if (FmIFld( fm, fno, row) == RECHAZADO) {
			fm1 =UseSubform(fm0, APRUEBA, 0, row);
			DoSubform(fm0, NULLFP, NULLFP, APRUEBA, 0 , row);

			if (ModeloComprobante (FmIFld( fm, I_TIPCOMP, row)) == MODOTO) {
				DisplayMsg( FALSE, "NO SE PUEDE RECHAZAR UNA OT de OPERACIONES");
				FmSetIFld( fm, APRUEBA, 0, row);
				return FM_ERROR;
			}
		}
	break;
	case NROEMP :
	    emp = FmIFld(fm, NROEMP);

	    if (FmChgFld(fm0))	{
        	InicListaXusr(FmIFld(fm0, NROEMP));
        	
        	ClearMulti();
        	
        	if (!SetMulti())
				Error("El Usuario no pertenece a un Grupo autorizado para realizar Aprobaciones");
			SetMultiRiff();
			SetMultiEspor();
        }
    break;
    case POSOT:
	    if (FmChgFld(fm))	{
        	ClearMulti();

	       	if (!SetMulti())
 				Error("El Usuario no pertenece a un Grupo autorizado para realizar Aprobaciones");
    		SetMultiRiff();
			SetMultiEspor();
    	}
    break;
	}
	return FM_OK;
}

bool ValidoEstadoOt ( short modelo, bool estadm, bool estoper, bool estvta)
{
	if (modelo == MODOTC)
		return ValidoEstadoOtc (estadm, estoper, estvta);
	if (modelo == MODOTO)
		return ValidoEstadoOto (estadm, estoper, estvta);

	return FALSE;
}

bool ValidoEstadoOtc ( bool estadm, bool estoper, bool estvta)
{
	if (operaciones || admvigi)
		if (estvta != APROBADO)
			return FALSE;

	return TRUE;
}

bool ValidoEstadoOto ( bool estadm, bool estoper, bool estvta)
{
	if (operaciones)
		return TRUE;
	if (comercial && estoper == APROBADO)
		return TRUE;
	if (admvigi &&  estoper == APROBADO &&  estvta == APROBADO)
		return TRUE;

	return FALSE;
}

short ModeloComprobante (short tipcomp) 
{
	switch (tipcomp) {
		case  OTCOMERC: return MODOTC;
		case  OTOPERAC: return MODOTO;
	}
	return NULL_SHORT;
}

/*******************************************
Devuelve TRUE si uso la funcion GraboPuesto
*******************************************/
bool SetearEstadoOtc(int i)
{
	bool grabo = FALSE;
	if (admvigi) {
		if (IFld(comerc|OT_ESTOPER) == APROBADO && IFld(comerc|OT_ESTVTA) == APROBADO &&
			FmIFld(fm0, APRUEBA, i) == APROBADO && !EsSubcontr(FmLFld(fm0, CLIENTE, i), FmIFld(fm0, OBJET, i))) {
			GraboPuesto(FmIFld(fm0, I_EMP, i), FmIFld(fm0, I_TIPCOMP, i), FmIFld(fm0, I_SERIE, i),
						FmSFld(fm0, DELEG, i), FmLFld(fm0, NROOT,     i), FmLFld(fm0, CLIENTE, i),
						FmIFld(fm0, OBJET, i), DFld(comerc|OT_FINICIO), DFld(comerc|OT_FFINAL), TFld(comerc|OT_HINICIO), TFld(comerc|OT_HFINAL));
			grabo = TRUE;
		}
		
		SetIFld(comerc|OT_ESTADM,   FmIFld(fm0, APRUEBA, i));
		SetDFld(comerc|OT_FOKOADM,  Today());
		SetTFld(comerc|OT_HOKOADM,  Hour());
		SetFld (comerc|OT_USUADM,   UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHA, FmSFld(fm1, OBS));
		if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
			SetIFld(comerc|OT_ESTVTA,  PENDIENTE);
			SetIFld(comerc|OT_ESTOPER, PENDIENTE);
		}
		
	}
	if (operaciones) {
		if (IFld(comerc|OT_ESTADM) == APROBADO  && IFld(comerc|OT_ESTVTA) == APROBADO &&
			FmIFld(fm0, APRUEBA, i) == APROBADO && !EsSubcontr(FmLFld(fm0, CLIENTE, i), FmIFld(fm0, OBJET, i))) {
			GraboPuesto(FmIFld(fm0, I_EMP, i), FmIFld(fm0, I_TIPCOMP, i), FmIFld(fm0, I_SERIE, i),
						FmSFld(fm0, DELEG, i), FmLFld(fm0, NROOT,     i), FmLFld(fm0, CLIENTE, i), 
						FmIFld(fm0, OBJET, i), DFld(comerc|OT_FINICIO),   DFld(comerc|OT_FFINAL), TFld(comerc|OT_HINICIO), TFld(comerc|OT_HFINAL));
			grabo = TRUE;
		}
		SetIFld(comerc|OT_ESTOPER,  FmIFld(fm0, APRUEBA, i));
		SetDFld(comerc|OT_FOKOPER,  Today());
		SetTFld(comerc|OT_HOKOPER,  Hour());
		SetFld (comerc|OT_USUOPER, UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHO, FmSFld(fm1, OBS));
		if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
			SetIFld(comerc|OT_ESTVTA, PENDIENTE);
			SetIFld(comerc|OT_ESTADM, PENDIENTE);
		}
	}
	if (comercial) {
		SetIFld(comerc|OT_ESTVTA,   FmIFld(fm0, APRUEBA, i));
		SetIFld(comerc|OT_ESTADM,   PENDIENTE);
		SetIFld(comerc|OT_ESTOPER,  PENDIENTE);
		SetDFld(comerc|OT_FOKVTA,   Today());
		SetTFld(comerc|OT_HOKVTA,   Hour());
		SetFld(comerc|OT_USUCOM, UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHV, FmSFld(fm1, OBS));
	}

	return grabo;
}

/*******************************************
Devuelve TRUE si uso la funcion GraboPuesto
********************************************/
bool SetearEstadoOto(int i)                  
{
	bool grabo = FALSE;

	if (admvigi) {
		SetIFld(comerc|OT_ESTADM,   FmIFld(fm0, APRUEBA, i));
		SetDFld(comerc|OT_FOKOADM,  Today());
		SetTFld(comerc|OT_HOKOADM,  Hour());
		SetFld (comerc|OT_USUADM,   UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHA, FmSFld(fm1, OBS));
		if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
			SetIFld(comerc|OT_ESTVTA,  PENDIENTE);
			SetIFld(comerc|OT_ESTOPER, PENDIENTE);
		}
	}
	if (operaciones) {
		if (FmIFld(fm0, APRUEBA, i) && !EsSubcontr(FmLFld(fm0, CLIENTE, i), FmIFld(fm0, OBJET, i))) {
			GraboPuesto(FmIFld(fm0, I_EMP, i), FmIFld(fm0, I_TIPCOMP, i), FmIFld(fm0, I_SERIE, i),
						FmSFld(fm0, DELEG, i), FmLFld(fm0, NROOT,     i), FmLFld(fm0, CLIENTE, i),
						FmIFld(fm0, OBJET, i), DFld(comerc|OT_FINICIO),   DFld(comerc|OT_FFINAL), TFld(comerc|OT_HINICIO), TFld(comerc|OT_HFINAL));
			grabo = TRUE;
		}

		SetIFld(comerc|OT_ESTOPER,  FmIFld(fm0, APRUEBA, i));
		SetDFld(comerc|OT_FOKOPER,  Today());
		SetTFld(comerc|OT_HOKOPER,  Hour());
		SetFld (comerc|OT_USUOPER,  UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHO, FmSFld(fm1, OBS));
		SetIFld(comerc|OT_ESTVTA,   PENDIENTE);
		SetIFld(comerc|OT_ESTADM,   PENDIENTE);
	}
	if (comercial) {
		SetIFld(comerc|OT_ESTVTA,   FmIFld(fm0, APRUEBA, i));
		SetIFld(comerc|OT_ESTADM,   PENDIENTE);
		SetDFld(comerc|OT_FOKVTA,   Today());
		SetTFld(comerc|OT_HOKVTA,   Hour());
		SetFld (comerc|OT_USUCOM,   UserName(GetUid()));
		SetFld (comerc|OT_MOTRECHV, FmSFld(fm1, OBS));
		if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
			SetIFld(comerc|OT_ESTADM,  PENDIENTE);
			SetIFld(comerc|OT_ESTOPER, PENDIENTE);
		}
	}

	return grabo;
}

void BajarCliente (long cliente, short objetivo)
{
    static dbcursor curpue;
    DATE fecbaja=MIN_DATE;

    /*Tiene que ser un objetivo riff */
	if (!ObjetRif(cliente, objetivo))
		return;
    
    if (!curpue)
    	curpue = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom (curpue, cliente, objetivo, MIN_SHORT, MIN_SHORT);
	SetCursorTo   (curpue, cliente, objetivo, MAX_SHORT, MAX_SHORT);

	//Veo si todos los objetivos estan dados de baja
	while (FetchCursor (curpue) != ERROR) {
		if (IFld (operac|PUESTOS_CANTVIG) != 0.0 && IsNull (operac|PUESTOS_FFINAL))
			return;
			
		if (DFld(operac|PUESTOS_FFINAL) > fecbaja)
			fecbaja = DFld(operac|PUESTOS_FFINAL);
	} 

	/*Grabo la fecha de finalizacion del objetivo */
	SetKey (comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord (comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_LOCK) != ERROR) {
		SetDFld (comerc|OBJETIVO_FECHAF, fecbaja);
		PutRecord (comerc|OBJETIVO);
		FreeTable (comerc|OBJETIVO);
	}
}

