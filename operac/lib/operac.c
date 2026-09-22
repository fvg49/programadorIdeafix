/********************************************************************
*
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
* CREATED          : 11/09/98
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*                      |
*DATE FecIng           |Devuelve la fecha de Ingreso del Legajo
*********************************************************************/
#include <ideafix.h>
#include <math.h>
#include "operac.sch"
#include "comerc.sch"
#include "comgral.sch"
#include "operac.h"
#include "disths.h"
#include "opedef.h"
#include "opedef.h"
#include "comerc.h"
#include "comgral.h"
#include "disthspro.h"
#include "ambiente.h"
#include "webinter.h"
#include "asist.sch"
#include "brigada.sch"
#include "sue.sch"
#include "billpro.sch"
#include "bill.sch"
#include "aurcus.sch"


char   horae[60], horas[60],  hsent[60], hssal[60], minue[30], minus[30];
bool BajarHoras(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, int codint);

DATE FechaFinalOt (long cliente, short objetivo, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, bool baja);
DATE FecIng(int emp, long nroleg);
bool EncontroFrancoRol (int emp, long nroleg, DATE fecha, char *vigil , bool *rolfranco);
int AsignoHoras(int p_hora, int *p_tothora) ;
int v_getline(char *line, int max, FILE *archi);


//-------------------------* AlOtroDia *-----------------------
// Devuelve -TRUE  si al dia siguiente de la fecha el vigilador esta asignado a algun puesto,
//          -FALSE si no esta asignado. 
bool AlOtroDia(int emp, long legajo, DATE fecha, bool enasig)
{
	dbtable	AASIG, AASIGH;
	schema old, operac;
	char dia[2];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG  = CreateAlias(operac|ASIG);
	AASIGH = CreateAlias(operac|ASIGH);
	sprintf(dia, "%1.1s", DiaLetra(fecha));

	if (enasig) {
		SetIFld(AASIG_EMP,      emp);
		SetLFld(AASIG_NROLEG,   legajo);
		SetLFld(AASIG_CLIENTE,  NULL_LONG);
		SetIFld(AASIG_OBJETIVO, NULL_SHORT);
		while (GetRecord(AASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (!strcmp(dia, SFld(AASIG_DIA1)) || !strcmp(dia, SFld(AASIG_DIA2)) ||
				!strcmp(dia, SFld(AASIG_DIA3)) || !strcmp(dia, SFld(AASIG_DIA4)) ||
				!strcmp(dia, SFld(AASIG_DIA5)) || !strcmp(dia, SFld(AASIG_DIA6)) ||
				!strcmp(dia, SFld(AASIG_DIA7)))
				return TRUE;
		}
	}
	else {
		SetIFld(AASIGH_EMP,      emp);
		SetLFld(AASIGH_NROLEG,   legajo);
		SetLFld(AASIGH_CLIENTE,  NULL_LONG);
		SetIFld(AASIGH_OBJETIVO, NULL_SHORT);
		while (GetRecord(AASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (DFld(operac|ASIGH_FECBAJ) < fecha)
				continue;
			if (!strcmp(dia, SFld(AASIGH_DIA1)) || !strcmp(dia, SFld(AASIGH_DIA2)) ||
				!strcmp(dia, SFld(AASIGH_DIA3)) || !strcmp(dia, SFld(AASIGH_DIA4)) ||
				!strcmp(dia, SFld(AASIGH_DIA5)) || !strcmp(dia, SFld(AASIGH_DIA6)) ||
				!strcmp(dia, SFld(AASIGH_DIA7)))
				return TRUE;
		}
	}  
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);
	return FALSE;
}

// ** Devuelve TRUE si en ASIGH el vigilador existe en el cliente/objetivo **
// bool prov: agregado para que confpar.c toma los provisorios y parte.c no. 
//-------------------------* ExisteEnAsigh *-----------------------
bool ExisteEnAsigh(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint, bool prov)
{
	schema old, operac;
    bool enasigh = FALSE;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	enasigh = FALSE;

	SetIFld(ASIGH_EMP,      emp);
	SetLFld(ASIGH_CLIENTE,  cliente);
	SetIFld(ASIGH_OBJETIVO, objetivo);
	SetIFld(ASIGH_PTOSER,   ptoser);
	SetIFld(ASIGH_PUESTO,   puesto);
	SetIFld(ASIGH_NROINT,   nroint);
	SetLFld(ASIGH_NROLEG,   legajo);
	SetDFld(ASIGH_FECBAJ,   NULL_DATE);
	SetDFld(ASIGH_FECALT,   NULL_DATE);
	while (GetRecord(ASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
		if (!strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO) && !prov) {
			continue;
		}
		if (IFld(ASIGH_MOTIVO) == ALTAPARTE)
			continue;

		if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
			enasigh = TRUE;                                             
		}
		if (StrCmp(SFld(ASIGH_VIGIL), PARTTIME) == 0 && StrCmp(SFld(ASIGH_DIA1), "P") == 0) {
			SetKey(operac|DIASPTIMEHbyEMP, emp, cliente, objetivo, legajo, ptoser, puesto, nroint, fecparte);
			if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
				enasigh = TRUE;
		}
	}
	SwitchToSchema(old);
	return enasigh;
}

// *** Devuelve TRUE si en ASIG el vigilador existe en el cliente/objetivo **
// *-------------------------* ExisteEnAsig *-----------------------
bool ExisteEnAsig(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint, bool prov)
{
	schema old, operac;
    bool enasig = FALSE;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objetivo);
	SetLFld(ASIG_NROLEG,   legajo);
	SetIFld(ASIG_PTOSER,   ptoser);
	SetIFld(ASIG_PUESTO,   puesto);
	SetIFld(ASIG_NROINT,   nroint);
	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR && fecparte >= DFld(ASIG_FECASIG)) { 
		if (Franco(emp, legajo, fecparte, SFld(ASIG_VIGIL), IFld(ASIG_NUMFRAN))) {
       		enasig = TRUE;
        }
		else {
		    if (DiasTrabajados(DiaLetra(fecparte), NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR,
						   SFld(ASIG_DIA1), SFld(ASIG_DIA2),SFld(ASIG_DIA3),SFld(ASIG_DIA4), SFld(ASIG_DIA5),
						   SFld(ASIG_DIA6),SFld(ASIG_DIA7)))
				enasig = TRUE;

			if (StrCmp(SFld(ASIG_VIGIL), PARTTIME) == 0 && StrCmp(SFld(ASIG_DIA1), "P") == 0) {
				SetKey(operac|DIASPTIMEbyEMP, emp, cliente, objetivo, legajo, ptoser, puesto, nroint, fecparte);
				if (GetRecord(operac|DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
					enasig = TRUE;
			}
			else {
				SetKey(operac|DIASPTIMEHbyEMP, emp, cliente, objetivo, legajo, ptoser, puesto, nroint, fecparte);
				if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
					enasig = TRUE;
			}
	 	}
	}
	else {
		enasig = ExisteEnAsigh(emp, cliente, objetivo, legajo, fecparte, ptoser, puesto, nroint, prov);
	}
	SwitchToSchema(old);
	return enasig;
}

// *** Devuelve TRUE si trabaja el dia en puesto efectivo **
// *-------------------------* TrabDiaEnPtoEfec *-----------------------*
bool TrabDiaEnPtoEfec(int emp, long cliente, int objetivo, long legajo, DATE fecparte)
{
	schema old, operac;  
	dbtable AASIG, AASIGH;
    bool trabaja = FALSE;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);

	AASIG  = CreateAlias(operac|ASIG);
	AASIGH = CreateAlias(operac|ASIGH);
	SetIFld(AASIG_EMP,      emp);
	SetLFld(AASIG_NROLEG,   legajo);
	SetLFld(AASIG_CLIENTE,  NULL_LONG);
	SetIFld(AASIG_OBJETIVO, NULL_SHORT);
 	while (GetRecord(AASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (fecparte < DFld(AASIG_FECASIG))
			continue;
		if (!EsEfectivo(emp, LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO), legajo, IFld(AASIG_PTOSER), IFld(AASIG_PUESTO), IFld(AASIG_NROINT),fecparte))
			continue;        
		if (DiasTrabajados(DiaLetra(fecparte), NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR,
						   SFld(AASIG_DIA1), SFld(AASIG_DIA2),SFld(AASIG_DIA3),SFld(AASIG_DIA4), SFld(AASIG_DIA5),
						   SFld(AASIG_DIA6),SFld(AASIG_DIA7)))
			trabaja = TRUE;

	}
	if (!trabaja) {
		SetIFld(AASIGH_EMP,      emp);
		SetLFld(AASIGH_NROLEG,   legajo);
		SetLFld(AASIGH_CLIENTE,  NULL_LONG);
		SetIFld(AASIGH_OBJETIVO, NULL_SHORT);
	 	while (GetRecord(AASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (!EsEfectivo(emp, LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO), legajo, IFld(AASIGH_PTOSER), IFld(AASIGH_PUESTO), IFld(AASIGH_NROINT), fecparte))
				continue;        
			if (fecparte >= DFld(AASIGH_FECALT) && fecparte <= DFld(AASIGH_FECBAJ)) {
				if (DiasTrabajados(DiaLetra(fecparte), NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR, NULL_STR,
						   SFld(AASIGH_DIA1), SFld(AASIGH_DIA2), SFld(AASIGH_DIA3), SFld(AASIGH_DIA4),
						   SFld(AASIGH_DIA5), SFld(AASIGH_DIA6), SFld(AASIGH_DIA7)))
					trabaja = TRUE;
			}
		}
	}
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);
    return trabaja;
}

// *** Devuelve TRUE si el vigilador esta asignado a un puesto Part Time en forma Efectiva. **
// *-------------------------* VigPartime *-----------------------
bool VigPartime(int emp, long legajo, DATE fecparte)
{
	schema old, operac;
	bool partime = FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_NROLEG,   legajo);
	SetLFld(ASIG_CLIENTE,  MIN_LONG);
	SetIFld(ASIG_OBJETIVO, MIN_SHORT);
	while (GetRecord(ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (!strcmp(SFld(ASIG_VIGIL), PARTTIME) && fecparte >= DFld(ASIG_FECASIG))
			partime = TRUE;
	}
	if (!partime) {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_NROLEG,   legajo);
		SetLFld(ASIGH_CLIENTE,  MIN_LONG);
		SetIFld(ASIGH_OBJETIVO, MIN_SHORT);
		while (GetRecord(ASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (IFld(ASIGH_MOTIVO) == DESXERROR || IFld(ASIGH_MOTIVO) == ALTAPARTE)
				continue;

			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
				if (!strcmp(SFld(ASIGH_VIGIL), PARTTIME))
					partime = TRUE;
				else
					partime = FALSE;
			}
		}
	}
	SwitchToSchema(old);
	return partime;
}

// *** Devuelve TRUE si el vigilador esta asignado a un puesto Part Time para un cliente/objetivo dado.**
// *-------------------------* EsPuestoPartime *-----------------------
bool EsPuestoPartime(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte)
{
	schema old, operac;
	bool partime = FALSE;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objetivo);
	SetLFld(ASIG_NROLEG,   legajo);
	SetIFld(ASIG_PTOSER,   ptoser);
	SetIFld(ASIG_PUESTO,   puesto);
	SetIFld(ASIG_NROINT,   nroint);
	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if (!strcmp(SFld(ASIG_VIGIL), PARTTIME))
			partime = TRUE;
	}
	else {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_CLIENTE,  cliente);
		SetIFld(ASIGH_OBJETIVO, objetivo);
		SetLFld(ASIGH_NROLEG,   legajo);
		SetIFld(ASIGH_PTOSER,   ptoser);
		SetIFld(ASIGH_PUESTO,   puesto);
		SetIFld(ASIGH_NROINT,   nroint);
		if (GetRecord(ASIGHbyPUESTO, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
				if (!strcmp(SFld(ASIGH_VIGIL), PARTTIME))
					partime = TRUE;
				else
					partime = FALSE;
			}
		}
	}
	SwitchToSchema(old);
	return partime;
}

// *** Devuelve TRUE si el vigilador esta asignado en forma efectiva a un puesto. **
// *-------------------------* EsEfectivo *-----------------------
bool EsEfectivo(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte)
{
	schema old, operac;
	bool efectivo = FALSE;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objetivo);
	SetLFld(ASIG_NROLEG,   legajo);
	SetIFld(ASIG_PTOSER,   ptoser);
	SetIFld(ASIG_PUESTO,   puesto);
	SetIFld(ASIG_NROINT,   nroint);
	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if (!strcmp(SFld(ASIG_EFECT), EFECTIVO)) {
			efectivo = TRUE;
		}
	}
	else {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_CLIENTE,  cliente);
		SetIFld(ASIGH_OBJETIVO, objetivo);
		SetIFld(ASIGH_PTOSER,   ptoser);
		SetIFld(ASIGH_PUESTO,   puesto);
		SetIFld(ASIGH_NROINT,   nroint);
		SetLFld(ASIGH_NROLEG,   legajo);
		SetDFld(ASIGH_FECBAJ,   MIN_DATE);
		SetDFld(ASIGH_FECALT,   MIN_DATE);
		while (GetRecord(ASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
				if (!strcmp(SFld(ASIGH_EFECT), EFECTIVO)) {
					efectivo = TRUE;
				}
				else {
					efectivo = FALSE;
				}
			}
		}
	}
	SwitchToSchema(old);
	return efectivo;
}

// *** Devuelve el tipo de vigilador ASIG_VIGIL. ***
// *-------------------------* TipoVig *-----------------------*
char * TipoVig(int emp, long cliente, int objetivo, long legajo, int tippto, int puesto, int nroint, DATE fecparte)
{
	schema old, operac;
	static char vigil[2];

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objetivo);
	SetLFld(ASIG_NROLEG,   legajo);
	SetIFld(ASIG_PTOSER,   tippto);
	SetIFld(ASIG_PUESTO,   puesto);
	SetIFld(ASIG_NROINT,   nroint);
	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		sprintf(vigil, SFld(ASIG_VIGIL));
	}
	else {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_CLIENTE,  cliente);
		SetIFld(ASIGH_OBJETIVO, objetivo);  
		SetLFld(ASIGH_NROLEG,   legajo);		
		SetIFld(ASIGH_PTOSER,   tippto);
		SetIFld(ASIGH_PUESTO,   puesto);
		SetIFld(ASIGH_NROINT,   nroint);
		if (GetRecord(ASIGHbyPUESTO, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
				sprintf(vigil, SFld(ASIGH_VIGIL));
			}
		}
	}
	SwitchToSchema(old);
	return vigil;
}


// *--------------* HoraEntEfectivo *-----------------
TIME HoraEntEfectivo(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrent=NULL_TIME;
	schema	 old, operac;
	dbtable	 AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
	c_asig	= CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;
		encontre = TRUE;
		hrent = TFld(AASIG_HSENT);
//		return TFld(AASIG_HSENT);
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				hrent = TFld(AASIGH_HSENT);
//				return TFld(AASIGH_HSENT);
			}
		}
	}
 	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);
	return hrent;
}

// --------------* HoraSalEfectivo *-----------------
TIME HoraSalEfectivo(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrsal=NULL_TIME;
	schema	 old, operac;
	dbtable	 AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
	c_asig	= CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;
		encontre = TRUE;

		hrsal = TFld(AASIG_HSSAL); 
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				hrsal = TFld(AASIGH_HSSAL);
//				return TFld(AASIGH_HSSAL);
			}                     
		}
	}
 	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);
	return hrsal;
}

//--------------* HrEntTrabEfec *-----------------
TIME HrEntTrabEfec(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrent=NULL_TIME;
	schema	 old, operac;
	dbtable	 AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
	c_asig	= CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;

		encontre = TRUE;

		hrent = HoraEntParte(emp, LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO), fecha, nroleg,
							 IFld(AASIG_PTOSER), IFld(AASIG_PUESTO), IFld(AASIG_NROINT));
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				hrent = HoraEntParte(emp, LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO), fecha, nroleg,
									 IFld(AASIGH_PTOSER), IFld(AASIGH_PUESTO), IFld(AASIGH_NROINT));
			}
		}
	}
 	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);
	return hrent;
}

// --------------* HrSalTrabEfec *-----------------
TIME HrSalTrabEfec(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrsal=NULL_TIME;
	schema	 old, operac;
//	dbtable	 AASIG, AASIGH, APARTE;
	dbtable	 AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
//	APARTE	= CreateAlias(operac|PARTE);
	c_asig	= CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;

		encontre = TRUE;

		hrsal = HoraSalParte(emp, LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO), fecha, nroleg,
							 IFld(AASIG_PTOSER), IFld(AASIG_PUESTO), IFld(AASIG_NROINT));
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				hrsal = HoraSalParte(emp, LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO), fecha, nroleg,
									 IFld(AASIGH_PTOSER), IFld(AASIGH_PUESTO), IFld(AASIGH_NROINT));
			}
		}
	}
 	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
//	DeleteAlias(APARTE);
	SwitchToSchema(old);
	return hrsal;
}

// --------------* HoraEntParte *-----------------
TIME HoraEntParte(int emp, long cliente, int objet, DATE fecha, long nroleg, int ptoser, int puesto, int nroint)
{
	schema	old, operac;
	dbtable	APARTE;
	TIME    hrent;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	APARTE	= CreateAlias(operac|PARTE);

	SetIFld(APARTE_EMP,      emp);
	SetLFld(APARTE_CLIENTE,  cliente);
	SetIFld(APARTE_OBJETIVO, objet);
	SetDFld(APARTE_DIA,      fecha);
	SetLFld(APARTE_NROLEG,   nroleg);
	SetIFld(APARTE_PTOSER,   ptoser);
	SetIFld(APARTE_PUESTO,   puesto);
	SetIFld(APARTE_NROINT,   nroint);
	if (GetRecord(APARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		hrent = TFld(APARTE_HORAENT);
	else
		hrent = StrToT("00:00");

	DeleteAlias(APARTE);
	return hrent;
}

// --------------* HoraSalParte *-----------------
TIME HoraSalParte(int emp, long cliente, int objet, DATE fecha, long nroleg, int ptoser, int puesto, int nroint)
{
	schema	old, operac;
	dbtable	APARTE;
	TIME    hrsal;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	APARTE	= CreateAlias(operac|PARTE);

	SetIFld(APARTE_EMP,      emp);
	SetLFld(APARTE_CLIENTE,  cliente);
	SetIFld(APARTE_OBJETIVO, objet);
	SetDFld(APARTE_DIA,      fecha);
	SetLFld(APARTE_NROLEG,   nroleg);
	SetIFld(APARTE_PTOSER,   ptoser);
	SetIFld(APARTE_PUESTO,   puesto);
	SetIFld(APARTE_NROINT,   nroint);
	if (GetRecord(APARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		hrsal = TFld(APARTE_HORASAL);
	}
	else {
		hrsal = StrToT("00:00");
	}
	DeleteAlias(APARTE);
	return hrsal;
}

// -------------------* GetPtoEfectivo *--------------------
void GetPtoEfectivo(int emp, long nroleg, int *ptoser, int *puesto)
{
	schema   old, operac;
	dbtable  AASIG;
	dbcursor c_asig;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIG   = CreateAlias(operac|ASIG);

	c_asig  = CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);

	*ptoser = NULL_SHORT;
	*puesto = NULL_SHORT;

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO))
			continue;

		*ptoser = IFld(AASIG_PTOSER);
		*puesto = IFld(AASIG_PUESTO);
	}
	DeleteCursor(c_asig);
	DeleteAlias(AASIG);
}

// -------------------------* GetTipPto *-----------------------
int GetTipPto(int emp, long cliente, int objetivo, long legajo)
{
	schema old, operac;
	int tippto;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetIFld(ASIG_EMP, 		emp		);
	SetLFld(ASIG_CLIENTE, 	cliente );
	SetIFld(ASIG_OBJETIVO,  objetivo);
	SetLFld(ASIG_NROLEG, 	legajo  );

	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		tippto = IFld(ASIG_PTOSER);
	else
		tippto = NULL_SHORT;

	SwitchToSchema(old);
	return tippto;
}


// -------------------------* GetPuesto *-----------------------
int GetPuesto(int emp, long cliente, int objetivo, long legajo)
{
	schema old, operac;
	int puesto;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objetivo);
	SetLFld(ASIG_NROLEG,   legajo);

	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		puesto = IFld(ASIG_PUESTO);
	else
		puesto = NULL_SHORT;

	SwitchToSchema(old);
	return puesto;
}
    

// -------------------------* GetRegimen *-----------------------
char *  GetRegimen(long cliente, int objetivo, int tippto, int puesto)
{
	schema old, operac;
	static char regimen[20];

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	SetLFld(PUESTOS_CLIENTE, cliente);
	SetIFld(PUESTOS_OBJET,   objetivo);
	SetIFld(PUESTOS_TIPPTO,  tippto);
	SetIFld(PUESTOS_CODINT,  puesto);
                             
	if (GetRecord(PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strcpy(regimen, SFld(PUESTOS_REGIM));
	else
		strcpy(regimen, NULL_STR);

	SwitchToSchema(old);
	return regimen;
}


// -------------------------* FeriadoNovia *-----------------------
bool FeriadoNovia(DATE fecha, int pais, int prov)
{
	schema old, operac;
	bool feriado;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetDFld(FERIADO_FECHA, fecha);
	SetIFld(FERIADO_PAIS,  pais);
	SetIFld(FERIADO_PROV,  prov);
	if ( GetRecord(FERIADObyFECHA, THIS_KEY, IO_NOT_LOCK) != ERROR)
		feriado = TRUE;
	else
		feriado = FALSE;

	SwitchToSchema(old);
	return feriado;
}


// -------------------------* Franco *-----------------------
bool Franco(int emp, long nroleg, DATE fecha, char vigil[2], int numfran)
{
	schema	old, operac;
	bool	esfranco = FALSE, salir = FALSE;
	int		diaslab, diasfranco;
	DATE	fecfranco = NULL_DATE, ffrancod, ffrancoh, ftrabd, ftrabh;
	char	regimen[50];
	short nfran;


	//Si esta asignado con un rol devuelvo si es franco en base al ROL
	if (EncontroFrancoRol (emp, nroleg, fecha, vigil , &esfranco)){
//		fprintf (stderr, "franco %ld %.3D %d \n", nroleg, fecha, esfranco);
		return esfranco;
	}

	//Devuelvo si es franco en base al Regimen

	esfranco = FALSE;
	// los PARTIME no tienen feriados!
	if (!strcmp(vigil, PARTTIME))
		return FALSE;

	// Calculo del franco para vigiladores y retenes
	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	GetRegimenEfectivo(emp, nroleg, regimen, fecha);
	GetFFrancoEfectivo(emp, nroleg, &fecfranco, &nfran, fecha);
	numfran = nfran;

	if (fecfranco == NULL_DATE || !strcmp(regimen, NULL_STR))
		return FALSE;

	// porque la primer fecha de franco del periodo es ASIG_FFRANCO
	// con lo cual cualquier fecha menor es laborable.
	if (fecha < fecfranco)
		return FALSE;

	// saco cantidad de dias de franco y cantidad de dias laborables que tiene el empleado
	diasfranco = GetDiasFranco(regimen, FALSE);
	diaslab    = GetDiasLaboral(regimen, FALSE);

	if (diasfranco == NULL_SHORT || diaslab == NULL_SHORT)
		return FALSE;

	// saco primer rango de fechas donde tiene franco
	ffrancod = fecfranco;

	//fecha de franco desde es: la fecha de franco desde mas la cantidad de días franco pendientes
	// cantidad de días francos totales menos  numfran - 1 (dias pasados)

	if (numfran != NULL_SHORT) {
		ffrancoh = ffrancod + (diasfranco - (numfran - 1) - 1);
	}
	else {
		ffrancoh = ffrancod + diasfranco - 1;
	}
	// fin saco primer rango de fechas donde tiene franco

	// saco primer rango de fechas donde tiene que trabajar
	ftrabd = ffrancoh + 1;
	ftrabh = ftrabd   + diaslab - 1;

	while (!salir && !esfranco) {
		if (fecha >= ffrancod && fecha <= ffrancoh) {
			esfranco = TRUE;
		}
		if (fecha >= ftrabd && fecha <= ftrabh) {
			salir = TRUE;
		}
		ffrancod = ftrabh   + 1;
		ffrancoh = ffrancod + diasfranco - 1;
		ftrabd   = ffrancoh + 1;
		ftrabh   = ftrabd   + diaslab    - 1;
	}
	return esfranco;
}

// ---------------------------* GetRegimenEfectivo *--------------------------
void GetRegimenEfectivo(int emp, long nroleg, char *regimen, DATE fecha)
{
	bool     encontre = FALSE;
	schema   old, operac;
	dbtable  AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIG   = CreateAlias(operac|ASIG);
	AASIGH  = CreateAlias(operac|ASIGH);
	c_asig  = CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh = CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	strcpy(regimen, NULL_STR);
	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;
		encontre = TRUE;
		strcpy(regimen, SFld(AASIG_REGIM));
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;

			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				strcpy(regimen, SFld(AASIGH_REGIM));
			}
		}
	}
	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
}

//----------------------------* GetFFrancoEfectivo *--------------------------
void GetFFrancoEfectivo(int emp, long nroleg, DATE *fecfranco, short *nfran, DATE fecha) 
{

	bool		encontre = FALSE;
	schema		old, operac;
	dbtable		AASIG, AASIGH;
	dbcursor	c_asig, c_asigh;

	(*fecfranco) = NULL_DATE;
	(*nfran) = NULL_SHORT;

	old		= CurrentSchema();
	operac	= OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
	c_asig	= CreateCursor(AASIGbyNROLEG, IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);
	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO))
			continue;
		if(fecha < DFld(AASIG_FECASIG))
			continue;			
		(*fecfranco) = DFld(AASIG_FFRANCO);
		(*nfran) = IFld(AASIG_NUMFRAN);
		encontre = TRUE;
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (!encontre && FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				(*fecfranco) = DFld(AASIGH_FFRANCO);
				(*nfran) = IFld(AASIGH_NUMFRAN);
				encontre = TRUE;
			}
		}
	}
	DeleteCursor(c_asigh);
	DeleteCursor(c_asig);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
}

//----------------------------* GetNumFrancoEfectivo *--------------------------
int GetNumFrancoEfectivo(int emp, long nroleg, DATE fecha) 
{
	bool		encontre = FALSE;
	schema		old, operac;
	dbtable		AASIG, AASIGH;
	dbcursor	c_asig, c_asigh;
	int			numfranco;

	old		= CurrentSchema();
	operac	= OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	numfranco = NULL_SHORT;

	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);

	c_asig	= CreateCursor(AASIGbyNROLEG, IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);

	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO) || DFld(AASIG_FECASIG) > fecha)
			continue;

		numfranco = IFld(AASIG_NUMFRAN);
		encontre = TRUE;
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (!encontre && FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				numfranco = IFld(AASIGH_NUMFRAN);
			}
		}
	}
	DeleteCursor(c_asigh);
	DeleteCursor(c_asig);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);

	return numfranco;
}

int GetCantHoras(TIME hdesde, TIME hhasta)
{
	TIME tothora;
	int i, j;
	static char inthora[5], hora[6];

	if ( hhasta < hdesde ) {
		tothora =  StrToT("235958") + StrToT("000002") - hdesde + hhasta;
	}	
	else {
		tothora =  hhasta - hdesde + StrToT("000002");
	}
	TToStr(tothora, hora, TFMT_SEPAR);

	for (i = 0, j = 0; i<6; i++) {
		if (hora[i] == ':')
			continue;

		inthora[j++] = hora[i];
	}
	if ((StrToI(inthora)%2) == 0)
		return StrToI(inthora);
	else
		return StrToI(inthora) + 41;
}


char * GetDescCond(int condicion)
{
	schema old, operac;
	static char cond[20];

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);	

	SetIFld(CONDICION_CODCOND, condicion);
	if ( GetRecord(CONDICIONbyCODCOND, THIS_KEY, IO_NOT_LOCK) != ERROR )
		strcpy(cond, SFld(CONDICION_DESCOR));
	else
		strcpy(cond, NULL_STR);

	SwitchToSchema(old);
	return cond;
}


char * GetDescMotivo(int condicion, int motivo)
{
	schema old, operac;
	static char motiv[20];

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);	

	SetIFld(MOTEXC_CODCOND, condicion);
	SetIFld(MOTEXC_CODMOT,  motivo   );	

	if ( GetRecord(MOTEXCbyCODCOND, THIS_KEY, IO_NOT_LOCK) != ERROR )
		strcpy(motiv, SFld(MOTEXC_DESCRIP));
	else
		strcpy(motiv, NULL_STR);

	SwitchToSchema(old);
	return motiv;
}

DATE GetFirstDay(int mes, int anio)
{
	static char fecha[10], smes[4], sanio[6];

	strcpy(fecha, "01" );
	IToStr(mes,   smes );
	strcat(fecha, smes );
	IToStr(anio,  sanio);
	strcat(fecha, sanio);

	return StrToD(fecha);
}

DATE GetLastDay(int mes, int anio)
{
	static char fecha[10], smes[4], sanio[6];

	strcpy(fecha, "01" );
	IToStr(mes,   smes );
	strcat(fecha, smes );
	IToStr(anio,  sanio);
	strcat(fecha, sanio);

	return StrToD(fecha);
}

long ObtenerLugPag (long cliente, short objetivo)
{
	schema prev, com;
	
	prev = CurrentSchema ();
	
	if ( (com=FindSchema ("comerc")) == (schema) ERROR)
		com = OpenSchema("comerc", IO_EABORT);

	SwitchToSchema (prev);

	SetKey(com|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord (com|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		return NULL_LONG;
	
	return  LFld(com|OBJETIVO_OFPAG);
}

long ArmarCCosto(long cliente, int objetivo)
{
	char ccosto[9], auxobj[3];

	sprintf(ccosto,"%ld", cliente);

	if (objetivo <= 9)
		strcat(ccosto, "0");

	sprintf(auxobj, "%d", objetivo);
	strcat(ccosto, auxobj);

	return StrToL(ccosto);
}

double ConvHraInt(TIME hdesde, TIME hhasta)
{
	int	   i, j, inthre, inthrs;
	double intmie = 0.0, intmis = 0.0, total = 0.0;
	bool   dospto = FALSE;
	char aux[100];

	// Cuando seteas la hora 0 el valor que te toma es 00:02 
	if ((hdesde == hhasta) && hdesde == StrToT("00:02") && hhasta == StrToT("00:02"))
		return total = 0.0;
	if ((hdesde == hhasta) && hdesde != StrToT("00:00") && hhasta != StrToT("00:00"))
		return total = 24.0;

	if (hdesde == StrToT("23:00") && hhasta == StrToT("23:59"))
		return total = 1.0;

	if (hdesde == StrToT("23:15") && hhasta == StrToT("23:59"))
		return total = 0.75;

	if (hdesde == StrToT("23:30") && hhasta == StrToT("23:59"))
		return total = 0.50;

	if (hdesde == StrToT("23:45") && hhasta == StrToT("23:59"))
		return total = 0.25;

	TToStr(hdesde, hsent, TFMT_SEPAR);
	TToStr(hhasta, hssal, TFMT_SEPAR);

	for (i = 0, j = 0; i<6; i++) {
		if (hsent[i] == ':') {
			dospto = TRUE;
			j = 0;
			continue;
		}
		if (!dospto)
			horae[j++] = hsent[i];
		else
			minue[j++] = hsent[i];
	}
	dospto = FALSE;
	for (i = 0, j = 0; i<6 ; i++) {
		if (hssal[i] == ':') {
			dospto = TRUE;
			j = 0;
			continue;
		}
		if (!dospto)
			horas[j++] = hssal[i];
		else
			minus[j++] = hssal[i];
	}
	inthre = StrToI(horae);
	inthrs = StrToI(horas);
	intmis = StrToF(minus)/60;
	if (StrToF(minue)/60 > 0)
		intmie = 1 - StrToF(minue)/60;
	else
		intmie = 0;

	if (inthre < inthrs) {
		if (!strcmp(hssal, "23:59")) {
			total = inthrs - inthre + 1;
			intmis = 0.0;
		}
		else {
			total  = inthrs - inthre;
		}
	}
	else {
		if (inthre > inthrs) {
			if (!strcmp(hsent, "23:59")) {
				total = inthrs + 24 - inthre - 1.0;
				intmie = 0.0;
			}
			else
				total  = inthrs + 24 - inthre;
		}
		else {
			// Los minutos nunca seran iguales porque sino ya salio al principio por ser hr y minuto iguales 
			if (StrToF(minue) > StrToF(minus))
				total = 24;
			if (StrToF(minue) < StrToF(minus))
				total = 0;
		}
	}

	if (intmie != 0)
		total--;
	sprintf(aux, " %f  %f  %f  %f", total, intmie, intmis, intmie+intmis); // este sprintf parece que no tiene sentido pero si se borra deja de funcionar. NO BORRAR. DHC
	return total += intmie + intmis;
}

bool DesasigTieneHsCargadas(int emp, long cliente, int objetivo, long nroleg, DATE fdesde, DATE fhasta)
{
	schema   old, operac;
	dbtable  ALPARTE;
	dbcursor c_parte;
	bool     tieneasig = FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	ALPARTE = CreateAlias(operac|PARTE);
	c_parte = CreateCursor(AlInd(ALPARTE, operac|PARTEbyLEG), IO_NOT_LOCK);

	SetCursorFrom(c_parte, emp, cliente, objetivo, nroleg, fdesde);
	SetCursorTo  (c_parte, emp, cliente, objetivo, nroleg, fhasta);
	while (FetchCursor(c_parte) != ERROR && !tieneasig) {
		if (FFld(AlFld(ALPARTE, operac|PARTE_HSNOR))   != 0.00 ||
			FFld(AlFld(ALPARTE, operac|PARTE_HS50))    != 0.00 ||
			FFld(AlFld(ALPARTE, operac|PARTE_HS100F))  != 0.00 ||
			FFld(AlFld(ALPARTE, operac|PARTE_HS100FE)) != 0.00) {
			tieneasig = TRUE;
		}
	}
	DeleteCursor(c_parte);
	DeleteAlias(ALPARTE);
	SwitchToSchema(old);
	return tieneasig;
}

int HorasStd(long cliente, int objetivo, int tippto, DATE fecha)
{
	schema  old, operac;
	dbcursor c_CPTO;
	double  std = 0.0;
	char dia[2];

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	sprintf(dia, "%1.1s",DiaLetra(fecha));
	c_CPTO = CreateCursor(PUESTOSbyCLIENTE, IO_NOT_LOCK);

	if (tippto != NULL_SHORT) {
		SetCursorFrom(c_CPTO, cliente, objetivo, tippto, MIN_SHORT);
		SetCursorTo  (c_CPTO, cliente, objetivo, tippto, MAX_SHORT);
	}
	else {
		SetCursorFrom(c_CPTO, cliente, objetivo, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_CPTO, cliente, objetivo, MAX_SHORT, MAX_SHORT);
	}
	while(FetchCursor(c_CPTO) != ERROR) {

		if (IFld(PUESTOS_CANTVIG) == 0)
			continue;

		if (!strcmp(SFld(PUESTOS_DIA1), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA2), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA3), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA4), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA5), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA6), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
		if (!strcmp(SFld(PUESTOS_DIA7), dia)) {
			std += ConvHraInt(TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL)) * IFld(PUESTOS_CANTPUE);
			continue;
		}
//		sprintf(stderr, "codint %d, ini %.3T  sal %.3T horas %.2f", IFld(PUESTOS_CODINT), TFld(PUESTOS_HINICIO), TFld(PUESTOS_HFINAL), std);

	}
	SwitchToSchema(old);


	return std*100;
}

char * DiaLetra(DATE fecha)
{
	static char fecname[15];
	static char nro[2];

	strcpy(fecname, DayName(fecha));

	if (!strcmp(fecname, "LUNES")) {
		strcpy(nro, "L");
	}
	if (!strcmp(fecname, "MARTES")) {
		strcpy(nro, "M");
	}
	if (!strcmp(fecname, "MIERCOLES")) {
		strcpy(nro, "X");
	}
	if (!strcmp(fecname, "JUEVES")) {
		strcpy(nro, "J");
	}
	if (!strcmp(fecname, "VIERNES")) {
		strcpy(nro, "V");
	}
	if (!strcmp(fecname, "SABADO")) {
		strcpy(nro, "S");
	}
	if (!strcmp(fecname, "DOMINGO")) {
		strcpy(nro, "D");
	}
	return nro;
}

int DiaNumero(DATE fecha)
{
	static char fecname[15];

	strcpy(fecname, DayName(fecha));

	if (!strcmp(fecname, "LUNES")) {
		return 0;
	}
	if (!strcmp(fecname, "MARTES")) {
		return 1;
	}
	if (!strcmp(fecname, "MIERCOLES")) {
		return 2;
	}
	if (!strcmp(fecname, "JUEVES")) {
		return 3;
	}
	if (!strcmp(fecname, "VIERNES")) {
		return 4;
	}          
	if (!strcmp(fecname, "SABADO")) {
		return 5;
	}
	if (!strcmp(fecname, "DOMINGO")) {
		return 6;
	}
	return NULL_SHORT;
}


bool Vacaciones(int emp, long nroleg, DATE fecha)
{
	bool     tienevac = FALSE;
	dbtable  AVACAC;
	dbcursor c_vacac;
	schema   old, operac;

	old     = CurrentSchema();
	operac  = OpenSchema("operac", IO_EABORT);
	(void)SwitchToSchema(old);
	AVACAC  = CreateAlias(operac|VACAC);
	c_vacac = CreateCursor(AVACACbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_vacac, emp, nroleg, MIN_DATE);
	SetCursorTo  (c_vacac, emp, nroleg, MAX_DATE);
	while (!tienevac && FetchCursor(c_vacac) != ERROR) {
		if (fecha >= DFld(AVACAC_FDESDE) &&
			fecha <= DFld(AVACAC_FHASTA)) {
			tienevac = TRUE;
		}
	}
	DeleteCursor(c_vacac);
	DeleteAlias(AVACAC);
	return tienevac;
}
bool TieneLic(int emp, long nroleg, DATE fecha)
{
	bool     tienelic = FALSE;
	dbtable  ALICEN;
	dbcursor c_licen;
	schema   old, asist;

	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	ALICEN  = CreateAlias(asist|LICEN);
	c_licen = CreateCursor(AlInd(ALICEN, asist|LICENbyTIPLIC), IO_NOT_LOCK);

	SetCursorFrom(c_licen, emp, nroleg, MIN_SHORT, MIN_DATE);
	SetCursorTo  (c_licen, emp, nroleg, MAX_SHORT, MAX_DATE);
	while (!tienelic && FetchCursor(c_licen) != ERROR) {
		if (IFld(AlFld(ALICEN, asist|LICEN_LICEN)) == 9999)
			continue;

		if (fecha >= DFld(AlFld(ALICEN, asist|LICEN_FECHAD)) &&
			fecha <= DFld(AlFld(ALICEN, asist|LICEN_FECHAH))) {
			tienelic = TRUE;
		}
	}
	DeleteCursor(c_licen);
	DeleteAlias(ALICEN);
	return tienelic;
}
bool Falto(int emp, long nroleg, DATE fecha)
{
	bool     falto = FALSE;
	dbtable  AASISTEN;
	dbcursor c_asisten;
	schema   old, asist;

	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	AASISTEN  = CreateAlias(asist|ASISTEN);
	c_asisten = CreateCursor(AlInd(AASISTEN, asist|ASISTENbyINDLEG), IO_NOT_LOCK);

	SetCursorFrom(c_asisten, emp, nroleg, fecha, MIN_SHORT);
	SetCursorTo  (c_asisten, emp, nroleg, fecha, MAX_SHORT);
	while (!falto && FetchCursor(c_asisten) != ERROR) {
		falto = TRUE;
	}
	DeleteCursor(c_asisten);
	DeleteAlias(AASISTEN);
	return falto;
}

void BorrarInasistencia(int emp, long nroleg, DATE fecha)
{
	dbtable  AASISTEN;
	dbcursor c_asisten;
	schema   old, asist;

	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	AASISTEN  = CreateAlias(asist|ASISTEN);
	c_asisten = CreateCursor(AlInd(AASISTEN, asist|ASISTENbyINDLEG), IO_NOT_LOCK);

	SetCursorFrom(c_asisten, emp, nroleg, fecha,    MIN_SHORT);
	SetCursorTo  (c_asisten, emp, nroleg, MAX_DATE, MAX_SHORT);
	while (FetchCursor(c_asisten) != ERROR) {
		DelRecord(asist|AASISTEN);
	}
	DeleteCursor(c_asisten);
	DeleteAlias(AASISTEN);
}	

// *************** FUNCIONES PARA ASIGL.EXE Y ASIGD.EXE *************************
// BusHoras:
// dada una hora, devuelve un entero que 
// representa la cantidad de horas enteras.
// esta función se utiliza en genparte, asigd y asigl
// Ingrid - 26/04/1999
int BusHora(TIME hora) {

	char	shora[10];
	int		ihora;
	// paso la hora desde a entero
	TToStr(hora, shora, TFMT_SEPAR);
	strncpy(shora, shora, 2);
	ihora	=	StrToI(shora);
	return ihora;
}



// Dados los días trabajados para dos puestos distintos, esta funcion
// devuelve TRUE si alguno de los días de trabajo del primer puesto también
// se trabaja en el segundo puesto.
// devuelve FALSE cuando ninguno de los días que se debe trabajar en el primer puesto
// se trabaja en el segundo puesto.
// Esto se usa para validar que al querer asignar a un vigilador a det. puesto
// no se superpongan los días con otra asignación que tenga ese vigilador.
// si devuelve FALSE, entonces los puestos no se superponen en días trabajados
// si devuelve TRUE, entonces se debe determinar si se superponen las horas trabajadas
// de no superponerse las horas la nueva asignacion es valida. Si se superponen los días Y
// las horas trabajadas, entonces se superponen los puestos con lo cual no es una asignacion
// valida (la superposicion de horas trabajadas se valida con la funcion "Superposicion")
bool DiasTrabajados(char *d1, char *d2, char *d3, char *d4, char *d5, char *d6, char *d7,
					char *dia1, char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7) {

	if (strcmp(d1, NULL_STR)) {
		if (SeTrabEnPuesto(d1, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d2, NULL_STR)) {
		if (SeTrabEnPuesto(d2, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d3, NULL_STR)) {
		if (SeTrabEnPuesto(d3, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d4, NULL_STR)) {
		if (SeTrabEnPuesto(d4, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d5, NULL_STR)) {
		if (SeTrabEnPuesto(d5, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d6, NULL_STR)) {
		if (SeTrabEnPuesto(d6, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	if (strcmp(d7, NULL_STR)) {
		if (SeTrabEnPuesto(d7, dia1, dia2, dia3, dia4, dia5, dia6, dia7))
			return TRUE;
	}
	return FALSE;
}

// devuelve TRUE si dado un dia "dia" este no es ninguno de los dias 1 a 7
// esto se usa para saber si por ejemplo, se esta tratando de asignar a un vigilador
// un dia que no se trabaja en el puesto al cual se lo asigna.
bool SeTrabEnPuesto (char *dia, char *dia1, char *dia2, char *dia3, char *dia4,
					char *dia5, char *dia6, char *dia7) 
{
	return (!strcmp(dia, dia1) ||
			!strcmp(dia, dia2) ||
			!strcmp(dia, dia3) ||
			!strcmp(dia, dia4) ||
			!strcmp(dia, dia5) ||
			!strcmp(dia, dia6) ||
			!strcmp(dia, dia7));
}


// Valida si el objetivo fue dado de alta en la Policia
bool EstaEnLaPolicia(int emp, long cliente, long cont) {

	schema  old, brigada;
	dbtable ACONTACTO;

	old     = CurrentSchema();
	brigada = OpenSchema("brigada", IO_EABORT);
	SwitchToSchema(old);

	if ((ACONTACTO = CreateAlias(brigada|CONTACTO)) == ERROR) {
		WiMsg("No se pudo crear el alias");
		return FALSE;
	}
	SetIFld(AlFld(ACONTACTO, brigada|CONTACTO_EMP), emp);
	SetLFld(AlFld(ACONTACTO, brigada|CONTACTO_CLIENTE), cliente);
	SetIFld(AlFld(ACONTACTO, brigada|CONTACTO_CONTRATO), cont);
	if (GetRecord(AlInd(ACONTACTO, brigada|CONTACTObyEMP), THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if (IsNull((AlFld(ACONTACTO, brigada|CONTACTO_FECALT)))) {
			DeleteAlias(ACONTACTO);
			return FALSE;
		}
		else {
			DeleteAlias(ACONTACTO);
			return TRUE;
		}
	}
	DeleteAlias(ACONTACTO);
	return FALSE;
}

bool MenorAMinFecFranco(DATE fecha, char *vigil, DATE fecfra, char *regimen, DATE *fecmax)
{
	int v_dias_lab = GetDiasLaboral(regimen, !(strcmp(vigil, PARTTIME)));

	if (v_dias_lab==NULL_SHORT) {
		WiMsg("regimen no valido %s", regimen);
		return TRUE;
	}

	(*fecmax) = fecha + v_dias_lab;

	if (fecfra != NULL_DATE && fecfra > (*fecmax)) {
		return TRUE;
	}
	return FALSE;
}

bool PuestoEsPartime(long cliente, int objetivo, int tippto, int codint)
{
	schema  old, operac;
	dbtable APUESTOS;
	bool    esparttime = FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);
	APUESTOS = CreateAlias(operac|PUESTOS);
	SetKey(APUESTOSbyCLIENTE, cliente, objetivo, tippto, codint);
	if (GetRecord(APUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if (!IsNull(APUESTOS_HORAPT) || !IsNull(APUESTOS_CODFREC)) {
			esparttime = TRUE;
		}
	}
	DeleteAlias(APUESTOS);
	return esparttime;
}
// **************** FIN FUNCIONES PARA ASIGL.EXE Y ASIGD.EXE *************************

char *DescrFrecuencia(char *codfrec) {

	schema  operac, old;
	dbtable AFRECUEN;
	static  char descrip[21];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(old);
	strcpy(descrip, NULL_STR);
	AFRECUEN = CreateAlias(operac|FRECUEN);

	SetKey(AFRECUENbyCODFREC, codfrec);
	if (GetRecord(AFRECUENbyCODFREC, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(descrip, SFld(AFRECUEN_DESCRIP));
	}
	DeleteAlias(AFRECUEN);
	return descrip;
}

void CalcularDetalle(int emp, long cliente, int objetivo, long nroleg, char * tipvig, DATE dia, int pais,
					 int prov, int hstot, TIME thssal, TIME hsentre, int ptoser, int puesto, int nroint,
					 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran, 
					 bool vieneparte, int hsigualcli)
{
	int horasno, horasfe, hsnorregim, hsnorotrocli, hsnornue, toths = 0, ctrlhs;
	bool impre=FALSE;
    int horas_hoy, horas_man;
    bool dia_vig_hoy=FALSE, dia_vig_man=FALSE; 
	
	*hsnor = *hs50 = *hs100f = *hs100fe = 0.0;
	hsnorregim   = GetHsNormales(regimen, FALSE);
	ctrlhs       = ConvHraInt(hsentre, thssal) * 100;
	hsnorotrocli = HsOtroCliente(emp, nroleg, dia, cliente, objetivo, ptoser, puesto, nroint, &toths, vieneparte);
	hsnornue     = hsnorregim - (hsnorotrocli + hsigualcli);

	horas_hoy = ConvHraInt(hsentre, StrToT("2359")) * 100;
	horas_man = ConvHraInt(StrToT("235958"), thssal) * 100;

	if (DiaVigilador(dia, pais, prov, emp)) {
		dia_vig_hoy = TRUE;
	}

	if (DiaVigilador(dia+1, pais, prov, emp)) {
		dia_vig_man = TRUE;
	}

	if (impre) fprintf (stderr, "%ld %.3D \n", nroleg, dia);

	//  Si ingreso hsent = 0 y thssal = 0 
	if (!ctrlhs)
		return;

	// para calcular el franco no hace falta mandarle el cli-obj ni el regimen:
	if (Franco(emp, nroleg, dia, tipvig, numfran) && !FeriadoNovia(dia, pais, prov)) {
		*hs100f = hstot;                    
	}
	else {
		// Dia Feriado 
		if (FeriadoNovia(dia, pais, prov)) {

			if (thssal <= hsentre) {
				// Entra a trabajar dia feriado y sale dia normal o franco
				horasfe  = ConvHraInt(hsentre, StrToT("2359")) * 100;
				horasno  = ConvHraInt(StrToT("235958"), thssal) * 100;
				*hs100fe = horasfe;

				// Sale día normal 
				if (horasfe >= hsnorregim) {
					*hs50 = horasno;
				}
				else {
					if (hstot <= hsnorregim) {
						*hsnor = horasno;
					}
					else {
						*hsnor = hsnorregim - horasfe;
						*hs50  = hstot - horasfe - (hsnorregim - horasfe);
					}
				}
				
				//Si el dia posterior al Feriado es el dia del Vigilador las extras al 50 van como ext.100
				if (DiaVigilador((dia+1), pais, prov, emp)) {
					*hs100f = *hs50;
					*hs50 = 0;
				}
			}
			else {
				// Trabaja todo el dia feriado
				*hs100fe = hstot;
			}
		}
		else {
			// Dia no Feriado 
			if (hsentre <= thssal) {
				// Trabaja todo el dia normal 
				if (hsnornue == 0) {
						// En el otro cliente trabajo todas las hs normales 
						*hs50 = hstot;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov, emp)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

						return;
				}
				if (toths == 0) {
					if (hstot < hsnorregim) {
						*hsnor = hstot;
					}
					else {
						*hsnor = hsnorregim;
					}
					*hs50 = GetHsExtras(regimen, hstot, FALSE);

					//Si el dia es el dia del Vigilador las horas extras van al 100
			  		if (DiaVigilador(dia, pais, prov, emp)) {
						*hs100f = *hs50;
						*hs50 = 0;
					}

				}
				else {
					if (hstot <= hsnornue) {
						*hsnor = hstot;
					}
					else {
						// Trabaja hs normales y extras 
						*hsnor = hsnornue;
						*hs50  = hstot - hsnornue;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov, emp)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

						return;
					}
				}
			}
			else {
				// Entra a trabajar dia normal y sale dia feriado 
				if (FeriadoNovia(dia + 1, pais, prov)) {
					horasno = ConvHraInt(hsentre, StrToT("2359")) * 100;
					if (horasno <= hsnorregim) {
						// El dia normal trabaja todas horas normales 
						*hsnor = horasno;
					}
					else {
						// El dia normal trabaja horas normales y extras al 50% 
						*hsnor = hsnorregim;
						*hs50  = horasno - hsnorregim;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov, emp)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

					}
					*hs100fe = ConvHraInt(StrToT("235958"), thssal) * 100;
				}
				else {
					// Entra a trabajar un dia normal y sale un dia normal 
					if (impre) fprintf (stderr, "Entra a trabajar un dia normal y sale un dia normal\n");

 					if (impre) fprintf (stderr, "HS hoy %d man %d\n", horas_hoy, horas_man);

					if (hsnornue == 0) {
						// En el otro cliente trabajo todas las hs normales
	 					if (impre) fprintf (stderr, "En el otro cliente trabajo todas las hs normales \n");
						*hs50 = hstot;

						if (dia_vig_hoy) {
							*hs100f = *hsnor < horas_hoy ? (horas_hoy - *hsnor) : 0;
							*hs50 = *hs50 - *hs100f;
						}
						if (dia_vig_man) {
							*hs100f = *hs50 == 0 ? 0: 
										(*hsnor > horas_hoy) ? *hs50 : 
										horas_man;
							*hs50 = *hs50 - *hs100f;
						}

						return;
					}
					if (toths == 0) {
						if (hstot < hsnorregim) {
							*hsnor = hstot;
						}
						else {
							*hsnor = hsnorregim;
						}
						*hs50 = GetHsExtras(regimen, hstot, FALSE);

						if (dia_vig_hoy) {
							*hs100f = *hsnor < horas_hoy ? (horas_hoy - *hsnor) : 0;
							*hs50 = *hs50 - *hs100f;
						}
						if (dia_vig_man) {
							*hs100f = *hs50 == 0 ? 0: 
										(*hsnor > horas_hoy) ? *hs50 : 
										horas_man;
							*hs50 = *hs50 - *hs100f;
						}

	 					if (impre) fprintf (stderr, "No trabajo en otro cliente \n");

					}
					else {
						if (hstot <= hsnornue) {
							*hsnor = hstot;
						}
						else {
							// Trabaja hs normales y extras 
							*hsnor = hsnornue;
							*hs50  = hstot - hsnornue;
							if (dia_vig_hoy) {
								*hs100f = *hsnor < horas_hoy ? (horas_hoy - *hsnor) : 0;
								*hs50 = *hs50 - *hs100f;
							}
							if (dia_vig_man) {
								*hs100f = *hs50 == 0 ? 0: 
										(*hsnor > horas_hoy) ? *hs50 : 
										horas_man;
								*hs50 = *hs50 - *hs100f;
							}
		 					if (impre) fprintf (stderr, "Trabaja hs normales y extras  \n");
							return;
						}
					}
				}
			}
		}
	}
}

void CalcularDetalleProv(int emp, long cliente, int objetivo, long nroleg, DATE dia, char * tipvig, int pais,
						 int prov, int hstot, TIME thssal, TIME hsentre, int ptoser, int puesto, int nroint,
						 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran,
						 bool vieneparte)
{
	int horasno, horasfe, tothor, hsnorregim, hsnorotrocli, hsnornue, toths = 0, ctrlhs;
	TIME hsentefec, hssalefec;
	bool trabaja;
    int horas_hoy, horas_man;
    bool dia_vig_hoy=FALSE, dia_vig_man=FALSE; 

	//Esto es porque hay invasion de memoria y no la pudimos encontrar.
	char dummy[100];
	sprintf(dummy, "%s %s", tipvig, regimen);



	horas_hoy = ConvHraInt(hsentre, StrToT("2359")) * 100;
	horas_man = ConvHraInt(StrToT("235958"), thssal) * 100;

	if (DiaVigilador(dia, pais, prov, emp)) {
		dia_vig_hoy = TRUE;
	}

	if (DiaVigilador(dia+1, pais, prov, emp)) {
		dia_vig_man = TRUE;
	}

	trabaja   = TrabDiaEnPtoEfec(emp, cliente, objetivo, nroleg, dia);
//	hsentefec = HoraEntEfectivo(emp, nroleg, dia);
//	hssalefec = HoraSalEfectivo(emp, nroleg, dia);

	hsentefec = HrEntTrabEfec(emp, nroleg, dia);
	hssalefec = HrSalTrabEfec(emp, nroleg, dia);

	// Total de horas trabajadas en horario efectivo 
	*hsnor = *hs50 = *hs100f = *hs100fe = 0.0;
	tothor     = ConvHraInt(hsentefec, hssalefec) * 100;
	hsnorregim = GetHsNormales(regimen, FALSE);
	ctrlhs     = ConvHraInt(hsentre, thssal) * 100;


	// Si ingreso hsent = 0 y thssal = 0 
	if (!ctrlhs)
		return;

	// para calcular el franco no hace falta mandarle el cli-obj ni el regimen:
	if (Franco(emp, nroleg, dia, tipvig, numfran) && !FeriadoNovia(dia, pais, prov)) {
		*hs100f = hstot;
	}
	else {
		// Dia Feriado 
		if (FeriadoNovia(dia, pais, prov)) {
			if (thssal <= hsentre) {
				// Entra a trabajar dia feriado y sale dia normal o franco
				horasfe  = ConvHraInt(hsentre, StrToT("2359")) * 100;
				horasno  = ConvHraInt(StrToT("235958"), thssal)   * 100;
				*hs100fe = horasfe;

				// Sale día normal 
				if (horasfe >= hsnorregim)
					*hs50 = horasno;
				else {
					if (hstot <= hsnorregim)
						*hsnor = horasno;
					else {
						*hsnor = hsnorregim - horasfe;
						*hs50  = hstot - horasfe - (hsnorregim - horasfe);
					}
				}
			}
			else {
				// Trabaja todo el dia feriado 
				*hs100fe = hstot;
			}
		}
		else {
			// Dia no Feriado 


			hsnorotrocli = HsOtroCliente(emp, nroleg, dia, cliente, objetivo, ptoser, puesto, nroint, &toths, vieneparte);
			hsnornue     = hsnorregim - hsnorotrocli;


			if (hsentre < thssal) {
				if (hsnornue == 0) {
					// En el otro cliente trabajo todas las hs normales 
					*hs50 = hstot;
					//Si el dia es el dia del Vigilador las horas extras van al 100
	 		  		if (dia_vig_hoy) {
						*hs100f = *hs50;
						*hs50 = 0;
					}
					return;
				}
				// Trabaja el dia en el Pto. Efectivo 
				if (trabaja) {
					if (hstot > tothor) {
						*hsnor = hsnorregim;
						*hs50  = GetHsExtras(regimen, tothor, FALSE) + (hstot - tothor);
		 		  		if (dia_vig_hoy) {
							*hs100f = *hs50;
							*hs50 = 0;
						}
					}
					// Trabaja provisorio despues de hora del horario efectivo 
					if (hsentre >= hssalefec || thssal <= hsentefec) {
						if (tothor < hsnorregim) {
							if (hstot + hsnorotrocli <= hsnorregim) {
								*hsnor = hstot;
								*hs50  = 0;  //esta asi porque entra arriba(tengo que verlo)
				 		  		if (dia_vig_hoy) {
									*hs100f = *hs50;
									*hs50 = 0;
								}
							}
							else {
								*hsnor = hsnorregim - hsnorotrocli;
								*hs50  = hstot - (hsnorregim - hsnorotrocli);
				 		  		if (dia_vig_hoy) {
									*hs100f = *hs50;
									*hs50 = 0;
								}
							}
						}
						else {
							*hs50 = hstot;
			 		  		if (dia_vig_hoy) {
								*hs100f = *hs50;
								*hs50 = 0;
							}
						}
						return;
					}
					// Entra provisorio dentro del horario efectivo 
					if (hsentre >= hsentefec && hsentre < hssalefec) {
						*hsnor = ConvHraInt(hsentre, hssalefec) * 100;
						*hs50  = ConvHraInt(hssalefec, thssal) * 100;
					}
					// Sale provisorio dentro del horario efectivo 
					if (thssal >= hsentefec && thssal < hssalefec) {
						*hsnor = ConvHraInt(hsentefec, thssal) * 100;
						*hs50  = ConvHraInt(hsentre, hsentefec) * 100;
						if (dia_vig_hoy) {
							*hs100f = *hs50;
							*hs50 = 0;
						}
					}
				}
				else {
					// Las hs trabajadas no superan las hs normales del otro cliente o pto. 
					if (hstot <= hsnornue) {
						*hsnor = hstot;
					}
					else {
						// Trabaja hs normales y extras 
						*hsnor = hsnornue;
						*hs50  = hstot - hsnornue;
		 		  		if (dia_vig_hoy) {
							*hs100f = *hs50;
							*hs50 = 0;
						}
					}
				}
			}
			else {
				// Entra a trabajar dia normal y sale dia feriado 
				if (FeriadoNovia(dia + 1, pais, prov)) {
					horasno = ConvHraInt(hsentre, StrToT("2359")) * 100;
					if (horasno <= hsnorregim) {
						// El dia normal trabaja todas horas normales 
						*hsnor = horasno;
					}
					else {
						// El dia normal trabaja horas normales y extras al 50% 
						*hsnor = hsnorregim;
						*hs50  = horasno - hsnorregim;
					}
					*hs100fe = ConvHraInt(StrToT("235958"), thssal) * 100;
				}
				else {
					// Entra a trabajar un dia normal y sale un dia normal 
					if (trabaja) {
						if (tothor < hsnorregim) {
							if (tothor + hstot < hsnorregim) {
								*hsnor = hstot;
							}
							else {
								*hsnor = hsnorregim - tothor;
								*hs50  = hstot - (hsnorregim - tothor);
								if (dia_vig_hoy) {
									*hs100f = *hs50 > horas_hoy ? horas_hoy : *hs50;
									*hs50 = *hs50 - *hs100f;
								}
								if (dia_vig_man) {
									*hs100f = *hs50 > horas_man ? horas_man : *hs50;
									*hs50 = *hs50 - *hs100f;
								}
							} 
						} 
						else {
							*hs50 = hstot;
							if (dia_vig_hoy) {
								*hs100f = *hs50 > horas_hoy ? horas_hoy : *hs50;
								*hs50 = *hs50 - *hs100f;
							}
							if (dia_vig_man) {
								*hs100f = *hs50 > horas_man ? horas_man : *hs50;
								*hs50 = *hs50 - *hs100f;
							}
						}
					}
					else {
						if (hsnornue == 0) {
							// En el otro cliente trabajo todas las hs normales 
							*hs50 = hstot;
							if (dia_vig_hoy) {
								*hs100f = *hs50 > horas_hoy ? horas_hoy : *hs50;
								*hs50 = *hs50 - *hs100f;
							}
							if (dia_vig_man) {
								*hs100f = *hs50 > horas_man ? horas_man : *hs50;
								*hs50 = *hs50 - *hs100f;
							}
							return;
						}
						// Las hs trabajadas no superan las hs normales del otro cliente
						if (hstot <= hsnornue) {
							*hsnor = hstot;
						}
						else {
							// Trabaja hs normales y extras 
							*hsnor = hsnornue;
							*hs50  = hstot - hsnornue;
							if (dia_vig_hoy) {
								*hs100f = *hsnor < horas_hoy ? (horas_hoy - *hsnor) : 0;
								*hs50 = *hs50 - *hs100f;
							}
							if (dia_vig_man) {
								*hs100f = *hs50 == 0 ? 0: 
										(*hsnor > horas_hoy) ? *hs50 : 
										horas_man;
								*hs50 = *hs50 - *hs100f;
							}
						}
					} 
				}
			}
		}
	}
}

void CalcularDetallePartime(long cliente, int objetivo, DATE dia, TIME hsentre, TIME thssal,
                             int * hsnor, int * hs100fe)
{
	int pais, prov, ctrlhs;

	schema prev = CurrentSchema();
	OpenSchema("comerc", IO_EABORT);
	*hsnor = *hs100fe = 0.0;

	ctrlhs = ConvHraInt(hsentre, thssal) * 100;

	// Si ingreso hsent = 0 y thssal = 0 
	if (!ctrlhs)
		return;



	SetKey(OBJETIVO, cliente, objetivo);
	(void)GetRecord(OBJETIVO, THIS_KEY, IO_NOT_LOCK);
	pais = IFld(OBJETIVO_PAIS);
	prov = IFld(OBJETIVO_PROV);			

	// Dia Feriado 
	if (FeriadoNovia(dia, pais, prov)) {
		if (thssal <= hsentre) {
			// Entra a trabajar dia feriado y sale dia normal 
			*hs100fe = ConvHraInt(hsentre, StrToT("2359"))*100;
			*hsnor   = ConvHraInt(StrToT("235958"), thssal)*100;
		}
		else {
			// Trabaja todo el dia feriado 
			*hs100fe = ConvHraInt(hsentre, thssal)*100;
		}
	}
	else {
		// Dia no Feriado 
		if (hsentre < thssal) {
			// Trabaja todo el dia normal 
			*hsnor = ConvHraInt(hsentre, thssal) * 100;
		}
		else {
			// Entra a trabajar dia normal y sale dia feriado 
			if (FeriadoNovia(dia + 1, pais, prov)) {
				*hsnor   = ConvHraInt(hsentre, StrToT("2359")) * 100;
				*hs100fe = ConvHraInt(StrToT("235958"), thssal) * 100;
			}
			else
				*hsnor = ConvHraInt(hsentre, thssal)*100;	
		}
	}           
	SwitchToSchema(prev);
}

int HsOtroCliente(int emp, long nroleg, DATE fecparte, long cliente, int obj, int ptoser, int puesto,
				  int nroint, int *hs, bool vieneparte)
{
	schema   prev, operac;
	dbtable  APARTE;
	dbcursor c_parte;
	int      hsnormal = 0;

	prev    = CurrentSchema();
	operac  = OpenSchema("operac", IO_EABORT);
	APARTE  = CreateAlias(operac|PARTE);
	c_parte = CreateCursor(AlInd(APARTE, PARTEbyEMPLE), IO_NOT_LOCK);
   
   	*hs = 0;

	SetCursorFrom(c_parte, emp, nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, nroleg, fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		// GAG(18/11/04)- Lo saque por el caso: cliente=3988, obj=4, dia=18/11/04. Tiene dos
		// registros y calculaba todo normal.
		// GAG(22/11/04)- Agregue la variable vieneparte para saber cuando se llama del PARTE o 
		// de la libreria. Si viene del parte no se deben contar las horas del mismo cliente porque 
		// para eso existe la funcion HsIgualCliente en el parte.c
		if (vieneparte) {
			if (LFld(APARTE_CLIENTE) == cliente && IFld(APARTE_OBJETIVO) == obj) {
				continue;
			}
		}
		hsnormal += IFld(APARTE_HSNOR);
		*hs      += IFld(APARTE_HSNOR) + IFld(APARTE_HS50) + IFld(APARTE_HS100F) + IFld(APARTE_HS100FE);
	}
	DeleteCursor(c_parte);
	DeleteAlias(APARTE);
	SwitchToSchema(prev);
	return hsnormal;
}

//-------------------------* GetNextNroint *-----------------------
// Devuelve el proximo número interno para el puesto 
            
int GetNextNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto)
{
	int next = 1;
	dbtable AASIG = (dbtable) ERROR;
	dbtable AASIGH = (dbtable) ERROR;
	schema prev = CurrentSchema();
	OpenSchema("operac", IO_EABORT);
	AASIG = CreateAlias(ASIG);       
	AASIGH = CreateAlias(ASIGH);
	SetKey(AlInd(AASIG, ASIGbyEMP), emp, cliente, objetivo, nroleg, ptoser, puesto, MAX_SHORT);
	if(GetRecord(AlInd(AASIG, ASIGbyEMP), PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR)
		next = IFld(AlFld(AASIG, ASIG_NROINT)) + 1;
	SetKey(AlInd(AASIGH, ASIGHbyPUESTO), emp, cliente, objetivo, nroleg, ptoser, puesto, MAX_SHORT);
	if(GetRecord(AlInd(AASIGH, ASIGHbyEMP), PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR) {
		if (next < (IFld(AlFld(AASIGH, ASIGH_NROINT)) + 1))
			next = IFld(AlFld(AASIGH, ASIGH_NROINT)) + 1;
	}
	DeleteAlias(AASIG);				                              
	DeleteAlias(AASIGH);
	SwitchToSchema(prev);
	return next;		
}

//-------------------------* BajaPuesto *-----------------------
// Devuelve si el puesto esta de baja 
bool BajaPuesto(long cliente, int objet, int tippto, DATE dia)
{
	bool baja = TRUE;
	dbtable APUESTOS = (dbtable) ERROR;
	dbcursor c_APUE  = (dbcursor) ERROR;
	schema operac, prev = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	APUESTOS = CreateAlias(PUESTOS);
	c_APUE   = CreateCursor(APUESTOSbyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom(c_APUE, cliente, objet, tippto, MIN_SHORT);
	SetCursorTo  (c_APUE, cliente, objet, tippto, MAX_SHORT);
	while(FetchCursor(c_APUE) != ERROR) {
		if ((IFld(APUESTOS_CANTVIG) != 0.0 &&  IsNull(APUESTOS_FFINAL) && dia >= DFld(APUESTOS_FINICIO)) ||
	    	(IFld(APUESTOS_CANTVIG) != 0.0 && !IsNull(APUESTOS_FFINAL) && DFld(APUESTOS_FFINAL) >= dia))
			baja = FALSE;
		if (IFld(APUESTOS_CANTVIG) == 0.0 && DFld(APUESTOS_FFINAL) != NULL_DATE && DFld(APUESTOS_FFINAL) >= dia)
			baja = FALSE;
		if (IFld(APUESTOS_CANTVIG) == 0.0 && DFld(APUESTOS_FFINAL) == NULL_DATE && DFld(APUESTOS_FINICIO) >= dia)
			baja = FALSE;
	}
	DeleteCursor(c_APUE);
	DeleteAlias(APUESTOS);
	SwitchToSchema(prev);

	return baja;
}

bool Superposicion(TIME hent, TIME hsal, TIME hdesde, TIME hhasta, bool total)
{
	int hent1=FALSE, hdesde1=FALSE;

	if ((hdesde == StrToT("00:00") && hhasta == StrToT("00:00")) ||
		(hent   == StrToT("00:00") && hsal   == StrToT("00:00")))
		return FALSE;

	if (hsal == StrToT("00:00"))
		hsal = MAX_TIME;

	if (hhasta == StrToT("00:00"))
		hhasta = MAX_TIME;

	if (hsal < hent)
		hent1 = TRUE;
	
	if (hhasta < hdesde)
		hdesde1 = TRUE;

	if (!hent1 && !hdesde1) {
		if (total) {
			if ((hent >= hdesde && hent <  hhasta) && (hsal >  hdesde && hsal <= hhasta)) {
			    	return TRUE;
			}
		}
		else {
			if ((hent >= hdesde && hent <  hhasta) || (hsal >  hdesde && hsal <= hhasta) ||
				(hent <= hdesde && hsal >= hhasta) || (hent <= hdesde && hsal >  hdesde) ||
				(hent <  hhasta && hsal >= hhasta)) {
			    	return TRUE;
			}
		}
	}
	else {
		if (hent1 && !hdesde1) {
			if (!total && (hsal>hdesde || hent < hhasta))
				return TRUE;
		}
		else {
			if (!hent1 && hdesde1) {
				if (total) {
					if (hhasta>=hsal || hdesde <=hent)
						return TRUE;
				}
				else {
					if (hhasta>hent || hdesde < hsal)
						return TRUE;
				}
			}
			else {
				if (total)
					return TRUE;
				else {
					if (hhasta >= hsal && hdesde <= hent)
						return TRUE;
					if (hhasta >= hsal && hdesde >= hent)
						return TRUE;
					if (hhasta <= hsal && hdesde <= hent)
						return TRUE;
					if (hhasta <= hsal && hdesde >= hent)
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}

//-------------------------* AsigCodInOperac *-----------------------
void AsigCodInOperac(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char* tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, int codint)
{
	schema old, operac;
	int codigo;
	DATE fechafin=NULL_DATE;
	bool encontro=TRUE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
    
	if (codint==NULL_SHORT){
		SetKey(PUESTOSbyCLIENTE, cliente, objetivo, tippto, MAX_SHORT);
		if (GetRecord(PUESTOSbyCLIENTE, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
			codigo  = IFld(PUESTOS_CODINT) + 1;
		else
			codigo = 1;

		SetKey (PUESTOSbyPUESTO, cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regimen, codfrec, tipodia);
		if (GetRecord(PUESTOSbyPUESTO, THIS_KEY, IO_LOCK) == ERROR ) 
			encontro=FALSE;
	
	}
	else{
		codigo = codint;

		SetKey (PUESTOSbyCLIENTE, cliente, objetivo, tippto, codigo);
		if (GetRecord(PUESTOSbyCLIENTE, THIS_KEY, IO_LOCK) == ERROR ) 
			encontro=FALSE;

	}

	if (encontro==FALSE || str_eq (cond, "A") ) {
		if (str_eq (cond, "B")) {
			char buffer[65];
			sprintf (buffer, "%d %d %d %d %T %T %s %s %s %s %s %s %s %s %s ", cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regimen, codfrec);
			//WiMsg ("El Puesto %s\nLo Quiere dar de Baja pero no esta dado de Alta", buffer);

			return ;
		}
			
		InitRecord(PUESTOS);
		SetIFld(PUESTOS_CODINT,  codigo);
		SetLFld(PUESTOS_CLIENTE, cliente);
		SetIFld(PUESTOS_OBJET,   objetivo);
		SetIFld(PUESTOS_TIPPTO,  tippto);
		SetIFld(PUESTOS_PUESTO,  puesto);
		SetTFld(PUESTOS_HINICIO, hinicio);
		SetTFld(PUESTOS_HFINAL,  hfinal);
		SetFld (PUESTOS_DIA1,    dia1);
		SetFld (PUESTOS_DIA2,    dia2);
		SetFld (PUESTOS_DIA3,    dia3);
		SetFld (PUESTOS_DIA4,    dia4);
		SetFld (PUESTOS_DIA5,    dia5);
		SetFld (PUESTOS_DIA6,    dia6);
		SetFld (PUESTOS_DIA7,    dia7);
		SetFld (PUESTOS_REGIM,   regimen);
		SetFld (PUESTOS_TIPODIA, tipodia);
		SetFld (PUESTOS_CODFREC, codfrec);
		SetIFld(PUESTOS_CANTPUE, cantpue);
		SetIFld(PUESTOS_CANTVIG, cantvig);
		SetIFld(PUESTOS_HORAPT,  horapt);
		SetIFld(PUESTOS_VIGI,    0);

		fechafin= FechaFinalOt (cliente, objetivo, finicio, ffinal, hiniot, hfinot, FALSE);
		SetDFld(PUESTOS_FFINAL,  fechafin);

		finicio = finicio != NULL_DATE ? finicio : ffinal;
		SetDFld(PUESTOS_FINICIO, finicio);

		PutRecord(PUESTOS);
		FreeTable(PUESTOS);

		SwitchToSchema(old);
		return;
    }    
	SwitchToSchema(old);
        
 	if (str_eq(cond, "B")) {
		//Si con codint no encuentra el puesto para bajar busca sin el codint
 		if (BajarHoras(cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regimen, tipodia, cantpue, cantvig,
 		                cond, codfrec, horapt, finicio, ffinal, hiniot, hfinot, codint)==FALSE) {
		  BajarHoras(cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regimen, tipodia, cantpue, cantvig,
		              cond, codfrec, horapt, finicio, ffinal, hiniot, hfinot, NULL_SHORT);
					 	
		}
	}

}

bool BajarHoras(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, int codint)
{
	find_mode mode;		
	schema old, operac;
	short puepend, vigpend, horpend;
	char  buffer[70];	
	bool grabo=FALSE;
	
	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
    
	puepend = cantpue == NULL_SHORT ? 0 : cantpue;
	vigpend = cantvig == NULL_SHORT ? 0 : cantvig;
	horpend = horapt  == NULL_SHORT ? 0 : horapt;
	
	mode=THIS_KEY|NEXT_KEY|PARTIAL_KEY;
	
	sprintf (buffer, "%d %d %d %d %T %T %s %s %s %s %s %s %s %s ", cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regimen);

	SetKey(PUESTOSbyPUESTO, cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5,
							dia6, dia7, regimen, codfrec, tipodia);

	while (GetRecord(PUESTOSbyPUESTO, mode, IO_LOCK,15) != ERROR && (puepend||vigpend||horpend)) {
        short bpue, bvig, bhor;
		mode = NEXT_KEY|PARTIAL_KEY;

		// Seleccion de codint si viene
		if (codint!=NULL_SHORT)
			if (codint !=IFld(PUESTOS_CODINT))
				continue;


		if (!str_eq (tipodia, SFld(PUESTOS_TIPODIA))) {
			continue;
		}

		if (DFld(PUESTOS_FFINAL) != NULL_DATE && finicio > DFld(PUESTOS_FFINAL))
			continue;

		// fprintf (stderr, "Baja %s %d %d %s\n", buffer, puepend, vigpend, codfrec);
		if (puepend > IFld(PUESTOS_CANTPUE)) {
			bpue = 	IFld(PUESTOS_CANTPUE);
			puepend = puepend - IFld(PUESTOS_CANTPUE);
		} 
		else {
			bpue = 	puepend;
			puepend = 0;
		} 

		if (vigpend > IFld(PUESTOS_CANTVIG)) {
			bvig = 	IFld(PUESTOS_CANTVIG);
			vigpend = vigpend - IFld(PUESTOS_CANTVIG);
		} 
		else {
			bvig = 	vigpend;
			vigpend = 0;
		} 

		if (IFld(PUESTOS_HORAPT) != NULL_SHORT) {
			if (horpend > IFld(PUESTOS_HORAPT)) {
				bhor = 	IFld(PUESTOS_HORAPT);
				horpend = horpend - IFld(PUESTOS_HORAPT);
			} 
			else {
				bhor = 	horpend;
				horpend = 0;
			}	
			SetIFld(PUESTOS_HORAPT,  IFld(PUESTOS_HORAPT) -  bhor);
		} 
		//Solo pongo fecha de baja si va a quedar en 0 la cant. de vig. 
		if (IFld(PUESTOS_CANTVIG) - bvig == 0 && bvig > 0) {
			DATE fechafin= FechaFinalOt (cliente, objetivo, finicio, ffinal, hiniot, hfinot, TRUE);
			SetDFld(PUESTOS_FFINAL,  fechafin);
		}
		SetIFld(PUESTOS_CANTPUE, IFld(PUESTOS_CANTPUE) - bpue);
		SetIFld(PUESTOS_CANTVIG, IFld(PUESTOS_CANTVIG) - bvig);

		grabo=TRUE;
		PutRecord(PUESTOS);
    }   
	FreeTable(PUESTOS);
	SwitchToSchema(old);
	return grabo;
}

DATE FechaFinalOt (long cliente, short objetivo, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, bool baja)
{

	DATE fecbaja=NULL_DATE;
	TIME horbaja;


	// Si tiene fecha inicio y final y es riff no tomo en cuenta la fecha de finalizacion 
	if (ObjetRif(cliente, objetivo) && !baja)
			return NULL_DATE;
		
	if (!ObjetRif(cliente, objetivo) && !baja && ffinal == NULL_DATE)
		return NULL_DATE;

    // Si es una baja 
	if (finicio != NULL_DATE && ffinal != NULL_DATE) {
	//	Si es una baja y tiene fecha de inicio y final es porque esta acompañado
	//	por el alta de un nuevo puesto y sin importar la hora lo tomo con el dia
	//	de inicio 
		if (baja) {
			fecbaja =  finicio;
			horbaja =  StrToT("0000");
		} 
		else {
			fecbaja =  ffinal;
			horbaja =  hfinot != NULL_TIME ? hfinot : StrToT("0000");
		} 

	}
	else {  

	//	Si es una baja y NO tiene fecha de inicio y final 
	//	tomo la que tiene valor
	//	Solo me fijo la hora si tiene fecha de finalizacion porque
	//	si termino != 00:00 no lo tomo en cuenta porque ese dia tiene que trabajar
	//	si termino = 00:00  lo tomo en cuenta porque ese dia NO tiene que trabajar
    	
		fecbaja = ffinal != NULL_DATE ? ffinal : finicio;
		horbaja = hfinot != NULL_TIME ? hfinot : StrToT("0000");
	} 

	//	si termino != 00:00 Termina ese dia
	//	si termino = 00:00  Termina el dia anterior

	if (horbaja == StrToT("0000")) {
		fecbaja -- ;
	}

	return fecbaja;		
}		

char *GetDescrLugPag (long lpago)
{
	static char desc[40];
	schema prev, sue;
	
	prev = CurrentSchema ();
	
	if ( (sue=FindSchema ("sue")) == (schema) ERROR)
		sue = OpenSchema("sue", IO_EABORT);

	SwitchToSchema (prev);

	SetKey(sue|OFPAGbyEMP, 1, lpago);
	if(GetRecord(sue|OFPAGbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
		strcpy (desc, NULL_STR);
	else	
		strcpy (desc, SFld(sue|OFPAG_DENOM));

	return desc;
}

double ConvFechaHraInt (DATE fdesde, DATE fhasta, TIME hdesde, TIME hhasta)
{
	double canths=0; 
	int dias, tdias;

	if (fdesde == fhasta) {
		canths = ConvHraInt(hdesde, hhasta);
	}

	if (fdesde > fhasta) {
		canths = 0;
	}

	if (fdesde < fhasta) {
		//Sumo las horas del primer dia
		canths += ConvHraInt(hdesde, StrToT ("23:59"));
		//Sumo las horas del ultimo dia
		canths += ConvHraInt(StrToT ("00:00"), hhasta);
		
		dias = fhasta - fdesde -1 ;
		for (tdias=0; tdias < dias; tdias ++) {
			//Sumo las horas de un dia
			canths += ConvHraInt(StrToT ("00:00"),StrToT ("23:59"));
		}
	}
	return canths;
}

bool DiaVigilador(DATE fecha, int pais, int prov, int emp)
{   
	//El 25 de abril es el dia del vigilador
	DATE fecv;
	short dia=25, mes=4, anio;

	if (emp == _EMP_SAPE ) {
		return FALSE;
	}

	anio = Year(fecha);
	fecv = DMYToD(dia, mes, anio);

	if (fecha == fecv)
		return TRUE;

	return FALSE;		
}

//-------------------------* GetRolDesc *--------------------------
char * GetRolDesc(int rol)
{
	schema  old, operac;
	static char descrol[35];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetIFld(operac|ROL_CODROL, rol);
	if (GetRecord(operac|ROLbyCODROL, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(descrol, SFld(operac|ROL_DESCRIP));
	}
	else {
	 	strcpy(descrol, NULL_STR);
	}
	SwitchToSchema(old);
	return descrol;
}

//-------------------------* ValorRol *--------------------------
char * ValorRol(int rol, int fila, int colum)
{
	schema  old, operac;
	static char valor[3];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetIFld(operac|RROL_CODROL, rol);
	SetIFld(operac|RROL_FILA,   fila);
	SetIFld(operac|RROL_COLUM,  colum);
	if (GetRecord(operac|RROLbyCODROL, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(valor, SFld(operac|RROL_VALOR));
	}
	else {
	 	strcpy(valor, NULL_STR);
	}
	SwitchToSchema(old);
	return valor;
}

TIME HoraInicioTurno(TIME hinicio, char *regim, char *rvalor)
{
	TIME horini;
	short turno;
	double hsreg;

	#ifdef _GETHORASTURNO
	fprintf (stderr, "HoraInicioTurno  hinicio %.3T reg %s valor %s \n", hinicio, regim, rvalor);
	#endif

	hsreg = (double)GetHsRegimen(regim)/100.0;

	horini= hinicio;
	turno = StrToI(rvalor);
	turno = turno > 0 ? turno -1 : 0;

	#ifdef _GETHORASTURNO
	fprintf (stderr, "HoraInicioTurno  turno %d hsreg %d %.3T\n", GetHsRegimen(regim), hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA));
	#endif

	horini= hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) > _SEGUNDOS_POR_DIA ? 
			hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) - _SEGUNDOS_POR_DIA : 
			hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA);

	return horini;
}

TIME HoraFinTurno(long cliente, short objet, short ptoser, short puesto, TIME hinicio, 
					char *regim, char *rvalor)
{
	TIME horfin;
	short turno;
	double hsreg, difhor;
	dbtable APUESTOS;
	schema  old, operac;
	TIME horpfin = NULL_TIME, horpini = NULL_TIME;
	bool diaposterior = FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	//Me fijo la hora fin del puesto
	APUESTOS = CreateAlias(operac|PUESTOS);
	SetKey(APUESTOSbyCLIENTE, cliente, objet, ptoser, puesto);
	if (GetRecord(APUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		horpfin = TFld(APUESTOS_HFINAL);
		horpini = TFld(APUESTOS_HINICIO);
	}
	DeleteAlias(APUESTOS);

	hsreg = (double)GetHsRegimen(regim)/100.0;

	horfin= hinicio;
	turno = StrToI(rvalor);
//	turno = turno > 0 ? turno -1 : 0;

	if (hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) > _SEGUNDOS_POR_DIA) {
		horfin = hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) - _SEGUNDOS_POR_DIA;
		diaposterior = TRUE;
	}
	else {
		horfin = hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA);
		diaposterior = FALSE;
	} 

	#ifdef _GETHORASTURNO
	fprintf (stderr, "Hora FinTurno hinicio %T hsreg %f turno %d\n", hinicio, hsreg, turno);
	fprintf (stderr, "HoraFinTurno  %.3T %.3T seg x dia %ld calculo  %ld \n", 
  		 (TIME)(hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA)), 
 		 (TIME)(hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) - _SEGUNDOS_POR_DIA),
		 _SEGUNDOS_POR_HORA, 
		 (hsreg * turno * _SEGUNDOS_POR_HORA));
	#endif


	if (horfin == StrToT("0000")) {
		return StrToT("2359");
	}
	
	if (horpini == StrToT("0000") && horpfin == StrToT("2359")) {
		#ifdef _GETHORASTURNO
			fprintf (stderr, "Sin cambiar  horpfin %.3T \n", horpfin);
		#endif
		return horfin;
	}

	#ifdef _GETHORASTURNO
	fprintf (stderr, "Hasta ahora horfin =  %.3T  \n", horfin);
	#endif
	
	if (!diaposterior) {
		//Me fijo la diferencia de horas - si no puedo cubrir otro turno estiro o achico el actual.
		difhor = (double)(horpfin - horfin);
		difhor = horpfin == NULL_TIME ? 0 : fabs(difhor) /  _SEGUNDOS_POR_HORA;

		#ifdef _GETHORASTURNO
			fprintf (stderr, "1 horfin %.3T horpfin %.3T difhor %.2f hsreg %.2f \n", horfin, horpfin, difhor, hsreg);
		#endif

		if (difhor > 0 && difhor < hsreg) {
			#ifdef _GETHORASTURNO
				fprintf (stderr, "PONGO horpfin %.3T \n", horpfin);
			#endif
			horfin = horpfin;
		}
	}
	return horfin;
}



// ***********************************************************************************************
// Esta funcion devuelve TRUE si el legajo esta asignado en la fecha en un puesto que tiene rol.
// Si devuelve TRUE en el parametro rolfranco devuelve:
// 	 TRUE : Si ese dia SI tiene franco
//   FALSE: Si ese dia NO tiene franco
// ***********************************************************************************************
bool EncontroFrancoRol (int emp, long nroleg, DATE fecha, char *vigil , bool *rolfranco)
{
	bool		encontre = FALSE;
	schema		old, operac;
	dbtable		AASIG, AASIGH;
	dbcursor	c_asig, c_asigh;
	char turno[30];
//	TIME tmph1, tmph2;

	*rolfranco = FALSE;
	strcpy (turno, NULL_STR);

	old		= CurrentSchema();
	operac	= OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);


	AASIG	= CreateAlias(operac|ASIG);
	AASIGH	= CreateAlias(operac|ASIGH);
	c_asig	= CreateCursor(AASIGbyNROLEG, IO_NOT_LOCK);
	c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (IFld(AASIG_CODROL) == NULL_SHORT)
			continue;

		if (strcmp(SFld(AASIG_EFECT), EFECTIVO) || DFld(AASIG_FECASIG) > fecha)
			continue;

		strcpy(turno, GetCondicRol(DFld(AASIG_FECASIG), fecha, IFld(AASIG_CODROL), IFld(AASIG_FILA), IFld(AASIG_COLUM)));
//WiMsg("turno a  %s", turno);
/*		GetHorasTurno ( emp, nroleg, LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO),
						DFld(AASIG_FECASIG), IFld(AASIG_CODROL), IFld(AASIG_FILA), IFld(AASIG_COLUM),
						!str_eq(SFld(AASIG_REGPTO), NULL_STR) ? SFld(AASIG_REGPTO): SFld(AASIG_REGIM),
						TFld(AASIG_HSENT), TFld(AASIG_HSSAL),
						fecha, IFld(AASIG_PTOSER), IFld(AASIG_PUESTO),
						SFld(AASIG_DIA1), SFld(AASIG_DIA2), SFld(AASIG_DIA3),
						SFld(AASIG_DIA4), SFld(AASIG_DIA5), SFld(AASIG_DIA6),
						SFld(AASIG_DIA7), turno, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN), &tmph1, &tmph2);
*/
		encontre = TRUE;
	}
	if (!encontre) {
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (!encontre && FetchCursor(c_asigh) != ERROR) {

			if (IFld(AASIGH_CODROL) == NULL_SHORT) 
				continue;

			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;

			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				strcpy(turno, GetCondicRol(DFld(AASIGH_FECALT), fecha, IFld(AASIGH_CODROL), IFld(AASIGH_FILA), IFld(AASIGH_COLUM)));
//WiMsg("turno b  %s", turno);

/*				GetHorasTurno (emp, nroleg, LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO),
						DFld(AASIGH_FECALT), IFld(AASIGH_CODROL), IFld(AASIGH_FILA), IFld(AASIGH_COLUM),
						!str_eq(SFld(AASIGH_REGPTO), NULL_STR) ? SFld(AASIGH_REGPTO): SFld(AASIGH_REGIM),  
						TFld(AASIGH_HSENT), TFld(AASIGH_HSSAL),
						fecha, IFld(AASIGH_PTOSER), IFld(AASIGH_PUESTO),
						SFld(AASIGH_DIA1), SFld(AASIGH_DIA2), SFld(AASIGH_DIA3),
						SFld(AASIGH_DIA4), SFld(AASIGH_DIA5), SFld(AASIGH_DIA6),
						SFld(AASIGH_DIA7), turno, SFld(AASIGH_VIGIL), IFld(AASIGH_NUMFRAN),
						&tmph1, &tmph2); 
*/
				encontre=TRUE;
			}
		}
	}

	DeleteCursor(c_asigh);
	DeleteCursor(c_asig);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);

	*rolfranco = str_eq (turno, _FRANCO);
	return encontre;
}

// ********************************************************************************************************
// GetHorasTurno : Calcula el horario correspondiente a una fecha
// 
// Parametros: 
// 	fecasig: Fecha de asignación. Para saber a partir de esa fecha como es la rotción de turnos.
// 	rol-fila-colum : Rol que esta asignado el vigilador (En la fecha de asignación).
// 	regim :  Regimen que esta asignado (para saber la duración del turno).
// 	hinicio-hfinal: Hora que tiene que cumplir el primer dia que trabaja.
//  efect : TRUE si es efectivo FALSE sino lo es.
// 	fecha : Fecha que se quiere averiguar.
// 
// Devuelve: 
// 	valor : Turno que le corresponde para la fecha.
// 	hdesde - hhasta: Horario que le corresponde para la fecha.
// 
// 
// Calculo: Rol estatico: No importa si ese dia trabaja o no en el puesto, se mueve en el rol siempre.
//                        Si no trabaja ese dia devuelve que no trabaja, salvo que sea franco.
//          Rol dinamico: Si ese dia no trabaja en el puesto 
// 
// ********************************************************************************************************
void GetHorasTurno (int p_emp, long p_nroleg, long cliente, short objet, DATE fecasig, short rol, 
                    short fila, short colum, char *regim, TIME hinicio, TIME hfinal,DATE fecha,
                    short ptoser, short puesto,	char *dia1, char *dia2, char *dia3, char *dia4,
                    char *dia5, char *dia6, char *dia7, bool efect,	char *valor, char *p_vigil,int  p_numfran,
                    TIME *hdesde, TIME *hhasta)
{

	schema  old, operac;
	short cantrol=0, periodo, ncol;
	short cdias=0, cpaso=0;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	#ifdef _GETHORASTURNO
    fprintf (stderr, "GetHorasTurno VIENE %ld %d fecasig %.3D rol %d fila %d col %d regim %s hini %T hfin %T fecha %.3D %d %d \n",
    						cliente, objet, 
							fecasig, rol, fila, colum, regim, hinicio, hfinal, fecha, ptoser, puesto);
	#endif

	*hdesde=StrToT("0000");
	*hhasta=StrToT("0000");
	strcpy (valor, _NO_TRABAJA);

   	// Verifico cuantos dias trabaja en la semana 
	if (!str_eq(dia1, NULL_STR)) cdias ++;
	if (!str_eq(dia2, NULL_STR)) cdias ++;
	if (!str_eq(dia3, NULL_STR)) cdias ++;
	if (!str_eq(dia4, NULL_STR)) cdias ++;
	if (!str_eq(dia5, NULL_STR)) cdias ++;
	if (!str_eq(dia6, NULL_STR)) cdias ++;
	if (!str_eq(dia7, NULL_STR)) cdias ++;


    // La fecha es incorrecta, porque es menor a la fecha de asignacion 
	if (fecha < fecasig) {
		return ;
	}


	SetIFld(operac|ROL_CODROL, rol);
	if (GetRecord(operac|ROLbyCODROL, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		return ;
	}



	//	Para no recorrer el rol desde la fecha de asignacion hasta la fecha pedida
	//	calculo en base a la cantidad de columnas que tiene el rol 
	SetIFld(operac|RROL_CODROL, rol);
	SetIFld(operac|RROL_FILA,   fila);
	SetIFld(operac|RROL_COLUM,  MIN_SHORT);
	while (GetRecord(operac|RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		cantrol ++;
	}


	if (cantrol == 0) return;


	// Si tiene franco o vacaciones no calculo nada 
	if (Franco (p_emp, p_nroleg, fecha, p_vigil, p_numfran)) {

		*hdesde=StrToT("0000");
		*hhasta=StrToT("0000");

		if (!efect && rol==_ROL_14x7x12)
			strcpy (valor, _NO_TRABAJA);
		else 
			strcpy (valor, SFld(operac|RROL_VALOR));

		return;
	} 

	
	
	if (Vacaciones(p_emp, p_nroleg, fecha)) {
		*hdesde=StrToT("0000");
		*hhasta=StrToT("0000");
		strcpy (valor, SFld(operac|RROL_VALOR));

		return;
	}


	if (cdias == 7 || IFld(operac|ROL_TIPROL) == _ROL_ESTATICO) {
		#ifdef _GETHORASTURNO
		fprintf (stderr, "Trabaja los 7 dias o el rol es estatico - Calculo directo\n");
		#endif
		// Si trabaja toda la semana puedo calcular directamente que turno le corresponde 
		periodo = (fecha-fecasig) / cantrol;

		#ifdef _GETHORASTURNO
		fprintf (stderr, "periodo %d cantrol %d \n", periodo, cantrol);
		#endif

		if (periodo > 0 ) {
			fecasig = fecasig + (periodo * cantrol);
			#ifdef _GETHORASTURNO
			fprintf (stderr, "Fecha %.3D FecAig %.3D periodo %d \n", fecha, fecasig, periodo);
			#endif			
		}
		ncol = colum + (fecha - fecasig);
		ncol = ncol <= cantrol ? ncol : (fecha - fecasig) - (cantrol - colum) ;
 		#ifdef _GETHORASTURNO
 		fprintf (stderr, "ncol = %d \n", ncol);
 		#endif			
	}
	else {
		// Recorro para saber cuantos dias cubrio 
		DATE auxfecha;

		for (cpaso=0, auxfecha=fecasig; auxfecha <= fecha; auxfecha ++) {
			if (!str_eq(DiaLetra(auxfecha), dia1) && !str_eq(DiaLetra(auxfecha), dia2) &&
				!str_eq(DiaLetra(auxfecha), dia3) && !str_eq(DiaLetra(auxfecha), dia4) &&
				!str_eq(DiaLetra(auxfecha), dia5) && !str_eq(DiaLetra(auxfecha), dia6) &&
				!str_eq(DiaLetra(auxfecha), dia7))
					continue;
			cpaso ++;
		}
		
		if (!cpaso)	return;

		cpaso --; //Esto es para ajustar la cantidad de dias que pasaron.
		#ifdef _GETHORASTURNO
		fprintf (stderr, "Trabaja %d dias - Pasaron %d dias de la fecha de asignacion\n", cdias, cpaso);
        #endif

		periodo = cpaso / cantrol;

		#ifdef _GETHORASTURNO
		fprintf (stderr, "periodo %d cantrol %d \n", periodo, cantrol);
		#endif

		if (periodo > 0 ) {
			cpaso = cpaso - (periodo * cantrol);
			#ifdef _GETHORASTURNO
			fprintf (stderr, "Fecha %.3D periodo %d cpaso quedo en %d \n", fecha, periodo, cpaso);
			#endif
		}

		ncol = colum + cpaso;
		ncol = ncol <= cantrol ? ncol : cpaso - (cantrol - colum) ;

		#ifdef _GETHORASTURNO
		fprintf (stderr, "Valor de ncol %d \n", ncol);
		#endif
	}

	#ifdef _GETHORASTURNO
	if (ncol < 0 ) fprintf (stderr, "Nueva columna %d \n", ncol);
	#endif
	SetKey(operac|RROLbyCODROL, rol, fila, ncol);
	if(GetRecord(operac|RROLbyCODROL, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		return;
	} 

	#ifdef _GETHORASTURNO
	fprintf (stderr, "Va a llamar a HoraInicio hinicio  %.3T regim %s SFld(operac|RROL_VALOR) %d %d %d %s \n", hinicio, regim, IFld(operac|RROL_CODROL), IFld(operac|RROL_FILA), IFld(operac|RROL_COLUM), SFld(operac|RROL_VALOR)); 
	#endif

	*hdesde = HoraInicioTurno(hinicio, regim, SFld(operac|RROL_VALOR));

	#ifdef _GETHORASTURNO
	fprintf (stderr, "Va a llamar a HoraFinTurno hinicio  %.3T regim %s SFld(operac|RROL_VALOR) %d %d %d %s \n", hinicio, regim, IFld(operac|RROL_CODROL), IFld(operac|RROL_FILA), IFld(operac|RROL_COLUM), SFld(operac|RROL_VALOR)); 
	#endif

	*hhasta = HoraFinTurno(cliente, objet, ptoser, puesto, hinicio, regim, SFld(operac|RROL_VALOR));   
	strcpy (valor, SFld(operac|RROL_VALOR));
	#ifdef _GETHORASTURNO
	fprintf (stderr, "Hdesde %.3T hhasta %.3T \n", *hdesde, *hhasta);
	#endif

	// Si es franco o no trabaja va sin horas 
	if (str_eq(valor, _FRANCO) || str_eq(valor, _NO_TRABAJA)) {
		*hdesde=StrToT("0000");
		*hhasta=StrToT("0000");
	}

	if (str_eq(valor, _TURNO_INICIAL)){
		// Si es el turno inicial pongo directamente el horario de la asignacion 
		#ifdef _GETHORASTURNO
		fprintf (stderr, "Turno inicial - Hora de la asignacion \n");
		#endif

		*hdesde=hinicio;
		*hhasta=hfinal;
	}

	#ifdef _GETHORASTURNO
	fprintf (stderr, "valor %C  VALOR %C \n", valor, SFld(operac|RROL_VALOR));
	#endif



	// Si el rol es dinamico valido que el dia trabaje
	// si el rol es estatico y es franco lo genero igual 
	if ((IFld(operac|ROL_TIPROL) == _ROL_ESTATICO && !str_eq(valor, _FRANCO)) ||
		 IFld(operac|ROL_TIPROL) == _ROL_DINAMICO){ 
		if (!str_eq(DiaLetra(fecha), dia1) && !str_eq(DiaLetra(fecha), dia2) &&
	 		!str_eq(DiaLetra(fecha), dia3) && !str_eq(DiaLetra(fecha), dia4) &&
			!str_eq(DiaLetra(fecha), dia5) && !str_eq(DiaLetra(fecha), dia6) &&
			!str_eq(DiaLetra(fecha), dia7)) {
				#ifdef _GETHORASTURNO
				fprintf (stderr, "No trabaja ese dia \n");
				#endif

			*hdesde=StrToT("0000");
			*hhasta=StrToT("0000");
			strcpy (valor, _NO_TRABAJA);
			return;
		}
	}

	#ifdef _GETHORASTURNO
	fprintf (stderr, "Fin: Fecha %.3D Turno %T %T rol %d fila %d col %d regim %s valor %s \n\n", fecha, *hdesde, *hhasta, rol, fila, ncol, regim, valor);
	#endif
}

void DesArmarCCosto(long nroccte, long *vcli, short *vobj)
{
//	double resto, real, valor;
//	int aux;

	// #include <math.h>

	*vcli = NULL_LONG;
	*vobj = NULL_SHORT;

	if (nroccte == NULL_LONG) {
		return ;
	}

	//Los ultimos dos digitos pertenecen al objetivo, el resto al cliente
//	valor = (double) nroccte / 100.0;

//	resto = modf(valor, &real);

	// *vcli = real;
 	
 	
 	*vcli =nroccte/100 ;
	*vobj =(nroccte % 100);
	//fprintf (stderr, "CLIENTEOBJ %ld %ld %d %.2f %.2f %9.60f \n", nroccte, *vcli, *vobj, real, resto, resto * 100.0);
}

char *TipoDia(long cliente, int objet, int tippto, int codint)
{
	schema  old, operac;
    static char dia[2];

	old		= CurrentSchema();
	operac	= OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetLFld(operac|PUESTOS_CLIENTE, cliente);	
	SetIFld(operac|PUESTOS_OBJET,   objet);	
	SetIFld(operac|PUESTOS_TIPPTO,  tippto);	
	SetIFld(operac|PUESTOS_CODINT,  codint);	
	if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strcpy(dia, SFld(operac|PUESTOS_TIPODIA));
	else
	 	strcpy(dia, NULL_STR);

	SwitchToSchema(old);
	return dia;
}

// ---------------------* FechaInicioPuesto *--------------------
DATE FechaInicioPuesto(long cliente, int objet, int tippto, int codint)
{
	schema  old, operac;
	dbtable APUESTOS = (dbtable) ERROR;
	static DATE fecinic;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	APUESTOS = CreateAlias(operac|PUESTOS);

	SetLFld(APUESTOS_CLIENTE, cliente);
	SetIFld(APUESTOS_OBJET,   objet);
	SetIFld(APUESTOS_TIPPTO,  tippto);
	SetIFld(APUESTOS_CODINT,  codint);
	if (GetRecord(APUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fecinic = DFld(APUESTOS_FINICIO);
	else
		fecinic = NULL_DATE;

	DeleteAlias(APUESTOS);
	SwitchToSchema(old);
	return fecinic;
}
//---------------------* FechaFinPuesto *--------------------
DATE FechaFinPuesto(long cliente, int objet, int tippto, int codint)
{
	schema  old, operac;
	dbtable APUESTOS = (dbtable) ERROR;
	static DATE fecfin;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	APUESTOS = CreateAlias(operac|PUESTOS);

	SetLFld(APUESTOS_CLIENTE, cliente);
	SetIFld(APUESTOS_OBJET,   objet);
	SetIFld(APUESTOS_TIPPTO,  tippto);
	SetIFld(APUESTOS_CODINT,  codint);
	if (GetRecord(APUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fecfin = DFld(APUESTOS_FFINAL);
	else
		fecfin = NULL_DATE;

	DeleteAlias(APUESTOS);
	SwitchToSchema(old);
	return fecfin;
}

bool CorrespondeDiaPuesto(short emp, long cliente, short objetivo, short tippto, short codint, DATE fecha)
{

	schema prev, com;
	short pais, prov;
	char tipodia[10];
	bool feriado;

	strcpy(tipodia, TipoDia(cliente, objetivo, tippto, codint));
	
	if (*tipodia ==_TIPO_DIA_T) {
		return TRUE;
	}

	prev = CurrentSchema();
	com = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(prev);

	SetKey(com|OBJETIVObyCLIENTE, cliente, objetivo);
	(void)GetRecord(com|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	pais = IFld(com|OBJETIVO_PAIS);
	prov = IFld(com|OBJETIVO_PROV);			
	feriado = FeriadoNovia(fecha, pais, prov);

	if (feriado && *tipodia == _TIPO_DIA_F) {
		return TRUE;
	}
	if (!feriado && *tipodia == _TIPO_DIA_H) {
		return TRUE;
	}
	return FALSE;
}

// GetCliObjEfectivo 
// Devuelve el cliente/objetivo que esta asignado en forma efectiva

void GetCliObjEfectivo(int emp, long nroleg, DATE fecha, long * cliente, int * objetivo)
{
	bool     encontre = FALSE;
	schema   old, operac;
	dbtable  AASIG, AASIGH;
	dbcursor c_asig, c_asigh;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIG   = CreateAlias(operac|ASIG);
	AASIGH  = CreateAlias(operac|ASIGH);
	c_asig  = CreateCursor(AASIGbyNROLEG,  IO_NOT_LOCK);
	c_asigh = CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

	*cliente=NULL_LONG;
	*objetivo=NULL_SHORT;
	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO)) {
			continue;
		}
		if (fecha < DFld(AASIG_FECASIG))
			continue;
		encontre = TRUE;
		*cliente=LFld(AASIG_CLIENTE);
		*objetivo=IFld(AASIG_OBJETIVO);
	}
	if (!encontre) { 	// devolver ultima asignacion efectiva: es la maxima fecha de alta
		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {

			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO))
				continue;
				
            if ( DFld(AASIGH_FECALT) > DFld(AASIGH_FECBAJ) || IFld(AASIGH_MOTIVO)==DESXERROR )
				continue;

			if (fecha >= DFld(AASIGH_FECALT) && fecha <= DFld(AASIGH_FECBAJ)) {
				*cliente  = LFld(AASIGH_CLIENTE);
				*objetivo = IFld(AASIGH_OBJETIVO);
			}
		}
	}
	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
}

// ************************* ExisteCliObjEnGrp *************************
bool ExisteCliObjEnGrp(int grupo, long cliente, int objetivo)
{
	schema prev, billpro;

	prev    = CurrentSchema();
	billpro = OpenSchema("billpro", IO_EABORT);
	SwitchToSchema(prev);

	SetKey(billpro|RCLIESPbyTIPCLI, grupo, cliente, NULL_SHORT);
	if (GetRecord(billpro|RCLIESPbyTIPCLI, THIS_KEY, IO_NOT_LOCK) != ERROR)
		return TRUE;
	else {
		SetKey(billpro|RCLIESPbyTIPCLI, grupo, cliente, objetivo);
		if (GetRecord(billpro|RCLIESPbyTIPCLI, THIS_KEY, IO_NOT_LOCK) != ERROR)
			return TRUE;
	}

	return FALSE;
}

// ************************* FecIng *************************
DATE FecIng(int emp, long nroleg)
{
	schema sue, old;
	DATE fecing = NULL_DATE;

	old = CurrentSchema();
	sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetIFld(sue|PER_EMP,    emp);
	SetLFld(sue|PER_NROLEG, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fecing = DFld(sue|PER_FECING);

	return fecing;
}


// ***************** MotDesagRota ******************
bool MotDesagRota(int motivo)
{
	schema operac, old;
	bool rotacion = FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	SetIFld(operac|MOTIVD_CODMOTD, motivo);
	if (GetRecord(operac|MOTIVDbyCODMOTD, THIS_KEY, IO_NOT_LOCK) != ERROR)
		rotacion = IFld(operac|MOTIVD_ROTACION);

	return rotacion;
}

// ***************** GetDescMotivd ******************
char * GetDescMotivd(int motivd)
{
	schema old, operac;
	static char dmotivd[30];

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);	

	SetIFld(operac|MOTIVD_CODMOTD, motivd);
	if (GetRecord(operac|MOTIVDbyCODMOTD, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strcpy(dmotivd, SFld(operac|MOTIVD_DESCRIP));
	else
		strcpy(dmotivd, NULL_STR);

	SwitchToSchema(old);
	return dmotivd;
}
// **********************************************************************************************************
// *                                            LeePuestos
// * Parametros:  
// *     long clie              = cliente
// *     int  obj               = objetivo
// *     int puestos[500][2]    = Matriz para devolucion de datos
// *     DATE p_fecha           = fecha a la cual se hace la consulta
// *     bool verpadre          = Si en verdadero muestra los puestos padre con la cantidad de puestos
// *                              (descontandole los hijos), si es falso solo muestra los hijos (activos)
// * Devuelve : 
// *    Matriz (puestos) con puestos mimp hijos y padres, segun se le indique en parametro "verpadre"
// *
// **********************************************************************************************************
void LeePuestos(long clie, int obj, int puestos[500][3], DATE p_fecha, bool verpadre)
{
	int v_i=0;
	dbcursor c_puestos;
	schema old, operac;
	bool leyo=FALSE;
	int cantpue=0;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);	

	// Inicializo matriz en la que devolvere puestos
	for(v_i=0; v_i<500 ; v_i++) {
		puestos[v_i][COLTIPPTO]=NULL_SHORT;
		puestos[v_i][COLCODINT]=NULL_SHORT;
		puestos[v_i][COLCANPUE]=NULL_SHORT;
	}
	
	v_i=0;
	c_puestos=CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(c_puestos, clie, obj, NULL_SHORT, NULL_SHORT);
	SetCursorTo  (c_puestos, clie, obj, MAX_SHORT,  MAX_SHORT);
	while(FetchCursor(c_puestos)!=ERROR){
		// Leo solo los puestos padres 
		if(!IsNull(operac|PUESTOS_PADREINT))
			continue;

		if (BajaPuesto(clie, obj, IFld(operac|PUESTOS_TIPPTO), p_fecha))
			continue;

		if (!ValidaPuesto(operac, p_fecha))
			continue;

		cantpue=IFld(operac|PUESTOS_CANTPUE);
		
		leyo=LeeProximo(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET),
		                IFld(operac|PUESTOS_TIPPTO),  IFld(operac|PUESTOS_CODINT),
		                 &v_i, puestos, operac, p_fecha, &cantpue, verpadre);

		if (!leyo || (verpadre && cantpue>0)) {
			puestos[v_i][COLTIPPTO]=IFld(operac|PUESTOS_TIPPTO);                   
			puestos[v_i][COLCODINT]=IFld(operac|PUESTOS_CODINT);
			puestos[v_i][COLCANPUE]=cantpue;
			v_i++;
		}
	}
	DeleteCursor(c_puestos);	
}

// **********************************************************************************************************
// * LeeProximo: trabaja en conjunto con LeePuesto, ver esa funcion
// **********************************************************************************************************

bool LeeProximo(long clie, int obj, int pto, int cod, int *p_i, int puestos[500][3], schema operac,
                DATE p_fecha, int *cantpue, bool verpadre)
{
	dbcursor c_puesto;
	bool encontro=FALSE;
	bool leyo=FALSE;
	int cantpuei=0;

	PushRecord(operac|PUESTOS);
	c_puesto=CreateCursor(operac|PUESTOSbyPADRE, IO_NOT_LOCK);
	SetCursorFrom(c_puesto, clie, obj, pto, cod);
	SetCursorTo  (c_puesto, clie, obj, pto, cod);
	while(FetchCursor(c_puesto)!=ERROR) {

		if (BajaPuesto(clie, obj, pto, p_fecha))
			continue;

		if (!ValidaPuesto(operac, p_fecha)) 
			continue;

		cantpuei=IFld(operac|PUESTOS_CANTPUE);

		leyo=LeeProximo(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET),
		                IFld(operac|PUESTOS_TIPPTO),  IFld(operac|PUESTOS_CODINT), p_i, puestos, operac,
		                p_fecha, &cantpuei, verpadre);

		(*cantpue) -=IFld(operac|PUESTOS_CANTPUE);

		if (!leyo || (verpadre && cantpuei>0)) {
			puestos[*p_i][COLTIPPTO]=IFld(operac|PUESTOS_TIPPTO);
			puestos[*p_i][COLCODINT]=IFld(operac|PUESTOS_CODINT);
			puestos[*p_i][COLCANPUE]=cantpuei;
			(*p_i)++;
		}
		encontro=TRUE;
	}
	PopRecord(operac|PUESTOS);
	return encontro;

}

bool ValidaPuesto(schema operac, DATE p_fecha)
{

	if ( IFld(operac|PUESTOS_CANTVIG) != 0.0 &&
	     IsNull(operac|PUESTOS_FFINAL)       &&
	    !IsNull(operac|PUESTOS_FINICIO) &&
		p_fecha < DFld(operac|PUESTOS_FINICIO) )
		return FALSE;

	if ( !IsNull(operac|PUESTOS_FFINAL)       &&
	    (p_fecha < DFld(operac|PUESTOS_FINICIO) || p_fecha > DFld(operac|PUESTOS_FFINAL) ) )
		return FALSE;

	if  (IFld(operac|PUESTOS_CANTVIG) == 0.0 &&
	     IsNull(operac|PUESTOS_FFINAL)       &&
		 p_fecha > DFld(operac|PUESTOS_FINICIO))
		return FALSE;

	return TRUE;

}

// *********************************************************************************************************
// *                               HelpPto
// *
// *	Ayuda manual de Puestos 
// *
// * Parametros:
// *
// *	fm        = descriptor de formulario 
// *	fno       = descriptor de campo
// *   row       = Numero de linea
// *   p_cliente = codigo de cliente
// *   p_objetivo= codigo de objetivo
// *   p_tippto  = tipo de puesto
// *   p_fecha   = Selecciona puestos en los que entraria esta fecha, si es NULL_DATE trae todos
// *   p_vigil   =
// *   p_efect   =
// *
// *
// * Funciones Dependientes: 
// *
// *           private int ValidaPto()
// *           void DisplayPto(char *buffer)
// *
// *********************************************************************************************************
fm_status HelpPto(form fm, fmfield fno, int row, long p_cliente, int p_objetivo, int p_tippto, 
                  DATE p_fecha, char * p_vigil, char *p_efect)
{
	static dbcursor CUR;
	schema old, ope;
	int n;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(ope);	

	CUR = CreateCursor(ope|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, p_cliente, p_objetivo, p_tippto, MIN_SHORT);
	SetCursorTo  (CUR, p_cliente, p_objetivo, p_tippto, MAX_SHORT);

	fechapuesto=p_fecha;
	sprintf(g_efect, "%s", p_efect);
	sprintf(g_vigil, "%s", p_vigil);
	
	n = PopUpDbMenu(10, 95, "Pto M Cteg  HrIni  HrFin      Dias        Regimen  CantVig CantPto TipoDia  FecIni   FecFin", CUR, 4, ValidaPto, DisplayPto);

	if (n >= 0) { 
		FmSetIFld (fm, fno, IFld(ope|PUESTOS_CODINT), row);
		FmShowFlds(fm, fno, fno, row);
	}
	DeleteCursor(CUR);

	return FM_OK;
}

int ValidaPto()
{
	int p_codint;

	p_codint = IFld(PUESTOS_CODINT);

	if (fechapuesto!=NULL_DATE){

		if (IFld(PUESTOS_CANTVIG) != 0.0 &&
		   ((IsNull(PUESTOS_FFINAL)  && !IsNull(PUESTOS_FINICIO) &&
			fechapuesto < DFld(PUESTOS_FINICIO)) ||
		   (!IsNull(PUESTOS_FFINAL) &&
		   (fechapuesto < DFld(PUESTOS_FINICIO) || fechapuesto > DFld(PUESTOS_FFINAL)))))
			return FALSE;



		if (IFld(PUESTOS_CANTVIG) == 0.0 && !IsNull(PUESTOS_FFINAL) &&
		    (fechapuesto > DFld(PUESTOS_FFINAL) || fechapuesto < DFld(PUESTOS_FINICIO)))
			return FALSE;


		if (IFld(PUESTOS_CANTVIG) == 0.0 && IsNull(PUESTOS_FFINAL) &&
		    fechapuesto > DFld(PUESTOS_FINICIO))
			return FALSE;


	}

	if (strcmp(g_vigil, NULL_STR)!=0 && strcmp(g_efect, NULL_STR)!=0) {


		// Verifica según el tipo de vigilador que es V, P, R, que sea de ese tipo de puesto part-time u otros.
		if (strcmp(g_vigil, "P") == 0 && strcmp(g_efect, "E") == 0) {
			if (IsNull(PUESTOS_CODFREC) && IsNull(PUESTOS_HORAPT))
				return FALSE;
		}

		if (strcmp(g_vigil, "V") == 0 && strcmp(g_efect, "E") == 0) {
			if (!IsNull(PUESTOS_CODFREC) || !IsNull(PUESTOS_HORAPT))
				return FALSE;
		}

		
	}


	return TRUE;
}
void DisplayPto(char *buffer)
{
	char esmimp[2];

	if (IsNull(PUESTOS_PADREINT))
		sprintf(esmimp," ");
	else
		sprintf(esmimp,"*");

	sprintf(buffer,"%2d %s %4d  %.*T  %.*T  %-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s  %8.8s    %4.2f    %3.3d    %2.2s     %.1D %.1D ",
			IFld(PUESTOS_CODINT), esmimp, IFld(PUESTOS_PUESTO), DFMT_SEPAR,
			TFld(PUESTOS_HINICIO), DFMT_SEPAR, TFld(PUESTOS_HFINAL), SFld(PUESTOS_DIA1),
			SFld(PUESTOS_DIA2), SFld(PUESTOS_DIA3), SFld(PUESTOS_DIA4),
			SFld(PUESTOS_DIA5), SFld(PUESTOS_DIA6), SFld(PUESTOS_DIA7),
			SFld(PUESTOS_REGIM), (double)IFld(PUESTOS_CANTVIG)/100.00,
			IFld(PUESTOS_CANTPUE),SFld(PUESTOS_TIPODIA),
			DFld(PUESTOS_FINICIO), DFld(PUESTOS_FFINAL));

}

bool PuestoAsignado(int p_emp, long p_cliente, int p_objet, int p_tippto, int p_codint, DATE p_fecfin)
{
	dbcursor c_asig, c_asigh;
	bool enasig;
	schema old, ope;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(ope);	

	enasig=FALSE;
	c_asig=CreateCursor(ope|ASIGbyPUESTO, IO_NOT_LOCK);
	SetCursorFrom(c_asig, p_emp, p_cliente, p_objet, p_tippto, p_codint, NULL_SHORT, NULL_LONG);
	SetCursorTo  (c_asig, p_emp, p_cliente, p_objet, p_tippto, p_codint, MAX_SHORT, MAX_LONG);
	while(FetchCursor(c_asig)!=ERROR) {
		if (p_fecfin <= DFld(ope|ASIG_FECBAJ) || IsNull(ope|ASIG_FECBAJ)){
			enasig=TRUE;
			break;
		}
	}
	DeleteCursor(c_asig);

	if (!enasig) {
		c_asigh=CreateCursor(ope|ASIGHbyEMP, IO_NOT_LOCK);
		SetCursorFrom(c_asigh, p_emp, p_cliente, p_objet, p_tippto, p_codint, NULL_SHORT, NULL_LONG,
		              NULL_DATE, NULL_DATE);
		SetCursorTo  (c_asigh, p_emp, p_cliente, p_objet, p_tippto, p_codint, MAX_SHORT, MAX_LONG,
		              MAX_DATE, MAX_DATE);
		while(FetchCursor(c_asigh)!=ERROR) {
			if(p_fecfin>=DFld(ope|ASIGH_FECALT) && p_fecfin <=DFld(ope|ASIGH_FECBAJ)){
				enasig=TRUE;
				break;
			}
		}
		DeleteCursor(c_asigh);
	}

	return enasig;

}
// ************************* GetConvenio ************************
int GetConvenio(int emp, long nroleg)
{
	schema sue, old;
	int convenio = NULL_SHORT;

	old = CurrentSchema();
	sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetKey(sue|PERbyEMP, emp, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		convenio = IFld(sue|PER_RELACION);

	return convenio;
}

char *TituloMsg(int p_tipomsg, int p_codigo) {

	sprintf(g_titmsg, "[ %s", NULL_STR);

	switch (p_tipomsg) {
		case TMSG_WAR:
			sprintf(g_titmsg, "%sAviso ", g_titmsg);
			break;
		case TMSG_ERR: 
			sprintf(g_titmsg, "%sError ", g_titmsg);
			break;
	}
	sprintf(g_titmsg, "%s Cod. %d ]", g_titmsg, p_codigo);

	return(g_titmsg);
}
// ***********************************************************************************************************
// *                                            ExisteParteCargado
// * Parametros: 
// *	p_emp:       Empresa
// *	p_cliente:   Cliente 
// *	p_objetivo:  Objetivo
// *	p_nroleg:    Número de Legajo
// *	p_fechad:    Fecha Desde
// *	p_fechah:    Fecha Hasta
// *
// * Funcionamiento: Recorre el PARTE con los parametros pasado y si existe algo cargado en este que 
// *                 tenga algun campo de hora distinto de 0 devuelve TRUE, si no encuentra nada devuelve
// *                 FALSE
// *
// ***********************************************************************************************************
bool ExisteParteCargado(int p_emp, long p_cliente, int p_objetivo, long p_nroleg, int tippto, int puesto, DATE p_fechad, DATE p_fechah)
{
	dbcursor v_c_parte;
	bool v_existe = FALSE;

	schema ope, old;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	v_c_parte = CreateCursor(ope|PARTEbyLEG, IO_NOT_LOCK);

	SetCursorFrom(v_c_parte, p_emp, p_cliente, p_objetivo, p_nroleg, p_fechad);
	SetCursorTo  (v_c_parte, p_emp, p_cliente, p_objetivo, p_nroleg, p_fechah == NULL_DATE ? MAX_DATE : p_fechah);

	while (FetchCursor(v_c_parte) != ERROR) {

		if ( tippto!=IFld(ope|PARTE_PTOSER) && puesto!=IFld(ope|PARTE_PUESTO) )
			continue; 

		if (IFld(ope|PARTE_HSNOR)  != 0 || IFld(ope|PARTE_HS50) != 0 ||
			IFld(ope|PARTE_HS100F) != 0 || IFld(ope|PARTE_HS100FE) != 0) {
			v_existe = TRUE;
			break;
		}
	}

	DeleteCursor(v_c_parte);
	return v_existe;
}
// ***********************************************************************************************************
// *                                          BorrarParteGenerado
// * Parametros: 
// *	p_emp:       Empresa
// *	p_cliente:   Cliente 
// *	p_objetivo:  Objetivo
// *	p_nroleg:    Número de Legajo
// *	p_fechad:    Fecha Desde
// *	p_fechah:    Fecha Hasta
// *	p_fecbaj:    Fecha de Baja (fecha desde la que se comienza a borrar cuando es por fecha)
// * p_porfecha:  Define como recorrera el parte la funcion
// *	p_efect :    Efectivo o provisorio
// * p_ptoser,
// * p_puesto, 
// *	p_nroint:	Definen el puesto
// *
// * Funcionamiento: Recorre el PARTE con los parametros pasado borrando estos registros
// *
// ***********************************************************************************************************
void BorrarParteGenerado(int p_emp, long p_cliente, int p_objetivo, long p_nroleg, DATE p_fechad,
                         DATE p_fechah, DATE p_fecbaj, bool p_porfecha, char *p_efect, int p_ptoser,
                         int p_puesto, int p_nroint)
{
	dbcursor curpar;
	schema ope, old;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	if (!p_porfecha) {
		curpar = CreateCursor(ope|PARTEbyLEG, IO_EABORT|IO_NOT_LOCK);
		SetCursorFrom(curpar, p_emp, p_cliente, p_objetivo, p_nroleg, p_fechad);
		SetCursorTo  (curpar, p_emp, p_cliente, p_objetivo, p_nroleg, p_fechah == NULL_DATE ? MAX_DATE : p_fechah);
		while (FetchCursor(curpar) != ERROR)
			DelRecord(ope|PARTE);
	}
	else {
		curpar = CreateCursor(ope|PARTEbyEMPLE, IO_EABORT|IO_NOT_LOCK);
		SetCursorFrom(curpar, p_emp, p_nroleg, p_fecbaj, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (curpar, p_emp, p_nroleg, MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(curpar) != ERROR) {
			if (*p_efect == 'P') {
				if (p_cliente == LFld(ope|PARTE_CLIENTE) && p_objetivo == IFld(ope|PARTE_OBJETIVO) && 
					p_ptoser == IFld(ope|PARTE_PTOSER) && p_puesto == IFld(ope|PARTE_PUESTO) &&
					p_nroint == IFld(ope|PARTE_NROINT))
					DelRecord(ope|PARTE);
			}
			else
				DelRecord(ope|PARTE);			
		}
	}
	DeleteCursor(curpar);
}

void VerificoDobleAsig(int p_emp, long p_nroleg)
{


	int  v_cant  = NULL_SHORT,
	     v_objet = NULL_SHORT,
	     v_tippto= NULL_SHORT,
	     v_puesto= NULL_SHORT,
	     v_nroint= NULL_SHORT;

	long v_cliente = NULL_LONG;
	DATE v_fecasig = MAX_DATE;

	schema ope, old;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	PushRecord(ope|ASIG);
	SetKey(ope|ASIGbyNROLEG, p_emp, p_nroleg, MIN_LONG, MIN_SHORT);
	while (GetRecord(ope|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (*SFld(ope|ASIG_EFECT) == 'E') {
			v_cant++;
			if (DFld(ope|ASIG_FECASIG) < v_fecasig) {
				v_fecasig = DFld(ope|ASIG_FECASIG);
				v_cliente = LFld(ope|ASIG_CLIENTE);
				v_objet   = IFld(ope|ASIG_OBJETIVO); 
				v_tippto  = IFld(ope|ASIG_PTOSER);
				v_puesto  = IFld(ope|ASIG_PUESTO); 
				v_nroint  = IFld(ope|ASIG_NROINT);
			}
		}
	}

	// Si esta duplicada la asignacion efectiva se borra la de fecha mas vieja
 	if (v_cant > 1) {
	 	SetKey(ope|ASIGbyLEGFEC, p_emp, p_nroleg, v_fecasig, v_cliente, v_objet);
		if (GetRecord(ope|ASIGbyLEGFEC, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			DelRecord(ope|ASIG);

			// Si es parttime  borro tambien los diasptime de este puesto
		 	if (*SFld(ope|ASIG_VIGIL) == 'P') {
				SetKey(ope|DIASPTIMEbyEMP, p_emp, v_cliente, v_objet, p_nroleg, v_tippto, v_puesto, v_nroint, MIN_DATE);
				while (GetRecord(ope|DIASPTIMEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) 
					DelRecord(ope|DIASPTIME);
			}

		}
	}

	PopRecord(ope|ASIG);

}

/***********************************************************************************************************
*                              CalculaFechaFranco y ControlFechaFranco
* Parametros:
*	p_rol, p_fila, p_colum = Definicion de rol
*	p_fecha                = Fecha de comienzo de asignacion
*	p_clieot               = Cliente 
*	p_objet                = Objetivo
*	p_tippto, p_codint     = Puesto
*	p_calcfec              = Parametro de devolucion de datos (fecha de franco)
*	p_lugar                = Parametro de devolucion de datos (numero de franco)
*
* Devuelve TRUE si pudo calcular, FALSE en caso contrario
*
***********************************************************************************************************/
bool CalculaFechaFranco(int p_rol, int p_fila, int p_colum, DATE p_fecha, long p_clieot, int p_objet, int p_tippto, int p_codint, DATE *p_calcfec, int *p_lugar)
{

	int 	totcol = 0;
	schema 	ope, old;
	bool 	es_franco=FALSE, 
			encontro_f=FALSE;
	bool 	v_msg	= FALSE;

	old = CurrentSchema();
	ope = FindSchema("operac");
	SwitchToSchema(old);

	if (p_rol==NULL_SHORT || p_fila==NULL_SHORT || p_colum==NULL_SHORT) {
		*p_calcfec=NULL_DATE;
		*p_lugar  =NULL_SHORT;
		return TRUE;
	}

	if (v_msg)fprintf(stderr, "CalculaFechaFranco: rol %d fila %d columna %d\n", p_rol, p_fila, p_colum);


	// Calculo Total de Columnas del Rol 
	SetKey(ope|RROLbyCODROL, p_rol, p_fila, MAX_SHORT );
	if (GetRecord(ope|RROLbyCODROL, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		totcol=IFld(ope|RROL_COLUM);
	}
	if (v_msg)fprintf(stderr, "CalculaFechaFranco: Total de Columnas del Rol: %d \n", totcol);

	// Me Fijo Si la Fecha de Asignacion Cae un Franco
	es_franco=FALSE;
	SetKey(ope|RROLbyCODROL, p_rol, p_fila, p_colum);
	if (GetRecord(ope|RROLbyCODROL, THIS_KEY, IO_NOT_LOCK, 2) != ERROR) 
		if (strcmp(SFld(ope|RROL_VALOR), "F")==0)
			es_franco=TRUE;

	if (!es_franco){ //Si no es Franco Busco el Primer Franco, si es Franco me Fijo que Numero Franco es 

		encontro_f=FALSE;

		// Busco si hay un franco despues 

		SetKey(ope|RROLbyCODROL, p_rol, p_fila, p_colum);
		while (GetRecord(ope|RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {

			if (v_msg)fprintf(stderr, "CalculaFechaFranco Busca Franco Despues: Columna rol: %d Valor: %s\n", IFld(ope|RROL_COLUM), SFld(ope|RROL_VALOR));

			if (strcmp(SFld(ope|RROL_VALOR), "F")==0) {
				encontro_f=TRUE;
				break;
			}
		}
		
        // Si no hay Franco despues busco uno anterior
		if (!encontro_f){ 
			/* Calculo de fecha de primer franco */
			SetKey(ope|RROLbyCODROL, p_rol, p_fila, NULL_SHORT);
			while (GetRecord(ope|RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {

				if (v_msg)fprintf(stderr, "CalculaFechaFranco Busca Franco Antes: Columna rol: %d Valor: %s\n", IFld(ope|RROL_COLUM), SFld(ope|RROL_VALOR));

				if (strcmp(SFld(ope|RROL_VALOR), "F")==0) {
					encontro_f=TRUE;
					break;
				}
			}
		}

		if (encontro_f){ 
			*p_lugar=1;
			if (p_colum<=IFld(ope|RROL_COLUM))
				*p_calcfec = p_fecha + ( IFld(ope|RROL_COLUM) - p_colum );
			else
				*p_calcfec = p_fecha + (( totcol - p_colum ) + IFld(ope|RROL_COLUM));
		}
		else {  // Si el Rol no tiene Francos
			*p_calcfec=NULL_DATE;
			*p_lugar  =NULL_SHORT;
		} 
	}
	else {
		*p_calcfec = p_fecha;

		/* Si cae en un franco averiguo que numero de franco es */
		*p_lugar=1;
		SetKey(ope|RROLbyCODROL, p_rol, p_fila, p_colum);
		while (GetRecord(ope|RROLbyCODROL, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR && strcmp(SFld(ope|RROL_VALOR), "F") == 0) {
			*p_lugar = *p_lugar + 1;
		} 
		if (IFld(ope|RROL_COLUM)==1) { // por si Llegue a la Primera Columna y la Ultima Tambien es Franco
			SetKey(ope|RROLbyCODROL, p_rol, p_fila, MAX_SHORT);
			while (GetRecord(ope|RROLbyCODROL, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR && strcmp(SFld(ope|RROL_VALOR), "F") == 0) {
				*p_lugar = *p_lugar + 1;
			} 
			
		}
				
	}

	if (v_msg)fprintf(stderr, "CalculaFechaFranco:  p_lugar %d p_calcfec %.3D\n", *p_lugar, *p_calcfec);

	if (ControlFechaFranco(*p_calcfec, p_clieot, p_objet, p_tippto, p_codint)) {
		return TRUE;
	}

	return FALSE;
}

bool ControlFechaFranco(DATE p_fecfra, long p_clieot, int p_objet, int p_tippto, int p_codint)
{
	int v_campo;
	bool v_falta;
	bool v_esta;
	bool v_msg=FALSE;
	char v_letradia[2];
	schema ope, old;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	if (v_msg)fprintf(stderr, "ControlFechaFranco: p_fecfra %.3D\n", p_fecfra);

	PushRecord(ope|PUESTOS);
	SetKey(ope|PUESTOSbyCLIENTE, p_clieot, p_objet, p_tippto, p_codint);
	GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	/* Me fijo si v_estan todos los dias de la semana */
	v_falta=FALSE;
	for (v_campo=PUESTOS_DIA1; v_campo<=PUESTOS_DIA7; v_campo++)
		if (IsNull(ope|v_campo)) {
			v_falta=TRUE;
			break;
		}

	if (v_msg)fprintf(stderr, "ControlFechaFranco: v_falta %B\n", v_falta);


	if (!v_falta) {
		PopRecord(ope|PUESTOS);
		return TRUE;
	}

	/* Me fijo si la fecha de franco coincide con algun dia que falte */

	sprintf(v_letradia, "%s",DiaLetra(p_fecfra));

	if (v_msg)fprintf(stderr, "ControlFechaFranco: v_letradia %s\n", v_letradia);

	v_esta=FALSE;
	for (v_campo=PUESTOS_DIA1; v_campo<=PUESTOS_DIA7; v_campo++) {
		if (strcmp(v_letradia, SFld(ope|v_campo))==0) {
			v_esta=TRUE;
			break;
		}
	} 

	PopRecord(ope|PUESTOS);

	if (v_msg)fprintf(stderr, "ControlFechaFranco: v_esta %B\n", v_esta);

	if (v_esta) 
		return FALSE;

	return TRUE;
}



// **********************************************************************************************************
// *                                             LegActivo
// * Parametros:
// *	p_emp    : Empresa
// *	p_nroleg : Numero de legajo
// *
// * Devuelve : Si legajo esta activo o no
// *
// ***********************************************************************************************************

int  LegActivo (int p_emp, long p_nroleg)
{
	int v_valido=0;
	schema v_sue, old; 

	old = CurrentSchema();
	v_sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetKey(v_sue|PERbyEMP, p_emp, p_nroleg);
	if (GetRecord(v_sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) 
		v_valido = IFld(v_sue|PER_ACTIVO);

	return v_valido;
}

// ***********************************************************************************************************
// *                                            GetCategoria
// * Parametros:
// *	p_emp    : Empresa
// *	p_nroleg : Numero de legajo
// *
// * Devuelve : Si legajo esta activo o no
// *
// ***********************************************************************************************************
int  GetCategoria (int p_emp, long p_nroleg)
{
	int  v_codcat=NULL_SHORT;
	schema v_sue, old;

	old = CurrentSchema();
	v_sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetKey(v_sue|PERbyEMP, p_emp, p_nroleg);
	if (GetRecord(v_sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) 
		v_codcat = IFld(v_sue|PER_CODCAT);

	return v_codcat;
}

// ***********************************************************************************************************
// *                                      HoraEnRangoHorario    
// * Parametros:
// *	p_hora       : Hora a verificar
// *	p_ini_rango y 
// *   p_fin_rango  : Define el rango horario
// *
// * Funcionamiento : Verifica si la hora esta dentro del rango dado (TRUE caso contrario FALSE)
// *
// ***********************************************************************************************************
bool HoraEnRangoHorario(TIME p_hora, TIME p_ini_rango, TIME p_fin_rango)
{
	
	// Puesto dura todo el dia 
	if (p_ini_rango == p_fin_rango || (p_ini_rango == StrToT("00:00") && p_ini_rango == StrToT("23:59") ))
		return TRUE;

	// Puesto que entra y sale el mismo dia
	if (p_ini_rango < p_fin_rango){
		if (p_hora >= p_ini_rango && p_hora <= p_fin_rango)
			return TRUE;
	}

	else { //Puesto que entra un dia y sale al otro
		if (p_hora >= p_ini_rango || p_hora <= p_fin_rango)
			return TRUE;
	}

	return FALSE;	
}

// ***********************************************************************************************************
// *                                      RangoHorarioEnRangoHorario
// * Parametros:
// *	p_horini     : Hora a verificar
// *	p_horfin     : Hora a verificar
// *	p_ini_rango y 
// *   p_fin_rango  : Define el rango horario
// *
// * Funcionamiento : Verifica que un rango horario este incluido en otro (TRUE caso contrario FALSE)
// *
// ***********************************************************************************************************
bool RangoHorarioEnRangoHorario(TIME p_horini, TIME p_horfin, TIME p_ini_rango, TIME p_fin_rango)
{
	bool v_respuesta = TRUE;

    if (p_ini_rango == StrToT("23:59"))
    	p_ini_rango = StrToT("00:00");
    

	if (GetCantHoras(p_ini_rango, p_fin_rango)/100==_HORAS_POR_DIA){
		v_respuesta = TRUE;
	}
	else {
		// Valida Hora de inicio		
		if (!HoraEnRangoHorario(p_horini, p_ini_rango, p_fin_rango)){
			v_respuesta = FALSE;
		}
		else{
			// Valida Hora de fin		
			if (!HoraEnRangoHorario(p_horfin, p_ini_rango, p_fin_rango)){
				v_respuesta = FALSE;
			}
			else {
				// Valida casos raros
				
				// Puesto entra y sale el mismo dia, y asignacion entra y sale al otro dia
				if (p_ini_rango < p_fin_rango)
					if (p_horini >= p_horfin){
						v_respuesta = FALSE;
					}
				// Puesto que entra y sale al otro dia, y asignacion en el mismo dia
				if (p_ini_rango > p_fin_rango) 
					if (HoraEnRangoHorario(p_horini, StrToT("00:00:00"), p_fin_rango))
						if (HoraEnRangoHorario(p_horfin, p_ini_rango, StrToT("23:59:00"))){
							v_respuesta = FALSE;
						}
			}
		}
	}
	
	return v_respuesta;
}

// ***********************************************************************************************************
// *                                      SuperposicionRangoHorario
// * Parametros:
// *	p_horini     : Hora a verificar
// *	p_horfin     : Hora a verificar
// *	p_ini_rango y 
// *    p_fin_rango  : Define el rango horario
// *
// * Funcionamiento : Verifica que un rango horario se superponga con otro por lo menos en algun momento
// *                         (TRUE caso contrario FALSE)
// *
// ***********************************************************************************************************
bool SuperposicionRangoHorario(TIME p_horini, TIME p_horfin, TIME p_ini_rango, TIME p_fin_rango)
{
	bool v_respuesta = FALSE;

    if (p_ini_rango == StrToT("23:59"))
    	p_ini_rango = StrToT("00:00");

    if (p_horini != p_fin_rango && p_horfin != p_ini_rango) {
		if (GetCantHoras(p_ini_rango, p_fin_rango)/100==_HORAS_POR_DIA){
			v_respuesta = TRUE;
		}
		else {
			// Valida Hora de inicio		
			if (HoraEnRangoHorario(p_horini, p_ini_rango, p_fin_rango)){
				v_respuesta = TRUE;
			}
			else{
				// Valida Hora de fin		
				if (HoraEnRangoHorario(p_horfin, p_ini_rango, p_fin_rango)){
					v_respuesta = TRUE;
				}
				else {
					if (HoraEnRangoHorario(p_ini_rango, p_horini, p_horfin) || HoraEnRangoHorario(p_fin_rango, p_horini, p_horfin) ){
						v_respuesta = TRUE;

					}
				}
			}
		}
    }
	return v_respuesta;
}

// ***********************************************************************************************************
// *                                      EsNocturnoRangoHorario
// * Parametros:
// *	p_horini     : Hora a verificar
// *	p_horfin     : Hora a verificar
// *
// * Funcionamiento : Verifica que un rango horario sea nocturno
// *
// ***********************************************************************************************************
bool EsNocturnoRangoHorario(TIME p_horini, TIME p_horfin)
{
    
	// Controlo si esta incluido el rango minimo nocturno al inicio del rango nocturno en el rango pedido
	if (RangoHorarioEnRangoHorario(StrToT(HOR_FIN_DIA), StrToT(HOR_FIN_DIA)+MIN_HS_NOCT*30, p_horini, p_horfin))
		return TRUE;

	// Controlo si esta incluido el rango minimo nocturno al final del rango nocturno en el rango pedido
	if (RangoHorarioEnRangoHorario(StrToT(HOR_INI_DIA), StrToT(HOR_INI_DIA)-MIN_HS_NOCT*30, p_horini, p_horfin))
		return TRUE;

	// Controlo si esta incluido el rango pedido dentro del rango nocturno
	if (RangoHorarioEnRangoHorario(p_horini, p_horfin, StrToT(HOR_FIN_DIA), StrToT(HOR_INI_DIA)))
		return TRUE;


	return FALSE;
}



// *********************************************************************************************************
// *                                         GetDeleg_FilXUsuario
// *
// * Parametros :
// *           p_emp    = empresa
// *           p_nroleg = Numero de Legajo
// *           p_fecha  = Fecha
// *
// * Devuelve   :
// *           Pasandole estos parametros devuelve en p_deleg y p_filial la delegacion y filial a la que 
// * correspondia en ese momento
// *
// *********************************************************************************************************
void GetDeleg_FilXUsuario(int p_emp, long p_nroleg, DATE p_fecha, char *p_deleg, char *p_filial)
{

	schema prev, com;

	long v_cliente  = NULL_LONG;
	int  v_objetivo = NULL_SHORT;

	prev = CurrentSchema();
	com = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(prev);


	GetCliObjEfectivo(p_emp, p_nroleg, p_fecha, &v_cliente, &v_objetivo);
	
	SetKey(com|OBJETIVObyCLIENTE, v_cliente, v_objetivo);
	if (GetRecord(com|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		sprintf(p_deleg,  "%s", SFld(com|OBJETIVO_DELEGA));
		sprintf(p_filial, "%s", SFld(com|OBJETIVO_FILIAL));
	}
	else {
		sprintf(p_deleg,  "%s", NULL_STR);
		sprintf(p_filial, "%s", NULL_STR);
	}

}


// ***********************************************************************************************************
// *                                      FechasSuperpuestas    
// * Parametros:
// *	p_fecini1     : Fecha de Inicio 1 MAX_DATE= fecha inicio abierta
// *	p_fecfin1     : Fecha de Inicio 1 MAX_DATE= fecha fin  abierta
// *	p_fecini2     : Fecha de Inicio 2 MAX_DATE= fecha inicio abierta
// *	p_fecfin2     : Fecha de Inicio 2 MAX_DATE= fecha fin  abierta
// *
// * Funcionamiento : Verifica que un rango de fechas este incluido en otro (TRUE caso contrario FALSE)
// *
// ***********************************************************************************************************
bool FechasSuperpuestas(DATE p_fecini1, DATE p_fecfin1, DATE p_fecini2, DATE p_fecfin2)
{
	bool v_respuesta = FALSE;
	//Fecha de inicio 1 en rango de fechas 2
	if (p_fecini1>=p_fecini2 && p_fecini1<=p_fecfin2 )
		v_respuesta=TRUE;

	//Fecha de fin 1 en rango de fechas 2
	if (p_fecfin1>=p_fecini2 && p_fecfin1<=p_fecfin2 )
		v_respuesta=TRUE;

	//en rango de fechas 2 incluido en rango de fechas 1
	if (p_fecini1<=p_fecini2 && p_fecfin1>=p_fecfin2 )
		v_respuesta=TRUE;

	return v_respuesta;
}
// *************************************************************************************************************
// *                                         ValidaXLegajo
// *
// * Parametros: 
// *              p_emp    =  Empresa
// *              p_nroleg =  Numero de Legajo
// *              p_fecha  =  Ademas de se un parametro de salida sirve para ingresar fecha desde si 
// *                          fuese necesario, sino deberia valer NULL_DATE
// *
// * Devuelve: 
// *             Verifica que para una empresa - vigilador no existan asignaciones efectivas con la misma fecha 
// *             desde. 
// *
// *             ValidaXLegajo() es verdadera si no existen fechas desde repetidas y falso si existe este error
// *
// *			   El parametro p_fecha devuelve cual es la fecha desde repetida, si no hay error es NULL_DATE
// *
// *************************************************************************************************************
bool ValidaXLegajo(int p_emp, long p_nroleg, DATE *p_fecha)
{
	dbcursor c_asig, c_asigh;
	schema ope, old;
	DATE p_fecha_desde = *p_fecha; 

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	*p_fecha=NULL_DATE;

	iniDFld = NULL;

	//Acumulo fechas de asigh


	c_asigh=CreateCursor(ope|ASIGHbyLEGFEC, IO_NOT_LOCK);
	SetCursorFrom(c_asigh, p_emp, p_nroleg, p_fecha_desde, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_asigh, p_emp, p_nroleg, MAX_DATE,      MAX_LONG,  MAX_SHORT);
	while(FetchCursor(c_asigh)!=ERROR) {
		if (*SFld(ope|ASIGH_EFECT) != 'E')
			continue;

		if (DFld(ope|ASIGH_FECALT) < p_fecha_desde)
			continue;

		//si todavia no tengo la fecha, la acumulo
		if (!ExisteDFld(iniDFld, DFld(ope|ASIGH_FECALT)))
			iniDFld = AcuDFld(iniDFld, &iniDFld, DFld(ope|ASIGH_FECALT));
		else{
			BorDFld(iniDFld);
			*p_fecha = DFld(ope|ASIGH_FECALT);

			return FALSE;
		}
		
	}
	DeleteCursor(c_asigh);

	c_asig=CreateCursor(ope|ASIGbyLEGFEC, IO_NOT_LOCK);
	SetCursorFrom(c_asig, p_emp, p_nroleg, p_fecha_desde,  NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_asig, p_emp, p_nroleg, MAX_DATE, MAX_LONG , MAX_SHORT);
	while(FetchCursor(c_asig)!=ERROR) {
		if (*SFld(ope|ASIG_EFECT) != 'E')
			continue;

		//si todavia no tengo la fecha, la acumulo
		if (!ExisteDFld(iniDFld, DFld(ope|ASIG_FECASIG)))
			iniDFld = AcuDFld(iniDFld, &iniDFld, DFld(ope|ASIG_FECASIG));
		else{
			BorDFld(iniDFld);
			*p_fecha = DFld(ope|ASIG_FECASIG);
			return FALSE;
		}
	} 
	DeleteCursor(c_asig);

	BorDFld(iniDFld);

	return TRUE;
}


// *****************************************************************************************************************************************
// *                                     AcuDFld(), ExisteDFld(), BorDFld()
// *
// * Estas 3 funciones sirver para generar una lista, acumular y ver si se repiten una sucecion de fechas
// *
// * Forma de uso: 
// * 
// *	[1mPrimero que nada se debe poner :[0m
// *     
// *       iniDFld = NULL;
// *
// *   [1mDespues dentro del bucle donde se leen las fechas y se acumulan ingresandolas en  <FECHA>:[0m
// *
// *    	iniDFld = AcuDFld(iniDFld, &iniDFld, <FECHA>); 
// *
// *	[1mCuando se termina de acumular todas las fecha se puede preguntar si esta un fecha con <FECHA_BUSCADA> de la siguiente manera : [0m
// *
// *		if (ExisteDFld(iniDFld, <FECHA_BUSCADA>)) {
// *	
// *		}
// *
// * [1m  Y por ultimo  hay que limpiar la lista para liberar la memoria con : [0m
// *
// *      BorDFld(iniDFld);
// *
// *****************************************************************************************************************************************

tnDFld AcuDFld(tnDFld nodop, tnDFld * nantp,DATE  p_fecha)
{
	tnDFld naux;

	if (nodop == NULL) {
		nodop = (tnDFld) malloc (sizeof(stnDFld));
		(*nodop).fecha = p_fecha;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).fecha == p_fecha) {
		}
		else {
			if ((*nodop).fecha < p_fecha)
				(*nodop).nsig = AcuDFld((*nodop).nsig, &(*nodop).nsig, p_fecha);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnDFld) malloc (sizeof(stnDFld));
				(*nodop).fecha = p_fecha;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

bool ExisteDFld(tnDFld nodop, DATE p_fecha)
{
	tnDFld nodo_aux;


	for (nodo_aux=nodop; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig) {
		if ((*nodo_aux).fecha==p_fecha)
			return TRUE;
	} 

	return FALSE;

}

void BorDFld(tnDFld nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorDFld((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}

// *************************************************************************************************************
// *                                         ValidaMalRangoFecha
// *
// * Parametros: 
// *              p_emp       =  Empresa
// *              p_nroleg    =  Numero de Legajo
// *              p_fecha_ini =  Fecha desde si fuese necesario, sino deberia valer NULL_DATE, ademas es 
// *                             parametro de entrada.
// *
// * Devuelve: 
// *             Verifica que para una empresa - vigilador no existan asignaciones fecha desde mayor que fecha
// *             hasta. 
// *			  
// *             en p_fecha_ini devuelve la fecha de inicio con problema
// *
// *************************************************************************************************************
bool ValidaMalRangoFecha(int p_emp, long p_nroleg, DATE *p_fecha_ini)
{
	dbcursor c_asig, c_asigh;
	schema ope, old;
	DATE p_fecha_desde = *p_fecha_ini; 

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	*p_fecha_ini=NULL_DATE;

	c_asigh=CreateCursor(ope|ASIGHbyLEGFEC, IO_NOT_LOCK);
	SetCursorFrom(c_asigh, p_emp, p_nroleg, p_fecha_desde, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_asigh, p_emp, p_nroleg, MAX_DATE,      MAX_LONG,  MAX_SHORT);
	while(FetchCursor(c_asigh)!=ERROR) {

		if (DFld(ope|ASIGH_FECALT) < p_fecha_desde)
			continue;

		if (IsNull(ope|ASIGH_FECBAJ))
			continue;

		if (DFld(ope|ASIGH_FECALT)>DFld(ope|ASIGH_FECBAJ)){
		 	*p_fecha_ini=DFld(ope|ASIGH_FECALT);
		 	return FALSE;
		}
		
	}
	DeleteCursor(c_asigh);

	c_asig=CreateCursor(ope|ASIGbyLEGFEC, IO_NOT_LOCK);
	SetCursorFrom(c_asig, p_emp, p_nroleg, p_fecha_desde,  NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_asig, p_emp, p_nroleg, MAX_DATE, MAX_LONG , MAX_SHORT);
	while(FetchCursor(c_asig)!=ERROR) {

		if (DFld(ope|ASIG_FECASIG) < p_fecha_desde)
			continue;

		if (IsNull(ope|ASIG_FECHAS))
			continue;

		if (DFld(ope|ASIG_FECASIG)>DFld(ope|ASIG_FECHAS)){
		 	*p_fecha_ini=DFld(ope|ASIG_FECASIG);
		 	return FALSE;
		}
	} 
	DeleteCursor(c_asig);

	return TRUE;
	
}

DATE FechaInicioUltAsigEfec(int emp, long nroleg)
{
	schema	 old, operac;
	dbtable	 AASIG, AASIGH;
	dbcursor c_asig, c_asigh;
	DATE 	 v_fecha=NULL_DATE;
	bool 	 encontre=FALSE;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	AASIG	= CreateAlias(operac|ASIG);
	c_asig	= CreateCursor(AASIGbyLEGFEC,  IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, NULL_DATE, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_DATE, MAX_LONG, MAX_SHORT);
	MoveCursorLast(c_asig);
	while (FetchCursorPrev(c_asig) != ERROR) {
		if (strcmp(SFld(AASIG_EFECT), EFECTIVO) != 0) 
			continue;


		v_fecha=DFld(AASIG_FECASIG);
		encontre=TRUE;
		break;
	}
 	DeleteCursor(c_asig);
	DeleteAlias(AASIG);

	if (!encontre) {
		AASIGH	= CreateAlias(operac|ASIGH);
		c_asigh	= CreateCursor(AASIGHbyNROLEG, IO_NOT_LOCK);

		SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {

			if (strcmp(SFld(AASIGH_EFECT), EFECTIVO) != 0)
				continue;

			if (v_fecha < DFld(AASIGH_FECALT)) 
				v_fecha = DFld(AASIGH_FECALT);

		}
		DeleteCursor(c_asigh);
		DeleteAlias(AASIGH);

	}
	SwitchToSchema(old);
	return v_fecha;
}

bool EstaEnAsig(int p_emp, long  p_cliente, int p_objetivo, long p_nroleg, int  p_ptoser, int  p_puesto, int  p_nroint) 
{
	schema	 old, operac;
	dbtable	 AASIG;
	bool v_encontro=FALSE;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);


	AASIG	= CreateAlias(operac|ASIG);
	SetKey(AlInd(AASIG, ASIGbyEMP), p_emp, p_cliente, p_objetivo, p_nroleg, p_ptoser, p_puesto, p_nroint);
	if(GetRecord(AlInd(AASIG, ASIGbyEMP), THIS_KEY, IO_NOT_LOCK) != ERROR) 
		v_encontro=TRUE;

	return v_encontro;
	
}

// *********************************************************************************************************
// *                                      GetObjetivoBaja
// * Parametros:
// *	int p_emp_o       = Empresa 
// *	long p_cliente_o  = Cliente Origen
// *	int p_objet_o     = Objetivo Origen
// *	char *p_regim     = Regimen de la asignacion 
// *	long *p_cliente_b = Cliente Destino (parametro de retorno)
// *	int * p_objet_b   = Objetivo Destino (parametro de retorno)
// *	int * p_tippto    = Tipo de Puesto Destino(parametro de retorno)
// *	int * p_puesto    = Nro. de Puesto Destino(parametro de retorno)
// *
// * Funcionamiento : Se le pasa empresa, cliente y objetivo y devuelve el cliente objetivo de baja que le 
// *                  corresponde
// *
// *********************************************************************************************************
bool GetObjetivoBaja(int p_emp_o, long p_cliente_o, int p_objet_o, char *p_regim, long *p_cliente_b, int * p_objet_b, int * p_tippto_b, int * p_puesto_b)
{
	schema	prev, billpro, comerc, operac;
	bool	v_encuentra = FALSE, 
			v_partime   = FALSE;

	short	v_pais_o= NULL_SHORT, 
			v_prov_o= NULL_SHORT;

	dbcursor c_puestos;

	prev    = CurrentSchema();
	billpro = OpenSchema("billpro", IO_EABORT);
	comerc  = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(prev);

	*p_cliente_b=NULL_LONG;
 	*p_objet_b  =NULL_SHORT;

	v_pais_o = v_prov_o = NULL_SHORT;			


	//Obtengo Pais y v_prov_oincia de objetivo origen
	SetKey(comerc|OBJETIVObyCLIENTE, p_cliente_o, p_objet_o);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK)!=ERROR){
		v_pais_o = IFld(comerc|OBJETIVO_PAIS);
		v_prov_o = IFld(comerc|OBJETIVO_PROV);			
	}


	//Busco cliente Objetivo de baja
	SetKey(billpro|RCLIESPbyTIPCLI, GRPBAJA, NULL_LONG, NULL_SHORT);
	while(GetRecord(billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR && !v_encuentra) {

		SetKey(comerc|OBJETIVO, LFld(billpro|RCLIESP_CLIENTE), IFld(billpro|RCLIESP_OBJET));
		if (IsNull(billpro|RCLIESP_OBJET)) {

			while (GetRecord(comerc|OBJETIVO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1)!=ERROR) {

			 	if (p_emp_o==IFld(comerc|OBJETIVO_EMP) && v_pais_o == IFld(comerc|OBJETIVO_PAIS) && v_prov_o == IFld(comerc|OBJETIVO_PROV)) {
			 		*p_cliente_b=LFld(comerc|OBJETIVO_CLIENTE);
			 		*p_objet_b  =IFld(comerc|OBJETIVO_OBJET);

					v_encuentra = TRUE;
					break;
			 	}
			}
		}
		else {
			if (GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
	
			 	if (p_emp_o == IFld(comerc|OBJETIVO_EMP) && v_pais_o == IFld(comerc|OBJETIVO_PAIS) && v_prov_o == IFld(comerc|OBJETIVO_PROV)) {
			 		*p_cliente_b=LFld(comerc|OBJETIVO_CLIENTE);
			 		*p_objet_b  =IFld(comerc|OBJETIVO_OBJET);
					v_encuentra = TRUE;
			 	}
			}
		}
	}


	if (v_encuentra) {
		// Me fijo si es partime el regimen de la asignacion
		SetFld (comerc|REGIMEN_REGIM,   p_regim);
		if (GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR)
			v_partime=IFld(comerc|REGIMEN_PARTIME);

		*p_tippto_b=NULL_SHORT;
		*p_puesto_b=NULL_SHORT;

		// Busco un puesto para esta provincia, que sea o no partime deacuerdo a la asignacion
		c_puestos=CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);
		SetCursorFrom(c_puestos, *p_cliente_b, *p_objet_b, NULL_SHORT, NULL_SHORT);
		SetCursorTo  (c_puestos, *p_cliente_b, *p_objet_b, MAX_SHORT,  MAX_SHORT);
		while(FetchCursor(c_puestos)!=ERROR) {

			SetFld (comerc|REGIMEN_REGIM,   SFld(operac|PUESTOS_REGIM));
			if (GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				if (v_partime==IFld(comerc|REGIMEN_PARTIME)) {
					*p_tippto_b=IFld(operac|PUESTOS_TIPPTO);
					*p_puesto_b=IFld(operac|PUESTOS_CODINT);
					break;
				}
			}

		}

		DeleteCursor(c_puestos);
	}	
	return v_encuentra;
}



// *** Devuelve TRUE si el vigilador esta asignado en forma PROVISORIA a un puesto. ***
///-------------------------* TieneProvisorioVigente *-----------------------
bool TieneProvisorioVigente(int emp, long legajo, DATE fecha)
{
	schema old, operac;
	bool provisorio = FALSE;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);
                                     
//WiMsg("%D", fecha);
	SetKey(ASIGbyLEGFEC, emp, legajo, NULL_DATE, NULL_LONG, NULL_SHORT);
	while (GetRecord(ASIGbyLEGFEC, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (!strcmp(SFld(ASIG_EFECT), EFECTIVO)) 
			continue;


		if(fecha <= DFld(ASIG_FECHAS)){
//WiMsg("A %D %d %d %d %D", DFld(ASIG_FECHAS), LFld(ASIG_NROLEG), LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO), DFld(ASIG_FECASIG));
			provisorio=TRUE;
			break;
		}
	}
	if(!provisorio) {
		SetKey(ASIGHbyLEGFEC, emp, legajo, fecha, NULL_LONG, NULL_SHORT);
		while (GetRecord(ASIGHbyLEGFEC, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			if (!strcmp(SFld(ASIGH_EFECT), EFECTIVO)) 
				continue;

			if (fecha >= DFld(ASIGH_FECALT) && fecha <= DFld(ASIGH_FECBAJ)) {
				provisorio=TRUE;
//WiMsg("B %D", DFld(ASIGH_FECHAS));
				break;
			}

		}
	}
	SwitchToSchema(old);
	return provisorio;
}

// ***************************************************************************************************************************************
// *                                        RecalculaParte()
// *
// * Borra todas las horas de un legajo para un rango de fechas, lo vuelve a calcular y lo graba en el PARTE
// *
// * Parametros :
// *       p_emp 	    = Empresa
// *		p_nroleg	= Numero de legajo
// *		p_fechad	= Fecha de comienzo del rango a recalcular
// *		p_fechah	= Fecha de fin del rango a recalcular
// *		p_fm		= Si se llama desde un programa con form y se queiere mostrar el comentario de lo que esta procesando, aca va el 
// *					  descriptor, sino va NULL_SHORT
// *		p_coment	= Campo donde iria la descripcion de lo que se procesa si p_fm no es NULL_SHORT
// *
// *
// ***************************************************************************************************************************************
void RecalculaParte(int p_emp, long p_nroleg, DATE p_fechad, DATE p_fechah, form p_fm, fmfield p_coment) 
{
	int hsnor=0,
		hs25=0,
		hs35=0,
		hsfra=0;

	char v_aux[60], v_condic='\0';

	dbcursor c_parte;
	schema old, operac;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	// Limpia Horas del Parte
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_fechad, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_fechah, MAX_LONG,  MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		hsnor = hs25 = hs35 = hsfra = 0.0;

		SetIFld(operac|PARTE_HSNOR, hsnor);
		SetIFld(operac|PARTE_HS50,  hs25);
		SetIFld(operac|PARTE_HS100F,hs35);
		SetIFld(operac|PARTE_HS100FE,hsfra);

		if (p_fm!=NULL_SHORT){
			sprintf(v_aux, "Borrando Horas Legajo %ld para el dia %.3D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
			FmSetFld(p_fm, p_coment, v_aux);
			WiRefresh();
		}

		PutRecord(operac|PARTE);

	}
	DeleteCursor(c_parte);


	// Recalcula Horas del Parte
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_fechad, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_fechah, MAX_LONG,  MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		hsnor = hs25 = hs35 = hsfra = 0.0;

		// Si hay otra condicion Grabada Toma Esa y la Graba en el parte Generado
		v_condic= CondicParteOtroObjetivo(p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG));
		if (v_condic!='\0' && v_condic!=*SFld(operac|PARTE_CONDIC)) {
			sprintf(v_aux, "%c", v_condic);
			SetFld(operac|PARTE_CONDIC, v_aux);
			PutRecord(operac|PARTE);
		}
			
		CalDetHorPer2(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
				                       IFld(operac|PARTE_NROINT), *SFld(operac|PARTE_CONDIC), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), 
				                       &hsnor, &hs25, &hs35, &hsfra);

//		CalculoDetalleHorasPer(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
//				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
//				                       IFld(operac|PARTE_NROINT), &hsnor, &hs25, &hs35, &hsfra, &hspeg, &hsgua);

		
		SetIFld(operac|PARTE_HSNOR, hsnor);
		SetIFld(operac|PARTE_HS50,  hs25);
		SetIFld(operac|PARTE_HS100F,hs35);
		SetIFld(operac|PARTE_HS100FE,hsfra);

		//fprintf(stderr, "\t\t\t%d\t%ld\t%d\t%.3D\n", IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA));

		if (p_fm!=NULL_SHORT){
			sprintf(v_aux, "Modificando Legajo %ld para el dia %.3D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
			FmSetFld(p_fm, p_coment, v_aux);
			WiRefresh();
		}
		PutRecord(operac|PARTE);

	}
	DeleteCursor(c_parte);
	if (p_fm!=NULL_SHORT){
		FmSetFld(p_fm, p_coment, "");
		WiRefresh();
	}
}

double CantidadHorasAsignadas(int p_emp, long p_nroleg, DATE p_dia) 
{
	dbtable	AASIG, AASIGH;
	schema old, operac;
	double tothoras=0;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIG  = CreateAlias(operac|ASIG);
	AASIGH = CreateAlias(operac|ASIGH);


	SetIFld(AASIG_EMP,      p_emp);
	SetLFld(AASIG_NROLEG,   p_nroleg);
	SetLFld(AASIG_CLIENTE,  NULL_LONG);
	SetIFld(AASIG_OBJETIVO, NULL_SHORT);
	while (GetRecord(AASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (p_dia<DFld(AASIG_FECASIG))
			continue;

		if (!IsNull(AASIG_FECHAS) && p_dia>DFld(AASIG_FECHAS))
			continue;


		if (!SeTrabEnPuesto(DiaLetra(p_dia), SFld(AASIG_DIA1), SFld(AASIG_DIA2), SFld(AASIG_DIA3), 
		                    SFld(AASIG_DIA4), SFld(AASIG_DIA5), SFld(AASIG_DIA6), SFld(AASIG_DIA7)))
			continue;

//		fprintf(stderr, "OPERAC.c   ASIG %d-%ld-%ld-%d-%.3D-%.3D-%.3T-%.3T\n", IFld(AASIG_EMP), LFld(AASIG_NROLEG), LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO), DFld(AASIG_FECASIG), DFld(AASIG_FECHAS), TFld(operac|ASIG_HSENT), TFld(operac|ASIG_HSSAL));

		tothoras +=ConvHraInt(TFld(AASIG_HSENT), TFld(AASIG_HSSAL)); 
	}

	SetIFld(AASIGH_EMP,      p_emp);
	SetLFld(AASIGH_NROLEG,   p_nroleg);
	SetDFld(AASIGH_FECBAJ,   p_dia);
	SetLFld(AASIGH_CLIENTE,  NULL_LONG);
	SetIFld(AASIGH_OBJETIVO, NULL_SHORT);
	while (GetRecord(AASIGHbyLEGFEC, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {

		if (DFld(AASIGH_FECALT) > p_dia)
			continue;

		if (DFld(AASIGH_FECBAJ) < p_dia)
			continue;
        
        if(IFld(AASIGH_MOTIVO) == 80) //Para no comparar un registro insertado
        	continue;
        
		if (!SeTrabEnPuesto(DiaLetra(p_dia), SFld(AASIGH_DIA1), SFld(AASIGH_DIA2), SFld(AASIGH_DIA3), 
		                    SFld(AASIGH_DIA4), SFld(AASIGH_DIA5), SFld(AASIGH_DIA6), SFld(AASIGH_DIA7)))
			continue;

//		fprintf(stderr, "OPERAC.c   ASIGH %d-%ld-%ld-%d-%.3D-%.3D-%.3T-%.3T\n", IFld(AASIGH_EMP), LFld(AASIGH_NROLEG), LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO), DFld(AASIGH_FECALT), DFld(AASIGH_FECBAJ), TFld(operac|ASIGH_HSENT), TFld(operac|ASIGH_HSSAL));

		tothoras +=ConvHraInt(TFld(AASIGH_HSENT), TFld(AASIGH_HSSAL) ); 

	}  
	DeleteAlias(AASIG);
	DeleteAlias(AASIGH);
	SwitchToSchema(old);

	return (tothoras);
	
}

bool PuestoVigente(DATE p_dia, long cliente, int objet, int tippto, int codint)
{
	DATE fecini, fecfin;
    bool devuelve;
    
	fecini = FechaInicioPuesto(cliente, objet, tippto, codint);
	fecfin = FechaFinPuesto(cliente, objet, tippto, codint);

   	devuelve=TRUE;
   
   	if (p_dia < fecini)
		devuelve=FALSE;
	
	if(fecfin != NULL_DATE && p_dia>fecfin )
		devuelve=FALSE;

	return devuelve;
}

DATE GetEgresoLegajo(int p_emp, long p_nroleg)
{
	schema sue, old;

	old = CurrentSchema();
	sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetKey(sue|PERbyEMP, p_emp, p_nroleg);
	if(GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		return DFld(sue|PER_FECEGR);
	}
	return NULL_DATE;
}

//-------------------------* EsInsertado *-----------------------
bool EsInsertado(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint)
{
	schema old, operac;
	dbcursor 	c_asigh;
    bool enasigh = FALSE;

	old = CurrentSchema();

	operac = OpenSchema("operac", IO_EABORT);

	SwitchToSchema(operac);

	enasigh = FALSE;

	c_asigh=CreateCursor(operac|ASIGHbyFECHABAJ, IO_NOT_LOCK);
	SetCursorFrom(c_asigh, emp, cliente, objetivo, fecparte, legajo, ptoser, puesto, nroint);
	SetCursorTo  (c_asigh, emp, cliente, objetivo, fecparte, legajo, ptoser, puesto, nroint);
	while(FetchCursor(c_asigh)!=ERROR) {

		if (IFld(ASIGH_MOTIVO) != ALTAPARTE)
			continue;

		if (fecparte != DFld(ASIGH_FECALT))
			continue;

		enasigh = TRUE;

	}
	DeleteCursor(c_asigh);

	SwitchToSchema(old);
	return enasigh;
}

// ***************************************************************************************************************************************
// *                                        RecalculaPartePer()
// *
// * Borra todas las horas de un legajo para un rango de fechas, lo vuelve a calcular y lo graba en el PARTE
// *
// * Parametros :
// *       p_emp 	    = Empresa
// *		p_nroleg	= Numero de legajo
// *		p_fechad	= Fecha de comienzo del rango a recalcular
// *		p_fechah	= Fecha de fin del rango a recalcular
// *		p_fm		= Si se llama desde un programa con form y se queiere mostrar el comentario de lo que esta procesando, aca va el 
// *					  descriptor, sino va NULL_SHORT
// *		p_coment	= Campo donde iria la descripcion de lo que se procesa si p_fm no es NULL_SHORT
// *
// *
// ***************************************************************************************************************************************
void RecalculaPartePer(int p_emp, long p_nroleg, DATE p_fechad, DATE p_fechah, form p_fm, fmfield p_coment, char * p_prog) 
{
	int hsnor=0,
		hs25=0,
		hs35=0,
		hsfra=0;

	char v_aux[60], v_condic='\0';
	char v_auxtpto[6];

	dbcursor c_parte;
	schema old, operac;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	// Limpia Horas del Parte
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_fechad, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_fechah, MAX_LONG,  MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		hsnor = hs25 = hs35 = hsfra = 0.0;

		SetIFld(operac|PARTE_HSNOR, hsnor);
		SetIFld(operac|PARTE_HS50,  hs25);
		SetIFld(operac|PARTE_HS100F,hs35);
		SetIFld(operac|PARTE_HS100FE,hsfra);

		if (p_fm!=NULL_SHORT){
			sprintf(v_aux, "Borrando Horas Legajo %ld para el dia %.3D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
			FmSetFld(p_fm, p_coment, v_aux);
			WiRefresh();
		}

		AudiGrabaHorasParte(operac, p_prog);
		PutRecord(operac|PARTE);

	}
	DeleteCursor(c_parte);

	// Recalcula Horas del Parte
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_fechad, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_fechah, MAX_LONG,  MAX_SHORT);

	// Proceso No Guardianias 
	while (FetchCursor(c_parte) != ERROR) {

		hsnor = hs25 = hs35 = hsfra = 0.0;

//		sprintf(v_auxtpto, "%d", IFld(operac|PARTE_PTOSER));
//		if (EsParNov(p_emp, PARNOV_TPUHSNOR, DFld(operac|PARTE_DIA), v_auxtpto))
//			continue;

		/* Si hay otra condicion Grabada Toma Esa y la Graba en el parte Generado
		v_condic= CondicParteOtroObjetivo(p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG));
		if (v_condic!='\0' && v_condic!=*SFld(operac|PARTE_CONDIC)) {
			sprintf(v_aux, "%c", v_condic);
			SetFld(operac|PARTE_CONDIC, v_aux);

			PutRecord(operac|PARTE);
		}
		*/

//		fprintf(stderr, "A %d\t%ld\t%d\t%d\t%.3D %d %d %d %d %T %T\n", IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), hsnor,hs25,hs35,hsfra, TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL));

		CalDetHorPer2(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
				                       IFld(operac|PARTE_NROINT), *SFld(operac|PARTE_CONDIC), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), 
				                       &hsnor, &hs25, &hs35, &hsfra);

//		CalculoDetalleHorasPer(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
//				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
//				                       IFld(operac|PARTE_NROINT), &hsnor, &hs25, &hs35, &hsfra, &hspeg, &hsgua);
		
		SetIFld(operac|PARTE_HSNOR, hsnor);
		SetIFld(operac|PARTE_HS50,  hs25);
		SetIFld(operac|PARTE_HS100F,hs35);
		SetIFld(operac|PARTE_HS100FE,hsfra);

//		fprintf(stderr, "B %d\t%ld\t%d\t%d\t%.3D %d %d %d %d %T %T\n", IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), hsnor,hs25,hs35,hsfra, TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL));

		if (p_fm!=NULL_SHORT){
			sprintf(v_aux, "Modificando Legajo %ld para el dia %.3D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
			FmSetFld(p_fm, p_coment, v_aux);
			WiRefresh();
		}
		AudiGrabaHorasParte(operac, p_prog);
		PutRecord(operac|PARTE);

	}

	MoveCursorFirst(c_parte);


/*	// Proceso Guardianias 
	while (FetchCursor(c_parte) != ERROR) {

		hsnor = hs25 = hs35 = hsfra = 0.0;

		sprintf(v_auxtpto, "%d", IFld(operac|PARTE_PTOSER));
		if (!EsParNov(p_emp, PARNOV_TPUHSNOR, DFld(operac|PARTE_DIA), v_auxtpto))
			continue;

		// Si hay otra condicion Grabada Toma Esa y la Graba en el parte Generado
		v_condic= CondicParteOtroObjetivo(p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG));
		if (v_condic!='\0' && v_condic!=*SFld(operac|PARTE_CONDIC)) {
			sprintf(v_aux, "%c", v_condic);
			SetFld(operac|PARTE_CONDIC, v_aux);

			PutRecord(operac|PARTE);
		}

//		CalculoDetalleHorasPer(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
//				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
//				                       IFld(operac|PARTE_NROINT), &hsnor, &hs25, &hs35, &hsfra, &hspeg, &hsgua);

		CalDetHorPer2(FALSE, NULL_SHORT, NULL_SHORT, p_emp, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), 
				                       DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG), IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
				                       IFld(operac|PARTE_NROINT), *SFld(operac|PARTE_CONDIC), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), 
				                       &hsnor, &hs25, &hs35, &hsfra);
		
		SetIFld(operac|PARTE_HSNOR, hsnor);
		SetIFld(operac|PARTE_HS50,  hs25);
		SetIFld(operac|PARTE_HS100F,hs35);
		SetIFld(operac|PARTE_HS100FE,hsfra);

//		fprintf(stderr, A "\t\t\t%d\t%ld\t%d\t%d\t%.3D %d %d %d %d \n", IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), hsnor,hs25,hs35,hsfra);

		if (p_fm!=NULL_SHORT){
			sprintf(v_aux, "Modificando Legajo %ld para el dia %.3D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
			FmSetFld(p_fm, p_coment, v_aux);
			WiRefresh();
		}
		AudiGrabaHorasParte(operac, p_prog);
		PutRecord(operac|PARTE);

	}
*/
	DeleteCursor(c_parte);
	if (p_fm!=NULL_SHORT){
		FmSetFld(p_fm, p_coment, "");
		WiRefresh();
	}
}

// ***************************************************************************************************************************************
// *                                        CalDetHorPer2()
// *
// * Calcula las horas que hay que grabar en un registro del parte teniendo en cuenta lo que hay grabado en otros registros del mismo dia 
// *
// * Parametros :
// *		 p_usafm    = es verdadero que tiene que contar horas del FM y falso si no
// *         p_fm  		= Descriptor del FM si el primer paramentro es verdadero, sino es NULL_SHORT
// *		 p_row      = Numero de linea del FM  si el primer paramentro es verdadero, sino es NULL_SHORT
// *
// *         p_emp 		= empresa 
// *         p_cliente	= cliente 
// *		 p_objetivo	= objetivo
// *		 p_dia		= fecha 
// *		 p_nroleg	= numero de legajo 
// * 		 p_ptoser 	= Puesto
// *		 p_puesto	= Puesto
// *		 p_nroint	= Puesto
// *
// * 
// * Devuelve	: 
// *		nor		  = Horas Normales
// * 		a25       = Horas al 25%
// *		a35       = Horas al 35%
// *		fra       = Horas Franco
// *
// ***************************************************************************************************************************************
void CalDetHorPer2(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,  DATE p_dia , long p_nroleg, 
                            int p_ptoser, int p_puesto, int p_nroint, char p_condic, TIME p_entrada, TIME p_salida, int *nor, int *a25, int *a35, int *fra)
{
	schema operac, old;

	dbcursor c_parte;

	int tot_horas=0,
		objefe=0,
		total_horas_linea=0;


	char v_regimen[12];
	double v_horas_regimen=0;

	int tipodia=0,
		v_pais=0,
		v_prov=0;

	int	tot_nor=0,
		tot_a25=0,
		tot_a35=0, 
		tot_fra=0,
		tot_peg=0;

	int	otr_nor=0,
		otr_a25=0,
		otr_a35=0,
		otr_fra=0;

	double	fm_otr_nor=0,
			fm_otr_a25=0,
			fm_otr_a35=0,
			fm_otr_fra=0;

	bool otras_horas=FALSE;

	long cliefe=0;

	char v_auxtpto[6];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

 	*nor = *a25 = *a35 = *fra = 0;

	if (p_entrada == StrToT("00:00") && p_salida ==  StrToT("00:00")) {
		return;
	}

	total_horas_linea = ConvHraInt(p_entrada, p_salida) * 100;

//	fprintf(stderr, "\n\nLegajo %d Condicion %c (%T) (%T) %d\n", p_nroleg, p_condic, p_entrada, p_salida, total_horas_linea);

	//------------------- Si son Pegadas son todas al 35% ----------------------------------------------//

	if (p_condic == _PEGADA_C) {
		*a35 = total_horas_linea;
//		fprintf (stderr, "Pegada a35 %d \n", *a35);
		return;
	}

	if (p_condic == _FRANCO_C) {
		*fra = total_horas_linea;
//		fprintf (stderr, "Franco %d \n", *fra);
		return;
	}

	//------------------- Si es Guardiania todas las horas son normales --------------------------------//

	sprintf(v_auxtpto, "%d", p_ptoser);
	if (EsParNov(p_emp, PARNOV_TPUHSNOR, p_dia, v_auxtpto)) {
//		if (total_horas_linea > _HORAS_REGIMEN_DEFAULT*100) {
//			*nor = _HORAS_REGIMEN_DEFAULT*100;
//		}
//		else {
			*nor = total_horas_linea;
//		}
//		fprintf (stderr, "Guardiania nor %d \n", *nor);
	}
	else {

	//------------------ Calculo cantidad de horas en otros puestos por tipo de hora  -----------------//

		tot_horas = 0;

		PushRecord(operac|PARTE);

		c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);

		SetCursorFrom(c_parte, p_emp, p_nroleg, p_dia, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_parte, p_emp, p_nroleg, p_dia, MAX_LONG, MAX_SHORT);

		while (FetchCursor(c_parte) != ERROR) {

			if (p_condic != *SFld(operac|PARTE_CONDIC))  // Solo sumo las que tienen la misma condicion.
				continue;

			tot_nor += IFld(operac|PARTE_HSNOR);
			tot_a25 += IFld(operac|PARTE_HS50);
			tot_a35 += IFld(operac|PARTE_HS100F);
			tot_fra += IFld(operac|PARTE_HS100FE);

			// si es el mismo puesto no lo tomo
			if (LFld(operac|PARTE_CLIENTE)  == p_cliente && 
			    IFld(operac|PARTE_OBJETIVO) == p_objetivo && 
			    IFld(operac|PARTE_PTOSER)   == p_ptoser &&
			    IFld(operac|PARTE_PUESTO)   == p_puesto &&
			    IFld(operac|PARTE_NROINT)   == p_nroint) {
				    continue;
		    }

			// si usa fm lo de este cliente/objetivo lo suma de la pantalla

			if (p_usafm &&
				LFld(operac|PARTE_CLIENTE)  == p_cliente && 
			    IFld(operac|PARTE_OBJETIVO) == p_objetivo)
			    continue;

			tot_horas += ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;	

			otr_nor += IFld(operac|PARTE_HSNOR);
			otr_a25 += IFld(operac|PARTE_HS50);
			otr_a35 += IFld(operac|PARTE_HS100F);
			otr_fra += IFld(operac|PARTE_HS100FE);

		}    	
//		fprintf (stderr, "A TotalHoras tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d\n", tot_nor, tot_a25, tot_a35, tot_fra);
//		fprintf (stderr, "A OtrasHoras otr_nor %d otr_a25 %d otr_a35 %d otr_fra %d\n", otr_nor, otr_a25, otr_a35, otr_fra);

		/* Confirmacion */

		if (!p_usafm)
			if (tot_horas == 0)
				tot_horas = total_horas_linea;
			else
				tot_horas += total_horas_linea;

		DeleteCursor(c_parte);

		if (p_usafm) {
			fm_otr_nor = fm_otr_a25 = fm_otr_a35 = fm_otr_fra = 0;

			CalHorMisCli2(p_fm, p_row, &fm_otr_nor, &fm_otr_a25, &fm_otr_fra, &fm_otr_a35);

//			fprintf (stderr, "CalHorMisCli fm_otr_nor %.2f fm_otr_a25 %.2f fm_otr_a35 %.2f fm_otr_fra %.2f\n", fm_otr_nor, fm_otr_a25, fm_otr_a35, fm_otr_fra);

			otr_nor   += ((int)fm_otr_nor)*100;
			otr_a25   += ((int)fm_otr_a25)*100;
			otr_a35   += ((int)fm_otr_a35)*100;
			otr_fra   += ((int)fm_otr_fra)*100;

			tot_horas  += ((int)fm_otr_nor)*100+((int)fm_otr_a25)*100+((int)fm_otr_a35)*100+((int)fm_otr_fra)*100;
			tot_horas  += ConvHraInt(FmTFld(p_fm, HSENTRE, p_row), FmTFld(p_fm, HSSAL, p_row)) * 100;
		}

//		fprintf (stderr, "B TotalHoras tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d\n", tot_nor, tot_a25, tot_a35, tot_fra);
//		fprintf (stderr, "B OtrasHoras otr_nor %d otr_a25 %d otr_a35 %d otr_fra %d\n", otr_nor, otr_a25, otr_a35, otr_fra);

		if ( (otr_nor + otr_a25 + otr_a35 + otr_fra) > 0)
			otras_horas = TRUE;
		else
			otras_horas = FALSE;

//		fprintf (stderr, "Total horas %d\n", tot_horas);

	//----------------------------------------------------Determino tipo de dia-----------------------------------------------------------//

		GetCliObjEfectivo(p_emp, p_nroleg, p_dia, &cliefe, &objefe);
		GetRegimenEfectivo(p_emp, p_nroleg, v_regimen, p_dia);
		v_horas_regimen = (double)GetHsRegimen(v_regimen);

		v_pais = GetCliePais(cliefe, objefe);
		v_prov = GetClieProv(cliefe, objefe);

		if (FeriadoNovia(p_dia, v_pais, v_prov))
			tipodia = _DIA_FERIADO;
		else
			tipodia = _DIA_NORMAL;

//		fprintf (stderr, "Tipodia  %d\n", tipodia);

	//------------------------------------Deacuerdo al tipo de dia distribuyo total de horas por dia -------------------------------------//
		
		DisHorPer2(p_emp, tot_horas, tipodia, v_horas_regimen , &tot_nor, &tot_a25,  &tot_a35, &tot_fra);

	//------------------------------------------Descuento si hubiera las horas en otros puestos ------------------------------------------//

//		fprintf (stderr, "C TotalHoras tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d\n", tot_nor, tot_a25, tot_a35, tot_fra);
//		fprintf (stderr, "C OtrasHoras otr_nor %d otr_a25 %d otr_a35 %d otr_fra %d\n", otr_nor, otr_a25, otr_a35, otr_fra);

//		if (tipodia == _DIA_FRANCO || tipodia == _DIA_PEGADA) {
//			otr_nor = otr_a25 = otr_a35 =  otr_fra = 0;
//		}

		tot_nor-=otr_nor;
	   	tot_a25-=otr_a25;
	   	tot_fra-=otr_fra;

		if (otr_a35 >= tot_peg)
			tot_peg=0;
		else
			tot_a35=0;

//		fprintf (stderr, "CalDetHorPer2 Para Puesto NOR =%d\tH25 =%d\tH35 =%d\tFRA =%d\tPEG=%d\n", tot_nor/100, tot_a25/100, tot_a35/100, tot_fra/100, tot_peg/100);
//		fprintf (stderr, "CalDetHorPer2 Horas a descontar total_horas_linea %d tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d tot_peg %d\n", 
//		                  total_horas_linea, tot_nor, tot_a25, tot_a35, tot_fra, tot_peg);


		//----------------------Controlo que lo que devuelvo sea coherente con la cantidad total de horas del puesto--------------------------//
		*nor=AsignoHoras(tot_nor, &total_horas_linea);
		*a25=AsignoHoras(tot_a25, &total_horas_linea);
		*a35=AsignoHoras(tot_a35, &total_horas_linea);
		*fra=AsignoHoras(tot_fra, &total_horas_linea);


		PopRecord(operac|PARTE);
    }
}

// ***************************************************************************************************************************************
// *                                        DisHorPer2()
// *
// * Dado un total de horas y el tipo de dia (normal, franco o feriado) distribuye cuantas hora corresponden a cada tipo de hora 
// *
// * Parametros :
// *       p_emp        = Empresa
// *       p_tothoras 	= Total de horas a distribuir
// *       p_tipodia	= Tipo de dia 
// * 									_DIA_FRANCO		   
// * 									_DIA_FERIADO
// * 									_DIA_NORMAL
// * Devuelve	: 
// *		nor			= Horas Normales
// * 		a25         = Horas al 25%
// *		a35         = Horas al 35%
// *		fra         = Horas Franco
// *
// *
// ***************************************************************************************************************************************
void DisHorPer2(int p_emp, int p_tothoras, int p_tipodia, int p_horas_regimen, int *nor, int *a25, int *a35, int *fra)

{
	int	tot_nor=0,
		tot_25=0,
		tot_35=0,
		tot_fra=0;

	int tot_horas=0, v_horas_regimen = p_horas_regimen;


	tot_horas = p_tothoras;

 	if (v_horas_regimen == 0) {
		v_horas_regimen = _HORAS_REGIMEN_DEFAULT*100;
	}

// 	fprintf (stderr, "DisHorPer2: tot_horas= %d \n", tot_horas);
//	fprintf (stderr, "DisHorPer2: p_tothoras %d  \n", p_tothoras);
//	fprintf (stderr, "DisHorPer2: horas_regimen= %d \n", v_horas_regimen);
//	fprintf (stderr, "DisHorPer2: min_hs_PEGADAS= %d\n", _MIN_HS_PEGADAS);

	switch(p_tipodia) {
		case _DIA_FERIADO :
		case _DIA_NORMAL :

			// Calculo Horas Normales
			if (tot_horas > 0) {
				if (tot_horas >= _HORAS_DE_TRABAJO) {
					tot_nor    = _HORAS_DE_TRABAJO;
					tot_horas -= _HORAS_DE_TRABAJO;
				}
				else {
					tot_nor = tot_horas ;
					tot_horas = 0;
				}
			}

			// Calculo Horas al 25% 
			if (tot_horas>0) {
				if (tot_horas>=_HORAS_AL_25) {
					tot_25=_HORAS_AL_25;
					tot_horas-=_HORAS_AL_25;
				}
				else{
					tot_25=tot_horas ;
					tot_horas=0;
				}
			}

			// Calculo Horas al 35% 
			if (tot_horas>0) {
				tot_35=tot_horas;
				tot_horas=0;
			} 
/**/
			/******************

			tot_nor=*nor;
			tot_25=*a25;
			tot_35=*a35;

			******************/

//			fprintf(stderr, "Antes HS Norm %d \n", tot_nor);
//			fprintf(stderr, "Antes HS 25 %d \n", tot_25);
//			fprintf(stderr, "Antes HS 35 %d \n", tot_35);
//			fprintf(stderr, "Antes HS Franco %d \n", tot_fra);
//			fprintf(stderr, "Antes Total hs %d \n", p_tothoras);
//			fprintf(stderr, "Antes TopeHs %.2f \n", v_horas_regimen+(double)_MIN_HS_PEGADAS*100);
			break;

		case _DIA_FRANCO :

			if (tot_horas>0) {
				switch(g_tcalculo[0]) {
					case 'A':
						tot_fra=tot_horas ;
						tot_horas=0;
						
						break;
					case 'B':
						tot_fra=tot_horas ;
						tot_horas=0;
						
						break;
				}
			}
			break;

	}


    *nor   = tot_nor;
    *a25   = tot_25;
    *a35   = tot_35;
    *fra   = tot_fra;

}

/***********************************************************************************************************
*                                     CalHorMisCli2
*
*	
*	form p_fm    : Es el descriptor del fm si es que se usa(parametro p_usafm=TRUE),si no deve ser NULL 
*	int  p_row   :	Es el numero de linea del multi si es que se usa(parametro p_usafm=TRUE),si no deve ser
*
* Devuelve : TRUE si hay otro registro del mismo legajo en em multi y ademas la suma de las horas por tipo
*           en los siguientes parametros
*
*	double *p_hs_nor    : Horas normales
*	double *p_hs_50     : Horas al 50%
*	double *p_hs_100fr  : Horas al 100% Franco
*	double *p_hs_100fe  : Horas al 100% Feriado
*
***********************************************************************************************************/
bool CalHorMisCli2(form p_fm, int p_row, double *p_hs_nor,double *p_hs_50, double *p_hs_100fr, 
                           double *p_hs_100fe)
{
	int v_i = 0;
	form v_fm1 = NULL;
	bool v_encontro=FALSE;	


	for (v_i = 0; v_i < FmFldLen(p_fm, MULTIPAR) && !FmIsNull(p_fm, NROLEG, v_i); v_i++) {

//WiMsg("%d %d %d ", v_i, p_row, FmLFld(p_fm, NROLEG, v_i));
//WiMsg("%d ", FmSFld(p_fm, COND, v_i));
		// Descarto otros legajos
		if (FmLFld(p_fm, NROLEG, p_row) != FmLFld(p_fm, NROLEG, v_i))
			continue;

		// Descarto misma linea
		if (v_i == p_row)
			continue;

		// Descarto si la condicion no es la misma
		if (*FmSFld(p_fm, COND, v_i) != *FmSFld(p_fm, COND, p_row))
			continue;

		v_encontro=TRUE;
		v_fm1 = UseSubform(p_fm, DETHS, 0, v_i);


//		fprintf(stderr, "CalHorsMisCli2 suma linea %d, Nor %d dob %d dym %d dymfe %d\n", v_i, FmIFld(v_fm1,  NORMAL), FmIFld(v_fm1, EXTRAS1), 
//		                                                                                             FmIFld(v_fm1, EXTRAS2), FmIFld(v_fm1, FRANCOS));

		*p_hs_nor   += (double)FmIFld(v_fm1,  NORMAL)/100;
		*p_hs_50    += (double)FmIFld(v_fm1, EXTRAS1)/100;
		*p_hs_100fe += (double)FmIFld(v_fm1, EXTRAS2)/100;
		*p_hs_100fr += (double)FmIFld(v_fm1, FRANCOS)/100;
		
	} 


	return v_encontro;
}

// ***************************************************************************************************************************************
// *                                        CalculoDetalleHorasPer()
// *
// * Calcula las horas que hay que grabar en un registro del parte teniendo en cuenta lo que hay grabado en otros registros del mismo dia 
// *
// * Parametros :
// *		 p_usafm    = es verdadero que tiene que contar horas del FM y falso si no
// *         p_fm  		= Descriptor del FM si el primer paramentro es verdadero, sino es NULL_SHORT
// *		 p_row      = Numero de linea del FM  si el primer paramentro es verdadero, sino es NULL_SHORT
// *
// *         p_emp 		= empresa 
// *         p_cliente	= cliente 
// *		 p_objetivo	= objetivo
// *		 p_dia		= fecha 
// *		 p_nroleg	= numero de legajo 
// * 		 p_ptoser 	= Puesto
// *		 p_puesto	= Puesto
// *		 p_nroint	= Puesto
// *
// * 
// * Devuelve	: 
// *		nor		  = Horas Normales
// * 		a25       = Horas al 25%
// *		a35       = Horas al 35%
// *		fra       = Horas Franco
// *
// ***************************************************************************************************************************************
void CalculoDetalleHorasPer(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,  DATE p_dia , long p_nroleg, 
                            int p_ptoser, int p_puesto, int p_nroint, int *nor, int *a25, int *a35, int *fra, int *peg, int *gua)
{
	schema operac, old;

	dbcursor c_parte;

//ACA
    bool v_msg=FALSE;

	char v_condic='\0';
	
	int tot_horas=0,
		objefe=0,
		tot_horas_puesto=0;


	char v_regimen[12];
	double v_horas_regimen=0;

	int tipodia=0,
		v_pais=0,
		v_prov=0;

	int	tot_nor=0,
		tot_a25=0,
		tot_a35=0, 
		tot_fra=0,
		tot_peg=0;

	int	otr_nor=0,
		otr_a25=0,
		otr_a35=0,
		otr_gua=0,
		otr_fra=0;

	char otr_condic='\0';

	double	fm_otr_nor=0,
			fm_otr_a25=0,
			fm_otr_a35=0,
			fm_otr_fra=0;

	bool otras_horas=FALSE;

	long cliefe=0;

	char v_auxtpto[6];

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

//fprintf(stderr, "aca (%T) (%T)\n", TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL));

	PushRecord(operac|PARTE);

	//------------------------------------Calculo cantidad de horas en otros puestos por tipo de hora  -----------------------------------//
	tot_horas=0;

	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_dia, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_dia, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {


		/**************/

		tot_nor   += IFld(operac|PARTE_HSNOR);
		tot_a25   += IFld(operac|PARTE_HS50);
		tot_a35   += IFld(operac|PARTE_HS100F);
		tot_fra   += IFld(operac|PARTE_HS100FE);

		/**************/

		// si es el mismo puesto no lo tomo
		if (LFld(operac|PARTE_CLIENTE)  == p_cliente && 
		    IFld(operac|PARTE_OBJETIVO) == p_objetivo && 
		    IFld(operac|PARTE_PTOSER)   == p_ptoser &&
		    IFld(operac|PARTE_PUESTO)   == p_puesto /*  DHC esto es un numero de renglon && 
		    IFld(operac|PARTE_NROINT)   == p_nroint*/) {

			if (IFld(operac|PARTE_NROINT) == p_nroint) {
				v_condic  =*SFld(operac|PARTE_CONDIC);
				if (*SFld(operac|PARTE_CONDIC) == _PEGADA_C) {
					tot_peg += IFld(operac|PARTE_HS100F);
				}
			    continue;
		    }
	    }

		if (p_usafm &&
			LFld(operac|PARTE_CLIENTE)  == p_cliente && 
		    IFld(operac|PARTE_OBJETIVO) == p_objetivo)
		    continue;

		/* si es guardiania acumulo las horas van todas normales */
//		sprintf(v_auxtpto, "%d", IFld(operac|PARTE_PTOSER));
//		if (EsParNov(p_emp, PARNOV_TPUHSNOR, p_dia, v_auxtpto)) {
//			otr_gua += IFld(operac|PARTE_HSNOR) + IFld(operac|PARTE_HS50) + IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE);
//		}

		otr_nor   += IFld(operac|PARTE_HSNOR);
		otr_a25   += IFld(operac|PARTE_HS50);

		if (*SFld(operac|PARTE_CONDIC) != _PEGADA_C)
			otr_a35   += IFld(operac|PARTE_HS100F);

		otr_fra   += IFld(operac|PARTE_HS100FE);
		otr_condic  = *SFld(operac|PARTE_CONDIC);

		if (v_msg)
			fprintf (stderr, "OtrasHoras otr_nor %d otr_a25 %d otr_a35 %d otr_fra %d\n", otr_nor, otr_a25, otr_a35, otr_fra);
	}    	
	DeleteCursor(c_parte);


	if (p_usafm){
		fm_otr_nor=0;
		fm_otr_a25=0;
		fm_otr_a35=0;
		fm_otr_fra=0;

		CalcHorasMismoCliente(p_fm, p_row, &fm_otr_nor, &fm_otr_a25, &fm_otr_fra, &fm_otr_a35);

		if (v_msg)
			fprintf (stderr, "CalcHorasMismoCliente fm_otr_nor %.2f fm_otr_a25 %.2f fm_otr_a35 %.2f fm_otr_fra %.2f\n", fm_otr_nor, fm_otr_a25, fm_otr_a35, fm_otr_fra);

		otr_nor   += ((int)fm_otr_nor)*100;
		otr_a25   += ((int)fm_otr_a25)*100;
		otr_a35   += ((int)fm_otr_a35)*100;
		otr_fra   += ((int)fm_otr_fra)*100;
	}

	if (v_msg)
		fprintf (stderr, "\nLEGAJO %ld día %.3D\nTotal de Otras Horas otr_nor %d otr_a25 %d otr_a35 %d otr_fra %d\n", p_nroleg, p_dia, otr_nor, otr_a25, otr_a35, otr_fra);


	if ( (otr_nor + otr_a25 + otr_a35 + otr_fra) > 0)
		otras_horas=TRUE;
	else
		otras_horas=FALSE;

		
		
	if (v_msg)
		fprintf (stderr, "Otras Horas %B\n", otras_horas );

	if (otras_horas){
		//-----------------------------------------Calculo cantidad total de horas de esa fecha ----------------------------------------------//
		tot_horas=0;

		//-------------horas de otros objetivos-----------------------
		c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
		SetCursorFrom(c_parte, p_emp, p_nroleg, p_dia, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_parte, p_emp, p_nroleg, p_dia, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_parte) != ERROR) {
			if (p_usafm && LFld(operac|PARTE_CLIENTE)  == p_cliente &&  IFld(operac|PARTE_OBJETIVO) == p_objetivo ) 
			   continue;

			tot_horas += ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;	
			if (v_msg)
				fprintf (stderr, "Total horas a %d %T %T %d\n", tot_horas, TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL))*100);
		} 
		DeleteCursor(c_parte);

		if (p_usafm){
			fm_otr_nor=0;
			fm_otr_a25=0;
			fm_otr_a35=0;
			fm_otr_fra=0;

			//-------------horas de otros registros del fm -----------------------
			CalcHorasMismoCliente(p_fm, p_row, &fm_otr_nor, &fm_otr_a25, &fm_otr_a35, &fm_otr_fra);

			tot_horas  += ((int)fm_otr_nor)*100;
			tot_horas  += ((int)fm_otr_a25)*100;
			tot_horas  += ((int)fm_otr_a35)*100;
			tot_horas  += ((int)fm_otr_fra)*100;

			if (*FmSFld(p_fm, COND, p_row) == _PEGADA_C)
				tot_peg = ConvHraInt(FmTFld(p_fm, HSENTRE, p_row), FmTFld(p_fm, HSSAL, p_row)) * 100;

			//-------------horas de este registro del fm -----------------------
			tot_horas  += ConvHraInt(FmTFld(p_fm, HSENTRE, p_row), FmTFld(p_fm, HSSAL, p_row)) * 100;

//fprintf(stderr, "B 6 %d\n", tot_horas);

		}
	}
	else {
		if (p_usafm){
			tot_horas  += ConvHraInt(FmTFld(p_fm, HSENTRE, p_row), FmTFld(p_fm, HSSAL, p_row)) * 100;	
			if (*FmSFld(p_fm, COND, p_row) == _PEGADA_C)
				tot_peg = tot_horas;
//fprintf(stderr, "C %d %d\n", tot_horas, tot_peg);
		}
		else{
			SetKey(operac|PARTEbyEMP, p_emp, p_cliente, p_objetivo, p_dia, p_nroleg, p_ptoser, p_puesto, p_nroint);
			if (GetRecord(operac|PARTEbyEMP,THIS_KEY, IO_NOT_LOCK)!=ERROR) {
				tot_horas = ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;
			}
		}
	} 

	if (v_msg)
		fprintf (stderr, "Total horas %d\n", tot_horas);

	//-------------------------------------Calculo Horas del puesto --------------------------------------------------------------------------//
	if (p_usafm) {
		tot_horas_puesto  = ConvHraInt(FmTFld(p_fm, HSENTRE, p_row), FmTFld(p_fm, HSSAL, p_row)) * 100;	
	}
	else{
		SetKey(operac|PARTEbyEMP, p_emp, p_cliente, p_objetivo, p_dia, p_nroleg, p_ptoser, p_puesto, p_nroint);
		if (GetRecord(operac|PARTEbyEMP,THIS_KEY, IO_NOT_LOCK) != ERROR)
			tot_horas_puesto = ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;
	}

	//------------------------------Si es Guardiania todas las horas son normales ------------------------------------------------------------//

	GetRegimenEfectivo(p_emp, p_nroleg, v_regimen, p_dia);
	v_horas_regimen= (double)GetHsRegimen(v_regimen);

	sprintf(v_auxtpto, "%d", p_ptoser);
	if (EsParNov(p_emp, PARNOV_TPUHSNOR, p_dia, v_auxtpto)) {
		if (tot_horas_puesto > _HORAS_REGIMEN_DEFAULT*100) {
//WiMsg("paso");			
			*gua=*nor=_HORAS_REGIMEN_DEFAULT*100;
		}
		else
			*gua=*nor=tot_horas_puesto;
		*a25=0;
		*a35=0;
		*fra=0;
		*peg=0;

		if (v_msg)
			fprintf (stderr, "DistibuyeHorasPer Guardiania horas reg %d tot_horas_puesto %d tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d tot_peg %d\n", _HORAS_REGIMEN_DEFAULT*100,
			                  tot_horas_puesto, tot_nor, tot_a25, tot_a35, tot_fra, tot_peg);
	}
	else {

	//----------------------------------------------------Determino tipo de dia-----------------------------------------------------------//
		if (p_usafm) {
			v_condic=*FmSFld(p_fm, COND, p_row);
		}

		// Determino condicion teniendo en cuenta lo de otros objetivos
/*		if (otr_condic!='\0') {
			if (v_condic!=otr_condic) {
				if (otr_nor!=0 || otr_a25!=0 || otr_a35!=0 || otr_fra==0) {
					v_condic=otr_condic;
				}
			}
		}
*/
		GetCliObjEfectivo(p_emp, p_nroleg, p_dia, &cliefe, &objefe);

		v_pais = GetCliePais(cliefe, objefe);
		v_prov = GetClieProv(cliefe, objefe);

//DHC
		if (v_condic==_PEGADA_C) 

			tipodia=_DIA_PEGADA;
		else {
			if (v_condic=='F') {
				tipodia=_DIA_FRANCO;
			}
			else {
				if (FeriadoNovia(p_dia, v_pais, v_prov)) 
					tipodia=_DIA_FERIADO;
				else 
					tipodia=_DIA_NORMAL;
			}
		}

		GetRegimenEfectivo(p_emp, p_nroleg, v_regimen, p_dia);
		v_horas_regimen= (double)GetHsRegimen(v_regimen);

		if (v_msg)
			fprintf (stderr, "Tipodia  %d\n", tipodia);

	//------------------------------------Deacuerdo al tipo de dia distribuyo total de horas por dia -------------------------------------//

		if (v_msg)
			fprintf (stderr, "DistibuyeHorasPer Total de horas del legajo para Fecha tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d \n", tot_nor, tot_a25, tot_a35, tot_fra);
		
		DistibuyeHorasPer(p_emp, tot_horas, tipodia, v_horas_regimen , &tot_nor, &tot_a25,  &tot_a35, &tot_fra, &tot_peg, &otr_gua);

	//------------------------------------------Descuento si hubiera las horas en otros puestos ------------------------------------------//

		if (v_msg){
			fprintf (stderr, "DistibuyeHorasPer Para Fecha NOR =%d\tH25 =%d\tH35 =%d\tFRA =%d\tPEG=%d\n", tot_nor/100, tot_a25/100, tot_a35/100, tot_fra/100, tot_peg/100);
			fprintf (stderr, "DistibuyeHorasPer Para Fecha ONOR=%d\tOH25=%d\tOH35=%d\tOFRA=%d\n", otr_nor/100, otr_a25/100, otr_a35/100, otr_fra/100);
			fprintf (stderr, "Tipodia  %d\n", tipodia);
		}

		if (tipodia == _DIA_FRANCO || tipodia == _DIA_PEGADA) {
			otr_nor = otr_a25 = otr_a35 =  otr_fra = 0;
		}

		tot_nor-=otr_nor;
	   	tot_a25-=otr_a25;
	   	tot_fra-=otr_fra;

		if (otr_a35 >= tot_peg)
			tot_peg=0;
		else
			tot_a35=0;

		if (v_msg) {
			fprintf (stderr, "DistibuyeHorasPer Para Puesto NOR =%d\tH25 =%d\tH35 =%d\tFRA =%d\tPEG=%d\n", tot_nor/100, tot_a25/100, tot_a35/100, tot_fra/100, tot_peg/100);
			fprintf (stderr, "DistibuyeHorasPer Horas a descontar tot_horas_puesto %d tot_nor %d tot_a25 %d tot_a35 %d tot_fra %d tot_peg %d\n", 
			                  tot_horas_puesto, tot_nor, tot_a25, tot_a35, tot_fra, tot_peg);
		}


		//----------------------Controlo que lo que devuelvo sea coherente con la cantidad total de horas del puesto--------------------------//
		*nor=AsignoHoras(tot_nor, &tot_horas_puesto);
		*a25=AsignoHoras(tot_a25, &tot_horas_puesto);
		*a35=AsignoHoras(tot_a35, &tot_horas_puesto);
		*fra=AsignoHoras(tot_fra, &tot_horas_puesto);
		*peg=tot_peg;


    }
	if (v_msg)
		fprintf (stderr, "DistibuyeHorasPer peg %d gua %d\n", *peg, *gua);

	PopRecord(operac|PARTE);

}



// ***************************************************************************************************************************************
// *                                        DistibuyeHorasPer()
// *
// * Dado un total de horas y el tipo de dia (normal, franco o feriado) distribuye cuantas hora corresponden a cada tipo de hora 
// *
// * Parametros :
// *       p_emp        = Empresa
// *       p_tothoras 	= Total de horas a distribuir
// *       p_tipodia	= Tipo de dia 
// * 									_DIA_FRANCO		   
// * 									_DIA_FERIADO
// * 									_DIA_NORMAL
// * Devuelve	: 
// *		nor			= Horas Normales
// * 		a25         = Horas al 25%
// *		a35         = Horas al 35%
// *		fra         = Horas Franco
// *		peg         = Horas Pegadas
// *
// *
// ***************************************************************************************************************************************
void DistibuyeHorasPer(int p_emp, int p_tothoras, int p_tipodia, int p_horas_regimen, int *nor, int *a25, int *a35, int *fra, int *peg, int *gua)

{
	int	tot_nor=0,
		tot_25=0,
		tot_35=0,
		tot_fra=0,
		tot_peg=0;

//ACA
    bool v_msg=FALSE;


	int tot_horas=p_tothoras,
		v_horas_regimen=p_horas_regimen;
	char tcalculo[10] ;


 	if (v_horas_regimen==0) {
		v_horas_regimen = _HORAS_REGIMEN_DEFAULT*100;
	}

	//DHC las guardianias se cuentan como 8 horas normales

//	if (tot_gua > v_horas_regimen)
//		tot_gua = v_horas_regimen;
	
	if (v_msg){
		fprintf (stderr, "DistibuyeHorasPer: tothoras= %2.f\n", tot_horas);
		fprintf (stderr, "DistibuyeHorasPer: p_tothoras %d  \n", p_tothoras);
		fprintf (stderr, "DistibuyeHorasPer: horas_regimen= %d \n", v_horas_regimen);
		fprintf (stderr, "DistibuyeHorasPer: min_hs_PEGADAS= %d\n", _MIN_HS_PEGADAS);
	}

	sprintf (tcalculo, "%s", GetParNov(p_emp, PARNOV_TCALC_HS, 1, Today()) );


	switch(p_tipodia) {
		case _DIA_PEGADA :
//fprintf(stderr, "PEGADAS %d\n", *peg);
			 tot_peg = *peg;
			 break;
		case _DIA_FERIADO :
		case _DIA_NORMAL :

			// Calculo Horas Normales
/*			if (tot_horas>0) {
				if (tot_horas >= _HORAS_DE_TRABAJO + tot_gua) {
					tot_nor=_HORAS_DE_TRABAJO + tot_gua;
					tot_horas-=_HORAS_DE_TRABAJO + tot_gua;
				}
				else {
					tot_nor=tot_horas ;
					tot_horas=0;
				}
			}

			// Calculo Horas al 25% 
			if (tot_horas>0) {
				if (tot_horas>=_HORAS_AL_25) {
					tot_25=_HORAS_AL_25;
					tot_horas-=_HORAS_AL_25;
				}
				else{
					tot_25=tot_horas ;
					tot_horas=0;
				}
			}

			// Calculo Horas al 35% 
			if (tot_horas>0) {
				tot_35=tot_horas;
				tot_horas=0;
			} 
*/
			/******************/

			tot_nor=*nor;
			tot_25=*a25;
			tot_35=*a35-*peg;

			/******************/

			if (v_msg){
				fprintf(stderr, "Antes HS Norm %d \n", tot_nor);
				fprintf(stderr, "Antes HS 25 %d \n", tot_25);
				fprintf(stderr, "Antes HS 35 %d \n", tot_35);
				fprintf(stderr, "Antes HS Franco %d \n", tot_fra);
				fprintf(stderr, "Antes HS PEG %d \n", tot_peg);
				fprintf(stderr, "Antes Total hs %d \n", p_tothoras);
				fprintf(stderr, "Antes TopeHs %.2f \n", v_horas_regimen+(double)_MIN_HS_PEGADAS*100);
			}

			// Calculo Horas Pegadas
			if (p_tothoras > (v_horas_regimen+(double)_MIN_HS_PEGADAS*100)) {

				if (v_msg)
					fprintf(stderr, "Trae Pegadas HS  %d \n", *peg);

				if (*peg > 0 ) {
					tot_peg = *peg;
					tot_35  = p_tothoras-tot_nor+tot_25-tot_peg;
				}
/*				else {
					aux_horas=p_tothoras-(tot_nor+tot_25);
					if (v_horas_regimen < tot_nor+tot_25) {
						if (v_msg)
							fprintf(stderr, "Fuerza HS al 35 %d \n", tot_35);
						tot_35= 200;
					}
					else {
						tot_35= v_horas_regimen-tot_nor-tot_25;
						if (v_msg)
							fprintf(stderr, "Calcula HS al 35 %d \n", tot_35);
					}
					aux_horas-=tot_35;
					tot_peg= aux_horas;
				}
*/			}
			if (v_msg){
				fprintf(stderr, "Despues HS Norm %d \n", tot_nor);
				fprintf(stderr, "Despues HS 25 %d \n", tot_25);
				fprintf(stderr, "Despues HS 35 %d \n", tot_35);
				fprintf(stderr, "Despues HS Franco %d \n", tot_fra);
				fprintf(stderr, "Despues HS PEG %d \n", tot_peg);
			}
			break;

		case _DIA_FRANCO :

			if (tot_horas>0) {
				switch(tcalculo[0]) {
					case 'A':
						tot_fra=tot_horas ;
						tot_horas=0;
						
						break;
					case 'B':
						tot_fra=tot_horas ;
						tot_horas=0;
						
						break;
				}
			}
			break;

	}


    *nor   = tot_nor;
    *a25   = tot_25;
    *a35   = tot_35;
    *fra   = tot_fra;
    *peg   = tot_peg;

}

// ***************************************************************************************************************************************
// *                                        GetCondicRol
// *
// * Devuelve la Condición para un día según el rol
// *
// * Parametros :
// *       fecasi		= Fecha Asignación
// *       fecha		= Fecha
// *       codrol		= Código Rol 
// *       fila			= Fila
// *       colum		= Columna
// *
// * Devuelve	: 
// *	   condic
// *
// ***************************************************************************************************************************************

char * GetCondicRol(DATE fecasi, DATE fecha, int codrol, int fila, int colum)
{
	schema  old, operac;
	int dias, cantrol=0, resto, respuesta;
	
	
	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

    // La fecha es incorrecta, porque es menor a la fecha de asignacion 
	if (fecha < fecasi) {
		return NULL_STR;
	}

	//	Para no recorrer el rol desde la fecha de asignacion hasta la fecha pedida
	//	calculo en base a la cantidad de columnas que tiene el rol 

	SetKey(operac|RROLbyCODROL, codrol, fila,  MAX_SHORT);
	if (GetRecord(operac|RROLbyCODROL, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		cantrol = IFld(operac|RROL_COLUM);
	}

	if (cantrol == 0) {
		return NULL_STR;
	}

	dias  = fecha-fecasi;
	resto = dias % cantrol;

	if ((colum+resto) > cantrol) {
		respuesta = resto - (cantrol-colum);
	}
	else {
		respuesta = colum+resto;
	}

	//fprintf(stderr, "codrol%d fila%d respuesta%d\n", codrol, fila,  respuesta);
	SetKey(operac|RROLbyCODROL, codrol, fila,  respuesta);
	if (GetRecord(operac|RROLbyCODROL, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		return SFld(operac|RROL_VALOR);
	}

	return NULL_STR;
}

// ***************************************************************************************************************************************
// *                                        CondicParteOtroObjetivo
// *
// * Devuelve la Condición de un legajo-dia que no sea del objetivo dado, acepta cliente y objetivo NULO, trayendo la que encuentre
// *
// * Parametros :
// *       emp			= Empresa
// *       cliente		= Cliente a ignorar
// *       objet		= Objetivo a ignorar
// *       fecha		= Fecha
// *       nroleg		= Numero de Legajo
// *
// * Devuelve	: 
// *	   condic
// *
// ***************************************************************************************************************************************
char CondicParteOtroObjetivo(int emp, long cliente, int objet, DATE fecha, long nroleg)
{
	schema	old, operac;
	dbtable	APARTE;
	char condicx;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	APARTE	= CreateAlias(operac|PARTE);

	condicx='\0';
	SetKey(APARTEbyEMPLE, emp, nroleg, fecha, NULL_LONG, NULL_SHORT);
	while (GetRecord(APARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR){
		if (LFld(APARTE_CLIENTE)==cliente && IFld(APARTE_OBJETIVO)== objet)
			continue;
		condicx = *SFld(APARTE_CONDIC);
		break;
	}

	DeleteAlias(APARTE);
	return condicx;
}
// **************************************************************************************************************************************[1m*[0m
// *                                        AudiGrabaHorasParte
// *
// * Llena los campos de la auditoria de horas del parte
// *
// * Parametros :
// *		p_operac 	= Esquema Operac
// *		p_programa	= Programa que modifica las horas
// *
// * Devuelve	: 
// *	   Llena los campos de auditoria de horas del parte, esta funcion se debe llamar antes de hacer un PutRecord del Parte
// *
// ***************************************************************************************************************************************

void AudiGrabaHorasParte(schema p_operac, char * p_programa )
{

	SetFld (p_operac|PARTE_HPROG, p_programa);
	SetDFld(p_operac|PARTE_HDATE, Today());
	SetTFld(p_operac|PARTE_HTIME, Hour());
	SetIFld(p_operac|PARTE_HUID, GetUid());
	
}

int AsignoHoras(int p_hora, int *p_tothora) 
{

	int v_horasi=0;

	if (*p_tothora > p_hora) {
	    v_horasi  = p_hora;
		*p_tothora-=p_hora;
	}
	else{
	    v_horasi = *p_tothora;
	    *p_tothora=0;
	}

	return (v_horasi);
}
// ***************************************************************************************************************************************
// *                                        TotalHsDia
// *
// * Devuelve la Cantidad de Horas que trabajo un legajo en una fecha 
// *
// * Parametros :
// *       p_emp		= Empresa
// *       p_nroleg		= Numero de Legajo
// *       p_dia 		= Fecha
// *
// ***************************************************************************************************************************************

double TotalHsDia(int p_emp, long p_nroleg, DATE p_dia)
{
	double total_horas=0;
	schema	old, operac;
	dbtable	APARTE;

	old	   = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	APARTE	= CreateAlias(operac|PARTE);

	total_horas=0;
	SetKey(APARTEbyEMPLE, p_emp, p_nroleg, p_dia, NULL_LONG, NULL_SHORT);
	while (GetRecord(APARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR){
		total_horas += ConvHraInt(TFld(APARTE_HORAENT), TFld(APARTE_HORASAL));
	}

	DeleteAlias(APARTE);
	return total_horas;
	
}

// ***************************************************************************************************************************************
// *                                        Hours
// *
// * Se le pasa un Time y devuelve la cantidad de horas en enteros 
// *
// * Parametros :
// *       p_hora		= Hora en formato TIME
// *
// ***************************************************************************************************************************************

int Hours(TIME p_hora)
{
    char	str_hora[12],
    		str_can_hs[3];
	
	
	TToStr(p_hora, str_hora, TFMT_SEPAR|TFMT_SECONDS);

	strncpy(str_can_hs, str_hora, 2);
	str_can_hs[2]='\0';

	
	return (StrToI(str_can_hs));

}

// ***************************************************************************************************************************************
// *                                        Minutes
// *
// * Se le pasa un Time y devuelve la cantidad de minutos que corresponden
// *
// * Parametros :
// *       p_hora		= Hora en formato TIME
// *
// ***************************************************************************************************************************************

int Minutes(TIME p_hora)
{
    char	str_hora[12],
   			str_can_min[3];
	
	TToStr(p_hora, str_hora, TFMT_SEPAR|TFMT_SECONDS);

	strncpy(str_can_min,str_hora+3, 2);
	str_can_min[2]='\0';

	return (StrToI(str_can_min));

}

// ***************************************************************************************************************************************
// *                                        Seconds
// *
// * Se le pasa un Time y devuelve la cantidad de Segundos que corresponden
// *
// * Parametros :
// *       p_hora		= Hora en formato TIME
// *
// ***************************************************************************************************************************************

int Seconds(TIME p_hora)
{
    char	str_hora[12],
			str_can_seg[3];
	
	TToStr(p_hora, str_hora, TFMT_SEPAR|TFMT_SECONDS);

	strncpy(str_can_seg, str_hora+6, 2);
	str_can_seg[2]='\0';

	
	return (StrToI(str_can_seg));

}

// ***************************************************************************************************************************************
// *                                        Seconds
// *
// * Sirve para sumar una cantidad de Tiempo a Una Hora dada
// *
// * Parametros :
// *		p_hora		= Hora Base en formato TIME
// *		p_canti		= Cantidad de tiempo a sumar (o restar si es naegativo)
// *		p_unidad	= Unidad de tiempo a sumar o restar
// *
// ***************************************************************************************************************************************
TIME SumaTiempo(TIME p_hora, double p_canti, int p_unidad)
{
	#define SEGUNDOS_X_DIA 86400

	bool v_msj=FALSE;

	double	v_horas=(double)Hours(p_hora), 
			v_minutos=(double)Minutes(p_hora),
			v_segundos=(double)Seconds(p_hora);
		
	double acu_en_seg=(v_horas * 3600) + (v_minutos * 60) + v_segundos;
	
	double v_canti_seg=p_canti;
	char str_hora_sal[12];	
	

	if (v_msj) fprintf(stderr, "Hora ingresada a SumaTiempo()  %.3T\n", p_hora);
	if (v_msj) fprintf(stderr, "Horas %.2f\n", v_horas);
	if (v_msj) fprintf(stderr, "Minutos %.2f\n", v_minutos);
	if (v_msj) fprintf(stderr, "Segundos %.2f\n", v_segundos);

	if (v_msj) fprintf(stderr, "Cantidad de Segundos Acumuladas Antes de Suma %.2f\n", acu_en_seg);

	switch(p_unidad) {
		case _HORAS:
			v_canti_seg=v_canti_seg * 3600;
			break;
		case _MINUTOS:
			v_canti_seg=v_canti_seg * 60;
			break;
	}

	if (v_msj) fprintf(stderr, "Segundos a Sumar %.2f\n", v_canti_seg);

	acu_en_seg += v_canti_seg;	

	if (v_msj) fprintf(stderr, "Cantidad de Segundos Acumuladas Despues de Suma %.2f\n", acu_en_seg);

	//si es negativo le sumo de a un dia hasta que sea positivo
	while(acu_en_seg < 0)
		acu_en_seg+=SEGUNDOS_X_DIA;

	if (v_msj) fprintf(stderr, "Cantidad de Segundos Acumuladas Despues Sacar Negativo %.2f\n", acu_en_seg);

	//si acumulo mas de un dia le resto de a un dia hasta que sea de un dia 
	while(acu_en_seg >= SEGUNDOS_X_DIA)
		acu_en_seg-=SEGUNDOS_X_DIA;

	if (v_msj) fprintf(stderr, "Cantidad de Segundos Acumuladas Despues Sacar Varios Dias %.2f\n", acu_en_seg);
		
	v_horas=(acu_en_seg/3600)-((long)acu_en_seg % 3600);
	acu_en_seg-=(v_horas*3600);
	if (v_msj) fprintf(stderr, "Horas de Resultado %.2f\n", v_horas);
	
	v_minutos=(acu_en_seg/60)-((long)acu_en_seg % 60);
	acu_en_seg-=(v_minutos*60);
	if (v_msj) fprintf(stderr, "Minutos de Resultado %.2f\n", v_minutos);
		
	v_segundos=acu_en_seg;
	if (v_msj) fprintf(stderr, "Segundos de Resultado %.2f\n", v_segundos);

	sprintf(str_hora_sal, "%02d:%02d:%02d", (int)v_horas, (int)v_minutos, (int)v_segundos);

	if (v_msj) fprintf(stderr, "Hora Completa resultante %s\n", str_hora_sal);

	return(StrToT(str_hora_sal));
	
}

// *********************************************************************************************************
// *                               HelpCodInt
// *
// *	Ayuda manual de Puestos por CodInt
// *
// * Parametros:
// *
// *	fm        = descriptor de formulario 
// *	fno       = descriptor de campo
// *   row       = Numero de linea
// *   p_cliente = codigo de cliente
// *   p_objetivo= codigo de objetivo
// *   p_fecha   = Selecciona puestos en los que entraria esta fecha, si es NULL_DATE trae todos
// *   p_vigil   =
// *   p_efect   =
// *
// *
// * Funciones Dependientes: 
// *
// *           private int ValidaCodInt()
// *           void DisplayCodInt(char *buffer)
// *
// *********************************************************************************************************
fm_status HelpCodInt(form fm, fmfield fno, int row, long p_cliente, int p_objetivo, DATE p_fecha)
{
	static dbcursor CUR;
	schema old, ope;
	int n;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(ope);	

	CUR = CreateCursor(ope|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
	SetCursorFrom(CUR, p_cliente, p_objetivo, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, p_cliente, p_objetivo, MAX_SHORT, MAX_SHORT);

	fechapuesto=p_fecha;

	n = PopUpDbMenu(10, 72, "Pto Cteg  HrIni  HrFin      Dias    Regimen  TipoDia  FecIni   FecFin", CUR, 4, ValidaCodInt, DisplayCodInt);

	if (n >= 0) { 
		FmSetIFld (fm, fno, IFld(ope|PUESTOS_CODINT), row);
		FmShowFlds(fm, fno, fno, row);
	}
	DeleteCursor(CUR);

	return FM_OK;
}
int ValidaCodInt()
{

	if (fechapuesto!=NULL_DATE){
		if (IFld(PUESTOS_CANTVIG) != 0.0 &&
		   ((IsNull(PUESTOS_FFINAL)  && !IsNull(PUESTOS_FINICIO) &&
			fechapuesto < DFld(PUESTOS_FINICIO)) ||
		   (!IsNull(PUESTOS_FFINAL) &&
		   (fechapuesto < DFld(PUESTOS_FINICIO) || fechapuesto > DFld(PUESTOS_FFINAL)))))
			return FALSE;


		if (IFld(PUESTOS_CANTVIG) == 0.0 && !IsNull(PUESTOS_FFINAL) &&
		    (fechapuesto > DFld(PUESTOS_FFINAL) || fechapuesto < DFld(PUESTOS_FINICIO)))
			return FALSE;

		if (IFld(PUESTOS_CANTVIG) == 0.0 && IsNull(PUESTOS_FFINAL) &&
		    fechapuesto > DFld(PUESTOS_FINICIO))
			return FALSE;

	}

	return TRUE;
}
void DisplayCodInt(char *buffer)
{
	sprintf(buffer,"%2d %4d", IFld(PUESTOS_CODINT), IFld(PUESTOS_PUESTO));
	sprintf(buffer,"%s %.*T %.*T", buffer, DFMT_SEPAR, TFld(PUESTOS_HINICIO), DFMT_SEPAR, TFld(PUESTOS_HFINAL)); 
	sprintf(buffer,"%s %-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s",buffer,SFld(PUESTOS_DIA1), SFld(PUESTOS_DIA2), SFld(PUESTOS_DIA3), SFld(PUESTOS_DIA4),	SFld(PUESTOS_DIA5), SFld(PUESTOS_DIA6), SFld(PUESTOS_DIA7));
	sprintf(buffer,"%s %8.8s    %2.2s  ",buffer, SFld(PUESTOS_REGIM), SFld(PUESTOS_TIPODIA));
	sprintf(buffer,"%s   %.1D %.1D ", buffer, DFld(PUESTOS_FINICIO), DFld(PUESTOS_FFINAL));

}

