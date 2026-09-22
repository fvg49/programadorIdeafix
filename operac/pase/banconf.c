/********************************************************************
*
* MODULE & VERSION : banconf.c	1.00
*
* CREATED          : 02/06/05 Fernando Ventura Goncalves
*
* DESCRIPTION:
*      Banco de funcion CalcDistrHoras()
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*********************************************************************/
#include <ideafix.h>
#include "banconf.fmh"

#include "comerc.h"
#include "operac.h"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "comerc.sch"

#define HSNORM 0
#define HSAL25 1
#define HSAL35 2
#define HSFRAN 3

#define MAXTIPHOR 4


double 	horadis[MAXTIPHOR];

form fm0;
schema operac, comerc, bill, sue;
double cantih[MAXTIPHOR];
int NLIN;

fm_cmd cmd;

         /* Fuciones externas */



/* Programa principal */
wcmd(contden, 1.00 07/06/2005)
{
	int v_nor, v_a25, v_a35, v_fra;
	int v_i;
	
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	fm0 = OpenForm("banconf", FM_EABORT);

	NLIN=FmFldLen(fm0, MULTI);
	while ((cmd=DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT) {
		switch(cmd) {
			case FM_READ:
			case FM_READ_NEXT:
			case FM_READ_PREV:  

				for(v_i=0; v_i<NLIN; v_i++)
					FmClearFlds(fm0, TIPHOR, CANHOR, v_i);


				DistibuyeHorasPer(FmIFld(fm0, EMP), FmIFld(fm0, CANHS), FmIFld(fm0, TDIA), &v_nor, &v_a25, &v_a35, &v_fra);
				for(v_i=0; v_i< MAXTIPHOR && v_i< NLIN; v_i++) {
					switch(v_i) {

						case HSNORM: 
							FmSetFld(fm0, TIPHOR, "Normales ", v_i); 
							FmSetIFld(fm0, CANHOR, v_nor);
							break;
						case HSAL25: 
							FmSetFld(fm0, TIPHOR, "al 25 % ", v_i); 
							FmSetIFld(fm0, CANHOR, v_a25);
							break;
						case HSAL35: 
							FmSetFld(fm0, TIPHOR, "al 35 %", v_i); 
							FmSetIFld(fm0, CANHOR, v_a35);
							break;
						case HSFRAN: 
							FmSetFld(fm0, TIPHOR, "Francos ",v_i); 
							FmSetIFld(fm0, CANHOR, v_fra);
							break;
					}
				}

				break;
		}
	}
}

