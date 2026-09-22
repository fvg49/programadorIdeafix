/*******************************************************************
*
* MODULE & VERSION : @(#)excep.c	1.3
* DATE             : 02/07/19
* TIME             : 17:54:39 
*
********************************************************************/
#include <ideafix.h>
#include "operac.sch"
#include "excepcion.h"

short InicioListaTipoExcepcion()
{
	short fila,col;
	schema   old, ope;
    dbcursor motexc;

	for (fila=0; fila < _MAXFILA_EXCEP; fila ++) {
		for (col=0; col < _MAXCOL_EXCEP; col ++) {
			 _lib_tipo_expcep[fila][col] = 0;
			 _lib_var_expcep[fila][col]  = 0;
		}
	}

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	motexc = CreateCursor(ope|MOTEXCbyCODCOND, IO_NOT_LOCK);
	SetCursorFrom(motexc, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (motexc, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(motexc) != ERROR) {
		if (IFld(ope|MOTEXC_TIPMOT) != NULL_SHORT) {
			_lib_tipo_expcep[IFld(ope|MOTEXC_CODCOND)][IFld(ope|MOTEXC_CODMOT)] = IFld(ope|MOTEXC_TIPMOT);
			_lib_var_expcep[IFld(ope|MOTEXC_CODCOND)][IFld(ope|MOTEXC_CODMOT)] = IFld(ope|MOTEXC_VEMP);
		}
	}
	DeleteCursor(motexc);	
	return 0;
}

short ParteTipoExcepcion(short tipo, short motivo)
{
	if (tipo >= _MAXFILA_EXCEP) {
		WiMsg ("El tipo de excepcion %d es mayor al permitido. Ver _lib_tipo_expcep ", tipo);
		return 0;
	}

	if (motivo >= _MAXCOL_EXCEP) {
		WiMsg ("El motivo de excepcion %d es mayor al permitido. Ver _lib_tipo_expcep ", motivo);
		return 0;
	}

	return _lib_tipo_expcep[tipo][motivo];
}

short ParteVarExcepcion(short tipo, short motivo)
{
	if (tipo >= _MAXFILA_EXCEP) {
		WiMsg ("El tipo de excepcion %d es mayor al permitido. Ver _lib_var_expcep ", tipo);
		return 0;
	}

	if (motivo >= _MAXCOL_EXCEP) {
		WiMsg ("El motivo de excepcion %d es mayor al permitido. Ver _lib_var_expcep ", motivo);
		return 0;
	}

	return _lib_var_expcep[tipo][motivo];
}

