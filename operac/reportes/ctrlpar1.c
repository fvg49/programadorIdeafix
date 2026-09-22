/********************************************************************
*
* MODULE & VERSION : @(#)ctrlpar1.c	1.1
* DATE             : 11/01/05
* TIME             : 10:36:18
*
* CREATED          : 
*
* DESCRIPTION:
*        Listado para comparar Hs. Normales Trabajadas contra Hs. Normales de Regimen
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "ctrlpar1.fmh"
#include "ctrlpa11.rph"
#include "ctrlpa12.rph"

#define ERR_ARCHI	"No se pudo abrir el archivo de salida."
#define MAX_LEG		100000

/* Estructuras */
struct Legajo {
	long   nroleg;
	DATE   dia;
	long   cliente;
	int    objet;
	char   regimen[15];
	double hsnor;
	double toths;
}leg[MAX_LEG];

struct Totales {
	long   nroleg;
	DATE   dia;
	double hsnor, toths;
}tot[MAX_LEG];

static void AbrirArchivo();
static void ImprimirArchivo();
static void AbrirReporte();
static void GenerarReporte();
static void ImprimirReporte();
static void AcumuloHoras(long nroleg, double hsnor, double toths, DATE dia);
static int  comparar(struct Legajo *a, struct Legajo *b);
static int  ordenartot(struct Totales *a, struct Totales *b);

/* Declaraciones globales */
FILE *fp;
form fm0;
report rp;
schema comerc, operac, bill, sue;
int vec = 0, vec1 = 0;
char regimen[15];
char bufferstr[50];

/* Programa principal */
wcmd(ctrlpar1, 1.1 01/05/11)
{
	fm0    = OpenForm  ("ctrlpar1", FM_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);
	sue    = OpenSchema("sue",      IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	GenerarReporte();

	if (*FmSFld(fm0, SALIDA) == 'A') {
		AbrirArchivo();
		ImprimirArchivo();
	}
	else {
		AbrirReporte();
		ImprimirReporte();
	}
}

static void AbrirReporte()
{
	if (*FmSFld(fm0, OPCION) == 'T')
		rp = OpenReport("ctrlpa11", RP_EABORT|RP_NOBEGIN);
	else
		rp = OpenReport("ctrlpa12", RP_EABORT|RP_NOBEGIN);

	if (*FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp, RP_IO_DEFAULT, NULL_STR );

	if (*FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp, RP_IO_TERM, NULL_STR );

	BeginReport(rp, 1, NULL_STR);

	RpSetDFld(rp, R_FECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp, R_FECHAH, FmDFld(fm0, FECHAH));
}

static void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, ARCHIVO),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
}

static void GenerarReporte()
{
	dbcursor c_parte = (dbcursor) ERROR;
	c_parte = CreateCursor(operac|PARTEbyDIA, IO_NOT_LOCK);

	SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_parte) != ERROR) {
		if (LFld(operac|PARTE_NROLEG) < FmLFld(fm0, VIGILD) ||
			LFld(operac|PARTE_NROLEG) > FmLFld(fm0, VIGILH))
			continue;

		if (IFld(operac|PARTE_HSNOR)  == 0 && IFld(operac|PARTE_HS50)    == 0 &&
			IFld(operac|PARTE_HS100F) == 0 && IFld(operac|PARTE_HS100FE) == 0)
			continue;

		sprintf(bufferstr, "Procesando DÍa %.1D", DFld(operac|PARTE_DIA));
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();

		leg[vec].nroleg  = LFld(operac|PARTE_NROLEG);
		leg[vec].dia     = DFld(operac|PARTE_DIA);
		leg[vec].cliente = LFld(operac|PARTE_CLIENTE);
		leg[vec].objet   = IFld(operac|PARTE_OBJETIVO);
		leg[vec].hsnor   = IFld(operac|PARTE_HSNOR);
		leg[vec].toths   = (IFld(operac|PARTE_HSNOR) + IFld(operac|PARTE_HS50) + IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE));
		strcpy(leg[vec].regimen, regimen);

		AcumuloHoras(leg[vec].nroleg, leg[vec].hsnor, leg[vec].toths, leg[vec].dia);
		vec++;

		if (vec == MAX_LEG)
			Error("El vector esta saturado");
	}
	qsort((char *)leg, (unsigned)(vec), sizeof(leg[0]), (IFPVCPVCP)comparar);
	qsort((char *)tot, (unsigned)(vec1), sizeof(tot[0]), (IFPVCPVCP)ordenartot);
}

static void AcumuloHoras(long nroleg, double hsnor, double toths, DATE dia)
{
	int i;

	for (i = 0; i < vec1; i++) {
		if (tot[i].nroleg == nroleg && tot[i].dia == dia) {
			tot[i].hsnor += hsnor;
			tot[i].toths += toths;
			return;
		}
	}
	tot[vec1].nroleg = nroleg;
	tot[vec1].hsnor  = hsnor;
	tot[vec1].toths  = toths;
	tot[vec1].dia    = dia;
	vec1++;
}

static int ordenartot(struct Totales *a, struct Totales *b)
{
	return	a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
			a->dia    < b->dia    ? -1 : a->dia    > b->dia    ? 1 :
			a->hsnor  < b->hsnor  ? -1 : a->hsnor  > b->hsnor  ? 1 : 0;
}

static int comparar(struct Legajo *a, struct Legajo *b)
{
	return	a->nroleg  < b->nroleg  ? -1 : a->nroleg  > b->nroleg  ? 1 :
			a->dia     < b->dia     ? -1 : a->dia     > b->dia     ? 1 :
			a->cliente < b->cliente ? -1 : a->cliente > b->cliente ? 1 :
			a->objet   < b->objet   ? -1 : a->objet   > b->objet   ? 1 : 0;
}

static void	ImprimirReporte()
{
	int i, j;

	sprintf(bufferstr, "Generando Impresion");
	FmSetFld(fm0, COMENT, bufferstr);
	WiRefresh();

	for (i = 0; i < vec1; i++) {
		GetRegimenEfectivo(FmIFld(fm0, EMP), tot[i].nroleg, regimen, tot[i].dia);

		SetFld(comerc|REGIMEN_REGIM, regimen);
		GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK);

		if (!strcmp(regimen, _REGIMEN_1x1x1))
			continue;


		if (tot[i].hsnor <= GetHsNormales(regimen, IFld(comerc|REGIMEN_PARTIME)))
			continue;


		sprintf(bufferstr, "Imprimiendo Vigilador %ld",tot[i].nroleg);
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();

		for (j = 0; j < vec && (leg[j].nroleg != tot[i].nroleg || leg[j].dia != tot[i].dia); j++);

		if (*FmSFld(fm0, OPCION) == 'D') {
			for (; j < vec && leg[j].nroleg == tot[i].nroleg && leg[j].dia == tot[i].dia; j++) {
				RpSetLFld(rp, R_NROLEG,   leg[j].nroleg);
				
				RpSetFld (rp, R_NOMBRE,   GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg));
				RpSetDFld(rp, R_DIA,      leg[j].dia);
				RpSetLFld(rp, R_CLIENTE,  leg[j].cliente);
				RpSetFld (rp, R_DCLIENTE, GetDescCli(leg[j].cliente));
				RpSetIFld(rp, R_OBJET,    leg[j].objet);
				RpSetFld (rp, R_DOBJET,   GetObjDescrip(leg[j].cliente, leg[j].objet));
				RpSetFld (rp, R_REGIMEN,  regimen);
				RpSetLFld(rp, R_HORAS,    leg[j].hsnor);
				DoReport (rp, LINEA1);
			}
		}


		if (*FmSFld(fm0, OPCION) == 'T') {
			RpSetLFld(rp, R_NROLEG, tot[i].nroleg);
			RpSetFld (rp, R_NOMBRE, GetNombreLeg(FmIFld(fm0, EMP), tot[i].nroleg));
			RpSetDFld(rp, R_DIA,    tot[i].dia);
			RpSetFld (rp, R_REGIM,  regimen);
			RpSetFld (rp, R_OBS,    "HSNORMALES");
			RpSetLFld(rp, R_TOTHS,  tot[i].hsnor);
			DoReport (rp, LINEA);
		}


	}
}

static void	ImprimirArchivo()
{
	int i, j;

	sprintf(bufferstr, "Generando Archivo");
	FmSetFld(fm0, COMENT, bufferstr);
	WiRefresh();

	for (i = 0; i < vec1; i++) {
		GetRegimenEfectivo(FmIFld(fm0, EMP), tot[i].nroleg, regimen, tot[i].dia);

		SetFld(comerc|REGIMEN_REGIM, regimen);
		GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK);

		if (!strcmp(regimen, _REGIMEN_1x1x1))
			continue;


		if (tot[i].hsnor <= GetHsNormales(regimen, IFld(comerc|REGIMEN_PARTIME)))
			continue;


		sprintf(bufferstr, "Imprimiendo Vigilador %ld",tot[i].nroleg);
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();

		for (j = 0; j < vec && (leg[j].nroleg != tot[i].nroleg || leg[j].dia != tot[i].dia); j++);

		if (*FmSFld(fm0, OPCION) == 'D') {
			for (; j < vec && leg[j].nroleg == tot[i].nroleg && leg[j].dia == tot[i].dia; j++) {


				fprintf(fp, "%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%.2lf\n", 
						leg[j].nroleg,  GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente),
						leg[j].objet, 	GetObjDescrip(leg[j].cliente, leg[j].objet),
						regimen,        leg[j].hsnor / 100.0);


			}
		}
		if (*FmSFld(fm0, OPCION) == 'T') {

			fprintf(fp, "%ld\t%30s\t%.2lf\n", tot[i].nroleg, GetNombreLeg(FmIFld(fm0, EMP), tot[i].nroleg),
											  regimen, tot[i].hsnor / 100.0);
		}
	}
}
