/********************************************************************
*
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : 11/04/2012
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
#include "l_ipccli.fmh"

/* Funciones Globales */
void AbrirEsquemas();
void AbrirFormulario();
void Proceso();
void AbrirSalida();
void CerrarReportes();
void RecopilarDatos();
void ImprimirReporte();
void ImprimirReporteByPrinter();
void ImprimirReporteByFile();

/* Variables Globales */  
report rp;
form fm;
FILE *farchivo=NULL;
schema comgral, bill;   
dbcursor c_ipccli;

wcmd(l_ipccli, %I% %G% )
{
	AbrirEsquemas();
	AbrirFormulario();
	if(DoForm(fm, NULLFP, NULLFP) != FM_EXIT)
		Proceso();
}

void AbrirEsquemas() 
{
	comgral = OpenSchema("comgral", IO_NOT_LOCK);
	bill = OpenSchema("bill", IO_NOT_LOCK);
}
void AbrirFormulario() 
{
	fm = OpenForm("l_ipccli", FM_EABORT);  
}
void Proceso() 
{
	AbrirSalida();
	RecopilarDatos();
	CerrarReportes();
}

void AbrirSalida() 
{
	if(*FmSFld(fm, SALIDA) == 'A') {
		if ((farchivo=fopen(FmSFld(fm, ARCHIVO), "w")) == NULL)
			Error("No se pudo generar el archivo");
	}
	else {
		rp = OpenReport("l_ipccli", RP_EABORT|RP_NOBEGIN);
		if (*FmSFld(fm, SALIDA) == 'I')
			RpSetOutput(rp, RP_IO_DEFAULT, NULL_STR);
		//Si la salida es Terminal
		if (*FmSFld(fm, SALIDA) == 'T')
			RpSetOutput(rp, RP_IO_TERM, NULL_STR);
	}
}

void CerrarReportes()
{
	if(*FmSFld(fm, SALIDA) == 'A')
		fclose(farchivo);
	else
		CloseReport(rp);
}

void RecopilarDatos() {
    
    //primary key (cliente, fechavig);
	c_ipccli = CreateCursor(comgral|IPCCLIbyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(c_ipccli, FmLFld(fm, CLID), FmDFld(fm, FDESDE));
	SetCursorTo  (c_ipccli, FmLFld(fm, CLIH), FmDFld(fm, FHASTA));
	
	while(FetchCursor(c_ipccli) != ERROR) {
		ImprimirReporte();
	}
	
	DeleteCursor(c_ipcli);
}

void ImprimirReporte() {
	if(*FmSFld(fm, SALIDA) == 'A')
		ImprimirReporteByPrinter();
	else
		ImprimirReporteByFile();
}

void ImprimirReporteByPrinter() {
	
}
void ImprimirReporteByFile() {
	
}
