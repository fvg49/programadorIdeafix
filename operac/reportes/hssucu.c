/********************************************************************
*
* MODULE & VERSION : @(#)hssucu.c	1.38
* DATE             : 02/06/07
* TIME             : 16:00:34
*
* CREATED          : 24/06/02
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*********************************************************************/
/* Nota:
Este programa es el hscliobj.c modificado.
Se cambió la función ImprimirReporte, por la nueva ImpReporte.
Pueden existir funciones que colaboraban con ImprimirReporte y que no se utilicen.
*/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "hssucu.fmh"
#include "hssucu.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"

#define R_SEPAR		";"

//Opciones del listado
#define DIFERENC     1
#define SERV_ADIC    2
#define HS_ACARGO    3
#define HS_NOPRES    4
#define TODOS        5
#define MAXCLI       80000
#define ERR_ARCHI    "No se pudo abrir el archivo."

/* Estructuras */
struct cliente {
	int emp;
	long cli;
	int  obj;
	int  puesto;
	DATE fecha;
	short nrosem;
	long svhn;
	long svh50;
	long svh100;
	long svh100f;
	long sthn;
	long sth50;
	long sth100;
	long sth100f;
	long sahn;
	long sah50;
	long sah100;
	long sah100f;
	long sehn;
	long seh50;
	long seh100; 
	long seh100f;
	long stdhs; //Horas standard
} pcli[MAXCLI], *ucli = pcli, *ecli, *ncli;

/* Funciones privadas */
void AbrirArchivo();
void ImprimirReporte();
void ImprimirArchivo();
void ImprimirDetalle();
void ImprimirNoPrint();
void ImprimirTitArch();
void ImprimirDetArch();
void TotGen();
void TotCli();
void TotObj();
void TotFch();
void TotPto();
void TitCli();
void TitObj();
void TitPto();
void TotSem();
void AcumularTotalizadores();
void LimpiarAcumTot();
void LimpiarAcumCli();
void LimpiarAcumObj();
void LimpiarAcumFch();
void LimpiarAcumPto();
void GenerarReporte();
void CargarCliObj(int emp, long cliente, int objetivo, int puesto, int ptoint, DATE fecha, short nrosem,
				  int condic, int hn, int h50, int h100, int h100f);
private int compcli(struct cliente *a, struct cliente *b);
private int compfec(struct cliente *a, struct cliente *b);
bool ValidoClienteObjetivo(long cliente, short objet);
void LimpiarAcumSem();
static fm_status after(form fm, fmfield fno, int row);
bool HayImproductividad();

static void ImpReporte();

/* Declaraciones globales */
FILE   *fp;
form   fm0;
report rp0;
schema comerc, operac, bill;
long   cliant;
int    objant, ptoant;
short  semant;
DATE   fecant;
char   dcliant[100], dobjant[100], dptoant[100];
bool nodif;

/* Totalizadores para impresion en Archivo ASCII */
/* Tot Generales */
long 	tsvhn, tsvh50, tsvh100, tsvtot,
		tsthn, tsth50, tsth100, tsth100f, tsttot, tstnpres,
		tdhn,  tdh50,  tdh100,  tdh100f,
		tsahn, tsah50, tsah100, tsatot,
		tsehn, tseh50, tseh100, tsetot,
		tdphn, tdph50, tdph100, tdph100f,
		tdnhn, tdnh50, tdnh100, tdnh100f;
/* Tot de Cliente */
long	csvhn, csvh50, csvh100, csvtot,
		csthn, csth50, csth100, csth100f, csttot, cstnpres,
		cdhn,  cdh50,  cdh100,  cdh100f,
		csahn, csah50, csah100, csatot,
		csehn, cseh50, cseh100, csetot,
		cdphn, cdph50, cdph100, cdph100f,
		cdnhn, cdnh50, cdnh100, cdnh100f;

/* Tot de Objetivo */
long	osvhn, osvh50, osvh100, osvtot,
		osthn, osth50, osth100, osth100f, osttot, ostnpres,
		odhn,  odh50,  odh100,  odh100f,
		osahn, osah50, osah100, osatot,
		osehn, oseh50, oseh100, osetot,
		odphn, odph50, odph100, odph100f,
		odnhn, odnh50, odnh100, odnh100f;
		
/* Tot de Fecha */
long	fsvhn, fsvh50, fsvh100, fsvtot,
		fsthn, fsth50, fsth100, fsth100f, fsttot, fstnpres,
		fdhn,  fdh50,  fdh100,  fdh100f,
		fsahn, fsah50, fsah100, fsatot,
		fsehn, fseh50, fseh100, fsetot,
		fdphn, fdph50, fdph100, fdph100f,
		fdnhn, fdnh50, fdnh100, fdnh100f;
		
/* Tot de Puesto */
long	psvhn, psvh50, psvh100, psvtot,
		psthn, psth50, psth100, psth100f, psttot, pstnpres,
		pdhn,  pdh50,  pdh100,  pdh100f,
		psahn, psah50, psah100, psatot,
		psehn, pseh50, pseh100, psetot,
		pdphn, pdph50, pdph100, pdph100f,
		pdnhn, pdnh50, pdnh100, pdnh100f;

/* Tot Semana */
long	ssvhn, ssvh50, ssvh100, ssvtot,
		ssthn, ssth50, ssth100, ssth100f, ssttot, sstnpres,
		sdhn,  sdh50,  sdh100,  sdh100f,
		ssahn, ssah50, ssah100, ssatot,
		ssehn, sseh50, sseh100, ssetot,
		sdphn, sdph50, sdph100, sdph100f,
		sdnhn, sdnh50, sdnh100, sdnh100f;

/* Programa principal */
wcmd(hssucu, 1.35 10/29/01)
{
	fm_cmd cmd;

	comerc = OpenSchema("comerc",   IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	fm0    = OpenForm  ("hssucu", FM_EABORT);

	while (DoForm(fm0, NULLFP, after) != FM_UPDATE)
		return;

	InicioListaTipoExcepcion();
	GenerarReporte();
	AbrirArchivo();
	ImpReporte();
	
	fclose(fp);
}

void GenerarReporte()
{
	dbcursor cpto, cparte, cexc, cretro, crexc;
	int  puesto;
	char buffer[100];
	DATE fecha;
	short tipoexc;

	cpto   = CreateCursor(PUESTOSbyCLIENTE, IO_NOT_LOCK);
	cexc   = CreateCursor(EXCEPCIONbyEMP,   IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMP,       IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyEMP,       IO_NOT_LOCK);
	crexc  = CreateCursor(RETROEXCbyEMP,    IO_NOT_LOCK);

	/*** Leo los puestos para informar aunque no tenga nada cargado *******************/
	SetCursorFrom(cpto, FmLFld(fm0, CLID), FmIFld(fm0, OBJD), MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cpto, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cpto) != ERROR) {
		sprintf (buffer, "Procesando Puestos de Cliente %ld Objetivo %d", LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		if(!ValidoClienteObjetivo(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET))) {
			continue;
		}

		if (!ObjActivo(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) && !IsNull(comerc|OBJETIVO_FECHAF) &&
			DFld(comerc|OBJETIVO_FECHAF) < FmDFld(fm0, FECHAD))
			continue;

		if (IFld(PUESTOS_TIPPTO) == BRIGADASINARMAS || IFld(PUESTOS_TIPPTO) == BRIGADACONARMAS)
			continue;

 		if ((IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > FmDFld(fm0, FECHAH)) ||
			(!IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FFINAL) < FmDFld(fm0, FECHAD))) {
			continue;
		}
		for (fecha = FmDFld(fm0, FECHAD); fecha <= FmDFld(fm0, FECHAH); fecha++) {
			if (IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > fecha)
				continue;
			CargarCliObj(FmIFld(fm0, EMP), LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET), IFld(PUESTOS_TIPPTO),
						 IFld(PUESTOS_PUESTO), fecha, NroSemana(FmDFld(fm0, FECHAD), fecha),
						 NULL_SHORT, 0, 0, 0, 0);
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cpto);

	/*** Leo el parte *******************/

	SetCursorFrom(cparte, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
						  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cparte, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
						  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cparte) != ERROR) {
		sprintf (buffer, "Procesando Parte de Cliente %ld Objetivo %d", LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		if(!ValidoClienteObjetivo(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))){
			continue;
		}

		if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;

		CargarCliObj(FmIFld(fm0, EMP), LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), IFld(PARTE_PTOSER),
					 IFld(PARTE_PUESTO), DFld(PARTE_DIA), NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)), 
					 NULL_SHORT,IFld(PARTE_HSNOR), IFld(PARTE_HS50),
					 IFld(PARTE_HS100F), IFld(PARTE_HS100FE));

	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);

	/*** Leo los retroactivos *******************/
	if (FmIFld(fm0, FRETRO)){
		SetCursorFrom(cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
							  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
							  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cretro) != ERROR) {
			sprintf (buffer, "Procesando Retro Cliente %ld Objetivo %d", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			if(!ValidoClienteObjetivo(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO))){
				continue;
			}

			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			CargarCliObj(FmIFld(fm0, EMP), LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), IFld(RETRO_PTOSER),
						 IFld(RETRO_PUESTO), DFld(RETRO_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)), NULL_SHORT, 
						 IFld(RETRO_DHSNOR), IFld(RETRO_DHS50),
						 IFld(RETRO_DHS100F), IFld(RETRO_DHS100FE));

		}
		SetCursorFrom(crexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (crexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(crexc) != ERROR) {
			sprintf (buffer, "Procesando RetroExc de Cliente %ld Objetivo %d", LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			if(!ValidoClienteObjetivo(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO))) {
				continue;
			}

			if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) || DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
				continue;

			tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), (RETROEXC_MOTIVO));
			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
					continue;
			}

			SetKey(comerc|OBJETIVO, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

			CargarCliObj(FmIFld(fm0, EMP), LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO),
						 IFld(RETROEXC_PTOSER), IFld(RETROEXC_PUESTO), DFld(RETROEXC_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(RETROEXC_DIA)),
						 IFld(RETROEXC_CONDIC), IFld(RETROEXC_DHORAS), IFld(RETROEXC_DHS50),
						 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
						 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0);
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
	DeleteCursor(crexc);

	/******************************************* 
		Ahora siempre lee Excepciones
		Antes solo para caso 
		2 - SERVICIOS ADICIONALES MAYOR QUE 0
		3 - HORAS A CARGO DE LA EMPRESA MAYOR A 0
	 o  5 - TODOS LOS DATOS
		Se saco el 03/05/2000 por pedido de Javier Daffunchio */

	SetCursorFrom(cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cexc) != ERROR) {
		sprintf (buffer, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		if(!ValidoClienteObjetivo(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO))) {
			continue;
		}

		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		SetKey(comerc|OBJETIVO, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		CargarCliObj(FmIFld(fm0, EMP), LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
					 IFld(EXCEPCION_PTOSER), IFld(EXCEPCION_PUESTO), DFld(EXCEPCION_DIA),
					 NroSemana(FmDFld(fm0, FECHAD), DFld(EXCEPCION_DIA)), 
					 IFld(EXCEPCION_CONDIC), IFld(EXCEPCION_HORAS), IFld(EXCEPCION_HS50),
					 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
					 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0);
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cexc);
	
}

// Carga el vector de PCLI.
// Si viene de PARTE h100 contiene horas extras de francos y 
// h100f de feriados.
// si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene
// vacio.
void CargarCliObj(int emp, long cliente, int objetivo, int puesto, int ptoint, DATE fecha, short nrosem,
				  int condic, int hn, int h50, int h100, int h100f)
{
	long svhn, svh50, svh100, svh100f;

	svhn = svh50 = svh100 = svh100f = 0;

	for (ecli = pcli; ecli < ucli; ecli++)
		if (ecli->cli == cliente && ecli->obj == objetivo && ecli->puesto == puesto && ecli->fecha == fecha) {
			break;
		}
	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI])
			Error("Tabla interna saturada. Max %d", MAXCLI);

		ucli->emp	  = emp;
		ucli->cli     = cliente;
		ucli->obj     = objetivo;
		ucli->puesto  = puesto;
		ucli->fecha   = fecha;
		ucli->nrosem  = nrosem;
		ucli->seh50   = 0;
		ucli->seh100  = 0;
		ucli->seh100f = 0;
		ucli->sahn    = 0;
		ucli->sah50   = 0;
		ucli->sah100  = 0;
		ucli->sah100f = 0;
		ucli->sthn    = 0;
		ucli->sth50   = 0;
		ucli->sth100  = 0;
		ucli->sth100f = 0;
		GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, puesto, fecha,
						FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD),
						&svhn, &svh50, &svh100, &svh100f);
		ucli->stdhs   = svhn + svh50 + svh100 + svh100f;
		ucli++;
	} 
	if (condic != NULL_SHORT) {		// VIENE DE EXCEPCION
		if (condic == ACARGO_EMP) {
			ecli->sehn    += hn;
			ecli->seh50   += h50;
			ecli->seh100  += h100;
			ecli->seh100f += h100f;
		}
		else {
			ecli->sahn    += hn;
			ecli->sah50   += h50;
			ecli->sah100  += h100;
			ecli->sah100f += h100f;
		}
	}
	else {                      // VIENE DE PARTE
		ecli->sthn    += hn;
		ecli->sth50   += h50;
		ecli->sth100  += h100;
		ecli->sth100f += h100f;
	}
	
	ecli->svhn    += svhn;
	ecli->svh50   += svh50;
	ecli->svh100  += svh100;
	ecli->svh100f += svh100f;
}

// Imprime los datos contenidos en el vector PCLI, de acuerdo a las opciones
// de consolidacion y salida por.
void ImprimirReporte()
{
	bool	first = TRUE;

	semant = NULL_SHORT;
	cliant = NULL_LONG;
	objant = ptoant = NULL_SHORT;
	fecant = NULL_DATE;

	// Ordenamientos
	if (*FmSFld(fm0, DET) == 'D')
		qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compfec);
	else
		qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	// Inicializaciones
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetIFld(rp0, PRI,  TRUE);
		RpSetIFld(rp0, FCH,  FALSE);
		RpSetIFld(rp0, PTO,  FALSE);
		RpSetIFld(rp0, OBJ,  FALSE);
		RpSetIFld(rp0, CLI,  FALSE);
		RpSetIFld(rp0, FDAT, FALSE);

		switch (*FmSFld(fm0, DET)) {
		case 'D':
			RpSetIFld(rp0, FDAT, TRUE); 
			RpSetIFld(rp0, OBJ,  TRUE);
			RpSetIFld(rp0, CLI,  TRUE);
			break;
		case 'F':
			RpSetIFld(rp0, FCH, TRUE); 
		case 'P':
			RpSetIFld(rp0, PTO, TRUE);
		case 'O':
			RpSetIFld(rp0, OBJ, TRUE);
		case 'C':
			RpSetIFld(rp0, CLI, TRUE);
			break;
		case 'S':
			RpSetIFld(rp0, RSEM, TRUE);
			break;
		}
		DoReport(rp0, ZPARAM);
	}
	else {
		LimpiarAcumTot();
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumFch();
		LimpiarAcumPto();
		LimpiarAcumSem();
	}

	// Recorrer la estructura e imprimir
	for (ecli = pcli; ecli < ucli; ecli++) {

		if (ecli->svhn  == 0 && ecli->svh50 == 0 && ecli->svh100 == 0 && ecli->svh100f == 0 &&
	 		ecli->sthn  == 0 && ecli->sth50 == 0 && ecli->sth100 == 0 && ecli->sth100f == 0 &&
	 		ecli->svhn  == 0 && ecli->svh50 == 0 && ecli->svh100 == 0 && ecli->svh100f == 0 &&
	 		ecli->sahn  == 0 && ecli->sah50 == 0 && ecli->sah100 == 0 && ecli->sah100f == 0 &&
	 		ecli->sehn  == 0 && ecli->seh50 == 0 && ecli->seh100 == 0 && ecli->seh100f == 0 &&
            ecli->stdhs == 0)
            continue;

		// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
		// tener que hacerlo en cada funcion que imprime detalles (arch, reporte, y no print)
		ecli->sthn    = ecli->sthn    - ecli->sahn    - ecli->sehn;
		ecli->sth50   = ecli->sth50   - ecli->sah50   - ecli->seh50;
		ecli->sth100  = ecli->sth100  - ecli->sah100  - ecli->seh100;
		ecli->sth100f = ecli->sth100f - ecli->sah100f - ecli->seh100f;

		if (cliant != ecli->cli || objant != ecli->obj || fecant!= ecli->fecha) {
			nodif = FALSE;
			if (*FmSFld(fm0, DET) == 'D' && !HayImproductividad()) {
				nodif = TRUE;
			}
		}

		switch (FmIFld(fm0, OPCION)) {
		case DIFERENC:
			if ((ecli->svhn   - ecli->sthn)   == 0 && (ecli->svh50   - ecli->sth50)   == 0 &&
				(ecli->svh100 - ecli->sth100) == 0 && (ecli->svh100f - ecli->sth100f) == 0 &&
 				 ecli->sth100f == 0)
				continue;
			 break;
		case SERV_ADIC:
			if (ecli->sahn <= 0 && ecli->sah50 <= 0 && ecli->sah100 <= 0 && ecli->sah100f <= 0)
				continue;
			break;
		case HS_ACARGO: 
			if (ecli->sehn <= 0 && ecli->seh50 <= 0 && ecli->seh100 <= 0 && ecli->seh100f <= 0)
				continue;
			break;
		case HS_NOPRES: 
			// diferencia entre total ST y SV > 0 
			if ((ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) -
				(ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f) <= 0)
				continue;
			break;
		}

		if (cliant != ecli->cli) {
			SetKey(bill|CLIENTEbyCLIENTE, ecli->cli);
			if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
				SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");
		}
		if (cliant != ecli->cli || objant != ecli->obj) {
			SetKey(comerc|OBJETIVObyCLIENTE, ecli->cli, ecli->obj);
			if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		 		SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");
		}
		if (ptoant != ecli->puesto) {
			SetKey(comerc|TPTOSERbyTIPPTO, ecli->puesto);
			if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) == ERROR)
				SetFld(comerc|TPTOSER_DESCRIP, "ERROR: Puesto Inexistente");
		}

		if (first) {
			first = FALSE;
			if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
				RpSetLFld(rp0, RCLI,  ecli->cli);
				RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				RpSetIFld(rp0, ROBJ,  ecli->obj);
				RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
				RpSetIFld(rp0, RPTO,  ecli->puesto);
				RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
				RpSetDFld(rp0, RFCH,  ecli->fecha);
				DoReport (rp0, ZCLI);
				DoReport (rp0, ZOBJ);
				DoReport (rp0, ZPTO);
				RpSetIFld(rp0, PRI, FALSE);
			}
			else {
				switch (*FmSFld(fm0, DET)) {
				case 'F':
					TitCli();
					TitObj();
					TitPto();
					break;
				case 'P':
					TitCli();
					TitObj();
					break;
				case 'D':
					TitCli();
					TitObj();
					break;
				case 'O':
					TitCli();
					break;
				}
			}
		}
		else
			ImprimirTitArch();
			
		if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
			ImprimirNoPrint();
		}
		else {
			AcumularTotalizadores();
		}

		if (*FmSFld(fm0, DET) == 'F') {
			if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
				ImprimirDetalle();
			else
				ImprimirDetArch();
		}
		cliant = ecli->cli;
		strcpy(dcliant, SFld(bill|CLIENTE_RAZSOC));
		objant = ecli->obj;
		strcpy(dobjant, SFld(comerc|OBJETIVO_DESCRIP));
		fecant = ecli->fecha;
		ptoant = ecli->puesto;
		semant = ecli->nrosem;
		strcpy(dptoant, SFld(comerc|TPTOSER_DESCRIP));
	}

	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetLFld(rp0, RCLI,  cliant);
		RpSetFld (rp0, RDCLI, dcliant);
		RpSetIFld(rp0, ROBJ,  objant);
		RpSetFld (rp0, RDOBJ, dobjant);
		RpSetDFld(rp0, RFCH,  fecant);
		RpSetIFld(rp0, RPTO,  ptoant);
		RpSetFld (rp0, RDPTO, dptoant);
	}

	switch (*FmSFld(fm0, DET)) {
		case 'F':
			TotSem();
		case 'P':
			TotPto();
			TotObj();
			TotCli();
			TotGen();
		break;
		case 'D':
			TotFch();
			TotSem();
			TotObj();
			TotCli();
			TotGen();
			break;
		case 'O':
			TotObj();
		case 'C':
			TotCli();
			TotGen();
			break;
		case 'S':
			TotSem();
			TotObj();
			TotCli();
			TotGen();
			break;
	} 
}

void AbrirArchivo()
{         
	if ((fp = fopen(FmSFld(fm0, NOMARCH),"w")) == (FILE*)NULL)
		Error(ERR_ARCHI);
}

// Esta funcion se utiliza siempre y es para imprimir el detalle en una 
// zona no print del reporte, cuyos campos son utilizados en las zonas
// automaticas de totalizacion.
void ImprimirNoPrint()
{
	// NOPRINT p/totalizar
	// STANDARD
	RpSetLFld(rp0, SVHN,   ecli->svhn);
	RpSetLFld(rp0, SVH50,  ecli->svh50);
	RpSetLFld(rp0, SVH100, ecli->svh100 + ecli->svh100f);
//	RpSetLFld(rp0, SVTOT,  ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
	RpSetLFld(rp0, SVTOT,  ecli->stdhs);

	// TRABAJADO
	RpSetLFld(rp0, STHN,    ecli->sthn);
	RpSetLFld(rp0, STH50,   ecli->sth50);
	RpSetLFld(rp0, STH100,  ecli->sth100);
	RpSetLFld(rp0, STH100F, ecli->sth100f);
	RpSetLFld(rp0, STTOT,   ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f);
	RpSetLFld(rp0, STNPRES, ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f -
                            (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f));
	// DIFERENCIA
	RpSetLFld(rp0, DIFHN,    ecli->svhn    - ecli->sthn);
	RpSetLFld(rp0, DIFH50,   ecli->svh50   - ecli->sth50);
	RpSetLFld(rp0, DIFH100,  ecli->svh100  - ecli->sth100);
	RpSetLFld(rp0, DIFH100F, ecli->svh100f - ecli->sth100f);

	// ADICIONALES
	RpSetLFld(rp0, SAHN,   ecli->sahn);
	RpSetLFld(rp0, SAH50,  ecli->sah50);
	RpSetLFld(rp0, SAH100, ecli->sah100 + ecli->sah100f);
	RpSetLFld(rp0, SATOT,  ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);

	// A CARGO EMPRESA
	RpSetLFld(rp0, SEHN,   ecli->sehn);
	RpSetLFld(rp0, SEH50,  ecli->seh50);
	RpSetLFld(rp0, SEH100, ecli->seh100 + ecli->seh100f);
	RpSetLFld(rp0, SETOT,  ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f);
	DoReport(rp0, ZNOPRINT);
}

// Se usa esta funcion cuando se debe imprimir el detalle de las fechas y 
// la salida es a reporte
void ImprimirDetalle()
{
	RpSetDFld(rp0, RFCH,    ecli->fecha);

	// STANDARD
	RpSetLFld(rp0, FSVHN,   ecli->svhn);
	RpSetLFld(rp0, FSVH50,  ecli->svh50);
	RpSetLFld(rp0, FSVH100, ecli->svh100 + ecli->svh100f);
//	RpSetLFld(rp0, FSVTOT,  ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
	RpSetLFld(rp0, FSVTOT,  ecli->stdhs);

	// TRABAJADO
	RpSetLFld(rp0, FSTHN,    ecli->sthn);
	RpSetLFld(rp0, FSTH50,   ecli->sth50);
	RpSetLFld(rp0, FSTH100,  ecli->sth100);
	RpSetLFld(rp0, FSTH100F, ecli->sth100f);
	RpSetLFld(rp0, FSTTOT,   ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f);
	RpSetLFld(rp0, FSTNPRES, ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f -
                            (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f));

	// DIFERENCIA
	RpSetLFld(rp0, FDIFHN,    ecli->svhn    - ecli->sthn);
	RpSetLFld(rp0, FDIFH50,   ecli->svh50   - ecli->sth50);
	RpSetLFld(rp0, FDIFH100,  ecli->svh100  - ecli->sth100);
	RpSetLFld(rp0, FDIFH100F, ecli->svh100f - ecli->sth100f);

	// ADICIONALES
	RpSetLFld(rp0, FSAHN,   ecli->sahn);
	RpSetLFld(rp0, FSAH50,  ecli->sah50);
	RpSetLFld(rp0, FSAH100, ecli->sah100 + ecli->sah100f);
	RpSetLFld(rp0, FSATOT,  ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);

	// A CARGO EMPRESA 
	RpSetLFld(rp0, FSEHN,   ecli->sehn);
	RpSetLFld(rp0, FSEH50,  ecli->seh50);
	RpSetLFld(rp0, FSEH100, ecli->seh100 + ecli->seh100f);
	RpSetLFld(rp0, FSETOT,  ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f);
	DoReport(rp0, ZFCH);
}

// cuando la  opcion de seleccion es detallando las fechas y a archivo
// de texto se utiliza esta funcion.
void ImprimirDetArch()
{
	fprintf(fp, "%8.8ld%s%s%s%4.4d%s%s%s%4.4d%s%s%s%.1D%s", 
				ecli->cli, 						R_SEPAR, 
				SFld(bill|CLIENTE_RAZSOC),      R_SEPAR, 
				ecli->obj,                      R_SEPAR, 
				SFld(comerc|OBJETIVO_DESCRIP),  R_SEPAR, 
				ecli->puesto,                   R_SEPAR, 
				SFld(comerc|TPTOSER_DESCRIP),   R_SEPAR, 
				ecli->fecha,					R_SEPAR );

	fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",

			 	(double)ecli->svhn / 100.0, 	R_SEPAR,
			 	(double)ecli->svh50 / 100.0,    R_SEPAR,
			 	(double)(ecli->svh100 + ecli->svh100f) / 100.0,	R_SEPAR,
			 	(double)(ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) / 100.0,	R_SEPAR,

				(double)ecli->sthn / 100.0,		R_SEPAR,
				(double)ecli->sth50 / 100.0,	R_SEPAR,
				(double)ecli->sth100 / 100.0,   R_SEPAR,
				(double)ecli->sth100f / 100.0,  R_SEPAR,
			    (double)(ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f) / 100.0, R_SEPAR,
			    
			 	(double)((ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) -
			 	         (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f)) / 100.0, R_SEPAR,
				
				(double)(ecli->svhn    - ecli->sthn) / 100.0, R_SEPAR,
				(double)(ecli->svh50   - ecli->sth50) / 100.0, R_SEPAR,
				(double)(ecli->svh100  - ecli->sth100) / 100.0, R_SEPAR,
				(double)(ecli->svh100f - ecli->sth100f) / 100.0, R_SEPAR,
			 	
			 	(double)ecli->sahn / 100.0, R_SEPAR,
			 	(double)ecli->sah50 / 100.0, R_SEPAR,
			 	(double)(ecli->sah100 + ecli->sah100f) / 100.0, R_SEPAR,
			 	(double)(ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f) / 100.0, R_SEPAR,
			 	
			 	(double)ecli->sehn / 100.0, R_SEPAR,
			 	(double)ecli->seh50 / 100.0, R_SEPAR,
			 	(double)(ecli->seh100 + ecli->seh100f) / 100.0, R_SEPAR,
			 	(double)(ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f) / 100.0, R_SEPAR,
				//Diferencias Positivas
			 	(double)(ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn)    / 100.0, R_SEPAR,
			 	(double)(ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50)   / 100.0, R_SEPAR,
			 	(double)(ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100)  / 100.0, R_SEPAR,
			 	(double)(ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f) / 100.0, R_SEPAR,
				//Diferencias Negativas
			 	(double)(ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn)    / 100.0, R_SEPAR,
			 	(double)(ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50)   / 100.0, R_SEPAR,
			 	(double)(ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100)  / 100.0, R_SEPAR,
			 	(double)(ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f) / 100.0);

}

// Imprime los titulos y totalizadores segun haya cambiado en el corte
// de control el cliente objetivo o puesto y segun sea la opcion de 
// consolidado elegida.
void ImprimirTitArch()
{
	/* Cambio el cliente */
	if (cliant != ecli->cli) {
		switch (*FmSFld(fm0, DET)) {
		case 'C':
			TotCli();
			if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
				RpSetLFld(rp0, RCLI, ecli->cli);
				RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
			}
			break;
		case 'O':
			TotObj();
			TotCli();
			TitCli();
			break;
		case 'D':
			TotFch();
			TotSem();
			TotObj();
			TotCli();
			TitCli();
			TitObj();
			break;
		case 'P':
			TotPto();
			TotObj();
			TotCli();
			TitCli();
			TitObj();
			break;
		case 'F':
			TotSem();
			TotPto();
			TotObj();
			TotCli();
			TitCli();
			TitObj();
			TitPto();
			break;
		case 'S':
			TotSem();
			break;
		}
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumFch();
		LimpiarAcumPto();
		LimpiarAcumSem();
	}
	else {
		/*Si cambio el objetivo */
		if (objant != ecli->obj) {
			switch (*FmSFld(fm0, DET)) {
			case 'O':
				TotObj();
				if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
					RpSetIFld(rp0, ROBJ,  ecli->obj);
					RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
				}
				break;
			case 'D':
				TotFch();
				TotSem();
				TotObj();
				TitObj();
				break;
			case 'P':
				TotPto();
				TotObj();
				TitObj();
				break;
			case 'F':
				TotSem();
				TotPto();
				TotObj();
				TitObj();
				TitPto();
				break;
			case 'S':
				TotSem();
				TotObj();
				TitObj();
				break;
			}
			LimpiarAcumObj();
			LimpiarAcumFch();
			LimpiarAcumPto();
			LimpiarAcumSem();
		}
		else {
			bool impsem=FALSE;

			/* Cambio la fecha */
			if (fecant != ecli->fecha) {

				switch (*FmSFld(fm0, DET)) {
				case 'D' :
					TotFch();
					break;
				}
				LimpiarAcumFch();
			}
			/* Cambio la semana */
			if (semant != ecli->nrosem) {
				switch (*FmSFld(fm0, DET)) {
				case 'D' :
				case 'F' :
				case 'S' :
					TotSem();
					impsem=TRUE;
					break;
				}
				LimpiarAcumSem();
			}
			/* Cambio el puesto */
			if (ptoant != ecli->puesto) {
				switch (*FmSFld(fm0, DET)) {
				case 'P' :
					TotPto();
					if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
						RpSetIFld(rp0, RPTO, ecli->puesto);
						RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
					}
					break;
				case 'F' :
					if (!impsem) {
						TotSem();
						LimpiarAcumSem();
					}
					TotPto();
					TitPto();
					break;
				}
				LimpiarAcumPto();
			}
		}
	}
}

//Limpia los acumuladores de cliente/totales/puesto y objetivos
void LimpiarAcumTot()
{
		tsvhn = tsvh50 = tsvh100 = tsvtot   = 0;
		tsthn = tsth50 = tsth100 = tsth100f = tsttot = tstnpres = 0;
		tdhn  = tdh50  = tdh100  = tdh100f  = 0;
		tsahn = tsah50 = tsah100 = tsatot   = 0;
		tsehn = tseh50 = tseh100 = tsetot   = 0;
		tdphn = tdph50 = tdph100 = tdph100f = 0;
		tdnhn = tdnh50 = tdnh100 = tdnh100f = 0;
}

void LimpiarAcumCli()
{
		csvhn = csvh50 = csvh100 = csvtot   = 0;
		csthn = csth50 = csth100 = csth100f = csttot = cstnpres = 0;
		cdhn  = cdh50  = cdh100  = cdh100f  = 0;
		csahn = csah50 = csah100 = csatot   = 0;
		csehn = cseh50 = cseh100 = csetot   = 0;
		cdphn = cdph50 = cdph100 = cdph100f = 0;
		cdnhn = cdnh50 = cdnh100 = cdnh100f = 0;
}

void LimpiarAcumObj()
{
		osvhn = osvh50 = osvh100 = osvtot   = 0;
		osthn = osth50 = osth100 = osth100f = osttot = ostnpres = 0;
		odhn  = odh50  = odh100  = odh100f  = 0;
		osahn = osah50 = osah100 = osatot   = 0;
		osehn = oseh50 = oseh100 = osetot   = 0; 
		odphn = odph50 = odph100 = odph100f = 0;
		odnhn = odnh50 = odnh100 = odnh100f = 0;
}

void LimpiarAcumFch()
{
		fsvhn = fsvh50 = fsvh100 = fsvtot   = 0;
		fsthn = fsth50 = fsth100 = fsth100f = fsttot = fstnpres = 0;
		fdhn  = fdh50  = fdh100  = fdh100f  = 0;
		fsahn = fsah50 = fsah100 = fsatot   = 0;
		fsehn = fseh50 = fseh100 = fsetot   = 0; 
		fdphn = fdph50 = fdph100 = fdph100f = 0;
		fdnhn = fdnh50 = fdnh100 = fdnh100f = 0;
}

void LimpiarAcumPto()
{
		psvhn = psvh50 = psvh100 = psvtot   = 0;
		psthn = psth50 = psth100 = psth100f = psttot = pstnpres = 0;
		pdhn  = pdh50  = pdh100  = pdh100f  = 0;
		psahn = psah50 = psah100 = psatot   = 0;
		psehn = pseh50 = pseh100 = psetot   = 0; 
		pdphn = pdph50 = pdph100 = pdph100f = 0;
		pdnhn = pdnh50 = pdnh100 = pdnh100f = 0;
}

void LimpiarAcumSem()
{
		ssvhn = ssvh50 = ssvh100 = ssvtot   = 0;
		ssthn = ssth50 = ssth100 = ssth100f = ssttot = sstnpres = 0;
		sdhn  = sdh50  = sdh100  = sdh100f  = 0;
		ssahn = ssah50 = ssah100 = ssatot   = 0;
		ssehn = sseh50 = sseh100 = ssetot   = 0; 
		sdphn = sdph50 = sdph100 = sdph100f = 0;
		sdnhn = sdnh50 = sdnh100 = sdnh100f = 0;
}

// Acumula los totalizadores de cliente-objtivo-puesto y gnerales con datos tomados del vector ecli
// en la posicion corrinente que es la que se esta tratando.
void AcumularTotalizadores()
{
	int totsvh, totsth;

	csvhn += ecli->svhn;
	osvhn += ecli->svhn;
	fsvhn += ecli->svhn;
	psvhn += ecli->svhn;
	ssvhn += ecli->svhn;
	tsvhn += ecli->svhn;

	csvh50 += ecli->svh50;
	osvh50 += ecli->svh50;
	fsvh50 += ecli->svh50;
	psvh50 += ecli->svh50;
	ssvh50 += ecli->svh50;
	tsvh50 += ecli->svh50;

	csvh100 += ecli->svh100 + ecli->svh100f;
	osvh100 += ecli->svh100 + ecli->svh100f;
	fsvh100 += ecli->svh100 + ecli->svh100f;
	psvh100 += ecli->svh100 + ecli->svh100f;
	ssvh100 += ecli->svh100 + ecli->svh100f;
	tsvh100 += ecli->svh100 + ecli->svh100f;

	totsvh  = ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f;
	csvtot += totsvh;
	osvtot += totsvh;
	fsvtot += totsvh;
	psvtot += totsvh;
	ssvtot += totsvh;
	tsvtot += totsvh;

	csthn += ecli->sthn;
	osthn += ecli->sthn;
	fsthn += ecli->sthn;
	psthn += ecli->sthn;
	ssthn += ecli->sthn;
	tsthn += ecli->sthn;

	csth50 += ecli->sth50;
	osth50 += ecli->sth50;
	fsth50 += ecli->sth50;
	psth50 += ecli->sth50;
	ssth50 += ecli->sth50;
	tsth50 += ecli->sth50;

	csth100 += ecli->sth100;
	osth100 += ecli->sth100;
	fsth100 += ecli->sth100;
	psth100 += ecli->sth100;
	ssth100 += ecli->sth100;
	tsth100 += ecli->sth100;

	csth100f += ecli->sth100f;
	osth100f += ecli->sth100f;
	fsth100f += ecli->sth100f;
	psth100f += ecli->sth100f;
	ssth100f += ecli->sth100f;
	tsth100f += ecli->sth100f;

	totsth  = ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f;
	csttot += totsth;
	osttot += totsth;
	fsttot += totsth;
	psttot += totsth;
	ssttot += totsth;
	tsttot += totsth;

	cstnpres += totsvh - totsth;
	ostnpres += totsvh - totsth;
	fstnpres += totsvh - totsth;
	pstnpres += totsvh - totsth;
	sstnpres += totsvh - totsth;
	tstnpres += totsvh - totsth;

	cdhn = cdhn + (ecli->svhn - ecli->sthn);
	odhn += ecli->svhn - ecli->sthn;
	fdhn += ecli->svhn - ecli->sthn;
	pdhn += ecli->svhn - ecli->sthn;
	sdhn += ecli->svhn - ecli->sthn;
	tdhn += ecli->svhn - ecli->sthn;

	cdh50 += ecli->svh50 - ecli->sth50;
	odh50 += ecli->svh50 - ecli->sth50;
	fdh50 += ecli->svh50 - ecli->sth50;
	pdh50 += ecli->svh50 - ecli->sth50;
	sdh50 += ecli->svh50 - ecli->sth50;
	tdh50 += ecli->svh50 - ecli->sth50;

	cdh100 += ecli->svh100 - ecli->sth100;
	odh100 += ecli->svh100 - ecli->sth100;
	fdh100 += ecli->svh100 - ecli->sth100;
	pdh100 += ecli->svh100 - ecli->sth100;
	sdh100 += ecli->svh100 - ecli->sth100;
	tdh100 += ecli->svh100 - ecli->sth100;

	cdh100f += ecli->svh100f - ecli->sth100f;
	odh100f += ecli->svh100f - ecli->sth100f;
	fdh100f += ecli->svh100f - ecli->sth100f;
	pdh100f += ecli->svh100f - ecli->sth100f;
	sdh100f += ecli->svh100f - ecli->sth100f;
	tdh100f += ecli->svh100f - ecli->sth100f;

	csahn += ecli->sahn;
	osahn += ecli->sahn;
	fsahn += ecli->sahn;
	psahn += ecli->sahn;
	ssahn += ecli->sahn;
	tsahn += ecli->sahn;

	csah50 += ecli->sah50;
	osah50 += ecli->sah50;
	fsah50 += ecli->sah50;
	psah50 += ecli->sah50;
	ssah50 += ecli->sah50;
	tsah50 += ecli->sah50;

	csah100 += ecli->sah100 + ecli->sah100f;
	osah100 += ecli->sah100 + ecli->sah100f;
	fsah100 += ecli->sah100 + ecli->sah100f;
	psah100 += ecli->sah100 + ecli->sah100f;
	ssah100 += ecli->sah100 + ecli->sah100f;
	tsah100 += ecli->sah100 + ecli->sah100f;

	csatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	osatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	fsatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	psatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	ssatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	tsatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;

	csehn += ecli->sehn;
	osehn += ecli->sehn;
	fsehn += ecli->sehn;
	psehn += ecli->sehn;
	ssehn += ecli->sehn;
	tsehn += ecli->sehn;

	cseh50 += ecli->seh50;
	oseh50 += ecli->seh50;
	fseh50 += ecli->seh50;
	pseh50 += ecli->seh50;
	sseh50 += ecli->seh50;
	tseh50 += ecli->seh50;

	cseh100 += ecli->seh100+ecli->seh100f;
	oseh100 += ecli->seh100+ecli->seh100f;
	fseh100 += ecli->seh100+ecli->seh100f;
	pseh100 += ecli->seh100+ecli->seh100f;
	sseh100 += ecli->seh100+ecli->seh100f;
	tseh100 += ecli->seh100+ecli->seh100f;

	csetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	osetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	fsetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	psetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	ssetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	tsetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;

	if (*FmSFld(fm0, DET) == 'D' && nodif) {
		return;
	}

	cdphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);
	odphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);
	fdphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);
	pdphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);
	sdphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);
	tdphn   += (ecli->svhn - ecli->sthn) < 0 ? 0 : (ecli->svhn - ecli->sthn);

	cdph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);
	odph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);
	fdph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);
	pdph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);
	sdph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);
	tdph50  += (ecli->svh50 - ecli->sth50) < 0 ? 0 : (ecli->svh50 - ecli->sth50);

	cdph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
	odph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
	fdph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
	pdph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
	sdph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
	tdph100 += (ecli->svh100 - ecli->sth100) < 0 ? 0 : (ecli->svh100 - ecli->sth100);

	cdph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	odph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	fdph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	pdph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	sdph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	tdph100f += (ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f - ecli->sth100f);

	cdnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);
	odnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);
	fdnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);
	pdnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);
	sdnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);
	tdnhn   += (ecli->svhn - ecli->sthn) > 0 ? 0 : (ecli->svhn - ecli->sthn);

	cdnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
	odnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
	fdnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
	pdnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
	sdnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
	tdnh50  += (ecli->svh50 - ecli->sth50) > 0 ? 0 : (ecli->svh50 - ecli->sth50);

	cdnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
	odnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
	fdnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
	pdnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
	sdnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
	tdnh100 += (ecli->svh100 - ecli->sth100) > 0 ? 0 : (ecli->svh100 - ecli->sth100);

	cdnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	odnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	fdnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	pdnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	sdnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);
	tdnh100f += (ecli->svh100f - ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);

}

// Imprime el total de un cliente en el archivo de salida
void TotCli()
{
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, DET) != 'F') {
	
	  	fprintf(fp, "%8.8ld%s%s%s%s%s%s%sTotal Cliente %8.8ld %20s%s",
				cliant, R_SEPAR, dcliant, R_SEPAR,R_SEPAR,R_SEPAR,R_SEPAR,R_SEPAR, cliant, R_SEPAR, dcliant);

		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)csvhn    / 100.0, R_SEPAR, (double)csvh50   / 100.0, R_SEPAR, (double)csvh100 / 100.0, R_SEPAR, (double)csvtot / 100.0,  R_SEPAR,
			(double)csthn    / 100.0, R_SEPAR, (double)csth50   / 100.0, R_SEPAR,
			(double)csth100  / 100.0, R_SEPAR, (double)csth100f / 100.0, R_SEPAR, (double)csttot  / 100.0, R_SEPAR,
			(double)cstnpres / 100.0, R_SEPAR, (double)cdhn     / 100.0, R_SEPAR, (double)cdh50   / 100.0, R_SEPAR,
			(double)cdh100   / 100.0, R_SEPAR, (double)cdh100f  / 100.0, R_SEPAR,
			(double)csahn    / 100.0, R_SEPAR, (double)csah50   / 100.0, R_SEPAR, (double)csah100 / 100.0, R_SEPAR, (double)csatot / 100.0,  R_SEPAR,
			(double)csehn    / 100.0, R_SEPAR, (double)cseh50   / 100.0, R_SEPAR, (double)cseh100 / 100.0, R_SEPAR, (double)csetot / 100.0,  R_SEPAR,
			(double)cdphn    / 100.0, R_SEPAR, (double)cdph50   / 100.0, R_SEPAR, (double)cdph100 / 100.0, R_SEPAR, (double)cdph100f/ 100.0, R_SEPAR,
			(double)cdnhn    / 100.0, R_SEPAR, (double)cdnh50   / 100.0, R_SEPAR, (double)cdnh100 / 100.0, R_SEPAR, (double)cdnh100f/ 100.0);
	}
//	else
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
		DoReport(rp0, ZTOTCLI);
}

// Imprime el total de un objetivo en el archivo de salida
void TotObj()
{
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, DET) != 'F') {
		
		fprintf(fp, "%8.8ld%s%s%s%4.4d%s%s%s%sCliente %8.8ld %20s%sTotal Objetivo %4.4d %20s%s",
			cliant, R_SEPAR,dcliant, R_SEPAR,objant, R_SEPAR,dobjant, R_SEPAR,R_SEPAR,cliant, dcliant, R_SEPAR, objant, dobjant, R_SEPAR);
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)osvhn    / 100.0, R_SEPAR, (double)osvh50   / 100.0,R_SEPAR, (double)osvh100 / 100.0,R_SEPAR,
			(double)osvtot   / 100.0, R_SEPAR, (double)osthn    / 100.0,R_SEPAR, (double)osth50  / 100.0,R_SEPAR,
			(double)osth100  / 100.0, R_SEPAR, (double)osth100f / 100.0,R_SEPAR, (double)osttot  / 100.0,R_SEPAR,
			(double)ostnpres / 100.0, R_SEPAR, (double)odhn     / 100.0,R_SEPAR, (double)odh50   / 100.0,R_SEPAR,
			(double)odh100   / 100.0, R_SEPAR, (double)odh100f  / 100.0,R_SEPAR, (double)osahn   / 100.0,R_SEPAR,
			(double)osah50   / 100.0, R_SEPAR, (double)osah100  / 100.0,R_SEPAR, (double)osatot  / 100.0,R_SEPAR,
			(double)osehn    / 100.0, R_SEPAR, (double)oseh50   / 100.0,R_SEPAR, (double)oseh100 / 100.0,R_SEPAR,  (double)osetot / 100.0,R_SEPAR,
			(double)odphn    / 100.0, R_SEPAR, (double)odph50   / 100.0,R_SEPAR, (double)odph100 / 100.0,R_SEPAR,  (double)odph100f/ 100.0,R_SEPAR,
			(double)odnhn    / 100.0, R_SEPAR, (double)odnh50   / 100.0,R_SEPAR, (double)odnh100 / 100.0,R_SEPAR,  (double)odnh100f/ 100.0);
	}
//	else {
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {    
		RpSetIFld(rp0, ROBJ,  objant);
		RpSetFld (rp0, RDOBJ, dobjant);
		DoReport(rp0, ZTOTOBJ);
	}
}

// Imprime el total de fecha en el archivo de salida
void TotFch()
{
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, DET) != 'F') {

		fprintf(fp, "%8.8ld%s%s%s%4.4d%s%s%s%.1D%s%sTotal Fecha %.1D%s",
			cliant, R_SEPAR,dcliant, R_SEPAR,objant, R_SEPAR,dobjant,R_SEPAR,fecant, R_SEPAR,R_SEPAR,fecant, R_SEPAR);
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)fsvhn    / 100.0, R_SEPAR, (double)fsvh50   / 100.0, R_SEPAR, (double)fsvh100 / 100.0,R_SEPAR, 
			(double)fsvtot   / 100.0, R_SEPAR, (double)fsthn    / 100.0, R_SEPAR, (double)fsth50  / 100.0,R_SEPAR, 
			(double)fsth100  / 100.0, R_SEPAR, (double)fsth100f / 100.0, R_SEPAR, (double)fsttot  / 100.0,R_SEPAR, 
			(double)fstnpres / 100.0, R_SEPAR,  (double)fdhn     / 100.0,R_SEPAR,  (double)fdh50   / 100.0,R_SEPAR, 
			(double)fdh100   / 100.0, R_SEPAR, (double)fdh100f  / 100.0, R_SEPAR, (double)fsahn   / 100.0,R_SEPAR, 
			(double)fsah50   / 100.0, R_SEPAR, (double)fsah100  / 100.0, R_SEPAR, (double)fsatot  / 100.0,R_SEPAR, 
			(double)fsehn    / 100.0, R_SEPAR, (double)fseh50   / 100.0, R_SEPAR, (double)fseh100 / 100.0, R_SEPAR, (double)fsetot / 100.0,R_SEPAR, 
			(double)fdphn    / 100.0, R_SEPAR, (double)fdph50   / 100.0, R_SEPAR, (double)fdph100 / 100.0, R_SEPAR, (double)fdph100f/ 100.0,R_SEPAR, 
			(double)fdnhn    / 100.0, R_SEPAR, (double)fdnh50   / 100.0, R_SEPAR, (double)fdnh100 / 100.0, R_SEPAR, (double)fdnh100f/ 100.0);
	}
//	else {
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetDFld(rp0, RFCH, fecant);
		DoReport(rp0, ZTOTFCH);
	}
}

// Imprime el total de puesto en el archivo de salida
void TotPto()
{
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, DET) != 'F') {
		
		fprintf(fp, "%8.8ld%s%s%s%4.4d%s%s%s%4.4d%s%sTotal Puesto %4.4d %20s%s",
			cliant, R_SEPAR, dcliant, R_SEPAR, objant, R_SEPAR, dobjant,R_SEPAR, ptoant, R_SEPAR, R_SEPAR, ptoant, dptoant, R_SEPAR );
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)psvhn    / 100.0, R_SEPAR, (double)psvh50   / 100.0, R_SEPAR, (double)psvh100 / 100.0, R_SEPAR, 
			(double)psvtot   / 100.0, R_SEPAR, (double)psthn    / 100.0, R_SEPAR, (double)psth50  / 100.0, R_SEPAR, 
			(double)psth100  / 100.0, R_SEPAR, (double)psth100f / 100.0, R_SEPAR, (double)psttot  / 100.0, R_SEPAR, 
			(double)pstnpres / 100.0, R_SEPAR, (double)pdhn     / 100.0, R_SEPAR, (double)pdh50   / 100.0, R_SEPAR, 
			(double)pdh100   / 100.0, R_SEPAR, (double)pdh100f  / 100.0, R_SEPAR, (double)psahn   / 100.0, R_SEPAR, 
			(double)psah50   / 100.0, R_SEPAR, (double)psah100  / 100.0, R_SEPAR, (double)psatot  / 100.0, R_SEPAR, 
			(double)psehn    / 100.0, R_SEPAR, (double)pseh50   / 100.0, R_SEPAR, (double)pseh100 / 100.0, R_SEPAR, (double)psetot / 100.0,  R_SEPAR, 
			(double)pdphn    / 100.0, R_SEPAR, (double)pdph50   / 100.0, R_SEPAR, (double)pdph100 / 100.0, R_SEPAR, (double)pdph100f/ 100.0, R_SEPAR, 
			(double)pdnhn    / 100.0, R_SEPAR, (double)pdnh50   / 100.0, R_SEPAR, (double)pdnh100 / 100.0, R_SEPAR, (double)pdnh100f/ 100.0);
	}
//	else
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
		DoReport(rp0, ZTOTPTO);
}

// Imprime el total de la semana
void TotSem()
{
	DATE fdesde, fhasta;

	#ifndef _NOVIA_VER_2_0
 		return;		
	#endif

	SemanaDesdeHasta(fecant, &fdesde, &fhasta);

	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R') && *FmSFld(fm0, DET) != 'F') {
		
		fprintf(fp, "%8.8ld%s%s%s%4.4d%s%s%s    %s%sTotal Semana del %.3D al %.3D%s",
			cliant, R_SEPAR, dcliant, R_SEPAR, objant, R_SEPAR, dobjant,R_SEPAR, R_SEPAR, R_SEPAR, fdesde, fhasta,R_SEPAR );
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)ssvhn    / 100.0, R_SEPAR, (double)ssvh50   / 100.0, R_SEPAR, (double)ssvh100 / 100.0, R_SEPAR, 
			(double)ssvtot   / 100.0, R_SEPAR, (double)ssthn    / 100.0, R_SEPAR, (double)ssth50  / 100.0, R_SEPAR, 
			(double)ssth100  / 100.0, R_SEPAR, (double)ssth100f / 100.0, R_SEPAR, (double)ssttot  / 100.0, R_SEPAR, 
			(double)sstnpres / 100.0, R_SEPAR, (double)sdhn     / 100.0, R_SEPAR, (double)sdh50   / 100.0, R_SEPAR, 
			(double)sdh100   / 100.0, R_SEPAR, (double)sdh100f  / 100.0, R_SEPAR, (double)ssahn   / 100.0, R_SEPAR, 
			(double)ssah50   / 100.0, R_SEPAR, (double)ssah100  / 100.0, R_SEPAR, (double)ssatot  / 100.0, R_SEPAR, 
			(double)ssehn    / 100.0, R_SEPAR, (double)sseh50   / 100.0, R_SEPAR, (double)sseh100 / 100.0, R_SEPAR, (double)ssetot / 100.0,  R_SEPAR, 
			(double)sdphn    / 100.0, R_SEPAR, (double)sdph50   / 100.0, R_SEPAR, (double)sdph100 / 100.0, R_SEPAR, (double)sdph100f/ 100.0, R_SEPAR, 
			(double)sdnhn    / 100.0, R_SEPAR, (double)sdnh50   / 100.0, R_SEPAR, (double)sdnh100 / 100.0, R_SEPAR, (double)sdnh100f/ 100.0);
	}

	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetDFld(rp0, RSEMD, fdesde);
		RpSetDFld(rp0, RSEMH, fhasta);
		DoReport(rp0, ZTOTSEM);
	}
}

// Imprime el total de puesto en el archivo de salida
void TotGen()
{
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, DET) != 'F') {

		fprintf(fp, "%s%s%s%s%s%sTotal GENERAL%s",R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR );

		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\n",
			(double)tsvhn    / 100.0, R_SEPAR, (double)tsvh50   / 100.0, R_SEPAR, (double)tsvh100 / 100.0, R_SEPAR, 
			(double)tsvtot   / 100.0, R_SEPAR, (double)tsthn    / 100.0, R_SEPAR, (double)tsth50  / 100.0, R_SEPAR, 
			(double)tsth100  / 100.0, R_SEPAR, (double)tsth100f / 100.0, R_SEPAR, (double)tsttot  / 100.0, R_SEPAR, 
			(double)tstnpres / 100.0, R_SEPAR, (double)tdhn     / 100.0, R_SEPAR, (double)tdh50   / 100.0, R_SEPAR, 
			(double)tdh100   / 100.0, R_SEPAR, (double)tdh100f  / 100.0, R_SEPAR, (double)tsahn   / 100.0, R_SEPAR, 
			(double)tsah50   / 100.0, R_SEPAR, (double)tsah100  / 100.0, R_SEPAR, (double)tsatot  / 100.0, R_SEPAR, 
			(double)tsehn    / 100.0, R_SEPAR, (double)tseh50   / 100.0, R_SEPAR, (double)tseh100 / 100.0, R_SEPAR, (double)tsetot / 100.0,  R_SEPAR, 
			(double)tdphn    / 100.0, R_SEPAR, (double)tdph50   / 100.0, R_SEPAR, (double)tdph100 / 100.0, R_SEPAR, (double)tdph100f/ 100.0, R_SEPAR, 
			(double)tdnhn    / 100.0, R_SEPAR, (double)tdnh50   / 100.0, R_SEPAR, (double)tdnh100 / 100.0, R_SEPAR, (double)tdnh100f/ 100.0);
	}
//	else
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
		DoReport(rp0, ZTOTGEN);
}

// Imprime el titulo de Cliente en el archivo de salida
void TitCli()
{
//	if (*FmSFld(fm0, SALIDA) == 'A')
//		fprintf(fp, "\tCliente %8.8ld %20s\n", ecli->cli, SFld(bill|CLIENTE_RAZSOC));
//	else {
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetLFld(rp0, RCLI,  ecli->cli);
		RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
		DoReport(rp0, ZCLI);
	}
}

// Imprime el titulo de objetivo en el archivo de salida
void TitObj()
{
//	if (*FmSFld(fm0, SALIDA) == 'A')
//		fprintf(fp, "\t\tObjetivo %4.4d %20s\n", ecli->obj, SFld(comerc|OBJETIVO_DESCRIP));
//	else {
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetIFld(rp0, ROBJ,  ecli->obj);
		RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
		DoReport(rp0, ZOBJ);
	}
}

// Imprime el titulo de puesto en el archivo de salida
void TitPto()
{
//	if (*FmSFld(fm0, SALIDA) == 'A')
//		fprintf(fp, "\t\t\tPuesto %4.4d %20s\n", ecli->puesto, SFld(comerc|TPTOSER_DESCRIP));
//	else {
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetIFld(rp0, RPTO,  ecli->puesto);
		RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
		DoReport(rp0, ZPTO);
	}
}

private int compcli(struct cliente *a, struct cliente *b)
{
	return	a->cli    < b->cli    ? -1 : a->cli    > b->cli    ? 1 :
			a->obj    < b->obj    ? -1 : a->obj    > b->obj    ? 1 :
			a->puesto < b->puesto ? -1 : a->puesto > b->puesto ? 1 :
			a->fecha  < b->fecha  ? -1 : a->fecha  > b->fecha  ? 1 :
			0;
}

private int compfec(struct cliente *a, struct cliente *b)
{
	return	a->cli    < b->cli    ? -1 : a->cli    > b->cli    ? 1 :
			a->obj    < b->obj    ? -1 : a->obj    > b->obj    ? 1 :
			a->fecha  < b->fecha  ? -1 : a->fecha  > b->fecha  ? 1 :
			0;
}

bool ValidoClienteObjetivo(long cliente, short objet)
{
	if (*FmSFld(fm0, TIPOBJ) == 'T') {
		return TRUE;
	}

	if (*FmSFld(fm0, TIPOBJ) == 'R' && ObjetRif(cliente, objet)){
		return TRUE;
	}

	if (*FmSFld(fm0, TIPOBJ) == 'E' && !ObjetRif(cliente, objet)){
		return TRUE;
	}

	return FALSE;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case SALIDA:
		if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA) == 'R') && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "hssucu.txt");
	break;
	case FECHAD:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'L')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "LUNES");
		#endif
	break;

	case FECHAH:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'D')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "DOMINGO");
		#endif
	break;
	case DET:
		#ifndef _NOVIA_VER_2_0
			if (*FmSFld(fm0, DET) == 'S') {
				WiMsg ("Esta opcion no esta disponible en esta version ");
				return FM_REDO;
			}
		#endif
	break;
	}
	return FM_OK;				
}	

bool HayImproductividad()
{
	long totnor=0, tot50=0, tot100=0, tot100f=0;
	bool impro=FALSE;

	for (ncli = ecli; ncli < ucli; ncli++) {
		if (ecli->cli != ncli->cli || ecli->obj != ncli->obj || ecli->fecha != ncli->fecha) {
			break;
		}

		totnor  += (ncli->svhn - ncli->sthn);
		tot50   += (ncli->svh50 - ncli->sth50);
		tot100  += (ncli->svh100 - ncli->sth100);
		tot100f += (ncli->svh100f - ncli->sth100f);

	}

	impro = (totnor != 0 || tot50 !=0 || tot100 != 0 || tot100f != 0);
	return impro;
}

static void ImpReporte()
{            
	long cliant=0;
	int objant=0, empant=0;    
	bool primero=TRUE;
	long sthnac=0, sth50ac=0, sth100ac=0, sth100fac=0;
	long sahnac=0, sah50ac=0, sah100ac=0, sah100fac=0;
	char direc[300];
	
	qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	for (ecli = pcli; ecli < ucli; ecli++) {

        if( !primero && (empant!=ecli->emp || cliant!=ecli->cli || objant!=ecli->obj)) {

			SetKey(bill|CLIENTEbyCLIENTE, cliant);
			if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
					SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");

			SetKey(comerc|OBJETIVObyCLIENTE, cliant, objant);
			if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		 		SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");

            SetKey(comerc|SUCXOBJbyEMP, empant, cliant, objant);
			if (GetRecord(comerc|SUCXOBJbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
		 		SetFld(comerc|SUCXOBJ_DESCRIP, "ERROR: Sucursal Inexistente");
			
	        sprintf(direc,"%s%s%s%s%ld%s%s%s%s%s%d%s%s",
	        	SFld(comerc|OBJETIVO_CALLE),		R_SEPAR,
	        	SFld(comerc|OBJETIVO_NRO),          R_SEPAR,
	        	LFld(comerc|OBJETIVO_LOCAL),       	R_SEPAR,
	        	GetDescLocali(IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV), LFld(comerc|OBJETIVO_LOCAL)), R_SEPAR,
	        	SFld(comerc|OBJETIVO_CODPOS),		R_SEPAR,
	        	IFld(comerc|OBJETIVO_PROV),         R_SEPAR,
	        	GetDescProv(IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV))
	        	);
			fprintf(fp, "%d%s%ld%s%s%s%d%s%s%s%d%s%s%s%s%s%9.2f%s%9.2f",
				empant,							R_SEPAR, 
				cliant, 						R_SEPAR, 
				SFld(bill|CLIENTE_RAZSOC),      R_SEPAR, 
				objant,	                        R_SEPAR, 
				SFld(comerc|OBJETIVO_DESCRIP),  R_SEPAR, 
				IFld(comerc|SUCXOBJ_SUCURSAL),  R_SEPAR, 
				SFld(comerc|SUCXOBJ_DESCRIP),   R_SEPAR, 								
				direc,	 					    R_SEPAR, 
			    (double)sthnac/100 + (double)sth50ac/100 + (double)sth100ac/100 + (double)sth100fac/100,  R_SEPAR,
			 	(double)sahnac/100 + (double)sah50ac/100 + (double)sah100ac/100 + (double)sah100fac/100
		 	   	);
    	    fprintf(fp, "\n");
    	    
		   	sthnac=0;
			sth50ac=0;
			sth100ac=0;
			sth100fac=0;
			
		 	sahnac=0;
		 	sah50ac=0;
		 	sah100ac=0;
		 	sah100fac=0;
        }

		// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
		// tener que hacerlo en cada funcion que imprime detalles (arch, reporte, y no print)
		ecli->sthn    = ecli->sthn    - ecli->sahn    - ecli->sehn;
		ecli->sth50   = ecli->sth50   - ecli->sah50   - ecli->seh50;
		ecli->sth100  = ecli->sth100  - ecli->sah100  - ecli->seh100;
		ecli->sth100f = ecli->sth100f - ecli->sah100f - ecli->seh100f;

		//  Acumular
		sthnac 		+= ecli->sthn;
		sth50ac 	+= ecli->sth50;
		sth100ac	+= ecli->sth100;
	    sth100fac	+= ecli->sth100f;

		sahnac	   	+= ecli->sahn;
		sah50ac	 	+= ecli->sah50;
		sah100ac	+= (ecli->sah100 + ecli->sah100f);
		sah100fac	+= (ecli->sah100f);

		empant = ecli->emp;
		cliant = ecli->cli;
		objant = ecli->obj;
		primero=FALSE;
	}

	// imprimir el ultimo
	if( 1 ) {
			SetKey(bill|CLIENTEbyCLIENTE, cliant);
			if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
					SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");

			SetKey(comerc|OBJETIVObyCLIENTE, cliant, objant);
			if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		 		SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");
            
			// primary key(emp, cliente, objetivo)
            SetKey(comerc|SUCXOBJbyEMP, empant, cliant, objant);
			if (GetRecord(comerc|SUCXOBJbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
		 		SetFld(comerc|SUCXOBJ_DESCRIP, "ERROR: Sucursal Inexistente");

	        sprintf(direc,"%s%s%s%s%ld%s%s%s%s%s%d%s%s",
	        	SFld(comerc|OBJETIVO_CALLE),		R_SEPAR,
	        	SFld(comerc|OBJETIVO_NRO),          R_SEPAR,
	        	LFld(comerc|OBJETIVO_LOCAL),       	R_SEPAR,
	        	GetDescLocali(IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV), LFld(comerc|OBJETIVO_LOCAL)), R_SEPAR,	        	
	        	SFld(comerc|OBJETIVO_CODPOS),		R_SEPAR,
	        	IFld(comerc|OBJETIVO_PROV),         R_SEPAR,
	        	GetDescProv(IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV))
	        	);
			fprintf(fp, "%d%s%ld%s%s%s%d%s%s%s%d%s%s%s%s%s%9.2f%s%9.2f",
				empant,							R_SEPAR, 
				cliant, 						R_SEPAR, 
				SFld(bill|CLIENTE_RAZSOC),      R_SEPAR, 
				objant,	                        R_SEPAR, 
				SFld(comerc|OBJETIVO_DESCRIP),  R_SEPAR, 
				IFld(comerc|SUCXOBJ_SUCURSAL),  R_SEPAR, 
				SFld(comerc|SUCXOBJ_DESCRIP),   R_SEPAR, 
				direc,	 					    R_SEPAR, 
			    (double)sthnac/100 + (double)sth50ac/100 + (double)sth100ac/100 + (double)sth100fac/100,  R_SEPAR,
			 	(double)sahnac/100 + (double)sah50ac/100 + (double)sah100ac/100 + (double)sah100fac/100
		 	   	);		 	   	
			fprintf(fp, "\n");

		   	sthnac=0;
			sth50ac=0;
			sth100ac=0;
			sth100fac=0;		    

		 	sahnac=0;
		 	sah50ac=0;
		 	sah100ac=0;
		 	sah100fac=0;
        }
}
