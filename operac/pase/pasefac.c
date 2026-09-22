/********************************************************************
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* DESCRIPTION:
*              Pase de Operaciones a Facturación.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
COMENTARIO: Si se modifica funcionalidad actualizar pasefac.hlp


PARAMETROS: 
	N: Modo Normal
    T: Modo Test
    S: Modo Simulación

*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "observa.h"
#include "bill.h"
#include "operac.h"
#include "comerc.h"
#include "comdef.h"
#include "comgral.h"
#include "billpro.h"
#include "comerc.sch"
#include "operac.sch"
#include "billpro.sch"
#include "bill.sch"
#include "pasefac.fmh"
#include "pasefac.rph"

#define MAXCLI  6000
#define MAXNOV  10000
#define USR_INTERF 1237L

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

#define _HORAS_POR_MES_POR_VIG	20000

#define TIPCONC_HORAS		1
#define TIPCONC_ADICIONAL	3
#define TIPCONC_MENSUAL		4

#define _DEBUG	1
#define MIN_VAR_NOVIP 1
#define MAX_VAR_NOVIP 9999

/* Estructuras */
struct cliente {
	int emp;
	long cli;
	short obj;
	long stdv; //Estandar de horas vendidas
	long stdt; //Estandar de horas trabajadas incluye adicional y a cargo de la emp
	long stda; //Estandar de horas adicionales
	long stde; //Estandar de horas a cargo de la empresa
	long stds; //Estandar de horas para el sueldo
	short  ptoser, puesto, conc;
} pcli[MAXCLI], *ucli = pcli, *ecli;

struct novedades {
	long intern, valor;
	short  nrovar;
} pnov[MAXNOV], *unov = pnov, *enov;

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

/* Funciones privadas */
private fm_status before(form fm, fmfield fno, int row);
private fm_status after(form fm, fmfield fno, int row);
void BuscoConceptos (long cliente, short obj, short ptoser, short puesto, DATE fdesde, DATE fhasta, short *conmen, short *conhor, short *conadi);
void GrabarNovedad ();
void CargarCliObj(short emp, long cliente, short objetivo, short condic, short ptoser, short puesto,
				  short hn, short h50, short h100, short h100f, short hsvad);
void AgruparPorNovedad ();
// agrego es_cstd porque si es cantidad para un puesto repetido se debe sumar
void CargarNovedad (long intern, short nrovar, long valor, short conc, bool es_cstd);
bool ObjetivoValido(long cliente, short objetivo);
void BorrarNovedad();
void CargarParte();
void CargarExcepcion();
void CargarRetro();
void CargarRetroExc();
private struct s_errores *CargarError(short codigo, long cliente, short objet, short conc, short ptoser, char *extra, long liq, long peval);
private void BorroListaErrores ();
void ImprimirErrores();
long CliMin();
long CliMax();
short ObjMin();
short ObjMax();
void CargarClientesValidos();
private struct s_estcli *CargarEstCliente(long cliente, short objet);
bool ClienteObjValido (long cliente, short obj);
private void BorroListaClientes();
void LogParte();
void LogRetro();
void ListarClienteObjValido ();
bool EstaAsignado(short emp, long cliente, short objet, long legajo, DATE fecparte);
double AdicionalExcepciones();
void LimpiarVarNov(long cliente, short objetivo);
bool ObjetivoConErrores(long cliente, short objetivo);
void ListarClienteObjetivo();
bool HayNovedad ();

/* Declaraciones globales */
form   fm0;
report rp0;
schema comerc, bill, operac, billpro;
char bufaux[50];
bool herror=FALSE, hmensaje=FALSE, modonorm=FALSE, modotest=FALSE, modosimu=FALSE;
struct s_errores *p_error=NULL, *aux_error;
struct s_estcli  *p_estcli=NULL, *aux_estcli;
FILE *fp_error;
struct comentario pobs[MAXOBS], *uobs = pobs, *eobs;
char cstd[150], archF[50];
FILE *fp1 = NULL;
DATE fecexe, fcierre;
TIME horexe;


wcmd(pasefac, %I% %G%)
{
	fm0			= OpenForm("pasefac",  FM_EABORT);

	comerc		= OpenSchema("comerc", IO_EABORT);
	bill		= OpenSchema("bill",   IO_EABORT);
	billpro		= OpenSchema("billpro",IO_EABORT);
	operac		= OpenSchema("operac", IO_EABORT);

	if (argc >1) {
		modonorm = str_eq (argv[1], "N");
		modotest = str_eq (argv[1], "T");
		modosimu = str_eq (argv[1], "S");
	}
	else
		Error("Se debe ejecutar con un parametro: T (Test), N (Normal), S (Simulación)");

	FmSetIFld(fm0, I_GRUPO, GrupoFact(GrupoUsr(GetUid())));
	
	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	SetTableCache(comerc|OBJETIVO,  10);
	SetTableCache(comerc|ITMFAC,  	100);

	fcierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
	fecexe  = Today();
	horexe  = Hour();

	InicioListaTipoExcepcion();
	SwitchToSchema(operac);		

	sprintf(archF, "/tmp/pasefac.%d.log", ProcPid());

	if ((fp_error = fopen (archF, "wt")) == (FILE *) NULL)
		Error ("No se pudo abrir el archivo de Log /tmp/pasefac.log");

	fprintf(fp_error, "Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));

	fprintf(fp_error, "Liquidacion %ld Fecha desde %.3D hasta %.3D Cliente %ld %d hasta %ld %d  \n",
					FmLFld(fm0, NROLIQ), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));

  	if ((fp1 = fopen("pasefac.txt", "a+")) == NULL)
  		Error("No se pudo crear el archivo de log pasefac.txt");

	ucli = pcli;

	CargarClientesValidos();

//	if (modotest)
//		ListarClienteObjValido ();

	unov=pnov;

	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'P') {
		CargarParte();
		CargarExcepcion();
	}

	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'R') {
		CargarRetro();
	}		


	
	ListarClienteObjetivo();

	AgruparPorNovedad ();
	ImprimirErrores();

	if (herror) {
		fprintf(fp_error, "Hubo errores Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		Error ("Hubo Errores - No se puede hacer el pase - Ver impresion ");
		fclose(fp_error);
		return;
	}


//	BeginTransaction();
	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'P') {
		fprintf(fp_error, "Empieza a grabar LogParte Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		LogParte();
	}

	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'R') {
		fprintf(fp_error, "Empieza a grabar LogRetro Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		LogRetro();
	}

	if (modonorm || modotest) {
		fprintf(fp_error, "Empieza a Borrar  novedades sobrantes Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		BorrarNovedad();
	}
	// No se registro nada 
	if (!HayNovedad()) {
		fprintf(fp_error, "No hay datos para procesar Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		Error ("No hay datos para procesar");
		fclose(fp_error);
		return;
	}

	//modo normal o modo simu borran novedad y graba novedades
	if (modonorm || modotest) {
		fprintf(fp_error, "Empieza a grabar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		GrabarNovedad();
	}

//	EndTransaction();


	fprintf(fp_error, "Fin grabar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	if (!herror && hmensaje) {
		fprintf(fp_error, "Hubo mensajes de aviso Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		WiMsg ("Hubo mensajes de aviso - Ver impresion ");
	}

	BorroListaErrores ();
	BorroListaClientes();
	fprintf(fp_error, "FIN PROCESO Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fclose(fp_error);
	fclose(fp1);
}

private fm_status before(form fm, fmfield fno, int row)
{
	long liq;

	switch (fno) {
	case NROLIQ:
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
	case AGRUPF:
		if (FmIsNull (fm, FECHAD)) {
			DATE fecini;
			
			fecini = FirstMonthDay(Today()) - 15; //Para saber el mes anterior
			FmSetDFld (fm0, FECHAD, DMYToD (23, Month(fecini), Year(fecini)));
			FmSetDFld (fm0, FECHAH, DMYToD (24, Month(Today()), Year(Today())));
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
				Warning("Esta liquidación está cerrada - debe ingresar otra");
				return FM_REDO;
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
			for (pos=0; GetRecord (billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK,1) != ERROR;pos ++) {
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
	}
	return FM_OK;
}

void CargarParte()
{
	dbcursor  cparte, cobj;
	DATE pdia=NULL_DATE;

	cparte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
	cobj   = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());

	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		if (LFld(comerc|OBJETIVO_RESUMEN) == NULL_LONG) {
			CargarError(_ERROR_RESUMEN, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
			continue;			
		}

		/*Cargo las horas del parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);

		//fprintf(stderr, "Seteo %.3D %.3D %ld %ld %d %d \n", FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), CliMin(), ObjMin(),  CliMax(), ObjMax());

		while (FetchCursor(cparte) != ERROR) {

			if (DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (pdia != DFld(operac|PARTE_DIA)) {
				sprintf (bufaux, "Procesando Parte dia %.3D cli %ld obj %d ", DFld(operac|PARTE_DIA), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
				FmSetFld (fm0, COMENT, bufaux);
				FmShowFlds (fm0, COMENT, COMENT);
				WiRefresh();
				pdia = DFld(operac|PARTE_DIA);
			}

			if (modonorm || modosimu) {
				if (LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)) {
					CargarError(_ERROR_LIQ, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR, LFld(operac|PARTE_LIQFAC), NULL_LONG);
					continue;			
				}
			}


/*			fprintf(stderr, "PARTE %ld %d %d %ld %.3D %d %d %d %d \n",
					LFld(operac|PARTE_CLIENTE), 
					IFld(operac|PARTE_OBJETIVO), 
					IFld(operac|PARTE_PTOSER), 
					LFld(operac|PARTE_NROLEG), 
					DFld(operac|PARTE_DIA),
					IFld(operac|PARTE_HSNOR), 
					IFld(operac|PARTE_HS50), 
					IFld(operac|PARTE_HS100F), 
					IFld(operac|PARTE_HS100FE));
*/
			CargarCliObj(FmIFld(fm0, EMP), 
					 LFld(operac|PARTE_CLIENTE), 
					 IFld(operac|PARTE_OBJETIVO), 
					 NULL_SHORT, 
					 IFld(operac|PARTE_PTOSER), 
					 IFld(operac|PARTE_PUESTO), 
					 IFld(operac|PARTE_HSNOR), 
					 IFld(operac|PARTE_HS50), 
					 IFld(operac|PARTE_HS100F), 
					 IFld(operac|PARTE_HS100FE),
					 0);
			
		}
		// Ahora repito la misma rutina para los hijos que tuviera
		SetKey(billpro|OBJETRELbyCOMER, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_LONG, NULL_SHORT);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			SetCursorFrom(cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo (cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cparte) != ERROR) {

				if (DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
					continue;

				if (modonorm || modosimu) {
					if (LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)) {
						CargarError(_ERROR_LIQ, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR, LFld(operac|PARTE_LIQFAC), NULL_LONG);
						continue;			
					}
				}
				CargarCliObj(FmIFld(fm0, EMP), 
						 LFld(comerc|OBJETIVO_CLIENTE), 
						 IFld(comerc|OBJETIVO_OBJET), 
						 NULL_SHORT, 
						 IFld(operac|PARTE_PTOSER), 
						 IFld(operac|PARTE_PUESTO), 
						 IFld(operac|PARTE_HSNOR), 
						 IFld(operac|PARTE_HS50), 
						 IFld(operac|PARTE_HS100F), 
						 IFld(operac|PARTE_HS100FE),
						 0);
			}
		}
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);
	FreeTable(operac|PARTE);
}

void CargarExcepcion()
{
	dbcursor  cexc, cobj;
	short tipoexc;
	double hsvig=0;

	/*Cargo las horas de excepciones */
	cexc   = CreateCursor(operac|EXCEPCIONbyEMP,   IO_NOT_LOCK);
	cobj   = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());

	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		if (LFld(comerc|OBJETIVO_RESUMEN) == NULL_LONG) {
			CargarError(_ERROR_RESUMEN, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
			continue;			
		}

		SetCursorFrom(cexc, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cexc, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);

		while (FetchCursor(cexc) != ERROR) {

			if (DFld(operac|EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
				continue;

			tipoexc = ParteTipoExcepcion(IFld(operac|EXCEPCION_CONDIC),IFld(operac|EXCEPCION_MOTIVO));

			/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
			if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC) {
				continue;
			}

			hsvig = AdicionalExcepciones();
			if (hsvig > 0) {
				fprintf (fp1, "LEXCEPCION\tNroliq\t%ld\tFecha\t%.3D\tHora\t%.3T\tCliente\t%ld\t%s\tObjetivo\t%d\t%s\tNroleg\t%ld\t%s\tFecha\t%.3D\tadicionales\t%.2f\n",
						FmLFld(fm0, NROLIQ),
						fecexe, horexe,
						LFld(operac|EXCEPCION_CLIENTE), 
						GetDescCli(LFld(operac|EXCEPCION_CLIENTE)),
						IFld(operac|EXCEPCION_OBJETIVO),
						GetObjDescrip(LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO)),
						LFld(operac|EXCEPCION_NROLEG),
						GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|EXCEPCION_NROLEG)),
						DFld(operac|EXCEPCION_DIA),
						(hsvig/100.0));
			}

			CargarCliObj(FmIFld(fm0, EMP),
						LFld(comerc|OBJETIVO_CLIENTE),
						IFld(comerc|OBJETIVO_OBJET),
						IFld(operac|EXCEPCION_CONDIC),
						IFld(operac|EXCEPCION_PTOSER),
						IFld(operac|EXCEPCION_PUESTO),
						IFld(operac|EXCEPCION_HORAS), 
						IFld(operac|EXCEPCION_HS50),
						FeriadoNovia(DFld(operac|EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(operac|EXCEPCION_HS100),
						FeriadoNovia(DFld(operac|EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(operac|EXCEPCION_HS100) : 0.0,
						(short)hsvig);

			sprintf (bufaux, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO));
			FmSetFld (fm0, COMENT, bufaux);
			WiRefresh();
		}
		// Ahora repito la misma rutina para los hijos que tuviera
		SetKey(billpro|OBJETRELbyCOMER, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_LONG, NULL_SHORT);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo),
			SetCursorFrom(cexc, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo (cexc, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cexc) != ERROR) {
				if (DFld(operac|EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
					continue;

				tipoexc = ParteTipoExcepcion(IFld(operac|EXCEPCION_CONDIC),IFld(operac|EXCEPCION_MOTIVO));

				/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
				if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC) {
					continue;
				}

				hsvig = AdicionalExcepciones();
				if (hsvig > 0) {
					fprintf (fp1, "LEXCEPCION\tNroliq\t%ld\tFecha\t%.3D\tHora\t%.3T\tCliente\t%ld\t%s\tObjetivo\t%d\t%s\tNroleg\t%ld\t%s\tFecha\t%.3D\tadicionales\t%.2f\n",
							FmLFld(fm0, NROLIQ),
							fecexe, horexe,
							LFld(operac|EXCEPCION_CLIENTE), 
							GetDescCli(LFld(operac|EXCEPCION_CLIENTE)),
							IFld(operac|EXCEPCION_OBJETIVO),
							GetObjDescrip(LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO)),
							LFld(operac|EXCEPCION_NROLEG),
							GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|EXCEPCION_NROLEG)),
							DFld(operac|EXCEPCION_DIA),
							(hsvig/100.0));
				}

				CargarCliObj(FmIFld(fm0, EMP),
						LFld(comerc|OBJETIVO_CLIENTE),
						IFld(comerc|OBJETIVO_OBJET),
						IFld(operac|EXCEPCION_CONDIC),
						IFld(operac|EXCEPCION_PTOSER),
						IFld(operac|EXCEPCION_PUESTO),
						IFld(operac|EXCEPCION_HORAS), 
						IFld(operac|EXCEPCION_HS50),
						FeriadoNovia(DFld(operac|EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(operac|EXCEPCION_HS100),
						FeriadoNovia(DFld(operac|EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(operac|EXCEPCION_HS100) : 0.0,
						(short)hsvig);

			}
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cexc);
	DeleteCursor(cobj);
}

void CargarRetro()
{
	dbcursor  cretro, cobj;

	cretro = CreateCursor(operac|RETRObyEMP, IO_NOT_LOCK);
	cobj   = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());

	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		if (LFld(comerc|OBJETIVO_RESUMEN) == NULL_LONG) {
			CargarError(_ERROR_RESUMEN, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_SHORT, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
			continue;			
		}

		/*Cargo las horas de retroactivos */
		InitRecord(operac|RETRO); //Este InitRecord es para que limpie el buffer y estar seguro que se pare en la primer posicion

		//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
		SetCursorFrom(cretro, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);

		MoveCursorFirst(cretro);

		while (FetchCursor(cretro) != ERROR) {
			//fprintf(stderr, "Cliente %ld Objetivo %d Dia %.3D leg %ld \n", LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), DFld(operac|RETRO_DIA), LFld(operac|RETRO_NROLEG));

			if (DFld(operac|RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (modonorm || modosimu) {
				if (LFld(operac|RETRO_LIQFAC) != NULL_LONG && LFld(operac|RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)) {
					CargarError(_MENSAJE_RETRO, LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
								LFld(operac|RETRO_LIQFAC), NULL_LONG);
					continue;			
				}
			}			

			if (modonorm && IFld(operac|RETRO_CONFIR) == A_CONF ) {
				CargarError(_ERROR_ESTADO, LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
						    NULL_LONG, NULL_LONG);
				continue;			
			}

			CargarCliObj(FmIFld(fm0, EMP), 
						 LFld(comerc|OBJETIVO_CLIENTE),
						 IFld(comerc|OBJETIVO_OBJET), 
						 NULL_SHORT, 
						 IFld(operac|RETRO_PTOSER), 
						 IFld(operac|RETRO_PUESTO), 
						 IFld(operac|RETRO_DHSNOR),
						 IFld(operac|RETRO_DHS50),
						 IFld(operac|RETRO_DHS100F),
						 IFld(operac|RETRO_DHS100FE),
						 0);

			CargarRetroExc();

			sprintf (bufaux, "Procesando Retro dia %.3D", DFld(operac|RETRO_DIA));
			FmSetFld (fm0, COMENT, bufaux);
			WiRefresh();
		}
		// Ahora repito la misma rutina para los hijos que tuviera
		SetKey(billpro|OBJETRELbyCOMER, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_LONG, NULL_SHORT);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
			SetCursorFrom(cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
								FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
								FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);

			MoveCursorFirst(cretro);

			while (FetchCursor(cretro) != ERROR) {
				if (DFld(operac|RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|RETRO_DIA) > FmDFld(fm0, FECHAH))
					continue;

				if (modonorm || modosimu) {
					if (LFld(operac|RETRO_LIQFAC) != NULL_LONG && LFld(operac|RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)) {
						CargarError(_MENSAJE_RETRO, LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
									LFld(operac|RETRO_LIQFAC), NULL_LONG);
						continue;			
					}
				}			

				if (modonorm && IFld(operac|RETRO_CONFIR) == A_CONF ) {
					CargarError(_ERROR_ESTADO, LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
						    NULL_LONG, NULL_LONG);
					continue;			
				}

				CargarCliObj(FmIFld(fm0, EMP), 
						 LFld(comerc|OBJETIVO_CLIENTE),
						 IFld(comerc|OBJETIVO_OBJET), 
						 NULL_SHORT, 
						 IFld(operac|RETRO_PTOSER), 
						 IFld(operac|RETRO_PUESTO), 
						 IFld(operac|RETRO_DHSNOR),
						 IFld(operac|RETRO_DHS50),
						 IFld(operac|RETRO_DHS100F),
						 IFld(operac|RETRO_DHS100FE),
						 0);

				CargarRetroExc();
			}
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
	DeleteCursor(cobj);
	FreeTable(operac|RETRO);
}

void CargarRetroExc()
{
	short tipoexc;

	/*Cargo las horas de retro excepciones */
	SetKey(operac|RETROEXCbyEMP, IFld(operac|RETRO_EMP), LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), DFld(operac|RETRO_DIA),
						LFld(operac|RETRO_NROLEG), IFld(operac|RETRO_PTOSER), IFld(operac|RETRO_PUESTO), IFld(operac|RETRO_NROINT),
						MIN_SHORT, MIN_SHORT);

	while (GetRecord(operac|RETROEXCbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {

		tipoexc = ParteTipoExcepcion(IFld(operac|RETROEXC_CONDIC), IFld(operac|RETROEXC_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		SetKey(comerc|OBJETIVO, LFld(operac|RETROEXC_CLIENTE), IFld(operac|RETROEXC_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		CargarCliObj(FmIFld(fm0, EMP),
					 LFld(comerc|OBJETIVO_CLIENTE),
					 IFld(comerc|OBJETIVO_OBJET), 
					 IFld(operac|RETROEXC_CONDIC),
					 IFld(operac|RETROEXC_PTOSER),
					 IFld(operac|RETROEXC_PUESTO),
					 IFld(operac|RETROEXC_DHORAS),
					 IFld(operac|RETROEXC_DHS50),   
					 FeriadoNovia(DFld(operac|RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(operac|RETROEXC_DHS100),
					 FeriadoNovia(DFld(operac|RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(operac|RETROEXC_DHS100) : 0.0,
					 0);
	}
}


/********************************************************************************
 Carga el vector 
 Si viene de PARTE h100 contiene horas extras de francos  y h100f de feriados.
 Si viene de EXCEPCION h100 contiene horas extras al 100% y h100f viene vacio.
*******************************************************************************/
void CargarCliObj(short emp, long cliente, short objetivo, short condic, short ptoser, short puesto,
				  short hn, short h50, short h100, short h100f, short hsvad)
{
	long svhn, svh50, svh100, svh100f;

	svhn = svh50 = svh100 = svh100f = 0;

	for (ecli = pcli; ecli < ucli; ecli++)
		if (ecli->cli == cliente && ecli->obj == objetivo && ecli->ptoser == ptoser && ecli->puesto == puesto) {
			break;
		}

	if (ecli == ucli) {
		if (ucli == &pcli[MAXCLI])
			Error("Tabla interna saturada. Max %d", MAXCLI);
		ucli->cli     = cliente;
		ucli->obj     = objetivo;
		ucli->stdt    = 0;
		ucli->stda    = 0;
		ucli->stde    = 0;
		ucli->stdv    = 0;
		ucli->stds    = 0;
		ucli->conc    = 0;
		ucli->ptoser  = ptoser;
		ucli->puesto  = puesto;

		/***************************
		Descomentar si se necesita el standard de horas vendidas
		for (fecha = FmDFld(fm0, FECHAD); fecha <= FmDFld(fm0, FECHAH); fecha++) {
			GetHorasPorDia(FmIFld(fm0, EMP), cliente, objetivo, ptoser, fecha, &svhn, &svh50, &svh100, &svh100f);
			ecli->stdv     += svhn + svh50 + svh100 + svh100f;
		} 
		***************************/
//		fprintf(stderr, "Cargo %ld %d puesto %d ptoser %d stdv %ld - stdt %ld - stda %ld - stde %ld \n", ucli->cli, ucli->obj,ucli->ptoser, ucli->puesto, ucli->stdv, ucli->stdt, ucli->stda, ucli->stde);
		ucli++;
	} 
	if (condic != NULL_SHORT) {		// VIENE DE EXCEPCION
		if (condic == ACARGO_EMP) {
			ecli->stde    += hn + h50 + h100 + h100f;
		}
		else {
			ecli->stda    += hn + h50 + h100 + h100f;
		}
	}
	else {                      // VIENE DE PARTE
 		ecli->stdt    += hn + h50 + h100 + h100f;
	}     

	ecli->stds += hsvad;

//	fprintf(stderr, "Cargofinal %ld %d ptoser %d puesto %d stdv %ld - stdt %ld - stda %ld - stde %ld \n", ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, ecli->stdv, ecli->stdt, ecli->stda, ecli->stde);
}

void AgruparPorNovedad ()
{
	long valor;
	short conmen, conhor, conadi;

	unov = pnov;	


	for (ecli = pcli; ecli < ucli; ecli++) {

		SetLFld(comerc|OBJETIVO_CLIENTE, ecli->cli);
		SetIFld(comerc|OBJETIVO_OBJET,   ecli->obj);
		(void) GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		
		if (_DEBUG)	fprintf(stderr, "Viene cli %ld obj %d ptoser %d puesto %d total hs %ld stdv %ld - stdt %ld - stda %ld - stde %ld \n", ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, (ecli->stdt - ecli->stda - ecli->stde), ecli->stdv, ecli->stdt, ecli->stda, ecli->stde);



		conmen = conhor = conadi = 0;
		BuscoConceptos (ecli->cli, ecli->obj, ecli->ptoser,ecli->puesto, FmDFld (fm0, FECHAD),  FmDFld (fm0, FECHAH), &conmen, &conhor, &conadi);

		if (conmen == 0 && conhor == 0) {
			if (_DEBUG)	fprintf(stderr, "NO hay conceptos \n");
			continue;
		}


		if (conmen > 0) {
			SetIFld(comerc|ITMFAC_ITEM, conmen);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);
			
			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
						   IFld(comerc|ITMFAC_CANT), //nrovar
						   100,
						   conmen,
						   FALSE);

			// Informa cantidad de puestos solo para precio cerrado
			// Se comenta este parrafo, porque la cantidad de puestos se informa desde MARTE
			SetKey(operac|PUESTOS, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), ecli->ptoser, ecli->puesto);
			//fprintf(stderr, "busco %d %ld %d %d %d\n", FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), ecli->ptoser, ecli->puesto);
			if (GetRecord(operac|PUESTOS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				//comento porque trae mal la cantidad de puestos en algunos casos 
				//SetKey(comerc|NPUESTObyCLIENTE, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), ecli->ptoser, IFld(operac|PUESTOS_PUESTO));
				//if (GetRecord(comerc|NPUESTObyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
 				SetKey(comerc|NPUESTObyCODINTCL, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT));
				if (GetRecord(comerc|NPUESTObyCODINTCL, THIS_KEY, IO_NOT_LOCK) != ERROR) {
					CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
						   IFld(comerc|ITMFAC_CSTD), //nrovar
						   IFld(comerc|NPUESTO_CANTPUE)*100,
						   conmen, TRUE);
						   //fprintf(stderr, "encuentro %d %ld %d %d %d\n", IFld(comerc|NPUESTO_EMP), LFld(comerc|NPUESTO_CLIENTE), IFld(comerc|NPUESTO_OBJET), IFld(comerc|NPUESTO_TIPPTO), IFld(comerc|NPUESTO_PUESTO)); 
						   //fprintf(stderr, "%d %d\n", IFld(comerc|NPUESTO_CANTPUE), IFld(comerc|ITMFAC_CSTD));
				}
				else {
					if (_DEBUG)	fprintf(stderr, "Error 2 %d %d ptoser %d puesto %d conc %d \n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), ecli->ptoser,ecli->puesto, conmen);
				}
			}
			else {
				if (_DEBUG)	fprintf(stderr, "Error 1 %d %d ptoser %d puesto %d conc %d \n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), ecli->ptoser,ecli->puesto, conmen);
			} //Fin del comentario 
		}


		if (conhor > 0) {

			valor =  ecli->stdt - ecli->stda - ecli->stde;

			if (valor < 0) {
				CargarError (_MENSAJE_NC, ecli->cli, ecli->obj, conhor, NULL_SHORT, NULL_STR, NULL_LONG, valor);
			}

			SetIFld(comerc|ITMFAC_ITEM, conhor);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

			if (conmen > 0) // Si tiene precio cerrado la cantidad de horas tiene que ser cero.
				valor = 0;

			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
				   IFld(comerc|ITMFAC_CANT), //nrovar
				   valor,
				   conhor, FALSE);

		}

		if (conadi > 0) {
			valor = ecli->stda;

			SetIFld(comerc|ITMFAC_ITEM, conadi);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
					   IFld(comerc|ITMFAC_CANT), //nrovar
					   valor,
					   conadi, FALSE);
		}                 
		else {
			if (_DEBUG)	fprintf(stderr, "No encontró concepto de horas adicionales ptoser %d puesto %d\n", ecli->ptoser,ecli->puesto);
		}


	}
}


void GrabarNovedad ()
{    
	long total=0;
	long cli;
	int obj;
	double pin;
	char v_auxitem[5];
		
	for (enov = pnov; enov < unov; enov++) {
		
		// GG 15092021 la novedad solo se debe grabar si no hay error para ese cliente/objetivo
		// en este punto no tengo cliente objetivo tengo el intern, lo usaré en prod.intern para obtener pin que se separa en cli obj
		// función de billing para obtener PIN que es clienteobjetivo PinActual(long intern)
		// luego con cli obj valido si aparece en la lista de errores, si no aparece grabo en varnov sino no hace nada
		
		pin = PinActual(enov->intern);	  	
	  	cli = (long)(pin/100);
	  	obj = (int)(pin - ((long)(pin/100)*100));
	  	
	  	//fprintf(stderr, "%d - %d - %d\n", (long)pin, cli, obj);
	   
	  	
		if (!ObjetivoConErrores(cli, obj)) {
			//fprintf(stderr, "GrabaNov %ld %d %ld \n", enov->intern, enov->nrovar, enov->valor);

			//Agrego validación de puestos compuestos, si es un puesto compuesto divido la cantidad por 2, si es par ok si es impar me quedo con el entero.
			// en caso de tener 300 al dividir por 100 tengo el 3 y al dividir por 2 en vez de 1.5 me quedo con el 1 y luego se multiplica por 100 de nuevo.
			sprintf(v_auxitem, "%d\0", enov->nrovar);
			if (EsParNov(PARNOV_EMPRESA_GENERAL, PARNOV_ITM_FAC_PC, Today(), v_auxitem))
   				enov->valor = (enov->valor / 100 /2) * 100;

			InitRecord (bill|VARNOV);
			SetLFld(bill|VARNOV_NROLIQ, FmLFld (fm0, NROLIQ));
			SetLFld(bill|VARNOV_INTERN, enov->intern);
			SetIFld(bill|VARNOV_NROVAR, enov->nrovar);
			SetLFld(bill|VARNOV_VALOR,  enov->valor);
		
			total += enov->valor;	
			if (_DEBUG)	 		fprintf(stderr, "Se Graba Novedad en bill.varnov : nroliq %ld - intern %ld - nrovar %d - valor %ld\n", FmLFld (fm0, NROLIQ),  enov->intern, enov->nrovar, enov->valor);
			PutRecord(bill|VARNOV);
		}
	}

	//fprintf(stderr, "TOTAL %ld \n", total);

}

void BuscoConceptos (long cliente, short obj, short ptoser, short codint, DATE fdesde, DATE fhasta, short *conmen, short *conhor, short *conadi)
{
	dbcursor cur;
	short puesto;

	if (_DEBUG)	fprintf(stderr, "Entra cliente %d obj %d ptoser %d codint %d\n", cliente, obj, ptoser, codint);

	SetKey(operac|PUESTOSbyCLIENTE, cliente, obj, ptoser, codint);
	if (GetRecord (operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
 		if (_DEBUG)	fprintf(stderr, "No se encontro cliente %d obj %d ptoser %d codint %d en PUESTOS\n", cliente, obj, ptoser, codint);
		return;
	}
	
	puesto = IFld(operac|PUESTOS_PUESTO);

	cur = CreateCursor (comerc|ITMXPUEbyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom (cur, cliente, obj, ptoser, puesto, TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL),
			SFld(operac|PUESTOS_DIA1), SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4), 
			SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7), 
			SFld(operac|PUESTOS_REGIM), MIN_SHORT);
	SetCursorTo   (cur, cliente, obj, ptoser, puesto, TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL),
			SFld(operac|PUESTOS_DIA1), SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4), 
	 		SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7), 
			SFld(operac|PUESTOS_REGIM), MAX_SHORT);

	while (FetchCursor (cur) != ERROR) {

		SetIFld(comerc|ITMFAC_ITEM, IFld(comerc|ITMXPUE_ITEM));
		GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

		switch (IFld (comerc|ITMFAC_TIPCONC)) {
		case TIPCONC_MENSUAL:
			*conmen = IFld (comerc|ITMXPUE_ITEM);
			if (_DEBUG)	fprintf(stderr, "Viene conc itmxpue %d MENSUAL  \n", IFld (comerc|ITMXPUE_ITEM));
			break;
		case TIPCONC_HORAS:
			*conhor = IFld (comerc|ITMXPUE_ITEM);
			if (_DEBUG)	fprintf(stderr, "Viene conc itmxpue %d HORAS  \n", IFld (comerc|ITMXPUE_ITEM));
			break;
		case TIPCONC_ADICIONAL:
			*conadi = IFld (comerc|ITMXPUE_ITEM);
			if (_DEBUG)	fprintf(stderr, "Viene conc itmxpue %d ADICONAL  \n", IFld (comerc|ITMXPUE_ITEM));
			break;
		default:
			if (_DEBUG)	fprintf(stderr, "Tipo de concepto no contemplado");
		}
	}


	if (conmen == 0 && conhor == 0) {
		if (_DEBUG)	fprintf(stderr, "da error  \n");
		CargarError (_ERROR_NOCONC, cliente, obj, NULL_SHORT, ptoser,
					NULL_STR, NULL_LONG, NULL_LONG);
		return;
	}

	//Valido si tiene precio cargado LD
/*	if (ultconc != NULL_SHORT) {
		SetIFld(comerc|ITMFAC_ITEM, ultconc);
		if (GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			SetKey(bill|VARFIJbyINTERN, LFld(comerc|OBJETIVO_INTERN), IFld(comerc|ITMFAC_PRECIO));
			if (GetRecord(bill|VARFIJbyINTERN, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				CargarError (_ERROR_NOPRECIO, cliente, obj, ultconc, ptoser,
						NULL_STR, NULL_LONG, NULL_LONG);
				return NULL_SHORT;
			}
		}
	}
*/
}

void CargarNovedad (long intern, short nrovar, long valor, short conc, bool es_cstd)
{
    //char v_auxitem[5];
    
	if (_DEBUG)	fprintf(stderr, "GrabaNovedad Conc %d nrovar %d valor %ld \n", conc, nrovar, valor);

	if (!valor)
		return;

	if (IFld(comerc|ITMFAC_TIPCONC) != TIPCONC_ADICIONAL && *FmSFld (fm0, TIPHO) == 'E')
		return;

	if (IFld(comerc|ITMFAC_TIPCONC) == TIPCONC_ADICIONAL && *FmSFld (fm0, TIPHO) == 'N')
		return;

	for (enov = pnov; enov < unov; enov++) {
		if (enov->intern == intern && enov->nrovar == nrovar) {
			break;
		}
	}
			
	if (enov == unov) {
		unov->intern  = intern;
		unov->nrovar  = nrovar;
		unov->valor   = valor;
		unov++;

		if (unov == &pnov[MAXNOV])
			Error("Tabla interna saturada. Max %d", MAXNOV);

		return;
	} 

	if (IFld(comerc|ITMFAC_TIPCONC) != TIPCONC_MENSUAL)
		enov->valor += valor;
		
    
    //sprintf(v_auxitem, "%d\0", nrovar);
    // si son líneas compuestas la cantidad de puesto no se debe sumar y debe ser una sola, es decir la de un puesto.
   	//if (es_cstd && EsParNov(PARNOV_EMPRESA_GENERAL, PARNOV_ITM_FAC_PC, Today(), v_auxitem))
   	//	enov->valor = valor;
   	//else
   	
   	// acá guarda la cantidad de puestos, si vienen dos lineas exactamente iguales suma las cantidades de puestos
	if (es_cstd) 
		enov->valor += valor;
}

private struct s_errores *CargarError(short codigo, long cliente, short objet, short conc, 
										short ptoser, char *extra, long liq, long peval)
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
	pnew->eval = peval;
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
				aux->eval += peval;
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

void ImprimirErrores()
{
	char buferr[200];
	
	rp0 = OpenReport("pasefac", RP_EABORT);

	RpSetLFld(rp0, RLIQUI, FmLFld(fm0, NROLIQ));
	RpSetFld (rp0, RDLIQUI, FmSFld(fm0, DESCLIQ));
	RpSetLFld(rp0, RCLID, FmLFld(fm0, CLID));
	RpSetFld (rp0, RDCLID, FmSFld(fm0, DCLID));
	RpSetLFld(rp0, RCLIH, FmLFld(fm0, CLIH));
	RpSetFld (rp0, RDCLIH, FmSFld(fm0, DCLIH));
	RpSetIFld(rp0, ROBJD, FmIFld(fm0, OBJD));
	RpSetFld (rp0, RDOBJD, FmSFld(fm0, DOBJD));
	RpSetIFld(rp0, ROBJH, FmIFld(fm0, OBJH));
	RpSetFld (rp0, RDOBJH, FmSFld(fm0, DOBJH));
	RpSetDFld (rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld (rp0, RFECHAH, FmDFld(fm0, FECHAH));

	for (aux_error = p_error; aux_error != NULL; aux_error = aux_error->next) {

		switch (aux_error->tipoerr){
		case  _ERROR_SUBCONC :
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d No existe el subconconcepto especial para el concepto %d", 
							aux_error->cliente, aux_error->objet, aux_error->econc);
		break;
		case _ERROR_HSADIC : 
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d Horas adicionales se quisieron asignar al concepto %d pero no esta cargado - falta precio",
							aux_error->cliente, aux_error->objet, aux_error->econc);
		break;
		case  _ERROR_NOCONC :
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d No se encontro concepto para cargar las horas %s en el puesto %d %s",
							aux_error->cliente, aux_error->objet, aux_error->extra, 
							aux_error->eptoser, GetDescPto(aux_error->eptoser));
		break;
		case  _ERROR_NOPRECIO :
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d No tiene precio el concepto %d para cargar las horas %s en el puesto %d %s",
							aux_error->cliente, aux_error->objet, aux_error->econc, aux_error->extra, 
							aux_error->eptoser, GetDescPto(aux_error->eptoser));
		break;
		case  _ERROR_LIQ : 
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d Tiene horas en la liquidacion %ld",
							aux_error->cliente, aux_error->objet, aux_error->eliq);
		break;
		case  _ERROR_ESTADO : 
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d No tiene el cierre completo",
							aux_error->cliente, aux_error->objet);
		break;
		case  _ERROR_RESUMEN : 
			LimpiarVarNov(aux_error->cliente, aux_error->objet);
			sprintf (buferr, "Error: Cliente %9.9ld Objetivo %4.4d No tiene resumen cargado",
							aux_error->cliente, aux_error->objet);
		break;
		case  _MENSAJE_ESTADO : 
			sprintf (buferr, "Aviso: Cliente %9.9ld Objetivo %4.4d No se paso. No tiene el cierre completo",
							aux_error->cliente, aux_error->objet);
		break;
		case  _MENSAJE_LIQ : 
			sprintf (buferr, "Aviso: Cliente %9.9ld Objetivo %4.4d NO se paso. Tiene horas en la liquidacion %ld",
							aux_error->cliente, aux_error->objet, aux_error->eliq);
		break;
		case  _MENSAJE_BILL : 
			sprintf (buferr, "Aviso: Cliente %9.9ld Objetivo %4.4d NO se paso. No tiene asignado a ningun resumen(billing)",
							aux_error->cliente, aux_error->objet);
		break;
		case  _MENSAJE_RETRO : 
			sprintf (buferr, "Aviso: Cliente %9.9ld Objetivo %4.4d tiene retroactivos en la liquidacion %ld. Se paso solo lo nuevo",
							aux_error->cliente, aux_error->objet, aux_error->eliq);
		break;
		case  _MENSAJE_NC : 
			sprintf (buferr, "Aviso: Cliente %9.9ld Objetivo %4.4d. Hay que hacer NC concepto %d por %.2f horas",
							aux_error->cliente, aux_error->objet, aux_error->econc, (-0.01 * (double) aux_error->eval));
		break;
		default : 
			sprintf (buferr, NULL_STR);
		break;
		}

		RpSetFld(rp0, RDETALLE, buferr);
		DoReport(rp0, LINEA);
	}
	EndReport(rp0);
}

private void BorroListaErrores ()
{
	struct s_errores *leq, *eqaux;

	for (leq = p_error ; leq != NULL; ) {
		eqaux = leq;
		leq = leq->next;
		free(eqaux);
	}
	p_error = NULL;
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
	struct s_estcli *leq, *eqaux;

	for (leq = p_estcli ; leq != NULL; ) {
		eqaux = leq;
		leq = leq->next;
		free(eqaux);
	}
	p_estcli = NULL;
}

void CargarClientesValidos()
{
	dbcursor curobj;
	bool estado_ok=TRUE, liq_ok=TRUE, tiene_parte=FALSE, bill_ok=TRUE;
	long liqmal=NULL_LONG;

	curobj=CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK); // Solo recorre los Objetivos relacionados a la Empresa cargada en Pantalla
	SetCursorFrom (curobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo	  (curobj, FmIFld(fm0, EMP), CliMax(), ObjMax());
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

			if (!modonorm){
				if (liq_ok && LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)){
					liq_ok = FALSE;
					liqmal = LFld(operac|PARTE_LIQFAC);
				}
			}				
			tiene_parte=TRUE;
		}

		SetKey(operac|PARTEbyPUESTO, FmIFld(fm0, EMP),LFld(comerc|OBJETIVO_CLIENTE), 
				IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);
		if (GetRecord (operac|PARTEbyPUESTO, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (DFld(operac|PARTE_DIA) >=  FmDFld(fm0, FECHAD)) {
//				if (modonorm && estado_ok && IFld(operac|PARTE_CONFIR) == A_CONF && DFld(operac|PARTE_DIA) > fcierre) {
//					estado_ok = FALSE;
//				}

				if (!modonorm){
					if (liq_ok &&  LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)){
						liq_ok = FALSE;
						liqmal = LFld(operac|PARTE_LIQFAC);
					}
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

bool ClienteObjValido (long cliente, short obj)
{
	bool encontro=FALSE;
	
	for (aux_estcli = p_estcli; !encontro && aux_estcli != NULL; aux_estcli = aux_estcli->next) {
		if (cliente == aux_estcli->cliok && obj == aux_estcli->objok)
			encontro = TRUE;
	}
	return encontro;
}


void LogParte()
{
	dbcursor  cparte, cobj;
	DATE pdia=NULL_DATE;

	cparte = CreateCursor(operac|PARTEbyEMP, IO_LOCK);
	cobj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		if (ObjetivoConErrores(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) {
//			fprintf(stderr, "NO CAMBIO LOG de %ld %d \n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			continue;
		}

		/*Cargo las horas del parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cparte) != ERROR) {

			if (DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (pdia != DFld(operac|PARTE_DIA)) {
				sprintf (bufaux, "Grabando Log Parte dia %.3D cli %ld obj %d ", DFld(operac|PARTE_DIA), LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
				FmSetFld (fm0, COMENT, bufaux);
				FmShowFlds (fm0, COMENT, COMENT);
				WiRefresh();
				pdia = DFld(operac|PARTE_DIA);
			}

			if (!modonorm && LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)) {
				continue;
			}

//			if (modonorm && IFld(operac|PARTE_CONFIR) == A_CONF && DFld(operac|PARTE_DIA) > fcierre ) {
//				continue;
//			}

			if (_DEBUG)	fprintf(stderr, "Se graba LogParte en operac.parte : nroliq %ld\n", FmLFld(fm0, NROLIQ));

			if (modonorm) {
				SetLFld(operac|PARTE_LIQFAC, FmLFld(fm0, NROLIQ));
				PutRecord(operac|PARTE);
				FreeRecord(operac|PARTEbyEMP,THIS_KEY);
			}
		}
		// Ahora repito la misma rutina para los hijos que tuviera
		
		//fprintf(stderr, "cli %ld obj %d\n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		SetKey(billpro|OBJETRELbyCOMER, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_LONG, NULL_SHORT);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			SetCursorFrom(cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo (cparte, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
						FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(cparte) != ERROR) {

				if (DFld(operac|PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|PARTE_DIA) > FmDFld(fm0, FECHAH))
					continue;

				if (!modonorm && LFld(operac|PARTE_LIQFAC) != NULL_LONG && LFld(operac|PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)) {
					continue;
				}
				if (modonorm) {
					SetLFld(operac|PARTE_LIQFAC, FmLFld(fm0, NROLIQ));
					PutRecord(operac|PARTE);
					FreeRecord(operac|PARTEbyEMP,THIS_KEY);
				}
			}
		}
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);
	FreeTable(operac|PARTE);
}

void LogRetro()
{
	dbcursor  cretro, cobj;

	cretro = CreateCursor(operac|RETRObyEMP, IO_NOT_LOCK);
	cobj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		SetKey(billpro|OBJETRELbyCLIENTE, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
		if (GetRecord(billpro|OBJETRELbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			continue; // Siendo de la Empresa Pedida, tiene un Objetivo Padre relacionado, no se lo toma en cuenta

		if (ObjetivoConErrores(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) {
//			fprintf(stderr, "NO CAMBIO LOG de %ld %d \n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			continue;
		}

		/*Cargo las horas de retroactivos */
		InitRecord(operac|RETRO); //Este InitRecord es para que limpie el buffer y estar seguro que se pare en la primer posicion
		//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
		SetCursorFrom(cretro, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);


		MoveCursorFirst(cretro);

		while (FetchCursor(cretro) != ERROR) {
			if (DFld(operac|RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|RETRO_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (ObjetivoConErrores(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO))){
//				fprintf(stderr, "NO CAMBIO LOG RETRO de %ld %d \n", LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO));
				continue;
			}

			if (LFld(operac|RETRO_LIQFAC) != NULL_LONG && LFld(operac|RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)) {
				continue;			
			}

			if (modonorm && IFld(operac|RETRO_CONFIR) == A_CONF ){
				continue;
			}

			if (_DEBUG)	fprintf(stderr, "Se graba LogRetro operac.retro : nroliq %ld\n", FmLFld(fm0, NROLIQ));

			if (modonorm) {
				SetLFld(operac|RETRO_LIQFAC, FmLFld(fm0, NROLIQ));
				PutRecord(operac|RETRO);
			   	FreeRecord(operac|RETRObyEMP,THIS_KEY);
			}

			sprintf (bufaux, "Log Retro dia %.3D", DFld(operac|RETRO_DIA));
			FmSetFld (fm0, COMENT, bufaux);
			WiRefresh();
		}
		// Ahora repito la misma rutina para los hijos que tuviera
		SetKey(billpro|OBJETRELbyCOMER, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), NULL_LONG, NULL_SHORT);
		while (GetRecord(billpro|OBJETRELbyCOMER, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
			SetCursorFrom(cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
								FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, IFld(billpro|OBJETREL_EMPOP), LFld(billpro|OBJETREL_CLIENTE), IFld(billpro|OBJETREL_OBJETOP),
								FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);

			MoveCursorFirst(cretro);

			while (FetchCursor(cretro) != ERROR) {
				if (DFld(operac|RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(operac|RETRO_DIA) > FmDFld(fm0, FECHAH))
					continue;

				if (ObjetivoConErrores(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO))){
	//				fprintf(stderr, "NO CAMBIO LOG RETRO de %ld %d \n", LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO));
					continue;
				}

				if (LFld(operac|RETRO_LIQFAC) != NULL_LONG && LFld(operac|RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)) {
					continue;			
				}

				if (modonorm && IFld(operac|RETRO_CONFIR) == A_CONF ){
					continue;
				}

				if (_DEBUG)	fprintf(stderr, "Se graba LogRetro operac.retro : nroliq %ld\n", FmLFld(fm0, NROLIQ));

				if (modonorm) {
					SetLFld(operac|RETRO_LIQFAC, FmLFld(fm0, NROLIQ));
					PutRecord(operac|RETRO);
				   	FreeRecord(operac|RETRObyEMP,THIS_KEY);
				}
			}
		}
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
	FreeTable(operac|RETRO);
}

void ListarClienteObjValido ()
{
	fprintf (stderr, "Lista de Cliente validos \n");
	for (aux_estcli = p_estcli; aux_estcli != NULL; aux_estcli = aux_estcli->next) {
		fprintf (stderr, "Cliente %ld obj %d \n", aux_estcli->cliok, aux_estcli->objok);
	}
}

bool EstaAsignado(short emp, long cliente, short objet, long legajo, DATE fecparte)
{
	schema old, voperac;
	bool enasig=FALSE;

	old    = CurrentSchema();
	voperac = OpenSchema("operac", IO_EABORT);

	SetIFld(ASIG_EMP,      emp);
	SetLFld(ASIG_CLIENTE,  cliente);
	SetIFld(ASIG_OBJETIVO, objet);
	SetLFld(ASIG_NROLEG,   legajo);
	SetIFld(ASIG_PTOSER, 	NULL_SHORT);
	SetIFld(ASIG_PUESTO, 	NULL_SHORT);
	SetIFld(ASIG_NROINT, 	NULL_SHORT);
	while (!enasig &&  GetRecord(ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 4) != ERROR) {
			if (fecparte >= DFld(ASIG_FECASIG)) {
				enasig = TRUE;
			}
	}
	if (!enasig) {
		SetIFld(ASIGH_EMP,      emp);
		SetLFld(ASIGH_NROLEG,   legajo);
		SetLFld(ASIGH_CLIENTE,  cliente);
		SetIFld(ASIGH_OBJETIVO, objet);
		SetIFld(ASIGH_PTOSER, 	NULL_SHORT);
		SetIFld(ASIGH_PUESTO, 	NULL_SHORT);
		SetIFld(ASIGH_NROINT, 	NULL_SHORT);
		while (GetRecord(ASIGHbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 4) != ERROR) {
			if (IFld(ASIGH_MOTIVO) == DESXERROR || IFld(ASIGH_MOTIVO) == ALTAPARTE)
				continue;

			if (fecparte >= DFld(ASIGH_FECALT) && fecparte <= DFld(ASIGH_FECBAJ)) {
				enasig = TRUE;
			}
		}
	}
	SwitchToSchema(old);
	return enasig;
}

double AdicionalExcepciones()
{
	double hsparte=0, hsexc=0, hsadic=0;
	bool estasig=FALSE;

	//Me fijo en el parte para saber si se toma para el aumento de sueldo

	if (IFld(operac|EXCEPCION_CONDIC) != FACTURABLE ) {
		return hsadic;
	}

	estasig = EstaAsignado(FmIFld(fm0, EMP), LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO),
							 LFld(operac|EXCEPCION_NROLEG), DFld(operac|EXCEPCION_DIA));

	if (estasig) {
		/****
		fprintf (stderr, "ESTA ASIGNADO cli %ld obj %d leg %ld dia %.3D \n",
					LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO),
					LFld(operac|EXCEPCION_NROLEG),  DFld(operac|EXCEPCION_DIA));
		****/		
		return hsadic;
	}

	SetKey(operac|PARTEbyEMP, FmIFld(fm0, EMP), LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO),
							DFld(operac|EXCEPCION_DIA), LFld(operac|EXCEPCION_NROLEG),
							IFld(operac|EXCEPCION_PTOSER),	IFld(operac|EXCEPCION_PUESTO),IFld(operac|EXCEPCION_NROINT));
	if (GetRecord(operac|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		hsparte = IFld(operac|PARTE_HSNOR) +  IFld(operac|PARTE_HS50) + IFld(operac|PARTE_HS100F) + IFld(operac|PARTE_HS100FE);
		hsexc   = IFld(operac|EXCEPCION_HORAS) + IFld(operac|EXCEPCION_HS50) + IFld(operac|EXCEPCION_HS100);
		hsadic  = hsexc >= hsparte ? hsexc : 0;
		
		/**
		fprintf (stderr, "cli %ld obj %d leg %ld dia %.3D %.2f %.2f adicionales %.2f \n",
						LFld(operac|EXCEPCION_CLIENTE), IFld(operac|EXCEPCION_OBJETIVO),
						LFld(operac|EXCEPCION_NROLEG),  DFld(operac|EXCEPCION_DIA),
						hsparte, hsexc, hsadic);
		***/
	}

	return hsadic;
}

void LimpiarVarNov(long cliente, short objetivo)
{
	static schema prev;
	static dbtable AOBJETIVO;

	prev = CurrentSchema();

	if (!AOBJETIVO) {
		AOBJETIVO = CreateAlias (comerc|OBJETIVO);
	}

	SetKey(AOBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(AOBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Error("Quizo limpiar variables del cliente %ld objetivo %d que no existe", cliente, objetivo);
	}

	if (LFld(AOBJETIVO_INTERN) == NULL_LONG) {
		return;
	}

	SetKey(bill|VARNOVbyNROLIQ, FmLFld(fm0, NROLIQ), LFld(AOBJETIVO_INTERN), MIN_SHORT);
	while(GetRecord(bill|VARNOVbyNROLIQ, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {

		if (LFld(bill|VARNOV_CUID) == USR_INTERF)
			continue;

		if (IFld(bill|VARNOV_NROVAR) < MIN_VAR_NOVIP || IFld(bill|VARNOV_NROVAR) > MAX_VAR_NOVIP)
			continue;

        if (_DEBUG)	fprintf(stderr, "Se borran VarMov en bill.varnov los registros con: nroliq %ld - intern %ld \n", FmLFld(fm0, NROLIQ), LFld(AOBJETIVO_INTERN));

		DelRecord(bill|VARNOV);
	}

	SwitchToSchema(prev);
}

bool ObjetivoConErrores(long cliente, short objetivo)
{
	for (aux_error = p_error; aux_error != NULL; aux_error = aux_error->next) {
		if (aux_error->cliente == cliente && aux_error->objet == objetivo) {
			return TRUE;
		}
	}
	return FALSE;
}



void ListarClienteObjetivo()
{

	if (!_DEBUG)
		return;

	fprintf(stderr, "LISTA OBJETIVOS\n");
	for (ecli = pcli; ecli < ucli; ecli++) {

		SetLFld(comerc|OBJETIVO_CLIENTE, ecli->cli);
		SetIFld(comerc|OBJETIVO_OBJET,   ecli->obj);
		(void) GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		
		fprintf(stderr, "\tcli %ld obj %d ptoser %d puesto %d total hs %ld stdv %ld - stdt %ld - stda %ld - stde %ld \n", ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, (ecli->stdt - ecli->stda - ecli->stde), ecli->stdv, ecli->stdt, ecli->stda, ecli->stde);
	}
	fprintf(stderr, "FIN LISTA OBJETIVOS\n");
}
void BorrarNovedad()
{

	SetKey(bill|VARNOVbyNROLIQ, FmLFld(fm0, NROLIQ), NULL_LONG, NULL_SHORT);
	while(GetRecord(bill|VARNOVbyNROLIQ, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {

		fprintf(fp_error, "Empieza a Recorrer Liquidacio %ld - Intern %d\n", LFld(bill|VARNOV_NROLIQ),  LFld(bill|VARNOV_INTERN));

		SetKey(comerc|OBJETIVObyNROINT, LFld(bill|VARNOV_INTERN));
		if(GetRecord(comerc|OBJETIVObyNROINT, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) {
				if (_DEBUG)	fprintf(stderr, "Se borran Novedad en bill.varnov los registros con: nroliq %ld - intern %ld \n", FmLFld(fm0, NROLIQ), LFld(comerc|OBJETIVO_INTERN));
				DelRecord(bill|VARNOV);
			}
		
		} 
	} 



}


bool HayNovedad ()
{    
	for (enov = pnov; enov < unov; enov++) {
		SetLFld(bill|VARNOV_NROLIQ, FmLFld (fm0, NROLIQ));
		SetLFld(bill|VARNOV_INTERN, enov->intern);
		SetIFld(bill|VARNOV_NROVAR, enov->nrovar);
		if (GetRecord(bill|VARNOVbyNROLIQ, THIS_KEY, IO_NOT_LOCK) != ERROR) {

//			fprintf (stderr, "valor VARNOV %ld-%ld-%d   (base %ld  vector %ld)\n", FmLFld (fm0, NROLIQ), enov->intern, enov->nrovar, LFld(bill|VARNOV_VALOR), enov->valor );

			if (LFld(bill|VARNOV_VALOR)!= enov->valor) {
				fprintf (stderr, "no coincide valor VARNOV %ld-%ld-%d   (base %ld  vector %ld)\n", FmLFld (fm0, NROLIQ), enov->intern, enov->nrovar, LFld(bill|VARNOV_VALOR), enov->valor );
				return TRUE;
			}
		} 
		else {
//			fprintf (stderr, "No encuentra registros en VARNOV %ld-%ld-%d\n", FmLFld (fm0, NROLIQ), enov->intern, enov->nrovar );
			return TRUE;
		} 

	}

	return FALSE;


}

