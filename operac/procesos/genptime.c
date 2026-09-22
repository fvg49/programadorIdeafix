/********************************************************************
*
* MODULE & VERSION : @(#)genptime.c	1.2 
* DATE             : 00/06/22 
* TIME             : 17:22:16 
*
* CREATED          : 06/06/2000
*
* DESCRIPTION:
*      Generación de Horas Part-Time
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 

Este programa genera las horas partime para los vigiladores pedidos en el form
para el periodo solicitado.

Genera horas para las asignaciones que cumplen las siguiente condiciones: 
	* La Asignación debe ser Part-Time (campo ASIG_VIGIL)
	* El puesto al que corresponde la asignación debe tener frecuencia semanal
	* Solo genera horas para las fechas que coinciden con la asignacion de la persona
	* Si para esa fecha ya esta generado NO LO VUELVE A GENERAR

*********************************************************************/
#include <ideafix.h>
#include "genptime.fmh"
#include "operac.sch"
#include "operac.h"
#include "opedef.h"
#define  _MSG_MALV	"Controle los datos ingresados\nNo se registraron horas Part-Time"

/*Funciones Privadas */

void GraboPartTime (DATE fecha);
void GenerarHorasPTime ();

// Declaraciones globales
form     fm0;
schema   operac;

wcmd(genptime, 1.2 06/22/00)
{
	fm_status cmd;

	fm0 = OpenForm("genptime", FM_EABORT);

	operac = OpenSchema("operac", IO_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT) {
		switch (cmd) {
		case FM_ADD :
		case FM_UPDATE :
       		GenerarHorasPTime ();
		case FM_IGNORE :                           
			break;
		}
	}
}

void GenerarHorasPTime ()
{       
	dbcursor cura;
	DATE fecha;
	int  dia;
	long vigil=NULL_LONG;
	bool encontro = FALSE;
	
	cura = CreateCursor (ASIGbyLEGFEC, IO_NOT_LOCK);

	SetCursorFrom (cura, FmIFld (fm0, EMP), FmIsNull (fm0, VIGILD) ? MIN_LONG : FmLFld (fm0, VIGILD), MIN_DATE, MIN_LONG, MIN_SHORT);
	SetCursorTo   (cura, FmIFld (fm0, EMP), FmIsNull (fm0, VIGILH) ? MAX_LONG : FmLFld (fm0, VIGILH), MAX_DATE, MAX_LONG, MAX_SHORT);
 
 	while (FetchCursor (cura) != ERROR ) {
				
		if (vigil != LFld (ASIG_NROLEG)) {
			char comen[50];
			sprintf (comen, "Procesando vigilador %ld", LFld (ASIG_NROLEG));
			FmSetFld (fm0,  COMENT, comen);
			WiRefresh();
			vigil = LFld (ASIG_NROLEG);
		}		
		 	    
 	    //La asignacion tiene que ser part-time
 	    if (*SFld (ASIG_VIGIL) != 'P')
 	    	continue;
		
		//El puesto tiene que ser semanal
		SetKey (PUESTOSbyCLIENTE, LFld (ASIG_CLIENTE), IFld (ASIG_OBJETIVO), IFld (ASIG_PTOSER), IFld (ASIG_PUESTO));
		if (GetRecord (PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
			continue;

		if (*SFld (PUESTOS_CODFREC) != 'S')
			continue;
					 	    
		for (fecha = FmDFld (fm0, FDESDE); fecha <=  FmDFld (fm0, FHASTA); fecha ++) {
				DATE fechaHasta;
				
                /*Si ya termino la asignacion del tipo */
				if (DFld (ASIG_FECHAS) != NULL_DATE && fecha > DFld (ASIG_FECHAS))
					break;

			 	/* Controlo que a la fecha ya este asignado */
 				if (DFld (ASIG_FECASIG) > fecha)
 					continue;                  

				/* Veo si el dia corresponde */ 					
				for (dia=0; dia < 7 ; dia ++) {
					if (dia(fecha) != *SFld (ASIG_DIA1+dia))
						continue;
						
					encontro = TRUE;
					GraboPartTime (fecha);		
						
				}	
 		}
 	} 
	
	FmSetFld (fm0,  COMENT, NULL_STR);
	WiRefresh();
    
    if (!encontro) 
    	Warning (_MSG_MALV);
    
}

void GraboPartTime (DATE fecha)
{

	SetKey (DIASPTIMEbyEMP, IFld (ASIG_EMP), LFld (ASIG_CLIENTE), IFld (ASIG_OBJETIVO), 
							LFld (ASIG_NROLEG), IFld (ASIG_PTOSER), IFld (ASIG_PUESTO),
							IFld (ASIG_NROINT), fecha);

	/*Si ya existe no lo vuelvo a grabar */
	if (GetRecord (DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR )
		return ;

	BeginTransaction ();
	InitRecord(DIASPTIME);
	SetIFld(DIASPTIME_EMP,		IFld (ASIG_EMP));
	SetLFld(DIASPTIME_CLIENTE,  LFld (ASIG_CLIENTE));
	SetIFld(DIASPTIME_OBJETIVO, IFld (ASIG_OBJETIVO));
	SetLFld(DIASPTIME_NROLEG,   LFld (ASIG_NROLEG));
	SetIFld(DIASPTIME_TIPPTO,   IFld (ASIG_PTOSER));
	SetIFld(DIASPTIME_PUESTO,   IFld (ASIG_PUESTO));
	SetIFld(DIASPTIME_NROINT,   IFld (ASIG_NROINT));
	SetDFld(DIASPTIME_DIA,      fecha);
	SetTFld(DIASPTIME_HENT,     TFld (ASIG_HSENT));
	SetTFld(DIASPTIME_HSAL,     TFld (ASIG_HSSAL));
	PutRecord(DIASPTIME);	
	EndTransaction ();
															 
}
