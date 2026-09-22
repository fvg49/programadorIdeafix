/********************************************************************
*
* MODULE & VERSION : %W% 
* DATE             : %E% 
* TIME             : %U% 
*
* CREATED          : 22/12/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lhsregim.fmh"
#include "lhsregi1.rph"
#include "lhsregi2.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "comerc.h"
#include "operac.h"

#define ERR_ARCHI	"No se pudo abrir el archivo de salida."
#define MAX_LEG		10000

static void	AbrirArchivo();
static void	ImprimirArchivo();
static void AbrirReporte();
static void GenerarReporte();
static void	ImprimirReporte();
static void AcumuloHoras(long nroleg, double horas);
static bool MinHoras(long nroleg);

/* Declaraciones globales */
form fm0;
report rp;
schema comerc, operac, bill, sue;
FILE *fp;

struct Legajo {
	long nroleg;
	DATE dia;
	long cliente;
	int  objet;
	TIME hsent;
	TIME hssal;
	double horas;
}leg[MAX_LEG];

struct Totales {
	long nroleg;
	double horas;
}tot[MAX_LEG];
int vec = 0;
int vec1= 0;

static int comparar(struct Legajo *a, struct Legajo *b);

/* Programa principal */
wcmd(lhsregim, %I% %G%)
{
	fm0    = OpenForm("lhsregim", FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

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
	if(*FmSFld(fm0, OPCION) == 'T')
		rp = OpenReport("lhsregi1", RP_EABORT|RP_NOBEGIN);
	else
	rp = OpenReport("lhsregi2", RP_EABORT|RP_NOBEGIN);

	if ( *FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp, RP_IO_DEFAULT, NULL_STR );

	if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp, RP_IO_TERM, NULL_STR );
	BeginReport(rp, 1, NULL_STR);
	RpSetFld (rp, RREGIM,  FmSFld(fm0, REGIM));
	RpSetDFld(rp, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp, RFECHAH, FmDFld(fm0, FECHAH));
}

static void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, ARCHIVO),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
}

static void GenerarReporte()
{
	dbcursor c_parte = (dbcursor) ERROR;
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);

	SetCursorFrom(c_parte, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_parte) != ERROR) {
		if(DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;
		if(StrCmp(SFld(operac|PARTE_CONDIC), "T"))
			continue;

		SetKey(operac|ASIGbyEMP, IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE),
								 IFld(operac|PARTE_OBJETIVO), LFld(operac|PARTE_NROLEG), 
								 IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), 
								 IFld(operac|PARTE_NROINT));
		(void)GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK);
		if(StrCmp(SFld(operac|ASIG_REGIM), FmSFld(fm0, REGIM)))
			continue;

		leg[vec].nroleg = LFld(operac|PARTE_NROLEG);
		leg[vec].dia    = DFld(operac|PARTE_DIA);
		leg[vec].cliente= LFld(operac|PARTE_CLIENTE);
		leg[vec].objet  = IFld(operac|PARTE_OBJETIVO);
		leg[vec].hsent  = TFld(operac|PARTE_HORAENT);
		leg[vec].hssal  = TFld(operac|PARTE_HORASAL);
		leg[vec].horas  = ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL))*100;
		AcumuloHoras(leg[vec].nroleg, leg[vec].horas);
		vec++;
		if(vec == MAX_LEG)
			Error("El vector esta saturado");
	}
	qsort((char *)leg, (unsigned)(vec), sizeof(leg[0]), (IFPVCPVCP)comparar);
}

static void AcumuloHoras(long nroleg, double horas)
{
	int i;

	for(i=0; i<vec1; i++) {
		if(tot[i].nroleg == nroleg) {
			tot[i].horas += horas;
			return;
		}
	}
	tot[vec1].nroleg = nroleg;
	tot[vec1].horas  = horas;
	vec1++;
}

static int comparar(struct Legajo *a, struct Legajo *b)
{
	return	a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
			a->dia    < b->dia    ? -1 : a->dia    > b->dia    ? 1 :
			a->cliente< b->cliente? -1 : a->cliente> b->cliente? 1 :
			a->objet  < b->objet  ? -1 : a->objet  > b->objet  ? 1 :
	0;
}

static bool MinHoras(long nroleg)
{
	int i;
	bool esta = FALSE;

	for(i=0; i < vec1; i++) {
		if(tot[i].nroleg != nroleg)
			continue;
		if(tot[i].horas > FmLFld(fm0, CANT))
			continue;
		esta = TRUE;
	}
	return esta;
}

static void	ImprimirReporte()
{
	int i;

	if (*FmSFld(fm0, OPCION) == 'D') {
		for (i=0; i<vec; i++) {
			if(!MinHoras(leg[i].nroleg))
				continue;

			RpSetLFld(rp, R_NROLEG, leg[i].nroleg);
			RpSetDFld(rp, R_DIA,    leg[i].dia);
			RpSetLFld(rp, R_CLIENTE,leg[i].cliente);
			RpSetIFld(rp, R_OBJET,  leg[i].objet);
			RpSetTFld(rp, R_HORENT, leg[i].hsent);
			RpSetTFld(rp, R_HORSAL, leg[i].hssal);
			RpSetLFld(rp, R_HORAS,  leg[i].horas);

			SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), leg[i].nroleg);
			(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
			SetKey(bill|CLIENTEbyCLIENTE, leg[i].cliente);
			(void)GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

			RpSetFld (rp, R_NOMBRE, SFld(sue|PER_APYNOM));
			RpSetFld (rp, R_DCLIENTE, SFld(bill|CLIENTE_RAZSOC));
			RpSetFld (rp, R_DOBJET,  GetObjDescrip(leg[i].cliente, leg[i].objet));
			DoReport(rp, LINEA1);
		}
	}
	if (*FmSFld(fm0, OPCION) == 'T') {
		for (i=0; i<vec1; i++) {
			if(!MinHoras(tot[i].nroleg))
				continue;

			RpSetLFld(rp, RNROLEG, tot[i].nroleg);
			SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), tot[i].nroleg);
			(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);

			RpSetFld (rp, RNOMBRE, SFld(sue|PER_APYNOM));
			RpSetLFld(rp, RHORAS,  tot[i].horas);
			DoReport(rp, LINEA);
		}
	}
}

static void	ImprimirArchivo()
{
	int i;

	if (*FmSFld(fm0, OPCION) == 'D') {
		fprintf(fp, "Legajo\tNombre\tDia\tCliente\t\tObjetivo\t\tHsEnt\tHsSal\tHoras\n");
		for (i=0; i<vec; i++) {
			if(!MinHoras(leg[i].nroleg))
				continue;

			SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), leg[i].nroleg);
			(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
			SetKey(bill|CLIENTEbyCLIENTE, leg[i].cliente);
			(void)GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

			fprintf(fp, "%ld\t%30s\t%D\t%ld\t%20s\t%d\t%10s\t%T\t%T\t%.2lf\n", 
						 leg[i].nroleg, SFld(sue|PER_APYNOM), leg[i].dia, leg[i].cliente,
						 SFld(bill|CLIENTE_RAZSOC), leg[i].objet, 
						 GetObjDescrip(leg[i].cliente, leg[i].objet), leg[i].hsent, leg[i].hssal, leg[i].horas/100.0);
		}
	}
	if (*FmSFld(fm0, OPCION) == 'T') {
		fprintf(fp, "Legajo\tNombre\tHoras\n");
		for (i=0; i<vec1; i++) {
			if(!MinHoras(tot[i].nroleg))
				continue;

			SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), tot[i].nroleg);
			(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
			fprintf(fp, "%ld\t%30s\t%.2lf\n", tot[i].nroleg, SFld(sue|PER_APYNOM),tot[i].horas/100.0);
		}
	}
}
