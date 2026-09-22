/********************************************************************
*
* MODULE & VERSION : @(#)lrotvig.c	1.6
* DATE             : 04/10/12
* TIME             : 09:14:37
*
* CREATED          : 30/01/01
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "lrotvig.fmh"
#include "lrotvig.rph" 
#include "lrotvig2.rph"
#include "comerc.h"
#include "operac.h"
#include "opedef.h"
#include "billpro.h"
#include "comerc.sch"
#include "operac.sch"
#include "filial.h"

#define R_SEPAR		";"

struct scliente {
	long cliente;
	short objet;
	short altas, bajas;
	struct scliente *sig;
} *scli_est = NULL;

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
struct scliente* InsertarNodo(short emp, long cliente, short objet, bool alta, bool baja, struct scliente *corr);
void GenerarReporte ();
void AbrirSalidaReporte();
void ImprimirRenglonReporte ();

static void AbrirSalidaArchivo();
static void ImprimirRenglonAssist();

bool AsigAnteriorRota(int emp, long legajo, DATE fecbaj, long cliente, int objet, int ptoser, int puesto,
					  int nroint);

/* Declaraciones globales */
form fm0;
schema com, ope;
report rp0;
FILE *fp;
//agrego para optimizacion
struct s_lisxusr_lib esta_lis;

/* Programa principal */
wcmd(lrotvig, 1.6 10/12/04)
{
	fm_cmd cmd;

	com=OpenSchema("comerc",	IO_EABORT);
	ope=OpenSchema("operac",	IO_EABORT);
	fm0 = OpenForm("lrotvig", FM_EABORT);

  	FmSetFld(fm0, COMENTARIO, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENTARIO, "");
	WiRefresh();
	
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		GenerarReporte ();
		break;
	case FM_DELETE:
		break;
	case FM_IGNORE:
		FmClearAllFlds (fm0);
		break;
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static fm_status before (form fm, fmfield fno, int row)
{
	switch (fno) {
	case ARCHIVO:
		if ((*FmSFld(fm0, SALIDA)=='A' || *FmSFld(fm0, SALIDA)=='R') && FmIsNull (fm0, ARCHIVO)) {
			FmSetFld (fm0, ARCHIVO, "lrotvig.txt");
		}
	break;
	case CLID:
	   	InicClientesXusr();
    	break;
    case CLIH:
    	break;
    case OBJD:
	   	InicObjetivosXusr(FmLFld(fm, CLID, row), FmIFld(fm, EMP, row));
    	break;
    case OBJH:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIH, row), FmIFld(fm, EMP, row));
    	break;
	case FFILIAL:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		if (FmChgFld(fm))
		    InicListaXusr(FmIFld(fm0, EMP));
    break;
	case CLID:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLID, row)), row);
    break;
    case CLIH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
			FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIH, row)),row);
   	break;
    case OBJD:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLID, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLID, row), FmIFld(fm, OBJD, row)), row);
	break;
    case OBJH:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIH, row));
		else	
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row) ,FmIFld(fm, OBJH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;				
}	

void GenerarReporte()
{
	struct scliente *old_scli;
	dbcursor c_asig, c_asigh;
	char comen[100];
	long legant = NULL_LONG, cantasigh = 0;
	DATE fecaltant = NULL_DATE;
	bool encontroleg = FALSE;
	bool alta=FALSE, baja=FALSE;

	c_asig  = CreateCursor (ope|ASIGbyFECHA,     IO_NOT_LOCK);
	c_asigh = CreateCursor (ope|ASIGHbyFECHAALT, IO_NOT_LOCK);

	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLID))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLID) && esta_lis.objetivo < FmIFld(fm0, OBJD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIH) && esta_lis.objetivo > FmIFld(fm0, OBJH))
	    	continue;
	
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FDESDE), MIN_LONG);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, FmDFld(fm0, FHASTA), MAX_LONG);
		while (FetchCursor(c_asig) != ERROR) {
			if (!ValidaFilial(LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

			if (IsNull(ope|ASIG_FECASIG)|| DFld(ope|ASIG_FECASIG) > FmDFld(fm0, FHASTA) || DFld(ope|ASIG_FECASIG) < FmDFld(fm0, FDESDE))
				continue;

			sprintf(comen, "Procesando Asig cliente %ld objetivo %d \n", LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO));
			FmSetFld (fm0, COMENTARIO, comen);
			WiRefresh();

			// Si el vigilador tiene mas de una asignacion en el cliente/objetivo para la misma fecha
			// debe contabilizarse como una sola alta o baja segun corresponda. (Esto es para cuando
			// existe asignacion Efectiva y varias Provisorias)
			if (LFld(ope|ASIG_NROLEG) != legant ||
			   (LFld(ope|ASIG_NROLEG) == legant && DFld(ope|ASIG_FECASIG) != fecaltant)) {
				legant    = LFld(ope|ASIG_NROLEG);
				fecaltant = DFld(ope|ASIG_FECASIG);
				encontroleg = FALSE;
			}
			if (encontroleg)
				continue;

			if (!AsigAnteriorRota(FmIFld(fm0, EMP), LFld(ope|ASIG_NROLEG), DFld(ope|ASIG_FECASIG) - 1, 
								  LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO), IFld(ope|ASIG_PTOSER),
								  IFld(ope|ASIG_PUESTO), IFld(ope|ASIG_NROINT)))
				continue;

			encontroleg = TRUE;

			scli_est = InsertarNodo(FmIFld(fm0, EMP), LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO), TRUE, FALSE, scli_est);
		}
	}
	
	DeleteCursor(c_asig);

	legant      = NULL_LONG;
	fecaltant   = NULL_DATE;
	encontroleg = FALSE;

	VolverInicioListaXusr();
	while(ProximoListaXusr(&esta_lis)) {
	    
	    if (esta_lis.cliente < FmLFld(fm0, CLID))
	    	continue;
	    if (esta_lis.cliente > FmLFld(fm0, CLIH))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLID) && esta_lis.objetivo < FmIFld(fm0, OBJD))
	    	continue;
	    if (esta_lis.cliente == FmLFld(fm0, CLIH) && esta_lis.objetivo > FmIFld(fm0, OBJH))
	    	continue;

		SetCursorFrom(c_asigh, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MIN_LONG, MIN_DATE, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP), esta_lis.cliente, esta_lis.objetivo, MAX_LONG, MAX_DATE, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			if (!ValidaFilial(LFld(ope|ASIGH_CLIENTE), IFld(ope|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
			

			sprintf(comen, "Procesando Asigh cliente %ld objetivo %d \n", LFld(ope|ASIGH_CLIENTE), IFld(ope|ASIGH_OBJETIVO));
			FmSetFld (fm0, COMENTARIO, comen);
			WiRefresh();

			// Si el vigilador tiene mas de una asignacion en el cliente/objetivo para la misma fecha
			// debe contabilizarse como una sola alta o baja segun corresponda. (Esto es para cuando
			// existe asignacion Efectiva y varias Provisorias)
			if (LFld(ope|ASIGH_NROLEG) != legant ||
			   (LFld(ope|ASIGH_NROLEG) == legant && DFld(ope|ASIGH_FECALT) != fecaltant)) {
				legant    = LFld(ope|ASIGH_NROLEG);
				fecaltant = DFld(ope|ASIGH_FECALT);
				encontroleg = FALSE;
			}

			if (encontroleg)
				continue;

			if (IFld(ope|ASIGH_MOTIVO) == ALTAPARTE || IFld(ope|ASIGH_MOTIVO) == DESXERROR)
				continue;

			//Si la fecha de baja es igual a la fecha hasta la baja es considerada para el proximo periodo.
			if (DFld(ope|ASIGH_FECBAJ) == FmDFld(fm0, FHASTA))
				continue;

			if (!IsNull(ope|ASIGH_FECALT) && DFld(ope|ASIGH_FECALT) <= FmDFld(fm0, FHASTA) && DFld(ope|ASIGH_FECALT) >= FmDFld(fm0, FDESDE)) {
				if (AsigAnteriorRota(FmIFld(fm0, EMP), LFld(ope|ASIGH_NROLEG), DFld(ope|ASIGH_FECALT) - 1, 
								LFld(ope|ASIGH_CLIENTE), IFld(ope|ASIGH_OBJETIVO), IFld(ope|ASIGH_PTOSER),
								IFld(ope|ASIGH_PUESTO), IFld(ope|ASIGH_NROINT))) {
					alta=TRUE;
					encontroleg = TRUE;
				}
			}
			//Si la fecha de baja es anterior a la fecha desde sera considerada baja del periodo.
			if (!IsNull(ope|ASIGH_FECALT) && (DFld(ope|ASIGH_FECBAJ) < FmDFld(fm0, FHASTA) && DFld(ope|ASIGH_FECBAJ) >= FmDFld(fm0, FDESDE)) || (DFld(ope|ASIGH_FECBAJ) + 1 == FmDFld(fm0, FDESDE))) {
				if (MotDesagRota(IFld(ope|ASIGH_MOTIVO))) {
					baja=TRUE;
					encontroleg = TRUE;
				}
			}

			if(!alta && !baja) {
				continue;
			}
			scli_est = InsertarNodo(FmIFld(fm0, EMP), LFld(ope|ASIGH_CLIENTE), IFld(ope|ASIGH_OBJETIVO),  alta, baja, scli_est);
		}
    }
   	
   	DeleteCursor(c_asigh);

	if (*FmSFld(fm0, SALIDA)=='R')
		AbrirSalidaArchivo();
    else
    	AbrirSalidaReporte();

	while(scli_est != NULL) {
		if (*FmSFld(fm0, SALIDA)=='R')
			ImprimirRenglonAssist();
		else
			ImprimirRenglonReporte();

		//Borro el que ya lei y leo el siguiente
		old_scli = scli_est;
		scli_est = scli_est->sig;
		free(old_scli);
	}

	if (*FmSFld(fm0, SALIDA)=='R')
		fclose(fp);
	else
		CloseReport(rp0);

	FmSetFld (fm0, COMENTARIO, NULL_STR);
	WiRefresh();
}

struct scliente* InsertarNodo(short emp, long cliente, short objet, bool alta, bool baja, struct scliente *corr)
{
	bool inserto = FALSE;
	struct scliente *ant, *tope, *nuevo;
	ant = corr;
	tope = corr;
	
	while(!inserto && corr != NULL && 
		 (corr->cliente < cliente || (corr->cliente == cliente && corr->objet <= objet))) {

		// Es menor sigo recorriendo
		if (corr->cliente < cliente || (corr->cliente == cliente && corr->objet < objet)) {
			ant = corr;
			corr = corr->sig;
		}
		else {
			// Lo encontre ya existe
			if (corr->cliente == cliente && corr->objet == objet){
				if (alta) corr->altas ++;
				if (baja) corr->bajas ++;
				inserto = TRUE;
			}
		}
	}

	//No lo encontro o encontro la posicion que va
	if (!inserto) {
		nuevo = (struct scliente *) malloc(sizeof(struct scliente));
		nuevo->cliente = cliente;
		nuevo->objet = objet;
		nuevo->altas = alta ? 1 : 0;
		nuevo->bajas = baja ? 1 : 0;
		nuevo->sig = corr;
		if (ant == corr) {
			corr = nuevo;
		}
		else {
			ant->sig = nuevo;
			corr = tope;
		}
	}
	if (inserto) {
		corr = tope;
	}
	return corr;
}

void AbrirSalidaReporte()
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("lrotvig2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_FILE, FmSFld(fm0, ARCHIVO));
	}
	else {
		rp0 = OpenReport("lrotvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, *FmSFld(fm0,SALIDA) == 'I' ? RP_IO_DEFAULT : RP_IO_TERM, NULL_STR);
	}
	BeginReport(rp0,1,NULL_STR);
	RpSetDFld(rp0, RDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, RHASTA, FmDFld(fm0, FHASTA));
}

void ImprimirRenglonReporte ()
{
	RpSetLFld(rp0, RCLIE,  scli_est->cliente);
	RpSetFld (rp0, RDCLIE, GetDescCli(scli_est->cliente));
	RpSetIFld(rp0, ROBJ,   scli_est->objet);
	RpSetFld (rp0, RDOBJ,  GetObjDescrip(scli_est->cliente, scli_est->objet));
	RpSetIFld(rp0, RALTAS, scli_est->altas);
	RpSetIFld(rp0, RBAJAS, scli_est->bajas);
	DoReport (rp0, LINEA);
}

static void AbrirSalidaArchivo()
{
	if ((fp=fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
		Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));
}

static void ImprimirRenglonAssist()
{
	fprintf(fp, "%02d%4d%s%ld%s%s%s%d%s%s%s%d%s%d\n",
		Month(FmDFld(fm0,FDESDE)),
		Year(FmDFld(fm0,FDESDE)), R_SEPAR,
		scli_est->cliente, R_SEPAR,
		GetDescCli(scli_est->cliente), R_SEPAR,
		scli_est->objet, R_SEPAR,
		GetObjDescrip(scli_est->cliente, scli_est->objet), R_SEPAR,
		scli_est->altas, R_SEPAR,
		scli_est->bajas);
}

bool AsigAnteriorRota(int emp, long legajo, DATE fecbaj, long cliente, int objet, int ptoser, int puesto,
					  int nroint)
{
	dbtable	AASIGH;
	schema  operac, old;
	bool rota = FALSE, tieneasigh = FALSE;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	AASIGH = CreateAlias(operac|ASIGH);

	SetIFld(AASIGH_EMP,      emp);
	SetLFld(AASIGH_NROLEG,   legajo);
	SetDFld(AASIGH_FECBAJ,   fecbaj);
	SetLFld(AASIGH_CLIENTE,  MAX_LONG);
	SetIFld(AASIGH_OBJETIVO, MAX_SHORT);
	while (!rota && GetRecord(AASIGHbyLEGFEC, PREV_KEY|PARTIAL_KEY, IO_LOCK, 3) != ERROR) {

		//Esta variable es para los casos que recien ingresan a la empresa y no tiene asig
		//historica entonces se debe considerar siempre como un ingreso.
		tieneasigh = TRUE;

		if (emp    == IFld(AASIGH_EMP)      && cliente == LFld(AASIGH_CLIENTE)) // &&
//			objet  == IFld(AASIGH_OBJETIVO) && ptoser  == IFld(AASIGH_PTOSER)  && 
//			puesto == IFld(AASIGH_PUESTO)   && nroint  == IFld(AASIGH_NROINT))
			break;

		if (MotDesagRota(IFld(AASIGH_MOTIVO)))
			rota = TRUE;
	}
	DeleteAlias(AASIGH);

	if (!tieneasigh)
		rota = TRUE;

	return rota;
}
