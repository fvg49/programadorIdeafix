/********************************************************************
* MODULE & VERSION : @(#)asige.c	1.10
* DATE             : 10/04/20
* TIME             : 11:08:52
*
* CREATED          :
* DESCRIPTION:
*      Asignación de Vigiladores.
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "operac.h"
#include "comerc.h"
#include "comgral.h"
#include "opedef.h"
#include "billpro.h"
#include "ambiente.h"
#include "asig.h"
#include "filial.h"
#include "asige.fmh"  // asige.fm
#include "asige1.fmh" // asige1.fm
#include "asige2.fmh" // asige2.fm
#include "asige3.fmh" // asige3.fm
#include "asige4.fmh" // asige4.fm
#include "asige5.fmh" // asige5.fm
#include "asige6.fmh" // asige6.fm
#include "sue.sch"
#include "sue.h"
#include "operac.sch"
#include "bill.sch"
#include "comerc.sch"
#include "webinter.h"

#define MAX_MESES         20
#define MAX_FECHAS        1000 // 92
#define MAX_DIAS          90   // originalmente tenia 30
#define MAX_ERRORS        20   // Se acumulan solo 20 errores, que es el mas o menos la capacidad de la
							   // pantalla para desplegarlos.
#define MSG_LEGINAC		 "El legajo se encuentra inactivo.\nLinea %d"
#define MSG_PARTE        "El empleado tiene parte generado y cargado. Imposible modificarlo."
#define MSG_FECFRA		 "La Fecha de Franco debe estar dentro del periodo de duración del puesto. (%.1D - %.1D)"
#define MSG_SUPERPOS     "Superposición Horaria!!!\nEl Vigilador: %s se encuentra asignado en:\nCliente: %ld  Obj: %d  Fec.Asig.: %.1D\nDías: %s %s %s %s %s %s %s  Hr.Ent.: %.1T  Hr.Sal.: %.1T\nLínea %d"



// Retorna el subindice segun la letra del dia, para el vector de validacion de frecuencia
#define	subdia(dia)	    (dia == 'L' ? 0 : dia == 'M' ? 1 : dia == 'X' ? 2 :	dia == 'J' ? 3 : dia == 'V' ? 4 : dia == 'S' ? 5 : dia == 'D' ? 6 : NULL_SHORT)
// Inversa de la anterior.
// Dado el subindice retorna la letra del dia a la que corresponde.
#define	diasub(i)	    (i == 0 ? "L" : i == 1 ? "M" : i == 2 ? "X" : i == 3 ? "J" : i == 4 ? "V" : i == 5 ? "S": i == 6 ? "D" : NULL_STR)
#define REG_ESP "4x2x12"	

//extern bool   InscriptoEnJuris(int emp, long nroleg, int pais, int provi);

static fm_status after(form, fmfield, int),        before(form, fmfield, int),
                 after_asige1(form, fmfield, int), before_asige1(form, fmfield, int),
                 after_asige2(form, fmfield, int),
                 after_asige3(form fm, fmfield fno, int row),
                 after_asige5(form, fmfield, int),
                 after_asige6(form, fmfield, int), before_asige6(form, fmfield, int); 
                 
static int  validate(void);
static int  CalcNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto);

static void CargarDiasPTime(long nroleg, int ptoser, int puesto, int nroint, form fm);
static void LeerObjetivo(long cliente, int objetivo);
static void Lectura(fm_cmd, find_mode);
static void ProcesoPostu(void);
static void GraboPuesto();
static void display(char * buffer);
static void GetRegimenEfectivoFm(int emp, long nroleg, char *regimen, DATE fecha);
static void RestauraSubForm(form fm, fmfield fno, int pos, int row);
static void Validaciones();
static bool ValInfo();
static bool HayVacante(long nroleg, int tippto, int codint, DATE fdesde, DATE fhasta, DATE *fechocup);
static bool DiaFranco(int emp, long nroleg, DATE fecasig, DATE fechas, char * dia);
static bool ValidarSumaMeses(form fm);
static bool FrancoFm(int emp, long nroleg, DATE fecha, char * regimen, DATE fecfranco, int numfran);
static bool ValidPuestoOcup(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
                            char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, int fila,
                            long cliefec, int objefec, int ptoefec, int codintefec, char * efect);
static bool ValidoUnAsig(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
                         char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, bool asigh);
static bool ValidoUnAsigFm(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
                           char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, bool asigh);
static bool VerificaPermisos();
static bool ControlesNivelLinea(int row, bool p_cambiofm);

bool TieneProvisorios(int p_emp, long p_nroleg, long p_cliente, int p_objet, int p_fecasi);


static void MuestraNrodeLineaActual(fmfield fno, int row);

private void CargarOtrasAsignaciones(form fm4, long nroleg);
private void PonerEnASIGH(form fm4, long nroleg, DATE fecha, int motivo, long reempl, DATE fecbaj);
private void DisplayMSGERR();
private void GenerarDiasPTSemanal(form fm);
private void CargarHsPT(form fmdias);
private bool ValidarPartesProvisorios(DATE fecha, form fm);
private bool ValidPartTime(form fm);
private bool ValidPartTimeFm(form fmdias, int row_padre, long legajo);
private bool ValidHsPT();
private bool PutMSGERR(char error[]);
private bool ValidaFrecuencia(form fmdias, DATE fecha, DATE fechas, char * codfrec, long nroleg, int ptoser,
							  int puesto, int nroint);
private bool PutErrorMesesFaltantes(DATE fechad, DATE fechah, int dia);
private int  compdia(DATE *a, DATE *b);

private void HelpFila(int rol);
private char *get_fila(char *buff, int lin);

private void MuevoAsigPartT(int p_lin, DATE p_fecbaja);
private void Cargo_fm1(form fm, fmfield fno, int row);

struct mes {
    int    mes;
    double horas;
    char   error[60];
} st_meses[MAX_MESES];
int topmes = 0;

// Vector para poner los errores en la asignación de part-times por hora, en teoría uno por cada día
// que tiene el multirenglón sería el máximo.
struct MSGERR {
    char error[100];
} MsgErr[MAX_ERRORS];
int topmerr = 0;

struct s_dias {
    int  ult;
    DATE fecha[MAX_FECHAS];
} dia[7];

struct dias {
    DATE dia;
    TIME hsent;
    TIME hssal;
} diaspt[MAX_DIAS];

// Estructura de Asig y AsigH para poder validar la superposicion contra las dos tablas.
struct Asig asigSt;
private	void CargarEstructuraAsig(int emp, long cliente, int objetivo, int ptoser, int puesto, int nroint,
								   long nroleg, char vigil[], char efect[], DATE fecasig, TIME hsent,
								   TIME hssal, char dia1[], char dia2[], char dia3[], char dia4[],
								   char dia5[], char dia6[], char dia7[], long reempl, DATE ffranco,
								   int numfran, int francero, char regim[], DATE fechas, DATE fecbaj);
// Declaraciones globales
form     fm0, fm1, fm2, fm3, fm4, fm5, fm6;
schema   sue, operac, bill, comerc;
dbtable  asig, aasig;
dbcursor c_asig, c_aasig, c_asigh, c_diaspt, c_ptoser, c_puestos, c_parte = NULL;
int      pue[MAXFILA][MAXCOL], ptoser, codint, row_padre, hsmes[13], rol, g_emp;
int      row_fm0;
bool     actualizar = TRUE;
DATE     fecasig, fecierre, fecierrefil;
TIME     horent, horsal;
char     t_vigil[2], // Para popup de puestos según el tipo de vigilador (V, P)
         t_efect[2]; // Para popup de puestos según el tipo de vigilador (V, P) y cond. de efectivo

fmfield	 campo_fm1=NULL_SHORT;

/* Esta matriz es para copiar todos los campo del fm1 y restaurarlos luego si es necesario */
char g_fm1_char[80][20];
char g_codprog[20], g_oi_sesion[30];
void GuardoFm1();
void RecuperoFm1();
char filial[7] = {'\0'};
bool cerradoxfil = FALSE; 
bool g_GRPSUPOPER;

wcmd(asige, 1.10 04/20/10 )
{
	fm_status cmd;
	
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	aasig   = CreateAlias(operac|ASIG);
	c_aasig = CreateCursor(AlInd(aasig, operac|ASIGbyNROLEG), IO_NOT_LOCK);
	asig    = CreateAlias(operac|ASIG);
	c_asig  = CreateCursor(AlInd(asig, operac|ASIGbyNROLEG), IO_EABORT|IO_NOT_LOCK);

	c_diaspt  = CreateCursor(operac|DIASPTIMEbyEMP,   IO_NOT_LOCK);
	c_asigh   = CreateCursor(operac|ASIGHbyNROLEG,    IO_EABORT|IO_NOT_LOCK);
	c_ptoser  = CreateCursor(comerc|PTOSERbyCLIENTE,  IO_NOT_LOCK|IO_CONTROL_BREAK);
	c_puestos = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

	sprintf(g_codprog,"%s", argv[0]);
	sprintf(g_oi_sesion,"%d", getpid());

	fm0 = OpenForm("asige", FM_EABORT);

	g_emp= StrToI(getenv("emp"));
	g_GRPSUPOPER=UsrInGrupo(GRPSUPOPER, GetUid());

	FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Clientes - Objetivos");
	WiRefresh();
	InicListaXusr(g_emp);
	FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Legajos");
	WiRefresh();

	InicLegajoXusr (g_emp, Today(), fm0, MENSAJE, TRUE, _TIPPER_ASIGNA);

//	ImprimeLegajoXusr();
	
	FmCallBacks(fm0, ASIGNA,  "asige2", NULLFP, after_asige2);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:      Lectura(cmd, THIS_KEY); break;
		case FM_READ_NEXT: Lectura(cmd, NEXT_KEY); break;
		case FM_READ_PREV: Lectura(cmd, PREV_KEY); break;
		case FM_ADD :
			InitRecord(operac|ASIG);
			InitRecord(operac|ASIGH);
			InitRecord(operac|DIASPTIME);
		case FM_UPDATE :
			if (!actualizar || ValInfo() || !VerificaPermisos()) {
				WiDialog(WD_OK,WD_OK, TituloMsg(TMSG_ERR,2054),"No se pudo llevar a cabo la actualización.");
				FmNextFld(fm0, CONTROL_FLD);
				actualizar = TRUE;
				break;
			}
//FER
//          WiMsg("No Graba");
//          continue;

			BeginTransaction();
			GraboPuesto();
			ProcesoPostu();
			EndTransaction();
			Validaciones();
			break;
		case FM_IGNORE :
			FreeTable(operac|ASIG);
			break;
		}
	}

	FinObjetivosXusr();
	FinClientesXusr();
    FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int  i;
	char regimen[15];

	CargarMatPuestos(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), pue);

	SetKey(operac|ASIGbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MIN_LONG, MIN_SHORT,
							 MIN_SHORT, MIN_SHORT);
	switch(GetRecord(operac|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 3)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(operac|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, 1);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	SetKey(operac|ASIGbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MIN_LONG, MIN_SHORT,
							 MIN_SHORT, MIN_SHORT);
	for (i=0; GetRecord(operac|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR &&
				i < FmFldLen(fm0, MULTI0); i++) {
		fm1 = UseSubform(fm0, HORARIO, 0, i);

		SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), IFld(operac|ASIG_PTOSER),
										IFld(operac|ASIG_PUESTO));
		(void) GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		DbToFm(fm1, I_VIGI, I_FFINAL);
		/* Campo internal que permite borrar el renglon si el vigi no esta en la base */
		FmSetIFld(fm0, I_BASE,     TRUE , i);
		FmSetIFld(fm0, I_NEW ,     FALSE, i);
		FmSetLFld(fm0, NROLEG,     LFld(operac|ASIG_NROLEG), i);
		FmSetFld (fm0, NOMBRE,     GetNombreLeg(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG)), i);
		FmSetIFld(fm0, ASIGNA,     1, i);
		FmSetFld (fm0, VIGILAD,    SFld(operac|ASIG_VIGIL), i);
		FmSetFld (fm0, EFECT,      SFld(operac|ASIG_EFECT), i);
		FmSetFld (fm0, IS_EFECT,   SFld(operac|ASIG_EFECT), i);
		FmSetDFld(fm0, I_FECINI ,  DFld(operac|PUESTOS_FINICIO), i);
		FmSetDFld(fm0, I_FECFIN ,  DFld(operac|PUESTOS_FFINAL), i);
		FmSetDFld(fm0, I_FECININ , DFld(operac|PUESTOS_FINICIO), i);
		FmSetDFld(fm0, I_FECFINN , DFld(operac|PUESTOS_FFINAL), i);
		FmSetIFld(fm0, I_PTOASI,   IFld(operac|ASIG_PTOSER), i);
		FmSetIFld(fm0, I_CODINASI, IFld(operac|ASIG_PUESTO), i);
		FmSetIFld(fm0, I_INTASI,   IFld(operac|ASIG_NROINT), i);
		FmSetIFld(fm1, I_HORAPT,   IFld(operac|PUESTOS_HORAPT));
		FmSetIFld(fm1, HORAPT,     IFld(operac|PUESTOS_HORAPT));
		FmSetFld (fm1, I_CODFREC,  SFld(operac|PUESTOS_CODFREC));
		FmSetFld (fm1, CODFREC,    SFld(operac|PUESTOS_CODFREC));
		FmSetFld (fm1, DCODFREC,   DescrFrecuencia(SFld(operac|PUESTOS_CODFREC)));
		GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), regimen, DFld(operac|ASIG_FECASIG));
		FmSetFld (fm1, REGIM,      regimen);
		FmSetFld (fm1, TIPODIA,    SFld(operac|PUESTOS_TIPODIA));
		FmSetIFld(fm1, TIPPTO,     IFld(operac|ASIG_PTOSER));
		FmSetIFld(fm1, CODINT,     IFld(operac|ASIG_PUESTO));
		FmSetIFld(fm1, I_NROINT,   IFld(operac|ASIG_NROINT));
		FmSetDFld(fm1, FECHA,      DFld(operac|ASIG_FECASIG));
		FmSetDFld(fm1, I_FCHDES,   DFld(operac|ASIG_FECASIG));
		FmSetDFld(fm1, FECFRA,     DFld(operac|ASIG_FFRANCO));
		FmSetIFld(fm1, NUMFRAN,    IFld(operac|ASIG_NUMFRAN));
		FmSetIFld(fm1, FRANCERO,   IFld(operac|ASIG_FRANCERO));
		FmSetTFld(fm1, HORENT,     TFld(operac|ASIG_HSENT));
		FmSetTFld(fm1, HORSAL,     TFld(operac|ASIG_HSSAL));
		FmSetFld (fm1, DIA1,       SFld(operac|ASIG_DIA1));
		FmSetFld (fm1, DIA2,       SFld(operac|ASIG_DIA2));
		FmSetFld (fm1, DIA3,       SFld(operac|ASIG_DIA3));
		FmSetFld (fm1, DIA4,       SFld(operac|ASIG_DIA4));
		FmSetFld (fm1, DIA5,       SFld(operac|ASIG_DIA5));
		FmSetFld (fm1, DIA6,       SFld(operac|ASIG_DIA6));
		FmSetFld (fm1, DIA7,       SFld(operac|ASIG_DIA7));
		FmSetIFld(fm1 ,CODROL,     IFld(operac|ASIG_CODROL));
		FmSetIFld(fm1, FILA,       IFld(operac|ASIG_FILA));
		FmSetIFld(fm1, COLUM,      IFld(operac|ASIG_COLUM));
		FmSetIFld(fm1, I_MODIF,    FALSE);
		if (*FmSFld(fm0, EFECT, i) == 'P') {
			fm3 = UseSubform(fm1, FECHA, 0, 0);
			FmSetDFld(fm3, FECHAS,     DFld(operac|ASIG_FECHAS));
			FmSetDFld(fm1, I_FCHHAS,   DFld(operac|ASIG_FECHAS));
		}
		if (StrCmp(SFld(operac|ASIG_VIGIL), PARTTIME) == 0)
			CargarDiasPTime(FmLFld(fm0, NROLEG, i), IFld(operac|ASIG_PTOSER), IFld(operac|ASIG_PUESTO), IFld(operac|ASIG_NROINT),fm1);
	}
}

static void ProcesoPostu(void)
{
	int i, j;
	bool errorasig = FALSE;
	DATE fecbaja;

	// Grabo las asignaciones actuales
	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {

		fm1 = UseSubform(fm0, HORARIO, 0, i);
		fm5 = UseSubform(fm1, SUBFPTIME, 0, 0);

		if (!FmIFld(fm0, ASIGNA, i)){				
			fm2 = UseSubform(fm0, ASIGNA, 0, i);
			errorasig = FmIFld(fm2, ERRORASIG); 
			fecbaja = FmDFld(fm2, FECBAJ);
		}
		else{
			fm6 = UseSubform(fm1, MOTIVO1, 0, 0);
			fecbaja = FmDFld(fm6, FECBAJ6);
		}

		if (!FmIFld(fm0, ASIGNA, i) || (FmIFld(fm1, I_MODIF) && FmIFld(fm1, I_EXISTE))) {

			//Muevo la asignación por dia de los partime
			MuevoAsigPartT(i, fecbaja);

			SetIFld(operac|ASIG_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|ASIG_CLIENTE,  FmLFld(fm0, CLIEOT));
			SetIFld(operac|ASIG_OBJETIVO, FmIFld(fm0, OBJET));
			SetLFld(operac|ASIG_NROLEG,   FmLFld(fm0, NROLEG,     i));
			SetIFld(operac|ASIG_PTOSER,   FmIFld(fm0, I_PTOASI,   i));
			SetIFld(operac|ASIG_PUESTO,   FmIFld(fm0, I_CODINASI, i));
			SetIFld(operac|ASIG_NROINT,   FmIFld(fm0, I_INTASI,   i));
			if (GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {

				if (!errorasig) {
					InitRecord(operac|ASIGH);
					SetIFld(operac|ASIGH_EMP,      IFld(operac|ASIG_EMP));
					SetLFld(operac|ASIGH_CLIENTE,  LFld(operac|ASIG_CLIENTE));
					SetIFld(operac|ASIGH_PTOSER,   IFld(operac|ASIG_PTOSER));
					SetIFld(operac|ASIGH_PUESTO,   IFld(operac|ASIG_PUESTO));
					SetIFld(operac|ASIGH_NROINT,   IFld(operac|ASIG_NROINT));
					SetIFld(operac|ASIGH_OBJETIVO, IFld(operac|ASIG_OBJETIVO));
					SetLFld(operac|ASIGH_NROLEG,   LFld(operac|ASIG_NROLEG));
					SetFld (operac|ASIGH_VIGIL,    SFld(operac|ASIG_VIGIL));
					SetFld (operac|ASIGH_EFECT,    SFld(operac|ASIG_EFECT));
					SetDFld(operac|ASIGH_FECALT,   DFld(operac|ASIG_FECASIG));
					SetFld (operac|ASIGH_DIA1,     SFld(operac|ASIG_DIA1));
					SetFld (operac|ASIGH_DIA2,     SFld(operac|ASIG_DIA2));
					SetFld (operac|ASIGH_DIA3,     SFld(operac|ASIG_DIA3));
					SetFld (operac|ASIGH_DIA4,     SFld(operac|ASIG_DIA4));
					SetFld (operac|ASIGH_DIA5,     SFld(operac|ASIG_DIA5));
					SetFld (operac|ASIGH_DIA6,     SFld(operac|ASIG_DIA6));
					SetFld (operac|ASIGH_DIA7,     SFld(operac|ASIG_DIA7));
					SetTFld(operac|ASIGH_HSENT,    TFld(operac|ASIG_HSENT));
					SetTFld(operac|ASIGH_HSSAL,    TFld(operac|ASIG_HSSAL));
					SetFld (operac|ASIGH_REGIM,    SFld(operac|ASIG_REGIM));
					SetFld (operac|ASIGH_TIPODIA,  SFld(operac|ASIG_TIPODIA));
					SetDFld(operac|ASIGH_FECHAS,   DFld(operac|ASIG_FECHAS));
					SetDFld(operac|ASIGH_FFRANCO,  DFld(operac|ASIG_FFRANCO));
					SetIFld(operac|ASIGH_NUMFRAN,  IFld(operac|ASIG_NUMFRAN));
					SetIFld(operac|ASIGH_FRANCERO, IFld(operac|ASIG_FRANCERO));
					SetIFld(operac|ASIGH_CODROL,   IFld(operac|ASIG_CODROL));
					SetIFld(operac|ASIGH_FILA,     IFld(operac|ASIG_FILA));
					SetIFld(operac|ASIGH_COLUM,    IFld(operac|ASIG_COLUM));

					if (!FmIFld(fm0, ASIGNA, i)) {
						SetDFld(operac|ASIGH_FECBAJ, FmDFld(fm2, FECBAJ) < DFld(operac|ASIG_FECASIG) ?
													 DFld(operac|ASIG_FECASIG) : FmDFld(fm2, FECBAJ));
						SetIFld(operac|ASIGH_MOTIVO, FmIFld(fm2, MOTIVO));
						SetLFld(operac|ASIGH_REEMPL, FmLFld(fm2, REEMPL));
					}
					else {
						SetDFld(operac|ASIGH_FECBAJ, FmDFld(fm6, FECBAJ6) < DFld(operac|ASIG_FECASIG) ?
													 DFld(operac|ASIG_FECASIG) : FmDFld(fm6, FECBAJ6));
						SetIFld(operac|ASIGH_MOTIVO, FmIFld(fm6, MOTIVO6));
						SetLFld(operac|ASIGH_REEMPL, FmLFld(fm6, REEMPL6));
					}
					PutRecord(operac|ASIGH);
					FreeTable(operac|ASIGH);

					// Actualizo la cantidad de vigiladores asignados al puesto del que se saca si cambio el pto.
					if (FmIFld(fm0, I_PTOASI,   i) != FmIFld(fm1, TIPPTO) ||
						FmIFld(fm0, I_CODINASI, i) != FmIFld(fm1, CODINT)) {
						SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIG_CLIENTE));
						SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIG_OBJETIVO));
						SetIFld(operac|PUESTOS_TIPPTO,  FmIFld(fm0, I_PTOASI,   i));
						SetIFld(operac|PUESTOS_CODINT,  FmIFld(fm0, I_CODINASI, i));
						if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_LOCK) != ERROR) {
							SetIFld(operac|PUESTOS_VIGI, IFld(operac|PUESTOS_VIGI) < 100 ? 0 :
														 IFld(operac|PUESTOS_VIGI) - 100);

							PutRecord(operac|PUESTOS);
						}
					}
				}
				DelRecord(operac|ASIG);

			}
 			if (!FmIFld(fm0, ASIGNA, i) && 
 			    (LegActivo (FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i)) != 1 ||
 			     *FmSFld(fm0, EFECT, i) == 'P') ) {

				fm2 = UseSubform(fm0, ASIGNA, 0, i);
				BorrarParteGenerado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), 
				                    FmLFld(fm0, NROLEG, i), NULL_DATE, NULL_DATE, FmDFld(fm2, FECBAJ) + 1,
				                    TRUE, FmSFld(fm0, EFECT, i), FmIFld(fm0, I_PTOASI, i),
									FmIFld(fm0, I_CODINASI, i), FmIFld(fm0, I_INTASI, i));
			}
		}

		if (!FmIFld(fm0, ASIGNA, i)) 
			continue;
		else {
			InitRecord(operac|ASIG);
			SetIFld(operac|ASIG_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|ASIG_CLIENTE,  FmLFld(fm0, CLIEOT));
			SetIFld(operac|ASIG_OBJETIVO, FmIFld(fm0, OBJET));
			SetLFld(operac|ASIG_NROLEG,   FmLFld(fm0, NROLEG, i));
			SetFld (operac|ASIG_VIGIL,    FmSFld(fm0, VIGILAD, i));
			SetFld (operac|ASIG_EFECT,    FmSFld(fm0, EFECT,  i));
			SetDFld(operac|ASIG_FECASIG,  FmDFld(fm1, FECHA));
			SetIFld(operac|ASIG_PTOSER,   FmIFld(fm1, TIPPTO));
			SetIFld(operac|ASIG_PUESTO,   FmIFld(fm1, CODINT));
			SetIFld(operac|ASIG_NROINT,   FmIFld(fm1, I_NROINT));
			SetFld (operac|ASIG_DIA1,     FmSFld(fm1, DIA1));
			SetFld (operac|ASIG_DIA2,     FmSFld(fm1, DIA2));
			SetFld (operac|ASIG_DIA3,     FmSFld(fm1, DIA3));
			SetFld (operac|ASIG_DIA4,     FmSFld(fm1, DIA4));
			SetFld (operac|ASIG_DIA5,     FmSFld(fm1, DIA5));
			SetFld (operac|ASIG_DIA6,     FmSFld(fm1, DIA6));
			SetFld (operac|ASIG_DIA7,     FmSFld(fm1, DIA7));
			SetTFld(operac|ASIG_HSENT,    FmTFld(fm1, HORENT));
			SetTFld(operac|ASIG_HSSAL,    FmTFld(fm1, HORSAL));
			SetDFld(operac|ASIG_FFRANCO,  FmDFld(fm1, FECFRA));
			SetIFld(operac|ASIG_NUMFRAN,  FmIFld(fm1, NUMFRAN));
			SetIFld(operac|ASIG_FRANCERO, FmIFld(fm1, FRANCERO));
			SetFld (operac|ASIG_REGIM,    FmSFld(fm1, REGIM));
			SetFld (operac|ASIG_TIPODIA,  FmSFld(fm1, TIPODIA));
			SetIFld(operac|ASIG_CODROL,   FmIFld(fm1, CODROL));
			SetIFld(operac|ASIG_FILA,     FmIFld(fm1, FILA));
			SetIFld(operac|ASIG_COLUM,    FmIFld(fm1, COLUM));

			if (*FmSFld(fm0, EFECT, i) == 'P') {
				fm3 = UseSubform(fm1, FECHA, 0, i);
				SetDFld (operac|ASIG_FECHAS,  FmDFld(fm3, FECHAS));
			}
			else {
				SetDFld (operac|ASIG_FECHAS,  NULL_DATE);
			}

			PutRecord(operac|ASIG);
			FreeTable(operac|ASIG);

			// Borro los que haya tenido de antes para regrabar.
			SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
										  FmLFld(fm0, NROLEG, i), IFld(fm1, TIPPTO), FmIFld(fm1, CODINT),
										  FmIFld(fm1, I_NROINT), MIN_DATE);
			while (GetRecord(operac|DIASPTIMEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR){
				DelRecord(operac|DIASPTIME);
			}
			//Actualizo la asignación de los part-times que son por horas.
			for (j = 0; j < FmFldLen(fm5, MULTDIAS) && !FmIsNull(fm5, DIA, j); j++) {
				InitRecord(operac|DIASPTIME);
				SetIFld(operac|DIASPTIME_EMP,      FmIFld(fm0, EMP));
				SetLFld(operac|DIASPTIME_CLIENTE,  FmLFld(fm0, CLIEOT));
				SetIFld(operac|DIASPTIME_OBJETIVO, FmIFld(fm0, OBJET));
				SetLFld(operac|DIASPTIME_NROLEG,   FmLFld(fm0, NROLEG, i));
				SetIFld(operac|DIASPTIME_TIPPTO,   FmIFld(fm1, TIPPTO));
				SetIFld(operac|DIASPTIME_PUESTO,   FmIFld(fm1, CODINT));
				SetIFld(operac|DIASPTIME_NROINT,   FmIFld(fm1, I_NROINT));
				SetDFld(operac|DIASPTIME_DIA,      FmDFld(fm5, DIA, j));
				SetTFld(operac|DIASPTIME_HENT,     FmTFld(fm5, HENT, j));
				SetTFld(operac|DIASPTIME_HSAL,     FmTFld(fm5, HSAL, j));

				PutRecord(operac|DIASPTIME);
				FreeTable(operac|DIASPTIME);
			}
			// Borro los partes que tienen generados sin cargar porque hubo modificaiones en el puesto
			if (FmIFld(fm1, I_MODIF)) {
				BorrarParteGenerado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), 
				                    FmLFld(fm0, NROLEG, i), FmDFld(fm1, FECHA),  FmDFld(fm1, I_FCHHAS),
				                    NULL_DATE, FALSE, NULL_STR, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			}

			fm4 = UseSubform(fm0, NROLEG, 0, i);

			if (FmIFld(fm0, I_NEW, i)) {
				PonerEnASIGH(fm4, FmLFld(fm0, NROLEG, i), FmDFld(fm1, FECHA), FmIFld(fm6, MOTIVO6),
								  FmLFld(fm6, REEMPL6), fecbaja);
			}
		}
		VerificoDobleAsig(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i));
	}
}

static fm_status before(form fm, fmfield fno, int row)
{
	int v_legact=FALSE;
	row_fm0 = row;
	MuestraNrodeLineaActual(fno, row);
	
	switch (fno) {
    case CLIEOT: 
	   	InicClientesXusr();
    	break;
    case OBJET :
	   	InicObjetivosXusr(FmLFld(fm, CLIEOT), FmIFld(fm, EMP));
    	break;
    case NROLEG :
		fm1 = UseSubform(fm0, HORARIO, 0, row);

		SetKey(operac|ASIGbyEMP, FmIFld(fm, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm, fno, row), FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmIFld(fm1, I_NROINT));
		if (GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			FmSetDisplayOnly(fm, fno, fno, TRUE);
		}
		else {
			FmSetDisplayOnly(fm, fno, fno, FALSE);
		}

/*		Asi estaba antes esto daba error de descritop de tabla DHC 15/12/2010
		if (!FmIsNull(fm, fno, row) && EstaEnAsig(FmIFld(fm, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm, fno, row), 
		                                          FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmIFld(fm1, I_NROINT))) {
		                                          
			FmSetDisplayOnly(fm, fno, fno, TRUE);
		}
		else {
			FmSetDisplayOnly(fm, fno, fno, FALSE);
		}
*/	   	
    	break;
	case EFECT :
		if(FmIFld(fm0, I_BASE, row) )
			return FM_SKIP;
		break;
		
	case ASIGNA :
		v_legact = LegActivo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row));

		if(!FmIFld(fm0, I_BASE, row) && v_legact ) {
			FmSetIFld(fm, fno, 1, row);
			return FM_SKIP;
		}
		if(strcmp(FmSFld(fm0, EFECT, row),"E")==0 && v_legact ) {
			FmSetIFld(fm, fno, 1, row);
			return FM_SKIP;
		}
		break;
	case HORARIO :
		fm1 = UseSubform(fm0, fno, 0, row);

		GuardoFm1();

    	break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	bool efectivo,   //Si pongo Efectivo en el form.
		 estaefec,   //Si ya está asignado Efectivo en la base.
		 v_legact,   //Legajo esta activo?
		 v_cambio_legajo=FALSE;
	DATE fechahasta;
	int  i, juris;
	long v_nroleg=NULL_LONG;


	if (FmKeyCode(fm) == K_DEL && fno >= NROLEG && fno <= HORARIO) {
		if (FmIFld(fm, I_BASE, row)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2001), "No puede eliminar al vigilador.\nLinea %d", row);
			return FM_REDO;
		}
		else
			return FM_OK;
	}

	row_padre = row;
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm)){
			FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Clientes - Objetivos");
			WiRefresh();
			InicListaXusr(FmIFld(fm0, EMP));
			FmSetFld(fm0, MENSAJE ,"Cargando Permisos Sobre Legajos");
			WiRefresh();
			InicLegajoXusr (FmIFld(fm, EMP), Today(), fm0, MENSAJE, TRUE, _TIPPER_ASIGNA);
		}
	    break;

	case CLIEOT:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);

		if (ValidaClienteXusr(FmLFld(fm, CLIEOT, row)))
		  	FmSetFld(fm, DCLIEOT, GetDescCliente(FmLFld(fm, CLIEOT, row)), row);
		else {
 			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIEOT, row));
			FmSetLFld(fm, CLIEOT, NULL_LONG, row);
			FmSetFld(fm, DCLIEOT, NULL_STR, row);
			return FM_REDO;
		}	
		break;

	case OBJET :
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIEOT, row));
  		else
			if (ValidaObjetivoXusr(FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
				FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row)), row);
			else	{
				Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row));
				FmSetIFld(fm, OBJET, NULL_SHORT, row);
				FmSetFld(fm, DOBJET, NULL_STR, row);
				return FM_REDO;
    		}
		
		LeerObjetivo(FmLFld(fm, CLIEOT), FmIFld(fm, fno));
//		if (!EstaEnLaPolicia(FmIFld(fm, EMP), FmLFld(fm, CLIEOT), FmLFld(fm, fno)))
//			Warning("El Objetivo no está dado de alta en la Policia.");

		//guardo filial del cliente objetivo
		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET)));	

		break;
	case AGRUPLIN: 
		if (FmKeyCode(fm) != K_DEL){
			if (!ControlesNivelLinea(row, FmChgFld(fm))) 
    		 	return FM_REDO;
		}

		break;
	case NROLEG :
		switch(FmKeyCode(fm)) {
			case K_HELP:
				if (FmIsDisplayOnly(fm, fno, row))
					return FM_REDO;
				HelpLegajo(fm0, NROLEG, row);
				break;
			case K_AF1:
				if (FmIsDisplayOnly(fm, fno, row))
					return FM_REDO;
				v_nroleg = ERROR;
				if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;
					FmSetLFld(fm, fno, v_nroleg, row);
 				break;
 			case K_META : 
 				fm4 = UseSubform(fm0, NROLEG, 0, row);
 				CargarOtrasAsignaciones(fm4, FmLFld(fm0, NROLEG, row));
 				DoSubform(fm0, NULLFP, NULLFP, fno, 0, row);
 				break; 
		}

		//  Controla que no se cambie el legajo de un linea ya cargada
		if (FmChgFld(fm) && strcmp(FmFldPrev(fm), NULL_STR) != 0) {
			if (WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_ERR, 2069), "No se puede cambiar un legajo, sin eliminar la linea\n desea continuar?\nLinea %d", row)==WD_YES) 
				FmSetKeyCode(fm0, K_DEL);
			else 
				FmSetLFld(fm0, NROLEG, StrToL(FmFldPrev(fm0)), row);
		}
	
		fm1 = UseSubform(fm0, HORARIO, 0, row);

		if (!FmIsNull(fm, fno, row)) {
			SetKey(operac|ASIGbyEMP, FmIFld(fm, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm, fno, row), FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmIFld(fm1, I_NROINT));
			if (GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				v_cambio_legajo=FALSE;
			}
			else {
				v_cambio_legajo=TRUE;
			}
		}
/*		Asi estaba antes esto daba error de descritop de tabla DHC 15/12/2010
		if (!FmIsNull(fm, fno, row))
			v_cambio_legajo= !EstaEnAsig(FmIFld(fm, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm, fno, row), 
			                             FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmIFld(fm1, I_NROINT));
		
*/
		// Valida que el legajo seleccionado no está inactivo!
		if (!FmIsNull(fm0, fno, row)){

			v_legact=LegActivo (FmIFld(fm, EMP), FmLFld(fm, fno, row));
			if (v_legact == 0 || v_legact == 2) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2068), "El legajo se encuentra inactivo.\nLinea %d", row);

				if (v_cambio_legajo && !ExisteCliObjEnGrp(GRPBAJA, FmLFld(fm0, CLIEOT), FmLFld(fm0, OBJET) ) )
					return FM_REDO;
			
			}

		}

		// Valido Legajo por Filiales
		if (!FmIsNull(fm, fno, row) && v_cambio_legajo){
			if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG, row), MAX_DATE))
				if (!FmIsNull(fm, fno, row)) {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2067), "No tiene permiso para ingresar este legajo\n Linea[%d]", row);
					return FM_REDO;
				}
		}
		FmSetFld(fm, NOMBRE, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row)), row);



		if(v_cambio_legajo)
			if (strcmp(FmFldPrev(fm), NULL_STR)==0) 
				FmSetIFld(fm0, I_NEW, TRUE, row);


		juris = GetJurisdiccion(FmLFld(fm, CLIEOT), FmIFld(fm, OBJET));

/*		if (v_cambio_legajo && !InscriptoEnJuris(FmIFld(fm, EMP), FmLFld(fm, fno, row), StrToI(ReadEnv("PAIS")), juris)) 
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2004),
			         "El Vigilador no está inscripto en la Jurisdicción %s.\nLinea %d", 
			         GetDescProv(StrToI(ReadEnv("PAIS")), juris), row);
*/
		break;
	case ASIGNA :
		if (FmIFld(fm0, ASIGNA, row))
			break;

		fm1 = UseSubform(fm0, HORARIO, 0, row);
		fm2 = UseSubform(fm,  ASIGNA,  0, row);
		FmSetDFld(fm2, I_FECASIG2, FmDFld(fm1, FECHA));

		fm3	= UseSubform(fm1, FECHA,   0);
		fechahasta = FmDFld(fm3, FECHAS);

		// esto se hace para dar la posibilidad de asignar a alguien a ese puesto que queda libre.
		pue[FmIFld(fm1, TIPPTO)][FmIFld(fm1, CODINT)] = pue[FmIFld(fm1, TIPPTO)][FmIFld(fm1, CODINT)] < 100 ?
													0 : pue[FmIFld(fm1, TIPPTO)][FmIFld(fm1, CODINT)] - 100;

		break;
	case AGRASIGNA:
		if ((strcmp(FmFldPrev(fm), "R") == 0 || strcmp(FmFldPrev(fm), "V") == 0) && *FmSFld(fm, VIGILAD) == 'P') {
			fm1 = UseSubform(fm0, HORARIO, 0, row);
			FmClearAllFlds(fm1);
		}
		if (strcmp(FmFldPrev(fm), "P") == 0 && (*FmSFld(fm, VIGILAD) == 'R' || *FmSFld(fm, VIGILAD) == 'V')) {
			fm1 = UseSubform(fm0, HORARIO, 0, row);
			FmClearAllFlds(fm1);
		}
		if (StrCmp(FmSFld(fm, EFECT, row), PROVISORIO) == 0)
			FmSetIFld(fm, I_CHGEFEC0, FALSE, row);

		// Verificacion de efectivo y provisorio y tipo de vigilante siempre el mismo en todos los clientes.
		efectivo = (StrCmp(FmSFld(fm, EFECT, row), EFECTIVO) == 0);
		estaefec = FALSE;

		for (i=0; i<FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
			if (FmLFld(fm0, NROLEG, i) != FmLFld(fm0, NROLEG, row))
				continue;
			if (row == i)
				continue;
			if (!StrCmp(FmSFld(fm0, EFECT, i), EFECTIVO))
				estaefec = TRUE;

			if (StrCmp(FmSFld(fm, VIGILAD, row), FmSFld(fm, VIGILAD, i))){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2006),"El Vigilador ya es de tipo %s\nLinea %d", FmSFld(fm, VIGILAD, i), row);
				return FM_REDO;
			}

			if (FmIsNull(fm0, IS_EFECT, row) && efectivo  && StrCmp(FmSFld(fm0, EFECT, row), FmSFld(fm0, EFECT,i))==0){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2007),
				         "El Vigilador ya se encuentra asignado en forma efectiva en el cliente/objetivo\nLinea %d", row);
				return FM_REDO;
			}
		}

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {

			if (!StrCmp(SFld(AlFld(asig, operac|ASIG_EFECT)), EFECTIVO)) {
				estaefec = TRUE;
				FmSetDFld(fm, I_FECEFEC,    DFld(AlFld(asig, operac|ASIG_FECASIG)),  row);
				FmSetLFld(fm, I_CLIEFEC,    LFld(AlFld(asig, operac|ASIG_CLIENTE)),  row);
				FmSetIFld(fm, I_OBJEFEC,    IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row);
				FmSetIFld(fm, I_PTOEFEC,    IFld(AlFld(asig, operac|ASIG_PTOSER)),   row);
				FmSetIFld(fm, I_CODINTEFEC, IFld(AlFld(asig, operac|ASIG_PUESTO)),   row);
				FmSetDFld(fm, I_FFINPTOEFC, FechaFinPuesto(LFld(AlFld(asig, operac|ASIG_CLIENTE)),
							IFld(AlFld(asig, operac|ASIG_OBJETIVO)), IFld(AlFld(asig, operac|ASIG_PTOSER)),
							IFld(AlFld(asig, operac|ASIG_PUESTO))),  row);
			}
			if (StrCmp(FmSFld(fm, VIGILAD, row), SFld(AlFld(asig, operac|ASIG_VIGIL))) != 0) {
				if (efectivo) {
					if (WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_WAR, 2008), 
					    "El Vigilador ya es de tipo %s en el Cliente %ld y Objetivo %d.\nDesea modificarlo?\nLinea %d",
						SFld(AlFld(asig, operac|ASIG_VIGIL)), LFld(AlFld(asig, operac|ASIG_CLIENTE)),
						IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row) == WD_NO)
						return FM_REDO;
				}
				else {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2009),
				             "El Vigilador ya es de tipo %s en el Cliente %ld y Objetivo %d.\nLínea %d", 
				              SFld(AlFld(asig, operac|ASIG_VIGIL)),	LFld(AlFld(asig, operac|ASIG_CLIENTE)),
				              IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row);
				    return FM_REDO;
				}
			}

			if (efectivo && estaefec && !FmIFld(fm, I_BASE, row)) {

				if(WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_WAR, 2010), 
				   "El Vigilador ya se encuentra efectivo en el Cliente: %ld y Objetivo: %d.\nDesea reasignarlo ?.\nLínea %d",
				   LFld(AlFld(asig, operac|ASIG_CLIENTE)),
				   IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row) == WD_YES) {

					FmSetIFld(fm0, I_BASE, TRUE, row);
					fm4 = UseSubform(fm, NROLEG, 0, row);
					CargarOtrasAsignaciones(fm4, FmLFld(fm, NROLEG, row));
					FmSetIFld(fm, I_CHGEFEC0, TRUE, row);
					return FM_OK;
				}
				else {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2011),
				         "El Vigilador ya se encuentra efectivo en el Cliente: %ld y Objetivo: %d\nLínea %d",
				         LFld(AlFld(asig, operac|ASIG_CLIENTE)), IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row);
				    return FM_REDO;
				}
			}
		}
		if (!efectivo && !estaefec) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2012),
			         "El Vigilador no tiene asociado un puesto donde este efectivo.\nPara asignarlo como provisorio, primero debe estar efectivo!\nLínea %d", row);
			return FM_REDO;
		}
		break;
	case HORARIO :
		fm1 = UseSubform(fm0, HORARIO, 0, row);

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			if (FmLFld(fm0, CLIEOT) == LFld(AlFld(asig, operac|ASIG_CLIENTE)) &&
				FmIFld(fm0, OBJET)  == IFld(AlFld(asig, operac|ASIG_OBJETIVO)))
				continue;
			if (!StrCmp(SFld(AlFld(asig, operac|ASIG_EFECT)), EFECTIVO))
				FmSetDFld(fm, I_FECEFEC, DFld(AlFld(asig, operac|ASIG_FECASIG)), row);
		}

		if (FmKeyCode(fm)==K_META || FmIsNull(fm1, FECHA, row)) 
			Cargo_fm1(fm, fno, row);

		break;
	}
	return FM_OK;
}

static fm_status before_asige1(form fm, fmfield fno, int row)
{
	char regimen[18];

	campo_fm1=fno;

	switch (fno) {
	case AGRTOT :
		if (FmDFld(fm, FECHA) == FmDFld(fm, I_FCHDES))
			return FM_SKIP;
		break;
	case TIPPTO :
		ptoser = FmIFld(fm, TIPPTO);
		codint = codint != 0 && FmIsNull(fm, CODINT) ? codint : FmIFld(fm, CODINT);
		break;
	case CODINT :
		codint = codint != 0 && FmIsNull(fm, CODINT) ? codint : FmIFld(fm, CODINT);
		break;
	case REGIM :
		if (!FmIFld(fm, I_EXISTE) || FmChgFld(fm)) {
			if (!StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)) {
				FmSetFld(fm, REGIM, GetRegimen(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO),
											   FmIFld(fm, CODINT)));
			}
			else {
				GetRegimenEfectivo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0), regimen, FmDFld(fm, FECHA));
				if (!StrCmp(regimen, NULL_STR))
					GetRegimenEfectivoFm(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0), regimen, FmDFld(fm, FECHA));
				FmSetFld(fm, REGIM, regimen);
			}
		}
		return FM_SKIP;
	case FECFRA: 
		if (!FmIsNull(fm1, CODROL))
			return FM_SKIP;
		break;

	case NUMFRAN:
		if (!FmIsNull(fm1, CODROL)) 
			return FM_SKIP;
		if (GetDiasFranco(FmSFld(fm, REGIM), FmIsNull(fm, HORAPT)? FALSE : TRUE) == 1)
			return FM_SKIP;
		break;
	case FECHA :
		fecasig = FmDFld(fm, fno);
		break;
	case HORENT:
	case HORSAL:
		horsal = FmTFld(fm, I_HORSAL);
		horent = FmTFld(fm, I_HORENT);
		break;

	case CODROL:
	case FILA:
	case COLUM:
		if (!UsrInGrupo(GRPROL, GetUid()))
			return FM_SKIP; 

		break;
	}

	return FM_OK;
}

static void GetRegimenEfectivoFm(int emp, long nroleg, char *regimen, DATE fecha)
{
	form fm;
	int  i;
      
	for(i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		if(FmLFld(fm0, NROLEG, i) != nroleg)
			continue;
		if(*FmSFld(fm0, EFECT, i) != 'E')
			continue;

		fm = UseSubform(fm0, HORARIO, 0, i);
		strcpy(regimen, FmSFld(fm, REGIM));
		break;
	}
	// Se hizo esto porque perdi el row del form fm0.
	RestauraSubForm(fm0, HORARIO, 0, row_padre);
}

static fm_status before_asige6(form fm, fmfield fno, int row)
{
	FmSetIFld(fm, I_EMP6, FmIFld(fm0, EMP)) ;

	return FM_OK;
}

static fm_status after_asige1(form fm, fmfield fno, int row)
{
	DATE fecmax, fecha, fechocup, v_calcfec=NULL_DATE, v_fecultasi=NULL_DATE;
	int  n, v_lugar=NULL_SHORT;
	bool puestopt;
	int v_codint_ant;
	int v_codcat=NULL_SHORT;
	int v_francos=0;
	double 	salario_legajo=0, 
			salario_puesto=0;


	char regimen[18];
   
	switch (fno) {
	case AGRTOT :
		if (!FmIFld(fm, I_MODIF) && FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES)) {
			FmSetDFld(fm, FECHA, FmDFld(fm, I_FCHDES));

			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2013),
			"Se ha modificado solamente la fecha de asignación, este no es un cambio válido.");
			return FM_REDO;
		}
		break;
	case FECHA :
		if ( !ValidaLegajoXusr(FmLFld(fm0, NROLEG, row_fm0), FmDFld(fm1, FECHA)))
			if (!FmIsNull(fm, fno, row)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2065), "No tiene permiso para ingresar este legajo para el %.3D\n Linea[%d]", FmDFld(fm1, FECHA), row);
				return FM_REDO;
			}

		SetIFld(sue|DATPERS_EMP,    FmIFld(fm0, EMP));
		SetLFld(sue|DATPERS_NROLEG, FmLFld(fm0, NROLEG, row_fm0));
		GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (FmDFld(fm, FECHA) < DFld(sue|DATPERS_FECANT)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2014),
			"La Fecha de Asignación debe ser mayor o igual a la Fecha de Ingreso: %.1D", 
			DFld(sue|DATPERS_FECANT));
			return FM_REDO;
		}
		fm3 = UseSubform(fm, FECHA, 0);

		fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));


		FmSetIFld(fm, I_EXISPAR, FALSE);
		if (FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES) && FmDFld(fm, FECHA) <= fecierre) {
			FmSetIFld(fm, I_EXISPAR, TRUE);
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2078), "El periodo al que pertenece esta fecha ya esta cerrado\nFecha de Cierre %.3D", fecierre); 
			return FM_REDO;
			
		}


		if (ExisteParteCargado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm0, NROLEG, row_fm0), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT), FmDFld(fm, FECHA), FmDFld(fm3, FECHAS))) {
			FmSetIFld(fm, I_EXISPAR, TRUE);
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2015), MSG_PARTE); 
			return FM_REDO;
		}

		fecierrefil = GetFechaCierreFilial(filial);
		if (FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES) && fecierrefil != NULL_DATE && FmDFld(fm, fno) <= fecierrefil) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2061),"El Parte está cerrado para la Filial %s el %.3D.\nNo podrá modificarse.", filial, fecierrefil);
			return FM_ERROR;
		}

		if (FmIFld(fm, I_CHGEFEC)) {
			if (!ValidarPartesProvisorios(FmDFld(fm, FECHA), fm)) {
				FmSetFld(fm, FECHA, FmFldPrev(fm));
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2061),
				"No es una fecha válida para la reasignación del efectivo, ya que existen partes \ncargados con fechas posteriores.");
				return FM_REDO;
			}

			if (FmIFld(fm0, I_NEW, row_padre)) {
				//El efectivo estaba asignado en un pto vencido y la nueva fecha de asignacion
				//es mayor a la fecha de fin del puesto en mas de un dia.
				if (!FmIsNull(fm0, I_FFINPTOEFC, row_padre) && 
					FmDFld(fm, fno) > FmDFld(fm0, I_FFINPTOEFC, row_padre) + 1){
					if (g_GRPSUPOPER) {
						if (WiDialog(WD_YES|WD_NO, WD_NO, TituloMsg(TMSG_ERR, 2017), "Cambio de Asignación Efectiva.\nLa Fecha de Asignación debe ser %.1D ya que el puesto anterior \nfinalizó el día %.1D.\nðDesea continuar con el cambio de asignación?", FmDFld(fm0, I_FFINPTOEFC, row_padre) + 1, FmDFld(fm0, I_FFINPTOEFC, row_padre)) != WD_YES)
							return FM_REDO;
					}
					else {
						WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2017), "Cambio de Asignación Efectiva.\nLa Fecha de Asignación debe ser %.1D ya que el puesto anterior \nfinalizó el día %.1D.", FmDFld(fm0, I_FFINPTOEFC, row_padre) + 1, FmDFld(fm0, I_FFINPTOEFC, row_padre));
						return FM_REDO;
					}
				}
			}
		}

		if (StrCmp(FmSFld(fm, I_EFECT), PROVISORIO) == 0) {
			if (FmDFld(fm, FECHA) < FmDFld(fm0, I_FECEFEC, row_fm0)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2018),
				         "La Fecha de Asignación debe ser mayor o igual\n a la Fecha de Asignación del puesto efectivo %.1D", 
				         FmDFld(fm0, I_FECEFEC, row_fm0));
				return FM_REDO;
			}


			DoSubform(fm, NULLFP, after_asige3, fno, 0);
			if (FmIsNull(fm3, FECHAS))
				return FM_ERROR;
		}

		// Controlo que cuando hago una nueva asignacion la fecha se mayor o mayor igual (depende si es provisorio o efectivo), que la de la ultima asignacion
        v_fecultasi = FechaInicioUltAsigEfec(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0));

		if (FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES) && !FmIsNull(fm, FECHA)){
			if (!StrCmp(FmSFld(fm, I_EFECT), PROVISORIO)){
				if (FmDFld(fm, FECHA) < v_fecultasi) {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2066),"La fecha debe ser posterior o igual a la\n ultima fecha de la asignacion efectiva(%.3D)", v_fecultasi);
					return FM_REDO;
				}
			}
    		else {
				if (FmDFld(fm, FECHA) <= v_fecultasi) {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2063),"La fecha debe ser posterior a la\n ultima fecha de la asignacion efectiva(%.3D)", v_fecultasi);
					return FM_REDO;
				}
			}
		}

		if (ExisteCliObjEnGrp(GRPBAJA, FmLFld(fm0, CLIEOT), FmLFld(fm0, OBJET))) {
			if (TieneProvisorioVigente(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_padre), FmDFld(fm1, FECHA))){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2076),"El Legajo Tiene Asignaciones Provisorias\n (Antes de asignar al cliente de baja se deberian bajar) ");
				return FM_REDO;
			}
		} 

		FmSetDFld(fm0, I_FECININ, FmDFld(fm, FECHA), row_padre);

		// esto se hace porque por una extraña razon pierde el descriptor del subformulario
		fm1 = fm;
		break;
	case TIPPTO :
		if (FmKeyCode(fm) == K_HELP) {
			SetCursorFrom(c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MIN_SHORT);
			SetCursorTo  (c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MAX_SHORT);

			n = PopUpDbMenu(10, 40, " Puestos de Trabajo ", c_ptoser, 4, validate, display);
			if (n >= 0) {
				if (FmIFld(fm, fno) != IFld(comerc|PTOSER_TIPPTO)) {
					FmSetIFld(fm, fno, IFld(comerc|PTOSER_TIPPTO));
					FmClearFlds(fm, CODINT, HORSAL);
					FmShowFlds (fm, CODINT, HORSAL);
				}
			}
		}
		if (ptoser != FmIFld(fm, TIPPTO)) {  // cambio el codigo y pudo porque no tiene parte cargado
			FmSetIFld(fm, I_MODIF, TRUE);
		}
		SetKey(comerc|PTOSERbyCLIENTE, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, fno, row));
		if (GetRecord(comerc|PTOSERbyCLIENTE, THIS_KEY, IO_NOT_LOCK)==ERROR 
		 || BajaPuesto(LFld(comerc|PTOSER_CLIENTE), IFld(comerc|PTOSER_OBJET), IFld(comerc|PTOSER_TIPPTO),	FmDFld(fm1, FECHA)) ){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2070), "El Tipo Puesto no es uno existente para el cliente/objetivo");
			return FM_REDO;
		}

		break;
	case CODINT :
		v_codint_ant=FmIFld(fm, CODINT); // guardo el actual

		if (FmKeyCode(fm) == K_HELP) {
			HelpPto(fm, fno, row, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO),
			        FmDFld(fm, FECHA), FmSFld(fm, I_VIGILAD), FmSFld(fm, I_EFECT));
		}
		if (!FmIsNull(fm, CODINT)) {
			SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
											FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));
			if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2019), "El Puesto no es uno existente para el cliente/objetivo");
				FmSetIFld(fm, CODINT, NULL_SHORT);
				return FM_REDO;
			}
		}

		if ( v_codint_ant != IFld(operac|PUESTOS_CODINT) || FmChgFld(fm)){  // si cambio
	  		FmClearFlds(fm, HORAPT, HORSAL);
	 		FmShowFlds (fm, HORAPT, HORSAL);
		}

		if ((!IsNull(operac|PUESTOS_FFINAL) && FmDFld(fm, FECHA) > DFld(operac|PUESTOS_FFINAL)) ||
			FmDFld(fm, FECHA) < DFld(operac|PUESTOS_FINICIO)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2020), 
			         "La Fecha de Asignación debe estar dentro del periodo de duración del puesto. (%.1D - %.1D)", 
			         DFld(operac|PUESTOS_FINICIO), DFld(operac|PUESTOS_FFINAL));
			FmNextFld(fm, FECHA);
		}

		FmSetFld (fm, TIPODIA,   SFld(operac|PUESTOS_TIPODIA));
		FmSetIFld(fm, I_VIGI,    IFld(operac|PUESTOS_VIGI));
		FmSetIFld(fm, I_CANTVIG, IFld(operac|PUESTOS_CANTVIG));
		FmSetIFld(fm, I_CATEG,   IFld(operac|PUESTOS_PUESTO));
		FmSetTFld(fm, I_HORENT,  TFld(operac|PUESTOS_HINICIO));
		FmSetTFld(fm, I_HORSAL,  TFld(operac|PUESTOS_HFINAL));
		FmSetFld (fm, I_DIA1,    SFld(operac|PUESTOS_DIA1));
		FmSetFld (fm, I_DIA2,    SFld(operac|PUESTOS_DIA2));
		FmSetFld (fm, I_DIA3,    SFld(operac|PUESTOS_DIA3));
		FmSetFld (fm, I_DIA4,    SFld(operac|PUESTOS_DIA4));
		FmSetFld (fm, I_DIA5,    SFld(operac|PUESTOS_DIA5));
		FmSetFld (fm, I_DIA6,    SFld(operac|PUESTOS_DIA6));
		FmSetFld (fm, I_DIA7,    SFld(operac|PUESTOS_DIA7));
		FmSetIFld(fm, I_HORAPT,  IFld(operac|PUESTOS_HORAPT));
		FmSetFld (fm, I_CODFREC, SFld(operac|PUESTOS_CODFREC));
		FmSetDFld(fm, I_FINICIO, DFld(operac|PUESTOS_FINICIO));
		FmSetDFld(fm, I_FFINAL,  DFld(operac|PUESTOS_FFINAL));
		FmSetFld (fm, DCODFREC,  DescrFrecuencia(FmSFld(fm, I_CODFREC)));

		fm3 = UseSubform(fm, FECHA, 0, 0);
		if (FmIsNull(fm, I_NROINT))
			FmSetIFld(fm, I_NROINT, CalcNroint(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
											   FmLFld(fm0, NROLEG, row_fm0), FmIFld(fm, TIPPTO), 
											   FmIFld(fm, CODINT)));

		if (codint != NULL_SHORT && codint != FmIFld(fm, CODINT)){
			FmSetIFld(fm, I_MODIF, TRUE);
		}

		sprintf(regimen, "%s", GetRegimen(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT)));

		if ((strcmp(regimen, _REGIM_EXTRA50)==0 || strcmp(regimen, _REGIM_EXTRA100)==0) && StrCmp(FmSFld(fm, I_EFECT), EFECTIVO) == 0) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2064), "Solo se puede cargar el regimen %s, en una asignacion que no sea efectiva", regimen);
			return FM_REDO;
		}


		break;
	case CODROL :
		rol = FmIFld(fm, fno);

		if (FmChgFld(fm)) {
			FmSetIFld(fm, I_MODIF, TRUE);

			if (CalculaFechaFranco(FmIFld(fm, CODROL), FmIFld(fm, FILA), FmIFld(fm, COLUM),
			                       FmDFld(fm1, FECHA), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
	                               FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), &v_calcfec, &v_lugar)){
				FmSetDFld(fm1, FECFRA,  v_calcfec);
				FmSetIFld(fm1, NUMFRAN, v_lugar);
	                               	
			}
		}

		break;
	case FILA :
		if (FmKeyCode(fm) == K_HELP) {
			HelpFila(rol);
		}

		if (FmChgFld(fm)) {
			FmSetIFld(fm, I_MODIF, TRUE);
			if (CalculaFechaFranco(FmIFld(fm, CODROL), FmIFld(fm, FILA), FmIFld(fm, COLUM),
			                       FmDFld(fm1, FECHA), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
	                               FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), &v_calcfec, &v_lugar)){
				FmSetDFld(fm1, FECFRA,  v_calcfec);
				FmSetIFld(fm1, NUMFRAN, v_lugar);
			}
		}

		break;
	case COLUM: 
		if (FmChgFld(fm)) {
			FmSetIFld(fm, I_MODIF, TRUE);
			if (CalculaFechaFranco(FmIFld(fm, CODROL), FmIFld(fm, FILA), FmIFld(fm, COLUM),
			                       FmDFld(fm1, FECHA), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
	                               FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), &v_calcfec, &v_lugar)) {
				FmSetDFld(fm1, FECFRA,  v_calcfec);
				FmSetIFld(fm1, NUMFRAN, v_lugar);
	                               	
			}

		}
		break;

	case AGRPUESTO :
		puestopt = PuestoEsPartime(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));

		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME)==0  && !puestopt && StrCmp(FmSFld(fm, I_EFECT), EFECTIVO) == 0) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2021), "El puesto elegido no es PartTime y el Vigilador si!");
			return FM_REDO;
		}

		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME)!=0 && puestopt && !StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2022), "El puesto elegido es PartTime y el Vigilador no!");
			return FM_REDO;
		}

		// lo puse aca y no en el after de CONINT para que salte el warning cuando el puesto esta bien ingresado
		v_codcat = GetCategoria(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0));

		salario_legajo = TotalSalario(CONVENIO, v_codcat) * 1000;
		salario_puesto = TotalSalario(CONVENIO, FmIFld(fm, I_CATEG)) * 1000;

		if (salario_legajo>salario_puesto) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2079), "La Categoria Salarial del Legajo es Mayor a la Vendida\nCategoria del Legajo $%.2f\nCategoria del Puesto $%.2f", salario_legajo, salario_puesto);
//			return FM_REDO;
		}
		else
			if (v_codcat != FmIFld(fm, I_CATEG)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2023), 
				         "El Vigilador tiene categoria %ld.\n Diferente a la categoria %d que necesita el puesto.", 
				         v_codcat, FmIFld(fm, I_CATEG));
			}
		break;
	case AGRASIG :
		// hubo un cambio de puesto:
		if ((codint != NULL_SHORT && codint != FmIFld(fm, CODINT)) ||
			(ptoser != NULL_SHORT && ptoser != FmIFld(fm, TIPPTO))) {
			pue[ptoser == NULL_SHORT ? FmIFld(fm, TIPPTO) : ptoser]
			   [codint == NULL_SHORT ? FmIFld(fm, CODINT) : codint] -= FmIFld(fm, I_CANTASIG);

			FmSetIFld(fm, I_CANTASIG, 0);
			FmSetIFld(fm, I_NODESCONTO, TRUE);
		}
		if (FmIFld(fm, I_NODESCONTO)) {
			if ((FmIFld(fm, I_CANTVIG) - pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)] < 100) &&
				(FmIFld(fm, I_CANTVIG) - pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)] > 0)) {
				FmSetIFld(fm, I_CANTASIG, FmIFld(fm, I_CANTVIG) - pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)]);
			}
			else {
				FmSetIFld(fm, I_CANTASIG, 100);
			}
			pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)] += FmIFld(fm, I_CANTASIG);
			FmSetIFld(fm, I_NODESCONTO, FALSE);
		}
		fm3 = UseSubform(fm, FECHA, 0);
		if (pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)] > FmIFld(fm, I_CANTVIG)) {
			if (StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)==0) 
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2024), 
			         "El Puesto tiene todos los vigiladores asignados.\n[1mPor favor, verifique los datos de asignación del vigilador.[0m");


			// si el error me dio para una asignación de provisorios y
			// hay alguna de los efectivos de vacaciones o con licencia tudu bom!
			if (StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)!=0 && !FmIFld(fm, I_EXISTE) &&
				!HayVacante(FmLFld(fm0, NROLEG, row_fm0), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT),
							FmDFld(fm, FECHA), FmDFld(fm3, FECHAS), &fechocup)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2025),
			         "El Puesto tiene todos los vigiladores asignados para el día: %.1D.\n     [1mPor favor, verifique los datos de asignación del vigilador.[0m", fechocup);

			}

			if (CalculaFechaFranco(FmIFld(fm, CODROL), FmIFld(fm, FILA), FmIFld(fm, COLUM),
			                       FmDFld(fm1, FECHA), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
	                               FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), &v_calcfec, &v_lugar)){

				FmSetDFld(fm1, FECFRA,  v_calcfec);
				FmSetIFld(fm1, NUMFRAN, v_lugar);
				
			}
			else {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2057),
			             "No coincide lo cargado en el rol con los dias de las semana ingresados.\n El primer franco según el rol deberia ser el %s %.3D",
			             DayName(v_calcfec), v_calcfec);
				FmNextFld(fm, CODROL);
			}

		}

		break;
	case FECFRA :
		if (FmChgFld(fm)) {
			FmSetIFld(fm, I_MODIF, TRUE);
		}
		if (!FmIsNull(fm, fno) && FmDFld(fm, fno) < FmDFld(fm, FECHA)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2026), MSG_FECFRA, FmDFld(fm, FECHA), FmDFld(fm, I_FFINAL));
			return FM_REDO;
		}

		if (FmDFld(fm, fno) < FmDFld(fm, I_FINICIO) || (!FmIsNull(fm, I_FFINAL) && FmDFld(fm, fno) > FmDFld(fm, I_FFINAL))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2027), MSG_FECFRA, FmDFld(fm, I_FINICIO), FmDFld(fm, I_FFINAL));
			return FM_REDO;
		}

		if (MenorAMinFecFranco(FmDFld(fm, FECHA), FmSFld(fm, I_VIGILAD), FmDFld(fm, fno), FmSFld(fm, REGIM), &fecmax)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2028), 
			         "Según la fecha de asignación y el regimen ingresado\nesta fecha de franco debe ser menor o igual a %.1D", fecmax);
			return FM_REDO;
		}
		break;
	case HORENT :
		if (!HoraEnRangoHorario(FmTFld(fm, fno), horent, horsal)){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2029), 
			"La hora de entrada debe estar dentro del rango del puesto (%.3T-%.3T)", horent, horsal);
			return FM_REDO;
		}

		if (FmTFld(fm, fno) == StrToT("235900")){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2031), "La hora de entrada no puede ser %.3T",  FmTFld(fm, fno));
			return FM_REDO;
		}
		break;

	case HORSAL :

		if (GetServicioObj(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET)) != BRIGADA){
			if (FmTFld(fm, HORSAL) == StrToT("0000")) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2032), "El horario debe ser distinto a 00:00.");
				return FM_REDO;
			}
		}
		else 
			if (FmTFld(fm, HORSAL) == StrToT("0000") && FmTFld(fm, HORENT) != StrToT("0000")) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2033), "El horario de salida no puede ser 00:00.\nsi el de entrada no es el mismo");
				FmNextFld(fm, HORENT);
			}

		if (!HoraEnRangoHorario(FmTFld(fm, fno), horent, horsal)){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2030), 
			        "La hora de salida debe estar dentro del rango del puesto (%.3T-%.3T)", horent, horsal);
			return FM_REDO;
		}

		if (FmChgFld(fm))
			FmSetIFld(fm, I_MODIF, TRUE);

		fecha = NULL_DATE;
		if (!StrCmp(FmSFld(fm, I_EFECT), PROVISORIO)) {
			fm3   = UseSubform(fm, FECHA, 0);
			fecha = FmDFld(fm3, FECHAS);
		}

		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) != 0) {
			// Solo ejecuta la funcion ValidPuestoOcup cuando el objetivo no es el 1000-6 (Prosegur) Y
			// el vigilator no es reten. Esto significa que si el vigilador es reten y esta siendo asignado
			// al objetivo 1000-6 (Prosegur) NO se debe validar superposicion.

			if (!(StrCmp(FmSFld(fm, I_VIGILAD), RETEN) == 0 &&
			    ExisteCliObjEnGrp(GRPRETPLANTA, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET))) &&
				StrCmp(TipoDia(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO),
					   FmIFld(fm, CODINT)), "F") &&
				!ValidPuestoOcup(FmLFld(fm0, NROLEG, row_fm0), FmDFld(fm, FECHA), FmTFld(fm, HORENT),
							FmTFld(fm, HORSAL), FmSFld(fm, DIA1), FmSFld(fm, DIA2), FmSFld(fm, DIA3),
							FmSFld(fm, DIA4), FmSFld(fm, DIA5), FmSFld(fm, DIA6), FmSFld(fm, DIA7),
							(!StrCmp(FmSFld(fm, I_EFECT), PROVISORIO)) ? FmDFld(fm, I_FECHAS) : NULL_DATE,
							row_padre, FmLFld(fm0, I_CLIEFEC, row_padre), FmIFld(fm0, I_OBJEFEC, row_padre),
							FmIFld(fm0, I_PTOEFEC, row_padre), FmIFld(fm0, I_CODINTEFEC, row_padre),
							FmSFld(fm, I_EFECT))) {
				// como queda el último registro corriente, tomo esos datos.
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2035),MSG_SUPERPOS, FmSFld(fm0, NOMBRE, row_fm0),
				                                             asigSt.cliente, asigSt.objetivo, 
															 asigSt.fecasig, asigSt.dia1, asigSt.dia2,
															 asigSt.dia3, asigSt.dia4, asigSt.dia5,
															 asigSt.dia6, asigSt.dia7, asigSt.hsent,
															 asigSt.hssal, row_padre);
				RecuperoFm1();
				FmNextFld(fm, FECHA);
			}
		}

		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) == 0) {
			if (StrCmp(FmSFld(fm, CODFREC), SEMANAL) == 0) {
				GenerarDiasPTSemanal(fm);
			}
		}

		break;
	case AGRHORAS : 
		if (!FmIsNull(fm, HORENT) && !FmIsNull(fm, HORSAL))
			if (!RangoHorarioEnRangoHorario(FmTFld(fm, HORENT),FmTFld(fm, HORSAL), horent, horsal)){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2034), 
				        "El rango horario especificado (%.3T-%.3T) no coicide con el del puesto(%.3T-%.3T)",
				        FmTFld(fm, HORENT),FmTFld(fm, HORSAL), horent, horsal);
				return FM_REDO;
			}
		break;
	case SUBFPTIME :
		if (StrCmp(FmSFld(fm,  I_VIGILAD), PARTTIME) == 0)
			DoSubform(fm, NULLFP, after_asige5, fno, 0, row);

		if (!FmIFld(fm, I_CHGEFEC)) {
			if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) == 0) {
				fm5 = UseSubform(fm, SUBFPTIME, 0);
				if (!ValidPartTime(fm5) || !ValidPartTimeFm(fm5, row_padre, FmLFld(fm0, NROLEG, row_fm0)) ||
					!ValidHsPT()) {
					DisplayMSGERR();
					FmNextFld(fm, DIA, 0);
				}
			}
		}
		break;
	case DIA1:
	case DIA2:
	case DIA3:
	case DIA4:
	case DIA5:
	case DIA6:
	case DIA7:
		if (*FmSFld(fm, I_EFECT) == 'P') {
			if (DiaFranco(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0), FmDFld(fm, FECHA), FmDFld(fm, I_FECHAS), FmSFld(fm, fno)))
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2036), 
				         "El Vigilador %ld tiene franco ese día.", FmLFld(fm0, NROLEG, row_fm0)); 
		}
		if (FmChgFld(fm))
			FmSetIFld(fm, I_MODIF, TRUE);
		break;
	case FRANCERO:
		if((!StrCmp(FmSFld(fm, REGIM), REG_ESP) || !StrCmp(FmSFld(fm, REGIM), REG_ESP_2) ||
			!StrCmp(FmSFld(fm, REGIM), REG_ESP_3)) &&
			FmIsNull(fm, fno)){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2037), "Campo obligatorio");
				return FM_REDO;
		}
		break;
	case NUMFRAN:
		v_francos=GetDiasFranco(FmSFld(fm, REGIM), FALSE);
		if(v_francos > 1 && FmIsNull(fm, fno) && FmIsNull(fm, CODROL)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2038), "Campo obligatorio");
			return FM_REDO;
		}

		if (!FmIsNull(fm, fno) && FmIFld(fm, fno)>v_francos) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2071), "El numero de franco debe ser menor o igual a %d para el regimen %s", v_francos, FmSFld(fm, REGIM));
			return FM_REDO;
		}

		break;
	case MOTIVO1:
		fm6 = UseSubform(fm, MOTIVO1, 0, 0);
		if (FmIFld(fm, I_MODIF) && FmIFld(fm, I_EXISTE)) {
			FmSetDFld(fm6, FECBAJ6, FmDFld(fm, FECHA) - 1);
			FmSetDFld(fm6, I_FECASIG26, FmDFld(fm, FECHA));
			DoSubform(fm, before_asige6, after_asige6, fno, 0);

			if (FmIsNull(fm6, MOTIVO6))
				return FM_ERROR;
		}
		break;
	}
	return FM_OK;
}
static int validate(void)
{
	if (!BajaPuesto(LFld(comerc|PTOSER_CLIENTE), IFld(comerc|PTOSER_OBJET), IFld(comerc|PTOSER_TIPPTO),
					FmDFld(fm1, FECHA)))
		return TRUE;
	return FALSE;
}

static void display(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(comerc|PTOSER_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(comerc|PTOSER_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}


static fm_status after_asige5(form fm, fmfield fno, int row)
{
	char buff[600], fchstr[3];
	int i;
	form fmpadre;

	fmpadre  = FmFather(fm);

	switch (fno) {
	case AGRHS :
		FmSetIFld(fm, HSXDIA, (GetCantHoras(FmTFld(fm, HENT, row), FmTFld(fm, HSAL, row))), row);
		break;
	case DIA:
		FmSetTFld(fm, I_HORENT1, FmTFld(fmpadre, HORENT));
		FmSetTFld(fm, I_HORSAL1, FmTFld(fmpadre, HORSAL));

		if (FmIsNull(fm, HENT, row) && FmLFld(fm, CANTH) > 0 && FmTFld(fm, I_HORENT1) > FmTFld(fm, HENT, row))
			FmSetTFld(fm, HENT, FmTFld(fm, I_HORENT1), row);

		if (FmIsNull(fm, HSAL, row) && FmLFld(fm, CANTH) > 0 && FmTFld(fm, I_HORSAL1) < FmTFld(fm, HENT, row))
			FmSetTFld(fm, HSAL, FmTFld(fm, I_HORSAL1), row);

		if (FmChgFld(fm)) {
			if (FmIsNull(fmpadre, HORAPT)) {
				fchstr[0] = dia(FmDFld(fm, DIA, row));
				strcpy(&fchstr[1], NULL_STR);

				if (SeTrabEnPuesto(&fchstr[0], FmSFld(fmpadre, DIA1), FmSFld(fmpadre, DIA2), FmSFld(fmpadre, DIA3),  
						   FmSFld(fmpadre, DIA4), FmSFld(fmpadre, DIA5), FmSFld(fmpadre, DIA6), FmSFld(fmpadre, DIA7))) {
					FmSetFld (fm, I_DIASTR,  &fchstr[0], row);
					FmSetTFld(fm, I_HORENT1, FmTFld(fmpadre, HORENT));
					FmSetTFld(fm, I_HORSAL1, FmTFld(fmpadre, HORSAL));
				}
				else {
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2039), 
					         "La fecha no corresponde a ningun día de los asignados.");

					return FM_REDO;
					
				}
			}
		}
		break;
	case HENT:
	case HSAL:
		break;
	case MULTDIAS :
		if (!FmIsNull(fmpadre, CODFREC) && StrCmp(FmSFld(fmpadre, CODFREC), SEMANAL) != 0) {
			if (!ValidaFrecuencia(fm, FmDFld(fmpadre, FECHA), FmDFld(fm3, FECHAS), FmSFld(fmpadre, CODFREC),
								  FmLFld(fm0, NROLEG, row_fm0), FmIFld(fmpadre, TIPPTO), FmIFld(fmpadre, CODINT),
								  FmIFld(fmpadre, I_NROINT))) {
				DisplayMSGERR();
				FmNextFld(fm, DIA, 0);
				break;
			}
		}
		fm3 = UseSubform(fmpadre, FECHA, 0, 0);
		CargarHsPT(fm);

		if ((!FmIFld(fmpadre, I_CHGEFEC) && 
			(!ValidPartTime(fm) || !ValidPartTimeFm(fm, row_padre, FmLFld(fm0, NROLEG, row_fm0)))) || !ValidHsPT()) {
			DisplayMSGERR();
			FmNextFld(fm, DIA, row);
			break;
		}
		if (!FmIsNull(fmpadre, HORAPT))
			if (ValidarSumaMeses(fm)) {
				for (i = 0; i < MAX_MESES && i < topmes; i++)
				strcat(buff, st_meses[i].error);
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2040), "%s", buff);
			}
		break;
	}
	return FM_OK;
}

static fm_status after_asige2(form fm, fmfield fno, int row)
{

	switch (fno) {
	case FECBAJ :


//		VER ACA de agregar para cuando solo sea efectivo



		if (!StrCmp(FmSFld(fm0, EFECT, row_padre), EFECTIVO))
			if (FmDFld(fm, fno) < FmDFld(fm0, I_FECININ, row_padre)) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2041), 
				         "La fecha de baja debe estar dentro del periodo de duracion del puesto (%.3D - %.3D)\nLínea %d",
				         FmDFld(fm0, I_FECININ, row_padre), FmDFld(fm0, I_FECFINN, row_padre), row_padre);
				return FM_REDO;
				
			}

		fm1 = UseSubform(fm0, HORARIO, 0, row_padre);

		if (FmDFld(fm, fno) < FmDFld(fm1, FECHA) && !FmIsNull(fm, fno)){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2075), "La fecha de baja debe se posterior a la de asignacion(%.3D)\nLínea %d", FmDFld(fm1, FECHA), row_padre);
			return FM_REDO;
		}

		fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));

		if (!FmIsNull(fm, fno) && FmDFld(fm, fno) < fecierre ) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2077), "No se puede modificar una asignacion fecha anterior a la del cierre %.3D\nLinea %d", fecierre, row_padre); 
			return FM_REDO;
		}

		if (ExisteParteCargado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), 
			                   FmLFld(fm0, NROLEG, row_padre),FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmDFld(fm, fno) + 1, NULL_DATE)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2042), MSG_PARTE, row_padre); 
			return FM_REDO;
		}

		fecierrefil = GetFechaCierreFilial(filial);
		if (fecierrefil != NULL_DATE && FmDFld(fm, fno) < fecierrefil) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2061),"El Parte está cerrado para la Filial %s el %.3D.\nNo podrá modificarse.", filial, fecierrefil);
			return FM_ERROR;
		}
		break;
	case REEMPL:
		if (FmIFld(fm, I_ACT2) == 0) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2043), "El legajo se encuetra inactivo.\nLínea %d",
			         row_padre); 
			return FM_REDO;
		}
		break;
	}
	return FM_OK;
}

static fm_status after_asige6(form fm, fmfield fno, int row) 
{
	switch (fno) {
	case FECBAJ6 :
/*		if (FmDFld(fm, fno) < FmDFld(fm, I_FECHINI6) ||
		   (!FmIsNull(fm, I_FECHFIN6) && FmDFld(fm, fno) > FmDFld(fm, I_FECHFIN6))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2044),
			         "La fecha de baja deberia estar dentro del periodo de duracion del puesto (%D - %D)",
			         FmDFld(fm, I_FECHINI6), FmDFld(fm, I_FECHFIN6)); 
			return FM_REDO;
		}
*/

		fm1 = UseSubform(fm0, HORARIO, 0, row_padre);

		fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
		if (!FmIsNull(fm, fno) && (FmDFld(fm, fno) < fecierre ||
			ExisteParteCargado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), 
			                   FmLFld(fm0, NROLEG, row_padre), FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmDFld(fm, fno) + 1, NULL_DATE))){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2045), MSG_PARTE, row_padre); 
			return FM_REDO;
		}
		break;
	case REEMPL:
		if (FmIFld(fm, I_ACT26) == 0) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2046), "El legajo se encuetra inactivo."); 
			return FM_REDO;
		}
		break;
	}
	return FM_OK;
}

static fm_status after_asige3(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FECHAS :
		if (!FmIsNull(fm, fno) && FmDFld(fm, fno) < FmDFld(fm, I_FECDESDE)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2047), 
			        "La Fecha Hasta debe ser mayor o igual\n a la Fecha Desde %.3D", 
					FmDFld(fm, I_FECDESDE));
			return FM_REDO;
		}

		if (FmDFld(fm, fno) < FmDFld(fm, I_FFINICIO) ||
			(!FmIsNull(fm, I_FFFINAL) && FmDFld(fm, fno) > FmDFld(fm, I_FFFINAL))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2048), 
			        "La Fecha Hasta debe estar dentro del periodo de duracion del puesto! (%.3D - %.3D)",
			        FmDFld(fm, I_FFINICIO), FmDFld(fm, I_FFFINAL));
			return FM_REDO;
		}

		fm1 = UseSubform(fm0, HORARIO, 0, row_padre);

		fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
		if (!FmIsNull(fm, fno) && (FmDFld(fm, fno) <= fecierre ||
			ExisteParteCargado(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
			                   FmLFld(fm0, NROLEG, row_padre), FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmDFld(fm, fno) + 1, NULL_DATE))) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2049), MSG_PARTE); 
			return FM_REDO;
		}

		fecierrefil = GetFechaCierreFilial(filial);
		if (fecierrefil != NULL_DATE && FmDFld(fm, fno) <= fecierrefil) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2061),"El Parte está cerrado para la Filial %s el %.3D.\nNo podrá modificarse.", filial, fecierrefil);
			return FM_ERROR;
		}

		break;
	}
	return FM_OK;
}

// Valida superposicion de horarios para vigilators que no son Part-Time.
// Debe Controlar tanto contra ASIG como ASIGH, por si se hizo una asignación adelantada.
static bool ValidPuestoOcup(long legajo, DATE p_fecasig, TIME p_horent, TIME p_horsal, char *dia1, char *dia2,
							char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, int fila,
							long cliefec, int objefec, int ptoefec, int codintefec, char * efect)
{
	int i; 
	form fm11, fm21 = NULL_SHORT, fm61= NULL_SHORT, fm31;

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), legajo, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), legajo, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {

		if (LFld(AlFld(asig, operac|ASIG_CLIENTE))  == cliefec    &&
			IFld(AlFld(asig, operac|ASIG_OBJETIVO)) == objefec    &&
			IFld(AlFld(asig, operac|ASIG_PTOSER))   == ptoefec    &&
			IFld(AlFld(asig, operac|ASIG_PUESTO))   == codintefec &&
			strcmp(efect, EFECTIVO)                 == 0) {
			continue;
		}

		CargarEstructuraAsig(FmIFld(fm0, EMP), LFld(AlFld(asig, operac|ASIG_CLIENTE)),
							 IFld(AlFld(asig, operac|ASIG_OBJETIVO)), IFld(AlFld(asig, operac|ASIG_PTOSER)),
							 IFld(AlFld(asig, operac|ASIG_PUESTO)),   IFld(AlFld(asig, operac|ASIG_NROINT)),
							 LFld(AlFld(asig, operac|ASIG_NROLEG)),   SFld(AlFld(asig, operac|ASIG_VIGIL)),
							 SFld(AlFld(asig, operac|ASIG_EFECT)),    DFld(AlFld(asig, operac|ASIG_FECASIG)),
							 TFld(AlFld(asig, operac|ASIG_HSENT)),    TFld(AlFld(asig, operac|ASIG_HSSAL)),
							 SFld(AlFld(asig, operac|ASIG_DIA1)),     SFld(AlFld(asig, operac|ASIG_DIA2)),
							 SFld(AlFld(asig, operac|ASIG_DIA3)),     SFld(AlFld(asig, operac|ASIG_DIA4)),
							 SFld(AlFld(asig, operac|ASIG_DIA5)),     SFld(AlFld(asig, operac|ASIG_DIA6)),
							 SFld(AlFld(asig, operac|ASIG_DIA7)),     LFld(AlFld(asig, operac|ASIG_REEMPL)),
							 DFld(AlFld(asig, operac|ASIG_FFRANCO)),  IFld(AlFld(asig, operac|ASIG_NUMFRAN)),
							 IFld(AlFld(asig, operac|ASIG_FRANCERO)), SFld(AlFld(asig, operac|ASIG_REGIM)),
							 DFld(AlFld(asig, operac|ASIG_FECHAS)),   DFld(AlFld(asig, operac|ASIG_FECBAJ)));
		if (!ValidoUnAsig(legajo, p_fecasig, p_horent, p_horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, FALSE)) {
			return FALSE;
		}
	}
	// ASIGH
	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), legajo, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), legajo, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		CargarEstructuraAsig(FmIFld(fm0, EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO),
							 IFld(operac|ASIGH_PTOSER), IFld(operac|ASIGH_PUESTO), IFld(operac|ASIGH_NROINT),
							 LFld(operac|ASIGH_NROLEG), SFld(operac|ASIGH_VIGIL),  SFld(operac|ASIGH_EFECT),
							 DFld(operac|ASIGH_FECALT), TFld(operac|ASIGH_HSENT),  TFld(operac|ASIGH_HSSAL),
							 SFld(operac|ASIGH_DIA1),   SFld(operac|ASIGH_DIA2),   SFld(operac|ASIGH_DIA3),
							 SFld(operac|ASIGH_DIA4),   SFld(operac|ASIGH_DIA5),   SFld(operac|ASIGH_DIA6),
							 SFld(operac|ASIGH_DIA7),   LFld(operac|ASIGH_REEMPL), DFld(operac|ASIGH_FFRANCO),
							 IFld(operac|ASIGH_NUMFRAN), IFld(operac|ASIGH_FRANCERO), SFld(operac|ASIGH_REGIM),
							 DFld(operac|ASIGH_FECHAS), DFld(operac|ASIGH_FECBAJ));
		if (!ValidoUnAsig(legajo, p_fecasig, p_horent, p_horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, TRUE)) {
			return FALSE;
		}
	}

	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		if (i == fila)
			continue;
		if (FmLFld(fm0, NROLEG, i) != legajo)
			continue;
		fm11 = UseSubform(fm0, HORARIO, 0, i);
		fm31 = UseSubform(fm1, FECHA,   0, i);

		if (!FmIFld(fm0, ASIGNA, i)) 
			fm21 = UseSubform(fm0, ASIGNA, 0, i);
		else
			fm61 = UseSubform(fm11, MOTIVO1, 0, 0);

		CargarEstructuraAsig(FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT),  FmIFld(fm0, OBJET), FmIFld(fm11, TIPPTO),
							 FmIFld(fm11, CODINT),    FmIFld(fm11, I_NROINT), FmLFld(fm0, NROLEG, i),
							 FmSFld(fm11, I_VIGILAD), FmSFld(fm11, I_EFECT),  FmDFld(fm11, FECHA),
							 FmTFld(fm11, HORENT),    FmTFld(fm11, HORSAL),   FmSFld(fm11, DIA1),
							 FmSFld(fm11, DIA2),      FmSFld(fm11, DIA3),     FmSFld(fm11, DIA4),
							 FmSFld(fm11, DIA5),      FmSFld(fm11, DIA6),     FmSFld(fm11, DIA7),
							 FmIFld(fm0, ASIGNA, i) == 0 ? FmLFld(fm21, REEMPL) : FmLFld(fm61, REEMPL6),
							 FmDFld(fm11, FECFRA),    FmIFld(fm11, NUMFRAN),  FmIFld(fm11, FRANCERO),
							 FmSFld(fm11, REGIM),     FmDFld(fm31, FECHAS),  NULL_DATE);
		/* Re-Seteamos el row del form principal porque se perdia el row. (Viene a ser como un Close) */
		fm11 = UseSubform(fm0, HORARIO, 0, fila);

		if (!ValidoUnAsigFm(legajo, p_fecasig, p_horent, p_horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, FALSE)) {
			return FALSE;
		}
	}
	return TRUE;
}

static bool ValidoUnAsig(long legajo, DATE p_fecasig, TIME p_horent, TIME p_horsal, char *dia1, char *dia2,
						 char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, bool asigh) 
{
	bool diastrab;

	// Si es un reten, no se busca superposición horaria
	// con el objetivo prosegur.
	if (ExisteCliObjEnGrp(GRPRETPLANTA, asigSt.cliente, asigSt.objetivo) && !StrCmp(asigSt.vigil, RETEN))
		return TRUE;

	// si es el mismo cli obj puede ser que este consultando una asignación ya existente
	// en la tabla asig, asi que no valido superpos.
	if (!asigh)
		if (FmLFld(fm0, CLIEOT) == asigSt.cliente && FmLFld(fm0, OBJET) == asigSt.objetivo) 
			return TRUE;

	if (asigh) {
		// si la asignación cargada en asigh con fecha de baja anterior
		// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
		if (asigSt.fecbaj < p_fecasig) {
			return TRUE;
		}
	}
	// si la asignación cargada en asig era provisoria con fecha de fin anterior
	// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
	if (asigSt.fechas != NULL_DATE && asigSt.fechas < p_fecasig) {
		return TRUE;
	}
	// si la asignación nueva es provisoria con fecha de fin anterior
	// a la fecha de asignación de la asignacion cargada en asig, no hay superposicion:
	if (fechas != NULL_DATE && fechas < asigSt.fecasig ) {
		return TRUE;
	}
	// si los dias no se superponen no hace falta seguir validando!
	diastrab = DiasTrabajados(dia1, dia2, dia3, dia4, dia5, dia6, dia7, asigSt.dia1, asigSt.dia2,
							  asigSt.dia3, asigSt.dia4, asigSt.dia5, asigSt.dia6, asigSt.dia7);
	if (!diastrab) 
		return TRUE;

	// si los dias se superponen pero las horas de trabajo no se superponen, todo bien!
	if (diastrab && !Superposicion(p_horent, p_horsal, asigSt.hsent, asigSt.hssal, FALSE)) {
		return TRUE;
	}
	// se detecto superposicion => devuelve FALSE:
	return FALSE;
}

static bool ValidoUnAsigFm(long legajo, DATE p_fecasig, TIME p_horent, TIME p_horsal, char *dia1, char *dia2,
						 char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, bool asigh)
{
	bool diastrab;

	if (asigh) {
		// si la asignación cargada en asigh con fecha de baja anterior
		// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
		if (asigSt.fecbaj < p_fecasig)
			return TRUE;
	}
	// si la asignación cargada en asig era provisoria con fecha de fin anterior
	// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
	if (asigSt.fechas != NULL_DATE && asigSt.fechas < p_fecasig) 
		return TRUE;

	// si la asignación nueva es provisoria con fecha de fin anterior
	// a la fecha de asignación de la asignacion cargada en asig, no hay superposicion:
	if (fechas != NULL_DATE && fechas < asigSt.fecasig ) 
		return TRUE;

	// si los dias no se superponen no hace falta seguir validando!
	diastrab = DiasTrabajados(dia1, dia2, dia3, dia4, dia5, dia6, dia7, asigSt.dia1, asigSt.dia2,
							  asigSt.dia3, asigSt.dia4, asigSt.dia5, asigSt.dia6, asigSt.dia7);
	if (!diastrab)
		return TRUE;

	// si los dias se superponen pero las horas de trabajo no se superponen, todo bien!
	if (diastrab && !Superposicion(p_horent, p_horsal, asigSt.hsent, asigSt.hssal, FALSE))
		return TRUE;

	// se detecto superposicion => devuelve FALSE:
	return FALSE;
}

static void GraboPuesto()
{
	SetLFld(operac|PUESTOS_CLIENTE, FmLFld(fm0, CLIEOT));
	SetIFld(operac|PUESTOS_OBJET,   FmIFld(fm0, OBJET));
	SetIFld(operac|PUESTOS_TIPPTO,  NULL_SHORT);
	SetIFld(operac|PUESTOS_CODINT,  NULL_SHORT);
	while (GetRecord(operac|PUESTOSbyCLIENTE, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR) {
		// Si el valor es mayor a 100 lo pongo en 99 porque es un campo de tamanio 4
		if (pue[IFld(operac|PUESTOS_TIPPTO)][IFld(operac|PUESTOS_CODINT)] >= 10000 )
			SetIFld(operac|PUESTOS_VIGI, 9900);
		else
			SetIFld(operac|PUESTOS_VIGI, pue[IFld(operac|PUESTOS_TIPPTO)][IFld(operac|PUESTOS_CODINT)]);

					
		PutRecord(operac|PUESTOS);
	}
}

static void CargarDiasPTime(long nroleg, int p_ptoser, int puesto, int nroint, form fm)
{

	int  i = 0;
	char fchstr[3];

	fm5 = UseSubform(fm, SUBFPTIME, 0, 0);

	SetCursorFrom(c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, p_ptoser,
							puesto, nroint, Today());
	SetCursorTo  (c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, p_ptoser,
							puesto, nroint, MAX_DATE);
	while (i < FmFldLen(fm5, MULTDIAS) && FetchCursor(c_diaspt) != ERROR) {
		FmSetDFld(fm5, DIA,    DFld(operac|DIASPTIME_DIA),  i);
		FmSetTFld(fm5, HENT,   TFld(operac|DIASPTIME_HENT), i);
		FmSetTFld(fm5, HSAL,   TFld(operac|DIASPTIME_HSAL), i);
		FmSetIFld(fm5, HSXDIA, GetCantHoras(FmTFld(fm5, HENT, i), FmTFld(fm5, HSAL, i)), i);

		fchstr[0] = dia(FmDFld(fm5, DIA, i));
		strcpy(&fchstr[1], NULL_STR);
		FmSetFld(fm5, I_DIASTR, &fchstr[0], i);
		i++;
	}
	FmSetIFld(fm5, I_CANTMULT, i);
}

private void CargarOtrasAsignaciones(form p_fm4, long nroleg)
{
	int i = 0;
	SetCursorFrom(c_aasig, FmIFld(fm0, EMP), nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_aasig, FmIFld(fm0, EMP), nroleg, MAX_LONG, MAX_SHORT);
	while (i < FmFldLen(p_fm4, MULTI) && FetchCursor(c_aasig) != ERROR) {

		SetKey(bill|CLIENTEbyCLIENTE, LFld(AlFld(aasig, operac|ASIG_CLIENTE)));
		if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) 
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2050), "El Cliente: %ld no existe!.", 
			         LFld(AlFld(aasig, operac|ASIG_CLIENTE)));

		LeerObjetivo(LFld(AlFld(aasig, operac|ASIG_CLIENTE)), IFld(AlFld(aasig, operac|ASIG_OBJETIVO)));

		FmSetLFld(p_fm4, CLI,        LFld(AlFld(aasig, operac|ASIG_CLIENTE)), i);
		FmSetFld (p_fm4, DCLI,       SFld(bill|CLIENTE_RAZSOC),               i);
		FmSetIFld(p_fm4, OBJ,        IFld(AlFld(aasig, operac|ASIG_OBJETIVO)),i);
		FmSetFld (p_fm4, DOBJ,       SFld(comerc|OBJETIVO_DESCRIP),           i);
		FmSetDFld(p_fm4, FASIG,      DFld(AlFld(aasig, operac|ASIG_FECASIG)), i);
		FmSetDFld(p_fm4, I_FECHASTA, DFld(AlFld(aasig, operac|ASIG_FECHAS)),  i);
		FmSetFld (p_fm4, I_VIGIL4,   SFld(AlFld(aasig, operac|ASIG_EFECT)),   i);
		FmSetIFld(p_fm4, I_TIPPTO,   IFld(AlFld(aasig, operac|ASIG_PTOSER)),  i);
		FmSetIFld(p_fm4, I_CODINT,   IFld(AlFld(aasig, operac|ASIG_PUESTO)),  i);
		FmSetIFld(p_fm4, I_NUMINT,   IFld(AlFld(aasig, operac|ASIG_NROINT)),  i);
		FmSetFld (p_fm4, ADIA1,      SFld(AlFld(aasig, operac|ASIG_DIA1)),    i);
		FmSetFld (p_fm4, ADIA2,      SFld(AlFld(aasig, operac|ASIG_DIA2)),    i);
		FmSetFld (p_fm4, ADIA3,      SFld(AlFld(aasig, operac|ASIG_DIA3)),    i);
		FmSetFld (p_fm4, ADIA4,      SFld(AlFld(aasig, operac|ASIG_DIA4)),    i);
		FmSetFld (p_fm4, ADIA5,      SFld(AlFld(aasig, operac|ASIG_DIA5)),    i);
		FmSetFld (p_fm4, ADIA6,      SFld(AlFld(aasig, operac|ASIG_DIA6)),    i);
		FmSetFld (p_fm4, ADIA7,      SFld(AlFld(aasig, operac|ASIG_DIA7)),    i);
		FmSetTFld(p_fm4, HORAENT,    TFld(AlFld(aasig, operac|ASIG_HSENT)),   i);
		FmSetTFld(p_fm4, HORASAL,    TFld(AlFld(aasig, operac|ASIG_HSSAL)),   i);
		i++;
	}
}

static void LeerObjetivo(long cliente, int objetivo)
{
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2051), "El Objetivo: %d del Cliente: %ld no existe!.",
		         objetivo, cliente);
}

static bool ValInfo()
{
	int i;
	
	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		fm1 = UseSubform(fm0, HORARIO, 0, i);
		fm2 = UseSubform(fm0, ASIGNA, 0,  i);
		fm3 = UseSubform(fm1, FECHA, 0);

 		// Verifico cuando es desasignacion si la fecha de baja es nula
		if( FmDFld(fm2, FECBAJ)==NULL_DATE && !FmIFld(fm0, ASIGNA, i) )	{
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2052), 
			         "El Vigilador %ld Fila %d No se puede Desasignar. \n La fecha de baja es Nula.",
			         FmLFld(fm0, NROLEG, i), i);
			return TRUE;	
		}

		if (FmIsNull(fm1, TIPPTO)) {
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2053), 
			         "Falta cargar información de la asignación para el Vigilador: %ld %s\nLínea %d",
			         FmLFld(fm0, NROLEG, i), FmSFld(fm0, NOMBRE, i), i);
			return TRUE;
		}
		else {
			// Si es vigilador PartTime, y no es cliente prosegur entonces valido superposicion
			// de horarios.
			if (StrCmp(FmSFld(fm0, VIGILAD, i), PARTTIME) == 0) {
				fm5 = UseSubform(fm1, SUBFPTIME, 0);
				if (!ExisteCliObjEnGrp(GRPRETPLANTA, FmLFld(fm0, CLIEOT), NULL_SHORT)) {
					if ((!FmIFld(fm1, I_CHGEFEC) &&
						(!ValidPartTime(fm5) || !ValidPartTimeFm(fm5, i, FmLFld(fm0, NROLEG, i)))) ||
						!ValidHsPT()) {
						actualizar = FALSE;
						DisplayMSGERR();
						return TRUE;
					}
					if (FmIsNull(fm1, HORAPT) || FmIFld(fm1, HORAPT) == 0) {
						if (!ValidaFrecuencia(fm5, FmDFld(fm1, FECHA), FmDFld(fm3, FECHAS),
											  FmSFld(fm1, CODFREC), FmLFld(fm0, NROLEG, i),
											  FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), FmIFld(fm1, I_NROINT))) {
							DisplayMSGERR();
							actualizar = FALSE;
							return TRUE;
						}
					}
				}
			}
			else {
				// pregunto que si es reten no estemos buscando superposiciones con el objetivo prosegur:
				if (!FmIFld(fm1, I_CHGEFEC)) {
					if (!(!StrCmp(FmSFld(fm0, VIGILAD, i), RETEN) && 
						ExisteCliObjEnGrp(GRPRETPLANTA, FmLFld(fm0, CLIEOT), FmLFld(fm0, OBJET))) &&
						StrCmp(TipoDia(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm1, TIPPTO),
							   FmIFld(fm1, CODINT)), "F") &&
						!ValidPuestoOcup(FmLFld(fm0, NROLEG, i), FmDFld(fm1, FECHA), FmTFld(fm1, HORENT),
										FmTFld(fm1, HORSAL), FmSFld(fm1, DIA1), FmSFld(fm1, DIA2),
										FmSFld(fm1, DIA3), FmSFld(fm1, DIA4), FmSFld(fm1, DIA5),
										FmSFld(fm1, DIA6), FmSFld(fm1, DIA7),
										(!StrCmp(FmSFld(fm0, EFECT, i), PROVISORIO)) ? FmDFld(fm3, FECHAS) :
										NULL_DATE, i, FmLFld(fm0, I_CLIEFEC, i), FmIFld(fm0, I_OBJEFEC, i),
										FmIFld(fm0, I_PTOEFEC, i), FmIFld(fm0, I_CODINTEFEC, i),
										FmSFld(fm0, EFECT, i))) {
						actualizar = FALSE;
						// como queda el último registro corriente, tomo esos datos.

						WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2058),
						         MSG_SUPERPOS, FmSFld(fm0, NOMBRE, i), asigSt.cliente, asigSt.objetivo,
								 asigSt.fecasig, asigSt.dia1, asigSt.dia2, asigSt.dia3, asigSt.dia4,
								 asigSt.dia5, asigSt.dia6, asigSt.dia7,	asigSt.hsent, asigSt.hssal, i);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

static bool HayVacante(long nroleg, int tippto, int p_codint, DATE fdesde, DATE fhasta, DATE *fechocup)
{
	form fm;
	int  i;
	bool hayvac = TRUE;
	DATE fecha;

	for (fecha = fdesde ; hayvac && fecha <= fhasta; fecha++) {
		hayvac = FALSE;
		// busco para todos los vigiladores asignados si alguno tiene vacaciones cada
		// día del periodo elegido
		for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
			fm = UseSubform(fm0, HORARIO, 0, i);
			if (FmIFld(fm, TIPPTO) == tippto && FmIFld(fm, CODINT) == p_codint && FmLFld(fm0, NROLEG, row_fm0) != nroleg) {
				if (Vacaciones(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha) ||
					TieneLic  (FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha) ||
					Falto     (FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha)) {
					hayvac = TRUE;
				}
			}
		}
	}
	(*fechocup) = fecha;
	RestauraSubForm( fm0, HORARIO, 0, row_padre);
	return hayvac;
}

static bool ValidarSumaMeses(form fm)
{
	int    i, j;
	double canth;
	bool   error;

	topmes = 0;
	for (i = 0; i < MAX_MESES; i++) {
		st_meses[i].mes = 0;
		st_meses[i].horas = 0;
		strcpy(st_meses[i].error, NULL_STR);
	}
	for (i = 0; i < FmFldLen(fm, MULTDIAS) && !FmIsNull(fm, DIA, i); i++) {
		for (j = 0; j < 5 && j < topmes; j++) {
			if (st_meses[j].mes   == Month(FmDFld(fm, DIA, i))) {
				st_meses[j].horas += FmLFld(fm, HSXDIA, i);
				break;
			}
		}
		if (j == 5){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2055),"Estructura interna saturada");
			Stop (0);
		}
		
		if (j == topmes) {
			st_meses[j].mes = Month(FmDFld(fm, DIA, i));
			st_meses[j].horas = FmLFld(fm, HSXDIA, i);
			topmes++;
		}
	}
	canth = FmLFld(fm, CANTH);
	error = FALSE;
	for (i = 0; i < 5 && i < topmes ; i++) {
		if (st_meses[i].horas != canth) {
			sprintf(st_meses[i].error, "\nMes %d con horas %.2f distinta a asignada %.2f\n",
					st_meses[i].mes, st_meses[i].horas/100, canth/100);
			strcat(st_meses[i].error, NULL_STR);
			error = TRUE;
		}
	}
	return error;
}

private void CargarHsPT(form fmdias)
{
	int i;

	for (i = 0; i < 13; i++)
		hsmes[i] = 0;

	for (i = 0; i < FmFldLen(fmdias,  MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++)
		hsmes[Month(FmDFld(fmdias, DIA, i))] += FmIFld(fmdias, HSXDIA, i);
}

private bool ValidHsPT()
{
	int  i;
	bool error = FALSE;
	char buff[200];

	// Inicilizo la estructura de errores para desplegar despues.
	topmerr = 0;

	for (i = 1; i < 13; i++) {
		if (hsmes[i] > HSPT) {
			sprintf(buff, "Se superaron las 133,34 horas permitidas para el mes de %s\n", MonthName(i));
			if (!PutMSGERR(buff))
				return FALSE;
			error = TRUE;
		}
	}
	if (error)
		return FALSE;
	return TRUE;
}

private bool ValidPartTimeFm(form fmdias, int p_row_padre, long legajo)
{
	int  i, j, t, topdia = 0;
	bool error = FALSE;	
	form fm11, fm15;
	char buff[200];

	for (i = 0; i < FmFldLen(fmdias,  MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {
		diaspt[i].dia   = NULL_DATE;
		diaspt[i].hsent = NULL_TIME;
		diaspt[i].hssal = NULL_TIME;
	}
	for (i = 0; i < FmFldLen(fmdias,  MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {

		diaspt[i].dia   = FmDFld(fmdias, DIA,  i);
		diaspt[i].hsent = FmTFld(fmdias, HENT, i);
		diaspt[i].hssal = FmTFld(fmdias, HSAL, i);
		topdia++;
	}
	for (i = 0; i < topdia; i++) {
		for (j = 0; j < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, j); j++) {
			if (j == p_row_padre)
				continue;
			if (FmLFld(fm0, NROLEG, j) != legajo)
				continue;
			fm11 = UseSubform(fm0, HORARIO, 0, j);
			fm15 = UseSubform(fm11, SUBFPTIME, 0, j);
			for (t = 0; t < FmFldLen(fm15, MULTDIAS) && !FmIsNull(fm15, DIA, t); t++) {
				if (diaspt[i].dia != FmDFld(fm15, DIA, t))
					continue;
				if (Superposicion(diaspt[i].hsent, diaspt[i].hssal, FmTFld(fm15,  HENT, t),
								  FmTFld(fm15,  HSAL, t), FALSE)) {
					sprintf(buff, "Día %.1D %.1T %.1T% asignado en el mismo cliente/objetivo.\n",
							  FmDFld(fm15, DIA, t), FmTFld(fm15, HENT, t), FmTFld(fm15, HSAL, t));
					if (!PutMSGERR(buff))
						return FALSE;
					error = TRUE;
				}
			}
		}
	}
	RestauraSubForm(fm0, HORARIO,   0, p_row_padre);
	RestauraSubForm(fm1, SUBFPTIME, 0, p_row_padre);
	if (error)
		return FALSE;
	return TRUE;
}

private bool ValidPartTime(form fmdias)
{
	int  i;
	bool error = FALSE;
	char buff[200];
	form fmpadre;

	fmpadre = FmFather(fmdias);
	// Inicilizo la estructura de errores para desplegar despues.
	topmerr = 0;

	for (i = 0; i < FmFldLen(fmdias,  MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {
		SetKey(operac|DIASPTIMEbyDIA, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0), FmDFld(fmdias, DIA, i),
								NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		while (GetRecord(operac|DIASPTIMEbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			// si es el mismo cliente objetivo es que esta modificandolo o consultandolo.

			if ((ExisteCliObjEnGrp(GRPRETPLANTA, LFld(operac|DIASPTIME_CLIENTE), IFld(operac|DIASPTIME_OBJETIVO))) ||
				(IFld(operac|DIASPTIME_TIPPTO)  == FmIFld(fmpadre, TIPPTO) && 
				 IFld(operac|DIASPTIME_PUESTO)  == FmIFld(fmpadre, CODINT) &&
				 IFld(operac|DIASPTIME_NROINT)  == FmIFld(fmpadre, I_NROINT))) {
				continue;
			}
			if (Superposicion(FmTFld(fmdias, HENT, i), FmTFld(fmdias, HSAL, i), TFld(operac|DIASPTIME_HENT),
							  TFld(operac|DIASPTIME_HSAL), FALSE)) {
				sprintf(buff, "Día %.1D %.1T %.1T% asignado en cliente %ld %d desde %.1T% a %.1T\n",
						  FmDFld(fmdias, DIA, i), FmTFld(fmdias, HENT, i), FmTFld(fmdias, HSAL, i),
						  LFld(operac|DIASPTIME_CLIENTE), IFld(operac|DIASPTIME_OBJETIVO),
						  TFld(operac|DIASPTIME_HENT),    TFld(operac|DIASPTIME_HSAL));
				if (!PutMSGERR(buff))
					return FALSE;
				error = TRUE;
			}
		}
		SetKey(operac|DIASPTIMEHbyDIA, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row_fm0), FmDFld(fmdias, DIA, i),
								NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		while (GetRecord(operac|DIASPTIMEHbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			// si es el mismo cliente objetivo es que esta modificandolo o consultandolo.
			if ((ExisteCliObjEnGrp(GRPRETPLANTA, LFld(operac|DIASPTIMEH_CLIENTE), LFld(operac|DIASPTIMEH_OBJETIVO))) &&
				FmIFld(fmpadre, I_EXISTE)) {
					continue;
			}
			if (Superposicion(FmTFld(fmdias, HENT, i), FmTFld(fmdias, HSAL, i), TFld(operac|DIASPTIMEH_HENT),
							  TFld(operac|DIASPTIMEH_HSAL), FALSE)) {
				sprintf(buff, "Día %.1D %.1T %.1T% asignado en cliente %ld %d desde %.1T% a %.1T\n",
						  FmDFld(fmdias, DIA, i), FmTFld(fmdias, HENT, i), FmTFld(fmdias, HSAL, i),
						  LFld(operac|DIASPTIMEH_CLIENTE), IFld(operac|DIASPTIMEH_OBJETIVO),
						  TFld(operac|DIASPTIMEH_HENT), TFld(operac|DIASPTIMEH_HSAL));
				if (!PutMSGERR(buff))
					return FALSE;
				error = TRUE;
			}
		}
	}
	if (error)
		return FALSE;
	return TRUE;
}


private bool PutMSGERR(char buff[])
{
	if (topmerr == MAX_ERRORS)
		return FALSE;

	strcpy(MsgErr[topmerr].error,  buff);
	strcat(MsgErr[topmerr].error, NULL_STR);
	topmerr++;
	return TRUE;
}

private void DisplayMSGERR()
{
	int i;
	char buff[1500];

	strcpy(buff, NULL_STR);
	for (i = 0; i < topmerr; i++) {
		strcat(buff, MsgErr[i].error);
	}
	WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2059), "%s", buff);
}

private void GenerarDiasPTSemanal(form fm)
{
	DATE fecha, fchhas;
	int  i=0, j;
	char fchstr[3];

	fm3 = UseSubform(fm, FECHA, 0, 0); 
	fm5 = UseSubform(fm, SUBFPTIME, 0, 0);

	//Cambio la fecha de asig para Efect. con lo cual los dias son a partir de la nueva fecha.
	if (FmIFld(fm, I_MODIF) && FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES)) {
		fecha = FmDFld(fm, FECHA);
		i = 0;
	}
	else {
		fecha = i == 0 || i == NULL_SHORT ? FmDFld(fm, FECHA) : FmDFld(fm5, DIA, i);
		i = FmIFld(fm5, I_CANTMULT) == NULL_SHORT ? 0 : FmIFld(fm5, I_CANTMULT);
	}
	fchhas = FmDFld(fm3, FECHAS) == NULL_DATE ? fecha + 90 : FmDFld(fm3, FECHAS);

	for (; fecha < fchhas && i < FmFldLen(fm5,  MULTDIAS); fecha++) {
		fchstr[0] = dia(fecha);
		strcpy(&fchstr[1], NULL_STR);

		if (SeTrabEnPuesto(&fchstr[0], FmSFld(fm, DIA1), FmSFld(fm, DIA2), FmSFld(fm, DIA3), FmSFld(fm, DIA4),
									   FmSFld(fm, DIA5), FmSFld(fm, DIA6), FmSFld(fm, DIA7))) {
			FmSetDFld(fm5, DIA,    fecha,                i);
			FmSetTFld(fm5, HENT,   FmTFld(fm, HORENT),   i);
			FmSetTFld(fm5, HSAL,   FmTFld(fm, HORSAL),   i);
			FmSetIFld(fm5, HSXDIA, (GetCantHoras(FmTFld(fm5, HENT, i), FmTFld(fm5, HSAL, i))), i);
			FmSetFld (fm5, I_DIASTR, &fchstr[0], i++);
		}
		for (j = i; j < FmFldLen(fm5, MULTDIAS) && !FmIsNull(fm5, DIA, j); j++)
			FmClearFlds(fm5, DIA, HSXDIA, j);
	}
}

private bool ValidaFrecuencia(form fmdias, DATE fecha, DATE fechas, char * codfrec, long nroleg, int p_ptoser,
							  int puesto, int nroint)
{
	int  j, i, subind;
	form fmpadre;
	char error[100];
	DATE fchact, fchant;
	bool encontro = FALSE;

	c_diaspt = CreateCursor(operac|DIASPTIMEbyEMP,   IO_NOT_LOCK);
	fmpadre  = FmFather(fmdias);

	// Inicilizo la estructura de errores para desplegar despues.
	topmerr = 0;

	// Inicializo con nulos para que quede despues bien ordenado
	for (i = 0 ; i < 7 ; i++)
		for (j = 0 ; j < MAX_FECHAS; j++) {
			dia[i].fecha[j] = NULL_DATE;
			dia[i].ult = 0;
		}

	//Cargo los dias que estas en la base.
	SetCursorFrom(c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, p_ptoser,
							puesto, nroint, fecha);       //MIN_DATE);
	SetCursorTo  (c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, p_ptoser,
							puesto, nroint, MAX_DATE);
	while (FetchCursor(c_diaspt) != ERROR) {
		subind = subdia(dia(DFld(operac|DIASPTIME_DIA)));
		dia[subind].fecha[dia[subind].ult++] = DFld(operac|DIASPTIME_DIA);
	}
	//Cargo los dias del form que no estan en la base.
	for (i = 0; i < FmFldLen(fmdias, MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {
		subind = subdia(*FmSFld(fmdias, I_DIASTR, i));
		encontro = FALSE;
		for (j = 0; j < dia[subind].ult; j++) {
			if (dia[subind].fecha[j] == FmDFld(fmdias, DIA, i)) {
				encontro = TRUE;
				break;
			}
		}
		if (!encontro) {
			dia[subind].fecha[dia[subind].ult++] = FmDFld(fmdias, DIA, i);
		}
	}
	// Ordeno por si acaso lo cargaron desordenado.
	for (i = 0 ; i < 7 ; i++) {
		qsort((char *)dia[i].fecha, MAX_FECHAS, sizeof(DATE), (IFPVCPVCP) compdia);
	}

	
	// Valido Frecuencia segun sea MENSUAL o QUINCENAL
	for (i = 0 ; i < 7 ; i++) {
		// Valido para el [0] que :
		//				1) Tenga si es uno de los dias asignados.
		// 				2) NO tenga si no es uno de los dias asignados.
		//				CONTINUO.
		// ESTA VALIDACION VALE PARA MENSUAL Y QUINCENAL.
		if (SeTrabEnPuesto(diasub(i), FmSFld(fmpadre, DIA1), FmSFld(fmpadre, DIA2), 
						   FmSFld(fmpadre, DIA3), FmSFld(fmpadre, DIA4), FmSFld(fmpadre, DIA5), 
						   FmSFld(fmpadre, DIA6), FmSFld(fmpadre, DIA7))) {
			if (dia[i].fecha[0] == NULL_DATE) {
				sprintf(error, "El día %s no tiene asignadas fechas para el período %.1D - %.1D\n",
							diasub(i), fecha, fechas);
				if (!PutMSGERR(error))
					return FALSE;
				continue;
			}
		}
		if (StrCmp(codfrec, QUINCENAL) == 0) {
			if (dia[i].fecha[0] - (7*2) >= fecha) {
				sprintf(error, "Quincenas anteriores detectadas sin asignar, fecha %.1D\n",
								dia[i].fecha[0] - (7*2));
				if (!PutMSGERR(error))
					return FALSE;
			}
		}
//		else {
		if (strcmp(codfrec, MENSUAL) == 0) {
			// Faltan meses desde la fecha de asignación a la primer fecha asignada
			if (MonthDiff(dia[i].fecha[0], fecha) != 0) {
				if (!PutErrorMesesFaltantes(fecha, AddMonth(dia[i].fecha[0],-1), i))
					return FALSE;
			}
		}
		for (j = 1; j < dia[i].ult; j++) {
			if (dia[i].fecha[j] == NULL_DATE)
				break;
			fchact = dia[i].fecha[j];
			fchant = dia[i].fecha[j-1];
			if (StrCmp(codfrec, MENSUAL) == 0) {
				// Hay meses en el medio de dos fechas consecutivas que no tienen asignacion
				if (MonthDiff(fchact, fchant) > 1) {
					if (!PutErrorMesesFaltantes(AddMonth(fchant,1), AddMonth(fchact, -1), i))
						return FALSE;
				}
				if (Month(fchact) == Month(fchant)) {
					// Un mes tiene mas de una asignación.
					sprintf(error, "Mas de una asignación para el dia %s en el mes %d\n",
								diasub(i), Month(fchant));
					if (!PutMSGERR(error))
						return FALSE;
				}
			}
			if (StrCmp(codfrec, QUINCENAL) == 0) {
				// NO hay 14 dias (2 semanas) entre dos de las fechas asignadas consecutivas.
				if (fchact - (7*2) != fchant) {
					if (fchact -(7*2) > fecha && fchact - (7*2)  < fechas)
						sprintf(error, "Quincena Erroneamente asignada %.1D - %.1D, debería ser %.1D - %.1D\n",
								fchant, fchact, fchact - (7*2), fchact);
					else
						sprintf(error, "Quincena Erroneamente asignada %.1D - %.1D, debería ser %.1D - %.1D\n",
								fchant, fchact, fchant, fchant + (7*2));
					if (!PutMSGERR(error))
						return FALSE;
				}
			}
		}
		if (StrCmp(codfrec, QUINCENAL) == 0) {
			if (dia[i].ult != 0)
				// Quedan quincenas posteriores a la ultima fecha asignada, dentro del periodo sin asignar.
				if (dia[i].fecha[j-1] + (7*2) <  fechas) {
					sprintf(error, "Quincenas posteriores detectadas sin asignar, fecha %.1D\n",
									dia[i].fecha[j-1] + (7*2));
					if (!PutMSGERR(error))
						return FALSE;
				}
		}

		if (strcmp(codfrec, MENSUAL) == 0) {
			if (dia[i].ult != 0)
				if (MonthDiff(fechas, dia[i].fecha[j-1]) != 0) {
					// Quedan meses desde el ultimo asignado a la fecha hasta del periodo que
					// no tienen fechas asignadas.
					if (!PutErrorMesesFaltantes(AddMonth(dia[i].fecha[j-1], 1), fechas, i))
						return FALSE;
				}
		}
	}
	if (topmerr != 0)
		return FALSE;
	return TRUE;
}

static int compdia(DATE *a, DATE *b)
{
	// para que los nulos queden al final
	if (*a == NULL_DATE)
		return 1;
	if (*b == NULL_DATE)
		return -1;
	return ( *a < *b ? -1 : (*a > *b ? 1 : 0));

}

private bool PutErrorMesesFaltantes(DATE fechad, DATE fechah, int p_dia)
{
	char error[100];
	int  aniod, anioh, k;
	DATE fchd, fchh;

	if (Year(fechad) == Year(fechah)) {
		for (k = Month(fechad); k  <= Month(fechah); k++) {
			sprintf(error, "Falta Asignación para el dia %s en el mes %d/%d\n", diasub(p_dia), k, Year(fechah));
			if (!PutMSGERR(error))
				return FALSE;
		}
	}
	else {
		aniod = Year(fechad);
		anioh = Year(fechah);

		fchd = fechad;
		fchh = anioh != aniod ? DMYToD(1, 12, aniod) : fechah;

		while (aniod  <= anioh) {
			for (k = Month(fchd); k  <= Month(fchh); k++) {
				sprintf(error, "Falta Asignación para el dia %s en el mes %d/%d\n", diasub(p_dia), k, aniod);
				if (!PutMSGERR(error))
					return FALSE;
			}
			aniod++;
			fchd = DMYToD(1, 1, aniod);
			fchh = anioh != aniod ? DMYToD( 1, 12, aniod) : fechah;
		}
	}
	return TRUE;
}

private	void CargarEstructuraAsig(int emp, long cliente, int objetivo, int p_ptoser, int puesto, int nroint,
			 					  long nroleg, char vigil[], char efect[], DATE p_fecasig, TIME hsent,
			 					  TIME hssal, char dia1[], char dia2[], char dia3[], char dia4[],
			 					  char dia5[], char dia6[], char dia7[], long reempl, DATE ffranco,
			 					  int numfran, int francero, char regim[], DATE fechas, DATE fecbaj)
{
	asigSt.emp      = emp;
	asigSt.cliente  = cliente;
	asigSt.objetivo = objetivo;
	asigSt.ptoser   =  p_ptoser;
	asigSt.puesto   = puesto;
	asigSt.nroint   = nroint;
	asigSt.nroleg   = nroleg;
	sprintf(asigSt.vigil, "%s", vigil);
	sprintf(asigSt.efect, "%s", efect);
	asigSt.fecasig  = p_fecasig;
	asigSt.hsent    = hsent;
	asigSt.hssal    = hssal;
	sprintf(asigSt.dia1, "%s", dia1);
	sprintf(asigSt.dia2, "%s", dia2);
	sprintf(asigSt.dia3, "%s", dia3);
	sprintf(asigSt.dia4, "%s", dia4);
	sprintf(asigSt.dia5, "%s", dia5);
	sprintf(asigSt.dia6, "%s", dia6);
	sprintf(asigSt.dia7, "%s", dia7);
	asigSt.reempl  = reempl;
	asigSt.ffranco = ffranco;
	asigSt.numfran = numfran;
	strcpy(asigSt.regim, regim);
	asigSt.fechas  = fechas;
	asigSt.fecbaj  = fecbaj;
}

private bool ValidarPartesProvisorios(DATE fecha, form fm)
{
	int i;
	form fmpadre;

	fmpadre = FmFather(fm);
	fm4 = UseSubform(fmpadre, NROLEG, 0, row_padre);

	fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));

	fm1 = UseSubform(fm0, HORARIO, 0, row_padre);

	for (i = 0; i  < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, CLI, i) ; i++) {
		if (fecha <= fecierre ||
			ExisteParteCargado(FmIFld(fm0, EMP), FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i),
							FmLFld(fmpadre, NROLEG, row_padre), FmIFld(fm1, TIPPTO), FmIFld(fm1, CODINT), fecha, NULL_DATE)) {
			return FALSE;
		}
	}
	for (i = 0; i < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, CLI, i); i++)
		FmSetDFld(fm4, I_FECHASTA, fecha - 1, i);

	return TRUE;
}

private	void PonerEnASIGH(form p_fm4, long nroleg, DATE fecha, int motivo, long reempl, DATE fecbaj)
{
	int  i;

	BorrarInasistencia(FmIFld(fm0, EMP), nroleg, fecha);

	for (i = 0; i < FmFldLen(p_fm4, MULTI) && !FmIsNull(p_fm4, CLI, i) ; i++) {

		SetKey(operac|ASIGbyEMP, FmIFld(fm0, EMP), FmLFld(p_fm4, CLI, i), FmIFld(p_fm4, OBJ, i), nroleg,
					FmIFld(p_fm4, I_TIPPTO, i), FmIFld(p_fm4, I_CODINT, i), FmIFld(p_fm4, I_NUMINT, i));
		if (GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_LOCK) == ERROR){
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2056),"No existe el vigilador %ld asignado en el cliente %ld objetivo %d\nLínea %d",
			         nroleg, FmLFld(p_fm4, CLI, i), FmIFld(p_fm4, OBJ, i), row_padre);
			Stop (0);
		}

		if (!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) > fecha)
			continue;

		BorrarParteGenerado(FmIFld(fm0, EMP), FmLFld(p_fm4, CLI, i), FmIFld(p_fm4, OBJ, i), nroleg, fecha, 
		                    StrCmp(FmSFld(p_fm4, I_VIGIL4, i), EFECTIVO) == 0 ? NULL_DATE : DFld(operac|ASIG_FECHAS),
		                    NULL_DATE, FALSE, NULL_STR, MIN_SHORT, MIN_SHORT, MIN_SHORT);
        
		InitRecord(operac|ASIGH);
		SetIFld(operac|ASIGH_EMP,      IFld(operac|ASIG_EMP));
		SetLFld(operac|ASIGH_CLIENTE,  LFld(operac|ASIG_CLIENTE));
		SetIFld(operac|ASIGH_PTOSER,   IFld(operac|ASIG_PTOSER));
		SetIFld(operac|ASIGH_PUESTO,   IFld(operac|ASIG_PUESTO));
		SetIFld(operac|ASIGH_NROINT,   IFld(operac|ASIG_NROINT));
		SetIFld(operac|ASIGH_OBJETIVO, IFld(operac|ASIG_OBJETIVO));
		SetLFld(operac|ASIGH_NROLEG,   LFld(operac|ASIG_NROLEG));
		SetFld (operac|ASIGH_VIGIL,    SFld(operac|ASIG_VIGIL));
		SetFld (operac|ASIGH_EFECT,    SFld(operac|ASIG_EFECT));
		SetDFld(operac|ASIGH_FECALT,   DFld(operac|ASIG_FECASIG));
		SetFld (operac|ASIGH_DIA1,     SFld(operac|ASIG_DIA1));
		SetFld (operac|ASIGH_DIA2,     SFld(operac|ASIG_DIA2));
		SetFld (operac|ASIGH_DIA3,     SFld(operac|ASIG_DIA3));
		SetFld (operac|ASIGH_DIA4,     SFld(operac|ASIG_DIA4));
		SetFld (operac|ASIGH_DIA5,     SFld(operac|ASIG_DIA5));
		SetFld (operac|ASIGH_DIA6,     SFld(operac|ASIG_DIA6));
		SetFld (operac|ASIGH_DIA7,     SFld(operac|ASIG_DIA7));
		SetTFld(operac|ASIGH_HSENT,    TFld(operac|ASIG_HSENT));
		SetTFld(operac|ASIGH_HSSAL,    TFld(operac|ASIG_HSSAL));
		SetFld (operac|ASIGH_REGIM,    SFld(operac|ASIG_REGIM));
		SetFld (operac|ASIGH_TIPODIA,  SFld(operac|ASIG_TIPODIA));
		SetDFld(operac|ASIGH_FECHAS,   FmDFld(p_fm4, I_FECHASTA, i));
		SetDFld(operac|ASIGH_FECBAJ,   DFld(operac|ASIGH_FECHAS));
		SetIFld(operac|ASIGH_MOTIVO,   motivo);
		SetLFld(operac|ASIGH_REEMPL,   reempl);
		SetDFld(operac|ASIGH_FFRANCO,  DFld(operac|ASIG_FFRANCO));
		SetIFld(operac|ASIGH_NUMFRAN,  IFld(operac|ASIG_NUMFRAN));
		SetIFld(operac|ASIGH_FRANCERO, IFld(operac|ASIG_FRANCERO));
		SetIFld(operac|ASIGH_CODROL,   IFld(operac|ASIG_CODROL));
		SetIFld(operac|ASIGH_FILA,     IFld(operac|ASIG_FILA));
		SetIFld(operac|ASIGH_COLUM,    IFld(operac|ASIG_COLUM));
		PutRecord(operac|ASIGH);
		DelRecord(operac|ASIG);
		FreeTable(operac|ASIGH);
		FreeTable(operac|ASIG);
		// Actualizo la cantidad de vigiladores asignados al puesto del que se saca.....
		SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIG_CLIENTE));
		SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIG_OBJETIVO));
		SetIFld(operac|PUESTOS_TIPPTO,  IFld(operac|ASIG_PTOSER));
		SetIFld(operac|PUESTOS_CODINT,  IFld(operac|ASIG_PUESTO));
		if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_LOCK) != ERROR) {
			SetIFld(operac|PUESTOS_VIGI, IFld(operac|PUESTOS_VIGI) < 100? 0:IFld(operac|PUESTOS_VIGI) - 100);
				PutRecord(operac|PUESTOS);
		}

		//Si es Part-Time paso tambien los DIASPTIME a DIASPTIMEH
		if (StrCmp(FmSFld(fm1, I_VIGILAD), PARTTIME) == 0) {
			SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), FmLFld(p_fm4, CLI, i), FmIFld(p_fm4, OBJ, i), nroleg,
				FmIFld(p_fm4, I_TIPPTO, i), FmIFld(p_fm4, I_CODINT, i), FmIFld(p_fm4, I_NUMINT, i), NULL_DATE);
			while (GetRecord(operac|DIASPTIMEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
				if (DFld(operac|DIASPTIME_DIA) <= fecbaj) {
					InitRecord(operac|DIASPTIMEH);
					SetIFld(operac|DIASPTIMEH_EMP,      IFld(operac|DIASPTIME_EMP));
					SetLFld(operac|DIASPTIMEH_CLIENTE,  LFld(operac|DIASPTIME_CLIENTE));
					SetIFld(operac|DIASPTIMEH_OBJETIVO, IFld(operac|DIASPTIME_OBJETIVO));
					SetLFld(operac|DIASPTIMEH_NROLEG,   LFld(operac|DIASPTIME_NROLEG));
					SetIFld(operac|DIASPTIMEH_TIPPTO,   IFld(operac|DIASPTIME_TIPPTO));
					SetIFld(operac|DIASPTIMEH_PUESTO,   IFld(operac|DIASPTIME_PUESTO));
					SetIFld(operac|DIASPTIMEH_NROINT,   IFld(operac|DIASPTIME_NROINT));
					SetDFld(operac|DIASPTIMEH_DIA,      DFld(operac|DIASPTIME_DIA));
					SetTFld(operac|DIASPTIMEH_HENT,     TFld(operac|DIASPTIME_HENT));
					SetTFld(operac|DIASPTIMEH_HSAL,     TFld(operac|DIASPTIME_HSAL));
					PutRecord(operac|DIASPTIMEH);
					FreeTable(operac|DIASPTIMEH);
				}
				DelRecord(operac|DIASPTIME);
			}
		}
	}
}


static int CalcNroint(int emp, long cliente, int objetivo, long nroleg, int p_ptoser, int puesto)
{
	int  nroint, i;

	nroint = GetNextNroint(emp, cliente, objetivo, nroleg, p_ptoser, puesto);

	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		if (nroleg != FmLFld(fm0, NROLEG, i))
			continue;
		fm1 = UseSubform(fm0, HORARIO, 0, i);
		if (p_ptoser != FmIFld(fm1, TIPPTO) || puesto != FmIFld(fm1, CODINT))
			continue;

		if (nroint <= FmIFld(fm1, I_NROINT)) {
			nroint = FmIFld(fm1, I_NROINT) + 1;
		}
	}
	RestauraSubForm(fm0, HORARIO, 0, row_padre);
	return nroint;
}

static bool DiaFranco(int emp, long nroleg, DATE p_fecasig, DATE fechas, char * p_dia)
{
	int  i;
	DATE fecha;

	for (fecha = p_fecasig; fecha <= p_fecasig + 6 ; fecha++) {
		if (Franco(emp, nroleg, fecha, "E", GetNumFrancoEfectivo(emp, nroleg, p_fecasig))) {
			if (!StrCmp(p_dia, DiaLetra(fecha))) {
				return TRUE;
			}
		}
	}
	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		if (FmLFld(fm0, NROLEG, i) != nroleg)
			continue;
		if (*FmSFld(fm0, EFECT, i) != 'E')
			continue;
		fm1 = UseSubform(fm0, HORARIO, 0, i);
		for (fecha = p_fecasig; fecha <= p_fecasig + 6; fecha++) {
			if (FrancoFm(emp, nroleg, fecha, FmSFld(fm1, REGIM), FmDFld(fm1, FECFRA), FmIFld(fm1, NUMFRAN))) {
				if (!StrCmp(p_dia, DiaLetra(fecha))) {
					RestauraSubForm(fm0, HORARIO, 0, row_padre);
					return TRUE;
				}
			}
		}
	}
	RestauraSubForm(fm0, HORARIO, 0, row_padre);
	return FALSE;
}

static bool FrancoFm(int emp, long nroleg, DATE fecha, char * regimen, DATE fecfranco, int numfran)
{
	bool esfranco = FALSE, salir = FALSE;
	int  diaslab, diasfranco;
	DATE ffrancod, ffrancoh, ftrabd, ftrabh;

	if (fecfranco == NULL_DATE || !strcmp(regimen, NULL_STR))
		return FALSE;

	// porque la primer fecha de franco del periodo es ASIG_FFRANCO
	// con lo cual cualquier fecha menor es laborable.
	if (fecha < fecfranco) {
		return FALSE;
	}

	// saco cantidad de dias de franco y cantidad de dias laborables que tiene el empleado
	diasfranco = GetDiasFranco(regimen, FALSE);
	diaslab    = GetDiasLaboral(regimen, FALSE);
	// fin saco cantidad de dias de franco y cantidad de dias laborables que tiene el empleado

	if (diasfranco == NULL_SHORT || diaslab == NULL_SHORT) {
		return FALSE;
	}
	// saco primer rango de fechas donde tiene franco
	ffrancod = fecfranco;

	//fecha de franco desde es: la fecha de franco desde mas la cantidad de días franco pendientes
	// cantidad de días francos totales menos  numfran - 1 (dias pasados)

	if (numfran != NULL_SHORT)
		ffrancoh = ffrancod + (diasfranco - (numfran - 1) - 1);
	else
		ffrancoh = ffrancod + diasfranco - 1;

	// fin saco primer rango de fechas donde tiene franco

	// saco primer rango de fechas donde tiene que trabajar
	ftrabd = ffrancoh + 1;
	ftrabh = ftrabd   + diaslab - 1;

	// fin saco primer rango de fechas donde tiene que trabajar
	while (!salir && !esfranco) {
		if (fecha >= ffrancod && fecha <= ffrancoh) {
			esfranco = TRUE;
		}
		if (fecha >= ftrabd && fecha <= ftrabh) {
			salir = TRUE;
		}
		ffrancod = ftrabh   + 1;
		ffrancoh = ffrancod + diasfranco - 1;
		ftrabd   = ffrancoh + 1;
		ftrabh   = ftrabd   + diaslab    - 1;
	}
	return esfranco;
}

static void RestauraSubForm(form fm, fmfield fno, int pos, int row)
{
	form fmnuevo;

	fmnuevo = UseSubform(fm, fno, pos, row);
}


private void MuevoAsigPartT(int p_lin, DATE p_fecbaja)
{
	form  fmaux;

	//Muevo la asignación por dia de los partime
	if (strcmp(FmSFld(fm0, VIGILAD, p_lin), PARTTIME) != 0)
		return;

	PushRecord(operac|ASIG);


    SetKey(operac|ASIGbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmLFld(fm0, OBJET), 
                             FmLFld(fm0, NROLEG, p_lin),NULL_SHORT,NULL_SHORT,NULL_SHORT);
    while(GetRecord(operac|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 4)!=ERROR) {

		fmaux = UseSubform(fm0, HORARIO, 0, p_lin);
		// el nroint deben ser iguales
		if ( FmIFld(fmaux, I_NROINT) != IFld(operac|ASIG_NROINT))
			continue;

		SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO),
								  FmLFld(fm0, NROLEG, p_lin), IFld(operac|ASIG_PTOSER),
								  IFld(operac|ASIG_PUESTO), IFld(operac|ASIG_NROINT), NULL_DATE);
		while (GetRecord(operac|DIASPTIMEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {

			if (DFld(operac|DIASPTIME_DIA) <= p_fecbaja) {

				InitRecord(operac|DIASPTIMEH);
				SetKey(operac|DIASPTIMEH, IFld(operac|DIASPTIME_EMP),
				                          LFld(operac|DIASPTIME_CLIENTE),
			                          IFld(operac|DIASPTIME_OBJETIVO),
			                          LFld(operac|DIASPTIME_NROLEG),
			                          IFld(operac|DIASPTIME_TIPPTO),
			                          IFld(operac|DIASPTIME_PUESTO),
			                          IFld(operac|DIASPTIME_NROINT),
			                          DFld(operac|DIASPTIME_DIA));
                          
				SetTFld(operac|DIASPTIMEH_HENT,     TFld(operac|DIASPTIME_HENT));
				SetTFld(operac|DIASPTIMEH_HSAL,     TFld(operac|DIASPTIME_HSAL));

				PutRecord(operac|DIASPTIMEH);
				FreeTable(operac|DIASPTIMEH);

			}
			DelRecord(operac|DIASPTIME);
		}
	}

	PopRecord(operac|ASIG);
}

private void HelpFila(int p_rol)
{
	int selected;
	static char buff0[60];
	char *label = "Turnos del Rol";

	selected = PopUpMenu(10,
						 75,
						 label,
						 get_fila,
						 buff0,
						 NULLFP,
						 NULL,
						 POP_DEFAULT);

	if (selected >= 0 ) {
		FmSetIFld(fm1, FILA, selected + 1);
	}
	FinPuestosVivos();
}

private char *get_fila(char *buff, int lin)
{
	char aux[5];
	bool encontro = FALSE;

	sprintf(buff, "Ciclo %d : ", lin);

	SetKey(operac|RROLbyCODROL, rol, lin, MIN_SHORT);
	while (GetRecord(operac|RROLbyCODROL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		sprintf(aux, " %s", SFld(operac|RROL_VALOR));
		strcat(buff, aux);
		encontro = TRUE;
	}
	if (!encontro)
		return NULL;

	return buff;
}

void GuardoFm1()
{
	int v_i, // campo del fm
	    v_j; // posicion dentro del vector

	// Limpio Matriz de campos
	for (v_j=0; v_j<80; v_j ++) 
		sprintf(g_fm1_char[v_j], "%s", NULL_STR);

	// Copio Todos los campos de fm1  a vector
	for (v_i=I_EFECT, v_j=0; v_i<=MOTIVO1 && v_j<80; v_i++, v_j++) {
		if (v_i != AGRTOT && v_i != AGRASIG && v_i != AGRPUESTO && v_i != AGRUPD && v_i != AGRHORAS)
			sprintf(g_fm1_char[v_j], "%s", FmSFld(fm1, v_i));
	}
	
	//Guardo valor de campo FECHAS de fm3
	v_j ++;
	fm3   = UseSubform(fm1, FECHA, 0);
	sprintf(g_fm1_char[v_j], "%s", FmSFld(fm3, FECHAS));

}

void RecuperoFm1()
{
	int v_i, // campo del fm
	    v_j; // posicion dentro del vector

	// Copio Todos los campos de vector a fm1  
	for (v_i=I_EFECT, v_j=0; v_i<=MOTIVO1 && v_j<80;  v_i++, v_j++) {
		
		/* En este switch van todos los campos que son agrupados o no van porque 
		   no figura el numero de campo en el archivo asige1.fmh  */

		switch(v_i){
			case I_FECHAS - 1:
			case MOTIVO1 - 1 :
			case AGRTOT :
			case AGRASIG : 
			case AGRPUESTO :
			case AGRUPD :
			case AGRHORAS : 
				continue;
		}
		FmSetFld(fm1, v_i, g_fm1_char[v_j]);
	} 

	if (FmIsNull(fm1, FECHA))
		FmSetDFld(fm1, I_FCHDES, FmDFld(fm1, FECHA));

	//Guardo valor de campo FECHAS de fm3
	v_j ++;
	fm3   = UseSubform(fm1, FECHA, 0);
	FmSetFld(fm3, FECHAS, g_fm1_char[v_j]);

}

private void Cargo_fm1(form formulario, fmfield campo, int fila)
{
	campo_fm1 = NULL_SHORT;

	DoSubform(formulario, before_asige1, after_asige1, campo, 0, fila);

	if (FmKeyCode(fm1) == K_IGNORE) {
		switch (campo_fm1) {
			case FECHA :
			case CONTROL_FLD :
				break;
			default :
				if (WiDialog(WD_OK|WD_NO, WD_NO, TituloMsg(TMSG_WAR, 2054), "Salida de formulario no valida\nLos cambios seran ignorados.\nDesea intentar otra vez?\nLínea %d", fila)==WD_OK) {
					RecuperoFm1();
					Cargo_fm1(formulario, campo, fila);
				}
				else
					RecuperoFm1();
				break;
		}
	}
}

bool TieneProvisorios(int p_emp, long p_nroleg, long p_cliente, int p_objet, int p_fecasi)
{
	bool v_hay_provisorio = FALSE;
	dbcursor v_c_asig = NULL, v_c_asigh = NULL;
	DATE v_fecdes, v_fechas;

	int v_i;

	// Me fijo en el formulario
	for (v_i = 0; v_i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {
		if (p_nroleg!=FmLFld(fm0, NROLEG, v_i))
			continue;

		if(strcmp(FmSFld(fm0, EFECT, v_i), EFECTIVO)==0 )
			continue;

		// No tomo en cuenta los que estoy desasignando
		if (!FmIFld(fm0, ASIGNA, v_i))
			continue;

		// No Tomo provisorios en los que no intervenga la fecha en cuestion

		fm1 = UseSubform(fm0, HORARIO, 0, v_i);
		fm3 = UseSubform(fm1, FECHA, 0);
		v_fecdes=FmDFld(fm1, FECHA);
		v_fechas=FmDFld(fm3, FECHAS);

		if (p_fecasi<v_fecdes || p_fecasi>v_fechas)
			continue;


		v_hay_provisorio=TRUE;
	} 

	// Si no encontre busco en la base ASIG
	if (!v_hay_provisorio){
		PushRecord(operac|ASIG);
		v_c_asig  = CreateCursor(operac|ASIGbyNROLEG, IO_EABORT|IO_NOT_LOCK);
		SetCursorFrom(v_c_asig, p_emp, p_nroleg, MIN_LONG, MIN_SHORT);
		SetCursorTo  (v_c_asig, p_emp, p_nroleg, MAX_LONG, MAX_SHORT);
		while (FetchCursor(v_c_asig) != ERROR) {
			if (StrCmp(SFld(operac|ASIG_EFECT), EFECTIVO)==0) 
				continue;

			if (LFld(operac|ASIG_CLIENTE)==p_cliente && IFld(operac|ASIG_OBJETIVO)==p_objet )
				continue;

			if (DFld(operac|ASIG_FECBAJ)<=p_fecasi && !IsNull(operac|ASIG_FECBAJ))
				continue;

			v_hay_provisorio=TRUE;
		} 
		DeleteCursor(v_c_asig);
		PopRecord(operac|ASIG);

		// Si no encontre busco en la base ASIGH
		if (!v_hay_provisorio) {
			PushRecord(operac|ASIGH);
			v_c_asigh  = CreateCursor(operac|ASIGHbyNROLEG, IO_EABORT|IO_NOT_LOCK);
			SetCursorFrom(v_c_asigh, p_emp, p_nroleg, MIN_LONG, MIN_SHORT);
			SetCursorTo  (v_c_asigh, p_emp, p_nroleg, MAX_LONG, MAX_SHORT);
			while (FetchCursor(v_c_asigh) != ERROR) {
				if (StrCmp(SFld(operac|ASIGH_EFECT), EFECTIVO)==0) 
					continue;

				if (DFld(operac|ASIGH_FECBAJ)<=p_fecasi && !IsNull(operac|ASIGH_FECBAJ))
					continue;

				if (LFld(operac|ASIGH_CLIENTE)==p_cliente && IFld(operac|ASIGH_OBJETIVO)==p_objet )
					continue;

				v_hay_provisorio=TRUE;
			} 
			DeleteCursor(v_c_asigh);
			PopRecord(operac|ASIGH);
		}	
	} 

 	return v_hay_provisorio;
}
static void MuestraNrodeLineaActual(fmfield fno, int row)
{
	char v_auxi[16];

	if (FmInMult(fm0, fno)!=ERROR)
		sprintf(v_auxi, "¡¡¡Linea %03d¡¡¡", row);
	else 
		sprintf(v_auxi, "¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡", row);
	FmSetFld(fm0, LINEA, v_auxi);
	
}


static void Validaciones()
{
	int v_i=NULL_SHORT;
    DATE v_fecha=NULL_DATE;
    char v_msg[100];

	for (v_i = 0; v_i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {

		if (fecierre!=NULL_DATE)
			v_fecha = fecierre - 10;
		else
			v_fecha = NULL_DATE;
		
		// Valida que por cada vigilador no tenga 2 asignaciones efectiva con misma fecha de comienzo	
		if (!ValidaXLegajo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, v_i), &v_fecha)){
			sprintf(v_msg, "Para el Legajo %ld hay mas de una asignacion con fecha %.3D\nAvisar a Sistemas", FmLFld(fm0, NROLEG, v_i), v_fecha);

			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2073), v_msg);
			GrabaMsg(g_codprog, g_oi_sesion, _WMSGLOG_TIPO_ERROR, v_msg);

		}

		if (fecierre!=NULL_DATE)
			v_fecha = fecierre - 10;
		else
			v_fecha = NULL_DATE;

		// Valida por cada vigilador que no haya fecha de comienzo de asignaciones mayores a fechas de fin 	
		if (!ValidaMalRangoFecha(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, v_i), &v_fecha)) {
			sprintf(v_msg, "Para el Legajo %ld hay un Error de fechas en la asignacion del dia %.3D\nAvisar a Sistemas", FmLFld(fm0, NROLEG, v_i), v_fecha);
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 2074), v_msg);
			GrabaMsg(g_codprog, g_oi_sesion, _WMSGLOG_TIPO_ERROR, v_msg);
		}
	}
}

static bool VerificaPermisos()
{
	int v_i=NULL_SHORT;
	dbcursor v_c_pervig;

	for (v_i = 0; v_i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, v_i); v_i++) {

		fm1 = UseSubform(fm0, HORARIO, 0, v_i);

		if (!FmIFld(fm1, I_MODIF)) 
			continue;

		v_c_pervig=CreateCursor(operac|PERVIGbyULTMOD, IO_NOT_LOCK);
		SetCursorFrom(v_c_pervig, TRUE, FmIFld(fm0, EMP),  NULL_STR,   NULL_STR,    NULL_LONG,  NULL_SHORT);
		SetCursorTo  (v_c_pervig, TRUE, FmIFld(fm0, EMP),  HIGH_VALUE, HIGH_VALUE,  MAX_LONG,   MAX_SHORT);
		while(FetchCursor(v_c_pervig)!=ERROR) {
			if (!IFld(operac|PERVIG_ACTIVO))
				continue;

			if (FmLFld(fm0, NROLEG, v_i)!=LFld(operac|PERVIG_NROLEG))
				continue;

			if (IFld(operac|PERVIG_TIPPER) != _TIPPER_ASIGNA)
				continue;

			if (DFld(operac|PERVIG_FECINI)<fecierre)
				continue;

			if (!ValidaFilialXusr(SFld(operac|PERVIG_FILORI)))
				continue;
			
			fm1 = UseSubform(fm0, HORARIO, 0, v_i);

			if (FmDFld(fm1, FECHA) < DFld(operac|PERVIG_FECINI)){
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2072),"El Vigilador %ld fue Cedido a la Filial %s \n con fecha %.3D y se Intento Asignar con fecha %.3D\n la operacion sera interrumpida\nLinea %d",
				         FmLFld(fm0, NROLEG, v_i), SFld(operac|PERVIG_FILIAL), DFld(operac|PERVIG_FECINI), FmDFld(fm1, FECHA), v_i);
				         
				return FALSE;
			}
		}
		DeleteCursor(v_c_pervig);
	}
	return TRUE;
}

static bool ControlesNivelLinea(int row, bool p_cambiofm)
{
	fm1 = UseSubform(fm0, HORARIO, 0, row);

	// Verifico que no se deje incompleto el subformulario fm1
	if (!FmIsNull(fm0, NROLEG, row) && FmIsNull(fm1, FECHA)) {
		WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2062), "No se completo correctamente la asignacion\nLinea %d",row);
		return FALSE;
	}

	fm1 = UseSubform(fm0, HORARIO, 0, row);

	/*Verifico que si se esta desasignando el efectivo, no tenga asignaciones provisorias sin desasignar*/

/*

FER para ver cuando vuelvo de vacaciones

	if(p_cambiofm){
		if (!FmIsNull(fm1, FECHA)){
			if(TieneProvisorios(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmDFld(fm1, FECHA)) ) {
				WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_ERR, 2060),
				         "No se puede desasignar el legajo %d\nsin desasignar antes sus asignaciones provisorias\nLinea %d", FmLFld(fm0, NROLEG, row), row);
				return FALSE;
			}
		}
	}
*/
	return TRUE;

}


