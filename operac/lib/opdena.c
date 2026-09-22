/********************************************************************
*
* MODULE & VERSION : @(#)opdena.c	1.1 
* DATE             : 01/07/06 
* TIME             : 11:22:48 
*
* CREATED          : 30/07/98
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*                      |
*char   *GetCategDesc  |Devuelve la descripción de la CATEGORIA de DENARIUS
*********************************************************************/
#include <ideafix.h>
#include "sue.sch"
#include "operac.h"

/*-------------------------* GetNombreLeg *--------------------------*/
char * GetNombreLeg(int emp, long legajo)
{
	schema  old, sue;
	static char nombre[61];

	old = CurrentSchema();

	sue = OpenSchema("sue", IO_EABORT);

	SwitchToSchema(sue);

	SetIFld(sue|PER_EMP, 	emp   );
	SetLFld(sue|PER_NROLEG, legajo);

	if ( GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR ) {
		strcpy(nombre, SFld(sue|PER_APYNOM));
	}
	else {
		strcpy(nombre, NULL_STR);
	}

	SwitchToSchema(old);		
	return nombre;	
}


