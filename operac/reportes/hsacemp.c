/********************************************************************
*
* MODULE & VERSION : @(#)hsacemp.c	1.17 
* DATE             : 08/06/27 
* TIME             : 15:26:30 
*
* CREATED          : 22/02/99 Gloria
*
* DESCRIPTION:
*  	Listado de Control de Horas a Cargo de la Empresa por
*   Cliente-Objetivo-Puesto (Detalle Fecha y Legajo)
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "hsacemp.fmh"
#include "hsacemp.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "filial.h"

#define R_SEPAR	";"

#define Q_MOTIVOS	5
#define ERR_ARCHI	"No se pudo abrir el archivo de salida."

/* Estructuras */               
#define MAXMOT	20

struct  climot {
	int		motivo;
	int		hn;
	int		h50;
	int		h100;
};

struct cliente {
	long	cliente;
	int		objetivo;
	int		puesto;
	DATE	fecha;
	long	nroleg;
	struct	climot motexc[MAXMOT];
	int		umot;
	struct  cliente *next;
} *pcli=NULL, *ecli;

struct motivos {
	int 	codmot;
	char	descrip[50];
} pmot[MAXMOT], *umot = pmot, *emot;


/* Funciones privadas */
void	CargarMotCli(struct cliente *cli, int i, int motivo, 
					 int hn, int h50, int h100);
struct cliente * CargarEstructura(long cliente, int objetivo, int puesto, 
						 DATE fecha, long nroleg);

static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
int	GetIMotivo(int motivo);
void	CargarMotExc();
void	GenerarReporte();
void	AbrirReporte();
void	AbrirArchivo();
void	ImprimirCabecera();
void	ImprimirCabArch();
void	ImprimirReporte();
void 	ImprimirTitArch();
void 	ImprimirDetalle();
void 	ImprimirNoPrint();
void 	ImprimirDetArch();
void 	TotGen();
void 	TotCli();
void 	TotObj();
void 	TotPto();
void	TotFch();
void 	TitCli();
void 	TitObj();
void 	TitPto();
void	TitFch();
void 	AcumularTotalizadores();
void 	LimpiarAcumTot();
void 	LimpiarAcumCli();
void 	LimpiarAcumObj();
void 	LimpiarAcumPto();
void 	LimpiarAcumFch();
void 	ImprimirDetArchAssist();

/* Declaraciones globales */
form fm0;
report rp0;
schema comerc, operac, bill;

FILE *fp;

/* Totalizadores */
int	cs1hn, cs1h50, cs1h100,	cs2hn, cs2h50, cs2h100, cs3hn, cs3h50, cs3h100, 
 	cs4hn, cs4h50, cs4h100, cs5hn, cs5h50, cs5h100;
int	os1hn, os1h50, os1h100,	os2hn, os2h50, os2h100, os3hn, os3h50, os3h100, 
 	os4hn, os4h50, os4h100, os5hn, os5h50, os5h100;
int	ps1hn, ps1h50, ps1h100,	ps2hn, ps2h50, ps2h100, ps3hn, ps3h50, ps3h100, 
	ps4hn, ps4h50, ps4h100, ps5hn, ps5h50, ps5h100;
int	fs1hn, fs1h50, fs1h100, fs2hn, fs2h50, fs2h100, fs3hn, fs3h50, fs3h100, 
 	fs4hn, fs4h50, fs4h100, fs5hn, fs5h50, fs5h100;
int	ts1hn, ts1h50, ts1h100,	ts2hn, ts2h50, ts2h100, ts3hn, ts3h50, ts3h100, 
 	ts4hn, ts4h50, ts4h100, ts5hn, ts5h50, ts5h100;

long cliant;
int	 objant, ptoant;
DATE fchant;
char dcliant[80], dobjant[50], dptoant[50];

/* Programa principal */
wcmd(hsacemp, 1.17 06/27/08)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill", IO_EABORT);

	fm0 = OpenForm("hsacemp", FM_EABORT);
    
  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
    
	while (DoForm(fm0, before, after) != FM_UPDATE) return;

	GenerarReporte();
	if (*FmSFld(fm0, SALIDA) == 'A' || *FmSFld(fm0, SALIDA) == 'R') {
		AbrirArchivo();
		ImprimirReporte();
	}
	else {
		AbrirReporte();
		ImprimirReporte();
		EndReport(rp0);
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
    FinListaXusr();
}

void AbrirReporte()
{
    rp0 = OpenReport("hsacemp", RP_EABORT|RP_NOBEGIN);
   
    //Si la salida es Impresora
    if ( *FmSFld(fm0, SALIDA) == 'I') {
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );
    }
	//Si la salida es Terminal
    if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR );
    BeginReport(rp0, 1, NULL_STR);    
} 

// Abre el archivo ascii indicado en pantalla :)
void	AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
}

void GenerarReporte()
{
	int indMot = 0;
	dbcursor cexc, crexc;
	struct cliente * ptrcli;
	
	CargarMotExc();                                    

	cexc  = CreateCursor(EXCEPCIONbyEMP, IO_NOT_LOCK);
	crexc = CreateCursor(RETROEXCbyEMP,  IO_NOT_LOCK);

	SetCursorFrom(cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD), 
						MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	SetCursorTo  (cexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH), 
						MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	while (FetchCursor(cexc) != ERROR) {
		if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) != BRIGADA) ||
		   ( *FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)) == BRIGADA))
			continue;

		if (IFld(EXCEPCION_CONDIC) != ACARGO_EMP)
			continue;

		if (DFld(EXCEPCION_DIA) < FmDFld(fm0, FECHAD) || DFld(EXCEPCION_DIA) > FmDFld(fm0, FECHAH))
			continue;

	    //valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO)))
		       	continue;
		
		if (!ValidaFilial(LFld(EXCEPCION_CLIENTE), IFld(EXCEPCION_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
	    
	    indMot = GetIMotivo(IFld(EXCEPCION_MOTIVO));

	    if (indMot == NULL_SHORT)
	    	continue;

	    ptrcli = CargarEstructura(LFld(EXCEPCION_CLIENTE),
				 				  IFld(EXCEPCION_OBJETIVO),
								  IFld(EXCEPCION_PTOSER),
						 		  DFld(EXCEPCION_DIA),
						 		  LFld(EXCEPCION_NROLEG));
	    CargarMotCli(ptrcli, indMot, 
	    					 IFld(EXCEPCION_MOTIVO),
	                         IFld(EXCEPCION_HORAS), 
	    					 IFld(EXCEPCION_HS50),
							 IFld(EXCEPCION_HS100));
	}
	DeleteCursor(cexc);

	/*** Leo las excepciones de retroactivos ***/
	if (FmIFld(fm0, FRETRO)) {
		SetCursorFrom(crexc, FmIFld(fm0, EMP), FmLFld(fm0, CLID), FmIFld(fm0, OBJD), FmDFld(fm0, FECHAD),
							MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (crexc, FmIFld(fm0, EMP), FmLFld(fm0, CLIH), FmIFld(fm0, OBJH), FmDFld(fm0, FECHAH),
							MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		while (FetchCursor(crexc) != ERROR) {
			if ((*FmSFld(fm0, OBJBRI) == 'B' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) != BRIGADA) ||
			   ( *FmSFld(fm0, OBJBRI) == 'V' && GetServicioObj(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)) == BRIGADA))
				continue;

			if (IFld(RETROEXC_CONDIC) != ACARGO_EMP)
				continue;

			if (DFld(RETROEXC_DIA) < FmDFld(fm0, FECHAD) || DFld(RETROEXC_DIA) > FmDFld(fm0, FECHAH))
				continue;

		     //valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO)))
		       	continue;
		
			if (!ValidaFilial(LFld(RETROEXC_CLIENTE), IFld(RETROEXC_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;
		    
		    indMot = GetIMotivo(IFld(RETROEXC_MOTIVO));
    	
	    	if (indMot == NULL_SHORT)
	    		continue;

		    ptrcli = CargarEstructura(LFld(RETROEXC_CLIENTE),
					 				  IFld(RETROEXC_OBJETIVO),
									  IFld(RETROEXC_PTOSER),
							 		  DFld(RETROEXC_DIA),
							 		  LFld(RETROEXC_NROLEG));
		    CargarMotCli(ptrcli, indMot, 
		    					 IFld(RETROEXC_MOTIVO),
	    	                     IFld(RETROEXC_DHORAS), 
	    						 IFld(RETROEXC_DHS50),
								 IFld(RETROEXC_DHS100));
		}
	}
	DeleteCursor(crexc);
}
                               	
int	GetIMotivo(int motivo)
{
	int i;
	for (i = 0, emot = pmot ; emot < umot; emot++, i++) {
		if (emot->codmot == motivo)
			return i;
	}
	return NULL_SHORT;
}

void CargarMotExc()
{
	dbcursor cmotivo;
	int i = 0;

	emot = pmot;

	cmotivo = CreateCursor(MOTEXC, IO_NOT_LOCK);

	SetCursorFrom(cmotivo, ACARGO_EMP, MIN_SHORT);
	SetCursorTo  (cmotivo, ACARGO_EMP, MAX_SHORT);
	while (FetchCursor(cmotivo) != ERROR) {
		emot->codmot = IFld(MOTEXC_CODMOT);
		strcpy(emot->descrip, SFld(MOTEXC_DESCRIP));
		emot++;
		if (emot == &pmot[MAXMOT])
			Error("Tabla interna saturada");
		if (i == Q_MOTIVOS) {
			Warning("Existen codificados mas motivos que los soportados por el listado.\n			         Se imprimiran los primeros %d.", Q_MOTIVOS);
			break;
		}
		i++;
	}
	umot = emot;
	DeleteCursor(cmotivo);
}

struct cliente * CargarEstructura(long cliente, int objetivo, int puesto, DATE fecha, long nroleg)
{
	bool encontro = FALSE;
	struct cliente *ant, *aux, *pnew;

	if (pcli == NULL) {
		pcli = (struct cliente *) Alloc(sizeof(struct cliente));

		pcli->cliente  = cliente;
		pcli->objetivo = objetivo;
		pcli->puesto   = puesto;
		pcli->fecha    = fecha;
		pcli->nroleg   = nroleg;
		pcli->next     = NULL;
		return pcli;
	}
	for (ant = pcli, aux = pcli; aux != NULL; aux = aux->next) {
		if (aux->cliente > cliente ||
			(aux->cliente == cliente && aux->objetivo > objetivo) ||
			(aux->cliente == cliente && aux->objetivo == objetivo &&
										aux->puesto > puesto) ||
			(aux->cliente == cliente && aux->objetivo == objetivo &&
			 aux->puesto  == puesto  && aux->fecha > fecha) ||
			(aux->cliente == cliente && aux->objetivo == objetivo && 
			 aux->puesto  == puesto  && aux->fecha == fecha &&
			 aux->nroleg > nroleg))
			break;
		if (aux->cliente == cliente && aux->objetivo == objetivo && 
			 aux->puesto == puesto  && aux->fecha    == fecha &&
			 aux->nroleg == nroleg) {
			 encontro = TRUE;
			 break;
		}
		ant = aux;
	}
	if (encontro) {
		return aux;
	}
	pnew = (struct cliente *) Alloc(sizeof(struct cliente));

	pnew->cliente = cliente;
	pnew->objetivo= objetivo;
	pnew->puesto  = puesto;
	pnew->fecha   = fecha;
	pnew->nroleg  = nroleg;
	pnew->next    = NULL;
	pnew->umot    = 0;

	if (aux == pcli) {
		pcli       = pnew;
		pnew->next = aux;
		return pnew;
	}
	pnew->next = ant->next;
	ant->next  = pnew;

	return pnew;
}

void CargarMotCli(struct cliente *cli, int i, int motivo, int hn, int h50, int h100)
{
	cli->motexc[i].motivo  = motivo;
	cli->motexc[i].hn     += hn;
	cli->motexc[i].h50    += h50;
	cli->motexc[i].h100   += h100;

	if (cli->umot < i)
		cli->umot = i;
}

void ImprimirReporte()
{
	bool first = TRUE;

	cliant = NULL_LONG;
	objant = ptoant = NULL_SHORT;

	// Inicializaciones
	if (*FmSFld(fm0, SALIDA) != 'R') {
		if (*FmSFld(fm0, SALIDA) != 'A' )
			ImprimirCabecera();
		else {
			ImprimirCabArch();
			LimpiarAcumTot();
			LimpiarAcumCli();
			LimpiarAcumObj();
			LimpiarAcumPto();
			LimpiarAcumFch();
		}
	}

	// Recorrer la estructura e imprimir
	for (ecli = pcli ; ecli != NULL ; ecli = ecli->next) {
		if (cliant != ecli->cliente) {
			SetKey(bill|CLIENTEbyCLIENTE, ecli->cliente);
			if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
				SetFld(bill|CLIENTE_RAZSOC, "ERROR: Cliente Inexistente");
		}
		if (cliant != ecli->cliente || objant != ecli->objetivo) {
			SetKey(comerc|OBJETIVObyCLIENTE, ecli->cliente, ecli->objetivo);
			if (GetRecord(comerc|OBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
				SetFld(comerc|OBJETIVO_DESCRIP, "ERROR: Objetivo Inexistente");
		}
		if (ptoant != ecli->puesto) {
			SetKey(comerc|TPTOSERbyTIPPTO, ecli->puesto);
			if (GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) == ERROR)
				SetFld(comerc|TPTOSER_DESCRIP, "ERROR: Puesto Inexistente");
		}
		// Titulos
		if (*FmSFld(fm0, SALIDA) != 'R') {
			if (first) {
				first = FALSE;
				if (*FmSFld(fm0, SALIDA) != 'A') {
					RpSetLFld(rp0, RCLI,    ecli->cliente);
					RpSetFld (rp0, RDCLI,   SFld(bill|CLIENTE_RAZSOC));
					RpSetIFld(rp0, ROBJ,    ecli->objetivo);
					RpSetFld (rp0, RDOBJ,   SFld(comerc|OBJETIVO_DESCRIP));
					RpSetIFld(rp0, RPTO,    ecli->puesto);
					RpSetFld (rp0, RDPTO,   SFld(comerc|TPTOSER_DESCRIP));
					RpSetDFld(rp0, RFCH,    ecli->fecha);
					RpSetLFld(rp0, RNROLEG, ecli->nroleg);
				}
				switch (*FmSFld(fm0, DET)) {
				case 'L':
					TitCli();
					TitObj();
					TitPto();
					TitFch();
					break;
				case 'F':
					TitCli();
					TitObj();
					TitPto();
					break;
				case 'P':
					TitCli();
					TitObj();
					break;
				case 'O':
					TitCli();
					break;
				}
			}
			else {
				ImprimirTitArch();
			}   
				
			// Acumular totalizadores
			if (*FmSFld(fm0, SALIDA) != 'A')
				ImprimirNoPrint();
			else {
				int i;
				for (i = 0; i < ecli->umot + 1; i++)
					AcumularTotalizadores(i, ecli->motexc[i].hn, ecli->motexc[i].h50, ecli->motexc[i].h100);
			}
		}         
		
		// detalle de legajos
		if (*FmSFld(fm0, DET) == 'L') {
			if (*FmSFld(fm0, SALIDA) == 'A' )
				ImprimirDetArch();   
			else if(*FmSFld(fm0, SALIDA) == 'R')
				ImprimirDetArchAssist();   
			else
				ImprimirDetalle();				
		}
		cliant = ecli->cliente;
		strcpy(dcliant, SFld(bill|CLIENTE_RAZSOC));
		objant = ecli->objetivo;
		strcpy(dobjant, SFld(comerc|OBJETIVO_DESCRIP));
		ptoant = ecli->puesto;
		strcpy(dptoant, SFld(comerc|TPTOSER_DESCRIP));
		fchant = ecli->fecha;
	}

	// Totales
	if (*FmSFld(fm0, SALIDA) != 'R') {
		if (ecli != pcli)
			if (*FmSFld(fm0, SALIDA) != 'A') {
				RpSetLFld(rp0, RCLI,  cliant);
				RpSetFld (rp0, RDCLI, dcliant);
				RpSetIFld(rp0, ROBJ,  objant);
				RpSetFld (rp0, RDOBJ, dobjant);
				RpSetIFld(rp0, RPTO,  ptoant);
				RpSetFld (rp0, RDPTO, dptoant);
				RpSetDFld(rp0, RFCH,  fchant);
			}
			switch (*FmSFld(fm0, DET)) {
				case 'L':
				case 'F': 
					TotFch();
				case 'P': 
					TotPto();
				case 'O': 
					TotObj();
				case 'C':
					TotCli();
					TotGen();
					break;
			}
	}
}

void ImprimirCabecera()
{
	int i;

	RpSetDFld(rp0, RFECHAD,   FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH,   FmDFld(fm0, FECHAH));
	RpSetLFld(rp0, RCLID,     FmLFld(fm0, CLID));
	RpSetIFld(rp0, ROBJD,     FmIFld(fm0, OBJD));
	RpSetFld (rp0, RDCLIOBJD, FmSFld(fm0, DOBJD));
	RpSetLFld(rp0, RCLIH,     FmLFld(fm0, CLIH));
	RpSetIFld(rp0, ROBJH,     FmIFld(fm0, OBJH));
	RpSetFld (rp0, RDCLIOBJH, FmSFld(fm0, DOBJH));
	RpSetIFld(rp0, RRETRO,    FmIFld(fm0, FRETRO));

	for ( i = 0; i < MAXMOT && pmot[i].codmot != 0 ; i++) {
		switch(i) {
		case 0:
			RpSetFld (rp0, RDMOT1, pmot[i].descrip);
			break;
		case 1:
			RpSetFld (rp0, RDMOT2, pmot[i].descrip);
			break;
		case 2:
			RpSetFld (rp0, RDMOT3, pmot[i].descrip);
			break;
		case 3:
			RpSetFld (rp0, RDMOT4, pmot[i].descrip);
			break;
		case 4:
			RpSetFld (rp0, RDMOT5, pmot[i].descrip);
			break;
		}
	}
	if (pmot[i].codmot != 0)
		Warning("Existen codificados mas motivos que los soportados por el listado.\n			         Se imprimiran los primeros %d.", Q_MOTIVOS);
}

void ImprimirCabArch()
{
	int i;

	fprintf(fp, "Cliente\tObjetivo\tPuesto\tFecha\tLegajo\t");

	for ( i = 0; i < MAXMOT && pmot[i].codmot != 0 ; i++) {
		switch(i) {
		case 0:
			fprintf(fp, "%s\t\t\t\t", pmot[i].descrip);
			break;
		case 1:
			fprintf(fp, "%s\t\t\t\t", pmot[i].descrip);
			break;
		case 2:
			fprintf(fp, "%s\t\t\t\t", pmot[i].descrip);
			break;
		case 3:
			fprintf(fp, "%s\t\t\t\t",  pmot[i].descrip);
			break;
		case 4:
			fprintf(fp, "%s\n",  pmot[i].descrip);
			break;
		}
	}
	fprintf(fp, "\t\t\t\t\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\tHn\tH50\tH100\tTotal\n");
	if (pmot[i].codmot != 0)
		Warning("Existen codificados mas motivos que los soportados por el listado.\n			         Se imprimiran los primeros %d.", Q_MOTIVOS);
}

void ImprimirTitArch()
{
	if (cliant != ecli->cliente) {
		switch (*FmSFld(fm0, DET)) {
			case 'C':
				TotCli();
				if (*FmSFld(fm0, SALIDA)!='A') {
					RpSetLFld(rp0, RCLI, ecli->cliente);
					RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
				}
				break;
			case 'O':
				TotObj(); TotCli();
				if (*FmSFld(fm0, SALIDA)!='A') {
					RpSetIFld(rp0, ROBJ, ecli->objetivo);
					RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
				}
				TitCli();
				break;
			case 'P': 
				TotPto();	TotObj();	TotCli();
				TitCli();	TitObj();
				break;
			case 'F':
				TotFch();	TotPto();   TotObj();	TotCli();

				if (*FmSFld(fm0, SALIDA)!='A') 
					RpSetDFld(rp0, RFCH, ecli->fecha);

				TitCli();	TitObj();	TitPto();
				break;
			case 'L':
				TotFch();	TotPto();   TotObj();	TotCli();
				TitCli();	TitObj();	TitPto();	TitFch();
				break;
		}
		LimpiarAcumCli();
		LimpiarAcumObj();
		LimpiarAcumPto();
		LimpiarAcumFch();
	} 
	else {
		if (objant != ecli->objetivo) {
			switch (*FmSFld(fm0, DET)) {
				case 'O':
					TotObj();
					if (*FmSFld(fm0, SALIDA)!='A') {
						RpSetIFld(rp0, ROBJ, ecli->objetivo);
						RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
					}
					break;
				case 'P':
					TotPto();	TotObj();
					TitObj();
					break;
				case  'F':
					TotFch(); TotPto(); TotObj();

					if (*FmSFld(fm0, SALIDA)!='A') 
						RpSetDFld(rp0, RFCH, ecli->fecha);

					TitObj(); TitPto();
					break;
				case 'L':
					TotFch();	TotPto();   TotObj();
					TitObj();	TitPto(); 	TitFch();
					break;
			}
			LimpiarAcumObj();
			LimpiarAcumPto();
			LimpiarAcumFch();
		}
		else {
			if (ptoant != ecli->puesto) {
				switch (*FmSFld(fm0, DET)) {
				case 'P':
					TotPto();
					if (*FmSFld(fm0, SALIDA)!='A') {
						RpSetIFld(rp0, RPTO, ecli->puesto);
						RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
					}
					break;
				case 'F': 
					TotFch();

					if (*FmSFld(fm0, SALIDA)!='A') 
						RpSetDFld(rp0, RFCH, ecli->fecha);

					TotPto();
					TitPto();
					break;
				case 'L':
					TotFch();
					TotPto();
					TitPto();
					TitFch();
				}
				LimpiarAcumPto();
				LimpiarAcumFch();
			}
			else {
				if (fchant != ecli->fecha) {
					switch (*FmSFld(fm0, DET)) {
					case 'F':
						TotFch();
						if (*FmSFld(fm0, SALIDA)!='A') 
							RpSetDFld(rp0, RFCH, ecli->fecha);
						break;
					case 'L':
						TotFch();
						TitFch();
						break;
					}
					LimpiarAcumFch();
				}
			}
		}
	}
}

void ImprimirDetArch()
{	int i;

	fprintf(fp, "\t\t\t\t%ld\t", ecli->nroleg);

	for (i = 0; i < ecli->umot + 1; i++) {
		switch (i) {
		case 0:
			fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f", (double)ecli->motexc[i].hn   / 100,
													  (double)ecli->motexc[i].h50  / 100,
													  (double)ecli->motexc[i].h100 / 100,
													  (double)(ecli->motexc[i].hn + ecli->motexc[i].h50 +
													   ecli->motexc[i].h100) / 100);
			break;
		case 1:
			fprintf(fp, "\t%9.2f\t%9.2f\t%9.2f\t%9.2f", (double)ecli->motexc[i].hn   / 100,
														(double)ecli->motexc[i].h50  / 100,
														(double)ecli->motexc[i].h100 / 100,
														(double)(ecli->motexc[i].hn + ecli->motexc[i].h50 +
																 ecli->motexc[i].h100)  / 100);
			break;
		case 2:
			fprintf(fp, "\t%9.2f\t%9.2f\t%9.2f\t%9.2f",
										(double)ecli->motexc[i].hn   / 100,
										(double)ecli->motexc[i].h50  / 100,
										(double)ecli->motexc[i].h100 / 100,
										(double)(ecli->motexc[i].hn + ecli->motexc[i].h50 +
												 ecli->motexc[i].h100) / 100);
			break;
		case 3:
			fprintf(fp, "\t%9.2f\t%9.2f\t%9.2f\t%9.2f",
										(double)ecli->motexc[i].hn   / 100,
										(double)ecli->motexc[i].h50  / 100,
										(double)ecli->motexc[i].h100 / 100,
										(double)(ecli->motexc[i].hn + ecli->motexc[i].h50 +
												 ecli->motexc[i].h100) / 100);
			break;
		case 4:
				fprintf(fp, "\t%9.2f\t%9.2f\t%9.2f\t%9.2f",
										(double)ecli->motexc[i].hn   / 100,
										(double)ecli->motexc[i].h50  / 100,
										(double)ecli->motexc[i].h100 / 100,
										(double)(ecli->motexc[i].hn + ecli->motexc[i].h50 + 
												 ecli->motexc[i].h100) / 100);
			break;
		}
//		AcumularTotalizadores(i,
//							  ecli->motexc[i].hn,
//							  ecli->motexc[i].h50,
//							  ecli->motexc[i].h100);
	}
	fprintf(fp, "\n");
}

void ImprimirDetalle()
{
	int i;

	RpClearZone(rp0, ZLEG);
	RpSetLFld(rp0, RNROLEG, ecli->nroleg);

	for (i = 0; i < ecli->umot + 1; i++) {
		switch (i) {
		case 0:
			RpSetIFld(rp0, FS1HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, FS1H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, FS1H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, FS1TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		case 1:
			RpSetIFld(rp0, FS2HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, FS2H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, FS2H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, FS2TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
	    case 2:
			RpSetIFld(rp0, FS3HN, 	ecli->motexc[i].hn);
			RpSetIFld(rp0, FS3H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, FS3H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, FS3TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
	    	break;
		case 3:
			RpSetIFld(rp0, FS4HN, 	ecli->motexc[i].hn);
			RpSetIFld(rp0, FS4H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, FS4H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, FS4TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		case 4:
			RpSetIFld(rp0, FS5HN, 	ecli->motexc[i].hn);
			RpSetIFld(rp0, FS5H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, FS5H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, FS5TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		}
	}
	DoReport(rp0, ZLEG);
}

void	ImprimirNoPrint()
{
	int i;
	int totlin = 0;

	RpClearZone(rp0, ZNOPRINT);	

	for (i = 0; i < ecli->umot + 1; i++) {
		totlin += ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100;
		switch (i) {
		case 0:
			RpSetIFld(rp0, S1HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, S1H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, S1H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, S1TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		case 1:
			RpSetIFld(rp0, S2HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, S2H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, S2H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, S2TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
	    case 2:
			RpSetIFld(rp0, S3HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, S3H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, S3H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, S3TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
	    	break;
		case 3:
			RpSetIFld(rp0, S4HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, S4H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, S4H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, S4TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		case 4:
			RpSetIFld(rp0, S5HN,   ecli->motexc[i].hn);
			RpSetIFld(rp0, S5H50,  ecli->motexc[i].h50);
			RpSetIFld(rp0, S5H100, ecli->motexc[i].h100);
			RpSetIFld(rp0, S5TOT,  ecli->motexc[i].hn + ecli->motexc[i].h50 + ecli->motexc[i].h100);
			break;
		}
	}
	RpSetIFld(rp0, STOTAL, totlin);
	DoReport(rp0, ZNOPRINT);
}

// Imprime el total de un cliente en el archivo de salida
void	TotCli()
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Cliente %8.8ld %20s\t\t\t\t\t", cliant, dcliant);
		fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\n",
							(double)cs1hn / 100, (double)cs1h50 / 100, (double)cs1h100 / 100,
							(double)(cs1hn + cs1h50 + cs1h100) / 100,
							(double)cs2hn / 100, (double)cs2h50 / 100, (double)cs2h100 / 100,
							(double)(cs2hn + cs2h50 + cs2h100) / 100,
							(double)cs3hn / 100, (double)cs3h50 / 100, (double)cs3h100 / 100,
							(double)(cs3hn + cs3h50 + cs3h100) / 100, 
							(double)cs4hn / 100, (double)cs4h50 / 100, (double)cs4h100 / 100,
							(double)(cs4hn + cs4h50 + cs4h100) / 100, 
							(double)cs5hn / 100, (double)cs5h50 / 100, (double)cs5h100 / 100,
							(double)(cs5hn + cs5h50 + cs5h100) / 100);
	 }
	 else
	 	DoReport(rp0, ZTOTCLI);
}

// Imprime el total de un objetivo ven el archivo de salida
void	TotObj() 
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Objetivo %4.4d %20s\t\t\t\t\t", objant, dobjant);
		fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\n",
							(double)os1hn / 100, (double)os1h50 / 100, (double)os1h100 / 100,
							(double)(os1hn + os1h50 + os1h100) / 100,
							(double)os2hn / 100, (double)os2h50 / 100, (double)os2h100 / 100,
							(double)(os2hn + os2h50 + os2h100) / 100,
							(double)os3hn / 100, (double)os3h50 / 100, (double)os3h100 / 100,
							(double)(os3hn + os3h50 + os3h100) / 100,
							(double)os4hn / 100, (double)os4h50 / 100, (double)os4h100 / 100,
							(double)(os4hn + os4h50 + os4h100) / 100,
							(double)os5hn / 100, (double)os5h50 / 100, (double)os5h100 / 100,
							(double)(os5hn + os5h50 + os5h100) / 100);
	}
	else 
		DoReport(rp0, ZTOTOBJ);
}

// Imprime el total de puesto en el archivo de salida
void	TotPto() 
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Puesto %4.4d %20s\t\t\t\t\t", ptoant,  dptoant);
		fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\n",
							(double)ps1hn / 100, (double)ps1h50 / 100, (double)ps1h100 / 100,
							(double)(ps1hn + ps1h50 + ps1h100) / 100,
							(double)ps2hn / 100, (double)ps2h50 / 100, (double)ps2h100 / 100,
							(double)(ps2hn + ps2h50 + ps2h100) / 100,
							(double)ps3hn / 100, (double)ps3h50 / 100, (double)ps3h100 / 100,
							(double)(ps3hn + ps3h50 + ps3h100) / 100,
							(double)ps4hn / 100, (double)ps4h50 / 100, (double)ps4h100 / 100,
							(double)(ps4hn + ps4h50 + ps4h100) / 100,
							(double)ps5hn / 100, (double)ps5h50 / 100, (double)ps5h100 / 100,
							(double)(ps5hn + ps5h50 + ps5h100) / 100);
	}
	else 
		DoReport(rp0, ZTOTPTO);
}

// Imprime el total de la fecha en el archivo de salida
void	TotFch() 
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "Total Fecha %.1D \t\t\t\t\t", fchant);
		fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\n",
							(double)fs1hn / 100, (double)fs1h50 / 100, (double)fs1h100 / 100,
							(double)(fs1hn + fs1h50 + fs1h100) / 100,
							(double)fs2hn / 100, (double)fs2h50 / 100, (double)fs2h100 / 100,
							(double)(fs2hn + fs2h50 + fs2h100) / 100,
							(double)fs3hn / 100, (double)fs3h50 / 100, (double)fs3h100 / 100,
							(double)(fs3hn + fs3h50 + fs3h100) / 100,
							(double)fs4hn / 100, (double)fs4h50 / 100, (double)fs4h100 / 100,
							(double)(fs4hn + fs4h50 + fs4h100) / 100,
							(double)fs5hn / 100, (double)fs5h50 / 100, (double)fs5h100 / 100,
							(double)(fs5hn + fs5h50 + fs5h100) / 100);
	}
	else
		DoReport(rp0, ZTOTFCH);
}


// Imprime el total de puesto en el archivo de salida
void	TotGen() 
{
	if (*FmSFld(fm0, SALIDA) == 'A') {

		fprintf(fp, "Total GENERAL\t\t\t\t\t" );
		fprintf(fp, "%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\t%9.2f\n",
							(double)ts1hn / 100, (double)ts1h50 / 100, (double)ts1h100 / 100,
							(double)(ts1hn + ts1h50 + ts1h100) / 100,
							(double)ts2hn / 100, (double)ts2h50 / 100, (double)ts2h100 / 100, 
							(double)(ts2hn + ts2h50 + ts2h100) / 100,
							(double)ts3hn / 100, (double)ts3h50 / 100, (double)ts3h100 / 100, 
							(double)(ts3hn + ts3h50 + ts3h100) / 100, 
							(double)ts4hn / 100, (double)ts4h50 / 100, (double)ts4h100 / 100, 
							(double)(ts4hn + ts4h50 + ts4h100) / 100, 
							(double)ts5hn / 100, (double)ts5h50 / 100, (double)ts5h100 / 100,
							(double)(ts5hn + ts5h50 + ts5h100) / 100);
	}
	else
		DoReport(rp0, ZTOTGEN);
}

// Imprime el titulo de Cliente en el archivo de salida
void	TitCli()
{
	char dcli[50];

	strcpy(dcli, SFld(bill|CLIENTE_RAZSOC));

	if (*FmSFld(fm0, SALIDA) == 'A') {
		fprintf(fp, "\nCliente %ld %s\n", ecli->cliente, dcli);
	}
	else {
	    RpSetLFld(rp0, RCLI,  ecli->cliente);
		RpSetFld (rp0, RDCLI, SFld(bill|CLIENTE_RAZSOC));
		DoReport(rp0, ZCLI);
	}
}

// Imprime el titulo de objetivo en el archivo de salida
void	TitObj()
{
	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "\tObjetivo %4.4d %20s\n", 
				ecli->objetivo, SFld(comerc|OBJETIVO_DESCRIP));
	else {
      	RpSetIFld(rp0, ROBJ,  ecli->objetivo);		
	    RpSetFld (rp0, RDOBJ, SFld(comerc|OBJETIVO_DESCRIP));
     	DoReport(rp0, ZOBJ);	
    }
}

// Imprime el titulo de puesto en el archivo de salida
void	TitPto()
{
	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "\t\tPuesto %4.4d %20s\n", 
				ecli->puesto, SFld(comerc|TPTOSER_DESCRIP));
	else {
   	    RpSetIFld(rp0, RPTO,  ecli->puesto);
   	   	RpSetFld (rp0, RDPTO, SFld(comerc|TPTOSER_DESCRIP));
        DoReport(rp0, ZPTO);
    }
}

// Imprime el titulo de la fecha en el archivo de salida
void	TitFch()
{
	if (*FmSFld(fm0, SALIDA) == 'A')
		fprintf(fp, "\t\t\t%.1D \n", ecli->fecha);
	else {
   	    RpSetDFld(rp0, RFCH,  ecli->fecha);
        DoReport(rp0, ZFCH);
    }
}

// Acumula los totalizadores de cliente-objtivo-puesto y gnerales con
// datos tomados del vector ecli en la posicion corrinente que es la que
// se esta tratando
void	AcumularTotalizadores(int subind, int hn, int h50, int h100)
{
    switch(subind) {
		case 0:
			cs1hn   += hn;		ts1hn	+= hn;		os1hn += hn;
			ps1hn   += hn;		fs1hn	+= hn; 
			cs1h50  += h50;		ts1h50	+= h50;		os1h50 += h50;
			ps1h50  += h50;		fs1h50	+= h50; 
			cs1h100 += h100;	ts1h100	+= h100;    os1h100 += h100;
			ps1h100 += h100;	fs1h100	+= h100; 
			break;
		case 1:
			cs2hn   += hn;		ts2hn	+= hn;		os2hn += hn;
			ps2hn   += hn;		fs2hn	+= hn; 
			cs2h50  += h50;		ts2h50	+= h50;		os2h50 += h50;
			ps2h50  += h50;		fs2h50	+= h50; 
			cs2h100 += h100;	ts2h100	+= h100;    os2h100 += h100;
			ps2h100 += h100;	fs2h100	+= h100; 
			break;
		case 2:
			cs3hn   += hn;		ts3hn	+= hn;		os3hn += hn;
			ps3hn   += hn;		fs3hn	+= hn; 
			cs3h50  += h50;		ts3h50	+= h50;		os3h50 += h50;
			ps3h50  += h50;		fs3h50	+= h50; 
			cs3h100 += h100;	ts3h100	+= h100;    os3h100 += h100;
			ps3h100 += h100;	fs3h100	+= h100; 
			break;
		case 3:
			cs4hn   += hn;		ts4hn	+= hn;		os4hn += hn;
			ps4hn   += hn;		fs4hn	+= hn; 
			cs4h50  += h50;		ts4h50	+= h50;		os4h50 += h50;
			ps4h50  += h50;		fs4h50	+= h50; 
			cs4h100 += h100;	ts4h100	+= h100;    os4h100 += h100;
			ps4h100 += h100;	fs4h100	+= h100; 
			break;
		case 4:
			cs5hn   += hn;		ts5hn	+= hn;		os5hn += hn;
			ps5hn   += hn;		fs5hn	+= hn; 
			cs5h50  += h50;		ts5h50	+= h50;		os5h50 += h50;
			ps5h50  += h50;		fs5h50	+= h50; 
			cs5h100 += h100;	ts5h100	+= h100;    os5h100 += h100;
			ps5h100 += h100;	fs5h100	+= h100; 
			break;
	} 
}

//Limpia los acumuladores de cliente/totales/puesto y objetivos
void 	LimpiarAcumTot()
{
	ts1hn = ts1h50 = ts1h100 =	ts2hn = ts2h50 = ts2h100 = 0;
	ts3hn = ts3h50 = ts3h100 = 0;
 	ts4hn = ts4h50 = ts4h100 = ts5hn = ts5h50 = ts5h100=0;
}

void	LimpiarAcumCli()
{
	cs1hn = cs1h50 = cs1h100 =	cs2hn = cs2h50 = cs2h100 =0;
	cs3hn = cs3h50 = cs3h100 = 0;
 	cs4hn = cs4h50 = cs4h100 = cs5hn = cs5h50 = cs5h100=0;
}

void	LimpiarAcumObj()
{
	os1hn = os1h50 = os1h100 =	os2hn = os2h50 = os2h100 =0;
	os3hn = os3h50 = os3h100 = 0;
 	os4hn = os4h50 = os4h100 = os5hn = os5h50 = os5h100=0;
}

void	LimpiarAcumPto()
{
	ps1hn = ps1h50 = ps1h100 =	ps2hn = ps2h50 = ps2h100 =0;
	ps3hn = ps3h50 = ps3h100 = 0;
	ps4hn = ps4h50 = ps4h100 = ps5hn = ps5h50 = ps5h100=0;
}

void	LimpiarAcumFch()
{
 	fs1hn = fs1h50 = fs1h100 = fs2hn = fs2h50 = fs2h100 = 0;
 	fs3hn = fs3h50 = fs3h100 = 0;
 	fs4hn = fs4h50 = fs4h100 = fs5hn = fs5h50 = fs5h100=0;
}

void ImprimirDetArchAssist()
{	int i;

	fprintf(fp, "%ld%s%s%s", ecli->cliente, R_SEPAR, GetDescCli(ecli->cliente), R_SEPAR);
	fprintf(fp, "%d%s%s%s", ecli->objetivo, R_SEPAR, GetObjDescrip(ecli->cliente, ecli->objetivo), R_SEPAR);
	fprintf(fp, "%d%s%s%s", ecli->puesto, R_SEPAR,   GetDescPto(ecli->puesto), R_SEPAR);
	fprintf(fp, "%.3D%s", ecli->fecha, R_SEPAR);
	fprintf(fp, "%ld%s%s%s", ecli->nroleg, R_SEPAR, GetNombreLeg(1, ecli->nroleg), R_SEPAR);

	for (i = 0; i < 5; i++) {	// son 5 columnas

		if(i<ecli->umot + 1)
	 		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s",	(double)ecli->motexc[i].hn   / 100, 	R_SEPAR,
													  	(double)ecli->motexc[i].h50  / 100, 	R_SEPAR,
													  	(double)ecli->motexc[i].h100 / 100,   	R_SEPAR,
													   	(double)(ecli->motexc[i].hn + ecli->motexc[i].h50 +
															ecli->motexc[i].h100) / 100,			R_SEPAR );
		else
	 		fprintf(fp, "%9.2f%s%9.2f%s%9.2f%s%9.2f%s", (double)0, 	R_SEPAR,
	 												    (double)0, 	R_SEPAR,
														(double)0, 	R_SEPAR,
														(double)0, 	R_SEPAR );

	}
	fprintf(fp, "\n");
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
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

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
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

