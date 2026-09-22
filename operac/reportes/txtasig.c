/********************************************************************
* MODULE & VERSION : @(#)txtasig.c	1.23
* DATE             : 04/09/24
* TIME             : 14:37:21
*
* CREATED          :
*
* DESCRIPTION:
*      Asignación de Vigiladores sin requerimientos.
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "bill.h"
#include "billpro.h"
#include "operac.h"
#include "brigada.h"
#include "txtasig.fmh"
#include "sue.sch"
#include "comerc.sch"
#include "billpro.sch"
#include "operac.sch"
#include "bill.sch"
#include "brigada.sch"
#include "filial.h"

#define PER_INEX   "No existe el Vigilador : %ld de la Empresa : %d (%s)\n"
#define ERR_ARCHI  "No se pudo abrir el archivo %s"

// Declaraciones de Funciones
static void LeerVigilador(int p_emp, long p_nroleg);
static void LeerProvXDiv(int p_pais, char *p_codprov);
static void ImprimoLinea();
void AltaPolicia (short , long , char *);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);

// Declaraciones globales
form   fm0;
schema operac, bill, billpro, comerc, sue, brigada;
FILE   *salida;
FILE   *error;
long vigilador = NULL_LONG, cliente = NULL_LONG;
int  objetivo  = NULL_SHORT, emp= NULL_SHORT, cant = 0;
int  ptoser=NULL_SHORT, puesto=NULL_SHORT, v_i=0;
char razsoc[65], objdes[50], empdes[35];
char regim[8], dias[8], efect;
TIME horent=NULL_TIME, horsal=NULL_TIME;

/* Programa principal */
wcmd(txtasig, 1.23 09/24/04)
{
	fm_status cmd;
	dbcursor c_asig;
	bool primero=FALSE;

	fm0    = OpenForm("txtasig",    FM_EABORT);

	comerc = OpenSchema("comerc", IO_EABORT);
	billpro= OpenSchema("billpro",IO_EABORT);
	brigada= OpenSchema("brigada",IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

    while ((cmd=DoForm(fm0, before, after)) != FM_EXIT) {

		if (cmd!=FM_UPDATE)
			continue;

		/* Abro Archivo */
		salida = fopen(FmSFld(fm0, NOMARCH), "w");
		error  = fopen("txtasig.err", "w");

		if (salida==NULL) {
			WiDialog(WD_OK, WD_OK, "Error", ERR_ARCHI, FmSFld(fm0, NOMARCH));
			continue;
		}
		if (error==NULL) {
			WiDialog(WD_OK, WD_OK, "Error", ERR_ARCHI, "txtasig.err");
			continue;
		}

		/* Imprimo Titulo */
		fprintf(salida, "Empresa\tDescripcion\t");
		fprintf(salida, "Cliente\tRazón Social\t");
		fprintf(salida, "Provincia de Cliente\tDescripcion\t");
		fprintf(salida, "Objetivo\tDescripcion de Objetivo\t");
		fprintf(salida, "Direccion de Objetivo\t");
		fprintf(salida, "Localidad de Objetivo\tDescripcion\t");
		fprintf(salida, "Codigo Postal del Objetivo\t");
		fprintf(salida, "Provincia de Objetivo\tDescripcion\t");
		fprintf(salida, "Delegacion Geografica\tDescripcion\t");
		fprintf(salida, "Vigilador\tApellido y Nombre\t");
		fprintf(salida, "Tipo de Documento\tNúmero de Documento\t");
		fprintf(salida, "C.U.I.L.\t");
		fprintf(salida, "Dirección de Vigilador\tCódigo Postal de Vigilador\t");
		fprintf(salida, "Localidad de Vigilador\tDescripcion\t");
		fprintf(salida, "Provincia de Vigilador\tDescripcion\t");
		fprintf(salida, "Fecha de Ingreso\t");
		fprintf(salida, "Categoria Salarial\tDescripcion\t");
		fprintf(salida, "Alta Policial\t");
		fprintf(salida, "Portación\tFecha\t");
		fprintf(salida, "C.L.U.\tFecha\t");
		fprintf(salida, "Puesto\t\t");
		fprintf(salida, "Regimen\t");
		fprintf(salida, "Día 1\tDía 2\tDía 3\tDía 4\tDía 5\tDía 6\tDía 7\t");
		fprintf(salida, "Horario Entrada\t");
		fprintf(salida, "Horario Salida\t");
		fprintf(salida, "Efectivo o Provisorio\n");

		vigilador = cliente = NULL_LONG;
		objetivo = emp = NULL_SHORT;
		cant = 0;

		sprintf(razsoc, "%s", NULL_STR);
		sprintf(objdes, "%s", NULL_STR);
		sprintf(empdes, "%s", NULL_STR);

		/* Recorro Asignaciones */
		c_asig = CreateCursor(operac|ASIGbyEMP, IO_NOT_LOCK);
		SetCursorFrom(c_asig, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIFld(fm0, OBJDESDE),
		                      MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_asig, FmIFld(fm0, EMP), FmIsNull(fm0, CLIHASTA) ? MAX_LONG :  FmLFld(fm0, CLIHASTA),
		                      FmIsNull(fm0, OBJHASTA) ? MAX_SHORT : FmIFld(fm0, OBJHASTA), 
		                      MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_asig) != ERROR) {
			//valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
		       	continue;
		
			if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
			
			if (Today() < DFld(operac|ASIG_FECASIG))
				continue;
			/* Filtro por Nro de Legajo */
			if (!FmIsNull(fm0, VIGDESDE) && LFld(operac|ASIG_NROLEG)<FmLFld(fm0, VIGDESDE))
				continue;
			if (!FmIsNull(fm0, VIGHASTA) && LFld(operac|ASIG_NROLEG)>FmLFld(fm0, VIGHASTA))
				continue;

			/* Si Cambia la Empresa Leo la Tabla*/
			if (emp != IFld(operac|ASIG_EMP))
				sprintf(empdes,"%s",GetDescEmpresa(IFld(operac|ASIG_EMP)));
			emp = IFld(operac|ASIG_EMP);

			/* Si Cambia el Cliente Leo la Tabla*/
			if (cliente != LFld(operac|ASIG_CLIENTE)){
				sprintf(razsoc, "%s", GetCliRazsoc(LFld(operac|ASIG_CLIENTE)));
				LeerProvXDiv(IFld(bill|CLIENTE_PAIS), SFld(bill|CLIENTE_PROVP));
			}

			/* Si Cambia el Objetivo o Cliente Leo la Tabla*/
			if (objetivo != IFld(operac|ASIG_OBJETIVO) || cliente != LFld(operac|ASIG_CLIENTE)) 
				sprintf(objdes, "%s", GetObjDescrip(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)));

			cliente = LFld(operac|ASIG_CLIENTE);
			objetivo  = IFld(operac|ASIG_OBJETIVO);

			/* Si Cambia el Vigilador Leo la Tabla*/
			if (vigilador != LFld(operac|ASIG_NROLEG)) 
				LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
			vigilador = LFld(operac|ASIG_NROLEG);

			ptoser= IFld(operac|ASIG_PTOSER);
			puesto= IFld(operac|ASIG_PUESTO);
			sprintf(regim, "%s", SFld(operac|ASIG_REGIM));
			dias[0]=*SFld(operac|ASIG_DIA1);
			dias[1]=*SFld(operac|ASIG_DIA2);
			dias[2]=*SFld(operac|ASIG_DIA3);
			dias[3]=*SFld(operac|ASIG_DIA4);
			dias[4]=*SFld(operac|ASIG_DIA5);
			dias[5]=*SFld(operac|ASIG_DIA6);
			dias[6]=*SFld(operac|ASIG_DIA7);

			horent=TFld(operac|ASIG_HSENT);
			horsal=TFld(operac|ASIG_HSSAL);
			efect=*SFld(operac|ASIG_EFECT);

			if ((cant++)%100==0) {
				DisplayMsg(FALSE, "ASIGNACIONES\nCliente %ld \tObjetivo %d", cliente , objetivo);
				WiRefresh();
			}
			ImprimoLinea();
		}
		DeleteCursor(c_asig);	

		vigilador = cliente = NULL_LONG;
		objetivo = emp = NULL_SHORT;
		cant = 0;

		sprintf(razsoc, "%s", NULL_STR);
		sprintf(objdes, "%s", NULL_STR);
		sprintf(empdes, "%s", NULL_STR);

		/* Recorro Asignaciones Historicas*/
		SetKey(operac|ASIGHbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, CLIDESDE), FmIFld(fm0, OBJDESDE),
		                               MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG, MIN_DATE, MIN_DATE);
		while (GetRecord(operac|ASIGHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
			//valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
		       	continue;
		
			if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

			/*Filtro Cliente*/
			if (!FmIsNull(fm0, CLIHASTA) && LFld(operac|ASIGH_CLIENTE)>FmLFld(fm0, CLIHASTA))
				break;

			/*Filtro Objetivo*/
			if (!FmIsNull(fm0, OBJHASTA) && IFld(operac|ASIGH_OBJETIVO)>FmIFld(fm0, OBJHASTA))
				continue;;

			/* Filtro por Nro de Legajo */
			if (!FmIsNull(fm0, VIGDESDE) && LFld(operac|ASIGH_NROLEG)<FmLFld(fm0, VIGDESDE))
				continue;
			if (!FmIsNull(fm0, VIGHASTA) && LFld(operac|ASIGH_NROLEG)>FmLFld(fm0, VIGHASTA))
				continue;

			if ((cant++)%100==0) {
				DisplayMsg(FALSE, "ASIGNACIONES HISTORICAS\nCliente %ld \tObjetivo %d", 
				                   LFld(operac|ASIGH_CLIENTE) , IFld(operac|ASIGH_OBJETIVO));
				WiRefresh();
			}
			/* Busco el ultimo por fecha de baja*/
			primero=TRUE;
			SetKey(operac|ASIGHbyEMP, IFld(operac|ASIGH_EMP), LFld(operac|ASIGH_CLIENTE), 
				                       IFld(operac|ASIGH_OBJETIVO), IFld(operac|ASIGH_PTOSER),
									   IFld(operac|ASIGH_PUESTO), IFld(operac|ASIGH_NROINT),
				                       LFld(operac|ASIGH_NROLEG), MAX_DATE, MAX_DATE);
			while (GetRecord(operac|ASIGHbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 7) != ERROR) {

				if (primero)
					PushRecord(operac|ASIGH);
				primero=FALSE;

				/* Si corresponde al periodo lo imprimo */
				if (Today() >= DFld(operac|ASIGH_FECBAJ)) 
					break;
				if (Today() <= DFld(operac|ASIGH_FECALT))
					continue;

				/* Si Cambia la Empresa Leo la Tabla*/
				if (emp != IFld(operac|ASIGH_EMP))
					sprintf(empdes,"%s",GetDescEmpresa(IFld(operac|ASIGH_EMP)));
				emp = IFld(operac|ASIGH_EMP);

				/* Si Cambia el Cliente Leo la Tabla*/
				if (cliente != LFld(operac|ASIGH_CLIENTE)){
					sprintf(razsoc, "%s", GetCliRazsoc(LFld(operac|ASIGH_CLIENTE)));
					LeerProvXDiv(IFld(bill|CLIENTE_PAIS), SFld(bill|CLIENTE_PROVP));
				}
				cliente = LFld(operac|ASIGH_CLIENTE);

				/* Si Cambia el Objetivo Leo la Tabla*/
				if (objetivo != IFld(operac|ASIGH_OBJETIVO)) 
					sprintf(objdes, "%s", GetObjDescrip(cliente, IFld(operac|ASIGH_OBJETIVO)));
				objetivo  = IFld(operac|ASIGH_OBJETIVO);

				/* Si Cambia el Vigilador Leo la Tabla*/
				if (vigilador != LFld(operac|ASIGH_NROLEG)) 
					LeerVigilador(FmIFld(fm0, EMP), LFld(operac|ASIGH_NROLEG));
				vigilador = LFld(operac|ASIGH_NROLEG);

				ptoser= IFld(operac|ASIGH_PTOSER);
				puesto= IFld(operac|ASIGH_PUESTO);
				sprintf(regim, "%s", SFld(operac|ASIGH_REGIM));
				dias[0]=*SFld(operac|ASIGH_DIA1);
				dias[1]=*SFld(operac|ASIGH_DIA2);
				dias[2]=*SFld(operac|ASIGH_DIA3);
				dias[3]=*SFld(operac|ASIGH_DIA4);
				dias[4]=*SFld(operac|ASIGH_DIA5);
				dias[5]=*SFld(operac|ASIGH_DIA6);
				dias[6]=*SFld(operac|ASIGH_DIA7);

				horent=TFld(operac|ASIGH_HSENT);
				horsal=TFld(operac|ASIGH_HSSAL);
				efect=*SFld(operac|ASIGH_EFECT);

				ImprimoLinea();
			}
			PopRecord(operac|ASIGH);
		}

		/* Cierro Archivo */
		fclose(salida);
		fclose(error);
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void ImprimoLinea()
{
		char altapol [50];
		bool portacion, clu;
		DATE fecporta, fecclu;
		fprintf(salida, "%d\t%s\t",  emp, empdes);                        // Empresa
		fprintf(salida, "%ld\t%s\t", cliente, razsoc);                    // Cliente

		fprintf(salida, "%d\t%s\t", IFld(billpro|PROVXDIV_PROVSUE),       // Provincia de Cliente
		                            GetDescProv(IFld(bill|CLIENTE_PAIS),
		                                        IFld(billpro|PROVXDIV_PROVSUE)));

		fprintf(salida, "%d\t%s\t", objetivo, objdes);                    // Objetivo
		fprintf(salida, "%s %s %s %s\t", SFld(comerc|OBJETIVO_CALLE),     // Direccion del Objetivo
		                                 SFld(comerc|OBJETIVO_NRO),
		                                 SFld(comerc|OBJETIVO_PISO),
		                                 SFld(comerc|OBJETIVO_DEPTO));

		fprintf(salida, "%ld\t%s\t", LFld(comerc|OBJETIVO_LOCAL),         // Localidad del Objetivo
		                             GetDescLocali(IFld(comerc|OBJETIVO_PAIS),
		                                           IFld(comerc|OBJETIVO_PROV),
		                                           LFld(comerc|OBJETIVO_LOCAL)));
		fprintf(salida, "%s\t", SFld(comerc|OBJETIVO_CODPOS));            // Código postal del Objetivo


		fprintf(salida, "%d\t%s\t", IFld(comerc|OBJETIVO_PROV),           // Provincia del Objetivo
		                            GetDescProv(IFld(comerc|OBJETIVO_PAIS),
		                                        IFld(comerc|OBJETIVO_PROV)));
		fprintf(salida, "%s\t%s\t", SFld(comerc|OBJETIVO_DELEGA),         // Delegacion Geografica
		                            GetDescDeleg(SFld(comerc|OBJETIVO_DELEGA)));

		fprintf(salida, "%ld\t%s\t", vigilador, SFld(sue|PER_APYNOM));    // Vigilador
		fprintf(salida, "%s\t%ld\t", SFld(sue|PER_CODDOC, 0),             // Documento
		                             LFld(sue|PER_NRODOC, 0));
		fprintf(salida, "%s\t"     , SFld(sue|DATPERS_CUIL));             // Cuil
		fprintf(salida, "%s\t%s\t" , SFld(sue|PER_DIREC),                 // Direccion de Vigilador
		                             SFld(sue|PER_CODPOST));              // Código postal de Vigilador
		fprintf(salida, "%ld\t%s\t", LFld(sue|PER_LOCAL),                 // Localidad del Vigilador
		                             GetDescLocali(IFld(sue|PER_CODPAIS),
		                                           IFld(sue|PER_PROV),
		                                           LFld(sue|PER_LOCAL)));


		fprintf(salida, "%ld\t%s\t", IFld(sue|PER_PROV),                  // Provincia del Vigilador
		                             GetDescProv(IFld(sue|PER_CODPAIS),
		                                           IFld(sue|PER_PROV)) );
		fprintf(salida, "%.3D\t", DFld(sue|PER_FECING));

		SetKey(sue|CATEbyCOD, IFld(sue|PER_RELACION), IFld(sue|PER_CODCAT));
		if(GetRecord(sue|CATEbyCOD, THIS_KEY, IO_NOT_LOCK)==ERROR)
			InitRecord(sue|CATE);

		fprintf(salida, "%d\t%s\t", IFld(sue|PER_CODCAT),                 // Categoria Salarial
		                            SFld(sue|CATE_DENOM));


		AltaPolicia (emp, vigilador, altapol);
		fprintf(salida, "%s\t", altapol);                                 // Alta Policial

		portacion=clu=FALSE;
		fecporta=fecclu=NULL_DATE;
		
		SetKey(brigada|CVIGIPOLbyEMP, emp, vigilador);
		GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (!IsNull(brigada|CVIGIPOL_VENPORTA) && DFld(brigada|CVIGIPOL_VENPORTA) > Today()) {
			portacion=TRUE;
			fecporta = DFld(brigada|CVIGIPOL_VENPORTA);
		}
		fprintf(salida, "%B\t"  , portacion);                             // Portacion 
		fprintf(salida, "%.3D\t", fecporta);                              // Fecha Portacion

		if (!IsNull(brigada|CVIGIPOL_VENCLU) && DFld(brigada|CVIGIPOL_VENCLU) > Today()) {
			clu   = TRUE;
			fecclu= DFld(brigada|CVIGIPOL_VENCLU);
		}

		fprintf(salida, "%B\t"  , clu);                             // CLU 
		fprintf(salida, "%.3D\t", fecclu);                              // Fecha CLU

		fprintf(salida, "%d\t%d\t", ptoser, puesto);                      // Puesto
		fprintf(salida, "%s\t"    , regim);                               // Regimen
		fprintf(salida, "%c\t%c\t%c\t%c\t%c\t%c\t%c\t", dias[0],          // Días
		                                                dias[1],
		                                                dias[2],
		                                                dias[3],
		                                                dias[4],
		                                                dias[5],
		                                                dias[6]);
		fprintf(salida, "%.3T\t"    , horent);                             // Hora De Entrada
		fprintf(salida, "%.3T\t"    , horsal);                             // Hora De Salida
		fprintf(salida, "%c"        , efect);                             // Efectivo o Provisorio
		fprintf(salida, "%\n");                                           // Fin de Registro


}
static void LeerVigilador(int p_emp, long p_nroleg)
{
	SetKey(sue|PERbyEMP, p_emp, p_nroleg);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		fprintf(error, PER_INEX, p_nroleg, p_emp, "PER");
		InitRecord(sue|PER);
	}
	SetKey(sue|DATPERSbyEMP, p_emp, p_nroleg);
	if (GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		fprintf(error, PER_INEX, p_nroleg, p_emp, "DATPERS");
		InitRecord(sue|DATPERS);
	}
}

static void LeerProvXDiv(int p_pais, char *p_codprov)
{
	if (strcmp(p_codprov, NULL_STR)==0) {
		InitRecord(billpro|PROVXDIV);
		return;
	}
	SetKey(billpro|PROVXDIVbyPAIS, p_pais, p_codprov);
	if (GetRecord(billpro|PROVXDIVbyPAIS, THIS_KEY, IO_NOT_LOCK) == ERROR ) {
		fprintf(error, "No Se Encuentra Relacion de País %d  Provincia %s\n", p_pais, p_codprov);
		InitRecord(billpro|PROVXDIV);
	}
}
void AltaPolicia (short emp, long nroleg, char *p_altapol)
{
	// Busco donde esta dado de alta en la Policia

	sprintf(p_altapol, "%s", NULL_STR);

	SetKey(brigada|VIGIPOLbyULTMOD, TRUE, emp, nroleg, NULL_SHORT, NULL_SHORT);
	while (GetRecord(brigada|VIGIPOLbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		if(!IFld(brigada|VIGIPOL_ACTIVO))
			continue;

		PushRecord(billpro|PROVXDIV);
		SetKey(billpro|PROVXDIVbyPORSUE, IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI));
		if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) == ERROR)
			InitRecord(billpro|PROVXDIV);

		sprintf(p_altapol, "%s %s", p_altapol, SFld(billpro|PROVXDIV_CODPROV));

		PopRecord(billpro|PROVXDIV);
	} 
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIDESDE:
	   	InicClientesXusr();
    	break;
    case CLIHASTA:
    	break;
    case OBJDESDE:
	   	InicObjetivosXusr(FmLFld(fm, CLIDESDE, row), FmIFld(fm, EMP, row));
    	break;
    case OBJHASTA:                                 
	   	InicObjetivosXusr(FmLFld(fm, CLIHASTA, row), FmIFld(fm, EMP, row));
    	break;
	case FFILIAL:
		break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
        	InicListaXusr(FmIFld(fm0, EMP));
    break;
	case CLIDESDE:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
		  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLIDESDE, row)), row);
    break;
    case CLIHASTA:
		if (FmKeyCode(fm) == K_HELP)
			HelpCliente(fm, fno, row);
  		else
			FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIHASTA, row)),row);
   	break;
    case OBJDESDE:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIDESDE, row));
  		else
			FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIDESDE, row), FmIFld(fm, OBJDESDE, row)), row);
	break;
    case OBJHASTA:
		if (FmKeyCode(fm) == K_HELP)
			HelpObjet(fm, fno, row, FmLFld(fm, CLIHASTA, row));
		else	
			FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIHASTA, row) ,FmIFld(fm, OBJHASTA, row)), row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	}
	return FM_OK;				
}	

