/********************************************************************
*
* MODULE & VERSION : @(#)lvacac.c	1.10 
* DATE             : 07/10/23 
* TIME             : 16:42:20 
*
* CREATED          : 13/10/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lvacac.fmh"
#include "lvacac.rph"
#include "operac.sch"
#include "sue.sch"
#include "comerc.h"
#include "operac.h"

#define MAXCLI  6
#define ERR_ARCHI  "No se pudo abrir el archivo!"
#define ARCHI     0
#define TERM      1
#define IMPRE     2
#define CABARCH	"Cliente\tDescripción del Cliente\tObjetivo\tDescripción del Objetivo\tVigilador\tNombre y Apellido\tPeriodo\tFecha Desde\tFecha Hasta\tDías\n"
#define DATARCH "%ld\t%s\t%d\t%s\t%ld\t%s\t%d\t%.3D\t%.3D\t%d\n"

struct cliente {
	long cli;
	int	 obj;
} cli[MAXCLI];

/* Funciones privadas */
static fm_status before(form, fmfield, int);
static void PrintHead(void);
static void Imprimir();
static void InicTarifa();
static void ObtenerCliObj(long nroleg);
static void AbrirSalida();

/* Declaraciones globales */
form fm0;
FILE   *fp = NULL;
report rp = ERROR;
bool   salida;
schema sue, oper;

/* Programa principal */
wcmd(lvacac, 1.10 10/23/07)
{
	dbcursor c_VAC = (dbcursor) ERROR;

	fm0  = OpenForm("lvacac",   FM_EABORT);
	sue  = OpenSchema("sue",    IO_EABORT);
	oper = OpenSchema("operac", IO_EABORT);

	if (DoForm(fm0, before, NULLFP) != FM_UPDATE) return;

	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;

	if (*FmSFld(fm0, OPCION) == 'L') {
		if (FmIsNull(fm0, PERIODO)) {
			c_VAC = CreateCursor(oper|VACACbyEMP, IO_NOT_LOCK);

			SetCursorFrom(c_VAC, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGD), MIN_DATE);
			SetCursorTo  (c_VAC, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGH), MAX_DATE);
		}
		else {
			c_VAC = CreateCursor(oper|VACACbyPERIODO, IO_NOT_LOCK);
			
			SetCursorFrom(c_VAC, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGD), FmIFld(fm0, PERIODO), MIN_DATE);
			SetCursorTo  (c_VAC, FmIFld(fm0, EMP), FmLFld(fm0, NROLEGH), FmIFld(fm0, PERIODO), MAX_DATE);
		} 

		while(FetchCursor(c_VAC) != ERROR) {
			if (!FmIsNull(fm0, PERIODO) && IFld(oper|VACAC_PERIODO) != FmIFld(fm0, PERIODO))
				continue;

			if (FmIsNull(fm0, PERIODO)) {
				if (!((Month(DFld(oper|VACAC_FDESDE)) == FmIFld(fm0, MES) &&
					   Year(DFld(oper|VACAC_FDESDE))  == FmIFld(fm0, ANIO)) ||
					  (Month(DFld(oper|VACAC_FHASTA)) == FmIFld(fm0, MES) &&
					   Year(DFld(oper|VACAC_FHASTA)) == FmIFld(fm0, ANIO))))
					continue;
			}
			Imprimir();
		}
	}
	else {
		c_VAC = CreateCursor(oper|VACACbyFECHA, IO_NOT_LOCK);

		SetCursorFrom(c_VAC, FmIFld(fm0, EMP), FmDFld(fm0, FECHA), MIN_LONG);
		SetCursorTo  (c_VAC, FmIFld(fm0, EMP), FmDFld(fm0, FECHA), MAX_LONG);
		while(FetchCursor(c_VAC) != ERROR) {
			InicTarifa();
			Imprimir();
		}
	}
	if (rp == ERROR && fp == NULL)
		Warning("No hay datos para emitir el listado.");
}

static void PrintHead(void)
{
	RpSetIFld(rp, RMES,     FmIFld(fm0, MES));
	RpSetIFld(rp, RANIO,    FmIFld(fm0, ANIO));
	RpSetLFld(rp, RNROLEGD, FmLFld(fm0, NROLEGD));
	RpSetDFld(rp, RFECREG,  FmDFld(fm0, FECHA));
	RpSetFld (rp, RAPYNOMD, FmSFld(fm0, APYNOMD));
	RpSetLFld(rp, RNROLEGH, FmLFld(fm0, NROLEGH));
	RpSetFld (rp, RAPYNOMH, FmSFld(fm0, APYNOMH));
}

static void Imprimir()
{

	int i;
	long totvac = 0;

	if (rp == ERROR && fp == NULL)
		AbrirSalida();

	totvac = DFld(oper|VACAC_FHASTA) - DFld(oper|VACAC_FDESDE) + 1;

	if (rp != ERROR)	{
	
		RpSetLFld(rp, RNROLEG,  LFld(oper|VACAC_NROLEG));
		RpSetFld (rp, RAPYNOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(oper|VACAC_NROLEG)));
		RpSetIFld(rp, RPERIODO, IFld(oper|VACAC_PERIODO));
		RpSetDFld(rp, RFDESDE,  DFld(oper|VACAC_FDESDE));
		RpSetDFld(rp, RFHASTA,  DFld(oper|VACAC_FHASTA));
		RpSetLFld(rp, RTOTVAC,  totvac);

		if (*FmSFld(fm0, OPCION) == 'F') {
			ObtenerCliObj(LFld(oper|VACAC_NROLEG));
			for (i = 0; i < MAXCLI; i++) {
				RpSetLFld(rp, RCLIENTE1 + (2 * i), cli[i].cli);
				RpSetIFld(rp, ROBJ1 + (2 * i),     cli[i].obj);
			}
		}
		else {
			for (i = 0; i < MAXCLI; i++) {
				RpSetLFld(rp, RCLIENTE1 + (2 * i), NULL_LONG);
				RpSetIFld(rp, ROBJ1 + (2 * i),     NULL_SHORT);
			}
		}
		DoReport(rp, ZLINEA);
	}
	
	if (fp != NULL)	{
		if (*FmSFld(fm0, OPCION) == 'F') {
			ObtenerCliObj(LFld(oper|VACAC_NROLEG));
			for (i = 0; i < MAXCLI; i++) {
				if (cli[i].cli != NULL_LONG && cli[i].obj != NULL_SHORT)
					fprintf(fp, DATARCH, cli[i].cli, GetDescCli(cli[i].cli),  cli[i].obj, GetObjDescrip(cli[i].cli, cli[i].obj), LFld(oper|VACAC_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(oper|VACAC_NROLEG)), IFld(oper|VACAC_PERIODO), DFld(oper|VACAC_FDESDE), DFld(oper|VACAC_FHASTA), totvac);
			}
		}
		else {
				fprintf(fp, DATARCH, NULL_LONG, NULL_STR, NULL_SHORT, NULL_STR, LFld(oper|VACAC_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(oper|VACAC_NROLEG)), IFld(oper|VACAC_PERIODO), DFld(oper|VACAC_FDESDE), DFld(oper|VACAC_FHASTA), totvac);
		}
	}
}

static void ObtenerCliObj(long nroleg)
{
	int i = 0;

	SetIFld(oper|ASIG_EMP,      FmIFld(fm0, EMP));
	SetLFld(oper|ASIG_NROLEG,   nroleg);
	SetLFld(oper|ASIG_CLIENTE,  MIN_LONG);
	SetIFld(oper|ASIG_OBJETIVO, MIN_SHORT);
	while (GetRecord(oper|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (!IsNull(oper|ASIG_FECHAS) && DFld(oper|ASIG_FECHAS) < DFld(oper|VACAC_FDESDE))
			continue;
		if (DFld(oper|ASIG_FECASIG) > DFld(oper|VACAC_FHASTA))
			continue;

		cli[i].cli = LFld(oper|ASIG_CLIENTE);
		cli[i].obj = IFld(oper|ASIG_OBJETIVO);
		i++;
	}
	SetIFld(oper|ASIGH_EMP,      FmIFld(fm0, EMP));
	SetLFld(oper|ASIGH_NROLEG,   nroleg);
	SetLFld(oper|ASIGH_CLIENTE,  MIN_LONG);
	SetIFld(oper|ASIGH_OBJETIVO, MIN_SHORT);
	while (GetRecord(oper|ASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (!IsNull(oper|ASIGH_FECBAJ) && DFld(oper|ASIGH_FECBAJ) < DFld(oper|VACAC_FDESDE))
			continue;
		if (DFld(oper|ASIGH_FECALT) > DFld(oper|VACAC_FHASTA))
			continue;

		cli[i].cli = LFld(oper|ASIGH_CLIENTE);
		cli[i].obj = IFld(oper|ASIGH_OBJETIVO);
		i++;
	}
}

static void InicTarifa()
{
	int i;

	for (i = 0; i < MAXCLI; i++) {
		cli[i].cli = NULL_LONG;
		cli[i].obj = NULL_SHORT;
	}
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));

   	   	fprintf(fp, CABARCH);
	}
	else {
		rp = OpenReport("lvacac", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
		if (BeginReport(rp, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
		PrintHead();
	}
}

static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lvacac.txt");
	break;
	}
	return FM_OK;
}
