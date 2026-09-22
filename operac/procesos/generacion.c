#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "opedef.h"
#include "genparte.rph"
#include "operac.sch"
#include "asist.sch"

#define WAR_PARTE_MODIF "El parte correspondiente a la Empresa: %d Cliente: %ld Objetivo: %d\nFecha: %.1D Puesto %d - %d Vigilador: %ld ya tiene horas cargadas.\nDesea regenerarlo?"
#define ERR_MEMORIA     "No hay más memoria!!!!!!!!!!!!!"

/* estructura para manejar a los vigiladores asignados en el 3000 1*/
typedef struct t_asig {
	long cliente;
	int  objetivo;
	int  ptoser;
	int  puesto;
	int  nroint;
	DATE fdesde;
	DATE fhasta;
	TIME hdesde;
	TIME hhasta;
	int  numfran;
	char efec[3];
	char dia1[2];
	char dia2[2];
	char dia3[2];
	char dia4[2];
	char dia5[2];
	char dia6[2];
	char dia7[2];
	char vigil[2];
	char regim[8];
    short rrol, rfila, rcol; 
	struct t_asig *sgte;
}	n_asig;
typedef n_asig *p_asig;
p_asig lista = NULL;

typedef struct t_hs {
	TIME hdesde;
	TIME hhasta;
	bool todo;        //todo=1 abarca toda la hora; todo=0 no abarca nada de esa hora.
}hs;
struct t_hs horario[24];

extern schema operac;
extern schema asist;
extern dbcursor c_asig, c_asigah;
extern dbtable  ALASIG, ALASIGH, APARTE;
extern bool imprimir;
extern report rp;
extern FILE *fp;

void GenParte(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde, DATE fechasta,
			  int conf_regen, int prg_form);
void GenParteH(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde, DATE fechasta,
			   int conf_regen, int prg_form);
void LimpiarParte(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde,
				  DATE fechasta, int conf_regen, int prg_form);
bool BuscarHoras(int emp, DATE fecha, TIME horaent, TIME horasal, TIME *hsent, TIME *hhsal);
void InicializarHorario(TIME horaent, TIME horasal);
bool TodoCubierto();
void NuevoHorario(TIME *hsent, TIME *hssal, TIME horaent, TIME horasal);
bool DebeTrabajar(char *efectivo, int emp, long nroleg, int ptoser, int puesto, int nroint,DATE fecha, char *dia1,
				  char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7, char *vigil,
				  long cliente, int objetivo,int numfran, char *regim);
void HsPTime(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
			 DATE fecha, TIME *hsent, TIME *hssal);
void DelParte(struct Asig *pasig, int emp, DATE fecdesde, DATE fechasta, int conf_regen);
void GraboParte(struct Asig *pasig, int emp, DATE fecdesde, DATE fechasta, int conf_regen, int prg_form,
				bool fromasig);
void AsigHora(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
			  DATE dia, TIME * hsentrada, TIME * hssalida, char *regim, int prg_form);
bool ClienteEspecial(long cliente, int objetivo, short *aviso, bool *ausentismo, char *condi, long nroleg);

private p_asig ArmarLista(int emp, long nroleg, DATE fdesde, DATE fhasta);
private p_asig CrearNodo();
private void   FreeLista();

void LimpiarParte(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde,
				  DATE fechasta, int conf_regen, int prg_form)
{
	struct Asig asig;
	dbcursor  c_asigh;
	c_asigh = CreateCursor(operac|ASIGHbyFECHABAJ, IO_NOT_LOCK);

	SetCursorFrom(c_asigh, emp, cliented, objetd, MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asigh, emp, clienteh, objeth, MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		if (!IsNull(operac|ASIGH_MOTIVO)) {
			SetKey(operac|MOTIVDbyCODMOTD, IFld(operac|ASIGH_MOTIVO));
			if (GetRecord(operac|MOTIVDbyCODMOTD, THIS_KEY, IO_NOT_LOCK) == ERROR ||
											  IFld(operac|MOTIVD_M_INC) == FALSE)
				continue;
		}

		if (GetServicioObj(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)) == BRIGADA)
			continue;

		asig.emp      = IFld(operac|ASIGH_EMP);
		asig.cliente  = LFld(operac|ASIGH_CLIENTE);
		asig.objetivo = IFld(operac|ASIGH_OBJETIVO);
		asig.ptoser   = IFld(operac|ASIGH_PTOSER);
		asig.puesto   = IFld(operac|ASIGH_PUESTO);
		asig.nroint   = IFld(operac|ASIGH_NROINT);
		asig.nroleg   = LFld(operac|ASIGH_NROLEG);
		asig.fecasig  = DFld(operac|ASIGH_FECALT);
		asig.hsent    = TFld(operac|ASIGH_HSENT);
		asig.hssal    = TFld(operac|ASIGH_HSSAL);
		sprintf(asig.vigil, "%s", SFld(operac|ASIGH_VIGIL));
		sprintf(asig.efect, "%s", SFld(operac|ASIGH_EFECT));
		sprintf(asig.dia1,  "%s", SFld(operac|ASIGH_DIA1));
		sprintf(asig.dia2,  "%s", SFld(operac|ASIGH_DIA2));
		sprintf(asig.dia3,  "%s", SFld(operac|ASIGH_DIA3));
		sprintf(asig.dia4,  "%s", SFld(operac|ASIGH_DIA4));
		sprintf(asig.dia5,  "%s", SFld(operac|ASIGH_DIA5));
		sprintf(asig.dia6,  "%s", SFld(operac|ASIGH_DIA6));
		sprintf(asig.dia7,  "%s", SFld(operac|ASIGH_DIA7));
		asig.reempl  = LFld(operac|ASIGH_REEMPL);
		asig.ffranco = DFld(operac|ASIGH_FFRANCO);
		asig.numfran = IFld(operac|ASIGH_NUMFRAN);
		asig.francero= IFld(operac|ASIGH_FRANCERO);
		sprintf(asig.regim, "%s", SFld(operac|ASIGH_REGIM));
		asig.rrol  = IFld(operac|ASIGH_CODROL);
		asig.rfila = IFld(operac|ASIGH_FILA);
		asig.rcol  = IFld(operac|ASIGH_COLUM);
		asig.fechas = DFld(operac|ASIGH_FECHAS);
		asig.fecbaj = DFld(operac|ASIGH_FECBAJ);
		if(asig.fechas == NULL_DATE) {
			asig.fechas = (asig.fecbaj < fechasta ? asig.fecbaj : fechasta);
		}
		DelParte(&asig, emp, fecdesde, fechasta, conf_regen);
		GraboParte(&asig, emp, fecdesde, fechasta, conf_regen, prg_form, FALSE);
	}
	DeleteCursor(c_asigh);
}

void GenParte(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde, DATE fechasta,
			  int conf_regen, int prg_form)
{
	struct Asig asig;
	dbcursor c_ASIG;
//	p_asig	aux;
//	char buffer[50];

	c_ASIG = CreateCursor(operac|ASIGbyFECHA, IO_NOT_LOCK);

	SetCursorFrom(c_ASIG, emp, cliented, objetd, MIN_DATE, MIN_LONG);
	SetCursorTo  (c_ASIG, emp, clienteh, objeth, MAX_DATE, MAX_LONG);
	while (FetchCursor(c_ASIG) != ERROR) {
		asig.emp      = IFld(operac|ASIG_EMP);
		asig.cliente  = LFld(operac|ASIG_CLIENTE);
		asig.objetivo = IFld(operac|ASIG_OBJETIVO);
		asig.ptoser   = IFld(operac|ASIG_PTOSER);
		asig.puesto   = IFld(operac|ASIG_PUESTO);
		asig.nroint   = IFld(operac|ASIG_NROINT);
		asig.nroleg   = LFld(operac|ASIG_NROLEG);
		sprintf(asig.vigil, "%s", SFld(operac|ASIG_VIGIL));
		sprintf(asig.efect, "%s", SFld(operac|ASIG_EFECT));
		asig.fecasig = DFld(operac|ASIG_FECASIG);
		asig.hsent   = TFld(operac|ASIG_HSENT);
		asig.hssal   = TFld(operac|ASIG_HSSAL);
		sprintf(asig.dia1, "%s", SFld(operac|ASIG_DIA1));
		sprintf(asig.dia2, "%s", SFld(operac|ASIG_DIA2));
		sprintf(asig.dia3, "%s", SFld(operac|ASIG_DIA3));
		sprintf(asig.dia4, "%s", SFld(operac|ASIG_DIA4));
		sprintf(asig.dia5, "%s", SFld(operac|ASIG_DIA5));
		sprintf(asig.dia6, "%s", SFld(operac|ASIG_DIA6));
		sprintf(asig.dia7, "%s", SFld(operac|ASIG_DIA7));
		asig.reempl   = LFld(operac|ASIG_REEMPL);
		asig.ffranco  = DFld(operac|ASIG_FFRANCO);
		asig.numfran  = IFld(operac|ASIG_NUMFRAN);
		asig.francero = IFld(operac|ASIG_FRANCERO);
		sprintf(asig.regim, "%s", SFld(operac|ASIG_REGIM));
		asig.fechas = DFld(operac|ASIG_FECHAS);
		asig.fecbaj = DFld(operac|ASIG_FECBAJ);
		asig.rrol  = IFld(operac|ASIG_CODROL);
		asig.rfila = IFld(operac|ASIG_FILA);
		asig.rcol  = IFld(operac|ASIG_COLUM);

//		sprintf (buffer, "Procesando Cliente %ld Objetivo %d", asig.cliente, asig.objetivo);
//		FmSetFld (fm0, COMENT, buffer);
//		WiRefresh();

	   	GraboParte(&asig, emp, fecdesde, fechasta, conf_regen, prg_form, TRUE);
	}
	DeleteCursor(c_ASIG);
//	FmSetFld (fm0, COMENT, NULL_STR);
//	WiRefresh();
}

void GenParteH(int emp, long cliented, long clienteh, int objetd, int objeth, DATE fecdesde, DATE fechasta,
			   int conf_regen, int prg_form)
{
	struct Asig asig;
	dbcursor c_ASIGH;
//	p_asig	aux;
//	char buffer[50];
	DATE ffin;

	c_ASIGH = CreateCursor(operac|ASIGHbyEMP, IO_NOT_LOCK);
	SetCursorFrom(c_ASIGH, emp, cliented, objetd, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE,
							MIN_DATE);
	SetCursorTo  (c_ASIGH, emp, clienteh, objeth, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG, MAX_DATE,
							MAX_DATE);
	while (FetchCursor(c_ASIGH) != ERROR) {
	
//		sprintf (buffer, "Procesando ASIGH Cliente %ld Objetivo %d", asig.cliente, asig.objetivo);
//		FmSetFld (fm0, COMENT, buffer);
//		WiRefresh();

		if (GetServicioObj(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)) == BRIGADA)
			continue;

		ffin = DFld(operac|ASIGH_FECBAJ) != NULL_DATE ? DFld(operac|ASIGH_FECBAJ) : DFld(operac|ASIGH_FECHAS);

		if (asig.fecasig > fechasta)
			continue;                                                        //--- Empieza despues ---//
		if (ffin != NULL_DATE && ffin < fecdesde)
			continue;                                                        //--- Ya termino ---//
		if (DFld(ASIGH_FECALT) > ffin || IFld(ASIGH_MOTIVO) == DESXERROR )
			continue;                                                        //--- Mal asignado ---//

		asig.emp      = IFld(operac|ASIGH_EMP);
		asig.cliente  = LFld(operac|ASIGH_CLIENTE);
		asig.objetivo = IFld(operac|ASIGH_OBJETIVO);
		asig.ptoser   = IFld(operac|ASIGH_PTOSER);
		asig.puesto   = IFld(operac|ASIGH_PUESTO);
		asig.nroint   = IFld(operac|ASIGH_NROINT);
		asig.nroleg   = LFld(operac|ASIGH_NROLEG);
		sprintf(asig.vigil, "%s", SFld(operac|ASIGH_VIGIL));
		sprintf(asig.efect, "%s", SFld(operac|ASIGH_EFECT));
		asig.fecasig = DFld(operac|ASIGH_FECALT);
		asig.hsent   = TFld(operac|ASIGH_HSENT);
		asig.hssal   = TFld(operac|ASIGH_HSSAL);
		sprintf(asig.dia1, "%s", SFld(operac|ASIGH_DIA1));
		sprintf(asig.dia2, "%s", SFld(operac|ASIGH_DIA2));
		sprintf(asig.dia3, "%s", SFld(operac|ASIGH_DIA3));
		sprintf(asig.dia4, "%s", SFld(operac|ASIGH_DIA4));
		sprintf(asig.dia5, "%s", SFld(operac|ASIGH_DIA5));
		sprintf(asig.dia6, "%s", SFld(operac|ASIGH_DIA6));
		sprintf(asig.dia7, "%s", SFld(operac|ASIGH_DIA7));
		asig.reempl   = LFld(operac|ASIGH_REEMPL);
		asig.ffranco  = DFld(operac|ASIGH_FFRANCO);
		asig.numfran  = IFld(operac|ASIGH_NUMFRAN);
		asig.francero = IFld(operac|ASIGH_FRANCERO);
		sprintf(asig.regim, "%s", SFld(operac|ASIGH_REGIM));
		asig.fechas = DFld(operac|ASIGH_FECHAS);
		asig.fecbaj = DFld(operac|ASIGH_FECBAJ);
		asig.rrol  = IFld(operac|ASIGH_CODROL);
		asig.rfila = IFld(operac|ASIGH_FILA);
		asig.rcol  = IFld(operac|ASIGH_COLUM);

	   	GraboParte(&asig, emp, fecdesde, fechasta, conf_regen, prg_form, TRUE);
	}
	DeleteCursor(c_ASIGH);
//	FmSetFld (fm0, COMENT, NULL_STR);
//	WiRefresh();
}

void GraboParte(struct Asig *pasig, int emp, DATE fecdesde, DATE fechasta, int conf_regen, int prg_form, 
				bool fromasig)
{
	DATE fecha,	fechahasta; 
	long nroleg	= NULL_LONG;
	TIME hsent,	hssal, hsentrada, hssalida;
	char condicion[5];
	bool especial, grabaus;
	short avisado;
//	int i, columna;

	// si las asignación es despues del rango pedido por pantalla continue!
	if ((pasig->fecasig) > fechasta) {
		return;
	}

	// si las asignación termina antes del rango pedido por pantalla continue!
	if (pasig->fecbaj != NULL_DATE && pasig->fecbaj < fecdesde) {
		return;
	}

	// Genero desde la fdesde del form solo si la fecha de asig es menor a la fdesde
	// de lo contrario, la asignación del empleado es posterior a la fdesde del form
	// con lo que los partes a generarse deben ser igual o mayor a la fecha de asig.
	fecha =	pasig->fecasig < fecdesde ? fecdesde : pasig->fecasig;

	// a los provisorios solo se le debe generar el parte hasta la fecha ASIG_FECBAJ.
	fechahasta = pasig->fecbaj != NULL_DATE ? ((fechasta > pasig->fecbaj) ?
				 pasig->fecbaj  : fechasta) : fechasta; 

	if (nroleg != pasig->nroleg && ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
		if (nroleg != NULL_LONG)
			FreeLista();
		lista  = ArmarLista(emp, pasig->nroleg, fecha, fechahasta);
		nroleg = pasig->nroleg;
	}

	// Tengo un cliente/objetivo, encontrar los dias en el rango fecha-fechahasta
	// Grabo uno por uno los registros o acumulo los dias para imprimir
	while (fecha <= fechahasta) {
		char valor[10];

		// valido si ese dia esta asignado a ese cli-obj ese dia
		// solamente si el puesto no es el efectivo!.
		if (!DebeTrabajar(pasig->efect, emp, pasig->nroleg, pasig->ptoser, pasig->puesto,
						  pasig->nroint, fecha,
						  pasig->dia1, pasig->dia2, pasig->dia3, pasig->dia4, pasig->dia5,
						  pasig->dia6, pasig->dia7,
						  pasig->vigil, pasig->cliente,
						  pasig->objetivo, pasig->numfran, pasig->regim)) {
			fecha = fecha + 1;
			continue;
		}

		SetKey(operac|PARTEbyEMP, emp, pasig->cliente, pasig->objetivo, fecha, pasig->nroleg,
								  pasig->ptoser, pasig->puesto, pasig->nroint);
		// esto lo que hace es avisar si el parte a regenerar ya fue modificado, si fue modificado,
		// pregunta si se lo quiere regenerar y actúa en función a la respuesta.
		if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (!fromasig) {
				fecha++;
				continue;
			}
			if (FFld(operac|PARTE_HSNOR)  != 0.00 || FFld(operac|PARTE_HS50)    != 0.00 ||
				FFld(operac|PARTE_HS100F) != 0.00 || FFld(operac|PARTE_HS100FE) != 0.00) {
				if (conf_regen)  {
					if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, WAR_PARTE_MODIF,
						emp, pasig->cliente, pasig->objetivo, fecha, pasig->ptoser,
						pasig->puesto, pasig->nroleg) == WD_NO) {
						fecha = fecha + 1;
						continue;
					}
				}
				else {
					fecha++;
					continue;
				}
			}
		}
		// Inicializo el registro. Si tenía horas cargadas serán borradas
		// ya que antes se dio la posibilidad al usu. de decidir esto.
		InitRecord(operac|PARTE);
		SetKey(operac|PARTEbyEMP, emp, pasig->cliente, pasig->objetivo, fecha, pasig->nroleg,
									   pasig->ptoser, pasig->puesto, pasig->nroint);
		SetFFld(operac|PARTE_HSNOR,   0.00);
		SetFFld(operac|PARTE_HS50,    0.00);
		SetFFld(operac|PARTE_HS100F,  0.00);
		SetFFld(operac|PARTE_HS100FE, 0.00);

		especial = ClienteEspecial(pasig->cliente, pasig->objetivo, &avisado, &grabaus, condicion, pasig->nroleg);
		if (especial) {

			SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
			SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
			SetFld (operac|PARTE_CONDIC,  condicion);
			SetIFld(operac|PARTE_CODAUS,  avisado);
			SetDFld(operac|PARTE_FECGEN,  Today());
			
			AudiGrabaHorasParte(operac, gg_prog);
		 	PutRecord(operac|PARTE);
			FreeTable(operac|PARTE);

			if (grabaus && !TieneLic(emp, pasig->nroleg, fecha)) {
				InitRecord(asist|ASISTEN);
				SetIFld(asist|ASISTEN_EMPRE,  emp);
				SetDFld(asist|ASISTEN_FECHA,  fecha);
				SetLFld(asist|ASISTEN_NROLEG, pasig->nroleg);
				SetIFld(asist|ASISTEN_CODNOV, IFld (operac|PARTE_CODAUS));
				SetLFld(asist|ASISTEN_VALOR,  100);
				SetIFld(asist|ASISTEN_JUSTIF, TRUE); /* seteo con TRUE para que en ASISTEN_JUSTIF sea un NO */
				PutRecord(asist|ASISTEN);
				FreeTable(asist|ASISTEN);
			}
			fecha = fecha + 1;
			continue;
		}
		if (Vacaciones(emp, pasig->nroleg, fecha)) {
			SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
			SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
			SetFld (operac|PARTE_CONDIC,  _VACACIONES);
		}
		else {
			if ((ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)  ||
				 ExisteCliObjEnGrp(GRPTRAFRA,    pasig->cliente, pasig->objetivo)) &&
				(TieneLic(emp, pasig->nroleg, fecha) ||
				 Falto(emp, pasig->nroleg, fecha))) {
				SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
				SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
				SetFld (operac|PARTE_CONDIC,  "A");
				SetIFld(operac|PARTE_CODAUS,  _CON_AVISO);
			}
			else {
				if (!TieneLic(emp, pasig->nroleg, fecha) &&
					Falto(emp, pasig->nroleg, fecha)) {
					SetIFld(asist|ASISTEN_EMPRE,  emp);
					SetDFld(asist|ASISTEN_FECHA,  fecha);
					SetLFld(asist|ASISTEN_NROLEG, pasig->nroleg);
					SetIFld(asist|ASISTEN_CODNOV, NULL_SHORT);
					while (GetRecord(asist|ASISTENbyEMPRE, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
						DelRecord(asist|ASISTEN);
					}
			  	}

				if (Franco(emp, pasig->nroleg, fecha, pasig->vigil, pasig->numfran)) {
					SetFld (operac|PARTE_CONDIC,  _FRANCO);

					if (ExisteCliObjEnGrp(GRPTRAFRA, pasig->cliente, pasig->objetivo)) {
						SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
						SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
					}
					else {
						if (!strcmp(pasig->efect,  EFECTIVO)) {
							SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
							SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
						}
						else {
							if (ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
								if (!BuscarHoras(emp, fecha, pasig->hsent, pasig->hssal, &hsent, &hssal)) {
									fecha = fecha + 1;
									continue;
								}
								SetTFld(operac|PARTE_HORAENT, hsent == NULL_TIME ? StrToT("0000") : hsent);
								SetTFld(operac|PARTE_HORASAL, hssal == NULL_TIME ? StrToT("0000") : hssal);
							}
							else {
								if (StrCmp(pasig->vigil, PARTTIME) == 0 && StrCmp(pasig->dia1, "P") == 0) {
									HsPTime(emp, pasig->cliente, pasig->objetivo, pasig->nroleg,
											pasig->ptoser, pasig->puesto, pasig->nroint, fecha, &hsent,
											&hssal);

									SetTFld(operac|PARTE_HORAENT, hsent);
									SetTFld(operac|PARTE_HORASAL, hssal);
								}
								else {
									SetTFld(operac|PARTE_HORAENT, pasig->hsent);
									SetTFld(operac|PARTE_HORASAL, pasig->hssal);
								}
							}
						}
					}
				}
				else {
					if (ExisteCliObjEnGrp(GRPRETPLANTA, pasig->cliente, pasig->objetivo)) {
						if (!BuscarHoras(emp, fecha, pasig->hsent, pasig->hssal, &hsent, &hssal)) {
							fecha = fecha + 1;
							continue;
						}
						SetTFld(operac|PARTE_HORAENT, hsent == NULL_TIME ? StrToT("0000") : hsent);
						SetTFld(operac|PARTE_HORASAL, hssal == NULL_TIME ? StrToT("0000") : hssal);
					}
					else {
						if (ExisteCliObjEnGrp(GRPTRAFRA, pasig->cliente, NULL_SHORT)) {
							SetTFld(operac|PARTE_HORAENT, StrToT("0000"));
							SetTFld(operac|PARTE_HORASAL, StrToT("0000"));
						}
						else {
							if (StrCmp(pasig->vigil, PARTTIME) == 0 && StrCmp(pasig->dia1, "P") == 0) {
								HsPTime(emp, pasig->cliente, pasig->objetivo, pasig->nroleg, pasig->ptoser,
										pasig->puesto, pasig->nroint, fecha, &hsent, &hssal);
								SetTFld(operac|PARTE_HORAENT, hsent);
								SetTFld(operac|PARTE_HORASAL, hssal);
							}
							else {
								SetTFld(operac|PARTE_HORAENT, pasig->hsent);
								SetTFld(operac|PARTE_HORASAL, pasig->hssal);
							}
						}
					}
					SetFld (operac|PARTE_CONDIC, _TRABAJA);
				}
			}
		}
		if (pasig->rrol != NULL_SHORT){
			/*Calcula el horario en base al rol que esta asignado el vigilador */
			GetHorasTurno (pasig->emp, pasig->nroleg, pasig->cliente, pasig->objetivo, pasig->fecasig, pasig->rrol,
						pasig->rfila, pasig->rcol, 
						!str_eq(pasig->regpto, NULL_STR) ? pasig->regpto : pasig->regim,
						pasig->hsent, pasig->hssal, fecha, 
						pasig->ptoser, pasig->puesto,
						pasig->dia1, pasig->dia2, pasig->dia3, pasig->dia4, pasig->dia5,
						pasig->dia6, pasig->dia7, valor, pasig->vigil, pasig->numfran, &hsentrada, &hssalida);


			if (str_eq(valor, _NO_TRABAJA)) {
				fecha++;
				continue;

			}
			SetTFld(operac|PARTE_HORAENT,  hsentrada);
			SetTFld(operac|PARTE_HORASAL,  hssalida); 
		}
		else {
			if ((!StrCmp(pasig->regim, REG_ESP) || !StrCmp(pasig->regim, REG_ESP_2) || !StrCmp(pasig->regim, REG_ESP_3)) &&
				str_eq (SFld(operac|PARTE_CONDIC), _TRABAJA) && pasig->francero == TRUE) {
				AsigHora(pasig->emp, pasig->cliente, pasig->objetivo, pasig->nroleg, pasig->ptoser,
						pasig->puesto, pasig->nroint, fecha, &hsentrada, &hssalida, pasig->regim, prg_form);
				SetTFld(operac|PARTE_HORAENT,  hsentrada);
				SetTFld(operac|PARTE_HORASAL,  hssalida); 
			}
		}
		SetDFld(operac|PARTE_FECGEN, Today());
		AudiGrabaHorasParte(operac, gg_prog);
		PutRecord(operac|PARTE);
		
		FreeTable(operac|PARTE);
		
		fecha = fecha + 1;
	}
}

void HsPTime(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint, DATE fecha,
			 TIME *hsent, TIME *hssal)
{
	SetKey(operac|DIASPTIMEbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
	if (GetRecord(operac|DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		*hsent = TFld(operac|DIASPTIME_HENT);
		*hssal = TFld(operac|DIASPTIME_HSAL);
	}
	else {
		SetKey(operac|DIASPTIMEHbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
		if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			*hsent = TFld(operac|DIASPTIMEH_HENT);
			*hssal = TFld(operac|DIASPTIMEH_HSAL);
		}
	}
}

void DelParte(struct Asig *pasig, int emp, DATE fecdesde, DATE fechasta, int conf_regen)
{
	DATE fechahasta;
	static dbcursor c_parte=(dbcursor)NULL;

	if (c_parte == (dbcursor)NULL) {
		c_parte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
	}

	if (pasig->fecbaj < fecdesde || pasig->fecbaj > fechasta)
		return;

	fechahasta = pasig->fechas == NULL_DATE ? MAX_DATE : pasig->fechas;
	SetCursorFrom(c_parte, pasig->emp, pasig->cliente, pasig->objetivo,
					pasig->fecasig > fecdesde ? pasig->fecasig : fecdesde, // f.desde
					MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_parte, pasig->emp, pasig->cliente, pasig->objetivo,	fechahasta, MAX_LONG, MAX_SHORT,
					MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_parte) != ERROR) {
		if (pasig->nroleg != LFld(operac|PARTE_NROLEG)) {
			continue;
		}
		// solo debe borrar los partes que corresponden al ASIGH.
		if (pasig->puesto != IFld(operac|PARTE_PUESTO) || pasig->ptoser != IFld(operac|PARTE_PTOSER) ||
			pasig->nroint != IFld(operac|PARTE_NROINT))
			continue;

		if (FFld(operac|PARTE_HSNOR)  != 0.00 || FFld(operac|PARTE_HS50)    != 0.00 ||
			FFld(operac|PARTE_HS100F) != 0.00 || FFld(operac|PARTE_HS100FE) != 0.00) {
			if (conf_regen) {
				if (WiDialog(WD_YES|WD_NO, WD_NO, NULL_STR, WAR_PARTE_MODIF, emp, pasig->cliente,
					pasig->objetivo, DFld(operac|PARTE_DIA), IFld(operac|PARTE_PTOSER),
					IFld(operac|PARTE_PUESTO), pasig->nroleg) == WD_NO) {
					continue;
				}
			}
			else
				continue;
		}
		DelRecord(operac|PARTE);
		FreeTable(operac|PARTE);
	}
}

bool ClienteEspecial(long cliente, int objetivo, short *aviso, bool *ausentismo, char *condi, long nroleg)
{
	int salida = 0;

	*aviso = NULL_SHORT;
	*ausentismo = FALSE;
	strcpy(condi, NULL_STR);
	
	if (ExisteCliObjEnGrp(GRPCONAVISO, cliente, objetivo)) {
		*aviso = _CON_AVISO;
	}
	else {
		if (ExisteCliObjEnGrp(GRPSINAVISO, cliente, objetivo)) {
			*aviso = _SIN_AVISO;
		}
		else {
			if (ExisteCliObjEnGrp(GRPNULLAUS, cliente, objetivo)) {
				*aviso = NULL_SHORT;
			}
	        else
				salida++;
		}
	}

	if (ExisteCliObjEnGrp(GRPGRABAINS, cliente, objetivo)) {
		*ausentismo = TRUE;
	}
	else {
		if (ExisteCliObjEnGrp(GRPNOGRABAINS, cliente, objetivo)) {
			*ausentismo = FALSE;
		}
		else
			salida++;
	}

	if (ExisteCliObjEnGrp(GRPAUSENTE, cliente, objetivo)) {
		strcpy (condi, _AUSENTE);
	}
	else {
		if (ExisteCliObjEnGrp(GRPTRABAJA, cliente, objetivo)) {
			strcpy (condi, _TRABAJA);
		}
		else
			salida++;
	}

	if (salida == 3)
		return FALSE;
	else
		return TRUE;
}

void AsigHora(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto, int nroint,
			  DATE dia, TIME * hsentrada, TIME * hssalida, char *regim, int prg_form)
{
	/********************************************************************************************
	Esta funcion devuelve la hora de entrada y salida 
	Solo se puede usar para un franquero (que rota el horario a cubrir)
	Solo se puede usar para un regimen 4x2x12 (donde rota el horario cada 2 dias -cantdiasP-) o
	para un regimen  8x4x12 (donde rota el horario cada 4 dias -cantdiasP-).
	********************************************************************************************/

	static dbtable AASIG, AASIGH;
	short diaslab, dias;
	DATE fecha, fechaA;
	bool impre = FALSE, hay_otros = FALSE;
	short cantdias=0, cantdiasP=0, diasarecorrer = 0;
	TIME hantent, hantsal;
	static dbcursor c_Asig=ERROR, c_Asigh=ERROR;

	if (c_Asig==ERROR)
		c_Asig = CreateCursor(AlInd(ALASIG,operac|ASIGbyPUESTO), IO_NOT_LOCK);

	if (c_Asigh==ERROR)
		c_Asigh = CreateCursor(AlInd(ALASIGH, operac|ASIGHbyEMP), IO_NOT_LOCK);


	if (strcmp(regim, REG_ESP_3)==0)
		cantdiasP = (int)GetDiasLaboral(regim, FALSE)/2;

	else 
		cantdiasP = GetDiasFranco(regim, FALSE);

	diasarecorrer =  GetDiasLaboral(regim, FALSE) + GetDiasFranco(regim, FALSE);

	*hsentrada = StrToT("0000");
	*hssalida  = StrToT("0000");

    if (impre) fprintf (stderr, "\n\nASIG HORA %d %ld %d %ld dia %.3D \n", emp, cliente, objetivo, nroleg, dia);

	if (!AASIG) {
		AASIG  = CreateAlias (operac|ASIG);
		AASIGH = CreateAlias (operac|ASIGH);
	}

	/*Leo la asignacion */
	SetKey (AASIGbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint);
	if (GetRecord (AASIGbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR || DFld (AASIG_FECASIG) > dia ) {
		bool encontro=FALSE;
		
		SetKey (AASIGHbyEMP, emp, cliente, objetivo, ptoser, puesto, nroint, nroleg, MIN_DATE, MIN_DATE);
		while (!encontro && GetRecord (AASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			if (DFld (AASIGH_FECALT) <= dia && DFld (AASIGH_FECBAJ) >= dia) {
				encontro = TRUE;
			} 
		}

		if (!encontro) {
			if (impre) fprintf (stderr, "No encontro puesto \n");
			return;
		} 
		
		CopyFld (AASIGH_NROLEG,  AASIG_NROLEG);
		CopyFld (AASIGH_VIGIL ,  AASIG_VIGIL);
		CopyFld (AASIGH_NUMFRAN, AASIG_NUMFRAN);
		CopyFld (AASIGH_FECALT,  AASIG_FECASIG);
		CopyFld (AASIGH_HSENT,   AASIG_HSENT);
		CopyFld (AASIGH_HSSAL,   AASIG_HSSAL);
		CopyFld (AASIGH_FFRANCO, AASIG_FFRANCO);
		CopyFld (AASIGH_REGIM,   AASIG_REGIM);
	} 

	/*Si tiene franco o vacaciones no calculo nada */
	if (Franco (emp, LFld (AASIG_NROLEG), dia, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
		if (impre) fprintf (stderr, "%.3D %ld Esta de franco FIN\n", dia, nroleg);
		return;
	} 

	if (Vacaciones(emp, nroleg, fecha)) {
		if (impre) fprintf (stderr, "%.3D %ld Esta de vacaciones FIN\n", dia, nroleg);
		return;
	}

	/*Si pido la fecha de asignacion y NO es franco  la hora es la ingresada en la asignacion */
	if (dia == DFld (AASIG_FECASIG) && 
		!Franco (emp, LFld (AASIG_NROLEG), dia, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
			*hsentrada = TFld (AASIG_HSENT);
			*hssalida  = TFld (AASIG_HSSAL);
			if (impre) fprintf (stderr, "FIN Es primer dia devuelve %T %T \n", *hsentrada, *hssalida);
	}

	if (dia != DFld (AASIG_FECASIG)) {

		/********************************************************************************************
		Calculo el dia posterior al de la asignacion a que hora tiene que trabajar
		Para esto reconstruyo apartir de lo que seria su franco anterior (Fecha de franco - dias laborables)
		En base a esa fecha puedo saber si el dia posterior al de asignacion tiene que seguir cumpliendo
		el mismo horario o si tiene que cambiar.
		*********************************************************************************************/
		diaslab = GetDiasLaboral(SFld (AASIG_REGIM), FALSE);
		diaslab = diaslab - 1 + IFld (AASIG_NUMFRAN);
		for (fechaA= DFld (AASIG_FFRANCO), dias=0; dias < diaslab; fechaA --, dias ++); 
		if (impre) fprintf (stderr, "El primer dia laborable es %.3D \n", fechaA);

		for (fecha=fechaA; fecha <= (DFld (AASIG_FECASIG)+1); fecha ++) {
			if (impre) fprintf (stderr, "for fecha %.3D asig %.3D \n", fecha, DFld (AASIG_FECASIG)+1);
			if (cantdias < cantdiasP) {
				cantdias ++;
				if (impre) fprintf (stderr, "dentro del if fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (AASIG_FECASIG)+1, cantdias, cantdiasP);
			} 
			else {
				cantdias = 1;
				if (impre) fprintf (stderr, "dentro del else fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (AASIG_FECASIG)+1, cantdias, cantdiasP);
			}
		}

		if (Franco (emp, LFld (AASIG_NROLEG), DFld (AASIG_FECASIG) , SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
			/*Si el dia de asignacion esta de franco la hora anterior es la inversa de la primer asignacion */
			//Inversa
			hantent= TFld (AASIG_HSSAL);
			hantsal= TFld (AASIG_HSENT);
			if (impre) fprintf (stderr, "Franco hantent %T hantsal %T \n", hantent,hantsal);
		}
		else {
			hantent= TFld (AASIG_HSENT);
			hantsal= TFld (AASIG_HSSAL);
			if (impre) fprintf (stderr, "No Franco hantent %T hantsal %T \n", hantent,hantsal);
		}

		if (impre) fprintf (stderr, "El dia posterior a la fecha de asig %.3D cantdias %d horant %T %T \n", DFld (AASIG_FECASIG)+1, cantdias, hantent, hantsal);

		if (strcmp(regim, REG_ESP_3)==0)
			cantdias = 2;

		/***************************************************************
		Busco para la fecha pedida a que hora trabaja
		Rota el horario con respecto a la hora anterior cada dos dias (cantdiasP)
		***************************************************************/

		for (fecha=DFld (AASIG_FECASIG)+1 ; fecha <= dia; fecha ++) {
			short periodo;
		
			if (impre) fprintf (stderr, "principio de for Dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T\n", fecha, cantdias, cantdiasP, *hsentrada, *hssalida,hantent,hantsal);
			/*********************************************************
			Lo que sigue se agrego para agilizar la funcion.
			Porque si el vigilador se asigno hace mucho recorría dia por dia.
			Como la secuencia de rotacion es la misma cada 6 dias puedo avanzar hasta un multiplo.
			De esta forma recorro a lo sumo solo 6 dias.
			**********************************************************/
			/*********************************************************
			Al agregarse el regimen 8x4x12 los dias a recorrer pueden ser 12 por ende puse la 
			variable diasarecorrer. GAG
			**********************************************************/
			periodo = (dia-fecha) / diasarecorrer;
			
			if (periodo > 0) {
				if (impre) fprintf (stderr, "Dia %.3D Fecha %.3D periodo %d ", dia, fecha, periodo);
				fecha = fecha + (periodo * diasarecorrer);
				if (impre) fprintf (stderr, "Nueva fecha %.3D  \n", fecha);
			}
			
			if (Franco (emp, LFld (AASIG_NROLEG), fecha, SFld(AASIG_VIGIL), IFld(AASIG_NUMFRAN))) {
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				cantdias=1;
				if (impre) fprintf (stderr, "Dia %.3D Franco \n", fecha);
				continue;
			} 	

			if (impre) fprintf(stderr, "fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
			if (cantdias < cantdiasP) {
				*hsentrada = hantsal;
				*hssalida  = hantent;
				if (impre) fprintf(stderr,"dentro del if fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d \n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias ++;
			}
			else {
				*hsentrada = hantent;
				*hssalida  = hantsal;
				if (impre) fprintf(stderr,"dentro del else fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias = 1;					
			}
			if (cantdias == cantdiasP) {
				hantent= *hsentrada;
				hantsal= *hssalida;
			}
			if (impre) fprintf(stderr,"al final fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);

			if (Vacaciones(emp, nroleg, fecha)) {
				/*No se modifica cantidad de dias la rotacion es como si estuviera trabajando */
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				if (impre) fprintf (stderr, "Dia %.3D Vacaciones \n", fecha);
			}
		} 
		if (impre) fprintf(stderr,"fin del for fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
	}

	if (*hsentrada == StrToT ("0000") && *hssalida == StrToT ("0000"))
		return;

	if (*hsentrada == StrToT ("2359"))
		*hsentrada = StrToT ("0000");

	/*Controlo que haya alguien a quien cubrir */
	SetCursorFrom(c_Asig, emp, cliente, objetivo, ptoser, puesto, MIN_SHORT, MIN_LONG);
	SetCursorTo  (c_Asig, emp, cliente, objetivo, ptoser, puesto, MAX_SHORT, MAX_LONG);
	while (!hay_otros && FetchCursor(c_Asig) != ERROR) {
		if (!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP) &&
			!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP_2)&&
			!str_eq(SFld(AlFld(ALASIG, operac|ASIG_REGIM)), REG_ESP_3)) {
			continue;
		}
		if (Vacaciones(emp, nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}

		if (Franco(emp, LFld(AlFld(ALASIG, operac|ASIG_NROLEG)), dia, SFld(AlFld(ALASIG, operac|ASIG_VIGIL)),
						IFld(AlFld(ALASIG, operac|ASIG_NUMFRAN)))) {
			if (*hsentrada == TFld(AlFld(ALASIG, operac|ASIG_HSENT))) {
				hay_otros = TRUE;
				break;
			}
		}
	}
//	DeleteCursor(c_Asig);

	SetCursorFrom(c_Asigh, emp, cliente, objetivo, ptoser, puesto, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
	SetCursorTo  (c_Asigh, emp, cliente, objetivo, ptoser, puesto, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
	while (!hay_otros && FetchCursor(c_Asigh) != ERROR) {
		if (!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP) &&
			!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP_2) &&
			!str_eq(SFld(AlFld(ALASIGH, operac|ASIGH_REGIM)), REG_ESP_3))
			continue;

		if (Vacaciones(emp, nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}
		if (Franco(emp, LFld(AlFld(ALASIGH, operac|ASIGH_NROLEG)), dia,
			SFld(AlFld(ALASIGH, operac|ASIGH_VIGIL)), IFld(AlFld(ALASIGH, operac|ASIGH_NUMFRAN)))) {
			if (*hsentrada == TFld(AlFld(ALASIGH, operac|ASIGH_HSENT))) {
				hay_otros  = TRUE;
			}
		}
	}
//	DeleteCursor(c_Asigh);

	if (!hay_otros) {
		char mensaje[200];
		if (prg_form) {
			if (rp == (report) ERROR)
				rp = OpenReport("genparte", RP_EABORT);

			sprintf(mensaje, "El Vigilador %ld es Franquero y no hay asignados para el cliente %ld %d otros Vigiladores a los que tenga que cubrir el día %.1D",
							nroleg, cliente, objetivo, dia);

			RpSetFld (rp, RMENSAJE, mensaje);
			DoReport (rp, ZLINEA);
		}
		else {
			if (fp == (FILE*) NULL)
				fp = fopen("/tmp/gparauto.txt","wt");

			fprintf(fp, "El Vigilador %ld es Franquero y no hay asignados para el cliente %ld %d otros Vigiladores a los que tenga que cubrir el día %.1D",
							nroleg, cliente, objetivo, dia);
		} 
	}
}

private p_asig ArmarLista(int emp, long nroleg, DATE fdesde, DATE fhasta)
{
	p_asig nuevonodo;

	SetCursorFrom(c_asig, emp, nroleg, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, emp, nroleg, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		SetKey(AlInd(ALASIGH, operac|ASIGHbyNROLEG), emp, nroleg, LFld(AlFld(ALASIG,
							  operac|ASIG_CLIENTE)), IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO)));
		(void)GetRecord(AlInd(ALASIGH, operac|ASIGHbyNROLEG), THIS_KEY, IO_NOT_LOCK);
		if (ExisteCliObjEnGrp(GRPRETPLANTA, LFld(AlFld(ALASIG, operac|ASIG_CLIENTE)), IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO)))) {
			continue;
		}
		if (!IsNull(AlFld(ALASIG, operac|ASIG_FECHAS)) &&
			DFld(AlFld(ALASIG, operac|ASIG_FECHAS)) < fdesde) {
			continue;
		}
		if (ExisteCliObjEnGrp(GRPRETPLANTA, LFld(AlFld(ALASIGH, operac|ASIG_CLIENTE)), IFld(AlFld(ALASIGH, operac|ASIG_OBJETIVO)))) {
			continue;
		}
		if (!IsNull(AlFld(ALASIGH, operac|ASIGH_FECHAS)) &&
			DFld(AlFld(ALASIGH, operac|ASIGH_FECHAS)) < fdesde) {
			continue;
		}
		nuevonodo       = CrearNodo();
		nuevonodo->sgte = lista;
		lista           = nuevonodo;
	}
	return lista;
}

private p_asig CrearNodo()
{
	p_asig aux;

	if ((aux = (p_asig) Alloc (sizeof(n_asig))) == NULL) {
		 WiMsg(ERR_MEMORIA);
		Stop(0);
	}
	aux->cliente  = LFld(AlFld(ALASIG, operac|ASIG_CLIENTE));
	aux->objetivo = IFld(AlFld(ALASIG, operac|ASIG_OBJETIVO));
	aux->ptoser   = IFld(AlFld(ALASIG, operac|ASIG_PTOSER));
	aux->puesto   = IFld(AlFld(ALASIG, operac|ASIG_PUESTO));
	aux->nroint   = IFld(AlFld(ALASIG, operac|ASIG_NROINT));
	aux->fdesde   = DFld(AlFld(ALASIG, operac|ASIG_FECASIG));
	aux->fhasta   = (!StrCmp(SFld(AlFld(ALASIG, operac|ASIG_EFECT)), PROVISORIO)) ?
					 DFld(AlFld(ALASIG, operac|ASIG_FECHAS)) : MAX_DATE;
	aux->hdesde   = TFld(AlFld(ALASIG, operac|ASIG_HSENT));
	aux->hhasta   = TFld(AlFld(ALASIG, operac|ASIG_HSSAL));
	aux->numfran  = IFld(AlFld(ALASIG, operac|ASIG_NUMFRAN));
	strcpy(aux->efec,  SFld(AlFld(ALASIG, operac|ASIG_EFECT)));
	strcpy(aux->dia1,  SFld(AlFld(ALASIG, operac|ASIG_DIA1)));
	strcpy(aux->dia2,  SFld(AlFld(ALASIG, operac|ASIG_DIA2)));
	strcpy(aux->dia3,  SFld(AlFld(ALASIG, operac|ASIG_DIA3)));
	strcpy(aux->dia4,  SFld(AlFld(ALASIG, operac|ASIG_DIA4)));
	strcpy(aux->dia5,  SFld(AlFld(ALASIG, operac|ASIG_DIA5)));
	strcpy(aux->dia6,  SFld(AlFld(ALASIG, operac|ASIG_DIA6)));
	strcpy(aux->dia7,  SFld(AlFld(ALASIG, operac|ASIG_DIA7)));
	strcpy(aux->vigil, SFld(AlFld(ALASIG, operac|ASIG_VIGIL)));
	aux->rrol  = IFld(AlFld(ALASIG, operac|ASIG_CODROL));
	aux->rfila = IFld(AlFld(ALASIG, operac|ASIG_FILA));
	aux->rcol  = IFld(AlFld(ALASIG, operac|ASIG_COLUM));
	aux->sgte =	NULL;
	return aux;
}

private void FreeLista()
{
	p_asig recorre, aux;
	for (recorre = lista; recorre;) {
		aux     = recorre;
		recorre = recorre->sgte;
		Free(aux);
	}
	lista =	NULL;
}

bool BuscarHoras(int emp, DATE fecha, TIME horaent, TIME horasal, TIME *hsent, TIME *hssal)
{
	p_asig aux;

	InicializarHorario(horaent, horasal);

	for (aux = lista ; aux != NULL; aux = aux->sgte) {
		if (aux->fdesde > fecha || fecha > aux->fhasta) {
			continue;
		}
		
		// valido si ese dia esta asignado a ese cli-obj ese dia
		// solamente si el puesto no es el efectivo!.
		if (!DebeTrabajar(aux->efec, emp, LFld(operac|ASIG_NROLEG), aux->ptoser, aux->puesto,
						  aux->nroint, fecha, aux->dia1, aux->dia2, aux->dia3, aux->dia4, aux->dia5,
						  aux->dia6, aux->dia7, aux->vigil, aux->cliente, aux->objetivo,
						  IFld(operac|ASIG_NUMFRAN), aux->regim)) {
			continue;
		}
		if (TodoCubierto()) return FALSE;
		// el sgte es el caso de que el periodo asignado a un objetivo que no es
		// prosegur hace que el periodo asignado a prosegur se "parta" en dos. Esto
		// hace que este asig. a prosegur, luego a un objetivo y luego denuevo a prosegur
		// y si miramos la clave de PARTE, veremos que no puede estar asignado dos veces
		// al mismo objetivo el mismo dia, y por otro lado, no se puede guardar en un solo
		// registro dos periodos de hora desde - hora hasta.
		// ejemplo: 3000,1 = de 00 a 08 hs || 3005,27 = de 02 a 04 => no se puede
		// generar dos registros en PARTE uno con 3000,1 de 00 a 04 y otro con 3000,1
		// de 04 a 08!!!!! => por ahora se decidió tirar un WARNING....
	}
	if (TodoCubierto()) return FALSE;
	NuevoHorario(hsent, hssal, horaent, horasal);
	return TRUE;
}

void InicializarHorario(TIME horaent, TIME horasal)
{
	int		horad, horah, i;
	char	aux[10];

	horad =	BusHora(horaent);
	horah =	BusHora(horasal);
	// Inicializo el vector de horarios.
	for (i = 0; i < 24; i++) {
		if ((horad <= horah && i >= horad && i <= horah) ||
			(horad >  horah && ((i >= horad && i <= 23)  || (i >= 0 && i <= horah)))) {
			sprintf(aux, "%02d00", i);
			horario[i].hdesde = (i == horad ) ? horaent : StrToT(aux);
			sprintf(aux, "%02d00", (i == 23) ? 0 : i + 1);
			horario[i].hhasta =	(i == horah) ? horasal : StrToT(aux);
			horario[i].todo	  =	TRUE;
			// el sgte if esta para la hora hasta. por ejemplo si
			// la hora hasta es la 00:00 hs que no genere en el horario[0]
			// nada porque es hasta las 00:00 hs. En cambio si es hasta las
			// 00:30 hs, si debe generar! (no entraria en el sgte if porque
			// la hora desde es distinta a la hasta: 00:00 hs. <> 00:30 hs.)
			if (horario[i].hdesde == horario[i].hhasta) {
				horario[i].hdesde =	NULL_TIME;
				horario[i].hhasta =	NULL_TIME;
				horario[i].todo	  =	FALSE;
			}
		}
		else {
			horario[i].hdesde =	NULL_TIME;
			horario[i].hhasta =	NULL_TIME;
			horario[i].todo	  =	FALSE;
		}
	}
}

bool TodoCubierto()
{
	int	i;

	for (i = 0; i < 24; i++) {
		if (horario[i].todo == TRUE)
			return FALSE;
	}
	return TRUE;
}

void NuevoHorario(TIME *hsent, TIME *hssal, TIME horaent, TIME horasal)
{
	int  horad, horah, i;
	TIME auxd = NULL_TIME, auxh = NULL_TIME;

	horad =	BusHora(horaent);
	horah =	BusHora(horasal);
	// para lo sgte se asume que horad != horah SIEMPRE!!!:
	for (i = horad; i != ((horah == 23) ? 0 : horah + 1); ((i==23) ? i=0 : i++)) {
		if (horario[i].todo == TRUE)
			break;
	}
	for (; i != ((horah == 23) ? 0 : horah + 1); ((i==23) ? i=0 : i++)) {
		if (!horario[i].todo)
			break;
		if (auxd == NULL_TIME) {
			auxd = horario[i].hdesde;
		}
		auxh = horario[i].hhasta;
	}
	(*hsent) = auxd;
	(*hssal) = auxh;
}

bool DebeTrabajar(char *efectivo, int emp, long nroleg, int ptoser, int puesto, int nroint, DATE fecha,
				  char *dia1, char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7,
				  char *vigil, long cliente, int objetivo, int numfran, char *regim)
{
	#ifdef _NOVIA_VER_2_0
	if (!CorrespondeDiaPuesto(emp, cliente, objetivo, ptoser, puesto, fecha)) {
		return FALSE;
	}
	#endif

	if (StrCmp(vigil, PARTTIME) == 0) {
		SetKey(operac|DIASPTIMEbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
		if (GetRecord(operac|DIASPTIMEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
			return TRUE;
		else{
			SetKey(operac|DIASPTIMEHbyEMP, emp, cliente, objetivo, nroleg, ptoser, puesto, nroint, fecha);
			if (GetRecord(operac|DIASPTIMEHbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else {
		if ((!strcmp (regim, REG_ESP_1) || !strcmp (regim, REG_ESP_1_SAP)) &&
			strcmp(efectivo, EFECTIVO) && Franco(emp, nroleg, fecha, vigil, numfran)) 
			return FALSE;

		if (((strcmp(efectivo, EFECTIVO)) ||
			(!strcmp(efectivo, EFECTIVO) && !Franco(emp, nroleg, fecha, vigil, numfran))) &&
			((dia(fecha) != *dia1) && (dia(fecha) != *dia2) && (dia(fecha) != *dia3) &&
			(dia(fecha)  != *dia4) && (dia(fecha) != *dia5) && (dia(fecha) != *dia6) &&
			(dia(fecha)  != *dia7)))
			return FALSE;
	}
	return TRUE;
}
