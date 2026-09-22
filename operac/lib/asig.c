/********************************************************************
*
* MODULE & VERSION : @(#)asig.c	1.1 
* DATE             : 02/06/13 
* TIME             : 15:26:43 
*
* CREATED          : 11/09/98
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*               |
*********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "operac.h"
#include "comerc.h"
#include "asist.sch"
#include "brigada.sch"
#include "asig.h"


// Busco para ese cli-obj, la cantidad ya asignada de vigiladores para cada puesto.
void CargarMatPuestos(long cliente, int objetivo, int pue[MAXFILA][MAXCOL])
{
	schema old, operac;
	dbtable puestos;
	int fila, col;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);
	if ((puestos = CreateAlias(operac|PUESTOS)) == ERROR) {
		WiMsg("No se pudo crear el alias de la Tabla PUESTOS.");
		return;
	}
	for (fila = 0; fila < MAXFILA; fila++) {
		for (col = 0; col < MAXCOL; col++) {
			pue[fila][col] = 0;
		}
	}
	SetLFld(AlFld(puestos, operac|PUESTOS_CLIENTE), cliente);
	SetIFld(AlFld(puestos, operac|PUESTOS_OBJET),   objetivo);
	SetIFld(AlFld(puestos, operac|PUESTOS_TIPPTO),  MIN_SHORT);
	SetIFld(AlFld(puestos, operac|PUESTOS_CODINT),  MIN_SHORT);
	while (GetRecord(AlInd(puestos, operac|PUESTOSbyCLIENTE), NEXT_KEY|PARTIAL_KEY,
																	IO_NOT_LOCK, 2) != ERROR) {
		pue[IFld(AlFld(puestos, operac|PUESTOS_TIPPTO))][IFld(AlFld(puestos, operac|PUESTOS_CODINT))] =
			IFld(AlFld(puestos, operac|PUESTOS_VIGI));
	}                          
	DeleteAlias(puestos);
}
