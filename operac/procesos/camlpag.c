/********************************************************************
*
* MODULE & VERSION : @(#)camlpag.c	1.1 
* DATE             : 00/12/28 
* TIME             : 15:50:47 
*
* CREATED          : 27/12/00
*
* DESCRIPTION:
*      Porceso : Cambio de Oficina de Pago.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "camlpag.rph"
#include "camlpag.fmh"
#include "bill.sch"
#include "comerc.sch"


/* Funciones privadas */
static void AbrirRp(void);


/* Declaraciones globales */
form   fm0;
schema comerc, bill;
report rp;


/* Programa principal */
wcmd(camlpag, 1.1 12/28/00)
{
	fm_cmd cmd;             
	dbcursor c_obj;
	
	bill   = OpenSchema("bill", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	
	fm0 = OpenForm("camlpag", FM_EABORT);
    
    
    c_obj = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
    
	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	AbrirRp();
	
	BeginTransaction();
	while(FetchCursor(c_obj) != ERROR) {
		if(LFld(comerc|OBJETIVO_OFPAG) != FmLFld(fm0, OFPAGV))
			continue;


		/* Informo el cambio */
		RpClearZone(rp, LINEA);
		RpSetLFld(rp, CLIE,  LFld(comerc|OBJETIVO_CLIENTE));    
		
		SetKey(bill|CLIENTEbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE));
		if(GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK)!= ERROR)		
			RpSetFld (rp, DCLIE, SFld(bill|CLIENTE_RAZSOC));		
				
		RpSetIFld(rp, OBJ,   IFld(comerc|OBJETIVO_OBJET));
	
		SetKey(comerc|OBJETIVObyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if(GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		    RpSetFld (rp, DOBJ,  SFld(comerc|OBJETIVO_DESCRIP));
  	
  		DoReport(rp, LINEA);

		/* modifico */             
		SetLFld(comerc|OBJETIVO_OFPAG, FmLFld(fm0, OFPAGN));
		PutRecord(comerc|OBJETIVO);
	}	
	EndTransaction();
	
	CloseReport(rp)	;
	DeleteCursor(c_obj);
}


static void AbrirRp(void)
{
	rp  = OpenReport("camlpag", RP_EABORT);

	RpSetFld(rp,  REMPRE,   FmSFld(fm0, DEMP));
	RpSetLFld(rp, ROFPAGV,  FmLFld(fm0, OFPAGV));
	RpSetFld (rp, RDOFPAGV, FmSFld(fm0, DESOFV));
	RpSetLFld(rp, ROFPAGN,  FmLFld(fm0, OFPAGN));
	RpSetFld (rp, RDOFPAGN, FmSFld(fm0, DESOFN));
}

