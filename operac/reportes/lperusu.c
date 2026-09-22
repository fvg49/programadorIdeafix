/********************************************************************
*
* MODULE & VERSION : @(#)lperusu.c	1.4
* DATE             : 09/05/19
* TIME             : 11:47:18
*
* CREATED          : 08/08/08
*
* DESCRIPTION:
*      Poder saber quien puede tocar cada vigilador
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lperusu.fmh"
#include "lperusu.rph"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "sue.sch"
#include "comerc.h"
#include "operac.h"
#include "filial.h"

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
void AbrirReporte();

/* Declaraciones globales */
form fm0;
FILE   *fsalida = NULL;
report rp0 = ERROR;
schema comerc, bill, v_sue, operac;
char g_filial[8];

int  g_obj   = 0;
long g_clie  = 0;



/* Programa principal */
wcmd(lperusu, 1.4 05/19/09)
{
	fm_cmd cmd;
	int v_grupo;
	char v_detalle [100], v_decpermi [15], v_delega[6];
	dbcursor c_pervig, c_filxgrup, c_filxgrup2;

	fm0  = OpenForm("lperusu",   FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill = OpenSchema("bill", IO_EABORT);
	v_sue = OpenSchema("sue", IO_EABORT);


	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			return;

		GetCliObjEfectivo(FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), FmDFld(fm0, FECHA), &g_clie, &g_obj);
        sprintf(g_filial, "%s", GetFilialDeObj(FmIFld(fm0, EMP), g_clie, g_obj));

		if (g_clie==NULL_LONG && g_obj==NULL_SHORT){
			WiMsg("El legajo %ld - %s lo pueden asignar todas las filiales por no estar efectivo en ningun cliente", FmLFld(fm0, NROLEG), FmSFld(fm0, DNROLEG));
			continue;
		}
	
		AbrirReporte();


		sprintf(v_detalle, "Por pertenecer a la filial %s - %s", g_filial, GetDescFilial(g_filial));

		// POR TENER FILIAL

 		if (FmIFld(fm0, PROCEDE)==0 || FmIFld(fm0, PROCEDE)==_PROCED_ASIGNA) {
			SetKey(comerc|USRXFILbyFILIAL, g_filial, NULL_SHORT);
			while(GetRecord(comerc|USRXFILbyFILIAL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1)!=ERROR) {
				if (*FmSFld(fm0, SALIDA) != 'A') {
					RpSetLFld(rp0, RUID, IFld(comerc|USRXFIL_CODUSU) );
					SetKey(bill|SUSUARIObyUID, IFld(comerc|USRXFIL_CODUSU));
					if (GetRecord(bill|SUSUARIObyUID, THIS_KEY, IO_NOT_LOCK)!=ERROR)
						RpSetFld (rp0, RDUID, SFld(bill|SUSUARIO_DESCRIP));
					else
						RpSetFld (rp0, RDUID, NULL_STR );
					RpSetFld (rp0, RDETALL,  v_detalle);

					DoReport(rp0, ZLINEA);

				}

			}

 		}
	

	    // POR PERMISOS

 		if (FmIFld(fm0, PROCEDE)==0 || FmIFld(fm0, PROCEDE)==_PROCED_PERVIG) {
			c_pervig  = CreateCursor(operac|PERVIGbyEMP, IO_NOT_LOCK);

	 		SetCursorFrom(c_pervig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), NULL_STR,   NULL_STR,   NULL_SHORT);
	 		SetCursorTo  (c_pervig, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), HIGH_VALUE, HIGH_VALUE, MAX_SHORT);

		    while(FetchCursor(c_pervig) != ERROR) {
				if (!IFld(operac|PERVIG_ACTIVO))
					continue;

				// Saco los permisos que se terminaron antes de la ultima fecha de cierre
				if (!IsNull(operac|PERVIG_FECFIN) && FmDFld(fm0, FECHA)> DFld(operac|PERVIG_FECFIN))
					continue;


				sprintf(v_decpermi, "%s", GetTPermi(FmIFld(fm0, EMP), IFld(operac|PERVIG_TIPPER)) );
		        sprintf(v_detalle, "Por permiso de %s Filial Origen: %s-%s Destino: %s-%s", v_decpermi,  
				                            SFld(operac|PERVIG_DELORI),  SFld(operac|PERVIG_FILORI),
				                            SFld(operac|PERVIG_DELEGA),  SFld(operac|PERVIG_FILIAL));

				SetKey(comerc|USRXFILbyFILIAL, SFld(operac|PERVIG_FILIAL), NULL_SHORT);
				while(GetRecord(comerc|USRXFILbyFILIAL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1)!=ERROR) {

					if (*FmSFld(fm0, SALIDA) != 'A') {
						RpSetLFld(rp0, RUID, IFld(comerc|USRXFIL_CODUSU) );
						SetKey(bill|SUSUARIObyUID, IFld(comerc|USRXFIL_CODUSU));
						if (GetRecord(bill|SUSUARIObyUID, THIS_KEY, IO_NOT_LOCK)!=ERROR)
							RpSetFld (rp0, RDUID, SFld(bill|SUSUARIO_DESCRIP));
						else
							RpSetFld (rp0, RDUID, NULL_STR );

						RpSetFld (rp0, RDETALL,  v_detalle);

						DoReport(rp0, ZLINEA);

				    } 
			    } 
			}
		    DeleteCursor(c_pervig);
		}

	    // POR GRUPOS DE FILIALES PARA INSERCION

 		if (FmIFld(fm0, PROCEDE)==0 || FmIFld(fm0, PROCEDE)==_PROCED_GRUFIL) {

			c_filxgrup = CreateCursor(operac|FILXGRUPbyFIL, IO_NOT_LOCK);
			c_filxgrup2= CreateCursor(operac|FILXGRUPbyEMP, IO_NOT_LOCK);

			sprintf(v_delega, "%s", GetDelegacion(g_filial) );

			// Leo grupos en los que esta la filial
			SetCursorFrom(c_filxgrup, FmIFld(fm0, EMP), v_delega, g_filial, NULL_SHORT);
			SetCursorTo(c_filxgrup,   FmIFld(fm0, EMP), v_delega, g_filial, MAX_SHORT);
		    while(FetchCursor(c_filxgrup) != ERROR) {
				v_grupo=IFld(operac|FILXGRUP_GRUPO);


				PushRecord(operac|FILXGRUP);
				// Acumulo todas las filiales de grupo
				SetCursorFrom(c_filxgrup2, FmIFld(fm0, EMP), v_grupo, NULL_STR,   NULL_STR);
				SetCursorTo  (c_filxgrup2, FmIFld(fm0, EMP), v_grupo, HIGH_VALUE, HIGH_VALUE);
			    while(FetchCursor(c_filxgrup2) != ERROR) {
					if (strcmp(SFld(operac|FILXGRUP_FILIAL), g_filial)==0)
						if (strcmp(SFld(operac|FILXGRUP_DELEGA), v_delega)==0)
							continue;

			        sprintf(v_detalle, "Permiso de insecion porque pertenece al grupo de Filiales %d, Filial %s ", v_grupo, SFld(operac|FILXGRUP_FILIAL));

					SetKey(comerc|USRXFILbyFILIAL, SFld(operac|FILXGRUP_FILIAL), NULL_SHORT);
					while(GetRecord(comerc|USRXFILbyFILIAL, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1)!=ERROR) {

						if (*FmSFld(fm0, SALIDA) != 'A') {
							RpSetLFld(rp0, RUID, IFld(comerc|USRXFIL_CODUSU) );
							SetKey(bill|SUSUARIObyUID, IFld(comerc|USRXFIL_CODUSU));
							if (GetRecord(bill|SUSUARIObyUID, THIS_KEY, IO_NOT_LOCK)!=ERROR)
								RpSetFld (rp0, RDUID, SFld(bill|SUSUARIO_DESCRIP));
							else
								RpSetFld (rp0, RDUID, NULL_STR );

							RpSetFld (rp0, RDETALL,  v_detalle);

							DoReport(rp0, ZLINEA);

					    } 
				    }

			    } 
				PopRecord(operac|FILXGRUP);
		    }
		    DeleteCursor(c_filxgrup2);
		    DeleteCursor(c_filxgrup);

		}

		if (*FmSFld(fm0, SALIDA) != 'A') 
			CloseReport(rp0);
		else
			fclose(fsalida);

	} 

}


static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lperusu.txt");
	break;
	}
	return FM_OK;
}

static fm_status after (form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROLEG:
		if (!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(PER_RELACION)) ||
		  (LFld(PER_CODCCOS) >= 10000 && LFld(PER_CODCCOS) < 100000)) {
			Warning("Legajo %ld no es vigilador", LFld(PER_NROLEG));
			return FM_REDO;
		}
		
		if (!LegActivo(FmIFld(fm0, EMP), FmLFld(fm0, fno))) {
			//leo en asig porque puede estar asignado en cliente especial y todavia no haber pasado a asigh
			SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), FmLFld(fm0, fno), NULL_LONG, NULL_SHORT);
			if(GetRecord(operac|ASIGbyNROLEG, PARTIAL_KEY|NEXT_KEY, IO_NOT_LOCK, 2) != ERROR)
				WiMsg("El Legajo %ld está Inactivo con Fecha de Egreso %.3D, su ultima asignación fue en Empresa %d Cliente %ld Objetivo %d", 
				       FmLFld(fm0, fno), DFld(PER_FECEGR), IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
            else {  //leo en asigh
 				SetKey(operac|ASIGHbyLEGFEC, FmIFld(fm0, EMP), FmLFld(fm0, fno), DFld(PER_FECEGR), NULL_LONG, NULL_SHORT);
				if(GetRecord(operac|ASIGHbyLEGFEC, PARTIAL_KEY|NEXT_KEY, IO_NOT_LOCK, 3) != ERROR)
					WiMsg("El Legajo %ld está Inactivo con Fecha de Egreso %.3D, su ultima asignación fue en Empresa %d Cliente %ld Objetivo %d", 
						   FmLFld(fm0, fno), DFld(PER_FECEGR), IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
            }	
			
			return FM_REDO;
		}	
		break;
	}
	return FM_OK;
}

void AbrirReporte()
{
	char auxi[5][168], agrega[100];
	char auxdia[20];
	int v_x, v_y = 0;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		rp0 = OpenReport("lperusu", RP_EABORT|RP_NOBEGIN);

		//Si la salida es Impresora
		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

		//Si la salida es Terminal
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

		BeginReport(rp0, 1, NULL_STR);

		sprintf(auxi[0],"%s", NULL_STR);
		sprintf(auxi[1],"%s", NULL_STR);
		sprintf(auxi[2],"%s", NULL_STR);
		sprintf(auxi[3],"%s", NULL_STR);
		sprintf(auxdia, "%s", NULL_STR);
		for (v_x = EMP; v_x <= DNROLEG; v_x++) {
			if (!FmIsNull(fm0, v_x)) {
				sprintf(agrega,"%s", NULL_STR);
				switch(v_x) {
					case EMP:
						sprintf(agrega, "Empresa: ");
						break; 
					case FECHA:
						sprintf(agrega, " Fecha: ");
						break; 
					case NROLEG:
						sprintf(agrega, " Legajo: ");
						break; 

				}
				switch(v_x) {
					case FECHA:
						DToStr(FmDFld(fm0, v_x), auxdia, DFMT_SEPAR);
						sprintf(agrega, "%s %s", agrega , auxdia);
						break;
					default: 
						sprintf(agrega, "%s %s", agrega , FmSFld(fm0, v_x));
						break;
				}
				if ((strlen(auxi[v_y])+strlen(agrega)) > 150)
					 v_y++;

				sprintf(auxi[v_y], "%s %s ", auxi[v_y], agrega);
			}
		}
		RpSetFld(rp0, RSELEC,  auxi[0]);
		RpSetFld(rp0, RSELEC1, auxi[1]);
		RpSetFld(rp0, RSELEC2, auxi[2]);
	}
	else {
		fsalida = fopen(FmSFld(fm0, NOMARCH), "w");
		fprintf(fsalida, "Nro.Legajo\tNombre y Apellido\tProcedencia\tDetalle\n") ;
	}
} 

