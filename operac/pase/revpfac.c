/********************************************************************
* MODULE & VERSION : @(#)revpfac.c	1.5
* DATE             : 02/04/16
* TIME             : 16:17:17
*
* DESCRIPTION:
*    Reversion del pase de Operaciones a Facturación.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "observa.h"
#include "operac.h"
#include "comerc.h"
#include "comdef.h"
#include "comgral.h"
#include "billpro.h"
#include "comerc.sch"
#include "comgral.sch"
#include "operac.sch"
#include "billpro.sch"
#include "bill.sch"
#include "pasefac.fmh"
#include "revpfac.fmh"

#define _ERROR_SUBCONC	1
#define _ERROR_HSADIC	2
#define _ERROR_NOCONC	3
#define _ERROR_LIQ		4
#define _ERROR_ESTADO	5
#define _ERROR_NOPRECIO	6
#define _ERROR_RESUMEN  7

#define _MENSAJE_ESTADO	10
#define _MENSAJE_LIQ	11
#define _MENSAJE_BILL	12
#define _MENSAJE_RETRO	13
#define _MENSAJE_NC		14

/* Funciones privadas */
private fm_status before(form fm, fmfield fno, int row);
private fm_status after(form fm, fmfield fno, int row);
void BorrarNovedad();
void ActualizoParte();
void ActualizoRetro();
bool ClienteObjValido (long cliente, short obj);
bool ObjetivoValido (long cliente, short objetivo);
void CargarClientesValidos();
long CliMin();
long CliMax();
short ObjMin();
short ObjMax();
private struct s_errores *CargarError(short codigo, long cliente, short objet, short conc, short ptoser, char *extra, long liq, long eval);
private struct s_estcli *CargarEstCliente(long cliente, short objet);
private void BorroListaClientes();
private void BorroListaErrores ();
void ListarClienteObjValido ();

/* Declaraciones globales */
form   fm0;
schema comerc, bill, operac, comgral, billpro;
FILE *fp_error;
char bufaux[50];
bool modor=FALSE,  hmensaje=FALSE;

struct s_errores {
	short tipoerr;
	long cliente;
	short objet;
	short econc;
	short eptoser;
	long  eliq, eval;
	char  extra[100];
	struct s_errores *next;
};

struct s_estcli {
	long  cliok;
	short objok;
	struct s_estcli *next;
};

struct s_errores *p_error=NULL, *aux_error;
struct s_estcli  *p_estcli=NULL, *aux_estcli;

wcmd(revpfac, 1.5 04/16/02)
{
	fm0    = OpenForm("revpfac", FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);
	comgral= OpenSchema("comgral", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	billpro= OpenSchema("billpro",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);

	if(argc >1) {
		modor = str_eq (argv[1], "R");
	}

	if (modor) {
		FmSetFld(fm0, COMENT2, "MODOR");
	}

 	FmSetIFld(fm0, I_GRUPO, GrupoFact(GrupoUsr(GetUid())));

	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	SwitchToSchema(operac);

	CargarClientesValidos();

//	ListarClienteObjValido ();

	sprintf (bufaux, "/tmp/revpfac.%d.log", GetUid());

	if ((fp_error = fopen (bufaux, "wt")) == (FILE *) NULL) Error ("No se pudo abrir el archiv de Log");
	fprintf(fp_error, "Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fprintf(fp_error, "Liquidacion %ld Cliente %ld %d hasta %ld %d \n",
					FmLFld(fm0, NROLIQ), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), 
					FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));

	fprintf(fp_error, "Empieza a borrar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	//BeginTransaction();

	if (!modor) {
		BorrarNovedad();
	}

	ActualizoParte();
	ActualizoRetro();
	//EndTransaction();
	BorroListaErrores ();
	BorroListaClientes();

	fprintf(fp_error, "FIN PROCESO Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fclose(fp_error);
}

private fm_status before(form fm, fmfield fno, int row)
{
	long liq;

	switch (fno) {
	case NROLIQ:
		GrupoFact(GrupoUsr(GetUid()));

		liq = RecuAbierta (NULL_LONG, GrupoFact(GrupoUsr(GetUid())), NULL_DATE, FmIFld(fm0, EMP));
		if (liq == NULL_LONG)
			Error("No existe liquidación abierta.");

		if (FmIsNull(fm0, NROLIQ)) {
			FmSetLFld(fm, fno, liq);
			FmShowFlds(fm, fno, fno);
			SetKey(bill|LIQUIDbyNROLIQ, liq);
			(void) GetRecord(bill|LIQUIDbyNROLIQ, THIS_KEY, IO_NOT_LOCK);
		}
		break;
	case FECHAD:
		if (FmIsNull(fm0, FECHAD)) {
			/* El periodo lo tomo como un mes anterior a la fecha hasta
			   Para no leer todo el parte */
			short dia, mes, anio;
			dia = Day(DFld(bill|LIQUID_FCHADH));
			mes = Month(FirstMonthDay(DFld(bill|LIQUID_FCHADH)) - 5);
			anio = Year(FirstMonthDay(DFld(bill|LIQUID_FCHADH)) - 5);
			FmSetDFld(fm0, FECHAD, DMYToD(dia,mes,anio));
			FmSetDFld(fm0, FECHAH, DFld(bill|LIQUID_FCHADH));
		}
		break;
	}
	return FM_OK;
}

private fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NROLIQ:  
		if (FmChgFld(fm0)) {
			SetKey(bill|LIQUIDbyNROLIQ, FmLFld(fm0, NROLIQ));
			(void) GetRecord(bill|LIQUIDbyNROLIQ, THIS_KEY, IO_NOT_LOCK);

			if(DFld(bill|LIQUID_FCHCIE)!=NULL_DATE) {
				Warning("Esta liquidación está cerrada ");
				if (!modor) {
					return FM_REDO;
				}
			}
		}
		break;
	case FILTRA:
		if (FmIFld(fm0, FILTRA)) {
			if (FmChgFld(fm0))
				FmSetIFld(fm0, CONSIDERA, FALSE);
		}
		else {
			int pos;
			FmClearFlds (fm0, CONSIDERA, DTIPCLI);
			for (pos=0; !FmIsNull (fm0, CLICON, pos); pos ++) {
				FmClearFlds(fm0, CLICON, DTOBJCON, pos);
			}

			if (FmChgFld(fm0)) {
				FmSetLFld(fm0, CLID, StrToL(GetParNov(FmIFld(fm0, EMP), _ORDEN_PARNOV_RANG_CLI, _ORDEN_PARNOV_RANG_CLI_DES, Today())));
				FmSetLFld(fm0, CLIH, StrToL(GetParNov(FmIFld(fm0, EMP), _ORDEN_PARNOV_RANG_CLI, _ORDEN_PARNOV_RANG_CLI_DES, Today())));
				FmSetIFld(fm0, OBJD, 1);
				FmSetIFld(fm0, OBJH, 99);
			}
		}
		break;
	case TIPCLI:
		if (FmChgFld(fm0)) {
			int pos;
			/* Agrego los nuevos clientes al final de lo que ya tenia */				
			for (pos=0; !FmIsNull (fm0, CLICON, pos); pos ++) {
				 FmClearFlds(fm0, CLICON, DTOBJCON, pos);
			}
			SetKey(billpro|RCLIESPbyTIPCLI, FmIFld(fm0, TIPCLI), MIN_LONG, MIN_SHORT);
			for (pos=0; GetRecord (billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK,1) != ERROR;pos++) {
				FmSetLFld(fm0, CLICON, LFld(billpro|RCLIESP_CLIENTE), pos);
				
				if (IFld(billpro|RCLIESP_OBJET) == 0)
					FmSetIFld(fm0, OBJCON, NULL_SHORT, pos);
				else
					FmSetIFld(fm0, OBJCON, IFld(billpro|RCLIESP_OBJET), pos);
				FmSetFld(fm0, DCLICON, GetDescCli(LFld(billpro|RCLIESP_CLIENTE)), pos);
				if (IFld(billpro|RCLIESP_OBJET) != NULL_SHORT)
					FmSetFld(fm0, DOBJCON, GetObjDescrip(LFld(billpro|RCLIESP_CLIENTE), 
														IFld(billpro|RCLIESP_OBJET)), pos);
			}
		}
		break;
	case CONSIDERA:
		if (FmIFld(fm0, FILTRA) && FmIFld(fm0, CONSIDERA)) {
			FmClearFlds(fm0, CLIDESDE, DOBJH);
		}
		else {
			if (FmChgFld(fm0)) {
				FmSetLFld(fm0, CLID, 3001);
				FmSetLFld(fm0, CLIH, 999999999);
				FmSetIFld(fm0, OBJD, 0);
				FmSetIFld(fm0, OBJH, 99);
			}
		}
		break;
	case OBJCON:
		if (FmIFld(fm0, fno, row) == 0)
			return FM_SKIP;
		
	}
	return FM_OK;
}

void BorrarNovedad()
{
	dbcursor c_obj;

	c_obj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_obj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (c_obj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(c_obj) != ERROR) {

		if (IsNull(comerc|OBJETIVO_INTERN))
			continue;

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) 
			continue;

		SetKey(bill|VARNOVbyNROLIQ, FmLFld(fm0, NROLIQ), LFld(comerc|OBJETIVO_INTERN), MIN_SHORT);
		while(GetRecord(bill|VARNOVbyNROLIQ, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			DelRecord(bill|VARNOV);
			//index nroliq (nroliq, intern, nrovar, fecvig);
			SetKey(comgral|VALIFEbyNROLIQ, LFld(bill|VARNOV_NROLIQ), LFld(bill|VARNOV_INTERN), IFld(bill|VARNOV_NROVAR), NULL_DATE);
			while (GetRecord(comgral|VALIFEbyNROLIQ, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
				DelRecord(comgral|VALIFE);
			}
		}
	}
	DeleteCursor(c_obj);
}


void ActualizoParte()
{
	dbcursor  cparte, c_obj;
	DATE pdia=NULL_DATE;

	cparte = CreateCursor(PARTEbyEMP, IO_LOCK);

	c_obj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_obj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (c_obj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(c_obj) != ERROR) {

		/*Cargo las horas del parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cparte) != ERROR) {

			if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (pdia != DFld(PARTE_DIA)) {
				sprintf (bufaux, "Procesando Parte Cliente %ld Objetivo %d dia %.3D ", LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), DFld(PARTE_DIA) );
				FmSetFld (fm0, COMENT, bufaux);
				FmShowFlds (fm0, COMENT, COMENT);
				WiRefresh();
				pdia = DFld(PARTE_DIA);
			}

			if (!ClienteObjValido(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO))) 
				continue;

			if (LFld(PARTE_LIQFAC) == FmLFld(fm0, NROLIQ)) {
				SetLFld(PARTE_LIQFAC, NULL_LONG);
				PutRecord(PARTE);
			}
		}
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);
	DeleteCursor(c_obj);
 	FreeTable(PARTE);
}

void ActualizoRetro()
{
	dbcursor  cretro;

	cretro = CreateCursor(RETRObyRDIA, IO_LOCK);

	/*Cargo las horas de retroactivos */
	SetCursorFrom(cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), CliMin(), ObjMin());
	SetCursorTo  (cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), CliMax(), ObjMax());
	while (FetchCursor(cretro) != ERROR) {
		if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
			continue;


		if (!ClienteObjValido(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)))
			continue;

		if (LFld(RETRO_LIQFAC) == FmLFld(fm0, NROLIQ)) {
			SetLFld(RETRO_LIQFAC, NULL_LONG);
			PutRecord(RETRO);
		}

		sprintf (bufaux, "Procesando Retro Cliente %ld Objetivo %d dia %.3D ", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(PARTE_DIA) );

		FmSetFld (fm0, COMENT, bufaux);
		WiRefresh();
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
 	FreeTable(RETRO);
}

void CargarClientesValidos()
{
	dbcursor curobj;
	bool estado_ok=TRUE, liq_ok=TRUE, tiene_parte=FALSE, bill_ok=TRUE;
	long liqmal=NULL_LONG;

	curobj=CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK); // Solo recorre los Objetivos relacionados a la Empresa cargada en Pantalla
	SetCursorFrom (curobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo	  (curobj, FmIFld(fm0, EMP), CliMax(), ObjMax());

//	fprintf(stderr, "CliMin() %ld, ObjMin()%d\n",CliMin(), ObjMin());
//	fprintf(stderr, "CliMax() %ld, ObjMax()%d\n",CliMax(), ObjMax());
	while (FetchCursor(curobj) != ERROR) {
		if (!ObjetivoValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

		if (GetServicioObj(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)) == BRIGADA)
			continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

        if (LFld(comerc|OBJETIVO_RESUMEN) == NULL_LONG) {
        	CargarError(_ERROR_RESUMEN, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
        	continue;
        }

		estado_ok = TRUE;
		liq_ok = TRUE;
		bill_ok = TRUE;
		tiene_parte=FALSE;
		//Busco en el parte si en los dias extremos tiene el cierre echo
		//no recorro todo porque tardaria mucho
		SetKey(operac|PARTEbyPUESTO, FmIFld(fm0, EMP),LFld(comerc|OBJETIVO_CLIENTE), 
				IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG);
		if (GetRecord (operac|PARTEbyPUESTO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {

			if (liq_ok && LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)){
				liq_ok = FALSE;
				liqmal = LFld(PARTE_LIQFAC);
			}
			tiene_parte=TRUE;
		}

		SetKey(operac|PARTEbyPUESTO, FmIFld(fm0, EMP),LFld(comerc|OBJETIVO_CLIENTE), 
				IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);
		if (GetRecord (operac|PARTEbyPUESTO, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (DFld(PARTE_DIA) >=  FmDFld(fm0, FECHAD)) {
				if (liq_ok &&  LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)){
					liq_ok = FALSE;
					liqmal = LFld(PARTE_LIQFAC);
				}
			}

		}

		//Valido que tengo el codigo de billing
		if (tiene_parte && IsNull(comerc|OBJETIVO_INTERN)) {
			bill_ok = FALSE;
		}

		if (estado_ok && liq_ok && bill_ok) {
			CargarEstCliente(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		}
		else {
			if (!bill_ok){
				CargarError (_MENSAJE_BILL, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
											NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
			}
			if (!estado_ok){
				CargarError (_MENSAJE_ESTADO, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
											NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
			}
			if (!liq_ok) {
				CargarError(_MENSAJE_LIQ, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							NULL_SHORT, NULL_SHORT, NULL_STR, liqmal, NULL_LONG);
			}
		}
	}
	DeleteCursor(curobj);
}
long CliMax()
{
	int pos;
	long climax=MIN_LONG;
	if (FmIFld(fm0, FILTRA) && FmIFld(fm0, CONSIDERA)) {
		for (pos=0; !FmIsNull (fm0, CLICON, pos); pos ++) {
			if (FmLFld(fm0, CLICON, pos) > climax) {
				climax = FmLFld(fm0, CLICON, pos);
			}
		}
		return climax;
	}
	else {
		return FmLFld(fm0, CLIH);
	}
}

long CliMin()
{
	int pos;
	long climin=MAX_LONG;

	if (FmIFld(fm0, FILTRA) && FmIFld(fm0, CONSIDERA)) {
		for (pos=0; !FmIsNull (fm0, CLICON, pos); pos ++) {
			if (FmLFld(fm0, CLICON, pos) < climin) {
				climin = FmLFld(fm0, CLICON, pos);
			}
		}
		return climin;
	}
	else {
		return FmLFld(fm0, CLID);
	}
}

short ObjMin()
{
   return FmIsNull(fm0, OBJD) ? MIN_SHORT : FmIFld(fm0, OBJD);
}

short ObjMax()
{
	return FmIsNull(fm0, OBJH) ? MAX_SHORT : FmIFld(fm0, OBJH);
}	

private struct s_errores *CargarError(short codigo, long cliente, short objet, short conc, 
										short ptoser, char *extra, long liq, long eval)
{
	bool existe = FALSE;
	struct s_errores *ant, *aux, *pnew;

	switch (codigo) {
		case _ERROR_SUBCONC:
		case _ERROR_HSADIC:
		case _ERROR_NOCONC :
		case _ERROR_NOPRECIO :
		case _ERROR_LIQ:
		case _ERROR_ESTADO:
		case _ERROR_RESUMEN :
//			herror=TRUE;
			hmensaje=TRUE;
		break;

		case _MENSAJE_ESTADO : 
		case _MENSAJE_LIQ :
		case _MENSAJE_BILL :
		case _MENSAJE_RETRO :
		case _MENSAJE_NC :
			hmensaje=TRUE;		
		break;

		default : 
			Error ("Vino un mensaje que no esta parametrizado");
		break;
	}
	pnew = (struct s_errores *) Alloc(sizeof(struct s_errores));
	pnew->tipoerr = codigo;
	pnew->cliente = cliente;
	pnew->objet = objet;
	pnew->econc = conc;
	pnew->eptoser = ptoser;
	pnew->eliq = liq;
	pnew->eval = eval;
	strcpy (pnew->extra, extra);
	pnew->next    = NULL;

	if (p_error == NULL ) {
		p_error = pnew;
		return p_error;
	}

	for (ant = p_error, aux = p_error; aux != NULL; ant=aux, aux = aux->next) {

		if ( aux->cliente > cliente)	break;
		if ( aux->cliente == cliente && aux->objet > objet) break;
		if ( aux->cliente == cliente && aux->objet == objet && aux->econc > conc ) break;
		if ( aux->cliente == cliente && aux->objet == objet && aux->econc == conc && aux->tipoerr > codigo) break;

		if ( aux->cliente == cliente && aux->objet == objet && aux->econc == conc && 
			aux->tipoerr == codigo && aux->eliq == liq ) {
			if (codigo == _ERROR_SUBCONC || codigo == _ERROR_HSADIC   || codigo == _ERROR_LIQ   ||
				codigo == _ERROR_ESTADO  || codigo == _MENSAJE_ESTADO || codigo == _MENSAJE_LIQ || 
				codigo == _MENSAJE_BILL  || codigo == _MENSAJE_RETRO  || codigo == _MENSAJE_NC  ||
				codigo == _ERROR_NOPRECIO || codigo == _ERROR_RESUMEN ) {
				aux->eval += eval;
				existe = TRUE;
				break;
			}
		}
	}

	if (existe) {
		free (pnew);
		return aux;
	} 	

	if (aux == p_error) {
		p_error = pnew;
		pnew->next = aux;
		return pnew;
	}

	pnew->next = ant->next;
	ant->next  = pnew;

	return pnew;
}

private struct s_estcli *CargarEstCliente(long cliente, short objet)
{
	bool existe = FALSE;
	struct s_estcli *ant, *aux, *pnew;

	pnew = (struct s_estcli *) Alloc(sizeof(struct s_estcli));
	pnew->cliok = cliente;
	pnew->objok = objet;
	pnew->next    = NULL;

	if (p_estcli == NULL ) {
		p_estcli = pnew;
		return p_estcli;
	}

	for (ant = p_estcli, aux = p_estcli; aux != NULL; ant=aux, aux = aux->next) {

		if ( aux->cliok > cliente)	break;
		if ( aux->cliok == cliente && aux->objok > objet) break;

		if ( aux->cliok == cliente && aux->objok == objet){
				existe = TRUE;
				break;
		}
	}

	if (existe) {
		free (pnew);
		return aux;
	} 	

	if (aux == p_estcli) {
		p_estcli = pnew;
		pnew->next = aux;
		return pnew;
	}

    pnew->next = ant->next;
	ant->next  = pnew;

	return pnew;
}

private void BorroListaClientes()
{
	struct s_estcli *eq, *eqaux;

	for (eq = p_estcli ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	p_estcli = NULL;
}

private void BorroListaErrores ()
{
	struct s_errores *eq, *eqaux;

	for (eq = p_error ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	p_error = NULL;
}
bool ClienteObjValido (long cliente, short obj)
{
	bool encontro=FALSE;
	
	for (aux_estcli = p_estcli; !encontro && aux_estcli != NULL; aux_estcli = aux_estcli->next) {
		if (cliente == aux_estcli->cliok && obj == aux_estcli->objok)
			encontro = TRUE;
	}
	return encontro;
}
void ListarClienteObjValido ()
{
	fprintf (stderr, "Lista de Cliente validos \n");
	for (aux_estcli = p_estcli; aux_estcli != NULL; aux_estcli = aux_estcli->next) {
		fprintf (stderr, "Cliente %ld obj %d \n", aux_estcli->cliok, aux_estcli->objok);
	}
}

bool ObjetivoValido (long cliente, short objetivo)
{
	int pos;
	bool esta=FALSE;
	
	if (!FmIFld(fm0, FILTRA) || 
		(FmIFld(fm0, FILTRA) && !FmIFld(fm0, CONSIDERA))) {
		if (cliente < FmLFld(fm0, CLID))
			return FALSE;

		if (cliente > FmLFld(fm0, CLIH))
			return FALSE;

		if (cliente == FmLFld(fm0, CLID) && objetivo < FmIFld(fm0, OBJD))
			return FALSE;

		if (cliente == FmLFld(fm0, CLIH) && objetivo > FmIFld(fm0, OBJH))
			return FALSE;
	}

	if (FmIFld(fm0, FILTRA)) {
		for (pos=0; !esta && !FmIsNull (fm0, CLICON, pos); pos ++) {
			if (cliente == FmLFld(fm0, CLICON, pos) && FmIsNull(fm0, OBJCON, pos)) {
				esta = TRUE;
			}
			if (cliente == FmLFld(fm0, CLICON, pos) && objetivo == FmIFld(fm0, OBJCON, pos)) {
				esta = TRUE;
			}
		}
		
		return (FmIFld(fm0, CONSIDERA) ? esta : !esta);
	}

	return TRUE;
}

