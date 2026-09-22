/********************************************************************
* MODULE & VERSION : @(#)oretro.c	1.14
* DATE             : 09/02/20
* TIME             : 11:21:40
*
* CREATED          : 20/02/01
*
* DESCRIPTION:
*             Carga de Retroactivos.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "opedef.h"
#include "billpro.h"
#include "comerc.sch"
#include "operac.sch"
#include "asist.sch"
#include "sue.sch"
#include "oretro.fmh"
#include "rexcep.fmh"    //fm2 fm4 Excepciones
#include "ausent.fmh"    //fm1 fm3 Ausentismo

#define WAR_SUPERPOS  "El Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T.\n\nDetalle de Horas :\n Hs. Normales: %.2f,  Hs.50: %.2f,  Hs.100 Franco: %.2f,  Hs.100 Feriado: %.2f."
#define WAR_SUPERPOSF "El Vigilador: %s está de Franco en:\nCliente %ld Objetivo %d"
#define	ERR_SUPERPOS  "SUPERPOSICION HORARIA.\n\nEl Vigilador: %s tiene horas cargadas en:\nCliente %ld Obj %d Hr.Ent %.1T Hr.Sal %.1T"
#define ERR_PTODIA    "No podrá asignar vigilador al puesto %d %d el día %s\nporque no se trabaja."
#define	ERR_PUEOCUP   "En el puesto %d %d hay más de %d horas.\nDebe cargar la diferencia en Excepciones."
#define ERR_HORAS     "Existen más horas %s en excepciones.\nVerifique la cantidad de horas."
#define WAR_PTOOCUP   "El puesto %d %d tiene todos los vigiladores asignados.\nPor favor, verifique los datos."

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
static fm_status before(form, fmfield, int), after(form, fmfield, int),

											 after_excep(form, fmfield, int);
static void Lectura(fm_cmd, find_mode);
static void ObtenerParte(int emp, long nroleg, DATE fecparte);
static void	CalcularDifHs(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
						  int nroint, int hsnor, int hs50, int hs100fe, int hs100fr, int *dhsnor, int *dhs50,
						  int *dhs100fe, int *dhs100fr);
static void	CompararHsParte(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
							int nroint, int condic, int motivo, int horas, int hs50, int hs100);
static void	CalcularDifHsExc(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
							 int nroint, int condic, int motivo, int horas, int hs50, int hs100,
							 int *dehoras, int *dehs50, int *dehs100);
static void display(char *buffer);
static void displayB(char *buffer);
static void displaypto(char *buffer);
static void displaynroint(char *buffer);
static int  validate(void);
static int  validateB(void);
static int  validatepto(void);
static int  validatenroint(void);
static bool SupPueFm(TIME hsent, TIME hssal, int fila);
static bool ErrorSuperposicion();
static bool ValidoClienteExcepcion (long cliente, short objetivo, short cond, short motivo, bool valida);
private void PutInTables();
private void DelVigilador();
//private fm_status HelpPuesto(form fm, fmfield fno, int row);
static bool BajaPuestoP(long cliente, int objet, int tippto, DATE dia);
//void BorrarExcepciones (int fila);

/* Declaraciones globales */
int i, row_padre, objet;
form fm0, fm1, fm2, fm3, fm4;
long nroleg, cliente;
schema operac, comerc, asist, sue;
bool aprob = FALSE;
TIME MaxHoraExtra2;


/* Programa principal */
wcmd(oretro, 1.14 02/20/09)
{
	fm_cmd cmd;

	fm0 = OpenForm("oretro", FM_EABORT);

	sue    = OpenSchema("sue",    IO_EABORT);
	asist  = OpenSchema("asist",  IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:  Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:
		/* si esta aprobada no permitir grabarla */
		if (aprob)
			return;

		BeginTransaction();
		PutInTables();
		#ifdef _NOVIA_VER_2_0
			ReclasificarHoras(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FECPARTE));
		#endif
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(operac|RETRO);
		break;
	}

	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	dbcursor c_RETRO, c_REX;
	int i, j;
	char regimen[20];
	DATE fecierre;
	bool entro=FALSE;

	SetIFld(operac|RETRO_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|RETRO_NROLEG,   FmLFld(fm0, NROLEG));
	SetDFld(operac|RETRO_DIA,      FmDFld(fm0, FECPARTE));
	SetLFld(operac|RETRO_CLIENTE,  MIN_LONG);
	SetIFld(operac|RETRO_OBJETIVO, MIN_SHORT);
	switch(GetRecord(operac|RETRObyREMPLE, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 3)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(operac|RETRObyREMPLE, NEXT_KEY|PARTIAL_KEY, 3);
		DbToFm(fm0, 0, NOMBRE);
		FmShowFlds(fm0, 0, NOMBRE);
		return;
	}
	ObtenerParte(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FECPARTE));

	c_RETRO = CreateCursor(operac|RETRObyREMPLE, IO_NOT_LOCK);
	c_REX   = CreateCursor(operac|RETROEXCbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_RETRO, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FECPARTE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_RETRO, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FECPARTE), MAX_LONG, MAX_SHORT);
	for (i = 0; i < FmFldLen(fm0, MULTIRET) && FetchCursor(c_RETRO) != ERROR; i++) {
		entro = TRUE;
		
		if (!ValidaListaXusr(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO)))
        	continue;
        
		FmSetLFld(fm0, RCLIE,      LFld(operac|RETRO_CLIENTE),  i);
		FmSetIFld(fm0, ROBJET,     IFld(operac|RETRO_OBJETIVO), i);
		FmSetIFld(fm0, I_CLINEW,   FALSE,                       i);

		GetRegimenEfectivo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), regimen, FmDFld(fm0, FECPARTE));
		FmSetFld (fm0, RREGIM,     regimen,                     i);
		FmSetIFld(fm0, RTIPPTO,    IFld(operac|RETRO_PTOSER),   i);
		FmSetIFld(fm0, RCODINT,    IFld(operac|RETRO_PUESTO),   i);
		FmSetIFld(fm0, RNROINT,    IFld(operac|RETRO_NROINT),   i);
		FmSetFld (fm0, RCOND,      SFld(operac|RETRO_CONDIC),   i);
		FmSetTFld(fm0, RHSENTRE,   TFld(operac|RETRO_HORAENT),  i);
		FmSetTFld(fm0, RHSSAL,     TFld(operac|RETRO_HORASAL),  i);
		FmSetIFld(fm0, RHSTOT,     ConvHraInt(TFld(operac|RETRO_HORAENT), TFld(operac|RETRO_HORASAL)) * 100, i);
		FmSetIFld(fm0, RNORMAL,    IFld(operac|RETRO_HSNOR),    i);
		FmSetIFld(fm0, REXTRAS1,   IFld(operac|RETRO_HS50),     i);
		FmSetIFld(fm0, REXTRAS2,   IFld(operac|RETRO_HS100FE),  i);
		FmSetIFld(fm0, RFRANCOS,   IFld(operac|RETRO_HS100F),   i);
		FmSetIFld(fm0, I_DHSNOR,   IFld(operac|RETRO_DHSNOR),   i);
		FmSetIFld(fm0, I_DHS50,    IFld(operac|RETRO_DHS50),    i);
		FmSetIFld(fm0, I_DHS100FE, IFld(operac|RETRO_DHS100FE), i);
		FmSetIFld(fm0, I_DHS100F,  IFld(operac|RETRO_DHS100F),  i);

		if (*FmSFld(fm0, RCOND, i) == 'A') {
			fm3 = UseSubform(fm0, RCOND, 0, i);
			FmSetIFld(fm3, CODNOV, IFld(operac|RETRO_CODAUS));
		}

		fm4 = UseSubform(fm0, REXCP, 0, i);
		SetCursorFrom(c_REX, FmIFld(fm0, EMP), LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO),
							 FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG), IFld(operac|RETRO_PTOSER),
							 IFld(operac|RETRO_PUESTO), IFld(operac|RETRO_NROINT), MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_REX, FmIFld(fm0, EMP), LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO),
							 FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG), IFld(operac|RETRO_PTOSER),
							 IFld(operac|RETRO_PUESTO), IFld(operac|RETRO_NROINT), MAX_SHORT, MAX_SHORT);
		for (j = 0; FetchCursor(c_REX) != ERROR && j < FmFldLen(fm4, MULTI); j++) {
			FmSetIFld(fm4, TIPFAC,    IFld(operac|RETROEXC_CONDIC), j);
			FmSetFld (fm4, DTIPFAC,   GetDescCond(IFld(operac|RETROEXC_CONDIC)), j);
			FmSetIFld(fm4, MOTIVO,    IFld(operac|RETROEXC_MOTIVO), j);
			FmSetFld (fm4, DMOTIV,    GetDescMotivo(IFld(operac|RETROEXC_CONDIC), IFld(operac|RETROEXC_MOTIVO)), j);
			FmSetIFld(fm4, HORAS,     IFld(operac|RETROEXC_HORAS),  j);
			FmSetIFld(fm4, HS50,      IFld(operac|RETROEXC_HS50),   j);
			FmSetIFld(fm4, HS100,     IFld(operac|RETROEXC_HS100),  j);
			FmSetFld (fm4, OBS,       SFld(operac|RETROEXC_OBS),    j);
			FmSetIFld(fm4, I_DEHORAS, IFld(operac|RETROEXC_DHORAS), j);
			FmSetIFld(fm4, I_DEHS50,  IFld(operac|RETROEXC_DHS50),  j);
			FmSetIFld(fm4, I_DEHS100, IFld(operac|RETROEXC_DHS100), j);
		}
	}
	DeleteCursor(c_RETRO);
	DeleteCursor(c_REX);

	fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
	if (entro && fecierre != NULL_DATE && DFld(operac|RETRO_FECREG) != NULL_DATE && DFld(operac|RETRO_FECREG) <= fecierre) {
		Warning("El Parte está cerrado el %.3D .\nNo podrá modificarse.", fecierre);
		aprob = TRUE;
		FmSetDisplayOnly(fm0, FECREG, CONTROL_FLD, TRUE);
		FmSetDisplayOnly(fm2, CODNOV, DESCRINAS,   TRUE);
		FmSetDisplayOnly(fm4, MULTI,  OBS,         TRUE);
	}
	else {
		FmSetDisplayOnly(fm0, FECREG, CONTROL_FLD, FALSE);
		FmSetDisplayOnly(fm2, CODNOV, DESCRINAS,   FALSE);
		FmSetDisplayOnly(fm4, MULTI,  OBS,         FALSE);
	}
}

static void ObtenerParte(int emp, long nroleg, DATE fecparte)
{
	dbcursor c_PARTE, c_EX;
	int i, j;
	char regimen[20];

	c_PARTE = CreateCursor(operac|PARTEbyEMPLE,   IO_NOT_LOCK);
	c_EX    = CreateCursor(operac|EXCEPCIONbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_PARTE, emp, nroleg, fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_PARTE, emp, nroleg, fecparte, MAX_LONG, MAX_SHORT);
	for (i = 0; i < FmFldLen(fm0, MULTIPAR) && FetchCursor(c_PARTE) != ERROR; i++) {
		FmSetLFld(fm0, PCLIE,  LFld(operac|PARTE_CLIENTE),  i);
		FmSetIFld(fm0, POBJET, IFld(operac|PARTE_OBJETIVO), i);
		GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), regimen, fecparte);
		FmSetFld (fm0, PREGIM,   regimen, i);
		FmSetFld (fm0, PCOND,    SFld(operac|PARTE_CONDIC),  i);
		FmSetIFld(fm0, PTIPPTO,  IFld(operac|PARTE_PTOSER),  i);
		FmSetIFld(fm0, PCODINT,  IFld(operac|PARTE_PUESTO),  i);
		FmSetIFld(fm0, PNROINT,  IFld(operac|PARTE_NROINT),  i);
		FmSetTFld(fm0, PHSENTRE, TFld(operac|PARTE_HORAENT), i);
		FmSetTFld(fm0, PHSSAL,   TFld(operac|PARTE_HORASAL), i);
		FmSetIFld(fm0, PHSTOT,   ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100, i);
		FmSetIFld(fm0, PNORMAL,  IFld(operac|PARTE_HSNOR),   i);
		FmSetIFld(fm0, PEXTRAS1, IFld(operac|PARTE_HS50),    i);
		FmSetIFld(fm0, PEXTRAS2, IFld(operac|PARTE_HS100FE), i);
		FmSetIFld(fm0, PFRANCOS, IFld(operac|PARTE_HS100F),  i);

		if (*FmSFld(fm0, PCOND, i) == 'A') {
			fm1 = UseSubform(fm0, PCOND, 0, i);
			FmSetIFld(fm1, CODNOV, IFld(operac|PARTE_CODAUS));
		}

		fm2 = UseSubform(fm0, PEXCP, 0, i);
		SetCursorFrom(c_EX, FmIFld(fm0, EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
							FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG), IFld(operac|PARTE_PTOSER),
							IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_EX, FmIFld(fm0, EMP), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
							FmDFld(fm0, FECPARTE), FmLFld(fm0, NROLEG), IFld(operac|PARTE_PTOSER),
							IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), MAX_SHORT, MAX_SHORT);
		for (j = 0; FetchCursor(c_EX) != ERROR && j < FmFldLen(fm2, MULTI); j++) {
			FmSetIFld(fm2, TIPFAC,  IFld(operac|EXCEPCION_CONDIC), j);
			FmSetFld (fm2, DTIPFAC, GetDescCond(IFld(operac|EXCEPCION_CONDIC)), j);
			FmSetIFld(fm2, MOTIVO,  IFld(operac|EXCEPCION_MOTIVO), j);
			FmSetFld (fm2, DMOTIV,  GetDescMotivo(IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO)), j);
			FmSetIFld(fm2, HORAS,   IFld(operac|EXCEPCION_HORAS), j);
			FmSetIFld(fm2, HS50,    IFld(operac|EXCEPCION_HS50),  j);
			FmSetIFld(fm2, HS100,   IFld(operac|EXCEPCION_HS100), j);
			FmSetFld (fm2, OBS,     SFld(operac|EXCEPCION_OBS),   j);
		}
	}
	DeleteCursor(c_PARTE);
	DeleteCursor(c_EX);
}

static fm_status before(form fm, fmfield fno, int row)
{


	switch (fno) {
	case REXTRAS1: 
	case RFRANCOS:
		if (VigPartime(FmIFld(fm, EMP), FmLFld(fm, NROLEG), FmDFld(fm, FECPARTE))) {
			FmSetIFld(fm, fno, 0, row);
			return FM_SKIP;
		}
		break;
	case REXTRAS2:
		if (VigPartime(FmIFld(fm, EMP), FmLFld(fm, NROLEG), FmDFld(fm, FECPARTE)) &&
			!FeriadoNovia(FmDFld(fm, FECPARTE), GetCliePais(FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row)),
					 GetProvObjet(FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row)))) {
			if (FmTFld(fm, RHSSAL, row)<FmTFld(fm, RHSENTRE, row) && 
				FeriadoNovia(FmDFld(fm, FECPARTE)+1, GetCliePais(FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row)),
					 GetProvObjet(FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row)))) {
				MaxHoraExtra2=FmTFld(fm, RHSSAL, row);
			}
			else {
				MaxHoraExtra2=0;
				FmSetIFld(fm, fno, 0, row);
				return FM_SKIP;
			}
		}
		break;
	case RDET: 
		break;
	case RCLIE:
	   	InicClientesXusr();
		break;
	case ROBJET:
	   	InicObjetivosXusr(FmLFld(fm, RCLIE, row), FmIFld(fm, EMP, row));
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	dbcursor c_ptoser, CUR, c_puesto;
	char regimen[20];
	int  n, hsnor = NULL_SHORT, hs100fe = NULL_SHORT;
	int dhsnor=0, dhs50=0, dhs100fe=0, dhs100fr=0;
	char str_aux[20];

	row_padre = row;

	switch (fno) {
	case EMP:
		InicListaXusr(FmIFld(fm0, EMP));
    break;
	case NROLEG:
		if (FmKeyCode(fm) == K_META || FmChgFld(fm) && FmKeyCode(fm) != K_DEL) {
			if (FmKeyCode(fm) == K_META) {
				nroleg = ERROR;
				if ((nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;
				FmSetLFld(fm, fno, nroleg, row);
			}
		}
		break;
	case FECPARTE:
		SetIFld(sue|PER_EMP,    FmIFld(fm, EMP));
		SetLFld(sue|PER_NROLEG, FmLFld(fm, NROLEG));
		if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR && IFld(sue|PER_ACTIVO) == 0) {
			if (FmDFld(fm0, FECPARTE) > DFld(sue|PER_FECEGR)) {
				Warning("El legajo se encuetra inactivo.");
				return FM_REDO;
			}
		}
		break;
	case RCLIE:
		if (FmKeyCode(fm) == K_HELP){
			HelpCliente(fm, fno, row);
			FmSetIFld(fm, ROBJET, NULL_SHORT,row);
		}	

		if (!FmIsNull(fm, fno, row ))
			if (!ValidaClienteXusr(FmLFld(fm, fno, row)))
				return FM_SKIP;

		break; 
		
	case ROBJET:


		if (FmKeyCode(fm) == K_HELP)	{
			HelpObjet2(fm, fno, row, FmLFld(fm, RCLIE, row));
			
//	    	FmSetFld(fm, RREGIM, NULL_STR); //Solo se comento esta linea para que no produzca error
	    }
	    
		if (!FmIsNull(fm, fno, row ))
			if (!ValidaObjetivoXusr(FmLFld(fm, RCLIE, row), FmIFld(fm, fno, row), FmIFld(fm, EMP)))
				return FM_SKIP;
        
        
		if (FmIsNull(fm, RREGIM, row)) {
			GetRegimenEfectivo(FmIFld(fm, EMP), FmLFld(fm, NROLEG), regimen, FmDFld(fm, FECPARTE));
			strcpy(str_aux, regimen);
			FmSetFld (fm, RREGIM, str_aux, row);
			FmSetIFld(fm, I_CLINEW, TRUE, row);
		}
		break;            
		
	case RTIPPTO:
		if (FmKeyCode(fm) == K_HELP) {
			//Si el Objetivo es de Brigada debo leer la tabla operac_puestos porque estos objetivos
			//no tienen OT generadas.
			if (GetServicioObj(FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row)) == BRIGADA) {
				c_puesto = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
				SetCursorFrom(c_puesto, FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row), MIN_SHORT, MIN_SHORT);
				SetCursorTo  (c_puesto, FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row), MAX_SHORT, MAX_SHORT);
				n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_puesto, 3, validateB, displayB);

				if ( n >= 0 )
					FmSetIFld(fm, fno, IFld(operac|PUESTOS_TIPPTO), row);
			}
			else {
				c_ptoser = CreateCursor(comerc|PTOSERbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
				SetCursorFrom(c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row),
										MIN_SHORT);
				SetCursorTo  (c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row),
										MAX_SHORT);
				n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_ptoser, 4, validate, display);

				if (n >= 0)
					FmSetIFld(fm, fno, IFld(comerc|PTOSER_TIPPTO),  row);
			}
		}
		break;
		
	case RCODINT:
		if (FmKeyCode(fm) == K_HELP) {
			CUR = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

			SetCursorFrom(CUR, FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row), FmIFld(fm0, RTIPPTO, row),
																								MIN_SHORT);
			SetCursorTo  (CUR, FmLFld(fm0, RCLIE, row), FmIFld(fm0, ROBJET, row), FmIFld(fm0, RTIPPTO, row),
																								MAX_SHORT);
			n = PopUpDbMenu(10, 85, "Pto Cteg  HrIni  HrFin      Dias        Regimen  CantVig CantPto  FecIni   FecFin", CUR, 4, validatepto, displaypto);

			if (n >= 0) { 
				FmSetIFld (fm, RCODINT, IFld(operac|PUESTOS_CODINT), row);
				FmShowFlds(fm, RCODINT, RCODINT, row);
			}
			DeleteCursor(CUR);
		}
		break;
	case RNROINT:
		if (FmKeyCode(fm) == K_HELP) {
			CUR = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK|IO_CONTROL_BREAK);

			SetCursorFrom(CUR, FmIFld(fm, EMP), FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row),
							   FmDFld(fm, FECPARTE), FmLFld(fm, NROLEG), FmIFld(fm, RTIPPTO, row),
							   FmIFld(fm, RCODINT, row), MIN_SHORT);
			SetCursorTo  (CUR, FmIFld(fm, EMP), FmLFld(fm, RCLIE, row), FmIFld(fm, ROBJET, row),
							   FmDFld(fm, FECPARTE), FmLFld(fm, NROLEG), FmIFld(fm, RTIPPTO, row),
							   FmIFld(fm, RCODINT, row), MAX_SHORT);
			n = PopUpDbMenu(10, 40, "   Puesto     HrDesde  HrHasta", CUR, 8, validatenroint, displaynroint);

			if (n > 0) { 
				FmSetIFld (fm, RNROINT, IFld(operac|PARTE_NROINT), row);
			}
			else
			    FmSetIFld (fm, RNROINT, 1, row);
			FmShowFlds(fm, RNROINT, RNROINT, row);
			DeleteCursor(CUR);
		}
	case RHSENTRE: 
		if (strcmp(FmFldPrev(fm), NULL_STR) && FmChgFld(fm))
			FmSetIFld(fm0, RHSTOT, ConvHraInt(FmTFld(fm0, RHSENTRE, row), FmTFld(fm0, RHSSAL, row)) * 100, row);
		break;
	case RHSSAL:
		if (FmChgFld(fm))
			FmSetIFld(fm0, RHSTOT, ConvHraInt(FmTFld(fm0, RHSENTRE, row), FmTFld(fm0, RHSSAL, row)) * 100, row);
		break;
	case RCOND:
		if (FmChgFld(fm) && (*FmSFld(fm, fno, row) == 'A' || *FmSFld(fm, fno, row) == 'V')) {

			CalcularDifHs(FmLFld(fm, RCLIE,   row), FmIFld(fm, ROBJET,  row), FmDFld(fm, FECPARTE),
						  FmLFld(fm, NROLEG),       FmIFld(fm, RTIPPTO, row), FmIFld(fm, RCODINT, row),
						  FmIFld(fm, RNROINT, row), 0, 0, 0, 0, &dhsnor, &dhs50, &dhs100fe, &dhs100fr);

			FmSetIFld(fm, I_DHSNOR,   dhsnor,   row);
			FmSetIFld(fm, I_DHS50,    dhs50,    row);
			FmSetIFld(fm, I_DHS100FE, dhs100fe, row);
			FmSetIFld(fm, I_DHS100F,  dhs100fr, row);


			if (*FmSFld(fm, fno, row) == 'A') { 
				int j;

				DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
				fm2 = UseSubform(fm0, REXCP, 0, row);
				for (j = 0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {
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
			FmSetTFld(fm, RHSENTRE, StrToT("0000"), row);
			FmSetTFld(fm, RHSSAL,   StrToT("0000"), row);
			FmSetIFld(fm, RHSTOT,   0,              row);
			FmSetIFld(fm, RNORMAL,  0,              row);
			FmSetIFld(fm, REXTRAS1, 0,              row);
			FmSetIFld(fm, REXTRAS2, 0,              row);
			FmSetIFld(fm, RFRANCOS, 0,              row);
			FmNextFld(fm, RCLIE,    row + 1);
		}
		break;
	case AGRUPHS:
		if (ErrorSuperposicion())
			return FM_ERROR;

/*		BorrarExcepciones (row);

		if (FmChgFld(fm)) {
			if (VigPartime(FmIFld(fm, EMP), FmLFld(fm, NROLEG, row), FmDFld(fm, FECPARTE))) {
				CalcularDetallePartime(FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmDFld(fm, FECPARTE),
									   FmTFld(fm, HSENTRE, row), FmTFld(fm, HSSAL, row), &hsnor, &hs100fe);

				SetIFld(operac|RETRO_HSNOR,   hsnor);
				SetIFld(operac|RETRO_HS50,    NULL_SHORT);
				SetIFld(operac|RETRO_HS100F,  NULL_SHORT);
				SetIFld(operac|RETRO_HS100FE, hs100fe);
				PutRecord(operac|RETRO);
			}
			else
				CalcularDetalleVigNue(row, FmLFld(fm, NROLEG, row), FmIFld(fm4, TIPPTO), FmIFld(fm4, CODINT), FmIFld(fm4, NROINT));
		}
		break;
*/

	case AGRUPDET:

		CalcularDifHs(FmLFld(fm, RCLIE,    row), FmIFld(fm, ROBJET,   row), FmDFld(fm, FECPARTE),
					  FmLFld(fm, NROLEG),        FmIFld(fm, RTIPPTO,  row), FmIFld(fm, RCODINT,  row),
					  FmIFld(fm, RNROINT,  row), FmIFld(fm, RNORMAL,  row), FmIFld(fm, REXTRAS1, row),
					  FmIFld(fm, REXTRAS2, row), FmIFld(fm, RFRANCOS, row), &dhsnor, &dhs50, &dhs100fe,
					  &dhs100fr);

		FmSetIFld(fm, I_DHSNOR,   dhsnor,   row);
		FmSetIFld(fm, I_DHS50,    dhs50,    row);
		FmSetIFld(fm, I_DHS100FE, dhs100fe, row);
		FmSetIFld(fm, I_DHS100F,  dhs100fr, row);

		break;

	case PEXCP :
		if (FmKeyCode(fm) == K_META) {
			fm2 = UseSubform(fm0, PEXCP, 0, row);
			FmSetDisplayOnly(fm2, MULTI,  OBS, TRUE);
	
			DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
		}
		break;
	case REXCP :
		if (FmKeyCode(fm) == K_META)
			DoSubform(fm, NULLFP, after_excep, fno, 0, row);
		break;
	case REXTRAS2:                    
		if (MaxHoraExtra2!=0) {
			if (FmIFld(fm, fno, row)!=0 &&FmIFld(fm, fno, row)> ConvHraInt(StrToT("00:00"), MaxHoraExtra2)*100) {
				WiMsg ("La cantidad maxima de horas extras son : %.1T ", MaxHoraExtra2);
				return FM_REDO;
			}
		}
		break;
	}
	
	return FM_OK;
}

static fm_status after_excep(form fm, fmfield fno, int row)
{
	int i, hsnor = 0, hs50 = 0, hs100 = 0, dehoras, dehs50, dehs100;

	switch (fno) {
	case AGRUP:
		if (!ValidoClienteExcepcion(FmLFld(fm0, RCLIE, row_padre), FmIFld(fm0, ROBJET, row_padre), 
								    FmIFld(fm, TIPFAC, row), FmIFld(fm, MOTIVO, row), FmIFld(fm, VALCLI, row))) {
			WiMsg ("El cliente objetivo no esta habilitado a usar este motivo de excepcion.");
			return FM_REDO;
		}
		break;
	case OBS:
		CompararHsParte(FmLFld(fm0, RCLIE, row_padre), FmIFld(fm0, ROBJET, row_padre), FmDFld(fm0, FECPARTE),
						 FmLFld(fm0, NROLEG), FmIFld(fm0, RTIPPTO, row_padre), FmIFld(fm0, RCODINT, row_padre),
						 FmIFld(fm0, RNROINT, row_padre), FmIFld(fm, TIPFAC, row), FmIFld(fm, MOTIVO, row),
						 FmIFld(fm, HORAS, row), FmIFld(fm, HS50, row), FmIFld(fm, HS100, row));

		CalcularDifHsExc(FmLFld(fm0, RCLIE, row_padre), FmIFld(fm0, ROBJET, row_padre), FmDFld(fm0, FECPARTE),
						 FmLFld(fm0, NROLEG), FmIFld(fm0, RTIPPTO, row_padre), FmIFld(fm0, RCODINT, row_padre),
						 FmIFld(fm0, RNROINT, row_padre), FmIFld(fm, TIPFAC, row), FmIFld(fm, MOTIVO, row),
						 FmIFld(fm, HORAS, row), FmIFld(fm, HS50, row), FmIFld(fm, HS100, row), &dehoras,
						 &dehs50, &dehs100);

		FmSetIFld(fm, I_DEHORAS, dehoras, row);
		FmSetIFld(fm, I_DEHS50,  dehs50,  row);
		FmSetIFld(fm, I_DEHS100, dehs100, row);
		break;
	case CONTROL_FLD :
		for (i = 0; i < FmFldLen(fm, MULTI) && !FmIsNull(fm, TIPFAC, i); i++) {
			hsnor += FmIFld(fm, HORAS, i);
			hs50  += FmIFld(fm, HS50,  i);
			hs100 += FmIFld(fm, HS100, i);
		}
		if (hsnor > FmIFld(fm0, RNORMAL, row_padre)) {
			Warning(ERR_HORAS,"normales");
			FmNextFld(fm, TIPFAC, 0);
		}
		if (hs50  > FmIFld(fm0, REXTRAS1, row_padre)) {
			Warning(ERR_HORAS,"extras al 50%");
			FmNextFld(fm, TIPFAC, 0);
		}
		if (hs100 > FmIFld(fm0, REXTRAS2, row_padre) + FmIFld(fm0, RFRANCOS, row_padre)) {
			Warning(ERR_HORAS,"extras al 100%");
			FmNextFld(fm, TIPFAC, 0);
		}
		break;
	}
	return FM_OK;
}

static void	CompararHsParte(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
							int nroint, int condic, int motivo, int horas, int hs50, int hs100)
{
	int i, j;

	for (i = 0; i < FmFldLen(fm0, MULTIPAR) && !FmIsNull(fm0, PCLIE, i); i++) {
		if (FmLFld(fm0, PCLIE,   i) != cliente || FmIFld(fm0, POBJET,  i) != objet  ||
			FmIFld(fm0, PTIPPTO, i) != tippto  || FmIFld(fm0, PCODINT, i) != codint ||
			FmIFld(fm0, PNROINT, i)	!= nroint)
			continue;

		fm2 = UseSubform(fm0, PEXCP, 0, i);
		for (j = 0; j < FmFldLen(fm2, MULTI) && !FmIsNull(fm2, TIPFAC, j); j++) {
			if (FmIFld(fm2, TIPFAC, j) != condic || FmIFld(fm2, MOTIVO, j) != motivo)
				continue;
			if (FmIFld(fm2, HORAS, j) != horas)
				Warning("Verifique los datos cargados.\nHay diferencias con las Hs. Normales cargadas en el Parte.\nHs. en Parte %.2f, Hs. en Retro %.2f.",
												(double)FmIFld(fm2, HORAS, j) / 100, (double)horas / 100);
			if (FmIFld(fm2, HS50,  j) != hs50)
				Warning("Verifique los datos cargados.\nHay diferencias con las Hs. Extras al 50% cargadas en el Parte.\nHs. en Parte %.2f, Hs. en Retro %.2f.",
												(double)FmIFld(fm2, HS50,  j) / 100, (double)hs50 / 100);
			if (FmIFld(fm2, HS100, j) != hs100)
				Warning("Verifique los datos cargados.\nHay diferencias con las Hs. Extras al 100% cargadas en el Parte.\nHs. en Parte %.2f, Hs. en Retro %.2f.",
												(double)FmIFld(fm2, HS100, j) / 100, (double)hs100 / 100);
		}
	}
}

static void	CalcularDifHsExc(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
							 int nroint, int condic, int motivo, int horas, int hs50, int hs100,
							 int *dehoras, int *dehs50, int *dehs100)
{
	SetIFld(operac|EXCEPCION_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|EXCEPCION_CLIENTE,  cliente);
	SetIFld(operac|EXCEPCION_OBJETIVO, objet);
	SetDFld(operac|EXCEPCION_DIA,      fecparte);
	SetLFld(operac|EXCEPCION_NROLEG,   nroleg);
	SetIFld(operac|EXCEPCION_PTOSER,   tippto);
	SetIFld(operac|EXCEPCION_PUESTO,   codint);
	SetIFld(operac|EXCEPCION_NROINT,   nroint);
	SetIFld(operac|EXCEPCION_CONDIC,   condic);
	SetIFld(operac|EXCEPCION_MOTIVO,   motivo);
	if (GetRecord(operac|EXCEPCIONbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		*dehoras = horas - IFld(operac|EXCEPCION_HORAS);
		*dehs50  = hs50  - IFld(operac|EXCEPCION_HS50);
		*dehs100 = hs100 - IFld(operac|EXCEPCION_HS100);
	}
	else {
		*dehoras = horas;
		*dehs50  = hs50;
		*dehs100 = hs100;
	} 
}

static void	CalcularDifHs(long cliente, int objet, DATE fecparte, long nroleg, int tippto, int codint,
						  int nroint, int hsnor, int hs50, int hs100fe, int hs100fr, int *dhsnor, int *dhs50,
						  int *dhs100fe, int *dhs100fr)
{
	SetIFld(operac|PARTE_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|PARTE_CLIENTE,  cliente);
	SetIFld(operac|PARTE_OBJETIVO, objet);
	SetDFld(operac|PARTE_DIA,      fecparte);
	SetLFld(operac|PARTE_NROLEG,   nroleg);
	SetIFld(operac|PARTE_PTOSER,   tippto);
	SetIFld(operac|PARTE_PUESTO,   codint);
	SetIFld(operac|PARTE_NROINT,   nroint);
	if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		*dhsnor   = hsnor   - IFld(operac|PARTE_HSNOR);
		*dhs50    = hs50    - IFld(operac|PARTE_HS50);
		*dhs100fe = hs100fe - IFld(operac|PARTE_HS100FE);
		*dhs100fr = hs100fr - IFld(operac|PARTE_HS100F);
	}
	else {
		*dhsnor   = hsnor;
		*dhs50    = hs50;
		*dhs100fe = hs100fe;
		*dhs100fr = hs100fr;
	} 
}

static int validate()
{
	if (!BajaPuesto(LFld(comerc|PTOSER_CLIENTE), IFld(comerc|PTOSER_OBJET), IFld(comerc|PTOSER_TIPPTO),
					FmDFld(fm0, FECPARTE)))
		return TRUE;
	return FALSE;		
}

static void display(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(comerc|PTOSER_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(comerc|PTOSER_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}

static int validateB()
{
	if(!BajaPuestoP(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO),
					FmDFld(fm0, FECPARTE)))
		return TRUE;
	return FALSE;		
}

static void displayB(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(operac|PUESTOS_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(operac|PUESTOS_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}

static int validatepto(void)
{
	if (IFld(operac|PUESTOS_CANTVIG) != 0.0 &&
	   ((IsNull(operac|PUESTOS_FFINAL)  && !IsNull(operac|PUESTOS_FINICIO) &&
		FmDFld(fm0, FECPARTE) < DFld(operac|PUESTOS_FINICIO)) ||
	   (!IsNull(operac|PUESTOS_FFINAL) &&
	   (FmDFld(fm0, FECPARTE) < DFld(operac|PUESTOS_FINICIO) || FmDFld(fm0, FECPARTE) > DFld(operac|PUESTOS_FFINAL)))))
		return FALSE;

	if (IFld(operac|PUESTOS_CANTVIG) == 0.0 && !IsNull(operac|PUESTOS_FFINAL) &&
	    (FmDFld(fm0, FECPARTE) > DFld(operac|PUESTOS_FFINAL) || FmDFld(fm0, FECPARTE) < DFld(operac|PUESTOS_FINICIO)))
		return FALSE;

	if (IFld(operac|PUESTOS_CANTVIG) == 0.0 && IsNull(operac|PUESTOS_FFINAL) &&
	    FmDFld(fm0, FECPARTE) > DFld(operac|PUESTOS_FINICIO))
		return FALSE;

	return TRUE;
}

static void displaypto(char *buffer)
{
	int i = 0;

	sprintf(buffer,"%2d %4d  %.*T  %.*T  %-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s  %8.8s    %4.2f    %3.3d   %.1D %.1D",
			IFld(operac|PUESTOS_CODINT), IFld(operac|PUESTOS_PUESTO), DFMT_SEPAR,
			TFld(operac|PUESTOS_HINICIO), DFMT_SEPAR, TFld(operac|PUESTOS_HFINAL), SFld(operac|PUESTOS_DIA1),
			SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4),
			SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7),
			SFld(operac|PUESTOS_REGIM), (double)IFld(operac|PUESTOS_CANTVIG)/100.00,
			IFld(operac|PUESTOS_CANTPUE),DFld(operac|PUESTOS_FINICIO), DFld(operac|PUESTOS_FFINAL));
}

static int validatenroint(void)
{
	return TRUE;
}

static void displaynroint(char *buffer)
{
	sprintf(buffer,"%2d %4d %2d    %.*T    %.*T",
			IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT),
			DFMT_SEPAR, TFld(operac|PARTE_HORAENT), DFMT_SEPAR, TFld(operac|PARTE_HORASAL));
}

private void PutInTables()
{
	dbcursor c_EX;
	int i, j;

	c_EX = CreateCursor(operac|RETROEXCbyLEGAJO, IO_NOT_LOCK);

	DelVigilador();

	for (i = 0; i < FmFldLen(fm0, MULTIRET) && !FmIsNull(fm0, RCLIE, i); i++) {

  		InitRecord(operac|RETRO);

			/*-------* Alta en Retro *-------*/
		SetIFld(operac|RETRO_EMP,      FmIFld(fm0, EMP));
		SetDFld(operac|RETRO_DIA,      FmDFld(fm0, FECPARTE));
		SetLFld(operac|RETRO_NROLEG,   FmLFld(fm0, NROLEG));
		SetLFld(operac|RETRO_CLIENTE,  FmLFld(fm0, RCLIE,    i));
		SetIFld(operac|RETRO_OBJETIVO, FmIFld(fm0, ROBJET,   i));
		SetTFld(operac|RETRO_HORAENT,  FmTFld(fm0, RHSENTRE, i));
		SetTFld(operac|RETRO_HORASAL,  FmTFld(fm0, RHSSAL,   i));
		SetIFld(operac|RETRO_PTOSER,   FmIFld(fm0, RTIPPTO,  i));
		SetIFld(operac|RETRO_PUESTO,   FmIFld(fm0, RCODINT,  i));
		SetIFld(operac|RETRO_NROINT,   FmIFld(fm0, RNROINT,  i));
		SetFld (operac|RETRO_CONDIC,   FmSFld(fm0, RCOND,    i));
		SetDFld(operac|RETRO_FECREG,   FmDFld(fm0, FECREG));
		SetIFld(operac|RETRO_HSNOR,    FmIFld(fm0, RNORMAL,  i));
		SetIFld(operac|RETRO_HS50,     FmIFld(fm0, REXTRAS1, i));
		SetIFld(operac|RETRO_HS100FE,  FmIFld(fm0, REXTRAS2, i));
		SetIFld(operac|RETRO_HS100F,   FmIFld(fm0, RFRANCOS, i));
		SetIFld(operac|RETRO_CONFIR,   0);
		SetIFld(operac|RETRO_DHSNOR,   FmIFld(fm0, I_DHSNOR,   i));
		SetIFld(operac|RETRO_DHS50,    FmIFld(fm0, I_DHS50,    i));
		SetIFld(operac|RETRO_DHS100FE, FmIFld(fm0, I_DHS100FE, i));
		SetIFld(operac|RETRO_DHS100F,  FmIFld(fm0, I_DHS100F,  i));

				/*-------* Alta en asist_asisten *--------*/
		if (*FmSFld(fm0, RCOND, i) == 'A') {
			fm3 = UseSubform(fm0, RCOND, 0, i);
	
			SetIFld(operac|RETRO_CODAUS,  FmIFld(fm3, CODNOV));

			SetIFld(asist|LICEN_EMP,    FmIFld(fm0, EMP));
			SetLFld(asist|LICEN_NROLEG, FmLFld(fm0, NROLEG));
			SetIFld(asist|LICEN_LICEN,  MIN_SHORT);
			SetDFld(asist|LICEN_FECHAD, MIN_DATE);
			if (GetRecord(asist|LICENbyTIPLIC, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
				if (FmDFld(fm0, FECPARTE) < DFld(asist|LICEN_FECHAD) ||
					FmDFld(fm0, FECPARTE) > DFld(asist|LICEN_FECHAH)) {
					InitRecord(asist|ASISTEN);
					SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
					SetDFld(asist|ASISTEN_FECHA,  FmDFld(fm0, FECPARTE));
					SetLFld(asist|ASISTEN_NROLEG, FmLFld(fm0, NROLEG));
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
				SetLFld(asist|ASISTEN_NROLEG, FmLFld(fm0, NROLEG));
				SetIFld(asist|ASISTEN_CODNOV, FmIFld(fm3, CODNOV));
				SetLFld(asist|ASISTEN_VALOR,  100);
				SetIFld(asist|ASISTEN_JUSTIF, TRUE); /* seteo con TRUE para que en ASISTEN_JUSTIF sea un NO */
				PutRecord(asist|ASISTEN);
			} 
		}
		PutRecord(operac|RETRO);

		/*-------* Alta en Excepciones *--------*/
		fm4 = UseSubform(fm0, REXCP, 0, i);

		for (j = 0; j < FmFldLen(fm4, MULTI) && !FmIsNull(fm4, TIPFAC, j); j++) {
			SetIFld(operac|RETROEXC_EMP,      FmIFld(fm0, EMP));
			SetLFld(operac|RETROEXC_NROLEG,   FmLFld(fm0, NROLEG));
			SetDFld(operac|RETROEXC_DIA,      FmDFld(fm0, FECPARTE));
			SetLFld(operac|RETROEXC_CLIENTE,  FmLFld(fm0, RCLIE,     i));
			SetIFld(operac|RETROEXC_OBJETIVO, FmIFld(fm0, ROBJET,    i));
			SetIFld(operac|RETROEXC_PTOSER,   FmIFld(fm0, RTIPPTO,   i));
			SetIFld(operac|RETROEXC_PUESTO,   FmIFld(fm0, RCODINT,   i));
			SetIFld(operac|RETROEXC_NROINT,   FmIFld(fm0, RNROINT,   i));
			SetIFld(operac|RETROEXC_CONDIC,   FmIFld(fm4, TIPFAC,    j));
			SetIFld(operac|RETROEXC_MOTIVO,   FmIFld(fm4, MOTIVO,    j));
			SetIFld(operac|RETROEXC_HORAS,    FmIFld(fm4, HORAS,     j));
			SetIFld(operac|RETROEXC_HS50,     FmIFld(fm4, HS50,      j));
			SetIFld(operac|RETROEXC_HS100,    FmIFld(fm4, HS100,     j));
			SetFld (operac|RETROEXC_OBS,      FmSFld(fm4, OBS,       j));
			SetIFld(operac|RETROEXC_DHORAS,   FmIFld(fm4, I_DEHORAS, j));
			SetIFld(operac|RETROEXC_DHS50,    FmIFld(fm4, I_DEHS50,  j));
			SetIFld(operac|RETROEXC_DHS100,   FmIFld(fm4, I_DEHS100, j));
			PutRecord(operac|RETROEXC);
		}
	}
	DeleteCursor(c_EX);
}

private void DelVigilador()
{
	SetIFld(operac|RETRO_EMP,      FmIFld(fm0, EMP));
	SetLFld(operac|RETRO_NROLEG,   FmLFld(fm0, NROLEG));
	SetDFld(operac|RETRO_DIA,      FmDFld(fm0, FECPARTE));
	SetLFld(operac|RETRO_CLIENTE,  MIN_LONG);
	SetIFld(operac|RETRO_OBJETIVO, MIN_SHORT);
	while (GetRecord(operac|RETRObyREMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		/*-------* Borrar operac_retroexc *--------*/
		SetIFld(operac|RETROEXC_EMP,       FmIFld(fm0, EMP));
		SetLFld(operac|RETROEXC_NROLEG,    FmLFld(fm0, NROLEG));
		SetDFld(operac|RETROEXC_DIA,       FmDFld(fm0, FECPARTE));
		SetLFld(operac|RETROEXC_CLIENTE,   MIN_LONG);
		SetIFld(operac|RETROEXC_OBJETIVO,  MIN_SHORT);
		while (GetRecord(operac|RETROEXCbyLEGAJO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
			DelRecord(operac|RETROEXC);

		DelRecord(operac|RETRO);
	}
	/*-------* Borrar asist_asisten *--------*/
	SetIFld(asist|ASISTEN_EMPRE,  FmIFld(fm0, EMP));
	SetDFld(asist|ASISTEN_FECHA,  FmDFld(fm0, FECPARTE));
	SetLFld(asist|ASISTEN_NROLEG, FmLFld(fm0, NROLEG));
	SetIFld(asist|ASISTEN_CODNOV, MIN_SHORT);
	while (GetRecord(asist|ASISTENbyEMPRE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
		DelRecord(asist|ASISTEN);
}

static bool SupPueFm(TIME hsent, TIME hssal, int fila)
{
	int  i;
	bool haysup = FALSE;

	for (i = 0; i < FmFldLen(fm0, MULTIRET) && !FmIsNull(fm0, RCLIE, i); i++) {
		if (i == fila)
			continue;

		haysup = Superposicion(hsent, hssal, FmTFld(fm0, RHSENTRE, i), FmTFld(fm0, RHSSAL, i), FALSE);

		if (!haysup)
			continue;
		break;
	}
	return haysup;
}

static bool ErrorSuperposicion ()
{
	int fila = 0;

	for (fila = 0; i < FmFldLen(fm0, MULTIRET) && !FmIsNull(fm0, RCLIE, fila); fila++) {
		if (SupPueFm(FmTFld(fm0, RHSENTRE, fila), FmTFld(fm0, RHSSAL, fila), fila)
			&& GetUid() != 1046) { 
			Warning(ERR_SUPERPOS, GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG)),
								  FmLFld(fm0, RCLIE, fila), FmIFld(fm0, ROBJET, fila), 
								  FmTFld(fm0, RHSENTRE, fila), FmTFld(fm0, RHSSAL, fila));
			return TRUE;
		}
	}
	return FALSE;
}

/*
void BorrarExcepciones (int fila) 
{
	form fmaux;
	int i;
*/
	//Borro solo cuando la hora desde y hasta es de 0000 a 0000	
/*	if (FmTFld (fm0, HSENTRE, fila) != StrToT("0000") ||
		FmTFld (fm0, HSSAL,   fila) != StrToT("0000"))
		return;
		
	fmaux = UseSubform(fm0, EXCP, 0, fila);
	for (i = 0; i < FmFldLen(fmaux, MULTI) && !FmIsNull(fmaux, TIPFAC, i); i++) {
		FmClearFlds (fmaux, TIPFAC, OBS, i);		
	} 
}
*/

static bool ValidoClienteExcepcion(long cliente, short objetivo, short cond, short motivo, bool valida)
{
	if (!valida)
		return TRUE;

	SetKey (operac|MOTXCLIbyCODCOND, cond, motivo, cliente, objetivo);
	if (GetRecord (operac|MOTXCLIbyCODCOND, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		return TRUE;
	}

	return FALSE;
}


static bool BajaPuestoP(long cliente, int objet, int tippto, DATE dia)
{
	bool baja = TRUE;
	dbtable APUESTO = (dbtable) ERROR;
	dbcursor c_APUE = (dbcursor) ERROR;

	APUESTO = CreateAlias(operac|PUESTOS);
	c_APUE  = CreateCursor(AlInd(APUESTO, operac|PUESTOSbyCLIENTE), IO_NOT_LOCK);

	SetCursorFrom(c_APUE, cliente, objet, tippto, MIN_SHORT);
	SetCursorTo  (c_APUE, cliente, objet, tippto, MAX_SHORT);
	while(FetchCursor(c_APUE) != ERROR) {
		if ((IFld(AlFld(APUESTO, operac|PUESTOS_CANTVIG)) != 0.0 &&  IsNull(AlFld(APUESTO, operac|PUESTOS_FFINAL)) &&
			 dia >= DFld(AlFld(APUESTO, operac|PUESTOS_FINICIO))) ||
	    	(IFld(AlFld(APUESTO, operac|PUESTOS_CANTVIG)) != 0.0 && !IsNull(AlFld(APUESTO, operac|PUESTOS_FFINAL)) &&
		     DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  >= dia))
			baja = FALSE;
		if (IFld(AlFld(APUESTO, operac|PUESTOS_CANTVIG)) == 0.0       &&
			DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  != NULL_DATE &&
			DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  >= dia)
			baja = FALSE;
		if (IFld(AlFld(APUESTO, operac|PUESTOS_CANTVIG)) == 0.0       &&
			DFld(AlFld(APUESTO, operac|PUESTOS_FFINAL))  == NULL_DATE &&
			DFld(AlFld(APUESTO, operac|PUESTOS_FINICIO)) >= dia)
			baja = FALSE;
	}
	DeleteCursor(c_APUE);
	DeleteAlias(APUESTO);
	return baja;
}
