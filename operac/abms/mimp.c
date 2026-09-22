/*******************************************************************
* MODULE & VERSION : @(#)mimp.c	1.5
* DATE             : 08/04/29
* TIME             : 10:58:25
*
* CREATED          : 17/10/05
*
* DESCRIPTION:
*      Carga de puestos para improductividad.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "mimp.fmh"
#include "operac.h"
#include "comerc.sch"
#include "operac.sch"
#include "filial.h"

/* Funciones privadas */
static void Lectura();
static void Relectura();
static fm_status before(form, fmfield, int);
static fm_status after (form, fmfield, int);

static void CopiarLinea(int row);   // Copia linea de Multi a MultiD
static int  BuscarLineaD(int row);  // Busca linea en MultiD debuelve TRUE si la encuntra
static int  BuscarLinea(int lin);   // Busca linea en Multi debuelve nro de linea  si la encuntra o NULL_SHORT si no

static bool ControloFormulario();	// Agrupa distintos controles sobre correspondencia entre puestos padres-hijos
static void LimpioSinMotivo();      // Saca las lineas del multi destino que no tienen motivo
static void ActualizoModif(int row);// Si no estan los hijos pone modif en no

static void CambiarFechaPuestos(long p_cliente, int p_objet, int p_tippto, int p_codint, int row);

/* Declaraciones globales */
form fm0;
schema comerc, operac;
int NLIN, NLIN1;
char filial[7] = {'\0'};

/* Programa principal */
wcmd(mimp, 1.5 04/29/08)
{
	int linea = 0, nuevo_codint = 1;
	fm_cmd cmd;

	fm0 = OpenForm("mimp", FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT | IO_SYMBOLS);
	operac = OpenSchema("operac", IO_EABORT | IO_SYMBOLS);

	NLIN  = FmFldLen(fm0, MULTI);
	NLIN1 = FmFldLen(fm0, MULTID);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_READ:
	case FM_READ_NEXT:
	case FM_READ_PREV:
		Lectura();
		break;
	case FM_ADD:
	case FM_UPDATE:
		BeginTransaction();

		for (linea = 0; linea < NLIN1 && !FmIsNull(fm0, TIPPTOD, linea); linea++) {
			/* Busco el codint que corresponda */
			nuevo_codint = 1;
			SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, TIPPTOD, linea), MAX_SHORT);
			if (GetRecord(operac|PUESTOSbyCLIENTE, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) 
				nuevo_codint = IFld(operac|PUESTOS_CODINT) + 1;
			else
				Error("No se encuentra registro en tabla puestos, se abortara el proceso.");

			/* Seteo con el codigo interno padre el buffer para que me queden igual los campos 
			   que no muestro*/
			SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET), FmIFld(fm0, TIPPTOD, linea),
											FmIFld(fm0, CODINT));
			GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

			/*Seteo nuevo registro */
			FmToDb(fm0, CODMOT, CANTVIGD, linea);
			SetIFld(operac|PUESTOS_CODINT,  nuevo_codint);

			PutRecord(operac|PUESTOS);
		}
		EndTransaction();
		break;

	case FM_IGNORE:
		for (linea = 0; linea < NLIN1 && !FmIsNull(fm0, TIPPTOD, linea); linea++)
			FmClearFlds(fm0, CODMOT, CANTVIGD, linea);

		break;
	}
}

static void Lectura()
{
	int v_puestos[500][3];
	int v_i, v_j;

	/* Completa una matriz con los puestos que correspondan */
	LeePuestos(FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), v_puestos, FmDFld(fm0, FECHOT), TRUE);

	for (v_i = 0, v_j = 0; v_i < 500 && v_puestos[v_i][COLTIPPTO] != NULL_SHORT && v_j < NLIN; v_i++) {

		SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),	v_puestos[v_i][COLTIPPTO],
		                                                                        v_puestos[v_i][COLCODINT]);
		if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {

			if (!IsNull(operac|PUESTOS_PADREINT))
				FmSetIFld(fm0, MIMP, TRUE, v_j);
			else
				FmSetIFld(fm0, MIMP, FALSE, v_j);

			FmSetIFld (fm0, MODIF,  FALSE,   v_j);
			DbToFm    (fm0, TIPPTO, PADREINT, v_j);
			FmSetIFld(fm0, CANTPUE, v_puestos[v_i][COLCANPUE], v_j);
			FmShowFlds(fm0, TIPPTO, PADREINT, v_j);
			v_j++;
		}
	}
	FmNextFld(fm0, MODIF, 0);
}
static fm_status before(form fm, fmfield fno, int row)
{

	switch(fno) {
		case CANTVIGD :
					// Me fijo si modifique la cantidad de puestos para 
					//  dejar o no modificar la cantidad de personas.
				SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
				                                FmIFld(fm0, TIPPTOD, row), FmIFld(fm0, CODINTD, row));
				GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

				if (IFld(operac|PUESTOS_CANTPUE)==FmIFld(fm, CANTPUED, row))
					return FM_SKIP;
			
			break;
	} 
	return FM_OK;
}
static fm_status after(form fm, fmfield fno, int row)
{
	int v_linea_multi;
	int v_linea_orig = 0;
	int v_linea_dest = 0;
	int v_campo=0;
	DATE fecierre, fecierrefil;
	dbcursor c_parte;
	bool hayhoras;
	char fecaux1[12], fecaux2[12];
	bool asignado;
	int emp;
	DATE fecfm;

	sprintf(fecaux1, "%s", NULL_STR);
	sprintf(fecaux2, "%s", NULL_STR);

	switch(fno) {
		case FECHOT:
		strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET)));	
		break;
		case FFINAL:
			if (FmChgFld(fm)) {
				/* Controlo que la fecha de finalizacion  sea mayor la de inicio*/
				if (FmDFld(fm, fno, row) < FmDFld(fm, FINICIO, row) && !FmIsNull(fm, fno, row)){
					DToStr(FmDFld(fm, fno, row), fecaux1, DFMT_SEPAR);
					DToStr(FmDFld(fm, FINICIO, row), fecaux2, DFMT_SEPAR);

					Warning("La Fecha de Finalizacion %s debe\n ser posterior[1m [0ma la Fecha de Inicio %s.", fecaux1, fecaux2);
					return FM_REDO;
				}
				SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
					                                FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row));
				GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

				/*La nueva fecha debe ser menor que la que estaba */
/*				if (FmDFld(fm, fno, row)>=DFld(operac|PUESTOS_FFINAL) && !FmIsNull(fm, fno, row)) {
					DToStr(FmDFld(fm, fno, row), fecaux1, DFMT_SEPAR);
					DToStr(DFld(operac|PUESTOS_FFINAL), fecaux2, DFMT_SEPAR);

					Warning("La Fecha de Finalizacion del Mimp %s debe\n ser anterior[1m [0ma la Fecha actualmente grabada en este mimp %s.", fecaux1, fecaux2);
					FmSetDFld(fm, fno, StrToD(FmFldPrev(fm)), row);
					return FM_REDO;
				}
*/
				/* Controlo que la fecha de finalizacion sea menor a la del puesto padre */
				SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
					                            FmIFld(fm0, TIPPTO, row), IFld(operac|PUESTOS_PADREINT));
				if(GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK)==ERROR){
					WiDialog(WD_OK, WD_OK, "Error", "No se encuentra el puesto padre %ld-%d-%d-%d.",
					         FmLFld(fm0, CLIE), FmIFld(fm0, OBJET), FmIFld(fm0, TIPPTOD, row),
					         IFld(operac|PUESTOS_CODINT));
					FmSetDFld(fm, fno, StrToD(FmFldPrev(fm)), row);
					return FM_REDO;
				}
				if (FmDFld(fm, fno, row) > DFld(operac|PUESTOS_FFINAL) && !IsNull(operac|PUESTOS_FFINAL)) {
					DToStr(FmDFld(fm, fno, row), fecaux1, DFMT_SEPAR);
					DToStr(DFld(operac|PUESTOS_FFINAL), fecaux2, DFMT_SEPAR);

					Warning("La Fecha de Finalizacion del Mimp %s debe\n ser anterior[1m [0ma la Fecha de Finalizacion del Puesto Padre %s.", fecaux1, fecaux2);
					FmSetDFld(fm, fno, StrToD(FmFldPrev(fm)), row);
					return FM_REDO;
					
				}

					/*Pregunto y grabo fecha nueva*/
				if (WiDialog(WD_OK|WD_CANCEL, WD_CANCEL, "Fecha de Finalizacion de MIMP",
				             "Seguro que Desea Cambiar la Fecha de Finalizacion de este Mimp?")==WD_OK) {

					SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
					                                FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row));
					GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

					asignado = PuestoAsignado(FmIFld(fm0, EMP), FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
					                          FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row),
					                          FmDFld(fm0, FFINAL, row));

					if ((asignado && WiDialog(WD_OK|WD_NO, WD_NO, "Aviso", 
					                 "El Puesto %d-%d Esta Asignado despues del %.3D \nDesea Continuar",
					                 FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row), 
					                 FmDFld(fm0, FFINAL, row))==WD_OK) ||
					     !asignado) {

						PushRecord(operac|PUESTOS);
					   	CambiarFechaPuestos(FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
						                    FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row), row);
						PopRecord (operac|PUESTOS);

						SetDFld(operac|PUESTOS_FFINAL, FmDFld(fm0, FFINAL, row));
						PutRecord(operac|PUESTOS);

						emp=FmIFld(fm0, EMP);
						fecfm = FmDFld(fm0, FECHOT);

						Relectura();
					}
					else
						FmSetDFld(fm, fno, StrToD(FmFldPrev(fm)), row);
				}
				else
					FmSetDFld(fm, fno, StrToD(FmFldPrev(fm)), row);
			}
			break;
		case MODIF:
			if (FmChgFld(fm)) {

				// Busco lineas que esta en DESTINO y no en Origen, las borro y reacomodo multi DESTINO
				for(v_linea_multi=0; v_linea_multi<NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_multi); v_linea_multi++){
					v_linea_orig=BuscarLinea(v_linea_multi);
					if (v_linea_orig==NULL_SHORT || !FmIFld(fm, MODIF, v_linea_orig)) {
						FmClearFlds(fm0, CODMOT, CANTVIGD, v_linea_multi);
						for(v_linea_dest=v_linea_multi+1; v_linea_dest<NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_dest); v_linea_dest++){
							for (v_campo=CODMOT; v_campo<=CANTVIGD; v_campo++) {
								FmSetFld(fm0, v_campo, FmSFld(fm0, v_campo, v_linea_dest + 1), v_linea_dest);
							}
							FmClearFlds(fm0, CODMOT, CANTVIGD, v_linea_dest + 1);
						}
					}
					FmShowFlds(fm0, CODMOT, CANTVIGD, v_linea_multi);
				}

				// Busco linea que estan en ORIGEN y no en DESTINO y las copio a DESTINO
				for(v_linea_multi=0; v_linea_multi<NLIN  && !FmIsNull(fm0, MODIF, v_linea_multi);   v_linea_multi++) {
					if (FmIFld(fm, MODIF, v_linea_multi)) {
						/* Copio si no existe la linea Destino */
						if (BuscarLineaD(v_linea_multi)==NULL_SHORT)
							CopiarLinea(v_linea_multi);
					}
				} 

			}
			break;
		case CODMOT:
			/* Si no es nulo el motivo y estoy insertando una linea copio el registro original para
		   modificarlo */
			if (!FmIsNull(fm, fno, row)){
				if (FmIsNull(fm, TIPPTOD, row) && row>0 && !FmIsNull(fm, TIPPTOD, row-1)) {
					v_linea_orig = BuscarLinea(row-1);
					CopiarLinea(v_linea_orig);
				}
			}
			break;
		case FINICIOD:

			if (!FmIsNull(fm, fno, row) && FmKeyCode(fm0) != K_DEL) {
				v_linea_orig = BuscarLinea(row);

				/* Controlo que la fecha de inicio nueva  sea mayor la del puesto original*/
				if (v_linea_orig != NULL_SHORT && FmDFld(fm0, fno, row) < FmDFld(fm0, FINICIO, v_linea_orig)){
					DToStr(FmDFld(fm0, fno, row), fecaux1, DFMT_SEPAR);
					DToStr(FmDFld(fm0, FINICIO, v_linea_orig), fecaux2, DFMT_SEPAR);

					Warning("La Fecha de Inicio %s del nuevo puesto debe\n ser posterior[1m [0ma la Fecha de Inicio %s del puesto original.", fecaux1, fecaux2);
					return FM_REDO;
				}
			}

			/* Controlo que la fecha de cierre no caiga despues de la de inicio nueva */
			fecierre = GetFechaCierreOpe(FmIFld(fm0, EMP));
			if (fecierre != NULL_DATE && fecierre > FmDFld(fm0, fno, row) && FmKeyCode(fm0) != K_DEL) {
				DToStr(fecierre, fecaux1, DFMT_SEPAR);
				DToStr(FmDFld(fm0, fno, row), fecaux2, DFMT_SEPAR);

				Warning("La Fecha de Inicio %s debe ser posterior a la Fecha de Cierre Mensual %s.", fecaux2, fecaux1);
				return FM_REDO;
			} 
			
			fecierrefil = GetFechaCierreFilial(filial);
            if (fecierrefil != NULL_DATE && FmDFld(fm0, fno, row) <= fecierrefil) {
				Warning("La fecha de Inicio %.3D debe ser posterior a la Fecha de Cierre %.3D de la Filial %s", FmDFld(fm0, fno, row), fecierrefil, filial);
				return FM_REDO;
			}
			
			break;
		case FFINALD:

			if (FmIsNull(fm, fno, row ))
				FmSetDFld(fm0, fno, FmDFld(fm0, FFINAL, v_linea_orig), row);

			if (!FmIsNull(fm, fno, row )) {
				/* Controlo que la fecha de finalizacion sea mayor a la de inicio */
				if(FmDFld(fm, fno, row)<FmDFld(fm, FINICIOD, row) ) {
					DToStr(FmDFld(fm0, fno, row), fecaux1, DFMT_SEPAR);
					DToStr(FmDFld(fm0, FINICIOD, row), fecaux2, DFMT_SEPAR);

    				Warning("La Fecha de Fin %s[1m [0mdel nuevo puesto debe\n ser posterior  a la Fecha de Inicio %s del nuevo puesto.",
								 fecaux1, fecaux2);
					FmSetDFld(fm0, FINICIOD, NULL_DATE, row);
					FmSetDFld(fm0, FFINALD, NULL_DATE, row);
					FmNextFld(fm0, FINICIOD, row);
				}

				/* Controlo que la fecha de finalizacion nueva sea menor a la original */
				v_linea_orig = BuscarLinea(row);
				if (v_linea_orig != NULL_SHORT && !FmIsNull(fm0, FFINAL, v_linea_orig))
					if (FmDFld(fm0, fno, row) > FmDFld(fm0, FFINAL, v_linea_orig)) {
						DToStr(FmDFld(fm0, fno, row), fecaux1, DFMT_SEPAR);
						DToStr(FmDFld(fm0, FFINAL, v_linea_orig), fecaux2, DFMT_SEPAR);

						Warning("La Fecha de Fin %s[1m [0mdel nuevo puesto debe\n ser anterior a la Fecha de Fin %s del puesto original.",
								 fecaux1, fecaux2);
						FmSetDFld(fm0, FINICIOD, NULL_DATE, row);
						FmSetDFld(fm0, FFINALD, NULL_DATE, row);
						FmNextFld(fm0, FINICIOD, row);
						return FM_OK;
					}
			}

			/* Me fijo que no tenga horas cargadas */
			hayhoras = FALSE;
			c_parte  = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
			SetCursorFrom(c_parte, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
						  FmDFld(fm0, FINICIOD, row), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, EMP), FmLFld(fm0, CLIE), FmIFld(fm0, OBJET),
						  FmDFld(fm0, FFINALD, row), MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
			while (FetchCursor(c_parte) != ERROR) {
				if (LFld(operac|PARTE_HSNOR)  > 0 || LFld(operac|PARTE_HS50)    > 0 ||
					LFld(operac|PARTE_HS100F) > 0 || LFld(operac|PARTE_HS100FE) > 0) {
					hayhoras = TRUE;
					break;
				}
			}
			DeleteCursor(c_parte);


			if (hayhoras) {
				DToStr(DFld(operac|PARTE_DIA), fecaux1, DFMT_SEPAR);
				Warning("Existen horas cargadas para el puesto nuevo el día %s.", fecaux1);

				FmSetDFld(fm0, FINICIOD, NULL_DATE, row);
				FmSetDFld(fm0, FFINALD, NULL_DATE, row);
				FmNextFld(fm0, FINICIOD, row);

				return FM_OK;
			}

			break;

		case HINICIOD:
			if (!FmIsNull(fm, fno, row) && FmKeyCode(fm0) != K_DEL) {
				v_linea_orig = BuscarLinea(row);

				if (FmTFld(fm, HINICIO, v_linea_orig) >  FmTFld(fm, HFINAL, v_linea_orig)) {
					/*Empieza un dia y termina al otro*/

					if (v_linea_orig != NULL_SHORT){
						/* Controlo que la hora de inicio nueva  sea mayor que la del puesto original*/
						if (FmTFld(fm0, fno, row) > FmTFld(fm, HFINAL, v_linea_orig) && 
						    FmTFld(fm0, fno, row) < FmTFld(fm, HINICIO, v_linea_orig)){

							WiDialog(WD_OK, WD_OK, "Error", "La Hora desde(%.3T) esta fuera del horario del padre(%.3T - %.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HINICIO, v_linea_orig), FmTFld(fm, HFINAL, v_linea_orig));
							return FM_REDO;
						}
					}
					
				}
				else {
					/*Empieza y termina el mismo dia*/

					if (v_linea_orig != NULL_SHORT){
						/* Controlo que la hora de inicio nueva  sea mayor que la del puesto original*/
						if (FmTFld(fm0, fno, row) < FmTFld(fm, HINICIO, v_linea_orig)){

							WiDialog(WD_OK, WD_OK, "Error", "La Hora desde(%.3T) del hijo no puede ser menor que la del padre(%.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HINICIO, v_linea_orig));
							return FM_REDO;
						}
						/* Controlo que la hora de inicio nueva  sea menor que la hora final del puesto original*/
						if (FmTFld(fm0, fno, row) > FmTFld(fm, HFINAL, v_linea_orig)){

							WiDialog(WD_OK, WD_OK, "Error", "La Hora desde(%.3T) del hijo no puede \nser mayor que la hora de finalizacion del padre(%.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HFINAL, v_linea_orig));
							return FM_REDO;
						}
					}
				} 
			}

			break;
		case HFINALD:
			if (!FmIsNull(fm, fno, row) && FmKeyCode(fm0) != K_DEL) {
				v_linea_orig = BuscarLinea(row);

				if (FmTFld(fm, HINICIO, v_linea_orig) >  FmTFld(fm, HFINAL, v_linea_orig)) {
					/*Empieza un dia y termina al otro*/
					if (v_linea_orig != NULL_SHORT){
						/* Controlo que la hora de inicio nueva  sea mayor que la del puesto original*/
						if (FmTFld(fm0, fno, row) > FmTFld(fm, HFINAL, v_linea_orig) && 
						    FmTFld(fm0, fno, row) < FmTFld(fm, HINICIO, v_linea_orig)){

							WiDialog(WD_OK, WD_OK, "Error", "La Hora hasta(%.3T) esta fuera del horario del padre(%.3T - %.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HINICIO, v_linea_orig), FmTFld(fm, HFINAL, v_linea_orig));
							return FM_REDO;
						}
					}
				}
				else {
					/*Empieza y termina el mismo dia*/

					/* Controlo que la hora de fin nueva sea menor la del puesto original*/
					if (v_linea_orig != NULL_SHORT){
						if(FmTFld(fm0, fno, row) > FmTFld(fm, HFINAL, v_linea_orig)){
							WiDialog(WD_OK, WD_OK, "Error", "La Hora hasta(%.3T) del hijo no puede ser mayor que la del padre(%.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HFINAL, v_linea_orig));
							return FM_REDO;
						}
						if(FmTFld(fm0, fno, row) < FmTFld(fm, HINICIO, v_linea_orig)){
							WiDialog(WD_OK, WD_OK, "Error", "La Hora hasta(%.3T) del hijo no puede ser menor que la de comienzo del padre(%.3T).", FmTFld(fm0, fno, row), FmTFld(fm, HINICIO, v_linea_orig));
							return FM_REDO;
						}
					}
				}
			}

			break;

		case REGIMD:
/*
			if (!FmIsNull(fm, fno, row )) {
				v_linea_orig = BuscarLinea(row);
*/
				/* Controlo que se cambie el regimen */
/*				if (v_linea_orig != NULL_SHORT)
					if (strcmp(FmSFld(fm0, fno, row), FmSFld(fm0, REGIM, v_linea_orig)) == 0) {
						Warning("El[1m [0mRegimen[1m [0mdel nuevo puesto debe\n ser distinto[1m [0mal regimen del puesto original %s.",
								 FmSFld(fm0, REGIM, v_linea_orig));
						return FM_REDO;
					}
			}
*/
			break;
		case CANTPUED: 

					//  Esto es por si modifico la cantidad de personas y vuelve a poner la misma cantidad
					//   de puestos que estaba originalmente.

				if (FmChgFld(fm)) {
					v_linea_orig = BuscarLinea(row);
					if (v_linea_orig != NULL_SHORT &&
					    FmIFld(fm0, CANTPUE, v_linea_orig) < FmIFld(fm0, CANTPUED, row)) {
						WiDialog(WD_OK, WD_OK, "Error", "El puesto hijo no puede tener mas puestos que el padre");
						return FM_REDO;
					}

					SetKey(operac|PUESTOSbyCLIENTE, FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
					                                FmIFld(fm0, TIPPTOD, row), FmIFld(fm0, CODINTD, row));
					GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

					if (IFld(operac|PUESTOS_CANTPUE)==FmIFld(fm, CANTPUED, row))
						FmSetIFld(fm, CANTVIGD, IFld(operac|PUESTOS_CANTVIG), row);
				}
			break;

	}


	if (fno >= CODMOT && fno <=CANTVIGD)
		if (FmKeyCode(fm0)==K_DEL) {
			ActualizoModif(row);
		}
		

	if (FmKeyCode(fm0)==K_PROCESS)
		if (!ControloFormulario()) {
			FmSetKeyCode(fm0, K_ENTER);
			return FM_REDO;
		}

	return FM_OK;
}

static int BuscarLineaD(int row)
{
	int	v_i = 0;

	for (v_i = 0; v_i < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_i); v_i++)
		if (FmIFld(fm0, TIPPTOD, v_i) == FmIFld(fm0, TIPPTO, row) &&
			FmIFld(fm0, CODINTD, v_i) == FmIFld(fm0, CODINT, row))
			return v_i;

	return NULL_SHORT;
}

static int BuscarLinea(int lin)
{
	int	v_i = 0;

	for (v_i = 0; v_i < NLIN1 && !FmIsNull(fm0, TIPPTO, v_i); v_i++)
		if (FmIFld(fm0, TIPPTO, v_i) == FmIFld(fm0, TIPPTOD, lin) &&
			FmIFld(fm0, CODINT, v_i) == FmIFld(fm0, CODINTD, lin))
			return v_i;

	return NULL_SHORT;
}

static void CopiarLinea(int row)
{
	int	v_i = 0, v_j = 0, v_k = 0;

	/* Busco primer linea libre del MULTID */
	for (v_i = 0; v_i < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_i); v_i++);

	/* Copio linea de MULTI(row) a MULTID(v_i) */
	for (v_j = TIPPTO, v_k = TIPPTOD; v_j <= CANTVIG || v_k <= CANTVIGD; v_j++, v_k++)
		switch(v_j) {
			case REGIM:
			case FINICIO:
			case FFINAL: 
				break;
			default: 
				FmSetFld(fm0, v_k, FmSFld(fm0, v_j, row), v_i);
				break;
		}
}
static bool ControloFormulario() {

	int v_linea_orig, v_linea_dest;
	double canti_horas_orig, canti_horas_dest;
	fmfield campo, campod;
	bool esta_padre;

	LimpioSinMotivo();

	/* Controlo que cantidad de hs de puesto real por dia sea igual a la sumatoria de los hijos */
	for (v_linea_orig = 0; v_linea_orig < NLIN && !FmIsNull(fm0, TIPPTO, v_linea_orig); v_linea_orig++){
		/* solo controlo los puestos que se modifican*/
    	if (!FmIFld(fm0, MODIF))
			continue;

		/* Hago el control por dia*/
		for (campo=DIA1; campo<=DIA7; campo ++) {
	
			canti_horas_orig=ConvHraInt(FmTFld(fm0, HINICIO,  v_linea_orig),
			                            FmTFld(fm0, HFINAL,  v_linea_orig));

			/* Si el campo es nulo paso al proximo */
			if (FmIsNull(fm0, campo, v_linea_orig))
				continue;
			
			/* Recorro los mimp por puesto por dia */
			canti_horas_dest=0;
			for (v_linea_dest = 0; v_linea_dest < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_dest); v_linea_dest++) {
				if(FmIFld(fm0, TIPPTO, v_linea_orig)!=FmIFld(fm0, TIPPTOD, v_linea_dest) ||
				   FmIFld(fm0, CODINT, v_linea_orig)!=FmIFld(fm0, CODINTD, v_linea_dest))
				   continue;
				for (campod=DIA1D; campod<=DIA7D; campod++) {
					if(strcmp(FmSFld(fm0, campo, v_linea_orig), FmSFld(fm0, campod, v_linea_dest))!=0)
						continue;
					canti_horas_dest+=ConvHraInt(FmTFld(fm0, HINICIOD,  v_linea_dest),
					                             FmTFld(fm0, HFINALD,  v_linea_dest));
				}
			}
			
			if (canti_horas_dest != canti_horas_orig) {
				WiDialog(WD_OK, WD_OK, "Error", "La cantidad de horas del dia (%s) del puesto %d %d, no coincide con los puestos mimp.",
				            FmSFld(fm0, campo, v_linea_orig),
				            FmIFld(fm0, TIPPTO, v_linea_orig),
				            FmIFld(fm0, CODINT, v_linea_orig));
				return FALSE;
			}
		}
	} 

	/* Recorro los mimp por puesto por dia para ver si hay dias que no estan en el puesto original */
	canti_horas_dest=0;
	for (v_linea_dest = 0; v_linea_dest < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_dest); v_linea_dest++){
		for (campod=DIA1D; campod<=DIA7D; campod ++) {
			/* Si el campo es nulo paso al proximo */
			if (FmIsNull(fm0, campod, v_linea_dest))
				continue;

			v_linea_orig=BuscarLinea(v_linea_dest);

			esta_padre=FALSE;
			for (campo=DIA1; campo<=DIA7; campo ++) {
				if (strcmp(FmSFld(fm0, campo, v_linea_orig), FmSFld(fm0, campod, v_linea_dest))==0) {
					esta_padre=TRUE;
					break;
				}
			}
			
			if(!esta_padre) {
				WiDialog(WD_OK, WD_OK, "Error", "El dia(%s) del puesto %d %d, linea %d, no coincide con el puesto padre linea %d.",
				            FmSFld(fm0, campod, v_linea_dest),
				            FmIFld(fm0, TIPPTO, v_linea_orig),
				            FmIFld(fm0, CODINT, v_linea_orig),
				            v_linea_dest, v_linea_orig);
				return FALSE;
				
		 	}
		}
	}

	
	return TRUE;
}
static void LimpioSinMotivo()
{
	int v_linea_dest;

	for (v_linea_dest = 0; v_linea_dest < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_dest); v_linea_dest++) {
		if (FmIsNull(fm0, CODMOT, v_linea_dest)){
			ActualizoModif(v_linea_dest);
			FmClearFlds(fm0, CODMOT, CANTVIGD, v_linea_dest);
			FmShowFlds(fm0, CODMOT, CANTVIGD, v_linea_dest);
		}
	}
	
}
static void ActualizoModif(int row)
{
	int v_i;
	bool hayotro;
	int v_linea_orig = 0;

		/* si borro una linea del multi de los mimp, me tengo que fijar si no hay otra linea del 
	mismo puesto, si es asi actualizo en el multi de los puestos originales y lo pongo como no
	modificado */

	hayotro = FALSE;
	for (v_i = 0; v_i < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_i); v_i++)
		if (FmIFld(fm0, TIPPTOD, v_i) == FmIFld(fm0, TIPPTOD, row) &&
			FmIFld(fm0, CODINTD, v_i) == FmIFld(fm0, CODINTD, row) && 
			v_i != row) {
				hayotro=TRUE;
				break;
			}
	if(!hayotro) {
		v_linea_orig = BuscarLinea(row);
		FmSetIFld(fm0, MODIF, FALSE, v_linea_orig);
	}

}
static void CambiarFechaPuestos(long p_cliente, int p_objet, int p_tippto, int p_padreint, int row)
{
	dbcursor c_padre;
	bool asignado;

	c_padre=CreateCursor(operac|PUESTOSbyPADRE, IO_NOT_LOCK);
	SetCursorFrom(c_padre, p_cliente, p_objet, p_tippto, p_padreint);
	SetCursorTo  (c_padre, p_cliente, p_objet, p_tippto, p_padreint);
	while(FetchCursor(c_padre)!=ERROR){

		asignado=PuestoAsignado(FmIFld(fm0, EMP), FmLFld(fm0, CLIE),  FmIFld(fm0, OBJET),
		         FmIFld(fm0, TIPPTO, row), FmIFld(fm0, CODINT, row), FmDFld(fm0, FFINAL, row));

		if((asignado && WiDialog(WD_OK|WD_NO, WD_NO, "Aviso",
		                "El Puesto hijo %d-%d Esta Asignado despues del %.3D \nDesea Continuar?",
		                IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT), 
		                FmDFld(fm0, FFINAL, row))==WD_OK) ||
		   !asignado) {

			PushRecord(operac|PUESTOS);

			CambiarFechaPuestos(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET),
			                    IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT), row);

			PopRecord(operac|PUESTOS);

			if (IsNull(operac|PUESTOS_FFINAL) || DFld(operac|PUESTOS_FFINAL)>FmDFld(fm0, FFINAL, row)){
				SetDFld(operac|PUESTOS_FFINAL, FmDFld(fm0, FFINAL, row));
				PutRecord(operac|PUESTOS);
			}

		}
	}
	DeleteCursor(c_padre);
}

static void Relectura()
{
	int v_linea_orig=0;
	int v_linea_dest=0;

	for (v_linea_orig = 0; v_linea_orig < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_orig); v_linea_orig++) {
		FmClearFlds(fm0, MODIF, MIMP, v_linea_orig);
		FmShowFlds(fm0, MODIF, MIMP, v_linea_orig);
	}
	for (v_linea_dest = 0; v_linea_dest < NLIN1 && !FmIsNull(fm0, TIPPTOD, v_linea_dest); v_linea_dest++) {
		FmClearFlds(fm0, CODMOT, CANTVIGD, v_linea_dest);
		FmShowFlds(fm0, CODMOT, CANTVIGD, v_linea_dest);
	}
	Lectura();

}

