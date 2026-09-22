/********************************************************************
*
* MODULE & VERSION : @(#)supxobj.c	1.6 
* DATE             : 02/07/01 
* TIME             : 12:16:22 
*
* CREATED          : 01/10/98
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "supxobj.fmh"
#include "comerc.sch"
#include "sue.sch"
#include "comerc.h"
#include "billpro.h"
#include "sue.h"

#define INACTIVO 0
#define MAX_OBJ	1000


/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
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
wcmd(supxobj, 1.6 07/01/02)
{
	fm_cmd cmd;
	fm0 = OpenForm("supxobj", FM_EABORT);

	sue    = OpenSchema("sue", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT)
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
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int i;

	InicVector();

	SetLFld(comerc|OBJETIVO_SVISOR,  FmLFld(fm0, SUPERV));
	SetLFld(comerc|OBJETIVO_CLIENTE, MIN_LONG);
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	switch(GetRecord(comerc|OBJETIVObySVISOR, NEXT_KEY|PARTIAL_KEY, IO_LOCK|IO_TEST, 1)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(comerc|OBJETIVObySVISOR, NEXT_KEY|PARTIAL_KEY, 1);
//		DbToFm(fm0, 0, DOBJ);
//		FmShowFlds(fm0, 0, DOBJ);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}
	InicVector();
	SetLFld(comerc|OBJETIVO_SVISOR,  FmLFld(fm0, SUPERV));
	SetLFld(comerc|OBJETIVO_CLIENTE, MIN_LONG);
	SetIFld(comerc|OBJETIVO_OBJET,   MIN_SHORT);
	for (i = 0; GetRecord(comerc|OBJETIVObySVISOR, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR &&
			i < FmFldLen(fm0, MULTIS); i++ ) {

		AgregarEstructura (LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		
		FmSetLFld(fm0, CLIE,  LFld(comerc|OBJETIVO_CLIENTE), i);
		FmSetFld (fm0, DCLIE, GetDescCli(LFld(comerc|OBJETIVO_CLIENTE)), i);
		FmSetIFld(fm0, OBJET, IFld(comerc|OBJETIVO_OBJET), i);
		FmSetFld (fm0, DOBJET,GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), LFld(comerc|OBJETIVO_OBJET)), i);
	}
}

private void PutInObjetivo()
{
	int i;
	long supervisor;
             
	for (i=0; i < FmFldLen(fm0, MULTIS) && !FmIsNull(fm0, CLIE, i); i++) {
		AgregarEstructura ( FmLFld(fm0, CLIE,  i), FmIFld(fm0, OBJET, i));
	}

	/* Los clientes - objetivos que se desasignaron a este supervisor no se borran
	   se guardan con supervisor NULL */
		
	for (i = 0;  i < ultpos; i++) {

		if (!InMulti(obj[i].cliente, obj[i].objetivo)) {
			supervisor = NULL_LONG;
		}
		else {
			supervisor = FmLFld(fm0, SUPERV);
		}                                    
		
	 	SetLFld(comerc|OBJETIVO_CLIENTE, obj[i].cliente);
		SetIFld(comerc|OBJETIVO_OBJET,   obj[i].objetivo);
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_LOCK) != ERROR && 
			LFld(comerc|OBJETIVO_SVISOR) != supervisor)  {
				SetLFld(comerc|OBJETIVO_SVISOR, supervisor);
				
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

static fm_status after(form fm, fmfield fno, int row)
{
	long nroleg;

	switch (fno) {
	case SUPERV:
		if (FmKeyCode(fm) == K_META) {
			nroleg = ERROR;
			if ( (nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
				return FM_REDO;
			FmSetLFld(fm, fno, nroleg);
		}
		SetIFld(sue|PER_EMP, FmIFld(fm, EMP));
		SetLFld(sue|PER_NROLEG, FmLFld(fm, fno));
		GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
		if ((GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) && IFld(sue|PER_ACTIVO) == INACTIVO) {
			Warning("Legajo inactivo.");
//			return FM_REDO;
		}
		break;
	case OBJET:
	 	SetLFld(comerc|OBJETIVO_CLIENTE, FmLFld(fm, CLIE, row));
		SetIFld(comerc|OBJETIVO_OBJET,   FmIFld(fm, OBJET, row));
		if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_LOCK) != ERROR && !IsNull(comerc|OBJETIVO_SVISOR) &&
			LFld(comerc|OBJETIVO_SVISOR) != FmLFld(fm, SUPERV, row))
			Warning("El cliente/objetivo tiene asignado el supervisor %ld.\nDebe desasignarlo de este supervisor.", LFld(comerc|OBJETIVO_SVISOR));
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
