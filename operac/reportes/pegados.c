/********************************************************************
* MODULE & VERSION : @(#)pegados.c	1.3
* DATE             : 12/03/28
* TIME             : 18:37:41
*
* CREATED          : 25/02/11    BY FVENTURA
*
* DESCRIPTION:
*             Listado de Partes Pegado
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "pegados.fmh"
#include "pegados.rph"
#include "filial.h"

#define MAX_PUEXDIA 10

/* Funciones privadas */
static void Proceso();
static void AbrirSalida();
static void	ArmarArchivo();
static void	ArmarListado();

bool EsPegado(int p_emp, int p_legajo, DATE p_dia);

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema ope, bill;
char buf[100];

/* Programa principal */
wcmd(pegados, 1.3 03/28/12 )
{
	fm0  = OpenForm("pegados",   FM_EABORT);
	ope  = OpenSchema("operac", IO_EABORT);
	bill = OpenSchema("bill",   IO_EABORT);

	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while(DoForm(fm0, NULLFP,NULLFP) != FM_EXIT) {
		AbrirSalida();
		Proceso();
		if (strcmp(FmSFld(fm0, SALIDA), "A"))
			CloseReport(rp0);
   }
   
   FinObjetivosXusr();   
   FinClientesXusr();
   FinListaXusr();
}

static void Proceso()
{

	DATE diaant=FmDFld(fm0, DIAD);
	long legant=NULL_LONG;
	int canptoxdia=0;

	SetKey(ope|PARTEbyEMPLE, FmIFld(fm0, EMP), FmLFld(fm0, VIGID), FmDFld(fm0, DIAD), MIN_LONG, MIN_SHORT);
	while (GetRecord(ope|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {

		//Filtro por maximo legajo
		if (!FmIsNull(fm0, VIGIH) && LFld(ope|PARTE_NROLEG)>FmLFld(fm0, VIGIH))
			break;

		if (diaant != DFld(ope|PARTE_DIA)) {
			if (canptoxdia>1) {

				PushRecord(ope|PARTE);
				if (EsPegado(IFld(ope|PARTE_EMP), legant, diaant)) {
					PopRecord(ope|PARTE);
					if (!strcmp(FmSFld(fm0, SALIDA), "A")) 
						ArmarArchivo();
					else 
						ArmarListado();
				}
				else
					PopRecord(ope|PARTE);


			}
			canptoxdia=0;
		}

		//Filtros por Fecha
		if (DFld(ope|PARTE_DIA)<FmDFld(fm0, DIAD)) {
			SetKey(ope|PARTEbyEMPLE, IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG), FmDFld(fm0, DIAD), NULL_LONG, NULL_SHORT);
			canptoxdia=0;
			continue;
		} 

		if (DFld(ope|PARTE_DIA)>FmDFld(fm0, DIAH)) {
			SetKey(ope|PARTEbyEMPLE, IFld(ope|PARTE_EMP), LFld(ope|PARTE_NROLEG)+1, FmDFld(fm0, DIAD), NULL_LONG, NULL_SHORT);
			canptoxdia=0;
			continue;
		}

		if (TFld(ope|PARTE_HORAENT)==StrToT("00:00:00") && TFld(ope|PARTE_HORASAL)==StrToT("00:00:00"))
			continue;

		canptoxdia++;
		diaant=DFld(ope|PARTE_DIA);
		legant=LFld(ope|PARTE_NROLEG);


	}
}

bool EsPegado(int p_emp, int p_legajo, DATE p_dia)
{
	TIME maxhfin=MIN_TIME;
	TIME minhini=MAX_TIME;
	TIME hora[MAX_PUEXDIA];
	bool maniana[MAX_PUEXDIA];
	bool maxmaniana=FALSE;
	int v_i=0, v_j=0, v_k=0;
	bool devuelve=FALSE;


	// Busco la hora de entrada del siguiente dia
  	SetKey(ope|PARTEbyEMPLE, p_emp, p_legajo, p_dia+1, MIN_LONG, MIN_SHORT);
	while (GetRecord(ope|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {

		if (TFld(ope|PARTE_HORAENT)==StrToT("00:00:00") && TFld(ope|PARTE_HORASAL)==StrToT("00:00:00"))
			continue;

		if (TFld(ope|PARTE_HORAENT) < minhini) {
			minhini=TFld(ope|PARTE_HORAENT);
		}
	} 


	for (v_i=0; v_i<=MAX_PUEXDIA; v_i++) {
		hora[v_i]=NULL_TIME;
		maniana[v_i]=FALSE;
	}
	v_i=0;

  	SetKey(ope|PARTEbyEMPLE, p_emp, p_legajo, p_dia, MIN_LONG, MIN_SHORT);
	while (GetRecord(ope|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		
		hora[v_i]=TFld(ope|PARTE_HORASAL);
		if (TFld(ope|PARTE_HORASAL)<TFld(ope|PARTE_HORAENT))
			maniana[v_i]=TRUE;
		else
			maniana[v_i]=FALSE;
		v_i++;
	} 

	//Limpia casos de suporpocicion de horas
	for (v_j=0; v_j<MAX_PUEXDIA && hora[v_j]!=NULL_TIME; v_j++) {
		for (v_i=0; v_i<MAX_PUEXDIA && hora[v_i]!=NULL_TIME; v_i++) {
			if (v_j==v_i)
				continue;
			if (hora[v_i]==hora[v_j] && maniana[v_i]==maniana[v_j]) {

				for (v_k=v_j; v_k<MAX_PUEXDIA - 1 ; v_k++){
					hora[v_k]=hora[v_k+1]; 
					maniana[v_k]=maniana[v_k+1];

				}

				continue;
			}
	    }
    }
	v_j=0;

	for (v_i=0; v_i<MAX_PUEXDIA; v_i++) {
		if (hora[v_i] != NULL_TIME)
			v_j++;
		else
			break;
	}

	maxmaniana=FALSE;
	maxhfin=NULL_DATE;
	if (v_j>1) {
		for (v_i=0; v_i<MAX_PUEXDIA && hora[v_i]!=NULL_TIME; v_i++) {
			if (maniana[v_i] && !maxmaniana){
				maxhfin=hora[v_i];
				maxmaniana=maniana[v_i];
			}
			else {
				if (maniana[v_i] == maxmaniana) {
					if (hora[v_i]>maxhfin) {
						maxhfin=hora[v_i];
						maxmaniana=maniana[v_i];
					}
				}
			}
		} 
	}

  	SetKey(ope|PARTEbyEMPLE, p_emp, p_legajo, p_dia, MIN_LONG, MIN_SHORT);
	while (GetRecord(ope|PARTEbyEMPLE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		if (TFld(ope|PARTE_HORASAL)==maxhfin)
			break;
	} 
	if (minhini==maxhfin)
		devuelve= TRUE;


	return (devuelve);

	
}


static void	ArmarListado()
{
	RpSetLFld(rp0, R_LEGAJO,  LFld(ope|PARTE_NROLEG));
	RpSetFld (rp0, R_APYNOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(ope|PARTE_NROLEG)));
	RpSetDFld(rp0, R_DIA,     DFld(ope|PARTE_DIA));
	RpSetLFld(rp0, R_CLI,     LFld(ope|PARTE_CLIENTE));
	if (ExisteCliente(LFld(ope|PARTE_CLIENTE)))
		RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	else 
		RpSetFld (rp0, R_DCLI,    NULL_STR);

	RpSetIFld(rp0, R_OBJ,     IFld(ope|PARTE_OBJETIVO));
	RpSetFld (rp0, R_DOBJ,    GetObjDescrip(LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO)));
	RpSetIFld(rp0, R_PTOSER,  IFld(ope|PARTE_PUESTO));
	RpSetIFld(rp0, R_PUESTO,  IFld(ope|PARTE_NROINT));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(ope|PARTE_PUESTO)));
	DoReport (rp0, LINEA);
}

static void	ArmarArchivo()
{
	char ccliente[50];

	if (ExisteCliente(LFld(ope|PARTE_CLIENTE)))
		sprintf(ccliente, "%s", SFld(bill|CLIENTE_RAZSOC));
	else 
		sprintf(ccliente, "%s", NULL_STR);

	fprintf(fp, "%ld\t%s\t%.3D\t", 
			LFld(ope|PARTE_NROLEG),  
			GetNombreLeg(FmIFld(fm0, EMP), LFld(ope|PARTE_NROLEG)), 
			DFld(ope|PARTE_DIA));
			
	fprintf(fp, "%ld\t%s\t%d\t%s\t%d\t%d\t", 
			LFld(ope|PARTE_CLIENTE), ccliente, 
			IFld(ope|PARTE_OBJETIVO), GetObjDescrip(LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO)),
			IFld(ope|PARTE_PUESTO), 
			IFld(ope|PARTE_NROINT));

	fprintf(fp, "%s\n",	GetDescPto(IFld(ope|PARTE_PUESTO)));
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));

		fprintf(fp, "Vigilador\tNombre\tDia\tCliente\tRazon Social\tObjetivo\tDescripcion\tPuesto\t\tDescripcion Pto\n");
	}
	else {
		rp0 = OpenReport("pegados", RP_EABORT|RP_NOBEGIN);

		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);

		BeginReport(rp0, 1, NULL_STR);
	}
}


