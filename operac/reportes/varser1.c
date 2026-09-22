/********************************************************************
*
* MODULE & VERSION : @(#)varser1.c	1.3
* DATE             : 21/05/11
* TIME             : 16:32:09
*
* CREATED          : 17/08/04
*
* DESCRIPTION:
*			Variaciones de Servicio
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comerc.h"
#include "operac.h"
#include "billpro.h"
#include "varser1.fmh"
#include "varser1.rph"
#include "comerc.sch"
#include "filial.h"

#define MAXPUE	  	10000
#define PTOS       	1
#define	R_SEPAR		"	"

#define	TIT_CLI  		"Cliente"
#define	TIT_DCLI  		"Descrip. Cliente"
#define	TIT_OBJ			"Objetivo"
#define	TIT_DOBJ		"Descrip. Objetivo"
#define	TIT_CATEG       "Categoría"
#define	TIT_DCATEG      "Descrip. Categoría"
#define TIT_CPU			"Cant. Puesto"
#define TIT_PUESTO	    "Puesto"
#define TIT_CODINT		"Codint"
#define TIT_DPUESTO		"Descrip. Puesto"
#define TIT_HORINI	    "Hora Ini."
#define TIT_HORFIN	    "Hora Fin."
#define TIT_DIAS 	    "Días"
#define TIT_REG       	"Régimen"
#define TIT_SUBREG	    "Subregimen"
#define TIT_BLANCO      "Tipo día"
#define TIT_HORASN      "Horas N"
#define TIT_EXT50       "Ext 50%"
#define TIT_EXT100      "Ext 100%"
#define TIT_TOTHS   	"Total Hs."
#define TIT_CANV 	    "Cant. Vig."

/* Declaraciones de Estructuras */
struct puestos {
	short ptoser, puesto, hspt;
	short codint;
	short cpue, objetivo;
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
	char tipodia[10];
	long salar, cliente, hsnorm, hs50, hs100, cvig;
}pripuesto[MAXPUE], *upuesto=pripuesto, *epuesto;

/* Funciones privadas */
private fm_status before(form fm, fmfield fno, int row);
private fm_status after(form fm, fmfield fno, int row);
private int comppue(struct puestos *a, struct puestos *b);
private void ArchivarPto();
void ObtenerPuesto();
void ImprimirPto();
void AbrirSalida();

/* Declaraciones globales */
FILE *fp = NULL;
form fm0;
report rp0;
schema com;
struct spuesto estpue;

/* Programa principal */
wcmd(varser1, 1.3 05/11/21)
{
	fm_cmd cmd;

	fm0 = OpenForm  ("varser1", FM_EABORT);
	com	= OpenSchema("comerc",  IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT)
	switch (cmd) {
	case FM_UPDATE:
		BeginTransaction();
		
		ObtenerPuesto();
		AbrirSalida();

		if (*FmSFld(fm0, SALIDA) == 'A') {

			ArchivarPto();
			fclose(fp);
		}
		else {
			ImprimirPto();
			CloseReport(rp0);
		}
		EndTransaction();
		break;
	case FM_IGNORE:
		break;
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

void ImprimirPto()
{
	if (upuesto == pripuesto)
		return;

	qsort((char *)pripuesto, (unsigned)(upuesto-pripuesto), sizeof(pripuesto[0]), (IFPVCPVCP)comppue);

	for (epuesto = pripuesto; epuesto < upuesto; epuesto ++) {
		if (!epuesto->cpue)
			continue;

		RpClearZone(rp0, LINPUE);
		RpSetLFld  (rp0, RCLIE,     epuesto->cliente);
		RpSetFld   (rp0, RDCLIE,    GetDescCli(epuesto->cliente));
		RpSetIFld  (rp0, ROBJ,      epuesto->objetivo);
		RpSetFld   (rp0, RDOBJ,     GetObjDescrip(epuesto->cliente, epuesto->objetivo));
		RpSetIFld  (rp0, R_CANTOP,  epuesto->cpue);
		RpSetIFld  (rp0, R_PTOSER,  epuesto->ptoser);
		RpSetIFld  (rp0, R_CODINT,  epuesto->codint);
		RpSetFld   (rp0, R_DPTOSER, GetDescPto(epuesto->ptoser));
		RpSetTFld  (rp0, R_HDESDE,  epuesto->hinicio);
		RpSetTFld  (rp0, R_HHASTA,  epuesto->hfinal);
		RpSetFld   (rp0, R_D1,      epuesto->dia1);
		RpSetFld   (rp0, R_D2,      epuesto->dia2);
		RpSetFld   (rp0, R_D3,      epuesto->dia3);
		RpSetFld   (rp0, R_D4,      epuesto->dia4);
		RpSetFld   (rp0, R_D5,      epuesto->dia5);
		RpSetFld   (rp0, R_D6,      epuesto->dia6);
		RpSetFld   (rp0, R_D7,      epuesto->dia7);
		RpSetFld   (rp0, R_REG,     epuesto->regim);
		RpSetIFld  (rp0, R_CATEG,   epuesto->puesto);
		RpSetFld   (rp0, R_SUBR,    epuesto->subreg);
		RpSetFld   (rp0, R_TIPODIA, epuesto->tipodia);
		RpSetLFld  (rp0, R_HSTOT,   epuesto->hsnorm + epuesto->hs50 + epuesto->hs100);
		RpSetLFld  (rp0, R_CANTVIG, epuesto->cvig);
		DoReport   (rp0, LINPUE);
	}
}

void ArchivarPto()
{
	char dias[20];
	if (upuesto == pripuesto)
		return;

	qsort((char *)pripuesto, (unsigned)(upuesto-pripuesto), sizeof(pripuesto[0]), (IFPVCPVCP)comppue);

    // titulos
	fprintf (fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		TIT_CLI,		R_SEPAR,	
		TIT_DCLI,		R_SEPAR,
		TIT_OBJ,        R_SEPAR,    
		TIT_DOBJ,		R_SEPAR,
		TIT_CPU,		R_SEPAR,    
		TIT_PUESTO,     R_SEPAR,    
		TIT_DPUESTO,	R_SEPAR,
		TIT_CODINT,		R_SEPAR,
		TIT_CATEG,      R_SEPAR,
		TIT_HORINI,    	R_SEPAR,
		TIT_HORFIN,    	R_SEPAR,
		TIT_DIAS,       R_SEPAR,
		TIT_REG,        R_SEPAR,
		TIT_SUBREG,     R_SEPAR,
		TIT_BLANCO,     R_SEPAR,
		TIT_HORASN,     R_SEPAR,
		TIT_EXT50,      R_SEPAR,
		TIT_EXT100,     R_SEPAR,
		TIT_TOTHS,  	R_SEPAR,
		TIT_CANV	    );

	// renglones
	for (epuesto = pripuesto; epuesto < upuesto; epuesto ++) {
		if (!epuesto->cpue)
			continue;    
                           
			sprintf(dias,"%s-%s-%s-%s-%s-%s-%s",
					strcmp(epuesto->dia1,"")==0? "  ": epuesto->dia1,
					strcmp(epuesto->dia2,"")==0? "  ": epuesto->dia2,
					strcmp(epuesto->dia3,"")==0? "  ": epuesto->dia3,
					strcmp(epuesto->dia4,"")==0? "  ": epuesto->dia4,
					strcmp(epuesto->dia5,"")==0? "  ": epuesto->dia5,
					strcmp(epuesto->dia6,"")==0? "  ": epuesto->dia6,
					strcmp(epuesto->dia7,"")==0? "  ": epuesto->dia7
					);
						//                                     1                                   2
                        // 1   2    3   4   5    6   7   8   9   0   1   2   3   4   5   6   7   8   0   1   2     3     4     5     6
 			//fprintf (fp, "%ld%s%s%s%d%s%s%s%d%s%d%s%s%s%d%s%.1T%s%.1T%s%s%s%s%s%s%s%s%s%.2f%s%.2f%s%.2f%s%.2f%s%.2f\n",
 			//fprintf (fp, "%ld%s%s%s%d%s%s%s%d%s%d%s%s%s%d%s%.1T%s%.1T%s%s%s%s%s%s%s%s%s%.2f%s%.2f%s%.2f%s%.2f%s%.2f\n",
 			//            1  2 3 4 5 6 7 8 9 10111213141516171819  2021  22232425262728293031  3233  3435  3637  3839
 			fprintf (fp, "%ld%s%s%s%d%s%s%s%d%s%d%s%s%s%d%s%d%s%.1T%s%.1T%s%s%s%s%s%s%s%s%s%.2f%s%.2f%s%.2f%s%.2f%s%.2f\n",
					epuesto->cliente, 				R_SEPAR,
					GetDescCli(epuesto->cliente),   R_SEPAR,
					epuesto->objetivo,              R_SEPAR,
					GetObjDescrip(epuesto->cliente, epuesto->objetivo),	R_SEPAR,
					epuesto->cpue,					R_SEPAR,
					epuesto->ptoser,				R_SEPAR,
					GetDescPto(epuesto->ptoser),    R_SEPAR,
					epuesto->codint,				R_SEPAR,
					epuesto->puesto,                R_SEPAR,
					epuesto->hinicio,               R_SEPAR,
					epuesto->hfinal,                R_SEPAR,
					dias,   			            R_SEPAR,
					epuesto->regim,                 R_SEPAR,
					epuesto->subreg,                R_SEPAR,
					epuesto->tipodia,               R_SEPAR,
					(float)epuesto->hsnorm/100,                R_SEPAR,
					(float)epuesto->hs50/100,                  R_SEPAR,
					(float)epuesto->hs100/100,                	R_SEPAR,
					(float)(epuesto->hsnorm + epuesto->hs50 + epuesto->hs100)/100,	R_SEPAR,
					(float)epuesto->cvig/100);
	}
}

void ObtenerPuesto()
{
	dbcursor c_OBJ;
	_SParam_PVivo parhora;

	upuesto = pripuesto;

	c_OBJ = CreateCursor(com|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	SetCursorFrom(c_OBJ, FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
	SetCursorTo  (c_OBJ, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
	while (FetchCursor(c_OBJ) != ERROR) {
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET)))
		   	continue;
		
		if (!ValidaFilial(LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		
		InicPuestosVivos(FmIFld(fm0, EMP), LFld(com|OBJETIVO_CLIENTE), IFld(com|OBJETIVO_OBJET),
						 NULL_SHORT,          //Puesto
						 FmIFld(fm0, MODELO), //Modelo OT
						 Today(),
						 Today(),
						 FALSE,               //Que no cargue tarifario
						 FALSE,               //Que no cargue bonos
						 _VALIDAR_NOFEC,      //Considera fecha de inicio de la ot
						 NULL_SHORT,
						 TRUE,
						 FALSE,
						 parhora,
						 FALSE);

		while (ProximoPuestoVivo (&estpue)) {
			if (!estpue.cpue)
				continue;

			upuesto->cliente  = LFld(com|OBJETIVO_CLIENTE);
			upuesto->objetivo = IFld(com|OBJETIVO_OBJET);
			upuesto->ptoser   = estpue.ptoser;
			upuesto->codint   = estpue.codint;
			upuesto->puesto   = estpue.puesto;
			upuesto->hinicio  = estpue.hinicio;
			upuesto->hfinal   = estpue.hfinal;
			upuesto->hspt     = estpue.hspt == NULL_SHORT ? 0 : estpue.hspt;
			upuesto->hsnorm   = estpue.hs_norm_ot;
			upuesto->hs50     = estpue.hs_50_ot;
			upuesto->hs100    = estpue.hs_100_ot;

			strcpy(upuesto->tipodia, estpue.tipodia);
			strcpy(upuesto->frec,    NULL_STR);
			strcpy(upuesto->dia1,    NULL_STR);
			strcpy(upuesto->dia2,    NULL_STR);
			strcpy(upuesto->dia3,    NULL_STR);
			strcpy(upuesto->dia4,    NULL_STR);
			strcpy(upuesto->dia5,    NULL_STR);
			strcpy(upuesto->dia6,    NULL_STR);
			strcpy(upuesto->dia7,    NULL_STR);
			strcpy(upuesto->regim,   NULL_STR);
			strcpy(upuesto->subreg,  NULL_STR);

			if (strcmp(estpue.frec,NULL_STR))
				sprintf(upuesto->frec,"%1s",estpue.frec);
			if (strcmp(estpue.dia1,NULL_STR))
				sprintf(upuesto->dia1,"%1s",estpue.dia1);
			if (strcmp(estpue.dia2,NULL_STR))
				sprintf(upuesto->dia2,"%1s",estpue.dia2);
			if (strcmp(estpue.dia3,NULL_STR))
				sprintf(upuesto->dia3,"%1s",estpue.dia3);
			if (strcmp(estpue.dia4,NULL_STR))
				sprintf(upuesto->dia4,"%1s",estpue.dia4);
			if (strcmp(estpue.dia5,NULL_STR))
				sprintf(upuesto->dia5,"%1s",estpue.dia5);
			if (strcmp(estpue.dia6,NULL_STR))
				sprintf(upuesto->dia6,"%1s",estpue.dia6);
			if (strcmp(estpue.dia7,NULL_STR))
				sprintf(upuesto->dia7,"%1s",estpue.dia7);

			strcpy(upuesto->regim,  estpue.regim);
			strcpy(upuesto->subreg, estpue.subreg);

			upuesto->salar = estpue.salar;
			upuesto->cpue  = estpue.cpue;
			upuesto->cvig  = estpue.cvig;
			upuesto ++;

			if (upuesto == &pripuesto[MAXPUE])
				Error("Tabla interna saturada. Max %d", MAXPUE);
		}
		FinPuestosVivos();
	}
}

void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));
	}
	else {                                                             
		rp0 = OpenReport("varser1", RP_EABORT|RP_NOBEGIN);

		RpSetOutput(rp0, str_eq(FmSFld(fm0,SALIDA),"I") ? RP_IO_DEFAULT : RP_IO_TERM, NULL_STR);
		BeginReport(rp0,1,NULL_STR);
	}
}

private int comppue(struct puestos *a, struct puestos *b)
{
	return
			a->cliente  > b->cliente  ? 1 : a->cliente  < b->cliente  ? -1 :
			a->objetivo > b->objetivo ? 1 : a->objetivo < b->objetivo ? -1 :
			a->ptoser   > b->ptoser   ? 1 : a->ptoser   < b->ptoser   ? -1 :
			a->puesto   > b->puesto   ? 1 : a->puesto   < b->puesto   ? -1 :
			0;
}


private fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
 	case NOMARCH:
		if (*FmSFld (fm, SALIDA) == 'A' && FmIsNull (fm, NOMARCH))
			FmSetFld (fm, NOMARCH, "varser1.txt");
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

