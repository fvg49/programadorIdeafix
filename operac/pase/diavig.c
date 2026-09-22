/********************************************************************
*
* MODULE & VERSION : @(#)diavig.c	1.1
* DATE             : 08/12/22
* TIME             : 18:18:22
*
* CREATED          : 05/01/99
*
* DESCRIPTION:
*             Este proceso se encarga de realizar un pase de las variables de horas,
*             centros de costo y lugar de pago a VAREMP de sue de Denarius.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*COMENTARIO 12/08/2008: se agrego que grabe en tabla VAREMP c. costo y lug. pago solo si tabla sue|PARAM
, con modulo 13=MOD_DENPPS y parametro 1=PAR_BLOQEMP, tiene como valor "N"(esto quiere decir que no esta 
implementado el modulo META4, y si tiene el valor "S" significa que si lo esta, por lo tanto se debe grabar 
en una tabla de META4.
COMENTARIO 05/08/2004 :  Se agrego la funcion CambiarLogParte porque acumulaba mal dentro del cursor del 
parte. Al sacarle el PutRecord dentro del FetchCursor anduvo bien. (LDias)
*COMENTARIO 21/09/2006 :  Para controlar lo que graba se le pasa un parametro cualquiera, y no grabara ni 
controla nada, pero saca a la salida  de errores un archivo que dice que deberia haber grabado (fventura)
*********************************************************************/
//#include <ideafix.h>
#include <den.h>
#include "operac.h"
#include "comerc.h"
#include "comgral.h"
#include "sue.sch"
#include "operac.sch"
#include "diavig.fmh"
#include "disths.h"

#define WAR_PERI_PASADO    "El período asociado a esta liquidación ya fue procesado.\nDesea volver a procesar?"
#define ERR_PERI_BORR      "El periodo de referencia del Sist.Novia: %d / %d de la Empresa: %d no existe."
#define ERR_CUR            "No se pudo crear el cursor para borrar la información vieja"
#define ERR_LEG            "No existe el empleado perteneciente a la Empresa: %d con Legajo : %ld"
#define ERR_NO_EXISTE_PERI "No existe cierre en Novia correspondiente al mes-año de la liquidación ingresada."

#define DIA_DEL_VIGILADOR            18 //Cantidad de Horas del dia del vigilador SAPESA(37)

#define _SUMA 1
#define _RESTA -1


// Funciones privadas
static fm_status after(form fm, fmfield fn0, int row);
static fm_status before(form fm, fmfield fno, int row);

static void LeerLegajo(int emp, long nroleg);
static void GuardarDatos(int emp, long nroleg, long nroliq, double hdiavig);
static void AlmacenarHoras(long nroleg, long nroliq, int vemp, long ccosto, double valor, bool suma);

bool AcumulaCanH(DATE p_dia, TIME p_hent, TIME p_hsal, int p_accion, double * p_canth);


// Variables globales.
form	fm0;
schema	sue, ope;
bool	liqsimul = FALSE, 
        probando=FALSE;
int		i;

FILE *salida = NULL;

wcmd(diavig, 1.1 12/22/08)
{
	long nroliq;
	int  emp;
	double canh=0;
	char v_aux[10];
    dbcursor cparte, cretro;
	DATE v_fecha;
	bool v_tiene_retro=FALSE;

	char nomarch[100];

	sue = OpenSchema("sue",     IO_NOT_LOCK);
	ope = OpenSchema("operac",  IO_NOT_LOCK);
	fm0 = OpenForm  ("diavig", FM_EABORT);


	if (DoForm(fm0, before, after) != FM_UPDATE) return;

	emp     = FmIFld(fm0, EMP);
	nroliq  = FmLFld(fm0, NROLIQ);


	if(*FmSFld(fm0, SALIDA)=='A') {
		sprintf(nomarch, "%s", FmSFld(fm0, NOMARCH));
		if ((salida = fopen(nomarch, "w")) == NULL)
			Error("No se pudo crear el archivo %s", nomarch);

		fprintf(salida, "Legajo\tNombre y Apellido\tHoras\tTiene Retro?\n\n");
	}
	v_fecha=FmDFld(fm0, FECHA);

	cparte = CreateCursor(ope|PARTEbyEMPLE, IO_NOT_LOCK);
	cretro = CreateCursor(ope|RETRObyREMPLE, IO_NOT_LOCK);

	SetKey(sue|PERbyEMP, emp, NULL_LONG);
	while (GetRecord(sue|PERbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {

		FmSetLFld(fm0, COMENTARIO, LFld(sue|PER_NROLEG) );
		sprintf(v_aux, "%s", SFld(sue|PER_APYNOM));
		FmSetFld(fm0, DCOMENTARIO, v_aux );
		WiRefresh();

		canh=0;
		v_tiene_retro=FALSE;
		SetCursorFrom(cparte, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), v_fecha - 1, NULL_LONG, NULL_SHORT);
		SetCursorTo  (cparte, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), v_fecha    , MAX_LONG, MAX_SHORT);
		while (FetchCursor(cparte) != ERROR) {

			if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(ope|PARTE_CLIENTE), IFld(ope|PARTE_OBJETIVO)))
				continue;

			AcumulaCanH(DFld(ope|PARTE_DIA) , TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL), _SUMA, &canh); 

//			fprintf (stderr, "PARTE %ld %.3D %.3T-%.3T %.2f\n", LFld(ope|PARTE_NROLEG), DFld(ope|PARTE_DIA) , TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL), canh);
		} 

		SetCursorFrom(cretro, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), v_fecha - 1, NULL_LONG, NULL_SHORT);
		SetCursorTo  (cretro, IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), v_fecha    , MAX_LONG, MAX_SHORT);
		while (FetchCursor(cretro) != ERROR) {

			if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO)))
				continue;

			if (AcumulaCanH(DFld(ope|RETRO_DIA) , TFld(ope|RETRO_HORAENT), TFld(ope|RETRO_HORASAL), _SUMA, &canh)) {
				v_tiene_retro=TRUE;

				SetKey (ope|PARTEbyEMP, IFld(ope|RETRO_EMP), LFld(ope|RETRO_CLIENTE), IFld(ope|RETRO_OBJETIVO), DFld(ope|RETRO_DIA), LFld(ope|RETRO_NROLEG), 
				                        IFld(ope|RETRO_PTOSER), IFld(ope|RETRO_PUESTO), IFld(ope|RETRO_NROINT));
				if (GetRecord(ope|PARTEbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
					AcumulaCanH(DFld(ope|PARTE_DIA) , TFld(ope|PARTE_HORAENT), TFld(ope|PARTE_HORASAL), _RESTA, &canh); 
				}
				
			} 

//			fprintf (stderr, "RETRO %ld %.3D %.3T-%.3T %.2f\n", LFld(ope|RETRO_NROLEG), DFld(ope|RETRO_DIA) , TFld(ope|RETRO_HORAENT), TFld(ope|RETRO_HORASAL), canh);
		} 

		if (canh==0)
			continue;

		switch(*FmSFld(fm0, SALIDA)) {
			case 'A':
				fprintf(salida, "%ld\t%s\t%.2f\t%B\n", LFld(sue|PER_NROLEG), v_aux, canh, v_tiene_retro);
				break;
			case 'D':
				GuardarDatos(IFld(sue|PER_EMP), LFld(sue|PER_NROLEG), FmLFld(fm0, NROLIQ), canh	*100);
				break;
						
		} 

	} 
	DeleteCursor(cparte);

	fclose(salida);
 	CloseAllSchemas();
}


static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case NROLIQ :
			// si la Liq está confirmada ya no se puede hacer nada!!!:
			if(FmChgFld(fm)) {
				SetKey(sue|LIQbyNROLIQ, FmLFld(fm, fno));
				if(GetRecord(sue|LIQbyNROLIQ, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
					
					if (IFld(sue|LIQ_CONFIRM)) {
						WiMsg("La liquidación se encuentra confirmada!!\nsolo se pueden procesar liquidaciones abiertas!");
						return FM_REDO;
					}
		
				}

				// leo el periodo ingresado:
				SetKey(ope|CIERREbyEMP, FmIFld(fm0, EMP), MAX_SHORT);
				if (GetRecord(ope|CIERREbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) == ERROR) {
					WiMsg("El periodo asociado a esta liquidación no fue creado\nen el Sistema Novia!. Consulte con Operaciones.");
					return FM_REDO;
				}

				SetLFld(sue|LIQ_NROLIQ, FmLFld(fm, fno));
				GetRecord(sue|LIQbyNROLIQ, THIS_KEY, IO_NOT_LOCK);
				
				if (IFld(sue|LIQ_EMP) != FmIFld(fm0, EMP)) {
					Warning ("La liquidacion no corresponde con la empresa");
					return FM_REDO;
				}
				
			}
			break;
	}
	return FM_OK;
}

static fm_status before(form fm, fmfield fno, int row)
{
	int v_ano=NULL_SHORT, 
		v_mes=NULL_SHORT,
		v_dia=NULL_SHORT;
	char v_fecha[10];
	

	switch (fno) {
		case EMP:

            v_ano=Year(Today());
            v_mes=Month(StrToD(DIAVIGI));
			v_dia=Day(StrToD(DIAVIGI));
			sprintf(v_fecha, "%d/%d/%d", v_dia, v_mes, v_ano);
            FmSetDFld(fm0, FECHA, StrToD(v_fecha));
            
			break;	

	}
	return FM_OK;          
	
   
}
bool AcumulaCanH(DATE p_dia, TIME p_hent, TIME p_hsal, int p_accion, double * p_canh) 
{
	bool v_suma=FALSE;

	// Ayer
	if (p_dia==FmDFld(fm0, FECHA) - 1) {
		if (p_hent > p_hsal){
			*p_canh+=(ConvHraInt(StrToT("00:00:00"), p_hsal) * p_accion);
			v_suma=TRUE;
		}
	}
	// Hoy
	else {
		if (p_hent <= p_hsal){
			*p_canh+=(ConvHraInt(p_hent, p_hsal) * p_accion);; 
			v_suma=TRUE;
		}
		else {
			*p_canh+=(ConvHraInt(p_hent, StrToT("23:59:00") ) * p_accion); ; 
			v_suma=TRUE;
		}
	}

	return v_suma;

}

static void GuardarDatos(int emp, long nroleg, long nroliq, double hdiavig)
{
	LeerLegajo(emp, nroleg);
	AlmacenarHoras(nroleg, nroliq, DIA_DEL_VIGILADOR,  LFld(sue|PER_CODCCOS), hdiavig, TRUE);
}

static void LeerLegajo(int emp, long nroleg)
{
	SetKey(sue|PERbyEMP, emp, nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(ERR_LEG, emp, nroleg);
	}
}
static void AlmacenarHoras(long nroleg, long nroliq, int vemp, long ccosto, double valor, bool suma)
{
	double valorant;
	InitRecord(sue|VAREMP);

	valorant=0;
	SetKey(sue|VAREMPbyNROLEG, nroleg, nroliq, vemp, NULL_LONG);
	if (suma) {
		if (GetRecord(sue|VAREMPbyNROLEG, THIS_KEY, IO_NOT_LOCK)!=ERROR)
			valorant=FFld(sue|VAREMP_VALEMP);
	}

	SetFFld(sue|VAREMP_VALEMP, valor + valorant);
	SetIFld(sue|VAREMP_USRID,  GetUid());
	SetDFld(sue|VAREMP_FECHA,  Today());
	SetTFld(sue|VAREMP_HORA,   Hour());

	PutRecord(sue|VAREMP);
	FreeTable(sue|VAREMP);
}
