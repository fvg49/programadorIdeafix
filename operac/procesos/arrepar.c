/********************************************************************
*
* MODULE & VERSION : @(#)arrepar.c	1.46 
* DATE             : 08/08/25 
* TIME             : 16:00:22 
*
* CREATED          : 17/09/98
*
* DESCRIPTION:
*      Arregla el parte (corre en crontab)
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "opedef.h"
#include "disths.h"
#include "ambiente.h"
#include "comgral.h"
#include "operac.sch"
#include "sue.sch"

//#define DEBUG	1

/* Declaraciones globales */
schema   operac, sue;
dbcursor c_parte, c_emps;

char g_prog[20];

/* Programa principal */
Program(arrepar, 1.46 08/25/08)
{
	int caso=0,
		cadiarpa=0,
		i=0;
	DATE fecdesde=NULL_DATE;

	sprintf(g_prog, "%s", argv[0]);

	fprintf(stderr, "Comienzo de arrepar.exe %.3D %.3T\n", Today(), Hour());
	
	operac  = OpenSchema("operac", IO_EABORT);
	sue     = OpenSchema("sue", IO_EABORT);
	
    c_emps = CreateCursor(sue|EMPSbyEMP, IO_NOT_LOCK);
    c_parte = CreateCursor(operac|PARTEbyDIA, IO_NOT_LOCK);

	SetCursorFrom (c_emps, NULL_SHORT);
	SetCursorTo   (c_emps, MAX_SHORT);
	while(FetchCursor(c_emps) != ERROR) {

		
		if (argc>1)
			fecdesde = StrToD(argv[1]);
		else{
			cadiarpa=StrToI(GetParNov(IFld(sue|EMPS_EMP), PARNOV_CADIARPA, 1, Today()));

			if (cadiarpa>0)
				fecdesde  = Today() - cadiarpa;
			else 
				fecdesde  = GetFechaCierreOpe(IFld(sue|EMPS_EMP));
		}


		fprintf(stderr, "\nEMPRESA %d FECHA DESDE : %.3D\n", IFld(sue|EMPS_EMP), fecdesde);
		SetCursorFrom (c_parte, IFld(sue|EMPS_EMP), fecdesde, NULL_LONG, NULL_SHORT);
		SetCursorTo   (c_parte, IFld(sue|EMPS_EMP), MAX_DATE, MAX_LONG, MAX_SHORT);
		while(FetchCursor(c_parte) != ERROR) {
			caso = 0;

			// Horas Negativas 
			if (IFld(operac|PARTE_HSNOR)<0 || IFld(operac|PARTE_HS50)<0 || IFld(operac|PARTE_HS100F)<0 || IFld(operac|PARTE_HS100FE)<0)
				caso=1;

 			// Horas FRANCO sin condic "F" 
			if (IFld(operac|PARTE_HS100FE)!=0 && *SFld(operac|PARTE_CONDIC)!='F')
				caso=2;

 			// Horas Negativas Contden 
			for (i=0 ; i<MAXTIPHOR ; i++)
				cantih[i] = NULL_DOUBLE;

			CalcDistrHoras(IFld(operac|PARTE_EMP),
			               LFld(operac|PARTE_NROLEG),
			               DFld(operac|PARTE_DIA),
			               TFld(operac|PARTE_HORAENT),
			               TFld(operac|PARTE_HORASAL),
			               LFld(operac|PARTE_CLIENTE),
			               IFld(operac|PARTE_OBJETIVO),
			               FALSE,
			               IFld(operac|PARTE_PTOSER),
			               IFld(operac|PARTE_PUESTO),
			               IFld(operac|PARTE_NROINT),
			               cantih);

			for (i=0 ; i<MAXTIPHOR ; i++){
				if (cantih[i]<0 && cantih[i]!=NULL_DOUBLE)
					caso=3;
				
			}


			if (caso>0) {
				fprintf(stderr, "EMP %d, NROLEG %ld, FECHA %.3D, CASO %d, MODIFICACION %.3D\n", IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), caso , DFld(operac|PARTE_MDATE));
				RecalculaPartePer(IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), DFld(operac|PARTE_DIA), NULL_SHORT, NULL_SHORT, g_prog); 
			}

			
		}
	}
	DeleteCursor(c_parte);
	DeleteCursor(c_emps);

	fprintf(stderr, "Fin de arrepar.exe %.3D %.3T\n", Today(), Hour());

	CloseAllSchemas();

}

