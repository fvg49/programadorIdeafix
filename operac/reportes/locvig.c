/********************************************************************
* MODULE & VERSION : @(#)locvig.c	1.2
* DATE             : 05/03/31
* TIME             : 11:12:12
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
#include "locvig.fmh"
#include "locvig.rph"
#include "locvig2.rph"
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

#define	_MAX_VIG		5000
struct t_vigil {
	long nroleg, nrodoc, locali, cliente;
	char apynom[50], tipdoc[5], calle[50], desclocal[50], descprov[50], telefono[50], franquero[5], regimen[10],
		 altapol1[5], altapol2[5], altapol3[5], altapol4[5], efect[20];
	DATE fecnac;
	TIME hent, hsal;
	bool porta, clu;
	short pais, provi, objetivo;
} pvig[_MAX_VIG], *uvig=pvig, *evig;

/* Funciones privadas */
static fm_status after (form, fmfield, int);
static fm_status before(form, fmfield, int);
static void AbrirSalida();
void AltaPolicia (short emp, long nroleg, char *altapol1, char *altapol2, char *altapol3, char *altapol4);
static void Imprimir();
bool LocalidadValida (short pais, short prov, long local);
bool AltaPolValida (char *altapol1, char *altapol2, char *altapol3, char *altapol4);
void InsertarNodo (char *altapol1, char *altapol2, char *altapol3, char *altapol4);
private int ordvig(struct t_vigil *a, struct t_vigil *b);
void ArmarLista();

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema operac, bill, comerc, sue, brigada, billpro;
bool salida;

/* Programa principal */
wcmd(locvig, 1.2 03/31/05)
{
	fm0    = OpenForm("locvig", FM_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	billpro   = OpenSchema("billpro",   IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	sue	   = OpenSchema("sue", 	IO_EABORT);
	brigada= OpenSchema("brigada", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	while(DoForm(fm0, before, after) != FM_EXIT) {
		ArmarLista();
		Imprimir ();
		FmSetFld(fm0, AVANCE, NULL_STR);
   }
}

static void Imprimir()
{

	if (pvig == uvig) {
		WiMsg ("No hay datos para mostrar");
		return;
	}
	
	qsort((char *)pvig, (unsigned)(uvig-pvig), sizeof(pvig[0]), (IFPVCPVCP)ordvig);

	FmSetFld(fm0, AVANCE, "Generando reporte");
	FmShowFlds(fm0, AVANCE, AVANCE);
	WiRefresh();
	
	AbrirSalida();
	for (evig=pvig; evig < uvig; evig ++) {
		SetLFld(bill|CLIENTE_CLIENTE, evig->cliente);
		GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
		RpSetLFld(rp0, RCLI,	evig->cliente);
		RpSetFld (rp0, RDCLI,	SFld(bill|CLIENTE_RAZSOC));
		RpSetIFld(rp0, ROBJ,	evig->objetivo);
		RpSetFld (rp0, RDOBJ,	GetObjDescrip(evig->cliente, evig->objetivo));
		RpSetLFld(rp0, RLEGAJO,	evig->nroleg);
		RpSetFld (rp0, RAPENOM,	evig->apynom);
		RpSetFld  (rp0, RTIPDOC,evig->tipdoc);
		RpSetLFld (rp0, RNRODOC,evig->nrodoc);
		RpSetFld  (rp0, RCALLE,	evig->calle);
		RpSetFld  (rp0, RLOCAL,	evig->desclocal);
		RpSetFld  (rp0, RPROVI,	evig->descprov);
		RpSetFld  (rp0, RTEL,	evig->telefono);
		RpSetFld (rp0, RFR,	evig->franquero);
		RpSetFld (rp0, REFEC,	evig->efect);
		RpSetFld (rp0, RREGIM,	evig->regimen);
		RpSetTFld(rp0, RHENT,   evig->hent);
		RpSetTFld(rp0, RHSAL,   evig->hsal);
		RpSetFld(rp0, RALTAP1,	evig->altapol1);
		RpSetFld(rp0, RALTAP2,	evig->altapol2);
		RpSetFld(rp0, RALTAP3,	evig->altapol3);
		RpSetFld(rp0, RALTAP4,	evig->altapol4);
		RpSetIFld (rp0, RCLU, 	evig->clu);
		RpSetIFld (rp0, RPORT,	evig->porta);
		DoReport (rp0, LINVIG);
		RpClearZone(rp0, LINVIG);
	}

	CloseReport(rp0);
}


static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		rp0 = OpenReport("locvig2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_FILE, FmSFld (fm0, ARCHIVO));
	}
	if (!strcmp(FmSFld(fm0, SALIDA), "T")) {
		rp0 = OpenReport("locvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
	}
	if (!strcmp(FmSFld(fm0, SALIDA), "I")) {
		rp0 = OpenReport("locvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
	}

	BeginReport(rp0, 1, NULL_STR);
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case ARCHIVO :
			break;
	}
	return FM_OK;
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
		case ARCHIVO :
			if (*FmSFld(fm, SALIDA) == 'A' && FmIsNull (fm, ARCHIVO)) {
				FmSetFld (fm, ARCHIVO, "locvig.txt");
			} 
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
	
	SetKey(brigada|VIGIPOLbyULTMOD ,TRUE, emp, nroleg, NULL_SHORT, NULL_SHORT);
	while (GetRecord(brigada|VIGIPOLbyULTMOD, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR) {
		if (!IFld(brigada|VIGIPOL_ACTIVO))
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

void ArmarLista()
{
	dbcursor c_per;
	char altapol1[5], altapol2[5], altapol3[5], altapol4[5];
	long cant_total, avance, count=0;

	uvig=pvig;

	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);
	SetCursorFrom(c_per, FmIFld(fm0, EMP), MIN_LONG);
	SetCursorTo  (c_per, FmIFld(fm0, EMP), MAX_LONG);
	cant_total = CountCursor(c_per);

	while (FetchCursor(c_per) != ERROR) {
		bool esta_asig = FALSE;

		avance = ++count * 100 / cant_total;
		if ((avance % 10) == 0) {
			char buff[64];
			sprintf(buff, "Procesando %3.3d %%", avance);
			FmSetFld(fm0, AVANCE, buff);
			FmShowFlds(fm0, AVANCE, AVANCE);
			WiRefresh();
		}

		if(!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)) )
			continue;

		if (!IFld(sue|PER_ACTIVO))
			continue;
			
		if (!LocalidadValida (IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV), LFld (sue|PER_LOCAL))) {
			continue;
		}

		AltaPolicia (FmIFld(fm0, EMP), LFld(sue|PER_NROLEG), altapol1, altapol2, altapol3, altapol4);

		if (!AltaPolValida (altapol1, altapol2, altapol3, altapol4)) {
			continue;
		}

		SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld (sue|PER_NROLEG));
		GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK);

		if (*FmSFld(fm0, INSCLU) == 'C' && !IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, INSCLU) == 'S' && IFld(brigada|CVIGIPOL_CLU))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'P' && !IFld(brigada|CVIGIPOL_PORTACION))
			continue;

		if (*FmSFld(fm0, PORTAC) == 'S' && IFld(brigada|CVIGIPOL_PORTACION))
			continue;
			
		SetKey(operac|ASIGbyNROLEG, FmIFld(fm0, EMP), LFld (sue|PER_NROLEG), MIN_LONG, MIN_SHORT);
		while (GetRecord (operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		
			esta_asig = TRUE;
			if (*FmSFld (fm0, PROVIS) != 'T' && !str_eq (FmSFld (fm0, PROVIS), SFld (operac|ASIG_EFECT)))
				continue;

			InsertarNodo (altapol1, altapol2, altapol3, altapol4);
		}
		
		if (!esta_asig) {
			// Si pidio solo efectivo o provisorio debe estar en asig
			if (*FmSFld (fm0, PROVIS) != 'T')
				continue;

			InitRecord (operac|ASIG);
			InsertarNodo (altapol1, altapol2, altapol3, altapol4);
		}		
	}
	
}

void InsertarNodo (char *altapol1, char *altapol2, char *altapol3, char *altapol4) 
{
	if (uvig == &pvig[_MAX_VIG])
		Error("Tabla interna saturada. Max %d", _MAX_VIG);

	uvig->nroleg = LFld (sue|PER_NROLEG);
	uvig->pais = IFld (sue|PER_CODPAIS);
	uvig->provi = IFld (sue|PER_PROV);
	uvig->locali = LFld (sue|PER_LOCAL);
	strcpy (uvig->desclocal, NULL_STR);
	strcpy (uvig->descprov, NULL_STR);

	SetKey (sue|PROVIbyPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV));
	if (GetRecord (sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strcpy (uvig->descprov, SFld (sue|PROVI_DENOM));

	SetKey (sue|LOCALIbyCODPAIS, IFld (sue|PER_CODPAIS), IFld (sue|PER_PROV), LFld (sue|PER_LOCAL));
	if (GetRecord (sue|LOCALIbyCODPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR)
		strcpy (uvig->desclocal, SFld (sue|LOCALI_DESCRIP));

	strcpy (uvig->apynom, SFld (sue|PER_APYNOM));
	strcpy (uvig->tipdoc, SFld (sue|PER_CODDOC, 0));
	uvig->nrodoc = LFld (sue|PER_NRODOC, 0);
	uvig->fecnac = DFld (sue|PER_FECNAC);
	strcpy (uvig->calle, SFld (sue|PER_DIREC));
	strcpy (uvig->telefono,	SFld (sue|PER_TELEF, 0));

	if (!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		strcpy (uvig->franquero, "F");
	else
		strcpy (uvig->franquero, NULL_STR);		

	if (!IsNull (operac|ASIG_EFECT))
		strcpy (uvig->efect, InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)));
	else
		strcpy (uvig->efect, NULL_STR);
	uvig->cliente = LFld(operac|ASIG_CLIENTE);	
	uvig->objetivo = IFld(operac|ASIG_OBJETIVO);	
	strcpy (uvig->regimen, SFld(operac|ASIG_REGIM));
	uvig->hent =  TFld(operac|ASIG_HSENT);
	uvig->hsal =  TFld(operac|ASIG_HSSAL);

	strcpy (uvig->altapol1, altapol1);
	strcpy (uvig->altapol2, altapol2);
	strcpy (uvig->altapol3, altapol3); 
	strcpy (uvig->altapol4, altapol4);
	
	SetKey(brigada|CVIGIPOLbyEMP, FmIFld(fm0, EMP), LFld(sue|PER_NROLEG));
	if (GetRecord(brigada|CVIGIPOLbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		uvig->clu = IFld(brigada|CVIGIPOL_CLU);
		uvig->porta = IFld(brigada|CVIGIPOL_PORTACION);
	}
	else {
		uvig->clu = FALSE;
		uvig->porta = FALSE;
	} 

	uvig ++;
}

bool LocalidadValida (short pais, short prov, long local)
{
	int fila;
	
	if (pais != FmIFld (fm0, CODPAIS))
		return FALSE;

	//Si dejo vacio el multi van todos
	if (FmIsNull (fm0, FPROVI, 0))
		return TRUE;
		
    for (fila=0; fila < FmFldLen (fm0, MULTIP);	fila++) {
    	if (FmIFld (fm0, FPROVI, fila) == prov && FmLFld (fm0, FLOCAL, fila) == local) 
    		return TRUE;
    }

   	return FALSE;   
}

bool AltaPolValida (char *altapol1, char *altapol2, char *altapol3, char *altapol4) 
{
	int fila;

	if (FmIsNull (fm0, PALTA, 0)) {
		return TRUE;
	}

    for (fila=0; fila < FmFldLen (fm0, MULTIPOL);	fila++) {
    	
    	if (FmIFld (fm0, PALTA, fila) == AERONAUTICA) {
			if (str_eq (altapol1, "A") ||
				str_eq (altapol2, "A") ||
				str_eq (altapol3, "A") ||
				str_eq (altapol4, "A"))
					return TRUE;
    	} 

    	if (FmIFld (fm0, PALTA, fila) == PREFECTURA) {
			if (str_eq (altapol1, "P") ||
				str_eq (altapol2, "P") ||
				str_eq (altapol3, "P") ||
				str_eq (altapol4, "P"))
					return TRUE;
    	} 
    	
		SetKey(billpro|PROVXDIVbyPORSUE, FmIFld (fm0, CODPAIS), FmIFld (fm0, PALTA, fila));
		if (GetRecord(billpro|PROVXDIVbyPORSUE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
			if (str_eq (altapol1, SFld (billpro|PROVXDIV_CODPROV)) ||
				str_eq (altapol2, SFld (billpro|PROVXDIV_CODPROV)) ||
				str_eq (altapol3, SFld (billpro|PROVXDIV_CODPROV)) ||
				str_eq (altapol4, SFld (billpro|PROVXDIV_CODPROV)))
					return TRUE;
		}
	}
	return FALSE;
}

private int ordvig(struct t_vigil *a, struct t_vigil *b)
{
	return	a->pais 	< b->pais ? -1 : a->pais > b->pais ? 1 :
			a->provi	< b->provi	? -1 : a->provi	> b->provi ? 1 :
			a->locali 	< b->locali	? -1 : a->locali> b->locali? 1 :
			a->nroleg 	< b->nroleg	? -1 : a->nroleg> b->nroleg? 1 :
//			strcmp(a->regimen, b->regimen) < 0 ? -1 : strcmp(a->regimen, b->regimen) > 0 ? -1 :
			0;
}




