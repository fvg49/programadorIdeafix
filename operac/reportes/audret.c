/*********************************************************************************
* MODULE & VERSION : @(#)audret.c	1.1
* DATE             : 11/07/20
* TIME             : 16:40:35
*
**********************************************************************************/
   
#include <ideafix.h>
#include "operac.sch"
#include "sue.sch"
#include "operac.h"
#include "comerc.h"
#include "audret.fmh"

#define DEBUG 0

schema operac, sue;
void AuditarRetroactivos(int p_emp, DATE p_fechad, DATE p_fechah, bool p_consretro, bool p_correrror, FILE *p_archi, form p_fm, fmfield p_coment);
static fm_status before(form fm, fmfield fno, int row);


bool g_graba=FALSE;
char g_prog[20];

form fm0;
FILE *fp1;
/* Programa principal */
wcmd(audret, 1.1 07/20/11)
{   
	fm_cmd cmd;
	char v_aux[100];

	if(argc >1) {
		g_graba = str_eq (argv[1], "G");
	}

	sprintf(g_prog, "%s", argv[0]);

	operac= OpenSchema("operac", IO_EABORT);
	sue= OpenSchema("sue", IO_EABORT);
	fm0 = OpenForm("audret", FM_EABORT);
	
	sprintf(v_aux,"audret-%d-%d.txt", getpid(), GetUid());
	FmSetFld(fm0, SALIDA, v_aux);

	while ((cmd = DoForm(fm0, before, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:


		fp1 = fopen(FmSFld(fm0, SALIDA),"w") ;
		
		AuditarRetroactivos( FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), FmIFld(fm0, CONSRETRO), FmIFld(fm0, CORRERROR), fp1, fm0, COMENT);
		break;

	}

	fclose(fp1);
	
}
static fm_status before(form fm, fmfield fno, int row)
{
	
	switch (fno) {
		case CORRERROR:
			if (!g_graba)
				return FM_SKIP;

			break;
	}
	return FM_OK;
}

void AuditarRetroactivos(int p_emp, DATE p_fechad, DATE p_fechah, bool p_consretro, bool p_correrror, FILE *p_archi, form p_fm, fmfield p_coment)

{
	dbcursor c_retro, c_per, c_parte;

	int 	 v_difsim=0,
			 v_difdob=0,
			 v_difdym=0,
			 v_difdymfe=0;

	int		 v_sim=0,
			 v_dob=0,
			 v_dym=0,
			 v_dymfe=0,
			 v_toths=0;


	int		 tot_sim=0,
			 tot_dob=0,
 			 tot_dym=0, 
	 		 tot_dymfe=0,
	 		 tot_peg=0,
	 		 tot_hs100=0,
	 		 vot_hs100=0;

	int		 tipodia=0;
	short	 v_pais= NULL_SHORT, 
			 v_prov= NULL_SHORT;
	
	DATE	 v_dia=NULL_DATE;
	bool	 v_hayretro=FALSE;
	char 	 v_condic='\0';
	long	 v_cliente=0;
	int		 v_objetivo=0;

	bool	 v_procetodo= NULL_BOOL;	
    
    
    char 	v_regimen[12]; //NUEVO
    double	v_horas_regimen=0;

	c_per   = CreateCursor(sue|PERbyACT, IO_NOT_LOCK);
	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	c_retro = CreateCursor(operac|RETRObyREMPLE, IO_NOT_LOCK);

	c_per=CreateCursor(sue|PERbyACT, IO_NOT_LOCK);
	SetCursorFrom(c_per, p_emp, 1, NULL_LONG);
	SetCursorTo  (c_per, p_emp, 2, MAX_LONG );
	while(FetchCursor(c_per)!=ERROR) {
		
//fprintf(stderr, "fecdesde: %.3D\n", p_fechad);
//fprintf(stderr, "fechasta: %.3D\n", p_fechah);
		for(v_dia=p_fechad; v_dia<=p_fechah; v_dia++) {

//fprintf(stderr, "fec: %.3D\n", v_dia);



			v_difsim=0;
			v_difdob=0;
			v_difdym=0;
			v_difdymfe=0;
//fprintf(stderr, "fec: %.3D\n", v_dia);
			if (p_consretro){
				v_hayretro=FALSE;		
				SetCursorFrom(c_retro, p_emp, LFld(sue|PER_NROLEG), v_dia, NULL_LONG, NULL_SHORT);
				SetCursorTo  (c_retro, p_emp, LFld(sue|PER_NROLEG), v_dia, MAX_LONG,  MAX_SHORT );
				while(FetchCursor(c_retro)!=ERROR){

					v_difsim+=IFld(operac|RETRO_DHSNOR);
					v_difdob+=IFld(operac|RETRO_DHS50);
					v_difdym+=IFld(operac|RETRO_DHS100F);
					v_difdymfe+=IFld(operac|RETRO_DHS100FE);

					v_hayretro=TRUE;

				}
			}

			v_sim=0;
			v_dob=0;
			v_dym=0;
			v_dymfe=0;
			SetCursorFrom(c_parte, p_emp, LFld(sue|PER_NROLEG), v_dia, NULL_LONG, NULL_SHORT);
			SetCursorTo  (c_parte, p_emp, LFld(sue|PER_NROLEG), v_dia, MAX_LONG,  MAX_SHORT );
			while(FetchCursor(c_parte)!=ERROR){

				v_sim+=IFld(operac|PARTE_HSNOR);
				v_dob+=IFld(operac|PARTE_HS50);
				v_dym+=IFld(operac|PARTE_HS100F);
				v_dymfe+=IFld(operac|PARTE_HS100FE);

			}

			if (v_hayretro){
				v_condic  =*SFld(operac|RETRO_CONDIC);
				v_cliente = LFld(operac|RETRO_CLIENTE);
				v_objetivo= IFld(operac|RETRO_OBJETIVO);
			}
			else{
				v_condic  =*SFld(operac|PARTE_CONDIC);
				v_cliente = LFld(operac|PARTE_CLIENTE);
				v_objetivo= IFld(operac|PARTE_OBJETIVO);
			}

			v_pais = GetCliePais(v_cliente, v_objetivo);
			v_prov = GetClieProv(v_cliente, v_objetivo);


			tipodia=_DIA_NORMAL;
			if (v_condic=='F') 
				tipodia=_DIA_FRANCO;
			else{
				if (FeriadoNovia(v_dia, v_pais, v_prov)) 
					tipodia=_DIA_FERIADO;
				else 
					tipodia=_DIA_NORMAL;
			}

			v_sim+=v_difsim;
			v_dob+=v_difdob;
			v_dym+=v_difdym;
			v_dymfe+=v_difdymfe;
			
			v_toths=v_sim+v_dob+v_dym+v_dymfe;

			tot_sim=tot_dob=tot_dym=tot_dymfe=tot_hs100=0;                                           

			vot_hs100=0;
			
			
			DistibuyeHorasPer(p_emp,  v_toths,   tipodia, v_horas_regimen,  &tot_sim, &tot_dob, &tot_dym, &tot_dymfe, &tot_peg);
			
			GetRegimenEfectivo(p_emp, LFld(sue|PER_NROLEG), v_regimen, v_dia);   
			v_horas_regimen= (double)GetHsRegimen(v_regimen);        
			
            tot_hs100 = v_dym + v_dymfe; // + tot_peg;
            vot_hs100 = tot_dym + tot_dymfe + tot_peg;
			
			if (tot_sim!=v_sim || tot_dob!=v_dob || tot_hs100!=vot_hs100){ 
			
				fprintf(p_archi, "\nError en Distribucion de horas para el legajo %ld el dia %.3D\n", LFld(sue|PER_NROLEG), v_dia);
				fprintf(p_archi, "GUARDADO  %c-%d-%d-%d-%d\n",v_condic, v_sim, v_dob, v_dym, v_dymfe);
				fprintf(p_archi, "CALCULADO %d-%d-%d-%d-%d-%d\n", tipodia, tot_sim, tot_dob,  tot_dym, tot_dymfe, tot_peg);

				if (p_correrror && g_graba)
					if (v_procetodo==TRUE || WiDialog(WD_YES|WD_NO, WD_NO, "Error Detectado",
					    "Error en Distribucion de horas para el legajo %ld el dia %.3D\nGUARDADO  %c-%d-%d-%d-%d\nCALCULADO %d-%d-%d-%d-%d\n Desea Recalcular?", 
					     LFld(sue|PER_NROLEG), v_dia, v_condic, v_sim, v_dob, v_dym, v_dymfe, tipodia, tot_sim, tot_dob,  tot_dym, tot_dymfe)==WD_YES) {
						
						if (v_procetodo==NULL_BOOL){
							if (WiDialog(WD_YES|WD_NO, WD_NO, "Error Detectado","Desea recalcular todos los casos que se encuentren?")==WD_YES) 
							   v_procetodo=TRUE;
							else
							   v_procetodo=FALSE;
					    } 
						RecalculaPartePer(p_emp, LFld(sue|PER_NROLEG), v_dia, v_dia, p_fm, p_coment, g_prog); 
						fprintf(p_archi, "ERROR CORREGIDO\n");
					}
			}
 		}
	}
	DeleteCursor(c_retro);
	DeleteCursor(c_parte);
	DeleteCursor(c_per);
}
