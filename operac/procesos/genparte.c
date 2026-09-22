/********************************************************************
*
* MODULE & VERSION : @(#)genparte.c	1.46 
* DATE             : 08/08/25 
* TIME             : 16:00:22 
*
* CREATED          : 17/09/98
*
* DESCRIPTION:
*      Generación del Parte Diario.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* MODIFICACION: 04/12/2000 Cambio el calculo de donde debe cubrir un franquero cuando el regimen es 4x2x12
*	Se modifico la funcion AsigHora
* 31/05/2002 - Se agrego la opcion de imprimir o grabar la generacion del parte. 
*
* MODIFICACION: 24/10/2003 Se creo GenParteH que es iguala GenParte pero lee ASIGH
*               No estaba regenerando el parte para los legajos que ya estaban en asigh.
*
* MODIFICACION: 20/01/2004 Funcion GetServicioObj. Se usa para filtrar los objetivos brigadas para que
*               no pida confirmacion de regeneracion del parte. Por eso se valida solo cuando se lee
*               asigh porque los brigadistas se insertan en obj brigadas y solo aparecen en asigh.
*               No se asignan en obj brigadas. El unico obj brigada con vigiladores asignados es el
*               cliente 1016 / 1 y si se debe validar.
*
*********************************************************************/
#include <ideafix.h>
#include "comgral.h"
#include "billpro.h"
#include "operac.h"
#include "comerc.h"
#include "opedef.h"
#include "ambiente.h"
#include "webinter.h"
#include "billpro.sch"
#include "operac.sch"
#include "asist.sch"
#include "genparte.fmh"
#include "genparte.rph"
#include "genpart2.rph"
#include "filial.h"

//#define DEBUG	1

#define WAR_PARTE_MODIF "El parte correspondiente a la Empresa: %d Cliente: %ld Objetivo: %d\nFecha: %.1D Puesto %d - %d Vigilador: %ld ya tiene horas cargadas.\nDesea regenerarlo?"
#define ERR_MEMORIA     "No hay más memoria!!!!!!!!!!!!!"
#define CANT_COL_DIAS	10		// cantidad de columnas para dias: capacidad del reporte.


static void AbrirArchLog();
struct s_lisxusr_lib esta_lis;

/* estructura para manejar a los vigiladores asignados en el 3000 1*/
typedef struct t_asig {
	long cliente;
	int  objetivo;
	int  ptoser;
	int  puesto;
	int  nroint;
	DATE fdesde;
	DATE fhasta;
	TIME hdesde;
	TIME hhasta;
	int  numfran;
	char efec[3];
	char dia1[2];
	char dia2[2];
	char dia3[2];
	char dia4[2];
	char dia5[2];
	char dia6[2];
	char dia7[2];
	char vigil[2];
	char regim[8];
	char regpto[8];
    short rrol, rfila, rcol; 
	struct t_asig *sgte;
}	n_asig;
typedef n_asig *p_asig;
p_asig lista = NULL;

typedef struct t_hs {
	TIME hdesde;
	TIME hhasta;
	bool todo;        //todo=1 abarca toda la hora; todo=0 no abarca nada de esa hora.
}hs;
struct t_hs horario[24];

// Definicion del Nodo para parte
typedef struct t_parte {
	int  ngrupdias;	   // numero de reporte, cada reporte tiene un max de CANT_COL_DIAS columnas
	int  emp;
	long cliente;
	int  objetivo;

	long nroleg;
	int  ptoser;
	int  puesto;
	char regimen[8];
	int  nroint;
	DATE dia[CANT_COL_DIAS];
	struct t_hs horas[CANT_COL_DIAS];
	struct t_parte *pi;
	struct t_parte *pd;
} n_parte;
typedef n_parte *p_parte;
p_parte baseparte=NULL;
               
/* Funciones privadas */
private void GenParte();
private void GenParteH();
private void LimpiarParte();

static fm_status after(form, fmfield, int);
static fm_status before(form, fmfield, int);
static p_asig ArmarLista(long nroleg, DATE fdesde, DATE fhasta);
static p_asig CrearNodo();
static void   FreeLista();

static bool BuscarHoras(DATE fecha, TIME horaent, TIME horasal, TIME *hsent, TIME *hhsal);
static void InicializarHorario(TIME horaent, TIME horasal);
static bool TodoCubierto();

static void NuevoHorario(TIME *hsent, TIME *hssal, TIME horaent, TIME horasal);
static bool DebeTrabajar(char *efectivo, int emp, long nroleg, int ptoser, int puesto, int nroint,DATE fecha, char *dia1,
						 char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, char *vigil,
						 long cliente, int objetivo,int numfran, char *regim);
static void HsPTime(long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint, DATE fecha,
					TIME *hsent, TIME *hssal);
static void DelParte(struct Asig *pasig);
static void GraboParte(struct Asig *pasig, bool fromasig);
// static void AsigHora(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
//					 DATE dia, TIME hsent, TIME hssal, TIME * hsentrada, TIME * hssalida);
static void AsigHora(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
					 DATE dia, TIME * hsentrada, TIME * hssalida, char *regim);
bool ClienteEspecial(long cliente, int objetivo, short *aviso, bool *ausentismo, char *condi, long nroleg, DATE p_fecha);
static void BorrarExcepciones(int emp, long cliente, int objetivo, DATE fecha, long nroleg, int ptoser,
                              int puesto, int nroint);


static void AbrirReporteParte();
static void CerrarReporteParte();

private void SetLinea(int emp, long nroleg, char *regimen, int ptoser, int puesto, struct t_hs  * hr, DATE * dia);
//private void SetLinea(int ngrupdias, long cliente, int objetivo, int emp, long nroleg, char *regimen, int ptoser, int puesto, struct t_hs  * hr, DATE * dia);

private void SetEncab(int ngrupdias, int emp, long cliente, int objet, DATE fecdesde);
private void PrintHead();

static p_parte CargoNodoParte(p_parte basei, int ngrupdias, int emp, long cliente, int  objetivo, long nroleg, int  ptoser, int  puesto, int  nroint, DATE dia, TIME hdesde, TIME hhasta, int columna);
static void PrintNodoParte(p_parte basei);
static void MuestroNodoParte(p_parte basei);
static void DeleteNodoParte(p_parte basei);
static int ObtColumnaRp(DATE dia, DATE diaini);
static int ObtNroGrupoDias(DATE dia, DATE diaini);
static short  GetCodigoAusentismoPeru(int p_emp, long p_cliente, int p_objetivo, DATE p_fecha);


/* Declaraciones globales */
schema   operac, asist, billpro;
form     fm0;
report rp = (report) ERROR;
dbcursor c_asig, c_asigah;
dbtable  ALASIG, ALASIGH, APARTE;
long     vec = 0L;
bool     conf_regen;
bool 	 soloimp=FALSE;
report rp2 = (report) ERROR;
FILE *archlog = (FILE *)NULL;
bool archOK = FALSE;
char filial[7] = {'\0'};
DATE fecierrefil;
char g_prog[20];

/* Programa principal */
wcmd(genparte, 1.46 08/25/08)
{
	fm_cmd cmd;

	sprintf(g_prog, "%s", argv[0]);


	fm0     = OpenForm("genparte", FM_EABORT);

	asist   = OpenSchema("asist",  IO_EABORT);
	operac  = OpenSchema("operac", IO_EABORT);
	billpro  = OpenSchema("billpro", IO_EABORT);

	if (argc !=	2)
		Error("El proceso debe ser invocado con 1 para confirmar la regeneración de partes confirmados\n o 0 para que no los tenga en cuenta."); 



	// Se controla que no se este ejectando el cierre, sino es asi  se permite ingresar al programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}

		
	ALASIG  = CreateAlias(operac|ASIG);
	ALASIGH = CreateAlias(operac|ASIGH);
	APARTE  = CreateAlias(operac|PARTE);
	c_asig  = CreateCursor(AlInd(ALASIG, operac|ASIGbyNROLEG), IO_NOT_LOCK);

	// 0: no va a pedir confirmación si tienen partes confirmados, directamente los saltearan
	// 1: confirma la regeneración de aquellos partes que estén ya confirmados
    // 2: solo impresion

	conf_regen 	= (StrToI(argv[1])==1? TRUE:FALSE); 
	soloimp 	= (StrToI(argv[1])==2? TRUE:FALSE); 
	
	FmSetIFld(fm0, I_SOLOIMP, soloimp); 

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) { 
	case FM_UPDATE: 

		// Se controla que no se este ejectando el cierre, sino es asi  se permite ejecutar el programa
		if (CierreActivo()) {
			WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
			continue;
		}

		AbrirArchLog();
		rp  = (report) ERROR;
        rp2 = (report) ERROR;

//		BeginTransaction();

		if(!soloimp)
        	LimpiarParte();

		GenParte();		// genera el parte o carga Nodo Parte para ASIG
		GenParteH();	// genera el parte o carga Nodo Parte para ASIGH

//		EndTransaction();

		if (rp != (report) ERROR) {
			CloseReport (rp);
			WiMsg ("Hubo errores al generar el parte. Se generó una impresión con los errores.");
		}

		if(baseparte!=NULL) {
			AbrirReporteParte();
			MuestroNodoParte(baseparte);
			CerrarReporteParte();
			DeleteNodoParte(baseparte);
			baseparte=NULL;
		}
		
		if (archOK == TRUE) {
 			fprintf(archlog, "Fin del Proceso %.3D - %.3T\n\n", Today(), Hour());
			archOK = FALSE;
			fclose(archlog);
		}	
		
		break;

	}
	DeleteCursor(c_asig);
	CloseAllSchemas();

	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();

}

private void LimpiarParte()
{
	struct Asig asig;
	dbcursor  c_asigh;
	char buffer[50];
	c_asigh = CreateCursor(operac|ASIGHbyFECHABAJ, IO_NOT_LOCK);

	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLIED))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIEH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIED) && esta_lis.objetivo < FmIFld(fm0, OBJETD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIEH) && esta_lis.objetivo > FmIFld(fm0, OBJETH))
	    	continue;

		SetCursorFrom(c_asigh, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MIN_DATE,
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MAX_DATE,
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (!IsNull(operac|ASIGH_MOTIVO)) {
				SetKey(operac|MOTIVDbyCODMOTD, IFld(operac|ASIGH_MOTIVO));
				if (GetRecord(operac|MOTIVDbyCODMOTD, THIS_KEY, IO_NOT_LOCK) == ERROR ||
												  IFld(operac|MOTIVD_M_INC) == FALSE)
					continue;
			}

			if (GetServicioObj(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)) == BRIGADA)
				continue;

			if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), NULL_STR, NULL_STR, FmSFld(fm0, FFILIAL)))
				continue;

			asig.emp      = IFld(operac|ASIGH_EMP);
			asig.cliente  = LFld(operac|ASIGH_CLIENTE);
			asig.objetivo = IFld(operac|ASIGH_OBJETIVO);
			asig.ptoser   = IFld(operac|ASIGH_PTOSER);
			asig.puesto   = IFld(operac|ASIGH_PUESTO);
			asig.nroint   = IFld(operac|ASIGH_NROINT);
			asig.nroleg   = LFld(operac|ASIGH_NROLEG);
			asig.fecasig  = DFld(operac|ASIGH_FECALT);
			asig.hsent    = TFld(operac|ASIGH_HSENT);
			asig.hssal    = TFld(operac|ASIGH_HSSAL);
			sprintf(asig.vigil, "%s", SFld(operac|ASIGH_VIGIL));
			sprintf(asig.efect, "%s", SFld(operac|ASIGH_EFECT));
			sprintf(asig.dia1,  "%s", SFld(operac|ASIGH_DIA1));
			sprintf(asig.dia2,  "%s", SFld(operac|ASIGH_DIA2));
			sprintf(asig.dia3,  "%s", SFld(operac|ASIGH_DIA3));
			sprintf(asig.dia4,  "%s", SFld(operac|ASIGH_DIA4));
			sprintf(asig.dia5,  "%s", SFld(operac|ASIGH_DIA5));
			sprintf(asig.dia6,  "%s", SFld(operac|ASIGH_DIA6));
			sprintf(asig.dia7,  "%s", SFld(operac|ASIGH_DIA7));
			asig.reempl  = LFld(operac|ASIGH_REEMPL);
			asig.ffranco = DFld(operac|ASIGH_FFRANCO);
			asig.numfran = IFld(operac|ASIGH_NUMFRAN);
			asig.francero= IFld(operac|ASIGH_FRANCERO);
			sprintf(asig.regim, "%s", SFld(operac|ASIGH_REGIM));
			sprintf(asig.regpto, "%s", SFld(operac|ASIGH_REGPTO));
			asig.fechas = DFld(operac|ASIGH_FECHAS);
			asig.fecbaj = DFld(operac|ASIGH_FECBAJ);
			asig.rrol  = IFld(operac|ASIGH_CODROL);
			asig.rfila = IFld(operac|ASIGH_FILA);
			asig.rcol  = IFld(operac|ASIGH_COLUM);
			if(asig.fechas == NULL_DATE) {
				asig.fechas = (asig.fecbaj < FmDFld(fm0, FHASTA) ? asig.fecbaj : FmDFld(fm0, FHASTA));
			}
			
			sprintf (buffer, "Procesando - Limpiando Parte - Cliente %ld Objetivo %d", asig.cliente, asig.objetivo);
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			
			DelParte(&asig);
			GraboParte(&asig, FALSE);
		}
	}

	DeleteCursor(c_asigh);
}   	

private void GenParte()
{
	struct Asig asig;
	dbcursor c_ASIG;
	char buffer[50];


	c_ASIG = CreateCursor(operac|ASIGbyFECHA, IO_NOT_LOCK);
	
	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLIED))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIEH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIED) && esta_lis.objetivo < FmIFld(fm0, OBJETD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIEH) && esta_lis.objetivo > FmIFld(fm0, OBJETH))
	    	continue;

		SetCursorFrom(c_ASIG, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MIN_DATE, MIN_LONG);
		SetCursorTo  (c_ASIG, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MAX_DATE, MAX_LONG);
		while (FetchCursor(c_ASIG) != ERROR) {
		    
			if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), NULL_STR, NULL_STR, FmSFld(fm0, FFILIAL)))
				continue;

		    strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));	

		    fecierrefil = GetFechaCierreFilial(filial);
		    
		            
			if (!soloimp)
		        if (fecierrefil != NULL_DATE && (FmDFld(fm0, FDESDE) <= fecierrefil || FmDFld(fm0, FHASTA) <= fecierrefil)) {
					Warning("El Parte está cerrado al %.3D para el Cliente %ld Objetivo %d Filial %s.\n Debe ingresar una fecha mayor al %.3D", 
							fecierrefil, LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), filial, fecierrefil);
					continue;
				}
            
            
			asig.emp      = IFld(operac|ASIG_EMP);
			asig.cliente  = LFld(operac|ASIG_CLIENTE);
			asig.objetivo = IFld(operac|ASIG_OBJETIVO);
			asig.ptoser   = IFld(operac|ASIG_PTOSER);
			asig.puesto   = IFld(operac|ASIG_PUESTO);
			asig.nroint   = IFld(operac|ASIG_NROINT);
			asig.nroleg   = LFld(operac|ASIG_NROLEG);
			sprintf(asig.vigil, "%s", SFld(operac|ASIG_VIGIL));
			sprintf(asig.efect, "%s", SFld(operac|ASIG_EFECT));
			asig.fecasig = DFld(operac|ASIG_FECASIG);
			asig.hsent   = TFld(operac|ASIG_HSENT);
			asig.hssal   = TFld(operac|ASIG_HSSAL);
			sprintf(asig.dia1, "%s", SFld(operac|ASIG_DIA1));
			sprintf(asig.dia2, "%s", SFld(operac|ASIG_DIA2));
			sprintf(asig.dia3, "%s", SFld(operac|ASIG_DIA3));
			sprintf(asig.dia4, "%s", SFld(operac|ASIG_DIA4));
			sprintf(asig.dia5, "%s", SFld(operac|ASIG_DIA5));
			sprintf(asig.dia6, "%s", SFld(operac|ASIG_DIA6));
			sprintf(asig.dia7, "%s", SFld(operac|ASIG_DIA7));
			asig.reempl   = LFld(operac|ASIG_REEMPL);
			asig.ffranco  = DFld(operac|ASIG_FFRANCO);
			//Lo saque porque sino tomaba el numfran del vig anterior. Entonces si el vig anterior tenia numfran=2 y este
			//estaba en null => tomaba numfan =2 y generaba cualquier franco.
			//		asig.numfran  = !IsNull(operac|ASIG_NUMFRAN) ? IFld(operac|ASIG_NUMFRAN) : asig.numfran;
			asig.numfran  = IFld(operac|ASIG_NUMFRAN);
			asig.francero = IFld(operac|ASIG_FRANCERO);
			sprintf(asig.regim, "%s", SFld(operac|ASIG_REGIM));
			sprintf(asig.regpto, "%s", SFld(operac|ASIG_REGPTO));
			asig.fechas = DFld(operac|ASIG_FECHAS);
			asig.fecbaj = DFld(operac|ASIG_FECBAJ);
			asig.rrol  = IFld(operac|ASIG_CODROL);
			asig.rfila = IFld(operac|ASIG_FILA);
			asig.rcol  = IFld(operac|ASIG_COLUM);
	        
			sprintf (buffer, "Procesando Asignación Actual - Cliente %ld Objetivo %d", asig.cliente, asig.objetivo);
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();
			
			fprintf(archlog, "%d\t%ld\t%d\n", asig.emp, asig.cliente, asig.objetivo);
			
			//		BeginTransaction();
		   	GraboParte(&asig, TRUE);
			//		EndTransaction();

		}
	}	
	
	DeleteCursor(c_ASIG);
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
}

private void GenParteH()
{
	struct Asig asig;
	dbcursor c_ASIGH;
	char buffer[50];
	DATE ffin;


	c_ASIGH = CreateCursor(operac|ASIGHbyEMP, IO_NOT_LOCK);
   
	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLIED))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIEH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIED) && esta_lis.objetivo < FmIFld(fm0, OBJETD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIEH) && esta_lis.objetivo > FmIFld(fm0, OBJETH))
	    	continue;

		SetCursorFrom(c_ASIGH, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MIN_SHORT,
						MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
		SetCursorTo  (c_ASIGH, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo,  MAX_SHORT,
						MAX_SHORT, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);

		while (FetchCursor(c_ASIGH) != ERROR) {
	
			if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), NULL_STR, NULL_STR, FmSFld(fm0, FFILIAL)))
				continue;

			if (GetServicioObj(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)) == BRIGADA)
				continue;

			sprintf (buffer, "Procesando Asignación Historica - Cliente %ld Objetivo %d", asig.cliente, asig.objetivo);
			FmSetFld (fm0, COMENT, buffer);
			WiRefresh();

			ffin = DFld(operac|ASIGH_FECBAJ) != NULL_DATE ? DFld(operac|ASIGH_FECBAJ) : DFld(operac|ASIGH_FECHAS);

			if (asig.fecasig > FmDFld(fm0, FHASTA)) continue;                  //--- Empieza despues ---//
			if (ffin != NULL_DATE && ffin < FmDFld(fm0, FDESDE)) continue;     //--- Ya termino     ---//

			if (DFld(operac|ASIGH_FECALT) > ffin) continue; //--- Mal asignado ---//
			if ( IFld(operac|ASIGH_MOTIVO) == DESXERROR ) continue; //--- Mal asignado ---// 

			
			strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)));	

		    fecierrefil = GetFechaCierreFilial(filial);
		    
        
			if (!soloimp)
		        if (fecierrefil != NULL_DATE && (FmDFld(fm0, FDESDE) <= fecierrefil || FmDFld(fm0, FHASTA) <= fecierrefil)) {
					Warning("El Parte está cerrado al %.3D para el Cliente %ld Objetivo %d Filial %s.\n Debe ingresar una fecha mayor al %.3D", 
							fecierrefil, LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), filial, fecierrefil);
					continue;
				}
			
			asig.emp      = IFld(operac|ASIGH_EMP);
			asig.cliente  = LFld(operac|ASIGH_CLIENTE);
			asig.objetivo = IFld(operac|ASIGH_OBJETIVO);
			asig.ptoser   = IFld(operac|ASIGH_PTOSER);
			asig.puesto   = IFld(operac|ASIGH_PUESTO);
			asig.nroint   = IFld(operac|ASIGH_NROINT);
			asig.nroleg   = LFld(operac|ASIGH_NROLEG);
			sprintf(asig.vigil, "%s", SFld(operac|ASIGH_VIGIL));
			sprintf(asig.efect, "%s", SFld(operac|ASIGH_EFECT));
			asig.fecasig = DFld(operac|ASIGH_FECALT);
			asig.hsent   = TFld(operac|ASIGH_HSENT);
			asig.hssal   = TFld(operac|ASIGH_HSSAL);
			sprintf(asig.dia1, "%s", SFld(operac|ASIGH_DIA1));
			sprintf(asig.dia2, "%s", SFld(operac|ASIGH_DIA2));
			sprintf(asig.dia3, "%s", SFld(operac|ASIGH_DIA3));
			sprintf(asig.dia4, "%s", SFld(operac|ASIGH_DIA4));
			sprintf(asig.dia5, "%s", SFld(operac|ASIGH_DIA5));
			sprintf(asig.dia6, "%s", SFld(operac|ASIGH_DIA6));
			sprintf(asig.dia7, "%s", SFld(operac|ASIGH_DIA7));
			asig.reempl   = LFld(operac|ASIGH_REEMPL);
			asig.ffranco  = DFld(operac|ASIGH_FFRANCO);
			//Lo saque porque sino tomaba el numfran del vig anterior. Entonces si el vig anterior tenia numfran=2 y este
			//estaba en null => tomaba numfan =2 y generaba cualquier franco.
			//		asig.numfran  = !IsNull(operac|ASIGH_NUMFRAN) ? IFld(operac|ASIGH_NUMFRAN) : asig.numfran;
			asig.numfran  = IFld(operac|ASIGH_NUMFRAN);
			asig.francero = IFld(operac|ASIGH_FRANCERO);
			sprintf(asig.regim, "%s", SFld(operac|ASIGH_REGIM));
			asig.fechas = DFld(operac|ASIGH_FECHAS);
			asig.fecbaj = DFld(operac|ASIGH_FECBAJ);
			asig.rrol  = IFld(operac|ASIGH_CODROL);
			asig.rfila = IFld(operac|ASIGH_FILA);
			asig.rcol  = IFld(operac|ASIGH_COLUM);

			fprintf(archlog, "%d\t%ld\t%d\n", asig.emp, asig.cliente, asig.objetivo);
            			
			//		BeginTransaction();
		   	GraboParte(&asig, TRUE);
			//		EndTransaction();

		}
	}
	
	DeleteCursor(c_ASIGH);
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
}

static p_asig ArmarLista(long nroleg, DATE fdesde, DATE fhasta)
{
	p_asig nuevonodo;

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		SetKey(AlInd(ALASIGH, operac|ASIGHbyNROLEG), FmIFld(fm0, EMP), nroleg, LFld(AlFld(ALASIG,
							  operac|ASIG_CLIENTE)), IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO)));
		(void)GetRecord(AlInd(ALASIGH, operac|ASIGHbyNROLEG), THIS_KEY, IO_NOT_LOCK);
		if (ExisteCliObjEnGrp(GRPRETPLANTA, LFld(AlFld(ALASIG, operac|ASIG_CLIENTE)), IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO)))) {
			continue;
		}
		if (!IsNull(AlFld(ALASIG, operac|ASIG_FECHAS)) &&
			DFld(AlFld(ALASIG, operac|ASIG_FECHAS)) < fdesde) {
			continue;
		}
		if (ExisteCliObjEnGrp(GRPRETPLANTA, LFld(AlFld(ALASIGH, operac|ASIG_CLIENTE)), IFld(AlFld(ALASIGH, operac|ASIG_OBJETIVO)))) {
			continue;
		}
		if (!IsNull(AlFld(ALASIGH, operac|ASIGH_FECHAS)) &&
			DFld(AlFld(ALASIGH, operac|ASIGH_FECHAS)) < fdesde) {
			continue;
		}
		nuevonodo       = CrearNodo();
		nuevonodo->sgte = lista;
		lista           = nuevonodo;
	}
	return lista;
}

static p_asig CrearNodo()
{
	p_asig aux;

	if ((aux = (p_asig) Alloc (sizeof(n_asig))) == NULL) {
		 WiMsg(ERR_MEMORIA);
		Stop(0);
	}
	aux->cliente  = LFld(AlFld(ALASIG, operac|ASIG_CLIENTE));
	aux->objetivo = IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO));
	aux->ptoser   = IFld(AlFld(ALASIG, operac|ASIG_PTOSER));
	aux->puesto   = IFld(AlFld(ALASIG, operac|ASIG_PUESTO));
	aux->nroint   = IFld(AlFld(ALASIG, operac|ASIG_NROINT));
	aux->fdesde   = DFld(AlFld(ALASIG, operac|ASIG_FECASIG));
	aux->fhasta   = (!StrCmp(SFld(AlFld(ALASIG, operac|ASIG_EFECT)), PROVISORIO)) ?
					 DFld(AlFld(ALASIG, operac|ASIG_FECHAS)) : MAX_DATE;
	aux->hdesde   = TFld(AlFld(ALASIG, operac|ASIG_HSENT));
	aux->hhasta   = TFld(AlFld(ALASIG, operac|ASIG_HSSAL));
	aux->numfran  = IFld(AlFld(ALASIG, operac|ASIG_NUMFRAN));
	strcpy(aux->efec,  SFld(AlFld(ALASIG, operac|ASIG_EFECT)));
	strcpy(aux->dia1,  SFld(AlFld(ALASIG, operac|ASIG_DIA1)));
	strcpy(aux->dia2,  SFld(AlFld(ALASIG, operac|ASIG_DIA2)));
	strcpy(aux->dia3,  SFld(AlFld(ALASIG, operac|ASIG_DIA3)));
	strcpy(aux->dia4,  SFld(AlFld(ALASIG, operac|ASIG_DIA4)));
	strcpy(aux->dia5,  SFld(AlFld(ALASIG, operac|ASIG_DIA5)));
	strcpy(aux->dia6,  SFld(AlFld(ALASIG, operac|ASIG_DIA6)));
	strcpy(aux->dia7,  SFld(AlFld(ALASIG, operac|ASIG_DIA7)));
	strcpy(aux->vigil, SFld(AlFld(ALASIG, operac|ASIG_VIGIL)));
	aux->rrol  = IFld(AlFld(ALASIG, operac|ASIG_CODROL));
	aux->rfila = IFld(AlFld(ALASIG, operac|ASIG_FILA));
	aux->rcol  = IFld(AlFld(ALASIG, operac|ASIG_COLUM));

	aux->sgte =	NULL;
	return aux;
}

static void FreeLista()
{
	p_asig recorre, aux;
	for (recorre = lista; recorre;) {
		aux     = recorre;
		recorre = recorre->sgte;
		Free(aux);
	}
	lista =	NULL;
}

static bool BuscarHoras(DATE fecha, TIME horaent, TIME horasal, TIME *hsent, TIME *hssal)
{
	p_asig aux;


	InicializarHorario(horaent, horasal);

	for (aux = lista ; aux != NULL; aux = aux->sgte) {
		if (aux->fdesde > fecha || fecha > aux->fhasta) {
			continue;
		}
		
		// valido si ese dia esta asignado a ese cli-obj ese dia
		// solamente si el puesto no es el efectivo!.
		if (!DebeTrabajar(aux->efec, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), aux->ptoser, aux->puesto,
						  aux->nroint, fecha, aux->dia1, aux->dia2, aux->dia3, aux->dia4, aux->dia5,
						  aux->dia6, aux->dia7, aux->vigil, aux->cliente, aux->objetivo,
						  IFld(operac|ASIG_NUMFRAN), aux->regim)) {
			continue;
		}
		if (TodoCubierto()) return FALSE;
		// el sgte es el caso de que el periodo asignado a un objetivo que no es
		// prosegur hace que el periodo asignado a prosegur se "parta" en dos. Esto
		// hace que este asig. a prosegur, luego a un objetivo y luego denuevo a prosegur
		// y si miramos la clave de PARTE, veremos que no puede estar asignado dos veces
		// al mismo objetivo el mismo dia, y por otro lado, no se puede guardar en un solo
		// registro dos periodos de hora desde - hora hasta.
		// ejemplo: 3000,1 = de 00 a 08 hs || 3005,27 = de 02 a 04 => no se puede
		// generar dos registros en PARTE uno con 3000,1 de 00 a 04 y otro con 3000,1
		// de 04 a 08!!!!! => por ahora se decidió tirar un WARNING....

	}
	if (TodoCubierto()) return FALSE;
	NuevoHorario(hsent, hssal, horaent, horasal);
	return TRUE;
}

static void InicializarHorario(TIME horaent, TIME horasal)
{
	int		horad, horah, i;
	char	aux[10];

	horad =	BusHora(horaent);
	horah =	BusHora(horasal);
	// Inicializo el vector de horarios.
	for (i = 0; i < 24; i++) {
		if ((horad <= horah && i >= horad && i <= horah) ||
			(horad >  horah && ((i >= horad && i <= 23)  || (i >= 0 && i <= horah)))) {
			sprintf(aux, "%02d00", i);
			horario[i].hdesde = (i == horad ) ? horaent : StrToT(aux);
			sprintf(aux, "%02d00", (i == 23) ? 0 : i + 1);
			horario[i].hhasta =	(i == horah) ? horasal : StrToT(aux);
			horario[i].todo	  =	TRUE;
			// el sgte if esta para la hora hasta. por ejemplo si
			// la hora hasta es la 00:00 hs que no genere en el horario[0]
			// nada porque es hasta las 00:00 hs. En cambio si es hasta las
			// 00:30 hs, si debe generar! (no entraria en el sgte if porque
			// la hora desde es distinta a la hasta: 00:00 hs. <> 00:30 hs.)
			if (horario[i].hdesde == horario[i].hhasta) {
				horario[i].hdesde =	NULL_TIME;
				horario[i].hhasta =	NULL_TIME;
				horario[i].todo	  =	FALSE;
			}
		}
		else {
			horario[i].hdesde =	NULL_TIME;
			horario[i].hhasta =	NULL_TIME;
			horario[i].todo	  =	FALSE;
		}
	}
}

static bool TodoCubierto()
{
	int	i;

	for (i = 0; i < 24; i++) {
		if (horario[i].todo == TRUE)
			return FALSE;
	}
	return TRUE;
}


static void NuevoHorario(TIME *hsent, TIME *hssal, TIME horaent, TIME horasal)
{
	int  horad, horah, i;
	TIME auxd = NULL_TIME, auxh = NULL_TIME;

	horad =	BusHora(horaent);
	horah =	BusHora(horasal);
	// para lo sgte se asume que horad != horah SIEMPRE!!!:
	for (i = horad; i != ((horah == 23) ? 0 : horah + 1); ((i==23) ? i=0 : i++)) {
		if (horario[i].todo == TRUE)
			break;
	}
	for (; i != ((horah == 23) ? 0 : horah + 1); ((i==23) ? i=0 : i++)) {
		if (!horario[i].todo)
			break;
		if (auxd == NULL_TIME) {
			auxd = horario[i].hdesde;
		}
		auxh = horario[i].hhasta;
	}
	(*hsent) = auxd;
	(*hssal) = auxh;
}

static bool DebeTrabajar(char *efectivo, int emp, long nroleg, int ptoser, int puesto, int nroint,
						 DATE fecha, char *dia1, char *dia2, char *dia3,
						 char *dia4, char *dia5, char *dia6, char *dia7, char *vigil, long cliente,
						 int objetivo, int numfran, char *regim)
{

	#ifdef _NOVIA_VER_2_0
	if (!CorrespondeDiaPuesto(emp, cliente, objetivo, ptoser, puesto, fecha)) {
		return FALSE;
	}
	#endif

	if (StrCmp(vigil, PARTTIME) == 0) {
		SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
		if (GetRecord(operac|DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
			return TRUE;
		else{
			SetKey(operac|DIASPTIMEHbyEMP, FmIFld(fm0, EMP), cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
			if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else {
		if ((!strcmp (regim, REG_ESP_1) || !strcmp (regim, REG_ESP_1_SAP) || !strcmp (regim, REG_ESP_3)) &&
			strcmp(efectivo, EFECTIVO) && Franco(emp, nroleg, fecha, vigil, numfran))
			return FALSE;

		if (((strcmp(efectivo, EFECTIVO)) ||
			(!strcmp(efectivo, EFECTIVO) && !Franco(emp, nroleg, fecha, vigil, numfran))) &&
			((dia(fecha) != *dia1) && (dia(fecha) != *dia2) && (dia(fecha) != *dia3) &&
			(dia(fecha)  != *dia4) && (dia(fecha) != *dia5) && (dia(fecha) != *dia6) &&
			(dia(fecha)  != *dia7)))
			return FALSE;
	}
	return TRUE;
}

static void GraboParte(struct Asig *pasig, bool fromasig)
{
	DATE fecha,	fechahasta, v_fecegr; 
	long nroleg	= NULL_LONG;
	TIME hsent,	hssal, hsentrada, hssalida;
	char condicion[5], v_condic='\0';
	bool especial, grabaus, v_preguntar=FALSE;
	short avisado;
	int  columna;

// si las asignación es despues del rango pedido por pantalla continue!
	if ((pasig->fecasig) > FmDFld(fm0, FHASTA)) {
		return;
	}

	// si las asignación termina antes del rango pedido por pantalla continue!
	if (pasig->fecbaj != NULL_DATE && pasig->fecbaj < FmDFld(fm0, FDESDE)) {
		return;
	}

	// Genero desde la fdesde del form solo si la fecha de asig es menor a la fdesde
	// de lo contrario, la asignación del empleado es posterior a la fdesde del form
	// con lo que los partes a generarse deben ser igual o mayor a la fecha de asig.
	fecha =	pasig->fecasig < FmDFld(fm0, FDESDE) ? FmDFld(fm0, FDESDE) : pasig->fecasig;

	// a los provisorios solo se le debe generar el parte hasta la fecha ASIG_FECBAJ.
	fechahasta = pasig->fecbaj != NULL_DATE ? ((FmDFld(fm0, FHASTA) > pasig->fecbaj) ?
				 pasig->fecbaj  : FmDFld(fm0, FHASTA)) : FmDFld(fm0, FHASTA); 

	if (nroleg != pasig->nroleg && ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
		if (nroleg != NULL_LONG)
			FreeLista();
		lista  = ArmarLista(pasig->nroleg, fecha, fechahasta);
		nroleg = pasig->nroleg;
	}

	// Tengo un cliente/objetivo, encontrar los dias en el rango fecha-fechahasta
	// Grabo uno por uno los registros o acumulo los dias para imprimir
	while (fecha <= fechahasta) {
		char valor[10];

		// valido si ese dia esta asignado a ese cli-obj ese dia
		// solamente si el puesto no es el efectivo!.

#if DEBUG
	fprintf(stderr,"nroleg %ld cli %ld obj %d ptoser %d pue %d fecha %.3D\n", 
		pasig->nroleg, pasig->cliente, pasig->objetivo, pasig->ptoser, pasig->puesto, fecha);
#endif
		if (!DebeTrabajar(pasig->efect, FmIFld(fm0, EMP), pasig->nroleg, pasig->ptoser, pasig->puesto,
						  pasig->nroint, fecha, 
						  pasig->dia1, pasig->dia2, pasig->dia3, pasig->dia4, pasig->dia5, 
						  pasig->dia6, pasig->dia7,
						  pasig->vigil, pasig->cliente,
						  pasig->objetivo, pasig->numfran, pasig->regim)) {
			fecha = fecha + 1;
			continue;
		}

		SetKey(operac|PARTEbyEMP, FmIFld(fm0, EMP), pasig->cliente,  pasig->objetivo,
				fecha,  pasig->nroleg ,  pasig->ptoser,  pasig->puesto, pasig->nroint);
		// esto lo que hace es avisar si el parte a regenerar ya fue modificado, si fue modificado,
		// pregunta si se lo quiere regenerar y actúa en función a la respuesta.
		if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (!fromasig) {
				fecha++;
				continue;
			}

			v_preguntar=FALSE;

			if (FFld(operac|PARTE_HSNOR)  != 0.00 || FFld(operac|PARTE_HS50)    != 0.00 ||
				FFld(operac|PARTE_HS100F) != 0.00 || FFld(operac|PARTE_HS100FE) != 0.00)
				v_preguntar=TRUE;

			if (TFld(operac|PARTE_HORAENT)  == StrToT("00:00") && TFld(operac|PARTE_HORASAL)  == StrToT("00:00") && *SFld(operac|PARTE_CONDIC) !='F')
				v_preguntar=TRUE;

			if (v_preguntar){
				if (conf_regen)  {
					if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, WAR_PARTE_MODIF,
						FmIFld(fm0, EMP), pasig->cliente, pasig->objetivo, fecha, pasig->ptoser,
						pasig->puesto, pasig->nroleg) == WD_NO) {
						fecha = fecha + 1;
						continue;
					}
				}
				else {
					fecha++;
					continue;
				}
			}


		}

		if (!PuestoVigente(fecha, pasig->cliente, pasig->objetivo, pasig->ptoser, pasig->puesto)) {
			fecha++;
			continue;
		}

		v_fecegr=GetEgresoLegajo(FmIFld(fm0, EMP), pasig->nroleg);
		if (v_fecegr!=NULL_DATE) {
			if (fecha>v_fecegr) {
				fecha++;
				continue;
			}
		}
		
		// Inicializo el registro. Si tenía horas cargadas serán borradas
		// ya que antes se dio la posibilidad al usu. de decidir esto.
		InitRecord(operac|PARTE);
		SetKey(operac|PARTEbyEMP, FmIFld(fm0, EMP), pasig->cliente, pasig->objetivo, fecha, pasig->nroleg,
													pasig->ptoser, pasig->puesto, pasig->nroint);
		SetFFld(operac|PARTE_HSNOR,   0.00);
		SetFFld(operac|PARTE_HS50,    0.00);
		SetFFld(operac|PARTE_HS100F,  0.00);
		SetFFld(operac|PARTE_HS100FE, 0.00);

		BorrarExcepciones(FmIFld(fm0, EMP), pasig->cliente, pasig->objetivo, fecha, pasig->nroleg,
						  pasig->ptoser, pasig->puesto, pasig->nroint);

		especial = ClienteEspecial(pasig->cliente, pasig->objetivo, &avisado, &grabaus, condicion, pasig->nroleg, DFld(operac|PARTE_DIA));

				
		// Si hay otra condicion Grabada Toma Esa y la Graba en el parte Generado
		v_condic= CondicParteOtroObjetivo(FmIFld(fm0, EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG));
		if (v_condic!='\0') 
			sprintf(condicion, "%c", v_condic);

		if (especial) {

			SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
			SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
			SetFld (operac|PARTE_CONDIC,  condicion);
			SetIFld(operac|PARTE_CODAUS,  avisado);
			SetDFld(operac|PARTE_FECGEN,  Today());
			
			if (soloimp) {                       
				columna = ObtColumnaRp(DFld(operac|PARTE_DIA), FmDFld(fm0,FDESDE));
#ifdef DEBUG
				fprintf(stderr, "Acumula Dia %3D Fecha %3D\n", DFld(operac|PARTE_DIA), fecha,);
#endif 

				baseparte=CargoNodoParte(baseparte,
					ObtNroGrupoDias(DFld(operac|PARTE_DIA), FmDFld(fm0,FDESDE)),
					IFld(operac|PARTE_EMP),
					LFld(operac|PARTE_CLIENTE), 
					LFld(operac|PARTE_OBJETIVO), 
					LFld(operac|PARTE_NROLEG), 
					IFld(operac|PARTE_PTOSER), 
					IFld(operac|PARTE_PUESTO),
					IFld(operac|PARTE_NROINT),  
					DFld(operac|PARTE_DIA),					
					TFld(operac|PARTE_HORAENT),
					TFld(operac|PARTE_HORASAL),
					columna
					);
			}
			else {
				AudiGrabaHorasParte(operac, g_prog);
			 	PutRecord(operac|PARTE);
				FreeTable(operac|PARTE);
                          
				if (grabaus && !TieneLic(FmIFld(fm0, EMP), pasig->nroleg, fecha)) {
					InitRecord(asist|ASISTEN);
					SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
					SetDFld(asist|ASISTEN_FECHA,  fecha);
					SetLFld(asist|ASISTEN_NROLEG, pasig->nroleg);
					SetIFld(asist|ASISTEN_CODNOV, IFld (operac|PARTE_CODAUS));
					SetLFld(asist|ASISTEN_VALOR,  100);
					SetIFld(asist|ASISTEN_JUSTIF, TRUE); /* seteo con TRUE para que en ASISTEN_JUSTIF sea un NO */
					PutRecord(asist|ASISTEN);
					FreeTable(asist|ASISTEN);
				}
			}
			fecha = fecha + 1;
			continue;
		}

		if (Vacaciones(FmIFld(fm0, EMP),  pasig->nroleg, fecha)) {
			SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
			SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
			SetFld (operac|PARTE_CONDIC,  _VACACIONES);
		}
		else {
			if ((ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)  ||
				 ExisteCliObjEnGrp(GRPTRAFRA,    pasig->cliente, pasig->objetivo)) &&
				(TieneLic(FmIFld(fm0, EMP), pasig->nroleg, fecha) ||
				 Falto(FmIFld(fm0, EMP),  pasig->nroleg, fecha))) {
				SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
				SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
				SetFld (operac|PARTE_CONDIC,  "A");
				SetIFld(operac|PARTE_CODAUS,  _CON_AVISO);
			}
			else {
				if (!soloimp && !TieneLic(FmIFld(fm0, EMP), pasig->nroleg, fecha) &&
						Falto(FmIFld(fm0, EMP), pasig->nroleg, fecha)) {
					SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
					SetDFld(asist|ASISTEN_FECHA,  fecha);
					SetLFld(asist|ASISTEN_NROLEG, pasig->nroleg);
					SetIFld(asist|ASISTEN_CODNOV, NULL_SHORT);
					while (GetRecord(asist|ASISTENbyEMPRE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
						DelRecord(asist|ASISTEN);
					}
			  	}

				if (Franco(FmIFld(fm0, EMP), pasig->nroleg, fecha, pasig->vigil, pasig->numfran)) {
					SetFld (operac|PARTE_CONDIC,  _FRANCO);

					if (ExisteCliObjEnGrp(GRPTRAFRA, pasig->cliente, pasig->objetivo)) {
						SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
						SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
					}
					else {
						if (!strcmp(pasig->efect,  EFECTIVO)) {
							SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
							SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
						}
						else {
							if (ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
								if (!BuscarHoras(fecha, pasig->hsent, pasig->hssal, &hsent, &hssal)) {
									fecha = fecha + 1;
									continue;
								}
								SetTFld(operac|PARTE_HORAENT, hsent == NULL_TIME ? StrToT("0000") : hsent);
								SetTFld(operac|PARTE_HORASAL, hssal == NULL_TIME ? StrToT("0000") : hssal);
							}
							else {
								if (StrCmp(pasig->vigil, PARTTIME) == 0 && StrCmp(pasig->dia1, "P") == 0) {
									HsPTime(pasig->cliente, pasig->objetivo, pasig->nroleg, pasig->ptoser,
											pasig->puesto, pasig->nroint, fecha, &hsent, &hssal);

									SetTFld(operac|PARTE_HORAENT, hsent);
									SetTFld(operac|PARTE_HORASAL, hssal);
								}
								else {
									SetTFld(operac|PARTE_HORAENT, pasig->hsent);
									SetTFld(operac|PARTE_HORASAL, pasig->hssal);
								}
							}
						}
					}
				}
				else {
					if (ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
						if (!BuscarHoras(fecha, pasig->hsent, pasig->hssal,	&hsent, &hssal)) {
							fecha = fecha + 1;
							continue;
						}
						SetTFld(operac|PARTE_HORAENT, hsent == NULL_TIME ? StrToT("0000") : hsent);
						SetTFld(operac|PARTE_HORASAL, hssal == NULL_TIME ? StrToT("0000") : hssal);
					}
					else {
						if (ExisteCliObjEnGrp(GRPTRAFRA, pasig->cliente, NULL_SHORT)) {
							SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
							SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
						}
						else {
							if (StrCmp(pasig->vigil, PARTTIME) == 0 && StrCmp(pasig->dia1, "P") == 0) {
								HsPTime(pasig->cliente, pasig->objetivo, pasig->nroleg, pasig->ptoser, pasig->puesto,
										pasig->nroint, fecha, &hsent, &hssal);
								SetTFld(operac|PARTE_HORAENT, hsent);
								SetTFld(operac|PARTE_HORASAL, hssal);
							}
							else {
								if (!StrCmp(pasig->regim, REG_16x12x12) &&
								   ((fecha == pasig->fecasig && pasig->ffranco - pasig->fecasig == GetDiasLaboral(pasig->regim, FALSE)) ||
								    Franco(FmIFld(fm0, EMP), pasig->nroleg, fecha + 1, pasig->vigil, pasig->numfran) ||
									Franco(FmIFld(fm0, EMP), pasig->nroleg, fecha - 1, pasig->vigil, pasig->numfran))) {
									SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
									SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
								}
								else {
									SetTFld(operac|PARTE_HORAENT, pasig->hsent);
									SetTFld(operac|PARTE_HORASAL, pasig->hssal);
								}
							}
						}
					}
					SetFld (operac|PARTE_CONDIC, _TRABAJA);
				}
			}
		}
		
		if (pasig->rrol != NULL_SHORT){

			/*Calcula el horario en base al rol que esta asignado el vigilador */
			GetHorasTurno (pasig->emp ,pasig->nroleg, pasig->cliente, pasig->objetivo, pasig->fecasig, pasig->rrol, pasig->rfila, pasig->rcol, 
						!str_eq(pasig->regpto, NULL_STR) ? pasig->regpto : pasig->regim,
						pasig->hsent, pasig->hssal, fecha, 
						pasig->ptoser, pasig->puesto,
						pasig->dia1, pasig->dia2, pasig->dia3, pasig->dia4, pasig->dia5,
						pasig->dia6, pasig->dia7, strcmp(pasig->efect,  EFECTIVO)==0, valor, pasig->vigil, pasig->numfran,
						&hsentrada, &hssalida);

#if DEBUG
			fprintf(stderr, "GetHsTurno %.3D %d %d %d %.3T %.3T %.3D \n", pasig->fecasig, pasig->rrol, pasig->rfila, pasig->rcol,  hsentrada, hssalida, fecha);
#endif

			if (str_eq(valor, _NO_TRABAJA)) {
				fecha++;
				continue;

			}
			SetTFld(operac|PARTE_HORAENT,  hsentrada);
			SetTFld(operac|PARTE_HORASAL,  hssalida); 
		}
		else {
			if ((!StrCmp(pasig->regim, REG_ESP) || !StrCmp(pasig->regim, REG_ESP_2) || !StrCmp(pasig->regim, REG_ESP_3)) &&
				str_eq (SFld(operac|PARTE_CONDIC), _TRABAJA) && pasig->francero == TRUE) {
				AsigHora(pasig->emp, pasig->cliente, pasig->objetivo, pasig->nroleg, pasig->ptoser,
						pasig->puesto, pasig->nroint, fecha, &hsentrada, &hssalida, pasig->regim);
				SetTFld(operac|PARTE_HORAENT,  hsentrada);
				SetTFld(operac|PARTE_HORASAL,  hssalida); 
#if DEBUG
				fprintf(stderr, "AsigHora %.3D %d %d %d %.3T %.3T %.3D \n", pasig->fecasig, pasig->rrol, pasig->rfila, pasig->rcol,  hsentrada, hssalida, fecha);
#endif
			}
		}
		SetDFld(operac|PARTE_FECGEN, Today());
		
		if (ExisteCliObjEnGrp(GRPHORACERO, LFld(operac|PARTE_CLIENTE), LFld(operac|PARTE_OBJETIVO))) {
			SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
			SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
		}

		if (!especial)
			if (v_condic!='\0')
				SetFld (operac|PARTE_CONDIC,  condicion);
		


		if (soloimp) {
			columna = ObtColumnaRp(DFld(operac|PARTE_DIA), FmDFld(fm0,FDESDE));
			baseparte=CargoNodoParte(baseparte,
					ObtNroGrupoDias(DFld(operac|PARTE_DIA), FmDFld(fm0,FDESDE)),
					IFld(operac|PARTE_EMP),
					LFld(operac|PARTE_CLIENTE), 
					LFld(operac|PARTE_OBJETIVO), 
					LFld(operac|PARTE_NROLEG), 
					IFld(operac|PARTE_PTOSER), 
					IFld(operac|PARTE_PUESTO), 
					IFld(operac|PARTE_NROINT), 
					DFld(operac|PARTE_DIA),
					TFld(operac|PARTE_HORAENT),
					TFld(operac|PARTE_HORASAL),
					columna
					);
		}
		else {
			AudiGrabaHorasParte(operac, g_prog);
			PutRecord(operac|PARTE);
		}		
		
		FreeTable(operac|PARTE);
		
		fecha = fecha + 1;
	}
}

static void HsPTime(long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint, DATE fecha,
					TIME *hsent, TIME *hssal)
{
	SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
	if (GetRecord(operac|DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		*hsent = TFld(operac|DIASPTIME_HENT);
		*hssal = TFld(operac|DIASPTIME_HSAL);
	}
	else {
		SetKey(operac|DIASPTIMEHbyEMP, FmIFld(fm0, EMP), cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
		if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			*hsent = TFld(operac|DIASPTIMEH_HENT);
			*hssal = TFld(operac|DIASPTIMEH_HSAL);
		}
	}
}
static void DelParte(struct Asig *pasig)
{

	DATE fechahasta;
	static dbcursor c_parte=(dbcursor)NULL;
	bool v_preguntar=FALSE;

	if (c_parte == (dbcursor)NULL) {
		c_parte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
	}

	if (pasig->fecbaj < FmDFld(fm0, FDESDE) || pasig->fecbaj > FmDFld(fm0, FHASTA))
		return;

	fechahasta = pasig->fechas == NULL_DATE ? MAX_DATE : pasig->fechas;
	SetCursorFrom(c_parte, pasig->emp, pasig->cliente, pasig->objetivo,
					pasig->fecasig > FmDFld(fm0, FDESDE) ? pasig->fecasig : FmDFld(fm0, FDESDE), // f.desde
					MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_parte, pasig->emp, pasig->cliente, pasig->objetivo,	fechahasta, MAX_LONG, MAX_SHORT,
					MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (pasig->nroleg != LFld(operac|PARTE_NROLEG)) {
			continue;
		}
		// solo debe borrar los partes que corresponden al ASIGH.
		if (pasig->puesto != IFld(operac|PARTE_PUESTO) || pasig->ptoser != IFld(operac|PARTE_PTOSER) ||
			pasig->nroint != IFld(operac|PARTE_NROINT))
			continue;


		v_preguntar=FALSE;

		if (FFld(operac|PARTE_HSNOR)  != 0.00 || FFld(operac|PARTE_HS50)    != 0.00 ||
			FFld(operac|PARTE_HS100F) != 0.00 || FFld(operac|PARTE_HS100FE) != 0.00)
				v_preguntar=TRUE;

		if (TFld(operac|PARTE_HORAENT)  == StrToT("00:00") && TFld(operac|PARTE_HORASAL)  == StrToT("00:00") && *SFld(operac|PARTE_CONDIC) !='F')
			v_preguntar=TRUE;

		if (*SFld(operac|PARTE_CONDIC) !='A')
			continue;

		if (v_preguntar){
			if (conf_regen) {
				if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, WAR_PARTE_MODIF, FmIFld(fm0, EMP),
					pasig->cliente, pasig->objetivo, DFld(operac|PARTE_DIA), IFld(operac|PARTE_PTOSER),
					IFld(operac|PARTE_PUESTO), pasig->nroleg) == WD_NO) {
					continue;
				}
			}
			else
				continue;
		}
		BorrarExcepciones(FmIFld(fm0, EMP), pasig->cliente, pasig->objetivo, DFld(operac|PARTE_DIA),
						  pasig->nroleg, pasig->ptoser, pasig->puesto, pasig->nroint);
		DelRecord(operac|PARTE);
		FreeTable(operac|PARTE);
	}
}

bool ClienteEspecial (long cliente, int objetivo, short *aviso, bool *ausentismo, char *condi, long nroleg, DATE p_fecha)
{
	int salida = 0;

	*aviso = NULL_SHORT;
	*ausentismo = FALSE;
	strcpy(condi, NULL_STR);

	*aviso= GetCodigoAusentismoPeru(FmIFld(fm0, EMP), cliente, objetivo, p_fecha);
	if (*aviso==NULL_SHORT)
		salida++;

	if (ExisteCliObjEnGrp(GRPGRABAINS, cliente, objetivo)) {
		*ausentismo = TRUE;
	}
	else {
		if (ExisteCliObjEnGrp(GRPNOGRABAINS, cliente, objetivo)) {
			*ausentismo = FALSE;
		}
		else
			salida++;
	}

	if (ExisteCliObjEnGrp(GRPAUSENTE, cliente, objetivo)) {
		strcpy (condi, _AUSENTE);
	}
	else {
		if (ExisteCliObjEnGrp(GRPTRABAJA, cliente, objetivo)) {
			strcpy (condi, _TRABAJA);
		}
		else
			salida++;
	}
	if (ExisteCliObjEnGrp(GRPGENAUS, cliente, objetivo)) {
		strcpy (condi, _AUSENTE);
	}
	else
		salida ++;

	if (ExisteCliObjEnGrp(GRPGENT00, cliente, objetivo)) {
		strcpy (condi, _TRABAJA);
	}
	else
		salida ++;



	if (salida == 5)
		return FALSE;
	else
		return TRUE;
}


static void AsigHora(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
					 DATE dia, TIME * hsentrada, TIME * hssalida, char *regim)
{
	/********************************************************************************************
	Esta funcion devuelve la hora de entrada y salida 
	Solo se puede usar para un franquero (que rota el horario a cubrir)
	Solo se puede usar para un regimen 4x2x12 (donde rota el horario cada 2 dias -cantdiasP-) o
	para un regimen  8x4x12 (donde rota el horario cada 4 dias -cantdiasP-).
	********************************************************************************************/

	static dbtable AASIG, AASIGH;
	short diaslab, dias;
	DATE fecha=NULL_DATE, fechaA;
	bool impre = FALSE, hay_otros = FALSE;
	short cantdias=0, cantdiasP=0, diasarecorrer = 0;
	TIME hantent, hantsal;
	static dbcursor c_Asig=ERROR, c_Asigh=ERROR;

	if (c_Asig==ERROR)
		c_Asig = CreateCursor(AlInd(ALASIG,operac|ASIGbyPUESTO), IO_NOT_LOCK);

	if (c_Asigh==ERROR)
		c_Asigh = CreateCursor(AlInd(ALASIGH, operac|ASIGHbyEMP), IO_NOT_LOCK);


	if (strcmp(regim, REG_ESP_3)==0)
		cantdiasP = (int)GetDiasLaboral(regim, FALSE)/2;

	else 
		cantdiasP = GetDiasFranco(regim, FALSE);

	
	if (impre) fprintf (stderr, "GetDiasLaboral %d GetDiasFranco %d\n", GetDiasLaboral(regim, FALSE), GetDiasFranco(regim, FALSE));


	diasarecorrer =  GetDiasLaboral(regim, FALSE) + GetDiasFranco(regim, FALSE);

	*hsentrada = StrToT("0000");
	*hssalida  = StrToT("0000");

    if (impre) fprintf (stderr, "\n\nASIG HORA %d %ld %d %ld dia %.3D \n", emp, cliente, objetivo, nroleg, dia);

	if (!AASIG) {
		AASIG  = CreateAlias (operac|ASIG);
		AASIGH = CreateAlias (operac|ASIGH);
	}

	/*Leo la asignacion */
	SetKey (AASIGbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint);
	if (GetRecord (AASIGbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR || DFld (AASIG_FECASIG) > dia ) {
		bool encontro=FALSE;
		
		SetKey (AASIGHbyEMP, emp, cliente, objetivo, ptoser, puesto, nroint, nroleg, MIN_DATE, MIN_DATE);
		while (!encontro && GetRecord (AASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			if (DFld (AASIGH_FECALT) <= dia && DFld (AASIGH_FECBAJ) >= dia) {
				encontro = TRUE;
			} 
		}

		if (!encontro) {
			if (impre) fprintf (stderr, "No encontro puesto \n");
			return;
		} 
		
		CopyFld (AASIGH_NROLEG,  AASIG_NROLEG);
		CopyFld (AASIGH_VIGIL ,  AASIG_VIGIL);
		CopyFld (AASIGH_NUMFRAN, AASIG_NUMFRAN);
		CopyFld (AASIGH_FECALT,  AASIG_FECASIG);
		CopyFld (AASIGH_HSENT,   AASIG_HSENT);
		CopyFld (AASIGH_HSSAL,   AASIG_HSSAL);
		CopyFld (AASIGH_FFRANCO, AASIG_FFRANCO);
		CopyFld (AASIGH_REGIM,   AASIG_REGIM);
	} 

	/*Si tiene franco o vacaciones no calculo nada */
	if (Franco (emp, LFld (AASIG_NROLEG), dia, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
		if (impre) fprintf (stderr, "%.3D %ld Esta de franco FIN\n", dia, nroleg);
		return;
	} 

	if (Vacaciones(emp, nroleg, fecha)) {
		if (impre) fprintf (stderr, "%.3D %ld Esta de vacaciones FIN\n", dia, nroleg);
		return;
	}

	/*Si pido la fecha de asignacion y NO es franco  la hora es la ingresada en la asignacion */
	if (dia == DFld (AASIG_FECASIG) && 
		!Franco (emp, LFld (AASIG_NROLEG), dia, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
			*hsentrada = TFld (AASIG_HSENT);
			*hssalida  = TFld (AASIG_HSSAL);
			if (impre) fprintf (stderr, "FIN Es primer dia devuelve %T %T \n", *hsentrada, *hssalida);
	}

	if (dia != DFld (AASIG_FECASIG)) {

		/********************************************************************************************
		Calculo el dia posterior al de la asignacion a que hora tiene que trabajar
		Para esto reconstruyo apartir de lo que seria su franco anterior (Fecha de franco - dias laborables)
		En base a esa fecha puedo saber si el dia posterior al de asignacion tiene que seguir cumpliendo
		el mismo horario o si tiene que cambiar.
		*********************************************************************************************/
		diaslab = GetDiasLaboral(SFld (AASIG_REGIM), FALSE);
		diaslab = diaslab - 1 + IFld (AASIG_NUMFRAN);
		for (fechaA= DFld (AASIG_FFRANCO), dias=0; dias < diaslab; fechaA --, dias ++); 
		if (impre) fprintf (stderr, "El primer dia laborable es %.3D \n", fechaA);

		for (fecha=fechaA; fecha <= (DFld (AASIG_FECASIG)+1); fecha ++) {
			if (impre) fprintf (stderr, "for fecha %.3D asig %.3D \n", fecha, DFld (AASIG_FECASIG)+1);
			if (cantdias < cantdiasP) {
				cantdias ++;
				if (impre) fprintf (stderr, "dentro del if fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (AASIG_FECASIG)+1, cantdias, cantdiasP);
			} 
			else {
				cantdias = 1;
				if (impre) fprintf (stderr, "dentro del else fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (AASIG_FECASIG)+1, cantdias, cantdiasP);
			}
		}

		if (Franco (emp, LFld (AASIG_NROLEG), DFld (AASIG_FECASIG) , SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
			/*Si el dia de asignacion esta de franco la hora anterior es la inversa de la primer asignacion */
			//Inversa
			hantent= TFld (AASIG_HSSAL);
			hantsal= TFld (AASIG_HSENT);
			if (impre) fprintf (stderr, "Franco hantent %T hantsal %T \n", hantent,hantsal);
		}
		else {
			hantent= TFld (AASIG_HSENT);
			hantsal= TFld (AASIG_HSSAL);
			if (impre) fprintf (stderr, "No Franco hantent %T hantsal %T \n", hantent,hantsal);
		}

		if (impre) fprintf (stderr, "El dia posterior a la fecha de asig %.3D cantdias %d horant %T %T \n", DFld (AASIG_FECASIG)+1, cantdias, hantent, hantsal);

		if (strcmp(regim, REG_ESP_3)==0)
			cantdias = 2;

		/***************************************************************
		Busco para la fecha pedida a que hora trabaja
		Rota el horario con respecto a la hora anterior cada dos dias (cantdiasP)
		***************************************************************/

		for (fecha=DFld (AASIG_FECASIG)+1 ; fecha <= dia; fecha ++) {
			short periodo;
		
			if (impre) fprintf (stderr, "principio de for Dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T\n", fecha, cantdias, cantdiasP, *hsentrada, *hssalida,hantent,hantsal);
			/*********************************************************
			Lo que sigue se agrego para agilizar la funcion.
			Porque si el vigilador se asigno hace mucho recorría dia por dia.
			Como la secuencia de rotacion es la misma cada 6 dias puedo avanzar hasta un multiplo.
			De esta forma recorro a lo sumo solo 6 dias.
			**********************************************************/
			/*********************************************************
			Al agregarse el regimen 8x4x12 los dias a recorrer pueden ser 12 por ende puse la 
			variable diasarecorrer. GAG
			**********************************************************/
			periodo = (dia-fecha) / diasarecorrer;
			
			if (periodo > 0) {
				if (impre) fprintf (stderr, "Dia %.3D Fecha %.3D periodo %d ", dia, fecha, periodo);
				fecha = fecha + (periodo * diasarecorrer);
				if (impre) fprintf (stderr, "Nueva fecha %.3D  \n", fecha);
			}
			
			if (Franco (emp, LFld (AASIG_NROLEG), fecha, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				cantdias=1;
				if (impre) fprintf (stderr, "Dia %.3D Franco \n", fecha);
				continue;
			} 	

			if (impre) fprintf(stderr, "fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
			if (cantdias < cantdiasP) {
				*hsentrada = hantsal;
				*hssalida  = hantent;
				if (impre) fprintf(stderr,"dentro del if fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d \n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias ++;
			}
			else {
				*hsentrada = hantent;
				*hssalida  = hantsal;
				if (impre) fprintf(stderr,"dentro del else fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias = 1;					
			}
			if (cantdias == cantdiasP) {
				hantent= *hsentrada;
				hantsal= *hssalida;
			}
			if (impre) fprintf(stderr,"al final fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);

			if (Vacaciones(emp, nroleg, fecha)) {
				/*No se modifica cantidad de dias la rotacion es como si estuviera trabajando */
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				if (impre) fprintf (stderr, "Dia %.3D Vacaciones \n", fecha);
			}
		} 
		if (impre) fprintf(stderr,"fin del for fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
	}

	if (*hsentrada == StrToT ("0000") && *hssalida == StrToT ("0000"))
		return;

	if (*hsentrada == StrToT ("2359"))
		*hsentrada = StrToT ("0000");

	/*Controlo que haya alguien a quien cubrir */
	SetCursorFrom(c_Asig, emp, cliente, objetivo, ptoser, puesto, MIN_SHORT, MIN_LONG);
	SetCursorTo  (c_Asig, emp, cliente, objetivo, ptoser, puesto, MAX_SHORT, MAX_LONG);
	while (!hay_otros && FetchCursor(c_Asig) != ERROR) {
		if (!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP) &&
			!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP_2)&&
			!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP_3)) {
			continue;
		}
		if (Vacaciones(emp, nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}

		if (Franco(emp, LFld(AlFld(ALASIG, operac|ASIG_NROLEG)), dia, SFld(AlFld(ALASIG, operac|ASIG_VIGIL)),
						IFld(AlFld(ALASIG, operac|ASIG_NUMFRAN)))) {
			if (*hsentrada == TFld(AlFld(ALASIG, operac|ASIG_HSENT))) {
				hay_otros = TRUE;
				break;
			}
		}
	}
//	DeleteCursor(c_Asig);

	SetCursorFrom(c_Asigh, emp, cliente, objetivo, ptoser, puesto, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
	SetCursorTo  (c_Asigh, emp, cliente, objetivo, ptoser, puesto, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
	while (!hay_otros && FetchCursor(c_Asigh) != ERROR) {
		if (!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP) &&
			!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP_2) &&
			!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP_3))
			continue;

		if (Vacaciones(emp, nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}
		if (Franco(emp, LFld(AlFld(ALASIGH, operac|ASIGH_NROLEG)), dia,
			SFld(AlFld(ALASIGH, operac|ASIGH_VIGIL)), IFld(AlFld(ALASIGH, operac|ASIGH_NUMFRAN)))) {
			if (*hsentrada == TFld(AlFld(ALASIGH, operac|ASIGH_HSENT))) {
				hay_otros  = TRUE;
			}
		}
	}            
//	DeleteCursor(c_Asigh);

	if (!hay_otros) {
		char mensaje[200];
		if (rp == (report) ERROR)
			rp = OpenReport("genparte", RP_EABORT);
		
		sprintf (mensaje, "El Vigilador %ld es Franquero y no hay asignados para el cliente %ld %d otros Vigiladores a los que tenga que cubrir el día %.1D",
																nroleg, cliente, objetivo, dia);
		RpSetFld (rp, RMENSAJE, mensaje);
		DoReport (rp, ZLINEA);
	}
}

static void AbrirReporteParte () 
{
	rp2 = OpenReport("genpart2", RP_EABORT|RP_NOBEGIN);
	RpSetOutput(rp2, *FmSFld(fm0, SALIDA)=='I'? RP_IO_DEFAULT:RP_IO_TERM, NULL_STR);
	BeginReport(rp2, 1, NULL_STR);
	PrintHead();
}


private void PrintHead()
{


	long clied=FmLFld(fm0, CLIED);
	long clieh=FmLFld(fm0, CLIEH);
	int objetd=FmIFld(fm0, OBJETD);
	int objeth=FmIFld(fm0, OBJETH);
	
	RpSetLFld(rp2, RCLIED,   clied);
	RpSetFld (rp2, RDCLIED,  GetDescCli(clied));
	RpSetLFld(rp2, RCLIEH,   clieh);
	RpSetFld (rp2, RDCLIEH,  GetDescCli(clieh));
	
	
	RpSetIFld(rp2, ROBJETD,  objetd);
	RpSetFld (rp2, RDOBJETD, GetObjDescrip(clied, objetd));
	RpSetIFld(rp2, ROBJETH,  objeth);
	RpSetFld (rp2, RDOBJETH, GetObjDescrip(clieh, objeth));
	
	RpSetDFld(rp2, RFDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp2, RFHASTA, FmDFld(fm0, FHASTA));
}

private void SetLinea(int emp, long nroleg, char *regimen, int ptoser, int puesto, struct t_hs  * hr, DATE * dia)
//int ngrupdias, long cliente, int objetivo, 
{
	int i=0;
	struct t_hs v_horario;
	RpSetLFld(rp2, R_LEGAJO,  nroleg);
	RpSetFld (rp2, R_APENOM,  GetNombreLeg(emp, nroleg));
	RpSetFld (rp2, R_REGIM,   regimen);
	RpSetIFld(rp2, R_PTOSER,  ptoser);
	RpSetIFld(rp2, R_PUESTO,  puesto);
	RpSetFld (rp2, R_DPTOSER, GetDescPto(ptoser) );

	for(i=0; i<CANT_COL_DIAS; i++) {

		v_horario = hr[i];
		
		RpSetTFld(rp2, R_HSD1+(2*i), v_horario.hdesde);
		RpSetTFld(rp2, R_HSH1+(2*i), v_horario.hhasta);
                                 
#ifdef DEBUG
fprintf(stderr,"%ld %s %d %d (%.3D) %T %T\n", nroleg, regimen, ptoser, puesto, dia[i], v_horario.hdesde, v_horario.hhasta);
#endif

	}

	DoReport(rp2, LINEA); 
}

private void SetEncab(int ngrupdias, int emp, long cliente, int objet, DATE fecdesde)
{
	DATE fecha, fechasta;
	int i;

	fechasta=fecdesde+CANT_COL_DIAS-1;

	RpSetIFld(rp2, RNGRUP, ngrupdias+1);
	RpSetDFld(rp2, RNGDES, fecdesde);
	RpSetDFld(rp2, RNGHAS, fechasta<FmDFld(fm0,FHASTA)? fechasta : FmDFld(fm0,FHASTA) );
	
	RpSetLFld(rp2, RCLI, 	cliente);
	RpSetFld (rp2, RDCLI,  	GetDescCli(cliente));
	RpSetIFld(rp2, ROBJ, 	objet);
	RpSetFld (rp2, RDOBJ, 	GetObjDescrip(cliente, objet));

#ifdef DEBUG
fprintf(stderr,"SetEnca (%.3D %.3D)\n", fecdesde, FmDFld(fm0,FHASTA) );
#endif

  	for (i=0; i<CANT_COL_DIAS; i++) {
		RpSetDFld(rp2, RDIA1+(1*i), NULL_DATE);
		RpSetLFld(rp2, RSTD1+(1*i), NULL_LONG);
	}  
  	
  	fecha = fecdesde;
  	for (i=0; fecha<=FmDFld(fm0,FHASTA) && i<CANT_COL_DIAS; i++) {

#ifdef DEBUG 
fprintf(stderr,"SetEnca (%.3D %.3D) en %.3D\n", fecdesde, FmDFld(fm0,FHASTA), fecha );
#endif 

		RpSetDFld(rp2, RDIA1+(1*i), fecha);
		if (cliente == 3763 && objet == 2 && (fecha == DMYToD(16,02,2000) || fecha == DMYToD(17,02,2000)))
			RpSetLFld(rp2, RSTD1+(1*i), NULL_LONG);
		else 
			RpSetLFld(rp2, RSTD1+(1*i), StdHr(emp, cliente, objet, NULL_SHORT, fecha, fecha));
		fecha++;
	} 
}

/*****************************************************************************************
* Funciones para el manejo del Nodo Parte:
*	 CargoNodoParte
*	 PrintNodoParte
*	 MuestroNodoParte
*	 DeleteNodoParte
*****************************************************************************************/
/*
ngrupdias, el el nro. de agrupacion de dias
columna, indica el nro de columna de la agrupacion ngrupdias, donde cargar los datos: dia, hr entrada y hora salida
*/                                     
static p_parte CargoNodoParte(p_parte basei, int ngrupdias, int emp, long cliente, int  objetivo, long nroleg, int  ptoser, int  puesto, int  nroint, DATE dia, TIME hdesde, TIME hhasta, int columna)
{
	p_parte aux;     
	int i;
	char regimen[15];
	struct t_hs v_horario;
	
	if (basei==NULL) {
		aux=(p_parte)malloc(sizeof(n_parte));
		if (aux==NULL) {
			WiMsg(" NO HAY SUFICIENTE MEMORIA DISPONIBLE  \n LLAME CON URGENCIA A SISTEMAS");
			return NULL;
		}
		// La inicializacion de un nodo, ocurre cuando cambia ngrupdias,
		// por lo tanto se deben inicializar los vectores en nulo
		aux->pi=NULL;
		aux->pd=NULL;

		aux->ngrupdias= ngrupdias;
		aux->emp	  = emp;
		aux->cliente  = cliente;
		aux->objetivo = objetivo;
		aux->nroleg	  = nroleg;
		aux->ptoser   = ptoser;
		aux->puesto   = puesto;
		GetRegimenEfectivo(emp, nroleg, regimen, dia);
		strcpy(aux->regimen, regimen);
		aux->nroint   = nroint;
	
		for (i=0; i<CANT_COL_DIAS; i++) {	// inicializacion de los vectores
			aux->dia[i] = NULL_DATE;
			v_horario.hdesde=NULL_TIME;
			v_horario.hhasta=NULL_TIME;
			aux->horas[i]=v_horario;
		}
		
		aux->dia[columna] = dia;
		v_horario.hdesde=hdesde;
		v_horario.hhasta=hhasta;
		aux->horas[columna]=v_horario;
		
		return aux;
	}
	
	// Acumular por ngrupdias, legajo, cliente, objetivo
	if ( basei->ngrupdias==ngrupdias && basei->emp==emp && basei->cliente==cliente && 
			basei->objetivo==objetivo && basei->nroleg==nroleg && basei->ptoser==ptoser && 
			basei->puesto==puesto && basei->nroint==nroint) {
        
        basei->dia[columna] = dia;
		v_horario.hdesde=hdesde;
		v_horario.hhasta=hhasta;		
		basei->horas[columna]=v_horario;

		return basei;
	}	
	
	if (basei->ngrupdias > ngrupdias   || (basei->ngrupdias==ngrupdias	&&
	   (basei->cliente   > cliente     || (basei->cliente==cliente		&&
	   (basei->objetivo  > objetivo    || (basei->objetivo==objetivo	&&
	   (basei->nroleg > nroleg     )))))))
		basei->pi=CargoNodoParte(basei->pi, ngrupdias, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, dia, hdesde, hhasta, columna);
	else
		basei->pd=CargoNodoParte(basei->pd, ngrupdias, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, dia, hdesde, hhasta, columna);
	
	return basei;
}                

static void MuestroNodoParte(p_parte basei)
{
	if (basei->pi!=NULL)
		MuestroNodoParte(basei->pi);
	PrintNodoParte(basei);     
	if (basei->pd!=NULL)
		MuestroNodoParte(basei->pd);
}


static void PrintNodoParte(p_parte basei)
{
	DATE diacom;
	int i;

  	for (i=0;  basei->dia[i]==NULL_DATE && i<CANT_COL_DIAS; i++);
	diacom=basei->dia[i]-i;

#ifdef DEBUG
	fprintf(stderr, "fdesde %3D\n", diacom);
#endif 
  	
	SetEncab(basei->ngrupdias, basei->emp, basei->cliente, basei->objetivo, diacom); // le paso el primer dia del ngrupdias
	
	SetLinea( //basei->ngrupdias, basei->cliente, basei->objetivo, 
		basei->emp, basei->nroleg, basei->regimen, basei->ptoser, basei->puesto, 
		basei->horas, basei->dia);
}

static void DeleteNodoParte(p_parte basei)
{
	if (basei->pi!=NULL)
		DeleteNodoParte(basei->pi);
	if (basei->pd!=NULL)
		DeleteNodoParte(basei->pd);
	free(basei);
}

static void CerrarReporteParte()
{
	if (rp2 != (report) ERROR) {
		CloseReport(rp2);
		rp2=(report)ERROR;
	}
}		

// devuelve la columna dentro de un nro. de reporte cualquiera, dado un dia
static int ObtColumnaRp(DATE dia, DATE diaini)
{
	int columna, nrodia;
	nrodia	= dia - diaini;
	columna = nrodia - ( ObtNroGrupoDias(dia,diaini) * CANT_COL_DIAS);
	return columna;
}

// devuelve el nro. de agrupacion de dias reporte dado un dia
static int ObtNroGrupoDias(DATE dia, DATE diaini)
{
	int nrodia, ngrupdias;
	nrodia	= dia - diaini;
	ngrupdias	= (int)(nrodia / CANT_COL_DIAS);
	return ngrupdias;
}

static void BorrarExcepciones(int emp, long cliente, int objetivo, DATE fecha, long nroleg, int ptoser,
                              int puesto, int nroint)
{
	SetIFld(operac|EXCEPCION_EMP,      emp);
	SetLFld(operac|EXCEPCION_CLIENTE,  cliente);
	SetIFld(operac|EXCEPCION_OBJETIVO, objetivo);
	SetDFld(operac|EXCEPCION_DIA,      fecha);
	SetLFld(operac|EXCEPCION_NROLEG,   nroleg);
	SetIFld(operac|EXCEPCION_PTOSER,   ptoser);
	SetIFld(operac|EXCEPCION_PUESTO,   puesto);
	SetIFld(operac|EXCEPCION_NROINT,   nroint);
	SetIFld(operac|EXCEPCION_CONDIC,   MIN_SHORT);
	SetIFld(operac|EXCEPCION_MOTIVO,   MIN_SHORT);
	while (GetRecord(operac|EXCEPCIONbyEMP, NEXT_KEY|PARTIAL_KEY , IO_NOT_LOCK, 8) != ERROR)
		DelRecord(operac|EXCEPCION);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	    case CLIED:
		   	InicClientesXusr();
    		break;
	    case CLIEH:
		   	InicClientesXusr();
    		break;
	    case OBJETD:
		   	InicObjetivosXusr(FmLFld(fm, CLIED, row), FmIFld(fm, EMP, row));
            break;
	    case OBJETH:
		   	InicObjetivosXusr(FmLFld(fm, CLIEH, row), FmIFld(fm, EMP, row));
            break;
	}
	return FM_OK;
	
}

static fm_status after(form fm, fmfield fno, int row)
{
	DATE fecierre;

	switch (fno) {
	case EMP:
		if (FmChgFld(fm))
			InicListaXusr(FmIFld(fm0, EMP));
    break;
	case FDESDE:
	case FHASTA:
		fecierre = GetFechaCierreOpe (FmIFld(fm0, EMP));
//		WiMsg("%.3D", fecierre);
		if (!soloimp)
	        if (FmDFld(fm, FDESDE) <= fecierre || FmDFld(fm, FHASTA) <= fecierre) {
				Warning("El Parte está cerrado al %.3D.\nDebe ingresar una fecha mayor al %.3D.", fecierre, fecierre);
				return FM_REDO;
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

static void AbrirArchLog()
{
	char nomarch[50] = {'\0'};
	
	sprintf(nomarch, "genparte.%d.log", GetUid());

	if (archOK == FALSE)	{
		if ((archlog = fopen(nomarch, "a+")) == (FILE*)NULL)
				Error("No se pudo generar el archivo %s", nomarch);
		archOK = TRUE;
	}
	
	fprintf(archlog, "Generación de Parte - Parametros de Ejecución:\n");
	fprintf(archlog, "Emp %d - Cliente Desde %ld Hasta %ld Objetivo Desde %d Hasta %d Fecha Desde %.3D Hasta %.3D \n", 
			FmIFld(fm0, EMP), FmLFld(fm0, CLIED), FmLFld(fm0, CLIEH), FmIFld(fm0, OBJETD), FmIFld(fm0, OBJETH), FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA));

	fprintf(archlog, "Inicia Proceso %.3D - %.3T\nClientes/Objetivos Procesados\n", Today(), Hour());
}

static short GetCodigoAusentismoPeru(int p_emp, long p_cliente, int p_objetivo, DATE p_fecha)
{
	static dbcursor c_rclies=(dbcursor)NULL;
	int ausente=NULL_SHORT;
//	fprintf(stderr, "Llama  %d %ld %d \n", p_emp, p_cliente, p_objetivo);

	c_rclies = CreateCursor(billpro|RCLIESPbyTIPCLI, IO_NOT_LOCK);
	SetCursorFrom(c_rclies, MIN_SHORT, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_rclies, MAX_SHORT, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_rclies) != ERROR) {

		if (LFld(billpro|RCLIESP_CLIENTE)!=p_cliente)
			continue;                                

		if (!IsNull(billpro|RCLIESP_OBJET))
			if (IFld(billpro|RCLIESP_OBJET)!=p_objetivo)
				continue;

//		fprintf(stderr, "2tipcli= %d\n", IFld(billpro|RCLIESP_TIPCLI));


		ausente=StrToI(GetParNov(p_emp, PARNOV_CODAUXGR, IFld(billpro|RCLIESP_TIPCLI), p_fecha));

		if (ausente != NULL_SHORT)
			break;
	}

//	fprintf(stderr, "ausente= %d\n", ausente);

	DeleteCursor(c_rclies);

	return ausente;


	
}
