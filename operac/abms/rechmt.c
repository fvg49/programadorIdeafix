/********************************************************************
*
* MODULE & VERSION : @(#)rechmt.c	1.1 
* DATE             : 06/05/15 
* TIME             : 12:22:15 
*
* CREATED          : 06/10/98
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "rechmt.fmh"
#include "comerc.sch"
#include "motrech.fmh"

/* Defines */
#define PENDIENTE	0
#define RECHAZADO	2

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
private bool SetMultiRech();
private void ClearMulti();

/* Declaraciones globales */
int i;
form fm0, fm1;
schema comerc;

/* Programa principal */
wcmd(rechmt, 1.1 05/15/06)
{
	fm_cmd cmd;
	fm0    = OpenForm("rechmt",   FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	SetMultiRech();

	if ((cmd = DoForm(fm0, NULLFP, after)) == FM_EXIT) return;
	switch (cmd) {
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROOT:
		DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
		break;
	}
	return FM_OK;
}

private bool SetMultiRech()
{
	dbcursor c_OT;

	c_OT = CreateCursor(comerc|OTbyAPROBO, IO_NOT_LOCK);

	SetCursorFrom(c_OT, RECHAZADO, MIN_SHORT, MIN_SHORT, MIN_SHORT, LOW_VALUE,  MIN_LONG);
	SetCursorTo  (c_OT, RECHAZADO, MAX_SHORT, MAX_SHORT, MAX_SHORT, HIGH_VALUE, MAX_LONG);
	for (i = 0; i < FmFldLen(fm0, MULTIR) && FetchCursor(c_OT) != ERROR;) {
		fm1 = UseSubform(fm0, NROOT, 0, i);

		SetIFld(comerc|NCBTES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		GetRecord(comerc|NCBTESbyTIPCOMP, THIS_KEY, IO_NOT_LOCK);

		SetIFld(comerc|NSERIES_EMP,     IFld(comerc|OT_EMP));
		SetIFld(comerc|NSERIES_TIPCOMP, IFld(comerc|OT_TIPCOMP));
		SetIFld(comerc|NSERIES_SERIE,   IFld(comerc|OT_SERIE));
		GetRecord(comerc|NSERIESbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (IFld(comerc|OT_EMP) == _EMP_PSA)
			FmSetFld(fm0, EMPR, GetValParam(MOD_VIGI, EMPOT), i);
		else
			FmSetFld(fm0, EMPR, GetValParam(MOD_VIGI, EMPSAPE), i);

		FmSetFld (fm0, TIPCOMPR, SFld(comerc|NSERIES_DESCOR),  i);
		FmSetFld (fm0, SERIER,   SFld(comerc|NCBTES_DESCOR),   i);
		FmSetFld (fm0, DELEGR,   SFld(comerc|OT_DELEG),        i);
		FmSetLFld(fm0, NROOTR,   LFld(comerc|OT_NROOT),        i);
		FmSetLFld(fm0, CLIENTER, LFld(comerc|OT_CLIENTE),      i);
		FmSetIFld(fm0, OBJETR,   IFld(comerc|OT_OBJET),        i);
		FmSetDFld(fm0, FOKVTAR,  DFld(comerc|OT_FOKVTA),       i);
		FmSetTFld(fm0, HOKVTAR,  TFld(comerc|OT_HOKVTA),       i);
		FmSetFld (fm0, USUCOMR,  SFld(comerc|OT_USUCOM),       i);
		FmSetDFld(fm0, FOKOPERR, DFld(comerc|OT_FOKOPER),      i);
		FmSetTFld(fm0, HOKOPERR, TFld(comerc|OT_HOKOPER),      i);
		FmSetFld (fm0, USUOPER,  SFld(comerc|OT_USUOPER),      i);
//		FmSetIFld(fm0, PLAZOR,   IFld(comerc|OT_PLAZO),        i);
		FmSetFld (fm0, TIPSERR,  ObjetRif(LFld(comerc|OT_CLIENTE), IFld(comerc|OT_OBJET)) ? "R" : "E", i);
		i++;
	}
	DeleteCursor(c_OT);
	return TRUE;
}

private void ClearMulti()
{
	for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, EMP, i); i++) {
		FmSetFld (fm0, EMPR,     NULL_STR,   i);
		FmSetFld (fm0, TIPCOMPR, NULL_STR,   i);
		FmSetFld (fm0, SERIER,   NULL_STR,   i);
		FmSetFld (fm0, DELEGR,   NULL_STR,   i);
		FmSetLFld(fm0, NROOTR,   NULL_LONG,  i);
		FmSetLFld(fm0, CLIENTER, NULL_LONG,  i);
		FmSetIFld(fm0, OBJETR,   NULL_SHORT, i);
		FmSetFld (fm0, TIPSERR,  NULL_STR,   i);
		FmSetDFld(fm0, FOKVTAR,  NULL_DATE,  i);
		FmSetTFld(fm0, HOKVTAR,  NULL_TIME,  i);
		FmSetFld (fm0, USUCOMR,  NULL_STR,   i);
		FmSetDFld(fm0, FOKOPERR, NULL_DATE,  i);
		FmSetTFld(fm0, HOKOPERR, NULL_TIME,  i);
		FmSetFld (fm0, USUOPER,  NULL_STR,   i);
	}
}
