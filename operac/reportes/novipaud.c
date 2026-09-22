/********************************************************************
* MODULE & VERSION : @(#)novipaud.c	1.5
* DATE             : 10/05/13
* TIME             : 12:08:47
*
* CREATED          : Andres Daniel Chimuris (chimuris)
*
* DESCRIPTION:
*      Vigiladores con Asignaciones pero sin Parte
*********************************************************************/

#include <ideafix.h>   
#include <math.h>
#include "novipaud.rph"
#include "novipaud.fmh"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "asist.sch"
#include "comerc.sch"
#include "comerc.h"        
#include "filial.h"
#include "billpro.h"


#define	subdia(dia)	    (dia == 'L' ? 0 : dia == 'M' ? 1 : dia == 'X' ? 2 :	dia == 'J' ? 3 : dia == 'V' ? 4 : dia == 'S' ? 5 : dia == 'D' ? 6 : dia == 'P' ? 7 : NULL_SHORT)
#define	diasub(i)	    (i == 0 ? "L" : i == 1 ? "M" : i == 2 ? "X" : i == 3 ? "J" : i == 4 ? "V" : i == 5 ? "S": i == 6 ? "D" : i == 7 ? "P" : NULL_STR)
#define ARCHI     0
#define TERM      1
#define IMPRE     2                                         //DESC OBJ
#define REGARCH "%d\t%ld\t%s\t%.3D\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n"
#define INACTIVO  0  //Si el vigilador en PER tiene 0 en el campo activo se lo considera inactivo.
#define IMP_VIGBAJ		3  //Imprimir vigibaj	
#define IMP_LNOPARTE	2  //Imprimir lnoparte
#define MAX_LEG		100000
#define TIPO_ERR_SIN_PARTE 	"Asignados sin parte"
#define TIPO_ERR_BAJA 		"Vigilador dado de baja"
#define TIPO_ERR_COND_DE_TRABAJ	"Condición de trabajo"
#define TIPO_ERR_HS_NOR_VS_HS_REG	"Hs. Normales VS Hs Regimen"
#define TIPO_ERR_HORAS_NEGATIVAS	"Tiene horas negativas"
#define TIPO_ERR_DIF_HS_EN_PARTE	"Dif entre hora ent y salida del parte con total de horas del mismo"
#define MAXPARTE 10000  //ctrlpar1

//Ctrlpar1
typedef struct partes_typ {
	long	cliente;
	int		objetivo;
	int 	ptoser;
	int		puesto;         
	char	cond;
	int		codaus;
	long	pos;
} partes_obj, *partes_ptr;                            

typedef struct legajos_typ {
	long nroleg;
	DATE dia;
	char cond;
	int	 codaus;
	bool diff;
	struct legajos_typ * next;
} legajos_obj, *legajos_ptr;

/* Estructuras */
struct Legajo {
	long   nroleg;
	DATE   dia;
	long   cliente;
	int    objet;
	char   regimen[15];
	double hsnor;
}leg[MAX_LEG];

struct Totales {
	long   nroleg;
	DATE   dia;
	double hsnor;
}tot[MAX_LEG];

int vec = 0, vec1 = 0;
char regimen[15];

schema 		operac, bill, comerc, sue, asist;
form   		fm0;
report 		rp0 = ERROR;
dbcursor 	c_per;

//Funciones universales.
//static fm_status after(form fm, fmfield fno, int row);
static fm_status before (form fm, fmfield fno, int row);
static void AbrirSalida();
void MostrarLegProcesados();
static void ImprimirInfo(int emp, long nroleg, DATE dia, char * vigil, long cliente, int objetivo, char detalle1[100], short impre);
                                                                                                                    //impre es el tipo de error
//Funciones para Listar Vigiladores sin Partes
static void ListadoNoParte();
static void Asig(long vigilador);
static void AsigH(long vigilador);

//Funciones de Listado para comparar Hs. Normales Trabajadas contra Hs. Normales de Regimen
static void RecorrerParte();
static void  ImprimirHsVSRegim();
static void AcumuloHoras(long nroleg, double hsnor, DATE dia);
static int  comparar(struct Legajo *a, struct Legajo *b);
static int  ordenartot(struct Totales *a, struct Totales *b);

//Funcion que Controla los Vigiladores con Baja
static void VigiBaj();

//Funciones ctrlpar2
bool CambiaDia(long nroleg, DATE dia);
void ImprimeTabla(void);
void AgregaATabla(long cliente, int objetivo, int ptoser, int puesto, char cond, int codaus);
partes_obj PxLegxDia[MAXPARTE];
long	cParte,bParte;
long	lastleg;
DATE	lastdia;

bool   HorasNegativas();
void   ImprimirHsNega(long nroleg);
bool   DifEnElParte();             
void   ImprimirDifenParte(long nroleg);
bool   DifConExcepciones(long p_nroleg, DATE p_dia, long p_cliente, int p_objetivo, double p_hsnor, double p_hs50, double p_hs100, int  p_ptoser, int p_puesto, int p_nroint, int p_condic);
void   ImprimirDifConExcepciones(long p_nroleg);


char nomdias[9][2];
bool   salida;
DATE fecierre = NULL_DATE;
FILE *fp=NULL;

wcmd(novipaud, 1.5 05/13/10)
{
	comerc = OpenSchema ("comerc", IO_EABORT);
	bill   = OpenSchema ("bill",   IO_EABORT);
	asist  = OpenSchema ("asist",  IO_EABORT); 
	sue    = OpenSchema ("sue",    IO_EABORT);
	operac = OpenSchema ("operac", IO_EABORT);
	fm0    = OpenForm   ("novipaud",  FM_EABORT);
	
	if (DoForm(fm0, before, NULLFP) != FM_UPDATE) return;

   	AbrirSalida();    
	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);

    if(FmIFld(fm0, CTRLCOND) == TRUE){  
		fecierre  = GetFechaCierreOpe(FmIFld(fm0, EMP));
		InicLegajoXusr (FmIFld(fm0, EMP), fecierre, fm0, COMENT, TRUE, _TIPPER_INSERTA);
	}
	SetCursorFrom(c_per, FmIFld(fm0, EMP), MIN_LONG);
	SetCursorTo  (c_per, FmIFld(fm0, EMP), MAX_LONG);
	while (FetchCursor(c_per) != ERROR) {
        
	    if (!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)))
			continue; 
		if (FmLFld(fm0, VIGILD) > LFld(sue|PER_NROLEG)) 
			continue;
		if (FmLFld(fm0, VIGILH) < LFld(sue|PER_NROLEG))
			continue;

	    if(FmIFld(fm0, LNOPARTE) == TRUE)
			ListadoNoParte();
        
		if(FmIFld(fm0, VIGIBAJ)  == TRUE)
			VigiBaj();
                       //hsnvsreg 

        if((FmIFld(fm0, HSNVSREG) == TRUE) || (FmIFld(fm0, CTRLCOND) == TRUE) || (FmIFld(fm0, HSPARTE) == TRUE)){
			RecorrerParte();
            if(FmIFld(fm0, HSNVSREG) == TRUE)
				ImprimirHsVSRegim();
        }	

        MostrarLegProcesados();    
    }
    
    if(FmIFld(fm0, CTRLCOND) == TRUE){
    	FinObjetivosXusr();
		FinClientesXusr();
	}	
}
        
static void ListadoNoParte()
{
	Asig (LFld(sue|PER_NROLEG));
	AsigH(LFld(sue|PER_NROLEG));
}                                                               

static void Asig(long vigilador)
{
	dbcursor c_asig;
	dbfield  campo_dia;

	bool leoparte = FALSE, dias_trabajados[9];
	int v_i=0;
	DATE fdesde, fhasta, dia;
	
	c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (DFld(operac|ASIG_FECASIG) > FmDFld(fm0, FHASTA))
			continue;

		if (!strcmp(SFld(operac|ASIG_EFECT), PROVISORIO) && DFld(operac|ASIG_FECHAS) < FmDFld(fm0, FDESDE))
			continue;

		fdesde = FmDFld(fm0, FDESDE);
	    fhasta = FmDFld(fm0, FHASTA);

		// inicializo "dias_trabajados" en False y nombres de dias en NULL
		for (v_i = 0; v_i < 9; v_i ++)
	    	dias_trabajados[v_i] = FALSE;
 
 	    //fecha desde de la asignacion
	    if (DFld(operac|ASIG_FECASIG) > fdesde)
	    	fdesde = DFld(operac|ASIG_FECASIG);
        
        //fecha hasta de la asinacion o del form en caso de ser nula por parttime
	    if (!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) <  fhasta)
	    	fhasta = DFld(operac|ASIG_FECHAS);
	    
	    // Marco en vector dias_trabajados los numeros asignados
		for (campo_dia = ASIG_DIA1; campo_dia <= ASIG_DIA7; campo_dia++) 
			if (strcmp(SFld(operac|campo_dia), NULL_STR) != 0) 
				dias_trabajados[subdia(*SFld(operac|campo_dia))] = TRUE;

	    //Cuento cuantos dias estan asignados
		for (v_i=0; v_i<9; v_i ++)
			if(dias_trabajados[v_i])
				sprintf(nomdias[v_i], "%s", diasub(v_i));
			else
				sprintf(nomdias[v_i], "%s", " ");

	    for (dia = fdesde; dia <= fhasta; dia++) {
	    	leoparte = TRUE;
			
			//si es parttime hay que ver los dias en DIASPTIME
			if (str_eq(SFld(operac|ASIG_VIGIL), PARTTIME)) {
				SetKey(operac|DIASPTIMEbyDIA, IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), dia, NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
				if (GetRecord(operac|DIASPTIMEbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR)
					leoparte = FALSE; //si dia no esta en DIASPTIME no debe leer el parte
            }

			if (leoparte) {
				SetKey(operac|PARTEbyEMPLE, IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), dia, NULL_LONG, NULL_SHORT);
				if(GetRecord(operac|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR)	{
					//si no esta en parte puede estar en retro
					SetKey(operac|RETRObyREMPLE, IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), dia, NULL_LONG, NULL_SHORT);
					if(GetRecord(operac|RETRObyREMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR) {
				   	   	ImprimirInfo(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), dia, SFld(operac|ASIG_VIGIL), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), NULL_STR, IMP_LNOPARTE);
					}
			    }
			}	
	    }
	}
}

static void AsigH(long vigilador)
{
	dbcursor c_asigh;
	dbfield  campo_dia;

	bool leoparte = FALSE, dias_trabajados[9];
	int v_i=0;
	DATE fdesde, fhasta, dia;
	
	c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
	   	// motivo 80 
	   	if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE)
	  		continue;
	 	// fecha de asignacion se va de rango hasta
	 	if (DFld(operac|ASIGH_FECALT) > FmDFld(fm0, FHASTA))
			continue;
        // si provisorio fecha hasta no puede ser menor a desde del rango
        if (!strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO) && DFld(operac|ASIGH_FECHAS) < FmDFld(fm0, FDESDE))
			continue;

		fdesde = FmDFld(fm0, FDESDE);
	    fhasta = FmDFld(fm0, FHASTA);

		// inicializo "dias_trabajados" en False y nombres de dias en NULL
		for (v_i = 0; v_i < 9; v_i ++)
	    	dias_trabajados[v_i] = FALSE;
 
 	    //fecha desde de la asignacion
	    if (DFld(operac|ASIGH_FECALT) > fdesde)
	    	fdesde = DFld(operac|ASIGH_FECALT);
        
        //fecha hasta de la asinacion o del form en caso de ser nula por parttime
	    if (!IsNull(operac|ASIGH_FECBAJ) && DFld(operac|ASIGH_FECBAJ) <  fhasta)
	    	fhasta = DFld(operac|ASIGH_FECBAJ);
	    
	    // Marco en vector dias_trabajados los numeros asignados
		for (campo_dia = ASIGH_DIA1; campo_dia <= ASIGH_DIA7; campo_dia++) 
			if (strcmp(SFld(operac|campo_dia), NULL_STR) != 0) 
				dias_trabajados[subdia(*SFld(operac|campo_dia))] = TRUE;
          
	    //Cuento cuantos dias estan asignados
		for (v_i=0; v_i<9; v_i ++)
			if(dias_trabajados[v_i])
				sprintf(nomdias[v_i], "%s", diasub(v_i));
			else
				sprintf(nomdias[v_i], "%s", " ");

	    for (dia = fdesde; dia <= fhasta; dia++) {
	    	leoparte = TRUE;
			
			//si es parttime hay que ver los dias en DIASPTIME
			if (str_eq(SFld(operac|ASIGH_VIGIL), PARTTIME)) {
				SetKey(operac|DIASPTIMEHbyDIA, IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_NROLEG), dia, NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
				if (GetRecord(operac|DIASPTIMEHbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR)
					leoparte = FALSE; //si dia no esta en DIASPTIME no debe leer el parte
            }

			if (leoparte) {
				SetKey(operac|PARTEbyEMPLE, IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_NROLEG), dia, NULL_LONG, NULL_SHORT);
				if(GetRecord(operac|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR)	{
					//si no esta en parte puede estar en retro
					SetKey(operac|RETRObyREMPLE, IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_NROLEG), dia, NULL_LONG, NULL_SHORT);
					if(GetRecord(operac|RETRObyREMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) == ERROR) {
				   	   	ImprimirInfo(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_NROLEG), dia, SFld(operac|ASIGH_VIGIL), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), NULL_STR, IMP_LNOPARTE);
					}
			    }
			}	
	    }
	}
}

static void ImprimirInfo(int emp, long nroleg, DATE dia, char * vigil, long cliente, int objetivo, char detalle1[100], short impre)
{
    char detalle[50];
	if (rp0 != ERROR) {
		RpSetIFld(rp0, R_EMP, emp);
		RpSetLFld(rp0, R_LEGAJO, nroleg);
		RpSetFld (rp0, R_APENOM, GetNombreLeg(emp, nroleg));
		
        if(impre != IMP_VIGBAJ){
			RpSetLFld(rp0, R_CLIENTE, cliente);
			RpSetIFld(rp0, R_OBJETIVO, objetivo);
		}
		
		if(impre == IMP_LNOPARTE){
			RpSetFld (rp0, R_TXTERROR, TIPO_ERR_SIN_PARTE);
			sprintf(detalle, "%.1D %s-%s-%s-%s-%s-%s-%s", dia, nomdias[0],nomdias[1],nomdias[2],nomdias[3],nomdias[4], nomdias[5],nomdias[6]);
			RpSetFld (rp0, R_DETERROR, detalle);
			DoReport(rp0, LINEA);
		}
		

		if(impre == IMP_VIGBAJ) {
			RpSetFld (rp0, R_TXTERROR, TIPO_ERR_BAJA);
			RpSetFld (rp0, R_DETERROR, detalle1);
			DoReport(rp0, LINEA);
		}  
		
	}
    
	if (fp != NULL){
	   	fprintf(fp, "%d\t%ld\t%s\t%ld\t%s\t%d\t%s\t", emp, nroleg, GetNombreLeg(emp, nroleg), cliente, GetDescCli( cliente ), objetivo, GetObjDescrip(cliente, objetivo));
		
		if(impre == IMP_LNOPARTE){
			fprintf(fp, "%s\t", TIPO_ERR_SIN_PARTE);
			sprintf(detalle, "%.1D %s-%s-%s-%s-%s-%s-%s", dia, nomdias[0],nomdias[1],nomdias[2],nomdias[3],nomdias[4], nomdias[5],nomdias[6]);
			fprintf (fp, "%s\n", detalle);
		}

		if(impre == IMP_VIGBAJ) {
			fprintf(fp, "%s\t",  TIPO_ERR_BAJA);
			fprintf(fp, "%s\n", detalle1);
		}  
	}   	
	strcpy (detalle, NULL_STR);
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
	  	if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
	  		Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));
      
   	    fprintf(fp, "EMP\tNRO. LEGAJO\tAPELLIDO Y NOMBRE\tCLIENTE\tDESC. CLIENTE\tOBJET\tDESC. OBJET\tTIPO ERROR\tDETALLE ERROR\n");
	}
	else {
		rp0 = OpenReport("novipaud", RP_NOBEGIN|RP_EABORT, 1);
		RpSetOutput(rp0, (!strcmp(FmSFld(fm0, SALIDA), "I") ? RP_IO_DEFAULT : RP_IO_TERM), NULL_STR);

		if (BeginReport(rp0, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
		RpSetDFld (rp0, R_FDESDE, FmDFld (fm0, FDESDE));
		RpSetDFld (rp0, R_FHASTA, FmDFld (fm0, FHASTA));
	}
}

static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		FmSetDFld(fm, FDESDE, FirstMonthDay(Today()));
 	break;
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "novipaud.txt");
	break;
	            
	}
	return FM_OK;
}

void MostrarLegProcesados() 
{
	FmSetLFld(fm0, PROCVIGI, LFld(sue|PER_NROLEG));
	FmSetFld (fm0, DPROCVIG, SFld(sue|PER_APYNOM));
	WiRefresh();
}

static void VigiBaj()
{
	char detalle[100];
	strcpy(detalle, NULL_STR);

    SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), NULL_LONG, NULL_SHORT);
	if(GetRecord(operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR){

		if(IFld(sue|PER_ACTIVO) == INACTIVO) {
		    strcpy(detalle, NULL_STR);
		    sprintf(detalle, "Fecha del egreso: %.3D", DFld(sue|PER_FECEGR));
			ImprimirInfo(IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), DFld(sue|PER_FECEGR), NULL_STR, LFld(operac|ASIG_CLIENTE), LFld(operac|ASIG_OBJETIVO), detalle, IMP_VIGBAJ);
		}
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void RecorrerParte()
{
	dbcursor c_parte = (dbcursor) ERROR;
	int    i = 0;
	char   cond=0;
	int	   codaus=0;
	bool   chgcond = FALSE;

	c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom(c_parte, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FDESDE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_parte, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FHASTA), MAX_LONG, MAX_SHORT);
                         //index emple(emp,               nroleg,                dia, cliente, objetivo)
	while(FetchCursor(c_parte) != ERROR) {
 
 		if (IFld(operac|PARTE_HSNOR)  == 0 && IFld(operac|PARTE_HS50)    == 0 &&
			IFld(operac|PARTE_HS100F) == 0 && IFld(operac|PARTE_HS100FE) == 0)
			continue;

		if(FmIFld(fm0, HSNVSREG) == TRUE){
            
			leg[vec].nroleg  = LFld(operac|PARTE_NROLEG);
			leg[vec].dia     = DFld(operac|PARTE_DIA);
			leg[vec].cliente = LFld(operac|PARTE_CLIENTE);
			leg[vec].objet   = IFld(operac|PARTE_OBJETIVO);
			leg[vec].hsnor   = IFld(operac|PARTE_HSNOR);
			strcpy(leg[vec].regimen, regimen);

			AcumuloHoras(leg[vec].nroleg, leg[vec].hsnor, leg[vec].dia);
			vec++;

			if (vec == MAX_LEG)
				Error("El vector esta saturado");
		}
			
		if(FmIFld(fm0, CTRLCOND) == TRUE){
			i = 0;

            if (!ValidaLegajoXusr(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA)))
				continue;
		
			if (CambiaDia(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA))) {
				if (chgcond) {
					ImprimeTabla();     //Incluye after y before de cada legajo dia.
					chgcond = FALSE;
				}
				cond   = *SFld(operac|PARTE_CONDIC);
				codaus = IFld(operac|PARTE_CODAUS);
				cParte = 0;
				bParte = -1;
			}
			AgregaATabla(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), IFld(operac|PARTE_PTOSER), 
						IFld(operac|PARTE_PUESTO), *SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_CODAUS));
    	    chgcond |= (cond != *SFld(operac|PARTE_CONDIC) || codaus!=IFld(operac|PARTE_CODAUS));
        }   
        
        if(FmIFld(fm0, HSPARTE) == TRUE) {
        	if(HorasNegativas())
        		ImprimirHsNega(LFld(sue|PER_NROLEG));
        	if(DifEnElParte())
        		ImprimirDifenParte(LFld(sue|PER_NROLEG));
        		//bool   DifConExcepciones(long p_nroleg, DATE p_dia, long p_cliente, int p_objetivo, double p_hsnor, double p_hs50, double p_hs100, int  p_ptoser, int p_puesto, int p_nroint, int p_condic, int p_motivo)
//        	if(!DifConExcepciones(LFld(sue|PER_NROLEG), DFld(operac|PARTE_DIA), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), FFld(operac|PARTE_HSNOR), FFld(operac|PARTE_HS50), FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE), IFld(operac|PARTE_PTOSER, IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), IFld(operac|PARTE_CONDIC))))	
//        		ImprimirDifConExcepciones(LFld(sue|PER_NROLEG));
        }
	}
	if(FmIFld(fm0, HSNVSREG) == TRUE){
		qsort((char *)leg, (unsigned)(vec), sizeof(leg[0]), (IFPVCPVCP)comparar);
		qsort((char *)tot, (unsigned)(vec1), sizeof(tot[0]), (IFPVCPVCP)ordenartot);
	}
	if(FmIFld(fm0, CTRLCOND) == TRUE)
		chgcond = FALSE;	
}        

static void	ImprimirHsVSRegim()
{                
	int i;           
	char detalle[50];

	for (i = 0; i < vec1; i++) {
		GetRegimenEfectivo(FmIFld(fm0, EMP), tot[i].nroleg, regimen, tot[i].dia);

		SetFld(comerc|REGIMEN_REGIM, regimen);
		GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK);

//		if (!strcmp(regimen, _REGIMEN_1x1x1))
//			continue;

//		if (tot[vec1].hsnor <= GetHsNormales(regimen, IFld(comerc|REGIMEN_PARTIME)))
//			continue;

//        if(tot[i].nroleg == LFld(sue|PER_NROLEG))
        
//        for (j = 0; j < vec && (leg[j].nroleg != tot[i].nroleg || leg[j].dia != tot[i].dia); j++);
//      


      	if (!strcmp(regimen, _REGIMEN_1x1x1) && (tot[vec1].hsnor <= GetHsNormales(regimen, IFld(comerc|REGIMEN_PARTIME))))
//			continue;

        
		if(tot[i].nroleg == LFld(sue|PER_NROLEG)){

            if(rp0 != ERROR)	{
	            RpSetIFld(rp0, R_EMP, FmIFld(fm0, EMP));
		  		RpSetLFld(rp0, R_LEGAJO, tot[vec1].nroleg);
				RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), tot[vec1].nroleg));
				RpSetLFld(rp0, R_CLIENTE,  NULL_LONG);
				RpSetIFld(rp0, R_OBJETIVO, NULL_SHORT);

				RpSetFld (rp0, R_TXTERROR, TIPO_ERR_HS_NOR_VS_HS_REG);
				sprintf(detalle, "%.3D Regimen: %s Hs. Normales: %.2f", tot[vec1].dia, regimen, tot[vec1].hsnor / 100);
				RpSetFld (rp0, R_DETERROR, detalle);
				DoReport(rp0, LINEA);                
				break;   
			}

			if(fp != NULL) {
                fprintf(fp, "%d\t%ld\t%s\t%ld\t%s\t%d\t%s\t", FmIFld(fm0, EMP),tot[i]. nroleg, GetNombreLeg(FmIFld(fm0, EMP), tot[i].nroleg), NULL_LONG, NULL_STR, NULL_SHORT, NULL_STR);
			    fprintf(fp, "%s\t", TIPO_ERR_HS_NOR_VS_HS_REG);
				sprintf(detalle, "%.3D Regimen: %s Hs. Normales: %.2f", tot[i].dia, regimen, tot[i].hsnor / 100);
				fprintf(fp, "%s\n", detalle);
				break;   
			}

		}
	}
}

static void AcumuloHoras(long nroleg, double hsnor, DATE dia)
{
	int i;

	for (i = 0; i < vec1; i++) {
		if (tot[i].nroleg == nroleg && tot[i].dia == dia) {
			tot[i].hsnor += hsnor;
			return;
		}
	}
	tot[vec1].nroleg = nroleg;
	tot[vec1].hsnor  = hsnor;
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
			a->objet   < b->objet   ? -1 : a->objet   > b->objet   ? 1 : 0;
}

//ctrlpar2
int CmpParte(long este, long sig)
{
	if ((PxLegxDia[este].cliente  == PxLegxDia[sig].cliente)  &&
		(PxLegxDia[este].objetivo == PxLegxDia[sig].objetivo) &&
		(PxLegxDia[este].ptoser   == PxLegxDia[sig].ptoser)   &&
		(PxLegxDia[este].puesto   == PxLegxDia[sig].puesto)   &&
		(PxLegxDia[este].cond     == PxLegxDia[sig].cond)     &&
		(PxLegxDia[este].codaus   == PxLegxDia[sig].codaus))
			return 0;

	if ((PxLegxDia[este].cliente  < PxLegxDia[sig].cliente  || (PxLegxDia[este].cliente  == PxLegxDia[sig].cliente  &&
		(PxLegxDia[este].objetivo < PxLegxDia[sig].objetivo || (PxLegxDia[este].objetivo == PxLegxDia[sig].objetivo &&
		(PxLegxDia[este].ptoser   < PxLegxDia[sig].ptoser   || (PxLegxDia[este].ptoser   == PxLegxDia[sig].ptoser   &&
		(PxLegxDia[este].puesto   < PxLegxDia[sig].puesto   || (PxLegxDia[este].puesto   == PxLegxDia[sig].puesto   &&
		(PxLegxDia[este].cond     < PxLegxDia[sig].cond     || (PxLegxDia[este].cond     == PxLegxDia[sig].cond     &&
		(PxLegxDia[este].codaus   < PxLegxDia[sig].codaus))))))))))))
		return -1;

		return 1;
}

//void DeleteLeg (legajos_ptr ptr) {
//	legajos_ptr aux;
//
//	while (ptr != NULL) {
//		aux = ptr->next;
//		free(ptr);
//		ptr = aux;
//	}
//}

legajos_ptr AgregarLegajos(legajos_ptr ptr, long nroleg, DATE dia, char cond, int codaus) 
{
	legajos_ptr next;

	if (ptr == NULL) { // Agrego el legajo.
		ptr = (legajos_ptr)malloc(sizeof(legajos_obj));

		ptr->next   = NULL;
		ptr->nroleg = nroleg;
		ptr->dia    = dia;
		ptr->cond   = cond;
		ptr->codaus = codaus;
		ptr->diff   = FALSE;
		return ptr;
	}
	if (nroleg < ptr->nroleg || (nroleg == ptr->nroleg && dia < ptr->dia)) {
		legajos_ptr nuevo;
		nuevo = AgregarLegajos(NULL, nroleg, dia,cond, codaus);
		nuevo->next = ptr;
		return nuevo;
	}
	for (next = ptr; next->next != NULL; next = next->next) {
		if ((next->nroleg == nroleg) && (next->dia == dia)) {
	 		next->diff |= (next->cond!=cond || next->codaus!=codaus);
			return ptr;
		}
		if (nroleg < next->next->nroleg || (nroleg == next->next->nroleg && dia < next->next->dia)) {
			legajos_ptr nuevo;
			nuevo = AgregarLegajos(NULL, nroleg, dia ,cond, codaus);
			nuevo->next = next->next;
			next->next  = nuevo;
			return ptr;
		}
	}
	if ((next->nroleg == nroleg) && (next->dia == dia)) {
		next->diff |= (next->cond!=cond || next->codaus!=codaus);
		return ptr;
	}
	next->next = AgregarLegajos(NULL, nroleg, dia,cond,codaus);
	return ptr;
}

bool CambiaDia(long nroleg, DATE dia)
{
	if ((nroleg != lastleg) || (dia != lastdia)) {
		if (*FmSFld(fm0, SALIDA) == 'A') {
			lastdia = dia;
			lastleg = nroleg;


		}
		else {
			RpSetLFld(rp0, R_LEGAJO,  lastleg);
		}
		lastdia = dia;
		lastleg = nroleg;
		return TRUE;
	}
	return FALSE;
}

void ImprimeTabla(void)
{
	long i;
	char desaus[26];       
	char detalle[50];

	if (cParte <= 0)
		return;


	for (i = bParte; i != -1; i = PxLegxDia[i].pos) {

		sprintf(desaus, "%s", NULL_STR);
		SetKey(asist|INASIST, PxLegxDia[i].codaus);
		if (GetRecord(asist|INASIST, THIS_KEY, IO_NOT_LOCK)!=ERROR)
			sprintf(desaus, "%s", SFld(asist|INASIST_DESCRINAS));

		if (*FmSFld(fm0, SALIDA) == 'A') {
		   	fprintf(fp, "%d\t%ld\t%s\t%ld\t%s\t%d\t%s\t", FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG)), PxLegxDia[i].cliente, GetDescCli( PxLegxDia[i].cliente ), PxLegxDia[i].objetivo, GetObjDescrip(PxLegxDia[i].cliente, PxLegxDia[i].objetivo));
    	    fprintf(fp, "%s\t", TIPO_ERR_COND_DE_TRABAJ);
			sprintf(detalle, "%.1D Pto Serv. %d Pto: %d %s C: %s %s", DFld(operac|PARTE_DIA), PxLegxDia[i].ptoser, PxLegxDia[i].puesto,GetDescPto(PxLegxDia[i].ptoser), &PxLegxDia[i].cond, desaus);
			fprintf(fp, "%s\n", detalle);
		}
		else {               

			RpSetIFld(rp0, R_EMP, FmIFld(fm0, EMP));
			RpSetLFld(rp0, R_LEGAJO, LFld(operac|PARTE_NROLEG));
			RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG)));
			RpSetLFld(rp0, R_CLIENTE,  PxLegxDia[i].cliente);
			RpSetIFld(rp0, R_OBJETIVO, PxLegxDia[i].objetivo);
			RpSetFld (rp0, R_TXTERROR, TIPO_ERR_COND_DE_TRABAJ);
			sprintf(detalle, "%.1D Pto Serv. %d Pto: %d %s C: %s %s", DFld(operac|PARTE_DIA), PxLegxDia[i].ptoser, PxLegxDia[i].puesto,GetDescPto(PxLegxDia[i].ptoser), &PxLegxDia[i].cond, desaus);
			RpSetFld (rp0, R_DETERROR, detalle);
			DoReport(rp0, LINEA);
		}
	}
}

void AgregaATabla(long cliente, int objetivo, int ptoser, int puesto, char cond, int codaus)
{
	long i,n;
	int cmp;

	if (cParte >= MAXPARTE) {
		WiMsg("Se supero la maxima cantidad permitida de clientes para este legajo %ld", LFld(operac|PARTE_NROLEG));
		exit(1);
	}

	PxLegxDia[cParte].cliente=cliente;
	PxLegxDia[cParte].objetivo=objetivo;
	PxLegxDia[cParte].ptoser=ptoser;
	PxLegxDia[cParte].puesto=puesto;
	PxLegxDia[cParte].cond=cond;
	PxLegxDia[cParte].codaus=codaus;
	PxLegxDia[cParte].pos=-1;

	if (bParte == -1) {
		bParte = cParte;
		cParte++;
		return;
	}
	if ((cmp = CmpParte(cParte,bParte)) < 0) {
		PxLegxDia[cParte].pos = bParte;
		bParte = cParte;
		cParte++;
		return;
	}
	if (!cmp)
		return; // es repetido y vuelvo sin mas.

	for (i = bParte, n = PxLegxDia[i].pos; n != -1; i = n, n = PxLegxDia[n].pos) {
		// es igual al siguiente -> vuelvo.
		if ((cmp = CmpParte(cParte,n)) == 0)
			return;

		// es el lugar correcto esta entre i y n.
		if (cmp < 0) {
			PxLegxDia[cParte].pos = PxLegxDia[i].pos;
			PxLegxDia[i].pos      = cParte;
			cParte++;
			return; // ya insertado vuelvo.
		}
	}
	PxLegxDia[i].pos = cParte;
	cParte++;
}

bool HorasNegativas()
{
	if((FFld(operac|PARTE_HSNOR) < 0) || (FFld(operac|PARTE_HS50) < 0) || (FFld(operac|PARTE_HS100F) < 0) || (FFld(operac|PARTE_HS100FE) < 0))
		return TRUE;
		
	return FALSE;	
}

void ImprimirHsNega(long nroleg)
{
	char detalle[50];
	if(rp0 != ERROR){
		RpSetIFld(rp0, R_EMP, FmIFld(fm0, EMP));
		RpSetLFld(rp0, R_LEGAJO, nroleg);
		RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), nroleg));
		RpSetLFld(rp0, R_CLIENTE,  LFld(operac|PARTE_CLIENTE));
		RpSetIFld(rp0, R_OBJETIVO, IFld(operac|PARTE_OBJETIVO));
		RpSetFld (rp0, R_TXTERROR,  TIPO_ERR_HORAS_NEGATIVAS);
		sprintf(detalle, "%.3D Horas(Norm: %.2f al 50: %.2f al 100 franco: %.2f y feri: %.2f )", DFld(operac|PARTE_DIA), FFld(operac|PARTE_HSNOR) / 100, FFld(operac|PARTE_HS50) / 100, FFld(operac|PARTE_HS100F) / 100, FFld(operac|PARTE_HS100FE) / 100);
		RpSetFld (rp0, R_DETERROR, detalle);
		DoReport(rp0, LINEA);
	}   
	
	if(fp != NULL) {  
    	fprintf(fp, "%d\t%ld\t%s\t%ld\t%s\t%d\t%s\t", FmIFld(fm0, EMP), nroleg, GetNombreLeg(FmIFld(fm0, EMP), nroleg), 
    												LFld(operac|PARTE_CLIENTE), GetDescCli(LFld(operac|PARTE_CLIENTE)), 
    												IFld(operac|PARTE_OBJETIVO), GetObjDescrip(LFld(operac|PARTE_CLIENTE) , 
    												IFld(operac|PARTE_OBJETIVO)));
		sprintf(detalle, "%.3D Horas(Norm: %.2f al 50: %.2f al 100 franco: %.2f y feri: %.2f )", DFld(operac|PARTE_DIA), 
					     FFld(operac|PARTE_HSNOR) / 100, FFld(operac|PARTE_HS50) / 100, FFld(operac|PARTE_HS100F) / 100,
					     FFld(operac|PARTE_HS100FE) / 100);
		fprintf(fp, "%s\t", TIPO_ERR_HORAS_NEGATIVAS);
		fprintf(fp, "%s\n", detalle);
	}
	
}

bool DifEnElParte()
{
	double sumhoras = NULL_DOUBLE;
	double hsparte = NULL_DOUBLE;                                                                                          
	
	sumhoras = FFld(operac|PARTE_HSNOR) + FFld(operac|PARTE_HS50) + FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE);
//	hsparte = GetCantHoras(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL));

	hsparte =  ConvHraInt(TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL)) * 100;

//	WiMsg("Sumhoras: %.2f", sumhoras);
//	WiMsg("Horas parte: %.2f", hsparte);

	if(floor(sumhoras) != floor(hsparte))
		return TRUE;
		
	return FALSE;
}

void ImprimirDifenParte(long nroleg)
{
	char detalle[100];
	if(rp0 != ERROR){
		RpSetIFld(rp0, R_EMP, FmIFld(fm0, EMP));
		RpSetLFld(rp0, R_LEGAJO, nroleg );
		RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), nroleg));
		RpSetLFld(rp0, R_CLIENTE,  LFld(operac|PARTE_CLIENTE));
		RpSetIFld(rp0, R_OBJETIVO, IFld(operac|PARTE_OBJETIVO));

		RpSetFld (rp0, R_TXTERROR,  TIPO_ERR_DIF_HS_EN_PARTE);
		sprintf(detalle, "%.3D Horario: %.1T a %.1T Parte total: %.2f", DFld(operac|PARTE_DIA), 
										TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), (FFld(operac|PARTE_HSNOR) + 
										FFld(operac|PARTE_HS50) + FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE)) / 100);
		RpSetFld (rp0, R_DETERROR, detalle);
		DoReport(rp0, LINEA);
	}   
	
	if(fp != NULL) {  
    	fprintf(fp, "%d\t%ld\t%s\t%ld\t%s\t%d\t%s\t", FmIFld(fm0, EMP), nroleg, GetNombreLeg(FmIFld(fm0, EMP), nroleg), 
    												LFld(operac|PARTE_CLIENTE), GetDescCli(LFld(operac|PARTE_CLIENTE)), 
    												IFld(operac|PARTE_OBJETIVO), GetObjDescrip(LFld(operac|PARTE_CLIENTE) , 
    												IFld(operac|PARTE_OBJETIVO)));
		sprintf(detalle, "%.3D Horario: %.1T a %.1T >> Parte total: %.2f", DFld(operac|PARTE_DIA), 
										TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), (FFld(operac|PARTE_HSNOR) + 
										FFld(operac|PARTE_HS50) + FFld(operac|PARTE_HS100F) + FFld(operac|PARTE_HS100FE)) / 100);

		fprintf(fp, "%s\t", TIPO_ERR_DIF_HS_EN_PARTE);
		fprintf(fp, "%s\n", detalle);
	}
}

/*
bool   DifConExcepciones(long p_nroleg, DATE p_dia, long p_cliente, int p_objetivo, double p_hsnor, double p_hs50, double p_hs100, int  p_ptoser, int p_puesto, int p_nroint, int p_condic)
{
  //primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo)
 	SetKey(operac|EXCEPCIONbyEMP, FmIFld(fm0, EMP),p_cliente, p_objetivo, p_dia, p_nroleg, p_ptoser, p_puesto, p_nroint, p_condic, NULL_SHORT);
	if(GetRecord(operac|EXCEPCIONbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		if(!((p_hsnor - FFld(operac|EXCEPCION_HORAS)) >= 0))	return FALSE;
		if(!(( p_hs50 - FFld(operac|EXCEPCION_HS50))  >= 0))	return FALSE;
		if(!((p_hs100 - FFld(operac|EXCEPCION_HS100)) >= 0))	return FALSE;
	}                       
	
	return TRUE;
	
	
}*/

/*
void   ImprimirDifConExcepciones(long p_nroleg)
{
	WiMsg("Imprimio - %ld", p_nroleg);	
}
*/
