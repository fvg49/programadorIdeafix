/********************************************************************
*
* MODULE & VERSION : @(#)hsclileg.c	1.22
* DATE             : 08/06/27
* TIME             : 13:23:43
*
* CREATED          : 18/02/99 Gloria
*
* DESCRIPTION:
*	Impresion de control de horas por cliente-objetivo-puesto-legajo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
Modificacion 12/01/2001
	Ahora se calcula por separado (para 100 franco y 100 fer) el estandard vendido,el estandard trabajado 	
	y los adicionales. Pero se muestran en una solo columna. Se calculan por separado para hacer las diferencias.

Modificacion 05/09/2001
Si esta definido  _NOVIA_VER_2_0 (en amiente.h): 
	La fecha desde debe ser Lunes.
	La fecha hasta debe ser Domingo.

*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "hsclileg.fmh"
#include "hsclileg.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "filial.h"

//Opciones del listado
#define DIFERENC	1
#define SERV_ADIC	2
#define HS_ACARGO	3
#define HS_NOPRES	4
#define TODOS		5
#define OpcionExcepcion(opcion) (opcion == SERV_ADIC || opcion == HS_ACARGO || opcion == TODOS)
#define MAXLEG		1000
#define MAXCLI		50000
#define	ERR_ARCHI	"No se pudo abrir el archivo."

/* Estructuras */
struct cliente {
	long cli;
	int	 obj;
    int  puesto;
    int  nroleg;
	int	 svhn;
	int  svh50;
	int  svh100;
	int  svh100f;
	int  sthn;
	int  sth50;
	int  sth100;
	int  sth100f;
	int  sahn;
	int  sah50;
	int  sah100;
	int  sah100f;
	int  sehn;
	int  seh50;
	int  seh100;
	int  seh100f;
} pcli[MAXCLI], *ucli = pcli, *ecli;

struct nroleg {
	long 	cli;
	int		obj;
	long	nroleg;
	int		puesto;
} pleg[MAXLEG], *uleg = pleg, *eleg;

/* anteriores */
long 	cliant;
int		objant, ptoant;
char 	dcliant[67], dobjant[55], dptoant[35];

/* Totalizadores para impresion en Archivo ASCII */
/* Tot Generales */
int 	tsvhn, tsvh50, tsvh100, tsvtot,
	    tsthn, tsth50, tsth100, tsth100f, tsttot, tstnpres,
		tdhn, tdh50, tdh100, tdh100f,
	 	tsahn, tsah50, tsah100, tsatot,
	 	tsehn, tseh50, tseh100, tsetot;
/* Tot de Cliente */
int 	csvhn, csvh50, csvh100, csvtot,
		csthn, csth50, csth100, csth100f, csttot, cstnpres,
		cdhn, cdh50, cdh100, cdh100f,
	 	csahn, csah50, csah100, csatot,
	 	csehn, cseh50, cseh100, csetot;
/* Tot de Objetivo */
int 	osvhn, osvh50, osvh100, osvtot,
		osthn, osth50, osth100, osth100f, osttot, ostnpres,
		odhn, odh50, odh100, odh100f,
	 	osahn, osah50, osah100, osatot,
	 	osehn, oseh50, oseh100, osetot;
/* Tot de Puesto */
int 	psvhn, psvh50, psvh100, psvtot,
		psthn, psth50, psth100, psth100f, psttot, pstnpres,
		pdhn,  pdh50, pdh100, pdh100f,
	 	psahn, psah50, psah100, psatot,
	 	psehn, pseh50, pseh100, psetot;

/* Funciones privadas */
void AbrirArchivo();
void AbrirReporte();
void ImprimirCabecera();
void SetearCabArch();
void ImprimirReporte();
void ImprimirArchivo();
void ImprimirDetalle();
void ImprimirNoPrint();
void ImprimirTitArch();
void ImprimirDetArch();
void TotGen();
void TotCli();
void TotObj();
void TotPto();
void TitCli();
void TitObj();
void TitPto();
void AcumularTotalizadores();
void LimpiarAcumTot();
void LimpiarAcumCli();
void LimpiarAcumObj();
void LimpiarAcumPto();
void GenerarReporte();
void CargarCliObj(int emp, long cliente, int objetivo, long nroleg, 
				  int puesto, int ptoint, DATE fecha, int condic, 
				  int hn, int h50, int h100, int h100f, bool calcstd);
void CalcEstVend(int emp, long cliente, int objetivo, long nroleg, 
				int puesto, int ptoint, DATE fecha, int *svhn, 
				int *svh50, int *svh100, int *svh100f);
private int compcli(struct cliente *a, struct cliente *b);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
FILE	*fp;
form fm0;
report rp0;
schema comerc, operac, bill;

/* Programa principal */
wcmd(hsclileg, 1.22 06/27/08)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	bill = OpenSchema("bill", IO_EABORT);

	fm0 = OpenForm  ("hsclileg", FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while (DoForm(fm0, before, after) != FM_UPDATE) return;

	InicioListaTipoExcepcion();
	GenerarReporte();	
	if (*FmSFld(fm0, SALIDA) == 'A') {
		AbrirArchivo();
		ImprimirReporte();
	}
	else {
		AbrirReporte();
		ImprimirCabecera();
		ImprimirReporte();
		EndReport(rp0);
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

void AbrirReporte()
{
    rp0 = OpenReport("hsclileg", RP_EABORT|RP_NOBEGIN);
   
    //Si la salida es Impresora
    if ( *FmSFld(fm0, SALIDA) == 'I') {
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );
    }
	//Si la salida es Terminal
    if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR );
    BeginReport(rp0, 1, NULL_STR);    
} 

void ImprimirCabecera()
{
	RpSetLFld(rp0, RCLID, FmLFld(fm0, CLID));
	RpSetLFld(rp0, RCLIH, FmLFld(fm0, CLIH));
	RpSetIFld(rp0, ROBJD, FmIFld(fm0, OBJD));
	RpSetIFld(rp0, ROBJH, FmIFld(fm0, OBJH));	
	RpSetFld (rp0, RDCLIOBJD, FmSFld(fm0, DOBJD));
	RpSetFld (rp0, RDCLIOBJH, FmSFld(fm0, DOBJH));
	RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));
	RpSetIFld (rp0, RRETRO,   FmIFld(fm0, FRETRO));
	
}

void GenerarReporte()
{
	dbcursor cparte, cexc, cretro, cretexc;
	int puesto;

	cexc   = CreateCursor(EXCEPCIONbyEMP, IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMP, IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyEMP, IO_NOT_LOCK);
	cretexc = CreateCursor(RETROEXCbyEMP, IO_NOT_LOCK);

	if (!FmIsNull(fm0, CLID)) {
		SetCursorFrom(cparte, FmIFld(fm0, EMP), FmLFld(fm0, CLID), 
							  FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD), 
							  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), 
							  FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH), 
							  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		SetCursorFrom(cparte, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, 
							  MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, 
							  MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while(FetchCursor(cparte) != ERROR) {
		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) != BRIGADA) ||
		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) == BRIGADA))
			continue;

		if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) ||
			DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;

		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		CargarCliObj(FmIFld(fm0, EMP),
					 LFld(PARTE_CLIENTE), 
					 IFld(PARTE_OBJETIVO), 
					 LFld(PARTE_NROLEG),
					 IFld(PARTE_PTOSER),
					 IFld(PARTE_PUESTO), 
					 DFld(PARTE_DIA), 
					 NULL_SHORT, 
					 IFld(PARTE_HSNOR),
					 IFld(PARTE_HS50),
					 IFld(PARTE_HS100F),
					 IFld(PARTE_HS100FE),
					 !str_eq (SFld(PARTE_CONDIC), "F")); //Va al standad de hs. si no es franco
	}
	DeleteCursor(cparte);

	// SOLO TENGO QUE HACER ESTO SI LA OPCION ES:
	//		2 - SERVICIOS ADICIONALES MAYOR QUE 0
	//      3 - HORAS A CARGO DE LA EMPRESA MAYOR A 0
	//   o  5 - TODOS LOS DATOS
	if (OpcionExcepcion(FmIFld(fm0, OPCION))) { 
		if (!FmIsNull(fm0, CLID)) {
			SetCursorFrom(cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), 
								FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD), 
								MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), 
								FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH), 
								MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		}
		else {
			SetCursorFrom(cexc, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, 
						  MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cexc, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, 
						  MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		}

		while (FetchCursor(cexc) != ERROR) {
			short condic, tipoexc;
		
			if ((*FmSFld(fm0, OBJBRI) == 'B' &&
				GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) != BRIGADA) ||
			   (*FmSFld(fm0, OBJBRI) == 'V' &&
			    GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) == BRIGADA))
				continue;

			if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) ||
				DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
				continue;

			tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC), IFld(EXCEPCION_MOTIVO));
			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
					continue;
			}

			SetKey(comerc|OBJETIVObyCLIENTE, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
			(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
			if (!ValidaListaXusr(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)))
				continue;
			if (!ValidaFilial(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
			
			CargarCliObj(FmIFld(fm0, EMP),
						 LFld(EXCEPCION_CLIENTE), 
						 IFld(EXCEPCION_OBJETIVO), 
						 LFld(EXCEPCION_NROLEG),
						 IFld(EXCEPCION_PTOSER),
						 IFld(EXCEPCION_PUESTO), 
						 DFld(EXCEPCION_DIA), 
						 IFld(EXCEPCION_CONDIC),
						 IFld(EXCEPCION_HORAS),
						 IFld(EXCEPCION_HS50),   
						 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
						 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0,
						 FALSE); //No va al standard de horas
		}       	
		DeleteCursor(cexc);
	}

	if (FmIFld(fm0, FRETRO)){ 
		if (!FmIsNull(fm0, CLID)) {
			SetCursorFrom(cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLID), 
								  FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD), 
								  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), 
								  FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH), 
								  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		}
		else {
			SetCursorFrom(cretro, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, 
								  MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, 
								  MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		}
		while(FetchCursor(cretro) != ERROR) {
			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) == BRIGADA))
				continue;

			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) ||
				DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
			if (!ValidaListaXusr(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)))
				continue;
			if (!ValidaFilial(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
			
			CargarCliObj(FmIFld(fm0, EMP),
						 LFld(RETRO_CLIENTE), 
						 IFld(RETRO_OBJETIVO), 
						 LFld(RETRO_NROLEG),
						 IFld(RETRO_PTOSER),
						 IFld(RETRO_PUESTO), 
						 DFld(RETRO_DIA), 
						 NULL_SHORT, 
						 IFld(RETRO_DHSNOR),
						 IFld(RETRO_DHS50),
						 IFld(RETRO_DHS100F),
						 IFld(RETRO_DHS100FE),
						 FALSE); //Sino acumula en el retroactivo algo que ya tiene
//						 !str_eq (SFld(RETRO_CONDIC), "F")); //Va al standad de hs. si no es franco
		}
		DeleteCursor(cretro);

		if (OpcionExcepcion(FmIFld(fm0, OPCION))) { 
			if (!FmIsNull(fm0, CLID)) {
				SetCursorFrom(cretexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), 
									FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD), 
									MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
				SetCursorTo  (cretexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), 
									FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH), 
									MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			}
			else {
				SetCursorFrom(cretexc, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, 
							  MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
				SetCursorTo  (cretexc, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, 
							  MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			}

			while (FetchCursor(cretexc) != ERROR) {
				short condic, tipoexc;

				if ((*FmSFld(fm0, OBJBRI) == 'B' &&
					GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) != BRIGADA) ||
				   (*FmSFld(fm0, OBJBRI) == 'V' &&
					GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) == BRIGADA))
					continue;

				if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) ||
					DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
					continue;

				tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), IFld(RETROEXC_MOTIVO));
				/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
				if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
						continue;
				}

				SetKey(comerc|OBJETIVObyCLIENTE, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
				(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

				//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
				if (!ValidaListaXusr(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)))
					continue;
				if (!ValidaFilial(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
					continue;
				
				CargarCliObj(FmIFld(fm0, EMP),
							 LFld(RETROEXC_CLIENTE), 
							 IFld(RETROEXC_OBJETIVO), 
							 LFld(RETROEXC_NROLEG),
							 IFld(RETROEXC_PTOSER),
							 IFld(RETROEXC_PUESTO), 
							 DFld(RETROEXC_DIA), 
							 IFld(RETROEXC_CONDIC),
							 IFld(RETROEXC_DHORAS),
							 IFld(RETROEXC_DHS50),   
							 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
							 FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0,
							 FALSE); //No va al standard de horas
			}       	
			DeleteCursor(cretexc);
		} //Fin retroexc
	} //Fin retro
}

int	   CargarPuesto(int emp, long cliente, int objetivo, long nroleg)
{
	int puesto;
	
	// Si esta el nroleg devuelvo el puesto que ya encontramos sino lo 
	// buscamos
	for (eleg =pleg ; eleg < uleg ; eleg++)
		if (eleg->cli == cliente && eleg->obj == objetivo &&
									eleg->nroleg == nroleg)
			return eleg->puesto;
    puesto = GetTipPto(emp, cliente, objetivo, nroleg);

	eleg->cli = cliente;
	eleg->obj = objetivo;
	eleg->nroleg = nroleg;
	eleg->puesto = puesto;
    
    return puesto;
}

// Carga el vector de PCLI.
//
// Si viene de PARTE h100 contiene horas extras de francos y 
// h100f de feriados.
// si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene
// vacio.

void	CargarCliObj(int emp, long cliente, int objetivo, long nroleg, 
					 int puesto, int ptoint, DATE fecha, int condic, 
					 int hn, int h50, int h100, int h100f, bool calcstd)
{
	int svhn, svh50, svh100, svh100f;

	svhn = svh50 = svh100 = svh100f = 0;
	
	for (ecli = pcli ; ecli < ucli ; ecli++) {
		if (ecli->cli == cliente && ecli->obj == objetivo &&
		    ecli->puesto == puesto && ecli->nroleg == nroleg) {
		    break;
		}
	}		
	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI]) Error("Tabla interna saturada. Max %d", MAXCLI);
		ucli->cli	= cliente;
		ucli->obj	= objetivo;
		ucli->puesto= puesto;
		ucli->nroleg= nroleg;
		ucli->sehn	= 0;
		ucli->seh50 = 0;
		ucli->seh100= 0;
		ucli->seh100f= 0;
		ucli->sahn	= 0;
		ucli->sah50	= 0;
		ucli->sah100= 0;
		ucli->sah100f= 0;
		ucli->sthn	= 0;
		ucli->sth50	= 0;
		ucli->sth100= 0;
		ucli->sth100f= 0;
        if (condic != NULL_SHORT && calcstd)
			CalcEstVend(emp, cliente, objetivo, nroleg, 
						puesto, ptoint, fecha, &svhn, &svh50, &svh100, &svh100f);
		ucli++;
	} 

	if (condic != NULL_SHORT) {		// VIENE DE EXCEPCION
		if (condic == ACARGO_EMP) {
			ecli->sehn += hn;
			ecli->seh50 += h50;
			ecli->seh100 += h100;
			ecli->seh100f += h100f;
		} else {
			ecli->sahn += hn;
			ecli->sah50 += h50;
			ecli->sah100 += h100;
			ecli->sah100f += h100f;
		}
	} else {                      // VIENE DE PARTE
		ecli->sthn += hn;
		ecli->sth50 += h50;
		ecli->sth100 += h100;
		ecli->sth100f += h100f;

		if (calcstd){
			CalcEstVend(emp, cliente, objetivo, nroleg, 
						puesto, ptoint, fecha, &svhn, &svh50, &svh100, &svh100f);
		}						
	}   
	ecli->svhn   += svhn;
	ecli->svh50  += svh50;
	ecli->svh100 += svh100;
	ecli->svh100f += svh100f;
}					 
                                 
// Calcula los estandares vendidos de un dia para un puesto de un empleado
// en un cliente - objetivo.                                 
void	CalcEstVend(int emp, long cliente, int objetivo, long nroleg, 
				   	int puesto, int ptoint, DATE fecha, int *svhn, int *svh50, 
				   	int *svh100, int *svh100f)
{
	long hortra=0,  hsnormal=0, hsextra=0, hsex100=0, hsex100f=0;
	bool encontro=FALSE;
	char regimen[10];

	
	*svhn =	0;
	*svh50=	0;
	*svh100 = 0;
	*svh100f = 0;
	
	/*Leo la asignacion para saber cuanto tiene que trabajar */
	/*Primero leo en ASIG */
	SetKey (operac|ASIGbyEMP, emp, cliente, objetivo, nroleg, puesto, ptoint, MIN_SHORT);
	while (GetRecord (operac|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR) {
		encontro = TRUE;
		strcpy(regimen, SFld (operac|ASIG_REGIM));
		hortra = ConvHraInt(TFld(operac|ASIG_HSENT), TFld(operac|ASIG_HSSAL)) * 100;
	}
	/*Si no esta en ASIG me fijo en ASIGH */
	if (!encontro) {
		SetKey (operac|ASIGHbyPUESTO, emp, cliente, objetivo, nroleg, puesto, ptoint, MIN_SHORT);
		while (GetRecord (operac|ASIGHbyPUESTO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR) {
			strcpy(regimen, SFld (operac|ASIGH_REGIM));
			hortra = ConvHraInt(TFld(operac|ASIGH_HSENT), TFld(operac|ASIGH_HSSAL)) * 100;
		}
	} 	
    
    if (!hortra)
    	return ;

	//Tomo las horas del regimen , cuanto van a normal y cuanto a extras
	SetKey(comerc|REGIMENbyREGI, regimen);
	if (GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		hsnormal =	IFld(comerc|REGIMEN_HSNORM);
		hsextra  =	IFld(comerc|REGIMEN_HSEXTRA);
		hsex100  = 0;
		hsex100f = 0;
	}

	if (hsnormal > hortra)
		hsnormal = hortra;

	hsextra  = hortra - hsnormal; //Extra es todo lo que no es normal

    /* Si es extra al 50 */
	if (str_eq (regimen, _REGIMEN_EXTRA50) ||
		str_eq (regimen, _REGIMEN_EXTRA50_SAPESA)) {
		hsnormal =	0;
		hsextra  =	hortra;
		hsex100  = 	0;
		hsex100f = 	0;
	}

    /* Si es extra al 100 */
	if (str_eq (regimen, _REGIMEN_EXTRA100) ||
		str_eq (regimen, _REGIMEN_EXTRA100_SAPESA)) {
		hsnormal =	0;
		hsextra  =	0;
		hsex100  = 	hortra;
		hsex100f = 	0;
	}

	if (FeriadoNovia(fecha, IFld(comerc|OBJETIVO_PAIS), 
										IFld(comerc|OBJETIVO_PROV))) {
		hsnormal =	0;
		hsextra  =	0;
		hsex100  = 	0;
		hsex100f = 	hortra;
	}

	*svh100 = hsex100;
	*svh100f = hsex100f;
	*svhn   = hsnormal;
	*svh50  = hsextra;
}

// Imprime los datos contenidos en el vector PCLI, de acuerdo a las opciones
// de consolidacion y salida por.
void	ImprimirReporte()
{
	bool	first = TRUE;

	cliant = NULL_LONG;
	objant = ptoant = NULL_SHORT;

	qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	if (*FmSFld(fm0, SALIDA) != 'A') {
	    RpSetIFld(rp0, PRI, TRUE);
		RpSetIFld(rp0, LEG, FALSE);
		RpSetIFld(rp0, PTO, FALSE); 
		RpSetIFld(rp0, OBJ, FALSE);
		RpSetIFld(rp0, CLI, FALSE);


		switch (*FmSFld(fm0, DET)) {
		case 'V':
			RpSetIFld(rp0, LEG, TRUE); 
		case 'P':                            
    	    RpSetIFld(rp0, PTO, TRUE);
		case 'O':
			RpSetIFld(rp0, OBJ, TRUE);
		case 'C':
			RpSetIFld(rp0, CLI, TRUE);
			break;
		}
		DoReport(rp0, ZPARAM);
	} 
	else {
		LimpiarAcumTot();
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumPto();		
	} 
	for (ecli = pcli ; ecli < ucli ; ecli++) {

    	// Recalculo los valores trabajados restandole lo de 
    	// empresa y adicional.
    	// lo hago aca para no tener que hacerlo en cada funcion
    	// que imprime detalles (arch, reporte, y no print)
	    ecli->sthn  = ecli->sthn - ecli->sahn - ecli->sehn;
	    ecli->sth50 = ecli->sth50 - ecli->sah50 - ecli->seh50;
    	ecli->sth100 = ecli->sth100 - ecli->sah100 - ecli->seh100;
   		ecli->sth100f = ecli->sth100f - ecli->sah100f - ecli->seh100f;

	    switch (FmIFld(fm0, OPCION)) {
	    case DIFERENC:
		    if ((ecli->svhn   - ecli->sthn)   == 0 &&
				(ecli->svh50  - ecli->sth50)  == 0 &&
				(ecli->svh100 - ecli->sth100) == 0 &&
				(ecli->svh100f - ecli->sth100f) == 0)
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
			if (*FmSFld(fm0, SALIDA) != 'A') {
			    RpSetLFld(rp0, RCLI,  ecli->cli);
				RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
	    	  	RpSetIFld(rp0, ROBJ,  ecli->obj);		
		    	RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
	   	    	RpSetIFld(rp0, RPTO,  ecli->puesto);
   			   	RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
			   	RpSetLFld(rp0, NROLEG,  ecli->nroleg);
			   	RpSetFld (rp0, DNROLEG, GetNombreLeg(FmIFld(fm0, EMP),
			   							ecli->nroleg));

				DoReport(rp0, ZCLI);
				DoReport(rp0, ZOBJ);
				DoReport(rp0, ZPTO);
				RpSetIFld(rp0, PRI, FALSE);
			}
			else {
				switch (*FmSFld(fm0, DET)) {
				case 'V':
					TitCli();
					TitObj();
					TitPto();
					break;
				case 'P':                            
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

		if (*FmSFld(fm0, SALIDA) != 'A')
			ImprimirNoPrint();
  		else  
			AcumularTotalizadores();

  		if (*FmSFld(fm0, DET) == 'V') {
			if (*FmSFld(fm0, SALIDA) != 'A')
				ImprimirDetalle();
			else
				ImprimirDetArch();
		}
	   	cliant = ecli->cli;
		strcpy(dcliant, SFld(bill|CLIENTE_RAZSOC));
		objant = ecli->obj;
		strcpy(dobjant, SFld(comerc|OBJETIVO_DESCRIP));
		ptoant = ecli->puesto;
		strcpy(dptoant, SFld(comerc|TPTOSER_DESCRIP));
	}
	if (*FmSFld(fm0, SALIDA) != 'A') {
	    RpSetLFld(rp0, RCLI,  	cliant);
	 	RpSetFld (rp0, RDCLI, 	dcliant);
	  	RpSetIFld(rp0, ROBJ,  	objant);
   		RpSetFld (rp0, RDOBJ, 	dobjant);
	    RpSetIFld(rp0, RPTO,  	ptoant);
		RpSetFld (rp0, RDPTO, 	dptoant);
	}
	switch (*FmSFld(fm0, DET)) {
		case 'P': 
		case 'V':
			TotPto();
		case 'O': 
			TotObj();
		case 'C':
			TotCli();       
			TotGen();
			break;			
	} 
}

// Abre el archivo ascii indicado en pantalla :)
void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
	else 
		SetearCabArch();
}

// Setea la cabecera del listado en el archivo ascii.
void	SetearCabArch()
{
	fprintf(fp, "Cliente\tRazón Social\tObjetivo\tDescrip. Obj\tPuesto\tDescrip. Puesto\tLegajp\tApellido y Nombre\tEstandard Vendido\t\t\t\tEstandar Trabajado\t\t\t\t\t\tDiferencia 1-2\t\t\t\tServicios Adicionales\t\t\t\tA cargo Empresa\n");
	fprintf(fp, "\t\t\t\t\t\t\t\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tH100F\tTotal\tHNpres\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\n");
}

// Esta funcion se utiliza siempre y es para imprimir el detalle en una 
// zona no print del reporte, cuyos campos son utilizados en las zonas
// automaticas de totalizacion.
void	ImprimirNoPrint()
{
	// NOPRINT p/totalizar
	// STANDARD
	RpSetIFld(rp0, SVHN,   ecli->svhn);
	RpSetIFld(rp0, SVH50,  ecli->svh50);
	RpSetIFld(rp0, SVH100, ecli->svh100 + ecli->svh100f);
	RpSetIFld(rp0, SVTOT,  ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
       // TRABAJADO
	RpSetIFld(rp0, STHN,	ecli->sthn);
	RpSetIFld(rp0, STH50, 	ecli->sth50);
	RpSetIFld(rp0, STH100,	ecli->sth100);
	RpSetIFld(rp0, STH100F,	ecli->sth100f);
	RpSetIFld(rp0, STTOT,	ecli->sthn + ecli->sth50 +
							ecli->sth100 + ecli->sth100f);
	RpSetIFld(rp0, STNPRES,	(RpIFld(rp0, SVTOT) - RpIFld(rp0, STTOT))*100);

	// DIFERENCIA
	RpSetIFld(rp0, DIFHN,	ecli->svhn   - ecli->sthn);
	RpSetIFld(rp0, DIFH50,	ecli->svh50	 - ecli->sth50);
	RpSetIFld(rp0, DIFH100, ecli->svh100 - ecli->sth100);
	RpSetIFld(rp0, DIFH100F,ecli->svh100f - ecli->sth100f);
//	RpSetIFld(rp0, DIFH100F,(-1)*(ecli->sth100f));
	// ADICIONALES
	RpSetIFld(rp0, SAHN, 	ecli->sahn);
	RpSetIFld(rp0, SAH50, 	ecli->sah50);
	RpSetIFld(rp0, SAH100,	ecli->sah100 + ecli->sah100f);
	RpSetIFld(rp0, SATOT, 	ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);
	// A CARGO EMPRESA
	RpSetIFld(rp0, SEHN, 	ecli->sehn);
	RpSetIFld(rp0, SEH50, 	ecli->seh50);
	RpSetIFld(rp0, SEH100,	ecli->seh100 + ecli->seh100f);
	RpSetIFld(rp0, SETOT, 	ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f);
	DoReport(rp0, ZNOPRINT);
}

// Se usa esta funcion cuando se debe imprimir el detalle de las fechas y 
// la salida es a reporte

void	ImprimirDetalle()
{
  	RpSetLFld(rp0, NROLEG,  ecli->nroleg);
  	RpSetFld (rp0, DNROLEG, GetNombreLeg(FmIFld(fm0, EMP), ecli->nroleg));

	// STANDARD
	RpSetIFld(rp0, FSVHN,   ecli->svhn);
	RpSetIFld(rp0, FSVH50,  ecli->svh50);
	RpSetIFld(rp0, FSVH100, ecli->svh100 + ecli->svh100f);
	RpSetIFld(rp0, FSVTOT,  ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
    // TRABAJADO                          
    
	RpSetIFld(rp0, FSTHN,	ecli->sthn);
	RpSetIFld(rp0, FSTH50,  ecli->sth50); 
	RpSetIFld(rp0, FSTH100, ecli->sth100);
	RpSetIFld(rp0, FSTH100F,ecli->sth100f);
	RpSetIFld(rp0, FSTTOT,	ecli->sthn + ecli->sth50 +
					 		ecli->sth100 + ecli->sth100f);
	RpSetIFld(rp0, FSTNPRES,(RpIFld(rp0, FSVTOT) -
								 RpIFld(rp0, FSTTOT)) * 100);
	// DIFERENCIA
	RpSetIFld(rp0, FDIFHN,	ecli->svhn   - ecli->sthn);
	RpSetIFld(rp0, FDIFH50, ecli->svh50  - ecli->sth50);
	RpSetIFld(rp0, FDIFH100,ecli->svh100 - ecli->sth100);
	RpSetIFld(rp0, FDIFH100F, ecli->svh100f - ecli->sth100f);
//	RpSetIFld(rp0, FDIFH100F,-(ecli->sth100f));
	// ADICIONALES
	RpSetIFld(rp0, FSAHN, 	ecli->sahn);
	RpSetIFld(rp0, FSAH50,  ecli->sah50);
	RpSetIFld(rp0, FSAH100, ecli->sah100 + ecli->sah100f);
	RpSetIFld(rp0, FSATOT,  ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);
	// A CARGO EMPRESA
	RpSetIFld(rp0, FSEHN, 	ecli->sehn);
	RpSetIFld(rp0, FSEH50,  ecli->seh50);
	RpSetIFld(rp0, FSEH100, ecli->seh100 + ecli->seh100f);
	RpSetIFld(rp0, FSETOT,  ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f);
	DoReport(rp0, ZLEG);
}   

// cuando la  opcion de seleccion es detallando las fechas y a archivo
// de texto se utiliza esta funcion.
void	ImprimirDetArch()
{
	fprintf(fp, "%8.8ld\t%20s\t%4.4d\t%20s\t%4.4d\t%20s\t%ld\t%s\t",
				 ecli->cli, SFld(bill|CLIENTE_RAZSOC), ecli->obj, SFld(comerc|OBJETIVO_DESCRIP),
				 ecli->puesto, SFld(comerc|TPTOSER_DESCRIP), ecli->nroleg,
				 GetNombreLeg(FmIFld(fm0, EMP), ecli->nroleg)); 
	fprintf(fp, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			 	(double)ecli->svhn  / 100.0,
			 	(double)ecli->svh50 / 100.0, 
			 	(double)(ecli->svh100 + ecli->svh100f) / 100.0,
			 	(double)(ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) / 100.0,
				(double)ecli->sthn    / 100.0,
				(double)ecli->sth50   / 100.0,
				(double)ecli->sth100  / 100.0,
				(double)ecli->sth100f / 100.0, 
				(double)(ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f) / 100.0, 
			 	(double)((ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) -              
			 	         (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f)) / 100.0, 
				(double)(ecli->svhn    - ecli->sthn)    / 100.0, 
				(double)(ecli->svh50   - ecli->sth50)   / 100.0,
				(double)(ecli->svh100  - ecli->sth100)  / 100.0,
				(double)(ecli->svh100f - ecli->sth100f) / 100.0,
			 	(double)ecli->sahn / 100.0,
			 	(double)ecli->sah50 / 100.0, 
			 	(double)(ecli->sah100 + ecli->sah100f) / 100.0,
			 	(double)(ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f) / 100.0,
			 	(double)ecli->sehn / 100.0,
			 	(double)ecli->seh50 / 100.0, 
			 	(double)(ecli->seh100 + ecli->seh100f) / 100.0,
			 	(double)(ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f) / 100.0); 
}

// Imprimelos titulos y totalizadores segun haya cambiado en el corte
// de control el cliente objetivo o puesto y segun sea la opcion de 
// consolidado elegida.
void	ImprimirTitArch()
{
	if (cliant != ecli->cli) {
		switch (*FmSFld(fm0, DET)) {
			case 'C':
				TotCli();
				if (*FmSFld(fm0, SALIDA)!='A') {
					RpSetLFld(rp0, RCLI, ecli->cli);
					RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				}
				break;			
			case 'O': 
				TotObj();	TotCli();
				TitCli();
				break;                           
			case 'P': 
				TotPto();	TotObj();	TotCli();
				TitCli();	TitObj();
				break;
			case 'V':
				TotPto();   TotObj();	TotCli();
				TitCli();	TitObj();	TitPto();
				break;
		}
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumPto();
	} 
	else {
		if (objant != ecli->obj) {
			switch (*FmSFld(fm0, DET)) {
				case 'O':                            
					TotObj();        
					if (*FmSFld(fm0, DET)!='A') {
    					RpSetIFld(rp0, ROBJ, ecli->obj);
						RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
					}
					break;
				case 'P':
					TotPto();	TotObj();
					TitObj();               
					break;
				case 'V': 
					TotPto();   TotObj();
					TitObj();   TitPto();
            	    break;
			}
			LimpiarAcumObj();
			LimpiarAcumPto();		
		} 
		else {  
			if (ptoant != ecli->puesto) {
				switch (*FmSFld(fm0, DET)) {
					case 'P':
						TotPto();
						if (*FmSFld(fm0, SALIDA)!='A') {
    					RpSetIFld(rp0, RPTO, ecli->puesto);          
						RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
						}
						break;
					case 'V': 
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
void 	LimpiarAcumTot()
{
		tsvhn= tsvh50= tsvh100= tsvtot  = 0;
	    tsthn= tsth50= tsth100= tsth100f= tsttot= tstnpres= 0;
		tdhn = tdh50 = tdh100 = tdh100f = 0;
	 	tsahn= tsah50= tsah100= tsatot  = 0;
	 	tsehn= tseh50= tseh100= tsetot  = 0;
}

void	LimpiarAcumCli()
{
	 	csvhn	= csvh50= csvh100= csvtot  = 0;
		csthn	= csth50= csth100= csth100f= csttot= cstnpres= 0;
		cdhn	= cdh50	= cdh100 = cdh100f = 0;
	 	csahn	= csah50= csah100= csatot  = 0;
	 	csehn	= cseh50= cseh100= csetot  = 0;
}

void	LimpiarAcumObj()
{
	 	osvhn	= osvh50= osvh100= osvtot  = 0;
		osthn	= osth50= osth100= osth100f= osttot= ostnpres= 0;
		odhn	= odh50	= odh100 = odh100f = 0;
	 	osahn	= osah50= osah100= osatot  = 0;
	 	osehn	= oseh50= oseh100= osetot  = 0; 
}

void	LimpiarAcumPto()
{
	 	psvhn= psvh50= psvh100= psvtot  = 0;
		psthn= psth50= psth100= psth100f= psttot= pstnpres= 0;
		pdhn = pdh50 = pdh100 = pdh100f = 0;
	 	psahn= psah50= psah100= psatot  = 0;
	 	psehn= pseh50= pseh100= psetot  = 0; 
}


// Acumula los totalizadores de cliente-objtivo-puesto y gnerales con
// datos tomados del vector ecli en la posicion corrinente que es la que
// se esta tratando
void	AcumularTotalizadores()
{
	int totsvh, totsth;
	
	csvhn	+= 	ecli->svhn;		osvhn	+= 	ecli->svhn;
	psvhn	+= 	ecli->svhn;    	tsvhn	+= 	ecli->svhn;

	csvh50  += 	ecli->svh50;	osvh50  += 	ecli->svh50;
	psvh50  += 	ecli->svh50;	tsvh50  += 	ecli->svh50;

	csvh100 += 	ecli->svh100 + ecli->svh100f;	osvh100 += 	ecli->svh100 + ecli->svh100f;
	psvh100 += 	ecli->svh100 + ecli->svh100f;	tsvh100 += 	ecli->svh100 + ecli->svh100f;

	totsvh = ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f;
	csvtot  += 	totsvh;      	osvtot  += 	totsvh;
	psvtot  += 	totsvh;      	tsvtot  +=  totsvh;

	csthn	+= 	ecli->sthn;  	osthn	+= 	ecli->sthn;
	psthn	+= 	ecli->sthn;  	tsthn	+=	ecli->sthn;

	csth50  +=	ecli->sth50; 	osth50  +=	ecli->sth50;
	psth50  +=	ecli->sth50; 	tsth50  +=	ecli->sth50;

	csth100 += 	ecli->sth100;	osth100 += 	ecli->sth100;
	psth100 += 	ecli->sth100;	tsth100 += 	ecli->sth100;

	csth100f+= 	ecli->sth100f; 	osth100f+= 	ecli->sth100f;
	psth100f+= 	ecli->sth100f; 	tsth100f+= 	ecli->sth100f;

	totsth  =	ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f;
	csttot	+= 	totsth;			osttot	+= 	totsth;
	psttot	+= 	totsth;        	tsttot	+=   totsth;

	cstnpres +=	totsvh -  totsth;	ostnpres +=	totsvh -  totsth;
	pstnpres +=	totsvh -  totsth;	tstnpres +=	totsvh -  totsth;

	cdhn	+= 	ecli->svhn - ecli->sthn; odhn	+= 	ecli->svhn - ecli->sthn;
	pdhn	+= 	ecli->svhn - ecli->sthn; tdhn	+= 	ecli->svhn - ecli->sthn;

	cdh50	+= 	ecli->svh50 - ecli->sth50; odh50	+= 	ecli->svh50 - ecli->sth50;
	pdh50	+= 	ecli->svh50 - ecli->sth50; tdh50	+= 	ecli->svh50 - ecli->sth50;

	cdh100 	+= 	ecli->svh100-ecli->sth100;	odh100 	+= 	ecli->svh100-ecli->sth100;
	pdh100 	+= 	ecli->svh100-ecli->sth100;	tdh100 	+= 	ecli->svh100-ecli->sth100;

	cdh100f	+= 	ecli->svh100f-ecli->sth100f;	odh100f	+= 	ecli->svh100f-ecli->sth100f;
	pdh100f	+= 	ecli->svh100f-ecli->sth100f;	tdh100f	+= 	ecli->svh100f-ecli->sth100f;

//	cdh100f +=	(-1)*ecli->sth100f;		odh100f +=	(-1)*ecli->sth100f;
//	pdh100f +=	(-1)*ecli->sth100f;    	tdh100f +=	(-1)*ecli->sth100f;

	csahn	+= 	ecli->sahn;			osahn	+= 	ecli->sahn;
	psahn	+= 	ecli->sahn;        	tsahn	+= 	ecli->sahn;

	csah50	+= 	ecli->sah50;		osah50	+= 	ecli->sah50;
	psah50	+= 	ecli->sah50;		tsah50	+= 	ecli->sah50;

	csah100	+= 	ecli->sah100 + ecli->sah100f;		osah100	+= 	ecli->sah100 + ecli->sah100f;
	psah100	+= 	ecli->sah100 + ecli->sah100f;		tsah100	+= 	ecli->sah100 + ecli->sah100f;

	csatot  =	ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	osatot  =	ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	psatot  =	ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;
	tsatot  =	ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f;

	csehn	+= 	ecli->sehn;			osehn	+= 	ecli->sehn;
	psehn   += 	ecli->sehn;        	tsehn   += 	ecli->sehn;

	cseh50	+= 	ecli->seh50;		oseh50	+= 	ecli->seh50;
	pseh50	+= 	ecli->seh50;        tseh50	+= 	ecli->seh50;

	cseh100	+= 	ecli->seh100 + ecli->seh100f;		oseh100	+= 	ecli->seh100 + ecli->seh100f;
	pseh100	+= 	ecli->seh100 + ecli->seh100f;      	tseh100	+= 	ecli->seh100 + ecli->seh100f;

	csetot  +=	ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	osetot  +=	ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	psetot  +=	ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
	tsetot  +=	ecli->sehn + ecli->seh50 + ecli->seh100 + ecli->seh100f;
}
// Imprime totales generales
void	TotGen()
{
/*	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total GENERAL\t\t\t");
		fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
						 	tsvhn, tsvh50, tsvh100, tsvtot,
							tsthn, tsth50, tsth100, tsth100f, tsttot, tstnpres,
							tdhn, tdh50, tdh100, tdh100f,
						 	tsahn, tsah50, tsah100, tsatot,
						 	tsehn, tseh50, tseh100, tsetot);
	 }
	 else
*/
	if (*FmSFld(fm0, SALIDA) != 'A')
	 	DoReport(rp0, ZTOTGEN);
}

// Imprime el total de un cliente en el archivo de salida
void	TotCli()
{
/*	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Cliente %8.8ld %20s\t\t\t", cliant, dcliant);
		fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
						 	csvhn, csvh50, csvh100, csvtot,
							csthn, csth50, csth100, csth100f, csttot, cstnpres,
							cdhn, cdh50, cdh100, cdh100f,
						 	csahn, csah50, csah100, csatot,
						 	csehn, cseh50, cseh100, csetot);
	 }
	 else
*/
	if (*FmSFld(fm0, SALIDA) != 'A')
	 	DoReport(rp0, ZTOTCLI);
}

// Imprime el total de un objetivo ven el archivo de salida
void	TotObj() 
{
/*	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Objetivo %4.4d %20s\t\t\t", objant, dobjant);
		fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
						 	osvhn, osvh50, osvh100, osvtot,
							osthn, osth50, osth100, osth100f, osttot, ostnpres,
							odhn, odh50, odh100, odh100f,
						 	osahn, osah50, osah100, osatot,
						 	osehn, oseh50, oseh100, osetot);
	}
	else 
*/
	if (*FmSFld(fm0, SALIDA) != 'A')
		DoReport(rp0, ZTOTOBJ);
}

// Imprime el total de puesto en el archivo de salida
void	TotPto() 
{
/*	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Puesto %4.4d %20s\t\t\t", ptoant,  dptoant);
		fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
						 	psvhn, psvh50, psvh100, psvtot,
							psthn, psth50, psth100, psth100f, psttot, pstnpres,
							pdhn, pdh50, pdh100, pdh100f,
						 	psahn, psah50, psah100, psatot,
						 	psehn, pseh50, pseh100, psetot);
	}
	else
*/
	if (*FmSFld(fm0, SALIDA) != 'A')
		DoReport(rp0, ZTOTPTO);
}

// Imprime el titulo de Cliente en el archivo de salida
void	TitCli()
{
/*	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "Cliente %8.8ld %20s\n",
							ecli->cli, SFld(bill|CLIENTE_RAZSOC));
	else {
*/
	if (*FmSFld(fm0, SALIDA) != 'A') {
	    RpSetLFld(rp0, RCLI,  ecli->cli);
		RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
		DoReport(rp0, ZCLI);
	}
}

// Imprime el titulo de objetivo en el archivo de salida
void	TitObj()
{
/*	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "\tObjetivo %4.4d %20s\n", 
				ecli->obj, SFld(comerc|OBJETIVO_DESCRIP));
	else {
*/
	if (*FmSFld(fm0, SALIDA) != 'A') {
      	RpSetIFld(rp0, ROBJ,  ecli->obj);		
	    RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
     	DoReport(rp0, ZOBJ);	
    }
}

// Imprime el titulo de puesto en el archivo de salida
void	TitPto()
{
/*	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "\t\tPuesto %4.4d %20s\n", 
				ecli->puesto, SFld(comerc|TPTOSER_DESCRIP));
	else {
*/
	if (*FmSFld(fm0, SALIDA) != 'A') {
   	    RpSetIFld(rp0, RPTO,  ecli->puesto);
   	   	RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
        DoReport(rp0, ZPTO);
    }
}



private int compcli(struct cliente *a, struct cliente *b)
{
	return	a->cli	< b->cli ? -1 : a->cli  > b->cli	? 1 :
			a->obj	< b->obj ? -1 : a->obj  > b->obj	? 1 :
			a->puesto	< b->puesto ? -1 : a->puesto > b->puesto	? 1 :  0;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
			InicListaXusr(FmIFld(fm0, EMP));
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
			FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIH, row)),row);
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
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row) ,FmIFld(fm, OBJH, row)), row);
    break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;				
}	

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
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

