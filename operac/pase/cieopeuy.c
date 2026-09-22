/********************************************************************
*
* MODULE & VERSION : @(#)cieopeuy.c	1.2 
* DATE             : 10/01/28 
* TIME             : 15:42:31
*
* CREATED          : 05/01/10
*
* DESCRIPTION:
*	Este proceso se encarga de realizar el cierre de operaciones para Uruguay.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*********************************************************************/
#include <ideafix.h>
#include "comgral.sch"
#include "sue.sch"
#include "comgral.h"
#include "operac.h"
#include "billpro.h"
#include "webinter.h"
#include "cieopeuy.fmh"


// Funciones privadas.
static fm_status after(form fm, fmfield fn0, int row);
static fm_status before(form fm, fmfield fn0, int row); 
static void Lectura();


// Variables globales.
form fm0;
schema comgral;
fm_cmd cmd;
tnparame inicio;
char g_codprog[20], g_oi_sesion[30];
char v_msg[100];
int v_usuar;

// Programa principal
wcmd(cieopeuy, 1.2 01/28/10)
{
	char v_aux[12];

	v_usuar=GetUid();

	comgral = OpenSchema("comgral", IO_EABORT);
	fm0 = OpenForm("cieopeuy", FM_EABORT);

	sprintf(g_codprog,"%s", argv[0]);
	sprintf(g_oi_sesion,"%.3D-%d", Today(), getpid());


	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ: Lectura(); break;
	case FM_ADD:
	case FM_UPDATE:

		SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), PARNOV_CIEOPERA, 1, FmDFld(fm0, FECCIE, 0));
        if(GetRecord(comgral|DEPANObyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR) {

			SetIFld(comgral|DEPANO_ACT, TRUE);
			sprintf(v_aux, "%.3D", FmDFld(fm0, FECCIE, 0));
			SetFld(comgral|DEPANO_VALOR, v_aux);
			PutRecord(comgral|DEPANO);

			sprintf(v_msg, "Cierre %.3D Grabado por %d - %s %.3D el %.3D - %.3T", FmDFld(fm0, FECCIE, 0), v_usuar, UserName(v_usuar), Today(), Hour());
			GrabaMsg(g_codprog, g_oi_sesion, _WMSGLOG_TIPO_MENSAJE, v_msg);
        	
        }
        else 
			WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 3000), "Esta fecha ya estaba grabada %.3D", FmDFld(fm0, FECCIE, 0));

		break;

	case FM_DELETE:
		break;
	}
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura()
{
   
   tnparame nodo_aux;
   int v_i=0;

   inicio =LisParNovNF(FmIFld(fm0,EMP ), PARNOV_CIEOPERA, _ORDEN_PARNOV_FEC_DESC);
   
   for (nodo_aux=inicio, v_i=0; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig, v_i++) {
		
		FmSetDFld(fm0, FECCIE, StrToD((*nodo_aux).parame), v_i);
		//fprintf(stderr, " %s\t%d\t%.3D\n", (*nodo_aux).parame, (*nodo_aux).nroren, (*nodo_aux).fecvig);
   } 
   BorNParame(inicio);
    	
}

static fm_status before(form fm, fmfield fno, int row)
{
	
	switch (fno) {
		case FECCIE:

			if (row != 0)
				return FM_SKIP;
           
			SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), PARNOV_CIEOPERA, 1, FmDFld(fm, FECCIE, row));
        	if(GetRecord(comgral|DEPANObyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) 
        		FmSetDisplayOnly(fm, fno, fno, TRUE);	
        	else
        		FmSetDisplayOnly(fm, fno, fno, FALSE);


			break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	DATE v_ultcie=NULL_DATE;

	switch (fno) {
		case FECCIE:
			if (FmChgFld(fm) && FmKeyCode(fm)!=K_IGNORE){
				v_ultcie=StrToD(GetParNov(FmIFld(fm0,EMP ), PARNOV_CIEOPERA, 1, MAX_DATE));
			    if (FmDFld(fm, fno, row)<=v_ultcie){
					WiDialog(WD_OK, WD_OK, TituloMsg(TMSG_WAR, 3001), "La fecha debe ser posterior a %.3D", v_ultcie);
			    	return FM_REDO;
			    }
			}

			if (FmKeyCode(fm)==K_DEL) {
				SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), PARNOV_CIEOPERA, 1, FmDFld(fm, FECCIE, row));
		       	if(GetRecord(comgral|DEPANObyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
					if (UsrInGrupo(GRPREVOPE, GetUid())){
						if(WiDialog(WD_YES|WD_NO, WD_YES, TituloMsg(TMSG_WAR, 3002), "Seguro que desea revertir el cierre del día %.3D", FmDFld(fm, FECCIE, row))==WD_YES) {
							DelRecord(comgral|DEPANO);

							sprintf(v_msg, "Cierre %.3D Borrado  por %d - %s %.3D el %.3D - %.3T", FmDFld(fm0, FECCIE, row), v_usuar, UserName(v_usuar), Today(), Hour());
							GrabaMsg(g_codprog, g_oi_sesion, _WMSGLOG_TIPO_MENSAJE, v_msg);
						}
					}
					else
						return FM_REDO;
				}

 			}

			if (FmKeyCode(fm)==K_INS) {
				SetKey(comgral|DEPANObyEMP, FmIFld(fm0, EMP), PARNOV_CIEOPERA, 1, FmDFld(fm, FECCIE, row));
	        	if(GetRecord(comgral|DEPANObyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR) 
					return FM_REDO;
			} 


			break;
	}
	return FM_OK;
}


