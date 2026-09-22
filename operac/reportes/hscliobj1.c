/********************************************************************
*
* MODULE & VERSION : @(#)hscliobj.c	1.38
* DATE             : 02/06/07
* TIME             : 16:00:34
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
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "billpro.h"
#include "hscliobj1.rph"
#include "hscliobj.fmh"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"

#define R_SEPAR_A	"	"
#define R_SEPAR_R	"	"     //Salida para gestion, cuando es para Assist va ;

//Opciones del listado
#define DIFERENC     1
#define SERV_ADIC    2
#define HS_ACARGO    3
#define HS_NOPRES    4
#define TODOS        5
#define MAXCLI       90000
#define ERR_ARCHI    "No se pudo abrir el archivo."

/*---------------------------------Declaraciones Listas Enlazadas------------------------------------------*/
typedef struct stndeleg * tndeleg;
typedef struct stncliente * tncliente;
typedef struct stnobjetivo * tnobjetivo;
typedef struct stnpuesto * tnpuesto;
typedef struct stnnrosem * tnnrosem;
typedef struct stnfecha * tnfecha;
typedef struct stnpuedel * tnpuedel;

typedef struct stndeleg {
	char	deleg[6];
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tncliente	ncliente;
	tndeleg	nsig;
} stndeleg;

typedef struct stncliente {
	long	cliente;
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tnobjetivo	nobjetivo;
	tncliente	nsig;
} stncliente;

typedef struct stnobjetivo {
	int		objetivo;
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tnpuesto	npuesto;
	tnobjetivo	nsig;
} stnobjetivo;

typedef struct stnpuesto {
	int		puesto;
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tnnrosem	nnrosem;
	tnpuesto	nsig;
} stnpuesto;

typedef struct stnnrosem {
	int		nrosem;
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tnfecha	nfecha;
	tnnrosem	nsig;
} stnnrosem;

typedef struct stnfecha {
	DATE	fecha;
	long	svhn;
	long	svh50;
	long	svh100;
	long	svh100f;
	long	sthn;
	long	sth50;
	long	sth100;
	long	sth100f;
	long	sahn;
	long	sah50;
	long	sah100;
	long	sah100f;
	long	sehn;
	long	seh50;
	long	seh100;
	long	seh100f;
	long	stdhs;
	tnpuedel npuedel;
	tnfecha	nsig;
} stnfecha;

typedef struct stnpuedel {
	int		puedel;
	tnpuedel	nsig;
} stnpuedel;

/* Funciones Privadas */
static tndeleg AcuNDeleg(tndeleg, tndeleg*);
static tncliente AcuNCliente(tncliente, tncliente*);
static tnobjetivo AcuNObjetivo(tnobjetivo, tnobjetivo*);
static tnpuesto AcuNPuesto(tnpuesto, tnpuesto*);
static tnnrosem AcuNNrosem(tnnrosem, tnnrosem*);
static tnfecha AcuNFecha(tnfecha, tnfecha*);
static tnpuedel AcuNPuedel(tnpuedel, tnpuedel*);

static void LisNDeleg(tndeleg);
static void LisNCliente(tncliente);
static void LisNObjetivo(tnobjetivo);
static void LisNPuesto(tnpuesto);
static void LisNNrosem(tnnrosem);
static void LisNFecha(tnfecha);

static void BorNDeleg(tndeleg);
static void BorNCliente(tncliente);
static void BorNObjetivo(tnobjetivo);
static void BorNPuesto(tnpuesto);
static void BorNNrosem(tnnrosem);
static void BorNFecha(tnfecha);
static void BorNPuedel(tnpuedel);

tndeleg	inicio;
bool	esta_puesto;
int		NivCon, partial;
int		objetivo, puesto, nrosem, puedel;
long	svhn, svh50, svh100, svh100f, sthn, sth50, sth100, sth100f, sahn, sah50, sah100, sah100f, sehn, seh50, seh100, seh100f, stdhs, cliente;
char	deleg[6];
DATE	fecha;
/*---------------------------------Fin Declaraciones Listas Enlazadas--------------------------------------*/

/* Funciones privadas */
static bool Suma(long p_svhn, long p_svh50, long p_svh100, long p_svh100f,
                 long p_sthn, long p_sth50, long p_sth100, long p_sth100f,
                 long p_sahn, long p_sah50, long p_sah100, long p_sah100f,
                 long p_sehn, long p_seh50, long p_seh100, long p_seh100f, long p_stdhs);
static void ImprimirCantidades(long p_svhn, long p_svh50, long p_svh100, long p_svh100f,
                               long p_sthn, long p_sth50, long p_sth100, long p_sth100f,
                               long p_sahn, long p_sah50, long p_sah100, long p_sah100f,
                               long p_sehn, long p_seh50, long p_seh100, long p_seh100f, long p_stdhs);
void AbrirArchivo();
void AbrirReporte();
void ImprimirCabecera();
void SetearCabArch();
void ImprimirReporte();
void ImprimirArchivo();
void ImprimirDetalle(tnfecha nodop);
void ImprimirNoPrint(tnfecha nodop);
void ImprimirTitArch();
void ImprimirDetArch(tnfecha nodop);
void TotGen();
void TotCli();
void TotObj();
void TotFch();
void TotPto();
void TotDeleg();
void TitDeleg();
void TitCli();
void TitObj();
void TitPto();
void TotSem();
void TotDeleg();
void AcumularTotalizadores(tnfecha nodop);
void LimpiarAcumTot();
void LimpiarAcumCli();
void LimpiarAcumObj();
void LimpiarAcumFch();
void LimpiarAcumPto();
void LimpiarAcumDeleg();
void GenerarReporte();
void CargarCliObj(int, char[5], long, int, int, int, DATE, short, int, int, int, int, int);
//private int compcli(struct cliente *a, struct cliente *b);
//private int compfec(struct cliente *a, struct cliente *b);
bool ValidoClienteObjetivo(long p_cliente, short p_objet);
void LimpiarAcumSem();
static fm_status after(form fm, fmfield fno, int row);
bool HayImproductividad(tnfecha nodop);
bool ValidaDelegacion( char p_deleg[6]);
fm_status HelpDelegacion(form fm, fmfield fno, int row);
static int validatepto(void);
static void displaypto(char *buffer);

/* Declaraciones globales */
FILE   *fp;
form   fm0;
report rp0;
schema comerc, operac, bill, sue;
long   cliant, prgant;
int    objant, ptoant;
short  semant;
DATE   fecant;
char   dcliant[100], dobjant[100], dptoant[100], delega[7];
bool nodif;
char R_SEPAR[2];
char delegacion[6];
char delegant[6], Desc_Delega[31], Desc_Delega_ant[31];

long cli_desde, cli_hasta;
int  obj_desde, obj_hasta;
bool first;

/* Programa principal */
wcmd(hscliobj, 1.35 10/29/01)
{
	sue    = OpenSchema("sue",      IO_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	fm0    = OpenForm  ("hscliobj", FM_EABORT);

	FmOnKey(fm0, K_HELP, HelpDelegacion, DELEG, FDELEGA);
	FmSetIFld( fm0, I_PAIS, PaisUsuario());

	while (DoForm(fm0, NULLFP, after) != FM_UPDATE)
		return;

	inicio = NULL;
	deleg[0] = '\0';
	cliente = NULL_LONG;
	objetivo = NULL_SHORT;
	puesto = NULL_SHORT;
	puedel = NULL_SHORT;
	fecha = NULL_DATE;
	nrosem = NULL_SHORT;

    strcpy ( delegacion, NULL_STR);
	cli_desde=FmIsNull( fm0, CLID) ? MIN_LONG : FmLFld( fm0, CLID);
	cli_hasta=FmIsNull( fm0, CLIH) ? MAX_LONG : FmLFld( fm0, CLIH);
	obj_desde=FmIsNull( fm0, OBJD) ? MIN_SHORT: FmIFld( fm0, OBJD);
	obj_hasta=FmIsNull( fm0, OBJH) ? MAX_SHORT: FmIFld( fm0, OBJH);

	InicioListaTipoExcepcion();

	switch (*FmSFld(fm0, FmRefFld(fm0, R_DETALLE))) {
	case 'D':
		NivCon=0;
		break;
	case 'C':
		NivCon=1;
		break;
	case 'O':
		NivCon=2;
		break;
	case 'P':
		NivCon=3;
		break;
	case 'S':
		NivCon=4;
		break;
	case 'F':
		NivCon=5;
		break;
	}

	GenerarReporte();

	if (*FmSFld(fm0, SALIDA)=='A' || *FmSFld(fm0, SALIDA)=='R') {
		AbrirArchivo();
		// tipo de R_SEPAR
		strcpy(R_SEPAR, (*FmSFld(fm0, SALIDA)=='A'? R_SEPAR_A : R_SEPAR_R));
		LisNDeleg(inicio);
		BorNDeleg(inicio);
	}
	else {
		AbrirReporte();
		ImprimirCabecera();
		LisNDeleg(inicio);
		BorNDeleg(inicio);
		EndReport(rp0);
	}
}

void AbrirReporte()
{
	rp0 = OpenReport("hscliobj1", RP_EABORT|RP_NOBEGIN);

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

void GenerarReporte()
{
	dbcursor cpto, cparte, cexc, cretro, crexc;
	char buffer[100];
	DATE v_fecha;
	short tipoexc;

	cpto   = CreateCursor(PUESTOSbyCLIENTE, IO_NOT_LOCK);
	cexc   = CreateCursor(EXCEPCIONbyEMP,   IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMP,       IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyEMP,       IO_NOT_LOCK);
	crexc  = CreateCursor(RETROEXCbyEMP,    IO_NOT_LOCK);

	/*** Leo los puestos para informar aunque no tenga nada cargado *******************/
	SetCursorFrom(cpto, cli_desde, obj_desde, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cpto, cli_hasta, obj_hasta, MAX_SHORT, MAX_SHORT);
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

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) != BRIGADA) ||
		   ( *FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)) == BRIGADA))
			continue;

		if (IFld(PUESTOS_TIPPTO) == BRIGADASINARMAS || IFld(PUESTOS_TIPPTO) == BRIGADACONARMAS)
			continue;

		if ((IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > FmDFld(fm0, FECHAH)) ||
			(!IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FFINAL) < FmDFld(fm0, FECHAD))) {
			continue;
		}
		for (v_fecha = FmDFld(fm0, FECHAD); v_fecha <= FmDFld(fm0, FECHAH); v_fecha++) {
			if (IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > v_fecha)
				continue;
			if( !FmIsNull( fm0, DELEGD)) {
				strcpy( delegacion, GetDelegaObj(LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET)));
				if (!ValidaDelegacion( delegacion))
					continue;
			}
			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET), IFld(PUESTOS_TIPPTO),
						 IFld(PUESTOS_PUESTO), v_fecha, NroSemana(FmDFld(fm0, FECHAD), v_fecha),
						 NULL_SHORT, 0, 0, 0, 0);
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cpto);

	/*** Leo el parte *******************/

	SetCursorFrom(cparte, FmIFld(fm0, EMP), cli_desde, obj_desde, FmDFld(fm0, FECHAD),
						  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cparte, FmIFld(fm0, EMP), cli_hasta, obj_hasta, FmDFld(fm0, FECHAH),
						  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cparte) != ERROR) {
		sprintf (buffer, "Procesando Parte de Cliente %ld Objetivo %d", LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		if(!ValidoClienteObjetivo(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))){
			continue;
		}

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) != BRIGADA) ||
		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) == BRIGADA))
			continue;

		if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if( !FmIsNull( fm0, DELEGD))	{
			strcpy( delegacion, GetDelegaObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)));
			if (!ValidaDelegacion( delegacion))
				continue;
		}
		CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), IFld(PARTE_PTOSER),
					 IFld(PARTE_PUESTO), DFld(PARTE_DIA), NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)), 
					 NULL_SHORT,IFld(PARTE_HSNOR), IFld(PARTE_HS50),
					 IFld(PARTE_HS100F), IFld(PARTE_HS100FE));

	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);

	/*** Leo los retroactivos *******************/
	if (FmIFld(fm0, FRETRO)){
		SetCursorFrom(cretro, FmIFld(fm0, EMP), cli_desde, obj_desde, FmDFld(fm0, FECHAD),
							  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), cli_hasta, obj_hasta, FmDFld(fm0, FECHAH),
							  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cretro) != ERROR) {
			sprintf (buffer, "Procesando Retro Cliente %ld Objetivo %d", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			if(!ValidoClienteObjetivo(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO))){
				continue;
			}

			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) == BRIGADA))
				continue;

			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if( !FmIsNull( fm0, DELEGD))	{
				strcpy( delegacion, GetDelegaObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)));
				if (!ValidaDelegacion( delegacion))
					continue;
			}
			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), IFld(RETRO_PTOSER),
						 IFld(RETRO_PUESTO), DFld(RETRO_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)), NULL_SHORT, 
						 IFld(RETRO_DHSNOR), IFld(RETRO_DHS50),
						 IFld(RETRO_DHS100F), IFld(RETRO_DHS100FE));

		}
		SetCursorFrom(crexc, FmIFld(fm0, EMP), cli_desde, obj_desde, FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (crexc, FmIFld(fm0, EMP), cli_hasta, obj_hasta, FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(crexc) != ERROR) {
			sprintf (buffer, "Procesando RetroExc de Cliente %ld Objetivo %d", LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			if(!ValidoClienteObjetivo(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO))) {
				continue;
			}

			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) == BRIGADA))
				continue;

			if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) || DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
				continue;

			tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), IFld(RETROEXC_MOTIVO));
			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
					continue;
			}

			SetKey(comerc|OBJETIVO, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

			if( !FmIsNull( fm0, DELEGD)){
				strcpy( delegacion, GetDelegaObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)));
				if (!ValidaDelegacion( delegacion))
					continue;
			}
			CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO),
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

	SetCursorFrom(cexc, FmIFld(fm0, EMP), cli_desde, obj_desde, FmDFld(fm0, FECHAD),
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cexc, FmIFld(fm0, EMP), cli_hasta, obj_hasta, FmDFld(fm0, FECHAH),
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cexc) != ERROR) {
		sprintf (buffer, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		if(!ValidoClienteObjetivo(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO))) {
			continue;
		}

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) != BRIGADA) ||
		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) == BRIGADA))
			continue;

		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		SetKey(comerc|OBJETIVO, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		if( !FmIsNull( fm0, DELEGD)){
			strcpy( delegacion, GetDelegaObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)));
			if (!ValidaDelegacion( delegacion))
				continue;
		}
		CargarCliObj(FmIFld(fm0, EMP), delegacion, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
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
void CargarCliObj(int p_emp, char p_deleg[6], long p_cliente, int p_objetivo, int p_puesto, int p_ptoint,
                  DATE p_fecha, short p_nrosem, int p_condic, int p_hn, int p_h50, int p_h100, int p_h100f)
{

	sprintf (deleg, "%s", p_deleg);
	cliente=p_cliente;
	objetivo=p_objetivo;
	puesto=p_puesto;
	puedel=p_puesto;
	fecha=p_fecha;
	nrosem=p_nrosem;

	/* Si es por Delegacion */
	if (NivCon==0) {
		puesto=1;
		nrosem=1;
	}

	svhn=svh50=svh100=svh100f=0;
	sthn=sth50=sth100=sth100f=0;
	sahn=sah50=sah100=sah100f=0;
	sehn=seh50=seh100=seh100f=0;
	stdhs=0;


	if (p_condic != NULL_SHORT) {		// VIENE DE EXCEPCION
		if (p_condic == ACARGO_EMP) {
			sehn    = p_hn;
			seh50   = p_h50;
			seh100  = p_h100;
			seh100f = p_h100f;
		}
		else {
			sahn    += p_hn;
			sah50   += p_h50;
			sah100  += p_h100;
			sah100f += p_h100f;
		}
	}
	else {                      // VIENE DE PARTE
		sthn    += p_hn;
		sth50   += p_h50;
		sth100  += p_h100;
		sth100f += p_h100f;
	}     

	inicio = AcuNDeleg(inicio, &inicio);

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

	fprintf (fp, "Fecha Desde %.3T Hasta %.3D \t %s \t %s \t %B considera retroactivos \t ",
			FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), FmSFld(fm0, DOPCION), FmSFld(fm0, DTIPOBJ),
			FmIFld(fm0, FRETRO));

	if (!FmIsNull(fm0, FECSTD)) {
	    fprintf (fp, "Fecha Base %.3D ", FmDFld(fm0, FECSTD));
	}

	fprintf (fp, " \n");

	fprintf(fp, "%s\t%s\tCliente\tRazon Social\tObjetivo\tDescr Obj\tPuesto\tDescr Puesto\tFecha\tEstandard Vendido\t\t\t\tEstandar Trabajado\t\t\t\t\t\tDiferencia 1-2\t\t\t\tServicios Adicionales\t\t\t\tA cargo Empresa\t\t\t\tDiferencia Positiva\t\t\t\tDiferencia Negativa\t\t\t\tProgramador\n", FmIsNull( fm0, DELEGD)? NULL_STR:"Delegacion", FmIsNull( fm0, DELEGD)? NULL_STR:"Descripcion");
	fprintf(fp, "\t\t\t\t\t\t\t\t\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tH100F\tTotal\tNPres\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tH100F\n");
}



bool ValidoClienteObjetivo(long p_cliente, short p_objet)
{

	/* Solamente leo el cliente objetibo si no es el que tengo leido */
	if(p_cliente!=LFld(comerc|OBJETIVO_CLIENTE) || p_objet!=IFld(comerc|OBJETIVO_OBJET)) {
		SetKey(comerc|OBJETIVObyCLIENTE, p_cliente, p_objet);
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR){
 			//Warning("ERROR: Objetivo Inexistente %ld-%d ", cliente, objet);
			return FALSE;
		}
	}

	/* Valido Empresa */
	if (IFld(comerc|OBJETIVO_EMP)!=FmIFld(fm0, EMP))
		return FALSE;

	if (*FmSFld(fm0, TIPOBJ) == 'T') {
		return TRUE;
	}

	if (*FmSFld(fm0, TIPOBJ) == 'R' && ObjetRif(p_cliente, p_objet)){
		return TRUE;
	}

	if (*FmSFld(fm0, TIPOBJ) == 'E' && !ObjetRif(p_cliente, p_objet)){
		return TRUE;
	}

	return FALSE;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
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
		if ( FmIsNull( fm, fno, row) && FmIsNull( fm, DELEGD)){
			DisplayMsg(FALSE, "Si No Informa Delegacion. El Cliente es Obligatorio.");
			return FM_ERROR;
		}
	break;
	}
	return FM_OK;				
}	

bool HayImproductividad(tnfecha nodop)
{
	long totnor=0, tot50=0, tot100=0, tot100f=0;
	bool impro=FALSE;


		totnor  = ((*nodop).svhn - (*nodop).sthn);
		tot50   = ((*nodop).svh50 - (*nodop).sth50);
		tot100  = ((*nodop).svh100 - (*nodop).sth100);
		tot100f = ((*nodop).svh100f - (*nodop).sth100f);
                

	impro = (totnor != 0 || tot50 !=0 || tot100 != 0 || tot100f != 0);
	return impro;

}


bool ValidaDelegacion( char p_deleg[6])
{
	bool salida=TRUE;
	if ( strcmp( p_deleg, FmSFld( fm0, DELEGD)) < 0 ||
		 strcmp( p_deleg, FmSFld( fm0, DELEGH)) > 0)
		 	salida=FALSE;

	return salida;
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
static tndeleg AcuNDeleg(tndeleg nodop, tndeleg * nantp)
{
	tndeleg naux;

	if (nodop == NULL) {
		nodop = (tndeleg) malloc (sizeof(stndeleg));
	
		sprintf((*nodop).deleg,"%s", deleg);
		(*nodop).ncliente = AcuNCliente(NULL, NULL);

		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;

		(*nodop).nsig = NULL;
	}
	else {
		if (strcmp((*nodop).deleg, deleg)==0) {

			(*nodop).ncliente = AcuNCliente((*nodop).ncliente, &(*nodop).ncliente);

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;
		}
		else {
			if (strcmp((*nodop).deleg, deleg) < 0)
				(*nodop).nsig = AcuNDeleg((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tndeleg) malloc (sizeof(stndeleg));

				sprintf((*nodop).deleg,"%s", deleg);
				(*nodop).ncliente = AcuNCliente(NULL, NULL);

				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).ncliente = NULL;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tncliente AcuNCliente(tncliente nodop, tncliente * nantp)
{
	tncliente naux;

	if (nodop == NULL) {
		nodop = (tncliente) malloc (sizeof(stncliente));

		(*nodop).cliente = cliente;
		(*nodop).nobjetivo = AcuNObjetivo(NULL, NULL);

		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).cliente == cliente) {

			(*nodop).nobjetivo = AcuNObjetivo((*nodop).nobjetivo, &(*nodop).nobjetivo);

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;
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

				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;
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
		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).objetivo == objetivo) {

			(*nodop).npuesto = AcuNPuesto((*nodop).npuesto, &(*nodop).npuesto);

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;

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
				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;

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

		(*nodop).puesto = puesto;
		(*nodop).nnrosem = AcuNNrosem(NULL, NULL);

		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).puesto == puesto) {

			(*nodop).nnrosem = AcuNNrosem((*nodop).nnrosem, &(*nodop).nnrosem);

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;

		}
		else {
			if ((*nodop).puesto < puesto)
				(*nodop).nsig = AcuNPuesto((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnpuesto) malloc (sizeof(stnpuesto));
				(*nodop).puesto = puesto;
				(*nodop).nnrosem = AcuNNrosem(NULL, NULL);

				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;
				(*nodop).nsig = naux;

				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnrosem AcuNNrosem(tnnrosem nodop, tnnrosem * nantp)
{
	tnnrosem naux;

	if (nodop == NULL) {
		nodop = (tnnrosem) malloc (sizeof(stnnrosem));

		(*nodop).nrosem = nrosem;

		(*nodop).nfecha = AcuNFecha(NULL, NULL);

		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nrosem == nrosem) {

			(*nodop).nfecha = AcuNFecha((*nodop).nfecha, &(*nodop).nfecha);

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;
		}
		else {
			if ((*nodop).nrosem < nrosem)
				(*nodop).nsig = AcuNNrosem((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnrosem) malloc (sizeof(stnnrosem));

				(*nodop).nfecha = AcuNFecha(NULL, NULL);

				(*nodop).nrosem = nrosem;
				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}
	return nodop;
}

static tnfecha AcuNFecha(tnfecha nodop, tnfecha * nantp)
{
	tnfecha naux;

	if (nodop == NULL) {
		nodop = (tnfecha) malloc (sizeof(stnfecha));

		if (NivCon==0)
			(*nodop).npuedel = AcuNPuedel(NULL, NULL);
		else 
			(*nodop).npuedel = 0;
		GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, puedel, fecha,
					FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD),
					&svhn, &svh50, &svh100, &svh100f);

		stdhs   = svhn + svh50 + svh100 + svh100f;

		(*nodop).fecha = fecha;
		(*nodop).svhn = svhn;
		(*nodop).svh50 = svh50;
		(*nodop).svh100 = svh100;
		(*nodop).svh100f = svh100f;
		(*nodop).sthn = sthn;
		(*nodop).sth50 = sth50;
		(*nodop).sth100 = sth100;
		(*nodop).sth100f = sth100f;
		(*nodop).sahn = sahn;
		(*nodop).sah50 = sah50;
		(*nodop).sah100 = sah100;
		(*nodop).sah100f = sah100f;
		(*nodop).sehn = sehn;
		(*nodop).seh50 = seh50;
		(*nodop).seh100 = seh100;
		(*nodop).seh100f = seh100f;
		(*nodop).stdhs = stdhs;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).fecha == fecha) {

			esta_puesto=TRUE;

			if (NivCon==0)
				(*nodop).npuedel = AcuNPuedel((*nodop).npuedel, &(*nodop).npuedel);
			else 
				(*nodop).npuedel = 0;

			if (!esta_puesto) {
				GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, puedel, fecha,
						FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD),
						&svhn, &svh50, &svh100, &svh100f);
				stdhs   = svhn + svh50 + svh100 + svh100f;
			}

			(*nodop).svhn += svhn;
			(*nodop).svh50 += svh50;
			(*nodop).svh100 += svh100;
			(*nodop).svh100f += svh100f;
			(*nodop).sthn += sthn;
			(*nodop).sth50 += sth50;
			(*nodop).sth100 += sth100;
			(*nodop).sth100f += sth100f;
			(*nodop).sahn += sahn;
			(*nodop).sah50 += sah50;
			(*nodop).sah100 += sah100;
			(*nodop).sah100f += sah100f;
			(*nodop).sehn += sehn;
			(*nodop).seh50 += seh50;
			(*nodop).seh100 += seh100;
			(*nodop).seh100f += seh100f;
			(*nodop).stdhs += stdhs;
		}
		else {
			if ((*nodop).fecha < fecha)
				(*nodop).nsig = AcuNFecha((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnfecha) malloc (sizeof(stnfecha));

				if (NivCon==0)
					(*nodop).npuedel = AcuNPuedel(NULL, NULL);
				else 
					(*nodop).npuedel = 0;
					
				GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, puedel, fecha,
						FmIsNull(fm0, FECSTD) ? fecha : FmDFld(fm0, FECSTD),
						&svhn, &svh50, &svh100, &svh100f);

				stdhs   = svhn + svh50 + svh100 + svh100f;

				(*nodop).fecha = fecha;
				(*nodop).svhn = svhn;
				(*nodop).svh50 = svh50;
				(*nodop).svh100 = svh100;
				(*nodop).svh100f = svh100f;
				(*nodop).sthn = sthn;
				(*nodop).sth50 = sth50;
				(*nodop).sth100 = sth100;
				(*nodop).sth100f = sth100f;
				(*nodop).sahn = sahn;
				(*nodop).sah50 = sah50;
				(*nodop).sah100 = sah100;
				(*nodop).sah100f = sah100f;
				(*nodop).sehn = sehn;
				(*nodop).seh50 = seh50;
				(*nodop).seh100 = seh100;
				(*nodop).seh100f = seh100f;
				(*nodop).stdhs = stdhs;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}
static tnpuedel AcuNPuedel(tnpuedel nodop, tnpuedel * nantp)
{
	tnpuedel naux;
	
	if (nodop == NULL) {
		nodop = (tnpuedel) malloc (sizeof(stnpuedel));
		(*nodop).puedel = puedel;
		(*nodop).nsig = NULL;
		esta_puesto=FALSE;
	}
	else {
		if ((*nodop).puedel == puedel) {
				esta_puesto=TRUE;
		}
		else {
			if ((*nodop).puedel < puedel)
				(*nodop).nsig = AcuNPuedel((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnpuedel) malloc (sizeof(stnpuedel));
				(*nodop).puedel = puedel;
				esta_puesto=FALSE;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static void LisNDeleg(tndeleg nodop)
{
	bool suma=FALSE;
	char aux[100];
	if (nodop == NULL)
		return;

	sprintf(deleg, "%s", (*nodop).deleg);
	strcpy( Desc_Delega, strcmp( deleg, NULL_STR) == 0 ? NULL_STR : GetDescDeleg(deleg));

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			suma = Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			            (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			            (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    	        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        	    (*nodop).stdhs);

			if (suma){
				RpSetFld (rp0, RDELEG, deleg);
				RpSetFld (rp0, RDDELEG, Desc_Delega);
				DoReport (rp0, ZDEL);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 

	if ((*nodop).ncliente != NULL)
		LisNCliente((*nodop).ncliente);

	sprintf(aux, "%5s %s", deleg, Desc_Delega);

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			if(suma){
				RpSetFld(rp0, TTEXTO, aux);
				ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			                        (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			                        (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    	                    (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        	                (*nodop).stdhs);
				DoReport(rp0, ZTOTAL);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 

	if ((*nodop).nsig != NULL)
		LisNDeleg((*nodop).nsig);
}

static void LisNCliente(tncliente nodop)
{
	bool suma=FALSE;
	char aux[100];
	if (nodop == NULL)
		return;

	cliente = (*nodop).cliente;
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			suma=Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			          (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			          (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    	      (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        	  (*nodop).stdhs);

			if (suma){
				RpSetLFld(rp0, RCLI,  cliente);
				RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				DoReport (rp0, ZCLI);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 
	

	if (NivCon > 1 || NivCon==0)
		if ((*nodop).nobjetivo != NULL)
			LisNObjetivo((*nodop).nobjetivo);

	sprintf(aux, "%9ld %s", cliente, SFld(bill|CLIENTE_RAZSOC));
	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			RpSetFld(rp0, TTEXTO, aux);
			if(suma){
				ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
		                        (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
		                        (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		                        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		                        (*nodop).stdhs);

				DoReport(rp0, ZTOTAL);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 

	if ((*nodop).nsig != NULL)
		LisNCliente((*nodop).nsig);
}

static void LisNObjetivo(tnobjetivo nodop)
{
	bool suma=FALSE;
	char aux[100];
	if (nodop == NULL)
		return;

	objetivo = (*nodop).objetivo;
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			suma=Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			          (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			          (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    	      (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        	  (*nodop).stdhs);

		    if (suma){
			    RpSetIFld(rp0, ROBJ,  objetivo);
				RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
				DoReport (rp0, ZOBJ);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 

	if (NivCon > 2 || NivCon==0)
		if ((*nodop).npuesto != NULL)
			LisNPuesto((*nodop).npuesto);

	sprintf(aux, "%4d %s", objetivo, SFld(comerc|OBJETIVO_DESCRIP));

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			if(suma){
				RpSetFld(rp0, TTEXTO, aux);
				ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
		        	                (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
		            	            (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		                	        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		                    	    (*nodop).stdhs);
				DoReport(rp0, ZTOTAL);
			}
			break;
		case 'A': 
			break;
		case 'R': 
			break;
	} 
	if ((*nodop).nsig != NULL)
		LisNObjetivo((*nodop).nsig);

}
static void LisNPuesto(tnpuesto nodop)
{
	bool suma=FALSE;
	char aux[100];
	if (nodop == NULL)
		return;

	puesto = (*nodop).puesto;

	if (NivCon!=0) {
		SetKey(comerc|TPTOSERbyTIPPTO, puesto);
		if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) == ERROR)
			SetFld(comerc|TPTOSER_DESCRIP, "ERROR: Puesto Inexistente");

		switch(*FmSFld(fm0, SALIDA)) {
			case 'I': 
			case 'T': 
				suma=Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
				          (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
				          (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    		      (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        		  (*nodop).stdhs);

			    if(suma){
					RpSetIFld(rp0, RPTO,  puesto);
					RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
					DoReport (rp0, ZPTO);
				}
				break;
			case 'A': 
				break;
			case 'R': 
				break;
		} 
	}

	if (NivCon > 3 || NivCon==0)
		if ((*nodop).nnrosem != NULL)
			LisNNrosem((*nodop).nnrosem);

	if (NivCon!=0) {
		sprintf(aux, "%4d %s", puesto, SFld(comerc|TPTOSER_DESCRIP));
		switch(*FmSFld(fm0, SALIDA)) {
			case 'I': 
			case 'T': 
				if(suma){
					RpSetFld(rp0, TTEXTO, aux);
					ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			        	                (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			            	            (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
			                	        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
			                    	    (*nodop).stdhs);
					DoReport(rp0, ZTOTAL);
				}
				break;
			case 'A': 
				break;
			case 'R': 
				break;
		} 
	}
	if ((*nodop).nsig != NULL)
		LisNPuesto((*nodop).nsig);
}
static void LisNNrosem(tnnrosem nodop)
{
	char aux[100];
	DATE fdesde, fhasta;
	char sdesde[15], shasta[15];
	bool suma = FALSE;
	if (nodop == NULL)
		return;

	nrosem = (*nodop).nrosem;

	if (NivCon > 4 || NivCon==0)
		if ((*nodop).nfecha != NULL)
			LisNFecha((*nodop).nfecha);

	if (NivCon == 4) {
		SemanaDesdeHasta(fecha, &fdesde, &fhasta);

		DToStr(fdesde, sdesde, DFMT_SEPAR);
		DToStr(fhasta, shasta, DFMT_SEPAR);

		sprintf(aux, " %s AL %s ", sdesde, shasta);

		switch(*FmSFld(fm0, SALIDA)) {
			case 'I': 
			case 'T': 
				suma=Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
				          (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
				          (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		    		      (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		        		  (*nodop).stdhs);
				if(suma){
					RpSetFld(rp0, TTEXTO, aux);
					ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			        	                (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			            	            (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
			                	        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
			                    	    (*nodop).stdhs);
					DoReport(rp0, ZTOTAL);
				}
				break;
			case 'A': 
				break;
			case 'R': 
				break;
		} 
	}

	if ((*nodop).nsig != NULL)
		LisNNrosem((*nodop).nsig);
}

static void LisNFecha(tnfecha nodop)
{
	bool suma = FALSE;
	if (nodop == NULL)
		return;

	/* Si El nivel no es por Delegacion usa toda los nodos, sino solo el de fecha */
	nodif = FALSE;
	if (NivCon == 0 && HayImproductividad(nodop)) {
		nodif = TRUE;
	} 
	fecha = (*nodop).fecha;

	switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
			suma=Suma((*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
			          (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
			          (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
	    		      (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
	        		  (*nodop).stdhs);
			if(suma){
				RpSetDFld(rp0, RFCH, fecha);
				ImprimirCantidades( (*nodop).svhn,  (*nodop).svh50,  (*nodop).svh100,  (*nodop).svh100f,
		                        (*nodop).sthn,  (*nodop).sth50,  (*nodop).sth100,  (*nodop).sth100f,
		                        (*nodop).sahn,  (*nodop).sah50,  (*nodop).sah100,  (*nodop).sah100f,
		                        (*nodop).sehn,  (*nodop).seh50,  (*nodop).seh100,  (*nodop).seh100f,
		                        (*nodop).stdhs);
				DoReport(rp0, ZFCH);
			}
			break;
		case 'A': 
		case 'R': 
			ImprimirDetArch(nodop);
			break;
	} 


	if ((*nodop).nsig != NULL)
		LisNFecha((*nodop).nsig);
}

static void BorNDeleg(tndeleg nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).ncliente != NULL)
		BorNCliente((*nodop).ncliente);

	if ((*nodop).nsig != NULL)
		BorNDeleg((*nodop).nsig);

	(*nodop).ncliente = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
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

	if ((*nodop).nnrosem != NULL)
		BorNNrosem((*nodop).nnrosem);

	if ((*nodop).nsig != NULL)
		BorNPuesto((*nodop).nsig);

	(*nodop).nnrosem = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNrosem(tnnrosem nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nfecha != NULL)
		BorNFecha((*nodop).nfecha);

	if ((*nodop).nsig != NULL)
		BorNNrosem((*nodop).nsig);

	(*nodop).nfecha = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNFecha(tnfecha nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).npuedel != NULL)
		BorNPuedel((*nodop).npuedel);

	if ((*nodop).nsig != NULL)
		BorNFecha((*nodop).nsig);

	(*nodop).npuedel = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}
static void BorNPuedel(tnpuedel nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNPuedel((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}               

static bool Suma(long p_svhn, long p_svh50, long p_svh100, long p_svh100f,
                 long p_sthn, long p_sth50, long p_sth100, long p_sth100f,
                 long p_sahn, long p_sah50, long p_sah100, long p_sah100f,
                 long p_sehn, long p_seh50, long p_seh100, long p_seh100f, long p_stdhs)
{

	if(p_svhn==0 && p_svh50==0 && p_svh100==0 && p_svh100f==0 &&
	   p_sthn==0 && p_sth50==0 && p_sth100==0 && p_sth100f==0 &&
	   p_sahn==0 && p_sah50==0 && p_sah100==0 && p_sah100f==0 &&
	   p_sehn==0 && p_seh50==0 && p_seh100==0 && p_seh100f==0 &&
	   p_stdhs==0) {
		return FALSE;
	}
	else
		return TRUE;
}
static void ImprimirCantidades(long p_svhn, long p_svh50, long p_svh100, long p_svh100f,
                               long p_sthn, long p_sth50, long p_sth100, long p_sth100f,
                               long p_sahn, long p_sah50, long p_sah100, long p_sah100f,
                               long p_sehn, long p_seh50, long p_seh100, long p_seh100f, long p_stdhs)
{
	// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
	// tener que hacerlo en cada funcion que imprime detalles (arch, reporte, y no print)
	p_sthn    = p_sthn    - p_sahn    - p_sehn;
	p_sth50   = p_sth50   - p_sah50   - p_seh50;
	p_sth100  = p_sth100  - p_sah100  - p_seh100;
	p_sth100f = p_sth100f - p_sah100f - p_seh100f;
//        WALTER

	// STANDARD
	RpSetLFld(rp0, FSVHN,   p_svhn);
	RpSetLFld(rp0, FSVH50,  p_svh50);
	RpSetLFld(rp0, FSVH100, p_svh100 + p_svh100f);
//	RpSetLFld(rp0, FSVTOT,  p_svhn + p_svh50 + p_svh100 + p_svh100f);
	RpSetLFld(rp0, FSVTOT,  p_stdhs);

	// TRABAJADO
	RpSetLFld(rp0, FSTHN,    p_sthn);
	RpSetLFld(rp0, FSTH50,   p_sth50);
	RpSetLFld(rp0, FSTH100,  p_sth100);
	RpSetLFld(rp0, FSTH100F, p_sth100f);
	RpSetLFld(rp0, FSTTOT,   p_sthn + p_sth50 + p_sth100 + p_sth100f);
	RpSetLFld(rp0, FSTNPRES, p_svhn + p_svh50 + p_svh100 + p_svh100f -
                            (p_sthn + p_sth50 + p_sth100 + p_sth100f));

	// DIFERENCIA
	RpSetLFld(rp0, FDIFHN,    p_svhn    - p_sthn);
	RpSetLFld(rp0, FDIFH50,   p_svh50   - p_sth50);
	RpSetLFld(rp0, FDIFH100,  p_svh100  - p_sth100);
	RpSetLFld(rp0, FDIFH100F, p_svh100f - p_sth100f);

	// ADICIONALES
	RpSetLFld(rp0, FSAHN,   p_sahn);
	RpSetLFld(rp0, FSAH50,  p_sah50);
	RpSetLFld(rp0, FSAH100, p_sah100 + p_sah100f);
	RpSetLFld(rp0, FSATOT,  p_sahn + p_sah50 + p_sah100 + p_sah100f);

	// A CARGO EMPRESA 
	RpSetLFld(rp0, FSEHN,   p_sehn);
	RpSetLFld(rp0, FSEH50,  p_seh50);
	RpSetLFld(rp0, FSEH100, p_seh100 + p_seh100f);
	RpSetLFld(rp0, FSETOT,  p_sehn + p_seh50 + p_seh100 + p_seh100f);

}

// cuando la  opcion de seleccion es detallando las fechas y a archivo
// de texto se utiliza esta funcion.
void  ImprimirDetArch(tnfecha nodop)
{
	// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
	// tener que hacerlo en cada funcion que imprime detalles 
	(*nodop).sthn    = (*nodop).sthn    - (*nodop).sahn    - (*nodop).sehn;
	(*nodop).sth50   = (*nodop).sth50   - (*nodop).sah50   - (*nodop).seh50;
	(*nodop).sth100  = (*nodop).sth100  - (*nodop).sah100  - (*nodop).seh100;
	(*nodop).sth100f = (*nodop).sth100f - (*nodop).sah100f - (*nodop).seh100f;
//        WALTER

	if (*FmSFld(fm0, SALIDA) == 'R') {
		fprintf(fp, "%d%s%s%s%d%s", IFld(sue|EMPS_PAIS),              R_SEPAR,
									GetDescPais(IFld(sue|EMPS_PAIS)), R_SEPAR,
									FmIFld(fm0, EMP),                 R_SEPAR);
	}

	fprintf(fp, "%s%s%s%s%ld%s%s%s%d%s%s%s%d%s%s%s%.1D%s", 
				deleg,                    R_SEPAR, 
				Desc_Delega,                    R_SEPAR, 
				cliente, 						R_SEPAR, 
				SFld(bill|CLIENTE_RAZSOC),      R_SEPAR, 
				objetivo,                      R_SEPAR, 
				SFld(comerc|OBJETIVO_DESCRIP),  R_SEPAR, 
				puesto,                   R_SEPAR, 
				SFld(comerc|TPTOSER_DESCRIP),   R_SEPAR, 
				fecha,					R_SEPAR );

	fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f\
%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%9.2f%s%s\n",
			 	(double)(*nodop).svhn / 100.0, 	R_SEPAR,
			 	(double)(*nodop).svh50 / 100.0,    R_SEPAR,
			 	(double)((*nodop).svh100 + (*nodop).svh100f) / 100.0,	R_SEPAR,
			 	(double)((*nodop).svhn + (*nodop).svh50 + (*nodop).svh100 + (*nodop).svh100f) / 100.0,	R_SEPAR,
				(double)(*nodop).sthn / 100.0,		R_SEPAR,
				(double)(*nodop).sth50 / 100.0,	R_SEPAR,
				(double)(*nodop).sth100 / 100.0,   R_SEPAR,
				(double)(*nodop).sth100f / 100.0,  R_SEPAR,
			    (double)((*nodop).sthn + (*nodop).sth50 + (*nodop).sth100 + (*nodop).sth100f) / 100.0, R_SEPAR,
			 	(double)(((*nodop).svhn + (*nodop).svh50 + (*nodop).svh100 + (*nodop).svh100f) -
			 	         ((*nodop).sthn + (*nodop).sth50 + (*nodop).sth100 + (*nodop).sth100f)) / 100.0, R_SEPAR,
				(double)((*nodop).svhn    - (*nodop).sthn) / 100.0, R_SEPAR,
				(double)((*nodop).svh50   - (*nodop).sth50) / 100.0, R_SEPAR,
				(double)((*nodop).svh100  - (*nodop).sth100) / 100.0, R_SEPAR,
				(double)((*nodop).svh100f - (*nodop).sth100f) / 100.0, R_SEPAR,
			 	(double)(*nodop).sahn / 100.0, R_SEPAR,
			 	(double)(*nodop).sah50 / 100.0, R_SEPAR,
			 	(double)((*nodop).sah100 + (*nodop).sah100f) / 100.0, R_SEPAR,
			 	(double)((*nodop).sahn + (*nodop).sah50 + (*nodop).sah100 + (*nodop).sah100f) / 100.0, R_SEPAR,
			 	(double)(*nodop).sehn / 100.0, R_SEPAR,
			 	(double)(*nodop).seh50 / 100.0, R_SEPAR,
			 	(double)((*nodop).seh100 + (*nodop).seh100f) / 100.0, R_SEPAR,
			 	(double)((*nodop).sehn + (*nodop).seh50 + (*nodop).seh100 + (*nodop).seh100f) / 100.0, R_SEPAR,
				//Diferencias Positivas
			 	(double)((*nodop).svhn - (*nodop).sthn) < 0 ? 0 : ((*nodop).svhn - (*nodop).sthn)    / 100.0, R_SEPAR,
			 	(double)((*nodop).svh50 - (*nodop).sth50) < 0 ? 0 : ((*nodop).svh50 - (*nodop).sth50)   / 100.0, R_SEPAR,
			 	(double)((*nodop).svh100 - (*nodop).sth100) < 0 ? 0 : ((*nodop).svh100 - (*nodop).sth100)  / 100.0, R_SEPAR,
			 	(double)((*nodop).svh100f - (*nodop).sth100f) < 0 ? 0 : ((*nodop).svh100f - (*nodop).sth100f) / 100.0, R_SEPAR,
				//Diferencias Negativas
			 	(double)((*nodop).svhn - (*nodop).sthn) > 0 ? 0 : ((*nodop).svhn - (*nodop).sthn)    / 100.0, R_SEPAR,
			 	(double)((*nodop).svh50 - (*nodop).sth50) > 0 ? 0 : ((*nodop).svh50 - (*nodop).sth50)   / 100.0, R_SEPAR,
			 	(double)((*nodop).svh100 - (*nodop).sth100) > 0 ? 0 : ((*nodop).svh100 - (*nodop).sth100)  / 100.0, R_SEPAR,
			 	(double)((*nodop).svh100f - (*nodop).sth100f) > 0 ? 0 : ((*nodop).svh100f - (*nodop).sth100f) / 100.0, R_SEPAR,
			 	IsNull(comerc|OBJETIVO_PROGRAM) ? NULL_STR : GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));

}

