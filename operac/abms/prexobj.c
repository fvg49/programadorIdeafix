/********************************************************************
*
* MODULE & VERSION : @(#)prexobj.c	1.1
* DATE             : 04/10/15
* TIME             : 14:02:30
*
* CREATED          : 15/10/04
*
* DESCRIPTION:
*             Personal que toma presentismo por Objetivos.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "prexobj.fmh"
#include "comerc.sch"
#include "sue.sch"
#include "comerc.h"
#include "billpro.h"
#include "sue.h"
#include "filial.h"

#define INACTIVO 0
#define MAX_OBJ	1000

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void Lectura(fm_cmd, find_mode);
private void PutInObjetivo();
private void InicVector();
private bool InMulti(long cliente, int objet);
void AgregarEstructura (long cliente, short objetivo);

/* Declaraciones de Estructuras */
struct objetivo {
	long cliente;
	int objetivo;
}obj[MAX_OBJ];

/* Declaraciones globales */
form fm0;
schema comerc, sue;
int ultpos=0;

/* Programa principal */
wcmd(prexobj, 1.1 10/15/04)
{
	fm_cmd cmd;
	fm0 = OpenForm("prexobj", FM_EABORT);

	sue    = OpenSchema("sue", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ: Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		PutInObjetivo();
		EndTransaction();
		break;
	case FM_IGNORE:
		FreeTable(OBJETIVO);
		break;
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int i;

	InicVector();

	SetLFld(comerc|OBJETIVO_PRESEN,  FmLFld(fm0, PERPRE));
	SetLFld(comerc|OBJETIVO_CLIENTE, MIN_LONG);
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	switch(GetRecord(comerc|OBJETIVObyPRESEN, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(comerc|OBJETIVObyPRESEN, NEXT_KEY|PARTIAL_KEY, 1);
//		DbToFm(fm0, 0, DOBJ);
//		FmShowFlds(fm0, 0, DOBJ);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	InicVector();
	SetLFld(comerc|OBJETIVO_PRESEN,  FmLFld(fm0, PERPRE));
	SetLFld(comerc|OBJETIVO_CLIENTE, MIN_LONG);
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	for (i = 0; GetRecord(comerc|OBJETIVObyPRESEN, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR && i < FmFldLen(fm0, MULTIS);) {
        //valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		       	continue;
        
		if (IFld(comerc|OBJETIVO_EMP) != FmIFld(fm0, EMP))
			continue;

		AgregarEstructura (LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		
		FmSetLFld(fm0, CLIE,  LFld(comerc|OBJETIVO_CLIENTE), i);
		FmSetFld (fm0, DCLIE, GetDescCli(LFld(comerc|OBJETIVO_CLIENTE)), i);
		FmSetIFld(fm0, OBJET, IFld(comerc|OBJETIVO_OBJET), i);
		FmSetFld (fm0, DOBJET,GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), LFld(comerc|OBJETIVO_OBJET)), i);
		i++;
	}
}

private void PutInObjetivo()
{
	int i;
	long presentismo;

	for (i=0; i < FmFldLen(fm0, MULTIS) && !FmIsNull(fm0, CLIE, i); i++) {
		AgregarEstructura ( FmLFld(fm0, CLIE,  i), FmIFld(fm0, OBJET, i));
	}

	/* Los clientes - objetivos que se desasignaron a este presentismo no se borran
	   se guardan con presentismo NULL */
		
	for (i = 0;  i < ultpos; i++) {

		if (!InMulti(obj[i].cliente, obj[i].objetivo)) {
			presentismo = NULL_LONG;
		}
		else {
			presentismo = FmLFld(fm0, PERPRE);
		}
		
	 	SetLFld(comerc|OBJETIVO_CLIENTE, obj[i].cliente);
		SetIFld(comerc|OBJETIVO_OBJET,   obj[i].objetivo);
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_LOCK) != ERROR &&
			LFld(comerc|OBJETIVO_PRESEN) != presentismo)  {
				SetLFld(comerc|OBJETIVO_PRESEN, presentismo);

				PutRecord(comerc|OBJETIVO);
		}
	}
}

private bool InMulti(long cliente, int objet)
{
	int i;

	for ( i=0; i < FmFldLen(fm0, MULTIS) && !FmIsNull(fm0, CLIE, i); i++ ) {
		if (FmLFld(fm0, CLIE, i) == cliente && FmIFld(fm0, OBJET, i) == objet)
			return TRUE;
	}
	return FALSE;
}

private void InicVector()
{
	int i;

	for (i = 0; i < MAX_OBJ; i++) {
		obj[i].cliente  = 0;
		obj[i].objetivo = 0;
	}

	ultpos=0;
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIE:
	   	InicClientesXusr();
    	break;
    case OBJET:
	   	InicObjetivosXusr(FmLFld(fm, CLIE, row), FmIFld(fm, EMP, row));
    	break;
	}
	return FM_OK;				
}


static fm_status after(form fm, fmfield fno, int row)
{
	long nroleg;

	switch (fno) {
	case EMP:
	    InicListaXusr(FmIFld(fm0, EMP));
    break;
	case PERPRE :
		if (FmKeyCode(fm) == K_META) {
			nroleg = ERROR;
			if ( (nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
				return FM_REDO;
			FmSetLFld(fm, fno, nroleg);
		}
		SetIFld(sue|PER_EMP,    FmIFld(fm, EMP));
		SetLFld(sue|PER_NROLEG, FmLFld(fm, fno));
		GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
		if ((GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) && IFld(sue|PER_ACTIVO) == INACTIVO) {
			Warning("Legajo inactivo.");
//			return FM_REDO;
		}        
		break;
	case CLIE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
			if (ValidaClienteXusr(FmLFld(fm, CLIE, row)))
			  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIE, row)), row);
			else {
  				if (!FmIsNull(fm, CLIE, row))	{
	  				Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIE, row));
					FmSetLFld(fm, CLIE, NULL_LONG, row);
					FmSetFld(fm, DCLIE, NULL_STR, row);
					return FM_REDO;
				}
  			}	
    break;
	case OBJET:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIE, row));
  		else
			if (ValidaObjetivoXusr(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
				FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row)), row);
			else	{
		  		if (!FmIsNull(fm, OBJET, row))	{
					Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row));
					FmSetIFld(fm, OBJET, NULL_SHORT, row);
					FmSetFld(fm, DOBJET, NULL_STR, row);
					return FM_REDO;
				} 
    		}

		SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm, CLIE, row));
		SetIFld(comerc|OBJETIVO_OBJET,   FmIFld(fm, OBJET, row));
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_LOCK) != ERROR && !IsNull(comerc|OBJETIVO_PRESEN) &&
			LFld(comerc|OBJETIVO_PRESEN) != FmLFld(fm, PERPRE, row))
			Warning("El cliente/objetivo tiene asignado el presentismo %ld.\nDebe desasignarlo de este presentismo.", LFld(comerc|OBJETIVO_PRESEN));
		break;
	}
	return FM_OK;
}

void AgregarEstructura (long cliente, short objetivo) 
{
	int i;
	bool encontro=FALSE;

	for (i=0; i < ultpos; i ++) {
		if (obj[i].cliente == cliente && obj[i].objetivo == objetivo) {
			encontro = TRUE;
			break;
		}
	}

	if (!encontro) {
		obj[ultpos].cliente = cliente;
		obj[ultpos].objetivo = objetivo;
		ultpos ++;

		if (ultpos == MAX_OBJ) Error ("Tabla interna saturada");
	}
}
