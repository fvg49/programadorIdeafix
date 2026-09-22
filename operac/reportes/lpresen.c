/********************************************************************
* MODULE & VERSION : @(#)lpresen.c	1.7
* DATE             : 04/12/06
* TIME             : 12:55:37
*
* CREATED          : 29/09/00
*
* DESCRIPTION:
*             Listado de Presentismo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lpresen.fmh"
#include "lpresen.rph"
#include "lpresen2.rph"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "operac.h"
#include "comerc.h"
#include "filial.h"

#define MAXPOS 80000

struct vigilador {
	long presen;
	DATE dia;
	long cliente;
	int  obj;
	TIME hentra;
	TIME hsalid;
	long nroleg;
	char regimen[12];
	char condic[1];
}pvig[MAXPOS], *uvig = pvig, *evig;

/* Funciones privadas */
static void CargarDatos();
static void AbrirSalida();
static void CargarVigilador(long presen, DATE fecha, long cliente, int obj, TIME hentra, TIME hsalid,
							long nroleg, char *regimen, char *condic);
static int  compvig(struct vigilador *a, struct vigilador *b);
static int compcli(struct vigilador *a, struct vigilador *b);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void DatosToRp();
static void ImprimirDatos();
static void GenerarArchivo();

/* Declaraciones globales */
FILE   *fp1;
form   fm0;
report rp0;
schema operac, bill, comerc;
bool presen, program, svisor;

/* Programa principal */
wcmd(lpresen, 1.7 12/06/04)
{
	fm0    = OpenForm("lpresen",  FM_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while (DoForm(fm0, before, after) != FM_UPDATE) return;

	presen = FALSE, program = FALSE, svisor = FALSE;

	CargarDatos();
	ImprimirDatos();

	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void CargarDatos()
{
	dbcursor c_parte, c_obj;
	DATE fecha;
	bool encontro = FALSE;

	c_parte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

	if (*FmSFld(fm0, OPCION) == 'P') {
		c_obj = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
		SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);

		presen = TRUE;
	}
	else {
		if (*FmSFld(fm0, OPCION) == 'G') {
			c_obj = CreateCursor(comerc|OBJETIVObyPROGRAM, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);

			program = TRUE;
		}
		else {
			if (*FmSFld(fm0, OPCION) == 'S') {
				c_obj = CreateCursor(comerc|OBJETIVObySVISOR, IO_NOT_LOCK);
				SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
				SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);

				svisor = TRUE;
			}
			else {
				c_obj = CreateCursor(comerc|OBJETIVO, IO_NOT_LOCK);
				SetCursorFrom(c_obj, FmLFld(fm0, CLIED), FmIFld(fm0, OBJETD));
				SetCursorTo  (c_obj, FmLFld(fm0, CLIEH), FmIFld(fm0, OBJETH));
			}
		}
	}
	while (FetchCursor(c_obj) != ERROR) {
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		       	continue;
		
		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		
		if (IFld(comerc|OBJETIVO_EMP) != FmIFld(fm0, EMP))
			continue;

		for (fecha = FmDFld(fm0, FECHAD); fecha <= FmDFld(fm0, FECHAH); fecha++) {
			encontro = FALSE;

			SetCursorFrom(c_parte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), 
								   IFld(comerc|OBJETIVO_OBJET), fecha, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE),
								   IFld(comerc|OBJETIVO_OBJET), fecha, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(c_parte) != ERROR) {
				char regimen[12];
				encontro = TRUE;

				GetRegimenEfectivo(IFld(PARTE_EMP), LFld(PARTE_NROLEG), regimen, fecha);
				CargarVigilador(presen  ? LFld(comerc|OBJETIVO_PRESEN)  :
								program ? LFld(comerc|OBJETIVO_PROGRAM) :
								svisor  ? LFld(comerc|OBJETIVO_SVISOR)  : NULL_LONG,
								fecha, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO),
								TFld(PARTE_HORAENT), TFld(PARTE_HORASAL), LFld(PARTE_NROLEG), regimen,
								SFld(PARTE_CONDIC));
			}
			if (!encontro) {
				CargarVigilador(presen  ? LFld(comerc|OBJETIVO_PRESEN)  :
								program ? LFld(comerc|OBJETIVO_PROGRAM) :
								svisor  ? LFld(comerc|OBJETIVO_SVISOR)  : NULL_LONG,
								fecha, LFld(comerc|OBJETIVO_CLIENTE),
								IFld(comerc|OBJETIVO_OBJET), NULL_TIME, NULL_TIME, NULL_LONG, NULL_STR,
								NULL_STR);
			}
		}
	}
}

static void ImprimirDatos()
{
	if (*FmSFld(fm0, SALIDA) != 'C')
		AbrirSalida();

	if (*FmSFld(fm0, OPCION) == 'P') {
		qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)compvig);

		if (*FmSFld(fm0, SALIDA) != 'C')
			RpSetIFld(rp0, (*FmSFld(fm0, SALIDA)=='A'? RPMODO2 : RPMODO1), 1);
	}
	
	if (*FmSFld(fm0, OPCION) == 'C') {
		qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)compcli);

		if (*FmSFld(fm0, SALIDA) != 'C')
			RpSetIFld(rp0, (*FmSFld(fm0, SALIDA)=='A'? RPMODO2 : RPMODO1), 2);
	}
	
	if (*FmSFld(fm0, SALIDA) == 'C')
		GenerarArchivo();
	else
		DatosToRp();
}

static void CargarVigilador(long presen, DATE fecha, long cliente, int obj, TIME hentra, TIME hsalid,
							long nroleg, char *regimen, char *condic)
{
	if (uvig == &pvig[MAXPOS])
		Error("Tabla interna saturada. Max %d", MAXPOS);

	uvig->presen  = presen;
	uvig->dia     = fecha;
	uvig->cliente = cliente;
	uvig->obj     = obj;
	uvig->hentra  = hentra;
	uvig->hsalid  = hsalid;
	uvig->nroleg  = nroleg;
	strcpy (uvig->regimen, regimen);
	strcpy (uvig->condic,  condic);
	uvig ++;
}

static int compvig(struct vigilador *a, struct vigilador *b)
{
	return a->presen  > b->presen  ? 1 : a->presen  < b->presen  ? -1 :
		   a->dia     > b->dia     ? 1 : a->dia     < b->dia     ? -1 :
		   a->cliente > b->cliente ? 1 : a->cliente < b->cliente ? -1 :
		   a->obj     > b->obj     ? 1 : a->obj     < b->obj     ? -1 :
		   a->hentra  > b->hentra  ? 1 : a->hentra  < b->hentra  ? -1 :
		   a->hsalid  > b->hsalid  ? 1 : a->hsalid  < b->hsalid  ? -1 :
		   a->nroleg  > b->nroleg  ? 1 : a->nroleg  < b->nroleg  ? -1 :
		   0;
}

static int compcli(struct vigilador *a, struct vigilador *b)
{
	return
		   a->dia     > b->dia     ? 1 : a->dia     < b->dia     ? -1 :
		   a->cliente > b->cliente ? 1 : a->cliente < b->cliente ? -1 :
		   a->obj     > b->obj     ? 1 : a->obj     < b->obj     ? -1 :
		   a->hentra  > b->hentra  ? 1 : a->hentra  < b->hentra  ? -1 :
		   a->hsalid  > b->hsalid  ? 1 : a->hsalid  < b->hsalid  ? -1 :
		   a->nroleg  > b->nroleg  ? 1 : a->nroleg  < b->nroleg  ? -1 :
		   0;
}

static void AbrirSalida()
{
	//Si la salida es Impresora
	if (*FmSFld(fm0, SALIDA) == 'I') {
		rp0 = OpenReport("lpresen", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	}

	//Si la salida es Terminal
	if (*FmSFld(fm0, SALIDA) == 'T') {
		rp0 = OpenReport("lpresen", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	}

	//Si la salida es Archivo
	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("lpresen2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0,  RP_IO_FILE, FmSFld(fm0, ARCHIVO));
	}
	BeginReport(rp0, 1, NULL_STR);

}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIED:
	   	InicClientesXusr();
    	break;
    case CLIEH:
    	break;
    case OBJETD:
	   	InicObjetivosXusr(FmLFld(fm, CLIED, row), FmIFld(fm, EMP, row));
    	break;
    case OBJETH:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIEH, row), FmIFld(fm, EMP, row));
    	break;
	case FFILIAL:
		break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
   		if (FmChgFld(fm))
		    InicListaXusr(FmIFld(fm0, EMP));
    break;
	case OPCION :
		if (FmChgFld(fm) && *FmSFld (fm, OPCION) == 'C') {
			FmClearFlds (fm, NROLEGD, APYNOMH);
		} 
		if (FmChgFld(fm) && *FmSFld (fm, OPCION) == 'P') {
			FmClearFlds (fm, CLIED, DOBJH);
		} 
		break;
	case SALIDA:
		if (FmChgFld(fm) && (*FmSFld (fm, SALIDA) == 'A' || *FmSFld (fm, SALIDA) == 'C') &&
			FmIsNull (fm, ARCHIVO)) {
			FmSetFld (fm , ARCHIVO, "lpresen.txt");
		} 
		break;
	case CLIED:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
   		else 
		  	FmSetFld(fm, DCLIED, GetDescCliente(FmLFld(fm, CLIED, row)), row);
    break;
    case CLIEH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIEH, GetDescCliente(FmLFld(fm, CLIEH, row)), row);
   	break;
    case OBJETD:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIED, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIED, row), FmIFld(fm, OBJETD, row)), row);
	break;
    case OBJETH:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIEH, row));
  		else
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row), FmIFld(fm, OBJETH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;
}
static void DatosToRp()
{
	int stdhs;
	char auxtime[6];
	int canhs=0;

	long cliant=NULL_LONG;
	int  objant=NULL_SHORT;
	DATE fecant=NULL_DATE;
	stdhs=0;
	for (evig = pvig; evig < uvig; evig++) {


		if (cliant!= evig->cliente ||
			objant!= evig->obj ||
			fecant!= evig->dia) {
//			fprintf(stderr, "	Salto\n\n\n");
			
			if (stdhs!=0 && canhs==0) {
				RpClearZone(rp0, (*FmSFld(fm0,SALIDA)=='A'? ZLINEA2 	: ZLINEA1));

				RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HDESDE2	: R_HDESDE1), "XX:XX");
				RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HHASTA2	: R_HHASTA1), "XX:XX");
				RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_APENOM2	: R_APENOM1), "Falta Programar Vigilador");
				DoReport (rp0, (*FmSFld(fm0,SALIDA)=='A'? ZLINEA2 	: ZLINEA1));

			}
			canhs=0;
		}
		stdhs=StdHr(FmIFld(fm0, EMP), evig->cliente, evig->obj, NULL_SHORT, evig->dia, evig->dia);

//		fprintf(stderr, "%ld\t%d\t%D\t%.1T\t%.1T\t%ld\n", evig->cliente, evig->obj, evig->dia, evig->hentra, evig->hsalid, evig->nroleg);

		RpSetLFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_SUPERV2	: R_SUPERV1), evig->presen);
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_DSUP2	: R_DSUP1),   GetNombreLeg(FmIFld(fm0, EMP), evig->presen));
		RpSetDFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_FECHA2	: R_FECHA1),  evig->dia);
		SetLFld(bill|CLIENTE_CLIENTE, evig->cliente);
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		RpSetLFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_CLI2	: R_CLI1),    evig->cliente);
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_DCLI2	: R_DCLI1),   SFld(bill|CLIENTE_RAZSOC));				
		RpSetLFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_STD2	: R_STD1),    stdhs);
		RpSetIFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_OBJ2	: R_OBJ1),    evig->obj);
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_DOBJ2	: R_DOBJ1),   GetObjDescrip(evig->cliente, evig->obj));
		TToStr(evig->hentra, auxtime, TFMT_SEPAR);
		RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HDESDE2	: R_HDESDE1), auxtime);
		TToStr(evig->hsalid, auxtime, TFMT_SEPAR);


		RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HHASTA2	: R_HHASTA1), auxtime);
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_REGIM2	: R_REGIM1),  evig->regimen);
		RpSetLFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_LEGAJO2	: R_LEGAJO1), evig->nroleg);
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_APENOM2	: R_APENOM1), GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg));
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_CONDIC2	: R_CONDIC1), evig->condic);

		DoReport (rp0, (*FmSFld(fm0,SALIDA)=='A'? ZLINEA2 	: ZLINEA1));

		canhs+=GetCantHoras(evig->hentra, evig->hsalid);

		cliant = evig->cliente;
		objant = evig->obj;
		fecant = evig->dia;

	}
	if (stdhs!=0 && canhs==0) {

		RpClearZone(rp0, (*FmSFld(fm0,SALIDA)=='A'? ZLINEA2 	: ZLINEA1));
		RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HDESDE2	: R_HDESDE1), "XX:XX");
		RpSetFld(rp0, (*FmSFld(fm0,SALIDA)=='A'? R_HHASTA2	: R_HHASTA1), "XX:XX");
		RpSetFld (rp0, (*FmSFld(fm0,SALIDA)=='A'? R_APENOM2	: R_APENOM1), "Falta Programar Vigilador");
		DoReport (rp0, (*FmSFld(fm0,SALIDA)=='A'? ZLINEA2 	: ZLINEA1));

	}

	CloseReport(rp0);
}

static void GenerarArchivo()
{
	char nomleg[61], nompre[61];
	int stdhs;

	int canhs=0;

	long cliant=NULL_LONG;
	int  objant=NULL_SHORT;
	DATE fecant=NULL_DATE;
	stdhs=0;

	if ((fp1 = fopen(FmSFld(fm0, ARCHIVO), "wt")) == (FILE *) NULL)
		Error("No se puede generar el archivo %s", FmSFld(fm0, ARCHIVO));

	fprintf(fp1, "Cliente\tRazon Social\tObjetivo\tDescrip\tPres/Svisor/Program\tNombre\tDia\tVigilador\tNombre\tRegimen\tCondic\tHr Entrada\tHr Salida\n");

	for (evig = pvig; evig < uvig; evig++) {

		if (cliant!= evig->cliente ||
			objant!= evig->obj ||
			fecant!= evig->dia) {
			
			if (stdhs!=0 && canhs==0) {

				SetLFld(bill|CLIENTE_CLIENTE, cliant);
				GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

				fprintf(fp1, "%ld\t", cliant);
				fprintf(fp1, "%s\t", SFld(bill|CLIENTE_RAZSOC));
				fprintf(fp1, "%d\t", objant);
				fprintf(fp1, "%s\t\t\t", GetObjDescrip(cliant, objant));
				fprintf(fp1, "%.3D\t\t", fecant);
				fprintf(fp1, "Falta Programar Vigilador\t\t\t");
				fprintf(fp1, "XX:XX\t");
				fprintf(fp1, "XX:XX\t");
				fprintf(fp1, "%d\t%d\n", stdhs, canhs);
			}
			canhs=0;
		}
		stdhs=StdHr(FmIFld(fm0, EMP), evig->cliente, evig->obj, NULL_SHORT, evig->dia, evig->dia);

		SetLFld(bill|CLIENTE_CLIENTE, evig->cliente);
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		strcpy(nomleg, GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg));
		strcpy(nompre, GetNombreLeg(FmIFld(fm0, EMP), evig->presen));

		fprintf(fp1, "%ld\t", evig->cliente);
		fprintf(fp1, "%s\t", SFld(bill|CLIENTE_RAZSOC));
		fprintf(fp1, "%d\t", evig->obj);
		fprintf(fp1, "%s\t", GetObjDescrip(evig->cliente, evig->obj));
		fprintf(fp1, "%ld\t", evig->presen);
		fprintf(fp1, "%s\t", nompre);
		fprintf(fp1, "%.3D\t", evig->dia);
		fprintf(fp1, "%ld\t", evig->nroleg);
		fprintf(fp1, "%s\t", nomleg);
		fprintf(fp1, "%s\t", evig->regimen);
		fprintf(fp1, "%s\t",  evig->condic);
		fprintf(fp1, "%.1T\t", evig->hentra);
		fprintf(fp1, "%.1T\n", evig->hsalid);

		canhs+=GetCantHoras(evig->hentra, evig->hsalid);
		cliant = evig->cliente;
		objant = evig->obj;
		fecant = evig->dia;

	}
	if (stdhs!=0 && canhs==0) {

		SetLFld(bill|CLIENTE_CLIENTE, cliant);
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		fprintf(fp1, "%ld\t", cliant);
		fprintf(fp1, "%s\t", SFld(bill|CLIENTE_RAZSOC));
		fprintf(fp1, "%d\t", objant);
		fprintf(fp1, "%s\t\t\t", GetObjDescrip(cliant, objant));
		fprintf(fp1, "%.3D\t\t", fecant);
		fprintf(fp1, "Falta Programar Vigilador\t\t\t");
		fprintf(fp1, "XX:XX\t");
		fprintf(fp1, "XX:XX\n");
	}

	fclose(fp1);
}
