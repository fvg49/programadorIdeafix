/********************************************************************
* MODULE & VERSION : @(#)lnoparte.c	1.15
* DATE             : 09/07/06
* TIME             : 11:54:12
*
* CREATED          :
*
* DESCRIPTION:
*      Vigiladores con Asignaciones pero sin Parte
*
*********************************************************************/
#include <ideafix.h>
#include "lnoparte.fmh"
#include "lnoparte.rph"
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
#define REGCABE   "Emp\tLegajo\tApellido y Nombre\tDia\tVigil\tCliente\tDescripción del Cliente\tObjetivo\tDescripción del Objetivo\tDia1\tDia2\tDia3\tDia4\tDia5\tDia6\tDia7\n"
#define REGARCH   "%d\t%ld\t%s\t%.3D\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n"

// Retorna el subindice segun la letra del dia, para el vector de validacion de frecuencia
#define	subdia(dia)	    (dia == 'L' ? 0 : dia == 'M' ? 1 : dia == 'X' ? 2 :	dia == 'J' ? 3 : dia == 'V' ? 4 : dia == 'S' ? 5 : dia == 'D' ? 6 : dia == 'P' ? 7 : NULL_SHORT)

// Dado el subindice retorna la letra del dia a la que corresponde.
#define	diasub(i)	    (i == 0 ? "L" : i == 1 ? "M" : i == 2 ? "X" : i == 3 ? "J" : i == 4 ? "V" : i == 5 ? "S": i == 6 ? "D" : i == 7 ? "P" : NULL_STR)

schema operac, bill, comerc, sue;
form   fm0;
FILE   *fp = NULL;
report rp0 = ERROR;
bool   salida;
dbcursor c_per;
char nomdias[9][2];

int g_emp;
DATE fecierre;


static fm_status before(form, fmfield, int);
static fm_status after(form fm, fmfield fno, int row);
static void Asig(long vigilador, tnfecxleg nodo_aux_fec);
static void AsigH(long vigilador, tnfecxleg nodo_aux_fec);
static void ImprimirInfo(int emp, long nroleg, DATE dia, char * vigil, long cliente, int objetivo);
static void AbrirSalida();

/* Programa principal */
wcmd(lnoparte, 1.5 01/15/08)
{
	tnlegxfil nodo_aux;

	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	fm0    = OpenForm("lnoparte",  FM_EABORT);

	// Inicio Permisos

	g_emp= StrToI(getenv("emp"));

	FmSetFld(fm0, COMENT ,"[1mCargando Permisos Sobre Clientes - Objetivos[0m");
	WiRefresh();

	InicListaXusrCO(g_emp);
	FmSetFld(fm0, COMENT ,"Cargando Permisos Sobre Legajos");
	WiRefresh();

	fecierre  = GetFechaCierreOpe(g_emp);

	InicLegajoXusr (g_emp, fecierre, fm0, COMENT, TRUE, MAX_SHORT);
	WiRefresh();

//	ImprimeLegajoXusr();


	if (DoForm(fm0, before, after) != FM_UPDATE) return;

	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;
	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	AbrirSalida();

	for (nodo_aux=inileg; nodo_aux!=NULL; nodo_aux=(*nodo_aux).nsig) {
		SetCursorFrom(c_per, FmIFld(fm0, EMP), (*nodo_aux).legxfil);
		SetCursorTo  (c_per, FmIFld(fm0, EMP), (*nodo_aux).legxfil);
		while (FetchCursor(c_per) != ERROR) {

			if (!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)))
				continue;
				
			//if (LFld(sue|PER_CODCCOS) >= 10000 && LFld(sue|PER_CODCCOS) < 100000)
			//	continue;

            if (!(LFld(sue|PER_CODCCOS)>=90000 && LFld(sue|PER_CODCCOS) < 999999))
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
	        
			Asig(LFld(sue|PER_NROLEG), (*nodo_aux).nfecxleg);
			AsigH(LFld(sue|PER_NROLEG), (*nodo_aux).nfecxleg);
		}
	}

	FinListaXusr ();
	FinLegajoXusr ();

}

static void Asig(long vigilador, tnfecxleg nodo_aux_fec)
{
	dbcursor c_asig;
	dbfield  campo_dia;

	bool leoparte = FALSE, dias_trabajados[9];
	int v_i=0, v_procede=NULL_SHORT;
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

			v_procede=GetProcedeXFecha(nodo_aux_fec, dia);
			
			if (v_procede==NULL_SHORT)
				continue;

			if (v_procede==_PROCED_GRUFIL)
				if (!ValidaObjetivoXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), IFld(operac|ASIG_EMP)))
		       		continue;
			
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
				   	   	ImprimirInfo(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), dia, SFld(operac|ASIG_VIGIL), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
					}
			    }
			}	
	    }
	}
}

static void AsigH(long vigilador, tnfecxleg nodo_aux_fec)
{
	dbcursor c_asigh;
	dbfield  campo_dia;

	bool leoparte = FALSE, dias_trabajados[9];
	int v_i=0, v_procede=NULL_SHORT;
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

			v_procede=GetProcedeXFecha(nodo_aux_fec, dia);
			
			if (v_procede==NULL_SHORT)
				continue;

			if (v_procede==_PROCED_GRUFIL)
				if (!ValidaObjetivoXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), IFld(operac|ASIGH_EMP)))
		       		continue;
			
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
				   	   	ImprimirInfo(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_NROLEG), dia, SFld(operac|ASIGH_VIGIL), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
					}
			    }
			}	
	    }
	}
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));

   	   	fprintf(fp, REGCABE);
	}
	else {
		rp0 = OpenReport("lnoparte", RP_NOBEGIN|RP_EABORT, 1);
		RpSetOutput(rp0, (!strcmp(FmSFld(fm0, SALIDA), "I") ? RP_IO_DEFAULT : RP_IO_TERM), NULL_STR);

		if (BeginReport(rp0, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
		
		RpSetDFld (rp0, R_FDESDE, FmDFld (fm0, FDESDE));
		RpSetDFld (rp0, R_FHASTA, FmDFld (fm0, FHASTA));
		if (!FmIFld(fm0, BAJA))
			RpSetIFld (rp0, RBAJA, FmIFld(fm0, BAJA));
	}
}

static void ImprimirInfo(int emp, long nroleg, DATE dia, char * vigil, long cliente, int objetivo)
{

	if (rp0 != ERROR) {
		RpSetIFld(rp0, R_EMP, emp);
		RpSetLFld(rp0, R_LEGAJO, nroleg);
		RpSetFld (rp0, R_APENOM, GetNombreLeg(emp, nroleg));
		RpSetDFld(rp0, R_DIA, dia);
		RpSetFld(rp0, R_VIGIL, vigil);
		RpSetLFld(rp0, R_CLIENTE, cliente);
		RpSetIFld(rp0, R_OBJETIVO, objetivo);
		RpSetFld (rp0, R_DIA1,   nomdias[0]);
		RpSetFld (rp0, R_DIA2,   nomdias[1]);
		RpSetFld (rp0, R_DIA3,   nomdias[2]);
		RpSetFld (rp0, R_DIA4,   nomdias[3]);
		RpSetFld (rp0, R_DIA5,   nomdias[4]);
		RpSetFld (rp0, R_DIA6,   nomdias[5]);
		RpSetFld (rp0, R_DIA7,   nomdias[6]);
		DoReport(rp0, LINEA);
	}

	if (fp != NULL)
	   	fprintf(fp, REGARCH, emp, nroleg, GetNombreLeg(emp, nroleg), dia, vigil, cliente, GetDescCli(cliente), objetivo, GetObjDescrip(cliente, objetivo), nomdias[0], nomdias[1], nomdias[2], nomdias[3], nomdias[4], nomdias[5], nomdias[6]);
}

static fm_status before (form fm, fmfield fno, int row)
{
	char fdes[8];
	
	sprintf(fdes, "%02d%01d%04d\n", 1, Month(Today()), Year(Today()));
	
	switch (fno) {
	case EMP:
		FmSetDFld(fm, FDESDE, StrToD(fdes)); //seteo la fecha en before emp para tener como default la fecha desde en el form.
 	break;
	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "lnoparte.txt");
	break;
	}
	return FM_OK;
}


static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm)) {
			FmSetFld(fm0, COMENT ,"[1mCargando Permisos Sobre Clientes - Objetivos[0m");
			WiRefresh();
			InicListaXusrCO(FmIFld(fm0, EMP));
			FmSetFld(fm0, COMENT ,"Cargando Permisos Sobre Legajos");
			WiRefresh();

			InicLegajoXusr (FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), fm0, COMENT, TRUE, MAX_SHORT);
	    }
    break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	case FDESDE:
	    if (FmChgFld(fm))
			InicLegajoXusr (FmIFld(fm0, EMP), FmDFld(fm0, FDESDE), fm0, COMENT, TRUE, MAX_SHORT);

	break;
	}
	return FM_OK;				
}	

