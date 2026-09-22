/********************************************************************
* MODULE & VERSION : @(#)asigman.c
* DATE             : 17/09/15
* TIME             : 12:00:00
*
* CREATED          :
* DESCRIPTION:
*
*      Asignación Manual de Vigiladores.
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "asig.h"
#include "billpro.h"
#include "asigman.fmh"
#include "operac.sch"
#include "bill.sch"
#include "comerc.sch"
#include "operac.h"
#include "brigada.h"

static fm_status after(form, fmfield, int);
static void Lectura(fm_cmd cmd, find_mode mode);

schema operac;
form fm0;
int i;

wcmd(asigman, 1.00 17/15/07)
{
	fm_status cmd;

	operac = OpenSchema("operac", IO_EABORT);


	fm0 = OpenForm("asigman", FM_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:      Lectura(cmd, THIS_KEY); break;
		case FM_READ_NEXT: Lectura(cmd, NEXT_KEY); break;
		case FM_READ_PREV: Lectura(cmd, PREV_KEY); break;
		case FM_ADD :
		case FM_UPDATE :
			BeginTransaction();

			InitRecord(operac|ASIGH);

			SetIFld(operac|ASIGH_EMP,      IFld(operac|ASIG_EMP));
			SetLFld(operac|ASIGH_CLIENTE,  LFld(operac|ASIG_CLIENTE));
			SetIFld(operac|ASIGH_PTOSER,   IFld(operac|ASIG_PTOSER));
			SetIFld(operac|ASIGH_PUESTO,   IFld(operac|ASIG_PUESTO));
			SetIFld(operac|ASIGH_NROINT,   IFld(operac|ASIG_NROINT));
			SetIFld(operac|ASIGH_OBJETIVO, IFld(operac|ASIG_OBJETIVO));
			SetLFld(operac|ASIGH_NROLEG,   LFld(operac|ASIG_NROLEG));
			SetFld (operac|ASIGH_VIGIL,    SFld(operac|ASIG_VIGIL));
			SetFld (operac|ASIGH_EFECT,    SFld(operac|ASIG_EFECT));
			SetDFld(operac|ASIGH_FECALT,   DFld(operac|ASIG_FECASIG));
			SetFld (operac|ASIGH_DIA1,     SFld(operac|ASIG_DIA1));
			SetFld (operac|ASIGH_DIA2,     SFld(operac|ASIG_DIA2));
			SetFld (operac|ASIGH_DIA3,     SFld(operac|ASIG_DIA3));
			SetFld (operac|ASIGH_DIA4,     SFld(operac|ASIG_DIA4));
			SetFld (operac|ASIGH_DIA5,     SFld(operac|ASIG_DIA5));
			SetFld (operac|ASIGH_DIA6,     SFld(operac|ASIG_DIA6));
			SetFld (operac|ASIGH_DIA7,     SFld(operac|ASIG_DIA7));
			SetTFld(operac|ASIGH_HSENT,    TFld(operac|ASIG_HSENT));
			SetTFld(operac|ASIGH_HSSAL,    TFld(operac|ASIG_HSSAL));
			SetFld (operac|ASIGH_REGIM,    SFld(operac|ASIG_REGIM));
			SetFld (operac|ASIGH_TIPODIA,  SFld(operac|ASIG_TIPODIA));
			SetDFld(operac|ASIGH_FECHAS,   DFld(operac|ASIG_FECHAS));
			SetDFld(operac|ASIGH_FFRANCO,  DFld(operac|ASIG_FFRANCO));
			SetIFld(operac|ASIGH_NUMFRAN,  IFld(operac|ASIG_NUMFRAN));
			SetIFld(operac|ASIGH_FRANCERO, IFld(operac|ASIG_FRANCERO));
			SetIFld(operac|ASIGH_CODROL,   IFld(operac|ASIG_CODROL));
			SetIFld(operac|ASIGH_FILA,     IFld(operac|ASIG_FILA));
			SetIFld(operac|ASIGH_COLUM,    IFld(operac|ASIG_COLUM));
			SetFld (operac|ASIGH_REGPTO,   SFld(operac|PUESTOS_SUBREG));

			SetDFld(operac|ASIGH_FECBAJ, FmDFld(fm0, FECBAJ));
			SetIFld(operac|ASIGH_MOTIVO, FmIFld(fm0, MOTIVO));

			PutRecord(operac|ASIGH);
			FreeTable(operac|ASIGH);

			DelRecord(operac|ASIG);

			InitRecord(operac|ASIG);
			SetIFld(operac|ASIG_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|ASIG_CLIENTE,  FmLFld(fm0, CLINUE));
			SetIFld(operac|ASIG_OBJETIVO, FmIFld(fm0, OBJNUE));
			SetLFld(operac|ASIG_NROLEG,   FmLFld(fm0, NROLEG));
			SetFld (operac|ASIG_VIGIL,    FmSFld(fm0, VIGNUE));
			SetFld (operac|ASIG_EFECT,    FmSFld(fm0, EFENUE));
			SetDFld(operac|ASIG_FECASIG,  FmDFld(fm0, FECBAJ)+1);
			SetIFld(operac|ASIG_PTOSER,   FmIFld(fm0, PTONUE));
			SetIFld(operac|ASIG_PUESTO,   FmIFld(fm0, PUENUE));
			SetIFld(operac|ASIG_NROINT,   FmIFld(fm0, INTNUE));
			SetFld (operac|ASIG_DIA1,     FmSFld(fm0, DIA1NUE));
			SetFld (operac|ASIG_DIA2,     FmSFld(fm0, DIA2NUE));
			SetFld (operac|ASIG_DIA3,     FmSFld(fm0, DIA3NUE));
			SetFld (operac|ASIG_DIA4,     FmSFld(fm0, DIA4NUE));
			SetFld (operac|ASIG_DIA5,     FmSFld(fm0, DIA5NUE));
			SetFld (operac|ASIG_DIA6,     FmSFld(fm0, DIA6NUE));
			SetFld (operac|ASIG_DIA7,     FmSFld(fm0, DIA7NUE));
			SetTFld(operac|ASIG_HSENT,    FmTFld(fm0, ENTNUE));
			SetTFld(operac|ASIG_HSSAL,    FmTFld(fm0, SALNUE));
			SetDFld(operac|ASIG_FFRANCO,  FmDFld(fm0, FRANUE));
			SetIFld(operac|ASIG_NUMFRAN,  FmIFld(fm0, NUMNUE));
			SetIFld(operac|ASIG_FRANCERO, FmIFld(fm0, FRONUE));
			SetFld (operac|ASIG_REGIM,    FmSFld(fm0, REGNUE));
			SetFld (operac|ASIG_TIPODIA,  FmSFld(fm0, TIPNUE));
			SetIFld(operac|ASIG_CODROL,   FmIFld(fm0, ROLNUE));
			SetIFld(operac|ASIG_FILA,     FmIFld(fm0, FILNUE));
			SetIFld(operac|ASIG_COLUM,    FmIFld(fm0, COLNUE));
			SetFld (operac|ASIG_REGPTO,   FmSFld(fm0, SUBNUE));
			SetDFld(operac|ASIG_FECHAS,   FmDFld(fm0, HASNUE));

			PutRecord(operac|ASIG);
			FreeTable(operac|ASIG);

			SetKey(operac|PARTEbyLEG, FmIFld(fm0, EMP), FmLFld(fm0, CLIANT), FmIFld(fm0, OBJANT), FmLFld(fm0, NROLEG), FmDFld(fm0, FECBAJ));
			while(FindRecord(operac|PARTEbyLEG, NEXT_KEY|PARTIAL_KEY, 4) != ERROR) {
				if (IFld(operac|PARTE_PTOSER) != FmIFld(fm0, PTOANT)
					|| IFld(operac|PARTE_PUESTO) != FmIFld(fm0, PUEANT)) {
					continue;
				}
				DelRecord(operac|PARTE);
			}

			EndTransaction();
			break;
		case FM_IGNORE :
			break;
		}
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{

	switch(mode) {
	case THIS_KEY:
		SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), MIN_LONG, MIN_BYTE);
		if (GetRecord(operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) == ERROR) {
			FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
			WiMsg("Error");
		}
		break;
	case NEXT_KEY:
		if (GetRecord(operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) == ERROR) {
			FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
			WiMsg("Error");
		}
		break;
	case PREV_KEY:
		if (GetRecord(operac|ASIGbyNROLEG, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) == ERROR) {
			FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
			WiMsg("Error");
		}
		break;
	}

	FmSetFld (fm0, NOMBRE, GetNombreLeg(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG)), i);
	
	DbToFm(fm0, CLIANT, HASNUE);

}


static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROLEG :

		break;
	}
	return FM_OK;
}


