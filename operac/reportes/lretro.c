/********************************************************************
* MODULE & VERSION : @(#)lretro.c	1.1 
* DATE             : 06/05/15 
* TIME             : 13:05:43 
*
* CREATED          : 05/03/2001
*
* DESCRIPTION:
*      Listado de Retroactivos.
*
*********************************************************************/
#include <ideafix.h>
#include "excepcion.h"
#include "lretro.fmh"
#include "lretro.rph"
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#include "bill.sch"
#include "comerc.sch"
#include "comerc.h"

#define CLI_INEX   "El Cliente: %ld no existe!."
#define OBJET_INEX "El Objetivo: %d del Cliente: %ld no existe!."

#define ERR_ARCHI  "No se pudo abrir el archivo!"

#define ARCHI  0
#define TERM   1
#define IMPRE  2

#define _LIS_ACTUAL		1
#define _LIS_HIS		2
#define _LIS_TOT		3

// Declaraciones de Funciones
static fm_status after(form, fmfield, int);

static void ListaPorCliObjCte();
static void ListaPorCliObjHis();
static void ListaPorVigiCte();
static void ListaPorVigiHis();
static void ListaTotVigi ();
static void ListaTotCliObj();

static void AbrirSalida(char *nombre);
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
void ProximoFranco ( short emp, long nroleg, char * vigil, DATE afranco, short numfran, DATE fasig, DATE *ffranco, short *nfran);

// Declaraciones globales
form   fm0;
schema operac, bill, comerc, sue;
FILE   *fp;
report rp0 = ERROR;
dbcursor cretro, cretroexc;
bool   salida, impri=TRUE;

/* Programa principal */
wcmd(lretro, 1.12 09/01/00)
{
	fm_status cmd;
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	fm0    = OpenForm("lretro",   FM_EABORT);

	if (DoForm(fm0, NULLFP, after) != FM_UPDATE)
		return;

	InicioListaTipoExcepcion();

	salida = (*FmSFld(fm0, SALIDA) == 'A') ? ARCHI : *FmSFld(fm0, SALIDA) == 'T' ? TERM : IMPRE;

	switch (FmIFld(fm0, LISTAPOR)) {
		case 1 :
				cretro = CreateCursor(RETRObyEMP, IO_NOT_LOCK);
				cretroexc = CreateCursor(RETROEXCbyEMP, IO_NOT_LOCK);

			break;
		case 2 :
		    break;
		case 3 :
		    break;
	}
}


	/*Recorro las excepciones */
	if (!FmIsNull(fm0, CLID)) {
		SetCursorFrom(cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		SetCursorFrom(cretro, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, MIN_DATE,
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretro, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_DATE,
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while (FetchCursor(cretro) != ERROR) {

		if (DFld(RETRO_DIA) < FmDFld(fm0, FECHAD) || DFld(RETRO_DIA) > FmDFld(fm0, FECHAH))
			continue;

		CargarCliObj(FmIFld(fm0, EMP),
					 LFld(RETRO_CLIENTE),
					 IFld(RETRO_OBJETIVO),
					 LFld(RETRO_NROLEG),
					 IFld(RETRO_PTOSER),
					 IFld(RETRO_PUESTO),
					 DFld(RETRO_DIA),
					 NULL_SHORT,
					 IFld(RETRO_DHSNOR),
					 IFld(RETRO_DHS50),
					 IFld(RETRO_DHS100F),
					 IFld(RETRO_DHS100FE));
		sprintf (buffer, "Procesando Retroactivo de Cliente %ld Objetivo %d", LFld(RETRO_CLIENTE), IFld(RETRO_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();
	}
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();
	DeleteCursor(cretro);

	/*Recorro los retroactivos excepciones */
	if (!FmIsNull(fm0, CLID)) {
		SetCursorFrom(cretroexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretroexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	else {
		SetCursorFrom(cretroexc, FmIFld(fm0, EMP), MIN_LONG, MIN_SHORT, MIN_DATE,
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cretroexc, FmIFld(fm0, EMP), MAX_LONG, MAX_SHORT, MAX_DATE,
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while (FetchCursor(cretroexc) != ERROR) {
		if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) || DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
			continue;
		SetKey(comerc|OBJETIVO, LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
		(void)GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK);

		tipoexc = ParteTipoExcepcion(IFld(RETROEXC_CONDIC), (RETROEXC_MOTIVO));
		/* Estos tipos no los cuento como excepcion, las horas del parte son las reales */
		if (tipoexc == _TIPO_EXCEP_NORM || tipoexc == _TIPO_EXCEP_DESC){
				continue;
		}

		CargarCliObj(FmIFld(fm0, EMP),
					 LFld(RETROEXC_CLIENTE),
					 IFld(RETROEXC_OBJETIVO),
					 LFld(RETROEXC_NROLEG),
					 IFld(RETROEXC_PTOSER),
					 IFld(RETROEXC_PUESTO),
					 DFld(RETROEXC_DIA),
					 IFld(RETROEXC_CONDIC),
					 IFld(RETROEXC_DHORAS),
					 IFld(RETROEXC_DHS50),
					 Feriado(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? 0.0 : IFld(RETROEXC_DHS100),
					 Feriado(DFld(RETROEXC_DIA), IFld(comerc|OBJETIVO_PAIS), IFld(comerc|OBJETIVO_PROV)) ? IFld(RETROEXC_DHS100) : 0.0);
		sprintf (buffer, "Procesando Retro Excepciones de Cliente %ld Objetivo %d", LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO));
		FmSetFld (fm0, COMENT, buffer);
		WiRefresh();
	}
	DeleteCursor(cretroexc);
	FmSetFld (fm0, COMENT, NULL_STR);
	WiRefresh();

static void ListaPorCliObjCte()
{
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	dbcursor c_asig;
	c_asig = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);
	SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIsNull(fm0, OBJDESDE) ? MIN_SHORT : FmIFld(fm0, OBJDESDE), MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIHASTA), FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {
		if(!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
			continue; 

		if(!FmIsNull(fm0, FDESDE) && (DFld(operac|ASIG_CDATE) != FmDFld(fm0, FDESDE)))
			continue;

		if (impri) {
			AbrirSalida("lretro1");
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
	
}

static void GuardarCliObjCte()
{
	DATE franco;
	short nfranco;
	   
	ProximoFranco (IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG), &franco, &nfranco);
	
	fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%s\t%s\t%d\t%s\t%s\t%D\t%D\t%D\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%T\t%T\n",
			LFld(operac|ASIG_CLIENTE), SFld(bill|CLIENTE_RAZSOC), IFld(operac|ASIG_OBJETIVO),
			SFld(comerc|OBJETIVO_DESCRIP), LFld(operac|ASIG_NROLEG), SFld(sue|PER_APYNOM),
			SFld(operac|ASIG_VIGIL), SFld(operac|ASIG_EFECT), IFld(operac|ASIG_PTOSER),
			GetDescPto(IFld(operac|ASIG_PTOSER)), SFld(operac|ASIG_REGIM), DFld(operac|ASIG_FECASIG),
			DFld(operac|ASIG_FECHAS), franco, nfranco, SFld(operac|ASIG_DIA1), SFld(operac|ASIG_DIA2),
			SFld(operac|ASIG_DIA3), SFld(operac|ASIG_DIA4), SFld(operac|ASIG_DIA5), SFld(operac|ASIG_DIA6),
			SFld(operac|ASIG_DIA7), TFld(operac|ASIG_HSENT), TFld(operac|ASIG_HSSAL));
}

static void ImprimirCliObjCte()
{
	DATE franco;
	short nfranco;

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
	RpSetDFld(rp0, R_FECASIG, DFld(operac|ASIG_FECASIG));
	RpSetDFld(rp0, R_FECHAS,  DFld(operac|ASIG_FECHAS));

	ProximoFranco (IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG), &franco, &nfranco);
	
	
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
	DoReport(rp0, LINEA);
}

static void AbrirSalida(char *nombre)
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
	fprintf(fp, "Cod.Cliente\tRaz.Soc.\tCod.Objetivo\tObjetivo\tLegajo\tApellido y Nombre\tVigilador/Reten\tEfectivo/Prov\tCod.Puesto\tPuesto\tRegimen\tFec. Asig.\t \tFec.Franco\tNro. Franco\tDía 1\tDía 2\tDía 3\tDía 4\tDía 5\tDía 6\tDía 7\tH.Ent.\tH.Sal.\n");
}

static void SetearCabeceraArchVigi()
{
	fprintf(fp, "Legajo\tApellido y Nombre\tCod.Cliente\tRaz.Soc.\tCod.Objetivo\tObjetivo\tVigilador/Reten\tEfectivo/Prov\tCod.Puesto\tPuesto\tRegimen\tFec. Asig.\t \tFec.Franco\tNro. Franco\tDía 1\tDía 2\tDía 3\tDía 4\tDía 5\tDía 6\tDía 7\tH.Ent.\tH.Sal.\n");
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
		if(!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
			continue; 


		if(!FmIsNull(fm0, FDESDE) && (DFld(operac|ASIG_CDATE) != FmDFld(fm0, FDESDE)))
			continue;

		if (impri) {
			AbrirSalida("lretro2");
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
		if (salida == ARCHI)
			GuardarVigiCte();
		if (salida != ARCHI)
			ImprimirVigiCte();

		vigilador = LFld(operac|ASIG_NROLEG);
		cliente   = LFld(operac|ASIG_CLIENTE);
		objetivo  = IFld(operac|ASIG_OBJETIVO);
	}
}

static void ImprimirVigiCte()
{
	DATE franco;
	short nfranco;

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
	RpSetDFld(rp0, RR_FECASIG, DFld(operac|ASIG_FECASIG));
	RpSetDFld(rp0, RR_FECHAS,  DFld(operac|ASIG_FECHAS));

	ProximoFranco(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG), &franco, &nfranco);

	RpSetDFld(rp0, RR_FFRANCO, franco);
	RpSetIFld(rp0, RR_NFRANCO, nfranco);
	
	RpSetFld (rp0, RR_DIA1,    SFld(operac|ASIG_DIA1));
	RpSetFld (rp0, RR_DIA2,    SFld(operac|ASIG_DIA2));
	RpSetFld (rp0, RR_DIA3,    SFld(operac|ASIG_DIA3));
	RpSetFld (rp0, RR_DIA4,    SFld(operac|ASIG_DIA4));
	RpSetFld (rp0, RR_DIA5,    SFld(operac|ASIG_DIA5));
	RpSetFld (rp0, RR_DIA6,    SFld(operac|ASIG_DIA6));
	RpSetFld (rp0, RR_DIA7,    SFld(operac|ASIG_DIA7));
	RpSetTFld(rp0, RR_HSENT,   TFld(operac|ASIG_HSENT));
	RpSetTFld(rp0, RR_HSSAL,   TFld(operac|ASIG_HSSAL));
	DoReport(rp0, LINEA2);
}

static void GuardarVigiCte()
{
	DATE franco;
	short nfranco;


	ProximoFranco (IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), SFld(operac|ASIG_VIGIL), DFld (operac|ASIG_FFRANCO), IFld(operac|ASIG_NUMFRAN), DFld (operac|ASIG_FECASIG),  &franco, &nfranco);
	
	fprintf(fp, "%ld\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%d\t%s\t%s\t%D\t%D\t%D\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%T\t%T\n",
			LFld(operac|ASIG_NROLEG),   SFld(sue|PER_APYNOM),
			LFld(operac|ASIG_CLIENTE),  SFld(bill|CLIENTE_RAZSOC),
			IFld(operac|ASIG_OBJETIVO), SFld(comerc|OBJETIVO_DESCRIP),
			SFld(operac|ASIG_VIGIL),    SFld(operac|ASIG_EFECT),
			IFld(operac|ASIG_PTOSER),   GetDescPto(IFld(operac|ASIG_PTOSER)),
			SFld(operac|ASIG_REGIM),    DFld(operac|ASIG_FECASIG), 
			DFld(operac|ASIG_FECHAS),   franco, nfranco,
			SFld(operac|ASIG_DIA1),     SFld(operac|ASIG_DIA2),
			SFld(operac|ASIG_DIA3),     SFld(operac|ASIG_DIA4),
			SFld(operac|ASIG_DIA5),     SFld(operac|ASIG_DIA6), SFld(operac|ASIG_DIA7),
			TFld(operac|ASIG_HSENT),    TFld(operac|ASIG_HSSAL));
}

static fm_status after(form fm, fmfield fno, int row)
{
	bool	efectivo;

	switch (fno) {
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
		Warning(PER_INEX, nroleg, emp);
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
			AbrirSalida("lretro1");
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
		if (salida == ARCHI)	GuardarCliObjHis();
		if (salida != ARCHI)	ImprimirCliObjHis();
		vigilador	=	LFld(operac|ASIGH_NROLEG);
		cliente		=	LFld(operac|ASIGH_CLIENTE);
		objetivo	=	IFld(operac|ASIGH_OBJETIVO);
	}
}

static void GuardarCliObjHis()
{
	fprintf(fp, "%ld\t%s\t%d\t%s\t%ld\t%s\t%s\t%s\t%d\t%s\t%s\t%D\t%D\t%D\t \t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%T\t%T\n",
			LFld(operac|ASIGH_CLIENTE), SFld(bill|CLIENTE_RAZSOC), IFld(operac|ASIGH_OBJETIVO),
			SFld(comerc|OBJETIVO_DESCRIP), LFld(operac|ASIGH_NROLEG), SFld(sue|PER_APYNOM),
			SFld(operac|ASIGH_VIGIL), SFld(operac|ASIGH_EFECT), IFld(operac|ASIGH_PTOSER),
			GetDescPto(IFld(operac|ASIGH_PTOSER)), SFld(operac|ASIGH_REGIM), DFld(operac|ASIGH_FECALT),
			DFld(operac|ASIGH_FECBAJ),NULL_DATE, SFld(operac|ASIGH_DIA1), SFld(operac|ASIGH_DIA2),
			SFld(operac|ASIGH_DIA3), SFld(operac|ASIGH_DIA4), SFld(operac|ASIGH_DIA5),
			SFld(operac|ASIGH_DIA6), SFld(operac|ASIGH_DIA7), TFld(operac|ASIGH_HSENT),
			TFld(operac|ASIGH_HSSAL));
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
	if(!IsNull(operac|ASIGH_FRANCERO) && IFld(operac|ASIGH_FRANCERO))
		RpSetFld(rp0, R_FR, "F");
	else
		RpSetFld(rp0, R_FR, NULL_STR);		

	RpSetIFld(rp0, R_PTOSER,  IFld(operac|ASIGH_PTOSER));
	RpSetIFld(rp0, R_PUESTO,  IFld(operac|ASIGH_PUESTO));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(operac|ASIGH_PTOSER)));
	RpSetFld (rp0, R_REGIM,   SFld(operac|ASIGH_REGIM));
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
		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
			continue;
		if(DFld(operac|ASIGH_FECBAJ) < DFld(operac|ASIGH_FECALT))
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
			AbrirSalida("lretro2");
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
}

static void GuardarVigiHis()
{
	fprintf(fp, "%ld\t%s\t%ld\t%s\t%d\t%s\t%s\t%s\t%d\t%s\t%s\t%D\t%D\t%D\t \t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%T\t%T\n",
			LFld(operac|ASIGH_NROLEG),   SFld(sue|PER_APYNOM),
			LFld(operac|ASIGH_CLIENTE),  SFld(bill|CLIENTE_RAZSOC),
			IFld(operac|ASIGH_OBJETIVO), SFld(comerc|OBJETIVO_DESCRIP),
			SFld(operac|ASIGH_VIGIL),    SFld(operac|ASIGH_EFECT),
			IFld(operac|ASIGH_PTOSER),   GetDescPto(IFld(operac|ASIGH_PTOSER)),
			SFld(operac|ASIGH_REGIM),    DFld(operac|ASIGH_FECALT),  DFld(operac|ASIGH_FECBAJ), NULL_DATE,
			SFld(operac|ASIGH_DIA1),     SFld(operac|ASIGH_DIA2),
			SFld(operac|ASIGH_DIA3),     SFld(operac|ASIGH_DIA4),
			SFld(operac|ASIGH_DIA5),     SFld(operac|ASIGH_DIA6), SFld(operac|ASIGH_DIA7),
			TFld(operac|ASIGH_HSENT),    TFld(operac|ASIGH_HSSAL));
}

static void ImprimirVigiHis()
{
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
	RpSetDFld(rp0, RR_FECASIG, DFld(operac|ASIGH_FECALT));
	RpSetDFld(rp0, RR_FECHAS,  DFld(operac|ASIGH_FECBAJ));
	RpSetDFld(rp0, RR_FFRANCO, NULL_DATE);
	RpSetIFld(rp0, RR_NFRANCO, NULL_SHORT);
	RpSetFld (rp0, RR_DIA1,    SFld(operac|ASIGH_DIA1));
	RpSetFld (rp0, RR_DIA2,    SFld(operac|ASIGH_DIA2));
	RpSetFld (rp0, RR_DIA3,    SFld(operac|ASIGH_DIA3));
	RpSetFld (rp0, RR_DIA4,    SFld(operac|ASIGH_DIA4));
	RpSetFld (rp0, RR_DIA5,    SFld(operac|ASIGH_DIA5));
	RpSetFld (rp0, RR_DIA6,    SFld(operac|ASIGH_DIA6));
	RpSetFld (rp0, RR_DIA7,    SFld(operac|ASIGH_DIA7));
	RpSetTFld(rp0, RR_HSENT,   TFld(operac|ASIGH_HSENT));
	RpSetTFld(rp0, RR_HSSAL,   TFld(operac|ASIGH_HSSAL));
	DoReport(rp0, LINEA2);
}


bool FechaFinValida (DATE desdefm, DATE hastafm,  DATE fdesde, DATE fhasta)
{
	fdesde = fdesde == NULL_DATE ? MIN_DATE : fdesde;	
	fhasta = fhasta == NULL_DATE ? MAX_DATE : fhasta;
	desdefm= desdefm== NULL_DATE ? MIN_DATE : desdefm;	
	hastafm= hastafm== NULL_DATE ? MAX_DATE : hastafm;	

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

	c_asig  = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);
	c_asigh	= CreateCursor(operac|ASIGHbyEMP, IO_NOT_LOCK);

	/*Leo los clientes objetivos pedidos porque puede ser que este en asig pero no en asigh */
	c_cliobj = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(c_cliobj, FmLFld(fm0, CLIDESDE), FmIsNull(fm0, OBJDESDE) ? MIN_SHORT : FmIFld(fm0, OBJDESDE));
	SetCursorTo  (c_cliobj, FmLFld(fm0, CLIHASTA), FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA));
	while (FetchCursor(c_cliobj) != ERROR) {
	
		/*Leo de la tabla asig las asignaciones actuales */
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			if(!IsNull(operac|ASIG_FECHAS) && DFld(operac|ASIG_FECHAS) < DFld(operac|ASIG_FECASIG))
				continue; 

			if(!FmIsNull(fm0, FDESDE) && 
			   !FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS)))
				continue;

			if (impri) {
				AbrirSalida("lretro1");
				impri = FALSE;
			}
			if (cliente != LFld(operac|ASIG_CLIENTE)) {
				LeerCliente(LFld(operac|ASIG_CLIENTE));
			}
			if (vigilador != LFld(operac|ASIG_NROLEG)) {
				LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
			}
			if (salida == ARCHI)	GuardarCliObjCte();
			if (salida != ARCHI)    ImprimirCliObjCte();

			vigilador = LFld(operac|ASIG_NROLEG);
			cliente   = LFld(operac|ASIG_CLIENTE);
			objetivo  = IFld(operac|ASIG_OBJETIVO);
		} 


		SetCursorFrom(c_asigh,  FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , 
							MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET) , 
							MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
							
		while (FetchCursor(c_asigh) != ERROR) {
			if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
				continue;
	
			if (DFld(operac|ASIGH_FECBAJ) < DFld(operac|ASIGH_FECALT))
				continue; 

			if(!FmIsNull(fm0, FDESDE) && 
			   !FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIGH_FECALT), DFld(operac|ASIGH_FECBAJ )))
				continue;

			if (impri) {
				AbrirSalida("lretro1");
				impri	=	FALSE;
			}
			if (cliente != LFld(operac|ASIGH_CLIENTE)) {
				LeerCliente(LFld(operac|ASIGH_CLIENTE));
			}
			if (vigilador != LFld(operac|ASIGH_NROLEG)) {
				LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
			}
			if (salida == ARCHI)	GuardarCliObjHis();
			if (salida != ARCHI)	ImprimirCliObjHis();
			vigilador	=	LFld(operac|ASIGH_NROLEG);
			cliente		=	LFld(operac|ASIGH_CLIENTE);
			objetivo	=	IFld(operac|ASIGH_OBJETIVO);
		}
		
	}		
}

static void ListaTotVigi ()
{
	long vigilador = NULL_LONG, cliente = NULL_LONG;
	int  objetivo  = NULL_SHORT;
	dbcursor c_asig, c_per, c_asigh;  

	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGDESDE));
	SetCursorTo  (c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGHASTA));
	while (FetchCursor(c_per) != ERROR) {

/*****************************************************
		if (IFld(sue|PER_RELACION) != CONVENIO)
			continue;
*******************************************************/

		c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {

			if(!FmIsNull(fm0, FDESDE) && 
			   !FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS)))
				continue;

			if (impri) {
				AbrirSalida("lretro2");
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

		c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);
		SetCursorFrom(c_asigh, FmIFld(fm0, EMP),  LFld(sue|PER_NROLEG), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_asigh, FmIFld(fm0, EMP),  LFld(sue|PER_NROLEG), MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_asigh) != ERROR) {
	  		if (IFld(operac|ASIGH_MOTIVO) == ALTAPARTE || IFld(operac|ASIGH_MOTIVO) == DESXERROR)
				continue;

			if(!FmIsNull(fm0, FDESDE) && 
			   !FechaFinValida (FmDFld(fm0, FDESDE), FmDFld(fm0, FHASTA), DFld(operac|ASIGH_FECALT), DFld(operac|ASIGH_FECBAJ )))
				continue;
		
			if (impri) {
				AbrirSalida("lretro2");
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
}

void ProximoFranco (short emp, long nroleg, char * vigil, DATE afranco, short numfran, DATE fasig, DATE *ffranco, short *nfran)
{
	/*Esta funcion devuelve la primer fecha de franco apartir de hoy
	  y el numero de franco que es */

	DATE franco = Today (),
		 faux   = Today () -5; //Esto esta por si ya viene de franco
	short num=1;
	bool esfranco=FALSE;

	if (afranco == NULL_DATE) {
		*ffranco = NULL_DATE;
		*nfran = NULL_SHORT;
		return ;
	}

	if (afranco == franco) {
		*ffranco = franco;
		*nfran = numfran;
		return ;
	}

	while (!(esfranco=Franco(emp, nroleg, faux, vigil, numfran)) || faux < franco) {
		if (esfranco && faux >= fasig) //Ademas de ser franco debe ser de esta asignacion
			num ++;
		else
			num = 1;
		faux ++;
	}

	*ffranco = faux;
	if (numfran == NULL_SHORT)
		*nfran = NULL_SHORT; //Solo se muestra el numero si tiene mas de un dia de franco
	else
		*nfran = num;
}


