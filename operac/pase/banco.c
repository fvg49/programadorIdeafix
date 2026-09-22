/********************************************************************
*
* MODULE & VERSION : banco.c	1.00
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
#include "banco.fmh"

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
extern bool FeriadoNovia(DATE, int, int);
extern double ConvHraInt(TIME, TIME);

static fm_status after(form fm, fmfield fno, int row);
static int validate();
static void display(char * buffer);

extern int GetHorasViaje(int ,int, int, DATE, int*);

/* Programa principal */
wcmd(contden, 1.00 07/06/2005)
{
	int v_i;
	int v_nor, v_a25, v_a35, v_fra, v_peg;
	
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	fm0 = OpenForm("banco", FM_EABORT);

	NLIN=FmFldLen(fm0, MULTI);
	while ((cmd=DoForm(fm0, NULLFP, after)) != FM_EXIT) {
		switch(cmd) {
			case FM_READ:
			case FM_READ_NEXT:
			case FM_READ_PREV:  
				

				for(v_i=0; v_i<NLIN; v_i++)
					FmClearFlds(fm0, TIPHOR, CANHOR, v_i);


				CalculoDetalleHorasPer(FALSE, 
				                       NULL_SHORT, 
				                       NULL_SHORT, 
				                       FmIFld(fm0, EMP),
				                       FmLFld(fm0, CLI),
				                       FmIFld(fm0, OBJ),
				                       FmDFld(fm0, FECHA),
				                       FmLFld(fm0, NROLEG),
				                       FmIFld(fm0, TIPPTO),
				                       FmIFld(fm0, CODINT),
				                       FmIFld(fm0, NROINT),
				                       &v_nor,
				                       &v_a25, 
				                       &v_a35, 
				                       &v_fra,
				                       &v_peg);


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
							FmSetIFld(fm0, CANHOR, v_a35+v_peg);
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


static fm_status after(form fm, fmfield fno, int row)
{
	dbcursor  c_puesto; // c_ptoser;
	int  n;

	switch (fno) {
	case TIPPTO :
		if(FmKeyCode(fm) == K_HELP)	{
			c_puesto = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK|IO_CONTROL_BREAK);
			SetCursorFrom(c_puesto, FmLFld(fm0, CLI), FmIFld(fm0, OBJ), MIN_SHORT, MIN_SHORT);
			SetCursorTo  (c_puesto, FmLFld(fm0, CLI), FmIFld(fm0, OBJ), MAX_SHORT, MAX_SHORT);
			n = PopUpDbMenu(10, 30, " Puestos de Trabajo ", c_puesto, 3, validate, display);

			if ( n >= 0 ) {

				FmSetIFld(fm, fno,     IFld(operac|PUESTOS_TIPPTO));
				FmSetFld (fm, DTIPPTO, SFld(comerc|TPTOSER_DESCOR));
			}
		}
		break;
	case CODINT :
		if (FmKeyCode(fm)==K_HELP) 
			HelpPto(fm, fno, row, FmLFld(fm0, CLI), FmIFld(fm0, OBJ), FmIFld(fm, TIPPTO),
			        FmDFld(fm0, FECHA), NULL_STR, NULL_STR);

		break;
	}
	return FM_OK;
}

static int validate()
{
	if(!BajaPuesto(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO), FmDFld(fm0, FECHA)))
		return TRUE;
	return FALSE;		
}

static void display(char * buffer)
{
	SetKey(comerc|TPTOSERbyTIPPTO, IFld(operac|PUESTOS_TIPPTO));
	GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK);
	sprintf(buffer,"%2d %-20.20s", IFld(operac|PUESTOS_TIPPTO), SFld(comerc|TPTOSER_DESCOR));
}



