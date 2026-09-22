/********************************************************************
*
* MODULE & VERSION : %W% 
* DATE             : %E% 
* TIME             : %U% 
*
* CREATED          : 10/11/06
*
* DESCRIPTION:
*      Procesos de interface de vacaciones a Plan Vacacional de Denarius
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "pasevac.fmh"
#include "pasevac.rph"
#include "operac.h"
#include "comerc.h"
#include "operac.sch"
#include "sue.sch"
#include "plv.sch"

/* Funciones privadas */
//static fm_status after(form fm, fmfield fno, int row);
static int  PeriodoVacacAbierto(int emp);
static int  CantDiasPeriodo(int emp, long nroleg, int periodo);
static void	Imprimir(int emp, long nroleg, int periodo, DATE fdesde, int diasper, bool pase);
static void AbrirReporte();
static bool ConvenioOk(int emp, long nroleg);

/* Declaraciones globales */
form   fm0;
report rp0 = ERROR;
schema ope, sue, plv;
dbcursor c_vacac;
int convemp, convper;

wcmd(pasevac, %I% %G%)
{
	bool grabo = TRUE, soloprint = FALSE, convenioOk = FALSE;
	int periodo, cantdias,
	             diasper,    // Cantidad de dias de vacaciones del periodo
	             diaspen;    // Dias pendientes de goce

	sue = OpenSchema("sue",    IO_EABORT);
	plv = OpenSchema("plv",    IO_EABORT);
	ope = OpenSchema("operac", IO_EABORT);

	fm0 = OpenForm("pasevac",  FM_EABORT);

	if (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

	c_vacac = CreateCursor(ope|VACACbyEMP, IO_NOT_LOCK);

	BeginTransaction();

	periodo = PeriodoVacacAbierto(FmIFld(fm0, EMP));

	SetCursorFrom(c_vacac, FmIFld(fm0, EMP), MIN_LONG, MIN_DATE);
	SetCursorTo  (c_vacac, FmIFld(fm0, EMP), MAX_LONG, MAX_DATE);
	while (FetchCursor(c_vacac) != ERROR) {
		if (DFld(ope|VACAC_FDESDE) < FmDFld(fm0, FDESDE) ||
			DFld(ope|VACAC_FDESDE) > FmDFld(fm0, FHASTA))
			continue;

		//No existen dias pendientes de dos periodos atras.
		//Por ejemplo, si el periodo abierto es 2006 no quedan dias pendientes de 2004.
		if (IFld(ope|VACAC_PERIODO) < periodo - 1)
			continue;

		convenioOk = ConvenioOk(FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG));
		if (!convenioOk)
			continue;

		diaspen = 0;
		diasper = CantDiasPeriodo(FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), IFld(ope|VACAC_PERIODO));

		//Solo grabo en PLV convenio de vigilancia para PROSEGUR y petrolero y refineria para SAPESA.

		SetKey(plv|LEGANIObyEMP, FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), periodo);
		if (GetRecord(plv|LEGANIObyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR || 
			(IFld(plv|LEGANIO_LICTOT) - IFld(plv|LEGANIO_LICTOM) > 0)) {
			grabo     = TRUE;
			soloprint = FALSE;
			
			diaspen   = IFld(plv|LEGANIO_LICTOT) - IFld(plv|LEGANIO_LICTOM);

			//No tiene dias pendientes del periodo anterior.
			if (IFld(ope|VACAC_PERIODO) < periodo && IFld(plv|LEGANIO_LICANT) == 0) {
				grabo = FALSE;
			}
					
			//Vienen vacaciones del periodo actual, quedando vacaciones pendientes de periodos anteriores.
			//Solo se imprimira para informar del caso pero no se grabara en PLV.
			if (diaspen > IFld(plv|LEGANIO_LICANIO) && IFld(ope|VACAC_PERIODO) == periodo) {
				grabo = FALSE;
				soloprint = TRUE;
			}

			if (soloprint) {
				if (rp0 == ERROR)
					AbrirReporte();

				Imprimir(FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), IFld(ope|VACAC_PERIODO),
						 DFld(ope|VACAC_FDESDE), diasper, TRUE);
			}

			if (grabo) 
			{
				// Cuento la cantidad de dias que ya estan pedidos
				cantdias = 0;
				SetKey(plv|PEDVACbyEMP, FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), periodo, MIN_DATE);
				while (GetRecord(plv|PEDVACbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR)
					cantdias += DFld(plv|PEDVAC_FHASTA) - DFld(plv|PEDVAC_FDESDE) + 1;


  				//Grabo si faltan cubrir dias para el periodo.
				if (IFld(ope|VACAC_PERIODO) < periodo && cantdias >= IFld(plv|LEGANIO_LICANT)) 
					grabo = FALSE;

				if (IFld(ope|VACAC_PERIODO) == periodo && cantdias >= IFld(plv|LEGANIO_LICTOT))
					grabo = FALSE;

				if (grabo) 
				{
					if (rp0 == ERROR)
						AbrirReporte();

					SetDFld(plv|PEDVAC_FDESDE,  DFld(ope|VACAC_FDESDE));
					SetDFld(plv|PEDVAC_FHASTA,  DFld(ope|VACAC_FDESDE) + diasper - 1); 
					SetLFld(plv|PEDVAC_NROLIQ,  NULL_LONG);
					SetIFld(plv|PEDVAC_CONFIRM, TRUE);


					if(GetRecord(plv|PEDVACbyEMP, THIS_KEY, IO_NOT_LOCK)==ERROR || IsNull(plv|PEDVAC_NROLIQ))
					{
						Imprimir( FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), 
								  IFld(ope|VACAC_PERIODO), DFld(ope|VACAC_FDESDE), diasper, FALSE);

//						fprintf(stderr, "emp %d\tnroleg %ld\tanio %d\tfdesde %.3D\tfhasta %.3D\tconfirm %d\n", FmIFld(fm0, EMP), LFld(ope|VACAC_NROLEG), periodo, DFld(ope|VACAC_FDESDE), DFld(ope|VACAC_FHASTA), IFld(plv|PEDVAC_CONFIRM) );
						PutRecord(plv|PEDVAC);
					}
				}
			}
		}
	}
	EndTransaction();

	if (rp0 != ERROR)
		CloseReport(rp0);
	else
		Warning("No hay datos para emitir el listado.");
}

static void AbrirReporte()
{
	rp0 = OpenReport("pasevac", RP_EABORT|RP_NOBEGIN);

	BeginReport(rp0, 1, NULL_STR);

	RpSetFld (rp0, REMP,      FmSFld(fm0, DEMP));
	RpSetDFld(rp0, RSALDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, RSALHASTA, FmDFld(fm0, FHASTA));
}

static int PeriodoVacacAbierto(int emp)
{
	SetIFld(plv|PERIVACA_EMP, emp);
	SetIFld(plv|PERIVACA_COD, MIN_SHORT);
	while (GetRecord(plv|PERIVACAbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) 
	{
		if (!IFld(plv|PERIVACA_CERRADO))
			return IFld(plv|PERIVACA_COD);
	}
}

/*
static fm_status after(form fm, fmfield fno, int row)
{
	switch(fno) {
		case EMP :
//			convemp = GetConvenioPorEmp(FmIFld(fm0, EMP));
			break;
	}
	return FM_OK;
}
*/
static int CantDiasPeriodo(int emp, long nroleg, int periodo)
{
	SetIFld(plv|LEGANIO_EMP,    emp);
	SetLFld(plv|LEGANIO_NROLEG, nroleg);
	SetIFld(plv|LEGANIO_ANIO,   periodo);
	if (GetRecord(plv|LEGANIObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR)
		return IFld(plv|LEGANIO_LICANIO);

	return 0;
}

static void	Imprimir(int emp, long nroleg, int periodo, DATE fdesde, int diasper, bool pase)
{
	RpSetLFld(rp0, RNROLEG,   nroleg);
	RpSetFld (rp0, RAPYNOM,   GetNombreLeg(emp, nroleg));
	RpSetIFld(rp0, RPERIODO,  periodo);
	RpSetDFld(rp0, RFDESDE,   fdesde);
	RpSetDFld(rp0, RFHASTA,   fdesde + diasper - 1);
	RpSetIFld(rp0, RCANTDIAS, (fdesde + diasper) - fdesde);
	RpSetIFld(rp0, RDIASPER,  diasper);
	RpSetIFld(rp0, RPERANT,   pase ? pase : NULL_SHORT);
	DoReport (rp0, ZLINEA);
}

static bool ConvenioOk(int emp, long nroleg)
{
	int convenio;

	convenio = GetConvenio(emp, nroleg);

	switch(convenio) {
		case CONVENIO :
		case CONV_PETROLEO :
		case CONV_REFINERIA :
			return TRUE;
			break;
		default:
			return FALSE;
			break;
	}
}
