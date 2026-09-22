/********************************************************************
* MODULE & VERSION : @(#)ptoven.c	1.3
* DATE             : 12/10/16
* TIME             : 12:41:23
*
* CREATED          : 03/03/05
*
* DESCRIPTION:
*             Listado de vigiladores asignados por puestos vencidos.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "ptoven.fmh"
#include "ptoven.rph"
#include "filial.h"

/* Funciones privadas */
static fm_status after(form, fmfield, int);
static fm_status before(form, fmfield, int);
static void Proceso();
static void AbrirSalida();
static void	ArmarArchivo();
static void	ArmarListado();

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema ope, bill;
char buf[100];

/* Programa principal */
wcmd(ptoven, 1.3 10/16/12)
{
	fm0  = OpenForm("ptoven",   FM_EABORT);
	ope  = OpenSchema("operac", IO_EABORT);
	bill = OpenSchema("bill",   IO_EABORT);

	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while(DoForm(fm0, before, after) != FM_EXIT) {

		AbrirSalida();
		Proceso();

		if (strcmp(FmSFld(fm0, SALIDA), "A"))
			CloseReport(rp0);
		else
			fclose(fp);
			
   		FmSetFld(fm0, COMENT, "");
   		WiMsg("El reporte ha finalizado correctamente");
   }
   
   FinObjetivosXusr();   
   FinClientesXusr();
   FinListaXusr();
}

static void Proceso()
{
	dbcursor c_puesto, c_asig;

	c_puesto = CreateCursor(ope|PUESTOSbyCLIENTE, IO_NOT_LOCK);

	if (*FmSFld(fm0, OPCION) == 'V') {
		c_asig = CreateCursor(ope|ASIGbyNROLEG, IO_NOT_LOCK);
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, VIGID), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, VIGIH), MAX_LONG, MAX_SHORT);
	}
	else {
		c_asig = CreateCursor(ope|ASIGbyEMP, IO_NOT_LOCK);
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), MIN_LONG, MIN_SHORT,
							  MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), MAX_LONG, MAX_SHORT,
							  MAX_SHORT, MAX_SHORT);
	}

	while (FetchCursor(c_asig) != ERROR) {
		if (*FmSFld(fm0, OPCION) == 'V') {
			sprintf (buf, "Procesando VIgilador %ld", LFld(ope|ASIG_NROLEG));
			FmSetFld (fm0, COMENT, buf);
			WiRefresh();
		}
		else {
			sprintf (buf, "Procesando Cliente %ld Objetivo %d", LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO));
			FmSetFld (fm0, COMENT, buf);
			WiRefresh();
		}
		
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO)))
		       	continue;
		
		if (!ValidaFilial(LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		SetLFld(ope|PUESTOS_CLIENTE, LFld(ope|ASIG_CLIENTE));
		SetIFld(ope|PUESTOS_OBJET,   IFld(ope|ASIG_OBJETIVO));
		SetIFld(ope|PUESTOS_TIPPTO,  IFld(ope|ASIG_PTOSER));
		SetIFld(ope|PUESTOS_CODINT,  IFld(ope|ASIG_PUESTO));
		if (GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (!IsNull(ope|PUESTOS_FFINAL) ||
			   (IFld(ope|PUESTOS_CANTPUE) == 0 && IFld(ope|PUESTOS_CANTVIG) == 0)) {

				SetLFld(bill|CLIENTE_CLIENTE, LFld(ope|PUESTOS_CLIENTE));
				GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

				if (*FmSFld(fm0, SALIDA) != 'A') {
			 		ArmarListado();
				}
				else
					ArmarArchivo();
			}
		}
	}
}

static void	ArmarListado()
{
	RpSetLFld(rp0, R_LEGAJO,  LFld(ope|ASIG_NROLEG));
	RpSetFld (rp0, R_APYNOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(ope|ASIG_NROLEG)));
	RpSetLFld(rp0, R_CLI,     LFld(ope|PUESTOS_CLIENTE));
	RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, R_OBJ,     IFld(ope|PUESTOS_OBJET));
	RpSetFld (rp0, R_DOBJ,    GetObjDescrip(LFld(ope|PUESTOS_CLIENTE), IFld(ope|PUESTOS_OBJET)));
	RpSetIFld(rp0, R_PTOSER,  IFld(ope|PUESTOS_TIPPTO));
	RpSetIFld(rp0, R_PUESTO,  IFld(ope|PUESTOS_CODINT));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(ope|ASIG_PTOSER)));
	RpSetDFld(rp0, R_FINICIO, DFld(ope|PUESTOS_FINICIO));
	RpSetDFld(rp0, R_FFINAL,  DFld(ope|PUESTOS_FFINAL));
	RpSetFld(rp0, R_FILIAL, GetFilialDeObj(FmIFld(fm0, EMP), (ope|PUESTOS_CLIENTE), IFld(ope|PUESTOS_OBJET)));
	DoReport (rp0, LINEA);
}

static void	ArmarArchivo()
{
	fprintf(fp, "%ld\t%s\t%ld\t%s\t%d\t%s\t%d\t%d\t%s\t%.3D\t%.3D\t%s\n",
			LFld(ope|ASIG_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(ope|ASIG_NROLEG)),
			LFld(ope|PUESTOS_CLIENTE), SFld(bill|CLIENTE_RAZSOC), IFld(ope|PUESTOS_OBJET),
			GetObjDescrip(LFld(ope|PUESTOS_CLIENTE), IFld(ope|PUESTOS_OBJET)),
			IFld(ope|PUESTOS_TIPPTO), IFld(ope|PUESTOS_CODINT), GetDescPto(IFld(ope|ASIG_PTOSER)),
			DFld(ope|PUESTOS_FINICIO), DFld(ope|PUESTOS_FFINAL), GetFilialDeObj(FmIFld(fm0, EMP), (ope|PUESTOS_CLIENTE), IFld(ope|PUESTOS_OBJET)));
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));

		fprintf(fp, "Vigilador\tNombre\tCliente\tRazon Social\tObjetivo\tDescripcion\tPuesto\t\tDescripcion Pto\tFecha Inicio\tFecha Final\tFilial\n");
	}
	else {
		rp0 = OpenReport("ptoven", RP_EABORT|RP_NOBEGIN);

		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);

		BeginReport(rp0, 1, NULL_STR);
	}
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
	case OPCION :
		switch(*FmSFld(fm, fno)) {
		case 'C' :
			FmSetFld (fm0, APYNOMD, NULL_STR);
			FmSetFld (fm0, APYNOMH, NULL_STR);
			break;
		case 'V' :
			FmSetFld (fm0, DCLID, NULL_STR);
			FmSetFld (fm0, DCLIH, NULL_STR);
			FmSetFld (fm0, DOBJD, NULL_STR);
			FmSetFld (fm0, DOBJH, NULL_STR);
			break;
		}
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
