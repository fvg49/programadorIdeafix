/************************************************************************************************
* MODULE & VERSION : @(#)lreten.c	1.11
* DATE             : 05/05/16 
* TIME             : 09:15:25 
*
* CREATED          : Gloria
*
* DESCRIPTION: Imprime la asignación de retenes. Si tienen parte cargado toma los valores de ahi
*              para horas de entrada, asistencia.
*
*************************************************************************************************/
#include <ideafix.h>
#include "lreten.fmh"
#include "lreten.rph"
#include "operac.h"
#include "comerc.h"
#include "brigada.h"
#include "sue.sch"
#include "operac.sch"
#include "brigada.sch"
#include "billpro.sch"
#include "comerc.sch"
#include "billpro.h"
#include "filial.h"

/* Estructura para almacenar y luego ordenar */
#define MAX_CLI	3000
#define MAX_RET 500

struct s_cliente {
	long 	nroint; // Guarda la posicion de retenciones que tiene los datos del legajo
	long    nroleg;
	long	cli;
	int		obj;
	DATE	fchdprov;
	DATE	fchhprov;
	TIME	horaent;
	TIME	horasal;
	char	diaslab1[2];
	char	diaslab2[2];
	char	diaslab3[2];
	char	diaslab4[2];
	char	diaslab5[2];
	char	diaslab6[2];
	char	diaslab7[2];
	char	condtrab[2];
	DATE	condfd;
	DATE	condfh;
	TIME	labd;
	TIME	labh;
}cliente[MAX_CLI];

struct s_retenes {
	long 	nroleg;
	char	apynom[55];
	char	regimen[15];
	DATE	franco;
	char	telefono[20];
	char	altapol1[2];
	char	altapol2[2];
	char	altapol3[2];
	char	altapol4[2];
	bool	clu;
	bool	portacion;
	int		hspta;
	char	condtrab[2];
} retenes[MAX_RET];

int subret = 0, ultcli=0;

/* Funciones Privadas */
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
void AbrirReporte();
void AbrirArchivo();
void GenerarAsig();
void GenerarAsigH();
void ImprimirReporte();
void ImprimirArchivo();
void ProcesoParte(long nroleg, DATE fecha, long cli, int objetivo);
private int retxalta (struct s_retenes *a, struct s_retenes *b);
private int retxleg  (struct s_retenes *a, struct s_retenes *b);
private int retxasist(struct s_retenes *a, struct s_retenes *b);
private int retxcliep(struct s_cliente *a, struct s_cliente *b);
private int retxcliee(struct s_cliente *a, struct s_cliente *b);
private int retxclieh(struct s_cliente *a, struct s_cliente *b);
void ImprimirReporteHoras();
void ImprimirArchivoHoras();

/* Declaraciones globales */
form   fm0;
report rp0;
schema sue, operac, brigada, billpro, comerc;
FILE *fp;

/* Programa principal */
wcmd(lreten, 1.11 05/16/05)
{
	fm_status cmd;

	comerc  = OpenSchema("comerc",  IO_EABORT);
	billpro = OpenSchema("billpro", IO_EABORT);
	sue     = OpenSchema("sue",	    IO_EABORT);
	brigada = OpenSchema("brigada", IO_EABORT);
	operac  = OpenSchema("operac",  IO_EABORT);

	fm0 = OpenForm("lreten", FM_EABORT);

	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
		switch (cmd) {
			case FM_UPDATE:
				GenerarAsig();
				GenerarAsigH();
				
				if (*FmSFld(fm0, SALIDA)=='A') {
					AbrirArchivo();
					if (*FmSFld (fm0, ORDEN) == 'H')
						ImprimirArchivoHoras();
					else		
						ImprimirArchivo();
					fclose(fp);
				}	
                else {
					AbrirReporte();
					if (*FmSFld (fm0, ORDEN) == 'H')
						ImprimirReporteHoras();
					else		
						ImprimirReporte();
					EndReport(rp0);
                }	

				return;				
				break;
			case FM_IGNORE:
				break;
		}
	
	FinObjetivosXusr();
	FinClientesXusr();
    FinListaXusr();
}

void GenerarAsig()
{
	dbcursor asigCur, asigTabCur;
	dbtable  asigTab;
	DATE franco;
	int  i;
	char diafec[5];

	asigTab    = CreateAlias(operac|ASIG);
	asigTabCur = CreateCursor(AlInd(asigTab, ASIGbyNROLEG), IO_NOT_LOCK);
	asigCur    = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);

	SetCursorFrom(asigCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (asigCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(asigCur) != ERROR) {
        //valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
		       	continue;
        
		if (DFld(operac|ASIG_FECASIG) > FmDFld(fm0, FECHA))
			continue;
		// Si es la opción del listado es DIARIO: entonces, tomo el día que es la fecha del listado
		// y lo tomo en cuenta unicamente si en el ASIG tengo algún dia? que sea igual
		if (*FmSFld(fm0, OPCION) == 'D') {
			// Busco el día que es la fecha del listado

			diafec [0] = dia(FmDFld(fm0, FECHA));
			strcpy(&diafec[1], NULL_STR);

			if (!SeTrabEnPuesto(diafec, SFld(operac|ASIG_DIA1), SFld(operac|ASIG_DIA2), SFld(operac|ASIG_DIA3),
					SFld(operac|ASIG_DIA4), SFld(operac|ASIG_DIA5), SFld(operac|ASIG_DIA6), SFld(operac|ASIG_DIA7)) &&
				!Franco(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), FmDFld(fm0, FECHA),
						SFld(operac|ASIG_VIGIL), IFld(operac|ASIG_NUMFRAN)))
				continue;
		}
		// Filtro : Rango de Retend, Inscripto en CLU, Portacion y Ausente (este en while de ASIG con Alias)
		//			Dejo para el momento de listar que este dado de alta en las provincias dentro del 
		//			rango por pantalla.

		if (!FmIsNull(fm0, RETEND) && (LFld(operac|ASIG_NROLEG) > FmLFld(fm0, RETENH) ||
									   LFld(operac|ASIG_NROLEG) < FmLFld(fm0, RETEND)))
			continue;
		// CLU y PORTACION

		SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
		GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (*FmSFld(fm0, INSCLU) == 'C' && !IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, INSCLU) == 'S' && IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'P' && !IFld(brigada|CVIGIPOL_PORTACION))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'S' && IFld(brigada|CVIGIPOL_PORTACION))
			continue;
		retenes[subret].clu       = IFld(brigada|CVIGIPOL_CLU);
		retenes[subret].portacion = IFld(brigada|CVIGIPOL_PORTACION);
		retenes[subret].nroleg    = LFld(operac|ASIG_NROLEG);

		// Obtengo el nombre y el telefono del legajo
		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
		GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);

		strncpy(retenes[subret].apynom,   SFld(sue|PER_APYNOM), 20);
		strcpy (retenes[subret].telefono, SFld(sue|PER_TELEF, 0));
		strcpy (retenes[subret].regimen,  SFld(operac|ASIG_REGIM));

		// Busco la proxima fecha Franco a partir de la pedida por pantalla

		franco = FmDFld(fm0, FECHA);
		// le agregue SFld(operac|ASIG_VIGIL) a la funcion Franco - Ingrid 12/15/1999

		while (!Franco(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), franco, SFld(operac|ASIG_VIGIL), IFld(operac|ASIG_NUMFRAN)) && !IsNull(operac|ASIG_FFRANCO)){
			franco++;
		}
		if(IsNull(operac|ASIG_FFRANCO))
			franco = NULL_DATE;

		retenes[subret].franco = franco;

		// TURNO // Cargo los dias laborales
		i = 0;
		// Cargo el horario de trabajo
//		retenes[subret].labd     = TFld(operac|ASIG_HSENT);
//		retenes[subret].labh     = TFld(operac|ASIG_HSSAL);

		// Busco donde esta dado de alta en la Policia
		i = 0;
		SetKey(brigada|VIGIPOLbyULTMOD, TRUE, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), NULL_SHORT, NULL_SHORT);
		while (GetRecord(brigada|VIGIPOLbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
//			if (!IFld(brigada|VIGIPOL_ACEPTADO))
//				continue;

			if(!UsrInGrupo(GRPBRIG, GetUid()))
				if (!InscVig(IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI),
							 IFld(brigada|VIGIPOL_ACEPTADO), DFld(brigada|VIGIPOL_FECHA), NULL))
					continue;

			if (!IFld(brigada|VIGIPOL_ACTIVO))
				continue;

			SetKey(billpro|PROVXDIVbyPORSUE, IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI));
			if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) != ERROR)
				switch (i) {
				case 0:
					strcpy(retenes[subret].altapol1, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 1:
					strcpy(retenes[subret].altapol2, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 2:
					strcpy(retenes[subret].altapol3, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 3:
					strcpy(retenes[subret].altapol4, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				}
			else {
				if (i < 4) {
					char provesp[5];
					SetIFld(sue|PROVI_PAIS,   IFld(brigada|VIGIPOL_CODPAIS));
					SetIFld(sue|PROVI_PROVIN, IFld(brigada|VIGIPOL_CODPROVI));
					if (GetRecord(sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
						if (IFld(sue|PROVI_PROVIN) == AERONAUTICA)
							strcpy(provesp, "A");
						else 
							strcpy(provesp, "P");
						switch (i) {
						case 0:
							strcpy(retenes[subret].altapol1, provesp);
							i++;
						break;
						case 1:
							strcpy(retenes[subret].altapol2, provesp);
							i++;
							break;
						case 2:
							strcpy(retenes[subret].altapol3, provesp);
							i++;
							break;
						case 3:
							strcpy(retenes[subret].altapol4, provesp);
							i++;
							break;
						}
					}
				}
			} 
			if (i == 4)
				break;
		}

		cliente[ultcli].condfd   = NULL_DATE;
		cliente[ultcli].condfh   = NULL_DATE;
		cliente[ultcli].horaent  = NULL_TIME;
		cliente[ultcli].horasal  = NULL_TIME;

		if (*FmSFld(fm0, PROVIS) != 'P') {
			// CARGO LA PLANTA.
			cliente[ultcli].nroint 	 = subret;
			cliente[ultcli].nroleg 	 = retenes[subret].nroleg;
			cliente[ultcli].cli      = LFld(operac|ASIG_CLIENTE);
			cliente[ultcli].obj      = IFld(operac|ASIG_OBJETIVO);
			cliente[ultcli].fchdprov = DFld(operac|ASIG_FECASIG);
			cliente[ultcli].fchhprov = DFld(operac|ASIG_FECHAS);

			// Cargo el horario de trabajo
			cliente[ultcli].labd = TFld(operac|ASIG_HSENT);
			cliente[ultcli].labh = TFld(operac|ASIG_HSSAL);

			strcpy(cliente[ultcli].diaslab1, SFld(operac|ASIG_DIA1));
			strcpy(cliente[ultcli].diaslab2, SFld(operac|ASIG_DIA2));
			strcpy(cliente[ultcli].diaslab3, SFld(operac|ASIG_DIA3));
			strcpy(cliente[ultcli].diaslab4, SFld(operac|ASIG_DIA4));
			strcpy(cliente[ultcli].diaslab5, SFld(operac|ASIG_DIA5));
			strcpy(cliente[ultcli].diaslab6, SFld(operac|ASIG_DIA6));
			strcpy(cliente[ultcli].diaslab7, SFld(operac|ASIG_DIA7));


			ProcesoParte(LFld(operac|ASIG_NROLEG), FmDFld(fm0, FECHA), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));

			strcpy(retenes[subret].condtrab, cliente[ultcli].condtrab);
			ultcli++;
			if (ultcli == MAX_CLI)
				Error ("Tabla interna saturada MAX_CLI tamaño %d", MAX_CLI);
		}
		if (*FmSFld(fm0, PROVIS) == 'E') {
			subret++;
			if (subret == MAX_RET)
				Error ("Tabla interna saturada MAX_RET tamaño %d", MAX_RET);
			continue;
		}
		// Proceso los ASIG de otros clientes que tenga el legajo corriente, con un alias sobre la
		// tabla ASIG.
		// index nroleg(emp, nroleg, cliente, objetivo);

		SetCursorFrom(asigTabCur, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
		SetCursorTo  (asigTabCur, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
		while (FetchCursor(asigTabCur) != ERROR) {
			 //valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
		       	continue;

			if (LFld(AlFld(asigTab, ASIG_CLIENTE)) == LFld(operac|ASIG_CLIENTE))
				continue;

			cliente[ultcli].nroleg 	 = retenes[subret].nroleg;
			cliente[ultcli].nroint   = subret;
			cliente[ultcli].cli      = LFld(AlFld(asigTab, ASIG_CLIENTE));
			cliente[ultcli].obj      = IFld(AlFld(asigTab, ASIG_OBJETIVO));
			cliente[ultcli].fchdprov = DFld(AlFld(asigTab, ASIG_FECASIG));
			cliente[ultcli].fchhprov = DFld(AlFld(asigTab, ASIG_FECHAS));
			cliente[ultcli].labd     = TFld(AlFld(asigTab, ASIG_HSENT));
			cliente[ultcli].labh     = TFld(AlFld(asigTab, ASIG_HSSAL));

			strcpy(cliente[ultcli].diaslab1, SFld(AlFld(asigTab, ASIG_DIA1)));
			strcpy(cliente[ultcli].diaslab2, SFld(AlFld(asigTab, ASIG_DIA2)));
			strcpy(cliente[ultcli].diaslab3, SFld(AlFld(asigTab, ASIG_DIA3)));
			strcpy(cliente[ultcli].diaslab4, SFld(AlFld(asigTab, ASIG_DIA4)));
			strcpy(cliente[ultcli].diaslab5, SFld(AlFld(asigTab, ASIG_DIA5)));
			strcpy(cliente[ultcli].diaslab6, SFld(AlFld(asigTab, ASIG_DIA6)));
			strcpy(cliente[ultcli].diaslab7, SFld(AlFld(asigTab, ASIG_DIA7)));

			ProcesoParte(LFld(operac|ASIG_NROLEG), FmDFld(fm0, FECHA), LFld(AlFld(asigTab, ASIG_CLIENTE)),
						 IFld(AlFld(asigTab, ASIG_OBJETIVO)));
			ultcli++;
			if (ultcli == MAX_CLI)
				Error ("Tabla interna saturada MAX_CLI tamaño %d", MAX_CLI);
		}
		subret++;
		if (subret == MAX_RET)
			Error ("Tabla interna saturada MAX_RET tamaño %d", MAX_RET);
	}
	DeleteCursor(asigTabCur);
	DeleteCursor(asigCur);
	DeleteAlias(asigTab);
}

void ProcesoParte(long nroleg, DATE fecha, long cli, int objetivo)
{
	DATE fd = NULL_DATE, fh = NULL_DATE;

	SetKey(PARTEbyEMPLE, FmIFld(fm0, EMP), nroleg, fecha, cli, objetivo);
 	if (GetRecord(PARTEbyEMPLE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		cliente[ultcli].nroleg 	= retenes[subret].nroleg;
		cliente[ultcli].nroint  = subret;
		cliente[ultcli].cli     = LFld(PARTE_CLIENTE);
		cliente[ultcli].cli     = LFld(PARTE_CLIENTE);
		cliente[ultcli].obj     = IFld(PARTE_OBJETIVO);
		cliente[ultcli].horaent = TFld(PARTE_HORAENT);
		cliente[ultcli].horasal = TFld(PARTE_HORASAL);

		strcpy(cliente[ultcli].condtrab, SFld(PARTE_CONDIC));

		fd = fh = NULL_DATE;
		// Si es condicion "Vacaciones" busco desde/hasta.
		if (*SFld(PARTE_CONDIC) == 'V') {
			SetKey(VACACbyEMP, FmIFld(fm0, EMP), nroleg, FmDFld(fm0, FECHA));
			if (GetRecord(VACACbyEMP, THIS_KEY|PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
	 			fd = DFld(VACAC_FDESDE);
				fh = DFld(VACAC_FHASTA);
			}
		}
	}
	cliente[ultcli].condfd = fd;
	cliente[ultcli].condfh = fh;
}

void AbrirReporte()
{
	rp0 = OpenReport("lreten", RP_EABORT|RP_NOBEGIN);

	//Si la salida es Impresora
	if ( *FmSFld(fm0, SALIDA) == 'I') {
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	}
	//Si la salida es Terminal
	if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	BeginReport(rp0, 1, NULL_STR);

	RpSetDFld(rp0, RFECHA,  FmDFld(fm0, FECHA));
	RpSetFld (rp0, ROPCION, FmSFld(fm0, DOPCION));
	RpSetLFld(rp0, RCLID,   FmLFld(fm0, CLID));
	RpSetLFld(rp0, RCLID,   FmLFld(fm0, CLID));
	RpSetIFld(rp0, ROBJD,   FmIFld(fm0, OBJD));
	RpSetFld (rp0, RDOBJD,  FmSFld(fm0, DOBJD));
	RpSetLFld(rp0, RCLIH,   FmLFld(fm0, CLIH));
	RpSetIFld(rp0, ROBJH,   FmIFld(fm0, OBJH));
	RpSetFld (rp0, RDOBJH,  FmSFld(fm0, DOBJH));
	RpSetLFld(rp0, RNROLEGD, FmLFld(fm0, RETEND));
	RpSetFld (rp0, RAPYNOMD, FmSFld(fm0, DRETEND));
	RpSetLFld(rp0, RNROLEGH, FmLFld(fm0, RETENH));
	RpSetFld (rp0, RAPYNOMH, FmSFld(fm0, DRETENH));
	RpSetFld (rp0, DPROVD,   FmSFld(fm0, DPROVD));
	RpSetFld (rp0, DPROVH,   FmSFld(fm0, DPROVH));
	RpSetFld (rp0, RASIG,    FmSFld(fm0, DPROVIS));
	RpSetFld (rp0, RASIST,   FmSFld(fm0, DCONDTRAB));
	RpSetFld (rp0, RRCLU, 	 FmSFld(fm0, DINSCLU));
	RpSetFld (rp0, RPORTAC,  FmSFld(fm0, DPORTAC));
	RpSetFld (rp0, RORDEN,   FmSFld(fm0, DORDEN));
}

void ImprimirReporte()
{
	/*Esta funcion recorre por la estructura retenes */

	int i, j;     
	bool entro;
	TIME horapta;
	char provd[5], provh[5];

	horapta = NULL_TIME;

	// Agregar el resto de los ordenamientos
	switch (*FmSFld(fm0, ORDEN)) {
	case 'L':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxleg);
		break;
	case 'P':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxalta);
		break;
	case 'A':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxasist);
		break;
	default:
		break;
	}

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVD));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provd, SFld(billpro|PROVXDIV_CODPROV));

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVH));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provh, SFld(billpro|PROVXDIV_CODPROV));

	entro= FALSE;


	// en el momento de imprimir tengo que mirar el rango de provincias de alta policial
	for (i = 0; i < subret; i++) {

		// Si ninguna provincia en la que esta dado de alta esta incluida en el rango pedido
		// no se toma en cuenta.
		if (!FmIsNull(fm0, PROVD)) {
			if ((strcmp(retenes[i].altapol1, provd) < 0 || strcmp(retenes[i].altapol1, provh) > 0) &&
				(strcmp(retenes[i].altapol2, provd) < 0 || strcmp(retenes[i].altapol2, provh) > 0) &&
				(strcmp(retenes[i].altapol3, provd) < 0 || strcmp(retenes[i].altapol3, provh) > 0) &&
				(strcmp(retenes[i].altapol4, provd) < 0 || strcmp(retenes[i].altapol4, provh) > 0))
				continue;
		}
		if (*FmSFld(fm0, CONDTRAB) != 'S' && strcmp(retenes[i].condtrab, FmSFld(fm0, CONDTRAB)) != 0)
			continue;

		if (entro) {
			DoReport(rp0, ZENTLIN);
		}

		RpClearZone(rp0, ZLINEA);

		RpSetLFld(rp0, RNROLEG,  retenes[i].nroleg);
		RpSetFld (rp0, RAPYNOM,  retenes[i].apynom);
		RpSetFld (rp0, RREGIMEN, retenes[i].regimen);
		RpSetDFld(rp0, RFRANCO,  retenes[i].franco);
		RpSetFld (rp0, RTELEF,   retenes[i].telefono);
		RpSetFld (rp0, RPOL1,    retenes[i].altapol1);
		RpSetFld (rp0, RPOL2,    retenes[i].altapol2);
		RpSetFld (rp0, RPOL3,    retenes[i].altapol3);
		RpSetFld (rp0, RPOL4,    retenes[i].altapol4);
		RpSetIFld(rp0, RPORT,    retenes[i].portacion);
		RpSetIFld(rp0, RCLU,     retenes[i].clu);

		switch (*FmSFld(fm0, ORDEN)) {
		case 'E': 
			qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxcliep);
			break;
		case 'C':
			qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxcliee);
			break;
		default :
			break;
		}

		entro = FALSE;		
		for (j = 0; j < ultcli; j++) {

			//Si el cliente no corresponde con el legajo
//			if (cliente[j].nroint != i)   NO PUEDO USAR LA POSICION PORQUE LOS ORDENE
//				continue; 

			
			if (cliente[j].nroleg != retenes[i].nroleg)
				continue; 

			if (entro)	
				RpClearZone(rp0, ZLINEA);

			entro = TRUE;
			RpSetLFld(rp0, RCLIPROV, cliente[j].cli);
			RpSetIFld(rp0, ROBJPROV, cliente[j].obj);
			RpSetTFld(rp0, RLABD,    cliente[j].labd);
			RpSetTFld(rp0, RLABH,    cliente[j].labh);
			RpSetDFld(rp0, RFCHD,    cliente[j].fchdprov);
			RpSetDFld(rp0, RFCHH,    cliente[j].fchhprov);
			RpSetFld (rp0, RCOND,    cliente[j].condtrab);
			RpSetDFld(rp0, RCONDD,   cliente[j].condfd);
			RpSetDFld(rp0, RCONDH,   cliente[j].condfh);
			RpSetFld (rp0, RDIA1,    cliente[j].diaslab1);
			RpSetFld (rp0, RDIA2,    cliente[j].diaslab2);
			RpSetFld (rp0, RDIA3,    cliente[j].diaslab3);
			RpSetFld (rp0, RDIA4,    cliente[j].diaslab4);
			RpSetFld (rp0, RDIA5,    cliente[j].diaslab5);
			RpSetFld (rp0, RDIA6,    cliente[j].diaslab6);
			RpSetFld (rp0, RDIA7,    cliente[j].diaslab7);
			RpSetTFld(rp0, RLLEGPTA, NULL_TIME);
			RpSetTFld(rp0, RSALPTA,  NULL_TIME);
			RpSetTFld(rp0, RLLEGOBJ, NULL_TIME);

			if (cliente[j].cli == FmLFld(fm0, CLIENTE)) {
				if (strcmp(cliente[j].condtrab, "T") == 0)
					RpSetIFld(rp0, RPTEPTA, TRUE);
				else
					RpSetIFld(rp0, RPTEPTA, FALSE);
				if (*FmSFld(fm0, OPCION) != 'D') {
					RpSetTFld(rp0, RLLEGPTA, cliente[j].horaent);
					RpSetTFld(rp0, RSALPTA,  cliente[j].horasal);
				}
				horapta = cliente[j].horaent;
			}
			else {
				if (*FmSFld(fm0, OPCION) != 'D')
					RpSetTFld(rp0, RLLEGOBJ, cliente[j].horaent);
			}
			
			DoReport(rp0, ZLINEA);


//			if (cliente[j+1].cli != 0 && cliente[j+1].nroint == i)
			if (cliente[j+1].cli != 0 && cliente[j+1].nroleg == retenes[i].nroleg)
				DoReport(rp0, ZENTLIN);

		}
	}
}

private int retxleg(struct s_retenes *a, struct s_retenes *b)
{
	return a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
		   0;
}

private int retxalta(struct s_retenes *a, struct s_retenes *b)
{
	return	strcmp(a->altapol1, b->altapol1) < 0 ? -1 : strcmp(a->altapol1, b->altapol1) > 0 ? 1 :
			strcmp(a->altapol2, b->altapol2) < 0 ? -1 : strcmp(a->altapol2, b->altapol2) > 0 ? 1 :
			strcmp(a->altapol3, b->altapol3) < 0 ? -1 : strcmp(a->altapol3, b->altapol3) > 0 ? 1 :
			strcmp(a->altapol4, b->altapol4) < 0 ? -1 : strcmp(a->altapol4, b->altapol4) > 0 ? 1 :
			0;
}

private int retxasist(struct s_retenes *a, struct s_retenes *b)
{
	return	strcmp(a->condtrab, b->condtrab) < 0 ? -1 : strcmp(a->condtrab, b->condtrab) > 0 ? 1 :
			0;
}

private int retxcliep(struct s_cliente *a, struct s_cliente *b)
{

	return  a->cli < b->cli ? -1 : a->cli > b->cli ? 1 :
			a->nroint > b->nroint ? -1 : a->nroint < b->nroint ? 1 :
			0;
}

private int retxclieh(struct s_cliente *a, struct s_cliente *b)
{
	// Ordena por hora de entrada
	return 	a->labd > b->labd ? 1 : a->labd < b->labd ? -1 :       
			a->nroleg < b->nroleg ? -1 : a->nroleg > b->nroleg ? 1 :
			a->cli < b->cli ? -1 : a->cli > b->cli ? 1 :
			0;
}

private int retxcliee(struct s_cliente *a, struct s_cliente *b)
{
	return  a->cli > b->cli ? -1 : a->cli < b->cli ? 1 :
			a->nroint > b->nroint ? -1 : a->nroint < b->nroint ? 1 :
			0;
}

void ImprimirReporteHoras()
{

	/*Esta funcion recorre por la estructura clientes */
	int i, j;     
	TIME horapta;
	char provd[5], provh[5];

	horapta = NULL_TIME;

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVD));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provd, SFld(billpro|PROVXDIV_CODPROV));

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVH));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provh, SFld(billpro|PROVXDIV_CODPROV));

	qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxclieh);

	for (j = 0; j < ultcli; j++) {

		//Leo los datos del legajo
		i=cliente[j].nroint;
		
		// Si ninguna provincia en la que esta dado de alta esta incluida en el rango pedido
		// no se toma en cuenta.
		if (!FmIsNull(fm0, PROVD)) {
			if ((strcmp(retenes[i].altapol1, provd) < 0 || strcmp(retenes[i].altapol1, provh) > 0) &&
				(strcmp(retenes[i].altapol2, provd) < 0 || strcmp(retenes[i].altapol2, provh) > 0) &&
				(strcmp(retenes[i].altapol3, provd) < 0 || strcmp(retenes[i].altapol3, provh) > 0) &&
				(strcmp(retenes[i].altapol4, provd) < 0 || strcmp(retenes[i].altapol4, provh) > 0))
				continue;
		}                   
		
		if (*FmSFld(fm0, CONDTRAB) != 'S' && strcmp(retenes[i].condtrab, FmSFld(fm0, CONDTRAB)) != 0)
			continue;

		RpClearZone(rp0, ZLINEA);

		if ((j==0) || (j > 0 && cliente[j].nroint != cliente[j-1].nroint)) {
			RpSetLFld(rp0, RNROLEG,  retenes[i].nroleg);
			RpSetFld (rp0, RAPYNOM,  retenes[i].apynom);
			RpSetFld (rp0, RREGIMEN, retenes[i].regimen);
			RpSetDFld(rp0, RFRANCO,  retenes[i].franco);
			RpSetFld (rp0, RTELEF,   retenes[i].telefono);
			RpSetFld (rp0, RPOL1,    retenes[i].altapol1);
			RpSetFld (rp0, RPOL2,    retenes[i].altapol2);
			RpSetFld (rp0, RPOL3,    retenes[i].altapol3);
			RpSetFld (rp0, RPOL4,    retenes[i].altapol4);
			RpSetIFld(rp0, RPORT,    retenes[i].portacion);
			RpSetIFld(rp0, RCLU,     retenes[i].clu);
    	}
    	
		RpSetFld (rp0, RDIA1,    cliente[j].diaslab1);
		RpSetFld (rp0, RDIA2,    cliente[j].diaslab2);
		RpSetFld (rp0, RDIA3,    cliente[j].diaslab3);
		RpSetFld (rp0, RDIA4,    cliente[j].diaslab4);
		RpSetFld (rp0, RDIA5,    cliente[j].diaslab5);
		RpSetFld (rp0, RDIA6,    cliente[j].diaslab6);
		RpSetFld (rp0, RDIA7,    cliente[j].diaslab7);
		RpSetTFld(rp0, RLABD,    cliente[j].labd);
		RpSetTFld(rp0, RLABH,    cliente[j].labh);
		RpSetLFld(rp0, RCLIPROV, cliente[j].cli);
		RpSetIFld(rp0, ROBJPROV, cliente[j].obj);
		RpSetDFld(rp0, RFCHD,    cliente[j].fchdprov);
		RpSetDFld(rp0, RFCHH,    cliente[j].fchhprov);
		RpSetFld (rp0, RCOND,    cliente[j].condtrab);
		RpSetDFld(rp0, RCONDD,   cliente[j].condfd);
		RpSetDFld(rp0, RCONDH,   cliente[j].condfh);
		RpSetTFld(rp0, RLLEGPTA, NULL_TIME);
		RpSetTFld(rp0, RSALPTA,  NULL_TIME);
		RpSetTFld(rp0, RLLEGOBJ, NULL_TIME);

		if (cliente[j].cli == FmLFld(fm0, CLIENTE)) {
				if (strcmp(cliente[j].condtrab, "T") == 0)
					RpSetIFld(rp0, RPTEPTA, TRUE);
				else
					RpSetIFld(rp0, RPTEPTA, FALSE);
				if (*FmSFld(fm0, OPCION) != 'D') {
					RpSetTFld(rp0, RLLEGPTA, cliente[j].horaent);
					RpSetTFld(rp0, RSALPTA,  cliente[j].horasal);
				}
				horapta = cliente[j].horaent;
		}
		else {
				if (*FmSFld(fm0, OPCION) != 'D')
					RpSetTFld(rp0, RLLEGOBJ, cliente[j].horaent);
		}
			
		DoReport(rp0, ZLINEA);
		DoReport(rp0, ZENTLIN);

	}
}

void GenerarAsigH()
{
	dbcursor asighCur, asigTabCur;
	dbtable  asighTab;
	DATE franco;
	int  i, pos;
	char	diafec[5];
	bool existe=FALSE;

	asighTab    = CreateAlias(operac|ASIGH);
	asigTabCur = CreateCursor(AlInd(asighTab, ASIGHbyNROLEG), IO_NOT_LOCK);
	asighCur   = CreateCursor(operac|ASIGHbyPUESTO, IO_NOT_LOCK);


	if (FmIsNull(fm0, RETEND)) {
		SetCursorFrom(asighCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (asighCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		SetCursorFrom(asighCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), FmLFld(fm0, RETEND), MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (asighCur, FmIFld(fm0, EMP), FmLFld(fm0, CLIENTE), FmIFld(fm0, OBJET), FmLFld(fm0, RETENH), MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while (FetchCursor(asighCur) != ERROR) {
		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
			continue;

		if (!(DFld(operac|ASIGH_FECALT) <= FmDFld(fm0, FECHA) && DFld(operac|ASIGH_FECBAJ) >= FmDFld(fm0, FECHA)))
			continue;

		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
		   	continue;

		for (pos = 0; pos < subret; pos++) {
			if (retenes[pos].nroleg == LFld(operac|ASIGH_NROLEG)) {
				existe = TRUE;
				break;
			}
		} 

		// Si es la opción del listado es DIARIO: entonces, tomo el día que es la fecha del listado
		// y lo tomo en cuenta unicamente si en el ASIGH tengo algún dia? que sea igual
		if (*FmSFld(fm0, OPCION) == 'D') {
			// Busco el día que es la fecha del listado
			diafec [0] = dia(FmDFld(fm0, FECHA));
			strcpy(&diafec[1], NULL_STR);

			if (!SeTrabEnPuesto(diafec, SFld(operac|ASIGH_DIA1), SFld(operac|ASIGH_DIA2), SFld(operac|ASIGH_DIA3),
					SFld(operac|ASIGH_DIA4), SFld(operac|ASIGH_DIA5), SFld(operac|ASIGH_DIA6), SFld(operac|ASIGH_DIA7)) &&
				!Franco(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG), FmDFld(fm0, FECHA),
						SFld(operac|ASIGH_VIGIL), IFld(operac|ASIGH_NUMFRAN))) {
				continue;
			}
		}

		SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
		GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (*FmSFld(fm0, INSCLU) == 'C' && !IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, INSCLU) == 'S' && IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'P' && !IFld(brigada|CVIGIPOL_PORTACION))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'S' && IFld(brigada|CVIGIPOL_PORTACION))
			continue;
		retenes[subret].clu = IFld(brigada|CVIGIPOL_CLU);
		retenes[subret].portacion = IFld(brigada|CVIGIPOL_PORTACION);

		retenes[subret].nroleg = LFld(operac|ASIGH_NROLEG);

		// Obtengo el nombre y el telefono del legajo
		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
		GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);

		strncpy(retenes[subret].apynom,   SFld(sue|PER_APYNOM), 20);
		strcpy (retenes[subret].telefono, SFld(sue|PER_TELEF, 0));

		strcpy(retenes[subret].regimen, SFld(operac|ASIGH_REGIM));

		// Busco la proxima fecha Franco a partir de la pedida por pantalla

		franco = FmDFld(fm0, FECHA);

		while (!Franco(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG), franco, SFld(operac|ASIGH_VIGIL), IFld(operac|ASIGH_NUMFRAN)) && !IsNull(operac|ASIGH_FFRANCO)){
			franco++;
		}
		if(IsNull(operac|ASIGH_FFRANCO))
			franco = NULL_DATE;

		retenes[subret].franco = franco;

		// TURNO // Cargo los dias laborales
		i = 0;
		// Cargo el horario de trabajo
//		retenes[subret].labd     = TFld(operac|ASIG_HSENT);
//		retenes[subret].labh     = TFld(operac|ASIG_HSSAL);

		// Busco donde esta dado de alta en la Policia
		i = 0;
		SetKey(brigada|VIGIPOLbyULTMOD, TRUE, FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG), NULL_SHORT, NULL_SHORT);
		while (GetRecord(brigada|VIGIPOLbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
//			if (!IFld(brigada|VIGIPOL_ACEPTADO))
//				continue;

			if (!IFld(brigada|VIGIPOL_ACTIVO))
				continue;

			SetKey(billpro|PROVXDIVbyPORSUE, IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI));
			if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) != ERROR)
				switch (i) {
				case 0:
					strcpy(retenes[subret].altapol1, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 1:
					strcpy(retenes[subret].altapol2, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 2:
					strcpy(retenes[subret].altapol3, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				case 3:
					strcpy(retenes[subret].altapol4, SFld(billpro|PROVXDIV_CODPROV));
					i++;
					break;
				}
			else {
				if (i < 4) {
					char provesp[5];
					SetIFld(sue|PROVI_PAIS,   IFld(brigada|VIGIPOL_CODPAIS));
					SetIFld(sue|PROVI_PROVIN, IFld(brigada|VIGIPOL_CODPROVI));
					if (GetRecord(sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
						if (IFld(sue|PROVI_PROVIN) == AERONAUTICA)
							strcpy(provesp, "A");
						else 
							strcpy(provesp, "P");
						switch (i) {
						case 0:
							strcpy(retenes[subret].altapol1, provesp);
							i++;
						break;
						case 1:
							strcpy(retenes[subret].altapol2, provesp);
							i++;
							break;
						case 2:
							strcpy(retenes[subret].altapol3, provesp);
							i++;
							break;
						case 3:
							strcpy(retenes[subret].altapol4, provesp);
							i++;
							break;
						}
					}
				}
			} 
			if (i == 4)
				break;
		}

		cliente[ultcli].condfd   = NULL_DATE;
		cliente[ultcli].condfh   = NULL_DATE;
		cliente[ultcli].horaent  = NULL_TIME;
		cliente[ultcli].horasal  = NULL_TIME;

		if (*FmSFld(fm0, PROVIS) != 'P') {
			// CARGO LA PLANTA.
			cliente[ultcli].nroint 	 = subret;
			cliente[ultcli].nroleg 	 = retenes[subret].nroleg;
			cliente[ultcli].cli      = LFld(operac|ASIGH_CLIENTE);
			cliente[ultcli].obj      = IFld(operac|ASIGH_OBJETIVO);
			cliente[ultcli].fchdprov = DFld(operac|ASIGH_FECALT);
			cliente[ultcli].fchhprov = DFld(operac|ASIGH_FECHAS);

			// Cargo el horario de trabajo
			cliente[ultcli].labd = TFld(operac|ASIGH_HSENT);
			cliente[ultcli].labh = TFld(operac|ASIGH_HSSAL);

			strcpy(cliente[ultcli].diaslab1, SFld(operac|ASIGH_DIA1));
			strcpy(cliente[ultcli].diaslab2, SFld(operac|ASIGH_DIA2));
			strcpy(cliente[ultcli].diaslab3, SFld(operac|ASIGH_DIA3));
			strcpy(cliente[ultcli].diaslab4, SFld(operac|ASIGH_DIA4));
			strcpy(cliente[ultcli].diaslab5, SFld(operac|ASIGH_DIA5));
			strcpy(cliente[ultcli].diaslab6, SFld(operac|ASIGH_DIA6));
			strcpy(cliente[ultcli].diaslab7, SFld(operac|ASIGH_DIA7));

			ProcesoParte(LFld(operac|ASIGH_NROLEG), FmDFld(fm0, FECHA), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));

			strcpy(retenes[subret].condtrab, cliente[ultcli].condtrab);
			ultcli++;
			if (ultcli == MAX_CLI)
				Error ("Tabla interna saturada MAX_CLI tamaño %d", MAX_CLI);
		}

		if (*FmSFld(fm0, PROVIS) == 'E') {
			if (!existe) {
				subret++;

				if (subret == MAX_RET)
					Error ("Tabla interna saturada MAX_RET tamaño %d", MAX_RET);

				continue;
			}
		}


		// Proceso de los ASIGH
		// index nroleg(emp, nroleg, cliente, objetivo),
		SetCursorFrom(asigTabCur, FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (asigTabCur, FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(asigTabCur) != ERROR) {
			//valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
			   	continue;

			if (IFld(AlFld(asighTab, ASIGH_MOTIVO)) == ALTAPARTE ||
				IFld(AlFld(asighTab, ASIGH_MOTIVO)) == DESXERROR)
				continue;

//			if (DFld(AlFld(asighTab, ASIGH_FECALT)) < FmDFld(fm0, FECHA) || 
//				DFld(AlFld(asighTab, ASIGH_FECBAJ)) > FmDFld(fm0, FECHA))
//				continue;

			if (!(DFld(AlFld(asighTab, ASIGH_FECALT)) <= FmDFld(fm0, FECHA) &&
				  DFld(AlFld(asighTab, ASIGH_FECBAJ)) >= FmDFld(fm0, FECHA)))
	  			continue;



			if (LFld(AlFld(asighTab, ASIGH_CLIENTE)) == LFld(operac|ASIGH_CLIENTE))
				continue;

			cliente[ultcli].nroleg 	 = retenes[subret].nroleg;
			cliente[ultcli].nroint   = subret;
			cliente[ultcli].cli      = LFld(operac|ASIGH_CLIENTE);
			cliente[ultcli].obj      = IFld(operac|ASIGH_OBJETIVO);
			cliente[ultcli].fchdprov = DFld(operac|ASIGH_FECALT);
			cliente[ultcli].fchhprov = DFld(operac|ASIGH_FECBAJ);
			cliente[ultcli].labd     = TFld(operac|ASIGH_HSENT);
			cliente[ultcli].labh     = TFld(operac|ASIGH_HSSAL);

			strcpy(cliente[ultcli].diaslab1, SFld(operac|ASIGH_DIA1));
			strcpy(cliente[ultcli].diaslab2, SFld(operac|ASIGH_DIA2));
			strcpy(cliente[ultcli].diaslab3, SFld(operac|ASIGH_DIA3));
			strcpy(cliente[ultcli].diaslab4, SFld(operac|ASIGH_DIA4));
			strcpy(cliente[ultcli].diaslab5, SFld(operac|ASIGH_DIA5));
			strcpy(cliente[ultcli].diaslab6, SFld(operac|ASIGH_DIA6));
			strcpy(cliente[ultcli].diaslab7, SFld(operac|ASIGH_DIA7));

			ProcesoParte(LFld(operac|ASIGH_NROLEG), FmDFld(fm0, FECHA), LFld(ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
			ultcli++ ;
			if (ultcli == MAX_CLI)
				Error ("Tabla interna saturada MAX_CLI tamaño %d", MAX_CLI);
		}

		if (!existe) {
			subret++;
			if (subret == MAX_RET)
				Error ("Tabla interna saturada MAX_RET tamaño %d", MAX_RET);
		}
	}

	DeleteCursor(asigTabCur);
	DeleteCursor(asighCur);
	DeleteAlias(asighTab);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
    case CLIENTE:
	   	InicClientesXusr();
    	break;
    case OBJET:
	   	InicObjetivosXusr(FmLFld(fm, CLIENTE, row), FmIFld(fm, EMP, row));
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
	case CLIENTE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);

		if (ValidaClienteXusr(FmLFld(fm, CLIENTE, row)))
		  	FmSetFld(fm, DCLIE, GetDescCliente(FmLFld(fm, CLIENTE, row)), row);
		else {
			Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIENTE, row));
			FmSetLFld(fm, CLIENTE, NULL_LONG, row);
 			FmSetFld(fm, DCLIE, NULL_STR, row);
			return FM_REDO;
  		}	
    break;
    case OBJET:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIENTE, row));

		if (ValidaObjetivoXusr(FmLFld(fm, CLIENTE, row), FmIFld(fm, OBJET, row), FmIFld(fm, EMP, row)))
			FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIENTE, row), FmIFld(fm, OBJET, row)), row);
		else	{
			Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIENTE, row), FmIFld(fm, OBJET, row));
			FmSetIFld(fm, OBJET, NULL_SHORT, row);
			FmSetFld(fm, DOBJET, NULL_STR, row);
			return FM_REDO;
    	}
	break;
	case CLID:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
    break;
    case CLIH:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
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
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row), FmIFld(fm, OBJH, row)), row);
	break;
	case SALIDA:
		if (*FmSFld(fm0, SALIDA)=='A') 
			FmSetFld(fm0, NOMARCH, "lreten.txt");
		else
			FmSetFld(fm0, NOMARCH, NULL_STR);
    break;
    }
	return FM_OK;				
}	

void AbrirArchivo()
{
	//La salida es Archivo

    if ((fp = fopen(FmSFld(fm0, NOMARCH),"w")) == (FILE*)NULL)
		Error("No se Pudo abrir el Archivo %s", FmSFld(fm0, NOMARCH));

	
	fprintf(fp, "Legajo\tNombre\tRegimen\tFranco\tTeléfono\tAlt.Pol\tPort\tCLU\tTurno\tAsignado en\tAusente\tPre\tLleg Pta\tSal Pta\tLLeg.Obj\n");
}

void ImprimirArchivo()
{
	/*Esta funcion recorre por la estructura retenes */

	int i, j;     
	bool entro;
	TIME horapta;
	char provd[5], provh[5];

	horapta = NULL_TIME;

	// Agregar el resto de los ordenamientos
	switch (*FmSFld(fm0, ORDEN)) {
	case 'L':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxleg);
		break;
	case 'P':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxalta);
		break;
	case 'A':
		qsort((char *)retenes, (unsigned)(subret), sizeof(retenes[0]), (IFPVCPVCP)retxasist);
		break;
	default:
		break;
	}

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVD));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provd, SFld(billpro|PROVXDIV_CODPROV));

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVH));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provh, SFld(billpro|PROVXDIV_CODPROV));

	entro = FALSE;


	// en el momento de imprimir tengo que mirar el rango de provincias de alta policial
	for (i = 0; i < subret; i++) {

		// Si ninguna provincia en la que esta dado de alta esta incluida en el rango pedido
		// no se toma en cuenta.
		if (!FmIsNull(fm0, PROVD)) {
			if ((strcmp(retenes[i].altapol1, provd) < 0 || strcmp(retenes[i].altapol1, provh) > 0) &&
				(strcmp(retenes[i].altapol2, provd) < 0 || strcmp(retenes[i].altapol2, provh) > 0) &&
				(strcmp(retenes[i].altapol3, provd) < 0 || strcmp(retenes[i].altapol3, provh) > 0) &&
				(strcmp(retenes[i].altapol4, provd) < 0 || strcmp(retenes[i].altapol4, provh) > 0))
				continue;
		}
		if (*FmSFld(fm0, CONDTRAB) != 'S' && strcmp(retenes[i].condtrab, FmSFld(fm0, CONDTRAB)) != 0)
			continue;

		fprintf(fp, "%ld\t%s\t%s\t%.3D\t%s\t%s %s %s %s\t%B\t%B\t", retenes[i].nroleg, retenes[i].apynom, retenes[i].regimen, 
		                       retenes[i].franco, retenes[i].telefono, retenes[i].altapol1, retenes[i].altapol2, 
		                       retenes[i].altapol3, retenes[i].altapol4, retenes[i].portacion, retenes[i].clu);

		switch (*FmSFld(fm0, ORDEN)) {
		case 'E': 
			qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxcliep);
			break;
		case 'C':
			qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxcliee);
			break;
		default :
			break;
		}

		entro = FALSE;		
		for (j = 0; j < ultcli; j++) {

			//Si el cliente no corresponde con el legajo
//			if (cliente[j].nroint != i)   NO PUEDO USAR LA POSICION PORQUE LOS ORDENE
//				continue; 

			
			if (cliente[j].nroleg != retenes[i].nroleg)
				continue; 

			entro = TRUE;

			fprintf(fp, "%s %s %s %s %s %s %s %T %T\t%ld %d %.3D %.3D\t%s %.3D %.3D\t", 
			             cliente[j].diaslab1, cliente[j].diaslab2, cliente[j].diaslab3, cliente[j].diaslab4, cliente[j].diaslab5,
		                 cliente[j].diaslab6, cliente[j].diaslab7, cliente[j].labd, cliente[j].labh, cliente[j].cli, cliente[j].obj,
		                 cliente[j].fchdprov, cliente[j].fchhprov, cliente[j].condtrab, cliente[j].condfd, cliente[j].condfh); 
	
			if (cliente[j].cli == FmLFld(fm0, CLIENTE)) {
				if (strcmp(cliente[j].condtrab, "T") == 0)
					fprintf(fp, "%B\t", TRUE);
				else
					fprintf(fp, "%B\t", FALSE);
				
				if (*FmSFld(fm0, OPCION) != 'D')
					fprintf(fp, "%T\t%T\t%T\n", cliente[j].horaent, cliente[j].horasal, NULL_TIME);
				else
					fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, NULL_TIME);

				horapta = cliente[j].horaent;
			}
			else {
				if (*FmSFld(fm0, OPCION) != 'D')
					fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, cliente[j].horaent);
				else
					fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, NULL_TIME);
			}
		}
	}
}

void ImprimirArchivoHoras()
{

	/*Esta funcion recorre por la estructura clientes */
	int i, j;     
	TIME horapta;
	char provd[5], provh[5];

	horapta = NULL_TIME;

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVD));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provd, SFld(billpro|PROVXDIV_CODPROV));

	SetKey(billpro|PROVXDIVbyPORSUE, FmIFld(fm0, I_CODPAIS), FmIFld(fm0, PROVH));
	if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK)!= ERROR)
		strcpy(provh, SFld(billpro|PROVXDIV_CODPROV));

	qsort((char *)cliente, (unsigned)(ultcli), sizeof(cliente[0]), (IFPVCPVCP)retxclieh);

	for (j = 0; j < ultcli; j++) {

		//Leo los datos del legajo
		i=cliente[j].nroint;
		
		// Si ninguna provincia en la que esta dado de alta esta incluida en el rango pedido
		// no se toma en cuenta.
		if (!FmIsNull(fm0, PROVD)) {
			if ((strcmp(retenes[i].altapol1, provd) < 0 || strcmp(retenes[i].altapol1, provh) > 0) &&
				(strcmp(retenes[i].altapol2, provd) < 0 || strcmp(retenes[i].altapol2, provh) > 0) &&
				(strcmp(retenes[i].altapol3, provd) < 0 || strcmp(retenes[i].altapol3, provh) > 0) &&
				(strcmp(retenes[i].altapol4, provd) < 0 || strcmp(retenes[i].altapol4, provh) > 0))
				continue;
		}                   
		
		if (*FmSFld(fm0, CONDTRAB) != 'S' && strcmp(retenes[i].condtrab, FmSFld(fm0, CONDTRAB)) != 0)
			continue;

		if ((j==0) || (j > 0 && cliente[j].nroint != cliente[j-1].nroint)) {
		   
		   	fprintf(fp, "%ld\t%s\t%s\t%.3D\t%s\t%s %s %s %s\t%B\t%B\t", retenes[i].nroleg, retenes[i].apynom, retenes[i].regimen,    
		                 retenes[i].franco, retenes[i].telefono, retenes[i].altapol1, retenes[i].altapol2, 
		                 retenes[i].altapol3, retenes[i].altapol4, retenes[i].portacion, retenes[i].clu);
    	}
    	
		fprintf(fp, "%s %s %s %s %s %s %s %T %T\t%ld %d %.3D %.3D\t%s %.3D %.3D\t", 
	             cliente[j].diaslab1, cliente[j].diaslab2, cliente[j].diaslab3, cliente[j].diaslab4, cliente[j].diaslab5,
                 cliente[j].diaslab6, cliente[j].diaslab7, cliente[j].labd, cliente[j].labh, cliente[j].cli, cliente[j].obj,
                 cliente[j].fchdprov, cliente[j].fchhprov, cliente[j].condtrab, cliente[j].condfd, cliente[j].condfh); 

		if (cliente[j].cli == FmLFld(fm0, CLIENTE)) {
			if (strcmp(cliente[j].condtrab, "T") == 0)
				fprintf(fp, "%B\t", TRUE);
			else
				fprintf(fp, "%B\t", FALSE);
			if (*FmSFld(fm0, OPCION) != 'D')
				fprintf(fp, "%T\t%T\t%T\n", cliente[j].horaent, cliente[j].horasal, NULL_TIME);
			else
				fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, NULL_TIME);

			horapta = cliente[j].horaent;
		}
		else {
			if (*FmSFld(fm0, OPCION) != 'D')
				fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, cliente[j].horaent);
			else
				fprintf(fp, "%T\t%T\t%T\n", NULL_TIME, NULL_TIME, NULL_TIME);
		}
	}
}

