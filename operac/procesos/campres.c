/********************************************************************
*
* MODULE & VERSION : @(#)campres.c	1.2
* DATE             : 07/11/02 
* TIME             : 12:57:06 
*
* CREATED          : 02/11/07
*
* DESCRIPTION:
*      Porceso : Cambio Masivo de Responsable de Presentismo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "bill.h"
#include "comerc.h"
#include "campres.rph"
#include "campres.fmh"
#include "bill.sch"
#include "comerc.sch"

/* Funciones privadas */
static void AbrirRp(void);

/* Declaraciones globales */
form   fm0;
schema comerc, bill;
report rp;

/* Programa principal */
wcmd(campres, 1.2 11/02/07)
{
	dbcursor c_obj;
	
	bill   = OpenSchema("bill", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	
	fm0 = OpenForm("campres", FM_EABORT);
    
    
    c_obj = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
    
	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	AbrirRp();
	
	BeginTransaction();
	while(FetchCursor(c_obj) != ERROR) {
		if(LFld(comerc|OBJETIVO_PRESEN) != FmLFld(fm0, PRESENV))
			continue;
		if(IFld(comerc|OBJETIVO_EMP) != FmIFld(fm0, EMP))
			continue;

		/* Informo el cambio */
		RpClearZone(rp, LINEA);
		RpSetLFld(rp, CLIE,  LFld(comerc|OBJETIVO_CLIENTE));    
		RpSetFld (rp, DCLIE, GetCliRazsoc(LFld(comerc|OBJETIVO_CLIENTE)));
		
		RpSetIFld(rp, OBJ,   IFld(comerc|OBJETIVO_OBJET));
	    RpSetFld (rp, DOBJ,  GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))); 

  		DoReport(rp, LINEA);

		/* modifico */             
		SetLFld(comerc|OBJETIVO_PRESEN, FmLFld(fm0, PRESENN));
		PutRecord(comerc|OBJETIVO);
	}	
	EndTransaction();
	
	CloseReport(rp)	;
	DeleteCursor(c_obj);
}


static void AbrirRp(void)
{
	rp  = OpenReport("campres", RP_EABORT);
      
	RpSetFld(rp,  REMPRE,   FmSFld(fm0, DEMP));
	RpSetLFld(rp, RPRESENV,  FmLFld(fm0, PRESENV));
	RpSetFld (rp, RDPRESENV, FmSFld(fm0, DPRESENV));
	RpSetLFld(rp, RPRESENN,  FmLFld(fm0, PRESENN));
	RpSetFld (rp, RDPRESENN, FmSFld(fm0, DPRESENN));
}

