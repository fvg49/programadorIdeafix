#include <ideafix.h>
#include "operac.sch"
#include "comerc.sch"
#include "sue.sch"
#include "comgral.sch"
#include "disths.h"
#include "comerc.h"
#include "comgral.h"
#include "opedef.h"
#include "operac.h"

//extern double ConvHraInt(TIME, TIME);
private void ImprimeHoras(bool p_msj, double* distrib, double* aux_distrib);
private void CalcDistrNocturnas(int p_emp, long p_nroleg, DATE p_fecha,  TIME p_horent, TIME p_horsal, long p_cli, int p_obj, bool p_retro, 
								int p_ptoser, int p_puesto, int p_nroint, bool p_msj, bool p_es_feriado_hoy, bool p_es_feriado_manana, double* distrib);


/***********************************************************************************************************
*                                          CargaDistrHoras
*                       
* Parametros:                       
*                       
*	p_empd    = Empresa desde
*	p_emph    = Empresa hasta
*	p_clid    = Cliente desde
*	p_clih    = Cliente hasta
*	p_objd    = Objetivo desde
*	p_objh    = Objetivo hasta
*	p_legd    = Legajo desde
*	p_legh    = Legajo hasta
*	p_fecd    = Fecha desde
*	p_fech    = Fecha hasta
*	p_retro   = Lee retroactivos? 1 Parte
*								  2 Retro
*                                 3 Parte y Retro
*   p_act     = Considera inactivos (true=Muestra todos, false=no muestra inactivos
*
*   p_destino = Indica si la salida se usara para un reporte o para el pase 
*               (porque deacuerdo a eso si no esta confirmado, es un warning o u error) 
*
*	p_liqui   = nrol de liquidacion si hubiera 
*
* 	p_ignhs0  = Ignora registros con sumatoria de Horas en 0
*	
*	p_msgerr  = Por esta varialble se le ingresa el nombre del archivo de error y devuelve mensaje de horas si hubiera
*
* Devuelve:  
*            Un puntero a la lista cargada con los parametros dados.
*
* Observacion: 
*			 La funcion selecciona el indice adecuado segun se le pase nro de legajo, la fecha o por 
*            defecto cliente, objetivo en ese orden de prioridad.
*
***********************************************************************************************************/

tnempres CargaDistrHoras(int p_empd, int p_emph, long p_clid, long p_clih, int p_objd, int p_objh, long p_legd, long p_legh, DATE p_fecd, DATE p_fech, 
                         int p_retro, bool p_act, int p_destino, long p_liqui, bool p_ignhs0, char * p_msgerr)
{

	bool v_primerro=TRUE;
	FILE *v_sal_error=NULL;
	char auxarch[100], v_auxtpto[100];

	dbcursor cparte, cretro;
	dbtable  APARTE;

	int      objant=0,
	         i,
	         convenio=0;
	char     regim[20],
			 v_tipo_calhs[2];
	double   canh, hsxreg, diferencia=0;
//	TIME	 asig_ent, asig_sal;
	schema   old, ope, operac, com;
	long     legant=0;
	bool     legact=FALSE;
//	tnempres ini;
	bool privez= TRUE, v_error=FALSE;
	bool esMensual = FALSE;
	
	if(GetTipoLiquidacionByNroLiq(p_empd, p_liqui) == TIPO_CIERRE_MENSUAL)
		esMensual = TRUE;
	 else
	 	esMensual = FALSE;	

	sprintf(auxarch, "%s", p_msgerr);

	ini = NULL;
	ini1= ini2= ini3= ini4= ini5= ini6= NULL;
	empres = NULL_SHORT;

	for (i=0 ; i<MAXTIPHOR ; i++) 
		cantih[i] = NULL_DOUBLE;

	nodleg = NULL_LONG;
	nodclie= NULL_LONG;
	objnod = NULL_SHORT;
	dianod = NULL_DATE;
	puenod[0] = '\0';
	hordes = NULL_TIME;

	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	com = OpenSchema("comerc", IO_EABORT);
	operac = FindSchema("operac");
	SwitchToSchema(old);

	APARTE  = CreateAlias(operac|PARTE);

	for (i=0 ; i<MAXTIPHOR ; i++){
		cantih[i] = NULL_DOUBLE;  
	}
	if(p_retro==1 || p_retro==3){
		privez=TRUE;
		if(p_legd!=NULL_LONG && p_empd!=NULL_SHORT) {
		     /* Indice por numero de legajo */
			cparte = CreateCursor(ope|PARTEbyEMPLE, IO_NOT_LOCK);

			SetCursorFrom(cparte, p_empd, p_legd, p_fecd, p_clid, p_objd);

			if(p_legh==NULL_LONG){
				SetCursorTo  (cparte, p_empd, MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);
			}
			else {
				if(p_fech==NULL_DATE){
					SetCursorTo  (cparte, p_emph, p_legh, MAX_DATE, MAX_LONG, MAX_SHORT);
				}
				else {
					if(p_clih==NULL_LONG){
						SetCursorTo  (cparte, p_emph, p_legh, p_fech, MAX_LONG, MAX_SHORT);
					}
					else{
						if(p_objh==NULL_SHORT) {
							SetCursorTo  (cparte, p_emph, p_legh, p_fech, p_clih, MAX_SHORT);
						}
						else {
							SetCursorTo  (cparte, p_emph, p_legh, p_fech, p_clih, p_objh);
						}
					}
				}
			}
		}
		else {
			if(p_fecd!=NULL_DATE  && p_empd!=NULL_SHORT){
				/* Indice por fecha */
				cparte = CreateCursor(ope|PARTEbyDIA, IO_NOT_LOCK);

				SetCursorFrom(cparte, p_empd, p_fecd, p_clid, p_objd);
				if (p_clih == NULL_LONG) {
					SetCursorTo(cparte, p_emph, p_fech, MAX_LONG, MAX_SHORT);
                }
				else {
					if (p_objh == NULL_SHORT) {
						SetCursorTo(cparte, p_emph, p_fech, p_clih, MAX_SHORT);
					}
					else {
						SetCursorTo(cparte, p_emph, p_fech, p_clih, p_objh);
					}
				}
			}
			else {
				/* Indice por Cliente Objetivo */
				cparte = CreateCursor(ope|PARTEbyLEG, IO_NOT_LOCK);

				SetCursorFrom(cparte, p_empd, p_clid, p_objd, p_legd,  p_fecd);
				if (p_emph == NULL_SHORT) {
					SetCursorTo(cparte, MAX_SHORT, MAX_LONG, MAX_SHORT, MAX_LONG, MAX_DATE);
				}
				else {
					if (p_clih == NULL_LONG) {
						SetCursorTo(cparte, p_emph, MAX_LONG, MAX_SHORT, MAX_LONG, MAX_DATE);
					}
					else {
						if (p_objh == NULL_SHORT) {
							SetCursorTo(cparte, p_emph, p_clih, MAX_SHORT, MAX_LONG, MAX_DATE);
						}
						else {
							if (p_legh == NULL_LONG) {
								SetCursorTo(cparte, p_emph, p_clih, p_objh, MAX_LONG, MAX_DATE);
							}
						}
					}
				}
			}
		}
		while (FetchCursor(cparte) != ERROR) {

//			fprintf(stderr, "Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", IFld(ope|PARTE_EMP), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO), LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA));

/*			if (p_grup)
				if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO)))
					continue;
*/

			/* Filtro por Cliente */
			if (p_clih != NULL_LONG && LFld(ope|PARTE_CLIENTE) > p_clih)
				continue;

			if (p_clid != NULL_LONG && LFld(ope|PARTE_CLIENTE) < p_clid)
				continue;

			/* Filtro por Objetivo */
			if (p_objh != NULL_SHORT && IFld(ope|PARTE_OBJETIVO) > p_objh)
				continue;

			if (p_objd != NULL_SHORT && IFld(ope|PARTE_OBJETIVO) < p_objd)
				continue;

			/* Filtro por Legajo */
			if (p_legh != NULL_LONG && LFld(ope|PARTE_NROLEG) > p_legh)
				continue;

			if (p_legd != NULL_LONG && LFld(ope|PARTE_NROLEG) < p_legd)
				continue;

			/* Filtro por Cliobj */
			if (p_fech != NULL_DATE && DFld(ope|PARTE_DIA) > p_fech)
				continue;

			if (p_fecd != NULL_DATE && DFld(ope|PARTE_DIA) < p_fecd)
				continue;

			if (IFld(ope|PARTE_OBJETIVO) != objant) {
				if (p_legd != NULL_LONG && p_empd != NULL_SHORT)
					DisplayMsg(FALSE, "Procesando Parte para Legajo %ld  Cliente %ld Objetivo %d", LFld(ope|PARTE_NROLEG), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO));
				else {
					if (p_fecd != NULL_DATE  && p_empd != NULL_SHORT)
						DisplayMsg(FALSE, "Procesando Parte Fecha %.3D Cliente %ld Objetivo %d",DFld(ope|PARTE_DIA), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO));
					else 
						DisplayMsg(FALSE, "Procesando Parte de Cliente %ld Objetivo %d", LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO));
				}
				WiRefresh();
				objant = IFld(ope|PARTE_OBJETIVO);
			}


			empres    = (long)IFld(ope|PARTE_EMP);
			nodleg    = LFld(ope|PARTE_NROLEG);
			nodclie   = LFld(ope|PARTE_CLIENTE);
			objnod    = IFld(ope|PARTE_OBJETIVO);
			dianod    = DFld(ope|PARTE_DIA);
			fechanoc  = DFld(ope|PARTE_DIA);
			legajonoc = LFld(ope|PARTE_NROLEG);
            sprintf(puenod, "%d-%d-%d", IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO), IFld(ope|PARTE_NROINT));
//            sprintf(puenod, "%d-%d", IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO));
			n_ptoser= IFld(ope|PARTE_PTOSER);
			n_puesto= IFld(ope|PARTE_PUESTO);
			n_codint= IFld(ope|PARTE_NROINT);
			hordes  = TFld(ope|PARTE_HORAENT);
			horhas  = TFld(ope|PARTE_HORASAL);
			condic  = *SFld(ope|PARTE_CONDIC);
			codnov  = IFld(ope|PARTE_CODAUS);

			//Guardo Puesto 
 			n_horini = n_horfin = NULL_TIME;
 			n_dia1 = n_dia2 = n_dia3 = n_dia4 =	n_dia5 = n_dia6 = n_dia7 ='\0';
 			sprintf(n_regim, "%s", NULL_STR);
 			n_canvig = NULL_LONG;
 			n_canpto = NULL_SHORT;
 			n_tipdia = '\0';
 			n_fecini = n_fecfin = NULL_DATE;


			// Si quiero que no salgan las T-00:00-00:00  descomentar estas lineas
			if(*SFld(ope|PARTE_CONDIC)=='T' && TFld(ope|PARTE_HORAENT)==StrToT("00:00:0000") && TFld(ope|PARTE_HORASAL)==StrToT("00:00:0000"))
				continue; 

			SetKey(ope|PUESTOSbyCLIENTE, LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO), IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO));
			if (GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {

				n_horini = TFld(ope|PUESTOS_HINICIO);
				n_horfin = TFld(ope|PUESTOS_HFINAL);
				n_dia1   = *SFld(ope|PUESTOS_DIA1);
				n_dia2   = *SFld(ope|PUESTOS_DIA2);
				n_dia3   = *SFld(ope|PUESTOS_DIA3);
				n_dia4   = *SFld(ope|PUESTOS_DIA4);
				n_dia5   = *SFld(ope|PUESTOS_DIA5);
				n_dia6   = *SFld(ope|PUESTOS_DIA6);
				n_dia7   = *SFld(ope|PUESTOS_DIA7);
				sprintf(n_regim, "%s", SFld(ope|PUESTOS_REGIM));
				n_canvig = LFld(ope|PUESTOS_CANTVIG);
				n_canpto = IFld(ope|PUESTOS_CANTPUE);
				n_tipdia = *SFld(ope|PUESTOS_TIPODIA);
				n_fecini = DFld(ope|PUESTOS_FINICIO);
				n_fecfin = DFld(ope|PUESTOS_FFINAL);
				
			} 

			sprintf(regim, "%s",NULL_STR);

			/* Si cambia el nroleg me fijo si esta activo y el tipo de convenio*/
			if (legant != LFld(ope|PARTE_NROLEG)) {
				legant = LFld(ope|PARTE_NROLEG);
				legact=LegajoActivo(IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG));
				convenio = GetConvenio(IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG));
			}


			// Si no es el destino un pase o una prueba, que no valide el que los legajos sean activos.
			if (!(p_destino == DESTINO_PASE || p_destino == DESTINO_TEST)) {
				if (!p_act)
					if (!legact)
						continue;
			}

			SetKey(ope| ASIGbyNROLEG, IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG), LFld(ope|PARTE_CLIENTE), LFld(ope|PARTE_OBJETIVO));
			if (GetRecord(ope|ASIGbyNROLEG, THIS_KEY, IO_NOT_LOCK) != ERROR)
				sprintf(regim, "%s",SFld(ope|ASIG_REGIM));
			else {
				SetKey(ope|ASIGHbyNROLEG, IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG), LFld(ope|PARTE_CLIENTE), LFld(ope|PARTE_OBJETIVO));
				if (GetRecord(ope|ASIGHbyNROLEG, THIS_KEY, IO_NOT_LOCK)!=ERROR)
					sprintf(regim, "%s",SFld(ope|ASIGH_REGIM));
			}
			canh = ConvHraInt(TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL));

			for (i = 0; i < MAXTIPHOR; i++) {
				cantih[i] = 0;
			} 

			if (*SFld(ope|PARTE_CONDIC) == 'T' ||
			    FFld(ope|PARTE_HSNOR) !=0 ||
			    FFld(ope|PARTE_HS50)   !=0 ||
			    FFld(ope|PARTE_HS100F) !=0 ||
			    FFld(ope|PARTE_HS100FE)!=0){

				diferencia=abs( (canh *100 )-(FFld(ope|PARTE_HSNOR) + FFld(ope|PARTE_HS50) + FFld(ope|PARTE_HS100F) + FFld(ope|PARTE_HS100FE)) );

				if (diferencia>=1) {

					if (v_primerro){
						v_sal_error = fopen(auxarch, "a+");
						v_primerro=FALSE;
					}
					v_error=TRUE;
					fprintf(v_sal_error, "Error de Confirmacion Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", 
					                   IFld(ope|PARTE_EMP), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO), LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA));
				
					if (p_ignhs0) 
						continue;


				}
			}
					
			sprintf(v_tipo_calhs, "%s", GetParNov(IFld(ope|PARTE_EMP), PARNOV_TCALC_HS, LINEA_UNICA, Today()) );

			// Calcula cantidad de francos si el tipo de calculo es "A" (Ej emp 40)
			// Calcula un feriado por mes  si el tipo de calculo es "B" (Ej emp 1 y 2)


/*			diasause=diasfran=diasfrat=diasvaca=diasvact=;
			diasadel = 0;
			switch (v_tipo_calhs[0]) {
			case 'A': 
				switch (*SFld(ope|PARTE_CONDIC)) {
				case _FRANCO_C:
					if (TFld(ope|PARTE_HORAENT)==StrToT("00:00:0000") && TFld(ope|PARTE_HORASAL)==StrToT("00:00:0000")) {
						diasfran=1;
					}
					else {
						diasfrat=1;
					}
					break;
				case _AUSENTE_C:
					SetTFld(ope|PARTE_HORAENT,StrToT("00:00:0000"));	// Encontre partes ausente con horas DHC
					SetTFld(ope|PARTE_HORASAL,StrToT("00:00:0000"));
					diasause=1;
					break;
				case _VACACIONES_C:
					if (TFld(ope|PARTE_HORAENT)==StrToT("00:00:0000") && TFld(ope|PARTE_HORASAL)==StrToT("00:00:0000")) {
						diasvaca=1;
					}
					else {
						diasvact=1;
					}
					break;
				}
				break;
			case 'B': 
				if (Day(DFld(ope|PARTE_DIA))==1 || privez)
					diasfran=1;
				break;
			}
*/
			cantih[HSNORM] = IFld(ope|PARTE_HSNOR);
			cantih[HSAL25] = IFld(ope|PARTE_HS50);
			cantih[HSAL35] = IFld(ope|PARTE_HS100F);
			cantih[HSFRAN] = IFld(ope|PARTE_HS100FE);

			CalcDistrHoras2(IFld(ope|PARTE_EMP),
			               LFld(ope|PARTE_NROLEG),
			               DFld(ope|PARTE_DIA),
			               *SFld(ope|PARTE_CONDIC),
			               TFld(ope|PARTE_HORAENT),
			               TFld(ope|PARTE_HORASAL),
			               LFld(ope|PARTE_CLIENTE),
			               IFld(ope|PARTE_OBJETIVO),
			               FALSE,
			               IFld(ope|PARTE_PTOSER),
			               IFld(ope|PARTE_PUESTO),
			               IFld(ope|PARTE_NROINT),
			               cantih);

			if (p_destino == DESTINO_PASE) 
			{
				if (esMensual) {
					if(!IsNull(ope|PARTE_LIQDENA) && LFld(ope|PARTE_LIQDENA)!=p_liqui){
						if (v_primerro){
							v_sal_error = fopen(auxarch, "w");
							v_primerro=FALSE;
						}
						fprintf(v_sal_error, "Error Parte Ya fue liquidado Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", 
						                   IFld(ope|PARTE_EMP), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO), LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA));
					} else 
					{
						SetLFld(ope|PARTE_LIQDENA, p_liqui);
						PutRecord(ope|PARTE);
					}					
				} else {
					if(!IsNull(ope|PARTE_LIQDENUS) && LFld(ope|PARTE_LIQDENUS)!=p_liqui){
						if (v_primerro){
							v_sal_error = fopen(auxarch, "w");
							v_primerro=FALSE;
						}
						fprintf(v_sal_error, "Error Parte Ya fue liquidado Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", 
						                   IFld(ope|PARTE_EMP), LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO), LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA));
					}else {
						SetLFld(ope|PARTE_LIQDENUS, p_liqui);
						PutRecord(ope|PARTE);
					}
				}
			}

			diasnoct=diasdiur=canpeg=diasadel=0;


			switch (*SFld(ope|PARTE_CONDIC)) {
			case _PEGADA_C:
				canpeg=1;
				break;
			case _ADELANTO_C:
				diasadel=1;
				break;
			}

			// Me fijo si ya considere nocturna esta fecha para este legajo
			sprintf(v_auxtpto, "%d", IFld(ope|PARTE_PTOSER));

//fprintf(stderr, "AA %d %D %T %T\n", LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA), TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL));

			if (*SFld(ope|PARTE_CONDIC) != _FRANCO_C && *SFld(ope|PARTE_CONDIC) != _PEGADA_C) { // Los francos y pegadas no se cuentan como jornada.

				if (hordes != StrToT("00:00") || horhas != StrToT("00:00")) {

                    SetKey(com|REGIMENbyREGI, NULL_STR);

					hsxreg = 0;

					SetKey(ope|PUESTOS, LFld(ope|PARTE_CLIENTE), LFld(ope|PARTE_OBJETIVO), IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO));
					if (GetRecord(ope|PUESTOS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
						if (!IsNull(ope|PUESTOS_SUBREG)) {
		                    SetKey(com|REGIMENbyREGI, SFld(ope|PUESTOS_SUBREG));
	                    }
	                    else {
		                    SetKey(com|REGIMENbyREGI, SFld(ope|PUESTOS_REGIM));
	                    }
                    }

                    if (GetRecord(com|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR) {
                    	hsxreg = IFld(com|REGIMEN_HSNORM);
                    }
                    else {
                    	hsxreg = _HORAS_REGIMEN_DEFAULT*100;
                    }
//fprintf(stderr, "dia %D %T horas feriado %f horas normales %f hsxreg %f nocturnas %f minnoct %f parnov %d \n", DFld(ope|PARTE_DIA), hordes, cantih[HSFERI], cantih[HSNORM]+cantih[HSNONO]+cantih[HSNOFE]+cantih[HNOFEN],
//hsxreg/2,(cantih[HSNONO]+cantih[HS25NO]+cantih[HS35NO]+cantih[HNOFEN]+cantih[H25FEN]+cantih[H35FEN])/100,MIN_HS_NOCT,!EsParNov(IFld(ope|PARTE_EMP), PARNOV_TPUHSNOR, DFld(ope|PARTE_DIA), v_auxtpto));

                    if (cantih[HSNORM]+cantih[HSNONO]+cantih[HSNOFE]+cantih[HNOFEN] > hsxreg/2) {
						if ((cantih[HSNONO]+cantih[HS25NO]+cantih[HS35NO]+cantih[HNOFEN]+cantih[H25FEN]+cantih[H35FEN])/100 >= MIN_HS_NOCT
						     && !EsParNov(IFld(ope|PARTE_EMP), PARNOV_TPUHSNOR, DFld(ope|PARTE_DIA), v_auxtpto)) {
							diasnoct=1;
//fprintf(stderr, "A %D %T %d %d \n", DFld(ope|PARTE_DIA), hordes, diasnoct, diasdiur);
						}
						else {
							diasdiur=1;
//fprintf(stderr, "B %D %T %d %d \n", DFld(ope|PARTE_DIA), hordes, diasnoct, diasdiur);
						}
                    }
				}
			}

//fprintf(stderr, "parte %D %s %T %T noct %d diur %d adel %d \n", DFld(ope|PARTE_DIA), SFld(ope|PARTE_CONDIC), TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL), diasnoct, diasdiur, diasadel);


			ini = AcuNEmpres(ini, &ini);
			
			privez=FALSE;
		}
		DeleteCursor(cparte);
		BorNLegajonoc(ini1);
		BorNLegajonoc(ini2);
		BorNLegajonoc(ini3);
		BorNLegajonoc(ini4);
		BorNLegajonoc(ini5);
		ini1 = ini2 = ini3 = ini4 = ini5 = NULL;
	}

	if (p_retro == 2 || p_retro == 3) {
		if (p_legd != NULL_LONG && p_empd != NULL_SHORT) {
			/* Indice por numero de legajo */
			cretro = CreateCursor(ope|RETRObyREMPLE, IO_NOT_LOCK);
    	
			SetCursorFrom(cretro, p_empd, p_legd, MIN_DATE, MIN_LONG, MIN_SHORT);
			if (p_legh == NULL_LONG)
				SetCursorTo(cretro, p_empd, MAX_LONG, MAX_DATE, MAX_LONG, MAX_SHORT);

		}
		else {
			if (p_fecd != NULL_DATE  && p_empd != NULL_SHORT) {
				/* Indice por fecha */
				cretro = CreateCursor(ope|RETRObyRFECREG, IO_NOT_LOCK);
				SetCursorFrom(cretro, p_empd, p_fecd, p_clid, p_objd);
				if (p_clih == NULL_LONG) 
					SetCursorTo  (cretro,  p_emph, p_fech, MAX_LONG, MAX_SHORT);
				else{
					if (p_objh==NULL_SHORT)
						SetCursorTo  (cretro,  p_emph, p_fech, p_clih, MAX_SHORT);
					else
						SetCursorTo  (cretro,  p_emph, p_fech, p_clih, p_objh);
				}
			}
			else {
			     /* Indice por Cliente Objetivo */
				cretro = CreateCursor(ope|RETRObyRLEG, IO_NOT_LOCK);
				objant=NULL_SHORT;
				SetCursorFrom(cretro, p_empd, p_clid, p_objd, p_legd,  MIN_DATE);
				
				if(p_emph==NULL_SHORT){
					SetCursorTo  (cretro, MAX_SHORT, MAX_LONG, MAX_SHORT, MAX_LONG, MAX_DATE);
				}
				else {
					if(p_clih==NULL_LONG){
						SetCursorTo  (cretro, p_emph, MAX_LONG, MAX_SHORT, MAX_LONG, MAX_DATE);
					}
					else {
						if(p_objh==NULL_SHORT){
							SetCursorTo  (cretro, p_emph, p_clih, MAX_SHORT, MAX_LONG, MAX_DATE);
						}
						else {
							if(p_legh==NULL_LONG)
								SetCursorTo  (cretro, p_emph, p_clih, p_objh, MAX_LONG, MAX_DATE);
						} 
					}
				}	
			}
		}	

		while (FetchCursor(cretro) != ERROR) {
/*			if (p_grup)
				if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO)))
					continue;
*/
			/* Filtro por Cliente */
			if (p_clih != NULL_LONG && LFld(ope|RETRO_CLIENTE) > p_clih)
				continue;

			if (p_clid != NULL_LONG && LFld(ope|RETRO_CLIENTE) < p_clid)
				continue;                

			/* Filtro por Objetivo */
			if (p_objh != NULL_SHORT && IFld(ope|RETRO_OBJETIVO) > p_objh)
				continue;

			if (p_objd != NULL_SHORT && IFld(ope|RETRO_OBJETIVO) < p_objd)
				continue;

			/* Filtro por Legajo */
			if (p_legh != NULL_LONG && LFld(ope|RETRO_NROLEG) > p_legh)
				continue;

			if (p_legd != NULL_LONG && LFld(ope|RETRO_NROLEG) < p_legd)
				continue;

			/* Filtro por Fecha */
			if (p_fech != NULL_DATE && DFld(ope|RETRO_FECREG) > p_fech)
				continue;

			if (p_fecd != NULL_DATE && DFld(ope|RETRO_FECREG) < p_fecd)
				continue;

			if (IFld(ope|RETRO_OBJETIVO) != objant) {
				if (p_legd != NULL_LONG && p_empd != NULL_SHORT)
					DisplayMsg(FALSE, "Procesando Parte para Legajo %ld  Cliente %ld Objetivo %d", LFld(ope|RETRO_NROLEG), LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO));
				else {
					if (p_fecd != NULL_DATE  && p_empd != NULL_SHORT)
						DisplayMsg(FALSE, "Procesando Parte Fecha %.3D Cliente %ld Objetivo %d",DFld(ope|RETRO_FECREG), LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO));
					else 
						DisplayMsg(FALSE, "Procesando Parte de Cliente %ld Objetivo %d", LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO));
				}
				WiRefresh();
				objant = IFld(ope|RETRO_OBJETIVO);
			}
				
			empres    = (long)IFld(ope|RETRO_EMP);
			nodleg    = LFld(ope|RETRO_NROLEG);
			nodclie   = LFld(ope|RETRO_CLIENTE);
			objnod    = IFld(ope|RETRO_OBJETIVO);
			dianod    = DFld(ope|RETRO_DIA);
			legajonoc =LFld(ope|RETRO_NROLEG);
			fechanoc  = DFld(ope|RETRO_DIA);
//			sprintf(puenod, "%d-%d-%d", IFld(ope|RETRO_PTOSER), IFld(ope|RETRO_PUESTO), IFld(ope|RETRO_NROINT));
			sprintf(puenod, "%d-%d", IFld(ope|RETRO_PTOSER), IFld(ope|RETRO_PUESTO));
			n_ptoser = IFld(ope|RETRO_PTOSER);
			n_puesto = IFld(ope|RETRO_PUESTO);
			n_codint = IFld(ope|RETRO_NROINT);
			hordes   = TFld(ope|RETRO_HORAENT);
			horhas   = TFld(ope|RETRO_HORASAL);

			//Guardo Puesto 
 			n_horini = n_horfin = NULL_TIME;
 			n_dia1 = n_dia2 = n_dia3 = n_dia4 =	n_dia5 = n_dia6 = n_dia7 ='\0';
 			sprintf(n_regim, "%s", NULL_STR);
 			n_canvig = NULL_LONG;
 			n_canpto = NULL_SHORT;
 			n_tipdia = '\0';
 			n_fecini = n_fecfin = NULL_DATE;

			SetKey(ope|PUESTOSbyCLIENTE, LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO), IFld(ope|RETRO_PTOSER), IFld(ope|RETRO_PUESTO));
			if (GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {

				n_horini = TFld(ope|PUESTOS_HINICIO);
				n_horfin = TFld(ope|PUESTOS_HFINAL);
				n_dia1   = *SFld(ope|PUESTOS_DIA1);
				n_dia2   = *SFld(ope|PUESTOS_DIA2);
				n_dia3   = *SFld(ope|PUESTOS_DIA3);
				n_dia4   = *SFld(ope|PUESTOS_DIA4);
				n_dia5   = *SFld(ope|PUESTOS_DIA5);
				n_dia6   = *SFld(ope|PUESTOS_DIA6);
				n_dia7   = *SFld(ope|PUESTOS_DIA7);
				sprintf(n_regim, "%s", SFld(ope|PUESTOS_REGIM));
				n_canvig = LFld(ope|PUESTOS_CANTVIG);
				n_canpto = IFld(ope|PUESTOS_CANTPUE);
				n_tipdia = *SFld(ope|PUESTOS_TIPODIA);
				n_fecini = DFld(ope|PUESTOS_FINICIO);
				n_fecfin = DFld(ope|PUESTOS_FFINAL);
				
			} 

			sprintf(regim, "%s",NULL_STR);


			/* Si cambia el nroleg me fijo si esta activo */
			if (legant != LFld(ope|RETRO_NROLEG)) {
				legant = LFld(ope|RETRO_NROLEG);
				legact=LegajoActivo(IFld(ope|RETRO_EMP), LFld(ope|RETRO_NROLEG));
			}

			// Si no es el destino un pase o una prueba, que no valide el que los legajos sean activos.
			if (!(p_destino == DESTINO_PASE || p_destino == DESTINO_TEST)) {
				if (!p_act)
					if (!legact)
						continue;
			}

			SetKey(ope|ASIGbyNROLEG, IFld(ope|RETRO_EMP), LFld(ope|RETRO_NROLEG), LFld(ope|RETRO_CLIENTE), LFld(ope|RETRO_OBJETIVO));
			if (GetRecord(ope|ASIGbyNROLEG, THIS_KEY, IO_NOT_LOCK) != ERROR)
				sprintf(regim, "%s",SFld(ope|ASIG_REGIM));
			else {
				SetKey(ope|ASIGHbyNROLEG, IFld(ope|RETRO_EMP), LFld(ope|RETRO_NROLEG), LFld(ope|RETRO_CLIENTE), LFld(ope|RETRO_OBJETIVO));
				if (GetRecord(ope|ASIGHbyNROLEG, THIS_KEY, IO_NOT_LOCK) != ERROR)
					sprintf(regim, "%s", SFld(ope|ASIGH_REGIM));
			}


			for (i = 0; i < MAXTIPHOR; i++) {
				cantih[i] = 0;
			} 

			if (p_destino==DESTINO_PASE) {
			
				if(esMensual) {

					if (!IsNull(ope|RETRO_LIQDENA) && LFld(ope|RETRO_LIQDENA)!=p_liqui) {
						if (v_primerro){
							v_sal_error = fopen(auxarch, "w");
							v_primerro=FALSE;
						}
						fprintf(v_sal_error, "Error Parte Ya fue liquidado Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", 
						                   IFld(ope|RETRO_EMP), LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO), LFld(ope|RETRO_NROLEG), DFld(ope|RETRO_DIA));
					}else {
						SetLFld(ope|RETRO_LIQDENA, p_liqui);
						PutRecord(ope|RETRO);
					}
				}else {
					if (!IsNull(ope|RETRO_LIQDENUS) && LFld(ope|RETRO_LIQDENUS)!=p_liqui) {
						if (v_primerro){
							v_sal_error = fopen(auxarch, "w");
							v_primerro=FALSE;
						}
						fprintf(v_sal_error, "Error Parte Ya fue liquidado Emp: %d - Cliente %ld - Objetivo %d - Numero de Legajo %ld - Dia %.3D\n", 
						                   IFld(ope|RETRO_EMP), LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO), LFld(ope|RETRO_NROLEG), DFld(ope|RETRO_DIA));
					}else {
						SetLFld(ope|RETRO_LIQDENUS, p_liqui);
						PutRecord(ope|RETRO);
					}					
				}
			
			}
				
			cantih[HSNORM] = IFld(ope|RETRO_DHSNOR);
			cantih[HSAL25] = IFld(ope|RETRO_DHS50);
			cantih[HSAL35] = IFld(ope|RETRO_DHS100F);
			cantih[HSFRAN] = IFld(ope|RETRO_DHS100FE);

            /* Separa Ultima Semana*/
			CalcDistrHoras2(IFld(ope|RETRO_EMP),
			               LFld(ope|RETRO_NROLEG),
			               dianod,
			               *SFld(ope|RETRO_CONDIC),
			               TFld(ope|RETRO_HORAENT),
			               TFld(ope|RETRO_HORASAL),
			               LFld(ope|RETRO_CLIENTE),
			               IFld(ope|RETRO_OBJETIVO),
			               TRUE,
			               IFld(ope|RETRO_PTOSER),
			               IFld(ope|RETRO_PUESTO),
			               IFld(ope|RETRO_NROINT),
			               cantih);

			sprintf(v_tipo_calhs, "%s", GetParNov(IFld(ope|RETRO_EMP), PARNOV_TCALC_HS, LINEA_UNICA, Today()) );

			// Calcula cantidad de francos si el tipo de calculo es "A" (Ej emp 40)
/*			diasause=diasfran=diasfrat=diasvaca=diasvact=fermens=0;
			switch (v_tipo_calhs[0]) {
				case 'A': 
					switch (*SFld(ope|RETRO_CONDIC)) {
					case _FRANCO_C:
						if (TFld(ope|RETRO_HORAENT)==StrToT("00:00:0000") && TFld(ope|RETRO_HORASAL)==StrToT("00:00:0000")) {
							diasfran=1;
						}
						else {
							diasfrat=1;
						}
						break;
					case _AUSENTE_C:
						diasause=1;
						break;
					case _VACACIONES_C:
						if (TFld(ope|RETRO_HORAENT)==StrToT("00:00:0000") && TFld(ope|RETRO_HORASAL)==StrToT("00:00:0000")) {
							diasvaca=1;
						}
						else {
							diasvact=1;
						}
						break;
					}
					break;
				case 'B': 
					break;
			}
*/
			diasnoct=diasdiur=canpeg=diasadel=0;

			switch (*SFld(ope|RETRO_CONDIC)) {
			case _PEGADA_C:
				canpeg=1;
				break;
			case _ADELANTO_C:
				diasadel=1;
				break;
			}

			/* No vamos a tomar los retros para contar jornadas DHC
			// Me fijo si ya considere nocturna esta fecha para este legajo

			yacargo=FALSE;
			sprintf(v_auxtpto, "%d", IFld(ope|RETRO_PTOSER));
			if ((cantih[HSNONO]+cantih[HS25NO]+cantih[HS35NO])>=MIN_HS_NOCT 
			     && !EsParNov(IFld(ope|RETRO_EMP), PARNOV_TPUHSNOR, DFld(ope|RETRO_DIA), v_auxtpto)) {
				ini1 = AcuNLegajonoc(ini1, &ini1);
				if (yacargo==TRUE || canpeg == 1)
					diasnoct=0;
				else
					diasnoct=1;
			
			}
			else {
				ini2 = AcuNLegajonoc(ini2, &ini2);
				if (yacargo==TRUE || canpeg == 1)
					diasdiur=0;
				else
					diasdiur=1;

				if (hordes == StrToT("00:00") && horhas == StrToT("00:00"))
					diasdiur = 0;
			}*/


//fprintf(stderr, "BBB noct %d diur %d\n", diasnoct, diasdiur);
			ini = AcuNEmpres(ini, &ini);

		} 
		DeleteCursor(cretro);
		BorNLegajonoc(ini1);
		BorNLegajonoc(ini2);
		ini1 = ini2 = NULL;

	}
	DeleteAlias(APARTE);

	sprintf(p_msgerr, "");

	if (v_error==TRUE){
		if (p_destino==DESTINO_REPORTE) {
			sprintf(p_msgerr, "Partes diarios sin confirmar","Se genero un archivo que muestra en que registros puede no estar bien el reporte: %s\n ", auxarch);
			
		} 
		else {
			sprintf(p_msgerr, "Partes diarios sin confirmar","Se genero un archivo con los objetivos y fechas que faltan confirmar: %s\n ", auxarch);
//			return (NULL);

		}
		
		fclose(v_sal_error);
	}

	return(ini);
}

/*********************************************************************************************************
*                                   CalcDistrHoras2
*                                   --------------
* Calcula la distribucion de Horas
*
* Parametros: 
*		int  p_emp        =  Empresa
*		long p_nroleg,    =  Numero de Legajo
*		DATE p_fecha,     =  Fecha
*		TIME p_horent,    =  Horario desde 
*		TIME p_horsal,    =  Horario hasta
*		long p_cli,       =  Cliente 
*		int  p_obj,       =  Objetivo
*		bool p_retro      =  Esta leyendo retro?
*       int p_ptoser      =  Tipo de puesto
*       int p_puesto      =  Codigo de puesto
*       int p_nroint      =  Número de puesto
*		double* distrib   =  Matriz de salida de tamaño MAXTIPHOR
*
*									HSNORM 0 horas normales 
*									HSAL25 1 horas al 25% 
*									HSAL35 2 horas al 35% 
*									HSFRAN 3 horas franco
*									HSFERI 4 horas feriado
*									HSNONO 5 horas normales nocturnas
*									HS25NO 6 horas al 25% nocturnas
*									HS35NO 7 horas al 35% nocturnas
*
********************************************************************************************************
* CREATED          : 10/06/05 Fernando Ventura Goncalves
*********************************************************************************************************/
void CalcDistrHoras2(int p_emp, long p_nroleg, DATE p_fecha, char p_condic, TIME p_horent, TIME p_horsal, long p_cli, int p_obj, 
                    bool p_retro, int p_ptoser, int p_puesto, int p_nroint, double* distrib)
{
//DHC
	bool v_msj=FALSE;

	double 	hsferiado=0, horas,
		auxhora,auxhora1,auxhora2,auxhora3,auxhora4,auxfalt1,auxfalt2;

//	int hsnor=0,		hs25=0,		hs35=0,		hspeg=0,
	int	hsfra=0,
		hsgua=0,
		v_pais=0,
		v_prov=0,
		modocalc, hora;

	bool	hsnoct=FALSE,
			es_feriado_hoy=FALSE,
			es_feriado_manana=FALSE;
	
	TIME v_horasal=NULL_TIME;

	schema old, ope, com, sue, cgr;

	char v_tipo_calhs[2];
	int  v_j, i;


	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	com = OpenSchema("comerc", IO_EABORT);
	sue = OpenSchema("sue"   , IO_EABORT);
	cgr = OpenSchema("comgral", IO_EABORT);
	SwitchToSchema(old);


	for(v_j=0; v_j<MAXTIPHOR; v_j++) {
		distrib[v_j]=0;
	}

	// Si la hora de entrada y salida son 00:00 no hace falta que calcule mas nada todas las horas dan 0
	if (p_horent==StrToT("00:00:00") && p_horsal==StrToT("00:00:00"))
		return;

	sprintf(v_tipo_calhs, "%s", GetParNov(p_emp, PARNOV_TCALC_HS, LINEA_UNICA, Today()) );

	if (p_retro) {
		distrib[HSNORM]=(double)LFld(ope|RETRO_DHSNOR);
		distrib[HSAL25]=(double)LFld(ope|RETRO_DHS50);
		distrib[HSAL35]=(double)LFld(ope|RETRO_DHS100F);
		distrib[HSFRAN]=(double)LFld(ope|RETRO_DHS100FE);
		distrib[HSFERI]=0;
		distrib[HSNONO]=0;
		distrib[HS25NO]=0;
		distrib[HS35NO]=0;
		distrib[HSPEGA]=0;
	}
	else {
		distrib[HSNORM]=(double)LFld(ope|PARTE_HSNOR);
		distrib[HSAL25]=(double)LFld(ope|PARTE_HS50);
		distrib[HSAL35]=(double)LFld(ope|PARTE_HS100F);
		distrib[HSFRAN]=(double)LFld(ope|PARTE_HS100FE);

//		hsnor = hs25 = hs35 = hsfra = hspeg = hsgua = 0.0;
//		CalculoDetalleHorasPer(FALSE, NULL_SHORT, NULL_SHORT, p_emp, p_cli, p_obj,  p_fecha , p_nroleg, p_ptoser, p_puesto, p_nroint, &hsnor, &hs25, &hs35, &hsfra, &hspeg, &hsgua);
//		CalDetHorPer2(FALSE, NULL_SHORT, NULL_SHORT, p_emp, p_cli, p_obj,  p_fecha , p_nroleg, p_ptoser, p_puesto, p_nroint, p_condic, p_horent, p_horsal, &hsnor, &hs25, &hs35, &hsfra);
//		distrib[HSNORM]=(double)hsnor;
//		distrib[HSAL25]=(double)hs25;
//		distrib[HSAL35]=(double)hs35;
//		distrib[HSFRAN]=(double)hsfra;

		if (p_condic == _PEGADA_C) {
			distrib[HSPEGA]=distrib[HSAL35];
			distrib[HSAL35]=0;
		}
		distrib[HSFERI]=0;
		distrib[HSNONO]=0;
		distrib[HS25NO]=0;
		distrib[HS35NO]=0;

	}

	if (v_msj) {
		fprintf (stderr, "\n\n\n\n\nLegajo %ld Dia %.3D\n", p_nroleg, p_fecha);
		fprintf (stderr, "\nTipo de Calculo de Horas  %s\n", v_tipo_calhs);
		fprintf (stderr, "\nPegadas %.2f\n", distrib[HSPEGA]);
		fprintf (stderr, "Noche desde   %s\n", HOR_FIN_DIA);
		fprintf (stderr, "Noche hasta   %s\n", HOR_INI_DIA);
		fprintf (stderr, "Hora entrada  %.3T\n", p_horent);
		fprintf (stderr, "Hora salida   %.3T\n", p_horsal);
		fprintf (stderr, "\nCalcDistrHoras  antes de procesar Horas Nocturnas\n");
	}
	
	ImprimeHoras(v_msj, distrib, NULL);

	// Hay Horas Nocturnas?
	if (distrib[HSPEGA] == 0) {
		hsnoct = SuperposicionRangoHorario(p_horent, p_horsal, StrToT(HOR_FIN_DIA), StrToT(HOR_INI_DIA));
		v_horasal = p_horsal;
	}

	if (v_msj) fprintf (stderr, "Horas Nocturnas?   %B\n", hsnoct);

	// Calculo Distribucion de Horas Nocturnas
	if (hsnoct) {
		CalcDistrNocturnas(p_emp, p_nroleg, p_fecha, p_horent, v_horasal, p_cli, p_obj, p_retro, p_ptoser, p_puesto, p_nroint, 
		                   v_msj, es_feriado_hoy, es_feriado_manana, distrib);
	}

	ImprimeHoras(v_msj, distrib,  NULL);

	// Inicializo variables de feriados y francos para liquidacion 
	for(v_j=HSNOFE; v_j<=H35FRN; v_j++)
		distrib[v_j]=0;

	if (hsfra > 0) {  // Distribuyo el franco
	
		modocalc = 0;
		if (p_horent < StrToT(HOR_INI_DIA)) {
			modocalc += 10;
		}
		else {
			if (p_horent >= StrToT(HOR_INI_DIA) && p_horent < StrToT(HOR_FIN_DIA)) {
				modocalc += 20;
			}
			else {
				modocalc += 30;
			}
		}
		if (p_horsal <= StrToT(HOR_INI_DIA)) {
			modocalc += 1;
		}
		else {
			if (p_horsal > StrToT(HOR_INI_DIA) && p_horsal <= StrToT(HOR_FIN_DIA)) {
				modocalc += 2;
			}
			else {
				modocalc += 3;
			}
		}
        auxhora = (double)ConvHraInt(p_horent, p_horsal)*100;
		auxfalt1=_HORAS_DE_TRABAJO;
		auxfalt2=_HORAS_AL_25;

		switch (modocalc) {
		case 11:
		case 33:
			if (p_horent > p_horsal) {
				WiMsg("error %T, %T", p_horent, p_horsal);
			}
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora;
					}
					distrib[hora] += horas;
					auxhora -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora >= auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora;
					}
					distrib[hora] += horas;
					auxhora -= horas;
					break;
				case 3:
					hora = H35FRN;
					distrib[hora] = auxhora;
					auxhora=0;
					break;
				}
			}
			break;
		case 12:
			auxhora1=(double)ConvHraInt(p_horent, StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora1 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora1 >= 0) {
						if (auxhora1 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora1 >= 0) {
						distrib[H35FRN] = auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
            break;
		case 13:
			auxhora1=(double)ConvHraInt(p_horent, StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora1 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = H25FRN;
					if (auxhora1 >= 0) {
						if (auxhora1 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora1 >= 0) {
						distrib[H35FRN] = auxhora1;
					}

					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					distrib[H35FRN] += auxhora3;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 21:
			auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), StrToT("23:59"))*100;
			auxhora1=(double)ConvHraInt(StrToT("00:00"), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HSNOFR;
					if (auxhora2 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = HS25FR;
					if (auxhora2 >= 0) {
						if (auxhora2 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;
                                            
					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					distrib[H35FRN] += auxhora1;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 22:
			if (p_horent > p_horsal) {
				auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
	 			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), StrToT("00:00"))*100;
	 			auxhora1=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
				auxhora4=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;


				for (i=1; auxhora > 0; i++) {
					switch(i) {
					case 1:
						hora = HSNOFR;
						if (auxhora2 >= auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HNOFRN;
						if (auxhora3 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HNOFRN;
						if (auxhora1 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HSNOFR;
						if (auxhora4 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora4;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora4 -= horas;
						auxfalt1 -= horas;

						break;
					case 2:
						hora = HS25FR;
						if (auxhora2 >= 0) {
							if (auxhora2 >= auxfalt2) {
								horas = auxfalt2;
							}
							else {
								horas = auxhora2;
							}
							distrib[hora] += horas;
							auxhora  -= horas;
							auxhora2 -= horas;
							auxfalt2 -= horas;
						}

						if (auxfalt2 == 0)
							continue;

						hora = H25FRN;
						if (auxhora3 > auxfalt2) {
							horas = auxfalt2;
						}                              
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;

						if (auxfalt2 == 0)
							continue;

						hora = H25FRN;
						if (auxhora1 > auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;

						if (auxfalt2 == 0)
							continue;

						hora = HS25FR;
						if (auxhora4 > auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora4;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora4 -= horas;
						auxfalt2 -= horas;

						break;
					case 3:
						if (auxhora2 >= 0) {
							distrib[HS35FR] = auxhora2;
						}

						if (auxhora3 >= 0) {
							distrib[H35FRN] = auxhora3;
						}

						if (auxhora1 >= 0) {
							distrib[H35FRN] += auxhora1;
						}

						distrib[HS35FR] += auxhora4;

						auxhora  = 0;
						break;
					}
				}
			}
			else {
				for (i=1; auxhora > 0; i++) {
					switch(i) {
					case 1:
						hora = HSNOFR;
						if (auxhora >= auxfalt1) {
							distrib[hora] = auxfalt1;
						}
						else {
							distrib[hora] = auxhora;
						}
						auxhora -= distrib[hora];
						break;
					case 2:
						hora = HS25FR;
						if (auxhora >= auxfalt2) {
							distrib[hora] = auxfalt2;
						}
						else {
							distrib[hora] = auxhora;
						}
						auxhora -= distrib[hora];
						break;
					case 3:
						hora = HS35FR;
						distrib[hora] = auxhora;
						auxhora=0;
						break;
					}
				}
			}
			break;
		case 23:
			auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HSNOFR;
					if (auxhora2 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = HS25FR;
					if (auxhora2 >= 0) {
						if (auxhora2 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					distrib[H35FRN] = auxhora3;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 31:
			auxhora3=(double)ConvHraInt(p_horent, StrToT("00:00"))*100;
			auxhora2=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
			auxhora1=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora3 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = H25FRN;
					if (auxhora3 >= 0) {
						if (auxhora3 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					if (auxhora1 >= 0) {
						distrib[H35FRN] += auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 32:
			auxhora3=(double)ConvHraInt(p_horent, StrToT("00:00"))*100;
			auxhora1=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora3 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora3 >= 0) {
						if (auxhora3 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					if (auxhora1 >= 0) {
						distrib[H35FRN] += auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
			break;
		default:
			WiMsg("error %d", modocalc);
		}
	}


	switch (v_tipo_calhs[0]) {
		case 'A':

			if (distrib[HSPEGA]>0) //DHC
				break;

			if (distrib[HSFRAN]>0)
				break;

			if (hsgua>0)
				break;
				

				// Entra y Sale el mismo dia 
				if(p_horsal>p_horent) {

					if (v_msj) fprintf (stderr, "Entra y sale el mismo dia\n");

					//Feriado 
					v_pais=GetCliePais(p_cli, p_obj);
					v_prov=GetProvObjet(p_cli, p_obj);
	
	
					if (v_msj) fprintf (stderr, "Pais %d Prov %d\n", v_pais, v_prov);

					if (FeriadoNovia(p_fecha, v_pais, v_prov) ) {
						if (v_msj) fprintf (stderr, "Es un Feriado\n");

						distrib[HSNOFE]+=distrib[HSNORM];
						distrib[HS25FE]+=distrib[HSAL25];
						distrib[HS35FE]+=distrib[HSAL35];

						distrib[HNOFEN]+=distrib[HSNONO];
						distrib[H25FEN]+=distrib[HS25NO];
						distrib[H35FEN]+=distrib[HS35NO];

						// Acumulo los tipos de horas que corresponden en feriadas
						distrib[HSFERI]=0;
						for(v_j=0; v_j<MAXTIPHOR; v_j++) {
							switch(v_j) {
								case HSNORM :
								case HSAL25 :
								case HSAL35 :
								case HSNONO :
								case HS25NO :
								case HS35NO :
									distrib[HSFERI]+=distrib[v_j];
									distrib[v_j]=0;
									break;
								case HSPEGA : //DHC Pidieron que para feriado tambien cuente las pegadas
									distrib[HSFERI]+=distrib[v_j];
									break;
							}
						} 
	                    es_feriado_hoy=TRUE;

					}
					
				}
				// Entra un dia y Sale al otro 
				else {
					if (v_msj) fprintf (stderr, "Entra y sale al otro dia\n");

                    es_feriado_hoy=FALSE;
                    es_feriado_manana=FALSE;

					v_pais=GetCliePais(p_cli, p_obj);
					v_prov=GetProvObjet(p_cli, p_obj);

					if (FeriadoNovia(p_fecha, v_pais, v_prov) ) {
						if (v_msj) fprintf (stderr, "Es un Feriado\n");

						hsferiado = ((double)ConvHraInt(p_horent, StrToT("23:59")))*100;
						if (v_msj) fprintf (stderr, "Hoy Hs Feriado  %.2f\n", hsferiado);
						Asignar(&distrib[HSNORM], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSNONO], &distrib[HNOFEN], &hsferiado);
						Asignar(&distrib[HSAL25], &distrib[HS25FE], &hsferiado);
						Asignar(&distrib[HS25NO], &distrib[H25FEN], &hsferiado);
						Asignar(&distrib[HSAL35], &distrib[HS35FE], &hsferiado);
						Asignar(&distrib[HS35NO], &distrib[H35FEN], &hsferiado);
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
//						Asignar(&distrib[HSPEGA], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSFRAN], &distrib[HSNOFE], &hsferiado);


/*						hspegadas = distrib[HSPEGA];  //DHC Pidieron que para feriado tambien cuente las pegadas
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
						distrib[HSPEGA] = hspegadas;
						distrib[HSAL35] = hspegadas;
*/
	                    es_feriado_hoy=TRUE;

					}
					if (FeriadoNovia(p_fecha+1, v_pais, v_prov) ) {
						hsferiado = ((double)ConvHraInt(StrToT("00:00"), p_horsal)*100);
						if (v_msj) fprintf (stderr, "Mañana Hs Feriado  %.2f\n", hsferiado);
						Asignar(&distrib[HSFRAN], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
//						Asignar(&distrib[HSPEGA], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HS35NO], &distrib[H35FEN], &hsferiado);
						Asignar(&distrib[HSAL35], &distrib[HS35FE], &hsferiado);
						Asignar(&distrib[HS25NO], &distrib[H25FEN], &hsferiado);
						Asignar(&distrib[HSAL25], &distrib[HS25FE], &hsferiado);
						Asignar(&distrib[HSNONO], &distrib[HNOFEN], &hsferiado);
						Asignar(&distrib[HSNORM], &distrib[HSNOFE], &hsferiado);



/*						hspegadas = distrib[HSPEGA];  //DHC Pidieron que para feriado tambien cuente las pegadas
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
						distrib[HSPEGA] = hspegadas;
						distrib[HSAL35] = hspegadas;
*/
	                    es_feriado_manana=TRUE;
					}


					distrib[HSFERI]=0;
					for(v_j=HSNOFE; v_j<=H35FEN; v_j++) 
						distrib[HSFERI]+=distrib[v_j];

				}
				
			break;
		case 'B':
			
			break;
		default :
			break;
			 
	}
	if (v_msj) fprintf (stderr, "\nCalcDistrHoras Despues de Feriados\n");
		ImprimeHoras(v_msj, distrib, NULL);

	return;	
}

/*********************************************************************************************************
*                                   CalcDistrHoras
*                                   --------------
* Calcula la distribucion de Horas
*
* Parametros: 
*		int  p_emp        =  Empresa
*		long p_nroleg,    =  Numero de Legajo
*		DATE p_fecha,     =  Fecha
*		TIME p_horent,    =  Horario desde 
*		TIME p_horsal,    =  Horario hasta
*		long p_cli,       =  Cliente 
*		int  p_obj,       =  Objetivo
*		bool p_retro      =  Esta leyendo retro?
*       int p_ptoser      =  Tipo de puesto
*       int p_puesto      =  Codigo de puesto
*       int p_nroint      =  Número de puesto
*		double* distrib   =  Matriz de salida de tamaño MAXTIPHOR
*
*									HSNORM 0 horas normales 
*									HSAL25 1 horas al 25% 
*									HSAL35 2 horas al 35% 
*									HSFRAN 3 horas franco
*									HSFERI 4 horas feriado
*									HSNONO 5 horas normales nocturnas
*									HS25NO 6 horas al 25% nocturnas
*									HS35NO 7 horas al 35% nocturnas
*
********************************************************************************************************
* CREATED          : 10/06/05 Fernando Ventura Goncalves
*********************************************************************************************************/
void CalcDistrHoras(int p_emp, long p_nroleg, DATE p_fecha, char p_condic, TIME p_horent, TIME p_horsal, long p_cli, int p_obj, 
                    bool p_retro, int p_ptoser, int p_puesto, int p_nroint, double* distrib)
{
//DHC
	bool v_msj=FALSE;

	double 	hsferiado=0, horas,
		auxhora,auxhora1,auxhora2,auxhora3,auxhora4,auxfalt1,auxfalt2;

	int hsnor=0,
		hs25=0,
		hs35=0,
		hsfra=0,
		hspeg=0,
		hsgua=0,
		v_pais=0,
		v_prov=0,
		modocalc, hora;

	bool	hsnoct=FALSE,
			es_feriado_hoy=FALSE,
			es_feriado_manana=FALSE;
	
	TIME v_horasal=NULL_TIME;


	schema old, ope, com, sue, cgr;

	char v_tipo_calhs[2];
	int  v_j, i;


	old = CurrentSchema();
	ope = OpenSchema("operac", IO_EABORT);
	com = OpenSchema("comerc", IO_EABORT);
	sue = OpenSchema("sue"   , IO_EABORT);
	cgr = OpenSchema("comgral", IO_EABORT);
	SwitchToSchema(old);


	for(v_j=0; v_j<MAXTIPHOR; v_j++) {
		distrib[v_j]=0;
	}

	// Si la hora de entrada y salida son 00:00 no hace falta que calcule mas nada todas las horas dan 0
	if (p_horent==StrToT("00:00:00") && p_horsal==StrToT("00:00:00"))
		return;

	sprintf(v_tipo_calhs, "%s", GetParNov(p_emp, PARNOV_TCALC_HS, LINEA_UNICA, Today()) );

	if (p_retro) {
		distrib[HSNORM]=(double)LFld(ope|RETRO_DHSNOR);
		distrib[HSAL25]=(double)LFld(ope|RETRO_DHS50);
		distrib[HSAL35]=(double)LFld(ope|RETRO_DHS100F);
		distrib[HSFRAN]=(double)LFld(ope|RETRO_DHS100FE);
		distrib[HSFERI]=0;
		distrib[HSNONO]=0;
		distrib[HS25NO]=0;
		distrib[HS35NO]=0;
		distrib[HSPEGA]=0;
	}
	else {
		hsnor = hs25 = hs35 = hsfra = hspeg = hsgua = 0.0;
//		CalculoDetalleHorasPer(FALSE, NULL_SHORT, NULL_SHORT, p_emp, p_cli, p_obj,  p_fecha , p_nroleg, p_ptoser, p_puesto, p_nroint, &hsnor, &hs25, &hs35, &hsfra, &hspeg, &hsgua);

		CalDetHorPer2(FALSE, NULL_SHORT, NULL_SHORT, p_emp, p_cli, p_obj,  p_fecha , p_nroleg, p_ptoser, p_puesto, p_nroint, p_condic, p_horent, p_horsal, &hsnor, &hs25, &hs35, &hsfra);

		distrib[HSNORM]=(double)hsnor;
		distrib[HSAL25]=(double)hs25;
		distrib[HSAL35]=(double)hs35;
		distrib[HSFRAN]=(double)hsfra;
		distrib[HSFERI]=0;
		distrib[HSNONO]=0;
		distrib[HS25NO]=0;
		distrib[HS35NO]=0;
		distrib[HSPEGA]=(double)hspeg;

	}

	if (v_msj) fprintf (stderr, "\n\n\n\n\nLegajo %ld Dia %.3D\n", p_nroleg, p_fecha);
	if (v_msj) fprintf (stderr, "\nTipo de Calculo de Horas  %s\n", v_tipo_calhs);
	if (v_msj) fprintf (stderr, "\nPegadas %.2f\n", distrib[HSPEGA]);
	if (v_msj) fprintf (stderr, "Noche desde   %s\n", HOR_FIN_DIA);
	if (v_msj) fprintf (stderr, "Noche hasta   %s\n", HOR_INI_DIA);
	if (v_msj) fprintf (stderr, "Hora entrada  %.3T\n", p_horent);
	if (v_msj) fprintf (stderr, "Hora salida   %.3T\n", p_horsal);

	if (v_msj) fprintf (stderr, "\nCalcDistrHoras  antes de procesar Horas Nocturnas\n");
		ImprimeHoras(v_msj, distrib, NULL);

	// Hay Horas Nocturnas?
	if (distrib[HSPEGA]>0) {
/*		dif_horas= ((int)distrib[HSPEGA])/100; DHC si tiene pegadas no se consideran nocturnas
		dif_horas= dif_horas * (-1);
		v_horasal=SumaTiempo(p_horsal, dif_horas, _HORAS);
		if (v_msj) fprintf (stderr, "\nHora de salida sin Pegadas %.3T\n", v_horasal);

		hsnoct= SuperposicionRangoHorario(p_horent, v_horasal, StrToT(HOR_FIN_DIA), StrToT(HOR_INI_DIA));
*/ 
	}
	else {
		hsnoct= SuperposicionRangoHorario(p_horent, p_horsal, StrToT(HOR_FIN_DIA), StrToT(HOR_INI_DIA));
		v_horasal=p_horsal;
	}

	if (v_msj) fprintf (stderr, "Horas Nocturnas?   %B\n", hsnoct);

	// Calculo Distribucion de Horas Nocturnas
	if (hsnoct) {
		CalcDistrNocturnas(p_emp, p_nroleg, p_fecha, p_horent, v_horasal, p_cli, p_obj, p_retro, p_ptoser, p_puesto, p_nroint, 
		                   v_msj, es_feriado_hoy, es_feriado_manana, distrib);
	}

	ImprimeHoras(v_msj, distrib,  NULL);

	// Inicializo variables de feriados y francos para liquidacion 
	for(v_j=HSNOFE; v_j<=H35FRN; v_j++)
		distrib[v_j]=0;

	if (hsfra > 0) {  // Distribuyo el franco
	
		modocalc = 0;
		if (p_horent < StrToT(HOR_INI_DIA)) {
			modocalc += 10;
		}
		else {
			if (p_horent >= StrToT(HOR_INI_DIA) && p_horent < StrToT(HOR_FIN_DIA)) {
				modocalc += 20;
			}
			else {
				modocalc += 30;
			}
		}
		if (p_horsal <= StrToT(HOR_INI_DIA)) {
			modocalc += 1;
		}
		else {
			if (p_horsal > StrToT(HOR_INI_DIA) && p_horsal <= StrToT(HOR_FIN_DIA)) {
				modocalc += 2;
			}
			else {
				modocalc += 3;
			}
		}
        auxhora = (double)ConvHraInt(p_horent, p_horsal)*100;
		auxfalt1=_HORAS_DE_TRABAJO;
		auxfalt2=_HORAS_AL_25;

		switch (modocalc) {
		case 11:
		case 33:
			if (p_horent > p_horsal) {
				WiMsg("error %T, %T", p_horent, p_horsal);
			}
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora;
					}
					distrib[hora] += horas;
					auxhora -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora >= auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora;
					}
					distrib[hora] += horas;
					auxhora -= horas;
					break;
				case 3:
					hora = H35FRN;
					distrib[hora] = auxhora;
					auxhora=0;
					break;
				}
			}
			break;
		case 12:
			auxhora1=(double)ConvHraInt(p_horent, StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora1 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora1 >= 0) {
						if (auxhora1 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora1 >= 0) {
						distrib[H35FRN] = auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
            break;
		case 13:
			auxhora1=(double)ConvHraInt(p_horent, StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora1 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = H25FRN;
					if (auxhora1 >= 0) {
						if (auxhora1 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora1 >= 0) {
						distrib[H35FRN] = auxhora1;
					}

					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					distrib[H35FRN] += auxhora3;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 21:
			auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), StrToT("23:59"))*100;
			auxhora1=(double)ConvHraInt(StrToT("00:00"), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HSNOFR;
					if (auxhora2 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = HS25FR;
					if (auxhora2 >= 0) {
						if (auxhora2 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;
                                            
					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					distrib[H35FRN] += auxhora1;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 22:
			if (p_horent > p_horsal) {
				auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
	 			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), StrToT("00:00"))*100;
	 			auxhora1=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
				auxhora4=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;


				for (i=1; auxhora > 0; i++) {
					switch(i) {
					case 1:
						hora = HSNOFR;
						if (auxhora2 >= auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HNOFRN;
						if (auxhora3 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HNOFRN;
						if (auxhora1 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt1 -= horas;

						if (auxfalt1 == 0)
							continue;

						hora = HSNOFR;
						if (auxhora4 > auxfalt1) {
							horas = auxfalt1;
						}
						else {
							horas = auxhora4;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora4 -= horas;
						auxfalt1 -= horas;

						break;
					case 2:
						hora = HS25FR;
						if (auxhora2 >= 0) {
							if (auxhora2 >= auxfalt2) {
								horas = auxfalt2;
							}
							else {
								horas = auxhora2;
							}
							distrib[hora] += horas;
							auxhora  -= horas;
							auxhora2 -= horas;
							auxfalt2 -= horas;
						}

						if (auxfalt2 == 0)
							continue;

						hora = H25FRN;
						if (auxhora3 > auxfalt2) {
							horas = auxfalt2;
						}                              
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;

						if (auxfalt2 == 0)
							continue;

						hora = H25FRN;
						if (auxhora1 > auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora1;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora1 -= horas;
						auxfalt2 -= horas;

						if (auxfalt2 == 0)
							continue;

						hora = HS25FR;
						if (auxhora4 > auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora4;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora4 -= horas;
						auxfalt2 -= horas;

						break;
					case 3:
						if (auxhora2 >= 0) {
							distrib[HS35FR] = auxhora2;
						}

						if (auxhora3 >= 0) {
							distrib[H35FRN] = auxhora3;
						}

						if (auxhora1 >= 0) {
							distrib[H35FRN] += auxhora1;
						}

						distrib[HS35FR] += auxhora4;

						auxhora  = 0;
						break;
					}
				}
			}
			else {
				for (i=1; auxhora > 0; i++) {
					switch(i) {
					case 1:
						hora = HSNOFR;
						if (auxhora >= auxfalt1) {
							distrib[hora] = auxfalt1;
						}
						else {
							distrib[hora] = auxhora;
						}
						auxhora -= distrib[hora];
						break;
					case 2:
						hora = HS25FR;
						if (auxhora >= auxfalt2) {
							distrib[hora] = auxfalt2;
						}
						else {
							distrib[hora] = auxhora;
						}
						auxhora -= distrib[hora];
						break;
					case 3:
						hora = HS35FR;
						distrib[hora] = auxhora;
						auxhora=0;
						break;
					}
				}
			}
			break;
		case 23:
			auxhora2=(double)ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100;
			auxhora3=(double)ConvHraInt(StrToT(HOR_FIN_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HSNOFR;
					if (auxhora2 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora3 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = HS25FR;
					if (auxhora2 >= 0) {
						if (auxhora2 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora2;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora2 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora3 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora2 >= 0) {
						distrib[HS35FR] = auxhora2;
					}

					distrib[H35FRN] = auxhora3;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 31:
			auxhora3=(double)ConvHraInt(p_horent, StrToT("00:00"))*100;
			auxhora2=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
			auxhora1=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora3 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;

					break;
				case 2:
					hora = H25FRN;
					if (auxhora3 >= 0) {
						if (auxhora3 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = H25FRN;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					if (auxhora1 >= 0) {
						distrib[H35FRN] += auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
			break;
		case 32:
			auxhora3=(double)ConvHraInt(p_horent, StrToT("00:00"))*100;
			auxhora1=(double)ConvHraInt(StrToT("00:00"), StrToT(HOR_INI_DIA))*100;
			auxhora2=(double)ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100;
			for (i=1; auxhora > 0; i++) {
				switch(i) {
				case 1:
					hora = HNOFRN;
					if (auxhora3 >= auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora3;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora3 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HNOFRN;
					if (auxhora1 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt1 -= horas;

					if (auxfalt1 == 0)
						continue;

					hora = HSNOFR;
					if (auxhora2 > auxfalt1) {
						horas = auxfalt1;
					}
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt1 -= horas;
					break;
				case 2:
					hora = H25FRN;
					if (auxhora3 >= 0) {
						if (auxhora3 >= auxfalt2) {
							horas = auxfalt2;
						}
						else {
							horas = auxhora3;
						}
						distrib[hora] += horas;
						auxhora  -= horas;
						auxhora3 -= horas;
						auxfalt2 -= horas;
					}

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora1 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora1;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora1 -= horas;
					auxfalt2 -= horas;

					if (auxfalt2 == 0)
						continue;

					hora = HS25FR;
					if (auxhora2 > auxfalt2) {
						horas = auxfalt2;
					}                              
					else {
						horas = auxhora2;
					}
					distrib[hora] += horas;
					auxhora  -= horas;
					auxhora2 -= horas;
					auxfalt2 -= horas;

					break;
				case 3:
					if (auxhora3 >= 0) {
						distrib[H35FRN] = auxhora3;
					}

					if (auxhora1 >= 0) {
						distrib[H35FRN] += auxhora1;
					}

					distrib[HS35FR] = auxhora2;

					auxhora  = 0;
					break;
				}
			}
			break;
		default:
			WiMsg("error %d", modocalc);
		}
	}

	switch (v_tipo_calhs[0]) {
		case 'A':
			if (distrib[HSPEGA]>0) //DHC
				break;
			if (distrib[HSFRAN]>0)
				break;
			if (hsgua>0)
				break;
				

				// Entra y Sale el mismo dia 
				if(p_horsal>p_horent) {

					if (v_msj) fprintf (stderr, "Entra y sale el mismo dia\n");

					//Feriado 
					v_pais=GetCliePais(p_cli, p_obj);
					v_prov=GetProvObjet(p_cli, p_obj);
	
	
					if (v_msj) fprintf (stderr, "Pais %d Prov %d\n", v_pais, v_prov);

					if (FeriadoNovia(p_fecha, v_pais, v_prov) ) {
						if (v_msj) fprintf (stderr, "Es un Feriado\n");

						distrib[HSNOFE]+=distrib[HSNORM];
						distrib[HS25FE]+=distrib[HSAL25];
						distrib[HS35FE]+=distrib[HSAL35];

						distrib[HNOFEN]+=distrib[HSNONO];
						distrib[H25FEN]+=distrib[HS25NO];
						distrib[H35FEN]+=distrib[HS35NO];

						// Acumulo los tipos de horas que corresponden en feriadas
						distrib[HSFERI]=0;
						for(v_j=0; v_j<MAXTIPHOR; v_j++) {
							switch(v_j) {
								case HSNORM :
								case HSAL25 :
								case HSAL35 :
								case HSNONO :
								case HS25NO :
								case HS35NO :
									distrib[HSFERI]+=distrib[v_j];
									distrib[v_j]=0;
									break;
								case HSPEGA : //DHC Pidieron que para feriado tambien cuente las pegadas
									distrib[HSFERI]+=distrib[v_j];
									break;
							}
						} 
	                    es_feriado_hoy=TRUE;

					}
					
				}
				// Entra un dia y Sale al otro 
				else {
					if (v_msj) fprintf (stderr, "Entra y sale al otro dia\n");

                    es_feriado_hoy=FALSE;
                    es_feriado_manana=FALSE;

					v_pais=GetCliePais(p_cli, p_obj);
					v_prov=GetProvObjet(p_cli, p_obj);

					if (FeriadoNovia(p_fecha, v_pais, v_prov) ) {
						if (v_msj) fprintf (stderr, "Es un Feriado\n");

						hsferiado = ((double)ConvHraInt(p_horent, StrToT("23:59")))*100;
						if (v_msj) fprintf (stderr, "Hoy Hs Feriado  %.2f\n", hsferiado);
						Asignar(&distrib[HSNORM], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSNONO], &distrib[HNOFEN], &hsferiado);
						Asignar(&distrib[HSAL25], &distrib[HS25FE], &hsferiado);
						Asignar(&distrib[HS25NO], &distrib[H25FEN], &hsferiado);
						Asignar(&distrib[HSAL35], &distrib[HS35FE], &hsferiado);
						Asignar(&distrib[HS35NO], &distrib[H35FEN], &hsferiado);
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
//						Asignar(&distrib[HSPEGA], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSFRAN], &distrib[HSNOFE], &hsferiado);


/*						hspegadas = distrib[HSPEGA];  //DHC Pidieron que para feriado tambien cuente las pegadas
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
						distrib[HSPEGA] = hspegadas;
						distrib[HSAL35] = hspegadas;
*/
	                    es_feriado_hoy=TRUE;

					}
					if (FeriadoNovia(p_fecha+1, v_pais, v_prov) ) {
						hsferiado = ((double)ConvHraInt(StrToT("00:00"), p_horsal)*100);
						if (v_msj) fprintf (stderr, "Mañana Hs Feriado  %.2f\n", hsferiado);
						Asignar(&distrib[HSFRAN], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
//						Asignar(&distrib[HSPEGA], &distrib[HSNOFE], &hsferiado);
						Asignar(&distrib[HS35NO], &distrib[H35FEN], &hsferiado);
						Asignar(&distrib[HSAL35], &distrib[HS35FE], &hsferiado);
						Asignar(&distrib[HS25NO], &distrib[H25FEN], &hsferiado);
						Asignar(&distrib[HSAL25], &distrib[HS25FE], &hsferiado);
						Asignar(&distrib[HSNONO], &distrib[HNOFEN], &hsferiado);
						Asignar(&distrib[HSNORM], &distrib[HSNOFE], &hsferiado);



/*						hspegadas = distrib[HSPEGA];  //DHC Pidieron que para feriado tambien cuente las pegadas
						Asignar(&distrib[HSPEGA], &distrib[HSAL35], &hsferiado);
						distrib[HSPEGA] = hspegadas;
						distrib[HSAL35] = hspegadas;
*/
	                    es_feriado_manana=TRUE;
					}


					distrib[HSFERI]=0;
					for(v_j=HSNOFE; v_j<=H35FEN; v_j++) 
						distrib[HSFERI]+=distrib[v_j];

				}
				
			break;
		case 'B':
			
			break;
		default :
			break;
			 
	}
	if (v_msj) fprintf (stderr, "\nCalcDistrHoras Despues de Feriados\n");
		ImprimeHoras(v_msj, distrib, NULL);

	return;	
}

/*********************************************************************************************************
*                                   CalcDistrNocturnas
*                                   ------------------
* Calcula la distribucion de Horas Nocturnas
*
* Parametros: 
*		int  p_emp        =  Empresa
*		long p_nroleg,    =  Numero de Legajo
*		DATE p_fecha,     =  Fecha
*		TIME p_horent,    =  Horario desde 
*		TIME p_horsal,    =  Horario hasta
*		long p_cli,       =  Cliente 
*		int  p_obj,       =  Objetivo
*		bool p_retro      =  Esta leyendo retro?
*       int p_ptoser      =  Tipo de puesto
*       int p_puesto      =  Codigo de puesto
*       int p_nroint      =  Número de puesto
*		bool p_msj		  =	 Imprime mensaje de debug si es si
*		double* distrib   =  Matriz de salida de tamaño MAXTIPHOR
*
*									HSNORM 0 horas normales 
*									HSAL25 1 horas al 25% 
*									HSAL35 2 horas al 35% 
*									HSFRAN 3 horas franco
*									HSFERI 4 horas feriado
*									HSNONO 5 horas normales nocturnas
*									HS25NO 6 horas al 25% nocturnas
*									HS35NO 7 horas al 35% nocturnas
*
********************************************************************************************************
* CREATED          : 10/06/05 Fernando Ventura Goncalves
*********************************************************************************************************/
private void CalcDistrNocturnas(int p_emp, long p_nroleg, DATE p_fecha,  TIME p_horent, TIME p_horsal, long p_cli, int p_obj, bool p_retro, 
								int p_ptoser, int p_puesto, int p_nroint, bool p_msj, bool p_es_feriado_hoy, bool p_es_feriado_manana, double* distrib)
{

	int v_j=0;

	double 	aux_distrib[MAXTIPHOR],
			canhsnoct=0, 
			canhsdia=0, 
			canhshoy=0;
			
			

	// [7m Limpio Matriz Auxiliar [0m
	for(v_j=0; v_j<MAXTIPHOR; v_j++) 
		aux_distrib[v_j]=0;

	if (p_msj) fprintf (stderr, "Hora Entrada = %.3T Hora Salida = %.3T\n", p_horent, p_horsal);


	if (p_horent<p_horsal) {   //  [7m Entra y sale el mismo dia [0m

		if (p_es_feriado_hoy) 
			return;

		// [7m Calculo Horas de Hoy [0m
//		canhshoy=(int)(ConvHraInt(p_horent, StrToT("00:00:00") )*100); antes estaba asi DHC
		canhshoy=(int)(ConvHraInt(p_horent, p_horsal )*100);

		// [7m Calculo las Horas diurnas y nocturnas de hoy  [0m
		canhsdia=0;
		if (p_horent<StrToT(HOR_FIN_DIA))
			canhsdia=(int)(ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100);
		canhsnoct=canhshoy - canhsdia;  



		if (p_msj) fprintf (stderr, "Hoy Hs HOY *       = %.2f\n", canhshoy);
		if (p_msj) fprintf (stderr, "Hoy Hs Diurnas *   = %.2f\n", canhsdia);
		if (p_msj) fprintf (stderr, "Hoy Hs Nocturnas * = %.2f\n", canhsnoct);

		// [7m Separo horas Nocturnas de Matriz auxiliar a la principal [0m
		// [7m Asigno de mayor a menor tipos de horas [0m
		Asignar(&distrib[HSAL35], &distrib[HS35NO], &canhsnoct);
		Asignar(&distrib[HSAL25], &distrib[HS25NO], &canhsnoct);
		Asignar(&distrib[HSNORM], &distrib[HSNONO], &canhsnoct);

		if (p_msj) fprintf (stderr, "\nDESPUES\n");
		ImprimeHoras(p_msj, distrib, aux_distrib);

	}
	else{	//  [7m Entra un dia y sale al otro [0m

		if (!p_es_feriado_hoy) {
			// [7m      HOY      [0m   

			// [7m Calculo Horas de Hoy [0m
			canhshoy=(int)(ConvHraInt(p_horent, StrToT("23:59:00") )*100);

			// [7m Calculo las Horas diurnas y nocturnas de hoy  [0m
			canhsdia=0;
			if (p_horent<StrToT(HOR_FIN_DIA))
				canhsdia=(int)(ConvHraInt(p_horent, StrToT(HOR_FIN_DIA))*100);
			canhsnoct=canhshoy - canhsdia;  

			if (p_msj) fprintf (stderr, "Hoy Hs HOY       = %.2f\n", canhshoy);
			if (p_msj) fprintf (stderr, "Hoy Hs Diurnas   = %.2f\n", canhsdia);
			if (p_msj) fprintf (stderr, "Hoy Hs Nocturnas = %.2f\n", canhsnoct);

			if (p_msj) fprintf (stderr, "\nANTES\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);

			// [7m Separo horas de Hoy en Matriz Auxiliar [0m
			// [7m Asigno de menor a mayor tipos de horas [0m
			Asignar(&distrib[HSNORM], &aux_distrib[HSNORM], &canhshoy);
			Asignar(&distrib[HSAL25], &aux_distrib[HSAL25], &canhshoy);
			Asignar(&distrib[HSAL35], &aux_distrib[HSAL35], &canhshoy);

			if (p_msj) fprintf (stderr, "\nSEPARO EN MATRIZ AUXILIAR LAS HORAS DE HOY\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);


			// [7m Separo horas Nocturnas de Matriz auxiliar a la principal [0m
			// [7m Asigno de mayor a menor tipos de horas [0m
			Asignar(&aux_distrib[HSAL35], &distrib[HS35NO], &canhsnoct);
			Asignar(&aux_distrib[HSAL25], &distrib[HS25NO], &canhsnoct);
			Asignar(&aux_distrib[HSNORM], &distrib[HSNONO], &canhsnoct);

			if (p_msj) fprintf (stderr, "\nDESPUES DE DESCONTAR LAS DE HOY\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);


			// [7m Paso horas Diurnas de Matriz auxiliar a la principal [0m
			for(v_j=0; v_j<MAXTIPHOR; v_j++) {
					switch(v_j) {
						case HSNORM :
						case HSAL25 :
						case HSAL35 :
							distrib[v_j]+=aux_distrib[v_j];
							aux_distrib[v_j]=0;
							break;
					}
			} 
			if (p_msj) fprintf (stderr, "\nDESPUES DE PASAR TODAS LAS HORAS DIURNAS A LA MATRIZ PRINCIPAL\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);
		}
		
		if (!p_es_feriado_manana) {
			// [7m    MAÑANA     [0m   

			// [7m Calculo Horas de Mañana [0m
			canhshoy= (int)(ConvHraInt(StrToT("00:00:00"), p_horsal)*100);

			// [7m Calculo las Horas diurnas y nocturnas de Mañana [0m
			canhsdia=0;
			if (StrToT(HOR_INI_DIA)<p_horsal)
				canhsdia=(int)(ConvHraInt(StrToT(HOR_INI_DIA), p_horsal)*100);
			canhsnoct= canhshoy - canhsdia;  

			if (p_msj) fprintf (stderr, "Hoy Hs MAÑANA           = %.2f\n", canhshoy);
			if (p_msj) fprintf (stderr, "Hs Diurnas mañana       = %.2f\n", canhsdia);
			if (p_msj) fprintf (stderr, "Hoy Hs Nocturnas Mañana = %.2f\n", canhsnoct);

			// [7m Separo horas de Hoy en Matriz Auxiliar [0m
			// [7m Asigno de mayor a menor tipos de horas [0m
			Asignar(&distrib[HSAL35], &aux_distrib[HSAL35], &canhshoy);
			Asignar(&distrib[HSAL25], &aux_distrib[HSAL25], &canhshoy);
			Asignar(&distrib[HSNORM], &aux_distrib[HSNORM], &canhshoy);

			if (p_msj) fprintf (stderr, "\nDESPUES DE DESCONTAR LAS DE MAÑANA\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);

			// [7m Separo horas Nocturnas de Matriz auxiliar a la principal [0m
			// [7m Asigno de menor a mayor tipos de horas [0m
			Asignar(&aux_distrib[HSNORM], &distrib[HSNONO], &canhsnoct);
			Asignar(&aux_distrib[HSAL25], &distrib[HS25NO], &canhsnoct);
			Asignar(&aux_distrib[HSAL35], &distrib[HS35NO], &canhsnoct);

			if (p_msj) fprintf (stderr, "\nDESPUES DE PASAR LAS HORAS NOCTURNAS A LA MATRIZ PRINCIPAL\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);

			// [7m Paso horas Diurnas de Matriz auxiliar a la principal [0m
			for(v_j=0; v_j<MAXTIPHOR; v_j++) {
					switch(v_j) {
						case HSNORM :
						case HSAL25 :
						case HSAL35 :
							distrib[v_j]+=aux_distrib[v_j];
							aux_distrib[v_j]=0;
							break;
					}
			} 
			if (p_msj) fprintf (stderr, "\nDESPUES DE CONSOLIDAR HORAS\n");
			ImprimeHoras(p_msj, distrib, aux_distrib);
		}	
	}

}         

private void ImprimeHoras(bool p_msj, double* distrib, double* aux_distrib)
{

	if (p_msj) fprintf (stderr, "\nMATRIZ PRINCIPAL \n");
	if (p_msj) fprintf (stderr, "Horas normales                %.2f\n", distrib[HSNORM]);
	if (p_msj) fprintf (stderr, "Horas al 25%%                 %.2f\n", distrib[HSAL25]);
	if (p_msj) fprintf (stderr, "Horas al 35%%                 %.2f\n", distrib[HSAL35]);
	if (p_msj) fprintf (stderr, "Horas Franco                  %.2f\n", distrib[HSFRAN]);
	if (p_msj) fprintf (stderr, "Horas Feriado                 %.2f\n", distrib[HSFERI]);
	if (p_msj) fprintf (stderr, "Horas normales noct.          %.2f\n", distrib[HSNONO]);
	if (p_msj) fprintf (stderr, "Horas al 25%%  noct.          %.2f\n", distrib[HS25NO]);
	if (p_msj) fprintf (stderr, "Horas al 35%%  noct.          %.2f\n", distrib[HS35NO]);
	if (p_msj) fprintf (stderr, "Horas Pegadas.                %.2f\n", distrib[HSPEGA]);
	if (p_msj) fprintf (stderr, "Horas normales Feriado        %.2f\n", distrib[HSNOFE]);
	if (p_msj) fprintf (stderr, "Horas al 25%% Feriado         %.2f\n", distrib[HS25FE]);
	if (p_msj) fprintf (stderr, "Horas al 35%% Feriado         %.2f\n", distrib[HS35FE]);
	if (p_msj) fprintf (stderr, "Horas normales Feriado Noct.  %.2f\n", distrib[HNOFEN]);
	if (p_msj) fprintf (stderr, "Horas al 25%% Feriado  Noct.  %.2f\n", distrib[H25FEN]);
	if (p_msj) fprintf (stderr, "Horas al 35%% Feriado  Noct.  %.2f\n", distrib[H35FEN]);
	if (p_msj) fprintf (stderr, "Horas normales Franco         %.2f\n", distrib[HSNOFR]);
	if (p_msj) fprintf (stderr, "Horas al 25%% Franco          %.2f\n", distrib[HS25FR]);
	if (p_msj) fprintf (stderr, "Horas al 35%% Franco          %.2f\n", distrib[HS35FR]);
	if (p_msj) fprintf (stderr, "Horas normales Franco Noct.   %.2f\n", distrib[HNOFRN]);
	if (p_msj) fprintf (stderr, "Horas al 25%% Franco  Noct.   %.2f\n", distrib[H25FRN]);
	if (p_msj) fprintf (stderr, "Horas al 35%% Franco  Noct.   %.2f\n", distrib[H35FRN]);

	if (aux_distrib!=NULL){
		if (p_msj) fprintf (stderr, "\nMATRIZ AUXILIAR \n");
		if (p_msj) fprintf (stderr, "Horas normales                %.2f\n", aux_distrib[HSNORM]);
		if (p_msj) fprintf (stderr, "Horas al 25%%                 %.2f\n", aux_distrib[HSAL25]);
		if (p_msj) fprintf (stderr, "Horas al 35%%                 %.2f\n", aux_distrib[HSAL35]);
		if (p_msj) fprintf (stderr, "Horas Franco                  %.2f\n", aux_distrib[HSFRAN]);
		if (p_msj) fprintf (stderr, "Horas Feriado                 %.2f\n", aux_distrib[HSFERI]);
		if (p_msj) fprintf (stderr, "Horas normales noct.          %.2f\n", aux_distrib[HSNONO]);
		if (p_msj) fprintf (stderr, "Horas al 25%%  noct.          %.2f\n", aux_distrib[HS25NO]);
		if (p_msj) fprintf (stderr, "Horas al 35%%  noct.          %.2f\n", aux_distrib[HS35NO]);
		if (p_msj) fprintf (stderr, "Horas Pegadas.                %.2f\n", aux_distrib[HSPEGA]);
		if (p_msj) fprintf (stderr, "Horas normales Feriado        %.2f\n", aux_distrib[HSNOFE]);
		if (p_msj) fprintf (stderr, "Horas al 25%% Feriado         %.2f\n", aux_distrib[HS25FE]);
		if (p_msj) fprintf (stderr, "Horas al 35%% Feriado         %.2f\n", aux_distrib[HS35FE]);
		if (p_msj) fprintf (stderr, "Horas normales Feriado Noct.  %.2f\n", aux_distrib[HNOFEN]);
		if (p_msj) fprintf (stderr, "Horas al 25%% Feriado  Noct.  %.2f\n", aux_distrib[H25FEN]);
		if (p_msj) fprintf (stderr, "Horas al 35%% Feriado  Noct.  %.2f\n", aux_distrib[H35FEN]);
		if (p_msj) fprintf (stderr, "Horas normales Franco         %.2f\n", aux_distrib[HSNOFR]);
		if (p_msj) fprintf (stderr, "Horas al 25%% Franco          %.2f\n", aux_distrib[HS25FR]);
		if (p_msj) fprintf (stderr, "Horas al 35%% Franco          %.2f\n", aux_distrib[HS35FR]);
		if (p_msj) fprintf (stderr, "Horas normales Franco Noct.   %.2f\n", aux_distrib[HNOFRN]);
		if (p_msj) fprintf (stderr, "Horas al 25%% Franco  Noct.   %.2f\n", aux_distrib[H25FRN]);
		if (p_msj) fprintf (stderr, "Horas al 35%% Franco  Noct.   %.2f\n", aux_distrib[H35FRN]);
	}
	if (p_msj) fprintf (stderr, "\n------------------------------------------------------------------- \n");
	
}
void Asignar(double* p_origen, double* p_destino ,double* p_tope)
{

	double v_tope	= *p_tope,
		   v_origen	= *p_origen,
		   v_destino= *p_destino;

	//fprintf(stderr, "PRIMERO v_tope: %.2f v_origen: %.2f v_destino: %.2f\n", v_tope, v_origen, v_destino );

	if (v_tope>0) {
		if (v_origen>=v_tope) {
			v_destino += v_tope;
			v_origen  -= v_tope;
			v_tope	   = 0;
		}
		else {
			v_destino += v_origen;
			v_tope    -= v_origen;
			v_origen   = 0; 
		}
	}

	*p_origen  = v_origen;
	*p_destino = v_destino;
	*p_tope    = v_tope;

	//fprintf(stderr, "SEGUNDO v_tope: %.2f v_origen: %.2f v_destino: %.2f\n", v_tope, v_origen, v_destino );
}


bool LegajoActivo(int p_emp, long p_nroleg)
{
	schema sue, old;

	old = CurrentSchema();
	sue = OpenSchema("sue", IO_EABORT);
	SwitchToSchema(old);

	SetKey(sue|PERbyEMP, p_emp, p_nroleg);
	if(GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
//		if(!IsNull(sue|PER_FECEGR))
		if(IFld(sue|PER_ACTIVO) == 0)
			return FALSE;
	}
	return TRUE;
}


tnempres AcuNEmpres(tnempres nodop, tnempres * nantp)
{
	int i=0;
	tnempres naux;

	if (nodop == NULL) {
		nodop = (tnempres) malloc (sizeof(stnempres));

		(*nodop).empres = empres;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).diasnoct = 0;
		(*nodop).diasdiur = 0;
		(*nodop).canpeg   = canpeg;

		(*nodop).nnodleg  = NULL;

		if (NivCon > 0)
			(*nodop).nnodleg = AcuNNodleg(NULL, NULL);

		(*nodop).diasnoct += diasnoct;
		(*nodop).diasdiur += diasdiur;
//fprintf(stderr, "c %d %d \n", diasnoct, diasdiur);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).empres == empres) {
			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

			if (NivCon > 0)
				(*nodop).nnodleg = AcuNNodleg((*nodop).nnodleg, &(*nodop).nnodleg);

			(*nodop).diasnoct += diasnoct;
			(*nodop).diasdiur += diasdiur;
//fprintf(stderr, "d %d %d \n", diasnoct, diasdiur);

		}
		else {
			if ((*nodop).empres < empres)
				(*nodop).nsig = AcuNEmpres((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnempres) malloc (sizeof(stnempres));
				(*nodop).empres = empres;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

				(*nodop).canpeg   = canpeg;
//				(*nodop).diasause = diasause;
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).nnodleg  = NULL;

				(*nodop).diasnoct = 0;
				(*nodop).diasdiur = 0;
//fprintf(stderr, "e %d %d \n", diasnoct, diasdiur);

				if (NivCon > 0)
					(*nodop).nnodleg = AcuNNodleg(NULL, NULL);

				(*nodop).diasnoct += diasnoct;
				(*nodop).diasdiur += diasdiur;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

tnnodleg AcuNNodleg(tnnodleg nodop, tnnodleg * nantp)
{
	int i=0;
	tnnodleg naux;

	if (nodop == NULL) {
		nodop = (tnnodleg) malloc (sizeof(stnnodleg));

		(*nodop).nodleg = nodleg;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg   = canpeg;

		(*nodop).nnodclie = NULL;

		(*nodop).diasnoct = 0;
		(*nodop).diasdiur = 0;
//fprintf(stderr, "f %d %d \n", diasnoct, diasdiur);

		if (NivCon > 1)
			(*nodop).nnodclie = AcuNNodclie(NULL, NULL);

		(*nodop).diasnoct += diasnoct;
		(*nodop).diasdiur += diasdiur;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nodleg == nodleg) {
			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

			if (NivCon > 1)
				(*nodop).nnodclie = AcuNNodclie((*nodop).nnodclie, &(*nodop).nnodclie);

			(*nodop).diasnoct += diasnoct;
			(*nodop).diasdiur += diasdiur;
//fprintf(stderr, "g %d %d \n", diasnoct, diasdiur);
		}
		else {
			if ((*nodop).nodleg < nodleg)
				(*nodop).nsig = AcuNNodleg((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnodleg) malloc (sizeof(stnnodleg));
				(*nodop).nodleg = nodleg;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

//				(*nodop).diasause = diasause;
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).nnodclie = NULL;

				(*nodop).diasnoct = 0;
				(*nodop).diasdiur = 0;
//fprintf(stderr, "h %d %d \n", diasnoct, diasdiur);

				if (NivCon > 1)
					(*nodop).nnodclie = AcuNNodclie(NULL, NULL);

				(*nodop).diasnoct += diasnoct;
				(*nodop).diasdiur += diasdiur;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

tnnodclie AcuNNodclie(tnnodclie nodop, tnnodclie * nantp)
{
	int i=0;
	tnnodclie naux;

	if (nodop == NULL) {
		nodop = (tnnodclie) malloc (sizeof(stnnodclie));

		(*nodop).nodclie = nodclie;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg   = canpeg;

   		(*nodop).nobjnod  = NULL;

		(*nodop).diasnoct = 0;
		(*nodop).diasdiur = 0;
//fprintf(stderr, "i %d %d \n", diasnoct, diasdiur);

		if (NivCon > 2)
			(*nodop).nobjnod = AcuNObjnod(NULL, NULL);

		(*nodop).diasnoct += diasnoct;
		(*nodop).diasdiur += diasdiur;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nodclie == nodclie) {
			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

			if (NivCon > 2)
				(*nodop).nobjnod = AcuNObjnod((*nodop).nobjnod, &(*nodop).nobjnod);

			(*nodop).diasnoct += diasnoct;
			(*nodop).diasdiur += diasdiur;
//fprintf(stderr, "j %d %d \n", diasnoct, diasdiur);
		}
		else {
			if ((*nodop).nodclie < nodclie)
				(*nodop).nsig = AcuNNodclie((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnodclie) malloc (sizeof(stnnodclie));
				(*nodop).nodclie = nodclie;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

//				(*nodop).diasnoct += diasnoct;
//				(*nodop).diasdiur += diasdiur;

//fprintf(stderr, "k %d %d \n", diasnoct, diasdiur);
//				(*nodop).diasause = diasause;
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).nobjnod  = NULL;

				(*nodop).diasnoct = 0;
				(*nodop).diasdiur = 0;

				if (NivCon > 2)
					(*nodop).nobjnod = AcuNObjnod(NULL, NULL);

				(*nodop).diasnoct += diasnoct;
				(*nodop).diasdiur += diasdiur;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

tnobjnod AcuNObjnod(tnobjnod nodop, tnobjnod * nantp)
{
	tnobjnod naux;
	int i=0;

	if (nodop == NULL) {
		nodop = (tnobjnod) malloc (sizeof(stnobjnod));

		(*nodop).objnod = objnod;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg   = canpeg;
		(*nodop).npuenod  = NULL;

		(*nodop).diasnoct = 0;
		(*nodop).diasdiur = 0;
//fprintf(stderr, "l %d %d \n", diasnoct, diasdiur);

		if (NivCon > 3)
			(*nodop).npuenod = AcuNPuenod(NULL, NULL);

		(*nodop).diasnoct += diasnoct;
		(*nodop).diasdiur += diasdiur;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).objnod == objnod) {
			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

			if (NivCon > 3)
				(*nodop).npuenod = AcuNPuenod((*nodop).npuenod, &(*nodop).npuenod);

			(*nodop).diasnoct += diasnoct;
			(*nodop).diasdiur += diasdiur;

//fprintf(stderr, "m %d %d \n", diasnoct, diasdiur);

		}
		else {
			if ((*nodop).objnod < objnod)
				(*nodop).nsig = AcuNObjnod((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnobjnod) malloc (sizeof(stnobjnod));
				(*nodop).objnod = objnod;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

//				(*nodop).diasause = diasause;
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).npuenod  = NULL;

				(*nodop).diasnoct = 0;
				(*nodop).diasdiur = 0;
//fprintf(stderr, "n %d %d \n", diasnoct, diasdiur);

				if (NivCon > 3)
					(*nodop).npuenod = AcuNPuenod(NULL, NULL);

				(*nodop).diasnoct += diasnoct;
				(*nodop).diasdiur += diasdiur;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}
tnpuenod AcuNPuenod(tnpuenod nodop, tnpuenod * nantp)
{
	tnpuenod naux;
	int i=0;

	if (nodop == NULL) {
		nodop = (tnpuenod) malloc (sizeof(stnpuenod));

		sprintf((*nodop).puenod,"%s", puenod);

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg   = canpeg;

		(*nodop).ndianod  = NULL;

  		(*nodop).n_ptoser = n_ptoser;
  		(*nodop).n_puesto = n_puesto;
  		(*nodop).n_codint = n_codint;
		(*nodop).n_horini = n_horini;
		(*nodop).n_horfin = n_horfin;
		(*nodop).n_dia1   = n_dia1;
		(*nodop).n_dia2   = n_dia2;
		(*nodop).n_dia3   = n_dia3;
		(*nodop).n_dia4   = n_dia4;
		(*nodop).n_dia5   = n_dia5;
		(*nodop).n_dia6   = n_dia6;
		(*nodop).n_dia7   = n_dia7;
		sprintf((*nodop).n_regim, "%s", n_regim);
		(*nodop).n_canvig = n_canvig;
		(*nodop).n_canpto = n_canpto;
		(*nodop).n_tipdia = n_tipdia;
		(*nodop).n_fecini = n_fecini;
		(*nodop).n_fecfin = n_fecfin;

		(*nodop).diasnoct=0;
		(*nodop).diasdiur=0;
//fprintf(stderr, "o %d %d \n", diasnoct, diasdiur);
		if (NivCon > 4)
			(*nodop).ndianod = AcuNDianod(NULL, NULL);

		(*nodop).diasnoct += diasnoct;
		(*nodop).diasdiur += diasdiur;

		(*nodop).nsig = NULL;
	}
	else {
		if (strcmp((*nodop).puenod, puenod)==0) {
			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

	  		(*nodop).n_ptoser = n_ptoser;
	  		(*nodop).n_puesto = n_puesto;
	  		(*nodop).n_codint = n_codint;
			(*nodop).n_horini = n_horini;
			(*nodop).n_horfin = n_horfin;
			(*nodop).n_dia1   = n_dia1;
			(*nodop).n_dia2   = n_dia2;
			(*nodop).n_dia3   = n_dia3;
			(*nodop).n_dia4   = n_dia4;
			(*nodop).n_dia5   = n_dia5;
			(*nodop).n_dia6   = n_dia6;
			(*nodop).n_dia7   = n_dia7;
			sprintf((*nodop).n_regim, "%s", n_regim);
			(*nodop).n_canvig = n_canvig;
			(*nodop).n_canpto = n_canpto;
			(*nodop).n_tipdia = n_tipdia;
			(*nodop).n_fecini = n_fecini;
			(*nodop).n_fecfin = n_fecfin;

			if (NivCon > 4)
				(*nodop).ndianod = AcuNDianod((*nodop).ndianod, &(*nodop).ndianod);

			(*nodop).diasnoct += diasnoct;
//fprintf(stderr, "p %d %d \n", diasnoct, diasdiur);
			(*nodop).diasdiur += diasdiur;


		}
		else {
			if (strcmp((*nodop).puenod, puenod) < 0)
				(*nodop).nsig = AcuNPuenod((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnpuenod) malloc (sizeof(stnpuenod));
				sprintf((*nodop).puenod,"%s", puenod);

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

//				(*nodop).diasause = diasause;
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).ndianod  = NULL;

		  		(*nodop).n_ptoser = n_ptoser;
		  		(*nodop).n_puesto = n_puesto;
		  		(*nodop).n_codint = n_codint;
				(*nodop).n_horini = n_horini;
				(*nodop).n_horfin = n_horfin;
				(*nodop).n_dia1   = n_dia1;
				(*nodop).n_dia2   = n_dia2;
				(*nodop).n_dia3   = n_dia3;
				(*nodop).n_dia4   = n_dia4;
				(*nodop).n_dia5   = n_dia5;
				(*nodop).n_dia6   = n_dia6;
				(*nodop).n_dia7   = n_dia7;
				sprintf((*nodop).n_regim, "%s", n_regim);
				(*nodop).n_canvig = n_canvig;
				(*nodop).n_canpto = n_canpto;
				(*nodop).n_tipdia = n_tipdia;
				(*nodop).n_fecini = n_fecini;
				(*nodop).n_fecfin = n_fecfin;

				(*nodop).diasnoct = 0;
//fprintf(stderr, "q %d %d \n", diasnoct, diasdiur);
				(*nodop).diasdiur = 0;

				if (NivCon > 4)
					(*nodop).ndianod = AcuNDianod(NULL, NULL);

				(*nodop).diasnoct += diasnoct;
				(*nodop).diasdiur += diasdiur;

				(*nodop).nsig = naux;

				*nantp = nodop;
			}
		}
	}

	return nodop;
}

tndianod AcuNDianod(tndianod nodop, tndianod * nantp)
{
	int i=0;
	tndianod naux;


	if (nodop == NULL) {
		nodop = (tndianod) malloc (sizeof(stndianod));

		(*nodop).dianod = dianod;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

		// Determina si es nocturna la jornada 

		(*nodop).diasnoct = diasnoct;
//fprintf(stderr, "r %d %d \n", diasnoct, diasdiur);
		(*nodop).diasdiur = diasdiur;
//fprintf(stderr, "a guarda %d cliente %d\n", diasdiur, nodclie);

//		(*nodop).diasause = diasause;
//fprintf(stderr, "F diasfran %d %d\n", (*nodop).diasfran, diasfran);
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg   = canpeg;

		(*nodop).nhordes  = NULL;

		if (NivCon > 5)
			(*nodop).nhordes = AcuNHordes(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).dianod == dianod) {


			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//fprintf(stderr, "E diasfran %d %d\n",(*nodop).diasfran, diasfran);
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel += diasadel;
			(*nodop).canpeg   += canpeg;

			// Determina si es nocturna la jornada y solo acumula si no lo hizo antes

			(*nodop).diasnoct += diasnoct;
//fprintf(stderr, "s %d %d \n", diasnoct, diasdiur);
			(*nodop).diasdiur += diasdiur;

			if ((*nodop).diasnoct > 1) {
				(*nodop).diasnoct = 1;
				diasnoct = 0;
			}
			if ((*nodop).diasdiur > 1) {
//fprintf(stderr, "guarda %d cliente %d y pone en cero\n", 1, nodclie);
				(*nodop).diasdiur = 1;
				diasdiur = 0;
			}
			
			if (NivCon > 5)
				(*nodop).nhordes = AcuNHordes((*nodop).nhordes, &(*nodop).nhordes);
		}
		else {
			if ((*nodop).dianod < dianod)
				(*nodop).nsig = AcuNDianod((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tndianod) malloc (sizeof(stndianod));
				(*nodop).dianod = dianod;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

				// Determina si es nocturna la jornada 

				(*nodop).diasnoct=diasnoct;
				(*nodop).diasdiur=diasdiur;
//fprintf(stderr, "b guarda %d cliente %d\n", diasdiur, nodclie);
	
//				(*nodop).diasause = diasause;
//fprintf(stderr, "D diasfran %d %d\n", (*nodop).diasfran, diasfran);
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).nhordes  = NULL;

				if (NivCon > 5)
					(*nodop).nhordes = AcuNHordes(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}



	return nodop;
}

tnhordes AcuNHordes(tnhordes nodop, tnhordes * nantp)
{
	tnhordes naux;
	int i=0;

	if (nodop == NULL) {
		nodop = (tnhordes) malloc (sizeof(stnhordes));

		(*nodop).hordes = hordes;
		(*nodop).horhas = horhas;
		(*nodop).condic = condic;
		(*nodop).codnov = codnov;

		for (i=0 ; i<MAXTIPHOR ; i++)
			(*nodop).cantih[i] = cantih[i];

//		(*nodop).diasause = diasause;
//fprintf(stderr, "C diasfran %d %d\n", (*nodop).diasfran, diasfran);
//		(*nodop).diasfran = diasfran;
//		(*nodop).diasfrat = diasfrat;
//		(*nodop).diasvaca = diasvaca;
//		(*nodop).diasvact = diasvact;
		(*nodop).diasadel = diasadel;
		(*nodop).canpeg = canpeg;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).hordes == hordes) {
			(*nodop).horhas = horhas;

			for (i=0 ; i<MAXTIPHOR ; i++)
				(*nodop).cantih[i] += cantih[i];

//			(*nodop).diasause += diasause;
//fprintf(stderr, "B diasfran %d %d\n", (*nodop).diasfran, diasfran);
//			(*nodop).diasfran += diasfran;
//			(*nodop).diasfrat += diasfrat;
//			(*nodop).diasvaca += diasvaca;
//			(*nodop).diasvact += diasvact;
			(*nodop).diasadel  += diasadel;
			(*nodop).canpeg   += canpeg;

		}
		else {
			if ((*nodop).hordes < hordes)
				(*nodop).nsig = AcuNHordes((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnhordes) malloc (sizeof(stnhordes));

				(*nodop).hordes = hordes;
				(*nodop).horhas = horhas;
				(*nodop).condic = condic;
				(*nodop).codnov = codnov;

				for (i=0 ; i<MAXTIPHOR ; i++)
					(*nodop).cantih[i] = cantih[i];

//				(*nodop).diasause = diasause;
//fprintf(stderr, "A diasfran %d %d\n", (*nodop).diasfran, diasfran);
//				(*nodop).diasfran = diasfran;
//				(*nodop).diasfrat = diasfrat;
//				(*nodop).diasvaca = diasvaca;
//				(*nodop).diasvact = diasvact;
				(*nodop).diasadel = diasadel;
				(*nodop).canpeg   = canpeg;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

void BorNEmpres(tnempres nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnodleg != NULL)
		BorNNodleg((*nodop).nnodleg);

	if ((*nodop).nsig != NULL)
		BorNEmpres((*nodop).nsig);

	(*nodop).nnodleg = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNNodleg(tnnodleg nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnodclie != NULL)
		BorNNodclie((*nodop).nnodclie);

	if ((*nodop).nsig != NULL)
		BorNNodleg((*nodop).nsig);

	(*nodop).nnodclie = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNNodclie(tnnodclie nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nobjnod != NULL)
		BorNObjnod((*nodop).nobjnod);

	if ((*nodop).nsig != NULL)
		BorNNodclie((*nodop).nsig);

	(*nodop).nobjnod = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNObjnod(tnobjnod nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).npuenod != NULL)
		BorNPuenod((*nodop).npuenod);

	if ((*nodop).nsig != NULL)
		BorNObjnod((*nodop).nsig);

	(*nodop).npuenod = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNPuenod(tnpuenod nodop)
{
	if (nodop == NULL)
		return;


	if ((*nodop).ndianod != NULL)
		BorNDianod((*nodop).ndianod);

	if ((*nodop).nsig != NULL)
		BorNPuenod((*nodop).nsig);

	(*nodop).ndianod = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNDianod(tndianod nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nhordes != NULL)
		BorNHordes((*nodop).nhordes);

	if ((*nodop).nsig != NULL)
		BorNDianod((*nodop).nsig);

	(*nodop).nhordes = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}


void BorNHordes(tnhordes nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNHordes((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}

tnlegajonoc AcuNLegajonoc(tnlegajonoc nodop, tnlegajonoc * nantp)
{
	tnlegajonoc naux;

	if (nodop == NULL) {
		nodop = (tnlegajonoc) malloc (sizeof(stnlegajonoc));
		(*nodop).legajonoc = legajonoc;
		(*nodop).nfechanoc = AcuNFechanoc(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).legajonoc == legajonoc) {
			(*nodop).nfechanoc = AcuNFechanoc((*nodop).nfechanoc, &(*nodop).nfechanoc);
		}
		else {
			if ((*nodop).legajonoc < legajonoc)
				(*nodop).nsig = AcuNLegajonoc((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnlegajonoc) malloc (sizeof(stnlegajonoc));
				(*nodop).legajonoc = legajonoc;

				(*nodop).nfechanoc = AcuNFechanoc(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

tnfechanoc AcuNFechanoc(tnfechanoc nodop, tnfechanoc * nantp)
{
	tnfechanoc naux;

	if (nodop == NULL) {
		nodop = (tnfechanoc) malloc (sizeof(stnfechanoc));
		(*nodop).fechanoc = fechanoc;
		(*nodop).nsig = NULL;
		yacargo=FALSE;
	}
	else {
		if ((*nodop).fechanoc == fechanoc) {
			yacargo=TRUE;
		}
		else {
			if ((*nodop).fechanoc < fechanoc)
				(*nodop).nsig = AcuNFechanoc((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnfechanoc) malloc (sizeof(stnfechanoc));
				(*nodop).fechanoc = fechanoc;
				yacargo=FALSE;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}


void BorNLegajonoc(tnlegajonoc nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nfechanoc != NULL)
		BorNFechanoc((*nodop).nfechanoc);

	if ((*nodop).nsig != NULL)
		BorNLegajonoc((*nodop).nsig);

	(*nodop).nfechanoc = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

void BorNFechanoc(tnfechanoc nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNFechanoc((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}


void LisNLegajonoc(tnlegajonoc nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).legajonoc == legajonoc) {
		if ((*nodop).nfechanoc != NULL)
			LisNFechanoc((*nodop).nfechanoc);
	}
	else {
		if ((*nodop).nsig != NULL)
			LisNLegajonoc((*nodop).nsig);
	}
}

void LisNFechanoc(tnfechanoc nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).fechanoc == fechanoc) {
		yacargo=TRUE;
	}
	else {
		if ((*nodop).nsig != NULL)
			LisNFechanoc((*nodop).nsig);
	}
}

/*

	GetTipoLiquidacionByNroLiq
	Parametros
		p_emp = Numero de empresa
		p_liqui = Numero de liquidacion

    Importante:
	De la posicion 0 a la 3 (los primeros 4 elemnetos) es el año
	De la posicion 4 a la 5 es el mes del periodo 1 a 12
	De la posicion 6 a la 7 es el código de tipo de cierre
*/

short GetTipoLiquidacionByNroLiq(short p_emp, long p_liqui)
{
	short respuesta = 0;
	schema old, oper;
	char strLiquidacion[10];
	char strAnio[5];
	char strTipcie[3];
	char strNumPer[3];
	int v_anio = 0;
	int v_numero_periodo = 0;
	int v_tipo_cierre = 0;
	
	//fprintf(stderr, "Funcion GetTipoLiquidacionByNroLiq();\n");

	old = CurrentSchema();
	oper = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);	
	
	strcpy(strLiquidacion, NULL_STR);
	sprintf(strLiquidacion, "%ld",p_liqui);
	
	//fprintf(stderr, "GetTipoLiquidacionByNroLiq - strLiquidacion: %s - %d\n", strLiquidacion, strlen(strLiquidacion));
	
	if(strlen(strLiquidacion) != 8)
		Error("Esta mal conformada el número de la liquidación");

	//fprintf(stderr, "GetTipoLiquidacionByNroLiq - POST ERROR\n");
    strcpy(strAnio, NULL_STR);
    sprintf(strAnio, "%c%c%c%c", strLiquidacion[0], strLiquidacion[1], strLiquidacion[2], strLiquidacion[3]);
    
    //fprintf(stderr, "GetTipoLiquidacionByNroLiq - POST strAnio\n");
    
    strcpy(strNumPer, NULL_STR);
    sprintf(strNumPer,"%c%c", strLiquidacion[4], strLiquidacion[5]);

    //fprintf(stderr, "GetTipoLiquidacionByNroLiq - POST strNumPer\n");

    strcpy(strTipcie, NULL_STR);
    sprintf(strTipcie,"%c%c", strLiquidacion[6], strLiquidacion[7]);
    
    v_anio = StrToI(strAnio);
    v_numero_periodo = StrToI(strNumPer);
    v_tipo_cierre = StrToI(strTipcie);
    
    //fprintf(stderr, "Empresa  %d anio %d numero per %d tipo_cierre %d\n", p_emp, v_anio, v_numero_periodo, v_tipo_cierre);
    
	//primary key (emp, anoper, tipcie, numper)
	SetKey(oper|PERIODObyEMP, p_emp, v_anio, v_numero_periodo, v_tipo_cierre);
	if(GetRecord(oper|PERIODObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
	{
		respuesta = IFld(oper|PERIODO_TIPCIE);
	}
	
	return respuesta;
}
