/********************************************************************
*
* MODULE & VERSION : @(#)lparteo.c	1.19 
* DATE             : 08/10/06 
* TIME             : 15:16:43 
*
* CREATED          : 01/10/98
*
* DESCRIPTION:
*      Impresión del Parte Diario
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "lparteo.fmh"
#include "lparteo.rph"
#include "lparteo2.rph"
#include "aurus.sch"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"
#include "sue.sch"
#include "filial.h"


struct s_lisxusr_lib este_lista;


/* Defines */
#define MAX_VECT   400
#define MAXLINEAS  18
#define MAXCLI     3000

typedef struct {
	long cliente;
	int  obj;
	long presen;
}cliente;
cliente cli[MAXCLI];

/* Funciones privadas */
private void InicCliente();
private void CargarCliente(long cliente, int obj, long presen);
private void ImprimirCliente();
private void PrintHead(long presen, long cliente, int objet);
private void PrintLinea(dbcursor cur, DATE fecha, long cliente, int objet, bool cliobjnue, bool colum, DATE fecini);
private void PrintEncab( DATE fecdesde, long cliente, int objet);
private void SetLinea(long nroleg, DATE fecha, long cliente, int objet, int ptoser, int puesto, int nroint);
private void PrintFinal();
private void InicVect();
private bool EsLegajo(long legajo, int ptoser, int puesto, int nroint);
static int HayMov(long cliente, int objet, long presen);
static fm_status before(form fm, fmfield fno, int row);
static fm_status after(form fm, fmfield fno, int row);
static void AbrirReporte();

/* Declaraciones globales */
form   fm0;
report rp0;
schema comerc, operac, sue;
int    vec = 0;
DATE   fbegin;
bool presen, program, svisor;
struct Legajo {
	long	legajo;
	int 	ptoser;
	int		puesto;
	int 	nroint;
}leg[MAX_VECT];

int linfec = 0;

/* Programa principal */
wcmd(lparteo, 1.19 10/06/08)
{
	fm_cmd cmd;
	DATE fecha, 
		 fechaini; //En esta fecha guarda la fecha de la primer columna para imprimir el encabezado.
	int i = 0;
	char buffer[50];
	dbcursor c_PARTE, c_obj;
	bool cliobjnue = FALSE,	first = FALSE;
	long cliant = NULL_LONG;
	int  objant = NULL_SHORT;

	fm0    = OpenForm  ("lparteo", FM_EABORT);
	comerc = OpenSchema("comerc",  IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);
	sue    = OpenSchema("sue",     IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)	{

		if (cmd == FM_IGNORE)
        	continue;

		presen = FALSE, program = FALSE, svisor = FALSE;

		AbrirReporte();
		InicCliente();

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
			i=0;

			if (FmIFld(fm0, MOSTRA)==2){
 				if (ExisteCliObjEnGrp(GRPPARH, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
					continue; 
			}					

			//valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			       	continue;

			if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

			if (!HayMov(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
						presen  ? LFld(comerc|OBJETIVO_PRESEN)  :
						program ? LFld(comerc|OBJETIVO_PROGRAM) :
						svisor  ? LFld(comerc|OBJETIVO_SVISOR)  : NULL_LONG))
				continue;

			sprintf(buffer, "Procesando Cliente %ld Objetivo %d", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			FmSetFld(fm0, COMENT, buffer);
			WiRefresh();

	//		BeginReport(rp0, 1, NULL_STR);
			fecha = fbegin = FmDFld(fm0, FDESDE);
			PrintHead(presen  ? LFld(comerc|OBJETIVO_PRESEN)  :
					  program ? LFld(comerc|OBJETIVO_PROGRAM) :
					  svisor  ? LFld(comerc|OBJETIVO_SVISOR)  : NULL_LONG,
					  LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));

			if ((LFld(comerc|OBJETIVO_CLIENTE) != cliant ||
				(LFld(comerc|OBJETIVO_CLIENTE) == cliant && IFld(comerc|OBJETIVO_OBJET) != objant)) && !first) {
				RpEjectPage(rp0);
				cliant = LFld(comerc|OBJETIVO_CLIENTE);
				objant = IFld(comerc|OBJETIVO_OBJET);
				first  = FALSE;
			}

			PrintEncab(fecha, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));

			c_PARTE = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

			cliobjnue = TRUE;

			while (fecha <= FmDFld(fm0, FHASTA)) {
				if (i == 0) {
					fechaini = fecha;
					InicVect();
				} 
				
				if (i < 5) {
					PrintLinea(c_PARTE, fecha, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
										cliobjnue, FALSE, fechaini);
				}
				else {
					fechaini = fecha;
					PrintFinal();
					if (*FmSFld (fm0,SALIDA) != 'A')
						RpEjectPage(rp0);
						
					PrintEncab(fechaini, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));

					fbegin = fecha;
					i = 0;

					InicVect();
					PrintLinea(c_PARTE, fecha, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
										cliobjnue, TRUE, fechaini);
										
				}
				fecha++;
				i++;
				cliobjnue = FALSE;
			}
			if (i <= 6)
				PrintFinal();
		//		EndReport(rp0);
		}
		FmSetFld(fm0, COMENT, NULL_STR);
		WiRefresh();
		ImprimirCliente();
		
		CloseReport(rp0);		

    }
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

private void PrintHead(long presen, long cliente, int objet)
{
	int  i;
	DATE fecha;

	RpSetLFld(rp0, R_CLIE,   cliente);
	RpSetFld (rp0, R_DCLIE,  GetDescCli(cliente));
	RpSetIFld(rp0, R_OBJET,  objet);
	RpSetFld (rp0, R_DOBJET, GetObjDescrip(cliente, objet));
	RpSetLFld(rp0, R_SUPERV, presen);
	RpSetFld (rp0, R_DSUP,   GetNombreLeg(FmIFld(fm0, EMP), presen));
	RpSetDFld(rp0, R_FDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, R_FHASTA, FmDFld(fm0, FHASTA));
}

private void PrintLinea(dbcursor cur, DATE fecha, long cliente, int objet, bool cliobjnue, bool colum, DATE fecini)
{
	int cantlineas = 0;

	if (cliobjnue || colum)
		linfec = 0;
	if (linfec <= MAXLINEAS && !cliobjnue)
		cantlineas = linfec;

	SetCursorFrom(cur, FmIFld(fm0, EMP), cliente, objet, fecha, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cur, FmIFld(fm0, EMP), cliente, objet, fecha, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cur) != ERROR) {
		/* Si el Legajo ya se proceso, entonces continuo */
		if (EsLegajo(LFld(operac|PARTE_NROLEG),IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT) )) {
			continue;
		}
		if (cantlineas == MAXLINEAS && *FmSFld(fm0,SALIDA) != 'A') {
			cantlineas = 0;
			linfec     = 0;
			PrintFinal();
			RpEjectPage(rp0);
			PrintEncab(fecini, cliente, objet);
		}
		SetLinea(LFld(operac|PARTE_NROLEG), fecha, cliente, objet, IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT));
		cantlineas++;
		linfec++;
	}
}

private void SetLinea(long nroleg, DATE fecha, long cliente, int objet, int ptoser, int puesto, int nroint)
{
	int     i = 0, j = 0, tippto;
	static dbtable APAR;
	char    regimen[15];

	RpClearZone(rp0, LINEA);

	i = LtoI(fecha - fbegin);

    if (!APAR)
		APAR   = CreateAlias(operac|PARTE);

	tippto = GetTipPto(FmIFld(fm0, EMP), cliente, objet, nroleg);

	RpSetLFld(rp0, R_LEGAJO, nroleg);
	RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), nroleg));
	GetRegimenEfectivo(FmIFld(fm0, EMP), nroleg, regimen, DFld(operac|PARTE_DIA));
	RpSetFld (rp0, R_REGIM,  regimen);

	while (i < 5 && fecha <= FmDFld(fm0, FHASTA)) {
		SetKey(AlInd(APAR, operac|PARTEbyEMP), FmIFld(fm0, EMP), cliente, objet, fecha++, nroleg, ptoser, puesto, nroint);
		if (GetRecord(AlInd(APAR, operac|PARTEbyEMP), THIS_KEY, IO_NOT_LOCK) != ERROR) {
			RpSetIFld(rp0, R_PTOSER,  IFld(AlFld(APAR, operac|PARTE_PTOSER)));
			RpSetIFld(rp0, R_PUESTO,  IFld(AlFld(APAR, operac|PARTE_PUESTO)));
			RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(AlFld(APAR, operac|PARTE_PTOSER))));
			RpSetTFld(rp0, R_HSD1+(2*i), TFld(AlFld(APAR, operac|PARTE_HORAENT)));
			RpSetTFld(rp0, R_HSH1+(2*i), TFld(AlFld(APAR, operac|PARTE_HORASAL)));
		}
		else {
			RpSetTFld(rp0, R_HSD1+(2*i), NULL_TIME);
			RpSetTFld(rp0, R_HSH1+(2*i), NULL_TIME);
		}
		i++;
	}
	DoReport(rp0, LINEA); 
	RpClearZone(rp0, LINEA);
}

private void PrintEncab (DATE fecdesde, long cliente, int objet)
{
	DATE fecha;
	int i;

	fecha = fecdesde;

	for (i = 0; fecha <= FmDFld(fm0, FHASTA)  && i < 5; i++) {
		RpSetDFld(rp0, R_DIA1+(2*i), fecha);

		if (cliente == 3763 && objet == 2 && (fecha == DMYToD(16,02,2000) || fecha == DMYToD(17,02,2000)))
			RpSetLFld(rp0, R_STD1+(2*i), NULL_LONG);
		else 
			RpSetLFld(rp0, R_STD1+(2*i), StdHr(FmIFld(fm0, EMP), cliente, objet, NULL_SHORT, fecha, fecha));

		fecha++;
	}
	DoReport(rp0, ENCAB);
	RpClearZone(rp0, ENCAB);
}

private void PrintFinal()
{
	DoReport(rp0,    FLIN);
	DoReport(rp0,    TOTAL);
	DoReport(rp0,    OBSERV);
	RpClearZone(rp0, FLIN);
	RpClearZone(rp0, TOTAL);
	RpClearZone(rp0, OBSERV);
}

private bool EsLegajo(long nroleg, int ptoser, int puesto, int nroint)
{
	int i = 0;

	for (i = 0; i < vec; i++) {
		if (leg[i].legajo == nroleg && leg[i].ptoser == ptoser &&
			leg[i].puesto == puesto && leg[i].nroint == nroint) {
			return TRUE;
		}
	}
	leg[vec].legajo = nroleg;
	leg[vec].ptoser = ptoser;
	leg[vec].puesto = puesto;
	leg[vec].nroint = nroint;
	vec++;

	if (vec == MAX_VECT)
		Error("Tabla interna saturada.");

	return FALSE;
}

private void InicVect()
{
	int i;
	for(i=0 ; i<vec ; i++) {
		leg[i].legajo = 0;
		leg[i].ptoser = 0;
		leg[i].puesto = 0;
		leg[i].nroint = 0;
	}
	vec = 0;
}

static int HayMov(long cliente, int objet, long presen)
{
	int count = 0;
	dbcursor CUR = (dbcursor) ERROR;

	CUR = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
	SetCursorFrom(CUR, FmIFld(fm0, EMP), cliente, objet, FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), cliente, objet, FmDFld(fm0, FHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	count=CountCursor(CUR);

	if(!count && presen != NULL_LONG)
		CargarCliente(cliente, objet, presen);
//		WiMsg ("Cliente %ld Objetivo %d no tiene datos", cliente, objet);
	DeleteCursor(CUR);
	return count;
}

static void AbrirReporte () 
{
	//Si la salida es Impresora
	if (*FmSFld(fm0, SALIDA) == 'I') {
		rp0 = OpenReport("lparteo", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	}

	//Si la salida es Terminal
	if (*FmSFld(fm0, SALIDA) == 'T') {
		rp0 = OpenReport("lparteo", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	}

	//Si la salida es Archivo
	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("lparteo2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0,  RP_IO_FILE, FmSFld (fm0, NOMARCH));
	}
	BeginReport(rp0, 1, NULL_STR);
}

private void InicCliente()
{
	int i;

	for (i = 0; i < MAXCLI; i++) {
		cli[i].cliente = 0;
		cli[i].obj     = 0;
		cli[i].presen  = 0;
	}
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH :
		if (FmIsNull (fm, fno) && *FmSFld (fm, SALIDA) == 'A')
			FmSetFld (fm, fno, "lparteo.txt");
		break;
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
			FmSetFld(fm, DCLIEH, GetDescCliente(FmLFld(fm, CLIEH, row)),row);
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
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row) ,FmIFld(fm, OBJETH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;				
}	



private void CargarCliente(long cliente, int obj, long presen)
{
	int i = 0;

	for (i = 0; cli[i].cliente != 0; i++);

	if (i > MAXCLI)
		Error("Tabla interna saturada. Max %d", MAXCLI);

	if (cli[i].cliente == 0) {
		cli[i].cliente = cliente;
		cli[i].obj     = obj;
		cli[i].presen  = presen;
	}
}

private void ImprimirCliente()
{
	int i;

	if (cli[1].cliente != 0)
		DoReport (rp0, NOPARTE);

	for (i = 0; cli[i].cliente != 0; i++) {
		RpSetLFld(rp0, R_NPCLIE,   cli[i].cliente);
		RpSetFld (rp0, R_NPDCLIE,  GetDescCli(cli[i].cliente));
		RpSetIFld(rp0, R_NPOBJET,  cli[i].obj);
		RpSetFld (rp0, R_NPDOBJET, GetObjDescrip(cli[i].cliente, cli[i].obj));
		RpSetLFld(rp0, R_NPSUPERV, cli[i].presen);
		RpSetFld (rp0, R_NPDSUP,   GetNombreLeg(FmIFld(fm0, EMP), cli[i].presen));
		DoReport (rp0, LNOPARTE);
	}
}
