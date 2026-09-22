/********************************************************************
*
* MODULE & VERSION : @(#)auxvig.c	1.6
* DATE             : 08/06/27
* TIME             : 16:56:18
*
* CREATED          : 12/06/02
*
* DESCRIPTION:
*	Listado de Control de Ausentismo por Vigilidor.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "ambiente.h"
#include "billpro.h"
#include "auxvig.fmh"
#include "auxvig.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "asist.sch"
#include "filial.h"


/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
//static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
schema comerc, operac, bill, sue;
form fm0;
report rp0=NULL;
//FILE *fp=NULL;
//int cantVigLista=0;

/* Programa principal */
wcmd(auxvig, 1.16 12/06/02)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);

	fm0 = OpenForm("lretro", FM_EABORT);
	
//	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
//  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
//	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while (DoForm(fm0,/* before,*/ after) != FM_EXIT) {

  	   	InicializarLista();

  	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}


static void AbrirReporte()
{
    if(rp0==NULL) {
    	rp0 = OpenReport("lretro", RP_EABORT|RP_NOBEGIN);
    }
 /*  
    // Si la salida es Impresora
    if ( *FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

	// Si la salida es Terminal
    if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

    BeginReport(rp0, 1, NULL_STR);
	RpSetIFld(rp0, POR, FmIFld(fm0,LISTAPOR) );
 */	
} 
 
 
//static fm_status before(form fm, fmfield fno, int row)
//{ 
/*
	switch (fno) {
	case CLIDESDE:
	   	InicClientesXusr();
    	break;
    case CLIHASTA:
    	break;
    case OBJDESDE:
	   	InicObjetivosXusr(FmLFld(fm, CLIDESDE, row), FmIFld(fm, EMP, row));
    	break;
    case OBJHASTA:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIHASTA, row), FmIFld(fm, EMP, row));
    	break;
	case FFILIAL:
		break;
	}
	return FM_OK;				
	
   */	
//}


static fm_status after(form fm, fmfield fno, int row)
{  

	/*
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
    break;
	case LISTAPOR :
		switch(FmIFld(fm, fno)) {
			case 1 :
				FmSetFld(fm0, DVIGD, NULL_STR);
				FmSetFld(fm0, DVIGH, NULL_STR);
				break;
			case 2 :
				FmSetFld(fm0, DCLID, NULL_STR);
				FmSetFld(fm0, DCLIH, NULL_STR);
				FmSetFld(fm0, DOBJD, NULL_STR);
				FmSetFld(fm0, DOBJH, NULL_STR);
				break;
		}
	case SALIDA:
		if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "auxvig.txt");
	break;
	case FECHAD:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'L')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "LUNES");
		#endif
	break;
	case FECHAH:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'D')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "DOMINGO");
		#endif
	break;
	case CLIDESDE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLIDESDE, row)), row);
    break;
    case CLIHASTA:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
			FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIHASTA, row)),row);
   	break;
    case OBJDESDE:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIDESDE, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIDESDE, row), FmIFld(fm, OBJDESDE, row)), row);
	break;
    case OBJHASTA:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIHASTA, row));
		else	
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIHASTA, row) ,FmIFld(fm, OBJHASTA, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;				
	
	*/
	WiMsg("Entro al fter");
}	

static char * GetDescLicencia(int codlic)
{
	dbtable  ATIPLIS;
	schema   old, asist;
	static char desc[16];

	sprintf(desc, "%s", NULL_STR);
	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	
	ATIPLIS = CreateAlias(asist|TIPLIS);
	
	SetKey(AlInd(ATIPLIS, asist|TIPLISbyCOD), codlic);
	if(GetRecord(AlInd(ATIPLIS, asist|TIPLISbyCOD), THIS_KEY, IO_NOT_LOCK)!=ERROR)
		sprintf(desc, "%s", SFld(AlFld(ATIPLIS, asist|TIPLIS_DESCOR)));
	
	DeleteAlias(ATIPLIS);
	return desc;
}

int GetNovedadLeg(int emp, long nroleg, DATE dia)
{
	dbtable  AASISTEN;
	schema   old, asist;
    int codnov=NULL_SHORT;
    
	codnov=NULL_SHORT;
	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);

	AASISTEN = CreateAlias(asist|ASISTEN);

	SetKey(AlInd(AASISTEN, asist|ASISTENbyEMPRE), emp, dia, nroleg, MIN_SHORT);
	if (GetRecord(AlInd(AASISTEN, asist|ASISTENbyEMPRE), NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3)!=ERROR) {
		codnov = IFld(AlFld(AASISTEN, asist|ASISTEN_CODNOV));
	}
	DeleteAlias(AASISTEN);
	return codnov;
}

static char * GetDescNovedad(int codnov)
{
	dbtable  AINASIST;
	schema   old, asist;
	static char desc[26];

	sprintf(desc, "%s", NULL_STR);
	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	
	AINASIST = CreateAlias(asist|INASIST);
	
	SetKey(AlInd(AINASIST, asist|INASISTbyCODINA), codnov);
	if(GetRecord(AlInd(AINASIST, asist|INASISTbyCODINA), THIS_KEY, IO_NOT_LOCK)!=ERROR)
		sprintf(desc, "%s", SFld(AlFld(AINASIST, asist|INASIST_DESCRINAS)));

	DeleteAlias(AINASIST);
	return desc;	
}

