/********************************************************************
*
* MODULE & VERSION : @(#)lhscont.c	1.34
* DATE             : 08/11/20
* TIME             : 12:58:06
* DESCRIPTION:
*               Impresion de control de horas por servicio
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*

IMPORTANTE: Como recorre el parte y calcula el standard de horas por puesto
si un puesto no esta cargado en el parte no corresponde el standard total con la suma mostrada en el reporte.
Para la opcion "consolidado por fecha" se soluciono calculando el std. total por objetivo

Si esta definido  _NOVIA_VER_2_0 (en amiente.h): 
	La fecha desde debe ser Lunes.
	La fecha hasta debe ser Domingo.
	Cuando se informa por fecha imprime un subtotal por semana.
	Se puede ejecutar el listado consolidado por semana.



*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "billpro.sch"
#include "lhscont.fmh"
#include "lhscont.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "filial.h"

#define VERSION		"1.33"

#define MAXLEG		 5000
#define MAXCLI	   100000

#define _LIST_FECHA					'D'
#define _LIST_FEC_SALTO				'P'
#define _LIST_PUESTO				'T'
#define _LIST_CONSOLIDADO_FECHA		'F'
#define _LIST_CONSOLIDADO_SEMANA	'S'


/* Estructuras */
struct cliente {
	long cli;
	int  obj;
	int  puesto;
	DATE fecha;
	short nrosem;
	int  svhn;
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
} pcli[MAXCLI], *ucli = pcli, *ecli, *fincli, *ncli;

struct nroleg {
	long cli;
	int  obj;
	long nroleg;
	int  puesto;
} pleg[MAXLEG], *uleg = pleg, *eleg;

/* Funciones privadas */
void ImprimirReporte();
void ImprimirDetalle();
void ImprimirNoPrint();
void ImprimirArchivo();
void TitCli();
void TitObj();
void TitPto();
void GenerarReporte();
void CargarCliObj(int emp, long cliente, int objetivo, long nroleg, int puesto, int ptoint, DATE fecha,
					short nrosem, int condic, int hn, int h50, int h100, int h100f);
void CalcEstVend(int emp, long cliente, int objetivo, long nroleg, int puesto, int ptoint, DATE fecha,
				 int *svhn, int *svh50, int *svh100, int *svh100f);
private int compcli(struct cliente *a, struct cliente *b);
private int compfec(struct cliente *a, struct cliente *b);
void AgruparEstructuraPorFecha ();
void ImpCli();
void ImpPto();
void ImpObj();
void TotSem();
static int EmpObjet(long cliente, int obj);
static void AbrirSalida();
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
FILE *fp;
form fm0;
report rp0;
schema comerc, operac, bill, billpro;
long cliant;
int  objant, ptoant;
short semant;
DATE fecant;
char dcliant[67], dobjant[30], dptoant[30];
//agrego para optimizacion
struct s_lisxusr_lib esta_lis;


/* Programa principal */
wcmd(lhscont, 1.34 11/20/08 )
{
	comerc = OpenSchema("comerc", IO_EABORT);
	billpro= OpenSchema("billpro", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

    fm0 = OpenForm  ("lhscont", FM_EABORT);
  	
  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while (DoForm(fm0, before, after) != FM_UPDATE) return;

	AbrirSalida();

	InicioListaTipoExcepcion();
	GenerarReporte();
	
	qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compcli);

	if (!strcmp(FmSFld(fm0, SALIDA), "A"))
		ImprimirArchivo();	
	else
		ImprimirReporte();

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();

	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void AbrirSalida()
{
	char v_aux[10];
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));
	}
	else {
		rp0 = OpenReport("lhscont", RP_EABORT|RP_NOBEGIN);

		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
		
		sprintf(v_aux,"%s", VERSION);
		RpSetFld(rp0, RVERSION, v_aux);
		RpSetFld(rp0, REMP, FmSFld(fm0, DEMP));
	}
}

void GenerarReporte()
{
	dbcursor cparte, cexc, cpto, cretro, cretroexc;
	char buffer[50];
	short tipoexc;

	cexc   = CreateCursor(EXCEPCIONbyEMP, IO_NOT_LOCK);
	cparte = CreateCursor(PARTEbyEMP, IO_NOT_LOCK);
	cpto   = CreateCursor(PUESTOSbyCLIENTE, IO_NOT_LOCK);
	cretro = CreateCursor(RETRObyEMP, IO_NOT_LOCK);
	cretroexc = CreateCursor(RETROEXCbyEMP, IO_NOT_LOCK);

	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLID))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLID) && esta_lis.objetivo < FmIFld(fm0, OBJD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIH) && esta_lis.objetivo > FmIFld(fm0, OBJH))
	    	continue;

		if (!ValidaFilial(esta_lis.cliente, esta_lis.objetivo, FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(esta_lis.cliente, esta_lis.objetivo) != BRIGADA) ||
			    (*FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(esta_lis.cliente, esta_lis.objetivo) == BRIGADA))
			continue;

		if (EmpObjet(esta_lis.cliente, esta_lis.objetivo) != FmIFld(fm0, EMP))
			continue;

		if (!ObjActivo(esta_lis.cliente, esta_lis.objetivo) && !IsNull(comerc|OBJETIVO_FECHAF) &&
				DFld(comerc|OBJETIVO_FECHAF) < FmDFld(fm0, FECHAD))
			continue;

		SetKey(billpro|OBJETRELbyCLIENTE, esta_lis.cliente, esta_lis.objetivo);
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		sprintf (buffer, "Procesando Puestos de Cliente %ld Objetivo %d", esta_lis.cliente, esta_lis.objetivo);
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();

		/* Busco todos los puestos y los cargo con hora 0 
		Esto es porque si no tiene horas cargadas en el parte hay que mostrarlo igual con el std. correspondiente*/	
		SetCursorFrom(cpto, esta_lis.cliente, esta_lis.objetivo, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cpto, esta_lis.cliente, esta_lis.objetivo, MAX_SHORT, MAX_SHORT);

		while (FetchCursor(cpto) != ERROR) {
			DATE pfecha;
			
			if ((IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > FmDFld(fm0, FECHAH)) ||
				(!IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FFINAL) < FmDFld(fm0, FECHAD))) {
				continue;
			}
			for (pfecha = FmDFld(fm0, FECHAD); pfecha <= FmDFld(fm0, FECHAH); pfecha++) {
				if (IsNull(PUESTOS_FFINAL) && DFld(PUESTOS_FINICIO) > pfecha)
					continue;
				CargarCliObj(FmIFld(fm0, EMP), LFld(PUESTOS_CLIENTE), IFld(PUESTOS_OBJET), NULL_LONG, 
							IFld(PUESTOS_TIPPTO), IFld(PUESTOS_PUESTO), pfecha, 
							NroSemana(FmDFld(fm0, FECHAD), pfecha),
							NULL_SHORT, 0, 0, 0, 0);
			}
		}

		/*Recorro el parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		
		while(FetchCursor(cparte) != ERROR) {

			if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			CargarCliObj(FmIFld(fm0, EMP),
						 LFld(PARTE_CLIENTE),
						 IFld(PARTE_OBJETIVO),
						 LFld(PARTE_NROLEG),
						 IFld(PARTE_PTOSER),
						 IFld(PARTE_PUESTO),
						 DFld(PARTE_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)),
						 NULL_SHORT,
						 IFld(PARTE_HSNOR),
						 IFld(PARTE_HS50),
						 IFld(PARTE_HS100F),
						 IFld(PARTE_HS100FE));

		}

		/*Recorro las excepciones */
		SetCursorFrom(cexc, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cexc, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		
		while (FetchCursor(cexc) != ERROR) {
			if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
				continue;
			
			tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
					continue;
			}

			CargarCliObj(FmIFld(fm0, EMP),
						 LFld(EXCEPCION_CLIENTE),
						 IFld(EXCEPCION_OBJETIVO),
						 LFld(EXCEPCION_NROLEG),
						 IFld(EXCEPCION_PTOSER),
						 IFld(EXCEPCION_PUESTO),
						 DFld(EXCEPCION_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(EXCEPCION_DIA)),
						 IFld(EXCEPCION_CONDIC),
						 IFld(EXCEPCION_HORAS),
						 IFld(EXCEPCION_HS50),
						 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
						 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0);
		}
		/*Recorro los retroactivos */
		SetCursorFrom(cretro, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		
		while (FetchCursor(cretro) != ERROR) {
			if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			CargarCliObj(FmIFld(fm0, EMP),
						 LFld(RETRO_CLIENTE),
						 IFld(RETRO_OBJETIVO),
						 LFld(RETRO_NROLEG),
						 IFld(RETRO_PTOSER),
						 IFld(RETRO_PUESTO),
						 DFld(RETRO_DIA),
						 NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)), 
						 NULL_SHORT,
						 IFld(RETRO_DHSNOR),
						 IFld(RETRO_DHS50),
						 IFld(RETRO_DHS100F),
						 IFld(RETRO_DHS100FE));
		}

		// Ahora repito la misma rutina para los hijos que tuviera
		SetKey(billpro|OBJETRELbyCOMER, esta_lis.cliente, esta_lis.objetivo, NULL_LONG, NULL_LONG);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			SetCursorFrom(cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAD),
							  MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo (cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAH),
							  MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cparte) != ERROR) {
				if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
					continue;

				CargarCliObj(FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo,
							 LFld(PARTE_NROLEG),
							 IFld(PARTE_PTOSER),
							 IFld(PARTE_PUESTO),
							 DFld(PARTE_DIA),
							 NroSemana(FmDFld(fm0, FECHAD), DFld(PARTE_DIA)),
							 NULL_SHORT,
							 IFld(PARTE_HSNOR),
							 IFld(PARTE_HS50),
							 IFld(PARTE_HS100F),
							 IFld(PARTE_HS100FE));
			}
			/*Recorro las excepciones */
			SetCursorFrom(cexc, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cexc, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cexc) != ERROR) {
				if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
					continue;
			
				tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
				/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
				if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
						continue;
				}

				CargarCliObj(FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo,
							 LFld(EXCEPCION_NROLEG),
							 IFld(EXCEPCION_PTOSER),
							 IFld(EXCEPCION_PUESTO),
							 DFld(EXCEPCION_DIA),
							 NroSemana(FmDFld(fm0, FECHAD), DFld(EXCEPCION_DIA)),
							 IFld(EXCEPCION_CONDIC),
							 IFld(EXCEPCION_HORAS),
							 IFld(EXCEPCION_HS50),
							 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
							 FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0);
			}
			/*Recorro los retroactivos */
			SetCursorFrom(cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cretro) != ERROR) {
				if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
					continue;

				CargarCliObj(FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo,
							 LFld(RETRO_NROLEG),
							 IFld(RETRO_PTOSER),
							 IFld(RETRO_PUESTO),
							 DFld(RETRO_DIA),
							 NroSemana(FmDFld(fm0, FECHAD), DFld(RETRO_DIA)), 
							 NULL_SHORT,
							 IFld(RETRO_DHSNOR),
							 IFld(RETRO_DHS50),
							 IFld(RETRO_DHS100F),
							 IFld(RETRO_DHS100FE));
			}
		}
	}
}

// Carga el vector de PCLI.
// Si viene de PARTE h100 contiene horas extras de francos y h100f de feriados.
// si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene vacio.
void	CargarCliObj(int emp, long cliente, int objetivo, long nroleg, int puesto, int ptoint, DATE fecha,
					 short nrosem, int condic, int hn, int h50, int h100, int h100f)
{
	int svhn, svh50, svh100, svh100f;
	bool esta = FALSE;

	svhn = svh50 = svh100 = svh100f = 0;

	for (ecli = pcli; ecli < ucli; ecli++)
		if (ecli->cli == cliente && ecli->obj == objetivo && ecli->puesto == puesto &&
			ecli->fecha == fecha) {
			esta = TRUE;
			break;
		}
	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI])
			Error("Tabla interna saturada. Max %d", MAXCLI);
		ucli->cli     = cliente;
		ucli->obj     = objetivo;
		ucli->puesto  = puesto;
		ucli->fecha   = fecha;
		ucli->nrosem  = nrosem;
		ucli->sehn    = 0;
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
		CalcEstVend(emp, cliente, objetivo, nroleg, puesto, ptoint, fecha, &svhn, &svh50, &svh100,
						&svh100f);
		ucli++;
	}
	if (condic != NULL_SHORT) {       // VIENE DE EXCEPCION
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
		if (!esta){
			CalcEstVend(emp, cliente, objetivo, nroleg, puesto, ptoint, fecha, &svhn, &svh50, &svh100,
						&svh100f);
		}						
	}
	ecli->svhn    += svhn;
	ecli->svh50   += svh50;
	ecli->svh100  += svh100;
	ecli->svh100f += svh100f;
}

// Calcula los estandares vendidos de un dia para un puesto de un empleado en un cliente - objetivo.
void CalcEstVend(int emp, long cliente, int objetivo, long nroleg, int puesto, int ptoint, DATE fecha,
				 int *svhn, int *svh50, int *svh100, int *svh100f)
{
	*svhn = *svh50 = *svh100 = *svh100f = 0.0;
	*svhn = StdHr(emp, cliente, objetivo, puesto, fecha, fecha);
}

// Imprime los datos contenidos en el vector PCLI, de acuerdo a las opciones
// de consolidacion y salida por.
void	ImprimirReporte()
{
	bool	first = TRUE;
	char buffer[50];
	cliant = NULL_LONG;
	objant = ptoant = semant = NULL_SHORT;
	fecant = NULL_DATE;

	if (*FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_FECHA && *FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_SEMANA)
		RpSetIFld (rp0, IMP_PUESTO, 1);
	else	
		RpSetIFld (rp0, IMP_PUESTO, 0);

	if (*FmSFld(fm0, OPCION) == _LIST_CONSOLIDADO_FECHA || *FmSFld(fm0, OPCION) == _LIST_CONSOLIDADO_SEMANA){
		AgruparEstructuraPorFecha ();
	}

	sprintf (buffer, "Generando reporte");
	FmSetFld (fm0, COMENT, buffer);
	WiRefresh();

	for (ecli = pcli; ecli < ucli; ecli++) {
	
		// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
		// tener que hacerlo en cada funcion que imprime detalles (arch, reporte, y no print)
		ecli->sthn    = ecli->sthn    - ecli->sahn    - ecli->sehn;
		ecli->sth50   = ecli->sth50   - ecli->sah50   - ecli->seh50;
		ecli->sth100  = ecli->sth100  - ecli->sah100  - ecli->seh100;
		ecli->sth100f = ecli->sth100f - ecli->sah100f - ecli->seh100f;

		if (!first && semant != ecli->nrosem && *FmSFld(fm0, OPCION) != _LIST_PUESTO) {
			TotSem();
		}

		/***  Imprimo los totales ***/
		if (*FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_FECHA && *FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_SEMANA) {
			if (ptoant != ecli->puesto || cliant != ecli->cli || objant != ecli->obj) {
				if(!first) {
					DoReport(rp0, ZTOTPTO);
				}
			}
		}
		if (objant != ecli->obj || cliant != ecli->cli) {
			if(!first)
				DoReport(rp0, ZTOTOBJ);
		}
		if (cliant != ecli->cli) {
			if(!first)
				DoReport(rp0, ZTOTCLI);
		}

		/***  Imprimo los titulos ***/
		if (ptoant != ecli->puesto || cliant != ecli->cli || objant != ecli->obj) {
			if (!first) {
				if ((cliant != ecli->cli || objant != ecli->obj) && *FmSFld(fm0, OPCION) == _LIST_CONSOLIDADO_FECHA){
					TitCli();
					TitObj();
					TitPto();
					RpEjectPage(rp0);
				}					
				if (*FmSFld(fm0, OPCION) == _LIST_FEC_SALTO) {
					TitCli();
					TitObj();
					TitPto();
					RpEjectPage(rp0);
/****************
					if (cliant == ecli->cli) {
						TitCli();
						ImpCli();
					}
					if (objant == ecli->obj) {
						TitObj();
						ImpObj();
					}
*****************/					
				}
			}
		}

		if (*FmSFld(fm0, OPCION) == _LIST_CONSOLIDADO_FECHA && first) {
		   	if (cliant != ecli->cli) {
				TitCli();
				ImpCli();
			}

			if (objant != ecli->obj || cliant != ecli->cli) {
				TitObj();
				ImpObj();
			}
        }
		
		if ((*FmSFld(fm0, OPCION) != _LIST_FEC_SALTO || first) && *FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_FECHA) {
		   	if (cliant != ecli->cli) {
				TitCli();
				ImpCli();
			}

			if (objant != ecli->obj || cliant != ecli->cli) {
				TitObj();
				ImpObj();
			}

			if (*FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_FECHA) {
				if (ptoant != ecli->puesto || cliant != ecli->cli || objant != ecli->obj) {
					TitPto();
					ImpPto();
				}
			}
       	}
		
		ImprimirNoPrint();
		if (*FmSFld(fm0, OPCION) != _LIST_PUESTO && *FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_SEMANA)
			ImprimirDetalle();
        
		cliant = ecli->cli;
		objant = ecli->obj;
		ptoant = ecli->puesto;
		semant = ecli->nrosem;
		fecant = ecli->fecha;
		first = FALSE;
	}

	/***  Imprimo los totales ***/
	if (*FmSFld(fm0, OPCION) != _LIST_PUESTO) {
 		TotSem();
	}
	
	if (*FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_FECHA && *FmSFld(fm0, OPCION) != _LIST_CONSOLIDADO_SEMANA)
		DoReport(rp0, ZTOTPTO);

	DoReport(rp0, ZTOTOBJ);
	DoReport(rp0, ZTOTCLI);
}

// Esta funcion se utiliza siempre y es para imprimir el detalle en una 
// zona no print del reporte, cuyos campos son utilizados en las zonas
// automaticas de totalizacion.
void ImprimirNoPrint()
{
	if (*FmSFld(fm0, OBJBRI) == 'B') {
		RpSetLFld(rp0, SVTOT,   0);
		RpSetLFld(rp0, STNPRES, 0);
		RpSetLFld(rp0, SATOT,   0);
	}
	else {
		RpSetLFld(rp0, SVTOT,   ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
		RpSetLFld(rp0, STNPRES, ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f -
							   (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f));
		RpSetLFld(rp0, SATOT,   ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);
	}
	RpSetLFld(rp0, STTOT,   ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f);
	DoReport(rp0, ZNOPRINT);
}

// Se usa esta funcion cuando se debe imprimir el detalle de las fechas y 
// la salida es a reporte
void ImprimirDetalle()
{
	RpSetDFld(rp0, RFCH,     ecli->fecha);
	if (*FmSFld(fm0, OBJBRI) == 'B') {
		RpSetLFld(rp0, SVTOT,   0);
		RpSetLFld(rp0, STNPRES, 0);
		RpSetLFld(rp0, SATOT,   0);
	}
	else {
		RpSetLFld(rp0, FSVTOT,   ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f);
		RpSetLFld(rp0, FSTNPRES, ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f -
								(ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f));
		RpSetLFld(rp0, FSATOT,   ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f);
	}
	RpSetLFld(rp0, FSTTOT,   ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f);
	DoReport(rp0, ZFCH);
}

// Imprime el titulo de Cliente en el archivo de salida
void TitCli()
{
	SetKey(bill|CLIENTEbyCLIENTE, ecli->cli);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");

	RpSetLFld(rp0, RCLI,  ecli->cli);
	RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
}

// Imprime el titulo de objetivo en el archivo de salida
void TitObj()
{
	SetKey(comerc|OBJETIVObyCLIENTE, ecli->cli, ecli->obj);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
	SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");

	RpSetIFld(rp0, ROBJ,  ecli->obj);
	RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
}

// Imprime el titulo de puesto en el archivo de salida
void TitPto()
{
	SetKey(comerc|TPTOSERbyTIPPTO, ecli->puesto);
	if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(comerc|TPTOSER_DESCRIP, "ERROR: Puesto Inexistente");

	RpSetIFld(rp0, RPTO,  ecli->puesto);
	RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
}

void ImpCli()
{
	DoReport(rp0, ZCLI);
}

void ImpObj()
{
	DoReport(rp0, ZOBJ);
}

void ImpPto()
{
	if (*FmSFld(fm0, OPCION) != _LIST_PUESTO)
		DoReport(rp0, ZPTO);
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
	return	a->cli   < b->cli   ? -1 : a->cli   > b->cli   ? 1 :
			a->obj   < b->obj   ? -1 : a->obj   > b->obj   ? 1 :
			a->fecha < b->fecha ? -1 : a->fecha > b->fecha ? 1 :
			0;
}

void AgruparEstructuraPorFecha () 
{
	bool esta = FALSE;

	fincli = pcli;
	qsort((char *)pcli, (unsigned)(ucli-pcli), sizeof(pcli[0]), (IFPVCPVCP)compfec);

	/* Esta funcion agrupa por cliente-objetivo-fecha sobre la estructura existente
	   el puesto no se elimina antes por el calculo de horas standar */
	for (ecli = pcli; ecli < ucli; ecli++) {
		for (esta=FALSE, ncli = pcli; ncli < fincli; ncli++) {
			if (ecli->cli == ncli->cli && ecli->obj == ncli->obj && ecli->fecha == ncli->fecha) {
				esta = TRUE;
				break;
			}
		}

		if (esta) {
			ncli->sehn    += ecli->sehn;
			ncli->seh50   += ecli->seh50;
			ncli->seh100  += ecli->seh100;
			ncli->seh100f += ecli->seh100f;
			ncli->sahn    += ecli->sahn;
			ncli->sah50   += ecli->sah50;
			ncli->sah100  += ecli->sah100;
			ncli->sah100f += ecli->sah100f;
			ncli->sthn    += ecli->sthn;
			ncli->sth50   += ecli->sth50;
			ncli->sth100  += ecli->sth100;
			ncli->sth100f += ecli->sth100f;
			ncli->svhn    += ecli->svhn;
			ncli->svh50   += ecli->svh50;
			ncli->svh100  += ecli->svh100;
			ncli->svh100f += ecli->svh100f;
		}
		else {
			ncli->cli     = ecli->cli;
			ncli->obj     = ecli->obj;
			ncli->puesto  = ecli->puesto;
			ncli->nrosem  = ecli->nrosem;
			ncli->fecha   = ecli->fecha;
			ncli->sehn    = ecli->sehn;
			ncli->seh50   = ecli->seh50;
			ncli->seh100  = ecli->seh100;
			ncli->seh100f = ecli->seh100f;
			ncli->sahn    = ecli->sahn;
			ncli->sah50   = ecli->sah50;
			ncli->sah100  = ecli->sah100;
			ncli->sah100f = ecli->sah100f;
			ncli->sthn    = ecli->sthn;
			ncli->sth50   = ecli->sth50;
			ncli->sth100  = ecli->sth100;
			ncli->sth100f = ecli->sth100f;
			ncli->svhn    = ecli->svhn; 
			ncli->svh50   = ecli->svh50;
			ncli->svh100  = ecli->svh100;
			ncli->svh100f = ecli->svh100f;
			fincli ++;
		}
	}

	/*Seteo el nuevo final de la estructura  */
	ucli = fincli;
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

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
	break;
	case OPCION:
		#ifndef _NOVIA_VER_2_0
			if (*FmSFld(fm0, OPCION) == _LIST_CONSOLIDADO_SEMANA) {
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

void TotSem()
{	
	DATE fdesde, fhasta;

	#ifndef _NOVIA_VER_2_0
 		return;		
	#endif

	SemanaDesdeHasta(fecant, &fdesde, &fhasta);
	RpSetDFld(rp0, RSEMD, fdesde);
	RpSetDFld(rp0, RSEMH, fhasta);
	DoReport(rp0, ZTOTSEM);
}

void ImprimirArchivo()
{
	char buffer[50];
	double hsstd = 0.0, hsnopres = 0.0, hstot = 0.0, hsadic = 0.0;

	AgruparEstructuraPorFecha ();

	sprintf (buffer, "Generando Archivo");
	FmSetFld (fm0, COMENT, buffer);
	WiRefresh();

	fprintf(fp, "Cliente\tDescrip\tObjetivo\tDescrip\t%Fecha\tHs Standart\t%Hs No Prestadas\tTotal\tServ Adic\t\n");

	for (ecli = pcli; ecli < ucli; ecli++) {
		// Recalculo los valores trabajados restandole lo de empresa y adicional. Lo hago aca para no
		// tener que hacerlo en cada funcion que imprime detalles (arch, reporte, y no print)
		ecli->sthn    = ecli->sthn    - ecli->sahn    - ecli->sehn;
		ecli->sth50   = ecli->sth50   - ecli->sah50   - ecli->seh50;
		ecli->sth100  = ecli->sth100  - ecli->sah100  - ecli->seh100;
		ecli->sth100f = ecli->sth100f - ecli->sah100f - ecli->seh100f;

		if (*FmSFld(fm0, OBJBRI) == 'B') {
			hsstd    = 0.0;
			hsnopres = 0.0;
			hsadic   = 0.0;
		}
		else {
			hsstd    = (double) (ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) / 100;
			hsnopres = (double)((ecli->svhn + ecli->svh50 + ecli->svh100 + ecli->svh100f) -   
					   			(ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f)) / 100;
			hsadic   = (double) (ecli->sahn + ecli->sah50 + ecli->sah100 + ecli->sah100f) / 100;
		}
		hstot = (double) (ecli->sthn + ecli->sth50 + ecli->sth100 + ecli->sth100f) / 100;

		fprintf(fp, "%ld\t%s\t%d\t%s\t%.3D\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t\n",
					 ecli->cli,
					 GetDescCli(ecli->cli),
					 ecli->obj,
					 GetObjDescrip(ecli->cli, ecli->obj),
					 ecli->fecha,
					 hsstd, hsnopres, hstot, hsadic);
	}
}

static int EmpObjet(long cliente, int obj)
{
	int emp = 1;

	SetLFld(comerc|OBJETIVO_CLIENTE, cliente);
	SetLFld(comerc|OBJETIVO_OBJET,   obj);
	if ( GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR ) {
	 	emp = IFld(comerc|OBJETIVO_EMP);
	}
	return emp;
}
