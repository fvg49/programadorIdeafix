/********************************************************************
* MODULE & VERSION : @(#)apromt.c	1.1
* DATE             : 06/05/15
* TIME             : 12:38:37
*
* CREATED          : 17/01/02
*
* DESCRIPTION:
*      Aprobación de Margen Teórico.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "apromt.fmh"
#include "comerc.sch"
#include "motrecmt.fmh"

/* Defines */
#define PENDIENTE		0
#define APROBADO		1
#define RECHAZADO		2
#define MAX_CLIENTE		300
#define MAX_OBJET		300
#define ESPORADICO		2
#define MT				4

/* Funciones privadas */
static bool SetMulti();
static void SetMultiMT(), StoreInOt(), ClearMulti();
static void SeteoMulti(int emp, int tipcomp, int serie, char * deleg, long nroot, char *abm, long cliente,
					   int objet, DATE fecreg, int estadm, int estoper, int estvta, int tope, DATE fokvta,
					   TIME hokvta, DATE fokoper, TIME hokoper, char *usucom, char *usuoper, int plazo);
bool ValidoEstadoOt(short modelo, bool estadm, bool estoper, bool estvta);
bool ValidoEstadoOtc(bool estadm, bool estoper, bool estvta);
short ModeloComprobante(short tipcomp);

/* Declaraciones globales */
int tope;
form fm0, fm1;
schema comerc;
dbcursor c_OT;
bool comercial, operaciones;

/* Programa principal */
wcmd(apromt, 1.1 05/15/06)
{
	fm_cmd cmd;
	fm0 = OpenForm("apromt", FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT | IO_SYMBOLS);

	comercial   = FALSE;
	operaciones = FALSE;
	tope = 0;

	if (!SetMulti())
		Error("El Usuario no pertenece a un Grupo autorizado para realizar Aprobaciones");
	SetMultiMT();

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		StoreInOt();
		ClearMulti();
		SetMultiMT();
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
}

static bool SetMulti()
{
	int ok = 0;    /* No lectura */

	if (UsrInGrupo(GRPOPER, GetUid()) || UsrInGrupo(GRPSUPOPER, GetUid())) {
		c_OT = CreateCursor(comerc|OTbyAPROBO, IO_NOT_LOCK);
		operaciones = TRUE;
		ok = 1;
		FmSetIFld(fm0, I_USUCOM, FALSE);
	}
	else {
		if (UsrInGrupo(GRPCOMER, GetUid())) {
			c_OT = CreateCursor(comerc|OTbyAPROBV, IO_NOT_LOCK);
			comercial = TRUE;
			ok = 1;
			FmSetIFld(fm0, I_USUCOM, TRUE);
		}
	}
	if (!ok) 
		return FALSE;
	return TRUE;
}

static void SetMultiMT()
{
	SetCursorFrom(c_OT, PENDIENTE, MIN_SHORT, MIN_SHORT, MIN_SHORT, LOW_VALUE,  MIN_LONG);
	SetCursorTo  (c_OT, PENDIENTE, MAX_SHORT, MAX_SHORT, MAX_SHORT, HIGH_VALUE, MAX_LONG);
	for (tope = 0; tope < FmFldLen(fm0, MULTI) && FetchCursor(c_OT) != ERROR;) {
		short modelo;
		modelo = ModeloComprobante (IFld(comerc|OT_TIPCOMP));

		if (modelo != MODOTC)
			continue;
		if (IFld(comerc|OT_TIPCOMP) != MT)
			continue;
		if (!ValidoEstadoOt(modelo, IFld(comerc|OT_ESTADM), IFld(comerc|OT_ESTOPER), IFld(comerc|OT_ESTVTA)))
			continue;

		SeteoMulti(IFld(comerc|OT_EMP),     IFld(comerc|OT_TIPCOMP), IFld(comerc|OT_SERIE), 
				   SFld(comerc|OT_DELEG),   LFld(comerc|OT_NROOT),   SFld(comerc|OT_ABM), 
				   LFld(comerc|OT_CLIENTE), IFld(comerc|OT_OBJET),   DFld(comerc|OT_FECREG),
				   IFld(comerc|OT_ESTADM),  IFld(comerc|OT_ESTOPER), IFld(comerc|OT_ESTVTA), tope,
				   DFld(comerc|OT_FOKVTA),  TFld(comerc|OT_HOKVTA),  DFld(comerc|OT_FOKOPER),
				   TFld(comerc|OT_HOKOPER), SFld(comerc|OT_USUCOM),  SFld(comerc|OT_USUOPER),
				   IFld(comerc|OT_PLAZO));
		tope++;
	}
}

static void SeteoMulti(int emp, int tipcomp, int serie, char * deleg, long nroot, char *abm, long cliente,
					   int objet, DATE fecreg, int estadm, int estoper, int estvta, int tope, DATE fokvta,
					   TIME hokvta, DATE fokoper, TIME hokoper, char *usucom, char *usuoper, int plazo)
{
		FmSetIFld(fm0, I_EMP,     emp,     tope);
		FmSetIFld(fm0, I_TIPCOMP, tipcomp, tope);
		FmSetIFld(fm0, I_SERIE,   serie,   tope);

		SetIFld(comerc|NCBTES_TIPCOMP, tipcomp);
		GetRecord(comerc|NCBTESbyTIPCOMP, THIS_KEY, IO_NOT_LOCK);

		SetIFld(comerc|NSERIES_EMP,     emp);
		SetIFld(comerc|NSERIES_TIPCOMP, tipcomp);
		SetIFld(comerc|NSERIES_SERIE,   serie);
		GetRecord(comerc|NSERIESbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (emp == _EMP_PSA)
			FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPOT), tope);
		else
			FmSetFld(fm0, EMP, GetValParam(MOD_VIGI, EMPSAPE), tope);

		FmSetFld (fm0, TIPCOMP, SFld(comerc|NSERIES_DESCOR),  tope);
		FmSetFld (fm0, SERIE,   SFld(comerc|NCBTES_DESCOR),   tope);
		FmSetFld (fm0, DELEG,   deleg,                        tope);
		FmSetLFld(fm0, NROOT,   nroot,                        tope);
		FmSetLFld(fm0, CLIENTE, cliente,                      tope);
		FmSetIFld(fm0, OBJET,   objet,                        tope);
		FmSetDFld(fm0, FOKVTA,  fokvta,                       tope);
		FmSetTFld(fm0, HOKVTA,  hokvta,                       tope);
		FmSetFld (fm0, USUCOM,  usucom,                       tope);
		FmSetDFld(fm0, FOKOPER, fokoper,                      tope);
		FmSetTFld(fm0, HOKOPER, hokoper,                      tope);
		FmSetFld (fm0, USUOPE,  usuoper,                      tope);
		FmSetIFld(fm0, PLAZO,   plazo,                        tope);
		FmSetFld (fm0, TIPSER,  ObjetRif(cliente, objet) ? "R" : "E", tope);

		if (operaciones)
			FmSetIFld(fm0, APRUEBA, estoper, tope);
		if (comercial)
			FmSetIFld(fm0, APRUEBA, estvta,  tope);
}

static void StoreInOt()
{
	int i, j;

	for (i = 0; i < FmFldLen(fm0, MULTI) && !FmIsNull(fm0, EMP, i); i++) {
		if (FmIFld(fm0, APRUEBA, i) == 0)
			continue;

		fm1 = UseSubform(fm0, APRUEBA, 2, i);

		SetIFld(comerc|OT_EMP,     FmIFld(fm0, I_EMP,     i));
		SetIFld(comerc|OT_TIPCOMP, FmIFld(fm0, I_TIPCOMP, i));
		SetIFld(comerc|OT_SERIE,   FmIFld(fm0, I_SERIE,   i));
		SetFld (comerc|OT_DELEG,   FmSFld(fm0, DELEG,     i));
		SetLFld(comerc|OT_NROOT,   FmLFld(fm0, NROOT,     i));
		if (GetRecord( comerc|OTbyEMP, THIS_KEY, IO_LOCK) != ERROR) {
			short modelo;

			modelo = ModeloComprobante (FmIFld(fm0, I_TIPCOMP, i));

			if (modelo != MODOTC) {
				WiMsg("Comprobante %d no se sabe procesar", FmIFld(fm0, I_TIPCOMP, i));
				continue;
			}
			if (operaciones) {
				SetIFld(comerc|OT_ESTOPER,  FmIFld(fm0, APRUEBA, i));
				SetDFld(comerc|OT_FOKOPER,  Today());
				SetTFld(comerc|OT_HOKOPER,  Hour());
				SetFld (comerc|OT_USUOPER,  UserName(GetUid()));
				SetIFld(comerc|OT_PLAZO,    FmIFld(fm0, PLAZO));
				if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
					SetIFld(comerc|OT_ESTVTA, PENDIENTE);
					SetIFld(comerc|OT_ESTADM, PENDIENTE);
					for (j = 0; j < FmFldLen(fm1, MULTIREC) && !FmIsNull(fm1, OBS, j); j++) {
						SetIFld(comerc|OBSRECH_EMP,     FmIFld(fm0, I_EMP,     i));
						SetIFld(comerc|OBSRECH_TIPCOMP, FmIFld(fm0, I_TIPCOMP, i));
						SetIFld(comerc|OBSRECH_SERIE,   FmIFld(fm0, I_SERIE,   i));
						SetFld (comerc|OBSRECH_DELEG,   FmSFld(fm0, DELEG,     i));
						SetLFld(comerc|OBSRECH_NROOT,   FmLFld(fm0, NROOT,     i));
						SetIFld(comerc|OBSRECH_SERIE,   FmIFld(fm0, I_SERIE,   i));
						SetIFld(comerc|OBSRECH_NRORENG, j);
						SetFld (comerc|OBSRECH_DESCRIP, FmSFld(fm1, OBS, j));
						PutRecord(comerc|OBSRECH);
					}
				}
			}
			if (comercial) {
				SetIFld(comerc|OT_ESTVTA,   FmIFld(fm0, APRUEBA, i));
				SetIFld(comerc|OT_ESTADM,   PENDIENTE);
				SetIFld(comerc|OT_ESTOPER,  PENDIENTE);
				SetDFld(comerc|OT_FOKVTA,   Today());
				SetTFld(comerc|OT_HOKVTA,   Hour());
				SetFld (comerc|OT_USUCOM,   UserName(GetUid()));
				if (FmIFld(fm0, APRUEBA, i) == RECHAZADO) {
					for (j = 0; j < FmFldLen(fm1, MULTIREC) && !FmIsNull(fm1, OBS, j); j++) {
						SetIFld(comerc|OBSRECH_EMP,     FmIFld(fm0, I_EMP,     i));
						SetIFld(comerc|OBSRECH_TIPCOMP, FmIFld(fm0, I_TIPCOMP, i));
						SetIFld(comerc|OBSRECH_SERIE,   FmIFld(fm0, I_SERIE,   i));
						SetFld (comerc|OBSRECH_DELEG,   FmSFld(fm0, DELEG,     i));
						SetLFld(comerc|OBSRECH_NROOT,   FmLFld(fm0, NROOT,     i));
						SetIFld(comerc|OBSRECH_SERIE,   FmIFld(fm0, I_SERIE,   i));
						SetIFld(comerc|OBSRECH_NRORENG, j);
						SetFld (comerc|OBSRECH_DESCRIP, FmSFld(fm1, OBS, j));
						PutRecord(comerc|OBSRECH);
					}
				}
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
		FmSetLFld(fm0, CLIENTE,   NULL_LONG,  i);
		FmSetIFld(fm0, OBJET,     NULL_SHORT, i);
		FmSetIFld(fm0, APRUEBA,   NULL_SHORT, i);
		FmSetFld (fm0, DAPRUEB,   NULL_STR,   i);
		FmSetFld (fm0, TIPSER,    NULL_STR,   i);
		FmSetDFld(fm0, FOKVTA,    NULL_DATE,  i);
		FmSetTFld(fm0, HOKVTA,    NULL_TIME,  i);
		FmSetFld (fm0, USUCOM,    NULL_STR,   i);
		FmSetDFld(fm0, FOKOPER,   NULL_DATE,  i);
		FmSetTFld(fm0, HOKOPER,   NULL_TIME,  i);
		FmSetFld (fm0, USUOPE,    NULL_STR,   i);
		FmSetIFld(fm0, PLAZO,     NULL_SHORT, i);
	}
}

bool ValidoEstadoOt ( short modelo, bool estadm, bool estoper, bool estvta)
{
	if (modelo == MODOTC)
		return ValidoEstadoOtc (estadm, estoper, estvta);

	return FALSE;
}

bool ValidoEstadoOtc ( bool estadm, bool estoper, bool estvta)
{
	if (operaciones)
		if (estvta != APROBADO)
			return FALSE;

	return TRUE;
}

short ModeloComprobante (short tipcomp) 
{
	switch (tipcomp) {
		case  OTCOMERC: return MODOTC;
		case  MTCOMERC: return MODOTC;
	}
	return NULL_SHORT;
}
