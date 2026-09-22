#include <ideafix.h>
#include "operac.sch"
#include "comerc.sch"
#include "sue.sch"
#include "comgral.sch"
#include "comerc.h"
#include "operac.h"
#include "disthspro.h"

/************************************************************************************************************
*                                    CalculaDetalleHorasPro
*
* Parametros : 
*
*	bool	p_usafm   :	Esto determina si se deven calcular las horas de otro puesto en pantalla en ese momento, 
*			         	esta opcion la usa el parte  
*	form 	p_fm      : Es el descriptor del fm si es que se usa(parametro p_usafm=TRUE),si no deve ser NULL 
*	int 	p_row     :	Es el numero de linea del multi si es que se usa(parametro p_usafm=TRUE),si no deve ser
*						NULL_SHORT 
*	int 	p_emp     :	Codigo de empresa
*	long	p_cliente : Codigo de cliente
*	int		p_objetivo: codigo de ojetivo
*	long	p_nroleg  : Numero de legajo 
*	DATE	p_fecha   : Fecha del Parte
*	TIME	p_hora_ent: Horario de entrada del vigilador
*	TIME	p_hora_sal: Horario de salida del vigilador
*   int		p_pais    : Codigo de pais (para calcular feriados)
*   int		p_prov    : Codigo de provincia (para calcular feriados)
*	char*	p_tipvig  :	Tipo de Vigilador 'E' o 'P' (para calcular Franco)
*
* Devuelve : 
*	Distribuye las horas trabajadas en los tipos de horas espesicicadas a continuacion:
*
*		int* p_hs_agrego_nor	: Horas normales
*		int* p_hs_agrego_50     : Horas al 50%
*		int* p_hs_agrego_100fr	: Horas al 100% Franco
*		int *p_hs_agrego_100fe	: Horas al 100% Feriado
*
* Comentarios : 
*	- Si el la fecha del parte cae Franco, todas las horas corresponderan a Horas al 100% Franco, auque 
*     halla horas fuera de esta fecha de franco
*   - Las horas al 100% feriado son solo las del dia feriado.
*	- Las horas que no son ni franco, ni feriado seran distribuidas entre horas normales y horas al 50% en
*     caso de superar la cantidad establesida por el regimen
*   - Dia del vigilador: Las horas que caigan en este dia y sean al 50% se convertiran en horas 100% Franco
*	- si el vigilador es PartTime todas las horas seran normales excepto la de los feriados (100 % Fe)
*
*  Orden de prioridad de atributos (De lo mas importante a lo mnos importante): 
*
*	- Horas al 100% Franco
*	- Horas al 100% Feriado
*	- Dia del Vigilador
*	- Horas al 50% 
*   - Horas Normales
*
************************************************************************************************************/
void CalculaDetalleHorasPro(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,
                            long p_nroleg, DATE p_fecha, TIME p_hora_ent, TIME p_hora_sal, int p_pais,
                            int p_prov, char *p_tipvig,  int  *p_hs_agrego_nor, int *p_hs_agrego_50,
                            int *p_hs_agrego_100fr, int *p_hs_agrego_100fe)
{

	double	v_hs_nor               = 0,
			v_hs_50                = 0,
			v_hs_100fr             = 0,
			v_hs_100fe             = 0, 
			v_hs_agrego_nor        = 0,
			v_hs_agrego_50         = 0,
			v_hs_agrego_100fr      = 0,
			v_hs_agrego_100fe      = 0;

	/* Sumo horas base de datos */
	CalcOtrasHoras(p_emp, p_nroleg, p_fecha, p_cliente, p_objetivo, NULL_LONG, NULL_SHORT, NULL_SHORT, 
		               NULL_SHORT, NULL_SHORT,&v_hs_nor, &v_hs_50, &v_hs_100fr, &v_hs_100fe);

	/* Sumo horas de formulario si corresponde */
	if (p_usafm)
		CalcHorasMismoCliente(p_fm, p_row, &v_hs_nor, &v_hs_50, &v_hs_100fr, &v_hs_100fe);


	DistribuyeHoras(p_emp, p_nroleg, p_fecha, p_tipvig, p_pais, p_prov, p_hora_ent, p_hora_sal, 
	                v_hs_nor, &v_hs_agrego_nor, &v_hs_agrego_50, &v_hs_agrego_100fr, &v_hs_agrego_100fe);

    
    	// Asigno lo calculado a los parametros de salida
	*p_hs_agrego_nor   = v_hs_agrego_nor   * 100;
    *p_hs_agrego_50    = v_hs_agrego_50    * 100;
    *p_hs_agrego_100fr = v_hs_agrego_100fr * 100;
    *p_hs_agrego_100fe = v_hs_agrego_100fe * 100;

	return;
}

/************************************************************************************************************
*                                         CalcOtrasHoras
*
* Parametros :
*
*	int   p_emp		: Codigo de empresa
*	long  p_nroleg	: Numero de legajo 
*	DATE  p_fecparte: Fecha del Parte
*	long  p_cliente	: Codigo de cliente que no se suma(se puede pasar como NULL_LONG)
*	int   p_obj     : Codigo de objetivo que no se suma(se puede pasar como NULL_SHORT)
*	long  p_cliente2: Otro codigo de cliente que no se suma (se puede pasar como NULL_LONG)
*	int   p_obj2    : Otro codigo de objetivo que no se suma, para usar con p_cliente2(se puede pasar como 
*                     NULL_SHORT)
*	int   p_ptoser2 : Tipo de puesto que no se suma, para usar con p_cliente2(se puede pasar como NULL_SHORT)
*   int   p_puesto2 : Numero de puesto que no se suma, para usar con p_cliente2(se puede pasar como 
*                     NULL_SHORT) 
*   int   p_nroint2 : Numero de linea del puesto que no se suma, para usar con p_cliente2(se puede pasar como
*                     NULL_SHORT)
*
* Devuelve : 
* 	Recorre la tabla Operac.Parte y si encuentra la horas en otro cliente - Objetivo, devuelve TRUE y la suma 
*   de las horas por tipo en los siguietes parametros :
*
*	double *p_hs_nor    : Horas normales
*	double *p_hs_50     : Horas al 50%
*	double *p_hs_100fr  : Horas al 100% Franco
*	double *p_hs_100fe  : Horas al 100% Feriado
*
*   Si el parametro p_usafm es FALSE lee todos los registros de Operac.Parte para ese legajo 
*
************************************************************************************************************/
bool CalcOtrasHoras(int p_emp, long p_nroleg, DATE p_fecparte, long p_cliente, int p_obj, long p_cliente2,
                    int p_obj2, int p_ptoser2, int p_puesto2, int p_nroint2, double *p_hs_nor, 
                    double *p_hs_50, double *p_hs_100fr, double *p_hs_100fe)
{
	schema   v_prev, v_operac;
	dbcursor c_parte;

	v_prev   = CurrentSchema();
	v_operac = OpenSchema("operac", IO_EABORT);

	PushRecord(v_operac|PARTE);
   
	c_parte  = CreateCursor(v_operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, p_emp, p_nroleg, p_fecparte, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, p_emp, p_nroleg, p_fecparte, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {

		// Esquivo cliente objetivo entero
		if (p_cliente != NULL_LONG && p_obj != NULL_SHORT)
			if (LFld(v_operac|PARTE_CLIENTE) == p_cliente)
				if (IFld(v_operac|PARTE_OBJETIVO) == p_obj) 
					continue;

		if (p_cliente2 != NULL_LONG && p_obj2 != NULL_SHORT && p_ptoser2 != NULL_SHORT &&
		    p_puesto2 != NULL_SHORT && p_nroint2 != NULL_SHORT)
			if (LFld(v_operac|PARTE_CLIENTE) == p_cliente2)
				if (IFld(v_operac|PARTE_OBJETIVO) == p_obj2) 
					if (IFld(v_operac|PARTE_PTOSER) == p_ptoser2) 
						if (IFld(v_operac|PARTE_PUESTO) == p_puesto2) 
							if (IFld(v_operac|PARTE_NROINT) == p_nroint2) 
								continue;

		*p_hs_nor   += (double)IFld(v_operac|PARTE_HSNOR)/100;
		*p_hs_50    += (double)IFld(v_operac|PARTE_HS50)/100;
		*p_hs_100fr += (double)IFld(v_operac|PARTE_HS100F)/100;
		*p_hs_100fe += (double)IFld(v_operac|PARTE_HS100FE)/100;
	}
	DeleteCursor(c_parte);

	SwitchToSchema(v_prev);

	if (RecordStackSize(v_operac|PARTE)>0)
		PopRecord(v_operac|PARTE);

	return (*p_hs_nor + *p_hs_50 + *p_hs_100fr + *p_hs_100fe)>0;
}
/***********************************************************************************************************
*                                     CalcHorasMismoCliente
*
*	
*	form p_fm    : Es el descriptor del fm si es que se usa(parametro p_usafm=TRUE),si no deve ser NULL 
*	int  p_row   :	Es el numero de linea del multi si es que se usa(parametro p_usafm=TRUE),si no deve ser
*
* Devuelve : TRUE si hay otro registro del mismo legajo en em multi y ademas la suma de las horas por tipo
*           en los siguientes parametros
*
*	double *p_hs_nor    : Horas normales
*	double *p_hs_50     : Horas al 50%
*	double *p_hs_100fr  : Horas al 100% Franco
*	double *p_hs_100fe  : Horas al 100% Feriado
*
***********************************************************************************************************/
bool CalcHorasMismoCliente(form p_fm, int p_row, double *p_hs_nor,double *p_hs_50, double *p_hs_100fr, 
                           double *p_hs_100fe)
{
	int v_i = 0;
	form v_fm1 = NULL;
	bool v_encontro=FALSE;	


	for (v_i = 0; v_i < FmFldLen(p_fm, MULTIPAR) && !FmIsNull(p_fm, NROLEG, v_i); v_i++) {

		// Descarto otros legajos
		if (FmLFld(p_fm, NROLEG, p_row) != FmLFld(p_fm, NROLEG, v_i))
			continue;

		// Descarto misma linea
		if (v_i == p_row)
			continue;

		v_encontro=TRUE;
		v_fm1 = UseSubform(p_fm, DETHS, 0, v_i);


//		fprintf(stderr, "CalcHorasMismoCliente suma linea %d, Nor %d dob %d dym %d dymfe %d\n", v_i, FmIFld(v_fm1,  NORMAL), FmIFld(v_fm1, EXTRAS1), 
//		                                                                                             FmIFld(v_fm1, EXTRAS2), FmIFld(v_fm1, FRANCOS));

		*p_hs_nor   += (double)FmIFld(v_fm1,  NORMAL)/100;
		*p_hs_50    += (double)FmIFld(v_fm1, EXTRAS1)/100;
		*p_hs_100fe += (double)FmIFld(v_fm1, EXTRAS2)/100;
		*p_hs_100fr += (double)FmIFld(v_fm1, FRANCOS)/100;
		
	} 


	return v_encontro;
}
/**********************************************************************************************************
*                AbarcaUnDia
*
* Parametros : 
*
*             TIME p_hora_ent : Hora de entrada
*             TIME p_hora_sal : Hora de salida
*
* Devuelve :  Si el horario ingresado abarca mas de un dia
*
*             double *p_horas_hoy : Horas pertenecientes al primer dia 
*             double *p_horas_man : Horas pertenecientes al segundo dia 
*
***********************************************************************************************************/
bool AbarcaUnDia(TIME p_hora_ent, TIME p_hora_sal, double *p_horas_hoy, double *p_horas_man)
{
	bool v_devuelve;

	if (p_hora_ent==StrToT("00:00") && p_hora_sal==StrToT("00:00")){
		*p_horas_hoy=0;
		*p_horas_man=0;
		v_devuelve=TRUE;
	}
	else
		if (p_hora_ent < p_hora_sal) {
			*p_horas_hoy= ConvHraInt(p_hora_ent, p_hora_sal);
			*p_horas_man=0;
			v_devuelve=TRUE;
		}
		else {
			*p_horas_hoy= ConvHraInt(p_hora_ent, StrToT("23:59"));
			*p_horas_man= ConvHraInt(StrToT("00:00"), p_hora_sal);
			v_devuelve=FALSE;
		}
		
	return v_devuelve;
}
/************************************************************************************************************
*                                     ModifDiaVigilador
*
* Parametros : 
*
*             int    p_emp     = Empresa
*             DATE   p_fecha   = Fecha del parte
*             bool   p_un_dia  = El periodo abarca solo un dia?
*             double p_hs_hoy  = Hora (en general) del dia de hoy (fecha de arriba)
*
* Devuelve: 
*			  Redistribuye las horas si correspondiera por ser dia del vigilador
*
*             - Dia del vigilador: Las horas que caigan en este dia y sean al 50% se convertiran en 
*                                  horas 100% Franco
*
*             double* p_hs_agrego_nor  = Horas normales
*             double* p_hs_agrego_50   = Horas al 50 %
*             double* p_hs_agrego_100f = horas al 100% Feriado
*
************************************************************************************************************/
void ModifDiaVigilador(int p_emp, DATE p_fecha, bool p_un_dia, double p_hs_hoy, double p_hs_man, 
                       double p_tope_hs_normales, double* p_hs_agrego_nor,
                       double* p_hs_agrego_50, double* p_hs_agrego_100fr)
{
	bool   v_dia_vigilador_hoy   = DiaVigilador(p_fecha, NULL_SHORT, NULL_SHORT, p_emp), 
	       v_dia_vigilador_man   = DiaVigilador(p_fecha+1, NULL_SHORT, NULL_SHORT, p_emp);

	double v_difer_dia_vigilador = 0;


	if (v_dia_vigilador_hoy){
		if(p_un_dia) //si hoy es el dia del vigilador y periodo es de un dia paso todo lo del 50% al 100% franco
			v_difer_dia_vigilador = *p_hs_agrego_50;
		else  // si abarca mas de un dia paso lo que tengo al 50% hoy al 100% franco
			v_difer_dia_vigilador = *p_hs_agrego_nor >= p_hs_hoy? 0 : (p_hs_hoy - *p_hs_agrego_nor);


		*p_hs_agrego_100fr += v_difer_dia_vigilador;
		*p_hs_agrego_50    -= v_difer_dia_vigilador;

	}

	if (v_dia_vigilador_man && !p_un_dia) { // Si Mañana es dia del vigilador, y abarca mas de un dia
	                                        // pongo las hora de mañana al 50% al 100% franco

		v_difer_dia_vigilador = *p_hs_agrego_nor <= p_hs_hoy? p_hs_man : *p_hs_agrego_nor - p_tope_hs_normales;
		*p_hs_agrego_100fr += v_difer_dia_vigilador;
		*p_hs_agrego_50    -= v_difer_dia_vigilador;
	}
	
}
/************************************************************************************************************
*                                     ModifDistribNormal
*
* Parametros : 
*
*             double p_hs_faltan_agregar = Horas que quedan por asignar y no son feriado, ni franco
*             double p_tope_hs_normales  = Tope de horas nomales del regimen, varia si otros puestos tienen 
*                                          horas
* Devuelve: 
*			  Distribuye las horas de la variable p_hs_faltan_agregar en horas normales y al 50% segun el 
*             tope establesido por la variable p_tope_hs_normales 
*
*             double* p_hs_agrego_nor  = Horas normales
*             double* p_hs_agrego_50   = Horas al 50 %
*
************************************************************************************************************/
void ModifDistribNormal(bool p_es_partime, double p_hs_faltan_agregar, double p_tope_hs_normales, 
                        double* p_hs_agrego_nor, double* p_hs_agrego_50)
{
	if(p_hs_faltan_agregar <= p_tope_hs_normales || p_es_partime) {
	     *p_hs_agrego_nor = p_hs_faltan_agregar;
	     *p_hs_agrego_50  = 0;
	}
	else {
	     *p_hs_agrego_nor = p_tope_hs_normales;
	     *p_hs_agrego_50  = p_hs_faltan_agregar - p_tope_hs_normales;
	}
	
}
/************************************************************************************************************
*                                          DistribuyeHoras
*
* Parametros : 
*
*	int 	p_emp     :	Codigo de empresa
*	long	p_nroleg  : Numero de legajo 
*	DATE	p_fecha   : Fecha del Parte
*	char*	p_tipvig  :	Tipo de Vigilador 'E' o 'P' (para calcular Franco)
*   int		p_pais    : Codigo de pais (para calcular feriados)
*   int		p_prov    : Codigo de provincia (para calcular feriados)
*	TIME	p_hora_ent: Horario de entrada del vigilador
*	TIME	p_hora_sal: Horario de salida del vigilador
*   double  p_hs_nor  : Horas normales en otros puestos
*
* Devuelve : 
*	Distribuye las horas trabajadas en los tipos de horas espesicicadas a continuacion:
*
*		int* p_hs_agrego_nor	: Horas normales
*		int* p_hs_agrego_50     : Horas al 50%
*		int* p_hs_agrego_100fr	: Horas al 100% Franco
*		int *p_hs_agrego_100fe	: Horas al 100% Feriado
*
************************************************************************************************************/
void DistribuyeHoras(int p_emp, long p_nroleg, DATE p_fecha, char *p_tipvig, int p_pais, int p_prov,
                     TIME p_hora_ent, TIME p_hora_sal, double p_hs_nor,double  *p_hs_agrego_nor, 
                     double *p_hs_agrego_50, double *p_hs_agrego_100fr, double *p_hs_agrego_100fe)
{

	bool v_un_dia              = FALSE, 
	     v_es_partime          = FALSE;

	int  v_numfran;

	double 	v_hs_faltan_agregar    = 0,
			v_hs_hoy               = 0,
			v_hs_man               = 0,
			v_tope_hs_normales     = 0,
			v_hs_agrego_nor        = 0,
			v_hs_agrego_50         = 0,
			v_hs_agrego_100fr      = 0,
			v_hs_agrego_100fe      = 0;

	char v_regimen[15];

	// Obtengo Horas Normales de Regimen Efectivo
	GetRegimenEfectivo(p_emp, p_nroleg, v_regimen, p_fecha);
	v_tope_hs_normales =((double)GetHsNormales(v_regimen, FALSE))/100;

	v_es_partime = VigPartime(p_emp, p_nroleg, p_fecha);

	// Horas que tengo que agregar en total
	v_hs_faltan_agregar = ConvHraInt(p_hora_ent, p_hora_sal);

	/* Me fijo si el periodo se extiende a un dia o a dos */
	v_un_dia = AbarcaUnDia(p_hora_ent, p_hora_sal, &v_hs_hoy, &v_hs_man);

    //////////
	//Franco// (Todas las horas al 100 % franco)
	//////////
	v_numfran = GetNumFrancoEfectivo(p_emp, p_nroleg, p_fecha);
	if (!v_es_partime && Franco(p_emp, p_nroleg, p_fecha, p_tipvig, v_numfran)) {
		v_hs_agrego_100fr = ConvHraInt(p_hora_ent, p_hora_sal);

		// Descuento las horas que ya distribui
		v_hs_faltan_agregar -= v_hs_agrego_100fr;

    }
    else {
        ///////////
	    //Feriado// (horas que correspondan al franco y resto se distribuyen)
		///////////
		// Hoy es feriado
		if (FeriadoNovia(p_fecha, p_pais, p_prov)) {

			// Si todas las horas pertenecen al mismo dia o son 2 dias feriados seguidos va todo al 100% feriado
			if (v_un_dia|| FeriadoNovia(p_fecha+1, p_pais, p_prov)) 
				v_hs_agrego_100fe = v_hs_faltan_agregar;
			else 
				v_hs_agrego_100fe=v_hs_hoy;
		}
		else // Mañana es feriado
			if(!v_un_dia && FeriadoNovia(p_fecha+1, p_pais, p_prov))
				v_hs_agrego_100fe  = v_hs_man;

		// Descuento las horas que ya distribui
		v_hs_faltan_agregar-=v_hs_agrego_100fe;
    }

	//Descuento las Horas que ya fueron pasadas en otros puestos
	v_tope_hs_normales -= p_hs_nor;

	///////////////////////
	//Distribucion Normal//
	///////////////////////
	ModifDistribNormal(v_es_partime, v_hs_faltan_agregar, v_tope_hs_normales, &v_hs_agrego_nor, &v_hs_agrego_50);

	/////////////////////
	//Dia del Vigilador//
	/////////////////////
    ModifDiaVigilador(p_emp, p_fecha, v_un_dia, v_hs_hoy, v_hs_man, v_tope_hs_normales, 
                      &v_hs_agrego_nor , &v_hs_agrego_50, &v_hs_agrego_100fr);

	*p_hs_agrego_nor   = v_hs_agrego_nor;
    *p_hs_agrego_50    = v_hs_agrego_50;
    *p_hs_agrego_100fr = v_hs_agrego_100fr;
    *p_hs_agrego_100fe = v_hs_agrego_100fe;

}
