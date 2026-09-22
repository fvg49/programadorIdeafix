/********************************************************************
*
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : 20/09/99
*
* DESCRIPTION:
*      Confirmación del Parte Diario en forma general.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "confpar.fmh"
#include "comerc.sch"
#include "operac.sch"
#include "ambiente.h"
#include "filial.h"
#include "comerc.h"
#include "disthspro.h"
#include "webinter.h"

static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void Proceso(void);
//extern double ConvHraInt();
static void AbrirArchLog();

/* Declaraciones globales */
form fm0;
schema oper, com;
long legajo_desde=MAX_LONG, legajo_hasta=MIN_LONG;
struct s_lisxusr_lib esta_lis;
FILE *archlog = (FILE *)NULL;
bool archOK = FALSE;
DATE fecierrefil;
char filial[7] = {'\0'};
char g_prog[20];


//-------------------------------------------------------<Comienzo de declaraciones p/listas enlazadas>------------------------------------//
typedef struct stndia * tndia;
typedef struct stnlegajo * tnlegajo;

typedef struct stndia {
	DATE	dia;
	tnlegajo	nlegajo;
	tndia	nsig;
} stndia;

typedef struct stnlegajo {
	long	legajo;
	tnlegajo	nsig;
} stnlegajo;

/* Funciones Privadas */
static tndia AcuNDia(tndia, tndia*);
static tnlegajo AcuNLegajo(tnlegajo, tnlegajo*);

static void LisNDia(tndia);
static void LisNLegajo(tnlegajo);

static void BorNDia(tndia);
static void BorNLegajo(tnlegajo);

tndia	g_inicio;

long	legajo;
DATE	dia;
//-------------------------------------------------------<Fin de declaraciones p/listas enlazadas>-----------------------------------------//

wcmd(confpar, %I% %G% )
{
	fm0  = OpenForm  ("confpar", FM_EABORT);
	oper = OpenSchema("operac",  IO_EABORT);
	com  = OpenSchema("comerc",  IO_EABORT);

	sprintf(g_prog, "%s", argv[0]);

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ingresar al programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}

	InicListaXusr(StrToI(ReadEnv("emp")));
	
	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ejecutar el programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}



	BeginTransaction();

	AbrirArchLog();
	Proceso();

	if (archOK == TRUE) {
		fprintf(archlog, "Fin del Proceso %.3D - %.3T\n\n", Today(), Hour());
		archOK = FALSE;
		fclose(archlog);
	}	

	EndTransaction();
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void Proceso(void)
{
	char tipvig[2], buffer[50];
	int pais, prov, hsnor, hs50, hs100f, hs100fe, hstot, numfran;
	char regimen[15];

	dbcursor Par = (dbcursor) ERROR;
	Par = CreateCursor(oper|PARTEbyEMP, IO_NOT_LOCK);
 
 	g_inicio = NULL; 
  	dia = NULL_DATE;
  	legajo = NULL_LONG;


	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis))	{ 

	    if (esta_lis.cliente < FmLFld(fm0, CLIED))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIEH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIED) && esta_lis.objetivo < FmIFld(fm0, OBJETD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIEH) && esta_lis.objetivo > FmIFld(fm0, OBJETH))
	    	continue;

		SetCursorFrom(Par, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAD),
						   MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (Par, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FECHAH),
						   MAX_LONG, MAX_SHORT, MAX_SHORT, MIN_SHORT);
		while(FetchCursor(Par) != ERROR) {
			if (!ValidaFilial(LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO), NULL_STR, NULL_STR, FmSFld(fm0, FFILIAL)))
				continue;
			
			if (DFld(oper|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(oper|PARTE_DIA) > FmDFld(fm0, FECHAH)) 
				continue;

			strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO)));	
		    fecierrefil = GetFechaCierreFilial(filial);
        
	        if (fecierrefil != NULL_DATE && (FmDFld(fm0, FECHAD) <= fecierrefil || FmDFld(fm0, FECHAH) <= fecierrefil)) {
				Warning("El Parte está cerrado al %.3D para el Cliente %ld Objetivo %d Filial %s.\n Debe ingresar una fecha mayor al %.3D", 
						fecierrefil, LFld(oper|ASIGH_CLIENTE), IFld(oper|ASIGH_OBJETIVO), filial, fecierrefil);
				continue;
			}
		
			if (LFld(oper|PARTE_NROLEG) > legajo_hasta) 
				legajo_hasta = LFld(oper|PARTE_NROLEG);

			if (LFld(oper|PARTE_NROLEG) < legajo_desde) 
				legajo_desde = LFld(oper|PARTE_NROLEG);

			
			hsnor = hs50 = hs100f = hs100fe = 0.0;
//			if (IFld(oper|PARTE_HSNOR)  == 0.0 && IFld(oper|PARTE_HS50)    == 0.0 && 
//				IFld(oper|PARTE_HS100F) == 0.0 && IFld(oper|PARTE_HS100FE) == 0.0 &&
//				!IsNull(oper|PARTE_HORAENT)    && !IsNull(oper|PARTE_HORASAL)) {
				sprintf (buffer, "Leyendo Cliente %ld Objetivo %d", LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO));
    			FmSetFld (fm0, COMENT, buffer);
				WiRefresh();
	
				fprintf(archlog, "%d\t%ld\t%d\t%d\n", IFld(oper|PARTE_EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO), LFld(oper|PARTE_NROLEG));
//				fprintf(stderr, "%d\t%ld\t%d\t%d\n", IFld(oper|PARTE_EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO), LFld(oper|PARTE_NROLEG));

//				if (ExisteEnAsig(FmIFld(fm0, EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO),
//								 LFld(oper|PARTE_NROLEG), DFld(oper|PARTE_DIA), IFld(oper|PARTE_PTOSER),
//								 IFld(oper|PARTE_PUESTO), IFld(oper|PARTE_NROINT), TRUE) ||
//					ExisteEnAsigh(FmIFld(fm0, EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO),
//								 LFld(oper|PARTE_NROLEG), DFld(oper|PARTE_DIA), IFld(oper|PARTE_PTOSER),
//								 IFld(oper|PARTE_PUESTO), IFld(oper|PARTE_NROINT),TRUE)) {

					sprintf(tipvig, "%s", TipoVig(FmIFld(fm0, EMP), LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO),
												  LFld(oper|PARTE_NROLEG), IFld(oper|PARTE_PTOSER),
												  IFld(oper|PARTE_PUESTO), IFld(oper|PARTE_NROINT),
												  DFld(oper|PARTE_DIA)));


					SetKey(com|OBJETIVO, LFld(oper|PARTE_CLIENTE), IFld(oper|PARTE_OBJETIVO));
					(void) GetRecord(com|OBJETIVO, THIS_KEY, IO_NOT_LOCK);
					pais    = IFld(com|OBJETIVO_PAIS);
					prov    = IFld(com|OBJETIVO_PROV);
					hstot   = ConvHraInt(TFld(oper|PARTE_HORAENT), TFld(oper|PARTE_HORASAL)) * 100;
					numfran = GetNumFrancoEfectivo(FmIFld(fm0, EMP), LFld(oper|PARTE_NROLEG), DFld(oper|PARTE_DIA));
					GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(oper|PARTE_NROLEG), regimen, DFld(oper|PARTE_DIA));



				  	dia    = DFld(oper|PARTE_DIA);
				  	legajo = LFld(oper|PARTE_NROLEG);

					g_inicio = AcuNDia(g_inicio, &g_inicio);
//				}
//			}
		}
	}

	LisNDia(g_inicio);
	BorNDia(g_inicio);
	
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	
	DeleteCursor(Par);
}


static fm_status after(form fm, fmfield fno, int row)
{
	DATE fecierre;

	switch (fno) {
	case EMP:
		if (FmChgFld(fm))
			InicListaXusr(FmIFld(fm0, EMP));
    break;
	case FECHAD:
	case FECHAH:
		fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));

		if (FmDFld(fm, FECHAD) <= fecierre || FmDFld(fm, FECHAH) <= fecierre) {
			Warning("El Parte está cerrado al %.3D.\nDebe ingresar una fecha mayor al %.3D.", fecierre, fecierre);
			return FM_REDO;
		}
		break;
	case CLIED:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIED, GetDescCliente(FmLFld(fm, CLIED, row)), row);
    break;
    case CLIEH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLIEH, GetDescCliente(FmLFld(fm, CLIEH, row)), row);
   	break;
    case OBJETD:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIED, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIED, row), FmIFld(fm, OBJETD, row)), row);
	break;
    case OBJETH:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIEH, row));
  		else
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row), FmIFld(fm, OBJETH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;

	}
	return FM_OK;

}

static fm_status before(form fm, fmfield fno, int row)
{

	switch (fno) {
	case CLIED:
	   	InicClientesXusr();
    	break;
    case CLIEH:
    	break;
    case OBJETD:
	   	InicObjetivosXusr(FmLFld(fm, CLIED, row), FmIFld(fm, EMP, row));
    	break;
    case OBJETH:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIEH, row), FmIFld(fm, EMP, row));
    	break;
	}
	return FM_OK;

}

static void AbrirArchLog()
{
	char nomarch[50] = {'\0'};
	
	sprintf(nomarch, "confpar.%d.log", GetUid());
		
	if (archOK == FALSE)	{
		if ((archlog = fopen(nomarch, "a+")) == (FILE*)NULL)
				Error("No se pudo generar el archivo %s", nomarch);
		archOK = TRUE;
	}
	
	fprintf(archlog, "Confirmación de Parte - Parametros de Ejecución:\n");
	fprintf(archlog, "Emp %d - Cliente Desde %ld Hasta %ld Objetivo Desde %d Hasta %d Fecha Desde %.3D Hasta %.3D \n", 
			FmIFld(fm0, EMP), FmLFld(fm0, CLIED), FmLFld(fm0, CLIEH), FmIFld(fm0, OBJETD), FmIFld(fm0, OBJETH), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH));

	fprintf(archlog, "Inicia Proceso %.3D - %.3T\nClientes/Objetivos Procesados\n", Today(), Hour());
}

static tndia AcuNDia(tndia nodop, tndia * nantp)
{
	tndia naux;

	if (nodop == NULL) {
		nodop = (tndia) malloc (sizeof(stndia));

		(*nodop).dia = dia;

		(*nodop).nlegajo = NULL;
		(*nodop).nlegajo = AcuNLegajo(NULL, NULL);

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).dia == dia) {
			(*nodop).nlegajo = AcuNLegajo((*nodop).nlegajo, &(*nodop).nlegajo);
		}
		else {
			if ((*nodop).dia < dia)
				(*nodop).nsig = AcuNDia((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tndia) malloc (sizeof(stndia));
				(*nodop).dia = dia;

				(*nodop).nlegajo = NULL;
				(*nodop).nlegajo = AcuNLegajo(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnlegajo AcuNLegajo(tnlegajo nodop, tnlegajo * nantp)
{
	tnlegajo naux;

	if (nodop == NULL) {
		nodop = (tnlegajo) malloc (sizeof(stnlegajo));

		(*nodop).legajo = legajo;

		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).legajo != legajo) {
			if ((*nodop).legajo < legajo)
				(*nodop).nsig = AcuNLegajo((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnlegajo) malloc (sizeof(stnlegajo));
				(*nodop).legajo = legajo;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static void LisNDia(tndia nodop)
{

	if (nodop == NULL)
		return;

	dia = (*nodop).dia;

	if ((*nodop).nlegajo != NULL)
		LisNLegajo((*nodop).nlegajo);

	if ((*nodop).nsig != NULL)
		LisNDia((*nodop).nsig);
}

static void LisNLegajo(tnlegajo nodop)
{

	if (nodop == NULL)
		return;

	legajo = (*nodop).legajo;


//	fprintf (stderr, "%.3D %ld\n", dia, legajo);


	RecalculaPartePer(FmIFld(fm0, EMP), legajo, dia, dia, fm0, COMENT, g_prog);


	if ((*nodop).nsig != NULL)
		LisNLegajo((*nodop).nsig);
}

static void BorNDia(tndia nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nlegajo != NULL)
		BorNLegajo((*nodop).nlegajo);

	if ((*nodop).nsig != NULL)
		BorNDia((*nodop).nsig);

	(*nodop).nlegajo = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNLegajo(tnlegajo nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNLegajo((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}

