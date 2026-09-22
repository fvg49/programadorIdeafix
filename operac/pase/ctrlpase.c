/********************************************************************
*
* MODULE & VERSION : @(#)ctrlpase.c	1.2
* DATE             : 22/01/05
* TIME             : 09:40:41
*
* CREATED          : 11/05/2016
*
* DESCRIPTION:
*    Reporte de control del pase de horas a  RR.HH. a Meta4
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*
*
*
*
*
*********************************************************************/
#include <ideafix.h>
#include "webinter.sch"
#include "sue.sch"
#include "operac.sch"
#include "ctrlpase.rph"
#include "ctrlpase.fmh"

/* Funciones Globales */
void AbrirEsquemas();
static fm_status after(form fm, fmfield fno, int row);
void AbrirFormulario();
void Proceso();
void AbrirSalida();
void CerrarReportes();
void RecopilarDatos();
void ImprimirReporte(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros);
void ImprimirReporteByPrinter(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros);
void ImprimirReporteByFile(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros);
long ArmarNumeroDeLiquidacion();
bool ExisteLiquidacion(long pNroliq);
void Mensaje(char * pMensaje);


#define DEF_PROSEGURIDAD 40

/* Variables Globales */  
report rp;
form fm0;
FILE *farchivo=NULL;
schema sc_webinter, sc_sue, sc_operac;

wcmd(ctrlpase, 1.2 01/05/22 )
{
	AbrirEsquemas();
	AbrirFormulario();
	if(DoForm(fm0, NULLFP, after) != FM_EXIT)
		Proceso();
}

void AbrirEsquemas() 
{
	sc_webinter = OpenSchema("webinter", IO_NOT_LOCK);
	sc_sue = OpenSchema("sue", IO_NOT_LOCK);
	sc_operac = OpenSchema("operac", IO_NOT_LOCK);
}

void AbrirFormulario() 
{
	fm0 = OpenForm("ctrlpase", FM_EABORT);  
}

void Proceso() 
{
	AbrirSalida();
	RecopilarDatos();
	CerrarReportes();
	WiMsg("El reporte ha finalizado");
}

void AbrirSalida() 
{
	if(*FmSFld(fm0, SALIDA) == 'A') {
		if ((farchivo=fopen(FmSFld(fm0, ARCHIVO), "w")) == NULL)
			Error("No se pudo generar el archivo");
		fprintf(farchivo, "Cod. Empresa\tDescripcion Empresa\tNumero de Liquidacion\tFecha desde\tFecha Hasta\tCantidad de Registros\n");
	}
	else {
		rp = OpenReport("ctrlpase", RP_EABORT|RP_NOBEGIN);
		
		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp, RP_IO_DEFAULT, NULL_STR);
		//Si la salida es Terminal
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp, RP_IO_TERM, NULL_STR);
	    
	    BeginReport(rp,1,NULL_STR);
		RpSetLFld(rp, RNROLIQ, FmLFld(fm0, NROLIQ));
		DoReport(rp, HEAD);
	}
}

void CerrarReportes()
{
	if(*FmSFld(fm0, SALIDA) == 'A')
		fclose(farchivo);
	else {
		DoReport(rp, FIN);
		CloseReport(rp);
	}
		
}

void RecopilarDatos() 
{
    //primary key (emp, nroliq, nroleg, fecasi, cliente, objetivo, ptoser, puesto, codint)
    long mNroliq = FmLFld(fm0, NROLIQ);
    int mEmpresa = 0;
    int mCountRows=0;
    DATE mFechaDesde = Today();
    DATE mFechaHasta = Today();
    char strMensaje[30];
    //schema c_emps;
    int mCotici =0;
    int  mEmpresaAuxiliar =0;
    DATE mFechaInicioProcesoAuxiliar = StrToD("01/01/2000");
    TIME mHoraInicioProcesoAuxiliar = StrToT("00:00");
    bool mFlagEsElUltimo = FALSE;
    bool mFlagEsLaPrimeraVez = FALSE;
    bool mFlagDebeImprimir = FALSE;
    dbcursor c_liquida;
    
	if(ExisteLiquidacion(mNroliq)) {
		
		//primary key (emp, cotici, fechad, fechah, feinpr, hoinpr)
		c_liquida = CreateCursor(sc_webinter|WCABLIQbyEMP, IO_NOT_LOCK);
		SetCursorFrom(c_liquida, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE, MIN_DATE, MIN_TIME);
		SetCursorTo(  c_liquida, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE, MAX_DATE, MAX_TIME);	
		strcpy(strMensaje, NULL_STR);

		mFechaInicioProcesoAuxiliar = StrToD("01/01/2000");
		mHoraInicioProcesoAuxiliar = StrToT("00:00"); 
        mEmpresaAuxiliar =0;
	    
		while (FetchCursor(c_liquida) != ERROR) {
		
			if(LFld(sc_webinter|WCABLIQ_NROLIQ) != mNroliq)
				continue;

			if(LFld(sc_webinter|WCABLIQ_COTICI) != FmIFld(fm0, COTICI) )
				continue;			
			
			if(DFld(sc_webinter|WCABLIQ_FECHAD) != FmDFld(fm0, FECHAD))
				continue;

			if(DFld(sc_webinter|WCABLIQ_FECHAH) != FmDFld(fm0, FECHAH))
				continue;		
			
			
			if(mFlagEsElUltimo && mEmpresaAuxiliar != IFld(sc_webinter|WCABLIQ_EMP))
				ImprimirReporte(mEmpresa, mFechaDesde, mFechaHasta, mCountRows);
			
			mFlagEsElUltimo = FALSE;
			mFlagEsLaPrimeraVez = FALSE;
			if(mEmpresaAuxiliar != IFld(sc_webinter|WCABLIQ_EMP))
			{
				if(mEmpresaAuxiliar == 0) {
					mFlagEsLaPrimeraVez = TRUE;
				}
				
				mEmpresaAuxiliar = IFld(sc_webinter|WCABLIQ_EMP);
				mFechaInicioProcesoAuxiliar = StrToD("01/01/2000");
				mHoraInicioProcesoAuxiliar = StrToT("00:00");
				mFlagEsElUltimo=TRUE;
			}else
			{
				if( mFechaInicioProcesoAuxiliar < DFld(sc_webinter|WCABLIQ_FEINPR)) {
					mFechaInicioProcesoAuxiliar = DFld(sc_webinter|WCABLIQ_FEINPR);
					mHoraInicioProcesoAuxiliar = TFld(sc_webinter|WCABLIQ_HOINPR);
					mFlagEsElUltimo=TRUE;
				}else {
					if( mFechaInicioProcesoAuxiliar == DFld(sc_webinter|WCABLIQ_FEINPR)) {
						if(mHoraInicioProcesoAuxiliar < TFld(sc_webinter|WCABLIQ_HOINPR)) {
							mHoraInicioProcesoAuxiliar = TFld(sc_webinter|WCABLIQ_HOINPR);
							mFlagEsElUltimo=TRUE;
						}
					}
				}
			}
			
			
			if(mFlagEsElUltimo)
			{
				mEmpresa = IFld(sc_webinter|WCABLIQ_EMP);
				mCountRows=LFld(sc_webinter|WCABLIQ_CANTIREG);
				mFechaDesde = DFld(sc_webinter|WCABLIQ_FECHAD);
				mFechaHasta = DFld(sc_webinter|WCABLIQ_FECHAH);
				mCotici = IFld(sc_webinter|WCABLIQ_COTICI);
				mFechaInicioProcesoAuxiliar = DFld(sc_webinter|WCABLIQ_FEINPR);
				mHoraInicioProcesoAuxiliar = TFld(sc_webinter|WCABLIQ_HOINPR);				
			}


			strcpy(strMensaje, NULL_STR);
			sprintf(strMensaje, "Procesando Empresa: %d", mEmpresa);

			Mensaje(strMensaje);

		}
		//Imprimie el ultimo
		ImprimirReporte(mEmpresa, mFechaDesde, mFechaHasta, mCountRows);
		DeleteCursor(c_liquida);
	}else {
		Error("El numero de liquidacion %ld no existe!!", FmLFld(fm0, NROLIQ));
	}
}

void ImprimirReporte(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros)
{
	if(*FmSFld(fm0, SALIDA) != 'A') {

//		WiMsg("Llego: %d %.3D %.3D %ld", pEmpresa, pFechaDesde, pFechaHasta, pCantidadDeRegistros);

		ImprimirReporteByPrinter(pEmpresa, pFechaDesde, pFechaHasta, pCantidadDeRegistros);
	}
	else
		ImprimirReporteByFile(pEmpresa, pFechaDesde, pFechaHasta, pCantidadDeRegistros);
}

void ImprimirReporteByPrinter(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros) 
{
	RpSetIFld(rp, REMP, pEmpresa);
	SetKey(sc_sue|EMPSbyEMP, pEmpresa);
	if(GetRecord(sc_sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		RpSetFld(rp, RRAZSOC, SFld(sc_sue|EMPS_DESCRIP));
	}
	RpSetDFld(rp, RFECDES, pFechaDesde);
	RpSetDFld(rp, RFECHAS, pFechaHasta);
	RpSetLFld(rp, RCANTIDAD, pCantidadDeRegistros);
	
	DoReport(rp, LINEA);
}

void ImprimirReporteByFile(int pEmpresa, DATE pFechaDesde, DATE pFechaHasta, long pCantidadDeRegistros) 
{
	fprintf(farchivo, "%d\t", pEmpresa);

	SetKey(sc_sue|EMPSbyEMP, pEmpresa);
	if(GetRecord(sc_sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		fprintf(farchivo,"%s\t", SFld(sc_sue|EMPS_DESCRIP));
	}else {
		fprintf(farchivo, "\t\t");
	}
	
	fprintf(farchivo, "%.3D\t", pFechaDesde);
	fprintf(farchivo, "%.3D\t", pFechaHasta);
	fprintf(farchivo, "%ld\n", pCantidadDeRegistros);
}

long ArmarNumeroDeLiquidacion() 
{
	long numeroDeLiquidacion = 0;
	
	/*ACHIMURIS*/
	/* Aca agregar el número de liquidacion que es un calculado */
	//primary key (emp, anoper, tipcie, numper)
	
	//TODO: Se podria mejorar lo hardcodear solo por la empresa 40
	
	SetKey(sc_operac|PERIODObyEMP, DEF_PROSEGURIDAD, FmIFld(fm0, ANIO), FmIFld(fm0, COTICI), FmIFld(fm0, PERIOD));
	if(GetRecord(sc_operac|PERIODObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		/*Armar número de liquidación*/
		//YYYYMMTT donde YYYY = año MM mes y TT el tipo
		//WiMsg("%.3D", DFld(operac|PERIODO_FECHAS));
		numeroDeLiquidacion = FmIFld(fm0, ANIO) * 10000 + Month(DFld(sc_operac|PERIODO_FECHAS)) * 100 + FmIFld(fm0, COTICI);
		//FmSetLFld(fm0, NROLIQ, numeroDeLiquidacion);
	}	
	
	return numeroDeLiquidacion;
}


void Mensaje(char * pMensaje)
{
	FmSetFld(fm0, COMENTA, pMensaje);
	WiRefresh();
}

bool ExisteLiquidacion(long pNroliq)
{
	return TRUE;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case PERIOD :
		FmSetLFld(fm, NROLIQ, ArmarNumeroDeLiquidacion());
		break;
	}
	return FM_OK;
}

