/********************************************************************
*
* MODULE & VERSION : @(#)lparteh.c	1.2 
* DATE             : 10/03/25
* TIME             : 12:44:37
*
* CREATED          : 16/03/2010
*
* DESCRIPTION:
*      Impresión del Parte Diario Por Turnos
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "lparteh.fmh"
#include "lparteh.rph"
#include "aurus.sch"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"
#include "sue.sch"
#include "filial.h"

#define MAXLINEA 60
#define ZONA_HEAD 	3
#define ZONA_ENCAB0 2
#define ZONA_ENCAB1 1
#define ZONA_ENCAB2 2
#define ZONA_LINEA	1
#define ZONA_FLIN	1
#define ZONA_TOTAL	4

//-----------------------------------------------------Definicion de Listas Enlazadas-----------------------------------------------------------------------//
typedef struct stnclient * tnclient;
typedef struct stnobjeti * tnobjeti;
typedef struct stnfecha * tnfecha;
typedef struct stnhdesde * tnhdesde;
typedef struct stnhhasta * tnhhasta;
typedef struct stnnroleg * tnnroleg;
typedef struct stnptoser * tnptoser;
typedef struct stnpuesto * tnpuesto;

typedef struct stnclient {
	long	client;
	tnobjeti	nobjeti;
	tnclient	nsig;
} stnclient;

typedef struct stnobjeti {
	int		objeti;
	tnfecha	nfecha;
	tnobjeti	nsig;
} stnobjeti;

typedef struct stnfecha {
	DATE	fecha;
	tnhdesde	nhdesde;
	tnfecha	nsig;
} stnfecha;

typedef struct stnhdesde {
	TIME	hdesde;
	tnhhasta	nhhasta;
	tnhdesde	nsig;
} stnhdesde;

typedef struct stnhhasta {
	TIME	hhasta;
	tnnroleg	nnroleg;
	tnhhasta	nsig;
} stnhhasta;

typedef struct stnnroleg {
	long	nroleg;
	tnptoser	nptoser;
	tnnroleg	nsig;
} stnnroleg;

typedef struct stnptoser {
	int		ptoser;
	tnpuesto	npuesto;
	tnptoser	nsig;
} stnptoser;

typedef struct stnpuesto {
	int		puesto;
	tnpuesto	nsig;
} stnpuesto;

/* Funciones Privadas */
static tnclient AcuNClient(tnclient, tnclient*);
static tnobjeti AcuNObjeti(tnobjeti, tnobjeti*);
static tnfecha AcuNFecha(tnfecha, tnfecha*);
static tnhdesde AcuNHdesde(tnhdesde, tnhdesde*);
static tnhhasta AcuNHhasta(tnhhasta, tnhhasta*);
static tnnroleg AcuNNroleg(tnnroleg, tnnroleg*);
static tnptoser AcuNPtoser(tnptoser, tnptoser*);
static tnpuesto AcuNPuesto(tnpuesto, tnpuesto*);

static void LisNClient(tnclient);
static void LisNObjeti(tnobjeti);
static void LisNFecha(tnfecha);
static void LisNHdesde(tnhdesde);
static void LisNHhasta(tnhhasta);
static void LisNNroleg(tnnroleg);
static void LisNPtoser(tnptoser);
static void LisNPuesto(tnpuesto);

static void BorNClient(tnclient);
static void BorNObjeti(tnobjeti);
static void BorNFecha(tnfecha);
static void BorNHdesde(tnhdesde);
static void BorNHhasta(tnhhasta);
static void BorNNroleg(tnnroleg);
static void BorNPtoser(tnptoser);
static void BorNPuesto(tnpuesto);

tnclient	inicio;

int		objeti, ptoser, puesto;
long	client, nroleg;
DATE	fecha;
TIME	hdesde, hhasta;

//-------------------------------------------------Fin Definicion de Listas Enlazadas-----------------------------------------------------------------------//

static fm_status before(form fm, fmfield fno, int row);
static fm_status after(form fm, fmfield fno, int row);
void SaltoDePagina(int p_zona, int p_zonalin);

void AbrirReporte();

/* Declaraciones globales */
form   fm0;
report rp0;
schema comerc, operac, sue, bill;
char auxi[150];
FILE *salida;
int g_lin=0;

/* Programa principal */
wcmd(lparteh, 1.2 03/25/10)
{
	fm_cmd cmd;
	dbcursor c_PARTE, c_obj;

	fm0    = OpenForm  ("lparteh", FM_EABORT);
	comerc = OpenSchema("comerc",  IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);
	sue    = OpenSchema("sue",     IO_EABORT);
	bill   = OpenSchema("bill",    IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)	{

		if (cmd == FM_IGNORE)
        	continue;

		inicio = NULL;
		client = NULL_LONG;
		objeti = NULL_SHORT;
		fecha  = NULL_DATE;
		hdesde = NULL_TIME;
		hhasta = NULL_TIME;
		nroleg = NULL_LONG;
		ptoser = NULL_SHORT;
		puesto = NULL_SHORT;


		if (*FmSFld(fm0, OPCION) == 'P') {
			c_obj = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);
		}
		else {
			if (*FmSFld(fm0, OPCION) == 'G') {
				c_obj = CreateCursor(comerc|OBJETIVObyPROGRAM, IO_NOT_LOCK);
				SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
				SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);

			}
			else {
				if (*FmSFld(fm0, OPCION) == 'S') {
					c_obj = CreateCursor(comerc|OBJETIVObySVISOR, IO_NOT_LOCK);
					SetCursorFrom(c_obj, FmLFld(fm0, NROLEGD), MIN_LONG, MIN_SHORT);
					SetCursorTo  (c_obj, FmLFld(fm0, NROLEGH), MAX_LONG, MAX_SHORT);
				}
				else {
					c_obj = CreateCursor(comerc|OBJETIVO, IO_NOT_LOCK);
					SetCursorFrom(c_obj, FmLFld(fm0, CLIED), FmIFld(fm0, OBJETD));
					SetCursorTo  (c_obj, FmLFld(fm0, CLIEH), FmIFld(fm0, OBJETH));
				}
			}
		}

		while (FetchCursor(c_obj) != ERROR) {

			if (FmIFld(fm0, MOSTRA)==2){
				if (!ExisteCliObjEnGrp(GRPPARH, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
					continue; 
			}					

			//valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			       	continue;

			if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

			c_PARTE = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
			SetCursorFrom(c_PARTE, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FDESDE),
			              MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (c_PARTE, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FHASTA), 
			              MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(c_PARTE) != ERROR) {
				
				if (TFld(operac|PARTE_HORAENT)==StrToT("00:00") && TFld(operac|PARTE_HORASAL)==StrToT("00:00"))
					continue;

				client = LFld(operac|PARTE_CLIENTE);
				objeti = IFld(operac|PARTE_OBJETIVO);
				fecha  = DFld(operac|PARTE_DIA);
				hdesde = TFld(operac|PARTE_HORAENT);
				hhasta = TFld(operac|PARTE_HORASAL);
				nroleg = LFld(operac|PARTE_NROLEG);
				ptoser = IFld(operac|PARTE_PTOSER);
				puesto = IFld(operac|PARTE_PUESTO);

				inicio = AcuNClient(inicio, &inicio);
			} 
    		DeleteCursor(c_PARTE);

			sprintf(auxi, "Procesando Cliente %ld Objetivo %d", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			FmSetFld(fm0, COMENT, auxi);
			WiRefresh();
			
		} 
		AbrirReporte () ;

		if (*FmSFld(fm0, SALIDA) != 'A') {
			RpSetLFld(rp0, R_CLIED, FmLFld(fm0, CLIED));
			RpSetFld (rp0, R_DCLIED, GetCliRazsoc(FmLFld(fm0, CLIED)));  
			RpSetLFld(rp0, R_CLIEH, FmLFld(fm0, CLIEH));
			RpSetFld (rp0, R_DCLIEH, GetCliRazsoc(FmLFld(fm0, CLIEH)));  
			RpSetIFld(rp0, R_OBJETD, FmIFld(fm0, OBJETD));
			RpSetFld (rp0, R_DOBJETD,GetObjDescrip(FmLFld(fm0, CLIED),FmIFld(fm0, OBJETD)));
			RpSetIFld(rp0, R_OBJETH, FmIFld(fm0, OBJETH));
			RpSetFld (rp0, R_DOBJETH,GetObjDescrip(FmLFld(fm0, CLIEH),FmIFld(fm0, OBJETH)));
			RpSetDFld(rp0, R_FDESDE, FmDFld(fm0, FDESDE));
			RpSetDFld(rp0, R_FHASTA, FmDFld(fm0, FHASTA));

			SaltoDePagina(HEAD, ZONA_HEAD);

		} 

		LisNClient(inicio);
		BorNClient(inicio);

		FmSetFld(fm0, COMENT, NULL_STR);
		WiRefresh();

		if (*FmSFld(fm0, SALIDA) == 'A') 
			fclose(salida);
		else
			CloseReport(rp0);		

    }
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}


void AbrirReporte () 
{
	//Si la salida es Impresora
	if (*FmSFld(fm0, SALIDA) == 'I') {
		rp0 = OpenReport("lparteh", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}

	//Si la salida es Terminal
	if (*FmSFld(fm0, SALIDA) == 'T') {
		rp0 = OpenReport("lparteh", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}

	//Si la salida es Archivo
	if (*FmSFld(fm0, SALIDA) == 'A') {
		salida=fopen(FmSFld(fm0, NOMARCH), "w+");
		fprintf(salida, "Cliente\tObjetivo\tFecha\tHora Desde\tHora Hasta\tLegajo\tTipo Puesto\tPuesto\n");
	}
}


static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARCH :
		if (FmIsNull (fm, fno) && *FmSFld (fm, SALIDA) == 'A')
			FmSetFld (fm, fno, "lparteh.txt");
		break;
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
			FmSetFld(fm, DCLIEH, GetDescCliente(FmLFld(fm, CLIEH, row)),row);
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
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row) ,FmIFld(fm, OBJETH, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;				
}	

static tnclient AcuNClient(tnclient nodop, tnclient * nantp)
{
	tnclient naux;

	if (nodop == NULL) {
		nodop = (tnclient) malloc (sizeof(stnclient));
		(*nodop).client = client;
		(*nodop).nobjeti = AcuNObjeti(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).client == client) {
			(*nodop).nobjeti = AcuNObjeti((*nodop).nobjeti, &(*nodop).nobjeti);
		}
		else {
			if ((*nodop).client < client)
				(*nodop).nsig = AcuNClient((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnclient) malloc (sizeof(stnclient));
				(*nodop).client = client;

				(*nodop).nobjeti = AcuNObjeti(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnobjeti AcuNObjeti(tnobjeti nodop, tnobjeti * nantp)
{
	tnobjeti naux;

	if (nodop == NULL) {
		nodop = (tnobjeti) malloc (sizeof(stnobjeti));

		(*nodop).objeti = objeti;
		(*nodop).nfecha = AcuNFecha(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).objeti == objeti) {
			(*nodop).nfecha = AcuNFecha((*nodop).nfecha, &(*nodop).nfecha);
		}
		else {
			if ((*nodop).objeti < objeti)
				(*nodop).nsig = AcuNObjeti((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnobjeti) malloc (sizeof(stnobjeti));
				(*nodop).objeti = objeti;

				(*nodop).nfecha = AcuNFecha(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnfecha AcuNFecha(tnfecha nodop, tnfecha * nantp)
{
	tnfecha naux;

	if (nodop == NULL) {
		nodop = (tnfecha) malloc (sizeof(stnfecha));
		(*nodop).fecha = fecha;
		(*nodop).nhdesde = AcuNHdesde(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).fecha == fecha) {
			(*nodop).nhdesde = AcuNHdesde((*nodop).nhdesde, &(*nodop).nhdesde);
		}
		else {
			if ((*nodop).fecha < fecha)
				(*nodop).nsig = AcuNFecha((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnfecha) malloc (sizeof(stnfecha));
				(*nodop).fecha = fecha;

				(*nodop).nhdesde = AcuNHdesde(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnhdesde AcuNHdesde(tnhdesde nodop, tnhdesde * nantp)
{
	tnhdesde naux;

	if (nodop == NULL) {
		nodop = (tnhdesde) malloc (sizeof(stnhdesde));
		(*nodop).hdesde = hdesde;
		(*nodop).nhhasta = AcuNHhasta(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).hdesde == hdesde) {
			(*nodop).nhhasta = AcuNHhasta((*nodop).nhhasta, &(*nodop).nhhasta);
		}
		else {
			if ((*nodop).hdesde < hdesde)
				(*nodop).nsig = AcuNHdesde((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnhdesde) malloc (sizeof(stnhdesde));
				(*nodop).hdesde = hdesde;

				(*nodop).nhhasta = AcuNHhasta(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnhhasta AcuNHhasta(tnhhasta nodop, tnhhasta * nantp)
{
	tnhhasta naux;

	if (nodop == NULL) {
		nodop = (tnhhasta) malloc (sizeof(stnhhasta));
		(*nodop).hhasta = hhasta;
		(*nodop).nnroleg = AcuNNroleg(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).hhasta == hhasta) {
			(*nodop).nnroleg = AcuNNroleg((*nodop).nnroleg, &(*nodop).nnroleg);
		}
		else {
			if ((*nodop).hhasta < hhasta)
				(*nodop).nsig = AcuNHhasta((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnhhasta) malloc (sizeof(stnhhasta));
				(*nodop).hhasta = hhasta;

				(*nodop).nnroleg = AcuNNroleg(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnnroleg AcuNNroleg(tnnroleg nodop, tnnroleg * nantp)
{
	tnnroleg naux;

	if (nodop == NULL) {
		nodop = (tnnroleg) malloc (sizeof(stnnroleg));
		(*nodop).nroleg = nroleg;
		(*nodop).nptoser = AcuNPtoser(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).nroleg == nroleg) {
			(*nodop).nptoser = AcuNPtoser((*nodop).nptoser, &(*nodop).nptoser);
		}
		else {
			if ((*nodop).nroleg < nroleg)
				(*nodop).nsig = AcuNNroleg((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnnroleg) malloc (sizeof(stnnroleg));
				(*nodop).nroleg = nroleg;

				(*nodop).nptoser = AcuNPtoser(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnptoser AcuNPtoser(tnptoser nodop, tnptoser * nantp)
{
	tnptoser naux;

	if (nodop == NULL) {
		nodop = (tnptoser) malloc (sizeof(stnptoser));
		(*nodop).ptoser = ptoser;
		(*nodop).npuesto = AcuNPuesto(NULL, NULL);
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).ptoser == ptoser) {
			(*nodop).npuesto = AcuNPuesto((*nodop).npuesto, &(*nodop).npuesto);
		}
		else {
			if ((*nodop).ptoser < ptoser)
				(*nodop).nsig = AcuNPtoser((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnptoser) malloc (sizeof(stnptoser));
				(*nodop).ptoser = ptoser;

				(*nodop).npuesto = AcuNPuesto(NULL, NULL);
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static tnpuesto AcuNPuesto(tnpuesto nodop, tnpuesto * nantp)
{
	tnpuesto naux;

	if (nodop == NULL) {
		nodop = (tnpuesto) malloc (sizeof(stnpuesto));
		(*nodop).puesto = puesto;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).puesto == puesto) {
		}
		else {
			if ((*nodop).puesto < puesto)
				(*nodop).nsig = AcuNPuesto((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnpuesto) malloc (sizeof(stnpuesto));
				(*nodop).puesto = puesto;

				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static void LisNClient(tnclient nodop)
{
	if (nodop == NULL)
		return;

	SwitchToSchema(bill);
	client = (*nodop).client;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetLFld(rp0, R_CLIE, client);
		RpSetFld(rp0, R_DCLIE, GetCliRazsoc(client));  
	}

	if ((*nodop).nobjeti != NULL)
		LisNObjeti((*nodop).nobjeti);

	if ((*nodop).nsig != NULL)
		LisNClient((*nodop).nsig);
}

static void LisNObjeti(tnobjeti nodop)
{
	long superv;

	if (nodop == NULL)
		return;

	objeti = (*nodop).objeti;

	if (*FmSFld(fm0, SALIDA) != 'A') {

		RpSetIFld(rp0, R_OBJET, objeti);
		SetLFld(comerc|OBJETIVO_CLIENTE, client );
		SetIFld(comerc|OBJETIVO_OBJET,   objeti );
		if ( GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR ) {
			RpSetFld(rp0, R_DOBJET, SFld(comerc|OBJETIVO_DESCRIP));  
			superv= (*FmSFld(fm0, OPCION) == 'P'? LFld(comerc|OBJETIVO_PRESEN)  : 
		             *FmSFld(fm0, OPCION) == 'G'? LFld(comerc|OBJETIVO_PROGRAM) :
	    	         *FmSFld(fm0, OPCION) == 'S'? LFld(comerc|OBJETIVO_SVISOR)  : NULL_LONG);
			RpSetIFld(rp0, R_SUPERV, superv);
    		RpSetFld(rp0, R_DSUP, GetNombreLeg(FmIFld(fm0, EMP), superv));

		}
		else {
			RpSetFld (rp0, R_DOBJET, NULL_STR);  
			RpSetIFld(rp0, R_SUPERV, NULL_SHORT);
    		RpSetFld (rp0, R_DSUP  , NULL_STR);
		}

		SaltoDePagina(ENCAB0, ZONA_ENCAB0);
	} 

	if ((*nodop).nfecha != NULL)
		LisNFecha((*nodop).nfecha);

	if ((*nodop).nsig != NULL)
		LisNObjeti((*nodop).nsig);
}

static void LisNFecha(tnfecha nodop)
{

	if (nodop == NULL)
		return;

	fecha = (*nodop).fecha;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetDFld(rp0, R_DIA1, fecha);
		RpSetIFld(rp0, R_STD1, StdHr(FmIFld(fm0, EMP), client, objeti, NULL_SHORT, fecha, fecha));

		SaltoDePagina(ENCAB1, ZONA_ENCAB1);
	}
	
	if ((*nodop).nhdesde != NULL){
		LisNHdesde((*nodop).nhdesde);

		SaltoDePagina(TOTAL, ZONA_TOTAL);
	}

	if ((*nodop).nsig != NULL)
		LisNFecha((*nodop).nsig);
}

static void LisNHdesde(tnhdesde nodop)
{
	if (nodop == NULL)
		return;

	hdesde = (*nodop).hdesde;

	if ((*nodop).nhhasta != NULL)
		LisNHhasta((*nodop).nhhasta);

	if ((*nodop).nsig != NULL)
		LisNHdesde((*nodop).nsig);
}

static void LisNHhasta(tnhhasta nodop)
{
	if (nodop == NULL)
		return;

	hhasta = (*nodop).hhasta;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetTFld(rp0, R_HDESDE, hdesde);
		RpSetTFld(rp0, R_HHASTA, hhasta);

		SaltoDePagina(ENCAB2, ZONA_ENCAB2);
	}

	if ((*nodop).nnroleg != NULL) {
		LisNNroleg((*nodop).nnroleg);
		SaltoDePagina(FLIN, ZONA_FLIN);
	}


	if ((*nodop).nsig != NULL)
		LisNHhasta((*nodop).nsig);
}

static void LisNNroleg(tnnroleg nodop)
{
	char regimen[11];
	if (nodop == NULL)
		return;

	nroleg = (*nodop).nroleg;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetLFld(rp0, R_LEGAJO, nroleg);
		RpSetFld (rp0, R_APENOM, GetNombreLeg(FmIFld(fm0, EMP), nroleg));
		GetRegimenEfectivo(FmIFld(fm0, EMP), nroleg, regimen, fecha);
		RpSetFld (rp0, R_REGIM,  regimen);
	}

	if ((*nodop).nptoser != NULL)
		LisNPtoser((*nodop).nptoser);

	if ((*nodop).nsig != NULL)
		LisNNroleg((*nodop).nsig);
}

static void LisNPtoser(tnptoser nodop)
{
	if (nodop == NULL)
		return;

	ptoser = (*nodop).ptoser;

	if (*FmSFld(fm0, SALIDA) != 'A') 
		RpSetIFld (rp0, R_PTOSER, ptoser);

	if ((*nodop).npuesto != NULL)
		LisNPuesto((*nodop).npuesto);

	if ((*nodop).nsig != NULL)
		LisNPtoser((*nodop).nsig);
}

static void LisNPuesto(tnpuesto nodop)
{
	if (nodop == NULL)
		return;

	puesto = (*nodop).puesto;

	if (*FmSFld(fm0, SALIDA) != 'A') {
		RpSetIFld(rp0, R_PUESTO, puesto);
		RpSetFld (rp0, R_DPTOSER, GetDescPto(ptoser));

		SaltoDePagina(LINEA, ZONA_LINEA);
	}
	else
		fprintf(salida, "%ld\t%d\t%.3D\t%.3T\t%.3T\t%ld\t%d\t%d\n", client, objeti,fecha, hdesde, hhasta, nroleg, ptoser, puesto);


	if ((*nodop).nsig != NULL)
		LisNPuesto((*nodop).nsig);
}

static void BorNClient(tnclient nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nobjeti != NULL)
		BorNObjeti((*nodop).nobjeti);

	if ((*nodop).nsig != NULL)
		BorNClient((*nodop).nsig);

	(*nodop).nobjeti = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNObjeti(tnobjeti nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nfecha != NULL)
		BorNFecha((*nodop).nfecha);

	if ((*nodop).nsig != NULL)
		BorNObjeti((*nodop).nsig);

	(*nodop).nfecha = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNFecha(tnfecha nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nhdesde != NULL)
		BorNHdesde((*nodop).nhdesde);

	if ((*nodop).nsig != NULL)
		BorNFecha((*nodop).nsig);

	(*nodop).nhdesde = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNHdesde(tnhdesde nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nhhasta != NULL)
		BorNHhasta((*nodop).nhhasta);

	if ((*nodop).nsig != NULL)
		BorNHdesde((*nodop).nsig);

	(*nodop).nhhasta = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNHhasta(tnhhasta nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nnroleg != NULL)
		BorNNroleg((*nodop).nnroleg);

	if ((*nodop).nsig != NULL)
		BorNHhasta((*nodop).nsig);

	(*nodop).nnroleg = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNNroleg(tnnroleg nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nptoser != NULL)
		BorNPtoser((*nodop).nptoser);

	if ((*nodop).nsig != NULL)
		BorNNroleg((*nodop).nsig);

	(*nodop).nptoser = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNPtoser(tnptoser nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).npuesto != NULL)
		BorNPuesto((*nodop).npuesto);

	if ((*nodop).nsig != NULL)
		BorNPtoser((*nodop).nsig);

	(*nodop).npuesto = NULL;
	(*nodop).nsig = NULL;

	free(nodop);
}

static void BorNPuesto(tnpuesto nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNPuesto((*nodop).nsig);

	(*nodop).nsig = NULL;

	free(nodop);
}


void SaltoDePagina(int p_zona, int p_zonalin)
{
	
	g_lin += p_zonalin;
	
	if (g_lin >= MAXLINEA) {
		DoReport(rp0, FLIN1);
		RpEjectPage(rp0);

		g_lin = p_zonalin;

		if (p_zona> HEAD){
			g_lin += ZONA_HEAD;
			DoReport(rp0, HEAD);
		}
		if (p_zona> ENCAB0){
			g_lin += ZONA_ENCAB0;
			DoReport(rp0, ENCAB0);
		}
		if (p_zona> ENCAB1){
			g_lin += ZONA_ENCAB1;
			DoReport(rp0, ENCAB1);
		}
		if (p_zona> ENCAB2){
			g_lin += ZONA_ENCAB2;
			DoReport(rp0, ENCAB2);
		}
	}
	DoReport(rp0, p_zona);

	if (p_zona==TOTAL && g_lin>=(MAXLINEA -15)){
		RpEjectPage(rp0);
		g_lin = ZONA_HEAD;
		DoReport(rp0, HEAD);

	}


	
}

