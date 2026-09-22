/********************************************************************
*
* MODULE & VERSION : @(#)ardptime.c	1.1 
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
#include "ardptime.fmh"
#include "operac.sch"
#include "operac.h"

#define DEBUG 0

#define ACC_NADA 0
#define ACC_MUEVE_CASO1 1
#define ACC_BORRA_CASO1 2
#define ACC_BORRA_CASO2 3
#define ACC_BORRA_CASO3 4
#define ACC_MUEVE_CASO2 5

/* Declaraciones globales */
form   fm0;
schema operac, comerc, bill;
report rp;
int accion;
FILE *salida;

/* Programa principal */
wcmd(ardptime, 1.1 12/28/00)
{
	fm_cmd cmd;             
	dbcursor c_asigh, c_diasptime;
	int obj;
	long cli;
	long total, cont; 
	char aux[40];
	
	operac = OpenSchema("operac", IO_EABORT);
	fm0 = OpenForm("ardptime", FM_EABORT);
    
	while((cmd=DoForm(fm0, NULLFP, NULLFP))!= FM_EXIT) {
		switch(cmd) {
			case FM_UPDATE:

				sprintf(aux, "%s.txt", FmSFld(fm0, SALIDA));
				if((salida=fopen(aux, "w"))==NULL)
					Error("No se pudo habrir el archivo aux");

				cli = MAX_LONG;
				if (!FmIsNull(fm0, CLIHAS))
					cli = FmLFld(fm0, CLIHAS);

				obj = MAX_SHORT;
				if (!FmIsNull(fm0, OBJHAS))
					obj = FmIFld(fm0, OBJHAS);
				cont=0;
				c_diasptime=CreateCursor(operac|DIASPTIMEbyEMP, IO_NOT_LOCK);
				SetCursorFrom(c_diasptime, FmIFld(fm0, EMP), FmLFld(fm0, CLIDES), FmIFld(fm0, OBJDES),
				                           NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_DATE);

				SetCursorTo  (c_diasptime, FmIFld(fm0, EMP), cli, obj,MAX_LONG, MAX_SHORT, MAX_SHORT,
				                           MAX_SHORT, MAX_DATE);
				total=CountCursor(c_diasptime);
				while (FetchCursor(c_diasptime)!=ERROR) {
					if (((cont++)%100)==0) {
						DisplayMsg(FALSE, "     %ld%%",(cont*100)/total);
						WiRefresh();
					}
					accion=ACC_NADA;
					if (DEBUG==1)
						fprintf (stderr,"Accion %d %s (%d)\t :%s\n", accion, __FILE__, __LINE__, __PRETTY_FUNCTION__);

					SetKey(operac|ASIGbyEMP, IFld(operac|DIASPTIME_EMP), LFld(operac|DIASPTIME_CLIENTE),
					                           IFld(operac|DIASPTIME_OBJETIVO), LFld(operac|DIASPTIME_NROLEG),
					                           IFld(operac|DIASPTIME_TIPPTO), IFld(operac|DIASPTIME_PUESTO),
					                           IFld(operac|DIASPTIME_NROINT));
					if(GetRecord(operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR ||
					   DFld(operac|DIASPTIME_DIA) < DFld(operac|ASIG_FECASIG)) {

						accion=ACC_BORRA_CASO1;
						c_asigh=CreateCursor(operac|ASIGHbyLEGFEC, IO_NOT_LOCK);
						SetCursorFrom(c_asigh, IFld(operac|DIASPTIME_EMP), LFld(operac|DIASPTIME_NROLEG),
						                       DFld(operac|DIASPTIME_DIA), LFld(operac|DIASPTIME_CLIENTE),
						                       IFld(operac|DIASPTIME_OBJETIVO));

						SetCursorTo  (c_asigh, IFld(operac|DIASPTIME_EMP), LFld(operac|DIASPTIME_NROLEG),
						                       MAX_DATE, LFld(operac|DIASPTIME_CLIENTE),
						                       IFld(operac|DIASPTIME_OBJETIVO));
						                       
						while (FetchCursor(c_asigh)!=ERROR) {
							if (strcmp(SFld(operac|ASIGH_VIGIL), PARTTIME) != 0)
								continue;

							if(LFld(operac|DIASPTIME_CLIENTE)!=LFld(operac|ASIGH_CLIENTE) ||
							   LFld(operac|DIASPTIME_OBJETIVO)!=LFld(operac|ASIGH_OBJETIVO) ||
							   IFld(operac|DIASPTIME_TIPPTO)!= IFld(operac|ASIGH_PTOSER) || 
							   IFld(operac|DIASPTIME_PUESTO)!= IFld(operac|ASIGH_PUESTO) ||
							   IFld(operac|DIASPTIME_NROINT)!= IFld(operac|ASIGH_NROINT) )
								continue;

							if(DFld(operac|DIASPTIME_DIA)>DFld(operac|ASIGH_FECBAJ))
								accion=ACC_BORRA_CASO2;
							else {
								if(Today()>=DFld(operac|ASIGH_FECALT) &&
								   Today()<=DFld(operac|ASIGH_FECBAJ) &&
								   DFld(operac|DIASPTIME_DIA)>=DFld(operac|ASIGH_FECALT) &&
								   DFld(operac|DIASPTIME_DIA)<=DFld(operac|ASIGH_FECBAJ)) {
								   	accion=ACC_MUEVE_CASO1;
								}
								else {
									if(Today()>DFld(operac|ASIGH_FECBAJ) &&
									   DFld(operac|DIASPTIME_DIA)>=DFld(operac|ASIGH_FECALT) &&
									   DFld(operac|DIASPTIME_DIA)<=DFld(operac|ASIGH_FECBAJ)) {
										if (DFld(operac|ASIGH_FECBAJ)<Today()-60)
										   	accion=ACC_BORRA_CASO3;
										else
										   	accion=ACC_MUEVE_CASO2;
									}
								} 
							}
						} 
						DeleteCursor(c_asigh);
					}
					if (accion==0)
						continue;

					fprintf (salida , "%d\t" , accion);
					fprintf (salida , "%d\t" , IFld(operac|DIASPTIME_EMP));
					fprintf (salida , "%ld\t", LFld(operac|DIASPTIME_CLIENTE));
					fprintf (salida , "%d\t" , IFld(operac|DIASPTIME_OBJETIVO));
					fprintf (salida , "%ld\t", LFld(operac|DIASPTIME_NROLEG));
					fprintf (salida , "%D\t" , DFld(operac|DIASPTIME_DIA));
					fprintf (salida , "%T\t" , TFld(operac|DIASPTIME_HENT));
					fprintf (salida , "%T\t" , TFld(operac|DIASPTIME_HSAL));
					fprintf (salida , "%D\t" , DFld(operac|DIASPTIME_CDATE));
					fprintf (salida , "%T\t" , TFld(operac|DIASPTIME_CTIME));
					fprintf (salida , "%ld\t", LFld(operac|DIASPTIME_CUID));
					fprintf (salida , "%D\t" , DFld(operac|DIASPTIME_MDATE));
					fprintf (salida , "%T\t" , TFld(operac|DIASPTIME_MTIME));
					fprintf (salida , "%ld\t", LFld(operac|DIASPTIME_MUID));
					fprintf (salida , "%d\t" , IFld(operac|DIASPTIME_TIPPTO));
					fprintf (salida , "%d\t" , IFld(operac|DIASPTIME_PUESTO));
					fprintf (salida , "%d\n" , IFld(operac|DIASPTIME_NROINT));

					if(!FmIFld(fm0, EJECUTA))
						continue;

					BeginTransaction();
					switch(accion) {
						case ACC_NADA: 
							break;
						case ACC_MUEVE_CASO1: 
						case ACC_MUEVE_CASO2: 
							InitRecord(operac|DIASPTIMEH);

							SetKey(operac|DIASPTIMEH, IFld(operac|DIASPTIME_EMP),
							                          LFld(operac|DIASPTIME_CLIENTE),
							                          IFld(operac|DIASPTIME_OBJETIVO),
							                          LFld(operac|DIASPTIME_NROLEG),
							                          IFld(operac|DIASPTIME_TIPPTO),
							                          IFld(operac|DIASPTIME_PUESTO),
							                          IFld(operac|DIASPTIME_NROINT),
							                          DFld(operac|DIASPTIME_DIA));
							                          

							SetTFld(operac|DIASPTIMEH_HENT,     TFld(operac|DIASPTIME_HENT));
							SetTFld(operac|DIASPTIMEH_HSAL,     TFld(operac|DIASPTIME_HSAL));

							PutRecord(operac|DIASPTIMEH);
							FreeTable(operac|DIASPTIMEH);
							
						case ACC_BORRA_CASO1:
						case ACC_BORRA_CASO2:
						case ACC_BORRA_CASO3:
							DelRecord(operac|DIASPTIME);
                        break;
					}
					EndTransaction();
				}
				DeleteCursor(c_diasptime);
                fclose(salida);
				break;  

		}
	}
}

