/********************************************************************
*
* MODULE & VERSION : @(#)gparauto.c	1.1
* DATE             : 05/12/19
* TIME             : 12:21:30
*
* CREATED          : 29/11/04
*
* DESCRIPTION:
*      Generación automática del Parte Diario.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "ambiente.h"
#include "operac.sch"
#include "sue.sch"

#define _DESTINO      "/usr91/productos/novia/gestion/etl/novedades"

void ImprimirListado();

/* Declaraciones globales */
schema operac, asist, sue;
FILE *fp = NULL;
FILE *fparte;
report rp = (report) ERROR;
dbcursor c_asig, c_asigah, c_emps;
dbtable  ALASIG, ALASIGH, APARTE, c_PARTE;
bool     conf_regen, prg_form;
char     nomarch[100], msj[100];

/* Programa principal */
Program(gparauto, 1.1 12/19/05)
{
	DATE fdesde, fhasta;

	sprintf(gg_prog, "%s", argv[0]);

	asist  = OpenSchema("asist",  IO_EABORT);
	sue    = OpenSchema("sue", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	//Para ejecutar el programa en forma automatica (gparauto.c)los parametros deben ser:
	// conf_regen = 0  //El prg no pide confirmacion de regeneracion.
	// prg_form   = 0  //El prg no se llama por form.

	if (argc < 2)
		Error("El proceso debe ser invocado con 1 para confirmar la regeneración de partes confirmados\n o 0 para que no los tenga en cuenta."); 
		
	conf_regen = (StrToI(argv[1]) == 1 ? TRUE : FALSE); 
	prg_form   = TRUE;

	if (argc > 2)
		prg_form  = (StrToI(argv[2]) == 1 ? TRUE : FALSE); 

	ALASIG  = CreateAlias(operac|ASIG);
	ALASIGH = CreateAlias(operac|ASIGH);
	APARTE  = CreateAlias(operac|PARTE);
	c_asig  = CreateCursor(AlInd(ALASIG, operac|ASIGbyNROLEG), IO_NOT_LOCK);
     
	c_emps = CreateCursor(sue|EMPSbyEMP, IO_NOT_LOCK);
	SetCursorFrom(c_emps, NULL_SHORT);
	SetCursorTo  (c_emps, MAX_SHORT);
	while (FetchCursor(c_emps) != ERROR) {

		BeginTransaction();

		fdesde = Today() + 1;
		fhasta = fdesde;

	/*	LimpiarParte(IFld(sue|EMPS_EMP), 3002, 3002, 1, 1, StrToD("20112005"), StrToD("10122005"), conf_regen, prg_form);
		GenParte(IFld(sue|EMPS_EMP), 3002, 3002, 1, 1, StrToD("20112005"), StrToD("10122005"), conf_regen, prg_form);
		GenParteH(IFld(sue|EMPS_EMP), 3002, 3002, 1, 1, StrToD("20112005"), StrToD("10122005"), conf_regen, prg_form);
	*/

		LimpiarParte(IFld(sue|EMPS_EMP), MIN_LONG, MAX_LONG, MIN_SHORT, MAX_SHORT, fdesde, fhasta, conf_regen, prg_form);
		GenParte    (IFld(sue|EMPS_EMP), MIN_LONG, MAX_LONG, MIN_SHORT, MAX_SHORT, fdesde, fhasta, conf_regen, prg_form);
		GenParteH   (IFld(sue|EMPS_EMP), MIN_LONG, MAX_LONG, MIN_SHORT, MAX_SHORT, fdesde, fhasta, conf_regen, prg_form);

		EndTransaction();

		sprintf(nomarch, "%s/%s", _DESTINO, "parteauto.txt");
		if ((fparte = fopen(nomarch, "wt")) == (FILE *)NULL) 
			Error("No se pudo generar el archivo %s", nomarch);

		c_PARTE = CreateCursor(operac|PARTEbyDIA, IO_NOT_LOCK);

		SetCursorFrom(c_PARTE, IFld(sue|EMPS_EMP), fdesde, MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_PARTE, IFld(sue|EMPS_EMP), fhasta, MAX_LONG, MAX_SHORT);
		while (FetchCursor(c_PARTE) != ERROR) {
			fprintf(fparte, "%d\t%ld\t%d\t%ld\t%.3D\t%.1T\t%.1T\t%s\t%d\t%d\t%d\n",
					IFld(operac|PARTE_EMP),     LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
					LFld(operac|PARTE_NROLEG),  DFld(operac|PARTE_DIA),     TFld(operac|PARTE_HORAENT),
					TFld(operac|PARTE_HORASAL), SFld(operac|PARTE_CONDIC),
					IFld(operac|PARTE_PTOSER),  IFld(operac|PARTE_PUESTO),  IFld(operac|PARTE_NROINT));
		}
		DeleteCursor(c_PARTE);
		fclose(fparte);

	//	if (ejecutaSP) {
			//--- Ejecuto sp para importacion de arhivos  ---//
	/*		sprintf(query, "DtsRun 'srvsqlent', 'sa','', 'ges_objetivos','','/A pais:8=%d /A empresa:8=%d /A anio:3=%d /A mes:3=%d /A archivo:8=%s'",
							paisemp, FmIFld(fm0, EMP), FmIFld(fm0, ANIO) , FmIFld(fm0, MES), nomarch);

			arSQL_Iniciar(_WEB_URL_GESNOV);
			arSQL_Ejecutar(MODO_EJECUCION_SINCRONICO, query, &resultadoEjecutar);
			if (resultadoEjecutar.codRetorno != CODIGO_RETORNO_OK && resultadoEjecutar.codRetorno != CODIGO_RETORNO_OK_DTS) {
				fprintf(fper, "%s: GenArchivoObjetivos ERROR %.3D %.3T %s resultado %d descrip %s\n", programa,Today(), Hour(), query, resultadoEjecutar.codRetorno,resultadoEjecutar.descripcion);
			}
			arSQL_Liberar();
		}
		*/
	}
	DeleteCursor(c_asig);
	DeleteCursor(c_emps);
	CloseAllSchemas();
}


