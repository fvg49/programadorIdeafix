/********************************************************************
* MODULE & VERSION : @(#)ldatvig.c	1.2
* DATE             : 11/09/23
* TIME             : 13:49:36
*
* CREATED          : 02/02/00
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
#include "ldatvig2.rph"
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
#include "suer.sch"
#include "filial.h"
#include "promex.sch"
#include "promex.h"
#include "prosegur.sch"

#define FEMENINO 	"FEMENINO"
#define MASCULINO 	"MASCULINO"

/* Funciones privadas */
static fm_status after (form, fmfield, int);
static fm_status before(form, fmfield, int);
static void ProcesoReporte();
static void ProcesoArchivo();
static void Proceso();
static void AbrirSalida();
static void	ImprimirVigiladorReporte();
static void	ImprimirVigiladorArchivo();
static void CerrarSalida();
void AltaPolicia (short emp, long nroleg, char *altapol1, char *altapol2, char *altapol3, char *altapol4);
static void ImprimirEncabezadoReporte();
static void ImprimirEncabezadoArchivo();

void GetApellidos(char * Ap, char * ApPat, char *ApMat);
int prep(char *Ap, int dondepuede, int tiporetorno);
void FixApellido(char *Entrada, char *Salida);
void SacarEspacios (char *SEin, char *SEout);
static double ObtengoSalarioInt(int emp, long nroleg);

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema operac, bill, comerc, sue, brigada, billpro, mx, suer, seg;
bool salida;

/* Programa principal */
wcmd(ldatvig, 1.2 09/23/11)
{
	fm0    = OpenForm("ldatvig", FM_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	billpro= OpenSchema("billpro", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	sue    = OpenSchema("sue", 	IO_EABORT);
	brigada= OpenSchema("brigada", IO_EABORT);
	operac = OpenSchema("operac",   IO_EABORT);
	suer   = OpenSchema("suer",     IO_EABORT);
	mx     = OpenSchema("promex",   IO_EABORT);
	seg    = OpenSchema("prosegur", IO_EABORT);

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

static void ProcesoReporte()
{
	dbcursor  c_asig = NULL, c_OBJ = NULL;
	bool first  = TRUE, first_asig=TRUE;

	c_asig = CreateCursor(operac|ASIGbyPUESTO, IO_NOT_LOCK);

	switch(*FmSFld(fm0, OPCION)) {
		case 'P':
			c_OBJ = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
			SetCursorFrom(c_OBJ, FmLFld(fm0, PRESD), MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_OBJ, FmLFld(fm0, PRESH), MAX_LONG, MAX_SHORT);
			break;
		case 'C':
			c_OBJ = CreateCursor(comerc|OBJETIVO, IO_NOT_LOCK);
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

			if (!first)
				RpEjectPage(rp0);

            ImprimirEncabezadoReporte();
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
					ImprimirEncabezadoReporte();
					first_asig = FALSE;
				}
            ImprimirVigiladorReporte();
	 	}
	}
} 

static void ProcesoArchivo()
{
	dbcursor  c_asig = NULL, c_OBJ = NULL;
//	bool first  = TRUE; //, first_asig=TRUE;

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

//		first_asig=TRUE;

		SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_LONG);
		SetCursorTo (c_asig, FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET),
							MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_LONG);

		while (FetchCursor(c_asig) != ERROR) {
			ImprimirVigiladorArchivo();
	 	}
	}
}

static void	ImprimirVigiladorReporte()
{
	char altapol1[10], altapol2[10], altapol3[10], altapol4[10];
	
	SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
	(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);
	RpSetLFld(rp0, RLEGAJO,  LFld(operac|ASIG_NROLEG));
	RpSetFld (rp0, RAPENOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG)));
	//EN la funcion GenNombreLeg se lee PER
	RpSetFld  (rp0, RTIPDOC,	SFld (sue|PER_CODDOC, 0));
	RpSetLFld (rp0, RNRODOC,	LFld (sue|PER_NRODOC, 0));
	RpSetDFld (rp0, RFECNAC,	DFld (sue|PER_FECNAC));
	RpSetFld  (rp0, RCALLE,		SFld (sue|PER_DIREC));

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

static void AbrirSalida()
{
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

static void ImprimirEncabezadoReporte()
{

	RpSetLFld(rp0, R_CLI,    LFld(comerc|OBJETIVO_CLIENTE));
	RpSetFld (rp0, R_DCLI,   SFld(bill|CLIENTE_RAZSOC));
	RpSetLFld(rp0, R_PRESEN, LFld(comerc|OBJETIVO_PRESEN));
	RpSetFld (rp0, R_DPRESEN,  GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PRESEN)));
	RpSetIFld(rp0, R_OBJ,    IFld(comerc|OBJETIVO_OBJET));
	RpSetFld (rp0, R_DOBJ,   GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));
	RpSetLFld(rp0, R_PROGRAM, LFld(comerc|OBJETIVO_PROGRAM));
	RpSetFld (rp0, R_DPROGRAM,   GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PROGRAM)));
    
    DoReport (rp0, ENCAB);
} 

static void ImprimirEncabezadoArchivo()
{
	//fprintf(fp, "Empresa\tDescripcion Empresa\tLegajo\tApellido y Nombre\tTipo Doc\tDocumento\tFec.Nac.\tDomicilio\tLocalidad\tProvincia\tTelefono\tFranquero\tRegimen\tH. Entrada\tH. Salida\tAlta Policial\tInscripto en Portacion\tFecha Venc\tInscripto en CLU\tFecha Venc\n");
//	fprintf(fp, "Empresa	Descripcion Empresa	Legajo	Apellido y Nombre	Cliente	Razon Social	Objetivo	Descripción Objetivo	Tipo de Documento	Numero del documento	RFC	IMSS	CURP	Fecha de Nacimiento	Direccion	Localidad	Provincia	Telefono	Es franquero?	Regimen	Hora de entrada	Hora de salida	Alta policial 1	Alta policial 2	Alta policial 3	Alta policial 4	Inscripto en Portacion	Fecha de Vencimiento	Inscripto en CLU	Fecha de vencimiento\n");
	fprintf(fp, "Empresa	Descripcion Empresa	Legajo	Apellido Paterno	Apellido Materno	Nombre	RFC	CURP	Puesto	Convenio	Categoria	Sal.Diario	Sal.Diario Int.	Forma Pago	Cod CCosto	CCosto	Fecha de Alta	Fecha de Baja	IMSS	Fec. Nacimiento	Fec.Ant.Reconocida	Telefono	Domicilio	Provincia	Localidad	C.P.	Estado Civil	Sexo	Est.Cursados	Ubicacion	Registro Patronal	Tipo de Documento	Nro de Documento	Cliente	Razon Social	Objetivo	Descripción Objetivo\tRegimen\tH. Entrada\tH. Salida\tAlta Policial 1\tAlta Policial 2\tAlta Policial 3\tAlta Policial 4\tInscripto en Portacion\tFecha Venc\tInscripto en CLU\tFecha Venc\tEs Francero?\n");
}

static void CerrarSalida()
{

	if (!strcmp(FmSFld(fm0, SALIDA), "A"))
		fclose(fp);
	else
		CloseReport(rp0);
}

static void Proceso()
{

	if (!strcmp(FmSFld(fm0, SALIDA), "A"))
		ProcesoArchivo();
	else
		ProcesoReporte();
}

static void	ImprimirVigiladorArchivo()
{
	char altapol1[10], altapol2[10], altapol3[10], altapol4[10], _rfc[40], _imss [40], _curp[40], apema[40], apepa[40], _fpago[30], _estadocivil[20];
	bool clu = FALSE, port = FALSE;
	DATE fvenclu, fvenport;
	double sueldo_basico, ticket, suelad, pread, tickad;
	
	SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG));
	(void)GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK);

	SetKey(sue|DATPERSbyEMP, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG));
	(void)GetRecord(sue|DATPERSbyEMP, THIS_KEY, IO_NOT_LOCK);
	GetApellidos(SFld(sue|DATPERS_APELL), apepa, apema);

	fprintf(fp, "%d\t", FmIFld(fm0, EMP));
	fprintf(fp, "%s\t", FmSFld(fm0, DEMP));
	fprintf(fp, "%ld\t",  LFld(operac|ASIG_NROLEG));
	fprintf(fp, "%s\t%s\t%s\t",   apepa, apema, SFld(sue|DATPERS_NOMBRE));
	strcpy(_rfc,	NULL_STR);
	strcpy(_imss,	NULL_STR);
	strcpy(_curp,	NULL_STR);
	SetKey(mx|DATPERSMbyEMP, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG));

	if (GetRecord(mx|DATPERSMbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
  		strcpy(_rfc , SFld(mx|DATPERSM_RFC) );
	    strcpy(_imss, SFld(mx|DATPERSM_IMSS));
	    strcpy(_curp, SFld(mx|DATPERSM_CURP));
	}

	fprintf(fp, "%s\t",	_rfc);
	fprintf(fp, "%s\t",	_curp);
//	PerH = FALSE;

//    SetKey(suer|PERHbyEMP, FmIFld(fm0, EMP), NULL_LONG, NULL_LONG);
//    while (GetRecord(suer|PERHbyEMP, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 1) != ERROR) {
//
//    	if (LFld(sue|PER_NROLEG) == LFld(suer|PERH_NROLEG)) {
//    		PerH = TRUE;
//    	}   
//    	else
//    		continue;
//    }
    SetKey(sue|PUESTObyCOD, LFld(sue|PER_CODTAR) );
	(void) GetRecord(sue|PUESTObyCOD, THIS_KEY, IO_NOT_LOCK); // == ERROR))
//    	Warning("Puesto invalido : %ld\nLegajo          : %ld", PerH ? LFld(suer|PERH_CODTAR) : LFld(sue|PER_CODTAR), LFld(sue|PER_NROLEG));

    fprintf(fp, "%s\t",	SFld(sue|PUESTO_DET));
    
    SetKey(sue|CONVbyRELACION,IFld(sue|PER_RELACION) );
	GetRecord(sue|CONVbyRELACION, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%s\t", SFld(sue|CONV_DESCRIP));

	
	SetKey(sue|CATEbyCOD, IFld(sue|PER_RELACION)  , IFld(sue|PER_CODCAT));
	GetRecord(sue|CATEbyCOD, THIS_KEY, IO_NOT_LOCK); 
	fprintf(fp, "%s\t", SFld(sue|CATE_DENOM));
	(void) GetSBasico(GetConvenioPorEmp(FmIFld(fm0, EMP)), IFld(sue|PER_CODCAT), &sueldo_basico, &ticket, &suelad, &pread, &tickad);
    fprintf(fp, "%.2f\t", sueldo_basico / 100);
    fprintf(fp, "%.2f\t", ObtengoSalarioInt(FmIFld(fm0, EMP), LFld(sue|PER_NROLEG)) / 100);		
	    
    
	switch (IFld(sue|PER_FORMA) ) {      
    	case 1: strcpy(_fpago,  "Efectivo"        ); break;         
        case 2: strcpy(_fpago,  "Cheque"          ); break;         
        case 3: strcpy(_fpago,  "Caja de Ahorro"  ); break;         
        case 4: strcpy(_fpago,  "Cuenta Corriente"); break;         
        case 5: strcpy(_fpago,  "Transferencia"   ); break;         
        case 6: strcpy(_fpago,  "Giro"            ); break;         
        case 7: strcpy(_fpago,  "Disposicion"     ); break;         
        default : strcpy(_fpago,    NULL_STR);                      
	}                                                               

	fprintf(fp, "%s\t",  _fpago);
	
	SetKey(sue|CCOSTObyEMP, FmLFld(fm0, EMP), LFld(sue|PER_CODCCOS));
	GetRecord(sue|CCOSTObyEMP, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%ld\t", LFld(sue|PER_CODCCOS));
	fprintf(fp, "%s\t", SFld(sue|CCOSTO_DENOM));

	fprintf(fp, "%.3D\t", DFld(sue|PER_FECING));
	fprintf(fp, "%.3D\t", DFld(sue|PER_FECEGR));
	fprintf(fp, "%s\t", _imss);	
	fprintf(fp, "%.3D\t", DFld (sue|PER_FECNAC));
	fprintf(fp, "%.3D\t", DFld(sue|DATPERS_FECANT));
	fprintf(fp, "%s\t", SFld(sue|PER_TELEF, 0));
	fprintf(fp, "%s\t",  SFld(sue|PER_DIREC));
	SetKey (sue|PROVIbyPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV));
	if (GetRecord (sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		fprintf(fp, "%s\t",	SFld (sue|PROVI_DENOM));
	else
		fprintf(fp, "%s\t", NULL_STR);	

	SetKey (sue|LOCALIbyCODPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV), LFld (sue|PER_LOCAL));
	if (GetRecord (sue|LOCALIbyCODPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		
		fprintf(fp, "%s\t",	SFld(sue|LOCALI_DESCRIP));
	else
		fprintf(fp, "%s\t", NULL_STR);

	fprintf(fp, "%s\t", SFld(sue|PER_CODPOST));
	
	switch (IFld(sue|PER_ESTCIV)) {                                     
    	case 1: strcpy(_estadocivil, "Soltero"    ); break;         
        case 2: strcpy(_estadocivil, "Casado"     ); break;         
        case 3: strcpy(_estadocivil, "Viudo"      ); break;         
        case 4: strcpy(_estadocivil, "Separado"   ); break;         
		case 5: strcpy(_estadocivil, "Divorciado" ); break;         
        case 6: strcpy(_estadocivil, "Convivencia"); break;         
        default: strcpy(_estadocivil, NULL_STR); break;             
	}                                                               
	
	fprintf(fp, "%s\t", _estadocivil);
	
    if(IFld(sue|PER_SEXO) == 1)
		fprintf(fp, "%s\t", MASCULINO);
	else
		fprintf(fp, "%s\t", FEMENINO);
	
	SetKey(sue|ESTUDbyCOD, IFld(sue|PER_CODEST));
    (void)GetRecord(sue|ESTUDbyCOD, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%s\t", SFld(sue|ESTUD_DET));
	
	SetKey(mx|EMPSUBI, FmIFld(fm0, EMP), LFld(sue|PER_CODUBI)  );
	GetRecord(mx|EMPSUBI, THIS_KEY, IO_NOT_LOCK);
	fprintf(fp, "%ld\t",	LFld(mx|EMPSUBI_UBI));
	fprintf(fp, "%s\t", SFld(mx|EMPSUBI_REGPAT));
  	fprintf(fp, "%s\t",	 SFld(sue|PER_CODDOC, 0));
	fprintf(fp, "%ld\t", LFld(sue|PER_NRODOC, 0));  
	fprintf(fp, "%ld\t",	LFld(comerc|OBJETIVO_CLIENTE));
	fprintf(fp, "%s\t",		SFld(bill|CLIENTE_RAZSOC));
	fprintf(fp, "%d\t",		IFld(comerc|OBJETIVO_OBJET));
	fprintf(fp, "%s\t",		GetObjDescrip(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)));

	fprintf(fp, "%s\t",	SFld(operac|ASIG_REGIM));
	fprintf(fp, "%.3T\t",	TFld(operac|ASIG_HSENT));
	fprintf(fp, "%.3T\t",	TFld(operac|ASIG_HSSAL));

	AltaPolicia (FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG), altapol1, altapol2, altapol3, altapol4);
	fprintf(fp, "%s\t",	altapol1);
	fprintf(fp, "%s\t",	altapol2);
	fprintf(fp, "%s\t",	altapol3);
	fprintf(fp, "%s\t",	altapol4);

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
	
	if (!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		fprintf(fp, "%B\t",	TRUE);
	else
		fprintf(fp, "%B\t",	FALSE);
	
	fprintf(fp, "\n");	
}

void GetApellidos(char * Ap, char * ApPat, char *ApMat) 
{   
	int 	apellidos 			= 0,//usa
    		compuestos 			= 0,//usa
    		esp_alfinal 		= 0,//usa
    		countchars 			= 0,//usa
    		countespa 			= 0,//usa 
    		countprep			= 0,
			flag_espacioalfinal = 0,//usa
    		flagP				= 0,
    		dondepuede			= 0,//usa
    		dondeespa		[15],
    		dondeprepo		[15], 
			dnd_ape			[2],
    		//variables del for
    		c,d,e;               
    		
    		for (c=0;c<14;c++)
    		{
    			dondeespa[c]=0;
    			dondeprepo[c]=0;
    		}
    		
    		
	/*									
	    se puede encontrar un apellido compuesto

	    1) al principio
	       DELLA TORRE GOMEZ
	    
	    2)en el primer espacio
	      PEREZ DEL PRADO    
	     
	    3)hay casos especiales  
	      MONTES DE OCA
	      
	    4)y fuera de la realidad
	    	DEL MONTE DE LA TORRE  
										*/
		    apellidos 			= 0,
    		compuestos 			= 0,
    		esp_alfinal 		= 0,
    		countchars 			= 0,
    		countespa 			= 0, 
    		countprep			= 0,
			flag_espacioalfinal = 0,
    		flagP				= 0,
    		dondepuede			= 0;
 
 	
//					CUENTA LOS ESPACIOS			   
	for(countchars=0;Ap[countchars]!='\0';countchars++)
	{
		if (Ap[countchars]==' ') 
		{
       		dondeespa[countespa]=countchars;
			countespa++;
 		}
	} 
    Ap[countchars]='\0';                                        
//			BUSCA LAS PREPOSICIONES   
            
    for (c=0;c<countchars;c++)
    {
       	if (Ap[c]=='\0') break;       
       	if (prep(Ap,c,0)>=1)
       	{                     
       		compuestos++;      
       		countespa=countespa -(prep(Ap,c,0));
       		c+=(prep(Ap,c,1));  //avanza caracteres            		
       	}
    }
            
    /*	REONOCE DONDE ESTAN LOS APELLIDOS		*/
    apellidos = countespa+1;  
    if(apellidos>=2)
    {                              
    	flagP	=	0;
       	for (c=0;c<countchars;c++)   
       	{  
       	    //if (Ap[c]=='\0') break;
       		if (flagP==0)
       		{
    			if (prep(Ap,c,0)>=1)
         		{
         	    	flagP++;
                }
       		}
       		if ((prep(Ap,0,0)==0)&&(prep(Ap,c,0)>=1))
       		{
      	  		dnd_ape[0]=0;
       	  		dnd_ape[1]=c; //o c+1 porque c es donde está el espacio	     
       	  		
       	  		//WiMsg("%d",dnd_ape[1]);
       	  		break;
       		}
       		if ((prep(Ap,0,0))>=1)
       		{
       	  		c+=(prep(Ap,c,1));
       	        for(d=c+1;Ap[d]!=' ';d++);
       	  		dnd_ape[0]=0;
       	  		dnd_ape[1]=d; //o d+1 porque d es donde está el espacio	 
       	  		
       	  		//WiMsg("%d",dnd_ape[1]);
       	  		break;	    
       		}  
        }                              
        if (flagP==0)
       	{
        	dnd_ape[0]=0;   
        	for(c=0;Ap[c]!=' ';c++);
        	dnd_ape[1]=c;
        }
        
       	if (dnd_ape[1]>0)
       	{
       		for (e=dnd_ape[0];e<dnd_ape[1];e++) 
        	{                        
          		ApPat[e]=Ap[e];
          		ApPat[e+1]='\0';
          	}              
        	for (e=0;e+dnd_ape[1]<countchars;e++) 
        	{    
        		
           		ApMat[e]=Ap[e+dnd_ape[1]];     
           		ApMat[e+1]='\0';
        	}
        }   
        SacarEspacios(ApMat, ApMat);
    }                       
    
    
	if (apellidos==1)
	{
	     strcpy(ApPat,Ap);
	     strcpy(ApMat,"");
	}    
                             
    /* */ for(d=0;d<countchars;d++)  	Ap[d]='\0';  
}      

int prep(char *Ap, int dondepuede, int tiporetorno) 
{   
	
	int hayPREP,longitud;

	if ((dondepuede!=0)&&(Ap[dondepuede-1]!=' '))			//PRIMER CARACTER ESPACIO EN BLANCO
    {
       	hayPREP=0;	
   		longitud=0; 
   		if (tiporetorno==0)return hayPREP;
   		else return longitud;
    }
	if (Ap[dondepuede]=='D')  
	{   
		if (Ap[dondepuede+1]==' ')							//encuentra D  y solo D
		{
	    	hayPREP=1;	
	    	longitud=1; 
	    	if (tiporetorno==0)return hayPREP;
	    	else return longitud;
		}        
		if ((Ap[dondepuede+1]=='\'')&&(Ap[dondepuede+2]==' '))	// encuentra  D'
		{

	    	hayPREP=1;	
	    	longitud=2; 
	    	if (tiporetorno==0)return hayPREP;
	    	else return longitud;
	    } 
		if (Ap[dondepuede+1]=='E')
		{
			if (Ap[dondepuede+2]==' ') 
	   		{
	   			if (Ap[dondepuede+3]=='L')
	   			{
	   				if (Ap[dondepuede+4]=='A')	
	   				{	
	   				    if (Ap[dondepuede+5]==' ')				//encuentra DE LA
	   					{                                                   	
					    	hayPREP=2;	
	    					longitud=5; 
	    					if (tiporetorno==0)return hayPREP;
	    					else return longitud;
	        			}
	   				    if (Ap[dondepuede+5]=='S')
	   					{   
	   						if (Ap[dondepuede+6]==' ')			//encuentra DE LAS
	   						{
						    	hayPREP=2;	
	    						longitud=6; 
						    	if (tiporetorno==0)return hayPREP;
	    						else return longitud;
	        				}	
		        			else
		        			{
	    	    				hayPREP=0;	
   								longitud=0; 
   								if (tiporetorno==0)return hayPREP;
   								else return longitud;
	        				}
	        			}
	        		}	
	        		if (Ap[dondepuede+4]=='O')	
	   				{	
	   				    if (Ap[dondepuede+5]==' ')				//encuentra DE LO
	   					{
	    					hayPREP=2;	
	    					longitud=5; 
	    					if (tiporetorno==0)return hayPREP;
	    					else return longitud;
	        			} 
	   				    if (Ap[dondepuede+5]=='S')
	   					{   
	   						if (Ap[dondepuede+6]==' ')			//encuentra DE LOS
	   						{
						    	hayPREP=2;	
	    						longitud=6; 
	    						if (tiporetorno==0)return hayPREP;
	    						else return longitud;
	        				}
	        				else
	        				{
	        					hayPREP=0;	
   								longitud=0; 
   								if (tiporetorno==0)return hayPREP;
   								else return longitud;
	        				}	
	        			}
	        			else
	        			{
	        				hayPREP=0;	
   							longitud=0; 
   							if (tiporetorno==0)return hayPREP;
   							else return longitud;
	        			}
	        		}	
	        	}		
	    		hayPREP=1;	                                        //encuentra DE 
	    		longitud=2; 
	    		if (tiporetorno==0)return hayPREP;
	    		else return longitud; 
	   		}
	   		if (Ap[dondepuede+2]=='L') 
	   		{ 
	   			if (Ap[dondepuede+3]==' ')							//encuentra DEL 
	   			{
			    	hayPREP=1;	
	    			longitud=3; 
	    			if (tiporetorno==0)return hayPREP;
	    			else return longitud;
	   			} 
	   			if (Ap[dondepuede+3]=='L') 
	   			{
            		if (Ap[dondepuede+4]=='A') 
	   				{
                		if (Ap[dondepuede+5]==' ')						//encuentra DELLA 
	   					{
					    	hayPREP=1;	
	    					longitud=5; 
	    					if (tiporetorno==0)return hayPREP;
	    					else return longitud;
						} 
						else
						{
							hayPREP=0;	
   							longitud=0; 
   							if (tiporetorno==0)return hayPREP;
   							else return longitud;
						}
	   				}
	   				else
	   				{
	   					hayPREP=0;	 
   						longitud=0; 
   						if (tiporetorno==0)return hayPREP;
   						else return longitud;
	   				} 
	   			}
	   			else
	   			{
	   				hayPREP=0;	
   					longitud=0; 
   					if (tiporetorno==0)return hayPREP;
   					else return longitud;
	   			}    
	  		}
	  		else
	  		{
	  			hayPREP=0;	
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;
	  		} 
		}             
		if (Ap[dondepuede+1]=='A')
		{
			if (Ap[dondepuede+2]==' ')								//encuentra DA 
	   		{
	    		hayPREP=1;	
	    		longitud=2; 
	    		if (tiporetorno==0)return hayPREP;
	    		else return longitud;	   	
	    	}  
		   	if (Ap[dondepuede+2]=='L') 
		   	{ 
	   			if (Ap[dondepuede+3]=='L') 
	   			{
            		if (Ap[dondepuede+4]=='A') 
	   				{
                		if (Ap[dondepuede+5]==' ')					 //encuentra DALLA 
	   					{
				    		hayPREP=1;	
	    					longitud=5; 
	    					if (tiporetorno==0)return hayPREP;
	    					else return longitud;
	   					}	
	   				} 
	   			}    
	   		}
	   		else
	   		{ 
		   		hayPREP=0;	
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;   
   			}	
		}
		if (Ap[dondepuede+1]=='O')
		{
			if (Ap[dondepuede+2]==' ')							//encuentra DO
			{
		    	hayPREP=1;	
   				longitud=2; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud; 
		    }                               
		    if (Ap[dondepuede+2]=='S')							
			{
		        if (Ap[dondepuede+3]==' ')						//encuentra DOS
		       	{
		       		hayPREP=1;	
   					longitud=3; 
   					if (tiporetorno==0)return hayPREP;
   					else return longitud; 
   				}	
		    }                               
		}  
		if (Ap[dondepuede+1]=='I')
		{
			if (Ap[dondepuede+2]==' ')							//encuentra DI
			{
		    	hayPREP=1;	
   				longitud=2; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud; 
		    } 
		   	else
			{
	   			hayPREP=0;	
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;   
   			}                                   
		}  
	}
    if ((Ap[dondepuede]=='S')&&(Ap[dondepuede+1]=='A')&&(Ap[dondepuede+2]=='N')&&(Ap[dondepuede+3]==' ')) //encuentra SAN
    {                
    	hayPREP=1;
   		longitud=3; 
   		if (tiporetorno==0)return hayPREP;
   		else return longitud; 	
    } 
    if (Ap[dondepuede]=='A')
    {                                                    
    	if ((Ap[dondepuede+1]=='S') && (Ap[dondepuede+2]==' '))  // encuentra AS
    	{
    		hayPREP=1;
   			longitud=2; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud; 	
    	}
    	if ((Ap[dondepuede+1]=='L') && (Ap[dondepuede+2]==' '))  // encuentra AL
    	{   
    		hayPREP=1;
   			longitud=2; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud; 	
    	}
    	else
    	{   
    		hayPREP=0;
   			longitud=0; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud;  
    	}     
   		hayPREP=0;
		longitud=0; 
		if (tiporetorno==0)return hayPREP;
		else return longitud; 	 
    }           
    if (Ap[dondepuede+1]=='L')
    {
    	if ((Ap[dondepuede]=='E') && (Ap[dondepuede+2]==' '))  // encuentra EL
    	{
    		hayPREP=1;
   			longitud=2; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud; 	
    	}      
    	if ((Ap[dondepuede]=='I') && (Ap[dondepuede+2]==' '))  // encuentra IL
    	{
    		hayPREP=1;
   			longitud=2; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud; 	
    	}   
    	else
    	{   
    		hayPREP=0;
   			longitud=0; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud; 	
    	} 
    } 
    if (Ap[dondepuede]=='L')
    {
    	if (Ap[dondepuede+1]=='E') 
    	{  
    		if (Ap[dondepuede+2]==' ')  							// encuentra LE
    		{
    			hayPREP=1;
   				longitud=2; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud; 	
   			}
   			if ((Ap[dondepuede+2]=='S')&&(Ap[dondepuede+3]==' '))	// encuentra LES		 
   			{
   				hayPREP=1;
   				longitud=3; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}   
   			else
   			{  
   				hayPREP=0;
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}                                                                        
    	} 
    	if (Ap[dondepuede+1]=='A')  
    	{                                       
    		if (Ap[dondepuede+2]==' ') 							 // encuentra LA
    		{
    			hayPREP=1;
   				longitud=2; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}
   			if ((Ap[dondepuede+2]=='S')&& (Ap[dondepuede+3]==' '))	// encuentra LAS		 
   			{
   				hayPREP=1;
   				longitud=3; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}           
   			else
   			{
   				hayPREP=0;
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}
    	}   
    	if (Ap[dondepuede+1]=='U') //para salvar de un error que no sé porque ocurría 
    	{
   			hayPREP=0;
   			longitud=0; 
   			if (tiporetorno==0)return hayPREP;
   			else return longitud;  
   		}
    	if (Ap[dondepuede+1]=='O') 
    	{   
    		if	(Ap[dondepuede+2]==' ')  						// encuentra LO
    		{
    			hayPREP=1;
   				longitud=2; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud; 	
   			}	 
   			if ((Ap[dondepuede+2]=='S')&& (Ap[dondepuede+3]==' '))	// encuentra LOS		 
   			{
   				hayPREP=1;
   				longitud=3; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}        
   			else
			{	
 				hayPREP=0;
   				longitud=0; 
   				if (tiporetorno==0)return hayPREP;
   				else return longitud;  
   			}       			
    	}       
    }
    else   
    /*			Casos Particulares           */
    //1: MONTES DE ... + palabra
    if ((Ap[dondepuede]=='M')&& (Ap[dondepuede+1]=='O')&& (Ap[dondepuede+2]=='N')&& (Ap[dondepuede+3]=='T')&&
    (Ap[dondepuede+4]=='E')&&(Ap[dondepuede+5]=='S')&&(Ap[dondepuede+6]==' ')&&(Ap[dondepuede+7]=='D')&&
    (Ap[dondepuede+8]=='E')&&(Ap[dondepuede+9]==' '))
    {
    	hayPREP=2;  // serian dos preposiciones como un "DE LA" o "DE LOS" este es un "MONTES DE"
   		longitud=9; 
   		if (tiporetorno==0)return hayPREP;
   		else return longitud;  
    }
    else
    {
 		hayPREP=0;
   		longitud=0; 
   		if (tiporetorno==0)return hayPREP;
   		else return longitud;  
   	}
    return 0;  
}

void FixApellido(char *Entrada, char *Salida) 
{
  
  int a;
	for (a=0; a < strlen(Entrada); a++)
	{   
	    if (Entrada[a]=='\0') break;
	    Salida[a]	=	Entrada[a];
	    
		if (Entrada[a] == 'á') Salida[a] = 'A';
		if (Entrada[a] == 'Á') Salida[a] = 'A';
		if (Entrada[a] == 'é') Salida[a] = 'E';
		if (Entrada[a] == 'É') Salida[a] = 'E';
		if (Entrada[a] == 'í') Salida[a] = 'I';
		if (Entrada[a] == 'Í') Salida[a] = 'I';
		if (Entrada[a] == 'ó') Salida[a] = 'O';
		if (Entrada[a] == 'Ó') Salida[a] = 'O';
		if (Entrada[a] == 'ú') Salida[a] = 'U';
		if (Entrada[a] == 'Ú') Salida[a] = 'U';
		if (Entrada[a] == 'ñ') Salida[a] = 'N';
		if (Entrada[a] == 'Ñ') Salida[a] = 'N';    
		Salida[a+1]='\0';
	}             
}

void SacarEspacios (char *SEin, char *SEout) 
{       
	int a,b;   
	strcpy(SEout,SEin);
	if (SEin[strlen(SEin)-1]==' ')	SEout[strlen(SEout)-1]='\0';
	for(b=0;b<strlen(SEin);b++)
	{
		if (SEout[0]==' ')   
		{
			for(a=0;a < strlen (SEout)-1;a++)
			{
				if (SEout[a]=='\0')break;
				SEout[a]=SEout[a+1];  
			}   
			SEout[a]='\0';
		}
		else break;
   }		
}

static double ObtengoSalarioInt(int emp, long nroleg)
{
	find_mode modo	=	THIS_KEY;
	double valor	=	0.0;
	
	SetKey(sue|ACUMPERbyEMP, emp, nroleg);
	if (GetRecord(sue|ACUMPERbyEMP, modo, IO_NOT_LOCK) != ERROR) {
		
		if(IsNull(sue|ACUMPER_ACUM,2)) 
			valor  = 0.0;
		else
			valor += FFld(sue|ACUMPER_ACUM, 2);			
	}
	return valor;
}

