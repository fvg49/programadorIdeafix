/********************************************************************
* MODULE & VERSION : @(#)asigh.c	1.2
* DATE             : 02/06/27
* TIME             : 16:05:28
* CREATED          :
* DESCRIPTION:
*      Este programa es una copia de Asignación de Vigiladores para
*      asignar vigiladores con fecha anterior a la primer asignacion
*
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "asig.h"
#include "asigh.fmh"
#include "asigh1.fmh"
#include "asigh2.fmh"
#include "asigh3.fmh"
#include "asigh4.fmh"
#include "asigh5.fmh"
#include "asigh6.fmh"
#include "sue.sch"
#include "operac.sch"
#include "bill.sch"
#include "comerc.sch"

#define	WAR_EFECT         "El Vigilador ya se encuentra efectivo en el Cliente: %ld y Objetivo: %d.\nDesea reasignarlo ?."
#define	CLI_INEX          "El Cliente: %ld no existe!."
#define	OBJET_INEX        "El Objetivo: %d del Cliente: %ld no existe!."
#define	ERR_SUPERPOS      "Superposición Horaria!!!\nEl Vigilador: %s se encuentra asignado en:\nCliente: %ld  Obj: %d  Fec.Asig.: %.1D\nDías: %s %s %s %s %s %s %s  Hr.Ent.: %.1T  Hr.Sal.: %.1T"
#define	ERROR_SUPER_PTIME "Part Time con Superposición Horiaria!!!\n"
#define	ERR_FALTAN        "Falta cargar información de la asignación para el Vigilador: %ld %s"
#define	WAR_OCUP          "El Puesto tiene todos los vigiladores asignados para el día: %.1D.\n     [1mPor favor, verifique los datos de asignación del vigilador.[0m"
#define WAR_OCUP_EFECT    "       El Puesto tiene todos los vigiladores asignados.\n[1mPor favor, verifique los datos de asignación del vigilador.[0m"
#define ERR_ASIG_VIG_CONF "El Vigilador ya es de tipo %s en el Cliente %ld y Objetivo %d.\nDesea modificarlo ?."
#define MAX_MESES         20
#define MAX_FECHAS        200 //92
#define MAX_DIAS          90 //30 originalmente tenia 30
#define MAX_ERRORS        20 // Se acumulan solo 20 errores, que es el mas o menos la capacidad de la
							 // pantalla para desplegarlos.

// Retorna el subindice segun la letra del dia, para el vector de validacion de frecuencia
#define	subdia(dia)	    (dia == 'L' ? 0 : dia == 'M' ? 1 : dia == 'X' ? 2 :	dia == 'J' ? 3 : dia == 'V' ? 4 : dia == 'S' ? 5 : dia == 'D' ? 6 : NULL_SHORT)
// Inversa de la anterior.
// Dado el subindice retorna la letra del dia a la que corresponde.
#define	diasub(i)	    (i == 0 ? "L" : i == 1 ? "M" : i == 2 ? "X" : i == 3 ? "J" : i == 4 ? "V" : i == 5 ? "S": i == 6 ? "D" : NULL_STR)
#define REG_ESP "4x2x12"	

extern double ConvHraInt(TIME hdesde, TIME hhasta);
static fm_status after(form, fmfield, int),        //before(form, fmfield, int),
                 after_asigh1(form, fmfield, int), before_asigh1(form, fmfield, int),
                 after_asigh2(form, fmfield, int),
                 after_asigh3(form fm, fmfield fno, int row),
                 after_asigh5(form, fmfield, int), before_asigh5(form, fmfield, int),
                 after_asigh6(form, fmfield, int);
static fm_status MostrarAsig(form fm, fmfield fno, int row);
static fm_status AyudaTipPto(form fm, fmfield fno, int row);
static fm_status CargarDiasPTime(long nroleg, int ptoser, int puesto, int nroint, form fm);
static int  validate(void);
static int  validPuesto();
static int  CalcNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto);
static void LeerCliente (long cliente);
static void LeerObjetivo(long cliente, int objetivo);
static void Lectura(fm_cmd, find_mode);
static void ProcesoPostu(void);
static void display(char * buffer);
static void displayPuesto(char * buffer);
static void GetRegimenEfectivoFm(int emp, long nroleg, char *regimen, DATE fecha);
static void RestauraSubForm(form fm, fmfield fno, int pos, int row);
static void VerificoDobleAsigI(long nroleg);

static bool ValInfo();
static bool HayVacante(long nroleg, int tippto, int codint, DATE fdesde, DATE fhasta, DATE *fechocup);
static bool TieneProvi(long nroleg);
static bool BajaPuesto(long cliente, int objet, int tippto, DATE fecasig);
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
private void CargarOtrasAsignaciones(form fm4, long nroleg);
private void BorrarParteGeneradoI(long cliente, int objetivo, long nroleg, DATE fechad, DATE fechah,
								 DATE fecbaj, bool porfecha, char *efect, int ptoser, int puesto, int nroint);
private void PonerEnASIGH(form fm4, long nroleg, DATE fecha, int motivo, long reempl, DATE fecbaj);
private void InitMSGERR();
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
private bool ExisteParteCargadoI(long cliente, int objetivo, long nroleg, DATE fechad, DATE fechah);
private bool PutErrorMesesFaltantes(DATE fechad, DATE fechah, int dia);
private int  compdia(DATE *a, DATE *b);
bool YaTieneAsignacion(short emp, long nroleg, DATE fecha);

struct mes {
    int    mes;
    double horas;
    char   error[60];
} meses[MAX_MESES];
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
int      pue[MAXFILA][MAXCOL], ptoser, codint, row_padre, hsmes[13];
bool     actualizar = TRUE;
DATE     fecasig;
TIME     horent, horsal;
char     t_vigil[2], // Para popup de puestos según el tipo de vigilador (V, P)
         t_efect[2]; // Para popup de puestos según el tipo de vigilador (V, P) y cond. de efectivo

wcmd(asigh, 1.2 06/27/02)
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

	fm0 = OpenForm("asigh", FM_EABORT);

	FmOnKey(fm0, K_META, MostrarAsig, NROLEG, NROLEG);

	FmCallBacks(fm0, HORARIO, "asigh1", before_asigh1, after_asigh1);
	FmCallBacks(fm0, ASIGNA,  "asigh2", NULLFP, after_asigh2);

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:      Lectura(cmd, THIS_KEY); break;
		case FM_READ_NEXT: Lectura(cmd, NEXT_KEY); break;
		case FM_READ_PREV: Lectura(cmd, PREV_KEY); break;
		case FM_ADD :
			InitRecord(operac|ASIG);
			InitRecord(operac|ASIGH);
			InitRecord(operac|DIASPTIME);
		case FM_UPDATE :
			if (!actualizar || ValInfo()) {
				FmSetError(fm0, CONTROL_FLD, M_ERR_ACT);
				actualizar = TRUE;
				break;
			}
			BeginTransaction();
			// GraboPuesto();
			ProcesoPostu();
			EndTransaction();
			break;
		case FM_IGNORE :
			break;
		}
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{

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
		if (!FmIFld(fm0, ASIGNA, i)) {
			fm2 = UseSubform(fm0, ASIGNA, 0, i);
			errorasig = FmIFld(fm2, ERRORASIG); 
			fecbaja = FmDFld(fm2, FECBAJ);
		}
		else {
			fm6 = UseSubform(fm1, MOTIVO1, 0, 0);
			fecbaja = FmDFld(fm6, FECBAJ6);
		}
		
		SetIFld(operac|ASIGH_EMP,      FmIFld(fm0, EMP));
		SetLFld(operac|ASIGH_CLIENTE,  FmLFld(fm0, CLIEOT));
		SetIFld(operac|ASIGH_OBJETIVO, FmIFld(fm0, OBJET));
		SetIFld(operac|ASIGH_PTOSER,   FmIFld(fm1, TIPPTO));
		SetIFld(operac|ASIGH_PUESTO,   FmIFld(fm1, CODINT));
		SetLFld(operac|ASIGH_NROLEG,   FmLFld(fm0, NROLEG, i));
		SetFld (operac|ASIGH_VIGIL,    FmSFld(fm0, VIGILAD, i));
		SetFld (operac|ASIGH_EFECT,    FmSFld(fm0, EFECT,  i));
		SetDFld(operac|ASIGH_FECALT,   FmDFld(fm1, FECHA));
		SetTFld(operac|ASIGH_HSENT,    FmTFld(fm1, HORENT));
   		SetTFld(operac|ASIGH_HSSAL,    FmTFld(fm1, HORSAL));
		SetFld (operac|ASIGH_DIA1,     FmSFld(fm1, DIA1));
		SetFld (operac|ASIGH_DIA2,     FmSFld(fm1, DIA2));
		SetFld (operac|ASIGH_DIA3,     FmSFld(fm1, DIA3));
		SetFld (operac|ASIGH_DIA4,     FmSFld(fm1, DIA4));
		SetFld (operac|ASIGH_DIA5,     FmSFld(fm1, DIA5));
		SetFld (operac|ASIGH_DIA6,     FmSFld(fm1, DIA6));
		SetFld (operac|ASIGH_DIA7,     FmSFld(fm1, DIA7));
		SetIFld(operac|ASIGH_MOTIVO,   NULL_SHORT);
		SetLFld(operac|ASIGH_REEMPL,   NULL_LONG);
		SetFld (operac|ASIGH_REGIM,    FmSFld(fm1, REGIM));
		SetDFld(operac|ASIGH_FFRANCO,  FmDFld(fm1, FECFRA));
		SetIFld(operac|ASIGH_NUMFRAN,  FmIFld(fm1, NUMFRAN));
		SetIFld(operac|ASIGH_FRANCERO, FmIFld(fm1, FRANCERO));
		SetIFld(operac|ASIGH_NROINT,   FmIFld(fm1, I_NROINT));
		SetFld (operac|ASIGH_TIPODIA,  FmSFld(fm1, TIPODIA));
		fm3 = UseSubform(fm1, FECHA, 0, i);
		SetDFld (operac|ASIGH_FECBAJ,  FmDFld(fm3, FECHAS));
		SetDFld (operac|ASIGH_FECHAS,  FmDFld(fm3, FECHAS));
		PutRecord(operac|ASIGH);
		FreeTable(operac|ASIGH);

		// Borro los que haya tenido de antes para regrabar.
		SetKey(operac|DIASPTIMEHbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
										  FmLFld(fm0, NROLEG, i), IFld(fm1, TIPPTO), FmIFld(fm1, CODINT),
										  FmIFld(fm1, I_NROINT), MIN_DATE);
		while (GetRecord(operac|DIASPTIMEHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR)
			DelRecord(operac|DIASPTIMEH);

		//Actualizo la asignación de los part-times que son por horas.
		for (j = 0; j < FmFldLen(fm5, MULTDIAS) && !FmIsNull(fm5, DIA, j); j++) {
			InitRecord(operac|DIASPTIMEH);
			SetIFld(operac|DIASPTIMEH_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|DIASPTIMEH_CLIENTE,  FmLFld(fm0, CLIEOT));
			SetIFld(operac|DIASPTIMEH_OBJETIVO, FmIFld(fm0, OBJET));
			SetLFld(operac|DIASPTIMEH_NROLEG,   FmLFld(fm0, NROLEG, i));
			SetIFld(operac|DIASPTIMEH_TIPPTO,   FmIFld(fm1, TIPPTO));
			SetIFld(operac|DIASPTIMEH_PUESTO,   FmIFld(fm1, CODINT));
			SetIFld(operac|DIASPTIMEH_NROINT,   FmIFld(fm1, I_NROINT));
			SetDFld(operac|DIASPTIMEH_DIA,      FmDFld(fm5, DIA, j));
			SetTFld(operac|DIASPTIMEH_HENT,     FmTFld(fm5, HENT, j));
			SetTFld(operac|DIASPTIMEH_HSAL,     FmTFld(fm5, HSAL, j));
			PutRecord(operac|DIASPTIMEH);
			FreeTable(operac|DIASPTIMEH);
		}
		// VerificoDobleAsigI(FmLFld(fm0, NROLEG, i));
	}
}
/*
static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EFECT :
		FmSetFld(fm, I_WIEFECT, FmSFld(fm, fno, row), row);
		break;
	}
	return FM_OK;
}
*/
static fm_status after(form fm, fmfield fno, int row)
{
	bool efectivo,   //Si pongo Efectivo en el form.
		 estaefec;   //Si ya está asignado Efectivo en la base.
	DATE fechahasta;
	int  i;

	if (FmKeyCode(fm) == K_DEL && fno >= NROLEG && fno <= HORARIO) {
		if (FmIFld(fm, I_BASE, row)) {
			Warning("No puede eliminar al vigilador.");
			return FM_REDO;
		}
		else
			return FM_OK;
	}
	row_padre = row;
	switch (fno) {
	case NROLEG :
		// Valida que el legajo seleccionado no está inactivo!
		FmSetIFld(fm, I_ROW, row, row);
		if (FmChgFld(fm) && (FmIFld(fm0, I_ACTIV, row) == 0 || FmIFld(fm0, I_ACTIV, row) == 2)) {
			return FmErrMsg(fm0, M_LEGINAC);
		}
		else {
			if (FmIFld(fm0, I_ACTIV, row) == 0 || FmIFld(fm0, I_ACTIV, row) == 2) {
				Warning("El legajo se encuentra inactivo.");
				return FM_OK;
			}
		}
		if (FmChgFld(fm) && !strcmp(FmFldPrev(fm), NULL_STR))
			FmSetIFld(fm0, I_NEW, TRUE, row);
		break;
	case OBJET :
		LeerObjetivo(FmLFld(fm, CLIEOT), FmIFld(fm, fno));
		if (!EstaEnLaPolicia(FmIFld(fm, EMP), FmLFld(fm, CLIEOT), FmLFld(fm, fno)))
			Warning("El Objetivo no está dado de alta en la Policia.");
		break;
	case ASIGNA :
		if (FmIFld(fm0, ASIGNA, row))
			break;

		fm1 = UseSubform(fm0, HORARIO, 0, row);
		fm2 = UseSubform(fm,  ASIGNA,  0, row);
		FmSetDFld(fm2, I_FECASIG2, FmDFld(fm1, FECHA));

		if (!StrCmp(FmSFld(fm0, EFECT, row), EFECTIVO) && TieneProvi(FmLFld(fm0, NROLEG, row)))
			return FmErrMsg(fm0, M_TIENE_PROVI);

		if (!StrCmp(FmSFld(fm0, EFECT, row), EFECTIVO))
			fechahasta = MAX_DATE;
		else {
			fm3	= UseSubform(fm1, FECHA,   0);
			fechahasta = FmDFld(fm3, FECHAS);
		}
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
			if (StrCmp(FmSFld(fm, VIGILAD, row), FmSFld(fm, VIGILAD, i)))
				return FmErrMsg(fm0, M_ERR_VIG, FmSFld(fm, VIGILAD, i)); 
			if (FmIsNull(fm0, IS_EFECT, row) && efectivo && !StrCmp(FmSFld(fm0, EFECT, row), FmSFld(fm0, EFECT,i)))
				return FmErrMsg(fm0, M_ERR_EFEC);
		}

/******************************
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
//			if (FmLFld(fm0, CLIEOT) == LFld(AlFld(asig, operac|ASIG_CLIENTE)) &&
//				FmIFld(fm0, OBJET)  == IFld(AlFld(asig, operac|ASIG_OBJETIVO))) {
//				continue;
//			}
			if (!StrCmp(SFld(AlFld(asig, operac|ASIG_EFECT)), EFECTIVO)) {
				estaefec = TRUE;
				FmSetDFld(fm, I_FECEFEC,    DFld(AlFld(asig, operac|ASIG_FECASIG)),  row);
				FmSetLFld(fm, I_CLIEFEC,    LFld(AlFld(asig, operac|ASIG_CLIENTE)),  row);
				FmSetIFld(fm, I_OBJEFEC,    IFld(AlFld(asig, operac|ASIG_OBJETIVO)), row);
				FmSetIFld(fm, I_PTOEFEC,    IFld(AlFld(asig, operac|ASIG_PTOSER)),   row);
				FmSetIFld(fm, I_CODINTEFEC, IFld(AlFld(asig, operac|ASIG_PUESTO)),   row);
			}
			if (StrCmp(FmSFld(fm, VIGILAD, row), SFld(AlFld(asig, operac|ASIG_VIGIL))) != 0) {
				if (efectivo) {
					if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, ERR_ASIG_VIG_CONF,
							SFld(AlFld(asig, operac|ASIG_VIGIL)), LFld(AlFld(asig, operac|ASIG_CLIENTE)),
							IFld(AlFld(asig, operac|ASIG_OBJETIVO))) == WD_NO)
						return FM_REDO;
				}
				else 
					return FmErrMsg(fm0, M_ERR_ASIG_VIG, SFld(AlFld(asig, operac|ASIG_VIGIL)),
								LFld(AlFld(asig, operac|ASIG_CLIENTE)), IFld(AlFld(asig, operac|ASIG_OBJETIVO)));
			}
//			if (efectivo && estaefec && (StrCmp(FmSFld(fm, I_WIEFECT, row), FmSFld(fm, EFECT, row)))) {

			if (efectivo && estaefec && !FmIFld(fm, I_BASE, row)) {
				if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, WAR_EFECT, LFld(AlFld(asig, operac|ASIG_CLIENTE)),
								IFld(AlFld(asig, operac|ASIG_OBJETIVO))) == WD_YES) {
					FmSetIFld(fm0, I_BASE, TRUE, row);
					fm4 = UseSubform(fm, NROLEG, 0, row);
					CargarOtrasAsignaciones(fm4, FmLFld(fm, NROLEG, row));
					FmSetIFld(fm, I_CHGEFEC0, TRUE, row);
					return FM_OK;
				}
				else
					return FmErrMsg(fm0, M_ERR_EFECT, LFld(AlFld(asig, operac|ASIG_CLIENTE)),
													  IFld(AlFld(asig, operac|ASIG_OBJETIVO)));
			}
			// Lo saque porque si volvia para atras y pasaba de nuevo por este agrup entraba al else,
			//   me seteaba I_CHGEFEC0 con false, y luego generaba doble asignacion efectiva.
		 //	else {
		 //		FmSetIFld(fm, I_CHGEFEC0, FALSE, row);
		 //	} 
		}
**********************/
		if (!efectivo && !estaefec)
			return FmErrMsg(fm0, M_NO_ESTA_EFEC);
		break;
	case HORARIO :
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, row), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			if (FmLFld(fm0, CLIEOT) == LFld(AlFld(asig, operac|ASIG_CLIENTE)) &&
				FmIFld(fm0, OBJET)  == IFld(AlFld(asig, operac|ASIG_OBJETIVO)))
				continue;
			if (!StrCmp(SFld(AlFld(asig, operac|ASIG_EFECT)), EFECTIVO))
				FmSetDFld(fm, I_FECEFEC, DFld(AlFld(asig, operac|ASIG_FECASIG)), row);
		}
		break;
	}
	return FM_OK;
}

static fm_status before_asigh1(form fm, fmfield fno, int row)
{
	char regimen[18];

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
				GetRegimenEfectivo(FmIFld(fm0, EMP), FmLFld(fm, I_NROLEG), regimen, FmDFld(fm, FECHA));
				if (!StrCmp(regimen, NULL_STR))
					GetRegimenEfectivoFm(FmIFld(fm0, EMP), FmLFld(fm, I_NROLEG), regimen, FmDFld(fm, FECHA));
				FmSetFld(fm, REGIM, regimen);
			}
		}
		return FM_SKIP;
	 case FRANCERO:
		if (StrToI(ReadEnv("PAIS")) == PAIS_CHI)
			return FM_SKIP;
		break;
	case NUMFRAN:
		if (StrToI(ReadEnv("PAIS")) == PAIS_CHI)
			 return FM_SKIP;
		if (GetDiasFranco(FmSFld(fm, REGIM), FmIsNull(fm, HORAPT)? FALSE : TRUE) == 1)
			return FM_SKIP;
		break;
	case FECHA :
		fecasig = FmDFld(fm, fno);
		break;
	case HORENT:
		horsal = FmTFld(fm, I_HORSAL);
		horent = FmTFld(fm, I_HORENT);
		break;
	case HORSAL:
		horsal = FmTFld(fm, I_HORSAL);
		horent = FmTFld(fm, I_HORENT);
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

static int validPuesto()
{
	if (IFld(operac|PUESTOS_CANTVIG) != 0.0 &&
	   ((IsNull(operac|PUESTOS_FFINAL)  && !IsNull(operac|PUESTOS_FINICIO) &&
		FmDFld(fm1, FECHA) < DFld(operac|PUESTOS_FINICIO)) ||
	   (!IsNull(operac|PUESTOS_FFINAL) &&
	   (FmDFld(fm1, FECHA) < DFld(operac|PUESTOS_FINICIO) || FmDFld(fm1, FECHA) > DFld(operac|PUESTOS_FFINAL))))) {
		return FALSE;
	}
	if (IFld(operac|PUESTOS_CANTVIG) == 0.0 && !IsNull(operac|PUESTOS_FFINAL) &&
		(FmDFld(fm1, FECHA) > DFld(operac|PUESTOS_FFINAL) || FmDFld(fm1, FECHA) < DFld(operac|PUESTOS_FINICIO))) {
		return FALSE;
	}
	if (IFld(operac|PUESTOS_CANTVIG) == 0.0 && IsNull(operac|PUESTOS_FFINAL) &&
		FmDFld(fm1, FECHA) > DFld(operac|PUESTOS_FINICIO)) {
		return FALSE;
	}
/*	if (!IsNull(operac|PUESTOS_FFINAL) && FmDFld(fm1, FECHA) > DFld(operac|PUESTOS_FFINAL))
		return FALSE;
	if (IFld(operac|PUESTOS_CANTVIG) == 0 && FmDFld(fm1, FECHA) > DFld(operac|PUESTOS_FFINAL))
		return FALSE;
*/
	// Verifica según el tipo de vigilador que es V, P, R, que sea de ese tipo de puesto part-time u otros.
	if (strcmp(t_vigil, "P") == 0 && strcmp(t_efect, "E") == 0) {
		if (IsNull(operac|PUESTOS_CODFREC) && IsNull(operac|PUESTOS_HORAPT))
			return FALSE;
	}
	if (strcmp(t_vigil, "V") == 0 && strcmp(t_efect, "E") == 0) {
		if (!IsNull(operac|PUESTOS_CODFREC) || !IsNull(operac|PUESTOS_HORAPT))
			return FALSE;
	}
	return TRUE;
}

static void displayPuesto(char * buffer)
{
	sprintf(buffer,"%4.4d %4.4d %.1T %.1T %1.1s %1.1s %1.1s %1.1s %1.1s %1.1s %1.1s %-8.8s %1.1s %.1D %.1D",
					IFld(operac|PUESTOS_CODINT),  IFld(operac|PUESTOS_PUESTO), TFld(operac|PUESTOS_HINICIO),
					TFld(operac|PUESTOS_HFINAL),  SFld(operac|PUESTOS_DIA1),   SFld(operac|PUESTOS_DIA2),
					SFld(operac|PUESTOS_DIA3),    SFld(operac|PUESTOS_DIA4),   SFld(operac|PUESTOS_DIA5),
					SFld(operac|PUESTOS_DIA6), 	  SFld(operac|PUESTOS_DIA7),   SFld(operac|PUESTOS_REGIM),
					SFld(operac|PUESTOS_TIPODIA), DFld(operac|PUESTOS_FINICIO), DFld(operac|PUESTOS_FFINAL));
}

static fm_status after_asigh1(form fm, fmfield fno, int row)
{
	DATE fecmax, fecha;
	int  n;

	bool puestopt, dia;

	switch (fno) {
	case AGRTOT:
		if (!FmIFld(fm, I_MODIF) && FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES)) {
			FmSetDFld(fm, FECHA, FmDFld(fm, I_FCHDES));
			return FmErrMsg(fm, M_MODIFIC);
		}
		break;
	case FECHA :
		SetIFld(sue|DATPERS_EMP,    FmIFld(fm0, EMP));
		SetLFld(sue|DATPERS_NROLEG, FmLFld(fm, I_NROLEG));
		GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (FmDFld(fm, FECHA) < DFld(sue|DATPERS_FECANT))
			return FmErrMsg(fm, M_FECING, DFld(sue|DATPERS_FECANT));

		fm3 = UseSubform(fm, FECHA, 0, 0);

		if (FmDFld(fm, FECHA) != FmDFld(fm, I_FCHDES) && ExisteParteCargadoI(FmLFld(fm, I_CLIEOT),
						FmIFld(fm, I_OBJET), FmLFld(fm,  I_NROLEG), FmDFld(fm, FECHA), FmDFld(fm3, FECHAS))) {
			FmSetIFld(fm, I_EXISPAR, TRUE);
			return FmErrMsg(fm, M_PARTE);
		}
		if (FmIFld(fm, I_CHGEFEC)) {
			if (!ValidarPartesProvisorios(FmDFld(fm, FECHA), fm)) {
				FmSetFld(fm, FECHA, FmFldPrev(fm));
				return FmErrMsg(fm, M_CHGEFEC);
			}
		}
/*		if (FmDFld(fm, fno) < FmDFld(fm, I_FINICIO) || 
			(!FmIsNull(fm, I_FFINAL) && FmDFld(fm, fno) > FmDFld(fm, I_FFINAL)))
			return FmErrMsg(fm, M_FECASIG, FmDFld(fm, I_FINICIO), FmDFld(fm, I_FFINAL));
*/
		if (!StrCmp(FmSFld(fm, I_EFECT), PROVISORIO)) {
			if (FmDFld(fm, FECHA) < FmDFld(fm, I_FECEFEC1))
				return FmErrMsg(fm, M_FECASIGE, FmDFld(fm, I_FECEFEC1));

		}

		DoSubform(fm, NULLFP, after_asigh3, fno, 0);
		if (FmIsNull(fm3, FECHAS))
			return FM_ERROR;

		if (YaTieneAsignacion(FmIFld(fm0, EMP), FmLFld(fm, I_NROLEG), FmDFld(fm3, FECHAS))){
			return FM_REDO;
		}

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
		break;
	case CODINT :
		if (FmKeyCode(fm) == K_HELP) {
			SetCursorFrom(c_puestos, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO), MIN_SHORT);
			SetCursorTo  (c_puestos, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm, TIPPTO), MAX_SHORT);

			strcpy(t_vigil, FmSFld(fm, I_VIGILAD));
			strcpy(t_efect, FmSFld(fm, I_EFECT));
			n = PopUpDbMenu(10, 70, " Puestos de Trabajo ", c_puestos, 4, validPuesto, displayPuesto);
			if (n >= 0) {
				if (FmIFld(fm, fno) != IFld(operac|PUESTOS_CODINT)) {
					FmSetIFld(fm, fno, IFld(operac|PUESTOS_CODINT));
					FmClearFlds(fm, HORAPT, HORSAL);
					FmShowFlds (fm, HORAPT, HORSAL);
				}
			}
		}
		else {
			if (!FmIsNull(fm, CODINT)) {
				SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET),
												FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));
				if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
					WiMsg("El Puesto no es uno existente para el cliente/objetivo");
					return FM_ERROR;
				}
			}
		}
		if ((!IsNull(operac|PUESTOS_FFINAL) && FmDFld(fm, FECHA) > DFld(operac|PUESTOS_FFINAL)) ||
			FmDFld(fm, FECHA) < DFld(operac|PUESTOS_FINICIO)) {
			(void) FmErrMsg(fm, M_FECASIG, DFld(operac|PUESTOS_FINICIO), DFld(operac|PUESTOS_FFINAL));
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
			FmSetIFld(fm, I_NROINT, CalcNroint(FmIFld(fm, I_EMP), FmLFld(fm, I_CLIEOT), FmIFld(fm, I_OBJET),
											   FmLFld(fm, I_NROLEG), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT)));

		if (codint != NULL_SHORT && codint != FmIFld(fm, CODINT)){
			FmSetIFld(fm, I_MODIF, TRUE);
		}

		break;
	case AGRPUESTO :
		puestopt = PuestoEsPartime(FmLFld(fm, I_CLIEOT), FmIFld(fm, I_OBJET), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT));

		if (!StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) && !puestopt && !StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)) {
			return FmErrMsg(fm, M_NOPTIME);
		}
		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) && puestopt && !StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)) {
			return FmErrMsg(fm, M_SIPTIME);
		}
		// lo puse aca y no en el after de CONINT para que salte el warning cuando el puesto esta bien ingresado
		if (FmIFld(fm, I_CODCATE) != FmIFld(fm, I_CATEG)) {
			Warning("El Vigilador tiene categoria %ld.\n Diferente a la categoria %d que necesita el puesto.",
					FmIFld(fm, I_CODCATE), FmIFld(fm, I_CATEG));
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

		/******* como son asignaciones viejas cantvif = 0 
		if (pue[FmIFld(fm, TIPPTO)][FmIFld(fm, CODINT)] > FmIFld(fm, I_CANTVIG)) {
			if (!StrCmp(FmSFld(fm, I_EFECT), EFECTIVO)) {
				Warning(WAR_OCUP_EFECT);
//				FmNextFld(fm, TIPPTO);
			}
			// si el error me dio para una asignación de provisorios y
			// hay alguna de los efectivos de vacaciones o con licencia tudu bom!
			if (StrCmp(FmSFld(fm, I_EFECT), EFECTIVO) && !FmIFld(fm, I_EXISTE) &&
				!HayVacante(FmLFld(fm, I_NROLEG), FmIFld(fm, TIPPTO), FmIFld(fm, CODINT),
							FmDFld(fm, FECHA), FmDFld(fm3, FECHAS), &fechocup)) {
				Warning(WAR_OCUP, fechocup);
//				FmNextFld(fm, TIPPTO);
//				return FM_REDO;
			}
		}
		**********/
		break;
	case FECFRA :
		if (FmChgFld(fm)) {
			FmSetIFld(fm, I_MODIF, TRUE);
		}
		if (!FmIsNull(fm, fno) && FmDFld(fm, fno) < FmDFld(fm, FECHA))
			return FmErrMsg(fm, M_FECHA, FmDFld(fm, FECHA));
		if (FmDFld(fm, fno) < FmDFld(fm, I_FINICIO) ||
			(!FmIsNull(fm, I_FFINAL) && FmDFld(fm, fno) > FmDFld(fm, I_FFINAL))) {
			return FmErrMsg(fm, M_FECFRA, FmDFld(fm, I_FINICIO), FmDFld(fm, I_FFINAL));
		}
		if (MenorAMinFecFranco(FmDFld(fm, FECHA), FmSFld(fm, I_VIGILAD), FmDFld(fm, fno), FmSFld(fm, REGIM),
							   &fecmax)) {
			return FmErrMsg(fm, M_DLAB, fecmax);
		}
		break;
	case HORENT :
		dia = FALSE;

		if (horsal > horent)
			dia = TRUE;

		if (FmTFld(fm, HORENT) < horent && dia)
			return FmErrMsg(fm, M_HRENT, horent);

		if (FmTFld(fm, HORENT) > horsal && dia)
			return FmErrMsg(fm, M_HRENT, horent);
		if (FmTFld(fm, HORENT) < horent && FmTFld(fm, HORENT) > horsal)
			return FmErrMsg(fm, M_HRENT, horent);
		break;
	case HORSAL :
		dia = FALSE;
//		fm1 = UseSubform(fm0, HORARIO, 0, row_padre);
		if (horsal > horent)
			dia = TRUE;

		if (FmTFld(fm, HORSAL) > horsal && dia)
			return FmErrMsg(fm, M_HRSAL, horsal);
		if (FmTFld(fm, HORENT) > FmTFld(fm, HORSAL) && FmTFld(fm, HORSAL) > horsal)
			return FmErrMsg(fm, M_HRSAL, horsal);
		if (FmTFld(fm, HORENT) < horent && FmTFld(fm, HORSAL) > horsal)
			return FmErrMsg(fm, M_HRSAL, horsal);
		if (horsal != FmTFld(fm, HORSAL))
			FmSetIFld(fm, I_MODIF, TRUE);

		fecha = NULL_DATE;
		if (!StrCmp(FmSFld(fm, I_EFECT), PROVISORIO)) {
			fm3   = UseSubform(fm, FECHA, 0);
			fecha = FmDFld(fm3, FECHAS);
		}

//VER. Tendria que validar cuando es provisorio (lo hace) y cuando cambia de cliente efectivo (no lo hace),
//     pero no cuando es efectivo  por 1er vez (lo hace). Tendría que sacar if (!FmIFld(fm, I_CHGEFEC)).
//		if (!FmIFld(fm, I_CHGEFEC)) {
			if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) != 0) {
				// Solo ejecuta la funcion ValidPuestoOcup cuando el objetivo no es el 1000-6 (Prosegur) Y
				// el vigilator no es reten. Esto significa que si el vigilador es reten y esta siendo asignado
				// al objetivo 1000-6 (Prosegur) NO se debe validar superposicion.

				if (!(!StrCmp(FmSFld(fm, I_VIGILAD), RETEN) &&
					 FmLFld(fm, I_CLIEOT) == CLIPROS &&	FmIFld(fm, I_OBJET) == OBJPROS) &&
					StrCmp(TipoDia(FmLFld(fm, I_CLIEOT), FmIFld(fm, I_OBJET), FmIFld(fm, TIPPTO),
						   FmIFld(fm, CODINT)), "F") &&
					!ValidPuestoOcup(FmLFld(fm, I_NROLEG), FmDFld(fm, FECHA), FmTFld(fm, HORENT),
								FmTFld(fm, HORSAL), FmSFld(fm, DIA1), FmSFld(fm, DIA2), FmSFld(fm, DIA3),
								FmSFld(fm, DIA4), FmSFld(fm, DIA5), FmSFld(fm, DIA6), FmSFld(fm, DIA7),
								FmDFld(fm, I_FECHAS),
								row_padre, FmLFld(fm0, I_CLIEFEC, row_padre), FmIFld(fm0, I_OBJEFEC, row_padre),
								FmIFld(fm0, I_PTOEFEC, row_padre), FmIFld(fm0, I_CODINTEFEC, row_padre),
								FmSFld(fm, I_EFECT))) {
					// como queda el último registro corriente, tomo esos datos.
					Warning(ERR_SUPERPOS, FmSFld(fm, I_NOMBRE), asigSt.cliente, asigSt.objetivo, 
																 asigSt.fecasig, asigSt.dia1, asigSt.dia2,
																 asigSt.dia3, asigSt.dia4, asigSt.dia5,
																 asigSt.dia6, asigSt.dia7, asigSt.hsent,
																 asigSt.hssal);
					FmNextFld(fm, TIPPTO);
				}
			}
//		}
		if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) == 0) {
			if (StrCmp(FmSFld(fm, CODFREC), SEMANAL) == 0) {
				GenerarDiasPTSemanal(fm);
			}
		}
		break;
	case SUBFPTIME :
		if (StrCmp(FmSFld(fm,  I_VIGILAD), PARTTIME) == 0)
			DoSubform(fm, before_asigh5, after_asigh5, fno, 0, row);

		if (!FmIFld(fm, I_CHGEFEC)) {
			if (StrCmp(FmSFld(fm, I_VIGILAD), PARTTIME) == 0) {
				fm5 = UseSubform(fm, SUBFPTIME, 0);
				if (!ValidPartTime(fm5) || !ValidPartTimeFm(fm5, row_padre, FmLFld(fm, I_NROLEG)) ||
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
			if (DiaFranco(FmIFld(fm0, EMP), FmLFld(fm, I_NROLEG), FmDFld(fm, FECHA), FmDFld(fm, I_FECHAS), FmSFld(fm, fno)))
				Warning("El Vigilador %ld tiene franco ese día.", FmLFld(fm, I_NROLEG));
		}
		if (FmChgFld(fm))
			FmSetIFld(fm, I_MODIF, TRUE);
		break;
	case FRANCERO:
		if(!StrCmp(FmSFld(fm, REGIM), REG_ESP) && FmIsNull(fm, fno)){
			WiMsg("Campo obligatorio");
			return FM_REDO;
		}
		break;
	case NUMFRAN:
		if(GetDiasFranco(FmSFld(fm, REGIM), FALSE) > 1 && FmIsNull(fm, fno)) {
			WiMsg("Campo obligatorio");
			return FM_REDO;
		}
		break;
	case MOTIVO1:
		fm6 = UseSubform(fm, MOTIVO1, 0, 0);
		if (FmIFld(fm, I_MODIF) && FmIFld(fm, I_EXISTE)) {
			FmSetDFld(fm6, FECBAJ6, FmDFld(fm, FECHA) - 1);
			FmSetDFld(fm6, I_FECASIG26, FmDFld(fm, FECHA));
			DoSubform(fm, NULLFP, after_asigh6, fno, 0);

			if (FmIsNull(fm6, MOTIVO6))
				return FM_ERROR;
		}
		break;
	}
	return FM_OK;
}

static fm_status before_asigh5(form fm, fmfield fno, int row)
{
	form fmpadre;
	fmpadre = FmFather(fm);
/*
	if (fno == MULTDIAS && FmDFld(fm, I_FPROVORI1) == FmDFld(fm, I_FECHAH)) {
		if (FmIFld(fmpadre, I_EXISPAR) || (StrCmp(FmSFld(fmpadre, I_EFECT), PROVISORIO) == 0 &&
												  FmIFld(fmpadre, I_EXISTE)))
			FmSetDisplayOnly(fm, MULTDIAS, MULTDIAS, TRUE);
	}
*/
	return FM_OK;
}

static fm_status after_asigh5(form fm, fmfield fno, int row)
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
//		if (FmChgFld(fm) && StrCmp(FmFldPrev(fm), NULL_STR) != 0)
//			FmSetIFld(fmpadre, I_MODIF, TRUE);

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
				else
					return FmErrMsg(fmpadre, M_DIA);
			}
		}
		break;
	case HENT:
	case HSAL:
//		if (FmChgFld(fm) && StrCmp(FmFldPrev(fm), NULL_STR) != 0 ) // && FmIFld(fmpadre, I_EXISTE)) 
//			FmSetIFld(fmpadre, I_MODIF, TRUE);
		break;
	case MULTDIAS :
		if (!FmIsNull(fmpadre, CODFREC) && StrCmp(FmSFld(fmpadre, CODFREC), SEMANAL) != 0) {
			if (!ValidaFrecuencia(fm, FmDFld(fmpadre, FECHA), FmDFld(fm3, FECHAS), FmSFld(fmpadre, CODFREC),
								  FmLFld(fmpadre, I_NROLEG), FmIFld(fmpadre, TIPPTO), FmIFld(fmpadre, CODINT),
								  FmIFld(fmpadre, I_NROINT))) {
				DisplayMSGERR();
				FmNextFld(fm, DIA, 0);
				break;
			}
		}
		fm3 = UseSubform(fmpadre, FECHA, 0, 0);
		CargarHsPT(fm);

		if ((!FmIFld(fmpadre, I_CHGEFEC) && 
			(!ValidPartTime(fm) || !ValidPartTimeFm(fm, row_padre, FmLFld(fm, I_NROLEGA)))) || !ValidHsPT()) {
			DisplayMSGERR();
			FmNextFld(fm, DIA, row);
			break;
		}
		if (!FmIsNull(fmpadre, HORAPT))
			if (ValidarSumaMeses(fm)) {
				for (i = 0; i < MAX_MESES && i < topmes; i++)
				strcat(buff, meses[i].error);
				Warning("%s", buff);
			}
		break;
	}
	return FM_OK;
}

static fm_status after_asigh2(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FECBAJ :
		if (FmDFld(fm, fno) < FmDFld(fm, I_FECHINI) ||
			(!FmIsNull(fm, I_FECHFIN) && FmDFld(fm, fno) > FmDFld(fm, I_FECHFIN))) {
			return FmErrMsg(fm, M_FECBAJ, FmDFld(fm, I_FECHINI), FmDFld(fm, I_FECHFIN));
		}
		break;
	case REEMPL:
		if (FmIFld(fm, I_ACT2) == 0) {
			return FmErrMsg(fm, M_LEGINACT2);
		}
		break;
	}
	return FM_OK;
}

static fm_status after_asigh6(form fm, fmfield fno, int row) 
{
	switch (fno) {
	case FECBAJ6 :
		if (FmDFld(fm, fno) < FmDFld(fm, I_FECHINI6) ||
			(!FmIsNull(fm, I_FECHFIN6) && FmDFld(fm, fno) > FmDFld(fm, I_FECHFIN6))) {
			return FmErrMsg(fm, M_FECBAJ6, FmDFld(fm, I_FECHINI6), FmDFld(fm, I_FECHFIN6));
		}
		if (!FmIsNull(fm, fno) && 
			ExisteParteCargadoI(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm0, NROLEG, row_padre),
							   FmDFld(fm, fno) + 1, NULL_DATE))
			return FmErrMsg(fm1, M_PARTE);
		break;
	case REEMPL:
		if (FmIFld(fm, I_ACT26) == 0) {
			return FmErrMsg(fm, M_LEGINACT26);
		}
		break;
	}
	return FM_OK;
}

static fm_status after_asigh3(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FECHAS :
		if (!FmIsNull(fm, fno) && FmDFld(fm, fno) < FmDFld(fm, I_FECDESDE)) {
			return FmErrMsg(fm, M_FECHAS, FmDFld(fm, I_FECDESDE));
		}
		if (FmDFld(fm, fno) < FmDFld(fm, I_FFINICIO) ||
			(!FmIsNull(fm, I_FFFINAL) && FmDFld(fm, fno) > FmDFld(fm, I_FFFINAL))) {
			return FmErrMsg(fm, M_FECHAS2, FmDFld(fm, I_FFINICIO), FmDFld(fm, I_FFFINAL));
		}
		if (!FmIsNull(fm, fno) && 
			ExisteParteCargadoI(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmLFld(fm0, NROLEG, row_padre),
							   FmDFld(fm, fno) + 1, NULL_DATE))
			return FmErrMsg(fm1, M_PARTE);
		break;
	}
	return FM_OK;
}

// Valida superposicion de horarios para vigilators que no son Part-Time.
// Debe Controlar tanto contra ASIG como ASIGH, por si se hizo una asignación adelantada.
static bool ValidPuestoOcup(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
							char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, int fila,
							long cliefec, int objefec, int ptoefec, int codintefec, char * efect)
{
	int i; 
	form fm11, fm21,fm61, fm31;

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), legajo, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), legajo, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {

/* Lo saque porque si estaba asignado solo en el efectivo no me daba superposicion. OJO que puede pinchar
   en otro caso (no se cual puede ser). Hay que verlo cuando se produzca. 
   (Ya se produjo un caso: (Y volvi a ponerlo)
   esta Efectivo y lo quiero asignar Efectivo en otro lado (Ya lo arregle))
*/
		if (LFld(AlFld(asig, operac|ASIG_CLIENTE))  == cliefec    &&
			IFld(AlFld(asig, operac|ASIG_OBJETIVO)) == objefec    &&
			IFld(AlFld(asig, operac|ASIG_PTOSER))   == ptoefec    &&
			IFld(AlFld(asig, operac|ASIG_PUESTO))   == codintefec &&
			!strcmp(efect, EFECTIVO)) {
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
		if (!ValidoUnAsig(legajo, fecasig, horent, horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, FALSE)) {
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
		if (!ValidoUnAsig(legajo, fecasig, horent, horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, TRUE)) {
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
//		fm41 = UseSubform(fm0, NROLEG,  0, i); FmDFld(fm41, I_FECHASTA, 0)

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

		if (!ValidoUnAsigFm(legajo, fecasig, horent, horsal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, fechas, FALSE)) {
			return FALSE;
		}
	}
	return TRUE;
}

static bool ValidoUnAsig(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
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
		if (FmLFld(fm0, CLIEOT) == asigSt.cliente && FmLFld(fm0, OBJET) == asigSt.objetivo) {
			return TRUE;
		}
	if (asigh) {
		// si la asignación cargada en asigh con fecha de baja anterior
		// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
		if (asigSt.fecbaj < fecasig) {
			return TRUE;
		}
	}
	// si la asignación cargada en asig era provisoria con fecha de fin anterior
	// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
	if (asigSt.fechas != NULL_DATE && asigSt.fechas < fecasig) {
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
	if (!diastrab) {
		return TRUE;
	}
	// si los dias se superponen pero las horas de trabajo no se superponen, todo bien!
	if (diastrab && !Superposicion(horent, horsal, asigSt.hsent, asigSt.hssal, FALSE)) {
		return TRUE;
	}
	
	// se detecto superposicion => devuelve FALSE:
	return FALSE;
}

static bool ValidoUnAsigFm(long legajo, DATE fecasig, TIME horent, TIME horsal, char *dia1, char *dia2,
						 char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, DATE fechas, bool asigh) 
{
	bool diastrab;

	if (asigh) {
		// si la asignación cargada en asigh con fecha de baja anterior
		// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
		if (asigSt.fecbaj < fecasig)
			return TRUE;
	}
	// si la asignación cargada en asig era provisoria con fecha de fin anterior
	// a la fecha de asignación de esta nueva asignacion, no hay superposicion:
	if (asigSt.fechas != NULL_DATE && asigSt.fechas < fecasig) {
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
	if (diastrab && !Superposicion(horent, horsal, asigSt.hsent, asigSt.hssal, FALSE))
		return TRUE;

	// se detecto superposicion => devuelve FALSE:
	return FALSE;
}

static fm_status CargarDiasPTime(long nroleg, int ptoser, int puesto, int nroint, form fm)
{
	int  i = 0;
	char fchstr[3];

	fm5 = UseSubform(fm, SUBFPTIME, 0, 0);

	SetCursorFrom(c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, ptoser,
							puesto, nroint, Today());
	SetCursorTo  (c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, ptoser,
							puesto, nroint, MAX_DATE);
	while (i < FmFldLen(fm5, MULTDIAS) && FetchCursor(c_diaspt) != ERROR) {
		FmSetDFld(fm5, DIA,    DFld(operac|DIASPTIME_DIA),  i);
		FmSetTFld(fm5, HENT,   TFld(operac|DIASPTIME_HENT), i);
		FmSetTFld(fm5, HSAL,   TFld(operac|DIASPTIME_HSAL), i);
		FmSetIFld(fm5, HSXDIA, (GetCantHoras(FmTFld(fm5, HENT, i), FmTFld(fm5, HSAL, i))), i);
		fchstr[0] = dia(FmDFld(fm5, DIA, i));
		strcpy(&fchstr[1], NULL_STR);
		FmSetFld(fm5, I_DIASTR, &fchstr[0], i);
		i++;
	}
	FmSetIFld(fm5, I_CANTMULT, i);

	return FM_OK;
}

static fm_status MostrarAsig(form fm, fmfield fno, int row)
{
	fm4 = UseSubform(fm0, NROLEG, 0, row);

	CargarOtrasAsignaciones(fm4, FmLFld(fm0, NROLEG, row));
	DoSubform(fm0, NULLFP, NULLFP, fno, 0, row);

	return FM_OK;

}

private void CargarOtrasAsignaciones(form fm4, long nroleg)
{
	int i = 0;

	SetCursorFrom(c_aasig, FmIFld(fm0, EMP), nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_aasig, FmIFld(fm0, EMP), nroleg, MAX_LONG, MAX_SHORT);
	while (i < FmFldLen(fm4, MULTI) && FetchCursor(c_aasig) != ERROR) {
/*		if (FmLFld(fm0, CLIEOT) == LFld(AlFld(aasig, operac|ASIG_CLIENTE)) &&
			FmIFld(fm0, OBJET)  == IFld(AlFld(aasig, operac|ASIG_OBJETIVO)))
			continue;
*/
		LeerCliente (LFld(AlFld(aasig, operac|ASIG_CLIENTE)));
		LeerObjetivo(LFld(AlFld(aasig, operac|ASIG_CLIENTE)), IFld(AlFld(aasig, operac|ASIG_OBJETIVO)));
		FmSetLFld(fm4, CLI,        LFld(AlFld(aasig, operac|ASIG_CLIENTE)), i);
		FmSetFld (fm4, DCLI,       SFld(bill|CLIENTE_RAZSOC),               i);
		FmSetIFld(fm4, OBJ,        IFld(AlFld(aasig, operac|ASIG_OBJETIVO)),i);
		FmSetFld (fm4, DOBJ,       SFld(comerc|OBJETIVO_DESCRIP),           i);
		FmSetDFld(fm4, FASIG,      DFld(AlFld(aasig, operac|ASIG_FECASIG)), i);
		FmSetDFld(fm4, I_FECHASTA, DFld(AlFld(aasig, operac|ASIG_FECHAS)),  i);
		FmSetFld (fm4, I_VIGIL4,   SFld(AlFld(aasig, operac|ASIG_EFECT)),   i);
		FmSetIFld(fm4, I_TIPPTO,   IFld(AlFld(aasig, operac|ASIG_PTOSER)),  i);
		FmSetIFld(fm4, I_CODINT,   IFld(AlFld(aasig, operac|ASIG_PUESTO)),  i);
		FmSetIFld(fm4, I_NUMINT,   IFld(AlFld(aasig, operac|ASIG_NROINT)),  i);
		FmSetFld (fm4, ADIA1,      SFld(AlFld(aasig, operac|ASIG_DIA1)),    i);
		FmSetFld (fm4, ADIA2,      SFld(AlFld(aasig, operac|ASIG_DIA2)),    i);
		FmSetFld (fm4, ADIA3,      SFld(AlFld(aasig, operac|ASIG_DIA3)),    i);
		FmSetFld (fm4, ADIA4,      SFld(AlFld(aasig, operac|ASIG_DIA4)),    i);
		FmSetFld (fm4, ADIA5,      SFld(AlFld(aasig, operac|ASIG_DIA5)),    i);
		FmSetFld (fm4, ADIA6,      SFld(AlFld(aasig, operac|ASIG_DIA6)),    i);
		FmSetFld (fm4, ADIA7,      SFld(AlFld(aasig, operac|ASIG_DIA7)),    i);
		FmSetTFld(fm4, HORAENT,    TFld(AlFld(aasig, operac|ASIG_HSENT)),   i);
		FmSetTFld(fm4, HORASAL,    TFld(AlFld(aasig, operac|ASIG_HSSAL)),   i);
		i++;
	}
}

static void LeerCliente (long cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		Warning(CLI_INEX, cliente);
}

static void LeerObjetivo(long cliente, int objetivo)
{
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		Warning(OBJET_INEX, cliente, objetivo);
}

static bool ValInfo()
{
	int i;
	bool msgerr;
	char dia1[2], dia2[2], dia3[2], dia4[2], dia5[2], dia6[2], dia7[2];

	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		fm1 = UseSubform(fm0, HORARIO, 0, i);
		fm3 = UseSubform(fm1, FECHA, 0);
		if (FmIsNull(fm1, TIPPTO)) {
			Warning(ERR_FALTAN, FmLFld(fm0, NROLEG, i), FmSFld(fm0, NOMBRE, i));
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
						ExisteCliObjEnGrp(GRPRETPLANTA, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET))) &&
						StrCmp(TipoDia(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), FmIFld(fm1, TIPPTO),
							   FmIFld(fm1, CODINT)), "F") &&
						!ValidPuestoOcup(FmLFld(fm0, NROLEG, i), FmDFld(fm1, FECHA), FmTFld(fm1, HORENT),
										FmTFld(fm1, HORSAL), FmSFld(fm1, DIA1), FmSFld(fm1, DIA2),
										FmSFld(fm1, DIA3), FmSFld(fm1, DIA4), FmSFld(fm1, DIA5),
										FmSFld(fm1, DIA6), FmSFld(fm1, DIA7),
										FmDFld(fm3, FECHAS), 
										i, FmLFld(fm0, I_CLIEFEC, i), FmIFld(fm0, I_OBJEFEC, i),
										FmIFld(fm0, I_PTOEFEC, i), FmIFld(fm0, I_CODINTEFEC, i),
										FmSFld(fm0, EFECT, i))) {
						actualizar = FALSE;
						// como queda el último registro corriente, tomo esos datos.
						Warning(ERR_SUPERPOS, FmSFld(fm0, NOMBRE, i), asigSt.cliente, asigSt.objetivo,
																asigSt.fecasig, asigSt.dia1,
																asigSt.dia2, asigSt.dia3, asigSt.dia4,
																asigSt.dia5, asigSt.dia6, asigSt.dia7,
																asigSt.hsent, asigSt.hssal);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

static bool HayVacante(long nroleg, int tippto, int codint, DATE fdesde, DATE fhasta, DATE *fechocup)
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
			if (FmIFld(fm, TIPPTO) == tippto && FmIFld(fm, CODINT) == codint && FmLFld(fm, I_NROLEG) != nroleg) {
				if (Vacaciones(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha) ||
					TieneLic  (FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha) ||
					Falto     (FmIFld(fm0, EMP), FmLFld(fm0, NROLEG, i), fecha)) {
					hayvac = TRUE;
				}
			}
		}
	}
//	(*fechocup) = fecha - 1;
	(*fechocup) = fecha;
	RestauraSubForm( fm0, HORARIO, 0, row_padre);
	return hayvac;
}

static bool TieneProvi(long nroleg)
{
	int cant = 0;

	SetCursorFrom(c_aasig, FmIFld(fm0, EMP), nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_aasig, FmIFld(fm0, EMP), nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_aasig) != ERROR) {
		if (LFld(AlFld(aasig, operac|ASIG_CLIENTE))  == FmLFld(fm0, CLIEOT) &&
			LFld(AlFld(aasig, operac|ASIG_OBJETIVO)) == FmLFld(fm0, OBJET)) {
			cant++;
//			continue;
		}
		else 
			return TRUE;
		if (cant > 1)
			return TRUE;
	}
	return FALSE;
}

static bool ValidarSumaMeses(form fm)
{
	int    i, j;
	double canth;
	bool   error;

	topmes = 0;
	for (i = 0; i < MAX_MESES; i++) {
		meses[i].mes = 0;
		meses[i].horas = 0;
		strcpy(meses[i].error, NULL_STR);
	}
	for (i = 0; i < FmFldLen(fm, MULTDIAS) && !FmIsNull(fm, DIA, i); i++) {
		for (j = 0; j < 5 && j < topmes; j++) {
			if (meses[j].mes   == Month(FmDFld(fm, DIA, i))) {
				meses[j].horas += FmLFld(fm, HSXDIA, i);
				break;
			}
		}
		if (j == 5)
			Error("Estructura interna saturada");
		if (j == topmes) {
			meses[j].mes = Month(FmDFld(fm, DIA, i));
			meses[j].horas = FmLFld(fm, HSXDIA, i);
			topmes++;
		}
	}
	canth = FmLFld(fm, CANTH);
	error = FALSE;
	for (i = 0; i < 5 && i < topmes ; i++) {
		if (meses[i].horas != canth) {
			sprintf(meses[i].error, "\nMes %d con horas %.2f distinta a asignada %.2f\n",
					meses[i].mes, meses[i].horas/100, canth/100);
			strcat(meses[i].error, NULL_STR);
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

	InitMSGERR();

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

private bool ValidPartTimeFm(form fmdias, int row_padre, long legajo)
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
			if (j == row_padre)
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
//		RestauraSubForm(fm15, SUBFPTIME, 0, row_padre);
	}
	RestauraSubForm(fm0, HORARIO,   0, row_padre);
	RestauraSubForm(fm1, SUBFPTIME, 0, row_padre);
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
	InitMSGERR();

	for (i = 0; i < FmFldLen(fmdias,  MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {
		SetKey(operac|DIASPTIMEbyDIA, FmIFld(fm0, EMP), FmLFld(fmdias, I_NROLEGA), FmDFld(fmdias, DIA, i),
								NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		while (GetRecord(operac|DIASPTIMEbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			// si es el mismo cliente objetivo es que esta modificandolo o consultandolo.
//			if ((LFld(operac|DIASPTIME_CLIENTE) == FmLFld(fm0, CLIEOT) &&
//				IFld(operac|DIASPTIME_OBJETIVO) == FmIFld(fm0, OBJET)) ||
//				(LFld(operac|DIASPTIME_CLIENTE) == CLIPROS && IFld(operac|DIASPTIME_OBJETIVO) == OBJPROS))

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
		SetKey(operac|DIASPTIMEHbyDIA, FmIFld(fm0, EMP), FmLFld(fmdias, I_NROLEGA), FmDFld(fmdias, DIA, i),
								NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		while (GetRecord(operac|DIASPTIMEHbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			// si es el mismo cliente objetivo es que esta modificandolo o consultandolo.
//			if (((LFld(operac|DIASPTIMEH_CLIENTE) == FmLFld(fm0, CLIEOT) &&
//				IFld(operac|DIASPTIMEH_OBJETIVO) == FmIFld(fm0, OBJET)) ||
//				(LFld(operac|DIASPTIMEH_CLIENTE) == CLIPROS  && LFld(operac|DIASPTIMEH_OBJETIVO) == OBJPROS)) &&
//				FmIFld(fmpadre, I_EXISTE))

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

private void InitMSGERR()
{
	topmerr = 0;
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
//	strcat(buff, NULL_STR);
	Warning("%s", buff);
}

private void GenerarDiasPTSemanal(form fm)
{
	DATE fecha, fchhas;
	int  i=0, j=0;
	char fchstr[3];

	fm3 = UseSubform(fm, FECHA, 0, 0); 
	fm5 = UseSubform(fm, SUBFPTIME, 0, 0);

//	fecha  = i == 0  || i == NULL_SHORT ? (Today() < FmDFld(fm, FECHA) ? FmDFld(fm, FECHA) : Today())
//															:  FmDFld(fm5, DIA, i);

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

	for (i+1 ; fecha < fchhas && i < FmFldLen(fm5,  MULTDIAS); fecha++) {
		fchstr[0] = dia(fecha);
		strcpy(&fchstr[1], NULL_STR);

		if (SeTrabEnPuesto(&fchstr[0], FmSFld(fm, DIA1), FmSFld(fm, DIA2), FmSFld(fm, DIA3), FmSFld(fm, DIA4),
									   FmSFld(fm, DIA5), FmSFld(fm, DIA6), FmSFld(fm, DIA7))) {
			FmSetDFld(fm5, DIA,    fecha,                i);
			FmSetTFld(fm5, HENT,   FmTFld(fm, HORENT),   i);
			FmSetTFld(fm5, HSAL,   FmTFld(fm, HORSAL),   i);
//			FmSetTFld(fm5, HENT,   FmTFld(fm, I_HORENT), i);
//			FmSetTFld(fm5, HSAL,   FmTFld(fm, I_HORSAL), i);
			FmSetIFld(fm5, HSXDIA, (GetCantHoras(FmTFld(fm5, HENT, i), FmTFld(fm5, HSAL, i))), i);
			FmSetFld (fm5, I_DIASTR, &fchstr[0], i++);
		}
		for (j = i; j < FmFldLen(fm5, MULTDIAS) && !FmIsNull(fm5, DIA, j); j++)
			FmClearFlds(fm5, DIA, HSXDIA, j);
	}
}

private bool ValidaFrecuencia(form fmdias, DATE fecha, DATE fechas, char * codfrec, long nroleg, int ptoser,
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
	InitMSGERR();
	// Inicializo con nulos para que quede despues bien ordenado
	for (i = 0 ; i < 7 ; i++)
		for (j = 0 ; j < MAX_FECHAS; j++) {
			dia[i].fecha[j] = NULL_DATE;
			dia[i].ult = 0;
		}
	// Asigno a cada posición del vector dia (0 - L , 1 - M, 2 - X, etc) las fechas que tienen
	// asignadas para cada uno de esos dias en el multirenglon.
/*	for (i = 0; i < FmFldLen(fmdias, MULTDIAS) && !FmIsNull(fmdias, DIA, i); i++) {
		subind = subdia(*FmSFld(fmdias, I_DIASTR, i));
		dia[subind].fecha[dia[subind].ult++] = FmDFld(fmdias, DIA, i);
	}
*/
	//Cargo los dias que estas en la base.
	SetCursorFrom(c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, ptoser,
							puesto, nroint, MIN_DATE);
	SetCursorTo  (c_diaspt, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), nroleg, ptoser,
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
		else {
			if (dia[i].fecha[0] != NULL_DATE) {
				sprintf(error, "El dia %s tiene asignadas fechas para el periodo y no es un dia laborable\n",
							diasub(i));
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
//		else { // Mensual
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

private bool PutErrorMesesFaltantes(DATE fechad, DATE fechah, int dia)
{
	char error[100];
	int  aniod, anioh, k;
	DATE fchd, fchh;

	if (Year(fechad) == Year(fechah)) {
		for (k = Month(fechad); k  <= Month(fechah); k++) {
			sprintf(error, "Falta Asignación para el dia %s en el mes %d/%d\n", diasub(dia), k, Year(fechah));
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
				sprintf(error, "Falta Asignación para el dia %s en el mes %d/%d\n", diasub(dia), k, aniod);
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

private bool ExisteParteCargadoI(long cliente, int objetivo, long nroleg, DATE fechad, DATE fechah)
{
	bool existe = FALSE;

	if (c_parte == NULL)
		c_parte = CreateCursor(operac|PARTEbyLEG, IO_NOT_LOCK);

	SetCursorFrom(c_parte, FmIFld(fm0, EMP), cliente, objetivo, nroleg, fechad);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), cliente, objetivo, nroleg, fechah == NULL_DATE ? MAX_DATE : fechah);

	while (FetchCursor(c_parte) != ERROR) {
		if (IFld(operac|PARTE_HSNOR)  != 0 || IFld(operac|PARTE_HS50) != 0 ||
			IFld(operac|PARTE_HS100F) != 0 || IFld(operac|PARTE_HS100FE) != 0) {
			existe = TRUE;
			break;
		}
	}
	return existe;
}

private void BorrarParteGeneradoI(long cliente, int objetivo, long nroleg, DATE fechad, DATE fechah,
								 DATE fecbaj, bool porfecha, char *efect, int ptoser, int puesto, int nroint)
{
	dbcursor curpar;

	if (!porfecha) {
		curpar = CreateCursor(operac|PARTEbyLEG, IO_EABORT|IO_NOT_LOCK);
		SetCursorFrom(curpar, FmIFld(fm0, EMP), cliente, objetivo, nroleg, fechad);
		SetCursorTo  (curpar, FmIFld(fm0, EMP), cliente, objetivo, nroleg, fechah == NULL_DATE ? MAX_DATE : fechah);
		while (FetchCursor(curpar) != ERROR)
			DelRecord(operac|PARTE);
	}
	else {
		curpar = CreateCursor(operac|PARTEbyEMPLE, IO_EABORT|IO_NOT_LOCK);
		SetCursorFrom(curpar, FmIFld(fm0, EMP), nroleg, fecbaj,   MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (curpar, FmIFld(fm0, EMP), nroleg, MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(curpar) != ERROR) {
			if (*efect == 'P') {
				if (cliente == LFld(operac|PARTE_CLIENTE) && objetivo == IFld(operac|PARTE_OBJETIVO) && 
					ptoser == IFld(operac|PARTE_PTOSER) && puesto == IFld(operac|PARTE_PUESTO) &&
					nroint == IFld(operac|PARTE_NROINT))
					DelRecord(operac|PARTE);
			}
			else
				DelRecord(operac|PARTE);			
		}
	}
	DeleteCursor(curpar);
}

private	void CargarEstructuraAsig(int emp, long cliente, int objetivo, int ptoser, int puesto, int nroint,
			 					  long nroleg, char vigil[], char efect[], DATE fecasig, TIME hsent,
			 					  TIME hssal, char dia1[], char dia2[], char dia3[], char dia4[],
			 					  char dia5[], char dia6[], char dia7[], long reempl, DATE ffranco,
			 					  int numfran, int francero, char regim[], DATE fechas, DATE fecbaj)
{
	asigSt.emp      = emp;
	asigSt.cliente  = cliente;
	asigSt.objetivo = objetivo;
	asigSt.ptoser   =  ptoser;
	asigSt.puesto   = puesto;
	asigSt.nroint   = nroint;
	asigSt.nroleg   = nroleg;
	sprintf(asigSt.vigil, "%s", vigil);
	sprintf(asigSt.efect, "%s", efect);
	asigSt.fecasig  = fecasig;
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
	fm4 = UseSubform(fmpadre, NROLEG, 0, FmIFld(fm, I_ROW0));
	for (i = 0; i  < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, CLI, i) ; i++) {
		if (ExisteParteCargadoI(FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i),
							   FmLFld(fmpadre, NROLEG, FmIFld(fm, I_ROW0)), fecha, NULL_DATE))
			return FALSE;
	}
	for (i = 0; i < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, CLI, i); i++)
		FmSetDFld(fm4, I_FECHASTA, fecha - 1, i);
	return TRUE;
}

private	void PonerEnASIGH(form fm4, long nroleg, DATE fecha, int motivo, long reempl, DATE fecbaj)
{
	int  i;

	BorrarInasistencia(FmIFld(fm0, EMP), nroleg, fecha);

	for (i = 0; i < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, CLI, i) ; i++) {
		SetKey(operac|ASIGbyEMP, FmIFld(fm0, EMP), FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i), nroleg,
					FmIFld(fm4, I_TIPPTO, i), FmIFld(fm4, I_CODINT, i), FmIFld(fm4, I_NUMINT, i));

		if (GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_LOCK) == ERROR)
			Error("No existe el vigilador %ld asignado en el cliente %ld objetivo %d",nroleg, FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i));

		if (!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) > fecha)
			continue;

		BorrarParteGeneradoI(FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i), nroleg, fecha,
							StrCmp(FmSFld(fm4, I_VIGIL4, i), EFECTIVO) == 0 ? 
							NULL_DATE : DFld(operac|ASIG_FECHAS), NULL_DATE, FALSE, NULL_STR,
							MIN_SHORT, MIN_SHORT, MIN_SHORT);

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
		SetDFld(operac|ASIGH_FECHAS,   FmDFld(fm4, I_FECHASTA, i));
		SetDFld(operac|ASIGH_FECBAJ,   DFld(operac|ASIGH_FECHAS));
//		SetIFld(operac|ASIGH_MOTIVO,   MOTIVO_DESASIG_EFEC);
		SetIFld(operac|ASIGH_MOTIVO,   motivo);
		SetLFld(operac|ASIGH_REEMPL,   reempl);
		SetDFld(operac|ASIGH_FFRANCO,  DFld(operac|ASIG_FFRANCO));
		SetIFld(operac|ASIGH_NUMFRAN,  IFld(operac|ASIG_NUMFRAN));
		SetIFld(operac|ASIGH_FRANCERO, IFld(operac|ASIG_FRANCERO));
		PutRecord(operac|ASIGH);
		DelRecord(operac|ASIG);
		FreeTable(operac|ASIGH);
		FreeTable(operac|ASIG);

		//Si es Part-Time paso tambien los DIASPTIME a DIASPTIMEH
//		if (StrCmp(SFld(operac|ASIGH_VIGIL), PARTTIME) == 0) {
		if (StrCmp(FmSFld(fm1, VIGILAD), PARTTIME) == 0) {
			SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), FmLFld(fm4, CLI, i), FmIFld(fm4, OBJ, i), nroleg,
				FmIFld(fm4, I_TIPPTO, i), FmIFld(fm4, I_CODINT, i), FmIFld(fm4, I_NUMINT, i), NULL_DATE);
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

static bool BajaPuesto(long cliente, int objet, int tippto, DATE fecasig)
{
	bool baja = TRUE;
	dbtable APUESTO = (dbtable) ERROR;
	dbcursor c_APUE = (dbcursor) ERROR;
	APUESTO = CreateAlias(operac|PUESTOS);
	c_APUE = CreateCursor(AlInd(APUESTO, operac|PUESTOSbyCLIENTE), IO_NOT_LOCK);

	SetCursorFrom(c_APUE, cliente, objet, tippto, MIN_SHORT);
	SetCursorTo  (c_APUE, cliente, objet, tippto, MAX_SHORT);
	while(FetchCursor(c_APUE) != ERROR ) {
		if (( IsNull(AlFld(APUESTO, operac|PUESTOS_FFINAL)) && 
					fecasig >= DFld(AlFld(APUESTO, operac|PUESTOS_FINICIO))) ||
			(!IsNull(AlFld(APUESTO, operac|PUESTOS_FFINAL)) && 
					DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL)) >= fecasig)) {
				baja = FALSE;
		}

		if (DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  != NULL_DATE &&
			DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  >= fecasig) {
			baja = FALSE;
		}

/*		if(IFld(AlFld(APUESTO, operac|PUESTOS_CANTVIG)) != 0.0 ||
			fecasig < DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL)))
			baja = FALSE;
*/
	}
	DeleteCursor(c_APUE);
	DeleteAlias(APUESTO);
	return baja;
}

static int CalcNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto)
{
	int  nroint, i;

	nroint = GetNextNroint(emp, cliente, objetivo, nroleg, ptoser, puesto);

	for (i = 0; i < FmFldLen(fm0, MULTI0) && !FmIsNull(fm0, NROLEG, i); i++) {
		if (nroleg != FmLFld(fm0, NROLEG, i))
			continue;
		fm1 = UseSubform(fm0, HORARIO, 0, i);
		if (ptoser != FmIFld(fm1, TIPPTO) || puesto != FmIFld(fm1, CODINT))
			continue;

		if (nroint <= FmIFld(fm1, I_NROINT)) {
			nroint = FmIFld(fm1, I_NROINT) + 1;
		}
	}
	RestauraSubForm(fm0, HORARIO, 0, row_padre);
	return nroint;
}

static bool DiaFranco(int emp, long nroleg, DATE fecasig, DATE fechas, char * dia)
{
	int  i;
	DATE fecha;

	for (fecha = fecasig; fecha <= fecasig + 6 ; fecha++) {
		if (Franco(emp, nroleg, fecha, "E", GetNumFrancoEfectivo(emp, nroleg, fecasig))) {
			if (!StrCmp(dia, DiaLetra(fecha))) {
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
//		for (fecha = fecasig; fecha <= fechas; fecha++) {
		for (fecha = fecasig; fecha <= fecasig + 6; fecha++) {
			if (FrancoFm(emp, nroleg, fecha, FmSFld(fm1, REGIM), FmDFld(fm1, FECFRA), FmIFld(fm1, NUMFRAN))) {
				if (!StrCmp(dia, DiaLetra(fecha))) {
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

static void VerificoDobleAsigI(long nroleg)
{
	int  cant = 0, objet, tippto, puesto, nroint;
	long cliente;
	DATE fecasig = MAX_DATE;

	SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), nroleg, MIN_LONG, MIN_SHORT);
	while (GetRecord(operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (*SFld(operac|ASIG_EFECT) == 'E') {
			cant++;
			if (DFld(operac|ASIG_FECASIG) < fecasig) {
				fecasig = DFld(operac|ASIG_FECASIG);
				cliente = LFld(operac|ASIG_CLIENTE);
				objet   = IFld(operac|ASIG_OBJETIVO); 
				tippto  = IFld(operac|ASIG_PTOSER);
				puesto  = IFld(operac|ASIG_PUESTO); 
				nroint  = IFld(operac|ASIG_NROINT);
			}
		}
	}
 	if (cant > 1) {
	 	SetKey(operac|ASIGbyLEGFEC, FmIFld(fm0, EMP), nroleg, fecasig, cliente, objet);
		if (GetRecord(operac|ASIGbyLEGFEC, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			DelRecord(operac|ASIG);
		}
	}
 	if (cant > 1 && *SFld(operac|ASIG_VIGIL) == 'P') {
		SetKey(operac|DIASPTIMEbyEMP, FmIFld(fm0, EMP), cliente, objet, nroleg, tippto, puesto, nroint, MIN_DATE);
		while (GetRecord(operac|DIASPTIMEbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			DelRecord(operac|DIASPTIME);
		}
	}
}

bool YaTieneAsignacion(short emp, long nroleg, DATE fecha)
{
	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (DFld(AlFld(asig, operac|ASIG_FECASIG)) <= fecha) {
			WiMsg("El vigilator esta asignado en el cliente %ld obj %d a partir del %.3D \n",
					LFld(AlFld(asig, operac|ASIG_CLIENTE)), IFld(AlFld(asig, operac|ASIG_OBJETIVO)) ,
					DFld(AlFld(asig, operac|ASIG_FECASIG)));
			return TRUE;
		}
	}

	SetCursorFrom(c_asigh, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		if (DFld(operac|ASIGH_FECALT) <= fecha) {
			WiMsg("El vigilator esta asignado en el cliente %ld obj %d a partir del %.3D \n",
					LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO),
					DFld(operac|ASIGH_FECALT));
			return TRUE;
		}
	}

	return FALSE;
}

