/********************************************************************
*
* MODULE & VERSION : @(#)ctrlfac.c	1.1
* DATE             : 01/10/02
* TIME             : 15:16:33
*
*
* DESCRIPTION:
*	Este proceso se encarga de ver que todos los dias se paso al modulo de facturación.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* COMENTARIO: Si se modifica funcionalidad actualizar ctrlfac.hlp
*********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "operac.h"
#include "ctrlfac.fmh"
#include "ctrlfac.rph"

//Define
#define _MENSAJE_PARTE		1
#define _MENSAJE_RETRO		2

// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);
static void RecorrerParte();
static void RecorrerRetro();
bool ObjetivoValido (long cliente, short objetivo);
private struct s_errores *CargarError(short codigo, long cliente, short objet, DATE fecha);
private void BorroListaErrores ();
void ImprimirErrores();

struct s_errores {
	short tipoerr;
	long cliente;
	short objet;
	DATE dia;
	struct s_errores *next;
};

// Variables globales.
form fm0;
struct s_errores *p_error=NULL, *aux_error;

// Programa principal
wcmd(ctrlfac, 1.1 10/02/01)
{
	fm0 = OpenForm("ctrlfac", FM_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	RecorrerParte();
	RecorrerRetro();
	ImprimirErrores();
}

static void RecorrerParte()
{
	DATE dia = NULL_DATE;
	dbcursor c_parte;

	c_parte = CreateCursor(PARTEbyDIA, IO_NOT_LOCK);
	SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (dia != DFld(PARTE_DIA)) {
			FmSetDFld(fm0, COMENTARIO,  DFld(PARTE_DIA));
			FmSetFld (fm0, DCOMENTARIO, "Parte");
			WiRefresh();
	 		dia = DFld(PARTE_DIA);
		}

		if (!ObjetivoValido (LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))) {
			continue;
		}

		if (LFld(PARTE_LIQFAC) == NULL_LONG){
			CargarError(_MENSAJE_PARTE, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), DFld(PARTE_DIA));
		}
	}
	FmSetDFld(fm0, COMENTARIO,  NULL_DATE);
	FmSetFld (fm0, DCOMENTARIO, NULL_STR);
	WiRefresh();
	DeleteCursor(c_parte);
}

static void RecorrerRetro()
{
	DATE dia = NULL_DATE;
	dbcursor c_retro;

	c_retro = CreateCursor(RETRObyRDIA, IO_LOCK);
	SetCursorFrom(c_retro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_retro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_retro) != ERROR) {
		if (dia != DFld(RETRO_DIA)) {
			FmSetDFld(fm0, COMENTARIO,  DFld(RETRO_DIA));
			FmSetFld (fm0, DCOMENTARIO, "Retroactivos");
			WiRefresh();
	 		dia = DFld(RETRO_DIA);
		}

		if (!ObjetivoValido (LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO))) {
			continue;
		}

		if (LFld(RETRO_LIQFAC) == NULL_LONG){
			CargarError(_MENSAJE_RETRO, LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA));
		}
	}
	FmSetDFld(fm0, COMENTARIO,  NULL_DATE);
	FmSetFld (fm0, DCOMENTARIO, NULL_STR);
	WiRefresh();
	DeleteCursor(c_retro);
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case CLID:
			break;
	}
	return FM_OK;
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

private struct s_errores *CargarError(short codigo, long cliente, short objet, DATE fecha)
{
	bool existe = FALSE;
	struct s_errores *ant, *aux, *pnew;

	pnew = (struct s_errores *) Alloc(sizeof(struct s_errores));
	pnew->tipoerr = codigo;
	pnew->cliente = cliente;
	pnew->objet = objet;
	pnew->dia  = fecha;
	pnew->next    = NULL;

	if (p_error == NULL ) {
		p_error = pnew;
		return p_error;
	}

	for (ant = p_error, aux = p_error; aux != NULL; ant=aux, aux = aux->next) {

		if ( aux->cliente > cliente)	break;
		if ( aux->cliente == cliente && aux->objet > objet) break;
		if ( aux->cliente == cliente && aux->objet == objet && aux->dia > fecha ) break;
		if ( aux->cliente == cliente && aux->objet == objet && aux->dia == fecha){
				existe = TRUE;
				break;
		}
	}

	if (existe) {
		free (pnew);
		return aux;
	} 	

	if (aux == p_error) {
		p_error = pnew;
		pnew->next = aux;
		return pnew;
	}

    pnew->next = ant->next;
	ant->next  = pnew;

	return pnew;
}

void ImprimirErrores()
{
	char buferr[200];
	report rp0;
	
	rp0 = OpenReport("ctrlfac", RP_EABORT);
	
	RpSetLFld(rp0, RCLID, FmLFld(fm0, CLID));
	RpSetFld (rp0, RDCLID, FmSFld(fm0, DCLID));
	RpSetLFld(rp0, RCLIH, FmLFld(fm0, CLIH));
	RpSetFld (rp0, RDCLIH, FmSFld(fm0, DCLIH));
	RpSetIFld(rp0, ROBJD, FmIFld(fm0, OBJD));
	RpSetFld (rp0, RDOBJD, FmSFld(fm0, DOBJD));
	RpSetIFld(rp0, ROBJH, FmIFld(fm0, OBJH));
	RpSetFld (rp0, RDOBJH, FmSFld(fm0, DOBJH));
	RpSetDFld (rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld (rp0, RFECHAH, FmDFld(fm0, FECHAH));

	for (aux_error = p_error; aux_error != NULL; aux_error = aux_error->next) {
     
     	switch (aux_error->tipoerr){
     	case  _MENSAJE_PARTE : 
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d Dia %.3D. No se paso el parte.", aux_error->cliente, aux_error->objet, aux_error->dia);
		break;
		case _MENSAJE_RETRO : 
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d Dia %.3D. No se paso retroactivos ", aux_error->cliente, aux_error->objet, aux_error->dia);
		break;
		default : 
			sprintf (buferr, NULL_STR);
		break;
		}

		RpSetFld(rp0, RDETALLE, buferr);
		DoReport(rp0, LINEA);
	}
	BorroListaErrores ();
	EndReport(rp0);
}

private void BorroListaErrores ()
{
	struct s_errores *eq, *eqaux;

	for (eq = p_error ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	p_error = NULL;
}

