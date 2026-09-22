/********************************************************************
* MODULE & VERSION : @(#)otoper.c	1.1
* DATE             : 09/12/16
* TIME             : 14:56:06
*
* CREATED          : 27/07/98
*
* DESCRIPTION:
*             ABM de Ordenes de Trabajo de Operaciones
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* fm0   otoper.fm
* fm1   ptoper.fm
* fm2   saloper.fm
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "ianus.h"
#include "sumin.h"
#include "operac.h"
#include "billpro.h"
#include "otoper.fmh"
#include "ptoper.fmh"
#include "saloper.fmh"
#include "comerc.sch"
#include "operac.sch"
#include "sue.sch"
#include "igral.sch"
#include "prosegur.sch"
#include "filial.h"

#define MAX_CODINT	50

struct s_codint {
	int		tippto;
	int		codint;
	int		cantvig;
	char	cond[2];
	bool	proc;
} v_codint [MAX_CODINT];

/* Funciones privadas */
private fm_status after(form fm, fmfield fno, int row);
private fm_status before(form fm, fmfield fno, int row);
private fm_status aft_ptoper(form fm, fmfield fno, int row);
private fm_status bef_ptoper(form fm, fmfield fno, int row);
static  void Lectura(fm_cmd, find_mode);
private void PutInPtoser(long nroot);
private void DelPuesto(int emp, int tipcomp, int serie, char *deleg, int nroot);
private	void BorrarPuesto();
private void BorraUnPuesto(int tippto, int codint, int cantvig, char cond[]);
private void ActualizCodint(int tippto, int codint);

static int validate(void);
static void display(char *buffer);
private fm_status HelpPuesto(form fm, fmfield fno, int row);

static int validate1(void);
static void display1(char *buffer);
private fm_status HelpCateg(form fm, fmfield fno, int row);

void HelpTptoSer(form fm, fmfield fno, int row);
static int validaTptoSer(void);
static void displayTptoSer(char *buffer);
void HelpRegim(form fm, fmfield fno, int row);
static int validaRegim(void);
static void displayRegim(char *buffer);
void HelpSubRegim(form fm, fmfield fno, int row);
static int validaSubRegim(void);
static void displaySubRegim(char *buffer);

/*-----------------------* Declaraciones globales *-----------------------------*/
form fm0, fm1, fm2;
schema comerc, sue, igral, prosegur, operac;
long nroot = 0, presen = 0;
double basico, present, ticket, pread, tickad, suelad, precio, otros;
bool lectura, permiso_valido, ptime;
;
int tippue, vigi, sub_cint = 0, convenio, emp;
long   tothoras = 0;

wcmd(otoper, 1.1 12/16/09)
{
	fm_cmd cmd;

	basico = present = ticket = suelad = pread = tickad = otros = 0.0;
	lectura = FALSE;

	igral    = OpenSchema("igral",    IO_EABORT);
	sue      = OpenSchema("sue",      IO_EABORT);
	prosegur = OpenSchema("prosegur", IO_EABORT);
	comerc   = OpenSchema("comerc",   IO_EABORT);
	operac   = OpenSchema("operac",   IO_EABORT);

	fm0      = OpenForm("otoper", FM_EABORT);

	/* Esta función resuelve los problemas de linkedición. NO SACARLA!!!! */
	Compilo();

	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	permiso_valido = TRUE;

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ     : Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT: Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV: Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(comerc|OT);
		InitRecord(comerc|OBJETIVO);
		InitRecord(comerc|REFER);
		InitRecord(comerc|NPUESTO);
	case FM_UPDATE:
		BeginTransaction();
		if ( FmLFld(fm0, NROOT) == NULL_LONG)
			nroot = GetNroot(FmIFld(fm0, EMP), FmIFld(fm0, TIPCOMP), FmIFld(fm0, SERIE), FmSFld(fm0, DELEG));
		else {
			if (FmLFld(fm0, NROOT) != NULL_LONG)
				nroot = FmLFld(fm0, NROOT);
		}
		FmToDb(fm0, 0, CONTROL_FLD);
		SetLFld(comerc|OT_NROOT,  nroot);
		SetIFld(comerc|OT_TIPSER, ObjetRif(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET)) ? 1 : 2);

		if (ExisteCliObjEnGrp(GRPOBJPLANTA, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET))) {
			SetIFld(comerc|OT_ESTVTA,  1);
			SetIFld(comerc|OT_ESTADM,  1);
			SetIFld(comerc|OT_ESTOPER, 1);
		}
		else {
			SetIFld(comerc|OT_ESTVTA,  0);
			SetIFld(comerc|OT_ESTADM,  0);
			SetIFld(comerc|OT_ESTOPER, 0);
		}
		SetIFld(comerc|OT_CODSER, 1);
		PutRecord(comerc|OT);
		FreeTable(comerc|OT);

		if (cmd == FM_UPDATE)
			BorrarPuesto();

		PutInPtoser(nroot);
		EndTransaction();

		if ( FmLFld(fm0, NROOT) == NULL_LONG )
			DisplayMsg(MSG_WAIT, "SE GENERO EL NUMERO DE OT : %ld", nroot);
		break;
	case FM_DELETE:
		break;
	case FM_IGNORE:
		FmSetLFld(fm0, NROOT, NULL_LONG);
		FmSetDisplayOnly(fm0, 0, CONTROL_FLD, FALSE);
		FreeTable(comerc|OBJETIVO);
		break;
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
    FinListaXusr();
}

/*--------------------* Rutina de lectura y pasaje a pantalla *----------------------*/
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int i = 0, j = 0, k, vigi = 0;

	sub_cint = 0;

	FmToDb(fm0, 0, NROOT);
	switch(GetRecord(comerc|OTbyEMP, mode, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(OTbyEMP, mode);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	FmSFld(fm0, DCLIE, GetDescCli(LFld(comerc|OT_CLIENTE)));
	FmSFld(fm0, DOBJ,  GetObjDescrip(LFld(comerc|OT_CLIENTE),IFld(comerc|OT_OBJET)));
	DbToFm(fm0, 0, HFIN, 0);

	lectura = TRUE;

	if (IFld(comerc|OT_ESTOPER) == 1 || IFld(comerc|OT_ESTADM) == 1)
		Warning("El Comprobante no podrá modificarse. Ya ha sido autorizado");

	/*------------* Lectura del Multi principal *-------------*/
	SetIFld(comerc|PTOSER_EMP,     FmIFld(fm0, EMP));
	SetIFld(comerc|PTOSER_TIPCOMP, FmIFld(fm0, TIPCOMP));
	SetIFld(comerc|PTOSER_SERIE,   FmIFld(fm0, SERIE));	
	SetFld (comerc|PTOSER_DELEG,   FmSFld(fm0, DELEG));
	SetLFld(comerc|PTOSER_NROOT,   FmLFld(fm0, NROOT));
	SetIFld(comerc|PTOSER_TIPPTO,  MIN_SHORT);
	for (i = 0; GetRecord(comerc|PTOSERbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 5) != ERROR &&
																i < FmFldLen(fm0, MULTIOT); i++) {
		FmSetIFld(fm0, M_PTOSER,  IFld(comerc|PTOSER_TIPPTO), i);
		FmSetIFld(fm0, I_PTOSER,  IFld(comerc|PTOSER_TIPPTO), i);
		FmSetFld (fm0, M_DCOPUES, GetDescPto(IFld(comerc|PTOSER_TIPPTO)), i);
		FmSetFld (fm0, M_COND,    SFld(comerc|PTOSER_COND),   i);

		/*-------------------* Lectura de Puestos *----------------------*/
		j = 0;
		fm1 = UseSubform(fm0, M_COND, 0, i);

		SetIFld(comerc|NPUESTO_EMP,     FmIFld(fm0, EMP));
		SetIFld(comerc|NPUESTO_TIPCOMP, FmIFld(fm0, TIPCOMP));
		SetIFld(comerc|NPUESTO_SERIE,   FmIFld(fm0, SERIE));
		SetFld (comerc|NPUESTO_DELEG,   FmSFld(fm0, DELEG));
		SetLFld(comerc|NPUESTO_NROOT,   FmLFld(fm0, NROOT));
		SetIFld(comerc|NPUESTO_TIPPTO,  IFld(comerc|PTOSER_TIPPTO));
		SetIFld(comerc|NPUESTO_NRORENG, MIN_SHORT);
		for (j = 0; GetRecord(comerc|NPUESTObyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR &&
																		j < FmFldLen(fm1, MULTI); j++) {
			vigi += IFld(comerc|NPUESTO_CANTVIG);
			k = 0;
			FmSetIFld(fm1, CATEGOR, IFld(comerc|NPUESTO_PUESTO),    j);
			FmSetTFld(fm1, HINICIO, TFld(comerc|NPUESTO_HINICIO),   j);
			FmSetTFld(fm1, HFINAL,  TFld(comerc|NPUESTO_HFINAL),    j);
			FmSetFld (fm1, DIAS1,   SFld(comerc|NPUESTO_DIAS, k),   j);
			FmSetFld (fm1, DIAS2,   SFld(comerc|NPUESTO_DIAS, k+1), j);
			FmSetFld (fm1, DIAS3,   SFld(comerc|NPUESTO_DIAS, k+2), j);
			FmSetFld (fm1, DIAS4,   SFld(comerc|NPUESTO_DIAS, k+3), j);
			FmSetFld (fm1, DIAS5,   SFld(comerc|NPUESTO_DIAS, k+4), j);
			FmSetFld (fm1, DIAS6,   SFld(comerc|NPUESTO_DIAS, k+5), j);
			FmSetFld (fm1, DIAS7,   SFld(comerc|NPUESTO_DIAS, k+6), j);
			FmSetFld (fm1, REGIM,   SFld(comerc|NPUESTO_REGIM),     j);
			FmSetFld (fm1, SUBREG,  SFld(comerc|NPUESTO_SUBREG),    j);
			FmSetFld (fm1, TIPODIA, SFld(comerc|NPUESTO_TIPODIA),   j);
			FmSetLFld(fm1, SALARIO, LFld(comerc|NPUESTO_SALARIO),   j);
			FmSetIFld(fm1, CANTPUE, IFld(comerc|NPUESTO_CANTPUE),   j);
			FmSetIFld(fm1, CANTVIG, IFld(comerc|NPUESTO_CANTVIG),   j);
			FmSetFld (fm1, COND,    SFld(comerc|NPUESTO_COND),      j);
			FmSetIFld(fm1, I_CANTP, IFld(comerc|NPUESTO_CANTPUE),   j);
			FmSetFld (fm1, I_CONDC, SFld(comerc|NPUESTO_COND),      j);
			FmSetIFld(fm1, HORAPT,  IFld(comerc|NPUESTO_HORAPT),    j);
			FmSetFld (fm1, CODFREC, SFld(comerc|NPUESTO_CODFREC),   j);
			if(!IsNull(comerc|NPUESTO_CODFREC) || !IsNull(comerc|NPUESTO_HORAPT))
				FmSetIFld(fm1, PTIME, TRUE, j);
			else
				FmSetIFld(fm1, PTIME, FALSE, j);

			// Obtengo el puesto de operac por el indice secundario para actualizar el internal con
			// el codigo interno.
			SetKey(operac|PUESTOSbyPUESTO, LFld(comerc|OT_CLIENTE), LFld(comerc|OT_OBJET),
										   IFld(comerc|PTOSER_TIPPTO), IFld(comerc|NPUESTO_PUESTO),
										   TFld(comerc|NPUESTO_HINICIO), TFld(comerc|NPUESTO_HFINAL),
										   SFld(comerc|NPUESTO_DIAS, 0), SFld(comerc|NPUESTO_DIAS, 1),
										   SFld(comerc|NPUESTO_DIAS, 2), SFld(comerc|NPUESTO_DIAS, 3),
										   SFld(comerc|NPUESTO_DIAS, 4), SFld(comerc|NPUESTO_DIAS, 5),
										   SFld(comerc|NPUESTO_DIAS, 6), SFld(comerc|NPUESTO_REGIM),
										   SFld(comerc|NPUESTO_CODFREC), SFld(comerc|NPUESTO_TIPODIA));

			GetRecord(operac|PUESTOSbyPUESTO, THIS_KEY, IO_NOT_LOCK);
			FmSetIFld(fm1, I_CODINT,  IFld(operac|PUESTOS_CODINT), j);
			FmSetIFld(fm1, I_CANTVIG, IFld(comerc|NPUESTO_CANTVIG), j);
			v_codint[sub_cint].tippto  = IFld(operac|PUESTOS_TIPPTO);
			v_codint[sub_cint].codint  = IFld(operac|PUESTOS_CODINT);
			v_codint[sub_cint].cantvig = IFld(comerc|NPUESTO_CANTVIG);
			strcpy(v_codint[sub_cint].cond, SFld(comerc|NPUESTO_COND));
			v_codint[sub_cint++].proc = FALSE;
		}
	}
	FmSetLFld(fm0, TOTPERS, vigi);

	if (IFld(comerc|OT_ESTVTA) == 1 || IFld(comerc|OT_ESTOPER) == 1 || IFld(comerc|OT_ESTADM) == 1) {
		FmSetDisplayOnly(fm0, 0, HFIN, TRUE);
		FmSetDisplayOnly(fm0, HFIN,   MULTIOT, TRUE);
		FmSetDisplayOnly(fm0, TOTPERS, TOTPERS, TRUE);
	}
	
	SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm0, CLIEOT));
	SetIFld(comerc|OBJETIVO_OBJET,   FmIFld(fm0, OBJET));
	(void)GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	//en una lista cliente/objetivo valido el permiso del usuario sobre el mismo
	if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) {
		Warning("El Usuario %d - %s no tiene permisos sobre el Cliente %ld - Objetivo %d - Filial %s", GetUid(), ReadEnv("LOGNAME"), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), SFld(comerc|OBJETIVO_FILIAL));
		permiso_valido = FALSE;
	}	
}

private void PutInPtoser(long nroot)
{
	int i, j, k;

	DelPuesto(FmIFld(fm0, EMP), FmIFld(fm0, TIPCOMP), FmIFld(fm0, SERIE), FmSFld(fm0, DELEG), nroot);

	for (i=0; i<FmFldLen(fm0, MULTIOT) && !FmIsNull(fm0, M_PTOSER, i); i++) {
		fm1 = UseSubform(fm0, M_COND, 0, i);

		SetIFld(comerc|PTOSER_EMP,     FmIFld(fm0, EMP));
		SetIFld(comerc|PTOSER_TIPCOMP, FmIFld(fm0, TIPCOMP));
		SetIFld(comerc|PTOSER_SERIE,   FmIFld(fm0, SERIE));
		SetFld (comerc|PTOSER_DELEG,   FmSFld(fm0, DELEG));
		SetLFld(comerc|PTOSER_NROOT,   nroot);
		SetIFld(comerc|PTOSER_TIPPTO,  FmIFld(fm0, M_PTOSER, i));
		SetFld (comerc|PTOSER_COND,    FmSFld(fm0, M_COND, i));
		SetLFld(comerc|PTOSER_CLIENTE, FmLFld(fm0, CLIEOT));
		SetIFld(comerc|PTOSER_OBJET,   FmIFld(fm0, OBJET));
		PutRecord(comerc|PTOSER);
		FreeTable(comerc|PTOSER);

		for (j = 0; j < FmFldLen(fm1, MULTI) && !FmIsNull(fm1, CATEGOR, j); j++) {
			SetIFld(comerc|NPUESTO_EMP,     FmIFld(fm0, EMP));
			SetIFld(comerc|NPUESTO_TIPCOMP, FmIFld(fm0, TIPCOMP));
			SetIFld(comerc|NPUESTO_SERIE,   FmIFld(fm0, SERIE));
			SetFld (comerc|NPUESTO_DELEG,   FmSFld(fm0, DELEG));
			SetLFld(comerc|NPUESTO_CLIENTE, FmLFld(fm0, CLIEOT));
			SetFld (comerc|NPUESTO_OBJET,   FmSFld(fm0, OBJET));
			SetLFld(comerc|NPUESTO_NROOT,   nroot);
			SetIFld(comerc|NPUESTO_TIPPTO,  FmIFld(fm0, M_PTOSER, i));
			SetIFld(comerc|NPUESTO_NRORENG, j);
			SetIFld(comerc|NPUESTO_PUESTO,  FmIFld(fm1, CATEGOR,  j));
			SetTFld(comerc|NPUESTO_HINICIO, FmTFld(fm1, HINICIO,  j));
			SetTFld(comerc|NPUESTO_HFINAL,  FmTFld(fm1, HFINAL,   j));
			SetFld (comerc|NPUESTO_REGIM,   FmSFld(fm1, REGIM,    j));
			SetFld (comerc|NPUESTO_SUBREG,  FmSFld(fm1, SUBREG,   j));
			SetFld (comerc|NPUESTO_TIPODIA, FmSFld(fm1, TIPODIA,  j));
			SetLFld(comerc|NPUESTO_SALARIO, FmLFld(fm1, SALARIO,  j));
			SetIFld(comerc|NPUESTO_CANTPUE, FmIFld(fm1, CANTPUE,  j));
			SetIFld(comerc|NPUESTO_CANTVIG, FmIFld(fm1, CANTVIG,  j));
			SetFld (comerc|NPUESTO_COND,    FmSFld(fm1, COND,     j));
			SetFld (comerc|NPUESTO_CODFREC, FmSFld(fm1, CODFREC,  j));
			SetIFld(comerc|NPUESTO_HORAPT,  FmIFld(fm1, HORAPT,   j));

			for (k = 0; k < 7; k++)
				SetFld (comerc|NPUESTO_DIAS, FmSFld(fm1, DIAS1+k, j), k);

			AsigCodInOperac(FmLFld(fm0, CLIEOT), FmLFld(fm0, OBJET), FmIFld(fm0, M_PTOSER, i),
			 				FmIFld(fm1, CATEGOR, j), FmTFld(fm1, HINICIO, j), FmTFld(fm1, HFINAL,  j),
							FmSFld(fm1, DIAS1,   j), FmSFld(fm1, DIAS2,   j), FmSFld(fm1, DIAS3,   j),
							FmSFld(fm1, DIAS4,   j), FmSFld(fm1, DIAS5,   j), FmSFld(fm1, DIAS6,   j),
							FmSFld(fm1, DIAS7,   j), FmSFld(fm1, REGIM,   j), FmSFld(fm1, TIPODIA, j), 
							FmIFld(fm1, CANTPUE, j),
							FmIFld(fm1, CANTVIG, j), FmSFld(fm1, COND,    j), FmSFld(fm1, CODFREC, j),
							FmIFld(fm1, HORAPT,  j),
							FmDFld(fm0, FINICIO), FmDFld(fm0, FFINAL), FmTFld(fm0, HINIC), FmTFld(fm0, HFIN), NULL_SHORT);
			PutRecord(comerc|NPUESTO);                                 
			
			FreeTable(comerc|NPUESTO);
		}
	}
}

private void DelPuesto(int emp, int tipcomp, int serie, char *deleg, int nroot)
{
	SetIFld(comerc|PTOSER_EMP,     emp);
	SetIFld(comerc|PTOSER_TIPCOMP, tipcomp);
	SetIFld(comerc|PTOSER_SERIE,   serie);
	SetFld (comerc|PTOSER_DELEG,   deleg);
	SetLFld(comerc|PTOSER_NROOT,   nroot);
	SetIFld(comerc|PTOSER_TIPPTO,  NULL_SHORT);
	while (GetRecord(comerc|PTOSERbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 5) != ERROR) {
		SetIFld(comerc|NPUESTO_EMP,     emp);
		SetIFld(comerc|NPUESTO_TIPCOMP, tipcomp);
		SetIFld(comerc|NPUESTO_SERIE,   serie);
		SetFld (comerc|NPUESTO_DELEG,   deleg);
		SetLFld(comerc|NPUESTO_NROOT,   nroot);
		SetIFld(comerc|NPUESTO_TIPPTO,  IFld(comerc|PTOSER_TIPPTO));
		SetIFld(comerc|NPUESTO_NRORENG, NULL_SHORT);
		while (GetRecord(comerc|NPUESTObyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 6) != ERROR) {
			DelRecord(comerc|NPUESTO);
		}
		DelRecord(comerc|PTOSER);
	}
}

private fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	    case CLIEOT:
		   	if (permiso_valido == FALSE)
		   		return FM_SKIP;
		   	InicClientesXusr();
    		break;
	    case OBJET:
		   	if (permiso_valido == FALSE)
		   		return FM_SKIP;
		   	InicObjetivosXusr(FmLFld(fm, CLIEOT, row), FmIFld(fm, EMP, row));
            break;
	}
	return FM_OK;
}

private fm_status after(form fm, fmfield fno, int row)
{
	if(FmKeyCode(fm) == K_PROCESS){
		//no se validaron los permisos del usuario para el cliente/objetivo
		if (permiso_valido == FALSE) {
			Warning("El Usuario %d - %s no tiene permisos sobre el Cliente %ld - Objetivo %d - Filial %s", GetUid(), ReadEnv("LOGNAME"), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), SFld(comerc|OBJETIVO_FILIAL));
			return FM_REDO;			
		}	
	}
	
	if (fm != fm0)
		return FM_OK;

	switch (fno) {
	case EMP:
		emp = FmIFld(fm0, EMP);

		if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
		break;
	case DELEG:
		if (!DelegInSerie(FmIFld(fm, EMP), FmIFld(fm, TIPCOMP), FmIFld(fm, SERIE), FmSFld(fm, DELEG))) {
			FmErrMsg(fm, M_NODELEG);
			return FM_ERROR;
		}
		break;
	case FINICIO:
		if (!strcmp(FmSFld(fm, ABM), "A") || !strcmp(FmSFld(fm, ABM), "M")) {
			if (FmIsNull(fm, fno))
				return FM_REDO;
		}
		break;
	case HINIC:
		if (!strcmp(FmSFld(fm, ABM), "A") || !strcmp(FmSFld(fm, ABM), "M")) {
			if (FmIsNull(fm, fno))
				return FM_REDO;
		}
		break;
	case M_PTOSER :
		if (FmKeyCode(fm) == K_HELP)
			HelpTptoSer(fm, fno, row);

       	SetKey(comerc|TPTOXEMPbyEMP, emp, FmIFld(fm, fno, row));
       	if (GetRecord(comerc|TPTOXEMPbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
       		SetKey(comerc|TPTOSERbyTIPPTO, IFld(comerc|TPTOXEMP_TIPPTO));
			if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) != ERROR)
				FmSetFld(fm, M_DCOPUES, SFld(comerc|TPTOSER_DESCRIP), row);
       	}
       	else
       		FmSetIFld(fm, fno, NULL_SHORT, row);
		
		tippue = FmIFld(fm, fno, row);
		FmSetIFld(fm0, I_MODPTO, TRUE, row);
		break;
	case M_COND :
		fm1 = UseSubform(fm, fno, 0, row);
	
		convenio = GetConvenioPorEmp(FmIFld(fm0, EMP));
	
		FmSetIFld(fm1, I_CONV, convenio);
		FmSetIFld(fm1, I_EMP, FmIFld(fm0, EMP));
		
		FmOnKey(fm1, K_HELP, HelpCateg,  CATEGOR, CATEGOR);
		FmOnKey(fm1, K_META, HelpPuesto, CATEGOR, CATEGOR);

		DoSubform(fm0, bef_ptoper, aft_ptoper, fno, 0, row);
		FmShowFlds(fm0,TOTPERS,TOTPERS);
		break;
	case CLIEOT:
		if (FmKeyCode(fm) == K_HELP)
  			HelpCliente(fm, fno, row);

		if (ValidaClienteXusr(FmLFld(fm, CLIEOT, row)))
		  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIEOT, row)), row);
		else {
			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIEOT, row));
			FmSetLFld(fm, CLIEOT, NULL_LONG, row);
  			FmSetFld(fm, DCLIE, NULL_STR, row);
			return FM_REDO;
  		}	
		break;
	case OBJET :
		if (FmKeyCode(fm) == K_HELP)
  			HelpObjet(fm, fno, row, FmLFld(fm, CLIEOT, row));

		if (ValidaObjetivoXusr(FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
			FmSetFld(fm, DOBJ, GetObjDescrip(FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row)), row);
		else	{
			Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIEOT, row), FmIFld(fm, OBJET, row));
			FmSetIFld(fm, OBJET, NULL_SHORT, row);
			FmSetFld(fm, DOBJ, NULL_STR, row);
			return FM_REDO;
    	}
		break;
	}
	return FM_OK;
}

private fm_status aft_ptoper(form fm, fmfield fno, int row)
{
	double total;

	if (FmKeyCode(fm) == K_DEL) {
		FmSetIFld(fm, I_MODPTOG, TRUE);
		FmSetIFld(fm, I_MODPTOM, FALSE, row);
		return FM_OK;
	}
	switch (fno) {
	case CATEGOR:
		if(!FmIsNull(fm, CATEGOR, row)){
			GetSBasico(convenio, FmIFld(fm, CATEGOR, row), &basico, &ticket, &suelad, &pread, &tickad);
			total = FtoL(basico);

			SetIFld(prosegur|CATESUPL_RELACION, convenio);
			SetIFld(prosegur|CATESUPL_CODCAT,   FmIFld(fm, CATEGOR, row));
			SetIFld(prosegur|CATESUPL_CODSUPL,  MIN_SHORT);
			while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
				total += FtoL(FFld(prosegur|CATESUPL_VAL));
			FmSetLFld(fm, SALARIO, total / 10.0, row);
		}
		break;
	case AGRUPM :
	case AGRUPD:
		FmSetIFld(fm, I_MODPTOM, TRUE, row);
		FmSetIFld(fm, I_MODPTOG, TRUE);
		break;
	case SALARIO :
		if (FmKeyCode(fm) == K_META) {
			fm2 = UseSubform(fm, fno, 0, row);

			FmSetLFld(fm2, SUELDO, FtoL(basico)/10);
			FmSetLFld(fm2, TICREV, 0);

			SetIFld(prosegur|CATESUPL_RELACION, convenio);
			SetIFld(prosegur|CATESUPL_CODCAT,   FmIFld(fm, CATEGOR, row));
			SetIFld(prosegur|CATESUPL_CODSUPL,  MIN_SHORT);
			while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
				switch (IFld(prosegur|CATESUPL_CODSUPL)) {
					case PRESEN:
						FmSetLFld(fm2, PRESEN, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case TICKCAN:
						FmSetLFld(fm2, TICCA, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case TICKRES:
						FmSetLFld(fm2, TICRE, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case TICKRESV:
					case ARINREP :
					case VIATICO :
					case ADBRIGADISTA:
					case TICKPLUS: 
						FmSetLFld(fm2, TICREV, FmLFld(fm2, TICREV) + (FtoL(FFld(prosegur|CATESUPL_VAL))/10));
						break;
					case COMIDAS:
						FmSetLFld(fm2, COMIDA, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADOBJET:
						FmSetLFld(fm2, ADOBJ, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADPREOB:
						FmSetLFld(fm2, ADPRE, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADTICAOBJ:
						FmSetLFld(fm2, ADTIC, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADFUOBJ:
						FmSetLFld(fm2, ADFOBJ, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADFUPREOBJ:
						FmSetLFld(fm2, ADFPROBJ, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					case ADFUTICOBJ:
						FmSetLFld(fm2, ADFTIOBJ, FtoL(FFld(prosegur|CATESUPL_VAL))/10);
						break;
					default: 
						FmSetLFld(fm2, TICREV, FmLFld(fm2, TICREV) + (FtoL(FFld(prosegur|CATESUPL_VAL))/10));
						break;
				}
			}
			DoSubform(fm, NULLFP, NULLFP, fno, 0, row);
		}
		break;
	case COND:
		if(FmChgFld(fm)) {
			if (!strcmp(FmSFld(fm, fno, row), "B")) {
				if (FmIFld(fm, CANTPUE, row) > PuestoAsig(FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), tippue,
														  FmIFld(fm, CATEGOR, row), FmTFld(fm, HINICIO, row),
														  FmTFld(fm, HFINAL,  row), FmSFld(fm, DIAS1, row),
														  FmSFld(fm, DIAS2,   row), FmSFld(fm, DIAS3, row),
														  FmSFld(fm, DIAS4,   row), FmSFld(fm, DIAS5, row),
														  FmSFld(fm, DIAS6,   row), FmSFld(fm, DIAS7, row),
														  FmSFld(fm, REGIM,   row), FmDFld(fm0, FECREG),
														  FmLFld(fm0, NROOT))) {
					FmErrMsg(fm, M_DELPUE);
					return FM_ERROR;
				}
			}
		}
		break;
	case REGIM:	
		ptime = FmIFld(fm, PTIME, row);
	        
	    if (FmKeyCode(fm) == K_HELP)
			HelpRegim(fm, fno, row);

        SetKey(comerc|REGIMENbyREG, FmSFld(fm, fno, row), ptime);
		if (GetRecord(comerc|REGIMENbyREG, THIS_KEY, IO_NOT_LOCK) != ERROR ) {
			SetKey(comerc|EMPXREGbyEMP, emp, IFld(comerc|REGIMEN_DIAS), IFld(comerc|REGIMEN_DFRAN), IFld(comerc|REGIMEN_HSREG), ptime);
			if (GetRecord(comerc|EMPXREGbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
	           	FmSetFld(fm, fno, SFld(comerc|REGIMEN_REGIM), row);
			else
				FmSetFld(fm, fno, NULL_STR, row);
		}	
        else
           	FmSetFld(fm, fno, NULL_STR, row);

		break;
	case SUBREG:
		if (FmKeyCode(fm) == K_HELP)
			HelpSubRegim(fm, fno, row);

		SetKey(comerc|SUBREGIMbySUBREG, FmSFld(fm, fno, row));
		if (GetRecord(comerc|SUBREGIMbySUBREG, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			SetKey(comerc|REGXSUBbySUBREG, emp, SFld(comerc|SUBREGIM_SUBREG), NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			if (GetRecord(comerc|REGXSUBbySUBREG, PARTIAL_KEY|NEXT_KEY, IO_NOT_LOCK, 2) == ERROR)
				FmSetFld(fm, fno, NULL_STR);
		}	
		else
			FmSetFld(fm, fno, NULL_STR);
		break;
	}
	return FM_OK;
}

private fm_status bef_ptoper(form fm, fmfield fno, int row)
{
	switch (fno) {
	case SALARIO :
/*		GetSBasico(convenio, FmIFld(fm, CATEGOR, row), &basico, &ticket, &suelad, &pread, &tickad);
		total = FtoL(basico);

		SetIFld(prosegur|CATESUPL_RELACION, convenio);
		SetIFld(prosegur|CATESUPL_CODCAT,   FmIFld(fm, CATEGOR, row));
		SetIFld(prosegur|CATESUPL_CODSUPL,  MIN_SHORT);
		while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
			total += FtoL(FFld(prosegur|CATESUPL_VAL));

		FmSetLFld(fm, fno, total / 10.0, row);
*/
//		DisplayMsg(MSG_WAIT, "PRECIONE HOME PARA VER DETALLE DE SALARIOS");
		break;
	}
	return FM_OK;
}

static int validate(void)
{
	if (!strcmp(SFld(comerc|NPUESTO_COND), "A"))
		return TRUE;
	return FALSE;
}

static void display(char *buffer)
{
	int i = 0;

	sprintf(buffer,"%d %02d %.*T %.*T %-1.1s %-1.1s %-1.1s %-1.1s %-1.1s %-1.1s %-1.1s %-8.8s %ld %d %d",
				   IFld(comerc|NPUESTO_TIPPTO), IFld(comerc|NPUESTO_PUESTO),
				   DFMT_SEPAR, TFld(comerc|NPUESTO_HINICIO), DFMT_SEPAR, TFld(comerc|NPUESTO_HFINAL),
				   SFld(comerc|NPUESTO_DIAS, i++), SFld(comerc|NPUESTO_DIAS, i++),
				   SFld(comerc|NPUESTO_DIAS, i++), SFld(comerc|NPUESTO_DIAS, i++),
				   SFld(comerc|NPUESTO_DIAS, i++), SFld(comerc|NPUESTO_DIAS, i++),
				   SFld(comerc|NPUESTO_DIAS, i++), SFld(comerc|NPUESTO_REGIM), LFld(comerc|NPUESTO_SALARIO),
				   IFld(comerc|NPUESTO_CANTPUE),   IFld(comerc|NPUESTO_CANTVIG));
}

private fm_status HelpPuesto(form fm, fmfield fno, int row)
{
	static dbcursor CUR = NULL;
	int n, i = 0;

	CUR = CreateCursor(comerc|NPUESTObyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), MAX_SHORT, MAX_SHORT);
	n = PopUpDbMenu(10, 60, "Puestos de Trabajo", CUR, 0, validate, display);

	if (n >= 0) {
		FmSetIFld(fm, fno,     IFld(comerc|NPUESTO_PUESTO),  row);
		FmSetTFld(fm, HINICIO, TFld(comerc|NPUESTO_HINICIO), row);
		FmSetTFld(fm, HFINAL,  TFld(comerc|NPUESTO_HFINAL),  row);
		FmSetFld (fm, DIAS1,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS2,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS3,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS4,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS5,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS6,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, DIAS7,   SFld(comerc|NPUESTO_DIAS, i++), row);
		FmSetFld (fm, REGIM,   SFld(comerc|NPUESTO_REGIM),   row); 
		FmSetIFld(fm, HORAPT,  IFld(comerc|NPUESTO_HORAPT),  row);
		FmSetFld (fm, CODFREC, SFld(comerc|NPUESTO_CODFREC), row);
		if(!IsNull(comerc|NPUESTO_CODFREC) || !IsNull(comerc|NPUESTO_HORAPT))
			FmSetIFld(fm, PTIME, TRUE, row);
		else
			FmSetIFld(fm, PTIME, FALSE, row);
		FmSetLFld(fm, SALARIO, LFld(comerc|NPUESTO_SALARIO), row);
		FmSetIFld(fm, CANTPUE, IFld(comerc|NPUESTO_CANTPUE), row);
		FmSetIFld(fm, CANTVIG, IFld(comerc|NPUESTO_CANTVIG), row);
		FmSetFld (fm, COND,    "B",                          row);
		FmShowFlds(fm, fno, COND);
	}
	DeleteCursor(CUR);
}

private fm_status HelpCateg(form fm, fmfield fno, int row)
{
	static dbcursor CUR = NULL;
	int n;

	CUR = CreateCursor(sue|CATEbyCOD, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, convenio, MIN_SHORT);
	SetCursorTo  (CUR, convenio, MAX_SHORT);

	n = PopUpDbMenu(10, 110, "Categorias            Salario Sueldo Pres T.Can TRe Otros  Comi SuAd. PrAd. TAd. AdFu AdFuPr AdFuTi", CUR, 0, validate1, display1);

	if (n >= 0)
		FmSetIFld(fm, fno, IFld(sue|CATE_CODCAT), row);

	FmShowFlds(fm, fno, fno, row);
	DeleteCursor(CUR);
}

static int  validate1(void)
{
	return TRUE;
}

static void display1(char *buffer)
{
	double total = 0,   presen = 0.0, ticca = 0.0, ticre = 0.0, ticrev = 0.0, comida = 0.0, 
		   adobj = 0.0, adpre  = 0.0, adtic = 0.0, adfobj = 0.0, adfprobj = 0.0, adftiobj = 0.0;

	total = FFld(sue|CATE_SBASICO);

	SetIFld(prosegur|CATESUPL_RELACION, convenio);
	SetIFld(prosegur|CATESUPL_CODCAT,   IFld(sue|CATE_CODCAT));
	SetIFld(prosegur|CATESUPL_CODSUPL,  MIN_SHORT);

	while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		total += FFld(prosegur|CATESUPL_VAL);
		switch (IFld(prosegur|CATESUPL_CODSUPL)) {
			case PRESEN:
				presen = FFld(prosegur|CATESUPL_VAL);
				break;
			case TICKCAN:
				ticca  = FFld(prosegur|CATESUPL_VAL);
				break;
			case TICKRES:
				ticre  = FFld(prosegur|CATESUPL_VAL);
				break;
			case TICKRESV:
			case ARINREP :
			case VIATICO :
			case ADBRIGADISTA:
			case TICKPLUS: 
				ticrev += FFld(prosegur|CATESUPL_VAL);
				break;
			case COMIDAS:
				comida = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADOBJET:
				adobj  = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADPREOB:
				adpre  = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADTICAOBJ:
				adtic  = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADFUOBJ:
				adfobj = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADFUPREOBJ:
				adfprobj = FFld(prosegur|CATESUPL_VAL);
				break;
			case ADFUTICOBJ:
				adftiobj = FFld(prosegur|CATESUPL_VAL);
				break;
		}
	}
	sprintf(buffer,"%4d %-15.15s %7.2f %6.2f %4.0f %4.0f %4.0f %4.0f %6.2f %4.0f %4.0f %4.0f   %4.0f %4.0f  %4.0f",
				   IFld(sue|CATE_CODCAT), SFld(sue|CATE_ABREV), total/1000,
				   FFld(sue|CATE_SBASICO)/1000, presen/1000, ticca/1000, ticre/1000, ticrev/1000, comida/1000, 
				   adobj/1000, adpre/1000, adtic/1000, adfobj/1000, adfprobj/1000, adftiobj/1000);
}

private void BorrarPuesto()
{
	int i, j, k;

	for (i = 0; i < FmFldLen(fm0, MULTIOT) && !FmIsNull(fm0, M_PTOSER, i); i++) {
		// Si se modifico el del primer form tengo que borrar todos sino tengo que 
		// recorrer los del subformulario y detectar cuales de esos fueron los que se modificaron 
		// para borrar esos.

		if (FmIFld(fm0, I_MODPTO, i)) { //Borro todos los puestos
			fm1 = UseSubform(fm0, M_COND, 0, i);
			for (k = 0; k < sub_cint; k++) {
				BorraUnPuesto(v_codint[k].tippto, v_codint[k].codint, v_codint[k].cantvig, v_codint[k].cond);
			}
		}
		else {
			if (FmIFld(fm0, I_MODSUB, i)) { // Verifico en el subform cual es el que se modifico y lo borro
				fm1 = UseSubform(fm0, M_COND, 0, i);
				for (j = 0; j < FmFldLen(fm1, MULTI) && !FmIsNull(fm1, CATEGOR, j); j++) {
					for (k = 0; k < sub_cint; k++) {
						if (FmIFld(fm1, I_CODINT, j) == v_codint[k].codint) {
							ActualizCodint(FmIFld(fm0, I_PTOSER, i), FmIFld(fm1, I_CODINT, j));
						}
					}
				}
			}
		}
	}
	for (i = 0; i< sub_cint ; i++) {
		if (!v_codint[i].proc) {
			BorraUnPuesto(v_codint[i].tippto, v_codint[i].codint, v_codint[i].cantvig, v_codint[i].cond);
		}
	}
}

private void BorraUnPuesto(int tippto, int codint, int cantvig, char cond[])
{
	SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIEOT), FmIFld(fm0, OBJET), tippto, codint);
	if(GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_LOCK)!=ERROR){
		if (IFld(operac|PUESTOS_CANTVIG) == cantvig)
		   	DelRecord(operac|PUESTOS);
		else {
			// segun la condicion B=sumo o A=resto
			if (strcmp(cond, "A") == 0)
				SetIFld(operac|PUESTOS_CANTVIG, IFld(operac|PUESTOS_CANTVIG) - cantvig);
			else
				SetIFld(operac|PUESTOS_CANTVIG, IFld(operac|PUESTOS_CANTVIG) + cantvig);
				PutRecord(operac|PUESTOS);
		} 
		ActualizCodint(tippto, codint);
	}
}

private void ActualizCodint(int tippto, int codint)
{
	int i;

	for (i = 0; i < sub_cint; i++) {
		if (v_codint[i].tippto == tippto && v_codint[i].codint == codint) {
			v_codint[i].proc = TRUE;
			break;
		}
	}
}

void HelpTptoSer(form fm, fmfield fno, int row)
{
	static dbcursor CUR = (dbcursor)NULL;
	int n;

	CUR = CreateCursor(comerc|TPTOXEMPbyEMP, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, FmIFld(fm0, EMP), MIN_SHORT);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), MAX_SHORT);

	n = PopUpDbMenu(10, 89, "Cód. Pto. Serv. - Descripción                    ", CUR, 0, validaTptoSer, displayTptoSer);
	if (n >= 0) {
		FmSetIFld(fm, fno,     IFld(comerc|TPTOSER_TIPPTO), row);
		FmSetFld(fm, fno + 1,  SFld(comerc|TPTOSER_DESCRIP), row);
		FmShowFlds(fm, fno, fno + 1, row);
	}
	DeleteCursor(CUR);
}

static int validaTptoSer(void)
{
	return TRUE;
}

static void displayTptoSer(char *buffer)
{                     
	char desc[31] = {'\0'};
	
	
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(comerc|TPTOXEMP_TIPPTO));
	if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strncpy(desc, SFld(comerc|TPTOSER_DESCRIP), 30);
	else
		strcpy(desc, NULL_STR);
	
	sprintf(buffer, "%d               %s", IFld(comerc|TPTOXEMP_TIPPTO), desc);
}

void HelpRegim(form fm, fmfield fno, int row)
{
	static dbcursor CUR = (dbcursor)NULL;
	int n;

	CUR = CreateCursor(comerc|EMPXREGbyEMP, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, FmIFld(fm0, EMP), MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);

	n = PopUpDbMenu(10, 89, "Regimen          Descripción                    ", CUR, 0, validaRegim, displayRegim);
	if (n >= 0) {
		FmSetFld(fm, fno,      SFld(comerc|REGIMEN_REGIM), row);
		FmShowFlds(fm, fno, fno, row);
	}
	DeleteCursor(CUR);
}

static int validaRegim(void)
{
	if (ptime != IFld(comerc|EMPXREG_PARTIME))
		return FALSE;	

	return TRUE;
}

static void displayRegim(char *buffer)
{                     
	char desc[31] = {'\0'};
	
	SetKey(comerc|REGIMENbyDIAS, IFld(comerc|EMPXREG_DIAS), IFld(comerc|EMPXREG_DFRAN), IFld(comerc|EMPXREG_HSREG), IFld(comerc|EMPXREG_PARTIME));
	if (GetRecord(comerc|REGIMENbyDIAS, THIS_KEY, IO_NOT_LOCK) != ERROR )
		strncpy(desc, SFld(comerc|REGIMEN_DESCRIP), 30);
	else
		strcpy(desc, NULL_STR);
	 
	sprintf(buffer, "%-8s        %s", SFld(comerc|REGIMEN_REGIM), desc);
}

void HelpSubRegim(form fm, fmfield fno, int row)
{
	static dbcursor CUR = (dbcursor)NULL;
	int n;

	CUR = CreateCursor(comerc|REGXSUBbyEMP, IO_NOT_LOCK|IO_CONTROL_BREAK);

	SetCursorFrom(CUR, FmIFld(fm0, EMP), MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, LOW_VALUE);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, HIGH_VALUE);

	n = PopUpDbMenu(10, 89, "Subregimen          Descripción                    ", CUR, 0, validaSubRegim, displaySubRegim);
	if (n >= 0) {
		FmSetFld(fm, fno,      SFld(comerc|REGXSUB_SUBREG), row);
		FmShowFlds(fm, fno, fno, row);
	}
	DeleteCursor(CUR);
}

static int validaSubRegim(void)
{
	if (ptime != IFld(comerc|REGXSUB_PARTIME))
		return FALSE;	

	return TRUE;
}

static void displaySubRegim(char *buffer)
{                     
	char desc[31] = {'\0'};
	
	
	SetKey(comerc|SUBREGIMbySUBREG, SFld(comerc|REGXSUB_SUBREG));
	if (GetRecord(comerc|SUBREGIMbySUBREG, THIS_KEY, IO_NOT_LOCK) != ERROR )
		strncpy(desc, SFld(comerc|SUBREGIM_DESCRIP), 30);
	else
		strcpy(desc, NULL_STR);
	 
	sprintf(buffer, "%-8s        %s", SFld(comerc|SUBREGIM_SUBREG), desc);
}

