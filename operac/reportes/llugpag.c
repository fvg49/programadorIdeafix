/********************************************************************
*
* MODULE & VERSION : @(#)llugpag.c	1.5 
* DATE             : 04/03/04 
* TIME             : 18:19:57 
*
* CREATED          : 15/10/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "llugpag.fmh"
#include "llugpag.rph"
#include "bill.sch"
#include "comerc.sch"
#include "sue.sch"
#include "operac.h"

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
static void Proceso(void);
static void PrintHead(void);

/* Declaraciones globales */
form fm0;
report rp;
schema bill, com;

/* Programa principal */
wcmd(llugpag, 1.5 03/04/04)
{
	fm0 = OpenForm("llugpag", FM_EABORT);

	if (DoForm(fm0, before, after) != FM_UPDATE) return;
	com = OpenSchema("comerc", IO_EABORT);
	bill = OpenSchema("bill", IO_EABORT);
	rp = OpenReport("llugpag", RP_EABORT|RP_NOBEGIN);  
	RpSetOutput(rp, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
	BeginReport(rp, 1, NULL_STR);     
	PrintHead();
    Proceso();
}

static void PrintHead(void)
{                       
	RpSetLFld(rp, RCLID, FmLFld(fm0, CLIED));
	RpSetFld (rp, RNOMBRED, FmSFld(fm0, NOMBRED));
	RpSetIFld(rp, ROBJD, FmIFld(fm0, OBJD));
	RpSetFld (rp, RDOBJD, FmSFld(fm0, DOBJD));
	RpSetLFld(rp, RCLIH, FmLFld(fm0, CLIEH));
	RpSetFld (rp, RNOMBREH, FmSFld(fm0, NOMBREH));
	RpSetIFld(rp, ROBJH, FmIFld(fm0, OBJH));
	RpSetFld (rp, RDOBJH, FmSFld(fm0, DOBJH));
	
}

static void Proceso(void)
{
	dbcursor c_OBJ = (dbcursor) ERROR;
	long lpago;
	
	c_OBJ = CreateCursor(com|OBJETIVO, IO_NOT_LOCK);
	SetCursorFrom(c_OBJ, FmLFld(fm0, CLIED), FmIFld(fm0, OBJD));
	SetCursorTo  (c_OBJ, FmLFld(fm0, CLIEH), FmIFld(fm0, OBJH)); 
	while(FetchCursor(c_OBJ) != ERROR) {
		if (!FmIFld(fm0, INACT) &&
			!ObjFecActivo(FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET), Today()))
				continue;

		if (FmIFld(fm0, INACT) &&
			ObjFecActivo(FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET), Today()))
				continue;


		RpClearZone(rp, LINEA);
		DbToRp(rp, RCLIE, ROFPAG);
		SetKey(bill|CLIENTE, LFld(com|OBJETIVO_CLIENTE));
		if(GetRecord(bill|CLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			RpSetFld(rp, RNOMBRE, SFld(bill|CLIENTE_RAZSOC));
		
		lpago= 	ObtenerLugPag (LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET));
		RpSetLFld(rp,ROFPAG,  lpago );
		RpSetFld(rp, RDOFPAG, GetDescrLugPag (lpago));

		DoReport(rp, LINEA);
	}
}
static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case AGRCLI:
		break;
	case CLIED:
		break;
	case CLIEH:
		break;
	case OBJD:
		break;
	case OBJH:
		break;
	case SALIDA:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case AGRCLI:
		break;
	case CLIED:
		break;
	case CLIEH:
		break;
	case OBJD:
		break;
	case OBJH:
		break;
	case SALIDA:
		break;
	}
	return FM_OK;
}

