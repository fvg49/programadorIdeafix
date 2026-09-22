/********************************************************************
*
* MODULE & VERSION : @(#)ctrlpar.c	1.5 
* DATE             : 05/01/18 
* TIME             : 11:53:04 
*
* CREATED          : 22/05/2000
*
* DESCRIPTION:
*      Compara Cantidad Horas Trabajadas Vs. Cantidad Detalle de Horas
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "ctrlpar.fmh"
#include "ctrlpar.rph"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"

/* Defines */
#define _MSG_MALHS    "Total de Horas Trabajadas diferente al Detalle de Horas"
#define _MSG_MALHSNOR "Horas Normales mayor a las permitidas por Regimen"

/* Funciones privadas */
void ControlarHs(long cliente, int objet);
void Agregar(char *msg);
bool ValidarDetalleHoras();
bool ValidarHorasNormales();
void AbrirReporte();
void SetearCabArch();
void ImprimirCabecera();
void ImprimirMalHs();

/* Declaraciones globales */
FILE   *fp;
form   fm0;
report rp0 = ERROR;
schema comerc, operac;
char   regimen[15];
short  horastra, hstot;

wcmd(ctrlpar, 1.5 01/18/05)
{
	fm_cmd cmd;
	DATE fecha;
	int i = 0;
	char buffer[50];
	dbcursor c_PARTE, c_OBJ;

	fm0    = OpenForm  ("ctrlpar", FM_EABORT);
	comerc = OpenSchema("comerc",  IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT) {
		switch (cmd) {
		case FM_ADD:
		case FM_UPDATE:
			AbrirReporte();

			c_OBJ = CreateCursor(comerc|OBJETIVO, IO_NOT_LOCK);

			SetCursorFrom(c_OBJ, FmIsNull (fm0, CLIED) ? MIN_LONG: FmLFld(fm0, CLIED) , FmIsNull (fm0, OBJETD) ? MIN_SHORT :  FmIFld(fm0, OBJETD));
			SetCursorTo  (c_OBJ, FmIsNull (fm0, CLIEH) ? MAX_LONG: FmLFld(fm0, CLIEH) , FmIsNull (fm0, OBJETH) ? MAX_SHORT :  FmIFld(fm0, OBJETH));
			while (FetchCursor(c_OBJ) != ERROR) {
				i = 0;

				sprintf(buffer, "Procesando Cliente %ld Objetivo %d", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
				FmSetFld(fm0, COMENT, buffer);
				WiRefresh();

				ControlarHs(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			}
			FmSetFld(fm0, COMENT, NULL_STR);

			if (rp0 != ERROR)
				CloseReport(rp0);
			break;
		case FM_IGNORE:
			break;
		}
	}
}

void ControlarHs(long cliente, int objet)
{
	static dbcursor CUR;

	if (!CUR)
		CUR = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

	SetCursorFrom(CUR, FmIFld(fm0, EMP), cliente, objet, FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (CUR, FmIFld(fm0, EMP), cliente, objet, FmDFld(fm0, FHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(CUR) != ERROR) {
		if (ValidarDetalleHoras())
			ImprimirMalHs();

//		if (ValidarHorasNormales())
//			ImprimirMalHs();
	}
}

//Devuelve TRUE : Si las horas trabajadas son diferente a la suma de las horas del detalle del parte.
bool ValidarDetalleHoras()
{
	horastra, hstot = 0;

	horastra = ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;
	hstot   += IFld(operac|PARTE_HSNOR)   == NULL_SHORT ? 0 : IFld(operac|PARTE_HSNOR);
	hstot   += IFld(operac|PARTE_HS50)    == NULL_SHORT ? 0 : IFld(operac|PARTE_HS50);
	hstot   += IFld(operac|PARTE_HS100F)  == NULL_SHORT ? 0 : IFld(operac|PARTE_HS100F);
	hstot   += IFld(operac|PARTE_HS100FE) == NULL_SHORT ? 0 : IFld(operac|PARTE_HS100FE);

	if (horastra == hstot)
		return FALSE;

	if (hstot == 0 && !FmIFld(fm0, VERHS))
		return FALSE;

	return TRUE;
}

//Devuelve TRUE : Si las horas normales del Parte son mayor a las horas normales del regimen.
bool ValidarHorasNormales()
{
	GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), regimen, DFld(operac|PARTE_DIA));

	SetFld(comerc|REGIMEN_REGIM, regimen);
	if (GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR && IFld(comerc|REGIMEN_PARTIME))
		return FALSE;

//	if (!strcmp(regimen, REG_ESP_1))
//		return FALSE;

	if (IFld(operac|PARTE_HSNOR) < GetHsNormales(regimen, IFld(comerc|REGIMEN_PARTIME)))
		return FALSE;

	return TRUE;
}

void AbrirReporte()
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
			Error("No se pudo abrir el archivo.");
		else
			SetearCabArch();
	}
	else {
		rp0 = OpenReport("ctrlpar", RP_EABORT|RP_NOBEGIN);

		//Si la salida es Impresora
		if ( *FmSFld(fm0, SALIDA) == 'I') {
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );
		}
		//Si la salida es Terminal
		if ( *FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

		BeginReport(rp0, 1, NULL_STR);

		ImprimirCabecera();
	}
}

void SetearCabArch()
{
	fprintf(fp, "Cliente\tRazon Social\tObjetivo\tDescrip. Obj.\tLegajor\tNombre y Apellido\tPuesto\t\t\tRegimen\tFecha\tHoras Trabajadas\tHn\tH50\tH100F\tH100FE\tTotal\n");
}

void ImprimirCabecera()
{
	RpSetDFld(rp0, R_FDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, R_FHASTA, FmDFld(fm0, FHASTA));

	RpSetLFld(rp0, R_CLID,  FmLFld(fm0, CLIED));
	RpSetLFld(rp0, R_CLIH,  FmLFld(fm0, CLIEH));

	RpSetIFld(rp0, R_OBJD,  FmIFld(fm0, OBJETD));
	RpSetIFld(rp0, R_OBJH,  FmIFld(fm0, OBJETH));
}

void ImprimirMalHs()
{
	GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), regimen, DFld(operac|PARTE_DIA));

	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%d\t%d\t%s\t%s\t%.1D\t%d\t%d\t%d\t%d\t%d\t%d\n",
				LFld(operac|PARTE_CLIENTE),  GetDescCli(LFld(operac|PARTE_CLIENTE)),
				IFld(operac|PARTE_OBJETIVO), GetObjDescrip(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)),
				LFld(operac|PARTE_NROLEG),   GetNombreLeg(FmIFld(fm0, EMP), LFld (operac|PARTE_NROLEG)),
				IFld(operac|PARTE_PTOSER),   IFld(operac|PARTE_PUESTO), GetDescPto(IFld(operac|PARTE_PTOSER)),
				regimen, DFld(operac|PARTE_DIA), horastra,
				IFld(operac|PARTE_HSNOR),    IFld(operac|PARTE_HS50), IFld(operac|PARTE_HS100F), 
				IFld(operac|PARTE_HS100FE),  hstot);
	}
	else {
		RpSetLFld(rp0, RCLI,      LFld(operac|PARTE_CLIENTE));
		RpSetFld (rp0, RDCLI,     GetDescCli(LFld(operac|PARTE_CLIENTE)));
		RpSetIFld(rp0, ROBJ,      IFld(operac|PARTE_OBJETIVO));
		RpSetFld (rp0, RDOBJ,     GetObjDescrip(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)));
		RpSetLFld(rp0, RLEG,      LFld(operac|PARTE_NROLEG));
		RpSetFld (rp0, R_APENOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld (operac|PARTE_NROLEG)));
		RpSetIFld(rp0, R_PTOSER,  IFld(operac|PARTE_PTOSER));
		RpSetIFld(rp0, R_PUESTO,  IFld(operac|PARTE_PUESTO));
		RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(operac|PARTE_PTOSER)));
		RpSetFld (rp0, R_REGIM,   regimen);
		RpSetDFld(rp0, R_DIA,     DFld(operac|PARTE_DIA));
		RpSetIFld(rp0, R_HSTRAB,  horastra);
		RpSetIFld(rp0, R_HSNOR,   IFld(operac|PARTE_HSNOR));
		RpSetIFld(rp0, R_HS50,    IFld(operac|PARTE_HS50));
		RpSetIFld(rp0, R_HS100F,  IFld(operac|PARTE_HS100F));
		RpSetIFld(rp0, R_HS100FE, IFld(operac|PARTE_HS100FE));
		RpSetIFld(rp0, R_HSTOTAL, hstot);
//		RpSetFld (rp0, R_MSG,     _MSG_MALHS);
//		RpSetFld (rp0, R_MSG,     _MSG_MALHSNOR);
		DoReport(rp0, LINEA);
	}
}
