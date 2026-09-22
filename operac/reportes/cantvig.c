/********************************************************************
*
* MODULE & VERSION : @(#)cantvig.c	1.13 
* DATE             : 08/06/30 
* TIME             : 11:36:20 
*
* CREATED          : 05/03/2001
*
* DESCRIPTION:
*	Muestra por cliente-objetivo la cantidad de vigiladores
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "billpro.h"
#include "cantvig.fmh"
#include "cantvig.rph"
#include "cantvig2.rph"
#include "sue.sch"
#include "comerc.sch"
#include "filial.h"

#define MAXPUE	10000
#define R_SEPAR	";"

/* Declaraciones de Estructuras */
struct puestos{
	short  ptoser,  puesto,  hspt;
	char frec[2];
	TIME hinicio;
	TIME hfinal;
	char dia1[2];
	char dia2[2];
	char dia3[2];
	char dia4[2];
	char dia5[2];
	char dia6[2];
	char dia7[2];
	char regim[20];
	char subreg[20];
	char tipodia[3];
	long salar, cliente, cvig;
	short  cpue, objetivo;
}pripuesto[MAXPUE], *upuesto=pripuesto, *epuesto;


/* Funciones privadas */
void ObtenerPuesto();
private fm_status before(form, fmfield, int);
private fm_status after (form, fmfield, int);

void GenerarReporte ();
void ImprimirReporte ();
private int comppue(struct puestos *a, struct puestos *b);
private void AbrirReporte();

static void AbrirArchivo();
static void ImprimirArchivo();
static void GenerarArchivo();

/* Declaraciones globales */
form fm0;
schema comerc;
report rp0;
struct spuesto estpue;
FILE *fp;

/* Programa principal */
wcmd(cantvig, 1.13 06/30/08)
{
	fm_cmd cmd;

	fm0 	= OpenForm("cantvig", FM_EABORT);
	comerc	= OpenSchema("comerc",  IO_EABORT);

  	FmSetFld(fm0, COMENTARIO, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENTARIO, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction ();
		GenerarReporte ();
		EndTransaction ();
		break;
	case FM_IGNORE:
		break;
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

private fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
		case ARCHIVO: 
			if ((*FmSFld(fm0, SALIDA)=='A' || *FmSFld(fm0, SALIDA)=='R' || *FmSFld(fm0, SALIDA)=='D') &&
				FmIsNull(fm0, ARCHIVO))
				FmSetFld(fm0, ARCHIVO, "cantvig.txt");
		break;
	 	case FECHA:
			if (FmIFld (fm, FECINI) && FmIsNull(fm, fno))
				FmSetDFld (fm, FECHA, Today());
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
private fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
		case EMP:
		    if (FmChgFld(fm))
    	    	InicListaXusr(FmIFld(fm0, EMP));
        break;
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
				FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIH, row)),row);
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
				FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIH, row) ,FmIFld(fm, OBJH, row)), row);
		break;
		case FFILIAL:
			if (FmKeyCode(fm) == K_HELP)
				HelpFilial(fm, fno, row);
		break;
	}
	return FM_OK;
}

void ImprimirReporte ()
{

	if (upuesto == pripuesto)
		return;
		
	qsort((char *)pripuesto, (unsigned)(upuesto-pripuesto), sizeof(pripuesto[0]), (IFPVCPVCP)comppue);

	for (epuesto=pripuesto; epuesto < upuesto ; epuesto ++) {

		if (!epuesto->cpue)
			continue;

//		RpClearZone	(rp0, LINVIG);
		RpSetLFld	(rp0, RCLIE,	epuesto->cliente);
		RpSetFld	(rp0, RDCLIE,  	GetDescCli(epuesto->cliente));
		RpSetIFld	(rp0, ROBJ,		epuesto->objetivo);
		RpSetFld	(rp0, RDOBJ,  	GetObjDescrip(epuesto->cliente, epuesto->objetivo));
		RpSetLFld	(rp0, RCANTVIG,	epuesto->cvig);

		DoReport (rp0, LINVIG);		
	}     
	
}

void ObtenerPuesto()
{
	dbcursor c_OBJ;
    _SParam_PVivo parhora;

	upuesto=pripuesto;

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
		if (FmIFld(fm0, OBJCONT) && IFld(comerc|OBJETIVO_SUBCON))
			continue;

		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		       	continue;
		
		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		InicPuestosVivos (FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), 
						NULL_SHORT, //Todos los puestos
						NULL_SHORT, //Todos los modelos de Ot
						!FmIFld(fm0, FECINI) ? Today() : FmDFld (fm0,FECHA),
						!FmIFld(fm0, FECINI) ? Today() : FmDFld (fm0,FECHA),
						FALSE, //Que no cargue tarifario
						FALSE, //Que no cargue bonos
						FmIFld(fm0, FECINI) ? _VALIDAR_FECINI : _VALIDAR_NOFEC,	//Considera fecha de inicio de la ot
						NULL_SHORT, // Todo tipo de OT
						FmIFld(fm0, OTAPROB),
						FALSE,
						parhora);

		while (ProximoPuestoVivo (&estpue)) {

			if (!estpue.cpue)
				continue;

			upuesto->cliente  =  LFld(comerc|OBJETIVO_CLIENTE);
			upuesto->objetivo =  IFld(comerc|OBJETIVO_OBJET);
			upuesto->ptoser  = estpue.ptoser;
			upuesto->puesto  = estpue.puesto;
			upuesto->hinicio = estpue.hinicio;
			upuesto->hfinal  = estpue.hfinal;
			upuesto->hspt    = estpue.hspt == NULL_SHORT ? 0 : estpue.hspt;
//			upuesto->hsnorm  = estpue.hs_norm_ot;
//			upuesto->hs50    = estpue.hs_50_ot;
//			upuesto->hs100   = estpue.hs_100_ot;
			strcpy (upuesto->tipodia, estpue.tipodia);

			strcpy (upuesto->frec, NULL_STR);
			strcpy (upuesto->dia1, NULL_STR);
			strcpy (upuesto->dia2, NULL_STR);
			strcpy (upuesto->dia3, NULL_STR);
			strcpy (upuesto->dia4, NULL_STR);
			strcpy (upuesto->dia5, NULL_STR);
			strcpy (upuesto->dia6, NULL_STR);
			strcpy (upuesto->dia7, NULL_STR);
			strcpy (upuesto->regim, NULL_STR);
			strcpy (upuesto->subreg, NULL_STR);

			if(strcmp(estpue.frec,NULL_STR))
				sprintf(upuesto->frec,"%1s",estpue.frec);
			if(strcmp(estpue.dia1,NULL_STR))
				sprintf(upuesto->dia1,"%1s",estpue.dia1);
			if(strcmp(estpue.dia2,NULL_STR))
				sprintf(upuesto->dia2,"%1s",estpue.dia2);
			if(strcmp(estpue.dia3,NULL_STR))
		  		sprintf(upuesto->dia3,"%1s",estpue.dia3);
			if(strcmp(estpue.dia4,NULL_STR))
				sprintf(upuesto->dia4,"%1s",estpue.dia4);
			if(strcmp(estpue.dia5,NULL_STR))
				sprintf(upuesto->dia5,"%1s",estpue.dia5);
			if(strcmp(estpue.dia6,NULL_STR))
				sprintf(upuesto->dia6,"%1s",estpue.dia6);
			if(strcmp(estpue.dia7,NULL_STR))
				sprintf(upuesto->dia7,"%1s",estpue.dia7);

			strcpy(upuesto->regim, estpue.regim);
			strcpy(upuesto->subreg, estpue.subreg);

			upuesto->salar  = estpue.salar;
			upuesto->cpue = estpue.cpue;
			upuesto->cvig = estpue.cvig;
			upuesto ++;

			if (upuesto == &pripuesto[MAXPUE]) Error("Tabla interna saturada. Max %d", MAXPUE);
		}
		FinPuestosVivos ();
	}
}	

void GenerarReporte ()
{
	ObtenerPuesto ();
	
	FmSetFld (fm0, COMENTARIO, NULL_STR);
	WiRefresh();
	
	if (*FmSFld(fm0, SALIDA) == 'D') {	// DW
		AbrirArchivo(); 
		GenerarArchivo();
		fclose(fp);
	}
	else {
		if (*FmSFld(fm0, SALIDA) == 'R') {	// Assist
			AbrirArchivo(); 
			ImprimirArchivo();
			fclose(fp);
		}
		else {
			AbrirReporte();
			ImprimirReporte();
			CloseReport(rp0);
		}
	}
}


private void AbrirReporte()
{
	
	if (*FmSFld(fm0, SALIDA) == 'A') {
		rp0 = OpenReport("cantvig2", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, RP_IO_FILE, FmSFld(fm0, ARCHIVO));
	}
	else {
		rp0 = OpenReport("cantvig", RP_EABORT|RP_NOBEGIN);
		RpSetOutput(rp0, *FmSFld(fm0,SALIDA) == 'I' ? RP_IO_DEFAULT : RP_IO_TERM, NULL_STR);
	}
	BeginReport(rp0,1,NULL_STR);
}

private int comppue(struct puestos *a, struct puestos *b)
{
	return	                                                                 
			a->cliente	> b->cliente	? 1 : a->cliente		< b->cliente		? -1 :
			a->objetivo	> b->objetivo	? 1 : a->objetivo		< b->objetivo		? -1 :
			a->ptoser	> b->ptoser		? 1 : a->ptoser			< b->ptoser		? -1 :
			a->puesto	> b->puesto		? 1 : a->puesto			< b->puesto		? -1 :
			0;
}


static void AbrirArchivo() 
{
	if ((fp=fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
		Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));
}

static void ImprimirArchivo()
{
	DATE fecha=FmIsNull(fm0,FECHA)? Today() : FmDFld(fm0,FECHA);
  	long cliant=0;
  	int  objant=0;
	bool primero=TRUE;
	double cantid=0;
		  		
	if (upuesto == pripuesto)
		return;
		
	qsort((char *)pripuesto, (unsigned)(upuesto-pripuesto), sizeof(pripuesto[0]), (IFPVCPVCP)comppue);

	for (epuesto=pripuesto; epuesto < upuesto ; epuesto ++) {

		if (!epuesto->cpue)
			continue;

		if(!primero && (epuesto->cliente!=cliant || epuesto->objetivo!=objant)) {
			fprintf(fp, "%02d%4d%s%ld%s%s%s%d%s%s%s%.2f\n",
				Month(fecha), Year(fecha),			R_SEPAR,
				cliant,                             R_SEPAR,
				GetDescCli(cliant),                	R_SEPAR,
				objant,                           	R_SEPAR,
				GetObjDescrip(cliant, objant), 		R_SEPAR,
				cantid/100
				);
			cantid=0;
		}
		primero=FALSE;
		cliant=epuesto->cliente; 
		objant=epuesto->objetivo;
		cantid+=epuesto->cvig;	// acumula cant. de vig. x cli/obj
	}     
	
	// el ultimo
	fprintf(fp, "%02d%4d%s%ld%s%s%s%d%s%s%s%.2f\n",
		Month(fecha), Year(fecha),			R_SEPAR,
		cliant,                             R_SEPAR,
		GetDescCli(cliant),                	R_SEPAR,
		objant,                           	R_SEPAR,
		GetObjDescrip(cliant, objant), 		R_SEPAR,
		cantid/100
		);
}

static void GenerarArchivo()
{
  	long cliant = 0;
  	int  objant = 0;
	int  ptoant = 0;
	bool primero = TRUE;
	double cantid = 0;
		  		
	if (upuesto == pripuesto)
		return;
		
	qsort((char *)pripuesto, (unsigned)(upuesto-pripuesto), sizeof(pripuesto[0]), (IFPVCPVCP)comppue);

	for (epuesto=pripuesto; epuesto < upuesto ; epuesto ++) {

		if (!epuesto->cpue)
			continue;

		if(!primero &&
			(epuesto->cliente != cliant || epuesto->objetivo != objant || epuesto->ptoser != ptoant)) {

			fprintf(fp, "%ld\t%s\t%d\t%s\t%d\t%s\t%d\t%s\t%.2f\n",
				cliant,	GetDescCli(cliant),
				objant, GetObjDescrip(cliant, objant),
				ptoant, GetDescPto(ptoant),
				GetGrupoPto(ptoant), GetDescGrupo(GetGrupoPto(ptoant)),
				cantid/100);

			cantid=0;
		}
		primero = FALSE;
		cliant  = epuesto->cliente; 
		objant  = epuesto->objetivo;
		ptoant  = epuesto->ptoser;
		cantid += epuesto->cvig;        // acumula cant. de vig. x cli/obj
	}     
	
	// el ultimo
	fprintf(fp, "%ld\t%s\t%d\t%s\t%d\t%s\t%d\t%s\t%.2f\n",
			cliant,	GetDescCli(cliant),
			objant, GetObjDescrip(cliant, objant),
			ptoant, GetDescPto(ptoant),
			GetGrupoPto(ptoant), GetDescGrupo(GetGrupoPto(ptoant)),
			cantid/100);
}

