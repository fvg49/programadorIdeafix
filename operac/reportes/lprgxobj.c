/********************************************************************
*
* MODULE & VERSION : @(#)lprgxobj.c	1.6 
* DATE             : 08/07/01 
* TIME             : 12:23:25 
*
* CREATED          : 22/10/07
*
* DESCRIPTION:
*      Listar los programadores asignados por empresa, cliente y objetivo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lprgxobj.fmh"
#include "comerc.sch"
#include "operac.sch"
#include "sue.sch"
#include "billpro.h"
#include "comerc.h"
#include "operac.h"
#include "filial.h"

#define ERR_ARCHI  "No se pudo abrir el archivo!"
#define ARCHI     0
#define TERM      1
#define IMPRE     2
#define REGCABE   "Emp\tCliente\tDescripción del Cliente\tObjetivo\tDescripción del Objetivo\tFilial\tDescrición de Filial\tResponsable Programador\tNombre y Apellido\tResponsable Presentismo\tNombre y Apellido\tCantidad de Vigiladores\n"
#define REGARCH   "%d\t%ld\t%s\t%d\t%s\t%s\t%s\t%7ld\t%s\t%7ld\t%s\t%.2f\n"

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
static void ImprimirInfo(long cli, int obj, char *fil, long prog, long pres, double canVigs);
static void AbrirSalida();

/* Declaraciones globales */
form fm0;
schema comerc;
int emp;
FILE   *fp = NULL;
bool   salida;
dbcursor c_objetivo;
struct spuesto estpue;

/* Programa principal */
wcmd(lprgxobj, 1.6 07/01/08)
{
    double cantVigs = 0.0;

	fm0 = OpenForm("lprgxobj", FM_EABORT);
    
    comerc = OpenSchema("comerc", IO_EABORT);
    
  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	if (DoForm(fm0, before, after) != FM_UPDATE) return;

	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;

    if (FmIsNull(fm0, PROGDES)) {
	    c_objetivo = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);
    	SetCursorFrom(c_objetivo, emp, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_objetivo, emp, MAX_LONG, MAX_SHORT);
    }
    else {
    	c_objetivo = CreateCursor(comerc|OBJETIVObyPROGRAM, IO_NOT_LOCK);
    	SetCursorFrom(c_objetivo, FmLFld(fm0, PROGDES), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_objetivo, FmLFld(fm0, PROGHAS), MAX_LONG, MAX_SHORT);
    }	
    	
	while (FetchCursor(c_objetivo) != ERROR) {
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		 	continue;
		
		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		if (FmIsNull(fm0, PROGDES)) {    	
        	//sin inactivos y sin asignados
        	if (FmIFld(fm0, OBJINACT) == FALSE && IFld(comerc|OBJETIVO_ACTIVO) == FALSE)
            	continue;
			if(FmIFld(fm0, OBJSASIG) == FALSE && IsNull(comerc|OBJETIVO_PROGRAM))
				continue;
        }
        else {
        	//filtra otras empresas
        	if (emp != IFld(comerc|OBJETIVO_EMP))
        		continue;
           	//filtra inactivos
        	if (IFld(comerc|OBJETIVO_ACTIVO) == FALSE)
            	continue;
        	//filtra objetivos sin asignar
        	if (IsNull(comerc|OBJETIVO_PROGRAM))
            	continue;
        }	
        
	    
	    if (FmIFld(fm0, OBJCSERV) == TRUE) {
		    _SParam_PVivo parhora; 
          
            
		    InicPuestosVivos(emp, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_SHORT, NULL_SHORT, FmDFld(fm0, FECHA), 
		    				FmDFld(fm0, FECHA), FALSE, FALSE, _VALIDAR_FECINI, //Considere la fecha de inicio de la OT
		    				NULL_SHORT, TRUE, FALSE, parhora);

			VolverInicioPuestosVivos ();
			while (ProximoPuestoVivo (&estpue)) {
				if (!estpue.cpue)
					continue;
				
		  		cantVigs += (double)estpue.crvig / 100.00;
			}
			FinPuestosVivos();
	    }	
	    
	    ImprimirInfo(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), SFld(comerc|OBJETIVO_FILIAL), LFld(comerc|OBJETIVO_PROGRAM), LFld(comerc|OBJETIVO_PRESEN), cantVigs);
    }
    
    if (fp == NULL)
    	Warning("No hay datos para los parametros ingresados");

    
    DeleteCursor(c_objetivo);
    
    FinListaXusr();
}


static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case PROGHAS:
		if (FmIsNull(fm, PROGDES))	{
            FmSetFld(fm, DPROGDES, NULL_STR);
            FmSetFld(fm, DPROGHAS, NULL_STR);
            FmSetLFld(fm, PROGHAS, NULL_LONG);
			return FM_SKIP;
		 }	
		break;
	case OBJSASIG:
		if (!FmIsNull(fm, PROGDES))
			return FM_SKIP;
		break;
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lprgxobj.txt");
		break;
	case FECHA:
		if (FmIFld(fm, OBJCSERV) == TRUE)
 			FmSetDFld(fm, FECHA, Today());
 		else	{	
 			FmSetDFld(fm, FECHA, NULL_DATE);
 			return FM_SKIP;
 		}
 		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	long nroleg;

	switch (fno) {
	case EMP:
		emp = FmIFld(fm0, EMP);
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
		break;
	case PROGDES:
		if (FmKeyCode(fm) == K_META) {
			nroleg = ERROR;
			if ( (nroleg = MenuNOM(emp)) == ERROR)
				return FM_REDO;
			FmSetLFld(fm, fno, nroleg);
		}
		break;
	case PROGHAS:
	    if (!FmIsNull(fm, PROGDES) && FmIsNull(fm, PROGHAS))
	    	return FM_REDO;
		if (FmKeyCode(fm) == K_META) {
			nroleg = ERROR;
			if ( (nroleg = MenuNOM(emp)) == ERROR)
				return FM_REDO;
			FmSetLFld(fm, fno, nroleg);
		}
	   	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));

   	   	fprintf(fp, REGCABE);
	}
}

static void ImprimirInfo(long cli, int obj, char *fil, long prog, long pres, double cantVigs)
{
	static char nomprog[61], nompres[61];
    strcpy(nomprog, GetNombreLeg(emp, prog));
    strcpy(nompres, GetNombreLeg(emp, pres));

	if (fp == NULL)
		AbrirSalida();

	if (fp != NULL)
	   	fprintf(fp, REGARCH, emp, cli, GetDescCli(cli), obj, GetObjDescrip(cli, obj), fil, GetDescFilial(fil), prog, nomprog, pres, nompres, cantVigs);
}
