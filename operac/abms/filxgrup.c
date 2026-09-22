/********************************************************************
*
* MODULE & VERSION : @(#)filxgrup.c	1.1
* DATE             : 08/06/09
* TIME             : 12:37:20
*
* CREATED          : 05/06/08
*
* DESCRIPTION:
*      ABM de Filiales por Grupo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "filial.h"
#include "filxgrup.fmh"
#include "operac.sch"
#include "comerc.sch"

/* Funciones privadas */

static void Lectura(fm_cmd, find_mode);

/* Declaraciones globales */
form fm0;
schema comerc, operac;

/* Programa principal */
wcmd(filxgrup, 1.1 06/09/08 ) 
{
	int v_i;
	char v_delega[6];
	fm_cmd cmd;
	dbcursor c_filxgrup=(dbcursor)NULL;
	
	fm0 = OpenForm("filxgrup", FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:		Lectura(cmd, THIS_KEY); break;
	case FM_READ_NEXT:	Lectura(cmd, NEXT_KEY); break;
	case FM_READ_PREV:	Lectura(cmd, PREV_KEY); break;
	case FM_ADD:
		InitRecord(operac|FILXGRUP);
	case FM_UPDATE:
		BeginTransaction();
		
		c_filxgrup = CreateCursor(operac|FILXGRUPbyEMP, IO_NOT_LOCK);

		SetCursorFrom(c_filxgrup, FmIFld(fm0, EMP), FmIFld(fm0, GRUPO), NULL_STR,   NULL_STR);
		SetCursorTo  (c_filxgrup, FmIFld(fm0, EMP), FmIFld(fm0, GRUPO), HIGH_VALUE, HIGH_VALUE);
		while(FetchCursor(c_filxgrup) != ERROR) {
			DelRecord(operac|FILXGRUP);
		}	
		
		for (v_i = 0; !FmIsNull(fm0, FFILIAL, v_i) && v_i<=FmFldLen(fm0, MULTI); v_i++) {

			sprintf (v_delega, "%s", GetDelegacion(FmSFld(fm0, FFILIAL, v_i)) );

			SetKey(operac|FILXGRUPbyEMP, FmIFld(fm0, EMP), FmIFld(fm0, GRUPO), v_delega, FmSFld(fm0, FFILIAL, v_i));
    		PutRecord(operac|FILXGRUP);
    	} 

		EndTransaction();
		DeleteCursor(c_filxgrup);

		break;
	case FM_IGNORE:
		FreeTable(operac|FILXGRUP);
		break;
	}
	
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int v_i=0;
	dbcursor c_filxgrup;
	c_filxgrup = CreateCursor(operac|FILXGRUPbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_filxgrup, FmIFld(fm0, EMP), FmIFld(fm0, GRUPO), NULL_STR,   NULL_STR);
	SetCursorTo  (c_filxgrup, FmIFld(fm0, EMP), FmIFld(fm0, GRUPO), HIGH_VALUE, HIGH_VALUE);
	while(FetchCursor(c_filxgrup) != ERROR) {
		if (v_i>=FmFldLen(fm0, MULTI)){
			WiDialog(WD_ABORT, WD_ABORT,"Error", "El proceso no puede cargar tantas filiales \npor favor avisar a sistemas");
			exit(0);
		}
		FmSetFld(fm0, FFILIAL, SFld(operac|FILXGRUP_FILIAL), v_i);
		FmSetFld(fm0, DFILIAL, GetDescFilial(SFld(operac|FILXGRUP_FILIAL)), v_i);

		v_i++;
	}	
	DeleteCursor(c_filxgrup);
}


