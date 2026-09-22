/********************************************************************
*
* MODULE & VERSION : @(#)delvig.c	1.6
* DATE             : 09/03/25
* TIME             : 10:35:03
*
* CREATED          : 17/09/98
*
* DESCRIPTION:
*      Generación del Parte Diario.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------

*********************************************************************/
#include <ideafix.h>
#include "delvig.fmh"
#include "delvig.rph"
#include "operac.sch"
#include "sue.sch"
#include "operac.h"
#include "filial.h"

/* Declaraciones globales */
schema   operac, sue;
form     fm0;
report rp = (report) ERROR;

static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
void BorrarAsignaciones();
void GrabarAsigH (DATE fecha);

/* Programa principal */
wcmd(delvig, 1.6 03/25/09)
{
	fm0     = OpenForm("delvig", FM_EABORT);
	sue     = OpenSchema("sue", IO_EABORT);
	operac  = OpenSchema("operac", IO_EABORT);

	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	BeginTransaction();
	BorrarAsignaciones(); 
	EndTransaction();
    
    FinObjetivosXusr();
}

void BorrarAsignaciones() 
{
	bool encontro = FALSE;
	dbcursor c_asig;
	DATE v_fecha=NULL_DATE;
	c_asig  = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);

	rp = OpenReport("delvig", RP_EABORT);

	RpSetIFld (rp, REMP,  FmLFld (fm0, EMP));
	RpSetFld  (rp, RDEMP,  FmSFld (fm0, DEMP));
	RpSetFld  (rp, RDELEG,  FmSFld (fm0, DELEG));
	RpSetFld  (rp, RDDELEG,  FmSFld (fm0, DDELEG));
	RpSetFld  (rp, RDELEGA,  FmSFld (fm0, FDELEGA));
	RpSetFld  (rp, RDDELEGA,  FmSFld (fm0, DDELEGA));
	RpSetFld  (rp, RFILIAL,  FmSFld (fm0, FFILIAL));
	RpSetFld  (rp, RDFILIAL,  FmSFld (fm0, DESCRIP));
	RpSetLFld (rp, RNROCLI,  FmLFld (fm0, FCLIENTE));
	RpSetFld  (rp, RDESCLI,  FmSFld (fm0, DCLIE));
	RpSetIFld (rp, RNROOBJ,  FmLFld (fm0, OBJDESDE));
	RpSetFld  (rp, RDESOBJ,  FmSFld (fm0, DOBJD));
	RpSetIFld (rp, RNROOBJH, FmLFld (fm0, OBJHASTA));
	RpSetFld  (rp, RDESOBJH, FmSFld (fm0, DOBJH));
	
	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, FCLIENTE), FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, FCLIENTE), FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
	
    	SetKey (sue|PERbyEMP, IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG));
		if ( GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR )
			continue;

		//fprintf(stderr, "%ld - %.3d\n", LFld (sue|PER_NROLEG), DFld (sue|PER_FECEGR));

		
		if (IsNull (sue|PER_FECEGR) || DFld (sue|PER_FECEGR) > Today ())
			continue;

		if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
 		if (!ValidaObjetivoXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), IFld(operac|ASIG_EMP)))
 			continue;
		/* Imprimo al vigilador */
		RpSetLFld (rp, RNROLEG, LFld (sue|PER_NROLEG));
		RpSetFld  (rp, RAPYNOM, SFld (sue|PER_APYNOM));
		RpSetDFld (rp, RFECEGR, DFld (sue|PER_FECEGR));
		DoReport  (rp, LINEA);


		//Si la fecha de egreso es menor a la desde de la asignacion va la desde
		if (DFld (sue|PER_FECEGR)>DFld (operac|ASIG_FECASIG))
			v_fecha = DFld (sue|PER_FECEGR);
		else
			v_fecha = DFld (operac|ASIG_FECASIG); 


		GrabarAsigH(v_fecha);
		
		//fprintf(stderr, "%d - %ld - %d\n", IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
		
		DelRecord (operac|ASIG);
		encontro = TRUE;
	} 
	
	if (!encontro)
		WiMsg ("No borro ninguna asignacion");
}

void GrabarAsigH (DATE fecha)
{

	CopyFld	(operac|ASIG_EMP, 		operac|ASIGH_EMP);
	CopyFld	(operac|ASIG_CLIENTE, 	operac|ASIGH_CLIENTE);
	CopyFld	(operac|ASIG_OBJETIVO, 	operac|ASIGH_OBJETIVO);
	CopyFld	(operac|ASIG_PTOSER, 	operac|ASIGH_PTOSER);
	CopyFld	(operac|ASIG_PUESTO, 	operac|ASIGH_PUESTO);
	CopyFld	(operac|ASIG_NROLEG, 	operac|ASIGH_NROLEG);
	CopyFld	(operac|ASIG_VIGIL, 	operac|ASIGH_VIGIL);
	CopyFld	(operac|ASIG_EFECT, 	operac|ASIGH_EFECT);
	CopyFld	(operac|ASIG_FECASIG, 	operac|ASIGH_FECALT);
	CopyFld	(operac|ASIG_HSENT, 	operac|ASIGH_HSENT);
	CopyFld	(operac|ASIG_HSSAL, 	operac|ASIGH_HSSAL);
	CopyFld	(operac|ASIG_DIA1, 		operac|ASIGH_DIA1);	
	CopyFld	(operac|ASIG_DIA2, 		operac|ASIGH_DIA2);	
	CopyFld	(operac|ASIG_DIA3, 		operac|ASIGH_DIA3);	
	CopyFld	(operac|ASIG_DIA4, 		operac|ASIGH_DIA4);	
	CopyFld	(operac|ASIG_DIA5, 		operac|ASIGH_DIA5);	
	CopyFld	(operac|ASIG_DIA6, 		operac|ASIGH_DIA6);	
	CopyFld	(operac|ASIG_DIA7, 		operac|ASIGH_DIA7);	
	CopyFld	(operac|ASIG_REEMPL, 	operac|ASIGH_REEMPL);	
	CopyFld	(operac|ASIG_REGIM, 	operac|ASIGH_REGIM);	
	CopyFld	(operac|ASIG_FECHAS,  	operac|ASIGH_FECHAS);	
	CopyFld	(operac|ASIG_FFRANCO,	operac|ASIGH_FFRANCO);	
	CopyFld	(operac|ASIG_NUMFRAN, 	operac|ASIGH_NUMFRAN);	
	CopyFld	(operac|ASIG_FRANCERO, 	operac|ASIGH_FRANCERO);	
	CopyFld	(operac|ASIG_NROINT, 	operac|ASIGH_NROINT);	
	SetIFld (operac|ASIGH_MOTIVO,	DADO_DE_BAJA);
	SetDFld (operac|ASIGH_FECBAJ,	fecha);
	SetFld  (operac|ASIGH_TIPODIA,  SFld(operac|ASIG_TIPODIA));
	PutRecord (operac|ASIGH);	
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
    case OBJDESDE:
	   	InicObjetivosXusr(FmLFld(fm, FCLIENTE, row), FmIFld(fm, EMP, row));
    	break;
    case OBJHASTA:                                 
	   	InicObjetivosXusr(FmLFld(fm, FCLIENTE, row), FmIFld(fm, EMP, row));
    	break;
	case FFILIAL:
		break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
    case OBJDESDE:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, FCLIENTE, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, FCLIENTE, row), FmIFld(fm, OBJDESDE, row)), row);
	break;
    case OBJHASTA:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, FCLIENTE, row));
  		else
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, FCLIENTE, row), FmIFld(fm, OBJHASTA, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;				
}	

