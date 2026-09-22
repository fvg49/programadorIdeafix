/********************************************************************
* MODULE & VERSION : @(#)pervig.c	1.17
* DATE             : 08/12/16
* TIME             : 15:05:06
*
* CREATED          : 22/04/98
*
* DESCRIPTION:
*      Permisos x Vigilador
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "filial.h"
#include "comerc.h"
#include "operac.h"
#include "operac.sch"
#include "pervig.fmh"
#include "pervig1.fmh"

/* Funciones privadas */
static fm_status after (form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

static void Lectura();
static void GrabaPermiso();
static bool Cambio(int p_i);
static void Lectura_fm1(form p_fm1, int p_row_padre);
static void MuestraNrodeLineaActual(fmfield fno, int row);

/* Declaraciones globales */
int g_emp;
DATE g_fecparcial, g_fecierre;
schema operac;
form fm0, fm1;
int i;
      
/* Programa principal */
wcmd(pervig, 1.17 12/16/08)
{
	fm_cmd cmd;

	fm0 = OpenForm("pervig", FM_EABORT);


	operac = OpenSchema("operac", IO_EABORT);
	g_emp= StrToI(getenv("emp"));

	
	InicLegajoXusr (g_emp, Today(), fm0, MENSAJE, FALSE, NULL_SHORT);

	g_fecparcial = GetFechaCierreParcial(g_emp);
	g_fecierre   = GetFechaCierreOpe(g_emp);

	while ((cmd = DoForm(fm0, before, after))  != FM_EXIT)
	switch (cmd) {
	case FM_READ: 
		Lectura(); 
		break;

	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();

		GrabaPermiso();
		EndTransaction();

		break;
	case FM_IGNORE:
		break;
    }
	FinLegajoXusr ();

}
static fm_status before(form fm, fmfield fno, int row)
{

	MuestraNrodeLineaActual(fno, row);

	FmSetFld(fm, MENSAJE, "");
	WiRefresh();

	switch (fno) {
	case FECDES:
		if (FmIsNull(fm,fno))
			FmSetDFld(fm, fno, g_fecierre);
		break;
	case NROLEG:
	case DELEGA:
	case FILIAL:
	case TIPPER:
	case FECINI:
	case FECFIN:
		if (FmIFld(fm, ACTIVO, row)==FALSE)
			return FM_SKIP;

		break;
	case ACTIVO:
		FmSetFld(fm, MENSAJE, "Presione <INICIO> Para Ver Historial");
		WiRefresh();
		break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	char 	v_deleg[6],
   			v_filial[7];

	long v_nroleg;
	
	int v_i=0;
	
	switch (fno) {
	case EMP:
		if (FmChgFld(fm))
			InicLegajoXusr (FmIFld(fm, EMP), Today(), fm0, MENSAJE, FALSE, NULL_SHORT);

		break;

	case NROLEG:
		if (FmKeyCode(fm) == K_HELP)
			HelpLegajo(fm, fno, row);
  		else {

			if (FmKeyCode(fm) == K_META) {
				v_nroleg = ERROR;

				if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;
				FmSetLFld(fm, fno, v_nroleg, row);
			}

		}

		if (!ValidaLegajoXusr(FmLFld(fm, NROLEG, row), MAX_DATE))
			if (!FmIsNull(fm, fno, row)) {
				WiDialog(WD_OK, WD_OK, "Error", "Legajo Inactivo");
				return FM_REDO;
			}

	  	FmSetFld(fm, DESC0, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row)), row);

		GetDeleg_FilXUsuario(FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row), Today(), v_deleg, v_filial);
		if (FmIsNull(fm, DELORI, row))
		  	FmSetFld(fm, DELORI, v_deleg , row);
		if (FmIsNull(fm, FILORI, row))
		  	FmSetFld(fm, FILORI, v_filial, row);

		break;


	case FECINI:
		if (FmDFld(fm,FECINI,row) < g_fecparcial) {
			WiDialog(WD_OK, WD_OK, "Error", "La fecha ingresada esta por debajo del cierre de operaciones %.3D", g_fecparcial);
			return FM_REDO;
		}
		if (FmDFld(fm,fno,row)< g_fecierre){
			WiDialog(WD_OK, WD_OK, "Error", "La fecha ingresada esta por debajo del cierre de operaciones %.3D", g_fecierre);
			return FM_REDO;
		}

		break;

	case AGRFEC:

		if (FmIsNull(fm0, FECFIN, row)){
			switch (FmIFld(fm0, TIPPER, row)) {
				case _TIPPER_INSERTA:
 					WiMsg("No se puede dejar sin fecha de finalizacion para este tipo de permiso");
					return FM_REDO;
				break;
			}
		}
		else {
			switch (FmIFld(fm0, TIPPER, row)) {
				case _TIPPER_ASIGNA:
 					WiMsg("Para este tipo de permiso no se puede ingresar fecha de finalizacion");
					return FM_REDO;
				break;
			}
			
		}

		// Control de que no se pisen los permisos 
		for (v_i=0; v_i<=FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {

			//Misma linea
			if (v_i==row)
				continue;

			//No es mismo legajo
			if(FmLFld(fm0, NROLEG, v_i)!=FmLFld(fm0, NROLEG, row))
				continue;

			//No se superponen las fechas 
			if(!FechasSuperpuestas(FmDFld(fm0, FECINI, row), FmDFld(fm0, FECFIN, row)==NULL_DATE? MAX_DATE : FmDFld(fm0, FECFIN, row), 
			                       FmDFld(fm0, FECINI, v_i), FmDFld(fm0, FECFIN, v_i)==NULL_DATE? MAX_DATE : FmDFld(fm0, FECFIN, v_i)))
				continue;

			WiDialog(WD_OK, WD_OK, "Error de Superpocicion de Fechas", "La fecha se superpone con la linea %d", v_i );
			return FM_REDO;
		} 
		break;

	case FILIAL:
    	if (FmKeyCode(fm) == K_HELP) {
			HelpFilXDel(fm, fno, row, FmSFld(fm, DELEGA, row));
    	}

		if (strcmp(FmSFld(fm, DELEGA, row), GetDelegacion(FmSFld(fm, fno, row)) ) ){
			WiDialog(WD_OK, WD_OK, "Error", "Filial %s No Es Valida Para La Delegacion %s\nLinea %d", FmSFld(fm, fno, row), FmSFld(fm, DELEGA, row), v_i );
			return FM_REDO;
		} 

		if (!strcmp(FmSFld(fm, FILORI, row), FmSFld(fm, fno, row)) ){
			WiDialog(WD_OK, WD_OK, "Error", "Filial %s No Es Valida Porque No Puede Ser Igual Que La Filial Origen\nLinea %d", FmSFld(fm, fno, row), v_i );
			return FM_REDO;
		} 
	    break;


	case ACTIVO:
		if (FmKeyCode(fm) == K_META) {
			fm1 = UseSubform(fm0, fno, 0, row);
			Lectura_fm1(fm1, row);
			DoSubform(fm0, NULLFP, NULLFP, fno, 0 , row);

			FmSetKeyCode(fm, K_ENTER);
		}
	}
	return FM_OK;				
}


static void Lectura()
{
	dbcursor v_c_pervig=NULL;
	int v_i=0;
	
	v_c_pervig=CreateCursor(operac|PERVIGbyULTMOD, IO_NOT_LOCK);
	SetCursorFrom(v_c_pervig, TRUE, FmIFld(fm0, EMP),  NULL_STR,   NULL_STR,    NULL_LONG,  NULL_SHORT);
	SetCursorTo  (v_c_pervig, TRUE, FmIFld(fm0, EMP),  HIGH_VALUE, HIGH_VALUE,  MAX_LONG,   MAX_SHORT);
	while(FetchCursor(v_c_pervig)!=ERROR) {

		if (DFld(operac|PERVIG_FECINI)<FmDFld(fm0, FECDES))
			continue;

		if (!ValidaFilialXusr(SFld(operac|PERVIG_FILORI)))
			continue;

        if (!IsNull(operac|PERVIG_FECINI) && !ValidaLegajoXusr(LFld(operac|PERVIG_NROLEG), DFld(operac|PERVIG_FECINI)))
			continue;

        if (!IsNull(operac|PERVIG_FECFIN) && !ValidaLegajoXusr(LFld(operac|PERVIG_NROLEG), DFld(operac|PERVIG_FECFIN)))
			continue;

		DbToFm(fm0, NROLEG, ACTIVO, v_i);
		FmSetFld(fm0, DESC0,  GetDescLegajo(IFld(operac|PERVIG_EMP), LFld(operac|PERVIG_NROLEG)));
		v_i ++;
	}
	DeleteCursor(v_c_pervig);
	
}

static void GrabaPermiso()
{
	bool v_ya_grabado=FALSE;

	int v_i=0,
	    v_nummod=1;

	dbcursor v_c_pervig=NULL;
	

	for (v_i=0; v_i<=FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {


		if (!Cambio(v_i))
			continue;

		// Grabo como historial si ya existia
//		SetKey(operac|PERVIGbyULTMOD, TRUE, FmIFld(fm0, EMP), FmSFld(fm0, DELEGA, v_i ), FmSFld(fm0, FILIAL, v_i ), FmLFld(fm0, NROLEG, v_i), NULL_SHORT);
//		if (GetRecord(operac|PERVIGbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2)!=ERROR) {


		SetKey(operac|PERVIGbyULTMOD, TRUE, FmIFld(fm0, EMP), NULL_STR, NULL_STR, NULL_LONG, NULL_SHORT);
		while(GetRecord(operac|PERVIGbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2)!=ERROR) {

			if (FmLFld(fm0, NROLEG, v_i) != LFld(operac|PERVIG_NROLEG))
				continue;
			
			v_nummod=IFld(operac|PERVIG_NUMMOD) + 1;

			SetIFld(operac|PERVIG_ULTMOD, FALSE);
			PutRecord(operac|PERVIG);
			break;
		}

		// Grabo nuevo registro
		FmToDb(fm0, EMP, FILIAL, v_i);
		SetIFld(operac|PERVIG_ULTMOD, TRUE);
		SetIFld(operac|PERVIG_ACTIVO, TRUE);
		SetIFld(operac|PERVIG_NUMMOD, v_nummod);
		SetIFld(operac|PERVIG_USUARI, GetUid());
		SetDFld(operac|PERVIG_FECHA, Today());
		SetTFld(operac|PERVIG_HORA, Hour());

		PutRecord(operac|PERVIG);

	}


	//Inactivo los que borro
	v_c_pervig=CreateCursor(operac|PERVIGbyULTMOD, IO_NOT_LOCK);
	SetCursorFrom(v_c_pervig, TRUE, FmIFld(fm0, EMP),  NULL_STR,   NULL_STR,    NULL_LONG,  NULL_SHORT);
	SetCursorTo  (v_c_pervig, TRUE, FmIFld(fm0, EMP),  HIGH_VALUE, HIGH_VALUE,  MAX_LONG,   MAX_SHORT);
	while(FetchCursor(v_c_pervig)!=ERROR) {
		if (!IFld(operac|PERVIG_ACTIVO))
			continue;

		if (DFld(operac|PERVIG_FECINI)<FmDFld(fm0, FECDES))
			continue;

		if (!ValidaFilialXusr(SFld(operac|PERVIG_FILORI)))
			continue;

        if (!IsNull(operac|PERVIG_FECINI) && !ValidaLegajoXusr(LFld(operac|PERVIG_NROLEG), DFld(operac|PERVIG_FECINI)))
			continue;

        if (!IsNull(operac|PERVIG_FECFIN) && !ValidaLegajoXusr(LFld(operac|PERVIG_NROLEG), DFld(operac|PERVIG_FECFIN)))
			continue;


		v_ya_grabado=FALSE;

		for (v_i=0; v_i<=FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++){
			if (FmLFld(fm0, NROLEG, v_i)==LFld(operac|PERVIG_NROLEG)){
				v_ya_grabado=TRUE;
				break;
			}
		}


		if (!v_ya_grabado){
			// Grabo como historial si ya existia
			v_nummod=IFld(operac|PERVIG_NUMMOD) + 1;
			SetIFld(operac|PERVIG_ULTMOD, FALSE);
			PutRecord(operac|PERVIG);

			// Grabo nuevo registro
			SetIFld(operac|PERVIG_ULTMOD, TRUE);
			SetIFld(operac|PERVIG_ACTIVO, FALSE);
			SetIFld(operac|PERVIG_NUMMOD, v_nummod);
			SetIFld(operac|PERVIG_USUARI, GetUid());
			SetDFld(operac|PERVIG_FECHA, Today());
			SetTFld(operac|PERVIG_HORA, Hour());

			PutRecord(operac|PERVIG);

		}

    }

}

static bool Cambio(int p_i) 
{


	SetKey(operac|PERVIGbyULTMOD, TRUE, FmIFld(fm0, EMP), FmSFld(fm0, DELEGA, p_i ), FmSFld(fm0, FILIAL, p_i ), FmLFld(fm0, NROLEG, p_i), NULL_SHORT);
	if (GetRecord(operac|PERVIGbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 5)==ERROR) 
		return TRUE;

	
	if (FmIFld(fm0, ACTIVO, p_i)!=IFld(operac|PERVIG_ACTIVO))
		return TRUE;

	if (FmIFld(fm0, TIPPER, p_i)!=IFld(operac|PERVIG_TIPPER))
		return TRUE;

	if (FmDFld(fm0, FECINI, p_i)!=DFld(operac|PERVIG_FECINI))
		return TRUE;

	if (FmDFld(fm0, FECFIN, p_i)!=DFld(operac|PERVIG_FECFIN))
		return TRUE;

	if (strcmp(FmSFld(fm0, DELORI, p_i), SFld(operac|PERVIG_DELORI))!=0)
		return TRUE;

	if (strcmp(FmSFld(fm0, FILORI, p_i), SFld(operac|PERVIG_FILORI))!=0)
		return TRUE;

	return FALSE;
	
}

static void Lectura_fm1(form p_fm1, int p_row_padre) 
{
	dbcursor v_c_pervig=NULL;
	int v_i=0, v_j=0, v_k=0, v_max;
	char v_aux [40];
	

	FmSetIFld(p_fm1, HEMP,     FmIFld(fm0, EMP));
	FmSetFld (p_fm1, HEMPDESC, FmSFld(fm0, EMPDESC));
	FmSetLFld(p_fm1, HNROLEG,  FmLFld(fm0, NROLEG, p_row_padre));
	FmSetFld (p_fm1, HDESC0,   GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, p_row_padre)));

	v_c_pervig=CreateCursor(operac|PERVIGbyEMP, IO_NOT_LOCK);

	SetCursorFrom(v_c_pervig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, p_row_padre), NULL_STR  , NULL_STR  , NULL_SHORT);
	SetCursorTo  (v_c_pervig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, p_row_padre), HIGH_VALUE, HIGH_VALUE, MAX_SHORT);
	MoveCursorLast(v_c_pervig);
	while(FetchCursorPrev(v_c_pervig)!=ERROR) {
		DbToFm(p_fm1, HDELORI, HFECHA, v_i);
		v_i ++;
	}
	DeleteCursor(v_c_pervig);

	// Ordeno por numero de modificacion
	for (v_i=0; v_i<FmFldLen(fm1, HMULTI0) && !FmIsNull(fm1, HDELORI, v_i); v_i++);

	v_max=v_i-1;

	for (v_i=0; v_i<v_max; v_i++) {
		for (v_j=0; v_j<v_max-v_i; v_j++) {
			if (FmIFld(fm1, HNUMMOD, v_j) > FmIFld(fm1, HNUMMOD, v_j + 1)) {
				for (v_k=HDELORI; v_k<=HFECHA; v_k++) {
					sprintf(v_aux,"%s", FmSFld(fm1, v_k, v_j));
					FmSetFld(fm1, v_k, FmSFld(fm1, v_k, v_j+1), v_j);
					FmSetFld(fm1, v_k, v_aux, v_j+1);
				} 
			}

		} 
	}


}

static void MuestraNrodeLineaActual(fmfield fno, int row)
{
	char v_auxi[16];

	if (FmInMult(fm0, fno)!=ERROR)
		sprintf(v_auxi, "¡¡¡Linea %03d¡¡¡", row);
	else 
		sprintf(v_auxi, "¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡", row);
	FmSetFld(fm0, LINEA, v_auxi);
	
}

