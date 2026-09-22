/********************************************************************
*
* MODULE & VERSION : @(#)recapar.c	1.46 
* DATE             : 08/08/25 
* TIME             : 16:00:22 
*
* CREATED          : 17/09/98
*
* DESCRIPTION:
*      Generación del Parte Diario.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* MODIFICACION: 04/12/2000 Cambio el calculo de donde debe cubrir un franquero cuando el regimen es 4x2x12
*	Se modifico la funcion AsigHora
* 31/05/2002 - Se agrego la opcion de imprimir o grabar la generacion del parte. 
*
* MODIFICACION: 24/10/2003 Se creo GenParteH que es iguala GenParte pero lee ASIGH
*               No estaba regenerando el parte para los legajos que ya estaban en asigh.
*
* MODIFICACION: 20/01/2004 Funcion GetServicioObj. Se usa para filtrar los objetivos brigadas para que
*               no pida confirmacion de regeneracion del parte. Por eso se valida solo cuando se lee
*               asigh porque los brigadistas se insertan en obj brigadas y solo aparecen en asigh.
*               No se asignan en obj brigadas. El unico obj brigada con vigiladores asignados es el
*               cliente 1016 / 1 y si se debe validar.
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "opedef.h"
#include "ambiente.h"
#include "sue.sch"
#include "operac.sch"
#include "asist.sch"
#include "recapar.fmh"

//#define DEBUG	1

static fm_status after(form, fmfield, int);
static fm_status before(form, fmfield, int);
/* Declaraciones globales */
schema   operac, sue;
form     fm0;
char g_prog[20];
dbcursor c_sue;

/* Programa principal */
wcmd(recapar, 1.46 08/25/08)
{

	fm_cmd cmd;
	sprintf(g_prog, "%s", argv[0]);
	
	fm0     = OpenForm("recapar", FM_EABORT);


	operac  = OpenSchema("operac", IO_EABORT);
	sue     = OpenSchema("sue", IO_EABORT);
	
	    c_sue = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
    
    while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) { 
	case FM_UPDATE:                                              
	
		SetCursorFrom (c_sue, FmIFld(fm0, EMP), FmLFld(fm0, VIGIDESD));
		SetCursorTo   (c_sue, FmIFld(fm0, EMP), FmLFld(fm0, VIGIHAST));

		while(FetchCursor(c_sue) != ERROR)
			RecalculaPartePer(FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), fm0, COMENT, g_prog); 

		break;
	}
	CloseAllSchemas();

}

static fm_status before(form fm, fmfield fno, int row)
{
switch (fno) {
		case EMP:
            break;
	}
	return FM_OK;
	
}

static fm_status after(form fm, fmfield fno, int row)
{

	switch (fno) {
	case EMP:
	    break;
	}
	return FM_OK;

}

