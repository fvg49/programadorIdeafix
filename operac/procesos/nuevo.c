/********************************************************************
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
*
* DESCRIPTION:
*	Listado de Control de Horas por Vigilidor.
*   Acumula en listas enlazadas por niveles, las horas de 
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*********************************************************************
* CREATED          : 02/06/05 Fernando Ventura Goncalves
********************************************************************/
#include <ideafix.h>
#include "nuevo.fmh"
//#include "nuevo1.fmh"
#include "nuevo.rph"
#include "disths.h"
#include "comerc.h"
#include "comgral.h"
#include "operac.h"
#include "novora.h"
#include "webinter.h"
#include "intora.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "comerc.sch"
#include "asist.sch"
#include "webinter.sch"
#include "billpro.sch"

#define _ENCAB 16

#define _INTERVALO_LEGAJO 100


#define TOT_NADA 1
#define TOT_ALGO 2
#define TOT_TODO 3

#define NIV_EMPRESA	0
#define NIV_LEGAJO	1
#define NIV_CLIENTE	2
#define NIV_OBJET	3
#define NIV_PUESTO	4
#define NIV_DIA		5
#define NIV_HORA	6

/* Estados de los detalles */
#define CONST_ESTADO_PEND_PROC		0
#define CONST_ESTADO_EN_EJECU		1
#define CONST_ESTADO_TERM_OK		2
#define CONST_ESTADO_TERM_WARN		3
#define CONST_ESTADO_TERM_ERR		4
#define CONST_ESTADO_ENVIADO		5




static fm_status before(form fm, fmfield fno, int row);
static fm_status after(form fm, fmfield fno, int row);

void GrabaLiquida(int p_emp, long p_nroliq, long p_cliented, long p_clienteh, int p_objetivod, int p_objetivoh, long  p_nrolegd, long  p_nrolegh, DATE p_fechad, DATE p_fechah, int p_retro, int p_tipocierre);
void GrabaDeallesDeLiquidacion(int p_emp, long p_nroliq, int p_estado_origen, int p_estado_destino, DATE p_fechad, DATE p_fechah);

void ImprimeArchivo(int p_nivact, double* canti);
void GrabaPase(int p_nivact, double* canti);

bool ControlarFechaCierre(DATE p_fechaf);


/* IMPORTANTE: No olvidar de dejar informaErrores en FALSE */
bool informaErrores = FALSE;


void LisNEmpres(tnempres nodop);
void LisNNodleg(tnnodleg nodop);
void LisNNodclie(tnnodclie nodop);
void LisNObjnod(tnobjnod nodop);
void LisNDianod(tndianod nodop);
void LisNPuenod(tnpuenod nodop);
void LisNHordes(tnhordes nodop);

char semp[100];
char sleg[100];
char scli[110];
char sobj[100];
char sobjefe[100];

char tercer[20];
char subter[10];
char regpue[20];
int	 divaur, depaur, cecoef;
char dececo[25];
long ofpag, clipad;
char zona[50];
int objefe, objpad;
long cliefe;
long g_total_registros_detalle = 0;


/* Funciones Privadas */
void AbrirReporte();
void SaltoPagina();
void CerrarPantalla();

report rp0;
form	fm0;		
schema operac, comerc, bill, sue, intora, webint, asist, billpro;
FILE *salida;

int  i, linearp;
double auxcan[MAXTIPHOR];

int MAXIMLINE;

DATE g_fechaEnvioInicioProceso;
TIME g_horarioEnvioInicioProceso;


/* Programa principal */
wcmd(nuevo, %I% %G% )
{
	fm_cmd cmd;
	int retro, canleg=0;
	long legdes;
	int v_destino;
	long v_nrolegd=NULL_LONG;
	long v_nrolegh=MAX_LONG;
	long v_nrolegt=MAX_LONG;
	bool encontro=FALSE;
	char v_msgerraux[100], v_archivo[100];;  

	char v_msgerr[100];

	billpro= OpenSchema("billpro",  IO_EABORT);
	webint = OpenSchema("webinter", IO_EABORT);
	intora = OpenSchema("intora",   IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);
	bill   = OpenSchema("bill",     IO_EABORT);
	sue    = OpenSchema("sue",      IO_EABORT);
	asist  = OpenSchema("asist",    IO_EABORT);
	comerc = OpenSchema("comerc",   IO_EABORT);

	fm0 = OpenForm("nuevo", FM_EABORT);

	// Se controla que no se este ejectando el cierre, sino es asi  se permite ingresar al programa
	if (CierreActivo()) {
		WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
		return;
	}

	if ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		if (cmd != FM_UPDATE)
			return;

		// Se controla que no se este ejectando el cierre, sino es asi  se permite ejecutar el programa
		if (CierreActivo()) {
			WiDialog(WD_OK, WD_OK, "Mensaje", "No se puede ejecutar este proceso porque en este momento esta ejecutandose un cierre");
			return;
		}

		//Inicializo el contador de registros en cero
		g_total_registros_detalle = 0;
	   
	   	NivCon = FmIFld(fm0, R_DETALLE);
		retro =FmIFld(fm0, FRETRO);
		g_fechaEnvioInicioProceso = Today();
		g_horarioEnvioInicioProceso = Hour();

		AbrirReporte();

		encontro=FALSE;

		canleg=0;
		legdes=FmLFld(fm0, VIGILD);

		switch(*FmSFld(fm0, SALIDA)) {
		case 'I': 
		case 'T': 
		case 'A': 
			v_destino=DESTINO_REPORTE;
			break;
		case 'P':
			v_destino=DESTINO_PASE;
			break;
		case 'S':
			v_destino=DESTINO_TEST;
			break;
		default:
			v_destino=DESTINO_REPORTE;
			break;
    	}
		inicio = NULL;


		DisplayMsg(FALSE, "Acumulando.......");
		WiRefresh();


		//fm1 = UseSubform(fm0, LIQUI, 0);


		v_nrolegd=FmLFld(fm0, VIGILD);
		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, VIGILD));
		if (GetRecord(sue|PERbyEMP, THIS_KEY|NEXT_KEY, IO_NOT_LOCK) != ERROR)
 			v_nrolegd=LFld(sue|PER_NROLEG);

		v_nrolegt=FmLFld(fm0, VIGILH);
		SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, VIGILH)+1);
		if (GetRecord(sue|PERbyEMP, PREV_KEY, IO_NOT_LOCK) != ERROR)
 			v_nrolegt=LFld(sue|PER_NROLEG);

		sprintf(v_archivo, "contden.%d.%D.%T.txt",GetUid(), Today(), Hour() );
		sprintf(v_msgerr,"");
		sprintf(v_msgerraux, "");

        if(informaErrores)
			fprintf(stderr, "Legajo tope  %ld  desde %ld\n", v_nrolegt, v_nrolegd);


		//TODO: Después descomentar
		//ACHIMURIS
		//v_nrolegt = 1000;
		
		for (; v_nrolegd<=v_nrolegt; v_nrolegd+=_INTERVALO_LEGAJO) {

			v_nrolegh=v_nrolegd+_INTERVALO_LEGAJO-1;
			if (v_nrolegh>v_nrolegt)
				v_nrolegh=v_nrolegt;

			if (informaErrores)
				fprintf(stderr, "Procesando Legajo %ld Hasta %ld\n", v_nrolegd, v_nrolegh);

			if (v_destino==DESTINO_TEST || v_destino==DESTINO_PASE)
				BeginTransaction();

			inicio=NULL;
 
			sprintf(v_msgerraux, "%s", v_archivo);

			inicio = CargaDistrHoras(FmIFld(fm0, EMP),  FmIFld(fm0, EMP), FmLFld(fm0, CLID),FmLFld(fm0, CLIH), FmIFld(fm0, OBJD), FmIFld(fm0, OBJH), 
			                         v_nrolegd, v_nrolegh, FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), retro, FmIFld(fm0, VIGACTIVO), v_destino, FmLFld(fm0, LIQUI), 
			                         TRUE, v_msgerraux);

			// si v_destino es pase y inicio es nulo continue;

			if (strlen(v_msgerraux)>0)
				sprintf(v_msgerr, "%s", v_msgerraux);

			if (inicio != NULL)
				encontro=TRUE;

			if(v_destino == DESTINO_TEST || v_destino == DESTINO_PASE)
				if (encontro)
					EndTransaction();

			DisplayMsg(FALSE, "Listando.......");
			WiRefresh();

			linearp += (_ENCAB-1);

			LisNEmpres(inicio);
			BorNEmpres(inicio);


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
	    case 'S': 
			DisplayMsg(FALSE, "Pase OK");
			WiRefresh();
			break;
		default: 
			DisplayMsg(FALSE, "Listado OK");
			WiRefresh();

			CloseReport(rp0);
			break;
		}

		if ((v_destino==DESTINO_PASE || v_destino==DESTINO_TEST) && encontro)
			GrabaLiquida(FmIFld(fm0, EMP), FmLFld(fm0, LIQUI), FmLFld(fm0, CLID),FmLFld(fm0, CLIH), FmIFld(fm0, OBJD), FmIFld(fm0, OBJH), 
			             FmLFld(fm0, VIGILD), FmLFld(fm0, VIGILH), FmDFld(fm0, FECHAD), FmDFld(fm0, FECHAH), retro, FmIFld(fm0, COTICI));

		if (v_destino==DESTINO_PASE || v_destino == DESTINO_TEST)
			if (!encontro) 
			    Error("No se encontraron registros para generar el pase en el periodo / empresa informado.");
				
//		}
		
		CerrarPantalla();
	}

}
void CerrarPantalla()
{
	switch(*FmSFld(fm0, SALIDA)) {
		case 'A':
			WiMsg("Archivo OK");
			break;
		case 'P': 
	    case 'S': 
			WiMsg("Pase OK");
			break;
		default: 
			WiMsg("Listado OK");
			break;
		}
}
static fm_status before(form fm, fmfield fno, int row)
{
	switch(fno){
	case CANLIN:
		if (*FmSFld(fm0, SALIDA)=='T') 
			FmSetIFld(fm, fno, WiHeight(SCREEN) -9 );
		break;
	} 
	return FM_OK;				

}

static fm_status after(form fm, fmfield fno, int row)
{
	long numeroDeLiquidacion = 0;
	switch(fno){
	case SALIDA:
		if ( *FmSFld(fm0, SALIDA)== 'P' || *FmSFld(fm0, SALIDA) == 'S'){
			FmClearFlds(fm0, DCLID, DCLID);
			FmClearFlds(fm0, DCLIH, DCLIH);
			FmClearFlds(fm0, DOBJD, DOBJD);
			FmClearFlds(fm0, DOBJH, DOBJH);
			FmClearFlds(fm0, DVIGILD, DVIGILD);
			FmClearFlds(fm0, DVIGILH, DVIGILH);
			
			/*ACHIMURIS*/
			/* Aca agregar el número de liquidacion que es un calculado */
			//primary key (emp, anoper, tipcie, numper)
			SetKey(operac|PERIODObyEMP, FmIFld(fm0, EMP), FmIFld(fm0, ANIO), FmIFld(fm0, COTICI), FmIFld(fm0, PERIOD));
			if(GetRecord(operac|PERIODObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				/*Armar número de liquidación*/
				//YYYYMMTT donde YYYY = año MM mes y TT el tipo
				//WiMsg("%.3D", DFld(operac|PERIODO_FECHAS));
				numeroDeLiquidacion = FmIFld(fm0, ANIO) * 10000 + Month(DFld(operac|PERIODO_FECHAS)) * 100 + FmIFld(fm0, COTICI);
				FmSetLFld(fm0, LIQUI, numeroDeLiquidacion);
			}
		}
		break;

		// Pongo como fecha hasta la fecha hasta de la liquidacion anterior mas 1


//		if (FmChgFld(fm) && FmIsNull(fm1, LIQUI1)) {
//			SetKey(webint|WCABLIQbyCREACION, FmIFld(fm0, EMP), MAX_DATE, MAX_TIME);
//			if (GetRecord(webint|WCABLIQbyCREACION, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) 
//				FmSetDFld(fm0, FECHAD, DFld(webint|WCABLIQ_FECHAH)+1);
//		}

	case LIQUI:

			if (!ControlarFechaCierre(FmDFld(fm0, FECHAH))) {
				WiDialog(WD_OK, WD_OK, "Error", "La Fecha Hasta de la Liquidacion Debe Ser \nAnterior o Igual a la Ultima Fecha de Cierre");

			//if (getuid() != 1897)
			//	return FM_REDO;
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


void AbrirReporte()
{
	char auxi[5][190], agrega[100];
	char auxdia[20];
	int v_x, v_y = 0;
	int v_i=0;

	if (*FmSFld(fm0, SALIDA)=='T') 
		MAXIMLINE=FmIFld(fm0, CANLIN);
	else 
		MAXIMLINE=40;


	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 

		rp0 = OpenReport("nuevo", RP_EABORT|RP_NOBEGIN);

		//Si la salida es Impresora
		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

		//Si la salida es Terminal
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );



		RpSetIFld(rp0, FLENGTH, 100);
		linearp = 0;

		BeginReport(rp0, 1, NULL_STR);

		RpSetFld(rp0, REMP, FmSFld(fm0, DEMP));

		sprintf(auxi[0],"%s", NULL_STR);
		sprintf(auxi[1],"%s", NULL_STR);
		sprintf(auxi[2],"%s", NULL_STR);
		sprintf(auxi[3],"%s", NULL_STR);
		sprintf(auxdia, "%s", NULL_STR);

		for (v_x = EMP; v_x <= DTOTALIZ; v_x++) {
			if (!FmIsNull(fm0, v_x)) {
				sprintf(agrega,"%s", NULL_STR);

				switch(v_x) {
				case EMP:
					sprintf(agrega, "Empresa: ");
					break; 
				case CLID:
					sprintf(agrega, "Cliente Desde: ");
					break; 
				case DCLID:
					sprintf(agrega, " ");
					break; 
				case CLIH:
					sprintf(agrega, "Cliente Hasta: ");
					break; 
				case DCLIH:
					sprintf(agrega, " ");
					break; 
				case OBJD:
					sprintf(agrega, "Objetivo Desde: ");
					break; 
				case DOBJD:
					sprintf(agrega, " ");
					break; 
				case OBJH:
					sprintf(agrega, "Objetivo Hasta: ");
					break; 
				case DOBJH:
					sprintf(agrega, " ");
					break; 
				case VIGILD:
					sprintf(agrega, "Vigilador Desde: ");
					break; 
				case DVIGILD:
					sprintf(agrega, " ");
					break; 
				case VIGILH:
					sprintf(agrega, "Vigilador Hasta: ");
					break; 
				case DVIGILH:
					sprintf(agrega, " ");
					break; 
				case TOTALIZ:
					sprintf(agrega, "Totalizadores: ");
					break; 
				case DTOTALIZ:
					sprintf(agrega, " ");
					break; 
				case FECHAD:
					sprintf(agrega, "Fecha Desde: ");
					break; 
				case FECHAH:
					sprintf(agrega, "Fecha Hasta: ");
					break; 
				case R_DETALLE:
					continue;
					break; 
				case DESCDET:
					sprintf(agrega, "Detallado ");
					break; 
				case FRETRO:
					sprintf(agrega, "Incluye Retro: ");
					break; 
				}
				switch(v_x) {
				case FECHAD:
				case FECHAH:
					DToStr(FmDFld(fm0, v_x), auxdia, DFMT_SEPAR);
					sprintf(agrega, "%s %s", agrega , auxdia);
					break;
				default: 
					sprintf(agrega, "%s %s", agrega , FmSFld(fm0, v_x));
					break;
				}

				if ((strlen(auxi[v_y])+strlen(agrega)) > 160)
					 v_y++;

				sprintf(auxi[v_y], "%s %s ", auxi[v_y], agrega);
			}
		}
		RpSetFld(rp0, RSELEC,  auxi[0]);
		RpSetFld(rp0, RSELEC1, auxi[1]);
		RpSetFld(rp0, RSELEC2, auxi[2]);

		break;
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
		fprintf(salida,"Cant.Jor.Pegadas\t");
		fprintf(salida,"Cant.Jorn.Nocturnas\t");
		fprintf(salida,"Cant.Jorn.Diurnas\t");
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
		fprintf(salida,"CONDICION");
		fprintf(salida,"Regimen");
/*		fprintf(salida,"Cant.Francos Gozados\t");
		fprintf(salida,"Cant.Francos Trabajados\t");
		fprintf(salida,"Cant.Vacaciones Gozadas\t");
		fprintf(salida,"Cant.Vacaciones Trabajadas\t");
		fprintf(salida,"Cant.Ausentes");
*/		fprintf(salida,"\n");
		break;
	}

} 

void SaltoPagina()
{
	int corte = MAXIMLINE;

	linearp++;
	if (((linearp + 1) / corte) == 1) {
		RpEjectPage(rp0);
		linearp = _ENCAB-9;


	}
}

void LisNEmpres(tnempres nodop)
{
	char aux [60];
	int nivelactual=NIV_EMPRESA;

	if (nodop == NULL)
		return;

	empres = (*nodop).empres;
	sprintf(aux, "%d", empres);
	sprintf(semp,  "%s", NULL_STR);
	diasdiur = (*nodop).diasdiur;
	diasnoct = (*nodop).diasnoct;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	SetKey(sue|EMPSbyEMP, empres);
	if (GetRecord(sue|EMPSbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR){
		sprintf(aux,  "%d - %s", empres, SFld(sue|EMPS_DESCRIP));
		sprintf(semp,  "%s",SFld(sue|EMPS_DESCRIP));
	}

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);


		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
			break;
		case TOT_ALGO:
		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 

		SaltoPagina();
		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}

	if (NivCon > nivelactual)
		if ((*nodop).nnodleg != NULL)
			LisNNodleg((*nodop).nnodleg);

	if ((*nodop).nsig != NULL)
		LisNEmpres((*nodop).nsig);
}

void LisNNodleg(tnnodleg nodop)
{
	char aux [60];
	int nivelactual=NIV_LEGAJO;

	if (nodop == NULL)
		return;


	nodleg = (*nodop).nodleg;
    
    if(informaErrores)
		fprintf(stderr, "   %ld\n", nodleg);
	sprintf(aux, "   %ld", nodleg);
	SetKey(sue|PERbyEMP, empres, nodleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR){
		sprintf(aux,  "    %ld - %s", nodleg, SFld(sue|PER_APYNOM));
		sprintf(sleg, "%s", SFld(sue|PER_APYNOM));
	}

	cliefe = objefe = 0;
	strcpy(sobjefe, NULL_STR);
	GetCliObjEfectivo(empres, nodleg, FmDFld(fm0, FECHAH), &cliefe, &objefe);

	diasnoct = (*nodop).diasnoct;
	diasdiur = (*nodop).diasdiur;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA:
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}
		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 

		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}


	if (NivCon > nivelactual)
		if ((*nodop).nnodclie != NULL)
			LisNNodclie((*nodop).nnodclie);

	if ((*nodop).nsig != NULL)
		LisNNodleg((*nodop).nsig);
}

void LisNNodclie(tnnodclie nodop)
{
	char aux [110];
	int nivelactual=NIV_CLIENTE;

	if (nodop == NULL)
		return;

	nodclie = (*nodop).nodclie;


	sprintf(aux, "%d", nodclie);
	SetKey(bill|CLIENTE, (*nodop).nodclie);
	if(GetRecord(bill|CLIENTE, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		sprintf(aux,  "      %ld -%s", (*nodop).nodclie, SFld(bill|CLIENTE_RAZSOC));
		sprintf(scli, "%s", SFld(bill|CLIENTE_RAZSOC));

	}
	diasdiur = (*nodop).diasdiur;
	diasnoct = (*nodop).diasnoct;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}
		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 
		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}


 
	if (NivCon > nivelactual)
		if ((*nodop).nobjnod != NULL)
			LisNObjnod((*nodop).nobjnod);

	if ((*nodop).nsig != NULL)
		LisNNodclie((*nodop).nsig);
}

void LisNObjnod(tnobjnod nodop)
{
	char aux [60];
	int nivelactual=NIV_OBJET;
	int empaux;

	if (nodop == NULL)
		return;

	objnod = (*nodop).objnod;
	sprintf(aux, "%d", objnod);

	ofpag = 0;
	strcpy(zona, NULL_STR);
	SetKey(comerc|OBJETIVO, nodclie, (*nodop).objnod);
	if (GetRecord(comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		sprintf(aux,  "         %ld - %s", (*nodop).objnod, SFld(comerc|OBJETIVO_DESCRIP));
		sprintf(sobj, "%s", SFld(comerc|OBJETIVO_DESCRIP));
		if (nodclie == cliefe && objnod == objefe)
			strcpy(sobjefe, sobj);
		ofpag = LFld(comerc|OBJETIVO_OFPAG);
		SetKey(comerc|FILIAL, SFld(comerc|OBJETIVO_FILIAL));
		if (GetRecord(comerc|FILIAL, THIS_KEY, IO_NOT_LOCK) != ERROR)
			strcpy(zona, SFld(comerc|FILIAL_DESCRIP));
	}

	strcpy(tercer, "0");
	strcpy(subter, "0");
	divaur = depaur = 0;
	switch(empres) {
	case 1:
	case 2:
		empaux=empres+100;
	   	break;
	default:
		empaux=empres;
	}

	SetKey(intora|INTSTERC, empaux, nodclie, objnod, ORIG_NOVIA);
	if (GetRecord(intora|INTSTERC, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(tercer, SFld(intora|INTSTERC_TERCERO));
		strcpy(subter, SFld(intora|INTSTERC_SUBTERC));
		divaur = IFld(intora|INTSTERC_DIV);
		depaur = IFld(intora|INTSTERC_DEPTO);
	}

	objpad = clipad = 0;
	SetKey(billpro|OBJETREL, nodclie, objnod);
	if (GetRecord(billpro|OBJETREL, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		objpad = IFld(billpro|OBJETREL_OBJETCOM);
		clipad = LFld(billpro|OBJETREL_CLIECOM);
	}


	diasdiur = (*nodop).diasdiur;
	diasnoct = (*nodop).diasnoct;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}
		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 
		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}



	if (NivCon > nivelactual)
		if ((*nodop).npuenod != NULL)
			LisNPuenod((*nodop).npuenod);

	if ((*nodop).nsig != NULL)
		LisNObjnod((*nodop).nsig);
}

void LisNPuenod(tnpuenod nodop)
{
	char	aux[60];
	int nivelactual=NIV_PUESTO;

	if (nodop == NULL)
		return;

	sprintf (puenod, "%s", (*nodop).puenod);
	n_ptoser=(*nodop).n_ptoser;
	n_puesto=(*nodop).n_puesto;
	n_codint=(*nodop).n_codint;

	n_horini = (*nodop).n_horini;
	n_horfin = (*nodop).n_horfin;
	n_dia1   = (*nodop).n_dia1;
	n_dia2   = (*nodop).n_dia2;
	n_dia3   = (*nodop).n_dia3;
	n_dia4   = (*nodop).n_dia4;
	n_dia5   = (*nodop).n_dia5;
	n_dia6   = (*nodop).n_dia6;
	n_dia7   = (*nodop).n_dia7;
	sprintf(n_regim, "%s", (*nodop).n_regim);
	n_canvig = (*nodop).n_canvig;
	n_canpto = (*nodop).n_canpto;
	n_tipdia = (*nodop).n_tipdia;
	n_fecini = (*nodop).n_fecini;
	n_fecfin = (*nodop).n_fecfin;
	
	sprintf(aux, "            %s", puenod);

	SetKey(comerc|TPTOSERbyTIPPTO, (*nodop).n_ptoser);
	if(GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
		sprintf(aux,  "         %s - %s", (*nodop).puenod, SFld(comerc|TPTOSER_DESCOR));
	}

	diasdiur = (*nodop).diasdiur;
	diasnoct = (*nodop).diasnoct;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}

		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 
		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}


	if (NivCon > nivelactual)
		if ((*nodop).ndianod != NULL)
			LisNDianod((*nodop).ndianod);

	if ((*nodop).nsig != NULL)
		LisNPuenod((*nodop).nsig);
}


void LisNDianod(tndianod nodop)
{
	char aux[60];
	int nivelactual=NIV_DIA;

	if (nodop == NULL)
		return;

	dianod = (*nodop).dianod;


//	DToStr(dianod, aux, DFMT_SEPAR|DFMT_YEAR4);
	sprintf(aux,"               %.3D", dianod);

	diasdiur = (*nodop).diasdiur;
	diasnoct = (*nodop).diasnoct;
//	diasfran = (*nodop).diasfran;
//	diasfrat = (*nodop).diasfrat;
//	diasvaca = (*nodop).diasvaca;
//	diasvact = (*nodop).diasvact;
//	diasause = (*nodop).diasause;
	diasadel = (*nodop).diasadel;
	canpeg   = (*nodop).canpeg;

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}

		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			RpSetIFld(rp0, NOCHES,  (*nodop).diasnoct);
			break;
		} 
		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	}

	if (NivCon > nivelactual)
		if ((*nodop).nhordes != NULL)
			LisNHordes((*nodop).nhordes);

	if ((*nodop).nsig != NULL)
		LisNDianod((*nodop).nsig);
}

void LisNHordes(tnhordes nodop)
{
	char	aux[60];
	int nivelactual=NIV_HORA;

	if (nodop == NULL)
		return;

	hordes = (*nodop).hordes;
	horhas = (*nodop).horhas;
	condic = (*nodop).condic;
	sprintf(aux, "                  %.3T - %.3T", hordes, horhas);

	codnov = 0;
	if (condic == _AUSENTE_C) {
		SetKey(asist|ASISTEN, empres, dianod, nodleg, NULL_SHORT);
		if (FindRecord(asist|ASISTEN, NEXT_KEY|PARTIAL_KEY, 3) != ERROR)
			codnov = IFld(asist|ASISTEN_CODNOV);
		else {
			if (informaErrores)
				fprintf(stderr, "error ausente %d %D %d %d\n", empres, dianod, nodleg, nodclie);
			codnov = (*nodop).codnov;
		}
	}

	switch(*FmSFld(fm0, SALIDA)) {
	case 'I': 
	case 'T': 
		RpClearZone(rp0, ZDETA);
		RpSetFld(rp0, DESNIV, aux);

		RpSetFFld(rp0, HSNOR,   NULL_DOUBLE);
		RpSetFFld(rp0, HS25,    NULL_DOUBLE);
		RpSetFFld(rp0, HS35,    NULL_DOUBLE);
		RpSetFFld(rp0, HSNONOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS25NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HS35NOC, NULL_DOUBLE);
		RpSetFFld(rp0, HSFRA,   NULL_DOUBLE);
		RpSetFFld(rp0, HSFER,   NULL_DOUBLE);
		RpSetFFld(rp0, PEGADA,  NULL_DOUBLE);
		RpSetIFld(rp0, NOCHES,  NULL_SHORT );

		switch(FmIFld(fm0, TOTALIZ)) {
		case TOT_NADA: 
		case TOT_ALGO:
			if (nivelactual!=NivCon) {
				break;
				
			}

		case TOT_TODO: 
			RpSetFFld(rp0, HSNOR,   (*nodop).cantih[HSNORM]);
			RpSetFFld(rp0, HS25,    (*nodop).cantih[HSAL25]);
			RpSetFFld(rp0, HS35,    (*nodop).cantih[HSAL35]);
			RpSetFFld(rp0, HSNONOC, (*nodop).cantih[HSNONO]);
			RpSetFFld(rp0, HS25NOC, (*nodop).cantih[HS25NO]);
			RpSetFFld(rp0, HS35NOC, (*nodop).cantih[HS35NO]);
			RpSetFFld(rp0, HSFRA,   (*nodop).cantih[HSFRAN]);
			RpSetFFld(rp0, HSFER,   (*nodop).cantih[HSFERI]);
			RpSetFFld(rp0, PEGADA,  (*nodop).cantih[HSPEGA]);
			break;
		} 
		SaltoPagina();

		DoReport(rp0, ZDETA);
		break;
	case 'A': 
		ImprimeArchivo(nivelactual, (*nodop).cantih);
		break;
	case 'P': 
	case 'S': 
		GrabaPase(nivelactual, (*nodop).cantih);
		break;
	}

//DHC agregue esto para que no lo cuente dos veces

	if(informaErrores)
		fprintf(stderr, "pone en cero cliente %d\n", nodclie);
    diasnoct=diasdiur=diasadel=0;


	if ((*nodop).nsig != NULL)
		LisNHordes((*nodop).nsig);
}

void ImprimeArchivo(int p_nivact, double* canti)
{
	int v_i=0;
	char v_auxtpto[6];

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
			fprintf(salida,"%.3T - %.3T",hordes, horhas);
			break;
		}
	}

	for (v_i=p_nivact; v_i<=NIV_HORA; v_i ++) 
		fprintf(salida,"\t");


	sprintf(v_auxtpto, "%d", n_ptoser);
	if (EsParNov(empres, PARNOV_TPUHSNOR, dianod, v_auxtpto) && FmIFld(fm0, GUARDIA) == 1) {
		if (canti[HSNORM]+canti[HSNONO] > _HORAS_REGIMEN_DEFAULT*100) {
			canti[HSNORM] = _HORAS_REGIMEN_DEFAULT*100;
			canti[HSNONO] = 0;
		}
	}
	fprintf(salida,"%.2f\t", canti[HSNORM]/100);
	fprintf(salida,"%.2f\t", canti[HSAL25]/100);
	fprintf(salida,"%.2f\t", canti[HSAL35]/100);
	fprintf(salida,"%.2f\t", canti[HSFRAN]/100);
	fprintf(salida,"%.2f\t", canti[HSFERI]/100);
	fprintf(salida,"%.2f\t", canti[HSNONO]/100);
	fprintf(salida,"%.2f\t", canti[HS25NO]/100);
	fprintf(salida,"%.2f\t", canti[HS35NO]/100);
	fprintf(salida,"%.2f\t", canti[HSPEGA]/100);
	if (canti[HSPEGA] > 0)
		fprintf(salida,"%d\t",  canpeg);
	else
		fprintf(salida,"%d\t",  0);
	fprintf(salida,"%d\t",  diasnoct);
	fprintf(salida,"%d\t",  diasdiur/*+diasadel*/);
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
	fprintf(salida,"%c", condic);
//	fprintf(salida,"%d\t",  diasfran);
//	fprintf(salida,"%d\t",  diasfrat);
//	fprintf(salida,"%d\t",  diasvaca);
//	fprintf(salida,"%d\t",  diasvact);
//	fprintf(salida,"%d",    diasause);

	fprintf(salida,"\n");
}


void GrabaPase(int p_nivact, double* canti)
{
	char aux[2];
	char v_auxtpto[6];
	int tipoCierre = 0;

	if (p_nivact!=NIV_HORA)
		return;

	tipoCierre = FmIFld(fm0, COTICI);
	InitRecord(webint|WDETLIQ);
	g_total_registros_detalle++;

	SetIFld(webint|WDETLIQ_EMP,		empres);
	SetLFld(webint|WDETLIQ_NROLIQ,	FmLFld(fm0, LIQUI));
	SetDFld(webint|WDETLIQ_FECHAD,	FmDFld(fm0, FECHAD));
	SetDFld(webint|WDETLIQ_FECHAH,	FmDFld(fm0, FECHAH));
	SetLFld(webint|WDETLIQ_NROLEG,	nodleg);
	SetLFld(webint|WDETLIQ_CLIENTE,	nodclie);
	SetIFld(webint|WDETLIQ_OBJETIVO,	objnod);
	SetFld (webint|WDETLIQ_TERCERO,	tercer);
	SetFld (webint|WDETLIQ_SUBTERC,	subter);
	SetIFld(webint|WDETLIQ_PTOSER,	n_ptoser);
	SetIFld(webint|WDETLIQ_PUESTO,	n_puesto);
	SetIFld(webint|WDETLIQ_CODINT,	n_codint);

	sprintf(v_auxtpto, "%d", n_ptoser);
	if (EsParNov(empres, PARNOV_TPUHSNOR, dianod, v_auxtpto) && FmIFld(fm0, GUARDIA) == 1) {
		if (canti[HSNORM]+canti[HSNONO] > _HORAS_REGIMEN_DEFAULT*100) {
			canti[HSNORM] = _HORAS_REGIMEN_DEFAULT*100;
			canti[HSNONO] = 0.0;
		}
	}
	SetFFld(webint|WDETLIQ_HSNORM,	canti[HSNORM]);
	SetFFld(webint|WDETLIQ_HSAL25,	canti[HSAL25]);
	SetFFld(webint|WDETLIQ_HSAL35,	canti[HSAL35]);
	SetFFld(webint|WDETLIQ_HSFRA,	canti[HSFRAN]);
	SetFFld(webint|WDETLIQ_HSFER,	canti[HSFERI]);
	SetFFld(webint|WDETLIQ_HSNONO,	canti[HSNONO]);
	SetFFld(webint|WDETLIQ_HS25NO,	canti[HS25NO]);
	SetFFld(webint|WDETLIQ_HS35NO,	canti[HS35NO]);
	SetFFld(webint|WDETLIQ_HSPEGA,	canti[HSPEGA]);
	SetFFld(webint|WDETLIQ_HSNOFE,	canti[HSNOFE]);
	SetFFld(webint|WDETLIQ_HS25FE,	canti[HS25FE]);
	SetFFld(webint|WDETLIQ_HS35FE,	canti[HS35FE]);
	SetFFld(webint|WDETLIQ_HSNONOFE,	canti[HNOFEN]);
	SetFFld(webint|WDETLIQ_HS25NOFE,	canti[H25FEN]);
	SetFFld(webint|WDETLIQ_HS35NOFE,	canti[H35FEN]);
	SetFFld(webint|WDETLIQ_HSNOFR,	canti[HSNOFR]);
	SetFFld(webint|WDETLIQ_HS25FR,	canti[HS25FR]);
	SetFFld(webint|WDETLIQ_HS35FR,	canti[HS35FR]);
	SetFFld(webint|WDETLIQ_HSNONOFR,	canti[HNOFRN]);
	SetFFld(webint|WDETLIQ_HS25NOFR,	canti[H25FRN]);
	SetFFld(webint|WDETLIQ_HS35NOFR,	canti[H35FRN]);

	SetIFld(webint|WDETLIQ_CANPEG,	canpeg);
	SetIFld(webint|WDETLIQ_JORNOC,	diasnoct);
	SetIFld(webint|WDETLIQ_JORDIU,	diasdiur/*+diasadel*/);


	SetFFld(webint|WDETLIQ_TOHSNO,	canti[HSNORM]+canti[HSNONO]/*+canti[HSNOFE]+canti[HNOFEN]*/);
	SetFFld(webint|WDETLIQ_TOHSEX,	canti[HSAL25]*1.25+canti[HSAL35]*1.35+canti[HS25NO]*1.25+canti[HS35NO]*1.35+canti[HSFRAN]*2+canti[HSFERI]*2);
	SetFFld(webint|WDETLIQ_TOHSNOR,	FFld(webint|WDETLIQ_TOHSNO)+FFld(webint|WDETLIQ_TOHSEX));

	strcpy(regpue, NULL_STR);
	SetKey(operac|PUESTOS, nodclie, objnod, n_ptoser, n_puesto);
	if (GetRecord(operac|PUESTOS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy(regpue, SFld(operac|PUESTOS_REGIM));
	}

	SetFld (webint|WDETLIQ_REGPUE,	regpue);
	SetIFld(webint|WDETLIQ_DIVAUR,	divaur);
	SetIFld(webint|WDETLIQ_DEPAUR,	depaur);

	if (cliefe == nodclie && objefe == objnod)
		SetIFld(webint|WDETLIQ_CECOEF,	1);
	else
		SetIFld(webint|WDETLIQ_CECOEF,	0);

	if (IFld(webint|WDETLIQ_CECOEF))
		SetFld (webint|WDETLIQ_DECECO,	sobjefe);
	else
		SetFld (webint|WDETLIQ_DECECO,   "NULL");

	SetLFld(webint|WDETLIQ_OFPAG,	ofpag);
	SetFld (webint|WDETLIQ_ZONA,		zona);

	sprintf(aux, "%c", condic);
	SetFld (webint|WDETLIQ_CONDIC,	aux);
	SetIFld(webint|WDETLIQ_CODNOV,	codnov);
	SetDFld(webint|WDETLIQ_FECASI,	dianod);

	SetIFld(webint|WDETLIQ_OBJPAD,	objpad);
	SetLFld(webint|WDETLIQ_CLIPAD,	clipad);
	
	//ACHIMURIS
	SetTFld(webint|WDETLIQ_HORAENT,		hordes);
	SetTFld(webint|WDETLIQ_HORASAL,		horhas);
	SetIFld(webint|WDETLIQ_ESTADO,		CONST_ESTADO_TERM_OK); //CONST_ESTADO_EN_EJECU);
	SetIFld(webint|WDETLIQ_COTICI,		tipoCierre);
	SetDFld(webint|WDETLIQ_FEINPR,		g_fechaEnvioInicioProceso);
	SetTFld(webint|WDETLIQ_HOINPR,		g_horarioEnvioInicioProceso);

	if(informaErrores)
		fprintf(stderr, "graba legajo %d %c %D n %d d %d a %d codnov %d\n", nodleg, condic, dianod, diasnoct, diasdiur, diasadel, codnov);

	PutRecord(webint|WDETLIQ);
}

void GrabaLiquida(int p_emp, long p_nroliq, long p_cliented, long p_clienteh, int p_objetivod, int p_objetivoh, long  p_nrolegd, long  p_nrolegh, DATE p_fechad, DATE p_fechah, int p_retro, int p_tipocierre)
{
	if (g_total_registros_detalle > 0)
	{
		SetLFld(webint|WCABLIQ_NROLIQ,		p_nroliq);
		SetIFld(webint|WCABLIQ_EMP, 		p_emp);
		SetLFld(webint|WCABLIQ_CLIENTED, 	p_cliented);
		SetLFld(webint|WCABLIQ_CLIENTEH, 	p_clienteh);
		SetIFld(webint|WCABLIQ_OBJETIVOD, 	p_objetivod);
		SetIFld(webint|WCABLIQ_OBJETIVOH, 	p_objetivoh);
		SetLFld(webint|WCABLIQ_NROLEGD, 	p_nrolegd);
		SetLFld(webint|WCABLIQ_NROLEGH, 	p_nrolegh);
		SetDFld(webint|WCABLIQ_FECHAD, 		p_fechad);
		SetDFld(webint|WCABLIQ_FECHAH, 		p_fechah);
		SetIFld(webint|WCABLIQ_COTICI,		p_tipocierre);
		SetDFld(webint|WCABLIQ_FEINPR,		g_fechaEnvioInicioProceso);
		SetTFld(webint|WCABLIQ_HOINPR,		g_horarioEnvioInicioProceso);
		
		
		
		
		//SetIFld(webint|WCABLIQ_RETRO, 		p_retro);
		SetIFld(webint|WCABLIQ_ESTADO,		CONST_ESTADO_TERM_OK);	//Lo grabamos directamente en 2 porque es el último en grabar...
		
		//ACHIMURIS
		SetLFld(webint|WCABLIQ_CANTIREG,	g_total_registros_detalle);
		
		if(informaErrores)
			fprintf(stderr, "graba cab p_emp: %d p_nroliq: %ld p_cliented: %ld p_clienteh: %d p_objetivod: %d p_objetivoh: %d p_nrolegd: %ld p_nrolegh: %ld p_fechad: %.3D p_fechah: %.3D p_retro: %d",  p_emp, p_nroliq, p_cliented, p_clienteh, p_objetivod, p_objetivoh, p_nrolegd, p_nrolegh, p_fechad, p_fechah, p_retro);

		PutRecord(webint|WCABLIQ);
		
	}
	
}


void GrabaDeallesDeLiquidacion(int p_emp, long p_nroliq, int p_estado_origen, int p_estado_destino, DATE p_fechad, DATE p_fechah)
{
//	int z = 0;
	//grabar (emp, nroliq, nroleg, fechad, fechah, estado);
	SetKey(webint|WDETLIQbyGRABAR, p_emp, p_nroliq, MIN_LONG, MIN_DATE, MIN_DATE, MIN_SHORT);
	
	
	DisplayMsg(FALSE, "Marcando registros");
	WiRefresh();

	while(GetRecord(webint|WDETLIQbyGRABAR, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR)
	{  
//		fprintf(stderr, "Leyendo %d\n", z);
		
		if (IFld(webint|WDETLIQ_ESTADO) !=  p_estado_origen)
			continue;
		if(DFld(webint|WDETLIQ_FECHAD)  != p_fechad)
			continue;
		if(DFld(webint|WDETLIQ_FECHAH)  != p_fechah)
			continue;
 
//		fprintf(stderr, "Leyendo FINALMENTE %d\n", z);
//		z++;
		
		SetIFld(webint|WDETLIQ_ESTADO, p_estado_destino);
		PutRecord(webint|WDETLIQ);		
		
	}

	DisplayMsg(FALSE, "Pase OK");
	WiRefresh();

	
}
