/********************************************************************
*
* MODULE & VERSION : @(#)cliesp.c	1.3
* DATE             : 08/03/17
* TIME             : 10:31:17
*
* CREATED          : 01/08/2001
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
* 
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* COMENTARIO: Si se modifica la funcionalidad actualizar cliesp.hlp

*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "comerc.sch"
#include "operac.sch"
#include "billpro.sch"
#include "filial.h"

#include "cliesp.fmh"

/* Funciones privadas */
static void Lectura(fm_cmd, find_mode);
void GrabarDatos();
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

/* Declaraciones globales */
form fm0;
schema billpro;

/* Programa principal */
wcmd(cliesp, 1.3 03/17/08)
{
	fm_cmd cmd;
	fm0 = OpenForm("cliesp", FM_EABORT);

	billpro = OpenSchema("billpro", IO_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ: Lectura(cmd,THIS_KEY); break;
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();
		GrabarDatos();
		EndTransaction();
		break;
	case FM_DELETE:
		SetKey (billpro|RCLIESPbyTIPCLI, FmIFld(fm0, TIPCLI), MIN_LONG, MIN_SHORT);
		while (GetRecord(billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
			DelRecord (billpro|RCLIESP);
		}
		FmToDb(fm0, TIPGRP, DESCRIP);
		DelRecord(billpro|CLIESP);
		break;		
	case FM_IGNORE:
		FreeTable(billpro|CLIESP);
		FreeTable(billpro|RCLIESP);
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura(fm_cmd cmd, find_mode mode)
{
	int i;
	int tipcli=0;

	
    InicListaXusr(StrToI(ReadEnv("EMP")));

	SetKey(billpro|CLIESPbyTIPCLI, FmIFld(fm0, TIPCLI));
	switch(GetRecord(billpro|CLIESPbyTIPCLI, THIS_KEY, IO_LOCK|IO_TEST)) {
	case IO_LOCKED:
		FmSetStatus(fm0, FM_LOCKED);
		FindRecord(billpro|CLIESPbyTIPCLI, THIS_KEY);
		DbToFm(fm0, 0, DESCRIP);
		FmShowFlds(fm0, 0, DESCRIP);
		return;
	case ERROR:
		FmSetStatus(fm0, cmd==FM_READ ? FM_NEW : FM_EOF); return;
	}


	DbToFm(fm0, 0, DESCRIP);
	FmShowFlds(fm0, 0, DESCRIP);
	//Borro los renglones actuales
	for (i=0; !FmIsNull(fm0, CLIE, i); i++)
		FmClearFlds(fm0, CLIE, DTOBJET, i);

	tipcli=FmIFld(fm0, TIPCLI);
	SetKey (billpro|RCLIESPbyTIPCLI, NULL_SHORT, NULL_LONG, NULL_SHORT);
	for (i = 0; GetRecord(billpro|RCLIESPbyTIPCLI, NEXT_KEY, IO_NOT_LOCK) != ERROR;) {

		if(tipcli!=IFld(billpro|RCLIESP_TIPCLI))
			continue;

		if (i == FmFldLen(fm0, MULTI)) {
			Error ("No se pueden mostrar tantos renglones - Agrandar el multirenglon");
		}
		
		
		//valida el cliente/objetivo para el usuario
		if (IsNull(billpro|RCLIESP_OBJET)){
			if (ValidaClienteXusr(LFld(billpro|RCLIESP_CLIENTE)))
				continue;
		}
		else{
			if (!ValidaListaXusr(LFld(billpro|RCLIESP_CLIENTE), IFld(billpro|RCLIESP_OBJET)))
		       	continue;
		}
		
		FmSetLFld(fm0, CLIE,  LFld(billpro|RCLIESP_CLIENTE), i);
		FmSetFld (fm0, DCLIE, GetDescCli(LFld(billpro|RCLIESP_CLIENTE)), i);
		FmSetIFld(fm0, OBJET, IFld(billpro|RCLIESP_OBJET), i);
		if (IFld(billpro|RCLIESP_OBJET) != NULL_SHORT)
			FmSetFld (fm0, DOBJET, GetObjDescrip(LFld(billpro|RCLIESP_CLIENTE), LFld(billpro|RCLIESP_OBJET)), i);

		i++;

	}
}

void GrabarDatos()
{
    int i;
	//Borro los datos viejos
	SetKey (billpro|RCLIESPbyTIPCLI, IFld(billpro|CLIESP_TIPCLI), MIN_LONG, MIN_SHORT);
	while (GetRecord(billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
		DelRecord (billpro|RCLIESP);
	}

	FmToDb(fm0, TIPGRP, DESCRIP);
	PutRecord(billpro|CLIESP);

	for (i = 0; !FmIsNull(fm0, CLIE, i); i ++){
		InitRecord(billpro|RCLIESP);
		SetLFld(billpro|RCLIESP_TIPCLI, FmIFld(fm0, TIPCLI));
		FmToDb(fm0, CLIE, OBJET, i);
		PutRecord(billpro|RCLIESP);
	}
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIE:
	   	InicClientesXusr();
    	break;
    case OBJET:
	   	InicObjetivosXusr(FmLFld(fm, CLIE, row), StrToI(ReadEnv("EMP")));
    	break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
  			if (!FmIsNull(fm, CLIE, row)) {
				if (ValidaClienteXusr(FmLFld(fm, CLIE, row)))
				  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIE, row)), row);
				else {
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
			if (!FmIsNull(fm, OBJET, row)) {
				if (ValidaObjetivoXusr(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row), StrToI(ReadEnv("EMP"))))
					FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row)), row);
				else	{
					Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIE, row), FmIFld(fm, OBJET, row));
					FmSetIFld(fm, OBJET, NULL_SHORT, row);
					FmSetFld(fm, DOBJET, NULL_STR, row);
					return FM_REDO;
				}
			
			} 
		break;
	}
	return FM_OK;				
}	

