/********************************************************************
* MODULE & VERSION : @(#)lnoasig.c	1.1
* DATE             : 11/01/05
* TIME             : 10:59:08
*
* CREATED          :
*
* DESCRIPTION:
*      Asignación de Vigiladores sin requerimientos.
*
*********************************************************************/
#include <ideafix.h>
#include "lnoasig.fmh"
#include "lnoasig.rph"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "comerc.sch"
#include "comerc.h"
#include "filial.h"

#define ERR_ARCHI  "No se pudo abrir el archivo!"
#define ARCHI     0
#define TERM      1
#define IMPRE     2
#define _8DIAS    8
#define MAXVIG 10000
#define _SOLO_ASIGNADO 2

// Retorna el subindice segun la letra del dia, para el vector de validacion de frecuencia
#define	subdia(dia)	    (dia == 'L' ? 0 : dia == 'M' ? 1 : dia == 'X' ? 2 :	dia == 'J' ? 3 : dia == 'V' ? 4 : dia == 'S' ? 5 : dia == 'D' ? 6 : dia == 'P' ? 7 : NULL_SHORT)

// Dado el subindice retorna la letra del dia a la que corresponde.
#define	diasub(i)	    (i == 0 ? "L" : i == 1 ? "M" : i == 2 ? "X" : i == 3 ? "J" : i == 4 ? "V" : i == 5 ? "S": i == 6 ? "D" : i == 7 ? "P" : NULL_STR)

/* Declaraciones de Estructuras */
struct vig {
	long  nroleg;
	DATE  fecing;
	long  cliente;
	short objetivo;
	char  regimen[9];
	char  dia1[2];
	char  dia2[2];
	char  dia3[2];
	char  dia4[2];
	char  dia5[2];
	char  dia6[2];
	char  dia7[2];
	int   prov;
	int   pais;
	long  local;
}pvig[MAXVIG], *uvig=pvig, *evig;

schema operac, bill, comerc, sue;
form   fm0;
FILE   *fp = NULL;
report rp0 = ERROR;
bool   salida;
char   dian[9][2];
int    dias, difdias, g_emp;
dbcursor c_per;
DATE fecierre;

static fm_status before(form, fmfield, int);
static fm_status after(form, fmfield, int);
static bool Asig(long vigilador, int v_dias, int *v_difdias, char *regimen, DATE *fecasig);
static void AsigH(long vigilador, int difdiass, char *regimen, DATE fecasig);
static void InsertarEnLista(long cliente, int objetivo, long nroleg, char *dia1, char *regimen,
							char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7,
							DATE fecing, int prov, int pais, long local);
private int ordvig(struct vig *a, struct vig *b);
static void ImprimirInfo();
static void Inicializar();

/* Programa principal */
wcmd(lnoasig, 1.1 01/05/11)
{
	char regimen[15];
	DATE fecasig;
	tnlegxfil nodo_aux;

	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	fm0    = OpenForm("lnoasig",  FM_EABORT);



	// Inicio Permisos

	g_emp= StrToI(getenv("emp"));
	fecierre  = GetFechaCierreOpe(g_emp);

	InicLegajoXusr (g_emp, fecierre, fm0, DPROCVIG, FALSE, MAX_SHORT);
	WiRefresh();

//	ImprimeLegajoXusr();



	if (DoForm(fm0, before, after) != FM_UPDATE) return;

	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;

	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);

	uvig = pvig;

	for (nodo_aux=inileg; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig) {
//	if ((*nodo_aux).legxfil != 51852)
//		continue;

		SetCursorFrom(c_per, FmIFld(fm0, EMP), (*nodo_aux).legxfil);
		SetCursorTo  (c_per, FmIFld(fm0, EMP), (*nodo_aux).legxfil);
		while (FetchCursor(c_per) != ERROR) {
//			if (!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)))
//				continue;

			if (!EsVigilador(FmIFld(fm0, EMP), IFld(sue|PER_RELACION), LFld(sue|PER_CODCCOS)))
				continue;

			if (FmIFld(fm0, BAJA)) {
				if (!IsNull(sue|PER_FECEGR) && DFld(sue|PER_FECEGR) < FmDFld(fm0, FDESDE))
					continue;
			}
			else {
				if (!IsNull(sue|PER_FECEGR))
					continue;
			}

			FmSetLFld(fm0, PROCVIGI, LFld(sue|PER_NROLEG));

			FmSetFld (fm0, DPROCVIG, SFld(sue|PER_APYNOM));
			WiRefresh();

			Inicializar();

			if (Asig(LFld(sue|PER_NROLEG), dias, &difdias, regimen, &fecasig)) {
				AsigH(LFld(sue|PER_NROLEG), difdias, regimen, fecasig);
				
			}
		}
	}
	//Ordeno lista de vigiladores.
	qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)ordvig);

	ImprimirInfo();
	
}

static void Inicializar()
{
	int v_i;
	
	dias    = 0;
	difdias = 0;

	for (v_i=0; v_i<9; v_i ++)
		sprintf(dian[v_i], "%s", NULL_STR);
}

static bool Asig(long vigilador, int v_dias, int *v_difdias, char *regimen, DATE *fecasig)
{
	dbcursor c_asig;
	bool     asig = FALSE, estaok = FALSE;
	int      cantdias = 0;

	// variables nuevas
	int v_i=0;
	dbfield  campo_dia;
	bool dias_trabajados[9];

	// inicializo "dias_trabajados" en False
	for (v_i=0; v_i<9; v_i ++)
		 dias_trabajados[v_i]=FALSE;

	c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if (DFld(operac|ASIG_FECASIG) > FmDFld(fm0, FHASTA)) {
			asig = TRUE;
			continue;
		}

		if (!strcmp(SFld(operac|ASIG_EFECT), PROVISORIO) && DFld(operac|ASIG_FECHAS) < FmDFld(fm0, FDESDE))
			continue;

	
		if (!strcmp(SFld(operac|ASIG_EFECT), EFECTIVO)) {
			strcpy(regimen, SFld(operac|ASIG_REGIM));
			*fecasig = DFld(operac|ASIG_FECASIG);
			SetFld(comerc|REGIMEN_REGIM, SFld(operac|ASIG_REGIM));
			if (GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK) != ERROR)
				v_dias = IFld(comerc|REGIMEN_DIAS);

		}
        
		asig = TRUE;

		// Marco en vector dias_trabajados los numeros asignados
		for (campo_dia=ASIG_DIA1; campo_dia<=ASIG_DIA7; campo_dia++) 
			if (strcmp(SFld(operac|campo_dia), NULL_STR)!=0) 
	   			dias_trabajados[subdia(*SFld(operac|campo_dia))]=TRUE;

	}


	//Cuento cuantos dias estan asignados
	for (v_i=0; v_i<9; v_i ++)
		if(dias_trabajados[v_i]) {
			cantdias ++;
			sprintf(dian[v_i], "%s", diasub(v_i));
		}

	//El regimen 8x4x2 tiene mas dias que los dias asignados (como max 7 dias) por eso se suma 1.
	if (cantdias >= v_dias || (cantdias + 1 == _8DIAS))
 		estaok = TRUE;


	if (!asig) {
		InsertarEnLista(NULL_LONG, NULL_SHORT, vigilador, regimen, NULL_STR, NULL_STR, NULL_STR, NULL_STR,
						NULL_STR, NULL_STR, NULL_STR, DFld(sue|PER_FECING), IFld(sue|PER_PROV), IFld(sue|PER_CODPAIS), LFld(sue|PER_LOCAL));
		return FALSE;
	}
    
    if (asig && FmIFld(fm0, MOSTRAR)==_SOLO_ASIGNADO)
    	estaok = TRUE;

	if (asig && !estaok) {
		*v_difdias = v_dias - cantdias;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

static void AsigH(long vigilador, int v_difdias, char *regimen, DATE fecasig)
{
	dbcursor c_asigh;
	bool     asig = FALSE;
	int      cantdias = 0;

	// variables nuevas
	int v_i=0;
	dbfield  campo_dia;
	bool dias_trabajados[9];

	// inicializo "dias_trabajados" en False
	for (v_i=0; v_i<9; v_i ++)
		 dias_trabajados[v_i]=FALSE;

	c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_asigh) != ERROR) {

		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE)
			continue;
		if (DFld(operac|ASIGH_FECBAJ) < FmDFld(fm0, FHASTA) || DFld(operac|ASIGH_FECBAJ) < fecasig)
			continue;
		if (DFld(operac|ASIGH_FECALT) > FmDFld(fm0, FHASTA))
			continue;
		if (!strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO) && DFld(operac|ASIGH_FECHAS) < FmDFld(fm0, FDESDE))
			continue;
		
		asig = TRUE;

		// Marco en vector dias_trabajados los numeros asignados
		for (campo_dia=ASIGH_DIA1; campo_dia<=ASIGH_DIA7; campo_dia++) 
			if (strcmp(SFld(operac|campo_dia), NULL_STR)!=0) 
				dias_trabajados[subdia(*SFld(operac|campo_dia))]=TRUE;
	}

	//Cuento cuantos dias estan asignados
	for (v_i=0; v_i<9; v_i ++)
		if(dias_trabajados[v_i]) {
			cantdias ++;
			sprintf(dian[v_i], "%s", diasub(v_i));
		}
    
	if (!asig || (cantdias != v_difdias && FmIFld(fm0, MOSTRAR)==_SOLO_ASIGNADO)) {

		InsertarEnLista(NULL_LONG, NULL_SHORT, vigilador, regimen, dian[0], dian[1], dian[2], dian[3],
		                dian[4], dian[5], dian[6], DFld(sue|PER_FECING), IFld(sue|PER_PROV), IFld(sue|PER_CODPAIS), LFld(sue|PER_LOCAL));
	}
}


static void InsertarEnLista(long cliente, int objetivo, long nroleg, char *regimen, char *dia1, 
							char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7,
							DATE fecing, int prov, int pais, long local)
{
	for (evig = pvig; evig < uvig; evig++);
	
	if (evig == uvig) {
		if (uvig == &pvig[MAXVIG])
			Error("Tabla interna saturada. Max %d", MAXVIG);

		uvig->nroleg   = nroleg;
		uvig->cliente  = cliente;
		uvig->objetivo = objetivo;
		uvig->fecing   = fecing;
		strcpy(uvig->regimen, regimen);
		strcpy(uvig->dia1, dia1);
		strcpy(uvig->dia2, dia2);
		strcpy(uvig->dia3, dia3);
		strcpy(uvig->dia4, dia4);
		strcpy(uvig->dia5, dia5);
		strcpy(uvig->dia6, dia6);
		strcpy(uvig->dia7, dia7);
		uvig->prov = prov;
		uvig->pais = pais;
		uvig->local = local;
		uvig ++;
	} 
}

private int ordvig(struct vig *a, struct vig *b)
{
	return	a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
			strcmp(a->regimen, b->regimen) < 0 ? -1 : strcmp(a->regimen, b->regimen) > 0 ? -1 :
			0;
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));

		fprintf(fp, "Legajo\tApellido y Nombre\tFecha Ingreso\tRegimen\tDia1\tDia2\tDia3\tDia4\tDia5\tDia6\tDia7\tNro Prov.\tProvincia\tNro. Local\tLocalidad\n");
	}
	else {
		rp0 = OpenReport("lnoasig", RP_NOBEGIN|RP_EABORT, 1);
		RpSetOutput(rp0, (!strcmp(FmSFld(fm0, SALIDA), "I") ? RP_IO_DEFAULT : RP_IO_TERM), NULL_STR);

		if (BeginReport(rp0, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
	}
}

static void ImprimirInfo()
{
	if (rp0 == ERROR && fp == NULL)
		AbrirSalida();

	if (rp0 != ERROR) {
		RpSetDFld (rp0, R_FDESDE, FmDFld (fm0, FDESDE));
		RpSetDFld (rp0, R_FHASTA, FmDFld (fm0, FHASTA));
		if (!FmIFld(fm0, BAJA))
			RpSetIFld (rp0, RBAJA, FmIFld(fm0, BAJA));
	}
	for (evig = pvig; evig < uvig; evig++) {
		if (evig == NULL) {
			Warning("No hay datos para emitir el listado");
			return;
		}

		if (rp0 != ERROR) {
			RpSetLFld(rp0, R_LEGAJO, evig->nroleg);
			RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg));
			RpSetDFld(rp0, R_FECING, evig->fecing);
			RpSetFld (rp0, R_REGIM,  evig->regimen);
			RpSetFld (rp0, R_DIA1,   evig->dia1);
			RpSetFld (rp0, R_DIA2,   evig->dia2);
			RpSetFld (rp0, R_DIA3,   evig->dia3);
			RpSetFld (rp0, R_DIA4,   evig->dia4);
			RpSetFld (rp0, R_DIA5,   evig->dia5);
			RpSetFld (rp0, R_DIA6,   evig->dia6);
			RpSetFld (rp0, R_DIA7,   evig->dia7);
			RpSetIFld(rp0, R_PROV,	 evig->prov);
			RpSetFld (rp0,  R_DPROV,	 GetDescProv(evig->pais, evig->prov));
			RpSetLFld(rp0, R_LOCAL,	 evig->local);
			RpSetFld (rp0,  R_DLOCAL,	 GetDescLocali(evig->pais, evig->prov, evig->local));

			DoReport(rp0, LINEA);
		}
		
		if (fp != NULL)	{//1   2    3    4   5   6   7   8   9   0   1
	   		fprintf(fp, "%ld\t%s\t%.3D\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t",
					evig->nroleg, GetNombreLeg(FmIFld(fm0, EMP), evig->nroleg), evig->fecing, evig->regimen,
					evig->dia1, evig->dia2, evig->dia3, evig->dia4, evig->dia5, evig->dia6, evig->dia7);
			fprintf(fp, "%d\t%s\t", evig->prov, GetDescProv(evig->pais, evig->prov));
			fprintf(fp, "%ld\t%s\n", evig->local, GetDescLocali(evig->pais, evig->prov, evig->local));
		}			
	}
}

static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lnoasig.txt");
	break;
	}
	return FM_OK;
}

static fm_status after (form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP   :
		if (FmChgFld(fm)) {
			InicLegajoXusr (FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), fm0, DPROCVIG, FALSE, MAX_SHORT);
		//	ImprimeLegajoXusr();
		}
		break;
	case FDESDE:
		if (FmChgFld(fm) && FmDFld(fm0, FDESDE)<fecierre) {
			fecierre= FmDFld(fm0, FDESDE);
			InicLegajoXusr (FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), fm0, DPROCVIG, FALSE, MAX_SHORT);
		//	ImprimeLegajoXusr();
		}
		break;

	}
	return FM_OK;
}

