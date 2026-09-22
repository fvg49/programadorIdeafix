/********************************************************************
* MODULE & VERSION : @(#)lasig.c	1.3
* DATE             : 13/12/16
* TIME             : 17:15:06
*
* CREATED          :
*
* DESCRIPTION:
*      Asignación de Vigiladores sin requerimientos.
*
*********************************************************************/
#include <ideafix.h>
#include "lasig.fmh"
#include "lasig1.rph"
#include "lasig2.rph"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "comerc.sch"
#include "comerc.h"
#include "filial.h"

#define CLI_INEX   "El Cliente: %ld no existe!."
#define OBJET_INEX "El Objetivo: %d del Cliente: %ld no existe!."
#define PER_INEX   "No existe el Vigilador : %ld de la Empresa : %d (%s)"

#define ERR_ARCHI  "No se pudo abrir el archivo!"

#define ARCHI  0
#define TERM   1
#define IMPRE  2

#define _LIS_ACTUAL		1
#define _LIS_HIS		2
#define _LIS_TOT		3

// Declaraciones de Funciones
static fm_status after(form, fmfield, int);
static fm_status before(form fm, fmfield fno, int row);

static void ListaPorCliObjCte();
static void ListaPorCliObjHis();
static void ListaPorVigiCte();
static void ListaPorVigiHis();
static void ListaTotVigi ();
static void ListaTotCliObj();

static void AbrirSalida(char nombre[20]);
static void SetearCabeceraRepCliObj();
static void SetearCabeceraRepVigi();
static void SetearCabeceraArchCliObj();
static void SetearCabeceraArchVigi();

static void LeerCliente (long cliente);
static void LeerVigilador(int emp, long nroleg);
static void LeerObjetivo(long cliente, int objetivo);

static void GuardarCliObjCte();
static void GuardarCliObjHis();
static void GuardarVigiCte();
static void GuardarVigiHis();

static void ImprimirCliObjCte();
static void ImprimirCliObjHis();
static void ImprimirVigiCte();
static void ImprimirVigiHis();

bool FechaFinValida (DATE desdefm, DATE hastafm,  DATE fdesde, DATE fhasta);
//void ProximoFranco ( short emp, long nroleg, char * vigil, DATE afranco, short numfran, DATE fasig, DATE *ffranco, short *nfran, int codrol, int fila, int colum);

// Declaraciones globales
form   fm0;
schema operac, bill, comerc, sue;
FILE   *fp=NULL;
report rp0 = ERROR;
bool   salida, impri=TRUE;

/* Programa principal */
wcmd(lasig, 1.3 12/16/13)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	fm0    = OpenForm("lasig",    FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while (DoForm(fm0, before, after) != FM_EXIT)	{
			impri=TRUE;
			salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;
			switch (FmIFld(fm0, LISTAPOR)) {
				case 1 :
					switch(FmIFld(fm0, TIPOLIS)) {
						case _LIS_ACTUAL :
							ListaPorCliObjCte();
							break;
						case _LIS_HIS :
							ListaPorCliObjHis();
							break;
						case _LIS_TOT :
							ListaTotCliObj();
							break;
					}
					break;
				case 2 :
					switch (FmIFld(fm0, TIPOLIS)) {
						case _LIS_ACTUAL :
							ListaPorVigiCte();
							break;
						case _LIS_HIS :
							ListaPorVigiHis();
							break;
						case _LIS_TOT :
							ListaTotVigi();
							break;
					}
			}
			if (rp0!=ERROR)
				CloseReport(rp0);
			if (fp!=NULL)
				fclose(fp);
			fp=NULL;
			rp0=ERROR;
	}
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void ListaPorCliObjCte()
{
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	dbcursor c_asig;
	c_asig = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);
	
	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIsNull(fm0, OBJDESDE) ? MIN_SHORT : FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIHASTA), FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

		if (!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
			continue; 

		if (!FmIsNull(fm0, FDESDE) && (DFld(operac|ASIG_CDATE) != FmDFld(fm0, FDESDE)))
			continue;

		if (impri) {
			AbrirSalida("lasig1");
			impri = FALSE;
		}

		if (cliente != LFld(operac|ASIG_CLIENTE)) {
			LeerCliente(LFld(operac|ASIG_CLIENTE));
		}
		if (objetivo != IFld(operac|ASIG_OBJETIVO)) {
			LeerObjetivo(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
		}
		if (vigilador != LFld(operac|ASIG_NROLEG)) {
			LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
		}

		if (salida == ARCHI)
			GuardarCliObjCte();
		if (salida != ARCHI)
			ImprimirCliObjCte();

		vigilador = LFld(operac|ASIG_NROLEG);
		cliente   = LFld(operac|ASIG_CLIENTE);
		objetivo  = IFld(operac|ASIG_OBJETIVO);
	}
	DeleteCursor(c_asig);	
}

static void GuardarCliObjCte()
{
	DATE franco=NULL_DATE;
	int nfranco=NULL_SHORT;

	CalculaFechaFranco(IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM), Today(), LFld (operac|ASIG_CLIENTE), IFld (operac|ASIG_OBJETIVO), IFld (operac|ASIG_PTOSER), IFld (operac|ASIG_PUESTO), &franco, &nfranco);
//	ProximoFranco (IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG), &franco, &nfranco, IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM));

	SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIG_CLIENTE));
	SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIG_OBJETIVO));
	SetIFld(operac|PUESTOS_TIPPTO,  IFld(operac|ASIG_PTOSER));
	SetIFld(operac|PUESTOS_CODINT,  IFld(operac|ASIG_PUESTO));
	GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	//             1   2   3   4    5   6   7   8   9   0   1   2       R
	fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%s\t%s\t%s\t%d\t%s\t%d\t%d\t%s\t%d\t%d\t%d\t%.3D\t%.3D\t%.3D\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%.1T\t%.1T\t%s\t%s\t%s\t%s\n",
//	fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%s\t%s\t%s\t%d\t%s\t%d\t%s\t%d\t%d\t%d\t%.3D\t%.3D\t%.3D\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%.1T\t%.1T\t%s\t%s\t%s\t%s\n",
			LFld(operac|ASIG_CLIENTE),            SFld(bill|CLIENTE_RAZSOC),
			IFld(operac|ASIG_OBJETIVO),           GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)),
			LFld(operac|ASIG_NROLEG),             SFld(sue|PER_APYNOM),
			SFld(sue|DATPERS_CUIL),               SFld(operac|ASIG_VIGIL),
			SFld(operac|ASIG_EFECT),              IFld(operac|ASIG_PTOSER),
			GetDescPto(IFld(operac|ASIG_PTOSER)), IFld(operac|PUESTOS_CODINT),
			IFld(operac|PUESTOS_PUESTO),		  SFld(operac|ASIG_REGIM),
			IFld(operac|ASIG_CODROL),             IFld(operac|ASIG_FILA),
			IFld(operac|ASIG_COLUM),              DFld(operac|ASIG_FECASIG),
			DFld(operac|ASIG_FECHAS),             franco,
			nfranco,                              SFld(operac|ASIG_DIA1),
			SFld(operac|ASIG_DIA2),               SFld(operac|ASIG_DIA3),
			SFld(operac|ASIG_DIA4),               SFld(operac|ASIG_DIA5),
			SFld(operac|ASIG_DIA6),               SFld(operac|ASIG_DIA7),
			TFld(operac|ASIG_HSENT),              TFld(operac|ASIG_HSSAL),
			NULL_STR, NULL_STR, //Valores Nulos: porque son el campo de motivo de desasignacion y su respectiva descripcion
			GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)),
    		GetDescFilial(GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
			);
}

static void ImprimirCliObjCte()
{
	DATE franco=NULL_DATE;
	int nfranco=NULL_SHORT;

	//DbToRp(rp0, R_LEGAJO, R_HSSAL);
	RpSetLFld(rp0, R_CLI,     LFld(operac|ASIG_CLIENTE));
	RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, R_OBJ,     IFld(operac|ASIG_OBJETIVO));
	RpSetFld (rp0, R_DOBJ,    GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
	RpSetLFld(rp0, R_LEGAJO,  LFld(operac|ASIG_NROLEG));
	RpSetFld (rp0, R_APENOM,  SFld(sue|PER_APYNOM));
	RpSetFld (rp0, R_VR,      InDescr(operac|ASIG_VIGIL, SFld(operac|ASIG_VIGIL)));
	RpSetFld (rp0, R_EF,      InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)));

	if(!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		RpSetFld(rp0, R_FR, "F");
	else
		RpSetFld(rp0, R_FR, NULL_STR);		
	RpSetIFld(rp0, R_PTOSER,  IFld(operac|ASIG_PTOSER));
	RpSetIFld(rp0, R_PUESTO,  IFld(operac|ASIG_PUESTO));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(operac|ASIG_PTOSER)));
	RpSetFld (rp0, R_REGIM,   SFld(operac|ASIG_REGIM));
	RpSetIFld(rp0, R_ROL,     IFld(operac|ASIG_CODROL));
	RpSetIFld(rp0, R_FILA,    IFld(operac|ASIG_FILA));
	RpSetIFld(rp0, R_COLUM,   IFld(operac|ASIG_COLUM));
	RpSetDFld(rp0, R_FECASIG, DFld(operac|ASIG_FECASIG));
	RpSetDFld(rp0, R_FECHAS,  DFld(operac|ASIG_FECHAS));

	CalculaFechaFranco(IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM), Today(), LFld (operac|ASIG_CLIENTE), IFld (operac|ASIG_OBJETIVO), IFld (operac|ASIG_PTOSER), IFld (operac|ASIG_PUESTO), &franco, &nfranco);
//	ProximoFranco (IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG), &franco, &nfranco, IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM));
	
	RpSetDFld(rp0, R_FFRANCO, franco);
	RpSetIFld(rp0, R_NFRANCO, nfranco);
	RpSetFld (rp0, R_DIA1,    SFld(operac|ASIG_DIA1));
	RpSetFld (rp0, R_DIA2,    SFld(operac|ASIG_DIA2));
	RpSetFld (rp0, R_DIA3,    SFld(operac|ASIG_DIA3));
	RpSetFld (rp0, R_DIA4,    SFld(operac|ASIG_DIA4));
	RpSetFld (rp0, R_DIA5,    SFld(operac|ASIG_DIA5));
	RpSetFld (rp0, R_DIA6,    SFld(operac|ASIG_DIA6));
	RpSetFld (rp0, R_DIA7,    SFld(operac|ASIG_DIA7));
	RpSetTFld(rp0, R_HSENT,   TFld(operac|ASIG_HSENT));
	RpSetTFld(rp0, R_HSSAL,   TFld(operac|ASIG_HSSAL));
	RpSetFld (rp0, R_FIL,		GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
    RpSetFld (rp0, R_DFIL,		GetDescFilial(GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO))));
	DoReport(rp0, LINEA);
}

static void AbrirSalida(char nombre[20])
{
	if (salida != ARCHI && rp0 == ERROR) {
		rp0 = OpenReport(nombre, RP_NOBEGIN);
		RpSetOutput(rp0, salida == TERM ? RP_IO_TERM : RP_IO_DEFAULT, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
		switch (FmIFld(fm0, LISTAPOR)) {
			case 1 :
				SetearCabeceraRepCliObj();
				break;
			case 2 :
				SetearCabeceraRepVigi();
				break;
		}
	}
	if (salida == ARCHI && (fp == (FILE*)NULL)) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
			Error(ERR_ARCHI);
		else {
			switch (FmIFld(fm0, LISTAPOR)) {
				case 1 :
					SetearCabeceraArchCliObj();
					break;
				case 2 :
					SetearCabeceraArchVigi();
					break;
			}
		}
	}
}

static void SetearCabeceraArchCliObj()
{   
	fprintf(fp, "Cod.Cliente\tRaz.Soc.\tCod.Objetivo\tObjetivo\tLegajo\tApellido y Nombre\tCUIL\tVigilador/Reten\tEfectivo/Prov\tCod.Puesto\tPuesto\tNro. interno de Puesto\tCateg Salarial\tRegimen\tRol\tFila\tColum\tFec.Asig.\t \tFec.Franco\tNro. Franco\tDía 1\tDía 2\tDía 3\tDía 4\tDía 5\tDía 6\tDía 7\tH.Ent.\tH.Sal.\tMotivo Desasig\tDescrip Motivo Desasig\tFilial\tDescripcion Filial\n");
}

static void SetearCabeceraArchVigi()
{
	fprintf(fp, "Legajo\tApellido y Nombre\tCUIL\tCod.Cliente\tRaz.Soc.\tCod.Objetivo\tObjetivo\tVigilador/Reten\tEfectivo/Prov\tCod.Puesto\tPuesto\tNro. interno de Puesto\tCateg Salarial\tRegimen\tRol\tFila\tColum\tFec.Asig.\t \tFec.Franco\tNro. Franco\tDía 1\tDía 2\tDía 3\tDía 4\tDía 5\tDía 6\tDía 7\tH.Ent.\tH.Sal.\tMotivo Desasig\tDescrip Motivo Desasig\tFilial\tDescripcion Filial\n");
}

static void SetearCabeceraRepCliObj()
{
	RpSetIFld(rp0, R_EMP,    FmIFld(fm0, EMP));
	RpSetFld (rp0, R_DEMP,   FmSFld(fm0, DEMP));
	RpSetLFld(rp0, R_CLIED,  FmLFld(fm0, CLIDESDE));
	RpSetFld (rp0, R_DCLIED, FmSFld(fm0, DCLID));
	RpSetIFld(rp0, R_OBJD,   FmIFld(fm0, OBJDESDE));
	RpSetFld (rp0, R_DOBJD,  FmSFld(fm0, DOBJD));
	RpSetLFld(rp0, R_CLIEH,  FmLFld(fm0, CLIHASTA));
	RpSetFld (rp0, R_DCLIEH, FmSFld(fm0, DCLIH));
	RpSetIFld(rp0, R_OBJH,   FmIFld(fm0, OBJHASTA));
	RpSetFld (rp0, R_DOBJH,  FmSFld(fm0, DOBJH));
	RpSetFld (rp0, R_TIPINF, FmSFld(fm0, DTIPLIS));
	RpSetDFld(rp0, R_FDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, R_FHASTA, FmDFld(fm0, FHASTA));
}

static void SetearCabeceraRepVigi()
{
	RpSetIFld(rp0, RR_EMP,    FmIFld(fm0, EMP));
	RpSetFld (rp0, RR_DEMP,   FmSFld(fm0, DEMP));
	RpSetLFld(rp0, RR_VIGD,   FmLFld(fm0, VIGDESDE));
	RpSetFld (rp0, RR_DVIGD,  FmSFld(fm0, DVIGD));
	RpSetLFld(rp0, RR_VIGH,   FmLFld(fm0, VIGHASTA));
	RpSetFld (rp0, RR_DVIGH,  FmSFld(fm0, DVIGH));
	RpSetFld (rp0, RR_TIPINF, FmSFld(fm0, DTIPLIS));
	RpSetDFld(rp0, RR_FDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, RR_FHASTA, FmDFld(fm0, FHASTA));
}

static void ListaPorVigiCte()
{
	dbcursor c_asig;
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;

	c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, VIGDESDE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, VIGHASTA), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

		if(!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
			continue; 


		if(!FmIsNull(fm0, FDESDE) && (DFld(operac|ASIG_CDATE) != FmDFld(fm0, FDESDE)))
			continue;
		
		if (impri) {
			AbrirSalida("lasig2");
			impri = FALSE;
		}
		if (vigilador != LFld(operac|ASIG_NROLEG)) {
			LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
		}
		if (cliente != LFld(operac|ASIG_CLIENTE)) {
			LeerCliente(LFld(operac|ASIG_CLIENTE));
		}
		if (objetivo != IFld(operac|ASIG_OBJETIVO)) {
			LeerObjetivo(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
		}

		vigilador = LFld(operac|ASIG_NROLEG);
		cliente   = LFld(operac|ASIG_CLIENTE);
		objetivo  = IFld(operac|ASIG_OBJETIVO);

		if (salida == ARCHI)
			GuardarVigiCte();
		if (salida != ARCHI)
			ImprimirVigiCte();
	}
	DeleteCursor(c_asig);
}

static void ImprimirVigiCte()
{
	DATE franco=NULL_DATE;
	int nfranco=NULL_SHORT;

	RpClearZone(rp0, LINEA);
	RpSetLFld(rp0, R_VIGI,  LFld(operac|ASIG_NROLEG));
	RpSetFld (rp0, R_DVIGI, SFld(sue|PER_APYNOM));
	RpSetLFld(rp0, RR_CLI,  LFld(operac|ASIG_CLIENTE));
	RpSetFld (rp0, RR_DCLI, SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, RR_OBJ,  IFld(operac|ASIG_OBJETIVO));
	RpSetFld (rp0, RR_DOBJ, GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
	RpSetFld (rp0, RR_VR,   InDescr(operac|ASIG_VIGIL, SFld(operac|ASIG_VIGIL)));
	RpSetFld (rp0, RR_EF,   InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)));
	if(!IsNull(operac|ASIG_FRANCERO) &&IFld(operac|ASIG_FRANCERO))
		RpSetFld(rp0, RR_FR, "F");
	else
		RpSetFld(rp0, RR_FR, NULL_STR);

	RpSetIFld(rp0, RR_PTOSER,  IFld(operac|ASIG_PTOSER));
	RpSetIFld(rp0, RR_PUESTO,  IFld(operac|ASIG_PUESTO));
	RpSetFld (rp0, RR_DPTOSER, GetDescPto(IFld(operac|ASIG_PTOSER)));
	RpSetFld (rp0, RR_REGIM,   SFld(operac|ASIG_REGIM));
	RpSetIFld(rp0, RR_ROL,     IFld(operac|ASIG_CODROL));
	RpSetIFld(rp0, RR_FILA,    IFld(operac|ASIG_FILA));
	RpSetIFld(rp0, RR_COLUM,   IFld(operac|ASIG_COLUM));
	RpSetDFld(rp0, RR_FECASIG, DFld(operac|ASIG_FECASIG));
	RpSetDFld(rp0, RR_FECHAS,  DFld(operac|ASIG_FECHAS));

	CalculaFechaFranco(IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM),/*DFld (operac|ASIG_FECASIG)*/ Today(), LFld (operac|ASIG_CLIENTE), IFld (operac|ASIG_OBJETIVO), IFld (operac|ASIG_PTOSER), IFld (operac|ASIG_PUESTO), &franco, &nfranco);

	RpSetDFld(rp0, RR_FFRANCO, franco);
	RpSetIFld(rp0, RR_NFRANCO, nfranco);
	
	RpSetFld (rp0, RR_DIA1,		SFld(operac|ASIG_DIA1));
	RpSetFld (rp0, RR_DIA2,		SFld(operac|ASIG_DIA2));
	RpSetFld (rp0, RR_DIA3,		SFld(operac|ASIG_DIA3));
	RpSetFld (rp0, RR_DIA4,		SFld(operac|ASIG_DIA4));
	RpSetFld (rp0, RR_DIA5,		SFld(operac|ASIG_DIA5));
	RpSetFld (rp0, RR_DIA6,		SFld(operac|ASIG_DIA6));
	RpSetFld (rp0, RR_DIA7,		SFld(operac|ASIG_DIA7));
	RpSetTFld(rp0, RR_HSENT,	TFld(operac|ASIG_HSENT));
	RpSetTFld(rp0, RR_HSSAL,	TFld(operac|ASIG_HSSAL));
    RpSetFld (rp0, RR_FIL,		GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
    RpSetFld (rp0, RR_DFIL,		GetDescFilial(GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO))));

    DoReport(rp0, LINEA2);
}

static void GuardarVigiCte()
{
	DATE franco=NULL_DATE;
	int nfranco=NULL_SHORT;
  

	CalculaFechaFranco(IFld(operac|ASIG_CODROL), IFld(operac|ASIG_FILA), IFld(operac|ASIG_COLUM), DFld (operac|ASIG_FECASIG), LFld (operac|ASIG_CLIENTE), IFld (operac|ASIG_OBJETIVO), IFld (operac|ASIG_PTOSER), IFld (operac|ASIG_PUESTO), &franco, &nfranco);

	SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIG_CLIENTE));
	SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIG_OBJETIVO));
	SetIFld(operac|PUESTOS_TIPPTO,  IFld(operac|ASIG_PTOSER));
	SetIFld(operac|PUESTOS_CODINT,  IFld(operac|ASIG_PUESTO));
	GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%ld\t%s\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%d\t%s\t%d\t%d\t%s\t%d\t%d\t%d\t%.3D\t%.3D\t%.3D\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%.1T\t%.1T\t%s\t%s\t%s\t%s\n",
			LFld(operac|ASIG_NROLEG),             SFld(sue|PER_APYNOM),//2
			SFld(sue|DATPERS_CUIL),               LFld(operac|ASIG_CLIENTE),//4
			SFld(bill|CLIENTE_RAZSOC),            IFld(operac|ASIG_OBJETIVO),//6
			GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)),//7
			SFld(operac|ASIG_VIGIL),//8
			SFld(operac|ASIG_EFECT),              IFld(operac|ASIG_PTOSER),//10
			GetDescPto(IFld(operac|ASIG_PTOSER)), IFld(operac|PUESTOS_CODINT),//12
			IFld(operac|PUESTOS_PUESTO),		  SFld(operac|ASIG_REGIM),
			IFld(operac|ASIG_CODROL),             IFld(operac|ASIG_FILA),
			IFld(operac|ASIG_COLUM),              DFld(operac|ASIG_FECASIG),
			DFld(operac|ASIG_FECHAS),             franco,
			nfranco,                              SFld(operac|ASIG_DIA1),
			SFld(operac|ASIG_DIA2),               SFld(operac|ASIG_DIA3),
			SFld(operac|ASIG_DIA4),               SFld(operac|ASIG_DIA5),
			SFld(operac|ASIG_DIA6),               SFld(operac|ASIG_DIA7),
			TFld(operac|ASIG_HSENT),              TFld(operac|ASIG_HSSAL),
			NULL_STR, NULL_STR, //Valores Nulos: porque son el campo de motivo de desasignacion y su respectiva descripcion
			GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)),
			GetDescFilial(GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
			);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
    case CLIDESDE:
	   	InicClientesXusr();
    	break;
    case CLIHASTA:
    	break;
    case OBJDESDE:
	   	InicObjetivosXusr(FmLFld(fm, CLIDESDE, row), FmIFld(fm, EMP, row));
    	break;
    case OBJHASTA:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIHASTA, row), FmIFld(fm, EMP, row));
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
		case LISTAPOR :
			switch(FmIFld(fm, fno)) {
				case 1 :
					FmSetFld(fm0, DVIGD, NULL_STR);
					FmSetFld(fm0, DVIGH, NULL_STR);
					break;
				case 2 :
					FmSetFld(fm0, DCLID, NULL_STR);
					FmSetFld(fm0, DCLIH, NULL_STR);
					FmSetFld(fm0, DOBJD, NULL_STR);
					FmSetFld(fm0, DOBJH, NULL_STR);
					break;
			}
			break;
		case CLIDESDE: 
			if (FmKeyCode(fm) == K_HELP)
  				HelpCliente(fm, fno, row);
	  		else
			  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLIDESDE, row)), row);
    	break;
	    case CLIHASTA:
			if (FmKeyCode(fm) == K_HELP)
				HelpCliente(fm, fno, row);
	  		else
				FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIHASTA, row)), row);
	    	break;
	    case OBJDESDE:
			if (FmKeyCode(fm) == K_HELP)
  				HelpObjet(fm, fno, row, FmLFld(fm, CLIDESDE, row));
	  		else
				FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIDESDE, row), FmIFld(fm, OBJDESDE, row)), row);
	    	break;
	    case OBJHASTA:
			if (FmKeyCode(fm) == K_HELP)
				HelpObjet(fm, fno, row, FmLFld(fm, CLIHASTA, row));
	  		else	
				FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIHASTA, row) ,FmIFld(fm, OBJHASTA, row)), row);
	    	break;
		case FFILIAL:
			if (FmKeyCode(fm) == K_HELP)
				HelpFilial(fm, fno, row);
			break;
	}
	return FM_OK;
}

static void LeerCliente (long cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(CLI_INEX, cliente);
	}
}

static void LeerObjetivo(long cliente, int objetivo)
{
	SetKey(comerc|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(OBJET_INEX, cliente, objetivo);
	}
}

static void LeerVigilador(int emp, long nroleg)
{
	SetKey(sue|PERbyEMP, emp, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(PER_INEX, nroleg, emp, "PER");
	}
	SetKey(sue|DATPERSbyEMP, emp, nroleg);
	if (GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(PER_INEX, nroleg, emp, "DATPERS");
	}
}

static void ListaPorCliObjHis()
{
	dbcursor c_asigh;
	bool esta;
	DATE fecha;
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	
	c_asigh	=	CreateCursor(operac|ASIGHbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIsNull(fm0, OBJDESDE) ? MIN_SHORT : FmIFld(fm0, OBJDESDE),
					MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, CLIHASTA), FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA),
					MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
	while (FetchCursor(c_asigh) != ERROR) {
 		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 
  
  		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
			continue;
		if (DFld(operac|ASIGH_FECBAJ) < DFld(operac|ASIGH_FECALT))
			continue; 

 		esta = TRUE;
		
		if(!FmIsNull(fm0, FDESDE) && !FmIsNull(fm0, FHASTA)){
			esta = FALSE;			
			for(fecha=DFld(operac|ASIGH_FECALT); fecha < DFld(operac|ASIGH_FECBAJ)+1; fecha++) {
				if(fecha < FmDFld(fm0, FDESDE) || fecha > FmDFld(fm0, FHASTA))
					continue;
				esta = TRUE;
				break;				
			}
		}
		if(!esta)
			continue;


		if (impri) {
			AbrirSalida("lasig1");
			impri	=	FALSE;
		}
		if (cliente != LFld(operac|ASIGH_CLIENTE)) {
			LeerCliente(LFld(operac|ASIGH_CLIENTE));
		}
		if (objetivo != IFld(operac|ASIGH_OBJETIVO)) {
			LeerObjetivo(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
		}
		if (vigilador != LFld(operac|ASIGH_NROLEG)) {
			LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
		}
		if (salida == ARCHI) GuardarCliObjHis();
		if (salida != ARCHI) ImprimirCliObjHis();
		vigilador = LFld(operac|ASIGH_NROLEG);
		cliente   = LFld(operac|ASIGH_CLIENTE);
		objetivo  = IFld(operac|ASIGH_OBJETIVO);
	}
	DeleteCursor(c_asigh);
}

static void GuardarCliObjHis()
{
	SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIGH_CLIENTE));
	SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIGH_OBJETIVO));
	SetIFld(operac|PUESTOS_TIPPTO,  IFld(operac|ASIGH_PTOSER));
	SetIFld(operac|PUESTOS_CODINT,  IFld(operac|ASIGH_PUESTO));
	GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%s\t%s\t%s\t%d\t%s\t%d\t%d\t%s\t%d\t%d\t%d\t%.3D\t%.3D\t%.3D\t \t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%.1T\t%.1T\t%d\t%s\t%s\t%s\n",
			LFld(operac|ASIGH_CLIENTE),            SFld(bill|CLIENTE_RAZSOC),
			IFld(operac|ASIGH_OBJETIVO),
			GetObjDescrip(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)),
			LFld(operac|ASIGH_NROLEG),             SFld(sue|PER_APYNOM),
			SFld(sue|DATPERS_CUIL),                SFld(operac|ASIGH_VIGIL),
			SFld(operac|ASIGH_EFECT),              IFld(operac|ASIGH_PTOSER),
			GetDescPto(IFld(operac|ASIGH_PTOSER)), IFld(operac|PUESTOS_CODINT),
			IFld(operac|PUESTOS_PUESTO),
			SFld(operac|ASIGH_REGIM),
			IFld(operac|ASIGH_CODROL),             IFld(operac|ASIGH_FILA),
			IFld(operac|ASIGH_COLUM),              DFld(operac|ASIGH_FECALT),
			DFld(operac|ASIGH_FECBAJ),             NULL_DATE,
			SFld(operac|ASIGH_DIA1),               SFld(operac|ASIGH_DIA2),
			SFld(operac|ASIGH_DIA3),               SFld(operac|ASIGH_DIA4),
			SFld(operac|ASIGH_DIA5),               SFld(operac|ASIGH_DIA6),
			SFld(operac|ASIGH_DIA7),               TFld(operac|ASIGH_HSENT),
			TFld(operac|ASIGH_HSSAL),              IFld(operac|ASIGH_MOTIVO),
			!IsNull(operac|ASIGH_MOTIVO) ? GetDescMotivd(IFld(operac|ASIGH_MOTIVO)) : NULL_STR,
			GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)),
			GetDescFilial(GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
			);
}

static void ImprimirCliObjHis()
{
	//DbToRp(rp0, R_LEGAJO, R_HSSAL);
	RpSetLFld(rp0, R_CLI,     LFld(operac|ASIGH_CLIENTE));
	RpSetFld (rp0, R_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, R_OBJ,     IFld(operac|ASIGH_OBJETIVO));
	RpSetFld (rp0, R_DOBJ,    GetObjDescrip(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)));
	RpSetLFld(rp0, R_LEGAJO,  LFld(operac|ASIGH_NROLEG));
	RpSetFld (rp0, R_APENOM,  SFld(sue|PER_APYNOM));
	RpSetFld (rp0, R_VR,      InDescr(operac|ASIGH_VIGIL, SFld(operac|ASIGH_VIGIL)));
	RpSetFld (rp0, R_EF,      InDescr(operac|ASIGH_EFECT, SFld(operac|ASIGH_EFECT)));

	if (!IsNull(operac|ASIGH_FRANCERO) && IFld(operac|ASIGH_FRANCERO))
		RpSetFld(rp0, R_FR, "F");
	else
		RpSetFld(rp0, R_FR, NULL_STR);

	RpSetIFld(rp0, R_PTOSER,  IFld(operac|ASIGH_PTOSER));
	RpSetIFld(rp0, R_PUESTO,  IFld(operac|ASIGH_PUESTO));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(operac|ASIGH_PTOSER)));
	RpSetFld (rp0, R_REGIM,   SFld(operac|ASIGH_REGIM));
	RpSetIFld(rp0, R_ROL,     IFld(operac|ASIGH_CODROL));
	RpSetIFld(rp0, R_FILA,    IFld(operac|ASIGH_FILA));
	RpSetIFld(rp0, R_COLUM,   IFld(operac|ASIGH_COLUM));

	RpSetDFld(rp0, R_FECASIG, DFld(operac|ASIGH_FECALT));
	RpSetDFld(rp0, R_FECHAS,  DFld(operac|ASIGH_FECBAJ));
	RpSetDFld(rp0, R_FFRANCO, NULL_DATE);
	RpSetIFld(rp0, R_NFRANCO, NULL_SHORT);
	RpSetFld (rp0, R_DIA1,    SFld(operac|ASIGH_DIA1));
	RpSetFld (rp0, R_DIA2,    SFld(operac|ASIGH_DIA2));
	RpSetFld (rp0, R_DIA3,    SFld(operac|ASIGH_DIA3));
	RpSetFld (rp0, R_DIA4,    SFld(operac|ASIGH_DIA4));
	RpSetFld (rp0, R_DIA5,    SFld(operac|ASIGH_DIA5));
	RpSetFld (rp0, R_DIA6,    SFld(operac|ASIGH_DIA6));
	RpSetFld (rp0, R_DIA7,    SFld(operac|ASIGH_DIA7));
	RpSetTFld(rp0, R_HSENT,   TFld(operac|ASIGH_HSENT));
	RpSetTFld(rp0, R_HSSAL,   TFld(operac|ASIGH_HSSAL));
	RpSetIFld(rp0, R_MOTD,    IFld(operac|ASIGH_MOTIVO));
	RpSetFld (rp0, R_DMOTD,   GetDescMotivd(IFld(operac|ASIGH_MOTIVO)));
	RpSetFld (rp0, R_FIL,		GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));
    RpSetFld (rp0, R_DFIL,		GetDescFilial(GetFilialDeObj(IFld(operac|ASIG_EMP), LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO))));
	DoReport(rp0, LINEA);
}

static void ListaPorVigiHis()
{
	dbcursor c_asigh;
	DATE fecha;
	bool esta;
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;

	c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, VIGDESDE), MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), FmLFld(fm0, VIGHASTA), MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_asigh) != ERROR) {
		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
			continue;
		if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
			continue;
		if (DFld(operac|ASIGH_FECBAJ) < DFld(operac|ASIGH_FECALT))
			continue;

		esta = TRUE;

		if (!FmIsNull(fm0, FDESDE) && !FmIsNull(fm0, FHASTA)) {
			esta = FALSE;
			for (fecha = DFld(operac|ASIGH_FECALT); fecha < DFld(operac|ASIGH_FECBAJ)+1; fecha++) {
				if (fecha < FmDFld(fm0, FDESDE) || fecha > FmDFld(fm0, FHASTA))
					continue;
				esta = TRUE;
				break;
			}
		}
		if (!esta)
			continue;
		if (impri) {
			AbrirSalida("lasig2");
			impri = FALSE;
		}
		if (vigilador != LFld(operac|ASIGH_NROLEG)) {
			LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
		}
		if (cliente != LFld(operac|ASIGH_CLIENTE)) {
			LeerCliente(LFld(operac|ASIGH_CLIENTE));
		}
		if (objetivo != IFld(operac|ASIGH_OBJETIVO)) {
			LeerObjetivo(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
		}
		if (salida == ARCHI)
			GuardarVigiHis();
		if (salida != ARCHI)
			ImprimirVigiHis();
		vigilador = LFld(operac|ASIGH_NROLEG);
		cliente   = LFld(operac|ASIGH_CLIENTE);
		objetivo  = IFld(operac|ASIGH_OBJETIVO);
	}
	DeleteCursor(c_asigh);
}

static void GuardarVigiHis()
{
	SetLFld(operac|PUESTOS_CLIENTE, LFld(operac|ASIGH_CLIENTE));
	SetIFld(operac|PUESTOS_OBJET,   IFld(operac|ASIGH_OBJETIVO));
	SetIFld(operac|PUESTOS_TIPPTO,  IFld(operac|ASIGH_PTOSER));
	SetIFld(operac|PUESTOS_CODINT,  IFld(operac|ASIGH_PUESTO));
	GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%ld\t%s\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%d\t%s\t%d\t%d\t%s\t%d\t%d\t%d\t%.3D\t%.3D\t%.3D\t \t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%.1T\t%.1T\t%d\t%s\t%s\t%s\n",
			LFld(operac|ASIGH_NROLEG),             SFld(sue|PER_APYNOM),
			SFld(sue|DATPERS_CUIL),                LFld(operac|ASIGH_CLIENTE),
			SFld(bill|CLIENTE_RAZSOC),             IFld(operac|ASIGH_OBJETIVO),
			GetObjDescrip(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)),
			SFld(operac|ASIGH_VIGIL),
			SFld(operac|ASIGH_EFECT),              IFld(operac|ASIGH_PTOSER),
			GetDescPto(IFld(operac|ASIGH_PTOSER)), IFld(operac|PUESTOS_CODINT),
			IFld(operac|PUESTOS_PUESTO),
			SFld(operac|ASIGH_REGIM),
			IFld(operac|ASIGH_CODROL),             IFld(operac|ASIGH_FILA),
			IFld(operac|ASIGH_COLUM),              DFld(operac|ASIGH_FECALT),
			DFld(operac|ASIGH_FECBAJ),             NULL_DATE,
			SFld(operac|ASIGH_DIA1),               SFld(operac|ASIGH_DIA2),
			SFld(operac|ASIGH_DIA3),               SFld(operac|ASIGH_DIA4),
			SFld(operac|ASIGH_DIA5),               SFld(operac|ASIGH_DIA6),
			SFld(operac|ASIGH_DIA7),               TFld(operac|ASIGH_HSENT),
			TFld(operac|ASIGH_HSSAL),              IFld(operac|ASIGH_MOTIVO),
			!IsNull(operac|ASIGH_MOTIVO) ? GetDescMotivd(IFld(operac|ASIGH_MOTIVO)) : NULL_STR,
			GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)),
			GetDescFilial(GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
			);
}

static void ImprimirVigiHis()
{
	RpClearZone(rp0, LINEA);

	RpSetLFld(rp0, R_VIGI,     LFld(operac|ASIGH_NROLEG));
	RpSetFld (rp0, R_DVIGI,    SFld(sue|PER_APYNOM));
	RpSetLFld(rp0, RR_CLI,     LFld(operac|ASIGH_CLIENTE));
	RpSetFld (rp0, RR_DCLI,    SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp0, RR_OBJ,     IFld(operac|ASIGH_OBJETIVO));
	RpSetFld (rp0, RR_DOBJ,    GetObjDescrip(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)));
	RpSetFld (rp0, RR_VR,      InDescr(operac|ASIGH_VIGIL, SFld(operac|ASIGH_VIGIL)));
	RpSetFld (rp0, RR_EF,      InDescr(operac|ASIGH_EFECT, SFld(operac|ASIGH_EFECT)));

	if(!IsNull(operac|ASIGH_FRANCERO) && IFld(operac|ASIGH_FRANCERO))
		RpSetFld(rp0, RR_FR, "F");
	else
		RpSetFld(rp0, RR_FR, NULL_STR);

	RpSetIFld(rp0, RR_PTOSER,  IFld(operac|ASIGH_PTOSER));
	RpSetIFld(rp0, RR_PUESTO,  IFld(operac|ASIGH_PUESTO));
	RpSetFld (rp0, RR_DPTOSER, GetDescPto(IFld(operac|ASIGH_PTOSER)));
	RpSetFld (rp0, RR_REGIM,   SFld(operac|ASIGH_REGIM));
	RpSetIFld(rp0, RR_ROL,     IFld(operac|ASIGH_CODROL));
	RpSetIFld(rp0, RR_FILA,    IFld(operac|ASIGH_FILA));
	RpSetIFld(rp0, RR_COLUM,   IFld(operac|ASIGH_COLUM));
	RpSetDFld(rp0, RR_FECASIG, DFld(operac|ASIGH_FECALT));
	RpSetDFld(rp0, RR_FECHAS,  DFld(operac|ASIGH_FECBAJ));
	RpSetDFld(rp0, RR_FFRANCO, NULL_DATE);
	RpSetIFld(rp0, RR_NFRANCO, NULL_SHORT);
	RpSetFld (rp0, RR_DIA1,    SFld(operac|ASIGH_DIA1));
	RpSetFld (rp0, RR_DIA2,    SFld(operac|ASIGH_DIA2));
	RpSetFld (rp0, RR_DIA3,    SFld(operac|ASIGH_DIA3));
	RpSetFld (rp0, RR_DIA4,    SFld(operac|ASIGH_DIA4));
	RpSetFld (rp0, RR_DIA5,		SFld(operac|ASIGH_DIA5));
	RpSetFld (rp0, RR_DIA6,		SFld(operac|ASIGH_DIA6));
	RpSetFld (rp0, RR_DIA7,		SFld(operac|ASIGH_DIA7));
	RpSetTFld(rp0, RR_HSENT,	TFld(operac|ASIGH_HSENT));
	RpSetTFld(rp0, RR_HSSAL,	TFld(operac|ASIGH_HSSAL));
	RpSetIFld(rp0, RR_MOTD,		IFld(operac|ASIGH_MOTIVO));
	RpSetFld (rp0, RR_DMOTD,	GetDescMotivd(IFld(operac|ASIGH_MOTIVO)));
    RpSetFld (rp0, RR_FIL,		GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)));
    RpSetFld (rp0, RR_DFIL,		GetDescFilial(GetFilialDeObj(IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO))));

	DoReport(rp0, LINEA2);
}

bool FechaFinValida (DATE desdefm, DATE hastafm,  DATE fdesde, DATE fhasta)
{
	fdesde  = fdesde  == NULL_DATE ? MIN_DATE : fdesde;
	fhasta  = fhasta  == NULL_DATE ? MAX_DATE : fhasta;
	desdefm = desdefm == NULL_DATE ? MIN_DATE : desdefm;
	hastafm = hastafm == NULL_DATE ? MAX_DATE : hastafm;

	if (fhasta < desdefm )
		return FALSE;

	if (fdesde > hastafm)
		return FALSE;

	return TRUE;
}

static void ListaTotCliObj()
{
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	dbcursor c_asig, c_cliobj, c_asigh;

	c_asig   = CreateCursor(operac|ASIGbyEMP,  IO_NOT_LOCK);
	c_asigh  = CreateCursor(operac|ASIGHbyEMP, IO_NOT_LOCK);
	c_cliobj = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);

	/*Leo los clientes objetivos pedidos porque puede ser que este en asig pero no en asigh */
	SetCursorFrom(c_cliobj, FmLFld(fm0, CLIDESDE), FmIFld(fm0, OBJDESDE));
	SetCursorTo  (c_cliobj, FmLFld(fm0, CLIHASTA), FmIFld(fm0, OBJHASTA));
	while (FetchCursor(c_cliobj) != ERROR) {
 		//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			continue;

		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		//fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 
		
		/*Leo de la tabla asig las asignaciones actuales */
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			if (!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
				continue;

			if (!FmIsNull(fm0, FDESDE) &&
				!FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS)))
				continue;

			if (impri) {
				AbrirSalida("lasig1");
				impri = FALSE;
			}
			if (cliente != LFld(operac|ASIG_CLIENTE)) {
				LeerCliente(LFld(operac|ASIG_CLIENTE));
			}
			if (vigilador != LFld(operac|ASIG_NROLEG)) {
				LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
			}
			if (salida == ARCHI) GuardarCliObjCte();
			if (salida != ARCHI) ImprimirCliObjCte();
			vigilador = LFld(operac|ASIG_NROLEG);
			cliente   = LFld(operac|ASIG_CLIENTE);
			objetivo  = IFld(operac|ASIG_OBJETIVO);
		}

		SetCursorFrom(c_asigh, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , 
							MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , 
							MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);

		while (FetchCursor(c_asigh) != ERROR) {
			if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
				continue;
			if (DFld(operac|ASIGH_FECBAJ) < DFld(operac|ASIGH_FECALT))
				continue;
			if (!FmIsNull(fm0, FDESDE) && 
				!FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIGH_FECALT), DFld(operac|ASIGH_FECBAJ )))
				continue;

			if (impri) {
				AbrirSalida("lasig1");
				impri = FALSE;
			}
			if (cliente != LFld(operac|ASIGH_CLIENTE)) {
				LeerCliente(LFld(operac|ASIGH_CLIENTE));
			}
			if (vigilador != LFld(operac|ASIGH_NROLEG)) {
				LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
			}
			if (salida == ARCHI) GuardarCliObjHis();
			if (salida != ARCHI) ImprimirCliObjHis();
			vigilador = LFld(operac|ASIGH_NROLEG);
			cliente   = LFld(operac|ASIGH_CLIENTE);
			objetivo  = IFld(operac|ASIGH_OBJETIVO);
		}
	}
	DeleteCursor(c_cliobj);
	DeleteCursor(c_asig);
	DeleteCursor(c_asigh);
}

static void ListaTotVigi ()
{
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	dbcursor c_asig, c_per, c_asigh;

	c_per   = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	c_asig  = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);
	c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);

	SetCursorFrom(c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGDESDE));
	SetCursorTo  (c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGHASTA));
	while (FetchCursor(c_per) != ERROR) {
/*****************************************************
		if (IFld(sue|PER_RELACION) != CONVENIO)
			continue;
*******************************************************/

		SetKey(sue|DATPERSbyEMP, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG));
		if (GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
			Warning(PER_INEX, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), "DATPERS2");
			continue;
		}

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
			if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
				continue;
			if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

		    //fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 

			if(!FmIsNull(fm0, FDESDE) && 
			   !FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS)))
				continue;

			if (impri) {
				AbrirSalida("lasig2");
				impri = FALSE;
			}
			if (cliente != LFld(operac|ASIG_CLIENTE)) {
				LeerCliente(LFld(operac|ASIG_CLIENTE));
			}
			if (objetivo != IFld(operac|ASIG_OBJETIVO)) {
				LeerObjetivo(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO));
			}
			if (salida == ARCHI)
				GuardarVigiCte();
			if (salida != ARCHI)
				ImprimirVigiCte();

			vigilador = LFld(operac|ASIG_NROLEG);
			cliente   = LFld(operac|ASIG_CLIENTE);
			objetivo  = IFld(operac|ASIG_OBJETIVO);
		}
		SetCursorFrom(c_asigh, FmIFld(fm0, EMP),  LFld(sue|PER_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP),  LFld(sue|PER_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
			//antes de cargar en una lista cliente/objetivo valido el permiso del usuario sobre el mismo y la filial y delegaciones ingresadas
			if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
				continue;
			if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

		    //fprintf(stderr, "Cliente %ld - Objetivo %d - Delega %s - Deleg %s - Filial %s\n", LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)); 
	   
	   		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
				continue;

			if (!FmIsNull(fm0, FDESDE) && 
				!FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIGH_FECALT), DFld(operac|ASIGH_FECBAJ )))
				continue;
			
			if (impri) {
				AbrirSalida("lasig2");
				impri = FALSE;
			}
			if (cliente != LFld(operac|ASIGH_CLIENTE)) {
				LeerCliente(LFld(operac|ASIGH_CLIENTE));
			}
			if (objetivo != IFld(operac|ASIGH_OBJETIVO)) {
				LeerObjetivo(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO));
			}
			if (salida == ARCHI)
				GuardarVigiHis();
			if (salida != ARCHI)
				ImprimirVigiHis();
			vigilador = LFld(operac|ASIGH_NROLEG);
			cliente   = LFld(operac|ASIGH_CLIENTE);
			objetivo  = IFld(operac|ASIGH_OBJETIVO);
		}
	}
	DeleteCursor(c_per);
	DeleteCursor(c_asigh);
	DeleteCursor(c_asig);
}
