/********************************************************************
*
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : chimuris (Andrés Chimuris)
*
* DESCRIPTION:
*	Listado de Control de los retroactivos.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "ambiente.h"
#include "billpro.h"
#include "lcretro.fmh"
#include "lcretro.rph"

#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "asist.sch"
#include "filial.h"

/* Funciones privadas */
//static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
void CargarReporte();       
void AbrirReporte();        
void CerrarReporte();
void DevuelveUser();  
void AbrirArchivo();
void CargarArchivo();
void CerrarArchivo();

/* Declaraciones globales */
schema comerc, operac, billpro, sue, bill;
form fm0;                   
FILE   *fp = NULL;
report rp0 = NULL;
dbcursor c_retro; 
dbcursor c_susuario;
char mensaje [50];
int i = 0;
int emp;      
long g_uid;
char tiporep;


/* Programa principal */
wcmd(lcretro, %I% %G%)
{
	fm_cmd cmd;
	char argaux[10];

	comerc  = OpenSchema("comerc", IO_EABORT);
	operac 	= OpenSchema("operac", IO_EABORT);
	billpro = OpenSchema("billpro",IO_EABORT);
	sue     = OpenSchema("sue",    IO_EABORT);
	bill	= OpenSchema("bill",   IO_EABORT);

	fm0 = OpenForm("lcretro", FM_EABORT);    
	emp = StrToI(ReadEnv("EMP"));

	sprintf(argaux, "%s", argv[1]);

	tiporep='P';
	if (argc !=	1) {
		switch (argaux[0]) {
			case 'R': 
				tiporep='R';
				break;
			default: 
				tiporep='P';
		}
	}


	WiMsg("tipo reporte %c", tiporep);

	if (tiporep=='R') {
		FmSetFld (fm0, TITULO, "Por fecha de registracion");
		FmSetDFld(fm0, FDESDE, Today()-365);
		FmSetDFld(fm0, FRDESDE, Today()-30);
		
	}
	else{
		FmSetFld(fm0, TITULO, "Por fecha del parte");
		FmSetDFld(fm0, FDESDE, Today()-30);
		FmSetDFld(fm0, FRDESDE, Today()-365);
	}

	FmSetDFld(fm0, FHASTA, Today());
	FmSetDFld(fm0, FRHASTA, Today());
	

	FmSetFld(fm0, COMENTARIO, "[1mProcesando Permisos de Filiales por Usuario[0m");
	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENTARIO, "");
	WiRefresh();
   
	while ( (cmd = DoForm(fm0, before, NULLFP)) != FM_EXIT) {	

		switch (cmd) {
		case FM_UPDATE:
			if(*FmSFld(fm0, SALIDA) != 'A'){
				CargarReporte();
				CerrarReporte();      
			}
			else {
				CargarArchivo();
				CerrarArchivo();
			}
					
			break;
		case FM_IGNORE:
			FmClearAllFlds(cmd);
		}    
	}

	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();	
}
    
void AbrirReporte() {
	rp0 = OpenReport("lcretro", RP_EABORT|RP_NOBEGIN);
}          

void CerrarReporte() {
	CloseReport(rp0);
}

void CargarReporte() {

	   AbrirReporte();
       RpSetOutput(rp0, *FmSFld(fm0, SALIDA) == 'T' ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
       c_retro = CreateCursor(operac|RETRObyRDIA, IO_NOT_LOCK);
                     //rdia(emp, dia, cliente, objetivo),
	   SetCursorFrom (c_retro, emp, FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
	   SetCursorTo   (c_retro, emp, FmDFld(fm0, FHASTA),  MIN_LONG, MIN_SHORT);
	   

	   BeginReport(rp0, 1, NULL_STR);
	   
	  
	   while(FetchCursor(c_retro)!=ERROR) {

			if (DFld(operac|RETRO_FECREG) < FmDFld(fm0, FRDESDE))
				continue;

			if (DFld(operac|RETRO_FECREG) > FmDFld(fm0, FRHASTA))
				continue;

			if (!ValidaListaXusr(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO)))
				continue;

			RpSetLFld	(rp0, RNROLEG,	 	 LFld(operac|RETRO_NROLEG)); 
			RpSetDFld	(rp0, RDIA,	     	 DFld(operac|RETRO_DIA)); 
			RpSetDFld	(rp0, RFCREG,	 	 DFld(operac|RETRO_FECREG));
			RpSetLFld   (rp0, RCLIENTE,  	 LFld(operac|RETRO_CLIENTE));
			RpSetIFld   (rp0, ROBJETIVO, 	 IFld(operac|RETRO_OBJETIVO));
			RpSetTFld   (rp0, RHENTRADA, 	 TFld(operac|RETRO_HORAENT));
			RpSetTFld   (rp0, RHSALIDA,      TFld(operac|RETRO_HORASAL));
			RpSetFFld   (rp0, RHNORMALES,    FFld(operac|RETRO_HSNOR));
			RpSetFFld   (rp0, RHEXT50,       FFld(operac|RETRO_HS50));
			RpSetFFld   (rp0, RHEXT100FRAN,  FFld(operac|RETRO_HS100F));
			RpSetFFld   (rp0, RHEXT100FER,   FFld(operac|RETRO_HS100FE));
			RpSetFld    (rp0, RCONDIC,       SFld(operac|RETRO_CONDIC));
			RpSetFFld   (rp0, RDHSNOR,       FFld(operac|RETRO_DHSNOR));
			RpSetFFld   (rp0, RDHS50,        FFld(operac|RETRO_DHS50));
			RpSetFFld   (rp0, RDHS100F,      FFld(operac|RETRO_DHS100F));
			RpSetFFld   (rp0, RDHS100FE,     FFld(operac|RETRO_DHS100FE));
			RpSetLFld   (rp0, RCUID,		 LFld(operac|RETRO_CUID));
			g_uid = LFld(operac|RETRO_CUID);
			
		    DevuelveUser();
	  		DoReport (rp0, LEGAJO);		
	  	} 
  	  	DeleteCursor(c_retro);
}           

void DevuelveUser() {

   	c_susuario = CreateCursor(bill|SUSUARIObyUID, IO_NOT_LOCK);
                     //rdia(emp, dia, cliente, objetivo),
   	SetCursorFrom (c_susuario, g_uid);//, FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
   	SetCursorTo   (c_susuario, g_uid);//, FmDFld(fm0, FHASTA),  MIN_LONG, MIN_SHORT);        
   
   	while(FetchCursor(c_susuario)!=ERROR) {
   		if(*FmSFld(fm0, SALIDA) != 'A')
    		RpSetFld (rp0, RUSUARIO, SFld(bill|SUSUARIO_DESCRIP));	 
    	else
    		fprintf(fp, "%s\n", SFld(bill|SUSUARIO_DESCRIP));
   }
   DeleteCursor(c_susuario);
}


private fm_status before(form fm, fmfield fno, int row)
{   
	switch (fno) {
	
	case ARCHIVO:
		if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, ARCHIVO))
			FmSetFld(fm0, ARCHIVO, "lcretro.txt");
		break;
	case FDESDE:
	case FHASTA:
		if (tiporep=='R')
			return FM_SKIP;

		break;
	case FRDESDE:
	case FRHASTA:
		if (tiporep=='P')
			return FM_SKIP;

		break;
	}
   	return FM_OK;
}    

/*
static fm_status after(form fm, fmfield fno, int row)
{  
	   

	WiMsg("Entro al fter");
} */	


void AbrirArchivo()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));
		fprintf(fp, "NRO. DE LEGAJO\tDIA\tFECHA DE CREACION\tCLIENTE\tOBJETIVO\tHS. ENTRADA\tHS. SALIDA\tHS. NORMALES\tHS. EXTRAS 50%\tHS. EXTRAS 100% FRANCO\tHS. EXTRAS 100% FERIADO\tCONDICION DE TRABAJO\tDIF. HORAS NORMALES\tDIF. DE HORAS EXTRAS AL 50%\tDIF. DE HORAS EXTRAS AL 100% FRANCO\tDIF. DE HORAS EXTRAS AL 100% FERIADO\tCUID\tUSUARIO\n");	
	}
}






void CargarArchivo() {

	   AbrirArchivo();
       c_retro = CreateCursor(operac|RETRObyRDIA, IO_NOT_LOCK);
                     //rdia(emp, dia, cliente, objetivo),
	   SetCursorFrom (c_retro, emp, FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
	   SetCursorTo   (c_retro, emp, FmDFld(fm0, FHASTA),  MIN_LONG, MIN_SHORT);
	   
	  
	   while(FetchCursor(c_retro)!=ERROR) {
			if (!ValidaListaXusr(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO)))
				continue;
          
			fprintf(fp, "%ld\t",			 LFld(operac|RETRO_NROLEG));//NRO. DE LEGAJO
			fprintf(fp, "%.3D\t", 	     	 DFld(operac|RETRO_DIA)); //DIA	
			fprintf(fp, "%.3D\t",	 		 DFld(operac|RETRO_FECREG));//FECHA DE CREACION
			fprintf(fp, "%ld\t",      		 LFld(operac|RETRO_CLIENTE));
			fprintf(fp, "%d\t",      	     IFld(operac|RETRO_OBJETIVO));
			fprintf(fp, "%.3T\t", 			 TFld(operac|RETRO_HORAENT));//HS. ENTRADA
			fprintf(fp, "%.3T\t",            TFld(operac|RETRO_HORASAL));//	HS. SALIDA	
			fprintf(fp, "%.2f\t",  			 FFld(operac|RETRO_HSNOR));//HS. NORMALES
			fprintf(fp, "%.2f\t",            FFld(operac|RETRO_HS50));//HS. EXTRAS 50
			fprintf(fp, "%.2f\t",  		  	 FFld(operac|RETRO_HS100F));
			fprintf(fp, "%.2f\t",			 FFld(operac|RETRO_HS100FE));
			fprintf(fp, "%s \t", 			 SFld(operac|RETRO_CONDIC));
			fprintf(fp, "%.2f\t", 			 FFld(operac|RETRO_DHSNOR));
			fprintf(fp, "%.2f\t",		     FFld(operac|RETRO_DHS50));
			fprintf(fp, "%.2f\t",			 FFld(operac|RETRO_DHS100F));
			fprintf(fp, "%.2f\t",			 FFld(operac|RETRO_DHS100FE));
			fprintf(fp, "%ld\t",			 LFld(operac|RETRO_CUID));
			g_uid = LFld(operac|RETRO_CUID);
			
		    DevuelveUser();
	  	} 
  	  	DeleteCursor(c_retro);
}           

void CerrarArchivo()
{
	if (fp != NULL)	{
		fclose(fp);
		fp = NULL;
	}
}

