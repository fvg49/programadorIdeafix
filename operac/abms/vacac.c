/********************************************************************
*
* MODULE & VERSION : @(#)vacac.c	1.1 
* DATE             : 10/03/31 
* TIME             : 14:47:15 
*
* CREATED          : 02/02/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "vacac.fmh"
#include "comerc.h"
#include "operac.h"
#include "filial.h"
#include "operac.sch"
#include "asist.sch"
#include "sue.sch"
#include "plv.sch"

#define WAR_LICEN       "Dentro del periodo ingresado %.1D - %.1D, el vigilador tiene licencias cargadas!"
#define WAR_ASISTEN     "Dentro del periodo ingresado %.1D - %.1D, el vigilador tiene inasistencias cargadas!"
#define WAR_PARTE       "Dentro del periodo ingresado %.1D - %.1D, el vigilador tiene horas cargadas!"
#define WAR_DIA_LICEN   "Para la Fecha Desde ingresada, el vigilador tiene licencias cargadas!"
#define WAR_DIA_ASISTEN "Para la Fecha Desde ingresada, el vigilador tiene inasistencias cargadas!"
#define WAR_DIA_PARTE   "Para la Fecha Desde ingresada, el vigilador tiene horas cargadas!"
#define ERR_FECING      "El Período de vacaciones debe ser a partir del %d"
#define ERR_TOTALDIAS   "La cantidad de dias de vacaciones es mayor\n a la cantidad de dias permitidas para el periodo."

/* Funciones privadas */
static  fm_status after(form fm, fmfield fno, int row);
static  void      Lectura(fm_cmd, find_mode);

static  bool Superpos(DATE fecha, DATE fdesde, DATE fhasta);
private bool TieneLicVacac(int emp, long nroleg, DATE fdesde, DATE fhasta);
private bool TieneAsisten(int emp, long nroleg, DATE fdesde, DATE fhasta);
private bool TieneHorasCargadas(int emp, long nroleg, DATE fdesde, DATE fhasta);

/* Declaraciones globales */
form   fm0;
schema asist, operac, sue, plv;
int i, g_emp;
DATE fecierre;

wcmd(vacac, 1.1 03/31/10)
{
	fm_cmd	cmd;

	asist  = OpenSchema("asist",  IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	plv    = OpenSchema("plv",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	fm0	= OpenForm("vacac", FM_EABORT);


	// Inicio Permisos
	g_emp= StrToI(getenv("emp"));
	fecierre  = GetFechaCierreOpe(g_emp);
	InicLegajoXusr (g_emp, fecierre, fm0, MENSAJE, FALSE, _TIPPER_ASIGNA);

//	ImprimeLegajoXusr();

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(VACAC);
	case FM_UPDATE:
		BeginTransaction();

		if (FmIsNull(fm0, FDESDE, 0)) {
			SetKey(VACACbyPERIODO, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO) + 1, MIN_DATE);
			if (GetRecord(VACACbyPERIODO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
				Error("No puede borrar el periodo %d.\nExisten periodos posteriores cargados.", FmIFld(fm0, PERIODO));
		}

		// Borrado
		SetKey(VACACbyPERIODO, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO), NULL_DATE);
		while (GetRecord(VACACbyPERIODO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			DelRecord(VACAC);
		}
		// Grabación
		FmToDb(fm0, 0, PERIODO, 0);
		for(i=0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, FDESDE, i); i++) {
			FmToDb(fm0, FDESDE, MODULO, i);
			if (FmIsNull(fm0, I_FECREG, i))
				SetDFld(VACAC_FECREG, Today());
			else
				SetDFld(VACAC_FECREG, FmDFld(fm0, I_FECREG, i));
			PutRecord(VACAC);
		}
		EndTransaction();
		break;
	case FM_DELETE:
		BeginTransaction();
		SetKey(VACACbyPERIODO, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO) + 1, MIN_DATE);
		if (GetRecord(VACACbyPERIODO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
			Error("No puede borrar el periodo %d.\nExisten periodos posteriores cargados.", FmIFld(fm0, PERIODO));

		FmToDb(fm0, 0, FDESDE);
		DelRecord(VACAC);
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(VACAC);
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	find_mode moden;
	int partial;

	FmToDb(fm0, 0, PERIODO);
	switch(mode) {
		case THIS_KEY: 
			moden = NEXT_KEY|PARTIAL_KEY;
			partial = 3;
			SetDFld(VACAC_FDESDE, MIN_DATE);
		break;
		case NEXT_KEY: 
			moden = NEXT_KEY|PARTIAL_KEY;
			partial = 1;
			SetDFld(VACAC_FDESDE, MAX_DATE);
		break;
		case PREV_KEY: 
			moden = PREV_KEY|PARTIAL_KEY;
			partial = 1;
			SetDFld(VACAC_FDESDE, MIN_DATE);
		break;
	} 

	switch(GetRecord(VACACbyPERIODO, moden, IO_LOCK|IO_TEST, partial)) {
		case IO_LOCKED:
			FmSetStatus(fm0, FM_LOCKED);
			FindRecord(VACACbyPERIODO, mode, partial);
			DbToFm(fm0, 0, PERIODO);
			FmShowFlds(fm0, 0, PERIODO);
			return;
		case ERROR:
			FmSetStatus(fm0, cmd == FM_READ ? FM_NEW : FM_EOF);
			return;
	}

	DbToFm(fm0, 0, PERIODO);
	SetDFld(VACAC_FDESDE, MIN_DATE);
	for (i=0; GetRecord(VACACbyPERIODO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR; i ++) {
		DbToFm(fm0, FDESDE, MODULO, i);
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	int diasper = 0;
	long v_nroleg;
	dbcursor c_vacac;
	int NLINE = 0;
	int c = 0;      
	int f = 0;        
	DATE anio;

	c_vacac	= CreateCursor(VACACbyPERIODO, IO_NOT_LOCK);

	switch(fno) {
		case EMP :

			if (FmChgFld(fm))
				InicLegajoXusr (FmIFld(fm, fno), fecierre, fm0, MENSAJE, FALSE, _TIPPER_ASIGNA);
			break;

		case NROLEG :

			switch(FmKeyCode(fm)) {
				case K_HELP:
					HelpLegajo(fm, fno, row);
					break;
				case K_META:
					v_nroleg = ERROR;
					if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
						return FM_REDO;

					FmSetLFld(fm, fno, v_nroleg, row);
					break;
			}
			if (!FmIsNull(fm, fno, row)){
				if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG), MAX_DATE))
					if (!FmIsNull(fm, fno, row)) {
						WiDialog(WD_OK, WD_OK, "Error", "Legajo Inactivo");
						return FM_REDO;
					}
			}

		  	FmSetFld(fm, APYNOM, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, NROLEG)));
			DbToFm(fm0, I_FECING, I_CCOSTO);

			if (FmIFld(fm0, I_ACTIVO) != 1)
				return FmErrMsg(fm0, M_ERR_NO_ACT);

			if (!EsVigilador(FmIFld(fm0, EMP), FmIFld(fm0, I_RELAC), FmLFld(fm0, I_CCOSTO))){
				Warning("El Legajo %ld no pertenece a un Vigilador.\n\n Por favor, informar de estas vacaciones\n al Depto. de Personal.", LFld(sue|PER_NROLEG));
			}
			break;
		case PERIODO :       
			
			SetKey(plv|LEGANIObyEMP, FmIFld(fm, EMP), FmLFld(fm, NROLEG), FmIFld(fm, fno));
			if (GetRecord(plv|LEGANIObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
				FmSetIFld(fm, DIASPER, IFld(plv|LEGANIO_LICANIO));
			else
				FmSetIFld(fm, DIASPER, 0);

			//--------- Valido el Periodo -----------
		 	if (FmIFld(fm, fno) <   Year(DFld(sue|DATPERS_FECANT))) {
		 		Warning(ERR_FECING,  Year(DFld(sue|DATPERS_FECANT)));
		 		return FM_SKIP;
		 	}
			//Valido que exista cargada vacaciones del periodo anterior
//			if (FmIFld(fm, fno) > Year(DFld(sue|PER_FECING))) {
			if (FmIFld(fm, fno) > Year(DFld(sue|DATPERS_FECANT))) {
				//Valido que en periodo anterior tenga dias de vacaciones. Puede ocurrir que por la fecha de
				//ingreso no corresponda dias en periodo anterior.
				SetKey(plv|LEGANIObyEMP, FmIFld(fm, EMP), FmLFld(fm, NROLEG), FmIFld(fm0, PERIODO) - 1);
				if (GetRecord(plv|LEGANIObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
					diasper = IFld(plv|LEGANIO_LICANIO);
				else
					diasper = 0;

				SetIFld(VACAC_EMP,     FmIFld(fm0, EMP));
				SetLFld(VACAC_NROLEG,  FmLFld(fm0, NROLEG));
				SetIFld(VACAC_PERIODO, FmIFld(fm0, PERIODO) - 1);
				SetDFld(VACAC_FDESDE,  MIN_DATE);
				if (GetRecord(VACACbyPERIODO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR &&
					diasper > 0) {
					Warning("No existe periodo %d cargado.", FmIFld(fm0, PERIODO) - 1);
					return FM_SKIP;
				}
			}
			break;
		case FDESDE :


			if (!FmIsNull(fm, fno, row)){
				if (FmChgFld(fm)){
					if (fecierre > FmDFld(fm, fno, row)) {
						fecierre= FmDFld(fm, fno, row);
						InicLegajoXusr (FmIFld(fm, EMP), fecierre, fm0, MENSAJE, FALSE, _TIPPER_ASIGNA);
					}            	

					if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG), FmDFld(fm, fno, row))){
						WiDialog(WD_OK, WD_OK, "Error", "No tiene permiso para ingresar este legajo el dia %.3D", FmDFld(fm, fno, row));
						return FM_REDO;
					}
				}
			}


			// check (this < i_fecegr), on error ERR_FECEGR,
			if (!FmIsNull(fm, I_FECEGR) && FmDFld(fm, fno, row) > FmDFld(fm, I_FECEGR)) {
				return FmErrMsg(fm, M_ERR_FECEGR, FmDFld(fm, I_FECEGR));
			}

			if (!FmIsNull(fm, fno, row) && FmDFld(fm, fno, row) < FmDFld(fm0, I_FECING))
				return FmErrMsg(fm, M_ERR_FECING, FmDFld(fm, I_FECING));

			SetCursorFrom(c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO) - 10, MIN_DATE);
			SetCursorTo  (c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO) + 1, MAX_DATE);
			while (FetchCursor(c_vacac) != ERROR) {
				// lo que sigue está sin el = en la comparación por si consultan un
				// registro que ya existe entonces no debería tirar errores!!!!
				if (FmDFld(fm0, FDESDE, row) >  DFld(operac|VACAC_FDESDE) &&
					FmDFld(fm0, FDESDE, row) <= DFld(operac|VACAC_FHASTA)) {
					return FmErrMsg(fm0, M_ERR_SUPERPOS, DFld(operac|VACAC_FDESDE), DFld(operac|VACAC_FHASTA));
				}
			}
			if (FmChgFld (fm)) {
				FmSetDFld (fm, I_FECREG, Today (), row);
			}

			break;
		case FHASTA:
			if (!FmIsNull(fm, fno, row)){
				if (FmChgFld(fm))
					if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG), FmDFld(fm, fno, row))){
						WiDialog(WD_OK, WD_OK, "Error", "No tiene permiso para ingresar este legajo el dia %.3D", FmDFld(fm, fno, row));
						return FM_REDO;
					}
			}

			if (TieneLicVacac(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FDESDE, row),
						FmDFld(fm0, FHASTA, row))) {
				Warning(WAR_LICEN, FmDFld(fm0, FDESDE, row), FmDFld(fm0, FHASTA, row));
				return FM_ERROR;
			}
			if (TieneAsisten(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FDESDE, row),
						FmDFld(fm0, FHASTA, row))) {
				Warning(WAR_ASISTEN, FmDFld(fm0, FDESDE, row), FmDFld(fm0, FHASTA, row));
				return FM_ERROR;
			}
			if (TieneHorasCargadas(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FDESDE, row),
						FmDFld(fm0, FHASTA, row))) {
				Warning(WAR_PARTE, FmDFld(fm0, FDESDE, row), FmDFld(fm0, FHASTA, row));
				return FM_ERROR;
			}
			break;
		
		case MODULO:
		
		    NLINE =  FmFldLen(fm, MULTI);
		    
		    /*
		    
		    

			SetCursorFrom(c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO), MIN_DATE);
			SetCursorTo  (c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO), MAX_DATE);
			while (FetchCursor(c_vacac) != ERROR) {
				// lo que sigue está sin el = en la comparación por si consultan un
				// registro que ya existe entonces no debería tirar errores!!!!
				if (FmDFld(fm0, FDESDE, row) >  DFld(operac|VACAC_FDESDE) &&
					FmDFld(fm0, FDESDE, row) <= DFld(operac|VACAC_FHASTA)) {
					return FmErrMsg(fm0, M_ERR_SUPERPOS, DFld(operac|VACAC_FDESDE), DFld(operac|VACAC_FHASTA));
				}
			}		    
		    
		    
		    
		    */
		    
 	  
		    
		    for (c = 0; c <= NLINE &&  FmDFld(fm0, FDESDE, c) != IsNull; c++){
		    	for(f = 0; f <= NLINE &&  FmDFld(fm0, FDESDE, f) != IsNull; f++){
		    	
		    		if(c == f)
		    			continue;                       
						
					if(FmDFld(fm0, FDESDE, c) >  FmDFld(fm0, FDESDE, f) &&
					  FmDFld(fm0, FDESDE, c) <= FmDFld(fm0, FHASTA, f))		    	  
						return FmErrMsg(fm0, M_ERR_SUPERPOS, FmDFld(fm0, FDESDE, c), FmDFld(fm0, FHASTA, c));		    	

		SetCursorFrom(c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO), MIN_DATE);
 		SetCursorTo  (c_vacac, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmIFld(fm0, PERIODO), MAX_DATE);
 		while (FetchCursor(c_vacac) != ERROR) {


				if (FmDFld(fm0, FDESDE, c) >  DFld(operac|VACAC_FDESDE) &&
					FmDFld(fm0, FDESDE, c) <= DFld(operac|VACAC_FHASTA)) 
					return FmErrMsg(fm0, M_ERR_SUPERPOS, DFld(operac|VACAC_FDESDE), DFld(operac|VACAC_FHASTA));		    	
		    	
		       }
		    	
		    }
		    
		    
		}   
			break;
		
		
			
		case CANTDIAS :
//			if (FmIFld(fm, TOTALDIA) > FmIFld(fm, DIASPER)) {  DHC por pedido de JLF 23/03/11
//				Warning(ERR_TOTALDIAS);
//			}
			break;
			if (FmChgFld (fm)) {
				FmSetDFld (fm, I_FECREG, Today (), row);
			}
	}
	return FM_OK;
}

private bool TieneLicVacac(int emp, long nroleg, DATE fdesde, DATE fhasta)
{
	dbcursor c_licen;

	c_licen	= CreateCursor(asist|LICENbyTIPLIC, IO_NOT_LOCK);

	SetCursorFrom(c_licen, emp, nroleg, MIN_SHORT, MIN_DATE);
	SetCursorTo  (c_licen, emp, nroleg, MAX_SHORT, MAX_DATE);
	while (FetchCursor(c_licen) != ERROR) {
		if (Superpos(DFld(asist|LICEN_FECHAD), fdesde, fhasta) ||
			Superpos(DFld(asist|LICEN_FECHAH), fdesde, fhasta) ||
			Superpos(fdesde, DFld(asist|LICEN_FECHAD), DFld(asist|LICEN_FECHAH))) {
				DeleteCursor(c_licen);
				return TRUE;
		}
	}
	DeleteCursor(c_licen);
	return FALSE;
}

static bool Superpos(DATE fecha, DATE fdesde, DATE fhasta)
{
	if (fecha >= fdesde && fecha <= fhasta)
		return TRUE;
	return FALSE;
}

private bool TieneAsisten(int emp, long nroleg, DATE fdesde, DATE fhasta)
{
	dbcursor c_asisten;

	c_asisten =	CreateCursor(asist|ASISTENbyINDLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asisten, emp, nroleg, fdesde, MIN_SHORT);
	SetCursorTo  (c_asisten, emp, nroleg, fhasta, MAX_SHORT);
	if (FetchCursor(c_asisten) != ERROR) {
		DeleteCursor(c_asisten);
		return TRUE;
	}
	DeleteCursor(c_asisten);
	return FALSE;
}

private bool TieneHorasCargadas(int emp, long nroleg, DATE fdesde, DATE fhasta)
{
	dbcursor c_parte;

	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);

	SetCursorFrom(c_parte, emp, nroleg, fdesde, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, nroleg, fhasta, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (FFld(operac|PARTE_HSNOR)  != 0.00 || FFld(operac|PARTE_HS50)    != 0.00 ||
			FFld(operac|PARTE_HS100F) != 0.00 || FFld(operac|PARTE_HS100FE) != 0.00) {
			DeleteCursor(c_parte);
			return TRUE;
		}
	}
	DeleteCursor(c_parte);
	return FALSE;
}
