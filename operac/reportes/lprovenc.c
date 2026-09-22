


/********************************************************************
* MODULE & VERSION : @(#)lprovenc.c	1.1@(#)lprovenc.c
* DATE             : 11/02/17
* TIME             : 15:08:36
*
* CREATED          :
*
* DESCRIPTION:
*      Listado de Asignación Vencidas de Vigiladores Provisorios.
*
*********************************************************************/
#include <ideafix.h>
#include "lprovenc.fmh"
#include "lprovenc.rph"
#include "operac.sch"
#include "operac.h"
#include "bill.sch"
#include "comerc.sch"
#include "comerc.h"

#define MAXVIG 5000

/* Declaraciones de Estructuras */
struct vigil {
	long nroleg;
	char apynom[60];
	long cliente;
	char razsoc[60];
	int  objet;
	char dobjet[60];
	long svisor;
	char dsvisor[60];
	DATE fecasig;
	DATE fechas;
	char observ[15];
}pvig[MAXVIG], *uvig = pvig, *evig;

static fm_status after(form fm, fmfield fno, int row);
static void ProcesarCliente(long cliente, int objet, long svisor);
static void ArmarListaSup(long nroleg, long cliente, int objet, long svisor, DATE fechasig, DATE fechas);
static void ImprimirInfo();
static void AbrirSalida();
private int ordvig(struct vigil *a, struct vigil *b);

// Declaraciones globales
form   fm0;
schema operac, bill, comerc;
report rp0 = ERROR;

/* Programa principal */
wcmd(lprovenc, 1.1  02/17/11)
{
	fm_status cmd;
	dbcursor  c_OBJ, c_OBJC;

	comerc = OpenSchema("comerc",   IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);
	fm0    = OpenForm  ("lprovenc", FM_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;

	if (*FmSFld(fm0, OPCION) == 'S') {
		c_OBJ = CreateCursor(comerc|OBJETIVObySVISOR, IO_NOT_LOCK);

		SetCursorFrom(c_OBJ, FmLFld(fm0, SUPERD), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_OBJ, FmLFld(fm0, SUPERH), MAX_LONG, MAX_SHORT);
		while(FetchCursor(c_OBJ) != ERROR)
			ProcesarCliente(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							LFld(comerc|OBJETIVO_SVISOR));
		DeleteCursor(c_OBJ);
	}
	else {
		short objdesde, objhasta;
		
		c_OBJC = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);

		objdesde = FmIsNull (fm0, OBJETD)? MIN_SHORT : FmIFld(fm0, OBJETD);
		objhasta = FmIsNull (fm0, OBJETH)? MAX_SHORT : FmIFld(fm0, OBJETH);

		SetCursorFrom(c_OBJC, FmLFld(fm0, CLIED), objdesde);
		SetCursorTo  (c_OBJC, FmLFld(fm0, CLIEH), objhasta); 
		while(FetchCursor(c_OBJC) != ERROR)
			ProcesarCliente(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							LFld(comerc|OBJETIVO_SVISOR));
		DeleteCursor(c_OBJC);
	}
	ImprimirInfo();
}

static void ProcesarCliente(long cliente, int objet, long svisor)
{
	dbcursor c_asig;

	c_asig = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), cliente, objet, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), cliente, objet, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (IsNull(operac|ASIG_FECHAS) || DFld(operac|ASIG_FECHAS) > FmDFld(fm0, FHASTA))
			continue;

		ArmarListaSup(LFld(operac|ASIG_NROLEG), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO),
					  svisor, DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS));                   
					  
//		MostrarLista ();					  
	}
}

static void ArmarListaSup(long nroleg, long cliente, int objet, long svisor, DATE fechasig, DATE fechas)
{
	bool repetido = FALSE;

	for (evig = pvig; evig < uvig; evig++) {
		if (evig->nroleg  == nroleg   && evig->cliente == cliente && evig->objet == objet &&
			evig->fecasig == fechasig && evig->fechas  == fechas) {
			repetido = TRUE;
		}
	}
	if (evig == uvig && !repetido) {
		if (uvig == &pvig[MAXVIG])
			Error("Tabla interna saturada. Max %d", MAXVIG);

		uvig->nroleg  = nroleg;
		uvig->cliente = cliente;
		uvig->objet   = objet;
		uvig->svisor  = svisor;
		uvig->fecasig = fechasig;
		uvig->fechas  = fechas;
		strcpy(uvig->apynom,  GetNombreLeg(FmIFld(fm0, EMP), uvig->nroleg));
		strcpy(uvig->dsvisor, GetNombreLeg(FmIFld(fm0, EMP), uvig->svisor));
		strcpy(uvig->dobjet,  GetObjDescrip(uvig->cliente,   uvig->objet));


/*		SetKey(bill|CLIENTEbyCLIENTE, cliente);
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		strcpy(uvig->razsoc, SFld(bill|CLIENTE_RAZSOC));
*/

		if (fechas < Today())
			strcpy(uvig->observ, "Desasignar");
		else
			strcpy(uvig->observ, NULL_STR);

		uvig ++;
	}
}

static void ImprimirInfo()
{
	long svisorant = NULL_LONG, cliant = NULL_LONG, objetant = NULL_SHORT;

	qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)ordvig);

	for (evig = pvig; evig < uvig; evig++) {
		if (evig == NULL) {
			Warning("No hay datos para emitir el listado.");
			return;
		}
		if (rp0 == ERROR)
			AbrirSalida();

		if (rp0 != ERROR) {
			if (*FmSFld(fm0, OPCION) == 'S')
				RpSetIFld(rp0, FILTRO, TRUE);
			else
				RpSetIFld(rp0, FILTRO, FALSE);

			RpSetLFld(rp0, R_LEGAJO,  evig->nroleg);
			RpSetFld (rp0, R_APENOM,  evig->apynom);
			RpSetLFld(rp0, R_SVISOR,  evig->svisor);
			RpSetFld (rp0, R_DSVISOR, evig->dsvisor);
			RpSetLFld(rp0, R_CLI,     evig->cliente);

			SetKey(bill|CLIENTEbyCLIENTE, evig->cliente);
			GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
			RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
			RpSetIFld(rp0, R_OBJ,     evig->objet);
			RpSetFld (rp0, R_DOBJ,    evig->dobjet);
			RpSetDFld(rp0, R_FECASIG, evig->fecasig);
			RpSetDFld(rp0, R_FECHAS,  evig->fechas);
			RpSetFld (rp0, R_OBSERV,  evig->observ);
			if (*FmSFld(fm0, OPCION) == 'S')
				DoReport(rp0, LINSUP);
			else
				DoReport(rp0, LINCLI);
		}
	}
}

private int ordvig(struct vigil *a, struct vigil *b)
{
	if (*FmSFld(fm0, OPCION) == 'S')
		return	a->svisor  < b->svisor  ? -1 : a->svisor  > b->svisor  ? 1 :
				a->nroleg  < b->nroleg  ? -1 : a->nroleg  > b->nroleg  ? 1 :
				a->cliente < b->cliente ? -1 : a->cliente > b->cliente ? 1 :
				a->objet   < b->objet   ? -1 : a->objet   > b->objet   ? 1 :
				0;
	else
		return	a->cliente < b->cliente ? -1 : a->cliente > b->cliente ? 1 :
				a->objet   < b->objet   ? -1 : a->objet   > b->objet   ? 1 :
				a->nroleg  < b->nroleg  ? -1 : a->nroleg  > b->nroleg  ? 1 :
				0;
}

static void AbrirSalida()
{
	rp0 = OpenReport("lprovenc", RP_EABORT|RP_NOBEGIN);

	if (*FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	if (*FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);

	BeginReport(rp0, 1, NULL_STR);

	RpSetDFld(rp0, LIMITE, FmDFld(fm0, FHASTA));
}

static fm_status after(form fm, fmfield fno, int row)
{
	bool	efectivo;

	switch (fno) {
		case OPCION :
			switch(*FmSFld(fm, fno)) {
				case 'C' :
					FmSetFld(fm0, APYNOMD, NULL_STR);
					FmSetFld(fm0, APYNOMH, NULL_STR);
					break;
				case 'S' :
					FmSetFld(fm0, DCLIED, NULL_STR);
					FmSetFld(fm0, DCLIEH, NULL_STR);
					FmSetFld(fm0, DOBJD,  NULL_STR);
					FmSetFld(fm0, DOBJH,  NULL_STR);
					break;
			}
			break;
	}
	return FM_OK;
}


void MostrarLista () 
{

	for (evig = pvig; evig < uvig; evig++) {
		
		fprintf (stderr, "LEO MOSTRAR  evig->svisor %ld evig->cliente %ld evig->objet %d \n",evig->svisor,evig->cliente,evig->objet);
		
	} 
	
	
}

