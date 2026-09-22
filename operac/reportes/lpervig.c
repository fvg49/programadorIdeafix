/********************************************************************
*
* MODULE & VERSION : @(#)lpervig.c	1.3 
* DATE             : 08/11/07 
* TIME             : 14:03:34 
*
* CREATED          : 13/10/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lpervig.fmh"
#include "lpervig.rph"
#include "filial.h"

/* Funciones privadas */
static fm_status before(form, fmfield, int);
void AbrirReporte();

/* Declaraciones globales */
form fm0;
FILE   *fsalida = NULL;
report rp0 = ERROR;

/* Programa principal */
wcmd(lpervig, 1.3 11/07/08)
{
	fm_cmd cmd;
	int tipper=0;

	tnlegxfil nodo_aux;

	fm0  = OpenForm("lpervig",   FM_EABORT);


	while ((cmd = DoForm(fm0, before, NULLFP)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			return;

		switch(FmIFld(fm0, VERVIG)) {
			case 0:
				tipper=NULL_SHORT;
				break;
			case 1:
			case 2:
				tipper=FmIFld(fm0, VERVIG);
				break;
			case 3:
				tipper=MAX_SHORT;
				break;
			
		}		
        
        // Acumulo Legajos
		
		InicLegajoXusrXUid (FmIFld(fm0, EMP), FmDFld(fm0, FECHA), fm0, MENSAJE, FALSE, tipper, FmLFld(fm0, USUARIO));

		AbrirReporte();

		// Listo Legajos

		for (nodo_aux=inileg; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig) {

			if (FmIFld(fm0, PROCEDE)>0 && FmIFld(fm0, PROCEDE)!=(*nodo_aux).procede)
				continue;

			if (*FmSFld(fm0, SALIDA) != 'A') {
	            RpSetLFld(rp0, RNROLEG, (*nodo_aux).legxfil );
	            RpSetFld (rp0, RAPYNOM, (*nodo_aux).descleg );
	            RpSetIFld(rp0, RPROCED, (*nodo_aux).procede );
	            RpSetFld (rp0, RDETALL, (*nodo_aux).detalle );

				DoReport(rp0, ZLINEA);
            }
            else
				fprintf(fsalida, "%ld\t%s\t%d\t%s\n", (*nodo_aux).legxfil, (*nodo_aux).descleg, (*nodo_aux).procede, (*nodo_aux).detalle) ;
		} 

		if (*FmSFld(fm0, SALIDA) != 'A') 
			CloseReport(rp0);
		else
			fclose(fsalida);

	} 


	FinLegajoXusr();
}


static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lpervig.txt");
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
		rp0 = OpenReport("lpervig", RP_EABORT|RP_NOBEGIN);

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
		for (v_x = EMP; v_x <= DVERVIG; v_x++) {
			if (!FmIsNull(fm0, v_x)) {
				sprintf(agrega,"%s", NULL_STR);
				switch(v_x) {
					case EMP:
						sprintf(agrega, "Empresa: ");
						break; 
					case FECHA:
						sprintf(agrega, " Fecha: ");
						break; 
					case USUARIO:
						sprintf(agrega, " Usuario: ");
						break; 
					case VERVIG:
						sprintf(agrega, " Permisos por vigilador: ");
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

