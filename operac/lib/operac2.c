/********************************************************************
*
* MODULE & VERSION : @(#)operac.c	1.52
* DATE             : 04/09/22
* TIME             : 15:31:02
*
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
#include "operac.h"
#include "opedef.h"
#include "comerc.h"
#include "ambiente.h"
#include "asist.sch"
#include "brigada.sch"
#include "sue.sch"
#include "billpro.sch"

char   horae[60], horas[60],  hsent[60], hssal[60], minue[30], minus[30];
void BajarHoras(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot);

DATE FechaFinalOt (long cliente, short objetivo, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, bool baja);
DATE FecIng(int emp, long nroleg);
bool EncontroFrancoRol (int emp, long nroleg, DATE fecha, char *vigil , bool *rolfranco);

/*-------------------------* AlOtroDia *-----------------------*/
/* Devuelve -TRUE  si al dia siguiente de la fecha el vigilador esta asignado a algun puesto,
            -FALSE si no esta asignado. */
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

/*** Devuelve TRUE si en ASIGH el vigilador existe en el cliente/objetivo ***/
/* bool prov: agregado para que confpar.c toma los provisorios y parte.c no. */
/*-------------------------* ExisteEnAsigh *-----------------------*/
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

/*** Devuelve TRUE si en ASIG el vigilador existe en el cliente/objetivo ***/
/*-------------------------* ExisteEnAsig *-----------------------*/
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

/*** Devuelve TRUE si trabaja el dia en puesto efectivo ***/
/*-------------------------* TrabDiaEnPtoEfec *-----------------------*/
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

/*** Devuelve TRUE si el vigilador esta asignado a un puesto Part Time en forma Efectiva. ***/
/*-------------------------* VigPartime *-----------------------*/
bool VigPartime(int emp, long legajo, DATE fecparte)
{
	schema old, operac;
	bool partime = FALSE, enasigh;

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

/*** Devuelve TRUE si el vigilador esta asignado a un puesto Part Time para un cliente/objetivo dado.***/
/*-------------------------* EsPuestoPartime *-----------------------*/
bool EsPuestoPartime(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte)
{
	schema old, operac;
	bool partime = FALSE, enasigh;

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

/*** Devuelve TRUE si el vigilador esta asignado en forma efectiva a un puesto. ***/
/*-------------------------* EsEfectivo *-----------------------*/
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

/*** Devuelve el tipo de vigilador ASIG_VIGIL. ***/
/*-------------------------* TipoVig *-----------------------*/
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
	if (GetRecord(ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		sprintf(vigil, SFld(ASIG_VIGIL));
	else {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_CLIENTE,  cliente);
		SetIFld(ASIGH_OBJETIVO, objetivo);  
		SetLFld(ASIGH_NROLEG,   legajo);		
		SetIFld(ASIGH_PTOSER,   tippto);
		SetIFld(ASIGH_PUESTO,   puesto);
		SetIFld(ASIGH_NROINT,   nroint);
		if (GetRecord(ASIGHbyPUESTO, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ))
				sprintf(vigil, SFld(ASIGH_VIGIL));
		}
	}
	SwitchToSchema(old);
	return vigil;
}


/*--------------* HoraEntEfectivo *-----------------*/
TIME HoraEntEfectivo(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrent;
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

/*--------------* HoraSalEfectivo *-----------------*/
TIME HoraSalEfectivo(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrsal;
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

/*--------------* HrEntTrabEfec *-----------------*/
TIME HrEntTrabEfec(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrent;
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

/*--------------* HrSalTrabEfec *-----------------*/
TIME HrSalTrabEfec(int emp, long nroleg, DATE fecha)
{
	bool	 encontre = FALSE;
	TIME     hrsal;
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

/*--------------* HoraEntParte *-----------------*/
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

/*--------------* HoraSalParte *-----------------*/
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

/*-------------------------* GetTipPtoEfec *-----------------------*/
int GetTipPtoEfec(int emp, long legajo, DATE fecha)
{
	schema old, operac;
	int tippto = NULL_SHORT;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_NROLEG,   legajo);
	SetLFld(ASIG_CLIENTE,  MIN_LONG);
	SetIFld(ASIG_OBJETIVO, MIN_SHORT);
	if (GetRecord(ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (!strcmp(SFld(ASIG_EFECT), EFECTIVO)) {
			tippto = IFld(ASIG_PTOSER);
		}
	}
	else {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_NROLEG,   legajo);
		SetLFld(ASIGH_CLIENTE,  MIN_LONG);
		SetIFld(ASIGH_OBJETIVO, MIN_SHORT);
		while (GetRecord(ASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (fecha >= DFld(ASIGH_FECALT) && fecha <= DFld(ASIGH_FECBAJ)) {
				if (!strcmp(SFld(ASIGH_EFECT), EFECTIVO)) {
					tippto = IFld(ASIGH_PTOSER);
				}
			}
		}
	}
	SwitchToSchema(old);
	return tippto;
}

/*-------------------------* GetTipPto *-----------------------*/
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


/*-------------------------* GetPuesto *-----------------------*/
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
    

/*-------------------------* GetRegimen *-----------------------*/
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


/*-------------------------* FeriadoNovia *-----------------------*/
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


/*-------------------------* Franco *-----------------------*/
bool Franco(int emp, long nroleg, DATE fecha, char vigil[2], int numfran)
{
	schema	old, operac;
	bool	esfranco = FALSE, salir = FALSE;
	int		diaslab, diasfranco;
	DATE	fecfranco = NULL_DATE, ffrancod, ffrancoh, ftrabd, ftrabh;
	char	regimen[50];
	short nfran;

	#ifdef _NOVIA_VER_2_0
	//Si esta asignado con un rol devuelvo si es franco en base al ROL
	if (EncontroFrancoRol (emp, nroleg, fecha, vigil , &esfranco)){
		//	fprintf (stderr, "franco %ld %.3D %d \n", nroleg, fecha, esfranco);
		return esfranco;
	}
	#endif

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

/*---------------------------* GetRegimenEfectivo *--------------------------*/
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

/*-------------------* GetPuestoEfectivo *-----------------------/
int GetPuestoEfectivo(int emp, long nroleg)
{
	schema	  old, operac;
	dbtable	  ASIGNAC;
	dbcursor  c_asig, c_asigh;

	old		= CurrentSchema();
	operac	= OpenSchema("operac", IO_EABORT);
	SwitchToSchema(operac);

	ASIGNAC	= CreateAlias(operac|ASIG);
	c_asig	= CreateCursor(AlInd(operac|ASIGNAC, operac|ASIGbyNROLEG), IO_NOT_LOCK);
	c_asigh	= CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(AlFld(ASIGNAC, operac|ASIG_EFECT)), EFECTIVO)) {
			continue;
		}
		return IFld(AlFld(ASIGNAC, operac|ASIG_PUESTO));
	}

	SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		if (strcmp(SFld(operac|ASIGH_EFECT), EFECTIVO))
			continue;

		if (IFld(AASIGH_MOTIVO) == DESXERROR)
			continue;

		if (fecha >= DFld(operac|ASIGH_FECALT) && fecha <= DFld(operac|ASIGH_FECBAJ))
			return IFld(operac|ASIGH_PUESTO));
	}
	DeleteCursor(c_asig);
	DeleteAlias(operac|ASIGNAC);
	DeleteCursor(c_asigh);
	SwitchToSchema(old);

	return NULL_SHORT;
}

/*----------------------------* GetFFrancoEfectivo *--------------------------*/
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

/*----------------------------* GetNumFrancoEfectivo *--------------------------*/
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

	/* Cuando seteas la hora 0 el valor que te toma es 00:02 */
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
		return total = 0.15;

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
			/* Los minutos nunca seran iguales porque sino ya salio al principio por ser hr y minuto iguales */
            /*****************************
            Esto esta mal porque tiene que comparar los minutos y no el proporcional con la hora
			if (intmie > intmis)
				total = 24;
			if (intmie < intmis)
				total = 0;
            *****************************/				
			if (StrToF(minue) > StrToF(minus))
				total = 24;
			if (StrToF(minue) < StrToF(minus))
				total = 0;
		}
	}

	if (intmie != 0)
		total--;

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

/**************** FUNCIONES PARA ASIGL.EXE Y ASIGD.EXE **************************/
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


// Esta función tiene dos usos principales
// dadas por la booleana "total":
//
// (a) si total está en TRUE:
// la función devuelve TRUE si hent-hsal está incluido (o es =) en hdesde-hhasta
// la función devuelve FALSE si alguna hora del rango hent-hsal no esta dentro del rango hdesde-hhasta
// esta funcionalidad se usa para validar en el asigd y en el asigl, que el rango de horas
// ingresado para un vigilador este dentro del rango permitido del puesto al cual es asignado
// (valida hent-hsal cargado por form contra hdesde-hhasta que trae del puesto elegido para ese vigilador
// que es también cargado por form).
//
// (b) si total esta en FALSE:
// la función devuelve TRUE si alguna hora del rango hent-hsal esta en el rango hdesde-hhasta
// la función devuelve FALSE si ninguna hora del rango hent-hsal esta en el rango hdesde-hhasta
// Esta funcionalidad se utiliza para determinar si dos rangos de horas se superponen
// esto se usa en asigd y asigl para no permitir que se cargue un rango de horas que se superponga
// con otro rango de horas ya cargado para ese mismo vigilador.
//
/*
bool Superposicion(TIME hent, TIME hsal, TIME hdesde, TIME hhasta, bool total)
{
	int i, j, horaent, horasal, horad, horah;
	char aux[10];
	bool incluida;
	horaent = BusHora(hent);
	horasal = BusHora(hsal);
	horad = BusHora(hdesde);
	horah = BusHora(hhasta);
	i = horaent;
	do {
		// si la hora hasta es en punto (ej: 08:00 hs) es hasta esa hora pero
		// no la incluye. En consecuencia, hace el break;
		if (i == horasal) {
			sprintf(aux, "%02d00", horasal);
			if (StrToT(aux) == hsal) {
				break;
			}
		}
		incluida = FALSE;
		j = horad;
		do {
			// si la hora hasta es en punto (ej: 08:00 hs) es hasta esa hora pero
			// no la incluye. En consecuencia, hace el break;
			if (j == horah) {
				sprintf(aux, "%02d00", horah);
				if (StrToT(aux) == hhasta) {
					break;
				}
			}
			if (i == j) {
				if (!total) {
					return TRUE;
				}
				else {
					incluida = TRUE;
				}
			}
			j = (j == 23) ? 0 : j + 1;
		} while ((j != (horah == 23 ? 0 : horah + 1)) && incluida == FALSE);
		if (!incluida && total) {
			return FALSE;
		}
		i =  (i == 23) ? 0 : i + 1;
	} while (i != ((horasal == 23 ? 0 : horasal + 1)));
	if (total) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}
*/
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
		return;
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
	(*fecmax) = fecha + GetDiasLaboral(regimen, !(strcmp(vigil, PARTTIME)));

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
/**************** FIN FUNCIONES PARA ASIGL.EXE Y ASIGD.EXE **************************/

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
					 int prov, int hstot, TIME hssal, TIME hsentre, int ptoser, int puesto, int nroint,
					 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran)
{
	int horasno, horasfe, hsnorregim, hsnorotrocli, hsnornue, toths = 0, ctrlhs;
	bool impre=FALSE;
    int horas_hoy, horas_man;
    bool dia_vig_hoy=FALSE, dia_vig_man=FALSE; 
	
	*hsnor = *hs50 = *hs100f = *hs100fe = 0.0;
	hsnorregim   = GetHsNormales(regimen, FALSE);
	ctrlhs       = ConvHraInt(hsentre, hssal) * 100;
	hsnorotrocli = HsOtroCliente(emp, nroleg, dia, cliente, objetivo, ptoser, puesto, nroint, &toths, FALSE);
	hsnornue     = hsnorregim - hsnorotrocli;

	horas_hoy = ConvHraInt(hsentre, StrToT("2359")) * 100;
	horas_man = ConvHraInt(StrToT("235958"), hssal) * 100;

	if (DiaVigilador(dia, pais, prov)) {
		dia_vig_hoy = TRUE;
	}

	if (DiaVigilador(dia+1, pais, prov)) {
		dia_vig_man = TRUE;
	}

	if (impre) fprintf (stderr, "%ld %.3D \n", nroleg, dia);

	/* Si ingreso hsent = 0 y hssal = 0 */
	if (!ctrlhs)
		return;

	// para calcular el franco no hace falta mandarle el cli-obj ni el regimen:
	if (Franco(emp, nroleg, dia, tipvig, numfran) && !FeriadoNovia(dia, pais, prov)) {
		*hs100f = hstot;                    
	}
	else {
		/* Dia Feriado */
		if (FeriadoNovia(dia, pais, prov)) {
			if (hssal < hsentre) {
				/* Entra a trabajar dia feriado y sale dia normal o franco*/
				horasfe  = ConvHraInt(hsentre, StrToT("2359")) * 100;
				horasno  = ConvHraInt(StrToT("235958"), hssal) * 100;
				*hs100fe = horasfe;

				/* Sale día normal */
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
				if (DiaVigilador((dia+1), pais, prov)) {
					*hs100f = *hs50;
					*hs50 = 0;
				}
			}
			else {
				/* Trabaja todo el dia feriado */
				*hs100fe = hstot;
			}
		}
		else {
			/* Dia no Feriado */
			if (hsentre <= hssal) {
				/* Trabaja todo el dia normal */
				if (hsnornue == 0) {
						/* En el otro cliente trabajo todas las hs normales */
						*hs50 = hstot;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

						return;
				}
				if (toths = 0) {
					if (hstot < hsnorregim) {
						*hsnor = hstot;
					}
					else {
						*hsnor = hsnorregim;
					}
					*hs50 = GetHsExtras(regimen, hstot, FALSE);

					//Si el dia es el dia del Vigilador las horas extras van al 100
			  		if (DiaVigilador(dia, pais, prov)) {
						*hs100f = *hs50;
						*hs50 = 0;
					}

				}
				else {
					if (hstot <= hsnornue) {
						*hsnor = hstot;
					}
					else {
						/* Trabaja hs normales y extras */
						*hsnor = hsnornue;
						*hs50  = hstot - hsnornue;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

						return;
					}
				}
			}
			else {
				/* Entra a trabajar dia normal y sale dia feriado */
				if (FeriadoNovia(dia + 1, pais, prov)) {
					horasno = ConvHraInt(hsentre, StrToT("2359")) * 100;
					if (horasno <= hsnorregim) {
						/* El dia normal trabaja todas horas normales */
						*hsnor = horasno;
					}
					else {
						/* El dia normal trabaja horas normales y extras al 50% */
						*hsnor = hsnorregim;
						*hs50  = horasno - hsnorregim;

						//Si el dia es el dia del Vigilador las horas extras van al 100
				  		if (DiaVigilador(dia, pais, prov)) {
							*hs100f = *hs50;
							*hs50 = 0;
						}

					}
					*hs100fe = ConvHraInt(StrToT("235958"), hssal) * 100;
				}
				else {
					/* Entra a trabajar un dia normal y sale un dia normal */
					if (impre) fprintf (stderr, "Entra a trabajar un dia normal y sale un dia normal\n");

 					if (impre) fprintf (stderr, "HS hoy %d man %d\n", horas_hoy, horas_man);

					if (hsnornue == 0) {
						/* En el otro cliente trabajo todas las hs normales */
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
					if (toths = 0) {
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
							/* Trabaja hs normales y extras */
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

/*					if (hstot < hsnorregim) {
						*hsnor = hstot;
					}
					else {
						*hsnor = hsnorregim;
					}
					*hs50 = GetHsExtras(regimen, hstot, FALSE);
*/
				}
			}
		}
	}
}

void CalcularDetalleProv(int emp, long cliente, int objetivo, long nroleg, DATE dia, char * tipvig, int pais,
						 int prov, int hstot, TIME hssal, TIME hsentre, int ptoser, int puesto, int nroint,
						 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran)
{
	int horasno, horasfe, horasext, tothor, hsnorregim, hsnorotrocli, hsnornue, toths = 0, ctrlhs;
	TIME hsentefec, hssalefec;
	bool trabaja;
    int horas_hoy, horas_man;
    bool dia_vig_hoy=FALSE, dia_vig_man=FALSE; 

	//Esto es porque hay invasion de memoria y no la pudimos encontrar.
	char dummy[100];
	sprintf(dummy, "%s %s", tipvig, regimen);

	horas_hoy = ConvHraInt(hsentre, StrToT("2359")) * 100;
	horas_man = ConvHraInt(StrToT("235958"), hssal) * 100;

	if (DiaVigilador(dia, pais, prov)) {
		dia_vig_hoy = TRUE;
	}

	if (DiaVigilador(dia+1, pais, prov)) {
		dia_vig_man = TRUE;
	}

	trabaja   = TrabDiaEnPtoEfec(emp, cliente, objetivo, nroleg, dia);
//	hsentefec = HoraEntEfectivo(emp, nroleg, dia);
//	hssalefec = HoraSalEfectivo(emp, nroleg, dia);
	hsentefec = HrEntTrabEfec(emp, nroleg, dia);
	hssalefec = HrSalTrabEfec(emp, nroleg, dia);

	/* Total de horas trabajadas en horario efectivo */
	*hsnor = *hs50 = *hs100f = *hs100fe = 0.0;
	tothor     = ConvHraInt(hsentefec, hssalefec) * 100;
	hsnorregim = GetHsNormales(regimen, FALSE);
	ctrlhs     = ConvHraInt(hsentre, hssal) * 100;

	/* Si ingreso hsent = 0 y hssal = 0 */
	if (!ctrlhs)
		return;

	// para calcular el franco no hace falta mandarle el cli-obj ni el regimen:
	if (Franco(emp, nroleg, dia, tipvig, numfran) && !FeriadoNovia(dia, pais, prov)) {
		*hs100f = hstot;
	}
	else {
		/* Dia Feriado */
		if (FeriadoNovia(dia, pais, prov)) {
			if (hssal < hsentre) {
				/* Entra a trabajar dia feriado y sale dia normal o franco*/
				horasfe  = ConvHraInt(hsentre, StrToT("2359")) * 100;
				horasno  = ConvHraInt(StrToT("235958"), hssal)   * 100;
				*hs100fe = horasfe;

				/* Sale día normal */
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
				/* Trabaja todo el dia feriado */
				*hs100fe = hstot;
			}
		}
		else {
			/* Dia no Feriado */
			hsnorotrocli = HsOtroCliente(emp, nroleg, dia, cliente, objetivo, ptoser, puesto, nroint, &toths, FALSE);
			hsnornue     = hsnorregim - hsnorotrocli;

			if (hsentre < hssal) {
				if (hsnornue == 0) {
					/* En el otro cliente trabajo todas las hs normales */
					*hs50 = hstot;
					//Si el dia es el dia del Vigilador las horas extras van al 100
	 		  		if (dia_vig_hoy) {
						*hs100f = *hs50;
						*hs50 = 0;
					}
					return;
				}
				/* Trabaja el dia en el Pto. Efectivo */
				if (trabaja) {
					if (hstot > tothor) {
						*hsnor = hsnorregim;
						*hs50  = GetHsExtras(regimen, tothor, FALSE) + (hstot - tothor);
		 		  		if (dia_vig_hoy) {
							*hs100f = *hs50;
							*hs50 = 0;
						}
					}
					/* Trabaja provisorio despues de hora del horario efectivo */
					if (hsentre >= hssalefec || hssal <= hsentefec) {
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
					/* Entra provisorio dentro del horario efectivo */
					if (hsentre >= hsentefec && hsentre < hssalefec) {
						*hsnor = ConvHraInt(hsentre, hssalefec) * 100;
						*hs50  = ConvHraInt(hssalefec, hssal) * 100;
					}
					/* Sale provisorio dentro del horario efectivo */
					if (hssal >= hsentefec && hssal < hssalefec) {
						*hsnor = ConvHraInt(hsentefec, hssal) * 100;
						*hs50  = ConvHraInt(hsentre, hsentefec) * 100;
						if (dia_vig_hoy) {
							*hs100f = *hs50;
							*hs50 = 0;
						}
					}
				}
				else {
					/* Las hs trabajadas no superan las hs normales del otro cliente o pto. */
					if (hstot <= hsnornue) {
						*hsnor = hstot;
					}
					else {
						/* Trabaja hs normales y extras */
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
				/* Entra a trabajar dia normal y sale dia feriado */
				if (FeriadoNovia(dia + 1, pais, prov)) {
					horasno = ConvHraInt(hsentre, StrToT("2359")) * 100;
					if (horasno <= hsnorregim) {
						/* El dia normal trabaja todas horas normales */
						*hsnor = horasno;
					}
					else {
						/* El dia normal trabaja horas normales y extras al 50% */
						*hsnor = hsnorregim;
						*hs50  = horasno - hsnorregim;
					}
					*hs100fe = ConvHraInt(StrToT("235958"), hssal) * 100;
				}
				else {
					/* Entra a trabajar un dia normal y sale un dia normal */
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
							/* En el otro cliente trabajo todas las hs normales */
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
						/* Las hs trabajadas no superan las hs normales del otro cliente*/
						if (hstot <= hsnornue) {
							*hsnor = hstot;
						}
						else {
							/* Trabaja hs normales y extras */
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

void CalcularDetallePartime(long cliente, int objetivo, DATE dia, TIME hsentre, TIME hssal,
                             int * hsnor, int * hs100fe)
{
	int pais, prov, ctrlhs;

	schema prev = CurrentSchema();
	OpenSchema("comerc", IO_EABORT);
	*hsnor = *hs100fe = 0.0;

	ctrlhs = ConvHraInt(hsentre, hssal) * 100;

	/* Si ingreso hsent = 0 y hssal = 0 */
	if (!ctrlhs)
		return;

	SetKey(OBJETIVO, cliente, objetivo);
	(void)GetRecord(OBJETIVO, THIS_KEY, IO_NOT_LOCK);
	pais = IFld(OBJETIVO_PAIS);
	prov = IFld(OBJETIVO_PROV);			

	/* Dia Feriado */
	if (FeriadoNovia(dia, pais, prov)) {
		if (hssal < hsentre) {
			/* Entra a trabajar dia feriado y sale dia normal */
			*hs100fe = ConvHraInt(hsentre, StrToT("2359"))*100;
			*hsnor   = ConvHraInt(StrToT("235958"), hssal)*100;
		}
		else {
			/* Trabaja todo el dia feriado */
			*hs100fe = ConvHraInt(hsentre, hssal)*100;
		}
	}
	else {
		/* Dia no Feriado */
		if (hsentre < hssal) {
			/* Trabaja todo el dia normal */
			*hsnor = ConvHraInt(hsentre, hssal) * 100;
		}
		else {
			/* Entra a trabajar dia normal y sale dia feriado */
			if (FeriadoNovia(dia + 1, pais, prov)) {
				*hsnor   = ConvHraInt(hsentre, StrToT("2359")) * 100;
				*hs100fe = ConvHraInt(StrToT("235958"), hssal) * 100;
			}
			else
				*hsnor = ConvHraInt(hsentre, hssal)*100;	
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
			if (LFld(APARTE_CLIENTE) == cliente && IFld(APARTE_OBJETIVO) == obj)
//				IFld(APARTE_PTOSER) == ptoser && IFld(APARTE_PUESTO) == puesto && IFld(APARTE_NROINT) == nroint)
				continue;
		}
		hsnormal += IFld(APARTE_HSNOR);
		*hs      += IFld(APARTE_HSNOR) + IFld(APARTE_HS50) + IFld(APARTE_HS100F) + IFld(APARTE_HS100FE);
	}
	DeleteCursor(c_parte);
	DeleteAlias(APARTE);
	SwitchToSchema(prev);
	return hsnormal;
}

/*-------------------------* GetNextNroint *-----------------------*/
/* Devuelve el proximo número interno para el puesto */
            
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

/*-------------------------* BajaPuesto *-----------------------*/
/* Devuelve si el puesto esta de baja */
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

/*-------------------------* AsigCodInOperac *-----------------------*/
void AsigCodInOperac(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char* tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot)
{
	schema old, operac;
	int codigo;
	DATE fechafin=NULL_DATE, fechaini = NULL_DATE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
    
	SetKey(PUESTOSbyCLIENTE, cliente, objetivo, tippto, MAX_SHORT);
	if (GetRecord(PUESTOSbyCLIENTE, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
		codigo  = IFld(PUESTOS_CODINT) + 1;
	else
		codigo = 1;


	SetKey (PUESTOSbyPUESTO, cliente, objetivo, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5,
							dia6, dia7, regimen, codfrec, tipodia);
	
	if (GetRecord(PUESTOSbyPUESTO, THIS_KEY, IO_LOCK) == ERROR || str_eq (cond, "A") ) {

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

/*		if (ffinal != NULL_DATE)
			fechafin= FechaFinalOt (cliente, objetivo, finicio, ffinal, hiniot, hfinot, FALSE);
		else
			fechafin= NULL_DATE;
*/					
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
 		BajarHoras (cliente, objetivo, tippto, puesto, hinicio, hfinal,
					 dia1, dia2, dia3, dia4, dia5, dia6,
					 dia7, regimen, tipodia, cantpue, cantvig, cond, codfrec, 
					 horapt, finicio, ffinal, hiniot, hfinot);
	}

}

void BajarHoras(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot)
{
	find_mode mode;		
	schema old, operac;
	bool entro=FALSE;
	short puepend, vigpend, horpend;
	char  buffer[70];	
	
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
		/*Solo pongo fecha de baja si va a quedar en 0 la cant. de vig. */
		if (IFld(PUESTOS_CANTVIG) - bvig == 0 && bvig > 0) {
			DATE fechafin= FechaFinalOt (cliente, objetivo, finicio, ffinal, hiniot, hfinot, TRUE);
			SetDFld(PUESTOS_FFINAL,  fechafin);
		}
		SetIFld(PUESTOS_CANTPUE, IFld(PUESTOS_CANTPUE) - bpue);
		SetIFld(PUESTOS_CANTVIG, IFld(PUESTOS_CANTVIG) - bvig);

		PutRecord(PUESTOS);
    }   
	FreeTable(PUESTOS);
	SwitchToSchema(old);
    /****
	if (vigpend || puepend || horpend ) {
		WiMsg ("No se pudo bajar puesto %s", buffer);        
	} 
	*****/
}

DATE FechaFinalOt (long cliente, short objetivo, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, bool baja)
{

	DATE fecbaja=NULL_DATE;
	TIME horbaja;


	/* Si tiene fecha inicio y final y es riff no tomo en cuenta la fecha de finalizacion */
	if (ObjetRif(cliente, objetivo) && 
//		finicio != NULL_DATE &&
//		ffinal  != NULL_DATE &&
		!baja)
			return NULL_DATE;
		
	if (!ObjetRif(cliente, objetivo) && !baja && ffinal == NULL_DATE)
		return NULL_DATE;

    /* Si es una baja */
	if (finicio != NULL_DATE && ffinal != NULL_DATE) {
	/*	Si es una baja y tiene fecha de inicio y final es porque esta acompañado
		por el alta de un nuevo puesto y sin importar la hora lo tomo con el dia
		de inicio */
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

	/*	Si es una baja y NO tiene fecha de inicio y final 
		tomo la que tiene valor
		Solo me fijo la hora si tiene fecha de finalizacion porque
		si termino != 00:00 no lo tomo en cuenta porque ese dia tiene que trabajar
		si termino = 00:00  lo tomo en cuenta porque ese dia NO tiene que trabajar
	 */
    	
		fecbaja = ffinal != NULL_DATE ? ffinal : finicio;
		horbaja = hfinot != NULL_TIME ? hfinot : StrToT("0000");
	} 

	/*	
		si termino != 00:00 Termina ese dia
		si termino = 00:00  Termina el dia anterior
	*/			
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

bool DiaVigilador(DATE fecha, int pais, int prov)
{   
	//El 25 de abril es el dia del vigilador
	DATE fecv;
	short dia=25, mes=4, anio;

	anio = Year(fecha);
	fecv = DMYToD(dia, mes, anio);

	if (fecha == fecv)
		return TRUE;

	return FALSE;		
}

/*-------------------------* GetRolDesc *--------------------------*/
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

/*-------------------------* ValorRol *--------------------------*/
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

	hsreg = (double)GetHsRegimen(regim)/100.0;

	horini= hinicio;
	turno = StrToI(rvalor);
	turno = turno > 0 ? turno -1 : 0;

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

	horfin= hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) > _SEGUNDOS_POR_DIA ? 
			hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA) - _SEGUNDOS_POR_DIA : 
			hinicio + (hsreg * turno * _SEGUNDOS_POR_HORA);

	if (horfin == StrToT("0000")) {
		return StrToT("2359");
	}
	
	// fprintf (stderr, "Viene %.3T %.3T  \n", horpini,  horpfin);
	if (horpini == StrToT("0000") && horpfin == StrToT("2359")) {
		// fprintf (stderr, "Sin cambiar  horpfin %.3T \n", horpfin);
		return horfin;
	}

	//Me fijo la diferencia de horas - si no puedo cubrir otro turno estiro o achico el actual.
	difhor = (double)(horpfin - horfin);
	difhor = horpfin == NULL_TIME ? 0 : fabs(difhor) /  _SEGUNDOS_POR_HORA;

	//fprintf (stderr, "1 horfin %.3T horpfin %.3T difhor %.2f hsreg %.2f \n", horfin, horpfin, difhor, hsreg);
	if (difhor > 0 && difhor < hsreg) {
		// fprintf (stderr, "PONGO horpfin %.3T \n", horpfin);
		horfin = horpfin;
	}

	return horfin;
}

/***********************************************************************************************
Esta funcion devuelve TRUE si el legajo esta asignado en la fecha en un puesto que tiene rol.
Si devuelve TRUE en el parametro rolfranco devuelve:
	TRUE : Si ese dia SI tiene franco
	FALSE: Si ese dia NO tiene franco
**********************************************************************************************/
bool EncontroFrancoRol (int emp, long nroleg, DATE fecha, char *vigil , bool *rolfranco)
{
	bool		encontre = FALSE;
	schema		old, operac;
	dbtable		AASIG, AASIGH;
	dbcursor	c_asig, c_asigh;
	char turno[30];
	TIME tmph1, tmph2;

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

		GetHorasTurno ( LFld(AASIG_CLIENTE), IFld(AASIG_OBJETIVO),
						DFld(AASIG_FECASIG), IFld(AASIG_CODROL), IFld(AASIG_FILA), IFld(AASIG_COLUM),
						!str_eq(SFld(AASIG_REGPTO), NULL_STR) ? SFld(AASIG_REGPTO): SFld(AASIG_REGIM),  
						TFld(AASIG_HSENT), TFld(AASIG_HSSAL),
						fecha, IFld(AASIG_PTOSER), IFld(AASIG_PUESTO),
						SFld(AASIG_DIA1), SFld(AASIG_DIA2), SFld(AASIG_DIA3),
						SFld(AASIG_DIA4), SFld(AASIG_DIA5), SFld(AASIG_DIA6),
						SFld(AASIG_DIA7), turno, &tmph1, &tmph2); 
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
				GetHorasTurno (LFld(AASIGH_CLIENTE), IFld(AASIGH_OBJETIVO),
						DFld(AASIGH_FECALT), IFld(AASIGH_CODROL), IFld(AASIGH_FILA), IFld(AASIGH_COLUM),
						!str_eq(SFld(AASIGH_REGPTO), NULL_STR) ? SFld(AASIGH_REGPTO): SFld(AASIGH_REGIM),  
						TFld(AASIGH_HSENT), TFld(AASIGH_HSSAL),
						fecha, IFld(AASIGH_PTOSER), IFld(AASIGH_PUESTO),
						SFld(AASIGH_DIA1), SFld(AASIGH_DIA2), SFld(AASIGH_DIA3),
						SFld(AASIGH_DIA4), SFld(AASIGH_DIA5), SFld(AASIGH_DIA6),
						SFld(AASIGH_DIA7), turno, &tmph1, &tmph2); 
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

/********************************************************************************************************
GetHorasTurno : Calcula el horario correspondiente a una fecha

Parametros: 
	fecasig: Fecha de asignación. Para saber a partir de esa fecha como es la rotción de turnos.
	rol-fila-colum : Rol que esta asignado el vigilador (En la fecha de asignación).
	regim :  Regimen que esta asignado (para saber la duración del turno).
	hinicio-hfinal: Hora que tiene que cumplir el primer dia que trabaja.
	fecha : Fecha que se quiere averiguar.

Devuelve: 
	valor : Turno que le corresponde para la fecha.
	hdesde - hhasta: Horario que le corresponde para la fecha.

Calculo: Rol estatico: No importa si ese dia trabaja o no en el puesto, se mueve en el rol siempre.
                       Si no trabaja ese dia devuelve que no trabaja, salvo que sea franco.
         Rol dinamico: Si ese dia no trabaja en el puesto 

********************************************************************************************************/
void GetHorasTurno (long cliente, short objet, DATE fecasig, short rol, short fila, short colum, char *regim, TIME hinicio, TIME hfinal,
					DATE fecha, short ptoser, short puesto, char *dia1, char *dia2, char *dia3, char *dia4, char *dia5,
					char *dia6, char *dia7, char *valor, TIME *hdesde, TIME *hhasta)
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

   	/*Verifico cuantos dias trabaja en la semana */
	if (!str_eq(dia1, NULL_STR)) cdias ++;
	if (!str_eq(dia2, NULL_STR)) cdias ++;
	if (!str_eq(dia3, NULL_STR)) cdias ++;
	if (!str_eq(dia4, NULL_STR)) cdias ++;
	if (!str_eq(dia5, NULL_STR)) cdias ++;
	if (!str_eq(dia6, NULL_STR)) cdias ++;
	if (!str_eq(dia7, NULL_STR)) cdias ++;

    /*La fecha es incorrecta, porque es menor a la fecha de asignacion */
	if (fecha < fecasig) {
		return ;
	}

	SetIFld(operac|ROL_CODROL, rol);
	if (GetRecord(operac|ROLbyCODROL, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		return ;
	}

	/*	Para no recorrer el rol desde la fecha de asignacion hasta la fecha pedida
		calculo en base a la cantidad de columnas que tiene el rol */
	SetIFld(operac|RROL_CODROL, rol);
	SetIFld(operac|RROL_FILA,   fila);
	SetIFld(operac|RROL_COLUM,  MIN_SHORT);
	while (GetRecord(operac|RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		cantrol ++;
	}

	if (cantrol == 0) return;

	if (cdias == 7 || IFld(operac|ROL_TIPROL) == _ROL_ESTATICO) {
		#ifdef _GETHORASTURNO
		fprintf (stderr, "Trajaba los 7 dias o el rol es estatico - Calculo directo\n");
		#endif
		/*Si trabaja toda la semana puedo calcular directamente que turno le corresponde */
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
	}
	else {
		/*Recorro para saber cuantos dias cubrio */
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
		fprintf (stderr, "Trajaba %d dias - Pasaron %d dias de la fecha de asignacion\n", cdias, cpaso);
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

	*hdesde = HoraInicioTurno(hinicio, regim, SFld(operac|RROL_VALOR));
	*hhasta = HoraFinTurno(cliente, objet, ptoser, puesto, hinicio, regim, SFld(operac|RROL_VALOR));   
	strcpy (valor, SFld(operac|RROL_VALOR));

	/*Si es franco o no trabaja va sin horas */
	if (str_eq(valor, _FRANCO) || str_eq(valor, _NO_TRABAJA)) {
		*hdesde=StrToT("0000");
		*hhasta=StrToT("0000");
	}

	if (str_eq(valor, _TURNO_INICIAL)){
		/*Si es el turno inicial pongo directamente el horario de la asignacion */
		#ifdef _GETHORASTURNO
		fprintf (stderr, "Turno inicial - Hora de la asignacion \n");
		#endif

		*hdesde=hinicio;
		*hhasta=hfinal;
	}

	/*Si el rol es dinamico valido que el dia trabaje
	  si el rol es estatico y es franco lo genero igual  */
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

void DesArmarCCosto(long nroccte, long *vcli, int *vobj)
{
	double resto, real, valor;
	int aux;

	// #include <math.h>

	*vcli = NULL_LONG;
	*vobj = NULL_SHORT;

	if (nroccte == NULL_LONG) {
		return ;
	}

	//Los ultimos dos digitos pertenecen al objetivo, el resto al cliente
	valor = (double) nroccte / 100.0;

	resto = modf(valor, &real);

	//*vcli = real;
 	
 	
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

/*---------------------* FechaInicioPuesto *--------------------*/
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
/*---------------------* FechaFinPuesto *--------------------*/
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

/* GetCliObjEfectivo 
* Devuelve el cliente/objetivo que esta asignado en forma efectiva
*/
void GetCliObjEfectivo(int emp, long nroleg, DATE fecha, long * cliente, int * objetivo)
{
	bool     encontre = FALSE;
	schema   old, operac;
	dbtable  AASIG, AASIGH;
	dbcursor c_asig, c_asigh;
	DATE maxfecalt=NULL_DATE;				

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

/************************* ExisteCliObjEnGrp *************************/
bool ExisteCliObjEnGrp(int grupo, long cliente, int objetivo)
{
	schema prev, billpro;
    long modo;

	prev    = CurrentSchema();
	billpro = OpenSchema("billpro", IO_EABORT);
	SwitchToSchema(prev);

 	SetIFld(billpro|RCLIESP_TIPCLI,  grupo);
	SetLFld(billpro|RCLIESP_CLIENTE, cliente);
	SetIFld(billpro|RCLIESP_OBJET,   objetivo);
	if (GetRecord(billpro|RCLIESPbyTIPCLI, THIS_KEY|NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
		return TRUE;

	return FALSE;
}

/************************* FecIng *************************/
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

/***************** MotDesagRota ******************/
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

/***************** GetDescMotivd ******************/
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

