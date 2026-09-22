/********************************************************************
*
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : 16/02/09
*
* DESCRIPTION:
*      Control de Horas sin Confirmar
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.sch"
#include "operac.sch"
#include "operac.h"
#include "comerc.h"
#include "ctrlhssc.fmh"
#include "filial.h"

static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void Proceso(void);
//extern double ConvHraInt();

/* Declaraciones globales */
form fm0;
schema oper, com;
struct s_lisxusr_lib esta_lis;
char filial[7] = {'\0'};
double hst = 0.0, hsc = 0.0;

wcmd(ctrlhssc, %I% %G%)
{
	fm0  = OpenForm  ("ctrlhssc", FM_EABORT);
	oper = OpenSchema("operac",  IO_EABORT);
	com  = OpenSchema("comerc",  IO_EABORT);

	InicListaXusr(StrToI(ReadEnv("emp")));
	
	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;
	BeginTransaction();

	Proceso();
    
	EndTransaction();
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void Proceso(void)
{

	dbcursor Par = (dbcursor) ERROR;
	Par = CreateCursor(oper|PARTEbyEMP, IO_NOT_LOCK);

	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis))	{ 
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLIED))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIEH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIED) && esta_lis.objetivo < FmIFld(fm0, OBJETD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIEH) && esta_lis.objetivo > FmIFld(fm0, OBJETH))
	    	continue;

		SetCursorFrom(Par, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD),
						   MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (Par, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH),
						   MAX_LONG, MAX_SHORT, MAX_SHORT, MIN_SHORT);
		while(FetchCursor(Par) != ERROR) {
			if (!ValidaFilial(LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO), NULL_STR, NULL_STR, FmSFld(fm0, FFILIAL)))
				continue;
			
			if (DFld(oper|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(oper|PARTE_DIA) > FmDFld(fm0, FECHAH)) {
				continue;
			}
		    
			//obtendo las horas teoricas que tendrias que estar cargadas
			hst = ConvHraInt(TFld(oper|PARTE_HORAENT), TFld(oper|PARTE_HORASAL)) * 100;
			
			//sumo las horas cargadas
			hsc = FFld(oper|PARTE_HSNOR) + FFld(oper|PARTE_HS50) + FFld(oper|PARTE_HS100F) + FFld(oper|PARTE_HS100FE);
			
			fprintf(stderr, "%.2f - %.2f\n", hst, hsc);
			
			
			//informo el error
			if (hst != hsc) {
				fprintf(stderr, "aa\n");	
			}	
		}
	}
	
	DeleteCursor(Par);
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		if (FmChgFld(fm))
			InicListaXusr(FmIFld(fm0, EMP));
    break;
	case FECHAD:
	case FECHAH:
		break;
	case CLIED:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIED, GetDescCliente(FmLFld(fm, CLIED, row)), row);
    break;
    case CLIEH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIEH, GetDescCliente(FmLFld(fm, CLIEH, row)), row);
   	break;
    case OBJETD:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIED, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIED, row), FmIFld(fm, OBJETD, row)), row);
	break;
    case OBJETH:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIEH, row));
  		else
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row), FmIFld(fm, OBJETH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIED:
	   	InicClientesXusr();
    	break;
    case OBJETD:
	   	InicObjetivosXusr(FmLFld(fm, CLIED, row), FmIFld(fm, EMP, row));
    	break;
    case OBJETH:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIEH, row), FmIFld(fm, EMP, row));
    	break;
	}
	return FM_OK;
}

