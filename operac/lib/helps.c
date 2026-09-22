/*********************************************************************************
*
* MODULE & VERSION : @(#)helps.c	1.1 
* DATE             : 02/06/13 
* TIME             : 15:31:19 
*
* CREATED          : 30/07/98
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*void HelpVendedor		Ayuda de BILLPRO.VENDEDORES. No tiene en cuenta el equipo

**********************************************************************************/
#include <ideafix.h>
#include "billpro.sch"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "operac.h"

/* Declaraciones de Funciones Privadas */
private bool validate();
private void display(char * buffer);
private void displayObj(char * buffer);
private void displayRes(char * buffer);
        

void HelpPtoSer(form fm, fmfield fno, int char * deleg)
{
	schema old, comerc;
	dbcursor c_VEND;
	int n = 0;

	old = CurrentSchema();

	comerc = OpenSchema("comerc", IO_EABORT);

	SwitchToSchema(comerc);

	c_ptoser = CreateCursor(comerc|PTOSERbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
	SetCursorFrom(c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MIN_SHORT);
	SetCursorTo  (c_ptoser, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), MAX_SHORT);
	n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_ptoser, 4, validate, displayPto);

			if ( n >= 0 ) {
				FmSetIFld(fm, fno,     IFld(comerc|PTOSER_TIPPTO));
				FmSetFld (fm, DTIPPTO, SFld(comerc|TPTOSER_DESCOR));
			}
		}
		break;
	}
	return FM_OK;
}

private bool validate()
{
	return TRUE;
}

private void displayPto(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(comerc|PTOSER_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(comerc|PTOSER_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}










void HelpVendedor(form fm, fmfield fno, char * deleg)
{
	schema old, billp;
	dbcursor c_VEND;
	int n = 0;

	old = CurrentSchema();

	billp = OpenSchema("billpro", IO_EABORT);

	SwitchToSchema(billp);
                          
	c_VEND = CreateCursor(billp|VENDEDORbyDELEG, IO_NOT_LOCK);

	SetCursorFrom(c_VEND, deleg, LOW_VALUE,  MIN_LONG);
	SetCursorTo  (c_VEND, deleg, HIGH_VALUE, MAX_LONG);

	n = PopUpDbMenu(15, 50, "Vendedores", c_VEND , 0, validate, display);

	if ( n >= 0 ) {
		FmSetLFld(fm, fno,   LFld(billp|VENDEDOR_NROLEG) );
		FmSetFld (fm, fno+1, SFld(billp|VENDEDOR_DESCRIP));
		FmShowFlds(fm, fno, fno+1);			
	}
		  
  	DeleteCursor(c_VEND);
	SwitchToSchema(old);
}



void HelpObjetivo(form fm, fmfield fno, long cliente)
{
	schema old, comerc;
	dbcursor c_OBJ;
	int n = 0;

	old = CurrentSchema();

	comerc = OpenSchema("comerc", IO_EABORT);

	SwitchToSchema(comerc);
                          
	c_OBJ = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom(c_OBJ, cliente, MIN_SHORT);
	SetCursorTo  (c_OBJ, cliente, MAX_SHORT);

	n = PopUpDbMenu(15, 50, "Objetivos", c_OBJ , 0, validate, displayObj);

	if ( n >= 0 ) {
		FmSetIFld(fm, fno,   IFld(comerc|OBJETIVO_OBJET)  );
		FmSetFld (fm, fno+1, SFld(comerc|OBJETIVO_DESCRIP));
		FmShowFlds(fm, fno, fno+1);			
	}
		  
  	DeleteCursor(c_OBJ);
	SwitchToSchema(old);
}


void HelpResumen(form fm, fmfield fno, long cliente)
{
	schema old, bill;
	dbcursor c_RES;
	int n = 0;

	old = CurrentSchema();

	bill = OpenSchema("bill", IO_EABORT);

	SwitchToSchema(bill);

	c_RES = CreateCursor(bill|RESUMENbyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom(c_RES, cliente, MIN_LONG);
	SetCursorTo  (c_RES, cliente, MAX_LONG);

	n = PopUpDbMenu(15, 50, "Resúmenes", c_RES , 0, validate, displayRes);

	if ( n >= 0 ) {
		FmSetLFld(fm, fno,   LFld(bill|RESUMEN_NRORES)  );
		FmSetFld (fm, fno+1, SFld(bill|RESUMEN_DESCRIP));
		FmShowFlds(fm, fno, fno+1);			
	}
		  
  	DeleteCursor(c_RES);
	SwitchToSchema(old);
}



/*-------------------------------* *-------------------------------*/

private bool validate()
{
	return TRUE;
}


private void display(char * buffer)
{
 	sprintf(buffer, "%8ld  %40s", LFld(VENDEDOR_NROLEG),
							 	  SFld(VENDEDOR_DESCRIP) );
}


private void displayObj(char * buffer)
{
 	sprintf(buffer, "%4d  %20s", IFld(OBJETIVO_OBJET), SFld(OBJETIVO_DESCRIP) );
}


private void displayRes(char * buffer)
{
 	sprintf(buffer, "%ld  %25s", LFld(RESUMEN_NRORES), SFld(RESUMEN_DESCRIP) );
}
