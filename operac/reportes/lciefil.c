/********************************************************************
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : 23/04/08
*
* DESCRIPTION:
*             Listado de Cierre por Filial
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lciefil.fmh"
#include "lciefil.rph"
#include "operac.sch"
#include "comerc.sch"
#include "operac.h"
#include "comerc.h"
#include "filial.h"

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void AbrirSalida();
static void	ImprimirLinea();	

/* Declaraciones globales */
FILE   *fp;
form   fm0;
report rp0;
schema operac, comerc;
bool datos;

/* Programa principal */
wcmd(lciefil, %I% %G%)
{
	dbcursor c_ciefil;
	
	fm0    = OpenForm("lciefil",  FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	datos = FALSE;
	
	while (DoForm(fm0, before, after) != FM_UPDATE) return;
	
	c_ciefil = CreateCursor(operac|CIEFILbyFILIAL, IO_NOT_LOCK);
	
	
	if (FmIsNull(fm0, FILDES)) {
		SetCursorFrom(c_ciefil, LOW_VALUE, MIN_DATE, MIN_DATE, MIN_TIME);
		SetCursorTo  (c_ciefil, HIGH_VALUE, MAX_DATE, MAX_DATE, MAX_TIME);
	}
	else {
		SetCursorFrom(c_ciefil, FmSFld(fm0, FILDES), FmDFld(fm0, FECHAD), MIN_DATE, MIN_TIME);
		SetCursorTo  (c_ciefil, FmSFld(fm0, FILHAS), FmDFld(fm0, FECHAH), MAX_DATE, MAX_TIME);
	}
	
	while(FetchCursor(c_ciefil) != ERROR) {
		if (DFld(operac|CIEFIL_FECCIE) < FmDFld(fm0, FECHAD) || DFld(operac|CIEFIL_FECCIE) > FmDFld(fm0, FECHAH))
        	continue;
	    if (!ValidaFilialXusr(SFld(operac|CIEFIL_FILIAL)))
	    	continue;
        
	    if (datos == FALSE)
		   	AbrirSalida();
		ImprimirLinea();	
	}	
    
    if (datos == FALSE)
    	Warning("No hay datos para los parametros ingresados");
    else {
		if (!strcmp(FmSFld(fm0, SALIDA), "A"))
			fclose(fp);
		else
			CloseReport(rp0);
    }	
}

static void ImprimirLinea()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		fprintf(fp, "%s\t%s\t%.3D\t%ld\t%.3D\t%.3T\t%B\t%ld\t%.3D\t%.3T\n",
		SFld(operac|CIEFIL_FILIAL), GetDescFilial(SFld(operac|CIEFIL_FILIAL)), DFld(operac|CIEFIL_FECCIE), LFld(operac|CIEFIL_IDCARGA),
		DFld(operac|CIEFIL_FECCARGA), TFld(operac|CIEFIL_HORACARGA), IFld(operac|CIEFIL_REVERTIDO), LFld(operac|CIEFIL_IDREVER), 
		DFld(operac|CIEFIL_FECREVER), TFld(operac|CIEFIL_HORAREVER));
	}
	else {
	    RpSetFld (rp0, RFILIAL, SFld(operac|CIEFIL_FILIAL));
	    RpSetFld (rp0, RDESCRIP, GetDescFilial(SFld(operac|CIEFIL_FILIAL)));
	    RpSetDFld(rp0, RFECCIE, DFld(operac|CIEFIL_FECCIE));
	    RpSetLFld(rp0, RIDCARGA, LFld(operac|CIEFIL_IDCARGA));
	    RpSetDFld(rp0, RFECCARGA, DFld(operac|CIEFIL_FECCARGA));
	    RpSetTFld(rp0, RHORACARGA, TFld(operac|CIEFIL_HORACARGA));
	    RpSetIFld(rp0, RREVERTIDO, IFld(operac|CIEFIL_REVERTIDO));
	    RpSetLFld(rp0, RIDREVER, LFld(operac|CIEFIL_IDREVER));
	    RpSetDFld(rp0, RFECREVER, DFld(operac|CIEFIL_FECREVER));
	    RpSetTFld(rp0, RHORAREVER, TFld(operac|CIEFIL_HORAREVER));
	    
	    DoReport(rp0, LINFIL);
	}	
}
 
static void AbrirSalida()
{
	datos = TRUE;
	
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));

		fprintf(fp, "Filial Desde %s %s\tFilial Hasta %s %s\tFechas Desde %.3D\tFecha Hasta %.3D\n",
			FmSFld(fm0, FILDES), FmSFld(fm0, DFILDES), 	FmSFld(fm0, FILHAS), FmSFld(fm0, DFILHAS), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH));

		fprintf(fp, "Filial\tDescrición\tFecha Cierre\tUsr que Cerró\tFecha de Carga\tHora Carga\tRevertido\tUsr que Revirtió\tFecha Reversión\tHora Reversión\n");
	}
	else {
		rp0 = OpenReport("lciefil", RP_NOBEGIN|RP_EABORT, 1);
		RpSetOutput(rp0, (!strcmp(FmSFld(fm0, SALIDA), "I") ? RP_IO_DEFAULT : RP_IO_TERM), NULL_STR);

		if (BeginReport(rp0, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
		
		RpSetFld(rp0, RFILDES, FmSFld(fm0, FILDES));
		RpSetFld(rp0, RDFILDES, FmSFld(fm0, DFILDES));
		RpSetFld(rp0, RFILHAS, FmSFld(fm0, FILHAS));
		RpSetFld(rp0, RDFILHAS, FmSFld(fm0, DFILHAS));
		RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
		RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));
		
		DoReport(rp0, ENCAB);
		
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FILDES:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
		else
			if (!FmIsNull(fm0, fno) && !ValidaFilialXusr(FmSFld(fm0, fno)))
				return FM_REDO;
	break;
	case FILHAS:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
		else
			if (!FmIsNull(fm0, fno) && !ValidaFilialXusr(FmSFld(fm0, fno)))
				return FM_REDO;
	break;
	}
	return FM_OK;
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case FILDES:
	break;
	case FILHAS:
		if (FmIsNull(fm, FILDES))
			return FM_SKIP;
	break;
	}
	return FM_OK;
}
