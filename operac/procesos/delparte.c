/********************************************************************
* MODULE & VERSION : @(#)delparte.c	1.6
* DATE             : 08/04/29
* TIME             : 13:19:15
*
* CREATED          : 23/03/07
*
* DESCRIPTION:
*             Proceso que borra inserciones en operac.parte y operac.asigh
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "comerc.sch"
#include "operac.sch"
#include "delparte.fmh"
#include "filial.h"

#define WAR_CERRADO_FILIAL   "El Parte está cerrado para la Filial %s el %.3D.\nNo podrá modificarse."

/* Funciones privadas */
static fm_status after(form, fmfield, int);
static void display(char *buffer);  
static void displaypto(char *buffer);
static void displaynroint(char *buffer);
static void LimpiarCampos();
static int  validate(void);
static int  validatepto(void);
static int  validatenroint(void);

/* Declaraciones globales */
schema ope, com;
FILE *fp1 = NULL;
form fm0;
bool borraparte = FALSE, borraexc = FALSE;
char filial[7] = {'\0'};

/* Programa principal */
wcmd(delparte, 1.6 04/29/08)
{
	fm_cmd cmd;

	fm0 = OpenForm("delparte", FM_EABORT);

	com = OpenSchema("comerc", IO_EABORT);
	ope = OpenSchema("operac", IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();

	  	if ((fp1 = fopen("/export/home/javierd/delparte.txt", "a+")) == NULL)
	  		Error("No se pudo crear el archivo de log delparte.txt");

		SetIFld(ope|ASIGH_EMP,      FmIFld(fm0, EMP));
		SetLFld(ope|ASIGH_CLIENTE,  FmLFld(fm0, CLIE));
		SetIFld(ope|ASIGH_OBJETIVO, FmIFld(fm0, OBJET));
		SetIFld(ope|ASIGH_PTOSER,   FmIFld(fm0, RTIPPTO));
		SetIFld(ope|ASIGH_PUESTO,   FmIFld(fm0, RCODINT));
		SetIFld(ope|ASIGH_NROINT,   FmIFld(fm0, RNROINT));
		SetLFld(ope|ASIGH_NROLEG,   FmLFld(fm0, NROLEG));
		SetDFld(ope|ASIGH_FECBAJ,   FmDFld(fm0, FECPARTE));
		SetDFld(ope|ASIGH_FECALT,   FmDFld(fm0, FECPARTE));
		if (GetRecord(ope|ASIGHbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR || IFld(ope|ASIGH_MOTIVO) != ALTAPARTE) {
			Warning("Los datos ingresados no corresponden\n a una inserción en el Parte Diario.\nNo podrán eliminarse.");
		}
		else {
			DelRecord(ope|ASIGH);

			borraexc   = FALSE;

			SetIFld(ope|PARTE_EMP,      FmIFld(fm0, EMP));
			SetLFld(ope|PARTE_CLIENTE,  FmLFld(fm0, CLIE));
			SetIFld(ope|PARTE_OBJETIVO, FmIFld(fm0, OBJET));
			SetDFld(ope|PARTE_DIA,      FmDFld(fm0, FECPARTE));
			SetLFld(ope|PARTE_NROLEG,   FmLFld(fm0, NROLEG));
			SetIFld(ope|PARTE_PTOSER,   FmIFld(fm0, RTIPPTO));
			SetIFld(ope|PARTE_PUESTO,   FmIFld(fm0, RCODINT));
			SetIFld(ope|PARTE_NROINT,   FmIFld(fm0, RNROINT));
			if (GetRecord(ope|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
				DelRecord(ope|PARTE);

			SetIFld(ope|EXCEPCION_EMP,      FmIFld(fm0, EMP));
			SetLFld(ope|EXCEPCION_CLIENTE,  FmLFld(fm0, CLIE));
			SetIFld(ope|EXCEPCION_OBJETIVO, FmIFld(fm0, OBJET));
			SetDFld(ope|EXCEPCION_DIA,      FmDFld(fm0, FECPARTE));
			SetLFld(ope|EXCEPCION_NROLEG,   FmLFld(fm0, NROLEG));
			SetIFld(ope|EXCEPCION_PTOSER,   FmIFld(fm0, RTIPPTO));
			SetIFld(ope|EXCEPCION_PUESTO,   FmIFld(fm0, RCODINT));
			SetIFld(ope|EXCEPCION_NROINT,   FmIFld(fm0, RNROINT));
			SetIFld(ope|EXCEPCION_CONDIC,   MIN_SHORT);
			SetIFld(ope|EXCEPCION_MOTIVO,   MIN_SHORT);
			while (GetRecord(ope|EXCEPCIONbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {
				DelRecord(ope|EXCEPCION);
				borraexc = TRUE;
			}

			fprintf(fp1, "\nELIMINACION DE PARTE - Fecha: %.3D  Hora: %.3T  Usr: %s - Vigilador: %ld %s  Fecha: %.3D  Cliente: %ld  Objetivo: %d  Puesto: %d %d %d\n",
				 Today(), Hour(), UserName(GetUid()), FmLFld(fm0, NROLEG), GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG)),
				 FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, RTIPPTO), FmIFld(fm0, RCODINT),
				 FmIFld(fm0, RNROINT));

			fprintf(fp1, "ELIMINACION DE ASIGH - Fecha: %.3D  Hora: %.3T  Usr: %s - Vigilador: %ld %s  Fecha: %.3D  Cliente: %ld  Objetivo: %d  Puesto: %d %d %d\n",
				 Today(), Hour(), UserName(GetUid()), FmLFld(fm0, NROLEG), GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG)),
				 FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, RTIPPTO), FmIFld(fm0, RCODINT),
				 FmIFld(fm0, RNROINT));

			if (borraexc)
				fprintf(fp1, "ELIMINACION DE EXCEPCIONES - Fecha: %.3D  Hora: %.3T  Usr: %s - Vigilador: %ld %s  Fecha: %.3D  Cliente: %ld  Objetivo: %d  Puesto: %d %d %d\n",
					 Today(), Hour(), UserName(GetUid()), FmLFld(fm0, NROLEG), GetNombreLeg(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG)),
					 FmDFld(fm0, FECPARTE), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, RTIPPTO), FmIFld(fm0, RCODINT),
					 FmIFld(fm0, RNROINT));
		}

		LimpiarCampos();
		fclose(fp1);
		EndTransaction();
		break;
	case FM_IGNORE:
		LimpiarCampos();
		break;
	}
}

static void LimpiarCampos()
{
	FmSetIFld(fm0, RTIPPTO,  NULL_SHORT);
	FmSetIFld(fm0, RCODINT,  NULL_SHORT);
	FmSetIFld(fm0, RNROINT,  NULL_SHORT);
	FmSetFld (fm0, RREGIM,   NULL_STR);
	FmSetFld (fm0, PCOND,    NULL_STR);
	FmSetTFld(fm0, PHSENTRE, NULL_TIME);
	FmSetTFld(fm0, PHSSAL,   NULL_TIME);
	FmSetIFld(fm0, PHSTOT,   NULL_SHORT);
	FmSetIFld(fm0, PNORMAL,  NULL_SHORT);
	FmSetIFld(fm0, PEXTRAS1, NULL_SHORT);
	FmSetIFld(fm0, PEXTRAS2, NULL_SHORT);
	FmSetIFld(fm0, PFRANCOS, NULL_SHORT);
}

static fm_status after(form fm, fmfield fno, int row)
{
	DATE fecierre, fecierrefil;
	dbcursor c_ptoser, CUR;
	char regimen[15];
	int  n;

	switch (fno) {
	case FECPARTE:
		fecierre = GetFechaCierreOpe(FmIFld(fm, EMP));

		if (FmDFld(fm, FECPARTE) <= fecierre) {
			Warning("El Parte está cerrado el %.3D.\nNo podrá eliminarse el Parte.", fecierre);
			return FM_REDO;
		}
		
		fecierrefil = GetFechaCierreFilial(filial);
        if (fecierrefil != NULL_DATE && FmDFld(fm0, FECPARTE) <= fecierrefil) {
			Warning(WAR_CERRADO_FILIAL, filial, fecierrefil);
			return FM_REDO;
		}
		break;
	case OBJET: 
		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET)));	
		break;
	case RTIPPTO:
		if (FmKeyCode(fm) == K_HELP) {
			c_ptoser = CreateCursor(com|PTOSERbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

			SetCursorFrom(c_ptoser, FmIFld(fm, EMP), FmLFld(fm, CLIE), FmIFld(fm, OBJET), MIN_SHORT);
			SetCursorTo  (c_ptoser, FmIFld(fm, EMP), FmLFld(fm, CLIE), FmIFld(fm, OBJET), MAX_SHORT);
			n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_ptoser, 4, validate, display);

			if (n >= 0)
				FmSetIFld(fm, fno, IFld(com|PTOSER_TIPPTO));
		}
		break;
	case RCODINT:
		if (FmKeyCode(fm) == K_HELP) {
			CUR = CreateCursor(ope|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);

			SetCursorFrom(CUR, FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmIFld(fm, RTIPPTO), MIN_SHORT);
			SetCursorTo  (CUR, FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmIFld(fm, RTIPPTO), MAX_SHORT);
			n = PopUpDbMenu(10, 85, "Pto Cteg  HrIni  HrFin      Dias        Regimen  CantVig CantPto  FecIni   FecFin", CUR, 4, validatepto, displaypto);

			if (n >= 0) { 
				FmSetIFld (fm, RCODINT, IFld(ope|PUESTOS_CODINT));
				FmShowFlds(fm, RCODINT, RCODINT);
			}
			DeleteCursor(CUR);
		}
		break;
	case RNROINT:
		if (FmKeyCode(fm) == K_HELP) {
			CUR = CreateCursor(ope|PARTEbyEMP, IO_NOT_LOCK|IO_CONTROL_BREAK);

			SetCursorFrom(CUR, FmIFld(fm, EMP), FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmDFld(fm, FECPARTE),
								FmLFld(fm, NROLEG), FmIFld(fm, RTIPPTO), FmIFld(fm, RCODINT), MIN_SHORT);
			SetCursorTo  (CUR, FmIFld(fm, EMP), FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmDFld(fm, FECPARTE),
								FmLFld(fm, NROLEG), FmIFld(fm, RTIPPTO), FmIFld(fm, RCODINT), MAX_SHORT);
			n = PopUpDbMenu(10, 40, "   Puesto     Hr. Desde  Hr. Hasta", CUR, 8, validatenroint, displaynroint);

			if (n > 0) { 
				FmSetIFld(fm, RNROINT, IFld(ope|PARTE_NROINT));
			}
			else
			    FmSetIFld(fm, RNROINT, 1);

			FmShowFlds(fm, RNROINT, RNROINT);
			DeleteCursor(CUR);
		}

		SetIFld(ope|PARTE_EMP,      FmIFld(fm, EMP));
		SetLFld(ope|PARTE_CLIENTE,  FmLFld(fm, CLIE));
		SetIFld(ope|PARTE_OBJETIVO, FmIFld(fm, OBJET));
		SetDFld(ope|PARTE_DIA,      FmDFld(fm, FECPARTE));
		SetLFld(ope|PARTE_NROLEG,   FmLFld(fm, NROLEG));
		SetIFld(ope|PARTE_PTOSER,   FmIFld(fm, RTIPPTO));
		SetIFld(ope|PARTE_PUESTO,   FmIFld(fm, RCODINT));
		SetIFld(ope|PARTE_NROINT,   FmIFld(fm, RNROINT));
		if (GetRecord(ope|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			GetRegimenEfectivo(FmIFld(fm, EMP), LFld(ope|PARTE_NROLEG), regimen, FmDFld(fm, FECPARTE));

			FmSetFld (fm, RREGIM,   regimen);
			FmSetFld (fm, PCOND,    SFld(ope|PARTE_CONDIC));
			FmSetTFld(fm, PHSENTRE, TFld(ope|PARTE_HORAENT));
			FmSetTFld(fm, PHSSAL,   TFld(ope|PARTE_HORASAL));
			FmSetIFld(fm, PHSTOT,   ConvHraInt(TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL)) * 100);
			FmSetIFld(fm, PNORMAL,  IFld(ope|PARTE_HSNOR));
			FmSetIFld(fm, PEXTRAS1, IFld(ope|PARTE_HS50));
			FmSetIFld(fm, PEXTRAS2, IFld(ope|PARTE_HS100FE));
			FmSetIFld(fm, PFRANCOS, IFld(ope|PARTE_HS100F));
		}
		break;
	}
	return FM_OK;
}

static int validate()
{
	if (!BajaPuesto(LFld(com|PTOSER_CLIENTE), IFld(com|PTOSER_OBJET), IFld(com|PTOSER_TIPPTO), FmDFld(fm0, FECPARTE)))
		return TRUE;

	return FALSE;
}

static void display(char * buffer)
{
	SetKey(com|TPTOSERbyTIPPTO, IFld(com|PTOSER_TIPPTO));
	GetRecord(com|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(com|PTOSER_TIPPTO), SFld(com|TPTOSER_DESCOR));
}

static int validatepto(void)
{
	if (IFld(ope|PUESTOS_CANTVIG) != 0.0 &&
	   ((IsNull(ope|PUESTOS_FFINAL)  && !IsNull(ope|PUESTOS_FINICIO) &&
		FmDFld(fm0, FECPARTE) < DFld(ope|PUESTOS_FINICIO)) ||
	   (!IsNull(ope|PUESTOS_FFINAL) &&
	   (FmDFld(fm0, FECPARTE) < DFld(ope|PUESTOS_FINICIO) || FmDFld(fm0, FECPARTE) > DFld(ope|PUESTOS_FFINAL)))))
		return FALSE;

	if (IFld(ope|PUESTOS_CANTVIG) == 0.0 && !IsNull(ope|PUESTOS_FFINAL) &&
	    (FmDFld(fm0, FECPARTE) > DFld(ope|PUESTOS_FFINAL) || FmDFld(fm0, FECPARTE) < DFld(ope|PUESTOS_FINICIO)))
		return FALSE;

	if (IFld(ope|PUESTOS_CANTVIG) == 0.0 && IsNull(ope|PUESTOS_FFINAL) &&
	    FmDFld(fm0, FECPARTE) > DFld(ope|PUESTOS_FINICIO))
		return FALSE;

	return TRUE;
}

static void displaypto(char *buffer)
{
	sprintf(buffer,"%2d %4d  %.*T  %.*T  %-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s-%-1.1s  %8.8s    %4.2f    %3.3d   %.1D %.1D",
			IFld(ope|PUESTOS_CODINT), IFld(ope|PUESTOS_PUESTO), DFMT_SEPAR,
			TFld(ope|PUESTOS_HINICIO), DFMT_SEPAR, TFld(ope|PUESTOS_HFINAL), SFld(ope|PUESTOS_DIA1),
			SFld(ope|PUESTOS_DIA2), SFld(ope|PUESTOS_DIA3), SFld(ope|PUESTOS_DIA4),
			SFld(ope|PUESTOS_DIA5), SFld(ope|PUESTOS_DIA6), SFld(ope|PUESTOS_DIA7),
			SFld(ope|PUESTOS_REGIM), (double)IFld(ope|PUESTOS_CANTVIG)/100.00,
			IFld(ope|PUESTOS_CANTPUE),DFld(ope|PUESTOS_FINICIO), DFld(ope|PUESTOS_FFINAL));
}

static int validatenroint(void)
{
	return TRUE;
}

static void displaynroint(char *buffer)
{
	sprintf(buffer,"%2d %4d %2d     %.*T      %.*T",
			IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO), IFld(ope|PARTE_NROINT),
			DFMT_SEPAR, TFld(ope|PARTE_HORAENT), DFMT_SEPAR, TFld(ope|PARTE_HORASAL));
}
