/********************************************************************
*
* MODULE & VERSION : @(#)genhspar.c	1.3
* DATE             : 05/03/30
* TIME             : 16:39:04
*
* CREATED          : 04/12/03
*
* DESCRIPTION:
*      Generación de Hs. Improductivas
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------

*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "operac.sch"
#include "sue.sch"
#include "genhspar.fmh"

#define MAXASIG      1000
#define MAXHORAS      100
#define RETEN_EN_OBJ    4
#define WAR_CERRADO  "El Parte está cerrado al día %.3D.\nNo podrá realizarse el proceso."

struct t_asig {
	long cliente;
	int  objetivo;
	long nroleg;
	int  ptoser;
	int  puesto;
	int  nroint;
	DATE fdesde;
	DATE fhasta;
	char dia1[2];
	char dia2[2];
	char dia3[2];
	char dia4[2];
	char dia5[2];
	char dia6[2];
	char dia7[2];
	char regim[8];
	char efect[2];
} pasig[MAXASIG], *uasig = pasig, *easig, *nasig;

struct t_horas {
	long nroleg;
	long cliente;
	int  objetivo;
	int  nroint;
	TIME horaent;
	TIME horasal;
} phoras[MAXHORAS], *uhoras = phoras, *ehoras, *nhoras;

/* Declaraciones globales */
schema   operac, sue;
form     fm0;
char g_prog[20];

void BuscarAsig();
void GrabarAsigH (DATE fecha);
void CargarAsig(long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint, char *dia1,
				char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, char * regim,
				DATE fdesde, DATE fhasta, char *efect);
void CargarHoras(long nroleg, long cliente, int obj, TIME horaent, TIME horasal);
static void	InicializarLista();

/* Programa principal */
wcmd(genhspar, 1.3 03/30/05)
{
	DATE fecierre;

	fm0    = OpenForm  ("genhspar", FM_EABORT);
	sue    = OpenSchema("sue",      IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);

	
	sprintf(g_prog, "%s", argv[0]);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE)
		return;

	fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));

	if (fecierre != NULL_DATE && FmDFld(fm0, FECD) <= fecierre)
		Error(WAR_CERRADO, fecierre);

	BeginTransaction();
	BuscarAsig();
	EndTransaction();
}

void BuscarAsig()
{
	dbcursor c_asig, c_asigh, c_parte;
	DATE fecha;
	int  toths = 0, tothsbri = 0, hsfaltantes = 0;
	bool grabo = FALSE, vacac = FALSE, ausen = FALSE;
	TIME entra = NULL_TIME, sale = NULL_TIME;

	c_asig  = CreateCursor(operac|ASIGbyEMP,     IO_NOT_LOCK);
	c_asigh = CreateCursor(operac|ASIGHbyPUESTO, IO_NOT_LOCK);
	c_parte = CreateCursor(operac|PARTEbyEMPLE,  IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT,
						  MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT,
						  MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		CargarAsig(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), LFld(operac|ASIG_NROLEG),
				   IFld(operac|ASIG_PTOSER),  IFld(operac|ASIG_PUESTO),   IFld(operac|ASIG_NROINT),
				   SFld(operac|ASIG_DIA1),    SFld(operac|ASIG_DIA2),     SFld(operac|ASIG_DIA3),
				   SFld(operac|ASIG_DIA4),    SFld(operac|ASIG_DIA5),     SFld(operac|ASIG_DIA6),
				   SFld(operac|ASIG_DIA7),    SFld(operac|ASIG_REGIM),    DFld(operac|ASIG_FECASIG),
				   IsNull(operac|ASIG_FECHAS) ? FmDFld(fm0, FECH) : DFld(operac|ASIG_FECHAS),
				   SFld(operac|ASIG_EFECT));
	}
	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT,
						  MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT,
						  MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		if (DFld(operac|ASIGH_FECALT) > FmDFld(fm0, FECH) ||
			DFld(operac|ASIGH_FECBAJ) < FmDFld(fm0, FECD))
			continue;

		CargarAsig(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), LFld(operac|ASIGH_NROLEG),
				   IFld(operac|ASIGH_PTOSER),  IFld(operac|ASIGH_PUESTO),   IFld(operac|ASIGH_NROINT),
				   SFld(operac|ASIGH_DIA1),    SFld(operac|ASIGH_DIA2),     SFld(operac|ASIGH_DIA3),
				   SFld(operac|ASIGH_DIA4),    SFld(operac|ASIGH_DIA5),     SFld(operac|ASIGH_DIA6),
				   SFld(operac|ASIGH_DIA7),    SFld(operac|ASIGH_REGIM),    DFld(operac|ASIGH_FECALT),
				   DFld(operac|ASIGH_FECBAJ),  SFld(operac|ASIGH_EFECT));
	}

	for (easig = pasig; easig < uasig; easig++) {
		for (fecha = easig->fdesde; fecha <= easig->fhasta; fecha++) {
			grabo = FALSE;
			toths = 0, tothsbri = 0, hsfaltantes = 0;

			InicializarLista();

			SetCursorFrom(c_parte, FmIFld(fm0, EMP), easig->nroleg, fecha, MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, EMP), easig->nroleg, fecha, MAX_LONG, MAX_SHORT);
			while (FetchCursor(c_parte) != ERROR) {
				if (TFld(operac|PARTE_HORAENT) != StrToT("00:00") ||
					TFld(operac|PARTE_HORASAL) != StrToT("00:00"))
					CargarHoras(easig->nroleg, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
							TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL));

				if (*SFld(operac|PARTE_CONDIC) == 'F' || *SFld(operac|PARTE_CONDIC) == 'A' ||
					*SFld(operac|PARTE_CONDIC) == 'V') {
					grabo = TRUE;
					break;
				}

  				if (LFld(operac|PARTE_CLIENTE)  == FmLFld(fm0, CLIENTE) &&
				   (IFld(operac|PARTE_OBJETIVO) >= FmIFld(fm0, OBJDESDE) && 
					IFld(operac|PARTE_OBJETIVO) <= FmIFld(fm0, OBJHASTA))) {
					tothsbri += IFld(operac|PARTE_HSNOR)  + IFld(operac|PARTE_HS50) +
								IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE);
				}
				else {
					toths += IFld(operac|PARTE_HSNOR)  + IFld(operac|PARTE_HS50) +
							 IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE);
				}
			}
			if (toths + tothsbri >= GetHsNormales(easig->regim, FALSE)) {
				continue;
			}
			if (!grabo) {
				vacac = FALSE, ausen = FALSE;
				entra = NULL_TIME, sale = NULL_TIME;

 				//Grabo el registro de horas improductivas.
				hsfaltantes = GetHsNormales(easig->regim, FALSE) - (toths + tothsbri);

				//Sumo las horas del 1016 (tothsbri) para grabar el parte.
				hsfaltantes = hsfaltantes + tothsbri;

				for (ehoras = phoras; ehoras < uhoras; ehoras++) {
					if (ehoras->nroleg != easig->nroleg)
						continue;
					if (entra == NULL_TIME) {
						entra = ehoras->horaent == NULL_TIME ? StrToT("00:00") : ehoras->horaent;
					}
					else {
						if (ehoras->horaent < entra)
							entra = ehoras->horaent;
		 			} 
					if (ehoras->horasal < ehoras->horaent)
						sale = StrToT("23:59");
					else {
						if (sale == NULL_TIME) {
							sale = ehoras->horasal == NULL_TIME ? StrToT("00:00") : ehoras->horasal;
						}
						else {
							if (ehoras->horasal > sale)
								sale = ehoras->horasal;
						}
					}
				}

				//El vigilador no tiene hora de entrada o salida porque los dias que tiene en el parte
				//estan con horas en 0. Si no se setean aca despues quedan mal las horas porque trabajan
				// con valores nulos.
				if (entra == NULL_TIME && sale == NULL_TIME) {
					entra = StrToT("00:00");
					sale  = StrToT("00:00");
				}

				if (entra - hsfaltantes >= StrToT("00:00")) {
					entra = StrToT("00:00");
					sale  = entra + ((hsfaltantes * _SEGUNDOS_POR_HORA) / 100);
				}
				else {
					if (sale + hsfaltantes <= StrToT("23:59")) {
						entra = sale;
						sale  = entra + (hsfaltantes / 100 * _SEGUNDOS_POR_HORA);
					}
					else {
						//horario intermedios
					}
				}

				SetIFld(operac|PARTE_EMP,      FmIFld(fm0, EMP));
				SetLFld(operac|PARTE_CLIENTE,  FmLFld(fm0, CLIENTE));
				SetIFld(operac|PARTE_OBJETIVO, FmIFld(fm0, OBJDESDE));
				SetDFld(operac|PARTE_DIA,      fecha);
				SetLFld(operac|PARTE_NROLEG,   easig->nroleg);
				SetIFld(operac|PARTE_PTOSER,   easig->ptoser);
				SetIFld(operac|PARTE_PUESTO,   easig->puesto);
				SetIFld(operac|PARTE_NROINT,   easig->nroint);
				if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
					SetTFld(operac|PARTE_HORAENT, entra);
					SetTFld(operac|PARTE_HORASAL, sale);
					if (FeriadoNovia(fecha, GetCliePais(FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE)),
									   GetClieProv(FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE))))
						SetIFld(operac|PARTE_HS100FE, hsfaltantes);
					else
						SetIFld(operac|PARTE_HSNOR,   hsfaltantes);

					AudiGrabaHorasParte(operac, g_prog);
					PutRecord(operac|PARTE);

					InitRecord(operac|EXCEPCION);
					SetIFld(operac|EXCEPCION_EMP,      FmIFld(fm0, EMP));
					SetLFld(operac|EXCEPCION_CLIENTE,  FmLFld(fm0, CLIENTE));
					SetIFld(operac|EXCEPCION_OBJETIVO, FmIFld(fm0, OBJDESDE));
					SetDFld(operac|EXCEPCION_DIA,      fecha);
					SetLFld(operac|EXCEPCION_NROLEG,   easig->nroleg);
					SetIFld(operac|EXCEPCION_CONDIC,   ACARGO_EMP);
					SetIFld(operac|EXCEPCION_MOTIVO,   RETEN_EN_OBJ);
					if (FeriadoNovia(fecha, GetCliePais(FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE)),
								   GetClieProv(FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJDESDE)))) {
						SetIFld(operac|EXCEPCION_HORAS, 0);
						SetIFld(operac|EXCEPCION_HS50,  0);
						SetIFld(operac|EXCEPCION_HS100, hsfaltantes);
				   }
					else {
						SetIFld(operac|EXCEPCION_HORAS, hsfaltantes);
						SetIFld(operac|EXCEPCION_HS50,  0);
						SetIFld(operac|EXCEPCION_HS100, 0);
					}
					SetFld (operac|EXCEPCION_OBS,      NULL_STR);
					SetIFld(operac|EXCEPCION_PTOSER,   easig->ptoser);
					SetIFld(operac|EXCEPCION_PUESTO,   easig->puesto);
					SetIFld(operac|EXCEPCION_NROINT,   easig->nroint);
					PutRecord(operac|EXCEPCION);
				}
			}
		}
	}
}

void CargarAsig(long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint, char *dia1,
				char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, char * regim,
				DATE fdesde, DATE fhasta, char *efect)
{
	bool otroefec = FALSE;

	for (easig = pasig; easig < uasig; easig++) {
		if (easig->nroleg == nroleg) {
			if (*efect == 'E') {
				otroefec = TRUE;
				break;
			}
			easig->fdesde = fdesde <= easig->fdesde ? fdesde : easig->fdesde;
			easig->fhasta = fhasta >= easig->fhasta ? fhasta : easig->fhasta;
			if (strcmp(easig->dia1, dia1))
				strcpy(easig->dia1, dia1);
			if (strcmp(easig->dia2, dia2))
				strcpy(easig->dia2, dia2);
			if (strcmp(easig->dia3, dia3))
				strcpy(easig->dia3, dia3);
			if (strcmp(easig->dia4, dia4))
				strcpy(easig->dia4, dia4);
			if (strcmp(easig->dia5, dia5))
				strcpy(easig->dia5, dia5);
			if (strcmp(easig->dia6, dia6))
				strcpy(easig->dia6, dia6);
			if (strcmp(easig->dia7, dia7))
				strcpy(easig->dia7, dia7);
			break;
		}
	}

	if (easig == uasig || otroefec) {
		if (uasig == &pasig[MAXASIG])
			Error("Tabla interna saturada. Max %d", MAXASIG);

		uasig->nroleg   = nroleg;
		uasig->cliente  = cliente;
		uasig->objetivo = objetivo;
		uasig->ptoser   = ptoser;
		uasig->puesto   = puesto;
		uasig->nroint   = nroint;
		uasig->fdesde   = fdesde < FmDFld(fm0, FECD) ? FmDFld(fm0, FECD) : fdesde;
		uasig->fhasta   = fhasta > FmDFld(fm0, FECH) ? FmDFld(fm0, FECH) : fhasta;
		strcpy(uasig->dia1, dia1);
		strcpy(uasig->dia2, dia2);
		strcpy(uasig->dia3, dia3);
		strcpy(uasig->dia4, dia4);
		strcpy(uasig->dia5, dia5);
		strcpy(uasig->dia6, dia6);
		strcpy(uasig->dia7, dia7);
		strcpy(uasig->regim, regim);
		uasig++;
	}
}

void CargarHoras(long nroleg, long cliente, int obj, TIME horaent, TIME horasal)
{
	for (ehoras = phoras; ehoras < uhoras; ehoras++) {
		if (ehoras->nroleg == nroleg && ehoras->cliente == cliente && ehoras->objetivo == obj) {
			ehoras->nroint  = ehoras->nroint + 1;
			ehoras->horaent = horaent;
			ehoras->horasal = horasal;
			break;
		}
		uhoras->nroleg   = nroleg;
		uhoras->cliente  = cliente;
		uhoras->objetivo = obj;
		uhoras->horaent  = horaent;
		uhoras->horasal  = horasal;
		uhoras++;
		break;
	}

	if (ehoras == uhoras) {
		if (uhoras == &phoras[MAXHORAS])
			Error("Tabla interna saturada. Max %d", MAXHORAS);

		uhoras->nroleg   = nroleg;
		uhoras->cliente  = cliente;
		uhoras->objetivo = obj;
		uhoras->nroint   = 1;
		uhoras->horaent  = horaent;
		uhoras->horasal  = horasal;
		uhoras++;
	}
}

static void	InicializarLista()
{
	int i = 0;
	if (i < MAXHORAS) {
		phoras[i].nroleg   = NULL_LONG;
		phoras[i].cliente  = NULL_LONG;
		phoras[i].objetivo = NULL_SHORT;
	}
	uhoras = phoras;
}


/*				if (Vacaciones(FmIFld(fm0, EMP), easig->nroleg, fecha))
					vacac = TRUE;

				if (TieneLic(FmIFld(fm0, EMP), easig->nroleg, fecha))
					ausen = TRUE;
*/
//				if (!vacac && !ausen) {

/*					if (vacac) {
						SetFld (operac|PARTE_CONDIC,  "V");
						SetTFld(operac|PARTE_HORAENT, StrToT("00:00"));
						SetTFld(operac|PARTE_HORASAL, StrToT("00:00"));
					}
					if (ausen) {
						SetFld (operac|PARTE_CONDIC,  "A");
						SetTFld(operac|PARTE_HORAENT, StrToT("00:00"));
						SetTFld(operac|PARTE_HORASAL, StrToT("00:00"));
					}
*/
//					if (!vacac && !ausen) {

//				if (!vacac && !ausen) {


//					SetTFld(operac|PARTE_HORAENT, StrToT("00:00"));
//					SetTFld(operac|PARTE_HORASAL, StrToT("00:00") + ((hsfaltantes + tothsbri) / 100 * _SEGUNDOS_POR_HORA));
//					SetIFld(operac|PARTE_HSNOR,   hsfaltantes + tothsbri);


