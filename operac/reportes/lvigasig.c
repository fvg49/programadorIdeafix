/********************************************************************
* MODULE & VERSION : @(#)lvigasig.c	1.3
* DATE             : 08/07/17
* TIME             : 13:35:42
*
* CREATED          :
*
* DESCRIPTION:
*      Asignación de Vigiladores por Fecha
*
*********************************************************************/
#include <ideafix.h>
#include "lvigasig.fmh"
#include "lvigasig.rph"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "comerc.sch"
#include "comerc.h"
#include "filial.h"
#include "opedef.h"

#define CLI_INEX   "El Cliente: %ld no existe!."
#define OBJET_INEX "El Objetivo: %d del Cliente: %ld no existe!."
#define PER_INEX   "No existe el Vigilador : %ld de la Empresa : %d (%s)"

#define ERR_ARCHI  "No se pudo abrir el archivo!"

#define ARCHI  0
#define TERM   1
#define IMPRE  2

#define _LIS_ACTUAL		1
#define _LIS_HIS		2
#define _LIS_TOT		3

// Declaraciones de Funciones
static fm_status after(form, fmfield, int);

static void ListaVigAsig();
static void AbrirSalida();

static void SetearCabeceraRepCliObj();
static void SetearCabeceraArchCliObj();

static void LeerCliente (long cliente);
static void LeerVigilador(int emp, long nroleg);
static void LeerObjetivo(long cliente, int objetivo);

static void GuardarCliObjCte();
static void ImprimirCliObjCte();
bool FechasLegEfectivo(int p_emp, long p_nroleg, long cliente, int objetivo, DATE *p_fecdes, DATE *p_fechas);

// Declaraciones globales
form   fm0;
schema operac, bill, comerc, sue;
FILE   *fp=NULL;
report rp0 = ERROR;
bool   salida, impri;
struct s_lisxusr_lib esta_lis;
long cliente, nroleg;
DATE fecdes, fechas;
int objetivo, emp;


/* Programa principal */
wcmd(lasig, 1.23 09/24/04)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	fm0    = OpenForm("lvigasig",    FM_EABORT);

	while (DoForm(fm0, NULLFP, after) != FM_UPDATE) return;
	    impri = FALSE;
	   	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;

		ListaVigAsig();
		
		if (rp0 != ERROR)
			CloseReport(rp0);
		if (fp != NULL)
			fclose(fp);
		fp = NULL;
		rp0 = ERROR;
}

static void ListaVigAsig()
{
	char comen[100];

	
	dbcursor c_per;

	emp = FmIFld(fm0, EMP);
	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	
	if (FmIsNull(fm0, VIG)) {
		SetCursorFrom(c_per, emp, MIN_LONG);
		SetCursorTo  (c_per, emp, MAX_LONG);
	}	
	else {
		SetCursorFrom(c_per, emp, FmLFld(fm0, VIG));
		SetCursorTo  (c_per, emp, FmLFld(fm0, VIG));
	}	
	
	while(FetchCursor(c_per) != ERROR)	{
		nroleg = LFld(sue|PER_NROLEG);
		
		//trae cliente objetivo donde esta efectivo el nroleg a una fecha..
		GetCliObjEfectivo(emp, nroleg, FmDFld(fm0, FECASIG), &cliente, &objetivo);
    	if (cliente == NULL_LONG || objetivo == NULL_SHORT)
	       	continue;
         
		if(!FechasLegEfectivo(emp, nroleg, cliente, objetivo, &fecdes, &fechas))
			continue;

		if (!ValidaFilial(cliente, objetivo, FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		LeerVigilador(emp, nroleg);
		LeerCliente(cliente);
		LeerObjetivo(cliente, objetivo);
		
		sprintf(comen, "Procesando Legajo %ld - %s\n", nroleg, SFld(sue|PER_APYNOM));
		FmSetFld (fm0, COMENTARIO, comen);
		WiRefresh();

		if (!impri) {
	 		AbrirSalida("lvigasig");
			impri = TRUE;
		}	
        
		if (salida == ARCHI)
			GuardarCliObjCte();
		if (salida != ARCHI)
			ImprimirCliObjCte();
	}

	DeleteCursor(c_per);
}

static void AbrirSalida(char nombre[20])
{
	if (salida != ARCHI && rp0 == ERROR) {
		rp0 = OpenReport(nombre, RP_NOBEGIN);
		RpSetOutput(rp0, salida == TERM ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
		SetearCabeceraRepCliObj();
	}
     
	if (salida == ARCHI && (fp == (FILE*)NULL)) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO),"wt")) == (FILE*)NULL)
			Error(ERR_ARCHI);
		else
			SetearCabeceraArchCliObj();
	}
}

static void SetearCabeceraRepCliObj()
{
	RpSetIFld(rp0, R_EMP,    FmIFld(fm0, EMP));
	RpSetFld (rp0, R_DEMP,   FmSFld(fm0, DEMP));
	RpSetDFld(rp0, R_FEC, FmDFld(fm0, FECASIG));
}

static void SetearCabeceraArchCliObj()
{   
	fprintf(fp, "Legajo\tApellido y Nombre\tFecha Desde\tFecha Hasta\tCod.Cliente\tRaz.Soc.\tCod.Objetivo\tObjetivo\t");
	fprintf(fp, "Delegación Comercial\tDescripción\tDelegación Operacional\tDescripción\tFilial\tDescrición de Filial\n");
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case EMP:
        break;
        case SALIDA:
			if (*FmSFld(fm0, SALIDA) == 'A')
		        FmSetFld(fm0, ARCHIVO, "lvigasig.txt");
		    else
		        FmSetFld(fm0, ARCHIVO, NULL_STR);
		break;
        
	}
	return FM_OK;
}

static void GuardarCliObjCte()
{
	fprintf(fp, "%ld\t%s\t%.3D\t%.3D\t%ld\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", nroleg, SFld(sue|PER_APYNOM), fecdes, fechas, cliente, SFld(bill|CLIENTE_RAZSOC), 
	       objetivo, GetObjDescrip(cliente, objetivo), SFld(comerc|OBJETIVO_DELEG), GetDescDeleg(SFld(comerc|OBJETIVO_DELEG)), SFld(comerc|OBJETIVO_DELEGA), 
	       GetDescDeleg(SFld(comerc|OBJETIVO_DELEGA)), SFld(comerc|OBJETIVO_FILIAL), GetDescFilial(SFld(comerc|OBJETIVO_FILIAL)));
}

static void ImprimirCliObjCte()
{
	RpSetLFld(rp0, R_LEGAJO,  nroleg);
	RpSetFld (rp0, R_APENOM,  SFld(sue|PER_APYNOM));
	RpSetDFld(rp0, R_FECASIG, fecdes);
	RpSetDFld(rp0, R_FECHAS,  fechas);
	RpSetLFld(rp0, R_CLI,     cliente);
	RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, R_OBJ,     objetivo);
	RpSetFld (rp0, R_DOBJ,    GetObjDescrip(cliente, objetivo));
    RpSetFld (rp0, R_FILIAL, SFld(comerc|OBJETIVO_FILIAL));
    RpSetFld (rp0, R_DFILIAL, GetDescFilial(SFld(comerc|OBJETIVO_FILIAL)));
    
	DoReport(rp0, LINEA);
}

static void LeerCliente (long cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(CLI_INEX, cliente);
	}
}

static void LeerObjetivo(long cliente, int objetivo)
{
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(OBJET_INEX, cliente, objetivo);
	}
}

static void LeerVigilador(int emp, long nroleg)
{
	SetKey(sue|PERbyEMP, emp, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(PER_INEX, nroleg, emp, "PER");
	}
	SetKey(sue|DATPERSbyEMP, emp, nroleg);
	if (GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(PER_INEX, nroleg, emp, "DATPERS");
	}
}


/*--------------* FechasLegEfectivo *-----------------*/
bool FechasLegEfectivo(int p_emp, long p_nroleg, long p_cliente, int p_objetivo, DATE *p_fecdes, DATE *p_fechas)
{
	bool	 encontre = FALSE;
	dbcursor c_asig, c_asigh;

	c_asig	= CreateCursor(operac|ASIGbyEMP,  IO_NOT_LOCK);
	c_asigh	= CreateCursor(operac|ASIGHbyPUESTO, IO_NOT_LOCK);

	*p_fecdes = NULL_DATE;
	*p_fechas = NULL_DATE;
	
	SetCursorFrom(c_asig, p_emp, p_cliente, p_objetivo, p_nroleg, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, p_emp, p_cliente, p_objetivo, p_nroleg, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (!encontre && FetchCursor(c_asig) != ERROR) {
		if (strcmp(SFld(operac|ASIG_EFECT), EFECTIVO)) {
			continue;
		}
        
        encontre = TRUE;
        *p_fecdes = DFld(operac|ASIG_FECASIG);
        *p_fechas = DFld(operac|ASIG_FECHAS);
        break;
	}

	if (!encontre) {
		SetCursorFrom(c_asigh, p_emp, p_cliente, p_objetivo, p_nroleg, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asigh, p_emp, p_cliente, p_objetivo, p_nroleg, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (strcmp(SFld(operac|ASIGH_EFECT), EFECTIVO))
				continue;
			
			encontre = TRUE;
			*p_fecdes = DFld(operac|ASIGH_FECALT);
	        *p_fechas = DFld(operac|ASIGH_FECBAJ);
    	    break;
		}
	}

 	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);

	return encontre;
}

