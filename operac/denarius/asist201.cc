/********************************************************************
* MODULE & VERSION : @(#)asist201.cc	1.1
* DATE             : 10/10/21   
* TIME             : 13:51:20
* CREATED          : (today())
* AUTOR            : Chimuris, Andres
* REVISO           : Fontanella, Jose Luis
* MODIFICO         : Chimuris, Andres
* DENOMINACION     : asist201
* DESCRIPCION      : Proceso de Listado de Ausentismo del Personal.
*
* REPORTES:  asist20a.rp
*			 asist20b.rp
*
* PANTALLAS: asist201.fm
*
* TABLA                 |  OPERACION
------------------------+------------------------------------------
* ASIST.ASISTEN         | Lect.
* ASIST.INASIST         | Lect.
* SUE.EMPS              | Lect.
* SUE.PER               | Lect.
*********************************************************************/
#include <ideafix.h>
#include "asist201.fmh"
#include "asist20a.rph"
#include "asist20b.rph"
#include "asist.sch"
#include "sue.sch"
#include "sue.h"

/* Funciones privadas */
/* ++ Prototypes ++ */
/*static void make_index(void);*/
void ListadeInasis(void);

char * SeteoDescrip();
static void ImprimirReporte(long p_valor);
void ImprimirArchivo(long p_valor);
/* -- Prototypes -- */

/* Declaraciones globales */
form fm0;
report rp0;
schema asist, sue, aux ;
struct licen{
	int codlic;
	char descrip[26];
	struct licen *sgte;
};       

int g_i = 1;

FILE   *fp=NULL; 

struct licen *cab; 
/* Programa principal */

wcmd(asist201, 1.1 10/21/10)
{
	
	long valor = 0L;
	dbcursor c_asisten = ERROR;
	
	bool detallado = FALSE;
	
	long nroleg_ant = 0;          
	char apeynom_ant[50];
	
	sue = OpenSchema("sue", IO_EABORT);
	asist = OpenSchema("asist", IO_EABORT|IO_SYMBOLS);
	
	fm0 = OpenForm("asist201", FM_EABORT);
	

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;


	rp0 = ERROR;               
	
	/* Me fijo si el indice esta vacio */
	if (GetRecord(asist|ASISTENbyINDLEG, FIRST_KEY, IO_NOT_LOCK) == ERROR){
		DWarning(DVARI ("VARI","NODATLIS"));/* No hay datos para emitir list*/
		return;
	}
 	
	c_asisten = CreateCursor(asist|ASISTENbyINDLEG, IO_NOT_LOCK);
   	
   	SetCursorFrom(c_asisten, FmIFld(fm0, EMPRE), FmLFld(fm0, LEGD), 
  		NULL_DATE, NULL_SHORT);
	
  	SetCursorTo(c_asisten, FmIFld(fm0, EMPRE), FmLFld(fm0, LEGH), 
   		MAX_DATE, MAX_SHORT);
	
	if (CountCursor(c_asisten) == 0) {
		DWarning(DVARI ("VARI","NODATLIS"));/* No hay datos para emitir list*/
		return;
	}
	else {
	     if (*FmSFld(fm0, DETALLADO) == 'S') {

	     	ListadeInasis();
	     	
	     	detallado = TRUE;
	     	if(*FmSFld(fm0, SALIDA)!= 'A') {
	     		rp0 = OpenReport("asist20a", RP_EABORT|RP_NOBEGIN);
	     	}
		 }	
		 else {
		 	detallado = FALSE;
		 	if(*FmSFld(fm0, SALIDA)!= 'A')
		 		rp0 = OpenReport("asist20b", RP_EABORT|RP_NOBEGIN);
		 }
		 
		 switch(*FmSFld(fm0, SALIDA)) {
		 case 'T':
		 	RpSetOutput(rp0, RP_IO_TERM, NULL);
		 	BeginReport(rp0, 1, "");
		 	break;
		 case 'I':
		 	RpSetOutput(rp0, RP_IO_PRINTER, NULL);
		 	BeginReport(rp0, 1, "");
		 	break;           
		 case 'A':
		 	
		 	fp = fopen(FmSFld(fm0, NOM_ARCHI),"wt"); 
		 	
		 	
			fprintf(fp, "Empresa: %d\t%s\n", FmIFld(fm0, EMPRE), FmSFld(fm0, EMPDESCR));
			fprintf(fp, "Legajos desde %ld %s hasta %ld %s\n", FmLFld(fm0, LEGD), FmSFld(fm0, DESC1), FmLFld(fm0, LEGH), FmSFld(fm0, DESC2));//
			fprintf(fp, "Fecha desde %.3D hasta %.3D\n", FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH));
			fprintf(fp, "Tipo de ausencia desde %d %s hasta %d %s\n", FmIFld(fm0, AUSD), FmSFld(fm0, DESC3), FmIFld(fm0, AUSH), FmSFld(fm0, DESC4));

			fprintf(fp, "Archivo detallado? %s\n\n", (char *) (FmSFld(fm0, DETALLADO)) == "S" ? "Si" :"No");
			
					 	
		 	if(!detallado)
		 		fprintf(fp, "Legajo\tApellido y Nombre\tCantidad de Ausencias\n");
		 		
		 	 else 
		 	 	fprintf(fp, "Empresa\tLegajo\tApellido y Nombre\tFecha\tTipo de Ausentismo\tDescripcion de Ausentismo\tValor\tJustifica?\n");
		 }
	}
	
	while (FetchCursor(c_asisten) != ERROR) { //Recorro lista de ausentes
		
		/*se verifica que la fecha de novedad esté dentro del rango*/
		if (FmDFld(fm0, FECHAD) > DFld(asist|ASISTEN_FECHA) ||
			FmDFld(fm0, FECHAH) < DFld(asist|ASISTEN_FECHA)) {
		  continue;
        }
	  	
	  	/*se verifica que el código de ausencia esté dentro del rango*/
	  	if (FmIFld(fm0, AUSD) > IFld(asist|ASISTEN_CODNOV) ||
	  		FmIFld(fm0, AUSH) < IFld(asist|ASISTEN_CODNOV)) {
		continue;		
	  	} 
	  				
		/* se controla el acceso */		
   /*		if (!Acceso(IFld(asist|ASISTEN_EMPRE), LFld(asist|ASISTEN_NROLEG), NULL_LONG)) {
			continue;
		} */

		/*se controla que haya datos en PER para el legajo */
  		SetIFld(sue|PER_EMP, FmIFld(fm0, EMPRE));
		SetLFld(sue|PER_NROLEG, LFld(asist|ASISTEN_NROLEG));
		if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			Message msgnodat = DGRAL ("PROCE", "NOEXTDAT") 
			                         (DWORD("TABLA" , "PER"), 
			                         LFld(asist|ASISTEN_NROLEG));
			DWarning(msgnodat);
		}			
		else 
			if(IFld(sue|PER_RELACION) != FmIFld(fm0, EMPRE))
				continue;
        
        	
     /* si el tiempo es nulo asumo un dia */
	    if (IsNull(asist|ASISTEN_VALOR)) {
			   //	acumulo += 100;
		  valor = 100;
	    }	
	    else {
		  //acumulo += LFld(asist|ASISTEN_VALOR);
		  valor = LFld(asist|ASISTEN_VALOR);
	    }	

	    if(*FmSFld(fm0, SALIDA)!= 'A')	    
			ImprimirReporte(valor);		
		else {  
			if(detallado)	{
				ImprimirArchivo(valor); 
			}	
			else {
			
				if(nroleg_ant != LFld(asist|ASISTEN_NROLEG)) {
					if(nroleg_ant != 0) {
						fprintf(fp, "%ld\t", nroleg_ant);	 	//Nro. de Legajo
						fprintf(fp, "%s\t",  apeynom_ant); 		//Apellido y Nombre
						fprintf(fp, "%d\n", g_i);               //Cantidad de dias que falto en el periodo del FM
						g_i = 1;
					} 
					nroleg_ant = LFld(asist|ASISTEN_NROLEG);
					strcpy(apeynom_ant, SFld(sue|PER_APYNOM));
				}
				else
					g_i++;
			}
		}
	}

	if(*FmSFld(fm0, SALIDA)== 'A') {
		if (!detallado) { //Imprime el último bloque de los ausentes legajos
			fprintf(fp, "%ld\t", nroleg_ant);	 	//Nro. de Legajo
			fprintf(fp, "%s\t",  apeynom_ant); 			//Apellido y Nombre
			fprintf(fp, "%d", g_i);
			fprintf(fp, "\n");
		}
		fclose(fp);
	}	    
}

void ListadeInasis(void)
{
	struct licen *nodo, *ant;
	dbcursor Curinasis;
    ant = NULL;
    
	Curinasis=CreateCursor(asist|INASISTbyCODINA, IO_NOT_LOCK);
	SetCursorFrom(Curinasis, MIN_SHORT);
	SetCursorTo(Curinasis, MAX_SHORT);

    while (FetchCursor(Curinasis) != ERROR) {
			if ( (nodo = (struct licen *) Alloc (sizeof(struct licen))) == NULL)
				DError(DGRAL ("PROCE", "NOMEM"));

			nodo->codlic = IFld(asist|INASIST_CODINA);
			strncpy(nodo->descrip, SFld(asist|INASIST_DESCRINAS), 25);
			nodo->sgte = NULL;
    
			if (ant)
				ant->sgte = nodo;
			else
				cab = nodo;
		
			ant = nodo;
	}
	DeleteCursor(Curinasis);
}

char *  SeteoDescrip()
{
	struct licen *p;
	
	for (p = cab; p != NULL && IFld(asist|ASISTEN_CODNOV) != p->codlic; p=p->sgte) ;			
	
	if (p) {
		return p->descrip;
	}  
		
	return NULL_STR; 
}

static void ImprimirReporte(long p_valor)
{
	char dev_descrip[26];
	strcpy(dev_descrip, NULL_STR);
	
	RpSetFld(rp0, RAPYNOM, SFld(sue|PER_APYNOM));
	RpSetFld(rp0, REMPRE, FmSFld(fm0, EMPDESCR));
	RpSetLFld(rp0, RNROLEG, LFld(asist|ASISTEN_NROLEG));
		
	RpSetDFld(rp0, RFECHA, DFld(asist|ASISTEN_FECHA));
    RpSetIFld(rp0, RCODAUS,IFld(asist|ASISTEN_CODNOV));

    strcpy(dev_descrip, SeteoDescrip());
    
    RpSetFld(rp0, RDESCRAUS, dev_descrip);
   	RpSetLFld(rp0, RTIEMPO, p_valor);
   	
	if (IFld(asist|ASISTEN_JUSTIF) == 0 ||
		IFld(asist|ASISTEN_JUSTIF)==1) {
			RpSetFld(rp0,RJUSTIF,
					(char *) (IFld(asist|ASISTEN_JUSTIF)==0 ? "Si" :"No"));
	 }			  
	 if (DoReport(rp0, LINEA)== ERROR){
		 CloseReport(rp0);
		 Stop(0);
	 }	
	 
	 RpClearZone(rp0, LINEA);
} 

void ImprimirArchivo(long p_valor)
{   
	char dev_descrip[26];
	strcpy(dev_descrip, NULL_STR);

	fprintf(fp, "%d\t",  FmIFld(fm0, EMPRE));  				//Empresa           
	fprintf(fp, "%ld\t", LFld(asist|ASISTEN_NROLEG));	 	//Nro. de Legajo
	fprintf(fp, "%s\t",  SFld(sue|PER_APYNOM)); 			//Apellido y Nombre
   	fprintf(fp, "%.3D\t", DFld(asist|ASISTEN_FECHA));		//Fecha
	fprintf(fp, "%d\t",  IFld(asist|ASISTEN_CODNOV));		//Codigo de novedad

	strcpy(dev_descrip, SeteoDescrip());					//Funcion que devuelve descripcion

   	fprintf(fp, "%s\t", dev_descrip);						//Descripcion de la Novedad
	fprintf(fp, "%ld\t",  p_valor / 100);						

	if (IFld(asist|ASISTEN_JUSTIF) == 0 ||
		IFld(asist|ASISTEN_JUSTIF)==1) {
			fprintf(fp, "%s", (char *) (IFld(asist|ASISTEN_JUSTIF)==0 ? "Si" :"No")); //Si la falta es justificada
    }

	fprintf(fp, "\n"); 										//Bajada de linea
}

