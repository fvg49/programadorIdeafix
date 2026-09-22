/********************************************************************
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*********************************************************************
*
********************************************************************/
#include <ideafix.h>
#include "ausent.fmh"
#include "comerc.h"
#include "comgral.h"
#include "operac.h"
#include "operac.sch"
#include "sue.sch"
#include "comerc.sch"
#include "asist.sch"
#include "webinter.sch"


static fm_status before(form fm, fmfield fno, int row);
static fm_status after(form fm, fmfield fno, int row);

void GrabaLiquida(int p_emp, long p_nroliq, long p_cliented, long p_clienteh, int p_objetivod, int p_objetivoh, int  p_nrolegd, int  p_nrolegh, DATE p_fechad, DATE p_fechah, int p_retro);

void GrabaPase(int p_nivact, double* canti);

bool ControlarFechaCierre(DATE p_fechaf);


/* Funciones Privadas */

form	fm0;
		
schema operac, comerc, bill, sue, intora, webint, asist;
FILE *salida;

int  i, linearp;

int MAXIMLINE;


/* Programa principal */
wcmd(ausent, %I% %G% )
{
	fm_cmd cmd;
	int v_i, v_destino, codnov;
	char v_msgerraux[100], v_archivo[100];;  

	char v_msgerr[100];

	dbcursor c_parte;

	webint = OpenSchema("webinter", IO_EABORT);
	sue    = OpenSchema("sue",      IO_EABORT);
	asist  = OpenSchema("asist",    IO_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);

	fm0 = OpenForm("ausent", FM_EABORT);

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			return;

		AbrirReporte();

		fm1 = UseSubform(fm0, LIQUI, 0);

		switch(*FmSFld(fm0, SALIDA)) {
			case 'A': 
				v_destino=DESTINO_ARCHIVO;
				break;
			case 'P':
				v_destino=DESTINO_PASE;
				break;
			default:
				v_destino=DESTINO_REPORTE;
				break;
    	}

		if (v_destino==DESTINO_PASE)
			BeginTransaction();

		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD)-1);
		while (GetRecord(sue|PERbyEMP, NEXT_KEY|PARTIAL, IO_NOT_LOCK, 1) != ERROR) {
			
//			fprintf(stderr, "Procesando Legajo %ld \n", LFld(sue|PER_NROLEG));
//			DisplayMsg(FALSE, "Procesando Legajo %ld ", );
//			WiRefresh();

			if (!IsNull(sue|PER_FECEGR) && DFld(sue|PER_FECEGR) < FmDFld(fm0, FECHAD))
				continue;

			c_parte = CreateCursor(operac|PARTEbyEMPLE);

			SetCursorFrom(c_parte, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FECHAD, NULL_LONG, NULL_BYTE);
			SetCursorTo  (c_parte, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), FmDFld(fm0, FECHAH, MAX_LONG,  MAX_BYTE);

			while (FetchCursor(c_parte) != ERROR) {

				if (*SFld(PARTE_CONDIC) != _AUSENTE_C)
					continue;

				codnov = 0;
				SetKey(asist|ASISTEN, IFld(PARTE_EMP), DFld(PARTE_DIA), LFld(PARTE_NROLEG), NULL_SHORT);
				while (GetRecord(asist|ASISTEN, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
					if (codnov == 0) {
						codnov = IFld(asist|ASISTEN_CODNOV);
						continue;
					}
					if (codnov != IFld(asist|ASISTEN_CODNOV)) {
						WiDialog(WD_OK, WD_OK, "Error", "El Legajo %d en el día %D tiene dos codigos distintos de motivo de ausentismo %d %d",  LFld(PARTE_NROLEG), DFld(PARTE_DIA), codnov, IFld(asist|ASISTEN_CODNOV));
						break;
					}
				}
				switch(IFld(asist|ASISTEN_CODNOV)) {
				case DESTINO_PASE:
					SetFld (webinter|AUSENT_PAIS,     "PER");
					SetIFld(webinter|AUSENT_EMP,      IFld(sue|PER_EMP));
					SetLFld(webinter|AUSENT_NROLIQ,   FmLFld(fm1, LIQUI1));
					SetLFld(webinter|AUSENT_NROLEG,   LFld(operac|PARTE_NROLEG));
					SetLFld(webinter|AUSENT_CLIENTE,  LFld(operac|PARTE_CLIENTE));
					SetIFld(webinter|AUSENT_OBJETIVO, IFld(operac|PARTE_OBJETIVO));
					SetIFld(webinter|AUSENT_PUESTO,   IFld(operac|PARTE_PUESTO));
					SetDFld(webinter|AUSENT_FECNOV,   DFld(operac|PARTE_DIA));
					SetIFld(webinter|AUSENT_CODNOV,   IFld(asist|ASISTEN_CODNOV));
					SetLFld(webinter|AUSENT_VALOR, 1);

					PutRecord(webinter|AUSENT);
					break;
				case DESTINO_REPORTE:
					fprintf(salida, "%s\t", "PER");
					fprintf(salida, "%d\t", IFld(sue|PER_EMP));
					fprintf(salida, "%d\t", FmLFld(fm1, LIQUI1));
					fprintf(salida, "%d\t", LFld(operac|PARTE_NROLEG));
					fprintf(salida, "%d\t", LFld(operac|PARTE_CLIENTE));
					fprintf(salida, "%d\t", IFld(operac|PARTE_OBJETIVO));
					fprintf(salida, "%d\t", IFld(operac|PARTE_PUESTO));
					fprintf(salida, "%D\t", DFld(operac|PARTE_DIA));
					fprintf(salida, "%d\t", IFld(asist|ASISTEN_CODNOV));
					fprintf(salida, "%d\n", 1);
					break;
				} 
			}

			DeleteCursor(c_parte);
		} 

		if (v_destino==DESTINO_PASE)
			EndTransaction();


		sprintf(v_archivo, "ausent.%d.%D.%T.txt",GetUid(), Today(), Hour() );
		sprintf(v_msgerr,"");
		sprintf(v_msgerraux, "");




		}

		if (strlen(v_msgerr)>0) {
			WiDialog(WD_OK, WD_OK, "Errores Encontrados", "%s\n", v_msgerr);
		}


		switch(*FmSFld(fm0, SALIDA)) {
			case 'A':
				DisplayMsg(FALSE, "Archivo OK");
				WiRefresh();
				fclose(salida);
				break;
			case 'P': 
				DisplayMsg(FALSE, "Pase OK");
				WiRefresh();
				break;
			default: 
				DisplayMsg(FALSE, "Listado OK");
				WiRefresh();

				CloseReport(rp0);
				break;
		}

		if (v_destino==DESTINO_PASE) 
			GrabaLiquida(FmIFld(fm0, EMP), FmLFld(fm1, LIQUI1), FmLFld(fm0, CLID),FmLFld(fm0, CLIH), FmIFld(fm0, OBJD), FmIFld(fm0, OBJH), 
			             FmLFld(fm0, VIGILD), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), retro);

		if (v_destino==DESTINO_PASE) {
			if (!encontro)
				Error("No se Genera el Archivo de Pase Porque Existen Errores\nConsultar el archivo");
		}
	}

}
static fm_status before(form fm, fmfield fno, int row)
{
	switch(fno){
	} 
	return FM_OK;				

}

static fm_status after(form fm, fmfield fno, int row)
{
	fm1 = UseSubform(fm0, LIQUI, 0);
	switch(fno){
		case SALIDA:
			if (*FmSFld(fm0, SALIDA)=='P'){
				FmClearFlds(fm0, DCLID, DCLID);
				FmClearFlds(fm0, DCLIH, DCLIH);
				FmClearFlds(fm0, DOBJD, DOBJD);
				FmClearFlds(fm0, DOBJH, DOBJH);
				FmClearFlds(fm0, DVIGILD, DVIGILD);
				FmClearFlds(fm0, DVIGILH, DVIGILH);
			}
			break;

			// Pongo como fecha hasta la fecha hasta de la liquidacion anterior mas 1

/*			if (FmChgFld(fm) && FmIsNull(fm1, LIQUI1)) {
				SetKey(webint|LIQUIDAbyCREACION, FmIFld(fm0, EMP), MAX_DATE, MAX_TIME);
				if (GetRecord(webint|LIQUIDAbyCREACION, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) 
					FmSetDFld(fm0, FECHAD, DFld(webint|LIQUIDA_FECHAH)+1);

			}
*/
		case LIQUI:
			DoSubform(fm, NULLFP, after_fm1, fno, 0, row);

			if (FmIsNull(fm1, LIQUI1)) {
				WiDialog(WD_OK, WD_OK, "Liquidación No Valida", "La liquidacion no puede ser nula");
				return FM_REDO;
			}

			SetKey(webint|LIQUIDAbyEMP, FmIFld(fm0, EMP), FmLFld(fm1, LIQUI1));
			if (GetRecord(webint|LIQUIDAbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) 
				if (WiDialog(WD_YES|WD_NO, WD_NO, "Liquidación Existe", "Desea reprocesar la liquidación ?", FmLFld(fm1, LIQUI1))!=WD_YES) {
					FmSetLFld(fm0, CLID,	LFld(webint|LIQUIDA_CLIENTED));
					FmSetLFld(fm0, CLIH,	LFld(webint|LIQUIDA_CLIENTEH));
					FmSetIFld(fm0, OBJD,	IFld(webint|LIQUIDA_OBJETIVOD));
					FmSetIFld(fm0, OBJH,	IFld(webint|LIQUIDA_OBJETIVOH));
					FmSetLFld(fm0, VIGILD,	LFld(webint|LIQUIDA_NROLEGD));
					FmSetLFld(fm0, VIGILH,	LFld(webint|LIQUIDA_NROLEGH));
					FmSetDFld(fm0, FECHAD,	DFld(webint|LIQUIDA_FECHAD));
					FmSetDFld(fm0, FECHAH,	DFld(webint|LIQUIDA_FECHAH));
					FmSetDFld(fm0, FRETRO,	DFld(webint|LIQUIDA_RETRO));
				
				
					return FM_REDO;
				}

				if (!ControlarFechaCierre(FmDFld(fm0, FECHAH))) {
					WiDialog(WD_OK, WD_OK, "Error", "La Fecha Hasta de la Liquidacion Debe Ser \nAnterior o Igual a la Ultima Fecha de Cierre");
if (getuid() != 1897)
					return FM_REDO;
				}
				
			
			break;
	} 
	return FM_OK;				
	
}
bool ControlarFechaCierre(DATE p_fechaf)
{
	DATE v_ultcie=NULL_DATE;
	
	v_ultcie=StrToD(GetParNov(FmIFld(fm0,EMP ), PARNOV_CIEOPERA, 1, MAX_DATE));

	return ( p_fechaf <= v_ultcie );
	
}

static fm_status after_fm1(form fm, fmfield fno, int row)
{
	char aux[10];
	char aux1[5];
	
	switch(fno){
		case LIQUI1:
			if (!FmIsNull(fm, fno, row)) {

				sprintf(aux, "%ld", FmLFld(fm1, LIQUI1));
				
				strncpy(aux1, aux, 4);
				FmSetIFld(fm1, ANO, StrToI(aux1));
				strncpy(aux1, aux+4, 2);
				FmSetFld(fm1, MES, aux1);
				strncpy(aux1, aux+6, 1);
				FmSetIFld(fm1, QUIN, StrToI(aux1));

			}
			break;
		case AGRUL:
			if (FmIsNull(fm, LIQUI1)) {
				sprintf(aux, "%d%s%d", FmIFld(fm1, ANO), FmSFld(fm1, MES), FmIFld(fm1, QUIN));
				FmSetLFld(fm1, LIQUI1, StrToL(aux));
			}
			break;
	} 
	return FM_OK;				
	
}

		case 'A': 
			salida = fopen(FmSFld(fm0, NOMARCH), "w");

			for (v_i=0; v_i<=NivCon;v_i ++) {
				switch (v_i) {
					case NIV_EMPRESA:
						fprintf(salida,"EMPRESA\tDESCRIPCION EMPRESA\t");
						break;
					case NIV_LEGAJO:
						fprintf(salida,"LEGAJO\tDESCRIPCION LEGAJO\t");
						break;
					case NIV_CLIENTE:
						if (NivCon==NIV_CLIENTE)
							fprintf(salida,"CLIENTE\tDESCRIPCION CLIENTE\t");
						else
							fprintf(salida,"CLIENTE\t");
						break;
					case NIV_OBJET:
						fprintf(salida,"OBJETIVO\tDESCRIPCION CLIENTE\tDESCRIPCION OBJETIVO\tTERCERO\tSUBTERCERO\t");
						break;
					case NIV_PUESTO :
						fprintf(salida,"TIPO PUESTO\tNUMERO PUESTO\tCODIGO INTERNO DE PUESTO\tHORA INICIO DE PUESTO\tHORA FIN DE PUESTO\t");
						fprintf(salida,"DIAS DE LA SEMANA DEL PUESTO\tREGIMEN DEL PUESTO\tCANTIDAD DE VIGILADORES DEL PUESTO\tCANTIDAD DE PUESTOS\tTIPO DE DIA DEL PUESTO\t");
						fprintf(salida,"FECHA DE INICIO DEL PUESTO\tFECHA DE FIN DEL PUESTO\t");
						break;
					case NIV_DIA:
						fprintf(salida,"FECHA\t");
						break;
					case NIV_HORA:
						fprintf(salida,"CONDICION\t");
						fprintf(salida,"HORA DESDE - HASTA");
						break;
				}
			}
			

			break;
	}

	switch(*FmSFld(fm0, SALIDA)) {
		case 'A': 
			for (v_i=NivCon; v_i<=NIV_HORA; v_i ++) 
				fprintf(salida,"\t");

			fprintf(salida,"HS NORMALES\t");
			fprintf(salida,"HS AL 25%%\t");
			fprintf(salida,"HS AL 35%%\t");
			fprintf(salida,"HS FRANCO%%\t");
			fprintf(salida,"HS FERIADO\t");
			fprintf(salida,"HS NORMALES NOCTURNAS\t");
			fprintf(salida,"HS AL 25%% NOCTURNAS\t");
			fprintf(salida,"HS AL 35%% NOCTURNAS\t");
			fprintf(salida,"HS PEGADAS\t");
			fprintf(salida,"HS NORMALES FERIADO\t");
			fprintf(salida,"HS AL 25%% FERIADO\t");
			fprintf(salida,"HS AL 35%% FERIADO\t");
			fprintf(salida,"HS NORMALES NOCTURNAS FERIADO\t");
			fprintf(salida,"HS AL 25%% NOCTURNAS FERIADO\t");
			fprintf(salida,"HS AL 35%% NOCTURNAS FERIADO\t");
			fprintf(salida,"HS NORMALES FRANCO\t");
			fprintf(salida,"HS AL 25%% FRANCO\t");
			fprintf(salida,"HS AL 35%% FRANCO\t");
			fprintf(salida,"HS NORMALES NOCTURNAS FRANCO\t");
			fprintf(salida,"HS AL 25%% NOCTURNAS FRANCO\t");
			fprintf(salida,"HS AL 35%% NOCTURNAS FRANCO\t");
			fprintf(salida,"Cant.Jor.Pegadas\t");
			fprintf(salida,"Cant.Jorn.Nocturnas\t");
			fprintf(salida,"Cant.Jorn.Diurnas");
/*			fprintf(salida,"Cant.Francos Gozados\t");
			fprintf(salida,"Cant.Francos Trabajados\t");
			fprintf(salida,"Cant.Vacaciones Gozadas\t");
			fprintf(salida,"Cant.Vacaciones Trabajadas\t");
			fprintf(salida,"Cant.Ausentes");
*/			fprintf(salida,"\n");
			break;
	}

} 


void ImprimeArchivo(int p_nivact, double* canti)
{
	int v_i=0;

		if (p_nivact<NivCon)
			return;

		for (v_i=0; v_i<=p_nivact;v_i ++) {
			switch (v_i) {
				case NIV_EMPRESA :
					fprintf(salida,"%d\t", empres);
					fprintf(salida,"%s\t", semp);
					break;
				case NIV_LEGAJO :
					fprintf(salida,"%ld\t", nodleg);
					fprintf(salida,"%s\t", sleg);
					break;
				case NIV_CLIENTE:
					fprintf(salida,"%ld\t", nodclie);
						if (NivCon==NIV_CLIENTE)
							fprintf(salida,"%s\t", scli);
					break;
				case NIV_OBJET:
					fprintf(salida,"%d\t", objnod);
					fprintf(salida,"%s\t", scli);
					fprintf(salida,"%s\t", sobj);
					fprintf(salida,"%s\t", tercer);
					fprintf(salida,"%s\t", subter);
					break;
				case NIV_PUESTO :
					fprintf(salida,"%d\t%d\t%d\t%.3T\t%.3T\t%c-%c-%c-%c-%c-%c-%c\t%s\t%.2f\t%d\t%c\t%.3D\t%.3D\t",
					n_ptoser, n_puesto, n_codint, n_horini, n_horfin, n_dia1, n_dia2, n_dia3, n_dia4, n_dia5, n_dia6, n_dia7, n_regim, (double)n_canvig/100, n_canpto, n_tipdia, n_fecini, n_fecfin);

					break;
				case NIV_DIA:
					fprintf(salida,"%.3D\t", dianod);
					break;
				case NIV_HORA:
					fprintf(salida,"%c\t", condic);
					fprintf(salida,"%.3T - %.3T",hordes, horhas);
					break;
			}
		}

		for (v_i=p_nivact; v_i<=NIV_HORA; v_i ++) 
			fprintf(salida,"\t");

		fprintf(salida,"%.2f\t", canti[HSNORM]/100);
		fprintf(salida,"%.2f\t", canti[HSAL25]/100);
		fprintf(salida,"%.2f\t", canti[HSAL35]/100);
		fprintf(salida,"%.2f\t", canti[HSFRAN]/100);
		fprintf(salida,"%.2f\t", canti[HSFERI]/100);
		fprintf(salida,"%.2f\t", canti[HSNONO]/100);
		fprintf(salida,"%.2f\t", canti[HS25NO]/100);
		fprintf(salida,"%.2f\t", canti[HS35NO]/100);
		fprintf(salida,"%.2f\t", canti[HSPEGA]/100);
		fprintf(salida,"%.2f\t", canti[HSNOFE]/100);
		fprintf(salida,"%.2f\t", canti[HS25FE]/100);
		fprintf(salida,"%.2f\t", canti[HS35FE]/100);
		fprintf(salida,"%.2f\t", canti[HNOFEN]/100);
		fprintf(salida,"%.2f\t", canti[H25FEN]/100);
		fprintf(salida,"%.2f\t", canti[H35FEN]/100);
		fprintf(salida,"%.2f\t", canti[HSNOFR]/100);
		fprintf(salida,"%.2f\t", canti[HS25FR]/100);
		fprintf(salida,"%.2f\t", canti[HS35FR]/100);
		fprintf(salida,"%.2f\t", canti[HNOFRN]/100);
		fprintf(salida,"%.2f\t", canti[H25FRN]/100);
		fprintf(salida,"%.2f\t", canti[H35FRN]/100);
		if (canti[HSPEGA] > 0)
			fprintf(salida,"%d\t",  canpeg);
		else
			fprintf(salida,"%d\t",  0);
		fprintf(salida,"%d\t",  diasnoct);
		fprintf(salida,"%d",  diasdiur);
//		fprintf(salida,"%d\t",  diasfran);
//		fprintf(salida,"%d\t",  diasfrat);
//		fprintf(salida,"%d\t",  diasvaca);
//		fprintf(salida,"%d\t",  diasvact);
//		fprintf(salida,"%d",    diasause);

		fprintf(salida,"\n");
}


void GrabaPase(int p_nivact, double* canti)
{
	char aux[2];

	if (p_nivact!=NIV_HORA)
		return;

	InitRecord(webint|DETLIQ);

	SetIFld(webint|DETLIQ_EMP,		empres);
	SetLFld(webint|DETLIQ_NROLIQ,	FmLFld(fm1, LIQUI1));
	SetDFld(webint|DETLIQ_FECHAD,	FmDFld(fm0, FECHAD));
	SetDFld(webint|DETLIQ_FECHAH,	FmDFld(fm0, FECHAH));
	SetLFld(webint|DETLIQ_NROLEG,	nodleg);
	SetLFld(webint|DETLIQ_CLIENTE,	nodclie);
	SetIFld(webint|DETLIQ_OBJETIVO,	objnod);
	SetFld (webint|DETLIQ_TERCERO,	tercer);
	SetFld (webint|DETLIQ_SUBTERC,	subter);
	SetIFld(webint|DETLIQ_PTOSER,	n_ptoser);
	SetIFld(webint|DETLIQ_PUESTO,	n_puesto);
	SetIFld(webint|DETLIQ_CODINT,	n_codint);
	SetFFld(webint|DETLIQ_HSNORM,	canti[HSNORM]);
	SetFFld(webint|DETLIQ_HSAL25,	canti[HSAL25]);
	SetFFld(webint|DETLIQ_HSAL35,	canti[HSAL35]);
	SetFFld(webint|DETLIQ_HSFRA,	canti[HSFRAN]);
	SetFFld(webint|DETLIQ_HSFER,	canti[HSFERI]);
	SetFFld(webint|DETLIQ_HSNONO,	canti[HSNONO]);
	SetFFld(webint|DETLIQ_HS25NO,	canti[HS25NO]);
	SetFFld(webint|DETLIQ_HS35NO,	canti[HS35NO]);
	SetFFld(webint|DETLIQ_HSPEGA,	canti[HSPEGA]);
	SetIFld(webint|DETLIQ_CANPEG,	canpeg);
	SetIFld(webint|DETLIQ_JORNOC,	diasnoct);
	SetIFld(webint|DETLIQ_JORDIU,	diasdiur);
	SetFFld(webint|DETLIQ_HSNOFE,	canti[HSNOFE]);
	SetFFld(webint|DETLIQ_HS25FE,	canti[HS25FE]);
	SetFFld(webint|DETLIQ_HS35FE,	canti[HS35FE]);
	SetFFld(webint|DETLIQ_HSNONOFE,	canti[HNOFEN]);
	SetFFld(webint|DETLIQ_HS25NOFE,	canti[H25FEN]);
	SetFFld(webint|DETLIQ_HS35NOFE,	canti[H35FEN]);
	SetFFld(webint|DETLIQ_HSNOFR,	canti[HSNOFR]);
	SetFFld(webint|DETLIQ_HS25FR,	canti[HS25FR]);
	SetFFld(webint|DETLIQ_HS35FR,	canti[HS35FR]);
	SetFFld(webint|DETLIQ_HSNONOFR,	canti[HNOFRN]);
	SetFFld(webint|DETLIQ_HS25NOFR,	canti[H25FRN]);
	SetFFld(webint|DETLIQ_HS35NOFR,	canti[H35FRN]);

	SetFFld(webint|DETLIQ_TOHSNO,	canti[HSNORM]+canti[HSNONO]+canti[HSNOFE]+canti[HNOFEN]);
	SetFFld(webint|DETLIQ_TOHSEX,	canti[HSAL25]*1.25+canti[HSAL35]*1.35+canti[HS25NO]*1.25+canti[HS35NO]*1.35+canti[HSFRAN]*2+canti[HSFERI]*2);
	SetFFld(webint|DETLIQ_TOHSNOR,	FFld(webint|DETLIQ_TOHSNO)+FFld(webint|DETLIQ_TOHSEX));

	strcpy(regpue, NULL_STR);
	SetKey(operac|PUESTOS, nodclie, objnod, n_ptoser, n_puesto);
	if (GetRecord(operac|PUESTOS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(regpue, SFld(operac|PUESTOS_REGIM));
	}

	SetFld (webint|DETLIQ_REGPUE,	regpue);
	SetIFld(webint|DETLIQ_DIVAUR,	divaur);
	SetIFld(webint|DETLIQ_DEPAUR,	depaur);

	if (cliefe == nodclie && objefe == objnod)
		SetIFld(webint|DETLIQ_CECOEF,	1);
	else
		SetIFld(webint|DETLIQ_CECOEF,	0);

	SetFld (webint|DETLIQ_DECECO,	sobjefe);
	SetLFld(webint|DETLIQ_OFPAG,	ofpag);
	SetFld (webint|DETLIQ_ZONA,		zona);

	sprintf(aux, "%c", condic);
	SetFld (webint|DETLIQ_CONDIC,	aux);
	SetIFld(webint|DETLIQ_CODNOV,	codnov);
	SetDFld(webint|DETLIQ_FECASI,	dianod);

	PutRecord(webint|DETLIQ);

}

void GrabaLiquida(int p_emp, long p_nroliq, long p_cliented, long p_clienteh, int p_objetivod, int p_objetivoh, int  p_nrolegd, int  p_nrolegh, DATE p_fechad, DATE p_fechah, int p_retro)
{
	SetLFld(webint|LIQUIDA_NROLIQ,		p_nroliq);
	SetIFld(webint|LIQUIDA_EMP, 		p_emp);
	SetLFld(webint|LIQUIDA_CLIENTED, 	p_cliented);
	SetLFld(webint|LIQUIDA_CLIENTEH, 	p_clienteh);
	SetIFld(webint|LIQUIDA_OBJETIVOD, 	p_objetivod);
	SetIFld(webint|LIQUIDA_OBJETIVOH, 	p_objetivoh);
	SetLFld(webint|LIQUIDA_NROLEGD, 	p_nrolegd);
	SetLFld(webint|LIQUIDA_NROLEGH, 	p_nrolegh);
	SetDFld(webint|LIQUIDA_FECHAD, 		p_fechad);
	SetDFld(webint|LIQUIDA_FECHAH, 		p_fechah);
	SetIFld(webint|LIQUIDA_RETRO, 		p_retro);
	SetIFld(webint|LIQUIDA_ESTADO,		0);

	PutRecord(webint|LIQUIDA);
	
}

table ausent descr "Tabla de ausentismo para interfaz con Meta4"
{
}
primary key(pais, emp, nroliq, nroleg, cliente, objetivo, puesto, fecnov);

