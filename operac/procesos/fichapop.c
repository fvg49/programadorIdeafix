/********************************************************************  
*
* MODULE & VERSION : @(#)fichapop.c	1.8
* DATE             : 22/08/03
* TIME             : 11:59:16
*
* CREATED          : 15/01/22
*
* DESCRIPTION:
*             Generar un reporte con las fichadas para informar a POPs
*             Guardamos un historico de ID que se informa a POP
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/

#include <ideafix.h>
#include "operac.sch"
#include "comerc.sch"
#include "comgral.sch"
#include "webinter.sch"
#include "operac.h"
#include "comerc.h"
#include "webinter.h"
#include "fichapop.fmh"
#include "fichapop.rph"

#define GRABA_IDENTIFICADOR_SOLO_POR_ARCHIVO 1 //Este flag solo graba identificador por archivo. Si se invoca por terminal el identificador sigue fijo.
#define DEF_CODIGO_PAIS "PE"  //Como es Perú, los legajos son PE + 8 dígitos, se completa con 0 a la izquierda si el largo es menor de 8.
#define ERR_ARCHI    "No se pudo abrir el archivo."
#define DEF_CODIGO_SERVICIO "11"
#define DEF_OPERACION "INSERT"
#define DEF_TIPO_DE_CUADRANTE "Trabajo"
#define DEF_ORIGEN "2"
#define DEF_PARAMETRO_IDENTIFICADOR 61 //Este valor cambia por país



/* Declaraciones globales */
schema operac, comerc, comgral, webinter;
form fm0;
report rp0;
FILE *fp;

char  gCodigoCentro[50], gCodigoPuntoServicio[30], gCodigoGrupoCliente[30];
long gCodigoCliente;
char  gCodigoEmpleado[30];
char gFechaCreacion[20], gFechaMensaje[20], gDiaFin[20], gDiaInicio[20];
ulong gIdentificador = 0L;




/*Declaración de funciones locales*/
void AbrirEsquemas();
void AbrirSalida();
void Procesar(long pClienteDesde, short pObjetivoDesde, long pClienteHasta, short pObjetivoHasta);
void CerrarSalida();
void InformarReporte();
void InformarPorArchivo();
void InformarPorTerminal();

void ObtenerFechaEnFormatoFichada(DATE pFecha, TIME pHorario, char * pFormato);
void ObtenerFechaDeInicioAndFinFichada(DATE pFechaParte, TIME pFechaDesdeParte, TIME pFechaHastaParte, char * pDiaInicio, char * pDiaFin);
void ObtenerLegajoEnFormato(int pEmp, long pLegajo, char * pLegajoEnFormato);
void ObtenerCodigoCentro(long pCliente, short pObjetivo, char * pCodigoCentro);
void ObtenerGrupoCliente(long pCliente, char * pGrupoCliente);
void ObtenerCodigoPuntoServicio(long pCliente, short pObjetivo, char * pCodigoPuntoServicio);

char * LeerParNov(int p_emp, int p_cod, int p_nroren, DATE p_fecvig);
void GrabaValorIdentificador(ulong pIdentificador);
ulong ObtenerIdentificador();


/* Programa principal */
wcmd (fichapop, 1.8 08/03/22)
{
	fm_cmd cmd;
	long cli_desde, cli_hasta;
	short obj_desde, obj_hasta;
	
	fm0 = OpenForm("fichapop", FM_EABORT);
	
	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	 {
	 	switch (cmd) {
	 		case FM_UPDATE:

	 		cli_desde=FmIsNull( fm0, CLID) ? MIN_LONG : FmLFld( fm0, CLID);
	 		cli_hasta=FmIsNull( fm0, CLIH) ? MAX_LONG : FmLFld( fm0, CLIH);
	 		obj_desde=FmIsNull( fm0, OBJD) ? MIN_SHORT: FmIFld( fm0, OBJD);
	 		obj_hasta=FmIsNull( fm0, OBJH) ? MAX_SHORT: FmIFld( fm0, OBJH);
	 		AbrirEsquemas();
	 		AbrirSalida();
	 		Procesar(cli_desde, obj_desde, cli_hasta, obj_hasta);
	 		CerrarSalida();
	 		WiMsg("Reporte finalizado");
	 		break;
	 	}
	 }
}




void AbrirEsquemas()
{
	operac = OpenSchema("operac", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	comgral = OpenSchema("comgral", IO_EABORT);
	webinter = OpenSchema("webinter", IO_EABORT);
}

void AbrirSalida()
{
	 if (*FmSFld(fm0, SALIDA)=='A') 
	 {
	 	/*Abrir Archivo*/
	 	if ((fp = fopen(FmSFld(fm0, NOMARCH),"w")) == (FILE*)NULL)
	 		Error(ERR_ARCHI);
		fprintf(fp, "Cod Centro\tCod Cliente\tCod Empleado\tCod Grupo Cliente\tCod Punto Servicio\tCod Servicio\tDía fin\tId del Turno\tDía inicio\tOperacion\tTipo de Cuadrante\tOrigen\n");
	 }else {
	 	rp0 = OpenReport("fichapop", RP_EABORT|RP_NOBEGIN);
	 	RpSetOutput(rp0, RP_IO_TERM, NULL_STR );
	 	RpClearZone(rp0, ENCAB);
	 	
	 	//%head(today, r_empresa, r_clientedesde, r_objetivodesde, r_clientehasta, r_objetivodesde, r_fechadesde, r_fechahasta) before (report, page) 
	 	RpSetIFld(rp0, R_EMPRESA, FmIFld(fm0, EMP));
	 	RpSetLFld(rp0, R_CLIENTEDESDE, FmIsNull( fm0, CLID) ? MIN_LONG : FmLFld( fm0, CLID));
	 	RpSetIFld(rp0, R_OBJETIVODESDE, FmIsNull( fm0, OBJH) ? MAX_LONG : FmIFld( fm0,OBJH));
	 	RpSetLFld(rp0, R_CLIENTEHASTA, FmIsNull( fm0, CLIH) ? MAX_LONG : FmLFld( fm0, CLIH));
	 	RpSetIFld(rp0, R_OBJETIVOHASTA, FmIsNull( fm0, OBJH) ? MAX_SHORT: FmIFld( fm0, OBJH));
	 	//DoReport(rp0, ENCAB);
	 	DoReport(rp0, FICHAJE);
	 	
	 }
}

void InformarReporte()
{
	 if (*FmSFld(fm0, SALIDA)=='A') 
	 {
	 	InformarPorArchivo();
	 }else {
	 	InformarPorTerminal();
	 }	
}

void InformarPorArchivo()
{
	fprintf(fp, "%s\t", gCodigoCentro); //Codigo Centro de Trabajo
	fprintf(fp, "%ld\t", gCodigoCliente); //Número de cliente
	fprintf(fp, "%s\t", gCodigoEmpleado); //Código del Empleado
	fprintf(fp, "%s\t", gCodigoGrupoCliente); //Grupo Cliente
	fprintf(fp, "%s\t", gCodigoPuntoServicio); //Codigo de Punto de Servicio
	fprintf(fp, "%s\t", DEF_CODIGO_SERVICIO); //Código de Servicio
	fprintf(fp, "%s\t", gDiaFin); //Dia de fin
	fprintf(fp, "%lu\t", gIdentificador);
	fprintf(fp, "%s\t", gDiaInicio); //Dia de inicio
	fprintf(fp, "%s\t", DEF_OPERACION); //Operacion
	fprintf(fp, "%s\t", DEF_TIPO_DE_CUADRANTE); //Tipo de Cuadrante
	fprintf(fp, "%s\n", DEF_ORIGEN); //Origen
}

void InformarPorTerminal()
{
//	 %fichadas(r_CodigoCentro, rCodigoCliente, r_CodigoEmpleado, r_CodigoGrupoCliente, rCodigoPuntoServicior, r_CodigoServicio, r_FechaCreacion, r_FechaMensaje, r_diaInicio, r_diaFin, r_operacion, r_tipocuad, r_origen)
    DoReport(rp0, FICHADAS);
	RpSetFld(rp0, R_CODIGOCENTRO, gCodigoCentro); //Codigo Centro de Trabajo
	RpSetLFld(rp0, R_CODIGOCLIENTE, gCodigoCliente); //Número de cliente
	RpSetFld(rp0, R_CODIGOEMPLEADO, gCodigoEmpleado); //Código del Empleado
	RpSetFld(rp0, R_CODIGOGRUPOCLIENTE, gCodigoGrupoCliente); //Grupo Cliente
	RpSetFld(rp0, R_CODIGOPUNTOSERVICIO, gCodigoPuntoServicio); //Codigo de Punto de Servicio
	RpSetFld(rp0, R_CODIGOSERVICIO, DEF_CODIGO_SERVICIO); //Código de Servicio
	RpSetFld(rp0, R_DIAFIN, gDiaFin); //Dia de fin
	RpSetFld(rp0, R_DIAINICIO, gDiaInicio); //Dia de inicio
    RpSetFld(rp0, R_OPERACION,DEF_OPERACION); //Operacion
	RpSetFld(rp0, R_TIPOCUAD, DEF_TIPO_DE_CUADRANTE); //Tipo de Cuadrante
	RpSetFld(rp0, R_ORIGEN, DEF_ORIGEN); //Origen
}

void Procesar(long pClienteDesde, short pObjetivoDesde, long pClienteHasta, short pObjetivoHasta)
{
	dbcursor cparte	;
	int codigoEmpresa = 0;
	DATE fechaDesde, fechaHasta;
	DATE fechaProceso;
	TIME horarioProceso;
	long clienteObjetivoAnterior=0;
	long clienteObjetivoActual=0;
	long mCliente;
	short mObjetivo;
	
	
	fechaProceso = Today();
	horarioProceso = Hour();
	
	codigoEmpresa = FmIFld(fm0, EMP);
	fechaDesde = FmDFld(fm0, FDESDE);
	fechaHasta = FmDFld(fm0, FHASTA);
	cparte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
	
	//index dia(emp, dia, cliente, objetivo),
	//primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
	SetCursorFrom(cparte, codigoEmpresa, pClienteDesde, pObjetivoDesde, fechaDesde, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cparte, codigoEmpresa, pClienteHasta, pObjetivoHasta, fechaHasta, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	
	
	ObtenerFechaEnFormatoFichada(fechaProceso, horarioProceso, gFechaCreacion);
	ObtenerFechaEnFormatoFichada(fechaProceso, horarioProceso, gFechaMensaje);
	
	gIdentificador = ObtenerIdentificador();

	while(FetchCursor(cparte) != ERROR)
	{
		if (DFld(operac|PARTE_DIA) < fechaDesde ||  DFld(operac|PARTE_DIA) > fechaHasta)
			continue;
		
		if (TFld(operac|PARTE_HORAENT) == StrToT("00:00") && TFld(operac|PARTE_HORASAL) == StrToT("00:00") )
			continue;		
		
		mCliente = LFld(operac|PARTE_CLIENTE);
		
		//if (mCliente != 1001)
		//	continue;
		
		mObjetivo = IFld(operac|PARTE_OBJETIVO);
		clienteObjetivoActual = mCliente * 100 + mObjetivo;
		
		/*switch(clienteObjetivoActual) {
			case 395922:
			case 512477:
			case 453621:
			case 343819:
			case 473160:
			case 357835:
				break;
			default:
				continue;
		}*/
		if(clienteObjetivoAnterior != clienteObjetivoActual) {
			gCodigoCliente = mCliente;
			/*Buscar datos de Cliente objetivos*/
			ObtenerCodigoCentro(mCliente, mObjetivo, gCodigoCentro);
			ObtenerGrupoCliente(mCliente, gCodigoGrupoCliente);
			ObtenerCodigoPuntoServicio(mCliente, mObjetivo, gCodigoPuntoServicio);
			//WiMsg("%s\n%s\n%s",gCodigoCentro, gCodigoGrupoCliente, gCodigoPuntoServicio);
		}
		clienteObjetivoAnterior = clienteObjetivoActual;
		
		gIdentificador++;
		ObtenerFechaDeInicioAndFinFichada(DFld(operac|PARTE_DIA), TFld(operac|PARTE_HORAENT), TFld(operac|PARTE_HORASAL), gDiaInicio, gDiaFin);
		ObtenerLegajoEnFormato(IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG), gCodigoEmpleado);

		InformarReporte();
		
		//WiMsg("Legajo: %s", gCodigoEmpleado);
	}
	GrabaValorIdentificador(gIdentificador);
}

void ObtenerCodigoCentro(long pCliente, short pObjetivo,  char * pCodigoCentro)
{
	char codigoCentro[50];
	sprintf(codigoCentro, "%ld-%03d", pCliente, pObjetivo);
	
	sprintf(pCodigoCentro, codigoCentro);
}

void ObtenerCodigoPuntoServicio(long pCliente, short pObjetivo, char * pCodigoPuntoServicio)
{
	char codigoPuntoServicio[50];
	short codigoPuntoFijo = 1; //Por definición siempre es 1

	sprintf(codigoPuntoServicio, "%ld-%03d-%02d", pCliente, pObjetivo, codigoPuntoFijo);

	sprintf(pCodigoPuntoServicio, codigoPuntoServicio);
}

void ObtenerGrupoCliente(long pCliente, char * pGrupoCliente)
{
	char codigoGrupoCliente[30];
	long codGrupoCliente = 0;
	
	//clihijo(clihijo); 
	SetKey(comerc|RELCLIbyCLIHIJO, pCliente);
	if(GetRecord(comerc|RELCLIbyCLIHIJO, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		codGrupoCliente = LFld(comerc|RELCLI_CLIPADRE);
		sprintf(codigoGrupoCliente, "%4ld", codGrupoCliente);
	}else {
		sprintf(codigoGrupoCliente, "000000");
	}
		
	sprintf(pGrupoCliente, codigoGrupoCliente);
}

void ObtenerFechaDeInicioAndFinFichada(DATE pFechaParte, TIME pFechaDesdeParte, TIME pFechaHastaParte, char * pDiaInicio, char * pDiaFin)
{
	ObtenerFechaEnFormatoFichada(pFechaParte, pFechaDesdeParte, pDiaInicio);
	if(pFechaHastaParte>pFechaDesdeParte || (pFechaHastaParte == pFechaDesdeParte && pFechaHastaParte == StrToT("00:00"))) {
		ObtenerFechaEnFormatoFichada(pFechaParte, pFechaHastaParte, pDiaFin);
	}else {
		ObtenerFechaEnFormatoFichada((pFechaParte+1), pFechaHastaParte, pDiaFin);
	}
}

void CerrarSalida()
{
	if (*FmSFld(fm0, SALIDA)=='A') 
		fclose(fp);
	else {
		DoReport(rp0, FICHADASFIN);
		CloseReport(rp0);
	}
		
}

void ObtenerFechaEnFormatoFichada(DATE pFecha, TIME pHorario, char * pFormato)
{
	char fechaEnFormatoDeRetorno [20];
	sprintf(fechaEnFormatoDeRetorno, "%.3D %.3T", pFecha, pHorario);
	//WiMsg("A - %s", fechaEnFormatoDeRetorno);
	sprintf(pFormato,  fechaEnFormatoDeRetorno);
}


void ObtenerLegajoEnFormato(int pEmp, long pLegajo, char * pLegajoEnFormato)
{
	char legajoEnFormatoDeRetorno[11];
	char legajoM4[11];
	char empStr[3];
	int empleg = 0;
	
	SetKey(webinter|WLEGAM4byEMPACTDN, pEmp, pLegajo);
  	if (GetRecord(webinter|WLEGAM4byEMPACTDN, THIS_KEY, IO_NOT_LOCK) != ERROR) {
  		strcpy(legajoM4, SFld(webinter|WLEGAM4_LEGAGLOB));
  		strcpy(pLegajoEnFormato, &legajoM4[2]); // toma la cadena desde el 3er elemento, sacá el PE de los legajos
  	}
  	else // en caso de no encontrar ese conjunto empresa-legajo lo arma con el cross de empresas de meta4
	{
		IToStr(pEmp, empStr);
		strcpy(empStr, GetICross(_CROSS_EMP_M4, empStr, TRUE));
		empleg = StrToI(empStr);
		sprintf(legajoEnFormatoDeRetorno, "%02d%06ld", empleg, pLegajo);
		sprintf(pLegajoEnFormato, legajoEnFormatoDeRetorno);
	}
}


char * LeerParNov(int p_emp, int p_cod, int p_nroren, DATE p_fecvig)
{
	SetKey(comgral|DEPANObyEMP, p_emp, p_cod, p_nroren, p_fecvig + 1 );
	if(GetRecord(comgral|DEPANObyEMP, PARTIAL_KEY|PREV_KEY, IO_NOT_LOCK, 3)!=ERROR){
	
		if (IFld(comgral|DEPANO_ACT))
			return (SFld(comgral|DEPANO_VALOR));
		else
			return NULL_STR;

	}

	return NULL_STR;
}



ulong ObtenerIdentificador()
{
	ulong valor = 0L;
	int renglon_0 = 0;
	//char * GetParNov(int p_emp, int p_cod, int p_nroren, DATE p_fecvig)
	char valorEnTabla[9];
	if (((*FmSFld(fm0, SALIDA)=='A') && GRABA_IDENTIFICADOR_SOLO_POR_ARCHIVO) || !GRABA_IDENTIFICADOR_SOLO_POR_ARCHIVO) {
		sprintf(valorEnTabla, "%s", LeerParNov(0, (int)DEF_PARAMETRO_IDENTIFICADOR, renglon_0, Today()));
		valor = StrToL(valorEnTabla);
	}
		
	
	return valor;
}

void GrabaValorIdentificador(ulong pIdentificador)
{
	int renglon_0=0;
	char valor[20];

	if (((*FmSFld(fm0, SALIDA)=='A') && GRABA_IDENTIFICADOR_SOLO_POR_ARCHIVO) || !GRABA_IDENTIFICADOR_SOLO_POR_ARCHIVO)
	{
		SetKey(comgral|DEPANObyEMP, 0, (int)DEF_PARAMETRO_IDENTIFICADOR, renglon_0, Today() + 1 );
		if(GetRecord(comgral|DEPANObyEMP, PARTIAL_KEY|PREV_KEY, IO_NOT_LOCK, 3)!=ERROR)
		{
			sprintf(valor, "%ld", pIdentificador);
			SetFld(comgral|DEPANO_VALOR, valor);
			PutRecord(comgral|DEPANO);
		}
		
	}
}
