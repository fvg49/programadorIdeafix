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
COMENTARIO: Si se modifica funcionalidad actualizar pfacnew.hlp

Modificacion 03/01/2000 : 
	Para el cliente cencosud (3001) las horas especiales del concepto 45 (horas vigilador A)
	deben ir al mismo concepto que las normales (45).

Modificacion 17/06/2003 : 
	Antes los errores terminaban el programa sin procesar nada.
	Ahora solo no pasan los objetivos con errores pero el resto se procesa.
 	Para eso se agrego la funcion LimpiarVarNov para borrar si genero algo para ese objetivo,
 	y la funcion ObjetivoConErrores para no cambiar el log (campo liqfac) de los objetivo que tuvieron errores.

Modificacion 21/10/2003 : 
	Se creo modom para pruebas de sistemas.
	Se creo mensaje _ERROR_NOPRECIO para los conceptos que no tienen el precio cargado.
	Cambio en donde deja el log. Ahora le agregue el pid para tener todos los logs.

Modificacion 21/10/2003 : 
	En la funcion CargarOtrosConc se calculaba a la fecha hasta, si no tiene datos busca una ot
	en el periodo ingresado y se calcula a esa fecha (funcion InicPuestosVivos)

PARAMETROS: 
	R: Acumula en VARNOV
	M: No modifica en la tabla PARTE y no valida la liquicion (PARTE_LIQFAC)
    T: Modo test - Para que salga solo los mensajes de error

*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "observa.h"
#include "operac.h"
#include "comerc.h"
#include "comdef.h"
#include "billpro.h"
#include "comerc.sch"
#include "operac.sch"
#include "billpro.sch"
#include "bill.sch"
#include "pfacnew.fmh"
#include "pfacnew.rph"

#define MAXCLI  6000
#define MAXNOV  10000

#define _NORMAL		1
#define _ESPECIAL	2

#define _CLI_CENCOSUD 		3001
#define _HS_VIGIL_A 		  45

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

// Para los conceptos especiales (1031, etc..)
#define _PASO_NORMAL	1
#define _PASO_ESPECIAL	2

/* Estructuras */
struct cliente {
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
short UltimoConcepto (long cliente, short obj, short ptoser, short puesto, short tipoh, DATE fdesde, DATE fhasta, short noconc1, short noconc2, short paso);
void Grabar ();
void GrabarNovedad ();
void GrabarRNovedad ();
void CargarCliObj(short emp, long cliente, short objetivo, short condic, short ptoser, short puesto,
				  short hn, short h50, short h100, short h100f, short hsvad);
void AgruparPorNovedad ();
void CargarNovedad (long intern, short nrovar, long valor, short tipoh, short conc);
short ConcHijo(short padre);
bool ExisteConcepto (long cliente, short obj, short conc);
bool ObjetivoValido(long cliente, short objetivo);
void BorrarNovedad();
void CargarParte();
void CargarExcepcion();
void CargarRetro();
void CargarRetroExc();
void CargarOtrosConc();
private struct s_errores *CargarError(short codigo, long cliente, short objet, short conc, 
	short ptoser, char *extra, long liq, long eval);
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
void MostrarComentario();
bool EstaAsignado(short emp, long cliente, short objet, long legajo, DATE fecparte);
double AdicionalExcepciones();
double CantVigAdicionales(short emp, long cliente, short objet);
bool TipoHorasPorIncrementoSueldo();
void LimpiarVarNov(long cliente, short objetivo);
bool ObjetivoConErrores(long cliente, short objetivo);
DATE FechaPrimerOT(short emp, long cliente, short objet, DATE fecotini, DATE ffin);

/* Declaraciones globales */
form   fm0;
report rp0;
schema comerc, bill, operac, billpro;
char bufaux[50];
bool herror=FALSE, hmensaje=FALSE, modor=FALSE, modom=FALSE, modot=FALSE;
struct s_errores *p_error=NULL, *aux_error;
struct s_estcli  *p_estcli=NULL, *aux_estcli;
FILE *fp_error;
struct comentario pobs[MAXOBS], *uobs = pobs, *eobs;
char cstd[150], archF[50];
FILE *fp1 = NULL;
DATE fecexe, fcierre;
TIME horexe;

wcmd(pfacnew, 1.1 10/29/09)
{
	fm0    = OpenForm("pfacnew", FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	billpro   = OpenSchema("billpro",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT); 

	if(argc >1) {
		modor = str_eq (argv[1], "R");
		modom = str_eq (argv[1], "M") || str_eq (argv[1], "T");
		modot = str_eq (argv[1], "T");
	}

	if (modor)	FmSetFld(fm0, COMENT2, "MODOR");
	if (modom) 	FmSetFld(fm0, COMENT2, "MODSIST");
	if (modot)	FmSetFld(fm0, COMENT2, "TEST");

 	FmSetIFld(fm0, I_GRUPO, GrupoFact(GrupoUsr(GetUid())));
	
	if (DoForm(fm0, before, after) != FM_UPDATE)
		return;

	SetTableCache(comerc|OBJETIVO,  10);
	SetTableCache(comerc|ITMFAC,  	100);

	fcierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
	fecexe = Today();
	horexe = Hour();

	InicioListaTipoExcepcion();
	SwitchToSchema(operac);		

	sprintf(archF, "/tmp/pfacnew.%d.log", ProcPid());

	if ((fp_error = fopen (archF, "wt")) == (FILE *) NULL) Error ("No se pudo abrir el archivo de Log /tmp/pfacnew.log");
	fprintf(fp_error, "Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	fprintf(fp_error, "Liquidacion %ld Fecha desde %.3D hasta %.3D Cliente %ld %d hasta %ld %d considerar %s origen %s \n",
					FmLFld(fm0, NROLIQ), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), 
					FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmSFld (fm0, DTIPOH), FmSFld (fm0, DTIPHOR));

  	if ((fp1 = fopen("pfacnew.txt", "a+")) == NULL)
  		Error("No se pudo crear el archivo de log pfacnew.txt");

	ucli=pcli;
	CargarClientesValidos();
	// ListarClienteObjValido ();

	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'P'){
		CargarParte();
		CargarExcepcion();
	}

	if (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'R'){
		CargarRetro();
	}		

	AgruparPorNovedad ();
	ImprimirErrores();

	if (herror) {
		fprintf(fp_error, "Hubo errores Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		Error ("Hubo Errores - No se puede hacer el pase - Ver impresion ");
		fclose(fp_error);
		return;
	}

	// No se registro nada 
	if (unov == pnov) {
		fprintf(fp_error, "No hay datos para procesar Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		Error ("No hay datos para procesar");
		fclose(fp_error);
		return;
	}

	BeginTransaction();
	if ( (*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'P') &&
	      !TipoHorasPorIncrementoSueldo()){
		fprintf(fp_error, "Empieza a grabar LogParte Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		LogParte();
	}

	if ((*FmSFld(fm0, TIPHOR) == 'T' || *FmSFld(fm0, TIPHOR) == 'R') &&
	     !TipoHorasPorIncrementoSueldo()){
		fprintf(fp_error, "Empieza a grabar LogRetro Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		LogRetro();
	}

	if (!modor && !modot) {
		BorrarNovedad();
	}

	if (!modot){
		fprintf(fp_error, "Empieza a grabar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
		if (modor) {
			fprintf(fp_error, "MODO R \n");
			GrabarRNovedad();
		}
		else {
			fprintf(fp_error, "MODO NORMAL \n");
			GrabarNovedad();
		}
	}
	fprintf(fp_error, "Fin grabar novedad Fecha %.3D Hora %.3T Usr %s \n", Today(), Hour(), UserName(GetUid()));
	EndTransaction();
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

			if (FmChgFld(fm0)){
				FmSetLFld(fm0, CLID, 3001);
				FmSetLFld(fm0, CLIH, 999999999);
				FmSetIFld(fm0, OBJD, 0);
				FmSetIFld(fm0, OBJH, 99);
			}
		}
		break;
	case TIPCLI:
		if (FmChgFld(fm0)) {
			int pos;
			/* Agrego los nuevos clientes al final de lo que ya tenia */				
			for (pos=0; !FmIsNull (fm0, CLICON, pos); pos ++) {
				// FmClearFlds(fm0, CLICON, DTOBJCON, pos);
			}
			SetKey(billpro|RCLIESPbyTIPCLI, FmIFld(fm0, TIPCLI), MIN_LONG, MIN_SHORT);
			for (/* pos=0 */; GetRecord (billpro|RCLIESPbyTIPCLI, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK,1) != ERROR;pos ++) {
				FmSetLFld(fm0, CLICON, LFld(billpro|RCLIESP_CLIENTE), pos);
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

	cparte = CreateCursor(PARTEbyEMP, IO_NOT_LOCK);
	cobj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(cobj) != ERROR) {
		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		/*Cargo las horas del parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);

		//fprintf(stderr, "Seteo %.3D %.3D %ld %ld %d %d \n", FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), CliMin(), ObjMin(),  CliMax(), ObjMax());

		while (FetchCursor(cparte) != ERROR) {

			if (!ClienteObjValido(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)))
				continue;

			if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (pdia != DFld(PARTE_DIA)) {
				sprintf (bufaux, "Procesando Parte dia %.3D cli %ld obj %d ", DFld(PARTE_DIA), LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO));
				FmSetFld (fm0, COMENT, bufaux);
				FmShowFlds (fm0, COMENT, COMENT);
				WiRefresh();
				pdia = DFld(PARTE_DIA);
			}

			if (!modom && !TipoHorasPorIncrementoSueldo()){
				if (LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ) && *FmSFld (fm0, TIPOH) == 'T') {
					CargarError(_ERROR_LIQ, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), NULL_SHORT, NULL_SHORT, 
								NULL_STR, LFld(PARTE_LIQFAC), NULL_LONG);
					continue;			
				}
			}

			if (IFld(PARTE_CONFIR) == A_CONF && DFld(PARTE_DIA) > fcierre) {
				CargarError(_ERROR_ESTADO, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
						    NULL_LONG, NULL_LONG);
				continue;			
			}

			if (LFld(comerc|OBJETIVO_RESUMEN) == NULL_LONG) {
				CargarError(_ERROR_RESUMEN, LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
						    NULL_LONG, NULL_LONG);
				continue;			
			}

    	    /***
			//fprintf(stderr, "PARTE %ld %d %d %ld %.3D %d %d %d %d \n",
					LFld(PARTE_CLIENTE), 
					IFld(PARTE_OBJETIVO), 
					 IFld(PARTE_PTOSER), 
					 LFld(PARTE_NROLEG), 
					 DFld(PARTE_DIA),
					 IFld(PARTE_HSNOR), 
					 IFld(PARTE_HS50), 
					 IFld(PARTE_HS100F), 
					 IFld(PARTE_HS100FE));
        	***/

			CargarCliObj(FmIFld(fm0, EMP), 
					 LFld(PARTE_CLIENTE), 
					 IFld(PARTE_OBJETIVO), 
					 NULL_SHORT, 
					 IFld(PARTE_PTOSER), 
					 IFld(PARTE_PUESTO), 
					 IFld(PARTE_HSNOR), 
					 IFld(PARTE_HS50), 
					 IFld(PARTE_HS100F), 
					 IFld(PARTE_HS100FE),
					 0);
		}
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);
	FreeTable(PARTE);
}

void CargarExcepcion()
{
	dbcursor  cexc;
	short tipoexc;
	double hsvig=0;

	/*Cargo las horas de excepciones */
	cexc   = CreateCursor(EXCEPCIONbyEMP,   IO_NOT_LOCK);
	SetCursorFrom(cexc, FmIFld(fm0, EMP), CliMin(), ObjMin(), FmDFld(fm0, FECHAD),
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cexc, FmIFld(fm0, EMP), CliMax(), ObjMax(), FmDFld(fm0, FECHAH),
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cexc) != ERROR) {
		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if (!ClienteObjValido(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)))
			continue;

		tipoexc = ParteTipoExcepcion(IFld(EXCEPCION_CONDIC),IFld(EXCEPCION_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		SetKey(comerc|OBJETIVO, LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);
	
        if (TipoHorasPorIncrementoSueldo() || *FmSFld(fm0, TIPTAR) == 'T') {
        	hsvig = AdicionalExcepciones();
			if (hsvig > 0) {
				fprintf (fp1, "LEXCEPCION\tNroliq\t%ld\tFecha\t%.3D\tHora\t%.3T\tCliente\t%ld\t%s\tObjetivo\t%d\t%s\tNroleg\t%ld\t%s\tFecha\t%.3D\tadicionales\t%.2f\n",
						FmLFld(fm0, NROLIQ),
						fecexe, horexe,
						LFld(EXCEPCION_CLIENTE), 
						GetDescCli(LFld(EXCEPCION_CLIENTE)),
						IFld(EXCEPCION_OBJETIVO),
						GetObjDescrip(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)),
						LFld(EXCEPCION_NROLEG),
						GetNombreLeg(FmIFld(fm0, EMP), LFld(EXCEPCION_NROLEG)),
						DFld(EXCEPCION_DIA),
						(hsvig/100.0));
			}
        }

		CargarCliObj(FmIFld(fm0, EMP),
					LFld(EXCEPCION_CLIENTE), 
					IFld(EXCEPCION_OBJETIVO),
					IFld(EXCEPCION_CONDIC),
					IFld(EXCEPCION_PTOSER),
					IFld(EXCEPCION_PUESTO),
					IFld(EXCEPCION_HORAS), 
					IFld(EXCEPCION_HS50),
					FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(EXCEPCION_HS100),
					FeriadoNovia(DFld(EXCEPCION_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(EXCEPCION_HS100) : 0.0,
					(short)hsvig);

		sprintf (bufaux, "Procesando Excepciones de Cliente %ld Objetivo %d", LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO));
		FmSetFld (fm0, COMENT, bufaux);
		WiRefresh();
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cexc);
}

void CargarRetro()
{
	dbcursor  cretro;

	cretro = CreateCursor(RETRObyRDIA, IO_NOT_LOCK);

	/*Cargo las horas de retroactivos */
	InitRecord(RETRO); //Este InitRecord es para que limpie el buffer y estar seguro que se pare en la primer posicion
	SetCursorFrom(cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), CliMin(), ObjMin());
	SetCursorTo  (cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), CliMax(), ObjMax());
	MoveCursorFirst(cretro);

	while (FetchCursor(cretro) != ERROR) {
		//fprintf(stderr, "Cliente %ld Objetivo %d Dia %.3D leg %ld \n", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA), LFld(RETRO_NROLEG));

		if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if (!ClienteObjValido(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)))
			continue;

		if (!TipoHorasPorIncrementoSueldo()){
			if (LFld(RETRO_LIQFAC) != NULL_LONG && LFld(RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)  && *FmSFld (fm0, TIPOH) == 'T') {
				CargarError(_MENSAJE_RETRO, LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
							LFld(RETRO_LIQFAC), NULL_LONG);
				continue;			
			}
		}			

		if (IFld(RETRO_CONFIR) == A_CONF ){
			CargarError(_ERROR_ESTADO, LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), NULL_SHORT, NULL_SHORT, NULL_STR,
					    NULL_LONG, NULL_LONG);
			continue;			
		}

		CargarCliObj(FmIFld(fm0, EMP), 
					 LFld(RETRO_CLIENTE), 
					 IFld(RETRO_OBJETIVO), 
					 NULL_SHORT, 
					 IFld(RETRO_PTOSER), 
					 IFld(RETRO_PUESTO), 
					 IFld(RETRO_DHSNOR),
					 IFld(RETRO_DHS50),
					 IFld(RETRO_DHS100F),
					 IFld(RETRO_DHS100FE),
					 0);

		CargarRetroExc();

		sprintf (bufaux, "Procesando Retro dia %.3D", DFld(RETRO_DIA));
		FmSetFld (fm0, COMENT, bufaux);
		WiRefresh();
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
	FreeTable(RETRO);
}

void CargarRetroExc()
{
	short tipoexc;

	/*Cargo las horas de retro excepciones */
	SetKey(RETROEXCbyEMP, IFld(RETRO_EMP), LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA),
						LFld(RETRO_NROLEG), IFld(RETRO_PTOSER), IFld(RETRO_PUESTO), IFld(RETRO_NROINT),
						MIN_SHORT, MIN_SHORT);

	while (GetRecord(RETROEXCbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 8) != ERROR) {

		tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), IFld(RETROEXC_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		SetKey(comerc|OBJETIVO, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		CargarCliObj(FmIFld(fm0, EMP),
					LFld(RETROEXC_CLIENTE), 
					IFld(RETROEXC_OBJETIVO),
					IFld(RETROEXC_CONDIC),
					IFld(RETROEXC_PTOSER),
					IFld(RETROEXC_PUESTO),
					IFld(RETROEXC_DHORAS),
					IFld(RETROEXC_DHS50),   
					FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
					FeriadoNovia(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0,
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
	DATE fecha;

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
		//fprintf(stderr, "Cargo %ld %d puesto %d ptoser %d stdv %ld - stdt %ld - stda %ld - stde %ld \n", ucli->cli, ucli->obj,ucli->ptoser, ucli->puesto, ucli->stdv, ucli->stdt, ucli->stda, ucli->stde);
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

	//fprintf(stderr, "Cargofinal %ld %d ptoser %d puesto %d stdv %ld - stdt %ld - stda %ld - stde %ld \n", ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, ecli->stdv, ecli->stdt, ecli->stda, ecli->stde);
}

void AgruparPorNovedad ()
{
	long valor;
	short nrovar,conc;
	short preciomen=0, concnor=0;
	int paso;
	short ultnor1=NULL_SHORT, ultext1=NULL_SHORT, ultnor2=NULL_SHORT, ultext2=NULL_SHORT;
	bool tiene_pmensual = FALSE;

	unov = pnov;	

	for (ecli = pcli; ecli < ucli; ecli++) {
		ultnor1=NULL_SHORT;
		ultext1=NULL_SHORT;
		ultnor2=NULL_SHORT;
		ultext2=NULL_SHORT;
		tiene_pmensual = FALSE;

		SetLFld(comerc|OBJETIVO_CLIENTE, ecli->cli);
		SetIFld(comerc|OBJETIVO_OBJET,   ecli->obj);
		(void) GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	for (paso=_PASO_NORMAL; paso <= _PASO_ESPECIAL; paso ++){
		
		//fprintf(stderr, "Paso %d Vien cli %ld obj %d ptoser %d puesto %d total hs %ld stdv %ld - stdt %ld - stda %ld - stde %ld \n", paso, ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, (ecli->stdt - ecli->stda - ecli->stde), ecli->stdv, ecli->stdt, ecli->stda, ecli->stde );
		//fprintf(stderr, "ultnor1 %d  ultnor2 %d \n",  ultnor1, ultnor2);

		preciomen=0;

		/* Cargo las horas normales aunque no tenga que grabarlo tengo que buscar el concepto igual
		   por si lo necesito en las especiales */
		valor =  ecli->stdt - ecli->stda - ecli->stde;
		conc=UltimoConcepto (ecli->cli, ecli->obj, ecli->ptoser,ecli->puesto, _NORMAL, FmDFld (fm0, FECHAD),  FmDFld (fm0, FECHAH), ultnor1, ultnor2, paso);
		ultnor1 = conc;

		if (conc == NULL_SHORT) {
			//fprintf(stderr, "NO hay conceptos para paso %d \n", paso);
			continue;
		}

		//fprintf(stderr, "Encontro ultimo concepto ptoser %d puesto %d conc %d \n", ecli->ptoser,ecli->puesto, conc);

		if (paso == _PASO_ESPECIAL &&  tiene_pmensual) {
			//fprintf(stderr, "CARGO EL ESPECIAL MENSUAL %d %d \n", ultnor1, ultnor2);

			SetIFld(comerc|ITMFAC_ITEM, _TARIFA_ESP_MENSUAL);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
						   IFld(comerc|ITMFAC_CANT), //nrovar
						   100,
						   _NORMAL,
						   IFld(comerc|ITMFAC_ITEM));
		}

		/*Si el concepto es precio mensual tengo que generar dos renglones 
		  uno con el concepto PRECIOMEN y cantidad = 1 
		  otro con el prox. concpeto */

		if (conc == PRECIOMEN ) {
			tiene_pmensual = TRUE;
			SetIFld(comerc|ITMFAC_ITEM, conc);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);
			
			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
						   IFld(comerc|ITMFAC_CANT), //nrovar
						   100,
						   _NORMAL,
						   conc);

			//fprintf(stderr, "Grabo ptoser %d puesto %d conc %d \n", ecli->ptoser,ecli->puesto, conc);
			conc=UltimoConcepto (ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, _NORMAL, FmDFld (fm0, FECHAD),  FmDFld (fm0, FECHAH), ultnor1, ultnor2, paso);
			ultnor2 = conc;
			//fprintf(stderr, "Busco un segundo concepto %d \n", conc);
			preciomen=conc;
		}				   

		concnor=conc;
		SetIFld(comerc|ITMFAC_ITEM, conc);
		GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

		if (valor < 0) {
			CargarError (_MENSAJE_NC, ecli->cli, ecli->obj, conc, NULL_SHORT, NULL_STR, NULL_LONG, valor);
		}
		else {
			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
					   IFld(comerc|ITMFAC_CANT), //nrovar
					   valor,
					   _NORMAL,
					   conc);
		}

		/* Cargo las horas adicionales */
		valor = ecli->stda;
		
		if (!valor)
			continue;

		/* Si el concepto de las horas normales es PRECIOMEN mando todo al concepto especial del
		   de las horas normales
		*/
		if (preciomen) {
			conc=ConcHijo(preciomen);
			//fprintf(stderr, "Concepto especial mensual %d \n", conc);
			if (conc == NULL_SHORT) {
				CargarError(_ERROR_SUBCONC, ecli->cli, ecli->obj, preciomen, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
				continue;
			}
		}
		else {
			conc=UltimoConcepto (ecli->cli, ecli->obj, ecli->ptoser, ecli->puesto, _ESPECIAL, FmDFld (fm0, FECHAD),  FmDFld (fm0, FECHAH), ultext1, ultext2, paso);
			//fprintf(stderr, "Concepto especial %d \n", conc);
			ultext1 = conc;
		}

		/*Para cencosud concepto 45 van al mismo concepto las horas normales y las especiales */
		if (ecli->cli == _CLI_CENCOSUD && concnor == _HS_VIGIL_A )
			conc = concnor;

		/*No encontro el concepto para las horas especiales busco el concepto padre del mismo concepto de las normales */
		if (conc == NULL_SHORT) {
			conc =  ConcHijo(concnor);

			if (conc == NULL_SHORT){
				CargarError(_ERROR_SUBCONC, ecli->cli, ecli->obj, concnor, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
				continue;
			}

			if (!ExisteConcepto (ecli->cli, ecli->obj, conc)) {
				CargarError(_ERROR_HSADIC, ecli->cli, ecli->obj, conc, NULL_SHORT, NULL_STR, NULL_LONG, NULL_LONG);
				continue;
			}
		} 

		SetIFld(comerc|ITMFAC_ITEM, conc);
		GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);
		if (valor < 0) {
			CargarError (_MENSAJE_NC, ecli->cli, ecli->obj, conc, NULL_SHORT, NULL_STR, NULL_LONG, valor);
		}
		else {
			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
					   IFld(comerc|ITMFAC_CANT), //nrovar
					   valor,
					   _ESPECIAL, 
					   conc);
		}
    }
	}

	/* Cargo otros conceptos de facturacion */
	CargarOtrosConc();
}


void GrabarNovedad ()
{    
	long total=0;
		
	for (enov = pnov; enov < unov; enov++) {
		//fprintf(stderr, "GrabaNov %ld %d %ld \n", enov->intern, enov->nrovar, enov->valor);
	    
	    InitRecord (bill|VARNOV);
    	SetLFld(bill|VARNOV_NROLIQ, FmLFld (fm0, NROLIQ));
		SetLFld(bill|VARNOV_INTERN, enov->intern);
		SetIFld(bill|VARNOV_NROVAR, enov->nrovar);
		SetLFld(bill|VARNOV_VALOR,  enov->valor);
		
		total += enov->valor;	
		PutRecord(bill|VARNOV);
	}

 	//fprintf(stderr, "TOTAL %ld \n", total);
	
}

void GrabarRNovedad ()
{    
	long total=0;
		
	for (enov = pnov; enov < unov; enov++) {
		//fprintf(stderr, "GrabaNov %ld %d %ld \n", enov->intern, enov->nrovar, enov->valor);

		SetKey(bill|VARNOVbyNROLIQ, FmLFld (fm0, NROLIQ), enov->intern, enov->nrovar);
		if (GetRecord(bill|VARNOVbyNROLIQ, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			SetLFld(bill|VARNOV_VALOR,  0);
		}
    	SetLFld(bill|VARNOV_NROLIQ, FmLFld (fm0, NROLIQ));
		SetLFld(bill|VARNOV_INTERN, enov->intern);
		SetIFld(bill|VARNOV_NROVAR, enov->nrovar);
		SetLFld(bill|VARNOV_VALOR,  LFld(bill|VARNOV_VALOR) + enov->valor);

		total += enov->valor;	
		PutRecord(bill|VARNOV);
	}

 	//fprintf(stderr, "TOTAL %ld \n", total);

}

/*****************************************
Devuelve el ultimo concepto utilizado
******************************************/
short UltimoConcepto (long cliente, short obj, short ptoser, short puesto, short tipoh, DATE fdesde, DATE fhasta, short conc1, short conc2, short paso)
{
	static cur;
	DATE ultfecha=MIN_DATE, fecact=NULL_DATE;
	short ultconc=NULL_SHORT;
	short minpue, maxpue;
	if (!cur)
		cur = CreateCursor (comerc|ITMXPUEbyCLIENTE, IO_NOT_LOCK);

	minpue = MIN_SHORT;
	maxpue = MAX_SHORT;

	if (puesto != NULL_SHORT) {
		SetKey(operac|PUESTOSbyCLIENTE, cliente, obj, ptoser, puesto);
		if (GetRecord (operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			minpue = IFld(operac|PUESTOS_PUESTO);
			maxpue = IFld(operac|PUESTOS_PUESTO);

			SetCursorFrom (cur, cliente, obj, ptoser, minpue, TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL),
					SFld(operac|PUESTOS_DIA1), SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4), 
					SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7), 
					SFld(operac|PUESTOS_REGIM), MIN_SHORT);
			SetCursorTo   (cur, cliente, obj, ptoser, maxpue, TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL),
					SFld(operac|PUESTOS_DIA1), SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4), 
					SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7), 
					SFld(operac|PUESTOS_REGIM), MAX_SHORT);

			while (FetchCursor (cur) != ERROR) {

				//Busco el ultimo concepto usado
				//fprintf(stderr, "Viene conc itmxpue %d %.3D  \n", IFld (comerc|ITMXPUE_ITEM), ultfecha);

		//      Se saco este filtro por el precio mensual.
		//		if (!ConceptoPorHora(IFld (comerc|ITMXPUE_ITEM))) {
		//			continue;
		//		}

				if (paso == _PASO_NORMAL && ConcEsEspecial (IFld (comerc|ITMXPUE_ITEM))) {
					continue;
				}

				if (paso == _PASO_ESPECIAL && !ConcEsEspecial (IFld (comerc|ITMXPUE_ITEM))) {
					continue;
				}

				//Busco el ultimo concepto usado
				//fprintf(stderr, "Viene conc PASO1 %d %.3D  \n", IFld (comerc|ITMXPUE_ITEM), ultfecha);

				//Si tiene precio mensual lo mando como ultimo concepto
				if (IFld (comerc|ITMXPUE_ITEM) == PRECIOMEN)
					fecact = MAX_DATE;
				else
					fecact = DFld (comerc|ITMXPUE_CDATE);

				if (fecact > ultfecha) {
					//No tiene que ser conc			
					if (IFld (comerc|ITMXPUE_ITEM) == conc1)
						continue;

					if (IFld (comerc|ITMXPUE_ITEM) == conc2)
						continue;

					SetKey (comerc|ITMFACbyITEM, IFld (comerc|ITMXPUE_ITEM));
					(void) GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

					//Para las normales NO tiene que ser un subconcepto
					if (tipoh == _NORMAL && IFld (comerc|ITMFAC_SUBCONC))
							continue;

					//Para las especiales tiene que ser un subconcepto
					if (tipoh == _ESPECIAL && !IFld (comerc|ITMFAC_SUBCONC))
							continue;

					ultfecha = fecact;
					ultconc  = IFld (comerc|ITMXPUE_ITEM);
				}
			}
		} //fin lectura de puestos
	} // fin puestos != NULL

	//Si no encontro para ese puesto exacto
	if (ultconc == NULL_SHORT){
		SetCursorFrom (cur, cliente, obj, ptoser, minpue, NULL_TIME, NULL_TIME, LOW_VALUE, LOW_VALUE, LOW_VALUE, LOW_VALUE, LOW_VALUE, LOW_VALUE, LOW_VALUE, LOW_VALUE, NULL_SHORT);
		SetCursorTo   (cur, cliente, obj, ptoser, maxpue, MAX_TIME, MAX_TIME, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, HIGH_VALUE, MAX_SHORT);
		while (FetchCursor (cur) != ERROR) {

	//      Se saco este filtro por el precio mensual.
	//		if (!ConceptoPorHora(IFld (comerc|ITMXPUE_ITEM))) {
	//			continue;
	//		}

			if (paso == _PASO_NORMAL && ConcEsEspecial (IFld (comerc|ITMXPUE_ITEM))) {
				continue;
			}

			if (paso == _PASO_ESPECIAL && !ConcEsEspecial (IFld (comerc|ITMXPUE_ITEM))) {
				continue;
			}

			//Busco el ultimo concepto usado
			//fprintf(stderr, "Viene conc PASO2 %d %.3D  \n", IFld (comerc|ITMXPUE_ITEM), ultfecha);

			//Si tiene precio mensual lo mando como ultimo concepto
			if (IFld (comerc|ITMXPUE_ITEM) == PRECIOMEN)
				fecact = MAX_DATE;
			else
				fecact = DFld (comerc|ITMXPUE_CDATE);

			if (fecact > ultfecha) {
				//No tiene que ser conc			
				if (IFld (comerc|ITMXPUE_ITEM) == conc1)
					continue;

				if (IFld (comerc|ITMXPUE_ITEM) == conc2)
					continue;

				SetKey (comerc|ITMFACbyITEM, IFld (comerc|ITMXPUE_ITEM));
				(void) GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);

				//Para las normales NO tiene que ser un subconcepto
				if (tipoh == _NORMAL && IFld (comerc|ITMFAC_SUBCONC))
						continue;

				//Para las especiales tiene que ser un subconcepto
				if (tipoh == _ESPECIAL && !IFld (comerc|ITMFAC_SUBCONC))
						continue;

				ultfecha = fecact;
				ultconc  = IFld (comerc|ITMXPUE_ITEM);
			}
		}	
	}

	//Si no encontre para puesto-ptoser busco para ese puesto
	if (ultconc == NULL_SHORT && puesto != NULL_SHORT) {
		ultconc = UltimoConcepto (cliente, obj, ptoser, NULL_SHORT, tipoh, fdesde, fhasta, conc1, conc2, paso);
	}

	if (paso == _PASO_NORMAL && ultconc==NULL_SHORT && tipoh == _NORMAL) {
		CargarError (_ERROR_NOCONC, cliente, obj, NULL_SHORT, ptoser,
					tipoh == _NORMAL ? "Normales" : "Especiales", NULL_LONG, NULL_LONG);
		return ultconc;
	}

	//Valido si tiene precio cargado LD
	if (ultconc != NULL_SHORT) {
		SetIFld(comerc|ITMFAC_ITEM, ultconc);
		if (GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			SetKey(bill|VARFIJbyINTERN, LFld(comerc|OBJETIVO_INTERN), IFld(comerc|ITMFAC_PRECIO));
			if (GetRecord(bill|VARFIJbyINTERN, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				CargarError (_ERROR_NOPRECIO, cliente, obj, ultconc, ptoser,
						tipoh == _NORMAL ? "Normales" : "Especiales", NULL_LONG, NULL_LONG);
				return NULL_SHORT;
			}
		}
	}

	return ultconc;
}

void CargarNovedad (long intern, short nrovar, long valor, short tipoh, short conc)
{

	//fprintf(stderr, "GrabaNovedad Conc %d valor %ld \n", conc, valor);
	if (!valor)
		return;

	/*Valido que lo que quiera cargar conicida con lo que se pidio por form */
	if (tipoh == _NORMAL && *FmSFld (fm0, TIPOH) == 'E')
		return;

	if (tipoh == _ESPECIAL && *FmSFld (fm0, TIPOH) == 'N')
		return;

	if (*FmSFld(fm0, TIPTAR) == 'N') {
		if (ConcEsEspecial(conc)){
			return;
		}
	}

	if (TipoHorasPorIncrementoSueldo()) {
		if (!ConcEsEspecial(conc)){
			return;
		}
	}

	if (*FmSFld(fm0, TIPTAR) == 'H') {
		if (!ConcEsEspecial(conc) || !ConceptoPorHora(conc)){
			return;
		}
	}

	if (*FmSFld(fm0, TIPTAR) == 'V') {
		if (!ConcEsEspecial(conc) || ConceptoPorHora(conc)){
			return;
		}
	}

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

	if (conc != PRECIOMEN && conc != _TARIFA_ESP_MENSUAL ) {
		enov->valor += valor;
	}
}

short ConcHijo(short padre)
{
	dbcursor Itm = (dbcursor) ERROR;
	Itm = CreateCursor(comerc|ITMFAC, IO_NOT_LOCK);
	SetCursorFrom(Itm, MIN_SHORT);
	SetCursorTo  (Itm, MAX_SHORT);
	while (FetchCursor(Itm) != ERROR) {
		if (IFld(comerc|ITMFAC_CONCPAD) == padre) {
			return IFld(comerc|ITMFAC_ITEM);
		}
	}
	return NULL_SHORT;
}

bool ExisteConcepto (long cliente, short obj, short conc)
{
	static cur;

	if (!cur)
		cur = CreateCursor (comerc|TARIFAbyCLIENTE, IO_NOT_LOCK);

	SetCursorFrom (cur, FmIFld(fm0, EMP), cliente, obj, MIN_SHORT, NULL_LONG);
	SetCursorTo   (cur, FmIFld(fm0, EMP),cliente, obj, MAX_SHORT, MAX_LONG);
	while (FetchCursor (cur) != ERROR) {
		//Ttiene que ser conc			
		if (IFld (comerc|TARIFA_CONC) == conc)
			return TRUE;
	}
	return FALSE;
}

void BorrarNovedad()
{
	dbcursor curobj;

	curobj=CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom (curobj, CliMin(), ObjMin());
	SetCursorTo	  (curobj, CliMax(), ObjMax());

	 while (FetchCursor(curobj) != ERROR) {
		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

		SetKey(bill|VARNOVbyNROLIQ, FmLFld(fm0, NROLIQ), LFld(comerc|OBJETIVO_INTERN), MIN_SHORT);
		while(GetRecord(bill|VARNOVbyNROLIQ, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			DelRecord(bill|VARNOV);
		}
	}
	DeleteCursor(curobj);
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

void ImprimirErrores()
{
	char buferr[200];
	report rp0;
	
	rp0 = OpenReport("pfacnew", RP_EABORT);
	
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
	RpSetFld (rp0, RCONS, FmSFld(fm0, DTIPOH));
	RpSetFld (rp0, RORIGEN, FmSFld(fm0, DTIPHOR));

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
	struct s_errores *eq, *eqaux;

	for (eq = p_error ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
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
	struct s_estcli *eq, *eqaux;

	for (eq = p_estcli ; eq != NULL; ) {
		eqaux = eq;
		eq = eq->next;
		free(eqaux);
	}
	p_estcli = NULL;
}

void CargarClientesValidos()
{
	dbcursor curobj;
	bool estado_ok=TRUE, liq_ok=TRUE, tiene_parte=FALSE, bill_ok=TRUE;
	long liqmal;

	curobj=CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom (curobj, CliMin(), ObjMin());
	SetCursorTo	  (curobj, CliMax(), ObjMax());

	 while (FetchCursor(curobj) != ERROR) {
		if (!ObjetivoValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

		if (GetServicioObj(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)) == BRIGADA)
			continue;

		estado_ok = TRUE;
		liq_ok = TRUE;
		bill_ok = TRUE;
		tiene_parte=FALSE;
		//Busco en el parte si en los dias extremos tiene el cierre echo
		//no recorro todo porque tardaria mucho
		SetKey(operac|PARTEbyPUESTO, FmIFld(fm0, EMP),LFld(comerc|OBJETIVO_CLIENTE), 
				IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG);
		if (GetRecord (operac|PARTEbyPUESTO, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (estado_ok && IFld(PARTE_CONFIR) == A_CONF && DFld(PARTE_DIA) > fcierre) {
				estado_ok = FALSE;
			}

			if (!modom && !TipoHorasPorIncrementoSueldo()){
				if (liq_ok && LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ) && *FmSFld (fm0, TIPOH) == 'T'){
					liq_ok = FALSE;
					liqmal = LFld(PARTE_LIQFAC);
				}
			}				
			tiene_parte=TRUE;
		}

		SetKey(operac|PARTEbyPUESTO, FmIFld(fm0, EMP),LFld(comerc|OBJETIVO_CLIENTE), 
				IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);
		if (GetRecord (operac|PARTEbyPUESTO, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
			if (DFld(PARTE_DIA) >=  FmDFld(fm0, FECHAD)) {
				if (estado_ok && IFld(PARTE_CONFIR) == A_CONF && DFld(PARTE_DIA) > fcierre) {
					estado_ok = FALSE;
				}

				if (!modom && !TipoHorasPorIncrementoSueldo()){
					if (liq_ok &&  LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)  && *FmSFld (fm0, TIPOH) == 'T'){
						liq_ok = FALSE;
						liqmal = LFld(PARTE_LIQFAC);
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

void CargarOtrosConc()
{
	dbcursor  cobj;
	struct s_tarifa_lib etarifa;
    _SParam_PVivo parhora;
	double oval, ostd;
	bool facturo_ad_vig, tiene_tarifa;
	DATE new_fecha;

	sprintf (bufaux, "Cargando conceptos especiales");
	FmSetFld (fm0, COMENT, bufaux);
	FmShowFlds (fm0, COMENT, COMENT);
	WiRefresh();

	cobj = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(cobj, CliMin(), ObjMin());
	SetCursorTo  (cobj, CliMax(), ObjMax());
TRACE
	while (FetchCursor(cobj) != ERROR) {
		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

TRACE
		InicializoComentario(pobs, &uobs);

		/* Lo llamo aca porque lo tengo que llamar antes de InicPuestos vivos
		 porque esta funcion tambien llama a InicPuestos
		 sino no anda */

		if (TipoHorasPorIncrementoSueldo() || *FmSFld(fm0, TIPTAR) == 'T') {
			DATE fec_primer_ot;
TRACE
			/*Esta funcion funciona mal si la primer ot es en la mitad de periodo
			  por eso calculo primero la fecha de la primera OT */
			fec_primer_ot=FechaPrimerOT(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), 
											IFld(comerc|OBJETIVO_OBJET),
											FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH));
TRACE
			ostd = CalcularStd(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							fec_primer_ot, FmDFld(fm0, FECHAH),
							fec_primer_ot, FmDFld(fm0, FECHAH),
							FmDFld(fm0, FECHAH), 10,
							TRUE, pobs, &uobs);
		}
		else {
			ostd = 0;
		}

		InicPuestosVivos (FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
						NULL_SHORT, //Puestos
						NULL_SHORT, //Modelo OT
						FmDFld(fm0, FECHAH),
						FmDFld(fm0, FECHAH),
						TRUE, 	//Carga tarifario
						FALSE, 	//No Carga bonos
						_VALIDAR_FECINI,	//Considera la fecha de inicio de la ot
						NULL_SHORT, // Todo tipo de OT
						TRUE,
						FALSE,
						parhora);
TRACE
		////// Me fijo si tiene tarifa ////////
		tiene_tarifa=FALSE;
		while (!tiene_tarifa && ProximaTarifaViva (&etarifa)) {
			if (etarifa.horfac <= 0) {
				continue;
			}
			tiene_tarifa=TRUE;
		}
TRACE
		////// Me fijo si tiene tarifa ////////
		//Sino busco la ultima OT y saco a esa fecha.
		if (!tiene_tarifa) {
			new_fecha = FechaInicioUltimoOt(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), 
						IFld(comerc|OBJETIVO_OBJET),
						FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), TRUE);
TRACE
			//fprintf(stderr, "NEWFECHA obj %d new_fecha  %.3D \n", IFld(comerc|OBJETIVO_OBJET), new_fecha);
			if (new_fecha >= FmDFld(fm0, FECHAD) && new_fecha  <= FmDFld(fm0, FECHAH)) {
				InicPuestosVivos (FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
						NULL_SHORT, //Puestos
						NULL_SHORT, //Modelo OT
						new_fecha,
						new_fecha,
						TRUE, 	//Carga tarifario
						FALSE, 	//No Carga bonos
						_VALIDAR_FECINI,	//Considera la fecha de inicio de la ot
						NULL_SHORT, // Todo tipo de OT
						TRUE,
						FALSE,
						parhora);
TRACE
				/* Me fijo si tiene tarifa */
				while (!tiene_tarifa && ProximaTarifaViva (&etarifa)) {
					tiene_tarifa=TRUE;
				}
			}//fin if new_fecha
		}//fin !tiene_tarifa

		//Si no tiene tarifas paso al proximo objetivo
		if (!tiene_tarifa) {
			continue;
		}
TRACE
		facturo_ad_vig = FALSE;
		VolverInicioPuestosVivos ();
		while (ProximaTarifaViva (&etarifa)) {
TRACE
			//fprintf(stderr, "COncepto  %d \n" , etarifa.conc);
			if (etarifa.conc == _TARIFA_AD_VIG && etarifa.horfac > 0) {
				facturo_ad_vig = TRUE;
			} 
TRACE
			if (etarifa.conc == _TARIFA_AD_VIG && etarifa.horfac <= 0) {
				continue;
			} 
TRACE
			if (!facturo_ad_vig && etarifa.conc == _TARIFA_AD_ESP_VIG ) {
				continue;
			} 
TRACE
			if (etarifa.porhora) continue;
			if (!etarifa.tfactu) continue;
			if (!etarifa.consrif) continue;
			if (etarifa.conc == PRECIOMEN) continue;
			if (etarifa.conc == _TARIFA_ESP_MENSUAL) continue;

            if (!TipoHorasPorIncrementoSueldo() && *FmSFld(fm0, TIPTAR) != 'T' && 
            	ConcEsEspecial(etarifa.conc)){
            	continue;
            }

			if (*FmSFld(fm0, TIPTAR) == 'H') {
				if (!ConcEsEspecial(etarifa.conc) || !ConceptoPorHora(etarifa.conc)){
					continue;
				}
			}

			if (*FmSFld(fm0, TIPTAR) == 'V') {
				if (!ConcEsEspecial(etarifa.conc) || ConceptoPorHora(etarifa.conc)){
					continue;
				}
			}
TRACE
			oval = etarifa.horfac;

			if (etarifa.conc == _TARIFA_AD_VIG ) {
				oval = ostd;
				
				sprintf(cstd, "RESUMEN\t%ld\t%ld\t%s\tObjetivo\t%d\t%s\tVig.Standard del %.3D al %.3D\t%.2f", 
							FmLFld(fm0, NROLIQ),
							LFld(comerc|OBJETIVO_CLIENTE),
							GetDescCli(LFld(comerc|OBJETIVO_CLIENTE)),
							IFld(comerc|OBJETIVO_OBJET),
							GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)),
							FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), oval);
				CargarComentario(pobs, &uobs, 100, cstd);

				sprintf(cstd, "\nCliente:%ld %s Objetivo:%d %s Liq.: %ld    FINAL = Standard del %.3D al %.3D =  %.2f ", 
							LFld(comerc|OBJETIVO_CLIENTE), 
							GetDescCli(LFld(comerc|OBJETIVO_CLIENTE)),
							IFld(comerc|OBJETIVO_OBJET), 
							GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)),
							FmLFld(fm0, NROLIQ),
							FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), oval);
				CargarComentario(pobs, &uobs, 1, cstd);
				sprintf(cstd, "DETALLE %.3D %.3T ", fecexe, horexe);
 				CargarComentario(pobs, &uobs, 2, cstd);
				MostrarComentario();
				oval = Redondeo (oval * 100.0, 0);
			}
			if (etarifa.conc == _TARIFA_AD_ESP_VIG ) {
				oval = CantVigAdicionales(IFld(comerc|OBJETIVO_EMP), 
											LFld(comerc|OBJETIVO_CLIENTE), 
											IFld(comerc|OBJETIVO_OBJET));
			}
TRACE
			/*Si no es por hora va directamente la cantidad de la OT */
			SetIFld(comerc|ITMFAC_ITEM, etarifa.conc);
			GetRecord(comerc|ITMFACbyITEM, THIS_KEY, IO_NOT_LOCK);
TRACE
			CargarNovedad (LFld(comerc|OBJETIVO_INTERN),
					   IFld(comerc|ITMFAC_CANT), //nrovar
					   (long)oval, 
					   _NORMAL,
					   etarifa.conc);
TRACE
		}
		FinPuestosVivos();
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cobj);
}

void LogParte()
{
	dbcursor  cparte, cobj;
	DATE pdia=NULL_DATE;

	cparte = CreateCursor(PARTEbyEMP, IO_LOCK);
	cobj = CreateCursor(comerc|OBJETIVObyEMP, IO_NOT_LOCK);

	SetCursorFrom(cobj, FmIFld(fm0, EMP), CliMin(), ObjMin());
	SetCursorTo  (cobj, FmIFld(fm0, EMP), CliMax(), ObjMax());
	while (FetchCursor(cobj) != ERROR) {

		if (!ClienteObjValido(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
				continue;

		if (ObjetivoConErrores(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))) {
			//fprintf(stderr, "NO CAMBIO LOG de %ld %d \n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET));
			continue;
		}

		/*Cargo las horas del parte */
		SetCursorFrom(cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAD), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmDFld(fm0, FECHAH), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(cparte) != ERROR) {

			if (DFld(PARTE_DIA) < FmDFld(fm0, FECHAD) || DFld(PARTE_DIA) > FmDFld(fm0, FECHAH))
				continue;

			if (!ClienteObjValido(LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO)))
				continue;

			if (pdia != DFld(PARTE_DIA)) {
				sprintf (bufaux, "Grabando Log Parte dia %.3D cli %ld obj %d ", DFld(PARTE_DIA), LFld(PARTE_CLIENTE), IFld(PARTE_OBJETIVO));
				FmSetFld (fm0, COMENT, bufaux);
				FmShowFlds (fm0, COMENT, COMENT);
				WiRefresh();
				pdia = DFld(PARTE_DIA);
			}

			if (!modom && LFld(PARTE_LIQFAC) != NULL_LONG && LFld(PARTE_LIQFAC) != FmLFld(fm0, NROLIQ)  && *FmSFld (fm0, TIPOH) == 'T') {
				continue;
			}

			if (IFld(PARTE_CONFIR) == A_CONF && DFld(PARTE_DIA) > fcierre ) {
				continue;
			}

			if (!modom) {
				SetLFld(PARTE_LIQFAC, FmLFld(fm0, NROLIQ));
				PutRecord (PARTE);
			}
		}
	}

	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cparte);
	FreeTable(PARTE);
}

void LogRetro()
{
	dbcursor  cretro;

	cretro = CreateCursor(RETRObyRDIA, IO_LOCK);

	/*Cargo las horas de retroactivos */
	InitRecord(RETRO); //Este InitRecord es para que limpie el buffer y estar seguro que se pare en la primer posicion
	SetCursorFrom(cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAD), CliMin(), ObjMin());
	SetCursorTo  (cretro, FmIFld(fm0, EMP), FmDFld(fm0, FECHAH), CliMax(), ObjMax());
	MoveCursorFirst(cretro);

	while (FetchCursor(cretro) != ERROR) {
		//fprintf(stderr, "Cliente %ld Objetivo %d Dia %.3D leg %ld \n", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO), DFld(RETRO_DIA), LFld(RETRO_NROLEG));

		if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
			continue;

		if (!ClienteObjValido(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO)))
			continue;

		if (ObjetivoConErrores(LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO))){
			//fprintf(stderr, "NO CAMBIO LOG RETRO de %ld %d \n", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO));
			continue;
		}

		if (LFld(RETRO_LIQFAC) != NULL_LONG && LFld(RETRO_LIQFAC) != FmLFld(fm0, NROLIQ)  && *FmSFld (fm0, TIPOH) == 'T') {
			continue;			
		}

		if (IFld(RETRO_CONFIR) == A_CONF ){
			continue;
		}

		if (!modom) {
			SetLFld(RETRO_LIQFAC, FmLFld(fm0, NROLIQ));
			PutRecord (RETRO);
		}

		sprintf (bufaux, "Log Retro dia %.3D", DFld(RETRO_DIA));
		FmSetFld (fm0, COMENT, bufaux);
		WiRefresh();
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);
	FreeTable(RETRO);
}

void ListarClienteObjValido ()
{
	fprintf (stderr, "Lista de Cliente validos \n");
	for (aux_estcli = p_estcli; aux_estcli != NULL; aux_estcli = aux_estcli->next) {
		fprintf (stderr, "Cliente %ld obj %d \n", aux_estcli->cliok, aux_estcli->objok);
	}
}

void MostrarComentario()
{

	qsort((char *)pobs, (unsigned)(uobs - pobs), sizeof(pobs[0]), (IFPVCPVCP)compobs_lib);

	PrincipioComentario(pobs, uobs, &eobs);
	if (!HayComentario(pobs, uobs, &eobs)) {
		return;
	}

	do {
		fprintf (fp1, "%s \n", eobs->sobs);
	}
	while (ProximoComentario(pobs, uobs, &eobs));
}

bool EstaAsignado(short emp, long cliente, short objet, long legajo, DATE fecparte)
{
	schema old, operac;
	bool enasig=FALSE;

	old    = CurrentSchema();
	operac = OpenSchema("operac", IO_EABORT);

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

	if (IFld(EXCEPCION_CONDIC) != FACTURABLE ) {
		return hsadic;
	}

	estasig = EstaAsignado(FmIFld(fm0, EMP), LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
							 LFld(EXCEPCION_NROLEG), DFld(EXCEPCION_DIA));

	if (estasig) {
		/****
		fprintf (stderr, "ESTA ASIGNADO cli %ld obj %d leg %ld dia %.3D \n",
					LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
					LFld(EXCEPCION_NROLEG),  DFld(EXCEPCION_DIA));
		****/		
		return hsadic;					
	}

	SetKey(PARTEbyEMP, FmIFld(fm0, EMP), LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
							DFld(EXCEPCION_DIA), LFld(EXCEPCION_NROLEG),
							IFld(EXCEPCION_PTOSER),	IFld(EXCEPCION_PUESTO),IFld(EXCEPCION_NROINT));
	if (GetRecord(PARTEbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		hsparte = IFld(PARTE_HSNOR) +  IFld(PARTE_HS50) + IFld(PARTE_HS100F) + IFld(PARTE_HS100FE);
		hsexc   = IFld(EXCEPCION_HORAS) + IFld(EXCEPCION_HS50) + IFld(EXCEPCION_HS100);
		hsadic  = hsexc >= hsparte ? hsexc : 0;
		
		/**
		fprintf (stderr, "cli %ld obj %d leg %ld dia %.3D %.2f %.2f adicionales %.2f \n",
						LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO),
						LFld(EXCEPCION_NROLEG),  DFld(EXCEPCION_DIA),
						hsparte, hsexc, hsadic);
		***/						
	}		

	return hsadic;
}

double CantVigAdicionales(short emp, long cliente, short objet)
{
	double cantv=0, votal=0;

	for (ecli = pcli; ecli < ucli; ecli++) {
		if (ecli->cli == cliente && ecli->obj == objet) {
			//fprintf(stderr, "Suma %ld  \n",  ecli->stds);
			cantv += ecli->stds;
		}
	}
	votal = cantv / _HORAS_POR_MES_POR_VIG;

	fprintf(fp1, "RESUMEN\t%ld\t%ld\t%s\tObjetivo\t%d\t%s\tVig.Adicionales del %.3D al %.3D\t%.2f\n", 
							FmLFld(fm0, NROLIQ),
							cliente, 
							GetDescCli(cliente),
							objet, 
		 					GetObjDescrip(cliente, objet),
							FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), votal);

	fprintf(fp1, "TOTAL EXCEPCIONES: Cliente %ld %s Objetivo %d %s Nroliq %ld Fecha %.3D Hora %.3T --- Total Horas %.2f / %.2f  =  %.2f \n\n",
					cliente, 
					GetDescCli(cliente),
					objet, 
 					GetObjDescrip(cliente, objet),
					FmLFld(fm0, NROLIQ),
					fecexe, horexe,
					(cantv / 100.0), (double)_HORAS_POR_MES_POR_VIG / 100.0 , votal);


	votal = Redondeo(votal * 100.0,0);
	//fprintf(stderr, "Devuelve Total %.2f \n",  votal);
	return votal;
}

bool TipoHorasPorIncrementoSueldo()
{

	if (*FmSFld(fm0, TIPTAR) == 'S' ) {
		return TRUE;
	}

	if (*FmSFld(fm0, TIPTAR) == 'H' ) {
		return TRUE;
	}

	if (*FmSFld(fm0, TIPTAR) == 'V' ) {
		return TRUE;
	}

	return FALSE;
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

DATE FechaPrimerOT(short emp, long cliente, short objet, DATE fecotini, DATE ffin)
{
	/* 
	 Si la primer Ot tiene fecha menor a la del parametro , devuelvo la del parametro
	*/
	bool uimpre=FALSE, encontro=FALSE;
	DATE fecini;
	DATE prifec = MAX_DATE;
	schema comerc, old;

	old = CurrentSchema();
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

	if (uimpre) fprintf(stderr, "Viene OBJ %ld  %d fecha %.3D \n",cliente, objet, fecotini);

	SetIFld(comerc|OT_EMP, emp);
	SetLFld(comerc|OT_CLIENTE, cliente);
	SetIFld(comerc|OT_OBJET, objet);
	SetDFld(comerc|OT_FECREG, MIN_DATE);

	//Leo la O.T. anterior por fecha de registracion
	while (!encontro && GetRecord(comerc|OTbyFECREG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR ) {
		
		if (uimpre) fprintf(stderr, "Viene OT %ld \n", LFld(comerc|OT_NROOT));

		if (IFld(comerc|OT_TIPCOMP) != OTCOMERC) {
			continue;
		}

		if (IFld(comerc|OT_ESTOPER) != APROBADO || IFld(comerc|OT_ESTADM) != APROBADO ||
					IFld(comerc|OT_ESTVTA)  != APROBADO) {
			continue;
		}

  		if (uimpre) fprintf(stderr, "La OT %ld es valida \n", LFld(comerc|OT_NROOT));
		fecini = DFld(comerc|OT_FINICIO) == NULL_DATE ? DFld(comerc|OT_FFINAL) : DFld(comerc|OT_FINICIO);

		if (fecini < fecotini) {
			if (uimpre) fprintf(stderr, "La OT %ld fecini %.3D no sigo buscando \n", LFld(comerc|OT_NROOT), fecini);
			encontro = TRUE;
		}

		if (uimpre) fprintf(stderr, "Fecha de inicio = %.3D \n", fecini);

		if (fecini != NULL_DATE && fecini < prifec) {
			if (uimpre) fprintf(stderr, "Ahora la primer OT %ld %.3D \n", LFld(comerc|OT_NROOT), fecini);
			prifec = fecini;
		}
	}

	if (encontro) {
 		if (uimpre) fprintf(stderr, "Hay ot anteriores, tomo fecha del parametro \n");
		prifec = fecotini;
	}

	if (prifec > ffin) {
 		if (uimpre) fprintf(stderr, "La primer ot es posterior al periodo a procesar \n");
		prifec = fecotini;
	}

	if (uimpre) fprintf(stderr, "Devuelve %.3D \n", prifec);
	return prifec;
}

