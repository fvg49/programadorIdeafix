#include <ideafix.h>
#include "comgral.sch"
#include "depano.fmh"
#include "depanoh.fmh"

static fm_status before(form fm, fmfield fno, int row);
static fm_status after(form fm, fmfield fno, int row);
static void Lectura(void);
static void LecturaHistorial(int row);
static void LimpioMulti(form fm, fmfield fno, int desde);
static void Grabo(void);

form fm0;
schema comgral;

wcmd(depano, 1.0 09/16/26)
{
	fm_cmd cmd;

	comgral = OpenSchema("comgral", IO_EABORT);
	fm0 = OpenForm("depano", FM_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:
			Lectura();
			break;
		case FM_ADD:
		case FM_UPDATE:
			Grabo();
			break;
		case FM_IGNORE:
			LimpioMulti(fm0, RENGLON, 0);
			break;
		}
	}
}

static void LimpioMulti(form fm, fmfield fno, int desde)
{
	int row;

	for (row = desde; row < FmFldLen(fm, MULTI); row++)
		FmClearFlds(fm, fno, VALOR, FECVIG, ACTIVO, row);
}

static void Grabo(void)
{
	int row;

	BeginTransaction();

	SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), FmIFld(fm0, COD), MIN_SHORT, MIN_DATE);
	while (GetRecord(comgral|DEPANObyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
		DelRecord(comgral|DEPANO);

	for (row = 0; row < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, RENGLON, row); row++) {
		SetIFld(comgral|DEPANO_EMP,    FmIFld(fm0, EMP));
		SetIFld(comgral|DEPANO_COD,    FmIFld(fm0, COD));
		SetIFld(comgral|DEPANO_NROREN, FmIFld(fm0, RENGLON, row));
		SetDFld(comgral|DEPANO_FECVIG, FmDFld(fm0, FECVIG, row));
		SetIFld(comgral|DEPANO_ACT,    FmIFld(fm0, ACTIVO, row));
		SetFld (comgral|DEPANO_VALOR,  FmSFld(fm0, VALOR, row));
		PutRecord(comgral|DEPANO);
	}

	EndTransaction();
}

static void Lectura(void)
{
	int row, ultimo_renglon, lin_fm;
	DATE fecha;

	LimpioMulti(fm0, RENGLON, 0);
	ultimo_renglon = MIN_SHORT;
	fecha = FmDFld(fm0, FECCON);
	if (FmIsNull(fm0, EMP) || FmIsNull(fm0, COD) || FmIsNull(fm0, FECCON))
		return;

	SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), FmIFld(fm0, COD), MAX_SHORT, MAX_DATE);
	if (GetRecord(comgral|DEPANObyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
		ultimo_renglon = IFld(comgral|DEPANO_NROREN);

	for (row = 0, lin_fm = 0;
	     row < FmFldLen(fm0, MULTI) && row <= ultimo_renglon;
	     row++) {
		SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), FmIFld(fm0, COD), row, fecha + 1);
		if (GetRecord(comgral|DEPANObyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			FmSetIFld(fm0, RENGLON, IFld(comgral|DEPANO_NROREN), lin_fm);
			FmSetFld(fm0, VALOR, SFld(comgral|DEPANO_VALOR), lin_fm);
			FmSetDFld(fm0, FECVIG, DFld(comgral|DEPANO_FECVIG), lin_fm);
			FmSetIFld(fm0, ACTIVO, IFld(comgral|DEPANO_ACT), lin_fm);
			lin_fm++;
		}
	}
}

static void LecturaHistorial(int row)
{
	form fm1;
	int i;

	fm1 = UseSubform(fm0, RENGLON, 0, row);

	SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), FmIFld(fm0, COD),
	       FmIFld(fm0, RENGLON, row), MIN_DATE);
	for (i = 0; i < FmFldLen(fm1, HMULTI); i++) {
		if (GetRecord(comgral|DEPANObyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR)
			break;
		FmSetDFld(fm1, HFECVIG, DFld(comgral|DEPANO_FECVIG), i);
		FmSetFld (fm1, HVALOR,  SFld(comgral|DEPANO_VALOR), i);
		FmSetIFld(fm1, HACTIVO, IFld(comgral|DEPANO_ACT), i);
	}

	DoSubform(fm0, NULLFP, NULLFP, RENGLON, 0, row);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case RENGLON:
		if (FmIsNull(fm, FECCON))
			return FM_SKIP;
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case RENGLON:
		if (FmKeyCode(fm) == K_META) {
			if (!FmIsNull(fm, RENGLON, row))
				LecturaHistorial(row);
			return FM_SKIP;
		}
		break;
	}
	return FM_OK;
}
