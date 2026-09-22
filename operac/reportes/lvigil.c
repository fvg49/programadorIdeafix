/********************************************************************
*
* MODULE & VERSION : @(#)lvigil.c	1.18 
* DATE             : 08/11/07 
* TIME             : 12:14:22 
*
* CREATED          : 27/07/99
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "filial.h"
#include "lvigil.fmh"       
#include "lvigil.rph"
#include "sue.sch"
#include "operac.sch"
#include "prosegur.sch"
#include "bill.sch"
#include "comerc.sch"

/* Funciones privadas */
static fm_status before(form, fmfield, int), after(form, fmfield, int);
static void Proceso(void);
static void ImprimoCompos(int codcat, char * categ);
static void GetSalario(int puesto, double sueldo_basico, double * plus, double * valor_tk, double * otros);

/* Declaraciones globales */
schema sue, oper, com, bill, seg;
form fm0;
report rp;
int convenio, g_emp;
DATE fecierre;

/* Programa principal */
wcmd(lvigil, 1.18 11/07/08)
{
	fm0 = OpenForm("lvigil", FM_EABORT);
	sue = OpenSchema("sue",    IO_EABORT);
	oper= OpenSchema("operac", IO_EABORT);
	com = OpenSchema("comerc", IO_EABORT);
	bill= OpenSchema("bill",   IO_EABORT);
	seg = OpenSchema("prosegur", IO_EABORT);

	FmShowAllFlds(fm0);

	// Inicio Permisos
	g_emp= StrToI(getenv("emp"));
	fecierre  = GetFechaCierreOpe(g_emp);
	InicLegajoXusr (g_emp, fecierre, fm0, MENSAJE, TRUE, MAX_SHORT);
//	ImprimeLegajoXusr();

	while(DoForm(fm0, before, after) != FM_EXIT)	{
		rp = OpenReport("lvigil", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp, RP_IO_TERM, NULL_STR );
    	BeginReport(rp, 1, NULL_STR);    

		Proceso();
		
		CloseReport(rp);
    }
}
static void Proceso(void) 
{
	char estciv[2];                                
	char regimen[9];
	long cliente=0;
	int objet=0;
	sprintf(regimen, "%s", NULL_STR);
	
	SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG));  
	(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
	RpSetLFld(rp, RNROLEG, LFld(sue|PER_NROLEG));
	RpSetFld (rp, RAPYNOM, SFld(sue|PER_APYNOM));

	if(IFld(sue|PER_SEXO) == 1)
		RpSetFld(rp, RSEXO, "M");
	else
		RpSetFld(rp, RSEXO, "F");

	RpSetFld (rp, RTIPDOC1, SFld(sue|PER_CODDOC, 0));
	RpSetLFld(rp, RNRODOC1, LFld(sue|PER_NRODOC, 0));
	RpSetFld (rp, RTIPDOC2, SFld(sue|PER_CODDOC, 1));
	RpSetLFld(rp, RNRODOC2, LFld(sue|PER_NRODOC, 1));
	RpSetFld (rp, RDIREC,   SFld(sue|PER_DIREC));
	RpSetFld (rp, RTELEF,   SFld(sue|PER_TELEF,  0));

	SetKey(sue|LOCALIbyCODPAIS, IFld(sue|PER_CODPAIS), IFld(sue|PER_PROV), LFld(sue|PER_LOCAL));
	(void)GetRecord(sue|LOCALIbyCODPAIS, THIS_KEY, IO_NOT_LOCK);
	RpSetFld (rp, RLOCAL, SFld(sue|LOCALI_DESCRIP));
	RpSetFld (rp, RCODPOS, SFld(sue|PER_CODPOST));
	RpSetDFld(rp, RFECNAC, DFld(sue|PER_FECNAC));

	SetKey(sue|DATPERSbyEMP, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG));
	(void)GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RCUIL, SFld(sue|DATPERS_CUIL));

	SetKey(sue|NACIONbyCODNAC, IFld(sue|PER_CODNAC));
	(void)GetRecord(sue|NACIONbyCODNAC, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RNACION, SFld(sue|NACION_DENOM));
	IToStr(IFld(sue|PER_ESTCIV), estciv);
	RpSetFld(rp, RESTCIV, InDescr(sue|PER_ESTCIV, estciv));

    SetKey(sue|ESTUDbyCOD, IFld(sue|PER_CODEST));
    (void)GetRecord(sue|ESTUDbyCOD, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RESTUD, SFld(sue|ESTUD_DET));
	DoReport(rp, LINEA);
	RpSetDFld(rp, RFECING, DFld(sue|PER_FECING));
	SetKey(oper|ASIGbyNROLEG, FmIFld(fm0, EMP), FmLFld(fm0, NROLEG), MIN_LONG, MIN_SHORT);
	while(GetRecord(oper|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if(StrCmp(SFld(oper|ASIG_EFECT), "E"))
			continue;
		cliente = LFld(oper|ASIG_CLIENTE);
		objet   = IFld(oper|ASIG_OBJETIVO);
		sprintf(regimen, "%s", SFld(oper|ASIG_REGIM));
		break;			
	}         
	RpSetLFld(rp, RCLIENTE, cliente);
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	(void)GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RDCLIENTE, SFld(bill|CLIENTE_RAZSOC));
	RpSetIFld(rp, ROBJET, objet);
	SetKey(com|OBJETIVObyCLIENTE, cliente, objet);
	(void)GetRecord(com|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RDOBJET, SFld(com|OBJETIVO_DESCRIP));
	RpSetFld(rp, RREGIMEN, regimen);
	SetKey(sue|CATEbyCOD, IFld(sue|PER_RELACION), IFld(sue|PER_CODCAT));
	(void)GetRecord(sue|CATEbyCOD, THIS_KEY, IO_NOT_LOCK);
	RpSetIFld(rp, RCODCAT, IFld(sue|PER_CODCAT));
	RpSetFld(rp, RCATEG, SFld(sue|CATE_DENOM));
	RpSetLFld(rp, RCTROCTO, LFld(sue|PER_CODCCOS));
	RpSetLFld(rp, RLUGPAG, LFld(sue|DATPERS_LUGPAG));
	SetKey(sue|OFPAGbyEMP, FmIFld(fm0, EMP), LFld(sue|DATPERS_LUGPAG));
	(void)GetRecord(sue|OFPAGbyEMP, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RDLUGPAG, SFld(sue|OFPAG_DENOM));
	RpSetIFld(rp, ROS, IFld(sue|PER_CODOS));
	SetKey(sue|OSOCbyEMP, FmIFld(fm0, EMP), IFld(sue|PER_CODOS));
	(void)GetRecord(sue|OSOCbyEMP, THIS_KEY, IO_NOT_LOCK);
	RpSetFld(rp, RDOS, SFld(sue|OSOC_DENOM));
	RpSetFld(rp, RNROAFIL, SFld(sue|PER_NROAOS));
	RpSetDFld(rp, RFECEGR, DFld(sue|PER_FECEGR));
	DoReport(rp, LINEA1);                        
	ImprimoCompos(IFld(sue|PER_CODCAT), SFld(sue|CATE_DENOM));
}                   
static void ImprimoCompos(int codcat, char * categ)
{
   	double sueldo_basico, ticket, suelad, pread, tickad;
	double valor_tk, plus, otros;

	(void) GetSBasico(convenio, codcat, &sueldo_basico, &ticket, &suelad, &pread, &tickad);
    (void) GetSalario(codcat, sueldo_basico, &plus, &valor_tk, &otros);                      
	RpSetIFld(rp, R_CATS,       codcat);
	RpSetFld (rp, R_CATSAL,     categ);
	RpSetLFld(rp, R_SBAS,       sueldo_basico/10); //Sueldo Básico
	RpSetLFld(rp, R_PLUS,       plus/10);
	RpSetLFld(rp, R_TK,         valor_tk / 10);                                                  
	RpSetLFld(rp, R_OTROS,      otros/10);                                                  

	RpSetLFld(rp, R_TOTAL,      (valor_tk/10 + plus/10 + sueldo_basico/10 + otros/10) );
	                             
	                             
	                             
	DoReport(rp, LINSAL);
}   
static void GetSalario(int puesto, double sueldo_basico, double * plus, double * valor_tk, double * otros)
{
    long porcentaje = NULL_LONG;
    *plus=*valor_tk=*otros=0.0;
    
//	(void) GetSBasico(convenio, codcat, &sueldo_basico, &ticket, &suelad, &pread, &tickad);
	SetKey(seg|CATESUPLbyRELACION, convenio, puesto, MIN_SHORT); 
	while (GetRecord(seg|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			switch (IFld(seg|CATESUPL_CODSUPL)) {
				case SUPLE_PLUS :
					*plus = FFld(seg|CATESUPL_VAL);
					break;   
					
				case SUPLE_PORC :
				    porcentaje = FFld(seg|CATESUPL_VAL);

					*valor_tk = (((*plus) + (sueldo_basico)) * porcentaje) / 100000;
					break;
					
				default :
					*otros += FFld(seg|CATESUPL_VAL);
					break;					
		}
	} 
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
		break;
	case NROLEG:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	long nroleg, cliente;
	int obj;

	switch (fno) {
	case EMP:
		convenio = GetConvenioPorEmp(FmIFld(fm0, EMP));

		if (FmChgFld(fm)){
			InicLegajoXusr (FmIFld(fm, EMP), fecierre, fm0, MENSAJE, TRUE, MAX_SHORT);
		}

		break;
	case NROLEG:

		switch(FmKeyCode(fm0)) {
			case K_HELP:
				HelpLegajo(fm0, fno, 0);
				break;
			case K_META:
				nroleg = ERROR;
				if ( (nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;

				FmSetLFld(fm, fno, nroleg, row);
				break;
		}

		if (!FmIsNull(fm, fno, row)){
			if ( !ValidaLegajoXusr(FmLFld(fm, NROLEG, row), MAX_DATE))
				if (!FmIsNull(fm, fno, row)) {
					WiDialog(WD_OK, WD_OK, "Error", "Legajo Inactivo");
					return FM_REDO;
				}
		}
		else {
			Warning("[1mEl dato es obligatorio[0m");
			return FM_REDO;
		}
	  	FmSetFld(fm, APYNOM, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, NROLEG, row)), row);

		if (!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION))) {
			Warning("Legajo %ld no es vigilador", LFld(sue|PER_NROLEG));
			return FM_REDO;
		}
		GetCliObjEfectivo(FmIFld(fm0, EMP), FmLFld(fm0,NROLEG), Today(), &cliente, &obj);

		if (GetUid() != 1046 && GetUid() != 1045 && GetUid() != 1443 &&
			ExisteCliObjEnGrp(GRPPERDISPS, cliente, obj))
			Error("No tiene permisos para consultar este legajo");

		break;
	}
	return FM_OK;
}


