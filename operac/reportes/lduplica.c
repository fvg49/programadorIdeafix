/********************************************************************
*
* MODULE & VERSION : @(#)lduplica.c	1.1
* DATE             : 11/01/05
* TIME             : 11:20:12
*
* CREATED          : 30/01/10
*
* DESCRIPTION:
*        Listado para buscar legajos repetidos
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "prosegur.sch"
#include "lduplica.fmh"
#include "billpro.h"

#define SUPLE_PLUS     1 //Suplemente PLUS
#define SUPLE_PORC     2 //Porcentaje del TK

#define ERR_ARCHI	"No se pudo abrir el archivo de salida."
#define MAX_LEG		150000
#define _TOPE_HORAS	1600
#define LEG_ACTIVO	1
#define LEG_EGRESO	2
#define LEG_INACTIVO	0

#define CONVENIO_1	1
#define CANTSUPLE  MAXSUPLEM
#define OTRO  (CANTSUPLE)


/*-----------------------------------------------------------------------------------------------
Salidas que genera:
HS_TOPE   : Vigilador que trabajo mas horas que el tope
DET_HSTOPE : Detalle del vigilador que trabajo mas horas que el tope
DUPLICA_CON_EXCEPCION : Hay que ajustarlo manualmente , es un registro del parte duplicado pero tiene excepcion
DUPLICA_BORRAR : Caso duplicado del parte, se puede eliminar. La salida std genera el archivo a borrar (fue modificada a fp la salida)
EXCEPCIONES_MAYORES_AL_PARTE : Las horas de las expcesiones son mayores a las horas del parte, ya sea normales, al 50 o al 100.
EXCEPCIONES_SIN_PARTE : En un día tiene horas en excepciones pero no en el parte.
SUPERPOSICION_HORARIA: Visualiza si el vigilador tiene horas trabajadas superpuesta para un mismo dia.
HOR_2400 : Trabajo 24 horas
PARTE_SIN_CONFIR : Hay que confirmar.
SIN_REGISTRAR : Parte sin registro, es decir que hay días que deberian estar cardagados pero que no lo fueron.
SUPERA_HS_NORMALES : Indica los casos donde el vigilador supera la cantidad de horas normales prefijada en el FM
-----------------------------------------------------------------------**/

/* Estructuras */
struct Legajo {
	long   nroleg;
	DATE   dia;
	short  emp;
	long   cliente;
	short  objet;
	char   regimen[15];
	TIME   hentrada, hsalida;
	bool   tiene_excepcion;
	short  ptoser, puesto, nroint;
	double hsnor; //Total de horas normales
	double toths; //Total de horas
	double tot_nor_dym; //Total de horas dobles y media para un dia T
}leg[MAX_LEG];

struct Totales {
	long   nroleg;
	DATE   dia;
	double hsnor, toths;
}tot[MAX_LEG];

static void AbrirArchivo();
static void ImprimirArchivo();
static void GenerarReporte();
static void RecorrerExcepciones();
static void SinParte();
static void AcumuloHoras(long nroleg, double hsnor, double toths, DATE dia);
static int  comparar(struct Legajo *a, struct Legajo *b);
static int  ordenartot(struct Totales *a, struct Totales *b);

static void ObtSuplementos(int puesto, double suplem[CANTSUPLE], double sueldo_basico);
static double SueldoConf(double suplem[CANTSUPLE], double basico);
//private bool SuperposicionHoraria(TIME p_horini, TIME p_horfin, TIME p_ini_rango, TIME p_fin_rango);
//private bool HoraEnHorario(TIME p_hora, TIME p_ini_rango, TIME p_fin_rango);

/* Declaraciones globales */
FILE *fp;
form fm0;
report rp;
schema comerc, operac, bill, sue, prosegur;
int vec = 0, vec1 = 0;
char regimen[15];
char bufferstr[50];
int convenio;
int g_puesto;

/* Programa principal */
wcmd(lduplica, 1.1 01/05/11)
{
	fm0    = OpenForm  ("lduplica", FM_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);
	sue    = OpenSchema("sue",      IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	prosegur = OpenSchema("prosegur", IO_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	convenio = CONVENIO;//GetConvenioPorEmp(FmIFld(fm0, EMP));                                        
	GenerarReporte();

	if (*FmSFld(fm0, SALIDA) == 'A') {
		AbrirArchivo();
		ImprimirArchivo();
	}
}

static void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, ARCHIVO),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
	
	fprintf(fp, "FILTRAR\n");
		
}

static void GenerarReporte()
{
	dbcursor c_parte = (dbcursor) ERROR;
	c_parte = CreateCursor(operac|PARTEbyDIA, IO_NOT_LOCK);
	SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_parte) != ERROR) {
	
		if (LFld(operac|PARTE_NROLEG) < FmLFld(fm0, VIGILD) ||
			LFld(operac|PARTE_NROLEG) > FmLFld(fm0, VIGILH))
			continue;

		sprintf(bufferstr, "Procesando DÍa %.1D", DFld(operac|PARTE_DIA));
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();

		leg[vec].nroleg  = LFld(operac|PARTE_NROLEG);
		leg[vec].dia     = DFld(operac|PARTE_DIA);
		leg[vec].emp     = IFld(operac|PARTE_EMP);
		leg[vec].cliente = LFld(operac|PARTE_CLIENTE);
		leg[vec].objet   = IFld(operac|PARTE_OBJETIVO);

		leg[vec].ptoser   = IFld(operac|PARTE_PTOSER);
		leg[vec].puesto   = IFld(operac|PARTE_PUESTO);
		leg[vec].nroint   = IFld(operac|PARTE_NROINT);

		leg[vec].hsnor   = IFld(operac|PARTE_HSNOR);
		leg[vec].hentrada = TFld(operac|PARTE_HORAENT);
		leg[vec].hsalida   = TFld(operac|PARTE_HORASAL);

		leg[vec].toths   = (IFld(operac|PARTE_HSNOR) + IFld(operac|PARTE_HS50) + IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE));

		// Total de horas dobles y media para un dia T
		leg[vec].tot_nor_dym  = 0;
		
		strcpy(leg[vec].regimen, regimen);


		leg[vec].tiene_excepcion = FALSE;
		SetKey(operac|EXCEPCIONbyEMP, IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), 
						IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG),
						IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), 
						MIN_SHORT, MIN_SHORT);
						
		while (GetRecord(operac|EXCEPCIONbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {
			leg[vec].tiene_excepcion = TRUE;
		}

		AcumuloHoras(leg[vec].nroleg, leg[vec].hsnor, leg[vec].toths, leg[vec].dia);
		vec++;

		if (vec == MAX_LEG)
			Error("El vector esta saturado");
	
	}

	qsort((char *)leg, (unsigned)(vec),  sizeof(leg[0]), (IFPVCPVCP)comparar);
	qsort((char *)tot, (unsigned)(vec1), sizeof(tot[0]), (IFPVCPVCP)ordenartot);
}

static void AcumuloHoras(long nroleg, double hsnor, double toths, DATE dia)
{
	int i;

	for (i = 0; i < vec1; i++) {
		if (tot[i].nroleg == nroleg && tot[i].dia == dia) {
			tot[i].hsnor += hsnor;
			tot[i].toths += toths;
			return;
		}
	}
	tot[vec1].nroleg = nroleg;
	tot[vec1].hsnor  = hsnor;
	tot[vec1].toths  = toths;
	tot[vec1].dia    = dia;
	vec1++;
}

static int ordenartot(struct Totales *a, struct Totales *b)
{
	return	a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
			a->dia    < b->dia    ? -1 : a->dia    > b->dia    ? 1 :
			a->hsnor  < b->hsnor  ? -1 : a->hsnor  > b->hsnor  ? 1 : 0;
}

static int comparar(struct Legajo *a, struct Legajo *b)
{
	return	a->nroleg  < b->nroleg  ? -1 : a->nroleg  > b->nroleg  ? 1 :
			a->dia     < b->dia     ? -1 : a->dia     > b->dia     ? 1 :
			a->cliente < b->cliente ? -1 : a->cliente > b->cliente ? 1 :
			a->objet   < b->objet   ? -1 : a->objet   > b->objet   ? 1 : 
			a->hentrada   < b->hentrada   ? -1 : a->hentrada   > b->hentrada   ? 1 :
			a->hsalida   < b->hsalida   ? -1 : a->hsalida   > b->hsalida   ? 1 : 0;
}


static void	ImprimirArchivo()
{
	int i, j;
	bool primero;
	TIME ant_hent, ant_hsal;
	long ant_cliente;
	int  ant_objetivo;
	bool supera_tope;
	double hsnor_x_leg = 0.0;
	long leg_ant = NULL_LONG;
	long cliente_anterior = NULL_LONG;
	int objet_anterior = NULL_SHORT, ptoser_anterior = NULL_SHORT, puesto_anterior = NULL_SHORT;
	double supper[CANTSUPLE], confper, basper, ticket, suelad, pread, tickad;
	int cate_vigilador = NULL_SHORT;
		
    sprintf(bufferstr, "Generando Archivo ");
	FmSetFld(fm0, COMENT, bufferstr);
	WiRefresh();

    		
	for (i = 0; i < vec1; i++) {
		GetRegimenEfectivo(FmIFld(fm0, EMP), tot[i].nroleg, regimen, tot[i].dia);

		SetFld(comerc|REGIMEN_REGIM, regimen);
		GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK);

		sprintf(bufferstr, "Generando Archivo legajo %ld ", tot[i].nroleg);
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();

		// ------- Tengo la lista con el total por dia -------
		supera_tope=FALSE;
		if (tot[i].toths > _TOPE_HORAS) {
			supera_tope=TRUE;
			fprintf(fp, "HS_TOPE\t%ld\t%30s\t%20s\t%.3D\t%.2f\n", tot[i].nroleg, GetNombreLeg(FmIFld(fm0, EMP), tot[i].nroleg),
											  regimen, tot[i].dia, (double)(tot[i].toths / 100.0));
		}
        if (leg_ant != tot[i].nroleg)   
        	hsnor_x_leg = 0.0;        	
			
		if(hsnor_x_leg > FmFFld(fm0, NORTOPE)) {
			fprintf(fp, "SUPERA_HS_NORMALES\t%ld\t%s\n", tot[i].nroleg, GetNombreLeg(FmIFld(fm0, EMP), tot[i].nroleg));
			hsnor_x_leg = 0.0;
		}
		
		hsnor_x_leg += tot[i].hsnor;
		leg_ant = tot[i].nroleg;
		
		for (j = 0; j < vec && (leg[j].nroleg != tot[i].nroleg || leg[j].dia != tot[i].dia); j++);
		
		primero=TRUE;
		ant_hent=NULL_TIME;
		ant_hsal=NULL_TIME;
		ant_cliente = NULL_LONG;
		ant_objetivo = NULL_SHORT;
        

        SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), tot[i].nroleg);
        GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
		GetSBasico(convenio, IFld(sue|PER_CODCAT), &basper, &ticket, &suelad, &pread, &tickad);
		ObtSuplementos(IFld(sue|PER_CODCAT), supper, basper);
		basper = (basper == NULL_DOUBLE) ? 0 : basper;
		confper = SueldoConf(supper, basper);
		cate_vigilador = IFld(sue|PER_CODCAT);

		for (; j < vec && leg[j].nroleg == tot[i].nroleg && leg[j].dia == tot[i].dia; j++) {

            cliente_anterior = leg[j].cliente;
            objet_anterior   = leg[j].objet;
            ptoser_anterior  = leg[j].ptoser;
            puesto_anterior  = leg[j].puesto;
			
            
            if (supera_tope) {
				fprintf(fp, "DET_HSTOPE\t%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%.2T\t%.2T\n", 
						leg[j].nroleg,  GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente),
						leg[j].objet, 	GetObjDescrip(leg[j].cliente, leg[j].objet),
						regimen,        leg[j].hentrada, leg[j].hsalida);
                	
            }

            //Controlo que no haya parte sin confirmar.
			if (leg[j].hsalida != StrToT("00:00") && leg[j].hentrada != StrToT("00:00")  && leg[j].toths == 0.0) {
				
				SetKey(operac|PARTEbyEMP, leg[j].emp, leg[j].cliente, leg[j].objet, leg[j].dia, leg[j].nroleg, leg[j].ptoser, leg[j].puesto, leg[j].nroint);
				
				if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				
					fprintf(fp, "PARTE_SIN_CONFIR\t%d\t%ld\t%s\t%d\t%s\t%.3D\t%ld\t%s\t%.2T\t%.2T\t%.2f\t%.2f\t%.2f\t%.2f\t%s\t%d\t%d\t%d\n",
               			IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), GetDescCli(LFld(operac|PARTE_CLIENTE)),
               			IFld(operac|PARTE_OBJETIVO), GetObjDescrip(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)), DFld(operac|PARTE_DIA), 
               			LFld(operac|PARTE_NROLEG), GetNombreLeg(IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG)), TFld(operac|PARTE_HORAENT), 
               			TFld(operac|PARTE_HORASAL), FFld(operac|PARTE_HSNOR), FFld(operac|PARTE_HS50), 
               			FFld(operac|PARTE_HS100F), FFld(operac|PARTE_HS100FE), SFld(operac|PARTE_CONDIC), 
               		    IFld(operac|PARTE_PTOSER),  IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT));
				}               		    
            }
            
            if (leg[j].hsalida == StrToT("23:59") && leg[j].hentrada == StrToT("00:00")) {

				fprintf(fp, "HOR_2400\t%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%.2T\t%.2T\n", 
						leg[j].nroleg,  GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente),
						leg[j].objet, 	GetObjDescrip(leg[j].cliente, leg[j].objet),
						regimen,        leg[j].hentrada, leg[j].hsalida);
                	
            }
			
			//Controlo superposicion horaria para el mismo dia solo con el registro anterior		
			if(!primero && SuperposicionHoraria(leg[j].hentrada, leg[j].hsalida, ant_hent, ant_hsal) && (leg[j].hentrada != StrToT("00:00") && leg[j].hsalida != StrToT("00:00"))) {
				fprintf(fp, "SUPERPOSICION_HORARIA\t%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%.2T\t%.2T\t", 
						leg[j].nroleg, GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente), leg[j].objet, GetObjDescrip(leg[j].cliente, leg[j].objet), 
						leg[j].hentrada, leg[j].hsalida);
				fprintf(fp, "%ld\t%s\t%d\t%s\t%.2T\t%.2T\n", 
						ant_cliente, GetDescCli(ant_cliente),ant_objetivo, GetObjDescrip(ant_cliente, ant_objetivo), 
						ant_hent, ant_hsal);

			}

			if (!primero && leg[j].hentrada == ant_hent && leg[j].hsalida == ant_hsal) {
                

                if (leg[vec].tiene_excepcion) {
					fprintf(fp, "DUPLICA_CON_EXCEPCION\t%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%.2T\t%.2T\n", 
						leg[j].nroleg,  GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente),
						leg[j].objet, 	GetObjDescrip(leg[j].cliente, leg[j].objet),
						regimen,        leg[j].hentrada, leg[j].hsalida);
                	
                }
                else {
					fprintf(fp, "DUPLICA_BORRAR\t%ld\t%s\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%.2T\t%.2T\n", 
						leg[j].nroleg,  GetNombreLeg(FmIFld(fm0, EMP), leg[j].nroleg), leg[j].dia,
						leg[j].cliente, GetDescCli(leg[j].cliente),
						leg[j].objet, 	GetObjDescrip(leg[j].cliente, leg[j].objet),
						regimen,        leg[j].hentrada, leg[j].hsalida);
                }
			}
			
			if(leg[j].hentrada != StrToT("00:00") && leg[j].hsalida != StrToT("00:00")) {
				primero=FALSE;
				ant_hent=leg[j].hentrada;
				ant_hsal=leg[j].hsalida;
				ant_objetivo = leg[j].objet;
				ant_cliente = leg[j].cliente;
			}
		}
	}

	RecorrerExcepciones();
	SinParte(); //Esta función busca los legajos que estan sin confirmar

}

static void RecorrerExcepciones()
{
	dbcursor c_exc = (dbcursor) ERROR; 
	char detalle_exp[50];
	// --- Control de excepciones -------	
	c_exc   = CreateCursor(operac|EXCEPCIONbyLEGAJO, IO_NOT_LOCK); //	legajo (emp, nroleg, dia, cliente, objetivo);
	SetCursorFrom(c_exc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD), MIN_DATE, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_exc, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH), MAX_DATE, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_exc) != ERROR) {
		if (DFld(operac|EXCEPCION_DIA) < FmDFld(fm0, FECHAD) ||
			DFld(operac|EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		sprintf(bufferstr, "Procesando Excepcion DÍa %.1D", DFld(operac|EXCEPCION_DIA));
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();
		
		SetKey(operac|PARTEbyEMP, IFld(operac|EXCEPCION_EMP), LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO), DFld(operac|EXCEPCION_DIA), LFld(operac|EXCEPCION_NROLEG), IFld(operac|EXCEPCION_PTOSER), IFld(operac|EXCEPCION_PUESTO), IFld(operac|EXCEPCION_NROINT));

		//Si no existe registro generar linea en fp con nroleg cliente objetivo dia puesto codint y datos de excepciones
		if(GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		    
		    strcpy (detalle_exp, NULL_STR);
			sprintf(detalle_exp, "%.2f  %.2f  %.2f", FFld(operac|EXCEPCION_HORAS) / 100, FFld(operac|EXCEPCION_HS50) / 100, FFld(operac|EXCEPCION_HS100) / 100);
			fprintf(fp, "EXCEPCIONES\tSIN_PARTE\t%ld\t%ld\t%d\t%.3D\t%d\t%d\t%d\t%d\t%d\t%s\n", 
						LFld(operac|EXCEPCION_NROLEG), LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO),
						DFld(operac|EXCEPCION_DIA), IFld(operac|EXCEPCION_PTOSER), IFld(operac|EXCEPCION_PUESTO), 
						IFld(operac|EXCEPCION_NROINT), IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO), 
						 detalle_exp);
		}
		else {
		//Si existe comparar cantidad de horas
		
			if((FFld(operac|PARTE_HSNOR) - FFld(operac|EXCEPCION_HORAS)) < 0) {

				strcpy (detalle_exp, NULL_STR);
				sprintf(detalle_exp, "HSNOR Parte: %.2f HSNOR Exp %.2f", FFld(operac|PARTE_HSNOR) / 100, FFld(operac|EXCEPCION_HORAS));
				fprintf(fp, "EXCEPCIONES\tMAYORES_AL_PARTE\t%ld\t%ld\t%d\t%.3D\t%d\t%d\t%d\t%d\t%d\t%s\n", LFld(operac|EXCEPCION_NROLEG), 
						LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO), DFld(operac|EXCEPCION_DIA), IFld(operac|EXCEPCION_PTOSER), IFld(operac|EXCEPCION_PUESTO),
						 
						IFld(operac|EXCEPCION_NROINT),  IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO), 
						detalle_exp);
			}
			if((FFld(operac|PARTE_HS50) - FFld(operac|EXCEPCION_HS50)) < 0) {

				strcpy (detalle_exp, NULL_STR);
				sprintf(detalle_exp, "HS50 Parte: %.2f HS50 Exp %.2f", FFld(operac|PARTE_HS50) / 100, FFld(operac|EXCEPCION_HS50));
				fprintf(fp, "EXCEPCIONES\tMAYORES_AL_PARTE\t%ld\t%ld\t%d\t%.3D\t%d\t%d\t%d\t%d\t%d\t%s\n", LFld(operac|EXCEPCION_NROLEG), 
						LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO), DFld(operac|EXCEPCION_DIA), IFld(operac|EXCEPCION_PTOSER), IFld(operac|EXCEPCION_PUESTO), 
						IFld(operac|EXCEPCION_NROINT), IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO),  detalle_exp);

			}
			if((FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE) - FFld(operac|EXCEPCION_HS100)) < 0) {

				strcpy (detalle_exp, NULL_STR);
				sprintf(detalle_exp, "HSNOR Parte: %.2f HSNOR Exp %.2f", (FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE))/ 100, FFld(operac|EXCEPCION_HS100));
				fprintf(fp, "EXCEPCIONES\tMAYORES_AL_PARTE\t%ld\t%ld\t%d\t%.3D\t%d\t%d\t%d\t%d\t%d\t%s\n", LFld(operac|EXCEPCION_NROLEG), 
						LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO), DFld(operac|EXCEPCION_DIA), IFld(operac|EXCEPCION_PTOSER), IFld(operac|EXCEPCION_PUESTO), 
	 					IFld(operac|EXCEPCION_NROINT), IFld(operac|EXCEPCION_CONDIC), IFld(operac|EXCEPCION_MOTIVO),  detalle_exp);
			}
		}
	} 
}


static void SinParte()
{
	dbcursor c_per   = (dbcursor) ERROR;
	DATE dia = NULL_DATE;
	long nro_leg = NULL_LONG;
	DATE fdesde = NULL_DATE;
	DATE fhasta = NULL_DATE;
	
	
	c_per     = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
    
    if(FmIsNull(fm0, VIGILD)) {
		SetCursorFrom(c_per, FmIFld(fm0, EMP), MIN_LONG);
		SetCursorTo  (c_per, FmIFld(fm0, EMP), MAX_LONG);
    }
    else {
		SetCursorFrom(c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD));
		SetCursorTo  (c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH));
    }
	
	while (FetchCursor(c_per) != ERROR) {
		
        if((IFld(sue|PER_ACTIVO) == LEG_INACTIVO) && (DFld(sue|PER_FECEGR) < FmDFld(fm0, FECHAD)))
        	continue;
	 
        if((IFld(sue|PER_ACTIVO) == LEG_EGRESO) && (DFld(sue|PER_FECEGR) < FmDFld(fm0, FECHAD)))
        	continue;
        
	    fdesde = FmDFld(fm0, FECHAD);
	    fhasta = FmDFld(fm0, FECHAH);
	    
	 	if(fdesde < DFld(sue|PER_FECING))
        	fdesde = DFld(sue|PER_FECING);
 		
 		if(DFld(sue|PER_FECEGR) != NULL_DATE)
 			if(fhasta > DFld(sue|PER_FECEGR))
				fhasta = DFld(sue|PER_FECEGR);	 			
        	
        nro_leg = LFld(sue|PER_NROLEG);
		sprintf(bufferstr, "Verificando el legajo %ld", nro_leg);
		FmSetFld(fm0, COMENT, bufferstr);
		WiRefresh();
        
//	  	if(IFld(sue|PER_RELACION) == CONVENIO_1) {
		  	for(dia = fdesde; dia <= fhasta; dia++)	{
					SetKey(operac|PARTEbyEMPLE,  FmIFld(fm0, EMP), nro_leg, dia, NULL_LONG, NULL_SHORT);
					if(GetRecord(operac|PARTEbyEMPLE, PARTIAL_KEY|NEXT_KEY, IO_NOT_LOCK, 3) == ERROR)	{
						fprintf(fp, "SIN_REGISTRAR\t%ld\t%s\t%.3D\n", LFld(sue|PER_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(sue|PER_NROLEG)), dia);
					}
			}
//	  	}
	} 
}

static void ObtSuplementos(int puesto, double suplem[CANTSUPLE], double sueldo_basico)
{
	int i;    
	double porcentaje = NULL_DOUBLE;

	for (i = 0 ; i < MAXSUPLEM ; i++)
		suplem[i] = 0.0;

	SetIFld(prosegur|CATESUPL_RELACION, convenio);
	SetIFld(prosegur|CATESUPL_CODCAT, puesto);
	SetIFld(prosegur|CATESUPL_CODSUPL, MIN_SHORT);
	
	while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		switch (IFld(prosegur|CATESUPL_CODSUPL)) {
			case SUPLE_PLUS :
				suplem[SUPLE_PLUS] = FFld(prosegur|CATESUPL_VAL);
				break;   

			case SUPLE_PORC :
			    porcentaje = FFld(prosegur|CATESUPL_VAL);
				suplem[SUPLE_PORC]= ((suplem[SUPLE_PLUS] + sueldo_basico) * (porcentaje / 100)) / 1000; 
				break;

			default :
				suplem[OTRO] = FFld(prosegur|CATESUPL_VAL);
				break;					
		}
	}
}         

static double SueldoConf(double suplem[CANTSUPLE], double basico)
{
	int i;
	double total = 0.0;
	for (i = 0 ; i < MAXSUPLEM ; i++) {
		total += suplem[i];
	}
	total += basico;
	return total;
}

