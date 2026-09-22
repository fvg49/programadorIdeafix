/********************************************************************
*
* MODULE & VERSION : @(#)asist215.cc	1.1
* DATE             : 10/10/21
* TIME             : 15:03:19
* CREATED          : Chimuris, Andres
* REVISO           : Fontanella, Jose Luis
*
* DENOMINACION     : asist215
* DESCRIPCION      : Proceso de Listado de totales de inasistencias 
*                    por tipo de codigo.
*
* REPORTES:  asis215.rp
*
* PANTALLAS: asist215.fm
*
* TABLA                 |  OPERACION
------------------------+------------------------------------------
* ASIST.ASISTEN         | Lect.
* ASIST.INASIST         | Lect.
* SUE.EMPS              | Lect.
* SUE.PER               | Lect.
*********************************************************************/
#include <ideafix.h>
#include "asist215.fmh"
#include "asist215.rph"
#include "asist.sch"
#include "sue.sch"
#include "sue.h"


/* Funciones privadas */

/* ++ Prototypes ++ */
static void ListadeInasis(void);
char * SeteoDescrip(void);

/* Declaraciones globales */
form fm0;
report rp0;
schema asist, sue, aux ;

struct licen{
	int codlic;
	char descrip[40];
	struct licen *sgte;
};
struct licen *cab; 
FILE   *fp=NULL; 

/* Programa principal */

wcmd(asist215, 1.1 10/21/10)
{
	long acumulo=0;
	long total=0;
	bool pase = FALSE;
	dbcursor c_asisten;
	char dev_descr[25];
    strcpy(dev_descr, NULL_STR);
    
/*	sue = OpenSchema("sue", IO_EABORT);*/

	asist = OpenSchema("asist", IO_EABORT | IO_SYMBOLS);
	fm0 = OpenForm("asist215", FM_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) {
		return;
	}
	rp0 = ERROR;

	if (GetRecord(asist|ASISTENbyINDNOV, FIRST_KEY, IO_NOT_LOCK) == ERROR){
		DWarning(DVARI ("VARI", "NODATLIS")); /*No hay datos para emitir el listado*/
		return;
	}
	c_asisten = CreateCursor(asist|ASISTENbyINDNOV, IO_NOT_LOCK);
	SetCursorFrom(c_asisten, FmIFld(fm0, EMPRE), FmIFld(fm0, CODD), 
		FmDFld(fm0, FECHAD), MIN_LONG);
	SetCursorTo(c_asisten, FmIFld(fm0, EMPRE), FmIFld(fm0, CODH), 
		FmDFld(fm0, FECHAH), MAX_LONG);
	
	if (CountCursor(c_asisten) == 0) {
		DWarning(DVARI ("VARI", "NODATLIS")); /*No hay datos para emitir el listado*/
		return;
	}
	else {
		rp0 = OpenReport("asist215", RP_EABORT|RP_NOBEGIN);
		switch(*FmSFld(fm0, SALIDA)){
		case 'T':
			RpSetOutput(rp0, RP_IO_TERM, NULL);
			break;
		case 'I':
			RpSetOutput(rp0, RP_IO_PRINTER, NULL);
			break;
		case 'A': 
			fp = fopen(FmSFld(fm0, NOM_ARCHI),"wt"); 
			fprintf(fp, "Empresa: %d %s\n", FmIFld(fm0, EMPRE), FmSFld(fm0, EMPDESCR));
			fprintf(fp, "Codigo Inasistencia desde %d %s hasta %d %s\n",FmIFld(fm0, CODD ), FmSFld(fm0, DESC1), FmIFld(fm0, CODH), FmSFld(fm0, DESC2));
			fprintf(fp, "Fecha desde %.3D hasta %.3D\n\n", FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH) );
			fprintf(fp, "Tipo de inasistencia\tDescripcion\tCantidad de Ausentes\n");
			break;
		}                       
		if (*FmSFld(fm0, SALIDA) != 'A') {
			BeginReport(rp0, 1, "");
			RpSetFld(rp0, REMPRE, FmSFld(fm0, EMPDESCR));
			RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
			RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));
		}
	}
	ListadeInasis();
	while (FetchCursor(c_asisten) != ERROR) {
		/*se agrega el if por problemas de rangos con el cursor*/	
		if (FmDFld(fm0, FECHAD) > DFld(asist|ASISTEN_FECHA) ||
			FmDFld(fm0, FECHAH) < DFld(asist|ASISTEN_FECHA)) continue;

		/* acceso */		
//		if (!Acceso(IFld(asist|ASISTEN_EMPRE), LFld(asist|ASISTEN_NROLEG), NULL_LONG)) {
//			continue;
//		}
        SetCursorTo(c_asisten, FmIFld(fm0, EMPRE), IFld(asist|ASISTEN_CODNOV),
         			FmDFld(fm0, FECHAH), MAX_LONG);
        while (FetchCursor(c_asisten) != ERROR) {
        	pase = TRUE;
			/*se agrega el if por problemas de rangos con el cursor*/	
			if (FmDFld(fm0, FECHAD) > DFld(asist|ASISTEN_FECHA) ||
				FmDFld(fm0, FECHAH) < DFld(asist|ASISTEN_FECHA)) continue;
			/* acceso */		
//			if (!Acceso(IFld(asist|ASISTEN_EMPRE), LFld(asist|ASISTEN_NROLEG), NULL_LONG)) {
//				continue;
//			}	
        	/* si el tiempo es nulo asumo un dia */
			if (IsNull(asist|ASISTEN_VALOR)) {
				acumulo += 100;
				total += 100;
			}	
			else {
				acumulo += LFld(asist|ASISTEN_VALOR);
				total += LFld(asist|ASISTEN_VALOR);
			}	
        }
        FetchCursorPrev(c_asisten);
        

		if (*FmSFld(fm0, SALIDA) != 'A') {
			RpSetIFld(rp0, RCODAUS, IFld(asist|ASISTEN_CODNOV));
    	    strcpy(dev_descr, SeteoDescrip());
	        RpSetLFld(rp0, RTOTINAS, acumulo);
	        RpSetFld(rp0, RDESCRAUS, dev_descr);
    	    DoReport(rp0, LINEA);
        }   
        else {
        	fprintf(fp, "%d\t", IFld(asist|ASISTEN_CODNOV));
        	strcpy(dev_descr, SeteoDescrip());
        	fprintf(fp, "%s\t", dev_descr);   
        	fprintf(fp, "%ld\n", acumulo/100);
        }
	    
	    acumulo = 0L;
	    
                             
        SetCursorFrom(c_asisten, FmIFld(fm0, EMPRE), IFld(asist|ASISTEN_CODNOV),
        				MAX_DATE, MAX_LONG);
        SetCursorTo(c_asisten, FmIFld(fm0, EMPRE), FmIFld(fm0, CODH), 
        				FmDFld(fm0, FECHAH), MAX_LONG);
    }
    if (rp0) {
    	CloseReport(rp0);
        DoReport(rp0, FINLINEA);
    }
}

static void ListadeInasis(void)
{
	struct licen *nodo, *ant;
	dbcursor Curinasis;
    ant = NULL;
    
	Curinasis=CreateCursor(INASISTbyCODINA, IO_NOT_LOCK);
	SetCursorFrom(Curinasis, MIN_SHORT);
	SetCursorTo(Curinasis, MAX_SHORT);
	
    while (FetchCursor(Curinasis) != ERROR) {
			if ( (nodo = (struct licen *) Alloc (sizeof(struct licen))) == NULL)
				DError(DGRAL ("PROCE", "NOMEM")); /*No hay mas espacio en momoria*/
			nodo->codlic = IFld(asist|INASIST_CODINA);
			strcpy(nodo->descrip, SFld(asist|INASIST_DESCRINAS));
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

