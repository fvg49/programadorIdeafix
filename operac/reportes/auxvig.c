/********************************************************************
*
* MODULE & VERSION : @(#)auxvig.c	1.6
* DATE             : 08/06/27
* TIME             : 16:56:18
*
* CREATED          : 12/06/02
*
* DESCRIPTION:
*	Listado de Control de Ausentismo por Vigilidor.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
* 
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "opechi.h"
#include "comerc.h"
#include "ambiente.h"
#include "billpro.h"
#include "auxvig.fmh"
#include "auxvig.rph"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
#include "asist.sch"
#include "filial.h"
//#include "comerc.c"

//#define DEBUG		1

#define MAXLEG			160000
#define	ERR_ARCHI		"No se pudo abrir el archivo."
#define COND_AUSENTE 	"A"
#define COND_VACACIONES "V"
#define SIN_LICENCIA 	-1
#define VACACIONES		"VACACIONES"
#define COD_NOV_VACACIONES 0 //Codigo de novedad asignado a vacaciones

/* Estructuras */
struct vigil {
	int emp;
	long nroleg;
	long cli;
	int	 obj;
    int  dia;
    char cond[2];
    int  codnov;
    int	 puesto;              
    int  tippto;
} pvig[MAXLEG], *uvig;

/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static void CargarDatos(int emp, long clid, long clih, int objd, int objh, DATE fechad, DATE fechah, long vigd, long vigh, bool conretro);
static void GenerarReporte(bool conlic);

static void InicializarLista();
static bool EstaVaciaLista();
static void CargarVigil(int emp, long nroleg, long cliente, int objetivo, DATE dia, char * condic, int codnov, int puesto, int tippto);
static int compvig(const void *a, const void *b);
int GetLicenciaLeg(int emp, long nroleg, DATE fecha);
int GetNovedadLeg(int emp, long nroleg, DATE fecha);
static char * GetDescNovedad(int codnov);

static void AbrirArchivo();
static void CerrarArchivo();
static void AbrirReporte();
static void CerrarReporte();
static void SetearCabArch();
static void ImprimirCabecera();
static void	ImprimirReporte(bool conlic);
static void ArchivarReporte(bool conlic);

/* Declaraciones globales */
schema comerc, operac, bill, sue;
form fm0;
report rp0=NULL;
FILE *fp=NULL;
int cantVigLista=0;

/* Programa principal */
wcmd(auxvig, 1.16 12/06/02)
{
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);

	fm0 = OpenForm("auxvig", FM_EABORT);
	
	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();
	
	while (DoForm(fm0, before, after) != FM_EXIT) {

		InicializarLista();

        CargarDatos(	FmIFld(fm0,EMP), 
					  	FmLFld(fm0,CLIDESDE), FmLFld(fm0,CLIHASTA), 
						FmIFld(fm0,OBJDESDE), FmIFld(fm0,OBJHASTA), 
						FmDFld(fm0,FECHAD),   FmDFld(fm0,FECHAH), 
						FmLFld(fm0,VIGDESDE), FmLFld(fm0,VIGHASTA),
						FmIFld(fm0,FRETRO) ); 

	    // if(!EstaVaciaLista ())
	    if (cantVigLista!=0) 
	    	GenerarReporte(TRUE);
  		    
		else
			WiMsg("No hay datos.");
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void GenerarReporte(bool conlic)
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		AbrirArchivo();
		SetearCabArch();
		ArchivarReporte(conlic);
		CerrarArchivo();     
		WiMsg("Se genero el archivo Exitosamente");
	}
	else {
		AbrirReporte();
		ImprimirCabecera();
		ImprimirReporte(conlic);
		CerrarReporte();
	}
}

static void AbrirReporte()
{
    if(rp0==NULL) {
    	rp0 = OpenReport("auxvig", RP_EABORT|RP_NOBEGIN);
    }
   
    // Si la salida es Impresora
    if ( *FmSFld(fm0, SALIDA) == 'I')
		RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );

	// Si la salida es Terminal
    if ( *FmSFld(fm0, SALIDA) == 'T')
		RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

    BeginReport(rp0, 1, NULL_STR);
	RpSetIFld(rp0, POR, FmIFld(fm0,LISTAPOR) );
} 

static void ImprimirCabecera()
{
	RpSetDFld(rp0, RFECHAD, FmDFld(fm0, FECHAD));
	RpSetDFld(rp0, RFECHAH, FmDFld(fm0, FECHAH));
    
    RpSetFld (rp0, RDELEG,  FmSFld(fm0, DELEG));
    RpSetFld (rp0, RDELEGA, FmSFld(fm0, FDELEGA));
    RpSetFld (rp0, RFILIAL, FmSFld(fm0, FFILIAL));
    
	RpSetLFld(rp0, RCLID,  FmLFld(fm0, CLIDESDE));
	RpSetFld (rp0, RDCLID, FmSFld(fm0, DCLID));
	RpSetLFld(rp0, RCLIH,  FmLFld(fm0, CLIHASTA));
	RpSetFld (rp0, RDCLIH, FmSFld(fm0, DCLIH));

	RpSetIFld(rp0, ROBJD,  FmIFld(fm0, OBJDESDE));
	RpSetFld (rp0, RDOBJD, FmSFld(fm0, DOBJD));
	RpSetIFld(rp0, ROBJH,  FmIFld(fm0, OBJHASTA));
	RpSetFld (rp0, RDOBJH, FmSFld(fm0, DOBJH));

	RpSetLFld(rp0, RVIGD,  FmLFld(fm0, VIGDESDE));
	RpSetFld (rp0, RDVIGD, FmSFld(fm0, DVIGD));
	RpSetLFld(rp0, RVIGH,  FmLFld(fm0, VIGHASTA));
	RpSetFld (rp0, RDVIGH, FmSFld(fm0, DVIGH));

	RpSetIFld(rp0, RRETRO,   FmIFld(fm0, FRETRO));
}

static void CargarDatos(int emp, long clid, long clih, int objd, int objh, DATE fechad, DATE fechah, long vigd, long vigh, bool conretro)
{
	dbcursor cparte, cretro;
	int objet;
	long nroleg, cliente;
	char condic[2];
	DATE dia;
	int codnov;
	int tippto, puesto;

#ifdef DEBUG
fprintf(stderr,"CargarVigil() - emp %d, cli %ld %ld, obj %d %d, leg %ld %ld, (%.3D %.3D)\n", 
		emp, clid, clih, objd, objh, vigd, vigh, fechad, fechah);
#endif
	
	cantVigLista=0;
	
	// 1ero. Cargar los partes con condicion "A"
	if (vigd!=NULL_LONG && vigh!=NULL_LONG) {
		cparte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
		SetCursorFrom(cparte, emp, vigd, fechad, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cparte, emp, vigh, fechah, MAX_LONG, MAX_SHORT);
	}
	else {
		cparte = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);
		SetCursorFrom(cparte, emp, clid, objd, fechad, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
		SetCursorTo  (cparte, emp, clih, objh, fechah, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
	}
	while(FetchCursor(cparte)!=ERROR) {
        //valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO)))
		       	continue;
		
		if (!ValidaFilial(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

        nroleg	= LFld(operac|PARTE_NROLEG);
		emp		= IFld(operac|PARTE_EMP);
		cliente	= LFld(operac|PARTE_CLIENTE);
		objet	= IFld(operac|PARTE_OBJETIVO);
		sprintf(condic, "%s", SFld(operac|PARTE_CONDIC));
		dia		= DFld(operac|PARTE_DIA);
     	codnov  = IFld(operac|PARTE_CODAUS);
     	//ptoser.puesto.
     	puesto	= IFld(operac|PARTE_PUESTO);
		tippto	= IFld(operac|PARTE_PTOSER);
	   	          
        // Necesito solo los ausentes de PARTE
        if((strcmp(condic, COND_AUSENTE)!=0) && (strcmp (condic, COND_VACACIONES) != 0))
        	continue;                    
        	

		// Filtro fechas de partes no solicitadas en el form
		if( dia < fechad ||	dia > fechah )
			continue;
        
//     	codnov = GetNovedadLeg(emp, nroleg, dia);

		CargarVigil(emp, nroleg, cliente, objet, dia, condic, codnov, puesto, tippto);
		
	}

	DeleteCursor(cparte);


    // 2do. Si corresponde. Cargar los retros con cualquier condicion
	if (conretro) {

	 	if (vigd!=NULL_LONG && vigh!=NULL_LONG) {
			cretro = CreateCursor(operac|RETRObyREMPLE, IO_NOT_LOCK);
			SetCursorFrom(cretro, emp, vigd, fechad, MIN_LONG, MIN_SHORT);
			SetCursorTo  (cretro, emp, vigh, fechah, MAX_LONG, MAX_SHORT);
		}
		else {
			cretro = CreateCursor(operac|RETRObyEMP, IO_NOT_LOCK);
			SetCursorFrom(cretro, emp, clid, objd, fechad, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
			SetCursorTo  (cretro, emp, clih, objh, fechah, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
		}
		while(FetchCursor(cretro)!=ERROR) {
		    //valida el cliente/objetivo para el usuario
			if (!ValidaListaXusr(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO)))
		       	continue;
		
			if (!ValidaFilial(LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
				continue;

	        nroleg	= LFld(operac|RETRO_NROLEG);
			emp		= IFld(operac|RETRO_EMP);
			cliente	= LFld(operac|RETRO_CLIENTE);
			objet	= IFld(operac|RETRO_OBJETIVO);
			sprintf(condic, "%s", SFld(operac|RETRO_CONDIC));
			dia		= DFld(operac|RETRO_DIA);
	     	codnov  = IFld(operac|RETRO_CODAUS);     
	     	puesto  = IFld(operac|RETRO_PUESTO);
	     	tippto  = IFld(operac|RETRO_PTOSER); //ptoser

			// Filtro fechas de partes no solicitadas en el form
			if( dia < fechad ||	dia > fechah )
				continue;
	       	
	       	codnov = GetNovedadLeg(emp, nroleg, dia);
            
            CargarVigil(emp, nroleg, cliente, objet, dia, condic, codnov, puesto, tippto);
		}
		
		DeleteCursor(cretro);
	}
}

static void CargarVigil(int emp, long nroleg, long cliente, int objetivo, DATE dia, char * condic, int codnov, int puesto, int tippto)
{
 	bool encontro=FALSE;	
	struct vigil * evig;

	// buscar si ya existe vigilador: emp, nroleg, cliente, objetivo, dia, y actuliza si corresponde

	for (evig=pvig; evig<uvig && !encontro; evig++) {

		if(evig->emp==emp && evig->nroleg==nroleg && evig->cli==cliente && 
				evig->obj==objetivo && evig->dia==dia) {

			// si la condicion es distinta de Ausente -> borrar 
			if(strcmp(condic,COND_AUSENTE)!=0 && strcmp(condic, COND_VACACIONES) != 0) {
				evig->emp	  = 0;	// borrado logico
				evig->nroleg  = 0;
				evig->cli	  = 0;
				evig->obj	  = 0;
				evig->dia     = 0;
			}
		    cantVigLista--;
			encontro=TRUE;	
		}
	}

	// dar de alta solo si esta Ausente (viene de PARTE o de RETRO)
	
	if(!encontro && (strcmp(condic,COND_AUSENTE)==0 || strcmp(condic,COND_VACACIONES)==0)) {
	 	if (uvig == &pvig[MAXLEG])
			Error("Tabla interna de Vigiladores saturada. Max %d", MAXLEG);

		uvig->emp	  = emp;
		uvig->nroleg  = nroleg;		
		uvig->cli	  = cliente;
		uvig->obj	  = objetivo;
		uvig->dia     = dia;
		strcpy(uvig->cond, condic);
		uvig->codnov  = codnov;
		uvig->puesto = puesto;
		uvig->tippto = tippto;
		uvig++;
		cantVigLista++;
	}
}					 

static int compvig(const void *a, const void *b)
{
	struct vigil *aa, *bb;
	aa = (struct vigil *)a;
	bb = (struct vigil *)b;
	return	                                                                 
			aa->nroleg > bb->nroleg ? 1 : aa->nroleg < bb->nroleg ? -1 :
			aa->dia    > bb->dia    ? 1 : aa->dia    < bb->dia    ? -1 :
			0;
}

static void	ImprimirReporte(bool conlic)
{
	struct vigil * evig;
	schema	cur;
	
	qsort((void *)pvig, (size_t)(uvig-pvig), (size_t)sizeof(pvig[0]), compvig);
                   
	for (evig=pvig ; evig < uvig ; evig++) {

		if(evig->emp==0 && evig->nroleg==0 )	// estos son los borrados logicos
			continue;

		RpClearZone(rp0, ZVIG); 
				
		// considera licencia?
		
	    RpSetLFld(rp0, RVIGIL,  evig->nroleg);

	    SetKey(sue|PERbyEMP, evig->emp, evig->nroleg);
	    if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK)!=ERROR)
	    	RpSetFld(rp0, RDVIGIL, SFld(sue|PER_APYNOM));

		RpSetLFld(rp0, RCLI, evig->cli);

		cur=CurrentSchema();
		RpSetFld(rp0, RDCLI, GetDescCli(evig->cli));
		SwitchToSchema(cur);
        
        RpSetIFld(rp0, ROBJ, evig->obj);
        RpSetFld(rp0, RDOBJ, GetObjDescrip(evig->cli, evig->obj));
        
        RpSetDFld(rp0, RDIA, evig->dia);
        RpSetFld(rp0, RCOND, evig->cond);

		if (strcmp( evig->cond, COND_AUSENTE)==0) {

	        RpSetIFld(rp0, RNOV, evig->codnov);
    	    RpSetFld(rp0, RDNOV, GetDescNovedad(evig->codnov));
		}
		else
		{
			RpSetIFld(rp0, RNOV, COD_NOV_VACACIONES);
    	    RpSetFld(rp0, RDNOV, VACACIONES);
		}
//        rpue, rtippue, rdesctpue
        RpSetIFld(rp0, RPUE, evig->puesto);
        RpSetIFld(rp0, RTIPPUE, evig->tippto);
        RpSetFld(rp0, RDESCTPUE, GetDescPto(evig->tippto));

		DoReport(rp0, ZVIG);
	}
}

static void AbrirArchivo()
{
	if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
		Error(ERR_ARCHI);
}

static void	SetearCabArch()
{
	fprintf(fp, "Empresa\tDescripción Empresa\tVigilador\tNombre y Apellido\tCliente\tRazón Social\tObjetivo\tDescrip. Objetivo\tFecha\tCond\tCódigo Novedad\tDescrip. Novedad\tPuesto\tTipo de Puesto\tDescripcion de Tipo de Puesto\n");
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
	case LISTAPOR :
		switch(FmIFld(fm, fno)) {
			case 1 :
				FmSetFld(fm0, DVIGD, NULL_STR);
				FmSetFld(fm0, DVIGH, NULL_STR);
				break;
			case 2 :
				FmSetFld(fm0, DCLID, NULL_STR);
				FmSetFld(fm0, DCLIH, NULL_STR);
				FmSetFld(fm0, DOBJD, NULL_STR);
				FmSetFld(fm0, DOBJH, NULL_STR);
				break;
		}
	case SALIDA:
		if (*FmSFld(fm0, SALIDA) == 'A' && FmIsNull(fm0, NOMARCH))
			FmSetFld(fm0, NOMARCH, "auxvig.txt");
	break;
	case FECHAD:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'L')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "LUNES");
		#endif
	break;
	case FECHAH:
		#ifdef _NOVIA_VER_2_0
			if (*DiaLetra(FmDFld(fm, fno)) != 'D')
				return FmErrMsg (fm, M_MAL_FECHA, DayName(FmDFld(fm, fno)),  "DOMINGO");
		#endif
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

static void ArchivarReporte(bool conlic)
{
	struct vigil * evig;
	schema	cur;
	char desccli[50];	
	
	qsort((void *)pvig, (size_t)(uvig-pvig), (size_t)sizeof(pvig[0]), compvig);
                   
	for (evig=pvig ; evig < uvig ; evig++) {

		if(evig->emp==0 && evig->nroleg==0 )	// estos son los borrados logicos
			continue;

		// considera licencia?
		
		cur=CurrentSchema();
		strcpy(desccli, GetDescCli(evig->cli));
		SwitchToSchema(cur);

	    
//	    WiMsg("PUESTO: %d %s",evig->puesto, GetDescPto(evig->puesto));
//	    WiMsg("TIPO DE SERVICIO: %d %s", evig->tippto, GetDescServicio(evig->tippto));
                   // 1    2   3   4    5   6   7     8   9   0   1   2   3
	    fprintf(fp, "%d\t%s\t%ld\t%s\t%ld\t%s\t%d\t%s\t%.3D\t%s\t%d\t%s\t%d\t%d\t%s\n", 
            
            evig->emp, FmSFld(fm0, DEMP),
	    	evig->nroleg, GetNombreLeg(evig->emp, evig->nroleg),
			evig->cli, desccli,
	        evig->obj, GetObjDescrip(evig->cli, evig->obj),
	        evig->dia,
	        evig->cond, (strcmp( evig->cond, COND_AUSENTE)==0)?evig->codnov:COD_NOV_VACACIONES,
	        (strcmp( evig->cond, COND_AUSENTE)==0)?GetDescNovedad(evig->codnov):VACACIONES,
	        evig->puesto, 
	        evig->tippto, GetDescPto(evig->tippto));
	}
}

static void CerrarReporte()
{
	if(rp0==NULL)
		CloseReport(rp0);
	rp0=NULL;
}

static void CerrarArchivo()
{
	fclose(fp);
	fp=NULL;
}

static void InicializarLista()
{
	int i;
	for (i=0; i<MAXLEG ; i++) {
	    pvig[i].emp=0;
		pvig[i].nroleg=0;
	    pvig[i].cli=0;
	    pvig[i].obj=0;
    	pvig[i].dia=NULL_DATE;
   	 	*pvig[i].cond=0; //='\0'
    	pvig[i].codnov=NULL_SHORT;
	}
	uvig=pvig;
}

static bool EstaVaciaLista()
{
	return (pvig==uvig? TRUE:FALSE);
}



int GetNovedadLeg(int emp, long nroleg, DATE dia)
{
	dbtable  AASISTEN;
	schema   old, asist;
    int codnov=NULL_SHORT;
    
	codnov=NULL_SHORT;
	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);

	AASISTEN = CreateAlias(asist|ASISTEN);

	SetKey(AlInd(AASISTEN, asist|ASISTENbyEMPRE), emp, dia, nroleg, MIN_SHORT);
	if (GetRecord(AlInd(AASISTEN, asist|ASISTENbyEMPRE), NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3)!=ERROR) {
		codnov = IFld(AlFld(AASISTEN, asist|ASISTEN_CODNOV));
	}
	DeleteAlias(AASISTEN);
	return codnov;
}

static char * GetDescNovedad(int codnov)
{
	dbtable  AINASIST;
	schema   old, asist;
	static char desc[26];

	sprintf(desc, "%s", NULL_STR);
	old   = CurrentSchema();
	asist = OpenSchema("asist", IO_EABORT);
	(void)SwitchToSchema(old);
	
	AINASIST = CreateAlias(asist|INASIST);
	
	SetKey(AlInd(AINASIST, asist|INASISTbyCODINA), codnov);
	if(GetRecord(AlInd(AINASIST, asist|INASISTbyCODINA), THIS_KEY, IO_NOT_LOCK)!=ERROR)
		sprintf(desc, "%s", SFld(AlFld(AINASIST, asist|INASIST_DESCRINAS)));

	DeleteAlias(AINASIST);
	return desc;	
}

