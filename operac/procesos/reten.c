/********************************************************************
* MODULE & VERSION : reten.c
* DATE             : 28/04/99
*
* CREATED 	       : Gloria
* DESCRIPTION      : Muestra para los vigiladores Retenes en que clientes 
*                    están asignados.
*
*********************************************************************/
#include <ideafix.h>
#include "reten.fmh"
#include "reten.rph"
#include "sue.sch"
#include "operac.sch"
#include "operac.h"
#include "comerc.h"
#include "brigada.sch"
#include "billpro.sch"
#include "comerc.sch"
#include "bill.sch"
#include "billpro.h"
#include "filial.h"

static void ObtenerAsignacion(form fm, fmfield fno, int row);
static void Lectura(fm_cmd, find_mode);
static void Proceso(void);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
form   fm0;
report rp;
schema sue, operac, brigada, billpro, comerc, bill;

wcmd(reten, 1.12  23/09/98)
{
	fm_status cmd;

	comerc  = OpenSchema("comerc",  IO_EABORT);
	billpro = OpenSchema("billpro", IO_EABORT);
	sue     = OpenSchema("sue",     IO_EABORT);
	brigada = OpenSchema("brigada", IO_EABORT);
	operac  = OpenSchema("operac",  IO_EABORT);
	bill    = OpenSchema("bill",    IO_EABORT);

	fm0 = OpenForm("reten", FM_EABORT);

	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:
			Lectura(cmd, THIS_KEY);
			break;
		case FM_UPDATE:
			if(FmIFld(fm0, IMPRIME)) {
				rp = OpenReport("reten", RP_EABORT);
				RpSetDFld(rp, RFECHA, FmDFld(fm0, FECHA));
				Proceso();
			}
			break;
		case FM_IGNORE :
			break;
		}
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
    FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	dbcursor asigCur, asighCur, asigTabCur;
	dbtable  asigTab;
	int      i, row;
	bool     otrocli;

	asigTab    = CreateAlias(operac|ASIG);
	asigTabCur = CreateCursor(AlInd(asigTab, ASIGbyNROLEG), IO_NOT_LOCK);
	asigCur    = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);
	asighCur   = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	row = 0;
	otrocli = FALSE;

	SetCursorFrom(asigCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT );
	SetCursorTo  (asigCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT );
	while (FetchCursor(asigCur) != ERROR) {
		if (DFld(ASIG_FECASIG) > FmDFld(fm0, FECHA))
			continue;

		// DATOS PERSONALES
		FmSetLFld(fm0, NROLEG, LFld(ASIG_NROLEG), row);

		// Obtengo el nombre y el telefono del legajo
		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), LFld(ASIG_NROLEG));
		GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
		FmSetFld (fm0, NOMBRE,    SFld(sue|PER_APYNOM), row);
		FmSetFld (fm0, TELEF,     SFld(sue|PER_TELEF, 0), row);

		// CLU y PORTACION
		SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
		GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

		FmSetIFld(fm0, CLU,    IFld(brigada|CVIGIPOL_CLU), row);
		FmSetIFld(fm0, PORTAC, IFld(brigada|CVIGIPOL_PORTACION), row);

		// ALTA EN LA POLICIA
		i = 0;
		SetKey(brigada|VIGIPOLbyULTMOD,TRUE , FmIFld(fm0, EMP), LFld(ASIG_NROLEG), NULL_SHORT, NULL_SHORT);
		while (GetRecord(brigada|VIGIPOLbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (IFld(brigada|VIGIPOL_ACEPTADO) != 3)
				continue;

			if (!IFld(brigada|VIGIPOL_ACTIVO))
				continue;

			if(!UsrInGrupo(GRPBRIG, GetUid()) )
				if (!InscVig(IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI),
			              IFld(brigada|VIGIPOL_ACEPTADO), DFld(brigada|VIGIPOL_FECHA), NULL))
					continue;

			SetKey(billpro|PROVXDIVbyPORSUE, IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI));
			if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) != ERROR)
				switch (i) {
				case 0:
					FmSetFld(fm0, ALTAPOL1, SFld(billpro|PROVXDIV_CODPROV), row);
					break;
				case 1:
					FmSetFld(fm0, ALTAPOL2, SFld(billpro|PROVXDIV_CODPROV), row);
					break;
				case 2:
					FmSetFld(fm0, ALTAPOL3, SFld(billpro|PROVXDIV_CODPROV), row);
					break;
				case 3:
					FmSetFld(fm0, ALTAPOL4, SFld(billpro|PROVXDIV_CODPROV), row);
					break;
				}
			i++;
			if (i == 4)
				break;
		}
		SetCursorFrom(asigTabCur, FmIFld(fm0, EMP), LFld(ASIG_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (asigTabCur, FmIFld(fm0, EMP), LFld(ASIG_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(asigTabCur) != ERROR) {
			if (LFld(AlFld(asigTab, ASIG_CLIENTE)) == FmLFld(fm0, CLIE))
				continue;
			if (FmDFld(fm0, FECHA) > DFld(AlFld(asigTab, ASIG_FECHAS)))
				continue;
			if (!otrocli)
				otrocli = TRUE;

			FmSetLFld(fm0, CLIASIG, LFld(AlFld(asigTab, ASIG_CLIENTE)),  row);
			FmSetIFld(fm0, OBJASIG, IFld(AlFld(asigTab, ASIG_OBJETIVO)), row);
			row++;
		}
		if (!otrocli) {
			// Proceso de los ASIGH
			// index nroleg(emp, nroleg, cliente, objetivo),
			SetCursorFrom(asighCur, FmIFld(fm0, EMP), LFld(ASIG_NROLEG), MIN_LONG, MIN_SHORT);
			SetCursorTo  (asighCur, FmIFld(fm0, EMP), LFld(ASIG_NROLEG), MAX_LONG, MAX_SHORT);
			while (FetchCursor(asighCur) != ERROR) {
				if (DFld(ASIGH_FECALT) < FmDFld(fm0, FECHA) ||
					DFld(ASIGH_FECBAJ) > FmDFld(fm0, FECHA)) {
					continue;
				}
				if (!otrocli) 
					otrocli = TRUE;

				FmSetLFld(fm0, CLIASIG, LFld(ASIGH_CLIENTE), row);
				FmSetIFld(fm0, OBJASIG, IFld(ASIGH_OBJETIVO), row);
				row++;
			}
		}
		if (!otrocli) {
			// seteo el multi con el cliente de la planta nada mas (ASIG).
			FmSetLFld(fm0, CLIASIG, LFld(ASIG_CLIENTE), row);
			FmSetIFld(fm0, OBJASIG, IFld(ASIG_OBJETIVO), row);
			row++;
		}
		otrocli = FALSE;
	}
}

static void Proceso(void)
{
	DATE franco;
	int i;

	for (i=0; i<FmFldLen(fm0, CLIENTES) && !FmIsNull(fm0, NROLEG, i); i++) {
		RpClearZone(rp, ZLINEA);
		RpSetLFld(rp, RNROLEG, FmLFld(fm0, NROLEG, i));
		RpSetFld(rp, RAPYNOM,  FmSFld(fm0, NOMBRE, i));
		SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), FmLFld(fm0, CLIE),
									FmIFld(fm0, OBJET));
		(void)GetRecord(operac|ASIGbyNROLEG, THIS_KEY, IO_NOT_LOCK);
		RpSetFld(rp, RREGIMEN, SFld(operac|ASIG_REGIM));
		franco = FmDFld(fm0, FECHA);
		// le agregue SFld(ASIG_VIGIL) a la funcion Franco - Ingrid 12/15/1999
		while (!Franco(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), franco, SFld(operac|ASIG_VIGIL),
									IFld(operac|ASIG_NUMFRAN)))
			franco++;

		RpSetDFld(rp, RFRANCO, franco);
		SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), FmLFld(fm0, CLIASIG, i),
									FmIFld(fm0, OBJASIG, i));
		(void)GetRecord(operac|ASIGbyNROLEG, THIS_KEY, IO_NOT_LOCK);
		RpSetTFld(rp, RLABD, TFld(operac|ASIG_HSENT));
		RpSetTFld(rp, RLABH, TFld(operac|ASIG_HSSAL));
		SetKey(operac|PARTEbyEMPLE, FmIFld(fm0, EMP),  FmLFld(fm0, NROLEG, i), FmDFld(fm0, FECHA),
									FmLFld(fm0, CLIE), FmIFld(fm0, OBJET));
		if (GetRecord(operac|PARTEbyEMPLE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if(!StrCmp(SFld(operac|PARTE_CONDIC), "T"))
				RpSetIFld(rp, RPTEPTA,  TRUE);
			else
				RpSetIFld(rp, RPTEPTA,  FALSE);

			RpSetTFld(rp, RLLEGPTA, TFld(operac|PARTE_HORAENT));
			RpSetTFld(rp, RLLEGOBJ, TFld(operac|PARTE_HORAENT));
			RpSetIFld(rp, RHSTOT, ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORAENT))*100);
		}
		RpSetFld (rp, RTELEF,   FmSFld(fm0, TELEF,    i));
		RpSetIFld(rp, RPORT,    FmIFld(fm0, PORTAC,   i));
		RpSetIFld(rp, RCLU,     FmIFld(fm0, CLU,      i));
		RpSetFld (rp, RPOL1,    FmSFld(fm0, ALTAPOL1, i));
		RpSetFld (rp, RPOL2,    FmSFld(fm0, ALTAPOL2, i));
		RpSetFld (rp, RPOL3,    FmSFld(fm0, ALTAPOL3, i));
		RpSetFld (rp, RPOL4,    FmSFld(fm0, ALTAPOL4, i));
		RpSetLFld(rp, RCLIPROV, FmLFld(fm0, CLIASIG,  i));

		SetKey(bill|CLIENTEbyCLIENTE, FmLFld(fm0, CLIASIG, i));
		(void)GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		RpSetFld(rp, RDCLIPROV, SFld(bill|CLIENTE_RAZSOC));
		SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIASIG, i), FmIFld(fm0, OBJASIG, i));
		(void)GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		RpSetFld(rp, RDOBJPROV, SFld(comerc|OBJETIVO_DESCRIP));
		RpSetIFld(rp, ROBJPROV, FmIFld(fm0, OBJASIG, i));
		DoReport(rp, ZLINEA);
		DoReport(rp, ZENTLIN);
	}
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
    case CLIE:
	   	InicClientesXusr();
    	break;
    case OBJET:
	   	InicObjetivosXusr(FmLFld(fm, CLIE, row), FmIFld(fm, EMP, row));
    	break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
    break;
	case CLIE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);

		if (ValidaClienteXusr(FmLFld(fm, CLIE, row)))
		  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIE, row)), row);
		else {
			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIE, row));
			FmSetLFld(fm, CLIE, NULL_LONG, row);
 			FmSetFld(fm, DCLIE, NULL_STR, row);
			return FM_REDO;
  		}	
    break;
    case OBJET:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIE, row));

		if (ValidaObjetivoXusr(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
			FmSetFld(fm, DOBJ, GetObjDescrip(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row)), row);
		else	{
			Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row));
			FmSetIFld(fm, OBJET, NULL_SHORT, row);
			FmSetFld(fm, DOBJ, NULL_STR, row);
			return FM_REDO;
    	}

	break;
	}
	return FM_OK;				
}	
