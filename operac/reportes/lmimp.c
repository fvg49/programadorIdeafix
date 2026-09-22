/********************************************************************
*
* MODULE & VERSION : @(#)lmimp.c	1.2
* DATE             : 06/07/18
* TIME             : 16:09:29
*
* CREATED          : 20/10/05
*
* DESCRIPTION:
*	Listado de Puestos de Improductividad.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*
*********************************************************************/

#include <ideafix.h>
#include "operac.h"
#include "lmimp.fmh"
#include "lmimp.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static void AbrirReporte();

static void ListaHijo(long clie, int obj, int pto, int cod);
static void ImprimeLinea(long p_cliente, int p_objet, int p_tippto, int p_codint, bool p_orig);

/* Declaraciones globales */
schema comerc, operac, bill, sue;
form fm0;
report rp0=NULL;
FILE *salida;

int nivel;

/* Programa principal */
wcmd(lmimp, 1.16 12/06/02)
{
 	fm_cmd cmd;
	dbcursor c_puestos;
	long clie, cliant;
	int  obj, tpto, codi, objant;

	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);

	fm0 = OpenForm("lmimp", FM_EABORT);
	while ((cmd=DoForm(fm0, NULLFP, after) )!= FM_EXIT) {

		if(cmd!=FM_UPDATE)
			continue;;

		AbrirReporte();
		clie = MAX_LONG;
		obj  = tpto = codi = MAX_SHORT;

		if(!FmIsNull(fm0, CLIEH))
			clie = FmLFld(fm0, CLIEH);
			
		if(!FmIsNull(fm0, OBJETH))
			obj  = FmIFld(fm0, OBJETH);

		if(!FmIsNull(fm0, TIPPTOH))
			tpto = FmIFld(fm0, TIPPTOH);

		if(!FmIsNull(fm0, CODINTH))
			codi = FmIFld(fm0, CODINTH);

		cliant=NULL_LONG;
		objant=NULL_SHORT;

		c_puestos=CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);
		SetCursorFrom(c_puestos, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, TIPPTO), FmIFld(fm0, CODINT));
		SetCursorTo  (c_puestos, clie, obj, tpto, codi);
		while(FetchCursor(c_puestos)!=ERROR){

			/* Leo Solo Puestos Padres */
			if(!IsNull(operac|PUESTOS_PADREINT))
				continue;
			/* Filtro por fecha si se pide */
			if(!FmIsNull(fm0, FECHOT)) {
				if (BajaPuesto(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO), FmDFld(fm0, FECHOT)))
					continue;
				if (!ValidaPuesto(operac, FmDFld(fm0, FECHOT))) 
					continue;
			}

			if(cliant!=LFld(operac|PUESTOS_CLIENTE)) {
				/* Seteo Cliente */
				InitRecord(bill|CLIENTE);
				SetKey(bill|CLIENTEbyCLIENTE, LFld(operac|PUESTOS_CLIENTE));
				GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
			    if (*FmSFld(fm0, SALIDA) != 'A') {
		            RpSetIFld(rp0, RCLIE, LFld(operac|PUESTOS_CLIENTE));
					RpSetFld (rp0, RCLIED, SFld(bill|CLIENTE_RAZSOC));
					DoReport(rp0, ZCLI);
			    }

				cliant=LFld(operac|PUESTOS_CLIENTE);
			}
			if(cliant!=LFld(operac|PUESTOS_CLIENTE) ||
				objant!=IFld(operac|PUESTOS_OBJET)) {
				/* Seteo Objetivo */
				SetKey(comerc|OBJETIVObyCLIENTE, LFld(operac|PUESTOS_CLIENTE),
				                                 IFld(operac|PUESTOS_OBJET));
				GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
			    if (*FmSFld(fm0, SALIDA) != 'A') {
					RpSetIFld(rp0, ROBJET, IFld(operac|PUESTOS_OBJET));
					RpSetFld (rp0, ROBJETD, SFld(comerc|OBJETIVO_DESCRIP));
					DoReport(rp0, ZOBJ);
				}
				objant=IFld(operac|PUESTOS_OBJET);
			}
			nivel=2;
			ImprimeLinea(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET),	
			             IFld(operac|PUESTOS_TIPPTO),  IFld(operac|PUESTOS_CODINT), TRUE);
	
			ListaHijo(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO),
			          IFld(operac|PUESTOS_CODINT));
		}
		DeleteCursor(c_puestos);

		if (*FmSFld(fm0, SALIDA) != 'A')
			CloseReport(rp0);
		else
			fclose(salida);

		FmClearAllFlds(fm0);
	} 
}


static void AbrirReporte()
{
	char auxiliar[5][130], agrega[100], auxdia[20];
	int v_x, v_y=0;

	if (*FmSFld(fm0, SALIDA) != 'A') {
	    if(rp0==NULL) {
	    	rp0 = OpenReport("lmimp", RP_EABORT|RP_NOBEGIN);
	    }
	   
	    // Si la salida es Impresora
	    if ( *FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

		// Si la salida es Terminal
	    if ( *FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

	    BeginReport(rp0, 1, NULL_STR);

		if (*FmSFld(fm0, SALIDA) != 'A') {
			sprintf(auxiliar[0],"%s", NULL_STR);
			sprintf(auxiliar[1],"%s", NULL_STR);
			sprintf(auxiliar[2],"%s", NULL_STR);
			sprintf(auxiliar[3],"%s", NULL_STR);
			sprintf(auxdia,"%s", NULL_STR);

			for (v_x =EMP; v_x<=(SALIDA-1); v_x++) {
				if (!FmIsNull(fm0, v_x)) {
					sprintf(agrega,"%s", NULL_STR);
					switch(v_x) {

						case EMP:
							sprintf(agrega, "Empresa: ");
							break; 
						case CLIE:
							sprintf(agrega, "Cliente desde: ");
							break; 
						case CLIEH:
							sprintf(agrega, "Cliente hasta:  ");
							break; 
						case OBJET:
							sprintf(agrega, "Objetivo desde: ");
							break; 
						case OBJETH:
							sprintf(agrega, "Objetivo hasta: ");
							break; 
						case TIPPTO:
							sprintf(agrega, "Tipo de Puesto desde: ");
							break; 
						case TIPPTOH:
							sprintf(agrega, "Tipo de Puesto hasta: ");
							break; 
						case CODINT:
							sprintf(agrega, "Codigo interno desde: ");
							break; 
						case CODINTH:
							sprintf(agrega, "Codigo interno hasta: ");
							break; 
						case NIVCON:
							sprintf(agrega, "Nivel de Consulta: ");
							break; 
						case DENICO:
							sprintf(agrega, " ");
							break; 
					}
					switch(v_x){
						case FECHOT:
							DToStr(FmDFld(fm0, v_x), auxdia, DFMT_SEPAR);
							sprintf(agrega, "%s %s ", agrega , auxdia);
							break;
						default: 
							sprintf(agrega, "%s %s ", agrega , FmSFld(fm0, v_x));
							break;
					}
					if((strlen(auxiliar[v_y])+strlen(agrega)) > 130)
						 v_y++;

					sprintf(auxiliar[v_y], "%s %s ", auxiliar[v_y] , agrega);
				}
			}
			RpSetFld(rp0, RSELEC,  auxiliar[0]);
			RpSetFld(rp0, RSELEC1, auxiliar[1]);
			RpSetFld(rp0, RSELEC2, auxiliar[2]);
		}		
	}
	else {
		salida=fopen(FmSFld(fm0, NOMARCH), "w");
		fprintf(salida, "Cliente\t\tObjetibo\t\tPuesto-Codint\tFecha de Inicio\tFecha de Finalizacion\t");
		fprintf(salida, "Hora de Inicio\tHora de Finalizacion\tDía1\tDía2\tDía3\tDía4\tDía5\tDía6\tDía7\t");
		fprintf(salida, "Regimen\tTipo de Día\tFrecuencia\tCantidad de Puestos\tCantidad de Vigiladores\n\n");
	}

} 

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case  CODINT:
		if (FmKeyCode(fm)==K_HELP) 
			HelpPto(fm, fno, row, FmLFld(fm, CLIE), FmIFld(fm, OBJET), FmIFld(fm, TIPPTO),
			        FmDFld(fm0, FECHOT), NULL_STR, NULL_STR);
		break;
	case CODINTH :
		if (FmKeyCode(fm)==K_HELP)
			HelpPto(fm, fno, row, FmLFld(fm, CLIEH), FmIFld(fm, OBJETH), FmIFld(fm, TIPPTOH),
			        FmDFld(fm0, FECHOT), NULL_STR, NULL_STR);
		break;
	case SALIDA:
		if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "lmimp.txt");
		break;
	}
	return FM_OK;				
}	

static void ListaHijo(long clie, int obj, int pto, int cod)
{
	dbcursor c_puestos;

 	if(!FmIsNull(fm0, NIVCON))
 		if(nivel>FmIFld(fm0, NIVCON))
			return;

	PushRecord(operac|PUESTOS);
	nivel++;

	c_puestos=CreateCursor(operac|PUESTOSbyPADRE, IO_NOT_LOCK);
	SetCursorFrom(c_puestos, clie, obj, pto, cod );
	SetCursorTo  (c_puestos, clie, obj, pto, cod );
	while(FetchCursor(c_puestos)!=ERROR){

		/* Filtro por fecha si se pide */
		if(!FmIsNull(fm0, FECHOT)) {
			if (BajaPuesto(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET), IFld(operac|PUESTOS_TIPPTO), FmDFld(fm0, FECHOT)))
				continue;
			if (!ValidaPuesto(operac, FmDFld(fm0, FECHOT))) 
				continue;
		}
		ImprimeLinea(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET),
	                IFld(operac|PUESTOS_TIPPTO),  IFld(operac|PUESTOS_CODINT), FALSE);

		ListaHijo(LFld(operac|PUESTOS_CLIENTE), LFld(operac|PUESTOS_OBJET),
		          IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT));
	}
	nivel--;

	DeleteCursor(c_puestos);
	PopRecord(operac|PUESTOS);
}

static void ImprimeLinea(long p_cliente, int p_objet, int p_tippto, int p_codint, bool p_orig)
{
	char auxdes[55];
	char auxtab[40];
	char auxdia[20];
	char auxhora[20];
	int v_i;

	PushRecord(operac|PUESTOS);

	SetKey(operac|PUESTOSbyCLIENTE, p_cliente, p_objet, p_tippto, p_codint);
	if(GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if (*FmSFld(fm0, SALIDA) != 'A') {

			/* auxtab puede llegar solo hasta 20 caracteres */
			sprintf(auxtab,"%s", NULL_STR);
			for(v_i=0; v_i<=(nivel>20?20:nivel); v_i++)
				sprintf(auxtab,"%s  ", auxtab);

			sprintf(auxdes, "%s%d-%d", auxtab, p_tippto, p_codint);
			if (p_orig)
				sprintf(auxdes, "%s - Original", auxdes);
			
			RpSetFld(rp0, RDETALLE, auxdes);
			/* Seteo Resto de campos */
			DbToRp(rp0, RFINICIO, RCODMOT);
			DoReport(rp0, ZLINEA);
		}
		else{
			fprintf(salida, "%ld\t%s\t", LFld(operac|PUESTOS_CLIENTE), SFld(bill|CLIENTE_RAZSOC));
			fprintf(salida, "%d\t%s\t", IFld(operac|PUESTOS_OBJET), SFld(comerc|OBJETIVO_DESCRIP));

			sprintf(auxdes, "%d-%d", p_tippto, p_codint);
			if (p_orig)
				sprintf(auxdes, "%s - Original", auxdes);
			else
				sprintf(auxdes, "%s - Modificado", auxdes);
			
			fprintf(salida, "%s\t", auxdes);

			DToStr(DFld(operac|PUESTOS_FINICIO), auxdia, DFMT_SEPAR);
			fprintf(salida, "%s\t", auxdia);
			DToStr(DFld(operac|PUESTOS_FFINAL),  auxdia, DFMT_SEPAR);
			fprintf(salida, "%s\t", auxdia);

			TToStr(TFld(operac|PUESTOS_HINICIO), auxhora, TFMT_SEPAR);
			fprintf(salida, "%s\t", auxhora);
			TToStr(TFld(operac|PUESTOS_HFINAL),  auxhora, TFMT_SEPAR);
			fprintf(salida, "%s\t", auxhora);

			
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA1));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA2));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA3));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA4));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA5));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA6));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_DIA7));

			fprintf(salida, "%s\t", SFld(operac|PUESTOS_REGIM));

			fprintf(salida, "%s\t", SFld(operac|PUESTOS_TIPODIA));
			fprintf(salida, "%s\t", SFld(operac|PUESTOS_CODFREC));
			fprintf(salida, "%d\t", IFld(operac|PUESTOS_CANTPUE));
			fprintf(salida, "%d\n", IFld(operac|PUESTOS_CANTVIG));

		}
	}
	PopRecord(operac|PUESTOS);

}
