/********************************************************************
*
* MODULE & VERSION : @(#)hsvigite.c	1.1
* DATE             : 08/04/01
* TIME             : 12:47:52
*
* CREATED          : 18/02/99 Gloria
*
* DESCRIPTION:
*	Listado de Control de Horas por Vigilidor.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 

*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "billpro.h"
#include "hsvigite.fmh"
#include "ambiente.h"
#include "filial.h"

#ifdef _NOVIA_VER_1_0
	#include "hsvigite.rph"
#else	
	#include "hsvigite2.rph"
#endif

#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "sue.sch"

#define MAXLEG		500000
#define	ERR_ARCHI	"No se pudo abrir el archivo."

#define _POR_FECHA		'F'
#define _POR_SEMANA		'S'
#define _POR_LEGAJO		'L'

#define _HORAS_STD	1
#define _HORAS_NOR	2

/* Estructuras */
struct vigil {
	char deleg[6];
	long nroleg;
	long cli;
	int	 obj;
    DATE fecha;
    int  ptoser;
	int  puesto;
	int  nroint;
    char cond[2];
    short nrosem, htipo;
    TIME  hsent, hssal;
	long  sthn, sth50, sth100, sth100f;
	long  sahn, sah50, sah100, sah100f;
	long  sehn, seh50, seh100, seh100f;
	long  sihn, sih50, sih100, sih100f;
	long  sistd;
} pvig[MAXLEG], *uvig = pvig, *evig;

/* anteriores */
long 	vigant;
char 	dvigant[67];
short	semant;
DATE    fecant;
/*
int		objant, ptoant;
, dobjant[30], dptoant[30];
*/
                  	
/* Totalizadores para impresion en Archivo ASCII */
/* Tot Generales */
long 	tsthn, tsth50, tsth100, tsth100f, 
	 	tsahn, tsah50, tsah100,
	 	tsihn, tsih50, tsih100,
	 	tsehn, tseh50, tseh100,
	 	tistd, tirea,  tidif, tiemp, tides;

/* Tot de Vigilador */
long 	vsthn, vsth50, vsth100, vsth100f,
	 	vsahn, vsah50, vsah100,
	 	vsihn, vsih50, vsih100,
	 	vsehn, vseh50, vseh100,
	 	vistd, virea,  vidif, viemp, vides;

/* Tot por Semana */
long 	ssthn, ssth50, ssth100, ssth100f,
	 	ssahn, ssah50, ssah100,
	 	ssihn, ssih50, ssih100,
	 	ssehn, sseh50, sseh100,
	 	sistd, sirea,  sidif, siemp, sides;

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
void TotLeg();
void TitLeg();
void TotSem();
void AcumularTotalizadores();
void AcumularStd();
void LimpiarAcumTot();
void LimpiarAcumLeg();
void GenerarReporte();
void CargarVigil(short htipo, bool esexcep, int emp, char deleg[6], long cliente, int objetivo, long nroleg, 
				  DATE fecha, TIME hsent, TIME hssal, int ptoser, int puesto, int nroint, char cond[],
				  int condic, int motivo, short nrosem, int hn, int h50, int h100, int h100f);
private int compvig(struct vigil *a, struct vigil *b);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
void LimpiarAcumSem();

/* Declaraciones globales */
FILE	*fp;
form fm0;
report rp0;
schema comerc, operac, bill, sue;
char delegacion[6], delant[6], Deleg_Descrip[31];
/* Programa principal */
wcmd(hsvigite, 1.1 04/01/08)
{


	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);

	fm0 = OpenForm("hsvigite", FM_EABORT);
	
	while (DoForm(fm0, before, after) != FM_UPDATE) return;
	strcpy( delegacion, NULL_STR);
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

	FinListaXusr();
}

void AbrirReporte()
{
	#ifdef _NOVIA_VER_1_0
	    rp0 = OpenReport("hsvigite", RP_EABORT|RP_NOBEGIN);
	#else	
	    rp0 = OpenReport("hsvigite2", RP_EABORT|RP_NOBEGIN);
	#endif

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
	RpSetFld (rp0, REMP,    FmSFld(fm0, DEMP));
	RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));

	RpSetLFld(rp0, RVIGILD,  FmLFld(fm0, VIGILD));
	RpSetFld (rp0, RDVIGILD, FmSFld(fm0, DVIGILD));

	RpSetLFld(rp0, RVIGILH,  FmLFld(fm0, VIGILH));
	RpSetFld (rp0, RDVIGILH, FmSFld(fm0, DVIGILH));
	RpSetIFld(rp0, RRETRO,   FmIFld(fm0, FRETRO));
}

void GenerarReporte()
{
	dbcursor cparte, cexc, cretro, cretexc;

	short tipoexc;

	long legant=NULL_LONG; 
	DATE dia_ant=NULL_DATE, semd, semh;
	short hsemana, estasem, antsem = NULL_SHORT;

	cexc   = CreateCursor(EXCEPCIONbyLEGAJO, IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMPLE, IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyREMPLE, IO_NOT_LOCK);
	cretexc= CreateCursor(RETROEXCbyLEGAJO, IO_NOT_LOCK);

	if (!FmIsNull(fm0, VIGILD)) {
		SetCursorFrom(cparte, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	}
	else {
		SetCursorFrom(cparte, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
	}
	while(FetchCursor(cparte) != ERROR) {
		if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) ||
			DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) != BRIGADA) ||
		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)) == BRIGADA))
			continue;
	
		if (legant != LFld(PARTE_NROLEG) || dia_ant != DFld(PARTE_DIA)) {

			if (legant != LFld(PARTE_NROLEG)) {
				antsem = NULL_SHORT;
				legant = LFld(PARTE_NROLEG);
			}

			estasem = NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA));
			if (estasem != antsem) {
				SemanaDesdeHasta(DFld(PARTE_DIA), &semd, &semh);

				hsemana = HorasNormalesPorSemana(IFld( PARTE_EMP), LFld(PARTE_NROLEG), GetCliePais(LFld(PARTE_CLIENTE),IFld(PARTE_OBJETIVO)),
													GetClieProv(LFld(PARTE_CLIENTE),IFld(PARTE_OBJETIVO)),
													estasem, semd, semh);

/*				//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
				if (!ValidaListaXusr(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)))
					continue;
				if (!ValidaFilial(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
					continue;
*/
				//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 
				
				CargarVigil(_HORAS_STD, FALSE,
					FmIFld(fm0, EMP),
					delegacion,
					LFld(PARTE_CLIENTE),
					IFld(PARTE_OBJETIVO),
					LFld(PARTE_NROLEG),
					DFld(PARTE_DIA),
					TFld(PARTE_HORAENT),
					TFld(PARTE_HORASAL),
					IFld(PARTE_PTOSER),
					IFld(PARTE_PUESTO),
					IFld(PARTE_NROINT),
					SFld(PARTE_CONDIC),
					NULL_SHORT,
					NULL_SHORT,
					NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)),
					hsemana,
					0,
					0,
					0);

				antsem = estasem;
			}
			dia_ant = DFld(PARTE_DIA);
		}

/*		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
*/
		
		CargarVigil(_HORAS_NOR, FALSE,
					FmIFld(fm0, EMP),
					delegacion,
					LFld(PARTE_CLIENTE),
					IFld(PARTE_OBJETIVO),
					LFld(PARTE_NROLEG),
					DFld(PARTE_DIA),
					TFld(PARTE_HORAENT),
					TFld(PARTE_HORASAL),
					IFld(PARTE_PTOSER),
					IFld(PARTE_PUESTO),
					IFld(PARTE_NROINT),
					SFld(PARTE_CONDIC),
					NULL_SHORT,
					NULL_SHORT,
					NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)),
					IFld(PARTE_HSNOR),
					IFld(PARTE_HS50),
					IFld(PARTE_HS100F),
					IFld(PARTE_HS100FE));
	}
	DeleteCursor(cparte);
	if (!FmIsNull(fm0, VIGILD)) {
		SetCursorFrom(cexc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT); 
		SetCursorTo  (cexc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	}
	else {
		SetCursorFrom(cexc, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cexc, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
	}

	while (FetchCursor(cexc) != ERROR) {
		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) ||
			DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) != BRIGADA) ||
		    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) == BRIGADA))
			continue;

		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM){
			continue;
		}

		SetKey(comerc|OBJETIVObyCLIENTE, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

/*		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)))
				continue;
		if (!ValidaFilial(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
*/
		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

		CargarVigil(_HORAS_NOR, TRUE,
					FmIFld(fm0, EMP),
					delegacion,
					LFld(EXCEPCION_CLIENTE),
					IFld(EXCEPCION_OBJETIVO),
					LFld(EXCEPCION_NROLEG),
					DFld(EXCEPCION_DIA),
					NULL_TIME,
					NULL_TIME,
					IFld(EXCEPCION_PTOSER),
					IFld(EXCEPCION_PUESTO),
					IFld(EXCEPCION_NROINT),
					NULL_STR,
					IFld(EXCEPCION_CONDIC),
					IFld(EXCEPCION_MOTIVO),
					NroSemana(FmDFld(fm0, FECHAD), DFld(EXCEPCION_DIA)),
					IFld(EXCEPCION_HORAS),
					IFld(EXCEPCION_HS50),
					FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
					FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0);
	}
	DeleteCursor(cexc);

	if (FmIFld(fm0, FRETRO)) {
		if (!FmIsNull(fm0, VIGILD)) {
			SetCursorFrom(cretro, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (cretro, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
		}
		else {
			SetCursorFrom(cretro, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE, MIN_LONG, MIN_SHORT);
			SetCursorTo  (cretro, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
		}
		while(FetchCursor(cretro) != ERROR) {
			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) ||
				DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)) == BRIGADA))
				continue;

/*			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
			if (!ValidaListaXusr(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)))
					continue;
			if (!ValidaFilial(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
*/
            //fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

			CargarVigil(_HORAS_NOR, FALSE,
						FmIFld(fm0, EMP),
						delegacion,
						LFld(RETRO_CLIENTE),
						IFld(RETRO_OBJETIVO),
						LFld(RETRO_NROLEG),
						DFld(RETRO_DIA),
						TFld(RETRO_HORAENT),
						TFld(RETRO_HORASAL),
						IFld(RETRO_PTOSER),
						IFld(RETRO_PUESTO),
						IFld(RETRO_NROINT),
						SFld(RETRO_CONDIC),
						NULL_SHORT, 
						NULL_SHORT,
						NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)),
						IFld(RETRO_DHSNOR),
						IFld(RETRO_DHS50),
						IFld(RETRO_DHS100F),
						IFld(RETRO_DHS100FE));
		}
		DeleteCursor(cretro);
	
		if (!FmIsNull(fm0, VIGILD)) {
			SetCursorFrom(cretexc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD), FmDFld(fm0, FECHAD), MIN_LONG,
								   MIN_SHORT);
			SetCursorTo  (cretexc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAH), MAX_LONG,
								   MAX_SHORT);
		}
		else {
			SetCursorFrom(cretexc, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE, MIN_LONG, MIN_SHORT);
			SetCursorTo  (cretexc, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
		}

		while (FetchCursor(cretexc) != ERROR) {
			if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) ||
				DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) == BRIGADA))
				continue;

			tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), IFld(RETROEXC_MOTIVO));
			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM){
					continue;
			}

			SetKey(comerc|OBJETIVObyCLIENTE, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
			(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
/*			if (!ValidaListaXusr(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)))
					continue;
			if (!ValidaFilial(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
            
            //fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 
*/
			CargarVigil(_HORAS_NOR, TRUE,
						FmIFld(fm0, EMP),
						delegacion,
						LFld(RETROEXC_CLIENTE),
						IFld(RETROEXC_OBJETIVO),
						LFld(RETROEXC_NROLEG),
						DFld(RETROEXC_DIA),
						NULL_TIME,
						NULL_TIME,
						IFld(RETROEXC_PTOSER),
						IFld(RETROEXC_PUESTO),
						IFld(RETROEXC_NROINT),
						NULL_STR,
						IFld(RETROEXC_CONDIC),
						IFld(RETROEXC_MOTIVO),
						NroSemana(FmDFld(fm0, FECHAD), DFld(RETROEXC_DIA)),
						IFld(RETROEXC_DHORAS),
						IFld(RETROEXC_DHS50),
						FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
						FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0);
		}
		DeleteCursor(cretexc);
	}
}

// Carga el vector de PVIG
//
// Si viene de PARTE h100 contiene horas extras de francos y 
// h100f de feriados.
// si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene
// vacio.

void	CargarVigil(short htipo, bool esexcep, int emp, char deleg[6], long cliente, int objetivo, long nroleg, 
					 DATE fecha, TIME hsent, TIME hssal, int ptoser, int puesto, int nroint, char cond[],
					 int condic, int motivo, short nrosem, int hn, int h50, int h100, int h100f)
{
	short tipoexc;
	
	if (*FmSFld(fm0, TIPTIE)!='T') { // Si pidieron todos ni siquiera pregunto porque siempre va todo
		bool partime=VigPartime(emp, nroleg, fecha);
		 //PuestoEsPartime(cliente, objetivo,ptoser,puesto);
		if (partime) {
			if (*FmSFld(fm0, TIPTIE)!='P') return; //El registro es Part Time pero pidieron Full Time
		}
		else {
			if (*FmSFld(fm0, TIPTIE)=='P') return; //El registro es Full Time pero pidieron Part Time
		}
	}

	if (!FmIFld(fm0, DETCLI)) {
		for (evig = pvig ; evig < uvig ; evig++)
			if (strcmp( evig->deleg, deleg) == 0  && evig->nroleg == nroleg && evig->fecha == fecha && evig->nrosem == nrosem && evig->htipo == htipo ) {
				break;
			}
	}
	else {
		for (evig = pvig ; evig < uvig ; evig++) {
			if (!esexcep && strcmp( evig->deleg, deleg) == 0  && evig->nroleg == nroleg &&
				evig->fecha == fecha && evig->cli == cliente &&
				evig->obj == objetivo && evig->nrosem == nrosem &&
				evig->ptoser == ptoser && evig->puesto == puesto &&
				evig->nroint == nroint) {
				strcpy(evig->cond, cond);
				evig->hsent = hsent;
				evig->hssal = hssal;
			}

			if (strcmp( evig->deleg, deleg) == 0  && evig->nroleg == nroleg && 
				evig->fecha == fecha && evig->cli == cliente && 
				evig->obj == objetivo && evig->nrosem == nrosem &&
				evig->ptoser == ptoser && evig->puesto == puesto &&
				evig->nroint == nroint &&
				evig->htipo == htipo ) {
				if (!str_eq(cond, NULL_STR))
					strcpy(evig->cond, cond);
				break;
			}

		}
	}
	if (evig == uvig) {
		if (uvig == &pvig[MAXLEG]) Error("Tabla interna saturada. Max %d", MAXLEG);
		strcpy(uvig->deleg, deleg);
		uvig->nroleg = nroleg;
		uvig->fecha  = fecha;
		uvig->hsent  = hsent;
		uvig->hssal  = hssal;
		uvig->ptoser = ptoser;
		uvig->puesto = puesto;
		uvig->nroint = nroint;
		strcpy(uvig->cond, cond);
		uvig->cli	= cliente;
		uvig->obj	= objetivo;
		uvig->nrosem = nrosem;
		uvig->htipo  = htipo;
		uvig->sehn	= 0;
		uvig->seh50 = 0;
		uvig->seh100= 0;
		uvig->seh100f= 0;
		uvig->sahn	= 0;
		uvig->sah50	= 0;
		uvig->sah100= 0;
		uvig->sah100f= 0;
		uvig->sihn	= 0;
		uvig->sih50	= 0;
		uvig->sih100= 0;
		uvig->sih100f= 0;
		uvig->sthn	= 0;
		uvig->sth50	= 0;
		uvig->sth100= 0;
		uvig->sth100f= 0;
		uvig->sistd= 0;
		uvig++;
	} 
	
	if (htipo == _HORAS_STD ) {
		evig->sistd += (hn + h50 + h100 + h100f);
		return;
	}
	
	if (condic != NULL_SHORT) {		// VIENE DE EXCEPCION
		tipoexc = ParteTipoExcepcion(condic, motivo);
		if (tipoexc == _TIPO_EXCEP_DESC){
			evig->sihn += hn;
			evig->sih50 += h50;
			evig->sih100 += h100;
			evig->sih100f += h100f;
		}
		else { 
			if (condic == ACARGO_EMP) {
				evig->sehn += hn;
				evig->seh50 += h50;
				evig->seh100 += h100;
				evig->seh100f += h100f;
			}
			else {
				evig->sahn += hn;
				evig->sah50 += h50;
				evig->sah100 += h100;
				evig->sah100f += h100f;
			}
		}
	} else {                      // VIENE DE PARTE
		evig->sthn += hn;
		evig->sth50 += h50;
		evig->sth100 += h100;
		evig->sth100f += h100f;
	}   
}					 

private int compvig(struct vigil *a, struct vigil *b)
{
	return	                                                                 
			strcmp(a->deleg ,b->deleg) >0  ? 1 : strcmp( a->deleg, b->deleg)< 0  ? -1 :
			a->nroleg > b->nroleg ? 1 : a->nroleg < b->nroleg ? -1 :
			a->fecha  > b->fecha  ? 1 : a->fecha  < b->fecha  ? -1 :
			a->htipo  > b->htipo  ? 1 : a->htipo  < b->htipo  ? -1 :
			0;
}

// Imprime los datos contenidos en el vector PCLI, de acuerdo a las opciones
// de consolidacion y salida por.
void	ImprimirReporte()
{
	bool	first = TRUE;

	LimpiarAcumTot();
	LimpiarAcumLeg();

	qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)compvig);
	for (evig = pvig ; evig < uvig ; evig++) {
		// fprintf (stderr, "VIENE %ld %.3D std  %ld %ld \n", evig->nroleg, evig->fecha, evig->sthn, evig->sistd);

    	// Recalculo los valores trabajados restandole lo de 
    	// empresa y adicional.
    	// lo hago aca para no tener que hacerlo en cada funcion
    	// que imprime detalles (arch, reporte, y no print)

	    evig->sthn  = evig->sthn - evig->sahn - evig->sehn;
	    evig->sth50 = evig->sth50 - evig->sah50 - evig->seh50;
	    evig->sth100 = evig->sth100   - evig->sah100 - evig->seh100;
	    evig->sth100f = evig->sth100f - evig->sah100f - evig->seh100f;

		if (strcmp( delant, evig->deleg) != 0)
			strcpy( Deleg_Descrip, strcmp( evig->deleg, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( evig->deleg));

		if (vigant != evig->nroleg) {
		    SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), evig->nroleg);
		    if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
	    		SetFld(sue|PER_APYNOM, "ERROR: Empleado Inexistente");
		}
		if (first) {
			first = FALSE;
			if (*FmSFld(fm0, SALIDA) != 'A') {
			    RpSetLFld(rp0, RVIGIL,  evig->nroleg);
				RpSetFld (rp0, RDVIGIL, SFld(sue|PER_APYNOM));
				
    			switch (*FmSFld(fm0,  R_DETALLE)) {
   				case _POR_FECHA:
					DoReport(rp0, ZVIG);
					break;
				case _POR_SEMANA:
					DoReport(rp0, ZVIG);
					break;
				case _POR_LEGAJO:
					break;
				}
    		}
			else {
    			switch (*FmSFld(fm0, R_DETALLE)) {
				case _POR_FECHA:
					TitLeg();
					break;
				case _POR_SEMANA:
					TitLeg();
					break;
				case _POR_LEGAJO:
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

		AcumularStd();
		if (evig->htipo != _HORAS_STD && *FmSFld(fm0, R_DETALLE) == _POR_FECHA) {
			if (*FmSFld(fm0, SALIDA) != 'A'){
				ImprimirDetalle();
			}				
			else{
				ImprimirDetArch();
			}
		}

	   	vigant = evig->nroleg;
		strcpy(dvigant, SFld(sue|PER_APYNOM));
		strcpy(delant, evig->deleg);
		semant = evig->nrosem;
		fecant = evig->fecha;
	}
	TotSem();
	TotLeg();
	TotGen();
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
	#ifdef _NOVIA_VER_1_0
		fprintf(fp, "\t\tVigilador\tNombre y Apellido\tCliente\tRazon Social\tObjetivo\tDescrip. Obj.\tFecha\tRegimen\tCond\tCond.Teor.\tDistinta Condicion\tHr Entrada\tHr Salida\tEstandar Trabajado\t\t\t\tServicios Adicionales\t\t\tA Cargo Empresa\n");
		fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tHn\tH50\tH100\n");
	#endif

	#ifdef _NOVIA_VER_2_0
		fprintf(fp, "Vigilador\tNombre y Apellido\tCliente\tRazon Social\tObjetivo\tDescrip. Obj.\tFecha\tCond\tEstandar Trabajado\t\t\t\tServicios Adicionales\t\t\tA Cargo Empresa\t\t\tImproductividad\n");
		fprintf(fp, "\t\t\t\t\t\t\t\t\t\tHn\tH50\tH100\tH100F\tHn\tH50\tH100\tHn\tH50\tH100\tStd.\tReal\tDiferencia\tEmpresa\tDescontar\n");
	#endif
}

// Esta funcion se utiliza siempre y es para imprimir el detalle en una 
// zona no print del reporte, cuyos campos son utilizados en las zonas
// automaticas de totalizacion.
void	ImprimirNoPrint()
{
	long sreal=0;

	// NOPRINT p/totalizar
       // TRABAJADO
	RpSetLFld(rp0, STHN,	evig->sthn);
	RpSetLFld(rp0, STH50, 	evig->sth50);
	RpSetLFld(rp0, STH100,	evig->sth100);
	RpSetLFld(rp0, STH100F,	evig->sth100f);
	// ADICIONALES
	RpSetLFld(rp0, SAHN, 	evig->sahn);
	RpSetLFld(rp0, SAH50, 	evig->sah50);
	RpSetLFld(rp0, SAH100,	evig->sah100 + evig->sah100f);
	// A CARGO EMPRESA
	RpSetLFld(rp0, SEHN, 	evig->sehn);
	RpSetLFld(rp0, SEH50, 	evig->seh50);
	RpSetLFld(rp0, SEH100,	evig->seh100 + evig->seh100f);

	#ifdef _NOVIA_VER_2_0
		// IMPRODUCTIVIDAD
		sreal = evig->sthn+evig->sth50+evig->sth100+evig->sth100f+
		        evig->sahn+evig->sah50+evig->sah100 + evig->sah100f+
		        evig-5>sehn+evig->seh50+evig->seh100 + evig->seh100f;

		RpSetLFld(rp0, ISTD, evig->sistd);
		RpSetLFld(rp0, IREA, sreal);
		RpSetLFld(rp0, IDIF, 0);
		RpSetLFld(rp0, IEMP, 0);
		RpSetLFld(rp0, IDES, (evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f));
	#endif		

	DoReport(rp0, ZNOPRINT);
}

// Se usa esta funcion cuando se debe imprimir el detalle de las fechas y 
// la salida es a reporte

void ImprimirDetalle()
{
	char regimen[15];
	long sreal;

	RpSetDFld(rp0, RFCH, evig->fecha);

	if (FmIFld(fm0, DETCLI)) {
		GetRegimenEfectivo(FmIFld(fm0, EMP), evig->nroleg, regimen, evig->fecha);

		RpSetLFld(rp0, RCLI,   evig->cli);
		RpSetIFld(rp0, ROBJ,   evig->obj);
		RpSetFld (rp0, RDOBJ,  GetObjDescrip(evig->cli, evig->obj));
		RpSetFld (rp0, RREGIM, regimen);
		RpSetTFld(rp0, RHSENT, evig->hsent);
		RpSetTFld(rp0, RHSSAL, evig->hssal);
	}
	RpSetFld (rp0, RCOND,   evig->cond);

    // TRABAJADO
	RpSetLFld(rp0, FSTHN,	evig->sthn);
	RpSetLFld(rp0, FSTH50,  evig->sth50);
	RpSetLFld(rp0, FSTH100, evig->sth100);
	RpSetLFld(rp0, FSTH100F,evig->sth100f);
	// ADICIONALES
	RpSetLFld(rp0, FSAHN, 	evig->sahn);
	RpSetLFld(rp0, FSAH50,  evig->sah50);
	RpSetLFld(rp0, FSAH100, evig->sah100 + evig->sah100f);
	// A CARGO EMPRESA
	RpSetLFld(rp0, FSEHN, 	evig->sehn);
	RpSetLFld(rp0, FSEH50,  evig->seh50);
	RpSetLFld(rp0, FSEH100, evig->seh100 + evig->seh100f);

	#ifdef _NOVIA_VER_2_0
		// IMPRODUCTIVIDAD

		sreal = evig->sthn+evig->sth50+evig->sth100+evig->sth100f+
		        evig->sahn+evig->sah50+evig->sah100 + evig->sah100f+
		        evig->sehn+evig->seh50+evig->seh100 + evig->seh100f;
		        
		RpSetLFld(rp0, FISTD, 0);
		RpSetLFld(rp0, FIREA, sreal);
		RpSetLFld(rp0, FIDIF, 0);
		RpSetLFld(rp0, FIEMP, 0);
		RpSetLFld(rp0, FIDES, (evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f));
	#endif
	
	DoReport(rp0, ZFCH);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
		case EMP:
			FmSetFld(fm0, R_DETALLE, "F");
			break;
	}
	return FM_OK;				
}	
static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		InicListaXusr(FmIFld(fm0, EMP));
    break;
   	case SALIDA:
		if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "hsvigite.txt");


		if (*FmSFld(fm0, SALIDA)=='A')
			FmSetIFld(fm0, IMPSUB, FALSE);
		else 
			FmSetIFld(fm0, IMPSUB, NULL_SHORT);
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
	case R_DETALLE:
		#ifdef _NOVIA_VER_1_0
			if (*FmSFld(fm0, fno) == _POR_SEMANA) {
				WiMsg ("Esta opcion no esta disponible en esta version ");
				return FM_REDO;
			}
		#endif

		#ifdef _NOVIA_VER_2_0
			/* No esta disponible porque las improductividades son semanales */
			if (*FmSFld(fm0, fno) == _POR_LEGAJO) {
				WiMsg ("Esta opcion no esta disponible en esta version ");
				return FM_REDO;
			}
		#endif
	break;

/*	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
*/
	}
	return FM_OK;				
}	

// cuando la  opcion de seleccion es detallando las fechas y a archivo
// de texto se utiliza esta funcion.
void ImprimirDetArch()
{
	long sreal;
	char regimen[15], v_vigil[2], v_cond_teorica[2];
	int v_numfran=NULL_SHORT;
	bool esfranco=FALSE;


    SetKey(bill|CLIENTEbyCLIENTE, evig->cli);
   	GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	GetRegimenEfectivo(FmIFld(fm0, EMP), evig->nroleg, regimen, evig->fecha);

	if (!FmIFld(fm0, DETCLI)){
		fprintf(fp, "\t%1.D\t\t", evig->fecha );
		fprintf(fp, "%s\t",       evig->cond  );
	}
	else{
		fprintf(fp, "%s\t",   evig->deleg);
		fprintf(fp, "%s\t",   strcmp( evig->deleg, NULL_STR) == 0 ? NULL_STR : GetDescDeleg( evig->deleg));
		fprintf(fp, "%ld\t",  evig->nroleg);
		fprintf(fp, "%s\t",   SFld(sue|PER_APYNOM));
		fprintf(fp, "%ld\t",  evig->cli);
		fprintf(fp, "%s\t",   SFld(bill|CLIENTE_RAZSOC));
		fprintf(fp, "%d\t",   evig->obj);
		fprintf(fp, "%s\t",   GetObjDescrip(evig->cli, evig->obj));
		fprintf(fp, "%.1D\t", evig->fecha);
		fprintf(fp, "%s\t",   regimen);
		fprintf(fp, "%s\t",   evig->cond);

		sprintf (v_vigil, "%s", TipoVig(FmIFld(fm0, EMP), evig->cli, evig->obj, evig->nroleg, 
		                                evig->ptoser, evig->puesto, evig->nroint, evig->fecha));

		esfranco=Franco(FmIFld(fm0, EMP), evig->nroleg, evig->fecha, v_vigil, v_numfran);

		if (esfranco)
 			sprintf(v_cond_teorica, "F");
 		else
 			sprintf(v_cond_teorica, "T");

		fprintf(fp, "%s\t", v_cond_teorica);
		fprintf(fp, "%B\t", (v_cond_teorica[0]!=evig->cond[0] && 
		                     (evig->cond[0]=='T' || evig->cond[0]=='F') ));
		
		fprintf(fp, "%.1T\t", evig->hsent);
		fprintf(fp, "%.1T\t", evig->hssal);
	}

	sreal = (evig->sthn+evig->sth50+evig->sth100+evig->sth100f +
		        evig->sahn+evig->sah50+evig->sah100 + evig->sah100f +
		        evig->sehn+evig->seh50+evig->seh100 + evig->seh100f);

	fprintf(fp, "%.2f\t", (double)evig->sthn    / 100);
	fprintf(fp, "%.2f\t", (double)evig->sth50   / 100);
	fprintf(fp, "%.2f\t", (double)evig->sth100  / 100);
	fprintf(fp, "%.2f\t", (double)evig->sth100f / 100);
	fprintf(fp, "%.2f\t", (double)evig->sahn    / 100);
	fprintf(fp, "%.2f\t", (double)evig->sah50   / 100);
	fprintf(fp, "%.2f\t", (double)(evig->sah100 + evig->sah100f) / 100);
	fprintf(fp, "%.2f\t", (double)evig->sehn  / 100);
	fprintf(fp, "%.2f\t", (double)evig->seh50 / 100);
	fprintf(fp, "%.2f"  , (double)(evig->seh100 + evig->seh100f) / 100);

	#ifdef _NOVIA_VER_1_0 
		fprintf(fp, "\n");
	#else
		fprintf(fp, "\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
					0.0,
					((double)sreal)/100.0,
					0.0,
					0.0,
					((double)(evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f) / 100.0));
	#endif
}

// Imprimelos titulos y totalizadores segun haya cambiado en el corte
// de control el vigilador segun sea la opcion de consolidado elegida.
void	ImprimirTitArch()
{
	switch (*FmSFld(fm0, R_DETALLE)) {
		case _POR_FECHA:
			if (semant != evig->nrosem || vigant != evig->nroleg) {
				TotSem();
				LimpiarAcumSem();
			}

			if (strcmp( delant, evig->deleg) != 0) {
				TotLeg();
				TitLeg();
				LimpiarAcumLeg();
			}
			else
				if (vigant != evig->nroleg) {
					TotLeg();
					TitLeg();
					LimpiarAcumLeg();
				}
		break;
		case _POR_SEMANA:
			if (semant != evig->nrosem || vigant != evig->nroleg) {
				TotSem();
				LimpiarAcumSem();
			}
			if ( strcmp( delant, evig->deleg)!=0) {
				TotLeg();
				TitLeg();
				LimpiarAcumLeg();
			}
			else{
				if (vigant != evig->nroleg) {
					TotLeg();
					TitLeg();
					LimpiarAcumLeg();
				}
			}
		break;
		case _POR_LEGAJO:
			if ( strcmp( delant, evig->deleg)!=0) {
				TotLeg();
				LimpiarAcumLeg();
			}
			else
				if (vigant != evig->nroleg) {
					TotLeg();
					LimpiarAcumLeg();
				}
		break;
	}
}

//Limpia los acumuladores de cliente/totales/puesto y objetivos
void LimpiarAcumTot()
{
	tsthn = tsth50 = tsth100 = tsth100f = 0;
	tsahn = tsah50 = tsah100 = 0;
	tsehn = tseh50 = tseh100 = 0;
	tsihn = tsih50 = tsih100 = 0;
 	tistd = tirea  = tidif = tiemp = tides = 0;
}

void LimpiarAcumLeg()
{
	vsthn = vsth50 = vsth100 = vsth100f = 0;
	vsahn = vsah50 = vsah100 = 0;
	vsehn = vseh50 = vseh100 = 0;
	vsihn = vsih50 = vsih100 = 0;
 	vistd = virea  = vidif = viemp = vides = 0;
}

void LimpiarAcumSem()
{
	ssthn = ssth50 = ssth100 = ssth100f = 0;
	ssahn = ssah50 = ssah100 = 0;
	ssehn = sseh50 = sseh100 = 0;
	ssihn = ssih50 = ssih100 = 0;
 	sistd = sirea  = sidif = siemp = sides = 0;
}

// Acumula los totalizadores de cliente-objtivo-puesto y gnerales con
// datos tomados del vector evig en la posicion corrinente que es la que
// se esta tratando
void AcumularTotalizadores()
{
	vsthn   += evig->sthn;
	tsthn   += evig->sthn;
	ssthn   += evig->sthn;

	vsth50  += evig->sth50;
	tsth50  += evig->sth50;
	ssth50  += evig->sth50;

	vsth100 += evig->sth100;
	tsth100 += evig->sth100;
	ssth100 += evig->sth100;

	vsth100f+= evig->sth100f;
	tsth100f+= evig->sth100f;
	ssth100f+= evig->sth100f;

	vsahn   += evig->sahn;
	tsahn   += evig->sahn;
	ssahn   += evig->sahn;

	vsah50  += evig->sah50;
	tsah50  += evig->sah50;
	ssah50  += evig->sah50;

	vsah100 += evig->sah100 + evig->sah100f;
	tsah100 += evig->sah100 + evig->sah100f;
	ssah100 += evig->sah100 + evig->sah100f;

	vsehn   += evig->sehn;
	tsehn   += evig->sehn;
	ssehn   += evig->sehn;

	vseh50  += evig->seh50;
	tseh50  += evig->seh50;
	sseh50  += evig->seh50;

	vseh100 += evig->seh100 + evig->seh100f;
	tseh100 += evig->seh100 + evig->seh100f;
	sseh100 += evig->seh100 + evig->seh100f;

	vsihn   += evig->sihn;
	tsihn   += evig->sihn;
	ssihn   += evig->sihn;

	vsih50  += evig->sih50;
	tsih50  += evig->sih50;
	ssih50  += evig->sih50;

	vsih100 += evig->sih100 + evig->sih100f;
	tsih100 += evig->sih100 + evig->sih100f;
	ssih100 += evig->sih100 + evig->sih100f;
}

void AcumularStd()
{
	long sreal;

	sreal = evig->sthn+evig->sth50+evig->sth100+evig->sth100f+
		        evig->sahn+evig->sah50+evig->sah100 + evig->sah100f+
		        evig->sehn+evig->seh50+evig->seh100 + evig->seh100f;

	vistd   += evig->sistd;
	tistd   += evig->sistd;
	sistd   += evig->sistd;

	virea   += sreal;
	tirea   += sreal;
	sirea   += sreal;

	vides   += evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f;
	tides   += evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f;
	sides   += evig->sihn + evig->sih50 + evig->sih100 + evig->sih100f;

}

// Imprime el total de un cliente en el archivo de salida
void TotLeg()
{
	if (!FmIFld(fm0, IMPSUB))
		return;

	if (*FmSFld(fm0, SALIDA) == 'A') {
	 	fprintf(fp, "Total Vigilador\t\t%7.7ld\t%20s\t\t\t\t\t\t\t\t\t\t", vigant, dvigant);
		fprintf(fp, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f",
					(double)vsthn / 100, (double)vsth50 / 100, (double)vsth100 / 100, (double)vsth100f / 100, 
					(double)vsahn / 100, (double)vsah50 / 100, (double)vsah100 / 100, 
					(double)vsehn / 100, (double)vseh50 / 100, (double)vseh100  / 100);

		#ifdef _NOVIA_VER_1_0
			fprintf(fp, "\n");
		#else
			fprintf(fp, "\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
					(double)vistd / 100, (double)virea / 100, (double)vidif /100 , (double)viemp/100,
					(double)vides / 100);

		#endif
	}
	else {
		RpSetLFld(rp0, RVIGIL,  vigant);
		RpSetFld (rp0, RDVIGIL, dvigant);
		DoReport(rp0, ZTOTVIG);
	}
}

void TotSem()
{
	if (!FmIFld(fm0, IMPSUB))
		return;

 #ifdef _NOVIA_VER_2_0
	DATE fdesde, fhasta;
	long sdif=0, semp=0;

	if (sirea < sistd) {
		sdif =  (sistd - sirea);
		semp =   sdif > sides ? (sdif - sides) : 0;
		vidif += sdif;
		tidif += sdif;
		viemp += semp;
		tiemp += semp;
	}

	SemanaDesdeHasta(fecant, &fdesde, &fhasta);

	if (*FmSFld(fm0, R_DETALLE) == _POR_LEGAJO)
		return;

	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "%ld\t%20s\tTOTAL DE LA SEMANA %.3D AL %.3D \t\t\t\t\t\t",vigant, dvigant, fdesde, fhasta);
		fprintf(fp, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
					(double)ssthn / 100, (double)ssth50 / 100, (double)ssth100 / 100, (double)ssth100f / 100, 
					(double)ssahn / 100, (double)ssah50 / 100, (double)ssah100 / 100, 
					(double)ssehn / 100, (double)sseh50  / 100,(double)sseh100 / 100,
					(double)sistd / 100, (double)sirea / 100, (double)sdif /100 , (double)semp/100,
					(double)(ssihn+ssih50+ssih100) / 100);
	}
	else {

		/* Genero el renglon con la diferencia semanal */
		if (sirea < sistd) {
			RpClearZone(rp0, ZNOPRINT);
			RpSetLFld(rp0, IDIF, sdif);
			RpSetLFld(rp0, IEMP, semp);
			DoReport(rp0, ZNOPRINT);  
		}

		RpSetDFld(rp0, RSEMD, fdesde);
		RpSetDFld(rp0, RSEMH, fhasta);
		DoReport (rp0, ZTOTSEM);
	}
 #endif
}

void TotGen()
{
	if (!FmIFld(fm0, IMPSUB))
		return;

	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total General \t\t\t\t\t\t\t\t\t\t\t\t\t");
		fprintf(fp, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f",
					(double)tsthn / 100, (double)tsth50 / 100, (double)tsth100 / 100, (double)tsth100f / 100, 
					(double)tsahn / 100, (double)tsah50 / 100, (double)tsah100 / 100, 
					(double)tsehn / 100, (double)tseh50 / 100, (double)tseh100  / 100);

		#ifdef _NOVIA_VER_1_0
			fprintf(fp, "\n");
		#else
			fprintf(fp, "\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
					(double)tistd / 100, (double)tirea / 100, (double)tidif /100 , (double)tiemp/100,
					(double)tides / 100);
		#endif
	}
	else
//	if (*FmSFld(fm0, SALIDA) != 'A')
		DoReport(rp0, ZTOTGEN);
}

// Imprime el titulo de Cliente en el archivo de salida
void  TitLeg()
{
	if (!FmIFld(fm0, IMPSUB))
		return;

	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "Legajo %7.7ld %20s\n", evig->nroleg, SFld(sue|PER_APYNOM));
	else {
//	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetLFld(rp0, RVIGIL,  evig->nroleg);
		RpSetFld (rp0, RDVIGIL, SFld(sue|PER_APYNOM));
		DoReport(rp0, ZVIG);
	}
}


