/********************************************************************
*
* MODULE & VERSION : @(#)mvaspu.c	1.1 
* DATE             : 10/09/28 
* TIME             : 15:27:45 
*
* CREATED          : 21/04/09
*
* DESCRIPTION:
*      Movimientos de asignaciones de una empresa a otra
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "mvaspu.fmh"
#include "mvaspu.rph"
#include "operac.h"
#include "comerc.h"
#include "filial.h"
#include "sue.sch"
#include "operac.sch"
#include "comgral.sch"
#include "comerc.sch"
#include "bill.sch"
#include "sue.sch"
#include "asist.sch"

#define SALTOPAG	30

/* ------------------------------------------------ COMIENZO DE DECLARACIONES PARA LISTAS ------------------------------------------------- */
	// TABLAS
#define _TABLA_ASIG   1
#define _TABLA_ASIGH  2

	// MOVIMIENTO
#define _MOVIM_ALTA   1
#define _MOVIM_BAJA   2

// minimo cliente que valida regimen
#define  _MINCLI	3000

// Respeta las validaciones o igual procesa
#define _FORZADO	  0


// MODO DE PROCESO
#define _MODPRO_REPORT	1
#define _MODPRO_PROCES	2
#define _MODPRO_REPPRO	3

typedef struct stnmovimien * tnmovimien;
typedef struct stnmovimien {
	int		movimien;
	int		tipo;
	int		tabla;
	int		tabla_o;
	int		emp;
	long	cliente;
	int		objetivo;
	int		ptoser;
	int		puesto;
	long	nroleg;
	char	vigil;
	char	efect;
	DATE	fecasig;
	TIME	horent;
	TIME	horsal;
	char	dia1;
	char	dia2;
	char	dia3;
	char	dia4;
	char	dia5;
	char	dia6;
	char	dia7;
	long	reepl;
	DATE	ffranco;
	char	regim[10];
	DATE	fechas;
	DATE	fecbaj;
	int		numfran;
	bool	francero;
	int		nroint;
	char	tipodia;
	int		codrol;
	int		fila;
	int		colum;
	char	regpto[8];
	int		motivo;
	tnmovimien	nsig;
} stnmovimien;


/* Funciones Privadas */
void IniciMovim();
static tnmovimien AcuNMovimien(tnmovimien, tnmovimien*);
static void LisNMovimien(tnmovimien);
static void BorNMovimien(tnmovimien);
static void AbrirReporte();
static void LeerCliente (long p_cliente);
static void ImprimeLog(char * p_msg); 
static void BorrarTablas(tnmovimien nodop);
static bool ValidaPuestoDestino(long p_cliente, int p_objet, int  p_codintd);
static DATE FechaPrimerFrancoBaja(DATE p_fecbase, int numdia);
static void AsigHora(int p_emp, long p_cliente, int p_objet, long p_nroleg, int p_ptoser, int p_puesto, int p_nroint,
					 DATE dia, TIME * hsentrada, TIME * hssalida, char *p_regim);
static fm_status after(form fm, fmfield fno, int row);
tnmovimien	inicio;

TIME	horent, horsal;
int		partial;
int		movimien, tipo, tabla, tabla_o, emp, objetivo, ptoser, puesto, numfran, nroint, codrol, fila, colum, motivo;
long	cliente, nroleg, reepl;
bool	francero;
char	vigil, efect, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim[10], tipodia, regpto[8];
DATE	fecasig, ffranco, fechas, fecbaj;

/* --------------------------------------------------- FIN DE DECLARACIONES PARA LISTAS --------------------------------------------------- */
/* Funciones privadas */
static fm_status before(form, fmfield, int);
//bool ValidaobjetivosDestino(int p_emp_o, long p_nroleg, DATE  p_fecha, int p_emp_d);
void MuevoAsig(int, long, int, long, int, int, int, DATE, int);
void ProximoFranco(short p_emp, long p_nroleg, char * p_vigil, DATE p_afranco, short p_numfran, DATE p_fasig, DATE p_fecact, DATE *p_ffranco, int *p_nfran);

/* Declaraciones globales */
int 	i, linrp=0;
form 	fm0;
report 	rp0;
FILE 	*ent,
		*alog, 
		*sal;
char 	nomarch[100];
schema 	sue, ope, comerc, bill, asist;
char 	msgaux[500];
bool 	reporte= FALSE, 
		grabar = FALSE,
		archivo= FALSE;

/* Programa principal */
wcmd(mvaspu, 1.1 09/28/10)
{
	fm_cmd cmd;
	bool v_msg=FALSE;
	char v_filial[8];

	sue   = OpenSchema("sue", IO_EABORT);
	asist = OpenSchema("asist", IO_EABORT);
	ope   = OpenSchema("operac", IO_EABORT);
	comerc= OpenSchema("comerc", IO_EABORT);
	bill  = OpenSchema("bill", IO_EABORT);

	fm0 = OpenForm("mvaspu", FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ( (cmd=DoForm(fm0, before, after)) != FM_EXIT ) {

		if (cmd!=FM_UPDATE) continue;

		AbrirReporte();

		// Recorre Multi
		BeginTransaction();

		if (v_msg) fprintf(stderr, "Emp %d- Cliente %ld- Objetivo %d\n", FmIFld(fm0, EMPO), FmLFld(fm0, CLIENTO), FmIFld(fm0, OBJETIV));

		//	primary key(emp, cliente, objetivo, nroleg, ptoser, puesto, nroint),

		SetKey (ope|ASIGbyEMP, FmIFld(fm0, EMPO), FmLFld(fm0, CLIENTO), FmIFld(fm0, OBJETIV), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		while(GetRecord(ope|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3)!=ERROR) {

			if (IFld(ope|ASIG_PUESTO)!= FmIFld(fm0,CODINTO))
				continue;

			IniciMovim();
			sprintf (v_filial, "%s", NULL_STR);
			sprintf(msgaux, "%s", NULL_STR );
		   	ImprimeLog(msgaux);
			sprintf(msgaux, "Procesando Legajo %ld - ", LFld(ope|ASIG_NROLEG));
		   	ImprimeLog(msgaux);


			if (v_msg) fprintf(stderr, "Nroleg %ld Puesto %d-%d-%d\n", LFld(ope|ASIG_NROLEG), IFld(ope|ASIG_PTOSER), IFld(ope|ASIG_PUESTO), IFld(ope|ASIG_NROINT));

			MuevoAsig (IFld(ope|ASIG_EMP), LFld(ope|ASIG_CLIENTE),IFld(ope|ASIG_OBJETIVO), LFld(ope|ASIG_NROLEG), IFld(ope|ASIG_PTOSER), IFld(ope|ASIG_PUESTO), 
			           IFld(ope|ASIG_NROINT), FmDFld(fm0, FECMOV), FmIFld(fm0, CODINTD)); 

			if (reporte){
				DoReport(rp0, ZENC);
				linrp+=4;
			}
			if (archivo){
				fprintf(sal, "Nro.Movimiento\tAccion\tEmpresa\tCliente\t\tObjetivo\t\tVig.\tEfect.\tPuesto\t\t\tRegimen\tFecha Desde\tFecha Hasta\tFecha Franco\tNum.Franco\t");
				fprintf(sal, "Dia1\tDia2\tDia3\tDia4\tDia5\tDia6\tDia7\tHora Entrada\tHora Salida\tMotivo\n");
			}

			LisNMovimien(inicio);
			BorNMovimien(inicio);

			if (reporte){
				DoReport(rp0, FLIN);
				linrp++;
			}
		}

		EndTransaction();

		fclose(alog);

        if (reporte)
        	CloseReport(rp0);

		if (archivo)
			fclose(sal);

	
		WiRefresh();
	}
}

void MuevoAsig(int p_emp, long p_cliente, int p_objetivo, long p_nroleg, int p_puesto, int p_codint, int p_nroint, DATE  p_fecha, int p_codintd) 
{

	bool 	v_msg=FALSE;

	char 	v_aux[20];

	int		v_nfranco = NULL_SHORT;   // Numero del proximo franco
	int 	v_dia=NULL_SHORT,
			v_mes=NULL_SHORT,
			v_ano=NULL_SHORT;

	DATE 	v_fec_ini_ori = NULL_DATE, //fecha inicio de origen
			v_fec_fin_ori = NULL_DATE, //fecha fin de origen
			v_fec_ini_des = NULL_DATE, //fecha inicio de destino
			v_fec_fin_des = NULL_DATE, //fecha fin de destinon
			v_fec_ini_baj = NULL_DATE, //fecha inicio de destino
			v_fec_fin_baj = NULL_DATE, //fecha fin de destinon
			v_franco      = NULL_DATE, //fecha de proximo franco
			v_frabaj	  = NULL_DATE; //fecha de primer franco para baja (1er domingo despues de asignar por defecto)

	TIME 	hsentrada, 
			hssalida;

	v_frabaj = FechaPrimerFrancoBaja(p_fecha, dia(0));


	if (!IsNull(ope|ASIG_FECHAS) && DFld(ope|ASIG_FECHAS)<p_fecha)
		return;

	v_fec_ini_ori =	v_fec_fin_ori = v_fec_ini_des = v_fec_fin_des = v_fec_ini_baj = v_fec_fin_baj = NULL_DATE; 

	v_fec_ini_ori = DFld(ope|ASIG_FECASIG);
	v_fec_fin_ori = p_fecha -1;
	v_fec_ini_des = p_fecha;
	v_fec_fin_des = DFld(ope|ASIG_FECHAS);
	v_fec_ini_baj = p_fecha;
	v_fec_fin_baj = p_fecha;



	if (v_fec_fin_ori >= v_fec_ini_ori) {

		// Guarda Origen
		movimien ++;
		tipo     = _MOVIM_ALTA;
		tabla    = _TABLA_ASIGH;
		tabla_o  = _TABLA_ASIG;
		emp      = p_emp;
		cliente  = p_cliente;
		objetivo = p_objetivo;
		ptoser   = p_puesto;
		puesto   = p_codint;
		nroint   = p_nroint;
		nroleg   = p_nroleg;
		vigil    = *SFld(ope|ASIG_VIGIL);
		efect    = *SFld(ope|ASIG_EFECT); 
		fecasig  = v_fec_ini_ori;
		horent   = TFld(ope|ASIG_HSENT);
		horsal   = TFld(ope|ASIG_HSSAL);
		dia1     = *SFld(ope|ASIG_DIA1);
		dia2     = *SFld(ope|ASIG_DIA2);
		dia3     = *SFld(ope|ASIG_DIA3);
		dia4     = *SFld(ope|ASIG_DIA4);
		dia5     = *SFld(ope|ASIG_DIA5);
		dia6     = *SFld(ope|ASIG_DIA6);
		dia7     = *SFld(ope|ASIG_DIA7);
		reepl    = LFld(ope|ASIG_REEMPL);
		ffranco  = DFld(ope|ASIG_FFRANCO);
		sprintf(regim, "%s", SFld(ope|ASIG_REGIM));
		fechas   = DFld(ope|ASIG_FECHAS);
		fecbaj   = v_fec_fin_ori;
		numfran  = IFld(ope|ASIG_NUMFRAN);
		francero = IFld(ope|ASIG_FRANCERO);
		tipodia  = *SFld(ope|ASIG_TIPODIA);
		codrol   = IFld(ope|ASIG_CODROL);
		fila     = IFld(ope|ASIG_FILA);
		colum    = IFld(ope|ASIG_COLUM);
		sprintf(regpto, "%s", SFld(ope|ASIG_REGPTO));
		motivo   = CAMBIO_PUESTO;

		inicio = AcuNMovimien(inicio, &inicio);
	}


	v_nfranco = IFld(ope|ASIG_NUMFRAN);
	v_franco  = DFld (ope|ASIG_FFRANCO);

	if (*SFld(ope|ASIG_EFECT)==EFECTIVO[0]){
		ProximoFranco (IFld(ope|ASIG_EMP), LFld(ope|ASIG_NROLEG), SFld(ope|ASIG_VIGIL), DFld (ope|ASIG_FFRANCO),
	                   IFld(ope|ASIG_NUMFRAN), DFld (ope|ASIG_FECASIG), v_fec_ini_des, &v_franco, &v_nfranco);
	}

	hsentrada=TFld(ope|ASIG_HSENT);
	hssalida=TFld(ope|ASIG_HSSAL);

	if (IFld(ope|ASIG_FRANCERO))
		if (strcmp (SFld(ope|ASIG_REGIM), REG_ESP)==0 || strcmp (SFld(ope|ASIG_REGIM), REG_ESP_3)==0){
			PushRecord(ope|ASIG);
			AsigHora(p_emp, p_cliente, p_objetivo, p_nroleg, p_puesto, p_codint, p_nroint, v_fec_ini_des, &hsentrada, &hssalida, SFld(ope|ASIG_REGIM));
			PopRecord(ope|ASIG);
		}

	// Guarda Destino
	movimien ++;
	tipo     = _MOVIM_ALTA;
	tabla    = _TABLA_ASIG;
	tabla_o  = _TABLA_ASIG;
	emp      = p_emp;
	cliente  = p_cliente;
	objetivo = objetivo;
	ptoser   = p_puesto;
	puesto   = p_codintd;
	nroint   = p_nroint;
	nroleg   = p_nroleg;
	vigil    = *SFld(ope|ASIG_VIGIL);
	efect    = *SFld(ope|ASIG_EFECT); 
	fecasig  = v_fec_ini_des;
	horent   = hsentrada;
	horsal   = hssalida;
	dia1     = *SFld(ope|ASIG_DIA1);
	dia2     = *SFld(ope|ASIG_DIA2);
	dia3     = *SFld(ope|ASIG_DIA3);
	dia4     = *SFld(ope|ASIG_DIA4);
	dia5     = *SFld(ope|ASIG_DIA5);
	dia6     = *SFld(ope|ASIG_DIA6);
	dia7     = *SFld(ope|ASIG_DIA7);
	reepl    = LFld(ope|ASIG_REEMPL);
	sprintf(regim, "%s", SFld(ope|ASIG_REGIM));
	fechas   = DFld(ope|ASIG_FECHAS);

	if (*SFld(ope|ASIG_EFECT)==EFECTIVO[0]){
		fecbaj   = v_fec_fin_des;
		numfran  = v_nfranco;
		ffranco  = v_franco;
	}
	else {
		if (v_msg) fprintf(stderr, "MuevoAsig: v_fec_fin_des %.3D\n", v_fec_fin_des);
		v_dia=Day(v_fec_ini_des);
		v_mes=Month(v_fec_ini_des);
		v_ano=Year(v_fec_ini_des)+2;

		sprintf(v_aux, "%02d%02d%04d",v_dia, v_mes, v_ano);

		fecbaj   = StrToD(v_aux);

		if (v_msg) fprintf(stderr, "MuevoAsig: Fecha final %.3D\n", fecbaj);
		numfran  = NULL_SHORT;
		ffranco  = NULL_DATE;
	}
	francero = IFld(ope|ASIG_FRANCERO);
	tipodia  = *SFld(ope|ASIG_TIPODIA);
	codrol   = IFld(ope|ASIG_CODROL);
	fila     = IFld(ope|ASIG_FILA);
	colum    = IFld(ope|ASIG_COLUM);
	sprintf(regpto, "%s", SFld(ope|ASIG_REGPTO));
	motivo   = NULL_SHORT;

	inicio = AcuNMovimien(inicio, &inicio);

	

	// Borrar asignacion
	movimien ++;
	tipo     = _MOVIM_BAJA;
	tabla    = _TABLA_ASIG;
	tabla_o  = _TABLA_ASIG;
	emp      = p_emp;
	cliente  = p_cliente;
	objetivo = p_objetivo;
	ptoser   = p_puesto;
	puesto   = p_codint;
	nroint   = p_nroint;
	nroleg   = p_nroleg;
	vigil    = *SFld(ope|ASIG_VIGIL);
	efect    = *SFld(ope|ASIG_EFECT); 
	fecasig  = DFld(ope|ASIG_FECASIG);
	horent   = TFld(ope|ASIG_HSENT);
	horsal   = TFld(ope|ASIG_HSSAL);
	dia1     = *SFld(ope|ASIG_DIA1);
	dia2     = *SFld(ope|ASIG_DIA2);
	dia3     = *SFld(ope|ASIG_DIA3);
	dia4     = *SFld(ope|ASIG_DIA4);
	dia5     = *SFld(ope|ASIG_DIA5);
	dia6     = *SFld(ope|ASIG_DIA6);
	dia7     = *SFld(ope|ASIG_DIA7);
	reepl    = LFld(ope|ASIG_REEMPL);
	ffranco  = DFld(ope|ASIG_FFRANCO);
	sprintf(regim, "%s", SFld(ope|ASIG_REGIM));
	fechas   = DFld(ope|ASIG_FECHAS);
	fecbaj   = DFld(ope|ASIG_FECBAJ);
	numfran  = IFld(ope|ASIG_NUMFRAN);
	francero = IFld(ope|ASIG_FRANCERO);
	tipodia  = *SFld(ope|ASIG_TIPODIA);
	codrol   = IFld(ope|ASIG_CODROL);
	fila     = IFld(ope|ASIG_FILA);
	colum    = IFld(ope|ASIG_COLUM);
	sprintf(regpto, "%s", SFld(ope|ASIG_REGPTO));
	motivo   = NULL_DATE;

	inicio = AcuNMovimien(inicio, &inicio);

}

void IniciMovim() 
{
	inicio   = NULL;
	movimien = 0;
	tipo     = NULL_SHORT;
	tabla    = NULL_SHORT;
	tabla_o  = NULL_SHORT;
	emp      = NULL_SHORT;
	cliente  = NULL_LONG;
	objetivo = NULL_SHORT;
	ptoser   = NULL_SHORT;
	puesto   = NULL_SHORT;
	nroleg   = NULL_LONG;
	vigil    = '\0';
	efect    = '\0';
	fecasig  = NULL_DATE;
	horent    = NULL_TIME;
	horsal    = NULL_TIME;
	dia1     = '\0';
	dia2     = '\0';
	dia3     = '\0';
	dia4     = '\0';
	dia5     = '\0';
	dia6     = '\0';
	dia7     = '\0';
	reepl    = NULL_LONG;
	ffranco  = NULL_DATE;
	regim[0] = '\0';
	fechas   = NULL_DATE;
	fecbaj   = NULL_DATE;
	numfran  = NULL_SHORT;
	francero = NULL_BOOL;
	nroint   = NULL_SHORT;
	tipodia  = '\0';
	codrol   = NULL_SHORT;
	fila     = NULL_SHORT;
	colum    = NULL_SHORT;
	regpto[0]= '\0';
	motivo   = NULL_SHORT;
	
}

static tnmovimien AcuNMovimien(tnmovimien nodop, tnmovimien * nantp)
{
	tnmovimien naux;

	if (nodop == NULL) {
		nodop = (tnmovimien) malloc (sizeof(stnmovimien));
		(*nodop).movimien = movimien;
		(*nodop).tipo = tipo;
		(*nodop).tabla = tabla;
		(*nodop).tabla_o = tabla_o;
		(*nodop).emp = emp;
		(*nodop).cliente = cliente;
		(*nodop).objetivo = objetivo;
		(*nodop).ptoser = ptoser;
		(*nodop).puesto = puesto;
		(*nodop).nroleg = nroleg;
		(*nodop).vigil = vigil;
		(*nodop).efect = efect;
		(*nodop).fecasig = fecasig;
		(*nodop).horent = horent;
		(*nodop).horsal = horsal;
		(*nodop).dia1 = dia1;
		(*nodop).dia2 = dia2;
		(*nodop).dia3 = dia3;
		(*nodop).dia4 = dia4;
		(*nodop).dia5 = dia5;
		(*nodop).dia6 = dia6;
		(*nodop).dia7 = dia7;
		(*nodop).reepl = reepl;
		(*nodop).ffranco = ffranco;
		sprintf((*nodop).regim,"%s", regim);
		(*nodop).fechas = fechas;
		(*nodop).fecbaj = fecbaj;
		(*nodop).numfran = numfran;
		(*nodop).francero = francero;
		(*nodop).nroint = nroint;
		(*nodop).tipodia = tipodia;
		(*nodop).codrol = codrol;
		(*nodop).fila = fila;
		(*nodop).colum = colum;
		sprintf((*nodop).regpto,"%s", regpto);
		(*nodop).motivo = motivo;
		(*nodop).nsig = NULL;
	}
	else {
		if ((*nodop).movimien == movimien) {
			(*nodop).tipo = tipo;
			(*nodop).tabla = tabla;
			(*nodop).tabla_o = tabla_o;
			(*nodop).emp = emp;
			(*nodop).cliente = cliente;
			(*nodop).objetivo = objetivo;
			(*nodop).ptoser = ptoser;
			(*nodop).puesto = puesto;
			(*nodop).nroleg = nroleg;
			(*nodop).vigil = vigil;
			(*nodop).efect = efect;
			(*nodop).fecasig = fecasig;
			(*nodop).horent = horent;
			(*nodop).horsal = horsal;
			(*nodop).dia1 = dia1;
			(*nodop).dia2 = dia2;
			(*nodop).dia3 = dia3;
			(*nodop).dia4 = dia4;
			(*nodop).dia5 = dia5;
			(*nodop).dia6 = dia6;
			(*nodop).dia7 = dia7;
			(*nodop).reepl = reepl;
			(*nodop).ffranco = ffranco;
			sprintf((*nodop).regim,"%s", regim);
			(*nodop).fechas = fechas;
			(*nodop).fecbaj = fecbaj;
			(*nodop).numfran = numfran;
			(*nodop).francero = francero;
			(*nodop).nroint = nroint;
			(*nodop).tipodia = tipodia;
			(*nodop).codrol = codrol;
			(*nodop).fila = fila;
			(*nodop).colum = colum;
			sprintf((*nodop).regpto,"%s", regpto);
			(*nodop).motivo = motivo;
		}
		else {
			if ((*nodop).movimien < movimien)
				(*nodop).nsig = AcuNMovimien((*nodop).nsig, &(*nodop).nsig);
			else {
				naux  = nodop;
				if (*nantp != nodop)
					*nantp = nodop;
				nodop = (tnmovimien) malloc (sizeof(stnmovimien));
				(*nodop).movimien = movimien;
				(*nodop).tipo = tipo;
				(*nodop).tabla = tabla;
				(*nodop).tabla_o = tabla_o;
				(*nodop).emp = emp;
				(*nodop).cliente = cliente;
				(*nodop).objetivo = objetivo;
				(*nodop).ptoser = ptoser;
				(*nodop).puesto = puesto;
				(*nodop).nroleg = nroleg;
				(*nodop).vigil = vigil;
				(*nodop).efect = efect;
				(*nodop).fecasig = fecasig;
				(*nodop).horent = horent;
				(*nodop).horsal = horsal;
				(*nodop).dia1 = dia1;
				(*nodop).dia2 = dia2;
				(*nodop).dia3 = dia3;
				(*nodop).dia4 = dia4;
				(*nodop).dia5 = dia5;
				(*nodop).dia6 = dia6;
				(*nodop).dia7 = dia7;
				(*nodop).reepl = reepl;
				(*nodop).ffranco = ffranco;
				sprintf((*nodop).regim,"%s", regim);
				(*nodop).fechas = fechas;
				(*nodop).fecbaj = fecbaj;
				(*nodop).numfran = numfran;
				(*nodop).francero = francero;
				(*nodop).nroint = nroint;
				(*nodop).tipodia = tipodia;
				(*nodop).codrol = codrol;
				(*nodop).fila = fila;
				(*nodop).colum = colum;
				sprintf((*nodop).regpto,"%s", regpto);
				(*nodop).motivo = motivo;
				(*nodop).nsig = naux;
				*nantp = nodop;
			}
		}
	}

	return nodop;
}

static void LisNMovimien(tnmovimien nodop)
{
	char	aux[60];

	if (nodop == NULL)
		return;

	movimien = (*nodop).movimien;

	if (reporte) {
		RpSetIFld(rp0, R_MOVIMIEN, movimien);
		
		// MOVIMIENTO
		switch((*nodop).tipo){
			case _MOVIM_ALTA: 
				sprintf (aux, "Graba");
				break;

			case _MOVIM_BAJA: 
				sprintf (aux, "Borra");
				break;
		}	

		// TABLAS
		switch((*nodop).tabla) {
			case _TABLA_ASIG: 
				sprintf (aux, "%s Actual", aux );
				break;

			case _TABLA_ASIGH: 
				sprintf (aux, "%s Historica", aux );
				break;
		}
		
		RpSetFld (rp0, R_ACCION, aux);
		RpSetIFld(rp0, R_EMP,    (*nodop).emp);

		RpSetLFld(rp0, R_CLI,    (*nodop).cliente);
		LeerCliente ((*nodop).cliente);
		RpSetFld (rp0, R_DCLI,   SFld(bill|CLIENTE_RAZSOC));

    	RpSetIFld(rp0, R_OBJ,     (*nodop).objetivo);
		RpSetFld (rp0, R_DOBJ,    GetObjDescrip((*nodop).cliente, (*nodop).objetivo));

		sprintf (aux, "%c",       (*nodop).vigil);
    	RpSetFld (rp0, R_VR  ,    aux);

		sprintf (aux, "%c",       (*nodop).efect);
    	RpSetFld (rp0, R_EF  ,    aux);
    	RpSetIFld(rp0, R_PTOSER,  (*nodop).ptoser);
    	RpSetIFld(rp0, R_PUESTO,  (*nodop).puesto);
		RpSetFld (rp0, R_DPTOSER, GetDescPto((*nodop).ptoser));
		RpSetFld (rp0, R_REGIM,   (*nodop).regim);
		RpSetDFld(rp0, R_FECASIG, (*nodop).fecasig);

		RpSetDFld(rp0, R_FECHAS,  (*nodop).fecbaj);

		RpSetDFld(rp0, R_FFRANCO, (*nodop).ffranco);
		RpSetIFld(rp0, R_NFRANCO, (*nodop).numfran);

		sprintf (aux, "%c",       (*nodop).dia1);
		RpSetFld (rp0, R_DIA1,    aux);

		sprintf (aux, "%c",       (*nodop).dia2);
		RpSetFld (rp0, R_DIA2,    aux);

		sprintf (aux, "%c",       (*nodop).dia3);
		RpSetFld (rp0, R_DIA3,    aux);

		sprintf (aux, "%c",       (*nodop).dia4);
		RpSetFld (rp0, R_DIA4,    aux);

		sprintf (aux, "%c",       (*nodop).dia5);
		RpSetFld (rp0, R_DIA5,    aux);

		sprintf (aux, "%c",       (*nodop).dia6);
		RpSetFld (rp0, R_DIA6,    aux);

		sprintf (aux, "%c",       (*nodop).dia7);
		RpSetFld (rp0, R_DIA7,    aux);

		RpSetTFld(rp0, R_HSENT,  (*nodop).horent);
		RpSetTFld(rp0, R_HSSAL,  (*nodop).horsal);

		RpSetIFld(rp0, R_MOTD,   (*nodop).motivo);

		DoReport(rp0, LINEA);
		linrp++;


		if (linrp>=SALTOPAG) {
			linrp=0;
			DoReport(rp0, FLIN);
			RpEjectPage(rp0);
			DoReport(rp0, ZENC);
		}

	}
	if (archivo) {
		fprintf(sal, "%d\t",            movimien);
		
		// MOVIMIENTO
		switch((*nodop).tipo){
			case _MOVIM_ALTA: 
				sprintf (aux, "Graba");
				break;

			case _MOVIM_BAJA: 
				sprintf (aux, "Borra");
				break;
		}	

		// TABLAS
		switch((*nodop).tabla) {
			case _TABLA_ASIG: 
				sprintf (aux, "%s Actual", aux );
				break;

			case _TABLA_ASIGH: 
				sprintf (aux, "%s Historica", aux );
				break;
		}

		fprintf(sal, "%s\t",            aux);
		fprintf(sal, "%d\t",            (*nodop).emp);

		LeerCliente ((*nodop).cliente);
		fprintf (sal, "%ld\t%s\t",    (*nodop).cliente, SFld(bill|CLIENTE_RAZSOC) );
		fprintf (sal, "%d\t%s\t",     (*nodop).objetivo, GetObjDescrip((*nodop).cliente, (*nodop).objetivo));
		fprintf (sal, "%c\t",         (*nodop).vigil);
		fprintf (sal, "%c\t",         (*nodop).efect);
		fprintf (sal, "%d\t%d\t%s\t", (*nodop).ptoser, (*nodop).puesto, GetDescPto((*nodop).ptoser));
		fprintf (sal, "%s\t",         (*nodop).regim);
		fprintf (sal, "%.3D\t",       (*nodop).fecasig);

		if ((*nodop).efect=='P' && (*nodop).tipo == _MOVIM_BAJA)
			fprintf (sal, "%.3D\t",   (*nodop).fechas);
		else
			fprintf (sal, "%.3D\t",   (*nodop).fecbaj);
		fprintf (sal, "%.3D\t",       (*nodop).ffranco);
		fprintf (sal, "%d\t",         (*nodop).numfran);
		fprintf (sal, "%c\t",       (*nodop).dia1);
		fprintf (sal, "%c\t",       (*nodop).dia2);
		fprintf (sal, "%c\t",       (*nodop).dia3);
		fprintf (sal, "%c\t",       (*nodop).dia4);
		fprintf (sal, "%c\t",       (*nodop).dia5);
		fprintf (sal, "%c\t",       (*nodop).dia6);
		fprintf (sal, "%c\t",       (*nodop).dia7);
		fprintf (sal, "%.3T\t",     (*nodop).horent);
		fprintf (sal, "%.3T\t",     (*nodop).horsal);
		fprintf (sal, "%d\n",       (*nodop).motivo);

	}

	if (grabar) {
		switch((*nodop).tabla) {
			case _TABLA_ASIG: 

				SetKey (ope|ASIGbyEMP, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, (*nodop).ptoser, (*nodop).puesto, (*nodop).nroint);
				sprintf (aux, "%c",       (*nodop).vigil);
				SetFld (ope|ASIG_VIGIL,   aux);
				sprintf  (aux, "%c",      (*nodop).efect);
				SetFld (ope|ASIG_EFECT,   aux);
				SetDFld(ope|ASIG_FECASIG, (*nodop).fecasig);
				SetTFld(ope|ASIG_HSENT,   (*nodop).horent);
				SetTFld(ope|ASIG_HSSAL,   (*nodop).horsal);
				sprintf (aux, "%c",       (*nodop).dia1);
				SetFld (ope|ASIG_DIA1,    aux);
				sprintf (aux, "%c",       (*nodop).dia2);
				SetFld (ope|ASIG_DIA2,    aux);
				sprintf (aux, "%c",       (*nodop).dia3);
				SetFld (ope|ASIG_DIA3,    aux);
				sprintf (aux, "%c",       (*nodop).dia4);
				SetFld (ope|ASIG_DIA4,    aux);
				sprintf (aux, "%c",       (*nodop).dia5);
				SetFld (ope|ASIG_DIA5,    aux);
				sprintf (aux, "%c",       (*nodop).dia6);
				SetFld (ope|ASIG_DIA6,    aux);
				sprintf (aux, "%c",       (*nodop).dia7);
				SetFld (ope|ASIG_DIA7,    aux);
				SetLFld(ope|ASIG_REEMPL,  (*nodop).reepl);
				SetDFld(ope|ASIG_FFRANCO, (*nodop).ffranco);
				SetFld (ope|ASIG_REGIM,   (*nodop).regim);
				SetDFld(ope|ASIG_FECHAS,  (*nodop).fecbaj);

				SetDFld(ope|ASIG_FECBAJ,  (*nodop).fecbaj);
				SetIFld(ope|ASIG_NUMFRAN, (*nodop).numfran);
				SetIFld(ope|ASIG_FRANCERO,(*nodop).francero);
				sprintf (aux, "%c",       (*nodop).tipodia);
				SetFld (ope|ASIG_TIPODIA, aux);
                SetIFld(ope|ASIG_CODROL,  (*nodop).codrol);                                              
                SetIFld(ope|ASIG_FILA,    (*nodop).fila);
                SetIFld(ope|ASIG_COLUM,   (*nodop).colum);
                SetFld (ope|ASIG_REGPTO,  (*nodop).regpto);

				// MOVIMIENTO
				switch((*nodop).tipo){
					case _MOVIM_ALTA: 
						PutRecord(ope|ASIG);
						break;

					case _MOVIM_BAJA: 
						DelRecord(ope|ASIG);
						BorrarTablas(nodop);
						break;
				}	
        		break;

			case _TABLA_ASIGH: 
				SetKey (ope|ASIGHbyEMP,(*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).ptoser, (*nodop).puesto,
				                       (*nodop).nroint, (*nodop).nroleg, (*nodop).fecbaj, (*nodop).fecasig);

				sprintf (aux, "%c",        (*nodop).vigil);
				SetFld (ope|ASIGH_VIGIL,   aux);
				sprintf  (aux, "%c",       (*nodop).efect);
				SetFld (ope|ASIGH_EFECT,   aux);
				SetTFld(ope|ASIGH_HSENT,   (*nodop).horent);
				SetTFld(ope|ASIGH_HSSAL,   (*nodop).horsal);
				sprintf (aux, "%c",        (*nodop).dia1);
				SetFld (ope|ASIGH_DIA1,    aux);
				sprintf (aux, "%c",        (*nodop).dia2);
				SetFld (ope|ASIGH_DIA2,    aux);
				sprintf (aux, "%c",        (*nodop).dia3);
				SetFld (ope|ASIGH_DIA3,    aux);
				sprintf (aux, "%c",        (*nodop).dia4);
				SetFld (ope|ASIGH_DIA4,    aux);
				sprintf (aux, "%c",        (*nodop).dia5);
				SetFld (ope|ASIGH_DIA5,    aux);
				sprintf (aux, "%c",        (*nodop).dia6);
				SetFld (ope|ASIGH_DIA6,    aux);
				sprintf (aux, "%c",        (*nodop).dia7);
				SetFld (ope|ASIGH_DIA7,    aux);
				SetIFld(ope|ASIGH_MOTIVO,  (*nodop).motivo);
				SetLFld(ope|ASIGH_REEMPL,  (*nodop).reepl);
				SetFld (ope|ASIGH_REGIM,   (*nodop).regim);
				SetDFld(ope|ASIGH_FECHAS,  (*nodop).fecbaj);
				
				SetDFld(ope|ASIGH_FFRANCO, (*nodop).ffranco);
				SetIFld(ope|ASIGH_NUMFRAN, (*nodop).numfran);
				SetIFld(ope|ASIGH_FRANCERO,(*nodop).francero);
				sprintf (aux, "%c",        (*nodop).tipodia);
				SetFld (ope|ASIGH_TIPODIA, aux);
                SetIFld(ope|ASIGH_CODROL,  (*nodop).codrol);
                SetIFld(ope|ASIGH_FILA,    (*nodop).fila);
                SetIFld(ope|ASIGH_COLUM,   (*nodop).colum);
                SetFld (ope|ASIGH_REGPTO,  (*nodop).regpto);

				// MOVIMIENTO
				switch((*nodop).tipo){
					case _MOVIM_ALTA: 
						PutRecord(ope|ASIGH);
						break;

					case _MOVIM_BAJA: 
						DelRecord(ope|ASIGH);

						BorrarTablas(nodop);
						break;

				}	
        		break;

		}
	}

	if ((*nodop).nsig != NULL)
		LisNMovimien((*nodop).nsig);
}

static void BorNMovimien(tnmovimien nodop)
{
	if (nodop == NULL)
		return;

	if ((*nodop).nsig != NULL)
		BorNMovimien((*nodop).nsig);

	(*nodop).nsig = NULL;
	free(nodop);
}


void ProximoFranco (short p_emp, long p_nroleg, char * p_vigil, DATE p_afranco, short p_numfran, DATE p_fasig, DATE p_fecact, DATE *p_ffranco, int *p_nfran)
{
	/*Esta funcion devuelve la primer fecha de franco apartir de hoy
	  y el numero de franco que es */

	DATE franco = p_fecact,
		 faux   = p_fecact -13; //Esto esta por si ya viene de franco
	short num=0;
	bool esfranco=FALSE;

	if (p_afranco == NULL_DATE) {
		*p_ffranco = NULL_DATE;
		*p_nfran = NULL_SHORT;
		return ;
	}

	if (p_afranco == franco) {
		*p_ffranco = franco;
		*p_nfran = p_numfran;
		return ;
	}

	while (!(esfranco=Franco(p_emp, p_nroleg, faux, p_vigil, p_numfran)) || faux < franco) {
		if (esfranco)
			num ++;
		else
			num = 1;
		faux ++;
	}
	*p_ffranco = faux;

	if (p_numfran == NULL_SHORT)
		*p_nfran = NULL_SHORT; //Solo se muestra el numero si tiene mas de un dia de franco
	else
		*p_nfran = num;
}

static bool ValidaPuestoDestino(long p_cliente, int p_objet, int  p_codintd) 
{
	bool 		v_valida=FALSE;
	dbcursor 	c_puestos;

	c_puestos	= CreateCursor(ope|PUESTOSbyCLIENTE,  IO_NOT_LOCK);
	SetCursorFrom(c_puestos, p_cliente, p_objet, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (c_puestos, p_cliente, p_objet, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(c_puestos) != ERROR) {
		if (IFld(ope|PUESTOS_CODINT)==p_codintd) {
			v_valida=TRUE;
			break;
		}
	}
	DeleteCursor(c_puestos);
	return v_valida;
} 

static void AbrirReporte()
{
	reporte=FALSE;
	archivo=FALSE;
	if (FmIFld(fm0, MODPRO)==_MODPRO_REPORT || FmIFld(fm0, MODPRO)==_MODPRO_REPPRO)
		switch(*FmSFld(fm0, SALIDA)) {
			case 'I': 
			case 'T': 
				reporte=TRUE;
				break;
			case 'A': 
				archivo=TRUE;
				break;
		}

	grabar=FALSE;
	if (FmIFld(fm0, MODPRO)==_MODPRO_PROCES || FmIFld(fm0, MODPRO)==_MODPRO_REPPRO)
		grabar=TRUE;

	// Abro Archivo de log
	sprintf(nomarch, "%s/logs/mvaspu.%d-%d-%D-%.T.log", getenv("OPERAC"), GetUid(), getpid(), Today(), Hour());		
	alog = fopen(nomarch, "w"); 

	if (reporte){
		rp0 = OpenReport("mvaspu", RP_EABORT|RP_NOBEGIN);
		//Si la salida es Impresora
		if ( *FmSFld(fm0, SALIDA) == 'I') 
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

		//Si la salida es Terminal
		if ( *FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

		BeginReport(rp0, 1, NULL_STR);

		RpSetIFld(rp0, R_EMP,  FmIFld(fm0, EMPO));
		RpSetDFld(rp0, R_FECMOV, FmDFld(fm0, FECMOV));
		RpSetLFld(rp0, R_CLI,  FmLFld(fm0, CLIENTO));
		RpSetDFld(rp0, R_DCLI, FmDFld(fm0, DCLIEN));
		RpSetIFld(rp0, R_OBJ,  FmLFld(fm0, OBJETIV));
		RpSetDFld(rp0, R_DOBJ, FmDFld(fm0, DOBJET));

	}

	if (archivo)
		sal = fopen(FmSFld(fm0, ARCHIVO), "w"); 

}

static void LeerCliente (long p_cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, p_cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		sprintf(msgaux, "No se encuentra cliente %ld",p_cliente);
		ImprimeLog(msgaux);

	}
}

static void ImprimeLog(char * p_msg)
{
	// Imprimo Archivo de Log
	fprintf (alog, "%s\n", p_msg);

	// Imprimo Reporte
	if (reporte) {

		RpSetFld (rp0, R_LOG, p_msg);
		DoReport(rp0, ZLOG);
		linrp++;

		if (linrp>=SALTOPAG) {
			linrp=0;
			RpEjectPage(rp0);
		}


	}
	if (archivo) 
		fprintf (sal, "%s\n", p_msg);

	// Muestro en Pantalla
	DisplayMsg(FALSE, p_msg);
	WiRefresh();
}

static void BorrarTablas(tnmovimien nodop)
{
	dbcursor c_parte, c_excepcion, c_partime, c_partimeh, c_retro, c_retroexc, c_asist;
	DATE v_fecdes = FmDFld(fm0, FECMOV);
	DATE v_fechas = MAX_DATE;

	if ((*nodop).fecasig>FmDFld(fm0, FECMOV))
		v_fecdes = (*nodop).fecasig;

	if ((*nodop).fecbaj != NULL_DATE)
		v_fechas = (*nodop).fecbaj;

	// Borrar Parte
	c_parte	= CreateCursor(ope|PARTEbyLEG,  IO_NOT_LOCK);
	SetCursorFrom(c_parte, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, v_fecdes);
	SetCursorTo  (c_parte, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, v_fechas);
	while (FetchCursor(c_parte) != ERROR) {
		if ((*nodop).ptoser != IFld(ope|PARTE_PTOSER))
			continue;

		if ((*nodop).puesto != IFld(ope|PARTE_PUESTO))
			continue;

		if ((*nodop).nroint != IFld(ope|PARTE_NROINT))
			continue;

//		sprintf(msgaux, "Borra PARTE emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//		                (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, 
//		                DFld(ope|PARTE_DIA), IFld(ope|PARTE_PTOSER), IFld(ope|PARTE_PUESTO), IFld(ope|PARTE_NROINT));
//	   	ImprimeLog(msgaux);
		DelRecord(ope|PARTE);

	} 
	DeleteCursor(c_parte);

	// Borrar Excepcion
	c_excepcion	= CreateCursor(ope|EXCEPCIONbyLEGAJO,  IO_NOT_LOCK);
	SetCursorFrom(c_excepcion, (*nodop).emp, (*nodop).nroleg, v_fecdes, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_excepcion, (*nodop).emp, (*nodop).nroleg, v_fechas, MAX_LONG,  MAX_SHORT);
	while (FetchCursor(c_excepcion) != ERROR) {

		if ((*nodop).cliente  != IFld(ope|EXCEPCION_CLIENTE))
			continue;

		if ((*nodop).objetivo != IFld(ope|EXCEPCION_OBJETIVO))
			continue;

		if ((*nodop).ptoser != IFld(ope|EXCEPCION_PTOSER))
			continue;

		if ((*nodop).puesto != IFld(ope|EXCEPCION_PUESTO))
			continue;

		if ((*nodop).nroint != IFld(ope|EXCEPCION_NROINT))
			continue;

//		sprintf(msgaux, "Borra EXCEPCION emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//		                (*nodop).emp, IFld(ope|EXCEPCION_CLIENTE), IFld(ope|EXCEPCION_OBJETIVO), (*nodop).nroleg, 
//		                DFld(ope|EXCEPCION_DIA), IFld(ope|EXCEPCION_PTOSER), IFld(ope|EXCEPCION_PUESTO), IFld(ope|EXCEPCION_NROINT));
//	   	ImprimeLog(msgaux);

		DelRecord(ope|EXCEPCION);
		
	} 
	DeleteCursor(c_excepcion);

	if ((*nodop).vigil==PARTTIME[0]) {

		// Borrar Diasptime
		c_partime	= CreateCursor(ope|DIASPTIMEbyDIA,  IO_NOT_LOCK);
		SetCursorFrom(c_partime, (*nodop).emp, (*nodop).nroleg, v_fecdes, NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT, NULL_SHORT);
		SetCursorTo  (c_partime, (*nodop).emp, (*nodop).nroleg, v_fechas, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_partime) != ERROR) {

			if ((*nodop).cliente  != IFld(ope|DIASPTIME_CLIENTE))
				continue;

			if ((*nodop).objetivo != IFld(ope|DIASPTIME_OBJETIVO))
				continue;

			if ((*nodop).ptoser != IFld(ope|DIASPTIME_TIPPTO))
				continue;

			if ((*nodop).puesto != IFld(ope|DIASPTIME_PUESTO))
				continue;

			if ((*nodop).nroint != IFld(ope|DIASPTIME_NROINT))
				continue;

//			sprintf(msgaux, "Borra DIASPTIME emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//			                (*nodop).emp, IFld(ope|DIASPTIME_CLIENTE), IFld(ope|DIASPTIME_OBJETIVO), (*nodop).nroleg, 
//			                DFld(ope|DIASPTIME_DIA), IFld(ope|DIASPTIME_TIPPTO), IFld(ope|DIASPTIME_PUESTO), IFld(ope|DIASPTIME_NROINT));
//		   	ImprimeLog(msgaux);

			DelRecord(ope|DIASPTIME);
		} 
		DeleteCursor(c_partime);

		// Borrar Diasptimeh
		// primary key(emp, cliente, objetivo, nroleg, tippto, puesto, nroint, dia),
		c_partimeh	= CreateCursor(ope|DIASPTIMEHbyEMP,  IO_NOT_LOCK);
		SetCursorFrom(c_partimeh, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, (*nodop).ptoser, (*nodop).puesto, (*nodop).nroint, v_fecdes);
		SetCursorTo  (c_partimeh, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, (*nodop).ptoser, (*nodop).puesto, (*nodop).nroint, v_fechas);
		while (FetchCursor(c_partimeh) != ERROR) {

//			sprintf(msgaux, "Borra DIASPTIMEH emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//			                (*nodop).emp, IFld(ope|DIASPTIMEH_CLIENTE), IFld(ope|DIASPTIMEH_OBJETIVO), (*nodop).nroleg, 
//			                DFld(ope|DIASPTIMEH_DIA), IFld(ope|DIASPTIMEH_TIPPTO), IFld(ope|DIASPTIMEH_PUESTO), IFld(ope|DIASPTIMEH_NROINT));
//		   	ImprimeLog(msgaux);

			DelRecord(ope|DIASPTIMEH);
		} 
		DeleteCursor(c_partimeh);
		
	}

	// Borrar Retro
	c_retro	= CreateCursor(ope|RETRObyRLEG,  IO_NOT_LOCK);
	SetCursorFrom(c_retro, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, v_fecdes);
	SetCursorTo  (c_retro, (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, v_fechas);
	while (FetchCursor(c_retro) != ERROR) {
		if ((*nodop).ptoser != IFld(ope|RETRO_PTOSER))
			continue;

		if ((*nodop).puesto != IFld(ope|RETRO_PUESTO))
			continue;

		if ((*nodop).nroint != IFld(ope|RETRO_NROINT))
			continue;

//		sprintf(msgaux, "Borra RETRO emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//		                (*nodop).emp, (*nodop).cliente, (*nodop).objetivo, (*nodop).nroleg, 
//		                DFld(ope|RETRO_DIA), IFld(ope|RETRO_PTOSER), IFld(ope|RETRO_PUESTO), IFld(ope|RETRO_NROINT));
//	   	ImprimeLog(msgaux);
		DelRecord(ope|RETRO);

	} 
	DeleteCursor(c_retro);

	// Borrar Retroexc
	// legajo (emp, nroleg, dia, cliente, objetivo);
	c_retroexc	= CreateCursor(ope|RETROEXCbyLEGAJO,  IO_NOT_LOCK);
	SetCursorFrom(c_retroexc, (*nodop).emp, (*nodop).nroleg, v_fecdes, NULL_LONG, NULL_SHORT);
	SetCursorTo  (c_retroexc, (*nodop).emp, (*nodop).nroleg, v_fechas, MAX_LONG, MAX_SHORT);
	while (FetchCursor(c_retroexc) != ERROR) {

		if ((*nodop).cliente  != IFld(ope|RETROEXC_CLIENTE))
			continue;

		if ((*nodop).objetivo != IFld(ope|RETROEXC_OBJETIVO))
			continue;

		if ((*nodop).ptoser != IFld(ope|RETROEXC_PTOSER))
			continue;

		if ((*nodop).puesto != IFld(ope|RETROEXC_PUESTO))
			continue;

		if ((*nodop).nroint != IFld(ope|RETROEXC_NROINT))
			continue;

//		sprintf(msgaux, "Borra RETROEXC emp= %d Cliente= %ld Objetivo= %d Nroleg=%ld Dia= %.3D puesto %d-%d-%d", 
//		                (*nodop).emp, IFld(ope|RETROEXC_CLIENTE), IFld(ope|RETROEXC_OBJETIVO), (*nodop).nroleg, 
//		                DFld(ope|RETROEXC_DIA), IFld(ope|RETROEXC_PTOSER), IFld(ope|RETROEXC_PUESTO), IFld(ope|RETROEXC_NROINT));
//	   	ImprimeLog(msgaux);

		DelRecord(ope|RETROEXC);
	} 
	DeleteCursor(c_retroexc);

	// Borrar Asist.Asisten
	c_asist	= CreateCursor(asist|ASISTENbyINDLEG,  IO_NOT_LOCK);
	SetCursorFrom(c_asist, (*nodop).emp, (*nodop).nroleg, v_fecdes, NULL_SHORT);
	SetCursorTo  (c_asist, (*nodop).emp, (*nodop).nroleg, v_fechas, MAX_SHORT);
	while (FetchCursor(c_asist) != ERROR) {

//		sprintf(msgaux, "Borra ASIST emp= %d  Nroleg=%ld Dia= %.3D ", (*nodop).emp, (*nodop).nroleg, DFld(asist|ASISTEN_FECHA));
//	   	ImprimeLog(msgaux);

		DelRecord(asist|ASISTEN);
	} 
	DeleteCursor(c_asist);

}

static DATE FechaPrimerFrancoBaja(DATE p_fecbase, int numdia)
{
	DATE v_fecha;

	for (v_fecha=p_fecbase; DiaLetra(v_fecha)[0]!='D'&& v_fecha <(v_fecha + 10); v_fecha ++);

	return v_fecha;
}


/*Funcion copiada de genparte */
static void AsigHora(int p_emp, long p_cliente, int p_objet, long p_nroleg, int p_ptoser, int p_puesto, int p_nroint,
					 DATE dia, TIME * hsentrada, TIME * hssalida, char *p_regim)
{
	/********************************************************************************************
	Esta funcion devuelve la hora de entrada y salida 
	Solo se puede usar para un franquero (que rota el horario a cubrir)
	Solo se puede usar para un regimen 4x2x12 (donde rota el horario cada 2 dias -cantdiasP-) o
	para un regimen  8x4x12 (donde rota el horario cada 4 dias -cantdiasP-).
	********************************************************************************************/

	short diaslab, dias;
	DATE fecha  = NULL_DATE,
		 fechaA = NULL_DATE;
	bool impre = FALSE, hay_otros = FALSE;
	short cantdias=0, cantdiasP=0, diasarecorrer = 0;
	TIME hantent, hantsal, v_hent, v_hsal;
	static dbcursor c_Asig=ERROR, c_Asigh=ERROR;
	
	v_hent= *hsentrada;
	v_hsal= *hssalida;

	if (c_Asig==ERROR)
		c_Asig = CreateCursor(ope|ASIGbyPUESTO, IO_NOT_LOCK);

	if (c_Asigh==ERROR)
		c_Asigh = CreateCursor(ope|ASIGHbyEMP, IO_NOT_LOCK);


	if (strcmp(p_regim, REG_ESP_3)==0)
		cantdiasP = (int)GetDiasLaboral(p_regim, FALSE)/2;

	else 
		cantdiasP = GetDiasFranco(p_regim, FALSE);

	diasarecorrer =  GetDiasLaboral(p_regim, FALSE) + GetDiasFranco(p_regim, FALSE);

	*hsentrada = StrToT("0000");
	*hssalida  = StrToT("0000");

	/*Leo la asignacion */
	SetKey (ope|ASIGbyEMP, p_emp, p_cliente, p_objet, p_nroleg, p_ptoser, p_puesto, p_nroint);
	if (GetRecord (ope|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR || DFld (ope|ASIG_FECASIG) > dia ) {
		bool encontro=FALSE;
		
		SetKey (ope|ASIGHbyEMP, p_emp, p_cliente, p_objet, p_ptoser, p_puesto, p_nroint, p_nroleg, MIN_DATE, MIN_DATE);
		while (!encontro && GetRecord (ope|ASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {
			if (DFld (ope|ASIGH_FECALT) <= dia && DFld (ope|ASIGH_FECBAJ) >= dia) {
				encontro = TRUE;
			} 
		}

		if (!encontro) {
			if (*hssalida == StrToT("000000") || *hsentrada == StrToT("000000")){
				*hsentrada = v_hent;
				*hssalida  = v_hsal;
			}
			return;
		} 
		
		CopyFld (ope|ASIGH_NROLEG,  ope|ASIG_NROLEG);
		CopyFld (ope|ASIGH_VIGIL ,  ope|ASIG_VIGIL);
		CopyFld (ope|ASIGH_NUMFRAN, ope|ASIG_NUMFRAN);
		CopyFld (ope|ASIGH_FECALT,  ope|ASIG_FECASIG);
		CopyFld (ope|ASIGH_HSENT,   ope|ASIG_HSENT);
		CopyFld (ope|ASIGH_HSSAL,   ope|ASIG_HSSAL);
		CopyFld (ope|ASIGH_FFRANCO, ope|ASIG_FFRANCO);
		CopyFld (ope|ASIGH_REGIM,   ope|ASIG_REGIM);
	} 

	/*Si tiene franco o vacaciones no calculo nada */
	if (Franco (p_emp, LFld (ope|ASIG_NROLEG), dia, SFld(ope|ASIG_VIGIL), IFld(ope|ASIG_NUMFRAN))) {
		if (*hssalida == StrToT("000000") || *hsentrada == StrToT("000000")){
			*hsentrada = v_hent;
			*hssalida  = v_hsal;
		}
		return;
	} 

	if (Vacaciones(p_emp, p_nroleg, fecha)) {
		if (*hssalida == StrToT("000000") || *hsentrada == StrToT("000000")){
			*hsentrada = v_hent;
			*hssalida  = v_hsal;
		}
		return;
	}

	/*Si pido la fecha de asignacion y NO es franco  la hora es la ingresada en la asignacion */
	if (dia == DFld (ope|ASIG_FECASIG) && 
		!Franco (p_emp, LFld (ope|ASIG_NROLEG), dia, SFld(ope|ASIG_VIGIL), IFld(ope|ASIG_NUMFRAN))) {
			*hsentrada = TFld (ope|ASIG_HSENT);
			*hssalida  = TFld (ope|ASIG_HSSAL);
			if (impre) fprintf (stderr, "FIN Es primer dia devuelve %T %T \n", *hsentrada, *hssalida);
	}

	if (dia != DFld (ope|ASIG_FECASIG)) {

		/********************************************************************************************
		Calculo el dia posterior al de la asignacion a que hora tiene que trabajar
		Para esto reconstruyo apartir de lo que seria su franco anterior (Fecha de franco - dias laborables)
		En base a esa fecha puedo saber si el dia posterior al de asignacion tiene que seguir cumpliendo
		el mismo horario o si tiene que cambiar.
		*********************************************************************************************/
		diaslab = GetDiasLaboral(SFld (ope|ASIG_REGIM), FALSE);
		diaslab = diaslab - 1 + IFld (ope|ASIG_NUMFRAN);
		for (fechaA= DFld (ope|ASIG_FFRANCO), dias=0; dias < diaslab; fechaA --, dias ++); 
		if (impre) fprintf (stderr, "El primer dia laborable es %.3D \n", fechaA);

		for (fecha=fechaA; fecha <= (DFld (ope|ASIG_FECASIG)+1); fecha ++) {
			if (impre) fprintf (stderr, "for fecha %.3D asig %.3D \n", fecha, DFld (ope|ASIG_FECASIG)+1);
			if (cantdias < cantdiasP) {
				cantdias ++;
				if (impre) fprintf (stderr, "dentro del if fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (ope|ASIG_FECASIG)+1, cantdias, cantdiasP);
			} 
			else {
				cantdias = 1;
				if (impre) fprintf (stderr, "dentro del else fecha %.3D asig %.3D cantdias %d cantdiasP %d \n", fecha, DFld (ope|ASIG_FECASIG)+1, cantdias, cantdiasP);
			}
		}

		if (Franco (p_emp, LFld (ope|ASIG_NROLEG), DFld (ope|ASIG_FECASIG) , SFld(ope|ASIG_VIGIL), IFld(ope|ASIG_NUMFRAN))) {
			/*Si el dia de asignacion esta de franco la hora anterior es la inversa de la primer asignacion */
			//Inversa
			hantent= TFld (ope|ASIG_HSSAL);
			hantsal= TFld (ope|ASIG_HSENT);
			if (impre) fprintf (stderr, "Franco hantent %T hantsal %T \n", hantent,hantsal);
		}
		else {
			hantent= TFld (ope|ASIG_HSENT);
			hantsal= TFld (ope|ASIG_HSSAL);
			if (impre) fprintf (stderr, "No Franco hantent %T hantsal %T \n", hantent,hantsal);
		}

		if (impre) fprintf (stderr, "El dia posterior a la fecha de asig %.3D cantdias %d horant %T %T \n", DFld (ope|ASIG_FECASIG)+1, cantdias, hantent, hantsal);

		if (strcmp(p_regim, REG_ESP_3)==0)
			cantdias = 2;

		/***************************************************************
		Busco para la fecha pedida a que hora trabaja
		Rota el horario con respecto a la hora anterior cada dos dias (cantdiasP)
		***************************************************************/

		for (fecha=DFld (ope|ASIG_FECASIG)+1 ; fecha <= dia; fecha ++) {
			short periodo;
		
			if (impre) fprintf (stderr, "principio de for Dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T\n", fecha, cantdias, cantdiasP, *hsentrada, *hssalida,hantent,hantsal);
			/*********************************************************
			Lo que sigue se agrego para agilizar la funcion.
			Porque si el vigilador se asigno hace mucho recorría dia por dia.
			Como la secuencia de rotacion es la misma cada 6 dias puedo avanzar hasta un multiplo.
			De esta forma recorro a lo sumo solo 6 dias.
			**********************************************************/
			/*********************************************************
			Al agregarse el regimen 8x4x12 los dias a recorrer pueden ser 12 por ende puse la 
			variable diasarecorrer. GAG
			**********************************************************/
			periodo = (dia-fecha) / diasarecorrer;
			
			if (periodo > 0) {
				if (impre) fprintf (stderr, "Dia %.3D Fecha %.3D periodo %d ", dia, fecha, periodo);
				fecha = fecha + (periodo * diasarecorrer);
				if (impre) fprintf (stderr, "Nueva fecha %.3D  \n", fecha);
			}
			
			if (Franco (p_emp, LFld (ope|ASIG_NROLEG), fecha, SFld(ope|ASIG_VIGIL), IFld(ope|ASIG_NUMFRAN))) {
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				cantdias=1;
				if (impre) fprintf (stderr, "Dia %.3D Franco \n", fecha);
				continue;
			} 	

			if (impre) fprintf(stderr, "fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
			if (cantdias < cantdiasP) {
				*hsentrada = hantsal;
				*hssalida  = hantent;
				if (impre) fprintf(stderr,"dentro del if fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d \n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias ++;
			}
			else {
				*hsentrada = hantent;
				*hssalida  = hantsal;
				if (impre) fprintf(stderr,"dentro del else fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
				cantdias = 1;					
			}
			if (cantdias == cantdiasP) {
				hantent= *hsentrada;
				hantsal= *hssalida;
			}
			if (impre) fprintf(stderr,"al final fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);

			if (Vacaciones(p_emp, p_nroleg, fecha)) {
				/*No se modifica cantidad de dias la rotacion es como si estuviera trabajando */
				*hsentrada = StrToT("0000");
				*hssalida  = StrToT("0000");
				if (impre) fprintf (stderr, "Dia %.3D Vacaciones \n", fecha);
			}
		} 
		if (impre) fprintf(stderr,"fin del for fecha %.3D dia %.3D hsentrada %T hssalida %T hantent %T hantsal %T cantdias %d\n",fecha, dia,*hsentrada, *hssalida,hantent,hantsal,cantdias);
	}

	if (*hsentrada == StrToT ("0000") && *hssalida == StrToT ("0000")){
		*hsentrada = v_hent;
		*hssalida  = v_hsal;
		return;
	}
	if (*hsentrada == StrToT ("2359"))
		*hsentrada = StrToT ("0000");

	/*Controlo que haya alguien a quien cubrir */
	SetCursorFrom(c_Asig, p_emp, p_cliente, p_objet, p_ptoser, p_puesto, MIN_SHORT, MIN_LONG);
	SetCursorTo  (c_Asig, p_emp, p_cliente, p_objet, p_ptoser, p_puesto, MAX_SHORT, MAX_LONG);
	while (!hay_otros && FetchCursor(c_Asig) != ERROR) {
		if (!str_eq(SFld(ope|ASIG_REGIM), REG_ESP) &&
			!str_eq(SFld(ope|ASIG_REGIM), REG_ESP_2)&&
			!str_eq(SFld(ope|ASIG_REGIM), REG_ESP_3)) {
			continue;
		}
		if (Vacaciones(p_emp, p_nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}

		if (Franco(p_emp, LFld(ope|ASIG_NROLEG), dia, SFld(ope|ASIG_VIGIL),	IFld(ope|ASIG_NUMFRAN))) {
			if (*hsentrada == TFld(ope|ASIG_HSENT)) {
				hay_otros = TRUE;
				break;
			}
		}
	}
//	DeleteCursor(c_Asig);

	SetCursorFrom(c_Asigh, p_emp, p_cliente, p_objet, p_ptoser, p_puesto, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
	SetCursorTo  (c_Asigh, p_emp, p_cliente, p_objet, p_ptoser, p_puesto, MAX_SHORT, MAX_LONG, MAX_DATE, MAX_DATE);
	while (!hay_otros && FetchCursor(c_Asigh) != ERROR) {
		if (!str_eq(SFld(ope|ASIGH_REGIM), REG_ESP) && !str_eq(SFld(ope|ASIGH_REGIM), REG_ESP_2) &&
			!str_eq(SFld(ope|ASIGH_REGIM), REG_ESP_3))
			continue;

		if (Vacaciones(p_emp, p_nroleg, dia)) {
			hay_otros  = TRUE;
			continue;
		}
		if (Franco(p_emp, LFld(ope|ASIGH_NROLEG), dia, SFld(ope|ASIGH_VIGIL), IFld(ope|ASIGH_NUMFRAN) ) ) {
			if (*hsentrada == TFld(ope|ASIGH_HSENT)) {
				hay_otros  = TRUE;
			}
		}
	}            
//	DeleteCursor(c_Asigh);

	if (*hssalida == StrToT("000000") || *hsentrada == StrToT("000000")){
		*hsentrada = v_hent;
		*hssalida  = v_hsal;
	}

}

static fm_status before(form fm, fmfield fno, int row)
{
	static char fecha[11];
	
	
	switch (fno) {
	case CLIENTO: 
	   	InicClientesXusr();

	    break;
    case OBJETIV:
	   	InicObjetivosXusr(FmLFld(fm, CLIENTO), FmIFld(fm, EMPO));

	    break;
   	case FECMOV:
    	sprintf(fecha, "01/%d/%d", Month(Today()), Year(Today()));
    	FmSetDFld(fm, fno, StrToD(fecha));
	    break;
    }
	
	return FM_OK;
}
static fm_status after(form fm, fmfield fno, int row)
{
	bool encontro_asig=FALSE;
	switch (fno) {
		case EMPO:
		    if (FmChgFld(fm))
    	    	InicListaXusr(FmIFld(fm0, EMPO));
	        break;
		case CLIENTO: 
			if (FmKeyCode(fm) == K_HELP)
  				HelpCliente(fm, fno, row);

			if (!FmIsNull(fm, fno)){
				if (ValidaClienteXusr(FmLFld(fm, fno)))
				  	FmSetFld(fm, DCLIEN, GetDescCliente(FmLFld(fm, CLIENTO)), row);
				else {

	 				Warning("No tiene permisos sobre el cliente %ld", FmLFld(fm, CLIENTO));
					FmSetLFld(fm, CLIENTO, NULL_LONG, row);
					FmSetFld(fm, DCLIEN, NULL_STR, row);
					return FM_REDO;
				}	
			}
	    	break;
	    case OBJETIV:
			if (FmKeyCode(fm) == K_HELP)
  				HelpObjet(fm, fno, row, FmLFld(fm, CLIENTO));

			if (!FmIsNull(fm, fno)){
				if (ValidaObjetivoXusr(FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV), FmIFld(fm, EMPO)))
					FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV)));
				else	{
					Warning("No tiene permisos sobre el Cliente %ld Objetivo %d", FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV));
					FmSetIFld(fm, OBJETIV, NULL_SHORT, row);
					FmSetFld(fm, DOBJET, NULL_STR, row);
					return FM_REDO;
		   		}
			}
			else{
				Warning("El Objetivo No puede ser nulo");
				return FM_REDO;
			}
	    	break;
		case CODINTO: 
			if (FmKeyCode(fm) == K_HELP)
  				HelpCodInt(fm, fno, row, FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV), FmDFld(fm, FECMOV));
	  		else
				FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV)));

			encontro_asig=FALSE;
			// Controlar que SI tenga asignaciones
			SetKey (ope|ASIGbyEMP, FmIFld(fm0, EMPO), FmLFld(fm0, CLIENTO), FmIFld(fm0, OBJETIV), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			while(GetRecord(ope|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3)!=ERROR) {
				if (IFld(ope|ASIG_PUESTO)==FmIFld(fm, fno)) {
					encontro_asig=TRUE;
					break;
				}
				
			} 
			if (!encontro_asig){
				Warning("No hay Asignacionespara el puesto %d", FmIFld(fm, fno));
				return FM_REDO;
			}
	    	break;


		case CODINTD: 
			if (FmKeyCode(fm) == K_HELP)
  				HelpCodInt(fm, fno, row, FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV), FmDFld(fm, FECMOV));
	  		else
				FmSetFld(fm, DOBJET, GetObjDescrip(FmLFld(fm, CLIENTO), FmIFld(fm, OBJETIV)));

			// Valida si existe el puesto destino
			if (!ValidaPuestoDestino(FmLFld(fm0, CLIENTO), FmIFld(fm0, OBJETIV), FmIFld(fm0, CODINTD))) {
				Warning("El Puesto %d No Existe", FmIFld(fm0, CODINTD));
				return FM_REDO;
			}

			// Controlar que NO tenga asignaciones
			SetKey (ope|ASIGbyEMP, FmIFld(fm0, EMPO), FmLFld(fm0, CLIENTO), FmIFld(fm0, OBJETIV), NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
			while(GetRecord(ope|ASIGbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3)!=ERROR) {
				if (IFld(ope|ASIG_PUESTO)==FmIFld(fm, fno)) {
					Warning("El Puesto %d Ya Tiene Asignaciones", IFld(ope|ASIG_PUESTO));
					return FM_REDO;
				}
				
			} 
			
			
			
		    break;
	}
	return FM_OK;
}

