/*******************************************************************
* MODULE & VERSION : @(#)lsinalta.c	1.1
* DATE             : 04/09/07
* TIME             : 18:54:17
*
* DENOMINACION     : lsinalta
*
* DESCRIPCION      : 
*                   Listado que informa los vigiladores que estan trabajando
*                   sin alta en la jurisdiccion del Objetivo.  
*
* TABLA         |  OPERACION
*---------------+---------------------------------------------------
*
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.sch"
#include "operac.sch"
#include "lsinalta.fmh"
#include "lsinalta.rph"

#define	_MAX_VIG		90000

struct t_vigil {
	long nroleg, cliente;
	int objet, juris;
	DATE fecasig;
} pvig[_MAX_VIG], *uvig=pvig, *evig;

void AbrirSalida();
static void ImprimirVig();
static void ProcesarVig();
static void CargarVig(long legajo, long cliente, int objet, int juris, DATE fecasig);

/* Declaraciones globales */
schema com, ope;
FILE *fp = NULL;
form fm0;
report rp0;
bool haydatos = FALSE;
char msg[100];

/* Programa principal */
wcmd(lsinalta, 1.1 09/07/04)
{
	fm_cmd cmd;

	fm0 = OpenForm("lsinalta", FM_EABORT);
	com = OpenSchema("comerc", IO_EABORT);
	ope = OpenSchema("operac", IO_EABORT);

	if ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT) {
		ProcesarVig();

		if (!haydatos)
			Warning("No hay datos para emitir el listado.");
		else {
			AbrirSalida();
			ImprimirVig();
		}
		FmSetFld (fm0, COMENT, NULL_STR);
		WiRefresh();
	}
}

static void ProcesarVig()
{
	dbcursor c_obj, c_asig;
	int juris;

	c_obj  = CreateCursor(com|OBJETIVObyEMP, IO_NOT_LOCK);
	c_asig = CreateCursor(ope|ASIGbyEMP,     IO_NOT_LOCK);

	SetCursorFrom(c_obj, FmIFld(fm0, EMP), !FmIsNull(fm0, CLID) ? FmLFld(fm0, CLID) : MIN_LONG,
										   !FmIsNull(fm0, OBJD) ? FmIFld(fm0, OBJD) : MIN_SHORT);
	SetCursorTo	 (c_obj, FmIFld(fm0, EMP), !FmIsNull(fm0, CLIH) ? FmLFld(fm0, CLIH) : MAX_LONG,
										   !FmIsNull(fm0, OBJH) ? FmLFld(fm0, OBJH) : MAX_SHORT);
	while (FetchCursor(c_obj) != ERROR) {
		if (!FmIsNull(fm0, PROV) && IFld(com|OBJETIVO_PROV) != FmIFld(fm0, PROV))
			continue;

		sprintf (msg, "Procesando Cliente %ld Objetivo %d", LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET));
		FmSetFld (fm0, COMENT, msg);
		WiRefresh();

		juris = GetJurisdiccion(LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET));

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
								MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo	 (c_asig, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
								MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			if (!InscriptoEnJuris(FmIFld(fm0, EMP), LFld(ope|ASIG_NROLEG), StrToI(ReadEnv("PAIS")), juris)) {
				haydatos = TRUE;

				CargarVig(LFld(ope|ASIG_NROLEG), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
							juris, DFld(ope|ASIG_FECASIG));
			}
		}
	}
	DeleteCursor(c_obj);
	DeleteCursor(c_asig);
}

static void CargarVig(long legajo, long cliente, int objet, int juris, DATE fecasig)
{
	bool repetido = FALSE;

	for (evig = pvig; evig < uvig; evig++) {
		if (evig->nroleg == legajo && evig->cliente == cliente && evig->objet == objet)
			repetido = TRUE;
	}
		
	if (!repetido) {
		if (evig == uvig) {
			if (uvig == &pvig[_MAX_VIG])
				Error("Tabla interna saturada. Max %d", _MAX_VIG);

			uvig->nroleg  = legajo;
			uvig->cliente = cliente;
			uvig->objet   = objet;
			uvig->juris   = juris;
			uvig->fecasig = fecasig;

			uvig++;
		}
	}
}

void AbrirSalida()
{
	if (*FmSFld(fm0, SALIDA) != 'A') {
		if (*FmSFld(fm0, SALIDA) == 'I') {
			rp0 = OpenReport("lsinalta", RP_EABORT|RP_NOBEGIN);
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		}

		if (*FmSFld(fm0, SALIDA) == 'T') {
			rp0 = OpenReport("lsinalta", RP_EABORT|RP_NOBEGIN);
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		}

		BeginReport(rp0, 1, NULL_STR);
	}
	else {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO), "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));

		fprintf(fp, "Legajo\tNombre\tCliente\tRazon Social\tObjetivo\tDescripcion\tJurisdiccion\tDescripcion\tFecha Asig\n");
	}
}

static void ImprimirVig()
{
	for (evig = pvig; evig < uvig; evig++) {
		if (*FmSFld(fm0, SALIDA) != 'A') {
			RpSetLFld(rp0, R_NROLEG,  evig->nroleg);
			RpSetFld (rp0, R_NOMBRE,  GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg));
			RpSetLFld(rp0, R_CLIENTE, evig->cliente);
			RpSetFld (rp0, R_RAZSOC,  GetDescCli(evig->cliente));
			RpSetIFld(rp0, R_OBJET,   evig->objet);
			RpSetFld (rp0, R_DOBJET,  GetObjDescrip(evig->cliente, evig->objet));
			RpSetIFld(rp0, R_JURIS,   evig->juris);
			RpSetFld (rp0, R_DJURIS,  GetDescProv(StrToI(ReadEnv("PAIS")), evig->juris));
			RpSetDFld(rp0, RFECASIG,  evig->fecasig);
			DoReport (rp0, LINEA);
		}
		else {
			fprintf(fp, "%ld\t%s\t%ld\t%s\t%d\t%s\t%d\t%s\t%.3D\n",
				evig->nroleg,  GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg),
				evig->cliente, GetDescCli(evig->cliente),
				evig->objet,   GetObjDescrip(evig->cliente, evig->objet),
				evig->juris,   GetDescProv(StrToI(ReadEnv("PAIS")), evig->juris),
				evig->fecasig);
		}
		sprintf (msg, "Generando Reporte de Salida. Vigilador %ld  %s", evig->nroleg, GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg));
		FmSetFld (fm0, COMENT, msg);
		WiRefresh();
	}
}
