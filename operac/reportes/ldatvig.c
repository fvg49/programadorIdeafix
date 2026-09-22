/********************************************************************
* MODULE & VERSION : %W%
* DATE             : %E%
* TIME             : %U%
*
* CREATED          : 10/02/12 (este programa no estuvo adminsitrado has
*
* DESCRIPTION:
*      Listado de vigiladores asignados por puestos.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "ldatvig.fmh"
#include "ldatvig.rph"
#include "operac.sch"
#include "comerc.sch"
#include "billpro.sch"
#include "brigada.sch"
#include "bill.sch"
#include "sue.sch"
#include "brigada.h"
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "filial.h"
#include "sue.h"

/* Funciones privadas */
static fm_status after (form, fmfield, int);
static fm_status before(form, fmfield, int);
static void Proceso();
static void ProcesarArchivo();
static void ProcesarReporte();
static void AbrirSalida();
static void ImprimirEncabezadoArchivo();
static void CerrarSalida();
static void	ImprimirVigiladorDatosPersonalesReporte();  //Rutina donde se imprime los datos del vigilador o por impresora o por terminal.
static void	ImprimirVigiladorDatosPersonalesArchivo();  //Rutina donde se imprime los datos del vigilador persistiendo en archivo (fichero).
void AltaPolicia (short emp, long nroleg, char *altapol1, char *altapol2, char *altapol3, char *altapol4);

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema operac, bill, comerc, sue, brigada, billpro;
bool salida;

/* Programa principal */
wcmd(ldatvig, %I% %G%)
{
	fm0    = OpenForm("ldatvig", FM_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	billpro= OpenSchema("billpro", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	sue    = OpenSchema("sue", 	IO_EABORT);
	brigada= OpenSchema("brigada", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while(DoForm(fm0, before, after) != FM_EXIT) {
		AbrirSalida();
		Proceso();
		CerrarSalida();
   }
   
   FinObjetivosXusr();   
   FinClientesXusr();
   FinListaXusr();
}

static void Proceso()	
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A"))
    	ProcesarArchivo();
	else
		ProcesarReporte();
}

static void ProcesarArchivo() 
{

	dbcursor  c_asig, c_OBJ = NULL;
	bool first_asig=TRUE;

	c_asig = CreateCursor(operac|ASIGbyPUESTO, IO_NOT_LOCK);

	switch(*FmSFld(fm0, OPCION)) {
		case 'P':
			c_OBJ = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, PRESD), NULL_LONG, NULL_SHORT);
			SetCursorTo  (c_OBJ, FmLFld(fm0, PRESH), MAX_LONG, MAX_SHORT);
			break;
		case 'C':
			c_OBJ = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
			SetCursorTo  (c_OBJ, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
			break;
		case 'R':
			c_OBJ = CreateCursor(comerc|OBJETIVObyPROGRAM, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, PRESD), NULL_LONG, NULL_SHORT);
			SetCursorTo  (c_OBJ, FmLFld(fm0, PRESH), MAX_LONG, MAX_SHORT);
			break;
		
	}

	while (FetchCursor(c_OBJ) != ERROR) {
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)) )
	       	continue;

		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		SetLFld(bill|CLIENTE_CLIENTE, LFld(comerc|OBJETIVO_CLIENTE));
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		first_asig=TRUE;

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG);
		SetCursorTo (c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);
		while (FetchCursor(c_asig) != ERROR) {
			ImprimirVigiladorDatosPersonalesArchivo();
	 	}
	}
}

static void ProcesarReporte()	
{
	dbcursor  c_asig;
	dbcursor  c_OBJ;
	bool first  = TRUE, first_asig=TRUE;

	c_asig = CreateCursor(operac|ASIGbyPUESTO, IO_NOT_LOCK);

	switch(*FmSFld(fm0, OPCION)) {
		case 'P':
			c_OBJ = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, PRESD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_OBJ, FmLFld(fm0, PRESH), MAX_LONG, MAX_SHORT);
			break;
		case 'C':
			c_OBJ = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
			SetCursorTo  (c_OBJ, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
			break;
		case 'R':
			c_OBJ = CreateCursor(comerc|OBJETIVObyPROGRAM, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, PRESD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_OBJ, FmLFld(fm0, PRESH), MAX_LONG, MAX_SHORT);
			break;
		
	}

	while (FetchCursor(c_OBJ) != ERROR) {
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)) )
	       	continue;

		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		SetLFld(bill|CLIENTE_CLIENTE, LFld(comerc|OBJETIVO_CLIENTE));
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

		if(*FmSFld (fm0, SALIDA) == 'I') {
			RpSetLFld(rp0, R_CLI,    LFld(comerc|OBJETIVO_CLIENTE));
			RpSetFld (rp0, R_DCLI,   SFld(bill|CLIENTE_RAZSOC));
			RpSetLFld(rp0, R_PRESEN, LFld(comerc|OBJETIVO_PRESEN));
			RpSetFld (rp0, R_DPRESEN,  GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PRESEN)));
			RpSetIFld(rp0, R_OBJ,    IFld(comerc|OBJETIVO_OBJET));
			RpSetFld (rp0, R_DOBJ,   GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
			RpSetLFld(rp0, R_PROGRAM, LFld(comerc|OBJETIVO_PROGRAM));
			RpSetFld (rp0, R_DPROGRAM,   GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));
            RpSetFld (rp0, R_FIL,  		GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
			RpSetFld (rp0, R_DFIL, 		GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))));
            
			if (!first)
				RpEjectPage(rp0);

			DoReport (rp0, ENCAB);
			first  = FALSE;
		} 

		first_asig=TRUE;

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG);
		SetCursorTo (c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);
		while (FetchCursor(c_asig) != ERROR) {
			if (first_asig) {
				RpSetLFld(rp0, R_CLI,    LFld(comerc|OBJETIVO_CLIENTE));
				RpSetFld (rp0, R_DCLI,   SFld(bill|CLIENTE_RAZSOC));
				RpSetLFld(rp0, R_PRESEN, LFld(comerc|OBJETIVO_PRESEN));
				RpSetFld (rp0, R_DPRESEN,  GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PRESEN)));
				RpSetIFld(rp0, R_OBJ,    IFld(comerc|OBJETIVO_OBJET));
				RpSetFld (rp0, R_DOBJ,   GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
				RpSetLFld(rp0, R_PROGRAM, LFld(comerc|OBJETIVO_PROGRAM));
				RpSetFld (rp0, R_DPROGRAM,   GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));
				RpSetFld (rp0, R_FIL,  		GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
				RpSetFld (rp0, R_DFIL, 		GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))));

				first_asig  = FALSE;
				DoReport (rp0, ENCAB);
			}
			ImprimirVigiladorDatosPersonalesReporte();
	 	}
	}
}

static void	ImprimirVigiladorDatosPersonalesReporte() {
	char altapol1[10], altapol2[10], altapol3[10], altapol4[10];
	RpSetLFld(rp0, RLEGAJO,  LFld(operac|ASIG_NROLEG));

	RpSetFld  (rp0, RAPENOM,  GetNombreLeg(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG)));
	RpSetFld  (rp0, RTIPDOC,	SFld (sue|PER_CODDOC, 0));
	RpSetLFld (rp0, RNRODOC,	LFld (sue|PER_NRODOC, 0));
	RpSetDFld (rp0, RFECNAC,	DFld (sue|PER_FECNAC));
	RpSetFld  (rp0, RCALLE,		SFld (sue|PER_DIREC));
	RpSetDFld (rp0, RFING,		DFld (sue|PER_FECING));
	RpSetDFld (rp0, RFEGR,		DFld (sue|PER_FECEGR));
	SetKey (sue|LOCALIbyCODPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV), LFld (sue|PER_LOCAL));
	if (GetRecord (sue|LOCALIbyCODPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		RpSetFld  (rp0, RLOCAL, SFld (sue|LOCALI_DESCRIP));

	SetKey (sue|PROVIbyPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV));
	if (GetRecord (sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		RpSetFld  (rp0, RPROVI, SFld (sue|PROVI_DENOM));

	RpSetFld  (rp0, RTEL,	SFld (sue|PER_TELEF, 0));

	if (!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		RpSetFld(rp0, RFR, "F");
	else
		RpSetFld(rp0, RFR, NULL_STR);

	RpSetFld (rp0, RREGIM, SFld(operac|ASIG_REGIM));
	RpSetIFld(rp0, RCODINT, IFld(operac|ASIG_PUESTO));
	RpSetTFld(rp0, RHENT,  TFld(operac|ASIG_HSENT));
	RpSetTFld(rp0, RHSAL,  TFld(operac|ASIG_HSSAL));

	AltaPolicia (FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), altapol1, altapol2, altapol3, altapol4);
 	RpSetFld(rp0, RALTAP1, altapol1);
	RpSetFld(rp0, RALTAP2, altapol2);
	RpSetFld(rp0, RALTAP3, altapol3);
	RpSetFld(rp0, RALTAP4, altapol4);

	SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
	GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

	RpSetIFld(rp0, RCLU,      FALSE);
	RpSetDFld(rp0, RFVENCLU,  NULL_DATE);
	RpSetIFld(rp0, RPORT,     FALSE);
	RpSetDFld(rp0, RFVENPORT, NULL_DATE);

	if (!IsNull(brigada|CVIGIPOL_VENCLU) && DFld(brigada|CVIGIPOL_VENCLU) > Today()) {
		RpSetIFld(rp0, RCLU,     TRUE);
		RpSetDFld(rp0, RFVENCLU, DFld(brigada|CVIGIPOL_VENCLU));
	}
	if (!IsNull(brigada|CVIGIPOL_VENPORTA) && DFld(brigada|CVIGIPOL_VENPORTA) > Today()) {
		RpSetIFld(rp0, RPORT,     TRUE);
		RpSetDFld(rp0, RFVENPORT, DFld(brigada|CVIGIPOL_VENPORTA));
	}

	DoReport (rp0, LINVIG);
	RpClearZone(rp0, LINVIG);
}

static void	ImprimirVigiladorDatosPersonalesArchivo()	{
	bool clu, port;
	DATE fvenclu, fvenport;
	
	fprintf(fp, "%ld\t",	LFld(operac|ASIG_NROLEG));
	fprintf(fp, "%s\t",		GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG)));
	                             
	//EN la funcion GenNombreLeg se lee PER
	fprintf(fp, "%s\t",		SFld (sue|PER_CODDOC, 0));
	fprintf(fp, "%ld\t",	LFld (sue|PER_NRODOC, 0));
	fprintf(fp, "%.3D\t",	DFld (sue|PER_FECNAC));
	fprintf(fp, "%s\t",		SFld (sue|PER_DIREC));
	SetKey (sue|LOCALIbyCODPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV), LFld (sue|PER_LOCAL));
	if (GetRecord (sue|LOCALIbyCODPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fprintf(fp, "%s\t",	SFld (sue|LOCALI_DESCRIP));
     else
     	fprintf(fp, "%s\t", NULL_STR);

	SetKey (sue|PROVIbyPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV));
	if (GetRecord (sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fprintf(fp, "%s\t",	SFld (sue|PROVI_DENOM));
	else
		fprintf(fp, "%s\t", NULL_STR);	

	fprintf(fp, "%s\t",		SFld (sue|PER_TELEF, 0));

	if (!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		fprintf(fp, "%s\t",	"F");
	else
		fprintf(fp, "%s\t",	NULL_STR);

	fprintf(fp, "%s\t",		SFld(operac|ASIG_REGIM));
	fprintf(fp, "%d\t",		IFld(operac|ASIG_PUESTO));
	fprintf(fp, "%.1T\t",	TFld(operac|ASIG_HSENT));
	fprintf(fp, "%.1T\t",	TFld(operac|ASIG_HSSAL));

	SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
	GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

	clu = FALSE; fvenclu = NULL_DATE; port = FALSE; fvenport = NULL_DATE; 

	if (!IsNull(brigada|CVIGIPOL_VENCLU) && DFld(brigada|CVIGIPOL_VENCLU) > Today()) {
		clu = TRUE;
		fvenclu = DFld(brigada|CVIGIPOL_VENCLU);
	}
	if (!IsNull(brigada|CVIGIPOL_VENPORTA) && DFld(brigada|CVIGIPOL_VENPORTA) > Today()) {
		port = TRUE;
		fvenport = DFld(brigada|CVIGIPOL_VENPORTA);
	}
	

	fprintf(fp, "%B\t",	port);
	fprintf(fp, "%.3D\t",	fvenport); 
	fprintf(fp, "%B\t",	clu);
	fprintf(fp, "%.3D\t",	fvenclu);
	fprintf(fp, "%ld\t",	LFld(comerc|OBJETIVO_CLIENTE));
	fprintf(fp, "%s\t",		SFld(bill|CLIENTE_RAZSOC));
	fprintf(fp, "%d\t",		IFld(comerc|OBJETIVO_OBJET));
	fprintf(fp, "%s\t",		GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));

	fprintf(fp, "%ld\t",	LFld(comerc|OBJETIVO_PRESEN));
	fprintf(fp, "%s\t",		GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PRESEN)));
	fprintf(fp, "%ld\t",	LFld(comerc|OBJETIVO_PROGRAM));
	fprintf(fp, "%s\t",		GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));
	fprintf(fp, "%.3D\t",	DFld(sue|PER_FECING));
	fprintf(fp, "%.3D\t",	DFld(sue|PER_FECEGR));
	fprintf(fp, "%s\t", 	GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
    fprintf(fp, "%s\t", 	GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET))));

	fprintf(fp, "\n");
}

static void AbrirSalida()	{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		fp = fopen(FmSFld(fm0, ARCHIVO) , "w");
		ImprimirEncabezadoArchivo();
	}
	
	if (!strcmp(FmSFld(fm0, SALIDA), "T")) {
		rp0 = OpenReport("ldatvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}
	
	if (!strcmp(FmSFld(fm0, SALIDA), "I")) {
		rp0 = OpenReport("ldatvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}
}

static void CerrarSalida() {

	if (strcmp(FmSFld(fm0, SALIDA), "A"))
		CloseReport(rp0);
	else
		fclose(fp);	
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case OPCION :
			switch(*FmSFld(fm, fno)) {
				case 'C' :
					FmSetFld (fm0, APYNOMD, NULL_STR);
					FmSetFld (fm0, APYNOMH, NULL_STR);
					break;
				case 'R' :
				case 'P' :
					FmSetFld (fm0, DCLID, NULL_STR);
					FmSetFld (fm0, DCLIH, NULL_STR);
					FmSetFld (fm0, DOBJD, NULL_STR);
					FmSetFld (fm0, DOBJH, NULL_STR);
					break;
			}
			break;
		case EMP:
		    if (FmChgFld(fm))
			    InicListaXusr(FmIFld(fm0, EMP));
    	break;
		case CLID:
			if (FmKeyCode(fm) == K_HELP)
				HelpCliente(fm, fno, row);
	  		else
			  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLID, row)), row);
	    break;
	    case CLIH:
			if (FmKeyCode(fm) == K_HELP)
				HelpCliente(fm, fno, row);
	  		else
			  	FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIH, row)), row);
	   	break;
	    case OBJD:
			if (FmKeyCode(fm) == K_HELP)
				HelpObjet(fm, fno, row, FmLFld(fm, CLID, row));
	  		else
				FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLID, row), FmIFld(fm, OBJD, row)), row);
		break;
    	case OBJH:
			if (FmKeyCode(fm) == K_HELP)
				HelpObjet(fm, fno, row, FmLFld(fm, CLIH, row));
	  		else
				FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row), FmIFld(fm, OBJH, row)), row);
		break;
		case FFILIAL:
			if (FmKeyCode(fm) == K_HELP)
				HelpFilial(fm, fno, row);
		break;
	}
	return FM_OK;
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
		case ARCHIVO :
			if (*FmSFld(fm, SALIDA) == 'A' && FmIsNull (fm, ARCHIVO)) {
				FmSetFld (fm, ARCHIVO, "ldatvig.txt");
			} 
		break;
		case CLID:
		   	InicClientesXusr();
   		break;
	    case CLIH:
    	break;
	    case OBJD:
		   	InicObjetivosXusr(FmLFld(fm, CLID, row), FmIFld(fm, EMP, row));
    	break;
	    case OBJH:                                 
		   	InicObjetivosXusr(FmLFld(fm, CLIH, row), FmIFld(fm, EMP, row));
    	break;
		case FFILIAL:
		break;

	}
	return FM_OK;
}

void AltaPolicia (short emp, long nroleg, char *altapol1, char *altapol2, char *altapol3, char *altapol4)
{
	// Busco donde esta dado de alta en la Policia
	int i = 0;

	strcpy (altapol1, NULL_STR);	
	strcpy (altapol2, NULL_STR);	
	strcpy (altapol3, NULL_STR);	
	strcpy (altapol4, NULL_STR);	

	SetKey(brigada|VIGIPOLbyULTMOD, TRUE, emp, nroleg, NULL_SHORT, NULL_SHORT);
	while (GetRecord(brigada|VIGIPOLbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		if(!IFld(brigada|VIGIPOL_ACTIVO))
			continue;

		if(!UsrInGrupo(GRPBRIG, GetUid()) )
			if (!InscVig(IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI),
			              IFld(brigada|VIGIPOL_ACEPTADO), DFld(brigada|VIGIPOL_FECHA), NULL))
				continue;
	
		SetKey(billpro|PROVXDIVbyPORSUE, IFld(brigada|VIGIPOL_CODPAIS), IFld(brigada|VIGIPOL_CODPROVI));
		if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) != ERROR)
			switch (i) {
			case 0:
				strcpy(altapol1, SFld(billpro|PROVXDIV_CODPROV));
				i++;
			break;
			case 1:
				strcpy(altapol2, SFld(billpro|PROVXDIV_CODPROV));
				i++;
				break;
			case 2:
				strcpy(altapol3, SFld(billpro|PROVXDIV_CODPROV));
				i++;
			break;
			case 3:
				strcpy(altapol4, SFld(billpro|PROVXDIV_CODPROV));
				i++;
			break;
		}
		else {
			if (i < 4) {
				char provesp[2];
				SetIFld(sue|PROVI_PAIS,   IFld(brigada|VIGIPOL_CODPAIS));
				SetIFld(sue|PROVI_PROVIN, IFld(brigada|VIGIPOL_CODPROVI));
				if (GetRecord(sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
					if (IFld(sue|PROVI_PROVIN) == AERONAUTICA)
						strcpy(provesp, "A");
					else 
						strcpy(provesp, "P");
					switch (i) {
					case 0:
						strcpy(altapol1, provesp);
						i++;
					break;
					case 1:
						strcpy(altapol2, provesp);
						i++;
					break;
					case 2:
						strcpy(altapol3, provesp);
						i++;
					break;
					case 3:
						strcpy(altapol4, provesp);
						i++;
					break;
					}
				}
			}
		}

		if (i == 4)
			break;
	} 
}

static void ImprimirEncabezadoArchivo() {
	fprintf(fp, "Legajo	Apellido y Nombre	Tipo Doc	Documento	Fec.Nac.	Domicilio	Localidad	Provincia	Telefono	Franquero	Regimen	   Nro Puesto	H. Entrada	H. Salida	Inscripto en Portacion	Fecha Venc	Inscripto en CLU	Fecha Venc	Cliente	Razon social	Objetivo	Descripcion Objetivo	Legajo Presentismo	Apellido y nombre del Presentismo	Legajo Programador	Apellido y Nombre del Programador	Fecha Ingreso	Fecha Cese	Filial Objetivo	Descrip. Filial Objetivo\n");
}


