/********************************************************************
*
* MODULE & VERSION : @(#)bajtem.c	1.00  
* DATE             : 08/08/25 
* TIME             : 16:00:22 
*
* CREATED          : 22/12/14
*
* DESCRIPTION: Da de baja los legajos marcados con baja temprana
*              una vez pasada la fecha de egreso.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "sue.sch"

/* Declaraciones globales */
schema sue;
dbcursor c_per, c_emp;
char g_prog[20];

/* Programa principal */
Program(bajtem, 1.00 22/12/14)
{
	sprintf(g_prog, "%s", argv[0]);

	fprintf(stderr, "Comienzo de bajtem.exe %.3D %.3T\n", Today(), Hour());
	
	sue = OpenSchema("sue", IO_EABORT);
	
    c_emp = CreateCursor(sue|EMPSbyEMP, IO_NOT_LOCK);
    c_per = CreateCursor(sue|PERbyACT, IO_NOT_LOCK);

	SetCursorFrom (c_emp, NULL_SHORT);
	SetCursorTo   (c_emp, MAX_SHORT);

	BeginTransaction();
	while(FetchCursor(c_emp) != ERROR) {

		SetCursorFrom (c_per, IFld(sue|EMPS_EMP), 2, NULL_LONG);
		SetCursorTo   (c_per, IFld(sue|EMPS_EMP), 2, MAX_LONG);

		while(FetchCursor(c_per) != ERROR) {
			if (DFld(sue|PER_FECEGR) < Today()-30) {
				SetIFld(sue|PER_ACTIVO, 0);

				fprintf(stderr, "Legajo %d %d Fecha Egreso %D\n", IFld(sue|EMPS_EMP), LFld(sue|PER_NROLEG), DFld(sue|PER_FECEGR));
				PutRecord(sue|PER);
			}
		}
	}
	EndTransaction();
	DeleteCursor(c_per);
	DeleteCursor(c_emp);

	fprintf(stderr, "Fin de bajtem.exe %.3D %.3T\n", Today(), Hour());

	CloseAllSchemas();

}

