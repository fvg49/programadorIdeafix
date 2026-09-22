/********************************************************************
* MODULE & VERSION : @(#)cuadrant.c	1.1
* DATE             : 07/03/05
* TIME             : 16:16:37
*
* CREATED          : 01/03/07
*
* DESCRIPTION:
*              Impresion de cuadrantes.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "operac.sch"
#include "comerc.sch"
#include "cuadrant.fmh"

typedef struct stncliente * tncliente;
typedef struct stnobjetivo * tnobjetivo;
typedef struct stnpuesto * tnpuesto;
typedef struct stnnroleg * tnnroleg;
typedef struct stndia * tndia;

typedef struct stncliente {
	long	cliente;
	tnobjetivo	nobjetivo;
	tncliente	nsig;
} stncliente;

typedef struct stnobjetivo {
	int		objetivo;
	tnpuesto	npuesto;
	tnobjetivo	nsig;
} stnobjetivo;

typedef struct stnpuesto {
	char	puesto[10];
	int		tippto;
	int		codint;
	tnnroleg	nnroleg;
	tnpuesto	nsig;
} stnpuesto;

typedef struct stnnroleg {
	long	nroleg;
	tndia	ndia;
	tnnroleg	nsig;
} stnnroleg;

typedef struct stndia {
	DATE	dia;
	char	condic;
	TIME	hrent;
	TIME	hrsal;
	tndia	nsig;
} stndia;

/* Funciones Privadas */
static fm_status after(form, fmfield, int);
static char * ObtenerPuesto(long cli, int obj, int ptoser, int cod);
static void AbrirSalida();
static void ColumDias();
static void ColumNros();

static tncliente AcuNCliente(tncliente, tncliente*);
static tnobjetivo AcuNObjetivo(tnobjetivo, tnobjetivo*);
static tnpuesto AcuNPuesto(tnpuesto, tnpuesto*);
static tnnroleg AcuNNroleg(tnnroleg, tnnroleg*);
static tndia AcuNDia(tndia, tndia*);

static void LisNCliente(tncliente);
static void LisNObjetivo(tnobjetivo);
static void LisNPuesto(tnpuesto);
static void LisNNroleg(tnnroleg);
static void LisNDia(tndia);

static void BorNCliente(tncliente);
static void BorNObjetivo(tnobjetivo);
static void BorNPuesto(tnpuesto);
static void BorNNroleg(tnnroleg);
static void BorNDia(tndia);

tncliente	inicio;

form fm0;
FILE *fp;
schema ope, com;
int		partial;
int		objetivo, tippto, codint;
long	cliente, nroleg;
char	puesto[10], condic;
DATE	dia, primerdia, ultimodia;
TIME	hrent, hrsal;

/* Programa principal */
wcmd(cuadrant, 1.1 03/05/07)
{
	fm_cmd cmd;
	dbcursor c_obj, c_parte;

	com = OpenSchema("comerc", IO_EABORT);
	ope = OpenSchema("operac", IO_EABORT);
	fm0 = OpenForm("cuadrant", FM_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			continue;

		primerdia = FirstMonthDay(DMYToD(1, FmIFld(fm0, MES), Year(Today())));
		ultimodia = LastMonthDay(primerdia);

		AbrirSalida();

		inicio    = NULL;
		cliente   = NULL_LONG;
		objetivo  = NULL_SHORT;
		puesto[0] = '\0';
		nroleg    = NULL_LONG;
		dia       = NULL_DATE;

		c_parte = CreateCursor(ope|PARTEbyEMP, IO_NOT_LOCK);

		switch (*FmSFld(fm0, OPCION)) {
		case 'P' :
			c_obj = CreateCursor(com|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);
			break;

		case 'G' :
			c_obj = CreateCursor(com|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);
			break;

		case 'S' :
			c_obj = CreateCursor(com|OBJETIVObyPROGRAM, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);
			break;

		default :
			c_obj = CreateCursor(com|OBJETIVObyCLIENTE, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, CLIED), FmIFld(fm0, OBJETD));
			SetCursorTo  (c_obj, FmLFld(fm0, CLIEH), FmIFld(fm0, OBJETH));
		}
		while (FetchCursor(c_obj) != ERROR) {
			//Descarto los objetivos por empresa
			if (FmIFld(fm0, EMP) != IFld(com|OBJETIVO_EMP))
				continue;

			SetCursorFrom(c_parte, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
									primerdia, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
									ultimodia, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(c_parte) != ERROR) {

				cliente  = LFld(ope|PARTE_CLIENTE);
				objetivo = IFld(ope|PARTE_OBJETIVO);
				nroleg   = LFld(ope|PARTE_NROLEG);
				dia      = DFld(ope|PARTE_DIA);
				tippto   = IFld(ope|PARTE_PTOSER);
				codint   = IFld(ope|PARTE_PUESTO);
				hrent    = TFld(ope|PARTE_HORAENT);
				hrsal    = TFld(ope|PARTE_HORASAL);
				condic   = *SFld(ope|PARTE_CONDIC);
				sprintf(puesto, "%d %d", IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO));
				inicio = AcuNCliente(inicio, &inicio);
			}
		}
		LisNCliente(inicio);
		BorNCliente(inicio);
	}
}

static void AbrirSalida()
{
	if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
		Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case OPCION :
			switch(*FmSFld(fm, fno)) {
				case 'C' :
					FmSetFld(fm0, APYNOMD, NULL_STR);
					FmSetFld(fm0, APYNOMH, NULL_STR);
					break;
				case 'S' :
				case 'P' :
				case 'G' :
					FmSetFld(fm0, DCLID, NULL_STR);
					FmSetFld(fm0, DCLIH, NULL_STR);
					FmSetFld(fm0, DOBJD, NULL_STR);
					FmSetFld(fm0, DOBJH, NULL_STR);
					break;
			}
			break;
	}
	return FM_OK;
}

static char * ObtenerPuesto(long cli, int obj, int ptoser, int cod)
{
	static char dpuesto[60];

	SetLFld(ope|PUESTOS_CLIENTE, cli);
	SetIFld(ope|PUESTOS_OBJET,   obj);
	SetIFld(ope|PUESTOS_TIPPTO,  ptoser);
	SetIFld(ope|PUESTOS_CODINT,  cod);
	if (GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
		sprintf(dpuesto, "%d  %d %s  %d  %d  %.1T %.1T  %s-%s-%s-%s-%s-%s-%s  %s  %s",
						IFld(ope|PUESTOS_TIPPTO), IFld(ope|PUESTOS_CODINT), GetDescPto(IFld(ope|PUESTOS_TIPPTO)),
						IFld(ope|PUESTOS_PUESTO), IFld(ope|PUESTOS_HORAPT),
						TFld(ope|PUESTOS_HINICIO), TFld(ope|PUESTOS_HFINAL), SFld(ope|PUESTOS_DIA1),
						SFld(ope|PUESTOS_DIA2), SFld(ope|PUESTOS_DIA3), SFld(ope|PUESTOS_DIA4),
						SFld(ope|PUESTOS_DIA5), SFld(ope|PUESTOS_DIA6), SFld(ope|PUESTOS_DIA7),
						SFld(ope|PUESTOS_REGIM), SFld(ope|PUESTOS_CODFREC));
	else
		strcpy(dpuesto, NULL_STR);

	return dpuesto;
}

static void ColumNros()
{
	int i;

	for (i = 1; i <= Day(ultimodia); i++) {
		fprintf(fp, "%d", i);

		if (i == Day(ultimodia))
			fprintf(fp, "\n");
		else
			fprintf(fp, "\t\t\t");
	}
}
static void ColumDias()
{
	DATE fecha;

	for (fecha = primerdia; fecha <= ultimodia; fecha++) {
		fprintf(fp, "%c", dia(fecha));

		if (fecha == ultimodia)
			fprintf(fp, "\n");
		else
			fprintf(fp, "\t\t\t");
	}
}

static tncliente AcuNCliente(tncliente nodop, tncliente * nantp)
{
	tncliente naux;

	if (nodop == NULL) {
		nodop = (tncliente) malloc (sizeof(stncliente));

		(*nodop).cliente = cliente;
		(*nodop).nobjetivo = AcuNObjetivo(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).cliente == cliente) {
			(*nodop).nobjetivo = AcuNObjetivo((*nodop).nobjetivo, &(*nodop).nobjetivo);
		}
		else {
			if ((*nodop).cliente < cliente)
				(*nodop).nsig = AcuNCliente((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;

				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tncliente) malloc (sizeof(stncliente));
				(*nodop).cliente = cliente;

				(*nodop).nobjetivo = AcuNObjetivo(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static tnobjetivo AcuNObjetivo(tnobjetivo nodop, tnobjetivo * nantp)
{
	tnobjetivo naux;

	if (nodop == NULL) {
		nodop = (tnobjetivo) malloc (sizeof(stnobjetivo));

		(*nodop).objetivo = objetivo;
		(*nodop).npuesto = AcuNPuesto(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).objetivo == objetivo) {
			(*nodop).npuesto = AcuNPuesto((*nodop).npuesto, &(*nodop).npuesto);
		}
		else {
			if ((*nodop).objetivo < objetivo)
				(*nodop).nsig = AcuNObjetivo((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnobjetivo) malloc (sizeof(stnobjetivo));
				(*nodop).objetivo = objetivo;

				(*nodop).npuesto = AcuNPuesto(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static tnpuesto AcuNPuesto(tnpuesto nodop, tnpuesto * nantp)
{
	tnpuesto naux;

	if (nodop == NULL) {
		nodop = (tnpuesto) malloc (sizeof(stnpuesto));

		sprintf((*nodop).puesto,"%s", puesto);
		(*nodop).tippto = tippto;
		(*nodop).codint = codint;
		(*nodop).nnroleg = AcuNNroleg(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if (strcmp((*nodop).puesto, puesto)==0) {
			(*nodop).tippto = tippto;
			(*nodop).codint = codint;
			(*nodop).nnroleg = AcuNNroleg((*nodop).nnroleg, &(*nodop).nnroleg);
		}
		else {
			if (strcmp((*nodop).puesto, puesto) < 0)
				(*nodop).nsig = AcuNPuesto((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnpuesto) malloc (sizeof(stnpuesto));
				sprintf((*nodop).puesto,"%s", puesto);

				(*nodop).tippto = tippto;
				(*nodop).codint = codint;
				(*nodop).nnroleg = AcuNNroleg(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static tnnroleg AcuNNroleg(tnnroleg nodop, tnnroleg * nantp)
{
	tnnroleg naux;

	if (nodop == NULL) {
		nodop = (tnnroleg) malloc (sizeof(stnnroleg));

		(*nodop).nroleg = nroleg;
		(*nodop).ndia = AcuNDia(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nroleg == nroleg) {
			(*nodop).ndia = AcuNDia((*nodop).ndia, &(*nodop).ndia);
		}
		else {
			if ((*nodop).nroleg < nroleg)
				(*nodop).nsig = AcuNNroleg((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;

				nodop = (tnnroleg) malloc (sizeof(stnnroleg));
				(*nodop).nroleg = nroleg;
				(*nodop).ndia = AcuNDia(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static tndia AcuNDia(tndia nodop, tndia * nantp)
{
	tndia naux;

	if (nodop == NULL) {
		nodop = (tndia) malloc (sizeof(stndia));

		(*nodop).dia = dia;
		(*nodop).condic = condic;
		(*nodop).hrent = hrent;
		(*nodop).hrsal = hrsal;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).dia == dia) {
			(*nodop).condic = condic;
			(*nodop).hrent = hrent;
			(*nodop).hrsal = hrsal;
		}
		else {
			if ((*nodop).dia < dia)
				(*nodop).nsig = AcuNDia((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tndia) malloc (sizeof(stndia));
				(*nodop).dia = dia;

				(*nodop).condic = condic;
				(*nodop).hrent = hrent;
				(*nodop).hrsal = hrsal;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static void LisNCliente(tncliente nodop)
{
	if (nodop == NULL)
		return;

	cliente = (*nodop).cliente;

	if ((*nodop).nobjetivo != NULL)
		LisNObjetivo((*nodop).nobjetivo);

	if ((*nodop).nsig != NULL)
		LisNCliente((*nodop).nsig);
}

static void LisNObjetivo(tnobjetivo nodop)
{
	if (nodop == NULL)
		return;

	objetivo = (*nodop).objetivo;

	fprintf(fp, "%ld\t%s\n", cliente, GetDescCli(cliente));
	fprintf(fp, "%d\t%s\n", objetivo, GetObjDescrip(cliente, objetivo));

	if ((*nodop).npuesto != NULL)
		LisNPuesto((*nodop).npuesto);

	if ((*nodop).nsig != NULL)
		LisNObjetivo((*nodop).nsig);
}

static void LisNPuesto(tnpuesto nodop)
{
	if (nodop == NULL)
		return;

	sprintf(puesto, "%s", (*nodop).puesto);

	fprintf(fp, "\n%s\n", ObtenerPuesto(cliente, objetivo, (*nodop).tippto, (*nodop).codint));
	
	fprintf(fp, "\t\t\t");
	ColumNros();
	fprintf(fp, "\t\t\t");
	ColumDias();

	if ((*nodop).nnroleg != NULL)
		LisNNroleg((*nodop).nnroleg);

	if ((*nodop).nsig != NULL)
		LisNPuesto((*nodop).nsig);
}

static void LisNNroleg(tnnroleg nodop)
{
	if (nodop == NULL)
		return;

	nroleg = (*nodop).nroleg;

	fprintf(fp, "%ld\t%s\t", nroleg, GetNombreLeg(FmIFld(fm0, EMP), nroleg));

	if ((*nodop).ndia != NULL)
		LisNDia((*nodop).ndia);

	if ((*nodop).nsig != NULL)
		LisNNroleg((*nodop).nsig);

	if ((*nodop).ndia == NULL && (*nodop).nsig == NULL)
		fprintf(fp, "\n");
}

static void LisNDia(tndia nodop)
{
	if (nodop == NULL)
		return;

	dia = (*nodop).dia;

	fprintf(fp, "%c\t%.1T\t%.1T\t", (*nodop).condic, (*nodop).hrent, (*nodop).hrsal);

	if ((*nodop).nsig == NULL)
		fprintf(fp, "\n");

	if ((*nodop).nsig != NULL)
		LisNDia((*nodop).nsig);
}

static void BorNCliente(tncliente nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nobjetivo != NULL)
		BorNObjetivo((*nodop).nobjetivo);

	if ((*nodop).nsig != NULL)
		BorNCliente((*nodop).nsig);

	(*nodop).nobjetivo = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNObjetivo(tnobjetivo nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).npuesto != NULL)
		BorNPuesto((*nodop).npuesto);

	if ((*nodop).nsig != NULL)
		BorNObjetivo((*nodop).nsig);

	(*nodop).npuesto = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNPuesto(tnpuesto nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnroleg != NULL)
		BorNNroleg((*nodop).nnroleg);

	if ((*nodop).nsig != NULL)
		BorNPuesto((*nodop).nsig);

	(*nodop).nnroleg = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNroleg(tnnroleg nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).ndia != NULL)
		BorNDia((*nodop).ndia);

	if ((*nodop).nsig != NULL)
		BorNNroleg((*nodop).nsig);

	(*nodop).ndia = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNDia(tndia nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNDia((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}
