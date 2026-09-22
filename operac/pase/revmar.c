/********************************************************************
* MODULE & VERSION : @(#)revmar.c	1.5
* DATE             : 02/04/16
* TIME             : 16:17:17
*
* DESCRIPTION:
*    Reversion del pase de Operaciones a Facturación.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "comdef.h"
#include "billpro.h"
#include "comerc.sch"
#include "comgral.sch"
#include "bill.sch"
#include "revmar.fmh"

/* Funciones privadas */
private fm_status before(form fm, fmfield fno, int row);
private fm_status after(form fm, fmfield fno, int row);
void BorrarNovedad();
bool ObjetivoValido(long cliente, short objetivo);

/* Declaraciones globales */
form   fm0;
schema comerc, bill, comgral;
FILE *fp_error;
char bufaux[50];

wcmd(revmar, 1.5 04/16/02)
{
	fm0    = OpenForm("revmar", FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);
	comgral= OpenSchema("comgral", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

 	FmSetIFld(fm0, I_GRUPO, GrupoFact(GrupoUsr(GetUid())));

	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	if ((fp_error = fopen ("revmar.log", "wt")) == (FILE *) NULL) Error ("No se pudo abrir el archiv de Log");
	fprintf(fp_error, "Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fprintf(fp_error, "Liquidacion %ld Cliente %ld %d hasta %ld %d \n",
					FmLFld(fm0, NROLIQ), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), 
					FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));

	fprintf(fp_error, "Empieza a borrar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	BeginTransaction();
	BorrarNovedad();
	EndTransaction();

	fprintf(fp_error, "FIN PROCESO Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fclose(fp_error);
}

private fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROLIQ:
		break;
	}
	return FM_OK;
}

private fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROLIQ:  
		SetKey(bill|LIQUIDbyNROLIQ, FmLFld(fm0, NROLIQ));
		if (GetRecord(bill|LIQUIDbyNROLIQ, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (DFld(bill|LIQUID_FCHCIE) != NULL_DATE) {
				Warning("Esta liquidación está cerrada ");
				return FM_REDO;
			}
		}
		else {
			FmSetFld(fm, DESCLIQ, "LIQUIDACION INEXISTENTE");
		}
		break;
	}
	return FM_OK;
}

void BorrarNovedad()
{
	dbcursor c_obj, c_valide;

	c_obj    = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);
	c_valide = CreateCursor(comgral|VALIFEbyNROLIQ, IO_NOT_LOCK);

	SetCursorFrom(c_obj, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
	SetCursorTo  (c_obj, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
	while (FetchCursor(c_obj) != ERROR) {

		if (IsNull(comerc|OBJETIVO_INTERN))
			continue;

		if (!ObjetivoValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

		//index nroliq (nroliq, intern, nrovar, fecvig);
		SetCursorFrom(c_valide, FmLFld(fm0, NROLIQ), LFld(comerc|OBJETIVO_INTERN), NULL_SHORT, NULL_DATE);
		SetCursorTo  (c_valide, FmLFld(fm0, NROLIQ), LFld(comerc|OBJETIVO_INTERN), MAX_SHORT, MAX_DATE);
		while (FetchCursor(c_valide) != ERROR) {
			SetKey(bill|VARNOVbyNROLIQ, LFld(comgral|VALIFE_NROLIQ), LFld(comgral|VALIFE_INTERN), IFld(comgral|VALIFE_NROVAR));
			if (GetRecord(bill|VARNOVbyNROLIQ, THIS_KEY, IO_NOT_LOCK) != ERROR)
				DelRecord(bill|VARNOV);

			DelRecord(comgral|VALIFE);
		}
	}
	DeleteCursor(c_obj);
	DeleteCursor(c_valide);
}

bool ObjetivoValido (long cliente, short objetivo)
{ 
	if (cliente < FmLFld(fm0, CLID))
		return FALSE;

	if (cliente > FmLFld(fm0, CLIH))
		return FALSE;

	if (cliente == FmLFld(fm0, CLID) && objetivo < FmIFld(fm0, OBJD))
		return FALSE;

	if (cliente == FmLFld(fm0, CLIH) && objetivo > FmIFld(fm0, OBJH))
		return FALSE;

	return TRUE;
}

