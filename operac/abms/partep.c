/********************************************************************
*EsVigilador
* MODULE & VERSION : @(#)partep.c	1.0
* DATE             : 01/04/13
* TIME             : 15:51:20
*
* CREATED          : 17/09/98
*                          
* DESCRIPTION:
*             Carga de Horas de los vigiladores.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "comgral.h"
#include "opedef.h"
#include "billpro.h"
#include "ambiente.h"
#include "disthspro.h"
#include "filial.h"
#include "webinter.h"
#include "comerc.sch"
#include "operac.sch"
#include "asist.sch"
#include "sue.sch"

#include "partep.fmh"        //fm0 (partep.fm)
#include "partep1.fmh"       //fm1 (partep1.fm)	Detalle de Horas
#include "partep2.fmh"       //fm2 (partep2.fm)	Excepciones
#include "partep3.fmh"       //fm3 (partep3.fm)	Ausentimo
#include "partep4.fmh"       //fm4 (partep4.fm)	Asignación del puesto
#include "partep5.fmh"       //fm5 (partep5.fm) Control de calculo de horas

#define WAR_SUPERPOS  "El Vigilador: %s está asignado en:\nCliente Obj. Hr.Ent  Hr.Sal  Condic. HsNor. Hs.50  Hs.100 Hs100F\n%s\n Linea (%03d)"
#define WAR_SUPERPOSF "El Vigilador: %s  está de Franco en:\nCliente Obj. Hr.Ent  Hr.Sal  Condic. HsNor. Hs.50  Hs.100 Hs100F\n%s"
#define WAR_SUPERPOS2 "SUPERPOSICION HORARIA.\n\nEl Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T\nLinea %d\nDesea Grabar de Todas Maneras ?"
#define	ERR_SUPERPOS  "SUPERPOSICION HORARIA.\n\nEl Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T\nLinea %d"
#define	ERR_SUPERPOS2 "SUPERPOSICION HORARIA.\n\nEl Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T Dia %.3D\nLinea %d"
#define WAR_SUPERPOS3 "SUPERPOSICION HORARIA.\n\nEl Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T Dia %.3D\nLinea %d\nDesea Grabar de Todas Maneras ?"
#define	ERR_PUEOCUP   "En el puesto %d %d hay más de %d horas.\nDebe cargar la diferencia en Excepciones.\nLinea %d"
#define WAR_PTOOCUP   "El puesto %d %d tiene todos los vigiladores asignados.\nPor favor, verifique los datos.\nLinea %d"
#define ERR_HORAS     "Existen más horas %s en excepciones.\nVerifique la cantidad de horas."
#define ERR_PTODIA    "No podrá asignar vigilador al puesto %d %d el día %s\nporque no se trabaja.\nLinea %d"
#define WAR_CERRADO   "El Parte está cerrado el %.3D.\nO se cerro este objetivo para facturar.\nNo podrá modificarse."
#define WAR_CERRADO_PARCIAL   "El Parte está cerrado en forma parcial el %.3D.\nNo podrá modificarse."

//-------------------------------------------Definiciones para listas enlazadas de ordenamiento----------------------------------------------------------------//

#define ORD_LEGAJO 1

typedef struct stnnhent * tnnhent;
typedef struct stnnhsal * tnnhsal;
typedef struct stnnnroleg * tnnnroleg;
typedef struct stnntippto * tnntippto;
typedef struct stnnpuesto * tnnpuesto;
typedef struct stnnnroint * tnnnroint;

typedef struct stnnhent {
	TIME	nhent;
	tnnhsal	nnhsal;
	tnnhent	nsig;
} stnnhent;

typedef struct stnnhsal {
	TIME	nhsal;
	tnnnroleg	nnnroleg;
	tnnhsal	nsig;
} stnnhsal;

typedef struct stnnnroleg {
	long	nnroleg;
	tnntippto	nntippto;
	tnnnroleg	nsig;
} stnnnroleg;

typedef struct stnntippto {
	int		ntippto;
	tnnpuesto	nnpuesto;
	tnntippto	nsig;
} stnntippto;

typedef struct stnnpuesto {
	int		npuesto;
	tnnnroint	nnnroint;
	tnnpuesto	nsig;
} stnnpuesto;

typedef struct stnnnroint {
	int		nnroint;
	tnnnroint	nsig;
} stnnnroint;

/* Funciones Privadas */

static tnnhent AcuNNhent(tnnhent, tnnhent*);
static tnnhsal AcuNNhsal(tnnhsal, tnnhsal*);
static tnnnroleg AcuNNnroleg(tnnnroleg, tnnnroleg*);
static tnntippto AcuNNtippto(tnntippto, tnntippto*);
static tnnpuesto AcuNNpuesto(tnnpuesto, tnnpuesto*);
static tnnnroint AcuNNnroint(tnnnroint, tnnnroint*);

static void LisNNhent(tnnhent);
static void LisNNhsal(tnnhsal);
static void LisNNnroleg(tnnnroleg);
static void LisNNtippto(tnntippto);
static void LisNNpuesto(tnnpuesto);
static void LisNNnroint(tnnnroint);

static void BorNNhent(tnnhent);
static void BorNNhsal(tnnhsal);
static void BorNNnroleg(tnnnroleg);
static void BorNNtippto(tnntippto);
static void BorNNpuesto(tnnpuesto);
static void BorNNnroint(tnnnroint);

tnnhent	iniord;
int		ntippto, npuesto, nnroint;
long	nnroleg;
TIME	nhent, nhsal;
//-------------------------------------------Fin de Definiciones para listas enlazadas de ordenamiento---------------------------------------------------------//

typedef struct lptos
{
	int  ptoser;
	int	 puesto;
	int  horas;
	int	 cantvig;
	int  cantpue;
	DATE ffinal;
	struct lptos *sgte;
} nptos;
nptos *listapto;
nptos *ultpto;

/* Funciones privadas */
//extern bool GetHorasViaje(int ,long, int, DATE, int*);
//static int HsIgualCliente(int emp, long nroleg, DATE fecparte, long cliente, int obj, int ptoser, int puesto,  int nroint, int *hs);
static fm_status before(form, fmfield, int), after(form, fmfield, int),
				 							 after_partep2(form, fmfield, int),
											 after_partep4(form, fmfield, int),
											 before_parte1(form, fmfield, int);
static void Lectura(fm_cmd, find_mode);
static void InsertarPuesto(long cliente, int obj);
static void FreeListaPtos();
static void display(char *buffer);
static void MuestraNrodeLineaActual(fmfield fno, int row);
static void ControlConsistenciaHoras();

static void LlenaHorasBase(long p_nroleg, int* p_hs_nor);
static void LlenaLegajoFm(long p_nroleg, int* v_hs_nor);

static bool WarPue(int emp, long nroleg, DATE fecparte, long cliente, int obj, TIME hsent, TIME hssal,
				   bool *franco, char * p_cadena);
static bool SupPue(int emp, long nroleg, DATE fecparte, long cliente, int obj, TIME hsent, TIME hssal);
static bool PueOcup(int emp, DATE fecparte, long cliente, int obj, TIME hsent, TIME hssal, int ptoser,
					int puesto, long * nroleg, int *hspto, int row);
static bool ValidarHsTot(long std, long hstot);
static bool Ausente(int emp, long nroleg, DATE fecparte, long * cliente, int * objet, bool *vacac);
static bool SupPueFm(long nroleg, TIME hsent, TIME hssal, int fila, TIME * hsentre, TIME * hssalida);
static bool ErrPue(int emp, long p_nroleg, DATE fecparte, long p_cliente, int p_obj, char p_tipodia,
                   int p_ptoser, int p_puesto, char *p_cadena);

int ErrPueFm(int p_row);

static bool BorrarAsisten(int emp, long nroleg, DATE fecparte, long cliente, int obj);
static bool ControlesNivelFormulario();
static bool ControlesNivelLinea(int row, bool p_cambiofm);
static bool YaCalculo(int p_row);


static int  CalcNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto);
static int  validate(void);

private void PutInTables();
private void DelVigilador();
private void SeteoCampos(int hsnor, int hs50, int hs100f, int hs100fe, int p_linea);

private void CargaFm(int p_emp, long p_nroleg, int p_i, DATE p_dia, char *p_condic, int p_ptoser, int p_puesto, int p_nroint, TIME p_hent, TIME p_hsal, int p_conf, 
					 int p_hsnor, int p_hs50, int p_hs100fe, int p_hs100f, int p_codaus);

//private void SeteoDetalle(int row, long p_nroleg, DATE fechaparte, bool tienedia, bool alotrodia, char * tipvig, int ptoser, int puesto, int nroint);
private int ObtenerExcep(int ptoser, int puesto);
private int HsExcep(int tiphs);
bool NecesitaOt (int fila);
void BorrarExcepciones (int fila);
bool ValidoClienteExcepcion (long cliente, short objetivo, short cond, short motivo, bool valida);
bool ErrorSuperposicion ();
bool EstanCorriendoLaImportacionDeHoras();

/* Declaraciones globales */
int i, row_padre, objet, hspto, g_hsnor, g_hs50, g_hs100f, g_hs100fe, g_hspega, g_guardi;

int g_tippto=NULL_SHORT,
    g_codint=NULL_SHORT,
    g_nroint=NULL_SHORT,
    g_emp=NULL_SHORT;

form fm0, fm1, fm2, fm3, fm4, fm5;
long nroleg, cliente;
char cadena[250];
char g_prog[20];
schema operac, comerc, asist, sue;
dbtable APARTE, aparte ;
bool newvig = FALSE, errorhs = FALSE, cambio = FALSE, vacac = FALSE,
     objconot = FALSE, renglon_ok = TRUE, franco = FALSE; 
bool cerrado = FALSE, cerradopar = FALSE, cerradoxfil = FALSE, listacok = FALSE, listaook = FALSE;  
DATE fecierre, feparcial;
bool g_GRPSUPOPER, g_GRPBRIG;
int hstot = 0;
int g_i=0, g_row;

/* Programa principal */
wcmd(partep, 1.0 01/05/13)
{
	fm_cmd cmd;

	sprintf(g_prog, "%s", argv[0]);

	fm0 = OpenForm("partep", FM_EABORT);

	sue    = OpenSchema("sue",    IO_EABORT);
	asist  = OpenSchema("asist",  IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	aparte  = CreateAlias(operac|PARTE);
	APARTE  = CreateAlias(operac|PARTE);

	g_GRPSUPOPER=UsrInGrupo(GRPSUPOPER, GetUid());
	g_GRPBRIG   =UsrInGrupo(GRPBRIG, GetUid());

	g_emp= StrToI(getenv("emp"));
	sprintf (g_tcalculo, "%s", GetParNov(g_emp, PARNOV_TCALC_HS, 1, Today()) );

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ingresar al programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}

	// Inicio Permisos
	FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Clientes - Objetivos");
	WiRefresh();
	InicListaXusr(g_emp);
	FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Legajos");
	WiRefresh();

	fecierre  = GetFechaCierreOpe(g_emp);

	InicLegajoXusr (g_emp, fecierre, fm0, MENSAJE, FALSE, _TIPPER_INSERTA);

//	ImprimeLegajoXusr();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:  Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:

		// Se controla que no se este ejectando el cierre, sino es asi  se permite ejecutar el programa
		if (CierreActivo()) {
			WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
			continue;
		}

		/* Esta Funcion Agrupa Todos los controles que se hacen antes de grabar */
		if (!ControlesNivelFormulario())
			continue;

		newvig   = FALSE;
		objconot = FALSE;
		
		BeginTransaction();

		PutInTables();

		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(operac|PARTE);
		errorhs  = FALSE;
		newvig   = FALSE;
		objconot = FALSE;
		break;
	}
	
	DeleteAlias(aparte);
	DeleteAlias(APARTE);
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	dbcursor c_PARTE; 
	int v_i;

	hstot = 0;
	
	if (EstanCorriendoLaImportacionDeHoras()) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1004), 
		"Se esta ejecutando la importación de horas.\nNO se puede ver el parte para esta fecha.");
		return;
	}

	InicioListaTipoExcepcion();
	
	SetIFld(operac|PARTE_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|PARTE_CLIENTE,  FmLFld(fm0, CLIE));
	SetIFld(operac|PARTE_OBJETIVO, FmIFld(fm0, OBJET));
	SetDFld(operac|PARTE_DIA,      FmDFld(fm0, FECPARTE));
	SetLFld(operac|PARTE_NROLEG,   MIN_LONG);
	SetIFld(operac|PARTE_PTOSER,   MIN_SHORT);
	SetIFld(operac|PARTE_PUESTO,   MIN_SHORT);
	SetIFld(operac|PARTE_NROINT,   MIN_SHORT);
	switch(GetRecord(operac|PARTEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 4)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(operac|PARTEbyEMP, NEXT_KEY|PARTIAL_KEY, 4);
		DbToFm(fm0, 0, DOBJ);
		FmShowFlds(fm0, 0, DOBJ);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	c_PARTE = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

	SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET));
	GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	iniord = NULL;
	nhent = NULL_TIME;
	nhsal = NULL_TIME;
	nnroleg = NULL_LONG;
	ntippto = NULL_SHORT;
	npuesto = NULL_SHORT;
	nnroint = NULL_SHORT;


	SetCursorFrom(c_PARTE, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmDFld(fm0, FECPARTE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_PARTE, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmDFld(fm0, FECPARTE), MAX_LONG, MAX_SHORT, MAX_SHORT, MIN_SHORT);
	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && FetchCursor(c_PARTE) != ERROR; v_i++) {

		if (FmIFld(fm0, ORDENA)==ORD_LEGAJO)
			CargaFm(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), v_i, FmDFld(fm0, FECPARTE), SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_PTOSER), 
					IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), IFld(operac|PARTE_CONFIR),
					IFld(operac|PARTE_HSNOR), IFld(operac|PARTE_HS50), IFld(operac|PARTE_HS100FE), IFld(operac|PARTE_HS100F),IFld(operac|PARTE_CODAUS));
		else {
			nhent =   TFld(operac|PARTE_HORAENT);
			nhsal =   TFld(operac|PARTE_HORASAL);
			nnroleg = LFld(operac|PARTE_NROLEG);
			ntippto = IFld(operac|PARTE_PTOSER);
			npuesto = IFld(operac|PARTE_PUESTO);
			nnroint = IFld(operac|PARTE_NROINT);

			iniord = AcuNNhent(iniord, &iniord);
		}

	}
	DeleteCursor(c_PARTE);

	if (FmIFld(fm0, ORDENA)!=ORD_LEGAJO) {
		g_i=0;
		LisNNhent(iniord);
		BorNNhent(iniord);
	}


//	FmSetIFld(fm0, STD, HorasStd(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT, FmDFld(fm0, FECPARTE)));

	if (GetServicioObj(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET)) == BRIGADA)
		FmSetLFld(fm0, STD, 0);
	else 
		FmSetLFld(fm0, STD, StdHr(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT,  FmDFld(fm0, FECPARTE), FmDFld(fm0, FECPARTE)));
	FmSetIFld(fm0, HSSTD,   hstot - HsExcep(FACTURABLE) - HsExcep(ACARGO_EMP));
	FmSetIFld(fm0, HSFAC,   HsExcep(FACTURABLE));
	FmSetIFld(fm0, HSACEMP, HsExcep(ACARGO_EMP));



	if (cerrado || cerradopar) {
		FmSetDisplayOnly(fm0, STD,    REGIM,       TRUE);    
		FmSetDisplayOnly(fm0, COND,   CONTROL_FLD, TRUE);
		FmSetDisplayOnly(fm1, NORMAL, FRANCOS,     TRUE);
		FmSetDisplayOnly(fm2, MULTI,  OBS,         TRUE);
		FmSetDisplayOnly(fm3, CODNOV, DESCRINAS,   TRUE);
		FmSetDisplayOnly(fm4, TIPPTO, NROINT,      TRUE);
	}
	else {
		FmSetDisplayOnly(fm0, STD,    REGIM,       FALSE);    
		FmSetDisplayOnly(fm0, COND,   CONTROL_FLD, FALSE);
		FmSetDisplayOnly(fm1, NORMAL, FRANCOS,     FALSE);
		FmSetDisplayOnly(fm2, MULTI,  OBS,         FALSE);
		FmSetDisplayOnly(fm3, CODNOV, DESCRINAS,   FALSE);
		FmSetDisplayOnly(fm4, TIPPTO, NROINT,      FALSE);
	}

}


static fm_status before(form fm, fmfield fno, int row)
{
	MuestraNrodeLineaActual(fno, row);

	switch (fno) {
	case NROLEG:
		if (!FmIsNull(fm, fno, row))
			return FM_SKIP;
		break;
	case REGIM:
		if (!FmIsNull(fm, fno, row))
			return FM_SKIP;
		break;
	case AGRUPHS :
		renglon_ok = TRUE;
		if (ExisteCliObjEnGrp(GRPHORACERO, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) 
			return FM_SKIP;

		break;
	case CLIE: 
		if (listacok == FALSE || FmChgFld(fm))  {
			InicClientesXusr();
			listacok = TRUE;
		}
		break;
	case OBJET:
		if (listaook == FALSE || FmChgFld(fm))  {
			InicObjetivosXusr(FmLFld(fm, CLIE, row), FmIFld(fm, EMP, row));
			listaook = TRUE;
		}
		break;

	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	int hstraslado;
	char regimen[15], v_tipvig[2], v_otr_condic;
//	long v_nroleg = NULL_LONG;
	int v_i = 0;


	if (FmKeyCode(fm) == K_INS && row == 0) {
		return FM_REDO;
	}
	if (FmKeyCode(fm) == K_DEL){

		fm4 = UseSubform(fm0, PTO, 0, row);

	    if (EsInsertado(FmIFld(fm, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE), FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), FmIFld(fm4, NROINT) )) {
			return FM_REDO;
	    }

	}

	if (FmKeyCode(fm) == K_DEL && FmIFld(fm, I_EXTASIG, row) && !FmIFld(fm, I_VIGINEW, row) &&
		fno != AGRUPHS && !g_GRPBRIG)
		return FM_REDO;

	if (FmKeyCode(fm) == K_DEL){
		renglon_ok = TRUE;
	}

	row_padre = row;

	switch (fno) {
	case EMP:
		if (FmChgFld(fm)){
    		sprintf (g_tcalculo, "%s", GetParNov(FmIFld(fm0, EMP), PARNOV_TCALC_HS, 1, Today()) );

			FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Clientes - Objetivos");
			WiRefresh();

		    InicListaXusr(FmIFld(fm0, EMP));

			FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Legajos");
			WiRefresh();

			InicLegajoXusr (FmIFld(fm, EMP), fecierre, fm0, MENSAJE, FALSE, _TIPPER_INSERTA);

//			fprintf(stderr,"empresa= %d\n", FmIFld(fm, EMP));
//			ImprimeLegajoXusr();
		}

	    break;
  	case CLIE:	
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);

		if (!FmIsNull(fm, fno)){
			if (ValidaClienteXusr(FmLFld(fm, CLIE, row)))
			  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIE, row)), row);
			else {
	 			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIE, row));
				FmSetLFld(fm, CLIE, NULL_LONG, row);
				FmSetFld(fm, DCLIE, NULL_STR, row);
				return FM_REDO;
			}	
		}
		break;

	case OBJET:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIE, row));

		if (FmIsNull(fm, fno)){
			Warning("El Objetivo No puede ser nulo");
			return FM_REDO;
		}
		if (ValidaObjetivoXusr(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
			FmSetFld(fm, DOBJ, GetObjDescrip(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row)), row);
		else	{
			Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row));
			FmSetIFld(fm, OBJET, NULL_SHORT, row);
			FmSetFld(fm, DOBJ, NULL_STR, row);
			return FM_REDO;
   		}

		if (g_GRPBRIG &&
			GetServicioObj(FmLFld(fm, CLIE), FmIFld(fm, fno)) != BRIGADA) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1007), "El Objetivo no es de Brigadas.");
			FmNextFld(fm, CLIE);
		}
		break;
	case FECPARTE:
		if (GetServicioObj(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET)) == BRIGADA)
			FmSetLFld(fm0, STD, 0);
		else 
			FmSetLFld(fm0, STD, StdHr(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), NULL_SHORT, 
									  FmDFld(fm0, FECPARTE), FmDFld(fm0, FECPARTE)));

		feparcial = GetFechaCierreParcial(FmIFld(fm0, EMP));
		fecierre  = GetFechaCierreOpe(FmIFld(fm0, EMP));

		if (feparcial != NULL_DATE && FmDFld(fm0, FECPARTE) > fecierre && FmDFld(fm0, FECPARTE) <= feparcial){
			cerradopar = TRUE;	
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1008), WAR_CERRADO_PARCIAL, feparcial);
		} 
		else {
			cerradopar = FALSE;	
		}

		if (!cerradopar && fecierre != NULL_DATE && FmDFld(fm0, FECPARTE) <= fecierre) {
			cerrado = TRUE;	
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1009), WAR_CERRADO, fecierre);
		} 
		else {
			cerrado = FALSE;	
		}

		if (FmIFld(fm0, EMP) == _EMP_SAPE) {
			if (!GetHorasViaje(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
							   FmDFld(fm0, FECPARTE), &hstraslado)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1010), 
				"Debe cargar Horas de Viaje pra el objetivo %d", FmIFld(fm0, OBJET));
			}
		}
		break;
	case COND:
		v_otr_condic=CondicParteOtroObjetivo(FmIFld(fm, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmDFld(fm, FECPARTE),FmLFld(fm, NROLEG, row));

		if (FmChgFld(fm) && *FmSFld(fm, fno, row) != v_otr_condic && v_otr_condic!='\0')
			if (WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_WAR, 1046), "Existe otro parte cargado\ncon la condicion %c en otro objetivo \ny se esta ingresando  %c\n Desea Continuar?", v_otr_condic, *FmSFld(fm, fno, row))==WD_NO){
				FmSetFld(fm, fno, FmFldPrev(fm));
				return FM_REDO;
			}

		if (FmChgFld(fm) && ( *FmSFld(fm, fno, row) != 'T' && *FmSFld(fm, fno, row) != 'V') ) {
			if (ExisteCliObjEnGrp(GRPHORACERO, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1045), "En este objetivo solo se puede poner la condicion T o V\nLinea %d", row);
				return FM_REDO;
			}
		}

		if (FmChgFld(fm) && *FmFldPrev(fm) == 'A' && *FmSFld(fm, fno, row) != 'A') 
			if (TieneLic(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE))) 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1012), 
				         "El Vigilador: %s  tiene Licencias cargadas.\nLinea %d", 
				         GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)), row);


		if (FmChgFld(fm) && (*FmSFld(fm, fno, row) == 'A' || *FmSFld(fm, fno, row) == 'V')) {
			if (WarPue(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE), FmLFld(fm, CLIE),
					   FmIFld(fm, OBJET), FmTFld(fm, HSENTRE, row), FmTFld(fm, HSSAL, row), &franco, cadena)) {
				if (franco)
					Warning(WAR_SUPERPOSF, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)), cadena);
				else
					Warning(WAR_SUPERPOS, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)), cadena);
			}

			if (*FmSFld(fm, fno, row) == 'A') {
				int j;
				DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
				fm2 = UseSubform(fm0, EXCP, 0, row);

				for (j=0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {
					FmSetIFld(fm2, TIPFAC,  NULL_SHORT, j);
					FmSetFld (fm2, DTIPFAC, NULL_STR,   j);
					FmSetIFld(fm2, MOTIVO,  NULL_SHORT, j);
					FmSetFld (fm2, DMOTIV,  NULL_STR,   j);
					FmSetIFld(fm2, HORAS,   NULL_SHORT, j);
					FmSetIFld(fm2, HS50,    NULL_SHORT, j);
					FmSetIFld(fm2, HS100,   NULL_SHORT, j);
					FmSetFld (fm2, OBS,     NULL_STR,   j);
				}
			}
			
			if (*FmSFld(fm, fno, row) == 'A' || strcmp(GetParNov(FmIFld(fm, EMP), PARNOV_V_HORAS, LINEA_UNICA, FmDFld(fm, FECPARTE)), "1")!=0) {

				FmSetTFld(fm, HSENTRE, StrToT("0000"), row);
				FmSetTFld(fm, HSSAL,   StrToT("0000"), row);
				FmSetIFld(fm, HSTOT,   0,              row);

				fm1 = UseSubform(fm, DETHS,  0, row);
				FmSetIFld(fm1, NORMAL,  0);
				FmSetIFld(fm1, EXTRAS1, 0);
				FmSetIFld(fm1, EXTRAS2, 0);
				FmSetIFld(fm1, FRANCOS, 0);
				FmNextFld(fm, fno, row+1);
			}
		}

		if (FmChgFld(fm)){
			if (*FmSFld(fm, fno, row) == 'A') {
				DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
				FmNextFld(fm, fno, row + 1);


			} 
			SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET));
			GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

			// Limpio lo del mismo legajo
			for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
				if (FmLFld(fm0, NROLEG, row)!=FmLFld(fm0, NROLEG, v_i))
					continue;

				SeteoCampos(0, 0, 0, 0, v_i);

			}

			// Recalculo lo del mismo legajo
			for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
				if (FmLFld(fm0, NROLEG, row)!=FmLFld(fm0, NROLEG, v_i))
					continue;

				if (FmIsNull(fm, HSENTRE, v_i) || FmIsNull(fm, HSSAL, v_i))
					continue;

				fm4 = UseSubform(fm0, PTO, 0, v_i);

				sprintf(v_tipvig, "%s", TipoVig(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
                                FmLFld(fm0, NROLEG, v_i), FmIFld(fm4, TIPPTO),FmIFld(fm4, CODINT), 
                                FmIFld(fm4, NROINT), FmDFld(fm0, FECPARTE)));


				CalDetHorPer2(TRUE, fm0, v_i, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), 
	                       FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG, v_i), FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), 
                           FmIFld(fm4, NROINT), *FmSFld(fm0, COND, v_i), FmTFld(fm, HSENTRE, v_i), FmTFld(fm, HSSAL, v_i), &g_hsnor, &g_hs50, &g_hs100f, &g_hs100fe);


//				fprintf(stderr, "A %d\t%d\t%d\t%d\t%d\n",g_hsnor, g_hs50, g_hs100f, g_hs100fe, v_i);
				SeteoCampos(g_hsnor, g_hs50, g_hs100fe, g_hs100f + g_hspega, v_i);



			}
		}
		break;
	case AGRUPHS:
//		fm1 = UseSubform(fm, DETHS, 0, row);

		if (FmChgFld(fm) || !renglon_ok) {
			fm4 = UseSubform(fm, PTO,   0, row);
			cambio = TRUE;

			if (WarPue(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE), FmLFld(fm, CLIE),
					   FmIFld(fm, OBJET), FmTFld(fm, HSENTRE, row), FmTFld(fm, HSSAL, row), &franco, cadena)) {
				if (franco)
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1047), WAR_SUPERPOSF, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)), cadena);
				else
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1047), WAR_SUPERPOS, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)), cadena);
			}

			BorrarExcepciones (row);
			if (!FmIFld(fm, I_EXTASIG, row)) {
				if (SupPue(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE), FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmTFld(fm, HSENTRE, row), 
				    FmTFld(fm, HSSAL, row))){
					if (!g_GRPSUPOPER) {
						WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1018),ERR_SUPERPOS, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)),
						         LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO), TFld(APARTE_HORAENT), TFld(APARTE_HORASAL), row);
						renglon_ok = FALSE;
						return FM_ERROR;
					}
					else {
						if (WiDialog(WD_OK|WD_NO, WD_OK, TituloMsg(TMSG_WAR, 1018), WAR_SUPERPOS2, GetNombreLeg(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row)),
						             LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO), TFld(APARTE_HORAENT), TFld(APARTE_HORASAL), row)==WD_NO) {
							renglon_ok = FALSE;
							return FM_ERROR;
						} 
					}
				}
			}


			SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET));
			GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);




			// Limpio lo del mismo legajo
			for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
				if (FmLFld(fm0, NROLEG, row)!=FmLFld(fm0, NROLEG, v_i))
					continue;

				SeteoCampos(0, 0, 0, 0, v_i);

			}

			// Recalculo lo del mismo legajo
			for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
				if (FmLFld(fm0, NROLEG, row)!=FmLFld(fm0, NROLEG, v_i))
					continue;

				fm4 = UseSubform(fm0, PTO, 0, v_i);

				sprintf(v_tipvig, "%s", TipoVig(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
                                FmLFld(fm0, NROLEG, v_i), FmIFld(fm4, TIPPTO),FmIFld(fm4, CODINT), 
                                FmIFld(fm4, NROINT), FmDFld(fm0, FECPARTE)));


				CalDetHorPer2(TRUE, fm0, v_i, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), 
	                       FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG, v_i), FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), 
                           FmIFld(fm4, NROINT), *FmSFld(fm0, COND, v_i), FmTFld(fm, HSENTRE, v_i), FmTFld(fm, HSSAL, v_i), &g_hsnor, &g_hs50, &g_hs100f, &g_hs100fe);

//				fprintf(stderr, "PARTE legajo %d cond %s g_hsnor %d, g_hs50 %d, g_hs100f %d, g_hs100fe %d, g_hspega %d\n", FmLFld(fm0, NROLEG, v_i), FmSFld(fm0, COND, v_i), g_hsnor, g_hs50, g_hs100f, g_hs100fe, g_hspega);

				SeteoCampos(g_hsnor, g_hs50, g_hs100fe, g_hs100f + g_hspega, v_i);
			}
		}
		break;
	case AGRUPLIN: 
		if (FmKeyCode(fm) != K_DEL)
			if (!ControlesNivelLinea(row, FmChgFld(fm))) 
    		 	return FM_REDO;

		break;
	case NROLEG:
		switch(FmKeyCode(fm)) {
			case K_HELP:
				HelpLegajo(fm, fno, row);
				break;
/* Comentado porque no compila denarius
			case K_META:
				v_nroleg = ERROR;
				if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;

				FmSetLFld(fm, fno, v_nroleg, row);
				break;
*/
		}
		if (!FmIsNull(fm, fno, row)) {

			if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG, row), FmDFld(fm0, FECPARTE)))
				if (!FmIsNull(fm, fno, row)) {
					WiDialog(WD_OK, WD_OK, "Error", "Legajo Inactivo");
					return FM_REDO;
				}
			// Control de baja temprana
			SetKey(sue|PER, FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row));
			if (GetRecord(sue|PER, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				if (IFld(sue|PER_ACTIVO) == 2 && DFld(sue|PER_FECEGR) < FmDFld(fm0, FECPARTE)) {
					WiDialog(WD_OK, WD_OK, "Error", "El Legajo %d se encuentra inactivo a partir del dia %D", FmLFld(fm, NROLEG, row), DFld(sue|PER_FECEGR));
					FmClearFlds(fm0, NROLEG, NROLEG, row);
					return FM_REDO;
				}
			}
		}

	  	FmSetFld(fm, NOMBRE, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row)), row);
		GetRegimenEfectivo(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), regimen, FmDFld(fm0, FECPARTE));
		if (strcmp(regimen, NULL_STR) != 0) {
			newvig = TRUE;
			FmSetFld (fm, REGIM,     regimen, row);
			FmSetIFld(fm, I_VIGINEW, TRUE,    row);
		}

		if (FmChgFld(fm)) {
			SeteoCampos(0, 0, 0, 0, row);
		}
		break;
	case PTO:
		fm4 = UseSubform(fm, fno, 0, row);
		if (*FmSFld(fm, COND, row) == 'T' || *FmSFld(fm, COND, row) == 'P' || FmIsNull(fm, COND, row)) {
			if ((FmKeyCode(fm) == K_META || newvig || FmIsNull(fm4, TIPPTO)) && FmKeyCode(fm)!= K_DEL) {

				// Guardo valores previos de puesto
				g_tippto=FmIFld(fm4, TIPPTO);
				g_codint=FmIFld(fm4, CODINT);
				g_nroint=FmIFld(fm4, NROINT);
//fprintf (stderr, "row %d nroint %d\n", row, g_nroint);
				DoSubform(fm, NULLFP, after_partep4, fno, 0, row);
//fprintf (stderr, "row %d NROINT %d\n", row, FmIFld(fm4, NROINT));
				if (objconot) {
					return FM_REDO;
				}
				newvig = FALSE;
			}
		}
		break;

	case HSENTRE:
	case HSSAL:
		if (FmChgFld(fm)){
			FmClearFlds(fm0, HSTOT, HSTOT, row);
			if (!FmIsNull(fm0, HSSAL, row) && !FmIsNull(fm0, HSENTRE, row)){
				FmSetIFld(fm0, HSTOT, ConvHraInt(FmTFld(fm0, HSENTRE, row), FmTFld(fm0, HSSAL, row)) * 100, row);
			}
		}
		break;

	case AGRUGRAL:
		FmSetIFld(fm, HSSTD,   FmLFld(fm, TOTAL) - HsExcep(FACTURABLE) - HsExcep(ACARGO_EMP));
		FmSetIFld(fm, HSFAC,   HsExcep(FACTURABLE));
		FmSetIFld(fm, HSACEMP, HsExcep(ACARGO_EMP));

		/* Esto lo pongo por si cambian el horario despues de haber dado superposicion,
		   => errorhs queda con TRUE y el mensaje sigue saliendo. */
		if (errorhs) 
			errorhs = FALSE;

		if (ExisteCliObjEnGrp(GRPRETENES, FmLFld(fm, CLIE), FmIFld(fm, OBJET)))
			break;

		if (ValidarHsTot(FmLFld(fm, STD), FmLFld(fm, TOTAL)) &&
			ConsideraStd(FmLFld(fm, CLIE), FmIFld(fm, OBJET))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1020), 
			         "Hay exceso de horas.\nVerifique los datos del parte.");
			errorhs = TRUE;
			return FM_ERROR;
		}
 		if (FmLFld(fm, STD) > FmLFld(fm, HSSTD))
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1021), "Existen %.2f horas caídas del Standard.", 
			        ((double)FmLFld(fm, STD) - (double)FmLFld(fm, HSSTD)) / 100);
		break;

	case DETHS:
		g_row=row;

		if (FmKeyCode(fm) == K_META)
			DoSubform(fm, before_parte1, NULLFP, fno, 0, row);
		break;

	case EXCP :
		if (FmKeyCode(fm) == K_META)
			DoSubform(fm, NULLFP, after_partep2, fno, 0, row);

		if ((!FmIFld(fm, I_EXTASIG, row) && FmIFld(fm0, I_VIGINEW, row)) || (cambio)) {
			if (PueOcup(FmIFld(fm, EMP), FmDFld(fm, FECPARTE), FmLFld(fm, CLIE), FmIFld(fm, OBJET),
						FmTFld(fm, HSENTRE, row), FmTFld(fm, HSSAL, row), FmIFld(fm4, TIPPTO),
						FmIFld(fm4, CODINT), &nroleg, &hspto, row) &&
				GetServicioObj(FmLFld(fm, CLIE), FmIFld(fm, OBJET)) != BRIGADA) {
				fm4 = UseSubform(fm, PTO,   0, row);
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1019), ERR_PUEOCUP, FmIFld(fm4, TIPPTO), 
				         FmIFld(fm4, CODINT), ((double)hspto)/100, row);

				FmNextFld(fm0, PTO, row_padre);
				break;
			}
			cambio = FALSE;
		}
		break;
	}

	return FM_OK;
}

static fm_status before_parte1(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NORMAL  :
	case EXTRAS1 :
	case EXTRAS2 :
	case FRANCOS :
		if (!g_GRPSUPOPER && !g_GRPBRIG)
			return FM_SKIP;
//		if (*FmSFld(fm0, COND, g_row) == _PEGADA_C)
//			return FM_SKIP;
		break;
	}
	return FM_OK;
}

static fm_status after_partep2(form fm, fmfield fno, int row)
{
	int v_i, hsnor = 0, hs50 = 0, hs100 = 0;

	switch (fno) {
	case AGRUPO:
		if (!ValidoClienteExcepcion (FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), 
									 FmIFld (fm, TIPFAC, row),  FmIFld (fm, MOTIVO, row),
									 FmIFld (fm, VALCLI, row))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1032), 
			"El cliente objetivo no esta habilitado a usar este motivo de excepcion");
			return FM_REDO;
		}
		break;
	case CONTROL_FLD :
		fm1 = UseSubform(fm0, DETHS, 0, row_padre);
		for (v_i=0; v_i < FmFldLen(fm, MULTI) && !FmIsNull(fm, TIPFAC, v_i); v_i++) {
			hsnor += FmIFld(fm, HORAS, v_i);
			hs50  += FmIFld(fm, HS50,  v_i);
			hs100 += FmIFld(fm, HS100, v_i);
		}
		if (hsnor > FmIFld(fm1, NORMAL)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1021), ERR_HORAS, "normales");
			FmNextFld(fm, TIPFAC, 0);
		}
		if (hs50  > FmIFld(fm1, EXTRAS1)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1022), ERR_HORAS, "extras al 50%");
			FmNextFld(fm, TIPFAC, 0);
		}
		if (hs100 > FmIFld(fm1, EXTRAS2) + FmIFld(fm1, FRANCOS)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1023), ERR_HORAS, "extras al 100%");
			FmNextFld(fm, TIPFAC, 0);
		}
		break;
	}
	return FM_OK;
}

static fm_status after_partep4(form fm, fmfield fno, int row)
{
	dbcursor  c_puesto; // c_ptoser;
	int  n;
	char dia[3];

	switch (fno) {
	case TIPPTO :
		if(FmKeyCode(fm) == K_HELP)	{
			c_puesto = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
			SetCursorFrom(c_puesto, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MIN_SHORT, MIN_SHORT);
			SetCursorTo  (c_puesto, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MAX_SHORT, MAX_SHORT);
			n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_puesto, 3, validate, display);

			if ( n >= 0 ) {

				FmSetIFld(fm, fno,     IFld(operac|PUESTOS_TIPPTO));
				FmSetFld (fm, DTIPPTO, SFld(comerc|TPTOSER_DESCOR));
			}
		}
		break;
	case CODINT :
		if (FmKeyCode(fm)==K_HELP) 
			HelpPto(fm, fno, row, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO),
			        FmDFld(fm0, FECPARTE), NULL_STR, NULL_STR);

		if (FmIsNull(fm, NROINT) || FmChgFld(fm)) { 
			FmSetIFld(fm, NROINT, CalcNroint(FmIFld(fm, I_EMP), FmLFld(fm, I_CLIEOT), FmIFld(fm, I_OBJET),
			                                 FmLFld(fm, I_NROLEG), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT)));
//WiMsg("setea %d %d %d %d", FmLFld(fm, I_NROLEG), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT), FmIFld(fm, NROINT));
		}
		objconot = FALSE;
		SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));
		if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (DFld(operac|PUESTOS_FFINAL) != NULL_DATE &&
				DFld(operac|PUESTOS_FFINAL) < FmDFld(fm0, FECPARTE)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1023),"No existe el puesto %d %d.",
				         FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));
				if (ObjConOt(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) {
					FmNextFld(fm, TIPPTO);
					objconot = TRUE;
				}
			}
			sprintf(dia, "%1.1s", DiaLetra(FmDFld(fm0, FECPARTE)));
			if (strcmp(dia, SFld(operac|PUESTOS_DIA1)) && strcmp(dia, SFld(operac|PUESTOS_DIA2)) &&
				strcmp(dia, SFld(operac|PUESTOS_DIA3)) && strcmp(dia, SFld(operac|PUESTOS_DIA4)) &&
				strcmp(dia, SFld(operac|PUESTOS_DIA5)) && strcmp(dia, SFld(operac|PUESTOS_DIA6)) &&
				strcmp(dia, SFld(operac|PUESTOS_DIA7)) && strcmp(DIAPTIME, SFld(operac|PUESTOS_DIA1))) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1024), ERR_PTODIA, FmIFld(fm, TIPPTO), 
				         FmIFld(fm, CODINT), DayName(FmDFld(fm0, FECPARTE)), row);

				if (ObjConOt(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) {
					FmClearFlds(fm, TIPPTO, NROINT);
					WiRefresh();
					FmNextFld(fm, TIPPTO);
					objconot = TRUE;
				}
			}
		}
		FmSetIFld(fm0, I_PTOSER, FmIFld(fm, TIPPTO), row_padre);
		FmSetIFld(fm0, I_PUESTO, FmIFld(fm, CODINT), row_padre);
		FmSetIFld(fm0, I_NROINT, FmIFld(fm, NROINT), row_padre);

		break;
	case AGRUPP:

			if (FmIsNull(fm, TIPPTO) ||  FmIsNull(fm, CODINT)){
				// Restauro valores anteriores del puesto
//DHC			FmSetIFld(fm4, TIPPTO, g_tippto);
//				FmSetIFld(fm4, CODINT, g_codint);
//				FmSetIFld(fm4, NROINT, g_nroint);
//fprintf(stderr, "paso");
				FmSetIFld(fm, TIPPTO, g_tippto);
				FmSetIFld(fm, CODINT, g_codint);
				FmSetIFld(fm, NROINT, g_nroint);

				if (WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_WAR, 1040),
				    "No se cargo el puesto desea volver a intentarlo?\n Linea %d", row_padre)==WD_YES) 
					 return FM_REDO;
				else {
					FmNextFld(fm, CONTROL_FLD);
					FmSetKeyCode(fm0, K_DEL);
				} 

			}
			else {
				if (!CorrespondeDiaPuesto(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), 
										  FmIFld(fm, TIPPTO), FmIFld(fm, CODINT), FmDFld(fm0, FECPARTE))) {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1025),
					         "El Tipo de día del puesto %d %d no corresponde con la fecha ingresada",
							 FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));

					return FM_REDO;
				}
			}


		break;		
	}

	return FM_OK;
}

static int validate()
{
	if(!BajaPuesto(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO), FmDFld(fm0, FECPARTE)))
		return TRUE;
	return FALSE;		
}

static void display(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(operac|PUESTOS_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(operac|PUESTOS_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}

private void PutInTables()
{
	dbcursor c_EX;
	int v_i, j;

	c_EX = CreateCursor(operac|EXCEPCIONbyEMP, IO_NOT_LOCK);

	DelVigilador();
	for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		InitRecord(operac|PARTE);

		fm1 = UseSubform(fm0, DETHS, 0, v_i);
		fm4 = UseSubform(fm0, PTO,   0, v_i);

			/*-------* Alta en Parte *--------*/
		SetIFld(operac|PARTE_EMP,      FmIFld(fm0, EMP));
		SetLFld(operac|PARTE_CLIENTE,  FmLFld(fm0, CLIE));
		SetIFld(operac|PARTE_OBJETIVO, FmIFld(fm0, OBJET));
		SetDFld(operac|PARTE_DIA,      FmDFld(fm0, FECPARTE));
		SetLFld(operac|PARTE_NROLEG,   FmLFld(fm0, NROLEG,  v_i));
		SetTFld(operac|PARTE_HORAENT,  FmTFld(fm0, HSENTRE, v_i));
		SetTFld(operac|PARTE_HORASAL,  FmTFld(fm0, HSSAL,   v_i));

		if (FmIFld(fm4, TIPPTO) != NULL_SHORT) {
			SetIFld(operac|PARTE_PTOSER, FmIFld(fm4, TIPPTO));
			SetIFld(operac|PARTE_PUESTO, FmIFld(fm4, CODINT));
			SetIFld(operac|PARTE_NROINT, FmIFld(fm4, NROINT));
		}
		else {
			SetIFld(operac|PARTE_PTOSER, FmIFld(fm0, I_PTOSER, v_i));
			SetIFld(operac|PARTE_PUESTO, FmIFld(fm0, I_PUESTO, v_i));
			SetIFld(operac|PARTE_NROINT, FmIFld(fm0, I_NROINT, v_i));
		}
				
		SetFld (operac|PARTE_CONDIC,  FmSFld(fm0, COND, v_i));
		SetIFld(operac|PARTE_CONFIR,  0);
		SetIFld(operac|PARTE_HSNOR,   FmIFld(fm1, NORMAL));
		SetIFld(operac|PARTE_HS50,    FmIFld(fm1, EXTRAS1));
		SetIFld(operac|PARTE_HS100F,  FmIFld(fm1, EXTRAS2));
		SetIFld(operac|PARTE_HS100FE, FmIFld(fm1, FRANCOS));


					/*-------* Alta en asist_asisten *--------*/
		if (*FmSFld(fm0, COND, v_i) == 'A') {
			fm3 = UseSubform(fm0, COND, 0, v_i);
	
			SetIFld(operac|PARTE_CODAUS,  FmIFld(fm3, CODNOV));
			SetIFld(asist|LICEN_EMP,    FmIFld(fm0, EMP));
			SetLFld(asist|LICEN_NROLEG, FmLFld(fm0, NROLEG, v_i));
			SetIFld(asist|LICEN_LICEN,  MIN_SHORT);
			SetDFld(asist|LICEN_FECHAD, MIN_DATE);
			if (GetRecord(asist|LICENbyTIPLIC, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
				if (FmDFld(fm0, FECPARTE) < DFld(asist|LICEN_FECHAD) ||
					FmDFld(fm0, FECPARTE) > DFld(asist|LICEN_FECHAH)) {
					InitRecord(asist|ASISTEN);
					SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
					SetDFld(asist|ASISTEN_FECHA,  FmDFld(fm0, FECPARTE));
					SetLFld(asist|ASISTEN_NROLEG, FmLFld(fm0, NROLEG, v_i));
					SetIFld(asist|ASISTEN_CODNOV, FmIFld(fm3, CODNOV));
					SetLFld(asist|ASISTEN_VALOR,  100);
					SetIFld(asist|ASISTEN_JUSTIF, TRUE); /* seteo con TRUE para que en ASISTEN_JUSTIF sea un NO */
					PutRecord(asist|ASISTEN);
				}
			}
			else {
				InitRecord(asist|ASISTEN);
				SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
				SetDFld(asist|ASISTEN_FECHA,  FmDFld(fm0, FECPARTE));
				SetLFld(asist|ASISTEN_NROLEG, FmLFld(fm0, NROLEG, v_i));
				SetIFld(asist|ASISTEN_CODNOV, FmIFld(fm3, CODNOV));
				SetLFld(asist|ASISTEN_VALOR,  100);
				SetIFld(asist|ASISTEN_JUSTIF, TRUE); /* seteo con TRUE para que en ASISTEN_JUSTIF sea un NO */
				PutRecord(asist|ASISTEN);
			} 
		}
//WiMsg("nroint %d %d ", IFld(operac|PARTE_NROINT), v_i);
		AudiGrabaHorasParte(operac, g_prog);

		/* Lo grabo aca porque graba CODAUS dentro del if de AUSENTISMO */
		PutRecord(operac|PARTE);

				/*-------* Alta en Asigh *--------*/
		/* Grabo en Asigh porque la asignación es por un día (La fecha del Parte) aquel legajo que inserte
		   porque vino como novedad en el parte y por ser provisorio no lo tengo asignado al cli/obj/puesto
		   que se esta cargando en el parte */
		if (!FmIFld(fm0, I_EXTASIG, v_i)) {
			InitRecord(operac|ASIGH);

			SetKey(operac|ASIGHbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
			                          FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), FmIFld(fm4, NROINT),
			                          FmLFld(fm0, NROLEG, v_i), FmDFld(fm0, FECPARTE), FmDFld(fm0, FECPARTE));
			if (GetRecord(operac|ASIGHbyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR ||
						TFld(operac|ASIGH_HSENT) != FmTFld(fm0, HSENTRE, v_i) ||
						TFld(operac|ASIGH_HSSAL) != FmTFld(fm0, HSSAL,   v_i)) {

				SetFld (operac|ASIGH_VIGIL,    "V");
				SetFld (operac|ASIGH_EFECT,    "P");
				SetDFld(operac|ASIGH_FECHAS,   FmDFld(fm0, FECPARTE));
				SetTFld(operac|ASIGH_HSENT,    FmTFld(fm0, HSENTRE, v_i));
				SetTFld(operac|ASIGH_HSSAL,    FmTFld(fm0, HSSAL,   v_i));
				SetFld (operac|ASIGH_REGIM,    FmSFld(fm0, REGIM, v_i));
				SetIFld(operac|ASIGH_MOTIVO,   ALTAPARTE);

				
				switch (dia(FmDFld(fm0, FECPARTE))) { 
				case 'L' : 
					SetFld(operac|ASIGH_DIA1, LUNES);
					break;
				case 'M' : 
					SetFld(operac|ASIGH_DIA2, MARTES);
					break;
				case 'X' : 
					SetFld(operac|ASIGH_DIA3, MIERCOLES);
					break;
				case 'J' : 
					SetFld(operac|ASIGH_DIA4, JUEVES);
					break;
				case 'V' : 
					SetFld(operac|ASIGH_DIA5, VIERNES);
					break;
				case 'S' : 
					SetFld(operac|ASIGH_DIA6, SABADO);
					break;
				case 'D' : 
					SetFld(operac|ASIGH_DIA7, DOMINGO);
					break;
				}
				PutRecord(operac|ASIGH);
			}
		}
		/*-------* Alta en Excepciones *--------*/
		fm2 = UseSubform(fm0, EXCP, 0, v_i);

		SetCursorFrom(c_EX, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmDFld(fm0, FECPARTE),
							FmLFld(fm0, NROLEG, v_i), FmIFld(fm0, I_PTOSER, v_i), FmIFld(fm0, I_PUESTO, v_i),
							FmIFld(fm0, I_NROINT, v_i), MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_EX, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmDFld(fm0, FECPARTE),
							FmLFld(fm0, NROLEG, v_i), FmIFld(fm0, I_PTOSER, v_i), FmIFld(fm0, I_PUESTO, v_i),
							FmIFld(fm0, I_NROINT, v_i), MAX_SHORT, MAX_SHORT);
		while(FetchCursor(c_EX) != ERROR)
		DelRecord(operac|EXCEPCION);

		for (j=0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {
			SetIFld(operac|EXCEPCION_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|EXCEPCION_CLIENTE,  FmLFld(fm0, CLIE));
			SetIFld(operac|EXCEPCION_OBJETIVO, FmIFld(fm0, OBJET));
			SetDFld(operac|EXCEPCION_DIA,      FmDFld(fm0, FECPARTE));
			SetLFld(operac|EXCEPCION_NROLEG,   FmLFld(fm0, NROLEG, v_i));
			SetIFld(operac|EXCEPCION_CONDIC,   FmIFld(fm2, TIPFAC, j));
			SetIFld(operac|EXCEPCION_MOTIVO,   FmIFld(fm2, MOTIVO, j));
			SetIFld(operac|EXCEPCION_HORAS,    FmIFld(fm2, HORAS,  j));
			SetIFld(operac|EXCEPCION_HS50,     FmIFld(fm2, HS50,   j));
			SetIFld(operac|EXCEPCION_HS100,    FmIFld(fm2, HS100,  j));
			SetFld (operac|EXCEPCION_OBS,      FmSFld(fm2, OBS,    j));

			if (FmIFld(fm0, I_VIGINEW, v_i)) {
				SetIFld(operac|EXCEPCION_PTOSER, FmIFld(fm4, TIPPTO));
				SetIFld(operac|EXCEPCION_PUESTO, FmIFld(fm4, CODINT));
				SetIFld(operac|EXCEPCION_NROINT, FmIFld(fm4, NROINT));
			}
			else {
				SetIFld(operac|EXCEPCION_PTOSER, FmIFld(fm0, I_PTOSER, v_i));
				SetIFld(operac|EXCEPCION_PUESTO, FmIFld(fm0, I_PUESTO, v_i));
				SetIFld(operac|EXCEPCION_NROINT, FmIFld(fm0, I_NROINT, v_i));
			}
			PutRecord(operac|EXCEPCION);
		}
	}
	DeleteCursor(c_EX);
}

private void DelVigilador()
{
	   /*--------* Borrar operac_parte *---------*/
	SetIFld(operac|PARTE_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|PARTE_CLIENTE,  FmLFld(fm0, CLIE));
	SetIFld(operac|PARTE_OBJETIVO, FmIFld(fm0, OBJET));
	SetDFld(operac|PARTE_DIA,      FmDFld(fm0, FECPARTE));
	SetLFld(operac|PARTE_NROLEG,   MIN_LONG);
	SetIFld(operac|PARTE_PTOSER,   MIN_SHORT);
	SetIFld(operac|PARTE_PUESTO,   MIN_SHORT);
	SetIFld(operac|PARTE_NROINT,   MIN_SHORT);
	while (GetRecord(operac|PARTEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 4) != ERROR) {
		/*-------* Borrar operac_excepcion *--------*/
		SetIFld(operac|EXCEPCION_EMP,       FmIFld(fm0, EMP));
		SetLFld(operac|EXCEPCION_CLIENTE,   FmLFld(fm0, CLIE));
		SetIFld(operac|EXCEPCION_OBJETIVO,  FmIFld(fm0, OBJET));
		SetDFld(operac|EXCEPCION_DIA,       FmDFld(fm0, FECPARTE));
		SetLFld(operac|EXCEPCION_NROLEG,    LFld(operac|PARTE_NROLEG));
		SetIFld(operac|EXCEPCION_PTOSER,    MIN_SHORT);
		SetIFld(operac|EXCEPCION_PUESTO,    MIN_SHORT);
		SetIFld(operac|EXCEPCION_NROINT,    MIN_SHORT);
		SetIFld(operac|EXCEPCION_CONDIC,    MIN_SHORT);
		SetIFld(operac|EXCEPCION_MOTIVO,    MIN_SHORT);
		while (GetRecord(operac|EXCEPCIONbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 5) != ERROR)
			DelRecord(operac|EXCEPCION);

		/*-------* Borrar asist_asisten *--------*/
		if (BorrarAsisten(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), FmDFld(fm0, FECPARTE),
						  FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) {
			SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
			SetDFld(asist|ASISTEN_FECHA,  FmDFld(fm0, FECPARTE));
			SetLFld(asist|ASISTEN_NROLEG, LFld(operac|PARTE_NROLEG));
			SetIFld(asist|ASISTEN_CODNOV, MIN_SHORT);
			while (GetRecord(asist|ASISTENbyEMPRE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
				DelRecord(asist|ASISTEN);
		}

		DelRecord(operac|PARTE);
	}
}

private void SeteoCampos(int hsnor, int hs50, int hs100f, int hs100fe, int p_linea)
{
	form fmaux;
	fmaux = UseSubform(fm0, DETHS, 0, p_linea);

	FmSetIFld(fmaux, NORMAL,  hsnor);
	FmSetIFld(fmaux, EXTRAS1, hs50);
	FmSetIFld(fmaux, EXTRAS2, hs100fe);
	FmSetIFld(fmaux, FRANCOS, hs100f);
	
}

static bool ValidarHsTot(long std, long p_hstot)
{
	int v_i, j, excep = 0;
	short tipoexc;

	if (p_hstot > std) {
		for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
			fm2 = UseSubform(fm0, EXCP, 0, v_i);
			for (j = 0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {

				tipoexc = ParteTipoExcepcion(FmIFld (fm2, TIPFAC, j), FmIFld(fm2, MOTIVO, j));
				/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
				if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
					continue;
				}

				excep += FmIFld(fm2, HORAS, j) + FmIFld(fm2, HS50, j) + FmIFld(fm2, HS100, j);
			}
		}

		if (p_hstot > (std + excep))
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

static bool PueOcup(int emp, DATE fecparte, long v_cliente, int obj, TIME hsent, TIME hssal,
					int ptoser, int puesto, long *p_nroleg, int *p_hspto, int row)
{
	bool ocupado = FALSE;
	int v_i, vig = 0, horas = 0, francos = 0, cexcep = 0;
	nptos *aux;
	bool encontro; 

	*p_nroleg = 0;
	InsertarPuesto(v_cliente, obj);

	// Acumulo cantidad de francos, horas del parte y cantidad total de lineas
	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		fm4 = UseSubform(fm0, PTO, 0, v_i);		

		if (FmIFld(fm4, TIPPTO) == ptoser  && FmIFld(fm4, CODINT) == puesto) {
			if (*FmSFld(fm0, COND, v_i) == 'F')
				francos++;
			if (*FmSFld(fm0, COND, v_i) == 'A')
				continue;

			horas += ConvHraInt(FmTFld(fm0, HSENTRE, v_i), FmTFld(fm0, HSSAL, v_i)) * 100;

			vig++;
		}
	} 

	// Me posiciono en lista en puesto 
	encontro=FALSE; 
	for (aux = listapto; aux != NULL; aux = aux->sgte) {
		if (aux->ptoser == ptoser && aux->puesto == puesto) {
			encontro=TRUE; 
			break;
		}
	}
	if (!encontro){
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1044), "No se encontro el puesto %d-%d", ptoser, puesto);
		return FALSE;
	}
	
	/* Calculo excepciones porque el total de hs trabajadas - excep <= Cant hs del puesto */
	cexcep = ObtenerExcep(ptoser, puesto);

	if (vig > aux->cantvig && (aux->ffinal != NULL_DATE && aux->ffinal < FmDFld(fm0, FECPARTE)))
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1026), WAR_PTOOCUP, aux->ptoser, aux->puesto, row);

	if (horas - cexcep > aux->horas * (aux->cantpue == 0 ? 1 : aux->cantpue)) 
		ocupado = TRUE;

	/* Valido con el std porque puede pasar que den de baja un puesto cuando tenia 3 (osea me quedan 2)
	** y quieran insertar un vig en una fecha cuando habia 3 y me rebota porque tiene mas hs ese dia 
	** (que son 3 pto) que la que dice el pto. (que tiene 2) */
	if (ocupado) {
		if (horas - cexcep <= StdHr(emp, v_cliente, obj, aux->ptoser, FmDFld(fm0, FECPARTE), FmDFld(fm0, FECPARTE)))
			ocupado = FALSE;
	}
	*p_hspto =aux->horas * (aux->cantpue == 0 ? 1 : aux->cantpue);
	FreeListaPtos();
	return ocupado;
}

private int ObtenerExcep(int ptoser, int puesto)
{
	int cantexcep = 0, v_i, j;

	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		fm2 = UseSubform(fm0, EXCP, 0, v_i);
		fm4 = UseSubform(fm0, PTO, 0, v_i);

		if (ptoser == FmIFld(fm4, TIPPTO) && puesto == FmIFld(fm4, CODINT)) {
			for (j = 0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++)
				cantexcep += FmIFld(fm2, HORAS, j) + FmIFld(fm2, HS50, j) + FmIFld(fm2, HS100, j);
		}
	}
	return cantexcep;
}

private int HsExcep(int tiphs)
{
	int hsfac = 0, v_i, j;

	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		fm2 = UseSubform(fm0, EXCP, 0, v_i);
		for (j = 0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {
 			if (FmIFld(fm2, TIPFAC, j) == tiphs)
				hsfac += FmIFld(fm2, HORAS, j) + FmIFld(fm2, HS50, j) + FmIFld(fm2, HS100, j);
		}
	}
	return hsfac;
}


static bool SupPueFm(long p_nroleg, TIME hsent, TIME hssal, int fila, TIME * hsentre, TIME * hssalida)
{
	int  v_i;
	bool haysup = FALSE;

	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		if (FmLFld(fm0, NROLEG, v_i) != p_nroleg)
			continue;

		if (v_i == fila)
			continue;

		//El puesto comieza despues que el del form
		if (FmTFld(fm0, HSENTRE, v_i) < FmTFld(fm0, HSSAL, v_i) && FmTFld(fm0, HSSAL, v_i) <= hsent) 
			continue;

		//El puesto termina antes que empiece el del form
		if (FmTFld(fm0, HSENTRE, v_i) < FmTFld(fm0, HSSAL, v_i) &&
			hssal <= FmTFld(fm0, HSENTRE, v_i) && hsent >= FmTFld(fm0, HSSAL, v_i)) 
			continue;

		haysup = Superposicion(hsent, hssal, FmTFld(fm0, HSENTRE, v_i), FmTFld(fm0, HSSAL, v_i), FALSE);

		if (!haysup)
			continue;
		*hsentre  = FmTFld(fm0, HSENTRE, v_i);
		*hssalida = FmTFld(fm0, HSSAL, v_i);
		break;
	}
	return haysup;
}


static void InsertarPuesto(long p_cliente, int obj)
{
	dbcursor c_pto;
	nptos *aux = NULL;

	c_pto = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom(c_pto, p_cliente, obj, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_pto, p_cliente, obj, MAX_SHORT, MAX_SHORT);

	while (FetchCursor(c_pto) != ERROR) {
		bool encontro; 
		short cantpue, cantvig;

		if (!IsNull(operac|PUESTOS_FFINAL) && FmDFld(fm0, FECPARTE) > DFld(operac|PUESTOS_FFINAL))
			continue;
		if (FmDFld(fm0, FECPARTE) < DFld(operac|PUESTOS_FINICIO))
			continue;

		if ((aux = (nptos *) Alloc (sizeof(nptos))) == NULL)
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1034), "No se puede alocar memoria");

		aux->sgte    = NULL;
		aux->ptoser  = IFld(operac|PUESTOS_TIPPTO);
		aux->puesto  = IFld(operac|PUESTOS_CODINT);
		aux->horas   = ConvHraInt(TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL)) * 100;
		aux->cantvig = IFld(operac|PUESTOS_CANTVIG)/100;
		encontro = StdPuesto(FmIFld(fm0, EMP), LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET),
							 IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_PUESTO),
							 SFld(operac|PUESTOS_REGIM),  TFld(operac|PUESTOS_HINICIO),
							 TFld(operac|PUESTOS_HFINAL), FmDFld (fm0, FECPARTE), &cantpue, &cantvig, TRUE,
							 FmDFld (fm0, FECPARTE), NULL_SHORT);

		/*Si se cargo por OT pongo la cantidad calculada sino pongo directamente lo de puestos */
		if (encontro)
			aux->cantpue = cantpue;
		else
			aux->cantpue = IFld(operac|PUESTOS_CANTPUE);

		aux->ffinal  = DFld(operac|PUESTOS_FFINAL);

		if (listapto == NULL) { // para la primera vez!
			listapto = aux;
		}
		else {
			ultpto->sgte = aux;
		}
		ultpto = aux;
	}
	DeleteCursor(c_pto);
}

static void FreeListaPtos()
{
	nptos *recorre, *aux;

	for (recorre = listapto; recorre;) {
		aux     = recorre;
		recorre = recorre->sgte;
		Free(aux);
	}
	listapto = NULL;
	ultpto   = NULL;
}

static int CalcNroint(int emp, long p_cliente, int objetivo, long p_nroleg, int ptoser, int puesto)
{
	int nroint, v_i;

	nroint = GetNextNroint(emp, p_cliente, objetivo, p_nroleg, ptoser, puesto);

	for (v_i = 0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		if (p_nroleg != FmLFld(fm0, NROLEG, v_i))
			continue;
		fm4 = UseSubform(fm0, PTO, 0, v_i);
		if (ptoser != FmIFld(fm4, TIPPTO) || puesto != FmIFld(fm4, CODINT))
			continue;
		if (nroint <= FmIFld(fm4, NROINT))
			nroint = FmIFld(fm4, NROINT)+1;
	}
	return nroint;
}

void BorrarExcepciones (int fila) 
{
	form fmaux;
	int v_i;

	//Borro solo cuando la hora desde y hasta es de 0000 a 0000	
	if (FmTFld (fm0, HSENTRE, fila) != StrToT("0000") ||
		FmTFld (fm0, HSSAL,   fila) != StrToT("0000"))
		return;
		
	fmaux = UseSubform(fm0, EXCP, 0, fila);
	for (v_i = 0; v_i < FmFldLen(fmaux, MULTI) && !FmIsNull(fmaux, TIPFAC, v_i); v_i++) {
		FmClearFlds (fmaux, TIPFAC, OBS, v_i);		
	} 
}

bool ErrorSuperposicion ()
{
	int fila=0;
	TIME hsentre, hssalida;
	long v_nroleg;
	int  v_hspto;
	for (fila = 0; i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, fila); fila++) {
		if (SupPueFm(FmLFld(fm0, NROLEG, fila), FmTFld(fm0, HSENTRE, fila), FmTFld(fm0, HSSAL, fila), fila, &hsentre, &hssalida)) { 
			if (!g_GRPSUPOPER){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1027), ERR_SUPERPOS, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, fila)),
						  FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), hsentre, hssalida, fila);
				return TRUE;
			}
			else {
				if (WiDialog(WD_OK|WD_NO, WD_OK, TituloMsg(TMSG_WAR, 1027), WAR_SUPERPOS2, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, fila)),
				             FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), hsentre, hssalida, fila)==WD_NO){
					
					return TRUE;
				}
			}
		}

		if (SupPue(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, fila), FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmTFld(fm0, HSENTRE, fila), 
		           FmTFld(fm0, HSSAL, fila)) ) {
			if (!g_GRPSUPOPER){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1028), ERR_SUPERPOS, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, fila)),
									  LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO),TFld(APARTE_HORAENT),	TFld(APARTE_HORASAL), fila);
				return TRUE;
			}
			else {
				if (WiDialog(WD_OK|WD_NO, WD_OK, TituloMsg(TMSG_WAR, 1028), WAR_SUPERPOS2, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, fila)),
				             LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO), TFld(APARTE_HORAENT),    TFld(APARTE_HORASAL), fila)==WD_NO) 
					return TRUE;

			}
		}

		if (!FmIFld(fm0, I_EXTASIG, fila) && FmIFld(fm0, I_VIGINEW, fila)) {
			fm4 = UseSubform(fm0, PTO, 0, fila);
			if (PueOcup(FmIFld(fm0, EMP), FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmTFld(fm0, HSENTRE, fila), FmTFld(fm0, HSSAL, fila), 
			            FmIFld(fm4, TIPPTO),FmIFld(fm4, CODINT), &v_nroleg, &v_hspto, fila) && GetServicioObj(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET)) != BRIGADA) {

				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1029), ERR_PUEOCUP, FmIFld(fm4, TIPPTO),FmIFld(fm4, CODINT), ((double)v_hspto)/100, fila);

				return TRUE;
			}
		}

	}
	return FALSE;
}



static bool Ausente(int emp, long p_nroleg, DATE fecparte, long * p_cliente, int * p_objet, bool *p_vacac)
{
	dbcursor c_parte;

	c_parte = CreateCursor(AlInd(APARTE, operac|PARTEbyEMPLE), IO_NOT_LOCK);

	SetCursorFrom(c_parte, emp, p_nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, p_nroleg, fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (LFld(APARTE_CLIENTE) == FmLFld(fm0, CLIE) && IFld(APARTE_OBJETIVO) == FmIFld(fm0, OBJET))
			continue;

		if (*SFld(APARTE_CONDIC) == 'A') {
			*p_cliente = LFld(APARTE_CLIENTE);
			*p_objet   = LFld(APARTE_OBJETIVO);
			*p_vacac   = FALSE;
			DeleteCursor(c_parte);
			return TRUE;
		}
		if (*SFld(APARTE_CONDIC) == 'V') {
			*p_cliente = NULL_LONG;
			*p_objet   = NULL_SHORT;
			*p_vacac   = TRUE;
			DeleteCursor(c_parte);
			return TRUE;
		}
	}
	DeleteCursor(c_parte);
	return FALSE;
}

static bool SupPue(int emp, long p_nroleg, DATE fecparte, long p_cliente, int obj, TIME hsent, TIME hssal)
{
	dbcursor c_parte;
	bool haysup = FALSE;
	DATE fecfin = NULL_DATE; 

	c_parte = CreateCursor(AlInd(APARTE, operac|PARTEbyEMPLE), IO_NOT_LOCK);

	fecfin = hssal < hsent ? (fecparte+1) : fecparte;
	SetCursorFrom(c_parte, emp, p_nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, p_nroleg, fecfin, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		if ((LFld(APARTE_CLIENTE) == p_cliente && IFld(APARTE_OBJETIVO) == obj))
			continue;

		if (TFld(APARTE_HORAENT) == StrToT("0000") && TFld(APARTE_HORASAL) == StrToT("0000")) 
			if (*SFld(APARTE_CONDIC) == 'A' || *SFld(APARTE_CONDIC) == 'F') 
				continue;

		//El puesto comieza despues que el del parte termino
		if (fecparte == DFld(APARTE_DIA) && TFld(APARTE_HORAENT)  < TFld(APARTE_HORASAL) && 
			TFld(APARTE_HORASAL) <= hsent) 
			continue;
		//El puesto termina antes que empiece el del parte
		if (fecparte == DFld(APARTE_DIA) && hsent < hssal && hssal <= TFld(APARTE_HORAENT)) 
			continue;
		//Estoy en el dia posterior y termina antes que empiece el proximo
		if (fecparte < DFld(APARTE_DIA) && hssal <= TFld(APARTE_HORAENT)) 
			continue;

		haysup = Superposicion(hsent, hssal, TFld(APARTE_HORAENT), TFld(APARTE_HORASAL), FALSE);

		if (!haysup)
			continue;
		break;
	}
	DeleteCursor(c_parte);
	return haysup;
}

// Hace un control de que el campo COND sea el mismo en otros clientes objetivos 
static bool ErrPue(int emp, long p_nroleg, DATE fecparte, long p_cliente, int p_obj, char p_tipodia,
                   int p_ptoser, int p_puesto, char *p_cadena)
{
	dbcursor c_parte;
	bool malasig = FALSE;
	char auxc[25];
	sprintf(p_cadena, "%s", NULL_STR);
	sprintf(auxc, "%s", NULL_STR);

	c_parte = CreateCursor(AlInd(aparte, operac|PARTEbyEMPLE), IO_NOT_LOCK);
	SetCursorFrom(c_parte, emp, p_nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, p_nroleg, fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (LFld(AlFld(aparte,PARTE_CLIENTE))  == p_cliente && 
		    IFld(AlFld(aparte,PARTE_OBJETIVO)) == p_obj && 
		    IFld(AlFld(aparte,PARTE_PTOSER)) == p_ptoser && 
		    IFld(AlFld(aparte,PARTE_PUESTO)) == p_puesto)
			continue;

		if (*SFld(AlFld(aparte,PARTE_CONDIC)) == p_tipodia)
			continue;

		if ((*SFld(AlFld(aparte,PARTE_CONDIC)) == _PEGADA_C && p_tipodia == _TRABAJA_C)
			|| (*SFld(AlFld(aparte,PARTE_CONDIC)) == _TRABAJA_C && p_tipodia == _PEGADA_C))
			continue;
		if ((*SFld(AlFld(aparte,PARTE_CONDIC)) == _ADELANTO_C && p_tipodia == _TRABAJA_C)
			|| (*SFld(AlFld(aparte,PARTE_CONDIC)) == _TRABAJA_C && p_tipodia == _ADELANTO_C))
			continue;
		
		/* Esto es para que se pueda poner en forma "conciente" en ausente */
		if (*SFld(AlFld(aparte,PARTE_CONDIC))=='T' && 
		    TFld(AlFld(aparte,PARTE_HORAENT))==StrToT("00:00") && 
		    TFld(AlFld(aparte,PARTE_HORASAL))==StrToT("00:00"))
			continue;

		malasig=TRUE;

	 	sprintf(p_cadena, "%s%6ld %4d ", p_cadena, LFld(AlFld(aparte,PARTE_CLIENTE)),
				                                  IFld(AlFld(aparte,PARTE_OBJETIVO)));

		TToStr(TFld(AlFld(aparte,PARTE_HORAENT)), auxc, TFMT_SEPAR);
		sprintf(p_cadena, "%s%6s  ", p_cadena, auxc);

		TToStr(TFld(AlFld(aparte,PARTE_HORASAL)), auxc, TFMT_SEPAR);
		sprintf(p_cadena, "%s%6s %7s ", p_cadena, auxc, SFld(AlFld(aparte,PARTE_CONDIC))); 
                                                                                                 
  		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HSNOR)))  / 100);
		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS50)))   / 100);
		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS100F))) / 100);
		sprintf(p_cadena, "%s%6.2f\n", p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS100FE)))/ 100);
		
	}
	DeleteCursor(c_parte);

	if (malasig)
		sprintf(p_cadena, "%s[1mAdvertencia! Existen  condiciones de trabajo  que no son \"%c\"[0m \n", p_cadena, p_tipodia);

	return (malasig); 
}

bool NecesitaOt (int fila)
{
    char dia[3];
	form fmaux;
    
    /*No necesita ot */
	if (!ObjConOt(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET))) 
		return FALSE;
               
	/*Necesita OT veo si el puesto cumple con el dia ingresado */	              
	fmaux = UseSubform(fm0, PTO,   0, fila);
	SetLFld(operac|PUESTOS_CLIENTE, FmLFld(fm0, CLIE));
	SetIFld(operac|PUESTOS_OBJET,   FmIFld(fm0, OBJET));
 	SetIFld(operac|PUESTOS_TIPPTO,  FmIFld(fmaux,  TIPPTO));
	SetIFld(operac|PUESTOS_CODINT,  FmIFld(fmaux,  CODINT));
	if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		return TRUE;

	if (DFld(operac|PUESTOS_FFINAL) != NULL_DATE &&
		DFld(operac|PUESTOS_FFINAL) < FmDFld(fm0, FECPARTE))
		return TRUE;

	sprintf(dia, "%1.1s", DiaLetra(FmDFld(fm0, FECPARTE)));
	if (strcmp(dia, SFld(operac|PUESTOS_DIA1)) && strcmp(dia, SFld(operac|PUESTOS_DIA2)) &&
		strcmp(dia, SFld(operac|PUESTOS_DIA3)) && strcmp(dia, SFld(operac|PUESTOS_DIA4)) &&
		strcmp(dia, SFld(operac|PUESTOS_DIA5)) && strcmp(dia, SFld(operac|PUESTOS_DIA6)) &&
		strcmp(dia, SFld(operac|PUESTOS_DIA7)) && strcmp(DIAPTIME, SFld(operac|PUESTOS_DIA1))) {
			return TRUE;
	}
	return FALSE;
}

bool ValidoClienteExcepcion (long p_cliente, short objetivo, short cond, short motivo, bool valida)
{
	if (!valida)
		return TRUE;

	SetKey (operac|MOTXCLIbyCODCOND, cond, motivo, p_cliente, objetivo);
	if (GetRecord (operac|MOTXCLIbyCODCOND, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		return TRUE;
	}

	return FALSE;
}

//Borra asisten si no encontro Ausente en otros objetivos.
static bool BorrarAsisten(int emp, long p_nroleg, DATE fecparte, long p_cliente, int obj)
{
	schema   prev, v_operac;
	dbcursor c_parte;
	bool     encontro = FALSE, estaotrocli = FALSE;

	prev    = CurrentSchema();
	v_operac  = OpenSchema("operac", IO_EABORT);
	c_parte = CreateCursor(AlInd(APARTE, PARTEbyEMPLE), IO_NOT_LOCK);

	SetCursorFrom(c_parte, emp, p_nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, p_nroleg, fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (LFld(APARTE_CLIENTE) == p_cliente && IFld(APARTE_OBJETIVO) == obj)
			continue;

		estaotrocli = TRUE;
		if (*SFld(APARTE_CONDIC) == 'A')
			encontro = TRUE;
	}
	DeleteCursor(c_parte);
	SwitchToSchema(prev);

	if (!estaotrocli)
		return TRUE;

	if (estaotrocli && !encontro)
		return TRUE;

	return FALSE;
}



bool EstanCorriendoLaImportacionDeHoras()
{
    char param[20];
	short pcerrado;

	if (fecierre < FmDFld(fm0, FECPARTE)) {
		return FALSE;
	}

	strcpy(param, GetValParam(MOD_VIGI, PAR_EJECUTANDO_CIERRE));
	pcerrado=StrToI(param);
	return pcerrado;
}

static bool ControlesNivelFormulario()
{
	ControlConsistenciaHoras();

	/* si esta cerrado no permitir grabarlo */
	if (cerrado) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1005), WAR_CERRADO, fecierre);
		return FALSE;
	}
	if (cerradopar) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1006), WAR_CERRADO_PARCIAL, feparcial);
		return FALSE;
	}

	/* Si existe Superpocicion horaria */
	if (ErrorSuperposicion ()) 
		return FALSE;


	return TRUE;	
}



static void MuestraNrodeLineaActual(fmfield fno, int row)
{
	char v_auxi[20];

	if (FmInMult(fm0, fno)!=ERROR)
		sprintf(v_auxi, "¡¡¡Linea %03d¡¡¡", row);
	else 
		sprintf(v_auxi, "¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡", row);
	FmSetFld(fm0, LINEA, v_auxi);

	FmSetFld(fm0, MENSAJE , "¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡");

	
}


static bool ControlesNivelLinea(int row, bool p_cambiofm)
{
	short tothoras=0, v_errlin;
	TIME hsentre, hssalida;
	char v_auxtpto[10];

	if (FmIsNull(fm0, NROLEG, row))
		return TRUE;

	if (*FmSFld(fm0, COND, row) == _AUSENTE_C) {
		fm3 = UseSubform(fm0, COND, 0, row);

		SetKey(asist|ASISTEN, FmIFld(fm0, EMP), FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG, row), NULL_SHORT);
		while (GetRecord(asist|ASISTEN, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (FmIFld(fm3, CODNOV) != IFld(asist|ASISTEN_CODNOV)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1050), "Existe duplicidad de condicion de ausente.\nLegajo %d condicion %d", FmLFld(fm0, NROLEG, row), IFld(asist|ASISTEN_CODNOV));
				return FALSE;
			}
		}
	}

	fm1 = UseSubform(fm0, DETHS, 0, row);
	/**********WARNING*****************/

	if (p_cambiofm){
		/*Se fija si tiene o no el puesto */
		if ((*FmSFld(fm0, COND, row) == 'F' || *FmSFld(fm0, COND, row) == 'T') && NecesitaOt (row)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1015), ERR_PTODIA, FmIFld(fm4, TIPPTO), 
				         FmIFld(fm4, CODINT), DayName(FmDFld(fm0, FECPARTE)), row);

//				FmSetFld (fm0, COND,    "F",            row);
//				FmSetTFld(fm0, HSENTRE, StrToT("0000"), row);
//				FmSetTFld(fm0, HSSAL,   StrToT("0000"), row);
//				FmSetIFld(fm0, HSTOT,   0,              row);
//				FmNextFld(fm0, COND,    row);
		}

	}

	/* Control de Superpocicion */
	
	if (ErrorSuperposicion()) 
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1037), "No se podran grabar los datos por existir superposición.");


	fm4 = UseSubform(fm0, PTO, 0, row);
	// Hace un control de que el campo COND sea el mismo en otros clientes objetivos 
	if (ErrPue(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE),
		   FmIFld(fm0, OBJET), *FmSFld(fm0, COND, row), FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), cadena)) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1002), "Existen horas trabajadas en otro objetivo con distinta condicion de trabajo ( %s )\n%s\nLinea %d", FmSFld(fm0, COND, row), cadena, row);
	}
	else {
		if ((v_errlin = ErrPueFm(row)) != NULL_SHORT) {

			if (FmIsNull(fm0, NROLEG, v_errlin)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1043), "El legajo %d tiene una jornada pegada y no tiene una jornada Trabajada en el día.", FmLFld(fm0, NROLEG, row));
			}
			else {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1043), 
				"Existen horas trabajadas en este objetivo con distinta condicion de trabajo para el mismo legajo (%d)\n%s en linea %d  y\n%s en linea %d   ",
				FmLFld(fm0, NROLEG, row), FmSFld(fm0, COND, row), row, FmSFld(fm0, COND, v_errlin), v_errlin);
				if (*FmSFld(fm0, COND, row) == _AUSENTE_C || *FmSFld(fm0, COND, row) == _VACACIONES_C
					|| *FmSFld(fm0, COND, v_errlin) == _AUSENTE_C || *FmSFld(fm0, COND, v_errlin) == _VACACIONES_C) {
					return FALSE;
				}
			}
		}
		else{
			if (*FmSFld(fm0, COND, row)=='A') {

				fm2 = UseSubform(fm0, EXCP, 0, row);
				FmClearAllFlds(fm2);

				FmSetTFld(fm0, HSENTRE, StrToT("0000"), row);
				FmSetTFld(fm0, HSSAL,   StrToT("0000"), row);
				FmSetIFld(fm0, HSTOT,   0,              row);

				SeteoCampos(0, 0, 0, 0, row);
			}
		}

	} 


	/**********ERRORES*****************/
	fm4 = UseSubform(fm0, PTO, 0, row);

	if (!CorrespondeDiaPuesto(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
	                          FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), FmDFld(fm0, FECPARTE)) ){
		if (FmTFld(fm0, HSENTRE, row)!=StrToT("00:00") || FmTFld(fm0, HSSAL, row)!=StrToT("00:00")) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1041), "No Corresponde puesto cargado en el formulario\nTipo Pto. %d\nCod.Int %d\nLinea %d",
			FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), row);
			return FALSE;
		
		}

	}

	tothoras = FmIFld(fm0, HSTOT, row);

	/* Diferencia de Horas Vs Horas de fm1*/
	if (FmIsNull(fm1, NORMAL)) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1031), "Error en la carga de detalle de Horas\nLinea %d", row);
		return FALSE;
	}
	else {
		if(tothoras != (FmIFld (fm1, NORMAL)+FmIFld (fm1, EXTRAS1)+FmIFld (fm1, EXTRAS2)+FmIFld (fm1, FRANCOS))) {

			sprintf(v_auxtpto, "%d", FmIFld(fm4, TIPPTO));

//			if (!EsParNov(FmIFld(fm0, EMP), PARNOV_TPUHSNOR, FmDFld(fm0, FECPARTE), v_auxtpto)) {
		
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1030), 
			        "El total de horas no corresponde con la distribución.\nSim %.2f\n Dob %.2f\nDyM %.2f \nDyM Franco %.2f \n\n\nLinea %d",
			        (double)FmIFld(fm1, NORMAL)/100, (double)FmIFld(fm1, EXTRAS1)/100,
			        (double)FmIFld(fm1, EXTRAS2)/100, (double)FmIFld (fm1, FRANCOS)/100, row);
				return FALSE;
//			}
		}
	} 

	if (FmIsNull(fm0, REGIM, row)) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1036), 
		         "El Vigilador no está asignado en forma efectiva a ningún cliente.\nPara cargar horas, primero debe estar asignado en forma efectiva.\n Linea %d", row);
		return FALSE;
	}


	/* Se fija si un legejo esta de vacaciones o ausente ese dia en otro cliente u objetivo */
	if (FmIFld(fm0, I_VIGINEW, row) && p_cambiofm) {

		if (Ausente(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), FmDFld(fm0, FECPARTE), &cliente, &objet, &vacac)){
			if (vacac) 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1013), 
				         "El vigilador se encuentra de vacaciones.\nLinea %d", row);
			else 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1014), 
				         "El vigilador se encuentra ausente\nen el Cliente %ld Objetivo %d.\nLinea %d",
				         cliente, objet, row);


			FmClearFlds(fm0, NROLEG, HSTOT, row);
			WiRefresh();

			FmSetKeyCode(fm0, K_DEL);

//			return FALSE;
		}
	}

	if (p_cambiofm){
		//Si cambió, valido la superposicion con los datos de fm.
		if (SupPueFm(FmLFld(fm0, NROLEG, row), FmTFld(fm0, HSENTRE, row), FmTFld(fm0, HSSAL, row), row, &hsentre, &hssalida)) { 
			if (!g_GRPSUPOPER){ 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 1016), ERR_SUPERPOS, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row)), FmLFld(fm0, CLIE),
			         FmIFld(fm0, OBJET), hsentre, hssalida, row);
				renglon_ok = FALSE;
				return FALSE;
			}
			else {
				if(WiDialog(WD_OK|WD_NO, WD_OK, TituloMsg(TMSG_WAR, 1016), WAR_SUPERPOS2, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row)), FmLFld(fm0, CLIE),
				            FmIFld(fm0, OBJET), hsentre, hssalida, row)==WD_NO){ 
					renglon_ok = FALSE;
					return FALSE;
				}
			}
		}

		//Si cambió, valido la superposicion con los datos de la base.
		if (SupPue(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmTFld(fm0, HSENTRE, row), 
		    FmTFld(fm0, HSSAL, row))) {
			if (!g_GRPSUPOPER){ 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 1017), ERR_SUPERPOS2,GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row)), LFld(APARTE_CLIENTE), 
				         IFld(APARTE_OBJETIVO), TFld(APARTE_HORAENT), TFld(APARTE_HORASAL), DFld(APARTE_DIA), row);
				renglon_ok = FALSE;
				return FALSE;
			}
			else {
				if (WiDialog(WD_OK|WD_NO, WD_OK, TituloMsg(TMSG_WAR, 1017), WAR_SUPERPOS3, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row)), LFld(APARTE_CLIENTE),
				             IFld(APARTE_OBJETIVO), TFld(APARTE_HORAENT), TFld(APARTE_HORASAL), DFld(APARTE_DIA), row)==WD_NO) {
					renglon_ok = FALSE;
					return FALSE;
				}
			}
		}

	}


	return TRUE;	
}
/************************************************************************************************************
*                                     ControlConsistenciaHoras()
*
* Controla que las horas que se van a grabar + las que estan en la base sean consistentes con el calculo de 
* horas, para esto carga un fm con las horas que hay hoy grabadas en la base + las que hay en el fm, y las 
* que deberia tener la base + las del fm. Si hay alguna diferencia muestra este fm y pregunta si se corrige 
* o no.
************************************************************************************************************/
static void ControlConsistenciaHoras()
{
	bool hay_difer=FALSE;

	int v_i      = 0,
	    v_j      = 0,
	    v_hs_nor = 0;
	char v_titulo[60];

	SetKey(comerc|OBJETIVObyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET));
	GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	// Recorro el multi del Parte 
	for (v_i=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {

		// Si ya verifique este legajo no lo vuelvo a hacer
		if (YaCalculo(v_i))
			continue;

		fm5 = UseSubform(fm0, CTRL, 0);

		// Limpio Fm y Seteo datos comunes
		FmClearAllFlds(fm5);
		FmSetLFld(fm5, CTRLNROLEG, FmLFld(fm0, NROLEG, v_i));
		FmSetFld (fm5, CTRLNOMBRE, FmSFld(fm0, NOMBRE, v_i));
		FmSetFld (fm5, CTRLREGIM , FmSFld(fm0, REGIM , v_i));
		FmSetIFld(fm5, CTRLNORMAL, GetHsNormales(FmSFld(fm0, REGIM , v_i), FALSE));
		FmSetIFld(fm5, CTRLLINEA,  v_i);
		sprintf(v_titulo,"%s¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡", TituloMsg(TMSG_WAR, 1042));
		FmSetFld(fm5, CTRLTITULO, v_titulo);

		v_hs_nor = 0;

		// Seteo cantidades de hora del FM
        LlenaLegajoFm (FmLFld(fm0, NROLEG, v_i), &v_hs_nor);

        PushRecord(comerc|OBJETIVO);
		// Seteo cantidades de hora de la base
		LlenaHorasBase(FmLFld(fm0, NROLEG, v_i), &v_hs_nor);
        PopRecord(comerc|OBJETIVO);

		// Verifico si hay diferencias 
		hay_difer = FALSE;
		for(v_j=0; v_j < FmFldLen(fm5, CTRLMULTI) && !FmIsNull(fm5, CTRLCLIEN, v_j); v_j++) {

			if (FmIFld(fm5, CTRLDIFER, v_j)==TRUE){
				hay_difer=TRUE;
				break;
			}
		}

		//Si hay diferencias muestro fm y pregunto si corrijo el problema
        if(hay_difer) {
            DoSubform(fm0, NULLFP, NULLFP, CTRL, 0);
			if (FmIFld(fm5, CTRLCORRI)) {
				for(v_i=0; v_i < FmFldLen(fm5, CTRLMULTI) && !FmIsNull(fm5, CTRLCLIEN, v_i); v_i++) {
					if (FmIFld(fm5, CTRLDIFER, v_i)) {
						SetKey(operac|PARTEbyEMP, FmIFld(fm0, EMP), FmLFld(fm5, CTRLCLIEN, v_i), 
						                          FmIFld(fm5, CTRLOBJET, v_i), FmDFld(fm0, FECPARTE),
						                          FmLFld(fm5, CTRLNROLEG), FmIFld(fm5, I_CTRLPTOSER, v_i),
						                          FmIFld(fm5, I_CTRLPUESTO, v_i), FmIFld(fm5, I_CTRLNROINT, v_i));
						if(GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {

							SetIFld(operac|PARTE_CONFIR,  0);
							SetIFld(operac|PARTE_HSNOR,   FmIFld(fm5, CTRLNORSU, v_i));
							SetIFld(operac|PARTE_HS50,    FmIFld(fm5, CTRL50SU,  v_i));
							SetIFld(operac|PARTE_HS100FE, FmIFld(fm5, CTRL100SU, v_i));
							SetIFld(operac|PARTE_HS100F,  FmIFld(fm5, CTRLFRASU, v_i));

							AudiGrabaHorasParte(operac, g_prog);
							PutRecord(operac|PARTE);
						}
					}
				}
			}
	    }
	    
	    
	}
}

static void LlenaLegajoFm(long p_nroleg, int* v_hs_nor)
{
	form v_fm1 = (form)NULL;
	int v_i = 0,
	    v_j = 0;

    *v_hs_nor=0;
	for (v_i=0, v_j=0; v_i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {

		if (p_nroleg != FmLFld(fm0, NROLEG, v_i))
			continue;

		v_fm1 = UseSubform(fm0, DETHS, 0, v_i);

		FmSetLFld(fm5, CTRLCLIEN, FmLFld(fm0, CLIE), v_j);
		FmSetIFld(fm5, CTRLOBJET, FmIFld(fm0, OBJET), v_j);

		FmSetTFld(fm5, CTRLHRENT, FmTFld(fm0, HSENTRE, v_i), v_j);
		FmSetTFld(fm5, CTRLHRSAL, FmTFld(fm0, HSSAL, v_i), v_j);

		FmSetIFld(fm5, CTRLNORAC, FmIFld(v_fm1, NORMAL), v_j); 
		FmSetIFld(fm5, CTRL50AC,  FmIFld(v_fm1, EXTRAS1), v_j); 
		FmSetIFld(fm5, CTRL100AC, FmIFld(v_fm1, EXTRAS2), v_j); 
		FmSetIFld(fm5, CTRLFRAAC, FmIFld(v_fm1, FRANCOS), v_j); 

		FmSetIFld(fm5, CTRLNORSU, FmIFld(v_fm1, NORMAL), v_j); 
		FmSetIFld(fm5, CTRL50SU,  FmIFld(v_fm1, EXTRAS1), v_j); 
		FmSetIFld(fm5, CTRL100SU, FmIFld(v_fm1, EXTRAS2), v_j); 
		FmSetIFld(fm5, CTRLFRASU, FmIFld(v_fm1, FRANCOS), v_j); 

		FmSetIFld(fm5, I_CTRLPTOSER, FmIFld(fm0, I_PTOSER, v_i), v_j);
		FmSetIFld(fm5, I_CTRLPUESTO, FmIFld(fm0, I_PUESTO, v_i), v_j);
		FmSetIFld(fm5, I_CTRLNROINT, FmIFld(fm0, I_NROINT, v_i), v_j);


		*v_hs_nor   += FmIFld(v_fm1, NORMAL);
        
		v_j++;
	}
}

static bool YaCalculo(int p_row)
{
	int v_i=p_row;
	
	for (v_i=p_row; v_i >= 0 ; v_i--) 
		if (FmLFld(fm0, NROLEG, p_row) == FmLFld(fm0, NROLEG, v_i) && p_row!=v_i)
			return TRUE;

	return FALSE;

}
static void LlenaHorasBase(long p_nroleg, int* p_hs_nor)
{
	int v_i;

	double	v_hs_nor               = 0,
			v_hs_50                = 0,
			v_hs_100fr             = 0,
			v_hs_100fe             = 0,
			v_hs_res_nor           = 0,
			v_hs_res_50            = 0,
			v_hs_res_100fr         = 0,
			v_hs_res_100fe         = 0;

	char v_tipvig[2];

	dbcursor c_parte;

	// Busco primer linea libre
	for (v_i=0; !FmIsNull(fm5, CTRLCLIEN, v_i); v_i++);

	c_parte  = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, FmIFld(fm0, EMP), p_nroleg, FmDFld(fm0, FECPARTE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), p_nroleg, FmDFld(fm0, FECPARTE), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		if (LFld(operac|PARTE_CLIENTE) == FmLFld(fm0, CLIE))
			if (IFld(operac|PARTE_OBJETIVO) == FmIFld(fm0, OBJET)) 
				continue;

		FmSetLFld(fm5, CTRLCLIEN, LFld(operac|PARTE_CLIENTE) , v_i);
		FmSetIFld(fm5, CTRLOBJET, IFld(operac|PARTE_OBJETIVO), v_i);
		FmSetTFld(fm5, CTRLHRENT, TFld(operac|PARTE_HORAENT), v_i);
		FmSetTFld(fm5, CTRLHRSAL, TFld(operac|PARTE_HORASAL), v_i);

		FmSetIFld(fm5, CTRLNORAC, IFld(operac|PARTE_HSNOR), v_i); 
		FmSetIFld(fm5, CTRL50AC,  IFld(operac|PARTE_HS50), v_i); 
		FmSetIFld(fm5, CTRL100AC, IFld(operac|PARTE_HS100FE), v_i); 
		FmSetIFld(fm5, CTRLFRAAC, IFld(operac|PARTE_HS100F), v_i); 

		// Busco si hay mas horas que no sean ni las del puesto que estoy calculando ahora ni las del FM
        PushRecord(operac|PARTE);
		v_hs_nor = v_hs_50 = v_hs_100fr = v_hs_100fe = 0;
		CalcOtrasHoras(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA),FmLFld(fm0, CLIE),
		                FmIFld(fm0, OBJET), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
		                IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT),
		                &v_hs_nor, &v_hs_50, &v_hs_100fr, &v_hs_100fe);
        PopRecord(operac|PARTE);

		//sumo las horas de la base que no las que calculo ahora y las que tengo ahora en el fm
		v_hs_nor += (double)*p_hs_nor/100;


        // Distribuyo las horas 
		SetKey(comerc|OBJETIVObyCLIENTE, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
		GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		sprintf(v_tipvig, "%s", TipoVig(FmIFld(fm0, EMP), LFld(operac|PARTE_CLIENTE),
		                                IFld(operac|PARTE_OBJETIVO), LFld(operac|PARTE_NROLEG),
		                                IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO),
		                                IFld(operac|PARTE_NROINT), DFld(operac|PARTE_DIA)));

		DistribuyeHoras(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), v_tipvig, 
		                IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV), TFld(operac|PARTE_HORAENT),
		                TFld(operac|PARTE_HORASAL), v_hs_nor, &v_hs_res_nor, &v_hs_res_50, &v_hs_res_100fr, 
		                &v_hs_res_100fe);

		FmSetIFld(fm5, I_CTRLPTOSER, IFld(operac|PARTE_PTOSER), v_i);
		FmSetIFld(fm5, I_CTRLPUESTO, IFld(operac|PARTE_PUESTO), v_i);
		FmSetIFld(fm5, I_CTRLNROINT, IFld(operac|PARTE_NROINT), v_i);

		FmSetIFld(fm5, CTRLNORSU, v_hs_res_nor   * 100, v_i); 
		FmSetIFld(fm5, CTRL50SU,  v_hs_res_50    * 100, v_i); 
		FmSetIFld(fm5, CTRL100SU, v_hs_res_100fe * 100, v_i); 
		FmSetIFld(fm5, CTRLFRASU, v_hs_res_100fr * 100, v_i); 

		v_i++;
	}
	DeleteCursor(c_parte);

}

int  ErrPueFm(int p_row)
{
	int v_i=0,  v_respuesta=NULL_SHORT, canpeg=0, cantra=0, canade=0;
	bool pegada = FALSE, adelanto = FALSE;

	if (*FmSFld(fm0, COND, p_row) == _PEGADA_C) {
		canpeg++;
		pegada = TRUE;
	}
	else {
		if (*FmSFld(fm0, COND, p_row) == _ADELANTO_C) {
			canade++;
			adelanto = TRUE;
		}
		else {
			if (*FmSFld(fm0, COND, p_row) == _TRABAJA_C) {
				cantra++;
			}
		}
	}
	
	for (v_i=0; v_i <FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		if (FmLFld(fm0, NROLEG, p_row) != FmLFld(fm0, NROLEG, v_i)) 
			continue;

		if (v_i == p_row)
			continue;

		switch (*FmSFld(fm0, COND, v_i)) {
		case _PEGADA_C:
			canpeg++;
			continue;
		case _ADELANTO_C:
			canade++;
			continue;
		case _TRABAJA_C:
			cantra++;
			if (pegada || adelanto) {
				continue;
			}
		}
		if (strcmp(FmSFld(fm0, COND, p_row), FmSFld(fm0, COND, v_i)) != 0) {
			v_respuesta = v_i;
			break;
		}
	} 
	if (canpeg || canade) {
		if (!cantra) {

			PushRecord(operac|PARTE);

			SetKey(operac|PARTEbyEMPLE, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, p_row), FmDFld(fm0, FECPARTE), NULL_LONG, NULL_BYTE);
			while(GetRecord(operac|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {

				if (LFld(operac|PARTE_CLIENTE) == FmLFld(fm0, CLIE) && 
				    IFld(operac|PARTE_OBJETIVO) == FmIFld(fm0, OBJET))
					continue;

				if (*SFld(operac|PARTE_CONDIC) == _TRABAJA_C) {
					
					cantra++;
				}
			}
			PopRecord(operac|PARTE);
		}
		if (!cantra) {
			v_respuesta = v_i;
		}
	}

	return v_respuesta;
}


private void CargaFm(int p_emp, long p_nroleg, int p_i, DATE p_dia, char *p_condic, int p_ptoser, int p_puesto, int p_nroint, TIME p_hent, TIME p_hsal, int p_conf, 
					 int p_hsnor, int p_hs50, int p_hs100fe, int p_hs100f, int p_codaus)
{
	dbcursor c_EX;
	char regimen[15];
	int tipptoefe = 0, puestoefe = 0, j;

	c_EX    = CreateCursor(operac|EXCEPCIONbyEMP, IO_NOT_LOCK);

	tipptoefe = 0, puestoefe = 0;
 	fm1 = UseSubform(fm0, DETHS, 0, p_i);

	FmSetLFld(fm0, NROLEG,  p_nroleg, p_i);
	FmSetFld (fm0, NOMBRE,  GetNombreLeg(p_emp, p_nroleg), p_i);
	FmSetIFld(fm0, I_VIGINEW, FALSE, p_i);
	GetRegimenEfectivo(p_emp, p_nroleg, regimen, p_dia);
	FmSetFld (fm0, REGIM,    regimen, p_i);
	FmSetFld (fm0, COND,     p_condic,  p_i);
	FmSetIFld(fm0, I_PTOSER, p_ptoser,  p_i);
	FmSetIFld(fm0, I_PUESTO, p_puesto,  p_i);
	FmSetIFld(fm0, I_NROINT, p_nroint,  p_i);
	FmSetTFld(fm0, HSENTRE,  p_hent, p_i);
	FmSetTFld(fm0, HSSAL,    p_hsal, p_i);
	FmSetIFld(fm0, HSTOT,    ConvHraInt(p_hent, p_hsal) * 100, p_i);

	if (p_conf != A_CONF) {
		cerrado = TRUE;
	}

	if (*FmSFld(fm0, COND, p_i) == 'A') {
		fm3 = UseSubform(fm0, COND, 0, p_i);
		FmSetIFld(fm3, CODNOV, p_codaus);
	}

 
  
  	hstot += FmIFld(fm0, HSTOT, p_i);

	FmSetIFld(fm0, I_EXTASIG, ExisteEnAsig(p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_nroleg, p_dia, p_ptoser, p_puesto, p_nroint, TRUE), p_i);
	FmSetIFld(fm0, I_EFECT,   EsEfectivo(p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_nroleg, p_ptoser, p_puesto, p_nroint, p_dia), p_i);
	FmSetIFld(fm0, I_PARTIME, EsPuestoPartime(p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_nroleg, p_ptoser, p_puesto, p_nroint, p_dia), p_i);


	GetPtoEfectivo(p_emp, p_nroleg, &tipptoefe, &puestoefe);

	FmSetFld(fm0, I_TIPVIG, TipoVig(p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_nroleg, tipptoefe, puestoefe, p_nroint, p_dia), p_i);

	if (p_hsnor  != 0 || p_hs50   != 0 || p_hs100fe != 0 || p_hs100f != 0){
		SeteoCampos(p_hsnor, p_hs50, p_hs100fe, p_hs100f, p_i);
	}
	else {
		SeteoCampos(0, 0, 0, 0, p_i);

		/*Si no cargo horas vuelvo */
		CalDetHorPer2(TRUE, fm0, p_i, p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_dia, p_nroleg, p_ptoser, p_puesto, p_nroint, 
		                        *FmSFld(fm0, COND, p_i), p_hent, p_hsal, &g_hsnor, &g_hs50, &g_hs100f, &g_hs100fe);

//WiMsg("row %d %f %f %f", p_i, g_hsnor, g_hs50, g_hs100fe + g_hspega);
		SeteoCampos(g_hsnor, g_hs50, g_hs100fe + g_hspega, g_hs100f, p_i);
	}


	fm4 = UseSubform(fm0, PTO, 0, p_i);
	FmSetIFld(fm4, TIPPTO,  p_ptoser);
	FmSetIFld(fm4, CODINT,  p_puesto);
	FmSetIFld(fm4, NROINT,  p_nroint);
	FmSetFld (fm4, DTIPPTO, GetDescPto(p_ptoser));

	fm2 = UseSubform(fm0, EXCP, 0, p_i);
	SetCursorFrom(c_EX, p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_dia,p_nroleg, FmIFld(fm0, I_PTOSER, p_i), FmIFld(fm0, I_PUESTO, p_i),
						FmIFld(fm0, I_NROINT, p_i), MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_EX, p_emp, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), p_dia, p_nroleg, FmIFld(fm0, I_PTOSER, p_i), FmIFld(fm0, I_PUESTO, p_i), 
	                    FmIFld(fm0, I_NROINT, p_i), MAX_SHORT, MAX_SHORT);

	for (j=0; FetchCursor(c_EX) != ERROR && j < FmFldLen(fm2, MULTI); j++) {
		FmSetIFld(fm2, TIPFAC,  IFld(operac|EXCEPCION_CONDIC), j);
		FmSetFld (fm2, DTIPFAC, GetDescCond(IFld(operac|EXCEPCION_CONDIC)), j);
		FmSetIFld(fm2, MOTIVO,  IFld(operac|EXCEPCION_MOTIVO), j);
		FmSetFld (fm2, DMOTIV,  GetDescMotivo(IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO)), j);
		FmSetIFld(fm2, HORAS,   IFld(operac|EXCEPCION_HORAS), j);
		FmSetIFld(fm2, HS50,    IFld(operac|EXCEPCION_HS50),  j);
		FmSetIFld(fm2, HS100,   IFld(operac|EXCEPCION_HS100), j);
		FmSetFld (fm2, OBS,     SFld(operac|EXCEPCION_OBS),   j);
	}

	DeleteCursor(c_EX);

}

static tnnhent AcuNNhent(tnnhent nodop, tnnhent * nantp)
{
	tnnhent naux;

	if (nodop == NULL) {
		nodop = (tnnhent) malloc (sizeof(stnnhent));

		(*nodop).nhent = nhent;
		(*nodop).nnhsal = AcuNNhsal(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nhent == nhent) 
			(*nodop).nnhsal = AcuNNhsal((*nodop).nnhsal, &(*nodop).nnhsal);
		else {
			if ((*nodop).nhent < nhent)
				(*nodop).nsig = AcuNNhent((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnhent) malloc (sizeof(stnnhent));
				(*nodop).nhent = nhent;
				(*nodop).nnhsal = AcuNNhsal(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnhsal AcuNNhsal(tnnhsal nodop, tnnhsal * nantp)
{
	tnnhsal naux;

	if (nodop == NULL) {
		nodop = (tnnhsal) malloc (sizeof(stnnhsal));

		(*nodop).nhsal = nhsal;
		(*nodop).nnnroleg = AcuNNnroleg(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nhsal == nhsal) 
			(*nodop).nnnroleg = AcuNNnroleg((*nodop).nnnroleg, &(*nodop).nnnroleg);
		else {
			if ((*nodop).nhsal < nhsal)
				(*nodop).nsig = AcuNNhsal((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnhsal) malloc (sizeof(stnnhsal));
				(*nodop).nhsal = nhsal;
				(*nodop).nnnroleg = AcuNNnroleg(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnnroleg AcuNNnroleg(tnnnroleg nodop, tnnnroleg * nantp)
{
	tnnnroleg naux;

	if (nodop == NULL) {
		nodop = (tnnnroleg) malloc (sizeof(stnnnroleg));

		(*nodop).nnroleg = nnroleg;
		(*nodop).nntippto = AcuNNtippto(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nnroleg == nnroleg) 
			(*nodop).nntippto = AcuNNtippto((*nodop).nntippto, &(*nodop).nntippto);
		else {
			if ((*nodop).nnroleg < nnroleg)
				(*nodop).nsig = AcuNNnroleg((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnnroleg) malloc (sizeof(stnnnroleg));
				(*nodop).nnroleg = nnroleg;

				(*nodop).nntippto = AcuNNtippto(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnntippto AcuNNtippto(tnntippto nodop, tnntippto * nantp)
{
	tnntippto naux;

	if (nodop == NULL) {
		nodop = (tnntippto) malloc (sizeof(stnntippto));

		(*nodop).ntippto = ntippto;
		(*nodop).nnpuesto = AcuNNpuesto(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).ntippto == ntippto) 
				(*nodop).nnpuesto = AcuNNpuesto((*nodop).nnpuesto, &(*nodop).nnpuesto);
		else {
			if ((*nodop).ntippto < ntippto)
				(*nodop).nsig = AcuNNtippto((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnntippto) malloc (sizeof(stnntippto));
				(*nodop).ntippto = ntippto;

				(*nodop).nnpuesto = AcuNNpuesto(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnpuesto AcuNNpuesto(tnnpuesto nodop, tnnpuesto * nantp)
{
	tnnpuesto naux;

	if (nodop == NULL) {
		nodop = (tnnpuesto) malloc (sizeof(stnnpuesto));

		(*nodop).npuesto = npuesto;
		(*nodop).nnnroint = AcuNNnroint(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).npuesto == npuesto) 
				(*nodop).nnnroint = AcuNNnroint((*nodop).nnnroint, &(*nodop).nnnroint);
		else {
			if ((*nodop).npuesto < npuesto)
				(*nodop).nsig = AcuNNpuesto((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnpuesto) malloc (sizeof(stnnpuesto));
				(*nodop).npuesto = npuesto;
				(*nodop).nnnroint = AcuNNnroint(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnnroint AcuNNnroint(tnnnroint nodop, tnnnroint * nantp)
{
	tnnnroint naux;

	if (nodop == NULL) {
		nodop = (tnnnroint) malloc (sizeof(stnnnroint));

		(*nodop).nnroint = nnroint;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nnroint == nnroint) {
		}
		else {
			if ((*nodop).nnroint < nnroint)
				(*nodop).nsig = AcuNNnroint((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnnroint) malloc (sizeof(stnnnroint));
				(*nodop).nnroint = nnroint;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static void LisNNhent(tnnhent nodop)
{
	if (nodop == NULL)
		return;

	nhent = (*nodop).nhent;

	if ((*nodop).nnhsal != NULL)
		LisNNhsal((*nodop).nnhsal);

	if ((*nodop).nsig != NULL)
		LisNNhent((*nodop).nsig);
}

static void LisNNhsal(tnnhsal nodop)
{
	if (nodop == NULL)
		return;

	nhsal = (*nodop).nhsal;

	if ((*nodop).nnnroleg != NULL)
		LisNNnroleg((*nodop).nnnroleg);

	if ((*nodop).nsig != NULL)
		LisNNhsal((*nodop).nsig);
}

static void LisNNnroleg(tnnnroleg nodop)
{
	if (nodop == NULL)
		return;

	nnroleg = (*nodop).nnroleg;

	if ((*nodop).nntippto != NULL)
		LisNNtippto((*nodop).nntippto);

	if ((*nodop).nsig != NULL)
		LisNNnroleg((*nodop).nsig);
}

static void LisNNtippto(tnntippto nodop)
{
	if (nodop == NULL)
		return;

	ntippto = (*nodop).ntippto;

	if ((*nodop).nnpuesto != NULL)
		LisNNpuesto((*nodop).nnpuesto);

	if ((*nodop).nsig != NULL)
		LisNNtippto((*nodop).nsig);
}

static void LisNNpuesto(tnnpuesto nodop)
{
	if (nodop == NULL)
		return;

	npuesto = (*nodop).npuesto;
	if ((*nodop).nnnroint != NULL)
		LisNNnroint((*nodop).nnnroint);

	if ((*nodop).nsig != NULL)
		LisNNpuesto((*nodop).nsig);
}

static void LisNNnroint(tnnnroint nodop)
{

	if (nodop == NULL)
		return;

	nnroint = (*nodop).nnroint;

	SetKey(operac|PARTEbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),  FmDFld(fm0, FECPARTE), nnroleg, ntippto, npuesto, nnroint);
	if(GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) 
		CargaFm(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), g_i++, FmDFld(fm0, FECPARTE), SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_PTOSER), 
				IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), IFld(operac|PARTE_CONFIR),
				IFld(operac|PARTE_HSNOR), IFld(operac|PARTE_HS50), IFld(operac|PARTE_HS100FE), IFld(operac|PARTE_HS100F),IFld(operac|PARTE_CODAUS));

	if ((*nodop).nsig != NULL)
		LisNNnroint((*nodop).nsig);
}

static void BorNNhent(tnnhent nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnhsal != NULL)
		BorNNhsal((*nodop).nnhsal);

	if ((*nodop).nsig != NULL)
		BorNNhent((*nodop).nsig);

	(*nodop).nnhsal = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNhsal(tnnhsal nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnnroleg != NULL)
		BorNNnroleg((*nodop).nnnroleg);

	if ((*nodop).nsig != NULL)
		BorNNhsal((*nodop).nsig);

	(*nodop).nnnroleg = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNnroleg(tnnnroleg nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nntippto != NULL)
		BorNNtippto((*nodop).nntippto);

	if ((*nodop).nsig != NULL)
		BorNNnroleg((*nodop).nsig);

	(*nodop).nntippto = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNtippto(tnntippto nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnpuesto != NULL)
		BorNNpuesto((*nodop).nnpuesto);

	if ((*nodop).nsig != NULL)
		BorNNtippto((*nodop).nsig);

	(*nodop).nnpuesto = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNpuesto(tnnpuesto nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnnroint != NULL)
		BorNNnroint((*nodop).nnnroint);

	if ((*nodop).nsig != NULL)
		BorNNpuesto((*nodop).nsig);

	(*nodop).nnnroint = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNnroint(tnnnroint nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNNnroint((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}

static bool WarPue(int emp, long p_nroleg, DATE fecparte, long p_cliente, int obj, TIME hsent, TIME hssal,
				   bool *p_franco, char *p_cadena)
{
	dbcursor c_parte;
	bool haysup  = FALSE,
	     malasig = FALSE;
	char auxc[15];

	sprintf(p_cadena, "%s", NULL_STR);
	sprintf(auxc, "%s", NULL_STR);

	c_parte = CreateCursor(AlInd(aparte, operac|PARTEbyEMPLE), IO_NOT_LOCK);

	*p_franco = FALSE;

	SetCursorFrom(c_parte, emp, p_nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, emp, p_nroleg, fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (LFld(AlFld(aparte,PARTE_CLIENTE)) == p_cliente && IFld(AlFld(aparte,PARTE_OBJETIVO)) == obj) {
			continue;
		}

		if (*SFld(AlFld(aparte,PARTE_CONDIC)) == 'F')
			*p_franco = TRUE;

		if (*p_franco && *SFld(AlFld(aparte,PARTE_CONDIC)) != 'F')
			malasig=TRUE;

		if (haysup && *SFld(AlFld(aparte,PARTE_CONDIC)) == 'F')
			malasig=TRUE;

		haysup = TRUE;

		sprintf(p_cadena, "%s%6ld %4d ", p_cadena, LFld(AlFld(aparte,PARTE_CLIENTE)),
				                                  IFld(AlFld(aparte,PARTE_OBJETIVO)));

		TToStr(TFld(AlFld(aparte,PARTE_HORAENT)), auxc, TFMT_SEPAR);
		sprintf(p_cadena, "%s%6s  ", p_cadena, auxc);

		TToStr(TFld(AlFld(aparte,PARTE_HORASAL)), auxc, TFMT_SEPAR);
		sprintf(p_cadena, "%s%6s %7s ", p_cadena, auxc, SFld(AlFld(aparte,PARTE_CONDIC))); 
                                                                                                 
  		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HSNOR)))  / 100);
		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS50)))   / 100);
		sprintf(p_cadena, "%s%6.2f ",  p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS100F))) / 100);
		sprintf(p_cadena, "%s%6.2f\n", p_cadena, ((double)LFld(AlFld(aparte,PARTE_HS100FE)))/ 100);
		
	}
	if (malasig)
		sprintf(p_cadena, "%sAdvertencia! Existen  condiciones de trabajo  que no son Francos. \n",p_cadena);

	DeleteCursor(c_parte);
	return haysup;
}


