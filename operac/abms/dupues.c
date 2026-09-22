/********************************************************************
*
* MODULE & VERSION : @(#)dupues.c	1.2
* DATE             : 13/03/04
* TIME             : 15:34:45
*
* CREATED          : 01/03/2013
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*      By Diego Linares
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "dupues.fmh"
#include "sue.sch"
#include "comerc.sch"
#include "bill.sch"
#include "billpro.sch"
#include "operac.sch"

/* Funciones privadas */
/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
static void Lectura(fm_cmd, find_mode);
static int validate(void);
static void display(char *buffer);
private fm_status HelpCliente(form fm, fmfield fno, int row);

/* Declaraciones globales */
form fm0;
schema comerc, sue, billpro, operac, bill;
char denomlp[40];
dbcursor CurPue;
int NLIN, i;
dbtable acliente;

/* Programa principal */
wcmd(dupues, 1.2 03/04/13)
{
	fm_cmd cmd;
	sue    = OpenSchema("sue", IO_EABORT);
	billpro= OpenSchema("billpro", IO_EABORT);
	bill   = OpenSchema("bill", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	fm0 = OpenForm("dupues", FM_EABORT);
	CurPue = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);
	NLIN = FmFldLen(fm0, MULTI);
	FmOnKey(fm0, K_HELP, HelpCliente, CLIEOP, CLIEOP);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_UPDATE:
		BeginTransaction();
		for (i = 0; i < NLIN && !FmIsNull(fm0, TIPPTO, i); i++) {
			if (!FmIFld(fm0, DUPLICA, i))
				continue;
			SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJETCOM), FmIFld(fm0, TIPPTO, i), FmIFld(fm0, CODINT, i));
			if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
				continue; // inconsistencia
			SetLFld(operac|PUESTOS_CLIENTE, FmLFld(fm0, CLIEOP));
			SetIFld(operac|PUESTOS_OBJET, FmIFld(fm0, OBJETOP));
			PutRecord(operac|PUESTOS);
		}

		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	i = 0;
	//primary key(cliente, objet, tippto, codint),
	SetCursorFrom(CurPue, FmLFld(fm0, CLIE), FmIFld(fm0, OBJETCOM), NULL_SHORT, NULL_SHORT);
	SetCursorTo  (CurPue, FmLFld(fm0, CLIE), FmIFld(fm0, OBJETCOM), MAX_SHORT, MAX_SHORT);
	while (FetchCursor(CurPue) != ERROR && i < NLIN) {

		/* Filtro por Rango de Fechas de Inicio */
		if (!IsNull(operac|PUESTOS_FINICIO) && (DFld(operac|PUESTOS_FINICIO) < FmDFld(fm0, FINID) ||
					(!FmIsNull(fm0, FINIH) && DFld(operac|PUESTOS_FINICIO) > FmDFld(fm0, FINIH))))
			continue;

		/* Filtro por Rango de Fechas de Inicio */
		if (!IsNull(operac|PUESTOS_FFINAL) && (DFld(operac|PUESTOS_FFINAL) < FmDFld(fm0, FFIND) ||
					(!FmIsNull(fm0, FFINH) && DFld(operac|PUESTOS_FINICIO) > FmDFld(fm0, FFINH))))
			continue;

		DbToFm(fm0, TIPPTO, FFINAL, i);
		FmShowFlds(fm0, TIPPTO, FFINAL, i);
		FmSetIFld(fm0, DUPLICA, FALSE, i);
		i++;
	}
	SetIFld(sue|EMPS_EMP, IFld(comerc|OBJETIVO_EMP));
	GetRecord(sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK);
	DbToFm(fm0, OBJETCOM, DEMP);
	FmShowFlds(fm0, OBJETCOM, DEMP);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case OBJETOP:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case OBJETOP:
		if (!FmIsNull(fm, fno)) {
			SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIEOP), FmIFld(fm, fno));
			if(GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				Warning("No existe el Objetivo %d para el Cliente %ld", FmIFld(fm, fno), FmLFld(fm0, CLIE));
				return FM_REDO;
			}
			SetIFld(sue|EMPS_EMP, IFld(comerc|OBJETIVO_EMP));
			GetRecord(sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK);
			DbToFm(fm, fno, DEMP2);
			FmShowFlds(fm, fno, DEMP2);
		}
		break;
	}
	return FM_OK;
}

private fm_status HelpCliente(form fm, fmfield fno, int row)
{
	static dbcursor CUR = NULL;
	int n;

	acliente  = CreateAlias(bill|CLIENTE);
	if (!FmIsNull(fm, DIDTRIB)) {
		CUR = CreateCursor(AlInd(acliente, bill|CLIENTEbyIDTRIBM), IO_NOT_LOCK|IO_CONTROL_BREAK);
		SetCursorFrom(CUR, FmSFld(fm0, DIDTRIB), MIN_LONG);
		SetCursorTo  (CUR, FmSFld(fm0, DIDTRIB), MAX_LONG);
		//WiMsg("Entro por RUC");
	}
	else {
		CUR = CreateCursor(AlInd(acliente, bill|CLIENTEbyTIPDOC), IO_NOT_LOCK|IO_CONTROL_BREAK);
		SetCursorFrom(CUR, FmIFld(fm0, FTIPDOC), FmSFld(fm0, FNRODOC));
		SetCursorTo  (CUR, FmIFld(fm0, FTIPDOC), FmSFld(fm0, FNRODOC));
		//WiMsg("Entro por NRODOC");
	}
	
	n = PopUpDbMenu(10, 75, "Cliente      Razón Social                                      RUC", CUR, 10, validate, display);
	if (n >= 0) {
		FmSetLFld(fm, fno,     LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)));
		FmSetFld (fm, DESCLI2, SFld(AlFld(acliente,bill|CLIENTE_RAZSOC)));
		FmShowFlds(fm, fno, DESCLI2);
	}
	DeleteCursor(CUR);
	DeleteAlias(acliente);
}

static int validate(void)
{
	//index comer(cliecom, objetcom, cliente, objetop);
	//WiMsg("1 validate cliente %ld", LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)));
	SetKey(billpro|OBJETRELbyCOMER, FmLFld(fm0, CLIE), FmIFld(fm0, OBJETCOM), LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)), NULL_SHORT);
	if (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR) {
		//WiMsg("1 validate cliente %ld Va a retornar FALSE", LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)));
		return FALSE;
	}
	//WiMsg("2 validate cliente %ld Va a retornar TRUE", LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)));
	return TRUE;
}

static void display(char *buffer)
{
	sprintf(buffer,"%8ld %-50.50s %-18.18s", LFld(AlFld(acliente, bill|CLIENTE_CLIENTE)), SFld(AlFld(acliente, bill|CLIENTE_RAZSOC)), SFld(AlFld(acliente,bill|CLIENTE_IDTRIB)));
}

