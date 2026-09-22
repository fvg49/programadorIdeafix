/********************************************************************
*
* MODULE & VERSION : @(#)opechi.c	1.8
* DATE             : 02/09/11
* TIME             : 08:47:49
*
* CREATED          : 27/05/2002
*
* DESCRIPTION:	
*	Modulo para el calculo de Bonos
*
* ROUTINE       |  PURPOSE
**********************************************************************/

#include <ideafix.h>
#include "operac.sch"
#include "comerc.sch"
#include "operac.h"
#include "opechi.h"
#include "opedef.h"
#include "comerc.h"
#include "ambiente.h"

#define	_ORIGEN_PARTE		1
#define	_ORIGEN_RETRO		2
#define _TODO_100FR         1
#define _TODO_100FE         2
#define _TODO_50            3
#define _TODO_NOR           4

struct s_parte_lib *pparte_lib=NULL, *eparte_vivo_lib;
struct s_hs_sem_lib *phs_sem_lib=NULL;

// Funciones Privadas:
// -------------------

private struct s_parte_lib *CargarVigil(short emp, long cliente, short objet, DATE fecha, long legajo, 
										short nrosem, char *condic, short codcond, short motivo,
										short origen,
										short ptoser, short puesto, short nroint,
										TIME hinicio, TIME hfinal,
										short hsnor, short hs50, short hs100fr, short hs100fe,
										short pais, short prov, DATE fechad, DATE fechah, short tipo);

private struct s_hs_sem_lib *CargarHsSemanal(short emp, long nroleg, short nrosem,
										short hsnor, short hs50, short hs100fr, short hs100fe,
										short pais, short prov, DATE fechad, DATE fechah, short tipo);

void CalculoHoras(short emp, long legajo, short nrosem, short hsnor, short hs50, short hs100fr, short hs100fe, 
				short *hornor, short *hor50, short *hor100fr,  short *hor100fe,
				short pais, short prov, DATE fechad, DATE fechah, short tipo);

bool Parte_EsMayor(struct s_parte_lib  *uno, struct s_parte_lib  *dos);
bool Parte_EsIgual(struct s_parte_lib  *uno, struct s_parte_lib  *dos);

void CargarParte(short emp, long nroleg, DATE fechad, DATE fechah);
void CargarRetro(short emp, long nroleg, DATE fechad, DATE fechah);

void VolverInicioParteList ();
bool ProximoParteList(struct s_parte_lib *este_p);
void FinParteList();
bool InicParteList(short emp, long nroleg, DATE fechad, DATE fechah);
short AusentesPorLegajo(short emp, long nroleg, DATE fechad, DATE fechah);

/************************************************************************************************/
bool InicParteList(short emp, long nroleg, DATE fechad, DATE fechah)
{
	long legajod, legajoh;

	if (fechad  == NULL_DATE || fechah == NULL_DATE) return FALSE;
	if (nroleg == NULL_LONG) return FALSE;
	if (emp == NULL_SHORT) return FALSE;

	CargarParte(emp, nroleg, fechad, fechah);

	eparte_vivo_lib = pparte_lib;
	return TRUE;
}

short NroSemana(DATE fechad, DATE fecha)
{
	long sem_desde, sem_hoy;

	if (fecha < fechad) return 0;

	// Week Devuelve	semana * 10000 + año
	sem_desde = (Week(fechad) - Year(fechad)) / 10000;
	sem_hoy = (Week(fecha) - Year(fecha)) / 10000;

	//Si cambio el año la semana desde es mayor a la pedida.
	if (sem_desde > sem_hoy) {
		sem_hoy += sem_desde;
	}

//	fprintf (stderr, "NROSEMANA: Fecha desde %.3D fecha %.3D nrosem %d \n", fechad, fecha, (sem_hoy-sem_desde));
	return (sem_hoy-sem_desde);
}

private struct s_parte_lib *CargarVigil(short emp, long cliente, short objet, DATE fecha, long nroleg, 
										short nrosem, char *condic, short codcond, short motivo,
										short origen,
										short ptoser, short puesto, short nroint,
										TIME hinicio, TIME hfinal,
										short hsnor, short hs50, short hs100fr, short hs100fe,
										short pais, short prov, DATE fechad, DATE fechah, short tipo)
{
	bool existe = FALSE;
	struct s_parte_lib *ant, *aux, *pnew;
	short hornor, hor50, hor100fr, hor100fe;
    bool cambio;

	pnew = (struct s_parte_lib *) Alloc(sizeof(struct s_parte_lib));
	pnew->cliente = cliente;
	pnew->objet = objet;
	pnew->fecha = fecha;
	pnew->nroleg = nroleg;
	pnew->nrosem = nrosem;
	pnew->codcond = codcond;
	pnew->motivo = motivo;
	pnew->origen = origen;
	pnew->ptoser = ptoser;
	pnew->puesto = puesto;
	pnew->nroint = nroint;
	pnew->hinicio = hinicio;
	pnew->hfinal = hfinal;

	CalculoHoras(emp, nroleg, nrosem, hsnor, hs50, hs100fr, hs100fe, &hornor, &hor50, &hor100fr, &hor100fe, pais,
				 prov, fechad, fechah, tipo);   

	cambio = (hsnor != hornor || hs50 != hor50);
	pnew->cambio = cambio;

	pnew->st_hn  = hornor;
	pnew->st_h50 = hor50;
	pnew->st_h100fr = hor100fr;
	pnew->st_h100fe = hor100fe;
	strcpy(pnew->condic, condic);

	pnew->next = NULL;

	if (pparte_lib == NULL ) {
		pparte_lib = pnew;
		return pparte_lib;
	}

	for (ant = pparte_lib, aux = pparte_lib; aux != NULL; ant=aux, aux = aux->next) {
		if (Parte_EsMayor(aux, pnew))
			break;

		if (Parte_EsIgual(aux, pnew)) {
			existe = TRUE;
			break;
		}
	}

	if (existe) {
		aux->st_hn  += hornor;
		aux->st_h50 += hor50;
		aux->st_h100fr  += hs100fr;
		aux->st_h100fe += hs100fe;
		aux->cambio = cambio ? cambio : aux->cambio;

		free (pnew);
		return aux;
	} 	

	if (aux == pparte_lib) {
		pparte_lib = pnew;
		pnew->next = aux;
		return pnew;
	}

    pnew->next = ant->next;
	ant->next  = pnew;
	return pnew;
}

private struct s_hs_sem_lib *CargarHsSemanal(short emp, long legajo, short nrosem,
										short hsnor, short hs50, short hs100fr, short hs100fe,
										short pais, short prov, DATE fechad, DATE fechah, short tipo)
{
	bool existe = FALSE;
	struct s_hs_sem_lib *ant, *aux, *pnew;
	short hornor, hor50, hor100fr, hor100fe;

	CalculoHoras(emp, legajo, nrosem, hsnor, hs50, hs100fr, hs100fe, &hornor, &hor50, &hor100fr, &hor100fe, pais,
				 prov, fechad, fechah, tipo);

	pnew = (struct s_hs_sem_lib *) Alloc(sizeof(struct s_hs_sem_lib));
	pnew->nroleg = legajo;
	pnew->nrosem = nrosem;
	pnew->st_hn  = hornor;
	pnew->st_h50 = hor50;
	pnew->next = NULL;

	if (phs_sem_lib == NULL ) {
		phs_sem_lib = pnew;
		return phs_sem_lib;
	}

	for (ant = phs_sem_lib, aux = phs_sem_lib; !existe && aux != NULL; ant=aux, aux = aux->next) {
		if(aux->nroleg > legajo) break;
		if(aux->nroleg == legajo && aux->nrosem > nrosem) break;
		if(aux->nroleg == legajo && aux->nrosem == nrosem) {
			existe = TRUE;
			break;
		}
	}

	if (existe) {
		aux->st_hn  += hornor;
		aux->st_h50 += hor50;

		free (pnew);
		return aux;
	} 	

	if (aux == phs_sem_lib) {
		phs_sem_lib = pnew;
		pnew->next = aux;
		return pnew;
	}

    pnew->next = ant->next;
	ant->next  = pnew;
	return pnew;
}

private void BorroListaHsSemanal()
{
	struct s_hs_sem_lib *eq, *eqaux;

	for (eq = phs_sem_lib ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	phs_sem_lib = NULL;
}

private void BorroParteList()
{
	struct s_parte_lib *eq, *eqaux;

	for (eq = pparte_lib ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	pparte_lib = NULL;
}

void FinParteList()
{
	BorroListaHsSemanal();
	BorroParteList();
}

void CalculoHoras(short emp, long legajo, short nrosem, short hsnor, short hs50, short hs100fr, short hs100fe, 
				short *hornor, short *hor50, short *hor100fr,  short *hor100fe,
				short pais, short prov, DATE fechad, DATE fechah, short tipo)
{
	struct s_hs_sem_lib  *aux;
	bool encontro = FALSE, timpre=FALSE;
	short horas, hsemana;
	long tothsn=0, toths50=0, normales=0;

	*hornor = 0; *hor50 = 0; *hor100fr = 0; *hor100fe = 0; 

	if (tipo == _TODO_100FR) {
		*hor100fr = hsnor + hs50 + hs100fr + hs100fe;
		return;
	}

	if (tipo == _TODO_100FE) {
		*hor100fe = hsnor + hs50 + hs100fr + hs100fe;
		return;
	}

	if (tipo == _TODO_50) {
		*hor50 = hsnor + hs50 + hs100fr + hs100fe;
		return;
	}

	hsemana = HorasNormalesPorSemana(emp, legajo, pais, prov, nrosem, fechad, fechah);
	if (timpre) fprintf(stderr, "Horas por semana %d %.3D %.3D %d \n", nrosem, fechad, fechah, hsemana);
	
	for (aux=phs_sem_lib; !encontro && aux != NULL; aux = aux->next) {
		if (legajo == aux->nroleg && nrosem == aux->nrosem) {
			encontro = TRUE;
			tothsn = aux->st_hn;
			toths50 = aux->st_h50;
		}
	}

	horas = (hsnor + hs50);
	if (horas >= 0) {
		//Incrementa horas son normales hasta cubrir el limite
		*hornor = (tothsn +  horas) <= hsemana ? horas : hsemana - tothsn;
		*hor50  = horas - (*hornor);
	}
	else {
		//Va a restar horas, saco lo que puedo de extras al 50 y el resto a las normales
		*hor50  = (toths50 > (-1 * horas)) ? horas : (-1 * toths50);
		*hornor = horas - (*hor50);
	}
	if (timpre) fprintf (stderr, "Calculo horas leg %ld horas %d %d tot %d tothsn %ld queda %d %d std. %d \n\n", legajo, hsnor, hs50, horas, tothsn, *hornor, *hor50, hsemana);
}

bool Parte_EsMayor(struct s_parte_lib  *uno, struct s_parte_lib  *dos)
{
	//Devuelve TRUE si el valor de la estructura uno es mas grande que la dos segun el indice

	if (uno->nroleg > dos->nroleg) return TRUE;
	if (uno->nroleg == dos->nroleg && uno->fecha > dos->fecha) return TRUE;
	if (uno->nroleg == dos->nroleg && uno->fecha == dos->fecha &&
		uno->cliente > dos->cliente) return TRUE;
	if (uno->nroleg == dos->nroleg && uno->fecha == dos->fecha &&
		uno->cliente == dos->cliente && uno->objet > dos->objet) return TRUE;
	if (uno->nroleg == dos->nroleg && uno->fecha == dos->fecha &&
		uno->cliente == dos->cliente && uno->objet == dos->objet && uno->origen == dos->origen) return TRUE;
	return FALSE;
}

bool Parte_EsIgual(struct s_parte_lib  *uno, struct s_parte_lib  *dos)
{
	if (uno->nroleg  == dos->nroleg  && uno->cliente == dos->cliente &&
	    uno->objet   == dos->objet   && 
	    uno->codcond == dos->codcond && uno->motivo  == dos->motivo  &&
	    uno->ptoser  == dos->ptoser  && uno->puesto  == dos->puesto  && uno->nroint == dos->nroint &&
	    uno->hinicio == dos->hinicio && uno->hfinal  == dos->hfinal  &&	    
	    uno->fecha   == dos->fecha   && uno->origen  == dos->origen) return TRUE;

	return FALSE;
}

bool ProximoParteList(struct s_parte_lib *este_p)
{
	if (eparte_vivo_lib == NULL)
		return FALSE;

	este_p->nroleg  = eparte_vivo_lib->nroleg;
	este_p->cliente = eparte_vivo_lib->cliente;
	este_p->objet   = eparte_vivo_lib->objet;
	este_p->fecha = eparte_vivo_lib->fecha;
	este_p->nrosem = eparte_vivo_lib->nrosem;
	este_p->codcond = eparte_vivo_lib->codcond;
	este_p->motivo = eparte_vivo_lib->motivo;
	este_p->origen = eparte_vivo_lib->origen;
	este_p->ptoser = eparte_vivo_lib->ptoser;
	este_p->puesto = eparte_vivo_lib->puesto;
	este_p->nroint = eparte_vivo_lib->nroint;
	este_p->cambio = eparte_vivo_lib->cambio;
	este_p->hinicio = eparte_vivo_lib->hinicio;
	este_p->hfinal = eparte_vivo_lib->hfinal;
	este_p->st_hn  = eparte_vivo_lib->st_hn;
	este_p->st_h50 = eparte_vivo_lib->st_h50;
	este_p->st_h100fr  = eparte_vivo_lib->st_h100fr;
	este_p->st_h100fe = eparte_vivo_lib->st_h100fe;
	strcpy(este_p->condic, eparte_vivo_lib->condic);

	eparte_vivo_lib = eparte_vivo_lib->next;
	return TRUE;
}

void VolverInicioParteList ()
{
	eparte_vivo_lib = pparte_lib;
}

void CargarParte(short emp, long nroleg, DATE fechad, DATE fechah)
{
	schema  old, operac;
	dbcursor c_parte;
	dbtable  APARTE;
	long cliant = NULL_LONG;
	short objant = NULL_SHORT;
	short pais, prov, tipo;
	bool franco=FALSE;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	APARTE  = CreateAlias(operac|PARTE);

	c_parte = CreateCursor(APARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom (c_parte, emp, nroleg, fechad, MIN_LONG, MIN_SHORT);
	SetCursorTo   (c_parte, emp, nroleg, fechah, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_parte) != ERROR) {
		if (LFld(APARTE_CLIENTE) != cliant || IFld(APARTE_OBJETIVO) != objant) {
			pais = GetCliePais(LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO));
			prov = GetClieProv(LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO));
			cliant = LFld(APARTE_CLIENTE);
			objant = IFld(APARTE_OBJETIVO);
		}
		
		/* Antes el franco se calculaba con la funcion franco pero si le cambian el franco a mano
		   me lo recalculaba, y eso esta mal.
		*/
		if (str_eq (_FRANCO, SFld(APARTE_CONDIC))) {
			franco = TRUE;
		}
		else {
			franco = FALSE;
		}
		if (franco &&
			!FeriadoNovia(DFld(APARTE_DIA), pais, prov)) {
			if (FecIng(emp, nroleg) < StrToD(FECING_LIMITE))
				tipo = _TODO_100FR;
			else
				tipo = _TODO_50;
		}
		else {
			if (FeriadoNovia(DFld(APARTE_DIA), pais, prov)) {
				if (FecIng(emp, nroleg) < StrToD(FECING_LIMITE))
					tipo = _TODO_100FE;
				else
					tipo = _TODO_50;
			}
			else
				tipo = _TODO_NOR;
		}

		CargarVigil(IFld(APARTE_EMP),
					LFld(APARTE_CLIENTE),
					IFld(APARTE_OBJETIVO),
					DFld(APARTE_DIA),
					LFld(APARTE_NROLEG),
					NroSemana(fechad, DFld(APARTE_DIA)),
					SFld(APARTE_CONDIC),
					NULL_SHORT, NULL_SHORT,
					_ORIGEN_PARTE,
					IFld(APARTE_PTOSER),
					IFld(APARTE_PUESTO),
					IFld(APARTE_NROINT),
					TFld(APARTE_HORAENT),
					TFld(APARTE_HORASAL),
					IFld(APARTE_HSNOR),
					IFld(APARTE_HS50),
					IFld(APARTE_HS100F),
					IFld(APARTE_HS100FE),
					pais, prov, fechad, fechah, tipo);

		CargarHsSemanal(emp, LFld(APARTE_NROLEG),
					NroSemana(fechad, DFld(APARTE_DIA)),
					IFld(APARTE_HSNOR),
					IFld(APARTE_HS50),
					IFld(APARTE_HS100F),
					IFld(APARTE_HS100FE),
					pais, prov, fechad, fechah, tipo);

		SetKey(operac|RETRObyEMP, IFld(APARTE_EMP), LFld(APARTE_CLIENTE), IFld(APARTE_OBJETIVO),
				DFld(APARTE_DIA), LFld(APARTE_NROLEG), IFld(APARTE_PTOSER), IFld(APARTE_PUESTO), 
				IFld(APARTE_NROINT));
		if (GetRecord(operac|RETRObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			CargarVigil(IFld(operac|RETRO_EMP),
					LFld(operac|RETRO_CLIENTE),
					IFld(operac|RETRO_OBJETIVO),
					DFld(operac|RETRO_DIA),
					LFld(operac|RETRO_NROLEG),
					NroSemana(fechad, DFld(operac|RETRO_DIA)),
					SFld(operac|RETRO_CONDIC),
					NULL_SHORT,
					NULL_SHORT,
					_ORIGEN_RETRO,
					IFld(operac|RETRO_PTOSER),
					IFld(operac|RETRO_PUESTO),
					IFld(operac|RETRO_NROINT),
					TFld(operac|RETRO_HORAENT),
					TFld(operac|RETRO_HORASAL),
					IFld(operac|RETRO_DHSNOR),
					IFld(operac|RETRO_DHS50),
					IFld(operac|RETRO_DHS100F),
					IFld(operac|RETRO_DHS100FE),
					pais, prov, fechad, fechah, tipo);

			CargarHsSemanal(emp, LFld(operac|RETRO_NROLEG),
					NroSemana(fechad, DFld(operac|RETRO_DIA)),
					IFld(operac|RETRO_DHSNOR),
					IFld(operac|RETRO_DHS50),
					IFld(operac|RETRO_DHS100F),
					IFld(operac|RETRO_DHS100FE),
					pais, prov, fechad, fechah, tipo);
		}
	}
	DeleteCursor(c_parte);
	DeleteAlias(APARTE);
}

void SemanaDesdeHasta(DATE fecbase, DATE *fdesde, DATE *fhasta)
{
	for (*fdesde=fecbase; *DiaLetra(*fdesde) != 'L'; (*fdesde) --);
	for (*fhasta=fecbase; *DiaLetra(*fhasta) != 'D'; (*fhasta) ++);
}

void ReclasificarHoras(short emp, long nroleg, DATE fecha)
{
	struct s_parte_lib paux;
	DATE fdesde, fhasta;
	short hornor, hor50, hor100, hor100f, horexc, v_extra_parte;
	schema  old, operac;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	SemanaDesdeHasta(fecha, &fdesde, &fhasta);

	if (!InicParteList(emp, nroleg, fdesde, fhasta))
		return;

	while(ProximoParteList(&paux)) {
		// fprintf (stderr, "leg %ld cli %ld obj %d fecha %.3D %s %d %d origen %d cambio %d %T %T %d %d %d horas %ld %ld %ld %ld \n",
		// paux.nroleg, paux.cliente, paux.objet, paux.fecha, paux.condic,
		// paux.codcond, paux.motivo, paux.origen, paux.cambio, paux.hinicio, paux.hfinal,
		// paux.ptoser, paux.puesto, paux.nroint,
		// paux.st_hn, paux.st_h50, paux.st_h100fr, paux.st_h100fe);

		if (!paux.cambio) continue;

		if (paux.origen == _ORIGEN_PARTE) {
			/*Busco el renglon del parte */
			SetKey(operac|PARTEbyEMP, emp, paux.cliente, paux.objet, paux.fecha, paux.nroleg, 
					paux.ptoser, paux.puesto, paux.nroint);
			if(GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				SetIFld(operac|PARTE_HSNOR,   paux.st_hn);
				SetIFld(operac|PARTE_HS50,    paux.st_h50);
				SetIFld(operac|PARTE_HS100F,  paux.st_h100fr);
				SetIFld(operac|PARTE_HS100FE, paux.st_h100fe);				

				PutRecord(operac|PARTE);
				v_extra_parte = IFld(operac|PARTE_HS50);

				/*Busco la excepcion correspondiente */
				SetKey(operac|EXCEPCIONbyEMP, IFld(operac|PARTE_EMP), LFld(operac|PARTE_CLIENTE), 
						IFld(operac|PARTE_OBJETIVO), DFld(operac|PARTE_DIA), LFld(operac|PARTE_NROLEG),
						IFld(operac|PARTE_PTOSER), IFld(operac|PARTE_PUESTO), IFld(operac|PARTE_NROINT), 
						MIN_SHORT, MIN_SHORT);
				while (GetRecord(operac|EXCEPCIONbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {
					horexc = IFld(operac|EXCEPCION_HORAS) + IFld(operac|EXCEPCION_HS50);
					SetIFld(operac|EXCEPCION_HS50, horexc >= v_extra_parte ? v_extra_parte : horexc);
					SetIFld(operac|EXCEPCION_HORAS, horexc - IFld(operac|EXCEPCION_HS50));
					PutRecord(operac|EXCEPCION);
					v_extra_parte -= IFld(operac|EXCEPCION_HS50);
					//fprintf (stderr, "excepcion %d %d extra parte \n", IFld(operac|EXCEPCION_HORAS), IFld(operac|EXCEPCION_HS50), v_extra_parte);
				}
			}
		}

		if (paux.origen == _ORIGEN_RETRO) {
			/*Busco el renglon del retroactivo */
			SetKey(operac|RETRObyEMP, emp, paux.cliente, paux.objet, paux.fecha, paux.nroleg, 
					paux.ptoser, paux.puesto, paux.nroint);
			if (GetRecord(operac|RETRObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				SetKey(operac|PARTEbyEMP, emp, paux.cliente, paux.objet, paux.fecha, paux.nroleg, 
						paux.ptoser, paux.puesto, paux.nroint);
				if(GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
					continue;
				}

				SetIFld(operac|RETRO_DHSNOR,  paux.st_hn);
				SetIFld(operac|RETRO_DHS50,   paux.st_h50);
				SetIFld(operac|RETRO_HSNOR, IFld(operac|PARTE_HSNOR)  + IFld(operac|RETRO_DHSNOR));
				SetIFld(operac|RETRO_HS50,  IFld(operac|PARTE_HS50)   + IFld(operac|RETRO_DHS50));
				PutRecord(operac|RETRO);
				v_extra_parte = IFld(operac|RETRO_HS50);

				/********* LEO Ver las excepciones como se contemplan
				SetKey(operac|RETROEXCbyEMP, IFld(operac|RETRO_EMP), LFld(operac|RETRO_CLIENTE), 
						IFld(operac|RETRO_OBJETIVO), DFld(operac|RETRO_DIA), LFld(operac|RETRO_NROLEG),
						IFld(operac|RETRO_PTOSER), IFld(operac|RETRO_PUESTO), IFld(operac|RETRO_NROINT), 
						MIN_SHORT, MIN_SHORT);
				while (GetRecord(operac|RETROEXCbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {
					horexc = IFld(operac|RETROEXC_HORAS) + IFld(operac|RETROEXC_HS50);
					SetIFld(operac|RETROEXC_HS50, horexc >= v_extra_parte ? v_extra_parte : horexc);
					SetIFld(operac|RETROEXC_HORAS, horexc - IFld(operac|RETROEXC_HS50));
					PutRecord(operac|RETROEXC);
					v_extra_parte -= IFld(operac|RETROEXC_HS50);
					//fprintf (stderr, "excepcion %d %d extra parte \n", IFld(operac|RETROEXC_HORAS), IFld(operac|RETROEXC_HS50), v_extra_parte);
				}
				************************/
			}
		}
	}
	FinParteList();
}

short HorasNormalesPorSemana(short emp, long nroleg, short pais, short prov, short nrosem, DATE fechad, DATE fechah)
{
	DATE fecini;
	short tothsem = _HORAS_NORMALES_POR_SEMANA;
	short cfaltas;
	static short paisa=NULL_SHORT, prova=NULL_SHORT, hsemant=NULL_SHORT;
	static DATE afechad=NULL_DATE, afechah=NULL_DATE;

	if (pais != paisa || prov != prova || afechad != fechad || afechah != fechah) {
		paisa = pais;
		prova = prov;
		afechad = fechad;
		afechah = fechah;
	}
	else {
		return hsemant;
	}

	//Para que nunca entre en loop el for
	if (fechad > fechah) return _HORAS_NORMALES_POR_SEMANA;

	for (fecini=fechad; fecini <= fechah; fecini ++) {
		if (FeriadoNovia(fecini, pais, prov)) {
			tothsem -= _HORAS_NORMALES_POR_FERIADO;
		}
	}

	cfaltas = AusentesPorLegajo(emp, nroleg, fechad, fechah);
	if (cfaltas > 0) {
		tothsem -= (_HORAS_NORMALES_POR_FERIADO * cfaltas);
	}

	hsemant = tothsem;
	return tothsem;
}

short AusentesPorLegajo(short emp, long nroleg, DATE fechad, DATE fechah)
{
	//Devuelve la cantidad de ausentes para un legajo.
	dbcursor c_parte;
	dbtable  APARTE;
	short acant=0;
	schema old, operac;

	old = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);
	SwitchToSchema(old);

	APARTE  = CreateAlias(operac|PARTE);

	c_parte = CreateCursor(APARTEbyEMPLE, IO_NOT_LOCK);
	SetCursorFrom (c_parte, emp, nroleg, fechad, MIN_LONG, MIN_SHORT);
	SetCursorTo   (c_parte, emp, nroleg, fechah, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_parte) != ERROR) {
		if (str_eq(SFld(APARTE_CONDIC), _AUSENTE)) {
			acant ++;
		}
	}
	DeleteCursor(c_parte);
	DeleteAlias(APARTE);
	return acant;
}

