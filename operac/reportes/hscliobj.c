/********************************************************************
*
* MODULE & VERSION : @(#)hscliobj.c	1.61
* DATE             : 08/01/09
* TIME             : 17:15:59
*
* CREATED          : 15/02/99 Gloria
*
* DESCRIPTION:
*          Impresión de control de horas por cliente-objetivo-puesto-fecha
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*MODIFICACION 22/05/2002: Si para un dia tengo dos puestos de 10 horas,
					donde uno se cubre con 8 hs. normales + 2 extras y el otro con 10 normales.
					Y tengo 2 vigiladores (un de 8 norm y otro de 10 norm).
					Y cubren alreves sus puestos , no es improductividad.
					Si la diferencia en el dia es 0, no es improductividad (en el lis. consolidado por dia).
					Para esto se creo la funcion HayImproductividad
*MODIFICACION 07/03/2007: novando. Se procedio a modificar el modo de procesamiento de clientes, antes
                    se procesaba todos los clientes y luego se recorria la estructura para poder
                    realizar la salida, actualmente se procede a procesar un cliente y realizar la salida
                    segun el forms(impresora,archivo etc). El cambio radica en que cada vez que cambia 
                    un cliente se corre el puntero de la estructura para poder aprovechar dicha estructura 
                    y no seguir almacenando y asi llenarla (ahora memoria al aprovechar al maximo la estructura).
     				Todas las varibles que se usan en los cortes de control se inicializan al principio.
                    Las funciones para limpiar extructuras auxiliares y totalizasores se llaman todas
                    al principio del proceso y luego son llamadas segun lo los detalles pedidos en el forms.
                    Si se elige en el forms algun filtro por delegacion se procede a recorrer la tabla
                    delega del esquema billpro, esto hace que el proceso de los clientes se haga ordenado
                    por delegacion.
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "billpro.h"
#include "hscliobj.fmh"
#include "hscliobj.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "billpro.sch"
#include "sue.sch"
#include "filial.h"


#define R_SEPAR_A	"	"
#define R_SEPAR_R	"	"     //Salida para gestion, cuando es para Assist va ;

//Opciones del listado
#define DIFERENC     1
#define SERV_ADIC    2
#define HS_ACARGO    3
#define HS_NOPRES    4
#define TODOS        5
#define MAXCLI       100000
#define ERR_ARCHI    "No se pudo abrir el archivo."

#define MAX_MOTIVO 50
#define MAX_FERIADO 60000 // cantidad de feriados para el periodo seleccionado

/* Estructuras */
struct cliente {
	char deleg[6];
	long cli;
	int  obj;
	int  puesto;
	int  codint;
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
	long hsnmot[MAX_MOTIVO]; //Horas normales por motivo
	long hs50mot[MAX_MOTIVO]; //Horas al 50  por motivo
	long hs100mot[MAX_MOTIVO]; //Horas al 100 franco por motivo
	long hs100fmot[MAX_MOTIVO]; //Horas al 100 feriado por motivo
	// Diferencia Positivas
	double dpsvhn;
	double dpsvh50;
	double dpsvh100;
	double dpsvh100f;
	// Diferencia Negativas
	double dnsvhn;
	double dnsvh50;
	double dnsvh100;
	double dnsvh100f;
} pcli[MAXCLI], *ucli = pcli, *ecli, *ncli, *auxcli;

struct feriado {
	long cli;
	int obj;
	DATE fecha;
} Pferiado[MAX_FERIADO];

/* Funciones privadas */
void AbrirArchivo();
void AbrirReporte();
void ImprimirCabecera();
void SetearCabArch();
void ImprimirReporte();
//void ImprimirArchivo();
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
void LimpiarAcumDeleg();
void GenerarReporte( long p_cliente, int p_objetivo );
void CargarCliObj(int emp, char delega[5], long cliente, int objetivo, int puesto, int ptoint, DATE fecha, short nrosem,
				  int condic, int hn, int h50, int h100, int h100f, int pcodmot, int codint);
private int compcli(struct cliente *a, struct cliente *b);
private int compfec(struct cliente *a, struct cliente *b);
bool ValidoClienteObjetivo(long cliente, short objet, DATE p_fecha);
void LimpiarAcumSem();
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
bool HayImproductividad();
fm_status HelpDelegacion(form fm, fmfield fno, int row);
static int validatepto(void);
static void displaypto(char *buffer);
static void AgregarMotivo(int p_codmot);
static void OrdenarMotivo();
void CargoFeriado( int p_pais, int p_provi, DATE p_fecha, long p_cli, int p_obj );
bool EsFeriado( DATE p_fecha, long p_cli, int p_obj );
void Limpio_Feriado();
void RecalculoValores();  // Recalcula y obtiene subtotales de diferencias pos/neg.
void AjustoDiasFeriado(); // Ajusta los dias Feriados y Pre/Feriados.
void CambioPunteroInicio();
/*********************************** Declaraciones globales *******************************************/
FILE   *fp;
form   fm0;
report rp0;
schema comerc, operac, bill, sue, billpro;
long   cliant, prgant;
int    objant, ptoant;
short  semant;
DATE   fecant;
char   dcliant[100], dobjant[100], dptoant[100], delega[7], filial[6] = {'\0'}, descfilial[50] = {'\0'};
bool nodif;
char R_SEPAR[2];
char delegacion[6];
char delegant[6], Desc_Delega[31], Desc_Delega_ant[31];
/* Totalizadores para impresion en Archivo ASCII */
/* Tot Generales */
long 	tsvhn, tsvh50, tsvh100, tsvtot,
		tsthn, tsth50, tsth100, tsth100f, tsttot, tstnpres,
		tdhn,  tdh50,  tdh100,  tdh100f,
		tsahn, tsah50, tsah100, tsatot,
		tsehn, tseh50, tseh100, tsetot,
		tdphn, tdph50, tdph100, tdph100f,
		tdnhn, tdnh50, tdnh100, tdnh100f,
		thsnmot[MAX_MOTIVO], ths50mot[MAX_MOTIVO], ths100mot[MAX_MOTIVO], ths100fmot[MAX_MOTIVO];

/* Tot de Delegacion */
long	gsvhn, gsvh50, gsvh100, gsvtot,
		gsthn, gsth50, gsth100, gsth100f, gsttot, gstnpres,
		gdhn,  gdh50,  gdh100,  gdh100f,
		gsahn, gsah50, gsah100, gsatot,
		gsehn, gseh50, gseh100, gsetot,
		gdphn, gdph50, gdph100, gdph100f,
		gdnhn, gdnh50, gdnh100, gdnh100f,
		ghsnmot[MAX_MOTIVO], ghs50mot[MAX_MOTIVO], ghs100mot[MAX_MOTIVO], ghs100fmot[MAX_MOTIVO];

/* Tot de Cliente */
long	csvhn, csvh50, csvh100, csvtot,
		csthn, csth50, csth100, csth100f, csttot, cstnpres,
		cdhn,  cdh50,  cdh100,  cdh100f,
		csahn, csah50, csah100, csatot,
		csehn, cseh50, cseh100, csetot,
		cdphn, cdph50, cdph100, cdph100f,
		cdnhn, cdnh50, cdnh100, cdnh100f,
		chsnmot[MAX_MOTIVO], chs50mot[MAX_MOTIVO], chs100mot[MAX_MOTIVO], chs100fmot[MAX_MOTIVO];

/* Tot de Objetivo */
long	osvhn, osvh50, osvh100, osvtot,
		osthn, osth50, osth100, osth100f, osttot, ostnpres,
		odhn,  odh50,  odh100,  odh100f,
		osahn, osah50, osah100, osatot,
		osehn, oseh50, oseh100, osetot,
		odphn, odph50, odph100, odph100f,
		odnhn, odnh50, odnh100, odnh100f,
		ohsnmot[MAX_MOTIVO], ohs50mot[MAX_MOTIVO], ohs100mot[MAX_MOTIVO], ohs100fmot[MAX_MOTIVO];
		
/* Tot de Fecha */
long	fsvhn, fsvh50, fsvh100, fsvtot,
		fsthn, fsth50, fsth100, fsth100f, fsttot, fstnpres,
		fdhn,  fdh50,  fdh100,  fdh100f,
		fsahn, fsah50, fsah100, fsatot,
		fsehn, fseh50, fseh100, fsetot,
		fdphn, fdph50, fdph100, fdph100f,
		fdnhn, fdnh50, fdnh100, fdnh100f,
		fhsnmot[MAX_MOTIVO], fhs50mot[MAX_MOTIVO], fhs100mot[MAX_MOTIVO], fhs100fmot[MAX_MOTIVO];
		
/* Tot de Puesto */
long	psvhn, psvh50, psvh100, psvtot,
		psthn, psth50, psth100, psth100f, psttot, pstnpres,
		pdhn,  pdh50,  pdh100,  pdh100f,
		psahn, psah50, psah100, psatot,
		psehn, pseh50, pseh100, psetot,
		pdphn, pdph50, pdph100, pdph100f,
		pdnhn, pdnh50, pdnh100, pdnh100f,
		phsnmot[MAX_MOTIVO], phs50mot[MAX_MOTIVO], phs100mot[MAX_MOTIVO], phs100fmot[MAX_MOTIVO];

/* Tot Semana */
long	ssvhn, ssvh50, ssvh100, ssvtot,
		ssthn, ssth50, ssth100, ssth100f, ssttot, sstnpres,
		sdhn,  sdh50,  sdh100,  sdh100f,
		ssahn, ssah50, ssah100, ssatot,
		ssehn, sseh50, sseh100, ssetot,
		sdphn, sdph50, sdph100, sdph100f,
		sdnhn, sdnh50, sdnh100, sdnh100f,
		shsnmot[MAX_MOTIVO], shs50mot[MAX_MOTIVO], shs100mot[MAX_MOTIVO], shs100fmot[MAX_MOTIVO];

long cli_desde, cli_hasta;
int  obj_desde, obj_hasta;
int  motivo[MAX_MOTIVO]; // En este vector se acumulan los motivos y luego se ordenan de menor a mayor
bool first;
void ImprimeUltimoTit();
/**************************************** Programa principal **********************************************/
wcmd(hscliobj, 1.61 01/09/08)
{
	fm_cmd cmd;
	int v_i;
	long v_cliente;
	struct s_lisxusr_lib v_esta_lis;
    
	sue    = OpenSchema("sue",      IO_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);
	billpro= OpenSchema("billpro",  IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	fm0    = OpenForm  ("hscliobj", FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
    
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)	{
        
        if (cmd == FM_IGNORE)
        	continue;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++)
			motivo[v_i] = NULL_SHORT;

		Limpio_Feriado();

		strcpy ( delegacion, NULL_STR);
		cli_desde=FmIsNull( fm0, CLID) ? MIN_LONG : FmLFld( fm0, CLID);
		cli_hasta=FmIsNull( fm0, CLIH) ? MAX_LONG : FmLFld( fm0, CLIH);
		obj_desde=FmIsNull( fm0, OBJD) ? MIN_SHORT: FmIFld( fm0, OBJD);
		obj_hasta=FmIsNull( fm0, OBJH) ? MAX_SHORT: FmIFld( fm0, OBJH);
		
		InicioListaTipoExcepcion();

		/*************************************************************/
		/* Apertura Segun Tipo de salida                             */
		/*************************************************************/
		if (*FmSFld(fm0, SALIDA)=='A' || *FmSFld(fm0, SALIDA)=='R')
		{
	 		AbrirArchivo();
	 		// tipo de R_SEPAR
	 		strcpy(R_SEPAR, (*FmSFld(fm0, SALIDA)=='A'? R_SEPAR_A : R_SEPAR_R));
		}
		else
		{
			AbrirReporte();
			ImprimirCabecera();
		}

		/*** Recorro Todos los Objetivos dentro del rango del Forms *******************/

		LimpiarAcumTot();      // Limpio a este nivel, los cortes de control se encargan 
		LimpiarAcumDeleg();    // de llamar a algunas de estas funciones si es necesario.
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumFch();
		LimpiarAcumPto();
		LimpiarAcumSem();

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		semant = NULL_SHORT;              // Inicializo variable cortes de control solo una vez
		cliant = NULL_LONG;               // funcionaria como antes del cambio
		objant = ptoant = NULL_SHORT;
		fecant = NULL_DATE;
		prgant = NULL_LONG;
		strcpy( delegant, NULL_STR);
		first=TRUE;
		v_cliente=-1;
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		while(ProximoListaXusr(&v_esta_lis)) {

			if (v_esta_lis.cliente < cli_desde)
				continue;
			if (v_esta_lis.cliente > cli_hasta)
				break;

			if (v_esta_lis.objetivo < obj_desde)
				continue;
			if (v_esta_lis.objetivo > obj_hasta)
				continue;


			if (!ValidaFilial(v_esta_lis.cliente, v_esta_lis.objetivo, FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
	        
			SetKey(comerc|OBJETIVObyCLIENTE, v_esta_lis.cliente, v_esta_lis.objetivo);
			GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

//	       	fprintf(stderr, "A Cliente %ld - Objetivo %d - Filial %s Dia %.3D Hora %.3T\n", v_esta_lis.cliente, v_esta_lis.objetivo, SFld(comerc|OBJETIVO_FILIAL), Today(), Hour());
			
			if( v_esta_lis.cliente!=v_cliente && v_cliente!=-1)
		 		ImprimirReporte();           // Imprime Reporte

			GenerarReporte(v_esta_lis.cliente, v_esta_lis.objetivo);

			v_cliente=LFld(comerc|OBJETIVO_CLIENTE);

		}
		FinListaXusr();

		ImprimirReporte();                  // Imprime Ultimo Reporte
		ImprimeUltimoTit();                 // Imprime Ultimos Totales

	    // +++++++++++++++++++++++
		// Finalizacion de proceso
	    // +++++++++++++++++++++++
		if (*FmSFld(fm0, SALIDA)=='A' || *FmSFld(fm0, SALIDA)=='R')
			fclose(fp);
		else
			EndReport(rp0);
    
    
    	break;
    
    }
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

void AbrirReporte()
{
	rp0 = OpenReport("hscliobj", RP_EABORT|RP_NOBEGIN);

	//Si la salida es Impresora
	if (*FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);

	//Si la salida es Terminal
	if (*FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	BeginReport(rp0, 1, NULL_STR);
}

void ImprimirCabecera()
{
	RpSetLFld(rp0, RCLID,     FmLFld(fm0, CLID));
	RpSetLFld(rp0, RCLIH,     FmLFld(fm0, CLIH));
	RpSetIFld(rp0, ROBJD,     FmIFld(fm0, OBJD));
	RpSetIFld(rp0, ROBJH,     FmIFld(fm0, OBJH));
	RpSetFld (rp0, RDCLIOBJD, FmSFld(fm0, DOBJD));
	RpSetFld (rp0, RDCLIOBJH, FmSFld(fm0, DOBJH));
	RpSetDFld(rp0, RFECHAD,   FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH,   FmDFld(fm0, FECHAH));
	RpSetFld (rp0, RDETA,     FmSFld(fm0, DOPCION));
	RpSetFld (rp0, RCONS,     FmSFld(fm0, DESCDET));
	RpSetFld (rp0, RTIPOBJ,   FmSFld(fm0, DTIPOBJ));
	RpSetIFld (rp0, RRETRO,   FmIFld(fm0, FRETRO));

	if (!FmIsNull(fm0, FECSTD)) {
		RpSetFld (rp0, RDFECHA,	"Fecha Base");
		RpSetDFld (rp0, RFECBASE,	FmDFld(fm0, FECSTD));
	}
	else {
		RpSetFld (rp0, RDFECHA,	NULL_STR);
		RpSetDFld (rp0, RFECBASE,	NULL_DATE);
	}
}

/*************************************************************************************/
/* El cliente y objetivo ahora se pasan por parametro, da el efecto de procesar      */
/* cliente por cliente                                                               */
/*************************************************************************************/
void GenerarReporte(long p_cliente, int p_objetivo)
{
	dbcursor cpto, cparte, cexc, cretro, crexc;
	//int  puesto;
	char buffer[100];
	DATE fecha;
	short tipoexc;
	int v_codmot = NULL_SHORT;

	cpto   = CreateCursor(PUESTOSbyCLIENTE, IO_NOT_LOCK);
	cexc   = CreateCursor(EXCEPCIONbyEMP,   IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMP,       IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyEMP,       IO_NOT_LOCK);
	crexc  = CreateCursor(RETROEXCbyEMP,    IO_NOT_LOCK);

	/*** Leo los puestos para informar aunque no tenga nada cargado *******************/
	SetCursorFrom(cpto, p_cliente, p_objetivo, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cpto, p_cliente, p_objetivo, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cpto) != ERROR) {
		if ((IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > FmDFld(fm0, FECHAH)) ||
			(!IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FFINAL) < FmDFld(fm0, FECHAD))) {
			continue;
		}

		if (IFld(PUESTOS_TIPPTO) == BRIGADASINARMAS || IFld(PUESTOS_TIPPTO) == BRIGADACONARMAS)
			continue;

		/* Si es MIMP no lo leo */
		if (!IsNull(operac|PUESTOS_PADREINT))
			continue;

		if (!ValidoClienteObjetivo(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET), NULL_DATE))
			continue;

		if (!ObjActivo(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) && !IsNull(comerc|OBJETIVO_FECHAF) &&
			DFld(comerc|OBJETIVO_FECHAF) < FmDFld(fm0, FECHAD))
			continue;

//		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) != BRIGADA) ||
//		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) == BRIGADA))
//			continue;

		strcpy(delegacion, GetDelegaObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)));

		sprintf (buffer, "Procesando Puestos de Cliente %ld Objetivo %d", LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		for (fecha = FmDFld(fm0, FECHAD); fecha <= FmDFld(fm0, FECHAH); fecha++) {
			if (IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > fecha)
				continue;

			if (IsNull(PUESTOS_DIA1 + DiaNumero(fecha)))
				continue;

			CargoFeriado(IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV), fecha, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) );
			
			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET), IFld(PUESTOS_TIPPTO),
						 IFld(PUESTOS_PUESTO), fecha, NroSemana(FmDFld(fm0, FECHAD), fecha),
						 NULL_SHORT, 0, 0, 0, 0, IFld(PUESTOS_CODMOT), IFld(PUESTOS_CODINT));
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cpto);

	/*** Leo el parte *******************/

	SetCursorFrom(cparte, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAD),
						  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cparte, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAH),
						  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cparte) != ERROR) {
		if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if (!ValidoClienteObjetivo(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO),DFld(PARTE_DIA)))
			continue;

//		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) != BRIGADA) ||
//		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) == BRIGADA))
//			continue;


		strcpy(delegacion, GetDelegaObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)));

		sprintf (buffer, "Procesando Parte de Cliente %ld Objetivo %d", LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		SetKey(PUESTOSbyCLIENTE, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), IFld(PARTE_PTOSER), IFld(PARTE_PUESTO));
		if (GetRecord(PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			v_codmot = IFld(PUESTOS_CODMOT);
		else
			v_codmot = NULL_SHORT;
			
		CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), IFld(PARTE_PTOSER),
					 IFld(PARTE_PUESTO), DFld(PARTE_DIA), NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)), NULL_SHORT,
					 IFld(PARTE_HSNOR), IFld(PARTE_HS50), IFld(PARTE_HS100F), IFld(PARTE_HS100FE), v_codmot, IFld(PARTE_PUESTO));
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);

	/*** Leo los retroactivos *******************/
	if (FmIFld(fm0, FRETRO)) {
		SetCursorFrom(cretro, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAD),
							  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAH),
							  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cretro) != ERROR) {
			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (!ValidoClienteObjetivo(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA)))
				continue;

//			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) != BRIGADA) ||
//			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) == BRIGADA))
//				continue;


			strcpy(delegacion, GetDelegaObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)));

			sprintf(buffer, "Procesando Retro Cliente %ld Objetivo %d", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO));
			FmSetFld(fm0, COMENT, buffer);
			WiRefresh();

			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), IFld(RETRO_PTOSER),
						 IFld(RETRO_PUESTO), DFld(RETRO_DIA), NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)), NULL_SHORT, 
						 IFld(RETRO_DHSNOR), IFld(RETRO_DHS50), IFld(RETRO_DHS100F), IFld(RETRO_DHS100FE), NULL_SHORT, IFld(RETRO_PUESTO));
		}
		SetCursorFrom(crexc, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (crexc, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(crexc) != ERROR) {
			if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) || DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (!ValidoClienteObjetivo(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO),DFld(RETROEXC_DIA)))
				continue;

//			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) != BRIGADA) ||
//			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) == BRIGADA))
//				continue;


			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), IFld(RETROEXC_MOTIVO));
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC)
				continue;

			strcpy(delegacion, GetDelegaObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)));

			sprintf (buffer, "Procesando RetroExc de Cliente %ld Objetivo %d", LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			SetKey(comerc|OBJETIVO, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO),
						 IFld(RETROEXC_PTOSER), IFld(RETROEXC_PUESTO), DFld(RETROEXC_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(RETROEXC_DIA)),
						 IFld(RETROEXC_CONDIC), IFld(RETROEXC_DHORAS), IFld(RETROEXC_DHS50),
						 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
						 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0,
						 NULL_SHORT, IFld(RETROEXC_PUESTO));
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
		5 - TODOS LOS DATOS
		Se saco el 03/05/2000 por pedido de Javier Daffunchio */

	SetCursorFrom(cexc, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAD),
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cexc, FmIFld(fm0, EMP), p_cliente, p_objetivo, FmDFld(fm0, FECHAH),
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cexc) != ERROR) {
		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if (!ValidoClienteObjetivo(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),DFld(EXCEPCION_DIA)))
			continue;

//		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) != BRIGADA) ||
//		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) == BRIGADA))
//			continue;


		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC), IFld(EXCEPCION_MOTIVO));
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC)
			continue;

		strcpy(delegacion, GetDelegaObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)));

		sprintf(buffer, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		FmSetFld(fm0, COMENT, buffer);
		WiRefresh();

		SetKey(comerc|OBJETIVO, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
					 IFld(EXCEPCION_PTOSER), IFld(EXCEPCION_PUESTO), DFld(EXCEPCION_DIA),
					 NroSemana(FmDFld(fm0, FECHAD), DFld(EXCEPCION_DIA)), 
					 IFld(EXCEPCION_CONDIC), IFld(EXCEPCION_HORAS), IFld(EXCEPCION_HS50),
					 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
					 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0,
					 NULL_SHORT, IFld(EXCEPCION_PUESTO));
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cexc);

	OrdenarMotivo();
}

// Carga el vector de PCLI.
// Si viene de PARTE h100 contiene horas extras de francos y 
// h100f de feriados.
// si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene
// vacio.
void CargarCliObj(int emp, char deleg[6], long cliente, int objetivo, int puesto, int ptoint, DATE fecha, short nrosem,
				  int condic, int hn, int h50, int h100, int h100f, int p_codmot, int codint)
{
	long svhn, svh50, svh100, svh100f;
	int v_i;

	svhn = svh50 = svh100 = svh100f = 0;

	AgregarMotivo(p_codmot);


	for (ecli = pcli; ecli < ucli; ecli++)
		if (strcmp(ecli->deleg, deleg)==0 && ecli->cli == cliente   && ecli->obj == objetivo && ecli->puesto == puesto && ecli->fecha == fecha && ecli->codint == codint) {
			break;
		}
	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI])      
			Error("Tabla interna saturada. Max %d", MAXCLI);

		strcpy( ucli->deleg, deleg);
		ucli->cli     = cliente;
		ucli->obj     = objetivo;
		ucli->puesto  = puesto;
		ucli->fecha   = fecha;
		ucli->nrosem  = nrosem;
		ucli->codint  = codint;

		ucli->stdhs  = 0; 

		ucli->seh50   = 0;
		ucli->seh100  = 0;
		ucli->seh100f = 0;
		ucli->sehn    = 0;

		ucli->sah50   = 0;
		ucli->sah100  = 0;
		ucli->sah100f = 0;
		ucli->sahn    = 0;

		ucli->sth50   = 0;
		ucli->sth100  = 0;
		ucli->sth100f = 0;
		ucli->sthn    = 0;

		ucli->dpsvhn   =0;
		ucli->dpsvh50  =0;
		ucli->dpsvh100 =0;
		ucli->dpsvh100f=0;

		ucli->dnsvhn    =0;
		ucli->dnsvh50   =0;
		ucli->dnsvh100  =0;
		ucli->dnsvh100f =0;

		ucli->svhn    = 0;
		ucli->svh50   = 0;
		ucli->svh100  = 0;
		ucli->svh100f = 0;

		for(v_i=0; v_i < MAX_MOTIVO; v_i++){
			ucli->hsnmot[v_i]   = 0;
			ucli->hs50mot[v_i]  = 0;
			ucli->hs100mot[v_i] = 0;
			ucli->hs100fmot[v_i]= 0;
		}

		//FER
//		fprintf(stderr, "emp %d , cliente %ld, objetivo %d, puesto %d, fecha %.3D, fecha std %.3D codint %d\n", FmIFld(fm0, EMP), cliente, objetivo, puesto, fecha, FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD), codint);


		GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, puesto, fecha,
						FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD),
						&svhn, &svh50, &svh100, &svh100f, codint); //Agregue la apertura por codint


		//FER
//		fprintf(stderr, "svhn %ld, &svh50 %ld, &svh100 %ld, &svh100f %ld, codint %d\n", svhn, svh50, svh100, svh100f, codint);

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


	if (p_codmot >= MAX_MOTIVO)
		Error("Tabla de motivo interna saturada. Max %d intentando guardar %d", MAX_MOTIVO, p_codmot);

	if (p_codmot!=NULL_SHORT){
		ecli->hsnmot[p_codmot]   += hn;
		ecli->hs50mot[p_codmot]  += h50;
		ecli->hs100mot[p_codmot] += h100;
		ecli->hs100fmot[p_codmot]+= h100f;
	}
}

// Imprime los datos contenidos en el vector PCLI, de acuerdo a las opciones
// de consolidacion y salida por.
void ImprimirReporte()
{
	if (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) == 'D')
		qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compfec);
	else
		qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetIFld(rp0, PRI,  TRUE);
		RpSetIFld(rp0, FCH,  FALSE);
		RpSetIFld(rp0, PTO,  FALSE);
		RpSetIFld(rp0, OBJ,  FALSE);
		RpSetIFld(rp0, CLI,  FALSE);
		RpSetIFld(rp0, DELE,  TRUE);
		RpSetIFld(rp0, FDAT, FALSE);

		switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE ))) {
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
	
	RecalculoValores();  // Recalcula y obtiene subtotales de diferencias posi/neg.
	AjustoDiasFeriado(); // Ajusta los dias Feriados y Pre/Feriados.

	for (ecli = pcli; ecli < ucli; ecli++) {

		if (ecli->svhn  == 0 && ecli->svh50 == 0 && ecli->svh100 == 0 && ecli->svh100f == 0 &&
	 		ecli->sthn  == 0 && ecli->sth50 == 0 && ecli->sth100 == 0 && ecli->sth100f == 0 &&
	 		ecli->svhn  == 0 && ecli->svh50 == 0 && ecli->svh100 == 0 && ecli->svh100f == 0 &&
	 		ecli->sahn  == 0 && ecli->sah50 == 0 && ecli->sah100 == 0 && ecli->sah100f == 0 &&
	 		ecli->sehn  == 0 && ecli->seh50 == 0 && ecli->seh100 == 0 && ecli->seh100f == 0 &&
            ecli->stdhs == 0)
            continue;

//        WALTER
		if (strcmp(delegant, ecli->deleg)!=0 || cliant != ecli->cli || objant != ecli->obj || fecant!= ecli->fecha){
			nodif = FALSE;
			if (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) == 'D' && !HayImproductividad()) {
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

		if( strcmp( delegant, ecli->deleg)!=0) {
			strcpy( Desc_Delega, strcmp( ecli->deleg, NULL_STR) == 0 ? NULL_STR : GetDescDeleg(ecli->deleg));
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
				strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), ecli->cli, ecli->obj));
				RpSetLFld(rp0, RCLI,  ecli->cli);
				RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				RpSetIFld(rp0, ROBJ,  ecli->obj);
				RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
				RpSetIFld(rp0, RPTO,  ecli->puesto);
				RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
				RpSetIFld(rp0, RCODINT, ecli->codint);
				RpSetDFld(rp0, RFCH,  ecli->fecha);
				DoReport (rp0, ZCLI);
				DoReport (rp0, ZOBJ);
				DoReport (rp0, ZPTO);
				RpSetIFld(rp0, PRI, FALSE);
			}
			else {

				switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
		else {
			PushRecord(comerc|OBJETIVO);
			ImprimirTitArch();
			PopRecord(comerc|OBJETIVO);
		}


		if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
			ImprimirNoPrint();
		}
		else {
			AcumularTotalizadores();
		}

		if (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) == 'F') {
			if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
				ImprimirDetalle();
			else{
				ImprimirDetArch();
			}
		}
	
		strcpy( delegant , ecli->deleg);
		cliant = ecli->cli;
		strcpy(dcliant, SFld(bill|CLIENTE_RAZSOC));
		objant = ecli->obj;
		strcpy(dobjant, SFld(comerc|OBJETIVO_DESCRIP));
	   	strcpy(delega, GetDelegaObj(ecli->cli, ecli->obj));
		fecant = ecli->fecha;
		ptoant = ecli->puesto;
		semant = ecli->nrosem;
		strcpy(dptoant, SFld(comerc|TPTOSER_DESCRIP));
		prgant = LFld(comerc|OBJETIVO_PROGRAM);

	}

	CambioPunteroInicio();
}

// Abre el archivo ascii indicado en pantalla :)
void AbrirArchivo()
{         
	if ((fp = fopen(FmSFld(fm0, NOMARCH),"w")) == (FILE*)NULL)
		Error(ERR_ARCHI);
	else if (*FmSFld(fm0, SALIDA)!='R')
		SetearCabArch();

	if (*FmSFld(fm0, SALIDA) == 'R') {
		SetIFld(sue|EMPS_EMP, FmIFld(fm0, EMP));
		GetRecord(sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK);
	}
}

// Setea la cabecera del listado en el archivo ascii.
void SetearCabArch()
{
    int v_i;

	fprintf (fp, "Fecha Desde %.3D Hasta %.3D \t %s \t %s \t %B considera retroactivos \t Ajusta Dif por Feriado %B \t",
			FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), FmSFld(fm0, DOPCION), FmSFld(fm0, DTIPOBJ),
			FmIFld(fm0, FRETRO),FmIFld(fm0, DIFE));

	if (!FmIsNull(fm0, FECSTD)) {
	    fprintf (fp, "Fecha Base %.3D ", FmDFld(fm0, FECSTD));
	}

	fprintf (fp, " \n");

	fprintf(fp, "Delegación\tDescripción\tFilial\tDescripción\tCliente\tRazon Social\tObjetivo\tDescr Obj\tPuesto\tDescr Puesto\tCod. Puesto\tFecha\tEstandard Vendido\t\t\t\tEstandar Trabajado\t\t\t\t\t\tDiferencia 1-2\t\t\t\tServicios Adicionales\t\t\t\tA cargo Empresa\t\t\t\tDiferencia Positiva\t\t\t\tDiferencia Negativa\t\t\t\tProgramador\n");
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tH100F\tTotal\tNPres\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tH100F\t");

	for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
		fprintf(fp, "\tMotivo %d Hn", motivo[v_i]);
		fprintf(fp, "\tMotivo %d H50", motivo[v_i]);
		fprintf(fp, "\tMotivo %d H100", motivo[v_i]);
		fprintf(fp, "\tMotivo %d H100F", motivo[v_i]);
	} 
	fprintf(fp, "\n");

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
	RpSetIFld(rp0, RCODINT, ecli->codint);
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
	int v_i;
    
	if (*FmSFld(fm0, SALIDA) == 'R') {
		fprintf(fp, "%d%s%s%s%d%s", IFld(sue|EMPS_PAIS),              R_SEPAR,
									GetDescPais(IFld(sue|EMPS_PAIS)), R_SEPAR,
									FmIFld(fm0, EMP),                 R_SEPAR);
	}

	strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), ecli->cli, ecli->obj));
	strcpy(descfilial, GetDescFilial(filial));

	fprintf(fp, "%s%s%s%s%s%s%s%s%ld%s%s%s%d%s%s%s%d%s%s%s%d%s%.1D%s", 
				ecli->deleg,                    R_SEPAR, 
				Desc_Delega,                    R_SEPAR, 
				filial,                         R_SEPAR, 
				descfilial,                     R_SEPAR, 
				ecli->cli, 						R_SEPAR, 
				SFld(bill|CLIENTE_RAZSOC),      R_SEPAR, 
				ecli->obj,                      R_SEPAR, 
				SFld(comerc|OBJETIVO_DESCRIP),  R_SEPAR, 
				ecli->puesto,                   R_SEPAR, 
				SFld(comerc|TPTOSER_DESCRIP),   R_SEPAR,
				ecli->codint,                   R_SEPAR, 
				ecli->fecha,					R_SEPAR );

	fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%s",
			 	(double)ecli->svhn / 100.0, 	R_SEPAR,
			 	(double)ecli->svh50 / 100.0,    R_SEPAR,
			 	(double)(ecli->svh100 + ecli->svh100f) / 100.0,	R_SEPAR,
			 	(double)(ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) / 100.0,	R_SEPAR,
				(double	)ecli->sthn / 100.0,		R_SEPAR,
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
				(double)ecli->dpsvhn   / 100.0, R_SEPAR,
				(double)ecli->dpsvh50  / 100.0, R_SEPAR,
				(double)ecli->dpsvh100 / 100.0, R_SEPAR,
				(double)ecli->dpsvh100f/ 100.0, R_SEPAR,
				
				//Diferencias Negativas
				(double)ecli->dnsvhn   / 100.0, R_SEPAR,
				(double)ecli->dnsvh50  / 100.0, R_SEPAR,
				(double)ecli->dnsvh100 / 100.0, R_SEPAR,
				(double)ecli->dnsvh100f/ 100.0, R_SEPAR,


			 	IsNull(comerc|OBJETIVO_PROGRAM) ? NULL_STR : GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));

	for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
		
		fprintf(fp, "%s%9.2f", R_SEPAR, (double)ecli->hsnmot[motivo[v_i]]   / 100.0);
		fprintf(fp, "%s%9.2f", R_SEPAR, (double)ecli->hs50mot[motivo[v_i]]  / 100.0);
		fprintf(fp, "%s%9.2f", R_SEPAR, (double)ecli->hs100mot[motivo[v_i]] / 100.0);
		fprintf(fp, "%s%9.2f", R_SEPAR, (double)ecli->hs100fmot[motivo[v_i]]/ 100.0);
	}

	fprintf(fp, "\n");
}

// Imprime los titulos y totalizadores segun haya cambiado en el corte
// de control el cliente objetivo o puesto y segun sea la opcion de 
// consolidado elegida.
void ImprimirTitArch()
{ 
	/* Cambio el cliente */
	if (strcmp( delegant, ecli->deleg)!=0) {
		switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
			case 'C':
				TotCli();
				TitCli();
				//if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
				//	RpSetLFld(rp0, RCLI, ecli->cli);
				//	RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				//}
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
		LimpiarAcumDeleg();
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumFch();
		LimpiarAcumPto();
		LimpiarAcumSem();

	}
	else{
		if (cliant != ecli->cli) {
			switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
// *** cambio seguro
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
				switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
// *** cambio seguro modifique tit Obj y listo. (puede que cause algun problema pero segun probe no).
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

					switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
					case 'D' :
						TotFch();
						break;
					}
					LimpiarAcumFch();
				}
				/* Cambio la semana */
				if (semant != ecli->nrosem) {
					switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
					switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
}

//Limpia los acumuladores de cliente/totales/puesto y objetivos
void LimpiarAcumTot()
{
	int v_i;

		tsvhn = tsvh50 = tsvh100 = tsvtot   = 0;
		tsthn = tsth50 = tsth100 = tsth100f = tsttot = tstnpres = 0;
		tdhn  = tdh50  = tdh100  = tdh100f  = 0;
		tsahn = tsah50 = tsah100 = tsatot   = 0;
		tsehn = tseh50 = tseh100 = tsetot   = 0;
		tdphn = tdph50 = tdph100 = tdph100f = 0;
		tdnhn = tdnh50 = tdnh100 = tdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			thsnmot[v_i]   = 0;
			ths50mot[v_i]  = 0;
			ths100mot[v_i] = 0;
			ths100fmot[v_i]= 0;
		}
}

void LimpiarAcumDeleg()
{
	int v_i;

		gsvhn = gsvh50 = gsvh100 = gsvtot   = 0;
		gsthn = gsth50 = gsth100 = gsth100f = gsttot = gstnpres = 0;
		gdhn  = gdh50  = gdh100  = gdh100f  = 0;
		gsahn = gsah50 = gsah100 = gsatot   = 0;
		gsehn = gseh50 = gseh100 = gsetot   = 0;
		gdphn = gdph50 = gdph100 = gdph100f = 0;
		gdnhn = gdnh50 = gdnh100 = gdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			ghsnmot[v_i]   = 0;
			ghs50mot[v_i]  = 0;
			ghs100mot[v_i] = 0;
			ghs100fmot[v_i]= 0;
		}
}

void LimpiarAcumCli()
{
	int v_i;

		csvhn = csvh50 = csvh100 = csvtot   = 0;
		csthn = csth50 = csth100 = csth100f = csttot = cstnpres = 0;
		cdhn  = cdh50  = cdh100  = cdh100f  = 0;
		csahn = csah50 = csah100 = csatot   = 0;
		csehn = cseh50 = cseh100 = csetot   = 0;
		cdphn = cdph50 = cdph100 = cdph100f = 0;
		cdnhn = cdnh50 = cdnh100 = cdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			chsnmot[v_i]   = 0;
			chs50mot[v_i]  = 0;
			chs100mot[v_i] = 0;
			chs100fmot[v_i]= 0;
		}
}

void LimpiarAcumObj()
{
	int v_i;

		osvhn = osvh50 = osvh100 = osvtot   = 0;
		osthn = osth50 = osth100 = osth100f = osttot = ostnpres = 0;
		odhn  = odh50  = odh100  = odh100f  = 0;
		osahn = osah50 = osah100 = osatot   = 0;
		osehn = oseh50 = oseh100 = osetot   = 0; 
		odphn = odph50 = odph100 = odph100f = 0;
		odnhn = odnh50 = odnh100 = odnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			ohsnmot[v_i]   = 0;
			ohs50mot[v_i]  = 0;
			ohs100mot[v_i] = 0;
			ohs100fmot[v_i]= 0;
		}
}

void LimpiarAcumFch()
{
	int v_i;

		fsvhn = fsvh50 = fsvh100 = fsvtot   = 0;
		fsthn = fsth50 = fsth100 = fsth100f = fsttot = fstnpres = 0;
		fdhn  = fdh50  = fdh100  = fdh100f  = 0;
		fsahn = fsah50 = fsah100 = fsatot   = 0;
		fsehn = fseh50 = fseh100 = fsetot   = 0; 
		fdphn = fdph50 = fdph100 = fdph100f = 0;
		fdnhn = fdnh50 = fdnh100 = fdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			fhsnmot[v_i]   = 0;
			fhs50mot[v_i]  = 0;
			fhs100mot[v_i] = 0;
			fhs100fmot[v_i]= 0;
		}
}

void LimpiarAcumPto()
{
	int v_i;

		psvhn = psvh50 = psvh100 = psvtot   = 0;
		psthn = psth50 = psth100 = psth100f = psttot = pstnpres = 0;
		pdhn  = pdh50  = pdh100  = pdh100f  = 0;
		psahn = psah50 = psah100 = psatot   = 0;
		psehn = pseh50 = pseh100 = psetot   = 0; 
		pdphn = pdph50 = pdph100 = pdph100f = 0;
		pdnhn = pdnh50 = pdnh100 = pdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			phsnmot[v_i]   = 0;
			phs50mot[v_i]  = 0;
			phs100mot[v_i] = 0;
			phs100fmot[v_i]= 0;
		}
}

void LimpiarAcumSem()
{
	int v_i;

		ssvhn = ssvh50 = ssvh100 = ssvtot   = 0;
		ssthn = ssth50 = ssth100 = ssth100f = ssttot = sstnpres = 0;
		sdhn  = sdh50  = sdh100  = sdh100f  = 0;
		ssahn = ssah50 = ssah100 = ssatot   = 0;
		ssehn = sseh50 = sseh100 = ssetot   = 0; 
		sdphn = sdph50 = sdph100 = sdph100f = 0;
		sdnhn = sdnh50 = sdnh100 = sdnh100f = 0;

		for (v_i=0; v_i<MAX_MOTIVO; v_i++) {
			shsnmot[v_i]   = 0;
			shs50mot[v_i]  = 0;
			shs100mot[v_i] = 0;
			shs100fmot[v_i]= 0;
		}
}

// Acumula los totalizadores de cliente-objtivo-puesto y gnerales con datos tomados del vector ecli
// en la posicion corrinente que es la que se esta tratando.
void AcumularTotalizadores()
{
	int totsvh, totsth, v_i;

	csvhn += ecli->svhn;
	osvhn += ecli->svhn;
	fsvhn += ecli->svhn;
	psvhn += ecli->svhn;
	ssvhn += ecli->svhn;
	tsvhn += ecli->svhn;
	gsvhn += ecli->svhn;

	csvh50 += ecli->svh50;
	osvh50 += ecli->svh50;
	fsvh50 += ecli->svh50;
	psvh50 += ecli->svh50;
	ssvh50 += ecli->svh50;
	tsvh50 += ecli->svh50;
	gsvh50 += ecli->svh50;

	csvh100 += ecli->svh100 + ecli->svh100f;
	osvh100 += ecli->svh100 + ecli->svh100f;
	fsvh100 += ecli->svh100 + ecli->svh100f;
	psvh100 += ecli->svh100 + ecli->svh100f;
	ssvh100 += ecli->svh100 + ecli->svh100f;
	tsvh100 += ecli->svh100 + ecli->svh100f;
	gsvh100 += ecli->svh100 + ecli->svh100f;

	totsvh  = ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f;
	csvtot += totsvh;
	osvtot += totsvh;
	fsvtot += totsvh;
	psvtot += totsvh;
	ssvtot += totsvh;
	tsvtot += totsvh;
	gsvtot += totsvh;

	csthn += ecli->sthn;
	osthn += ecli->sthn;
	fsthn += ecli->sthn;
	psthn += ecli->sthn;
	ssthn += ecli->sthn;
	tsthn += ecli->sthn;
	gsthn += ecli->sthn;

	csth50 += ecli->sth50;
	osth50 += ecli->sth50;
	fsth50 += ecli->sth50;
	psth50 += ecli->sth50;
	ssth50 += ecli->sth50;
	tsth50 += ecli->sth50;
	gsth50 += ecli->sth50;

	csth100 += ecli->sth100;
	osth100 += ecli->sth100;
	fsth100 += ecli->sth100;
	psth100 += ecli->sth100;
	ssth100 += ecli->sth100;
	tsth100 += ecli->sth100;
	gsth100 += ecli->sth100;

	csth100f += ecli->sth100f;
	osth100f += ecli->sth100f;
	fsth100f += ecli->sth100f;
	psth100f += ecli->sth100f;
	ssth100f += ecli->sth100f;
	tsth100f += ecli->sth100f;
	gsth100f += ecli->sth100f;

	totsth  = ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f;
	csttot += totsth;
	osttot += totsth;
	fsttot += totsth;
	psttot += totsth;
	ssttot += totsth;
	tsttot += totsth;
	gsttot += totsth;

	cstnpres += totsvh - totsth;
	ostnpres += totsvh - totsth;
	fstnpres += totsvh - totsth;
	pstnpres += totsvh - totsth;
	sstnpres += totsvh - totsth;
	tstnpres += totsvh - totsth;
	gstnpres += totsvh - totsth;

	cdhn = cdhn + (ecli->svhn - ecli->sthn);
	odhn += ecli->svhn - ecli->sthn;
	fdhn += ecli->svhn - ecli->sthn;
	pdhn += ecli->svhn - ecli->sthn;
	sdhn += ecli->svhn - ecli->sthn;
	tdhn += ecli->svhn - ecli->sthn;
	gdhn += ecli->svhn - ecli->sthn;


	cdh50 += ecli->svh50 - ecli->sth50;
	odh50 += ecli->svh50 - ecli->sth50;
	fdh50 += ecli->svh50 - ecli->sth50;
	pdh50 += ecli->svh50 - ecli->sth50;
	sdh50 += ecli->svh50 - ecli->sth50;
	tdh50 += ecli->svh50 - ecli->sth50;
	gdh50 += ecli->svh50 - ecli->sth50;

	cdh100 += ecli->svh100 - ecli->sth100;
	odh100 += ecli->svh100 - ecli->sth100;
	fdh100 += ecli->svh100 - ecli->sth100;
	pdh100 += ecli->svh100 - ecli->sth100;
	sdh100 += ecli->svh100 - ecli->sth100;
	tdh100 += ecli->svh100 - ecli->sth100;
	gdh100 += ecli->svh100 - ecli->sth100;

	cdh100f += ecli->svh100f - ecli->sth100f;
	odh100f += ecli->svh100f - ecli->sth100f;
	fdh100f += ecli->svh100f - ecli->sth100f;
	pdh100f += ecli->svh100f - ecli->sth100f;
	sdh100f += ecli->svh100f - ecli->sth100f;
	tdh100f += ecli->svh100f - ecli->sth100f;
	gdh100f += ecli->svh100f - ecli->sth100f;

	csahn += ecli->sahn;
	osahn += ecli->sahn;
	fsahn += ecli->sahn;
	psahn += ecli->sahn;
	ssahn += ecli->sahn;
	tsahn += ecli->sahn;
	gsahn += ecli->sahn;

	csah50 += ecli->sah50;
	osah50 += ecli->sah50;
	fsah50 += ecli->sah50;
	psah50 += ecli->sah50;
	ssah50 += ecli->sah50;
	tsah50 += ecli->sah50;
	gsah50 += ecli->sah50;

	csah100 += ecli->sah100 + ecli->sah100f;
	osah100 += ecli->sah100 + ecli->sah100f;
	fsah100 += ecli->sah100 + ecli->sah100f;
	psah100 += ecli->sah100 + ecli->sah100f;
	ssah100 += ecli->sah100 + ecli->sah100f;
	tsah100 += ecli->sah100 + ecli->sah100f;
	gsah100 += ecli->sah100 + ecli->sah100f;

	csatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	osatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	fsatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	psatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	ssatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	tsatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	gsatot += ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;

	csehn += ecli->sehn;
	osehn += ecli->sehn;
	fsehn += ecli->sehn;
	psehn += ecli->sehn;
	ssehn += ecli->sehn;
	tsehn += ecli->sehn;
	gsehn += ecli->sehn;

	cseh50 += ecli->seh50;
	oseh50 += ecli->seh50;
	fseh50 += ecli->seh50;
	pseh50 += ecli->seh50;
	sseh50 += ecli->seh50;
	tseh50 += ecli->seh50;
	gseh50 += ecli->seh50;

	cseh100 += ecli->seh100+ecli->seh100f;
	oseh100 += ecli->seh100+ecli->seh100f;
	fseh100 += ecli->seh100+ecli->seh100f;
	pseh100 += ecli->seh100+ecli->seh100f;
	sseh100 += ecli->seh100+ecli->seh100f;
	tseh100 += ecli->seh100+ecli->seh100f;
	gseh100 += ecli->seh100+ecli->seh100f;

	csetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	osetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	fsetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	psetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	ssetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	tsetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	gsetot += ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;

	/* Acumula por Motivo */

	for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++){
		chsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		ohsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		fhsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		phsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		shsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		thsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;
		ghsnmot[motivo[v_i]] += ecli->hsnmot[motivo[v_i]] ;

		chs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		ohs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		fhs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		phs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		shs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		ths50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;
		ghs50mot[motivo[v_i]] += ecli->hs50mot[motivo[v_i]] ;

		chs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		ohs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		fhs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		phs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		shs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		ths100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;
		ghs100mot[motivo[v_i]] += ecli->hs100mot[motivo[v_i]] ;

		chs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		ohs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		fhs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		phs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		shs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		ths100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
		ghs100fmot[motivo[v_i]] += ecli->hs100fmot[motivo[v_i]] ;
	}
	if (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) == 'D' && nodif) {
		return;
	}

	cdphn   += ecli->dpsvhn;
	odphn   += ecli->dpsvhn;
	fdphn   += ecli->dpsvhn;
	pdphn   += ecli->dpsvhn;
	sdphn   += ecli->dpsvhn;
	tdphn   += ecli->dpsvhn;
	gdphn   += ecli->dpsvhn;

	cdph50  += ecli->dpsvh50;
	odph50  += ecli->dpsvh50;
	fdph50  += ecli->dpsvh50;
	pdph50  += ecli->dpsvh50;
	sdph50  += ecli->dpsvh50;
	tdph50  += ecli->dpsvh50;
	gdph50  += ecli->dpsvh50;

	cdph100 += ecli->dpsvh100;
	odph100 += ecli->dpsvh100;
	fdph100 += ecli->dpsvh100;
	pdph100 += ecli->dpsvh100;
	sdph100 += ecli->dpsvh100;
	tdph100 += ecli->dpsvh100;
	gdph100 += ecli->dpsvh100;

	cdph100f += ecli->dpsvh100f;
	odph100f += ecli->dpsvh100f;
	fdph100f += ecli->dpsvh100f;
	pdph100f += ecli->dpsvh100f;
	sdph100f += ecli->dpsvh100f;
	tdph100f += ecli->dpsvh100f;
	gdph100f += ecli->dpsvh100f;

	cdnhn   += ecli->dnsvhn;
	odnhn   += ecli->dnsvhn;
	fdnhn   += ecli->dnsvhn;
	pdnhn   += ecli->dnsvhn;
	sdnhn   += ecli->dnsvhn;
	tdnhn   += ecli->dnsvhn;
	gdnhn   += ecli->dnsvhn;

	cdnh50  += ecli->dnsvh50;
	odnh50  += ecli->dnsvh50;
	fdnh50  += ecli->dnsvh50;
	pdnh50  += ecli->dnsvh50;
	sdnh50  += ecli->dnsvh50;
	tdnh50  += ecli->dnsvh50;
	gdnh50  += ecli->dnsvh50;

	cdnh100 += ecli->dnsvh100;
	odnh100 += ecli->dnsvh100;
	fdnh100 += ecli->dnsvh100;
	pdnh100 += ecli->dnsvh100;
	sdnh100 += ecli->dnsvh100;
	tdnh100 += ecli->dnsvh100;
	gdnh100 += ecli->dnsvh100;

	cdnh100f += ecli->dnsvh100f;
	odnh100f += ecli->dnsvh100f;
	fdnh100f += ecli->dnsvh100f;
	pdnh100f += ecli->dnsvh100f;
	sdnh100f += ecli->dnsvh100f;
	tdnh100f += ecli->dnsvh100f;
	gdnh100f += ecli->dnsvh100f;


}


// Imprime el total de un cliente en el archivo de salida
void TotCli()
{
	int v_i;

	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {
	
	    strcpy( Desc_Delega_ant, strcmp( delegant, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( delegant));

	  	strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), cliant, objant));
		strcpy(descfilial, GetDescFilial(filial));
	  	
	  	fprintf(fp, "%s%s%s%s%s%s%s%s%8.8ld%s%s%s%s%s%s%sTotal Cliente %8.8ld %20s%s",
				delegant, R_SEPAR, Desc_Delega_ant, R_SEPAR, filial, R_SEPAR, descfilial, R_SEPAR, cliant, R_SEPAR, dcliant, R_SEPAR,R_SEPAR,R_SEPAR,R_SEPAR,R_SEPAR, cliant, dcliant, R_SEPAR);

		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s",
			(double)csvhn    / 100.0, R_SEPAR, (double)csvh50   / 100.0, R_SEPAR, (double)csvh100 / 100.0, R_SEPAR, (double)csvtot / 100.0,  R_SEPAR,
			(double)csthn    / 100.0, R_SEPAR, (double)csth50   / 100.0, R_SEPAR,
			(double)csth100  / 100.0, R_SEPAR, (double)csth100f / 100.0, R_SEPAR, (double)csttot  / 100.0, R_SEPAR,
			(double)cstnpres / 100.0, R_SEPAR, (double)cdhn     / 100.0, R_SEPAR, (double)cdh50   / 100.0, R_SEPAR,
			(double)cdh100   / 100.0, R_SEPAR, (double)cdh100f  / 100.0, R_SEPAR,
			(double)csahn    / 100.0, R_SEPAR, (double)csah50   / 100.0, R_SEPAR, (double)csah100 / 100.0, R_SEPAR, (double)csatot / 100.0,  R_SEPAR,
			(double)csehn    / 100.0, R_SEPAR, (double)cseh50   / 100.0, R_SEPAR, (double)cseh100 / 100.0, R_SEPAR, (double)csetot / 100.0,  R_SEPAR,
			(double)cdphn    / 100.0, R_SEPAR, (double)cdph50   / 100.0, R_SEPAR, (double)cdph100 / 100.0, R_SEPAR, (double)cdph100f/ 100.0, R_SEPAR,
			(double)cdnhn    / 100.0, R_SEPAR, (double)cdnh50   / 100.0, R_SEPAR, (double)cdnh100 / 100.0, R_SEPAR, (double)cdnh100f/ 100.0, R_SEPAR);

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, chsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, chs50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, chs100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, chs100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");

	}
//	else
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
		DoReport(rp0, ZTOTCLI);
}

// Imprime el total de un objetivo en el archivo de salida
void TotObj()
{
	int v_i;

	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {
		
	    strcpy( Desc_Delega_ant, strcmp( delegant, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( delegant));
		
		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), cliant, objant));
		strcpy(descfilial, GetDescFilial(filial));

		fprintf(fp, "%s%s%s%s%s%s%s%s%8.8ld%s%s%s%4.4d%s%s%s%sCliente %8.8ld %20s%sTotal Objetivo %4.4d %20s%s",
			delegant, R_SEPAR, Desc_Delega_ant, R_SEPAR, filial, R_SEPAR, descfilial, R_SEPAR, cliant, R_SEPAR,dcliant, R_SEPAR,objant, R_SEPAR,dobjant, R_SEPAR,R_SEPAR,cliant, dcliant, R_SEPAR, objant, dobjant, R_SEPAR);
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s",
			(double)osvhn    / 100.0, R_SEPAR, (double)osvh50   / 100.0,R_SEPAR, (double)osvh100 / 100.0,R_SEPAR,
			(double)osvtot   / 100.0, R_SEPAR, (double)osthn    / 100.0,R_SEPAR, (double)osth50  / 100.0,R_SEPAR,
			(double)osth100  / 100.0, R_SEPAR, (double)osth100f / 100.0,R_SEPAR, (double)osttot  / 100.0,R_SEPAR,
			(double)ostnpres / 100.0, R_SEPAR, (double)odhn     / 100.0,R_SEPAR, (double)odh50   / 100.0,R_SEPAR,
			(double)odh100   / 100.0, R_SEPAR, (double)odh100f  / 100.0,R_SEPAR, (double)osahn   / 100.0,R_SEPAR,
			(double)osah50   / 100.0, R_SEPAR, (double)osah100  / 100.0,R_SEPAR, (double)osatot  / 100.0,R_SEPAR,
			(double)osehn    / 100.0, R_SEPAR, (double)oseh50   / 100.0,R_SEPAR, (double)oseh100 / 100.0,R_SEPAR,  (double)osetot / 100.0,R_SEPAR,
			(double)odphn    / 100.0, R_SEPAR, (double)odph50   / 100.0,R_SEPAR, (double)odph100 / 100.0,R_SEPAR,  (double)odph100f/ 100.0,R_SEPAR,
			(double)odnhn    / 100.0, R_SEPAR, (double)odnh50   / 100.0,R_SEPAR, (double)odnh100 / 100.0,R_SEPAR,  (double)odnh100f/ 100.0, R_SEPAR);

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, ohsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ohs50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ohs100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ohs100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");
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
	int v_i;

	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {

	    strcpy( Desc_Delega_ant, strcmp( delegant, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( delegant));

		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), cliant, objant));
		strcpy(descfilial, GetDescFilial(filial));

		fprintf(fp, "%s%s%s%s%s%s%s%s%8.8ld%s%s%s%4.4d%s%s%s%.1D%s%sTotal Fecha %.1D%s",
			delega, R_SEPAR, Desc_Delega_ant, R_SEPAR, filial, R_SEPAR, descfilial, R_SEPAR,cliant, R_SEPAR, dcliant, R_SEPAR,objant, R_SEPAR, dobjant,R_SEPAR,fecant, R_SEPAR,R_SEPAR,fecant, R_SEPAR);

		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%s",
			(double)fsvhn    / 100.0, R_SEPAR, (double)fsvh50   / 100.0, R_SEPAR, (double)fsvh100 / 100.0,R_SEPAR, 
			(double)fsvtot   / 100.0, R_SEPAR, (double)fsthn    / 100.0, R_SEPAR, (double)fsth50  / 100.0,R_SEPAR, 
			(double)fsth100  / 100.0, R_SEPAR, (double)fsth100f / 100.0, R_SEPAR, (double)fsttot  / 100.0,R_SEPAR, 
			(double)fstnpres / 100.0, R_SEPAR,  (double)fdhn     / 100.0,R_SEPAR,  (double)fdh50   / 100.0,R_SEPAR, 
			(double)fdh100   / 100.0, R_SEPAR, (double)fdh100f  / 100.0, R_SEPAR, (double)fsahn   / 100.0,R_SEPAR, 
			(double)fsah50   / 100.0, R_SEPAR, (double)fsah100  / 100.0, R_SEPAR, (double)fsatot  / 100.0,R_SEPAR, 
			(double)fsehn    / 100.0, R_SEPAR, (double)fseh50   / 100.0, R_SEPAR, (double)fseh100 / 100.0, R_SEPAR, (double)fsetot / 100.0,R_SEPAR, 
			(double)fdphn    / 100.0, R_SEPAR, (double)fdph50   / 100.0, R_SEPAR, (double)fdph100 / 100.0, R_SEPAR, (double)fdph100f/ 100.0,R_SEPAR, 
			(double)fdnhn    / 100.0, R_SEPAR, (double)fdnh50   / 100.0, R_SEPAR, (double)fdnh100 / 100.0, R_SEPAR, (double)fdnh100f/ 100.0,R_SEPAR, 
			GetNombreLeg(FmIFld(fm0, EMP), prgant));

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, fhsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f%s", R_SEPAR, fhs50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f%s", R_SEPAR, fhs100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f%s", R_SEPAR, fhs100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");

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
	int v_i;
	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {
		
	    strcpy( Desc_Delega_ant, strcmp( delegant, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( delegant));
		
		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), cliant, objant));
		strcpy(descfilial, GetDescFilial(filial));

		fprintf(fp, "%s%s%s%s%s%s%s%s%8.8ld%s%s%s%4.4d%s%s%s%4.4d%s%sTotal Puesto %4.4d %20s%s",
			delegant, R_SEPAR, Desc_Delega_ant, R_SEPAR, filial, R_SEPAR, descfilial, R_SEPAR,cliant, R_SEPAR, dcliant, R_SEPAR, objant, R_SEPAR, dobjant,R_SEPAR, ptoant, R_SEPAR, R_SEPAR, ptoant, dptoant, R_SEPAR );
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s",
			(double)psvhn    / 100.0, R_SEPAR, (double)psvh50   / 100.0, R_SEPAR, (double)psvh100 / 100.0, R_SEPAR, 
			(double)psvtot   / 100.0, R_SEPAR, (double)psthn    / 100.0, R_SEPAR, (double)psth50  / 100.0, R_SEPAR, 
			(double)psth100  / 100.0, R_SEPAR, (double)psth100f / 100.0, R_SEPAR, (double)psttot  / 100.0, R_SEPAR, 
			(double)pstnpres / 100.0, R_SEPAR, (double)pdhn     / 100.0, R_SEPAR, (double)pdh50   / 100.0, R_SEPAR, 
			(double)pdh100   / 100.0, R_SEPAR, (double)pdh100f  / 100.0, R_SEPAR, (double)psahn   / 100.0, R_SEPAR, 
			(double)psah50   / 100.0, R_SEPAR, (double)psah100  / 100.0, R_SEPAR, (double)psatot  / 100.0, R_SEPAR, 
			(double)psehn    / 100.0, R_SEPAR, (double)pseh50   / 100.0, R_SEPAR, (double)pseh100 / 100.0, R_SEPAR, (double)psetot / 100.0,  R_SEPAR, 
			(double)pdphn    / 100.0, R_SEPAR, (double)pdph50   / 100.0, R_SEPAR, (double)pdph100 / 100.0, R_SEPAR, (double)pdph100f/ 100.0, R_SEPAR, 
			(double)pdnhn    / 100.0, R_SEPAR, (double)pdnh50   / 100.0, R_SEPAR, (double)pdnh100 / 100.0, R_SEPAR, (double)pdnh100f/ 100.0, R_SEPAR);

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, phsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, phs50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, phs100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, phs100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");

	}
//	else
	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R')
		DoReport(rp0, ZTOTPTO);
}

// Imprime el total de la semana
void TotSem()
{
	DATE fdesde, fhasta;
	int v_i;

	#ifndef _NOVIA_VER_2_0
 		return;		
	#endif

	SemanaDesdeHasta(fecant, &fdesde, &fhasta);

	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R') && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {
		
	    strcpy( Desc_Delega_ant, strcmp( delegant, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( delegant));

		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), cliant, objant));
		strcpy(descfilial, GetDescFilial(filial));

		fprintf(fp, "%s%s%s%s%s%s%s%s%8.8ld%s%s%s%4.4d%s%s%s    %s%sTotal Semana del %.3D al %.3D%s",
			delegant, R_SEPAR, Desc_Delega_ant, R_SEPAR, filial, R_SEPAR, descfilial, R_SEPAR,cliant, R_SEPAR, dcliant, R_SEPAR, objant, R_SEPAR, dobjant,R_SEPAR, R_SEPAR, R_SEPAR, fdesde, fhasta,R_SEPAR );
		
		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s",
			(double)ssvhn    / 100.0, R_SEPAR, (double)ssvh50   / 100.0, R_SEPAR, (double)ssvh100 / 100.0, R_SEPAR, 
			(double)ssvtot   / 100.0, R_SEPAR, (double)ssthn    / 100.0, R_SEPAR, (double)ssth50  / 100.0, R_SEPAR, 
			(double)ssth100  / 100.0, R_SEPAR, (double)ssth100f / 100.0, R_SEPAR, (double)ssttot  / 100.0, R_SEPAR, 
			(double)sstnpres / 100.0, R_SEPAR, (double)sdhn     / 100.0, R_SEPAR, (double)sdh50   / 100.0, R_SEPAR, 
			(double)sdh100   / 100.0, R_SEPAR, (double)sdh100f  / 100.0, R_SEPAR, (double)ssahn   / 100.0, R_SEPAR, 
			(double)ssah50   / 100.0, R_SEPAR, (double)ssah100  / 100.0, R_SEPAR, (double)ssatot  / 100.0, R_SEPAR, 
			(double)ssehn    / 100.0, R_SEPAR, (double)sseh50   / 100.0, R_SEPAR, (double)sseh100 / 100.0, R_SEPAR, (double)ssetot / 100.0,  R_SEPAR, 
			(double)sdphn    / 100.0, R_SEPAR, (double)sdph50   / 100.0, R_SEPAR, (double)sdph100 / 100.0, R_SEPAR, (double)sdph100f/ 100.0, R_SEPAR, 
			(double)sdnhn    / 100.0, R_SEPAR, (double)sdnh50   / 100.0, R_SEPAR, (double)sdnh100 / 100.0, R_SEPAR, (double)sdnh100f/ 100.0, R_SEPAR);

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, shsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, shs50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, shs100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, shs100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");

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
	int v_i;


	if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA)=='R')  && *FmSFld(fm0, FmRefFld(fm0, R_DETALLE)) != 'F') {

		fprintf(fp, "%s%s%s%s%s%s%s%s%s%sTotal GENERAL%s",R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR, R_SEPAR );

		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s",
			(double)tsvhn    / 100.0, R_SEPAR, (double)tsvh50   / 100.0, R_SEPAR, (double)tsvh100 / 100.0, R_SEPAR, 
			(double)tsvtot   / 100.0, R_SEPAR, (double)tsthn    / 100.0, R_SEPAR, (double)tsth50  / 100.0, R_SEPAR, 
			(double)tsth100  / 100.0, R_SEPAR, (double)tsth100f / 100.0, R_SEPAR, (double)tsttot  / 100.0, R_SEPAR, 
			(double)tstnpres / 100.0, R_SEPAR, (double)tdhn     / 100.0, R_SEPAR, (double)tdh50   / 100.0, R_SEPAR, 
			(double)tdh100   / 100.0, R_SEPAR, (double)tdh100f  / 100.0, R_SEPAR, (double)tsahn   / 100.0, R_SEPAR, 
			(double)tsah50   / 100.0, R_SEPAR, (double)tsah100  / 100.0, R_SEPAR, (double)tsatot  / 100.0, R_SEPAR, 
			(double)tsehn    / 100.0, R_SEPAR, (double)tseh50   / 100.0, R_SEPAR, (double)tseh100 / 100.0, R_SEPAR, (double)tsetot / 100.0,  R_SEPAR, 
			(double)tdphn    / 100.0, R_SEPAR, (double)tdph50   / 100.0, R_SEPAR, (double)tdph100 / 100.0, R_SEPAR, (double)tdph100f/ 100.0, R_SEPAR, 
			(double)tdnhn    / 100.0, R_SEPAR, (double)tdnh50   / 100.0, R_SEPAR, (double)tdnh100 / 100.0, R_SEPAR, (double)tdnh100f/ 100.0, R_SEPAR);

			for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
				fprintf(fp, "%s%9.2f", R_SEPAR, thsnmot[motivo[v_i]]   / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ths50mot[motivo[v_i]]  / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ths100mot[motivo[v_i]] / 100.0);
				fprintf(fp, "%s%9.2f", R_SEPAR, ths100fmot[motivo[v_i]]/ 100.0);
			}
			fprintf(fp, "\n");
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
			RpSetIFld(rp0, RPTO, ecli->puesto);
			RpSetFld(rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
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
	return	strcmp(a->deleg, b->deleg)<0  ? -1 : strcmp(a->deleg, b->deleg)>0  ? 1 :
			a->cli    < b->cli    ? -1 : a->cli    > b->cli    ? 1 :
			a->obj    < b->obj    ? -1 : a->obj    > b->obj    ? 1 :
			a->puesto < b->puesto ? -1 : a->puesto > b->puesto ? 1 :
			a->fecha  < b->fecha  ? -1 : a->fecha  > b->fecha  ? 1 :
			0;
}

private int compfec(struct cliente *a, struct cliente *b)
{
	return	strcmp( a->deleg ,b->deleg) < 0  ? -1 : strcmp( a->deleg, b->deleg)>0  ? 1 :
			a->cli    < b->cli    ? -1 : a->cli    > b->cli    ? 1 :
			a->obj    < b->obj    ? -1 : a->obj    > b->obj    ? 1 :
			a->fecha  < b->fecha  ? -1 : a->fecha  > b->fecha  ? 1 :
			0;
}

bool ValidoClienteObjetivo(long cliente, short objet, DATE p_fecha)
{

	/* Solamente leo el cliente objetibo si no es el que tengo leido */
	if(cliente!=LFld(comerc|OBJETIVO_CLIENTE) || objet!=IFld(comerc|OBJETIVO_OBJET)) {
		SetKey(comerc|OBJETIVObyCLIENTE, cliente, objet);
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR){
 			//Warning("ERROR: Objetivo Inexistente %ld-%d ", cliente, objet);
			return FALSE;
		}
	}

	CargoFeriado( IFld(comerc|OBJETIVO_PAIS),IFld(comerc|OBJETIVO_PROV), p_fecha, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) );


	/* Valido Empresa */
	if (IFld(comerc|OBJETIVO_EMP)!=FmIFld(fm0, EMP))
		return FALSE;

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

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	break;
    case CLID:
	       	InicClientesXusr();
    	break;
    case CLIH:
    	break;
    case OBJD:
	   	InicObjetivosXusr(FmLFld(fm, CLID, row), FmIFld(fm, EMP, row));
    	break;
    case OBJH:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIH, row), FmIFld(fm, EMP, row));
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
	case SALIDA:
		if ((*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA) == 'R') && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "hscliobj.txt");

		if (*FmSFld(fm0, SALIDA) == 'R') {
			SetDecPointToPoint();
		} 
		else {
			SetDecPointToComma();
		}

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
	case I_DETA1:
	case I_DETA2:
		#ifndef _NOVIA_VER_2_0
			if (*FmSFld(fm0, fno) == 'S') {
				WiMsg ("Esta opcion no esta disponible en esta version ");
				return FM_REDO;
			}
		#endif
	break;
	case CLID:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLID, row)), row);
    break;
    case CLIH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIH, row)), row);
   	break;
    case OBJD:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLID, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLID, row), FmIFld(fm, OBJD, row)), row);
	break;
    case OBJH:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIH, row));
  		else
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row), FmIFld(fm, OBJH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
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

fm_status HelpDelegacion(form fm, fmfield fno, int row)
{
	static dbcursor CUR;
	int n;

	CUR = CreateCursor(comerc|PROVXDELbyDELEG, IO_NOT_LOCK|IO_CONTROL_BREAK);
	SetCursorFrom(CUR, LOW_VALUE,  MIN_SHORT);
	SetCursorTo  (CUR, HIGH_VALUE, MAX_SHORT);
	n = PopUpDbMenu(10, 70, "Puestos de Trabajo", CUR, 1, validatepto, displaypto);

	if (n >= 0) { 
		FmSetFld  (fm, fno, SFld(comerc|PROVXDEL_DELEG), row);
		FmSetFld  (fm, fno+1, GetDescDeleg (SFld (comerc|PROVXDEL_DELEG)), row);
		FmShowFlds(fm, fno, fno+1, row);
	}
	DeleteCursor(CUR);
	if (n >= 0)
		return FM_OK;
	return FM_ERROR;
}

static int validatepto(void)
{
	return TRUE;
}

static void displaypto(char *buffer)
{
	sprintf(buffer,"%-5.5s %-30.30s",
			SFld(comerc|PROVXDEL_DELEG), GetDescDeleg (SFld(comerc|PROVXDEL_DELEG)));
}

static void AgregarMotivo(int p_codmot)
{
	int v_i;
	
	for (v_i=0; v_i<MAX_MOTIVO; v_i++){
		if (motivo[v_i]==NULL_SHORT)
			break;
		if (motivo[v_i] == p_codmot)
			break;
	}
	motivo[v_i]=p_codmot;
	
}
static void OrdenarMotivo()
{
	/* Ordeno el vecto de motivos de menor a mayor con el metodo de ordenamiento de burbujas */

	int v_i, v_j, v_aux;
	bool ordena;

	for (v_i=0; v_i<MAX_MOTIVO && motivo[v_i]!=NULL_SHORT; v_i++) {
		ordena=FALSE;
		for (v_j=v_i+1; v_j<MAX_MOTIVO && motivo[v_j]!=NULL_SHORT; v_j++) {
			
			if (motivo[v_i]>motivo[v_j]) {
				v_aux=motivo[v_i];
				motivo[v_i]=motivo[v_j];
				motivo[v_j]=v_aux;

				ordena=TRUE;
			}
		}
		if (!ordena)
			break;
	}

}

/*******************************************************************************/
/* Tomo los dias previos al feriado como feriados propiamente dichos (fecha -1)*/
/*                                                                             */
/*******************************************************************************/
void CargoFeriado( int p_pais, int p_provi, DATE p_fecha, long p_cli, int p_obj )
{
	int v_i;

	if( FeriadoNovia( p_fecha, p_pais, p_provi)==FALSE )
		return;

	for( v_i=0; v_i < MAX_FERIADO ; v_i++)
	{
		if( p_cli == Pferiado[v_i].cli && p_obj == Pferiado[v_i].obj &&
			p_fecha == Pferiado[v_i].fecha)
		{
			break;
		}
		
		if( Pferiado[v_i].cli==NULL_LONG )	// busco algun lugar libre
		{
			Pferiado[v_i].cli   = p_cli;
			Pferiado[v_i].obj   = p_obj;
			Pferiado[v_i].fecha = p_fecha;
		
			v_i++;

			break;
		}        
	}

	return;
}


bool EsFeriado( DATE p_fecha, long p_cli, int p_obj )
{
	int v_i;
	bool v_es_feriado;
	
	v_es_feriado=FALSE;

	for( v_i=0; v_i < MAX_FERIADO && Pferiado[v_i].cli!=NULL_LONG; v_i++)
	{
		if( p_cli == Pferiado[v_i].cli && p_obj == Pferiado[v_i].obj &&
			p_fecha == Pferiado[v_i].fecha)
		{
	 		v_es_feriado=TRUE;
			break;	
		}
  	}

	return v_es_feriado;
}
                                
void Limpio_Feriado()
{
	int v_i;
	
	for( v_i=0; v_i < MAX_FERIADO && Pferiado[v_i].cli!=NULL_LONG; v_i++)
	{
		Pferiado[v_i].cli  = NULL_LONG;
		Pferiado[v_i].obj  = NULL_SHORT;
		Pferiado[v_i].fecha= NULL_DATE;
  	}
}

void RecalculoValores()
{
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
		
		/******************************************************/
		// Obtengo los subtotales de los diferencias
		/******************************************************/
		
		// Diferencias Positivas
		ecli->dpsvhn   +=(ecli->svhn    - ecli->sthn   ) < 0 ? 0 : (ecli->svhn   - ecli->sthn);
		ecli->dpsvh50  +=(ecli->svh50   - ecli->sth50  ) < 0 ? 0 : (ecli->svh50  - ecli->sth50);
		ecli->dpsvh100 +=(ecli->svh100  - ecli->sth100 ) < 0 ? 0 : (ecli->svh100 - ecli->sth100);
		ecli->dpsvh100f+=(ecli->svh100f - ecli->sth100f) < 0 ? 0 : (ecli->svh100f- ecli->sth100f);
		
		// Diferencias Negativas
		ecli->dnsvhn   +=(ecli->svhn   - ecli->sthn   ) > 0 ? 0 : (ecli->svhn - ecli->sthn);
		ecli->dnsvh50  +=(ecli->svh50  - ecli->sth50  ) > 0 ? 0 : (ecli->svh50 - ecli->sth50);
		ecli->dnsvh100 +=(ecli->svh100 - ecli->sth100 ) > 0 ? 0 : (ecli->svh100 - ecli->sth100);
		ecli->dnsvh100f+=(ecli->svh100f- ecli->sth100f) > 0 ? 0 : (ecli->svh100f - ecli->sth100f);

	}
}

/************************************************************************/
/* Realiza el ajuste propiamente dicho, recorre la lista con 2 punteros */
/* para encontrar                                                       */
/************************************************************************/
void AjustoDiasFeriado()
{

	if( FmIFld(fm0, DIFE)==FALSE ) // si no pide ajuste.
		return;

	if( Pferiado[0].cli == NULL_LONG ) // si el periodo no posee feriados (posicion 0).
		return;

	for (ecli = pcli; ecli < ucli; ecli++)
	{
		/***************************************************************************************/
		// Si busca el dia anterior y posterior al feriado.
		/***************************************************************************************/
		if( EsFeriado( ecli->fecha,ecli->cli,ecli->obj)==TRUE)
		{
			// recorro nuevamente la lista.
			for (auxcli = pcli; auxcli < ucli; auxcli++)
			{
                /**************************************/ 
				// busco el PRE-FERIADO o POST-FERIADO 
                /**************************************/ 				
				// OJO TENER CUIDADO CON LOS NOMBRES DE LOS CAMPOS,
				// SE PARECEN MUCHO Y SE CONFUNDE

				if( ((ecli->fecha-1)==auxcli->fecha &&
					ecli->cli==auxcli->cli && ecli->obj==auxcli->obj) ||
					((ecli->fecha+1)==auxcli->fecha && 
					ecli->cli==auxcli->cli && ecli->obj==auxcli->obj) )
				{
                	/*
                	WiMsg("aca EMPIEZA 1 posi ECLI fec %.3D %.2f neg %.2f a50 %.2f neg %.2f al100%.2f neg %.2f al100f %.2f neg %.2f posi AUXCLI fec %.3D %.2f neg %.2f a50 %.2f neg %.2f al100%.2f neg %.2f al100f %.2f neg %.2f",
                	ecli->fecha  ,ecli->dpsvhn  ,ecli->dnsvhn  ,ecli->dpsvh50  ,ecli->dnsvh50  ,ecli->dpsvh100  ,ecli->dnsvh100  ,ecli->dpsvh100f  ,ecli->dnsvh100f,
                	auxcli->fecha,auxcli->dpsvhn,auxcli->dnsvhn,auxcli->dpsvh50,auxcli->dnsvh50,auxcli->dpsvh100,auxcli->dnsvh100,auxcli->dpsvh100f,auxcli->dnsvh100f);
                    */

 					// +++++++++++++++++++++++++++++
 					// Limpio campos si corresponden 
 					// +++++++++++++++++++++++++++++ 

					// +++++++++++++++++++++
					// Diferencias Positivas
					// +++++++++++++++++++++

					if( (ecli->dpsvhn+auxcli->dnsvhn)==0 )
					{
						ecli->dpsvhn  = 0;
						auxcli->dnsvhn= 0;
					}

					if(	(ecli->dpsvh50+auxcli->dnsvh50)==0 )
				  	{
						ecli->dpsvh50  = 0;
 						auxcli->dnsvh50= 0;
					}

					if( (ecli->dpsvh100+auxcli->dnsvh100)==0 )
					{
						ecli->dpsvh100  = 0;
 						auxcli->dnsvh100= 0;
					}

					if( (ecli->dpsvh100f+auxcli->dnsvh100f)==0 )
					{
						ecli->dpsvh100f  = 0;	
 						auxcli->dnsvh100f= 0;
					} 					

					// +++++++++++++++++++++
					// Diferencias Negativas
					// +++++++++++++++++++++

					if( (ecli->dnsvhn+auxcli->dpsvhn)==0 )
					{
						ecli->dnsvhn  = 0;
						auxcli->dpsvhn= 0;
					}

					if(	(ecli->dnsvh50+auxcli->dpsvh50)==0 )
				  	{
						ecli->dnsvh50  = 0;	
 						auxcli->dpsvh50= 0;
					}
					
					if( (ecli->dnsvh100+auxcli->dpsvh100)==0 )
					{
						ecli->dnsvh100  = 0;
 						auxcli->dpsvh100= 0;
					}

					if( (ecli->dnsvh100f+auxcli->dpsvh100f)==0 )
					{
						ecli->dnsvh100f  = 0;
 						auxcli->dpsvh100f= 0;
					} 					
				}
			}
		}
	}
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Solamente iguala el puntero del ultimo cliente al primero, generando asi
// la reutilizacion de la estructura.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CambioPunteroInicio()
{
	ucli = pcli;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Imprime los ultimos titulos y totales despues de la ultima impresion 
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void ImprimeUltimoTit()
{

	if (*FmSFld(fm0, SALIDA) != 'A' && *FmSFld(fm0, SALIDA)!='R') {
		RpSetLFld(rp0, RCLI,  cliant);
		RpSetFld (rp0, RDCLI, dcliant);
		RpSetIFld(rp0, ROBJ,  objant);
		RpSetFld (rp0, RDOBJ, dobjant);
		RpSetDFld(rp0, RFCH,  fecant);
		RpSetIFld(rp0, RPTO,  ptoant);
		RpSetFld (rp0, RDPTO, dptoant);
	}

	switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
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
