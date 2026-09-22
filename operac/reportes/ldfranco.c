/********************************************************************
*
* MODULE & VERSION : @(#)ldfranco.c	1.3 
* DATE             : 05/10/21 
* TIME             : 12:25:14 
*
* CREATED          : 24/10/05
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Ver tema de mismo dia en 2 puestos y objetivos distintos
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <ideafix.h>
#include "ldfranco.fmh"
#include "ldfranco.rph"
#include "operac.h"
#include "comerc.h"
#include "comerc.sch"
#include "operac.sch"
#include "bill.sch"
#include "sue.sch"
/******************** Definiciones de Esquemas  Form  y reports *******************************/
form fm0;
report rp0;
schema comerc, operac, bill, sue;

/************************** Definiciones de Estructuras **************************************/
typedef struct francoPA {
	int		emp;
	long	cliente;
	int		objet;
	int		tpue;
	char	regim[9];
	long	legajo;
	int		cantdi; // cant dias 
	int		cantfr; // cant francos rales
	int		fratra; // cant francos trabajados
	int		franot; // cant francos no trabajados
	DATE 	dia   ;
	bool  	retro ; // FALSE sin retroactivo, TRUE modif por retroactivo.
} francoPA;

// Definicion del NODO
typedef struct tnode {
	francoPA data;
	struct tnode *pi;
	struct tnode *pd;
} bnode;
typedef bnode *punt;

/******************************* Funciones para el arbol ************************************/
static punt CargoNodoA(punt base, francoPA * s, int modo);
static void MuestroNodoA(punt base);
static void DeleteNodoA(punt base);
static void PrintA(punt base);
static void DeleteNodoA(punt base);



static punt CargoNodoB(punt base, francoPA * s);
static void MuestroNodoB(punt base);
static void DeleteNodoB(punt base);
static void PrintB(punt base);
static void DeleteNodoB(punt base);


/********************************* Funciones Privadas ****************************************/
static fm_status before(form, fmfield, int);
static fm_status after(form, fmfield, int);

static void AbrirReporte();
static void CerrarReporte();

/********************************** Variables Globales ****************************************/

punt baseA=NULL;
punt baseB=NULL;
FILE  *salida;

/********************************** Programa principal ****************************************/
wcmd(ldfranco, 1.3 10/21/05)
{
	fm_cmd cmd;
	dbcursor c_parte,c_retro;
	long v_vigdesde,v_vighasta;
	char v_regim[10];
	int v_modo;
	francoPA s;


	v_vigdesde	= v_vighasta=0;

	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill"  , IO_EABORT);
	sue    = OpenSchema("sue"   , IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	fm0    = OpenForm("ldfranco", FM_EABORT);
	
	while ((cmd = DoForm(fm0,before, after)) != FM_EXIT) 
	{
		baseA=NULL;
		baseB=NULL;

		if (cmd == FM_IGNORE)
			FmClearAllFlds(fm0);
		
		if (cmd != FM_UPDATE)
			continue;

		v_vigdesde=FmLFld(fm0, W0_VIGDESDE);
		if( FmLFld(fm0, W0_VIGDESDE)==NULL_LONG )
			v_vigdesde=NULL_LONG;

		v_vighasta=FmLFld(fm0, W0_VIGHASTA);
		if( FmLFld(fm0, W0_VIGHASTA)==NULL_LONG )
			v_vighasta=MAX_LONG;

		// +++++++++++++++++++++++++++++++++++++++
		// Seleccion del cursor
		// +++++++++++++++++++++++++++++++++++++++
		if( FmLFld(fm0, W0_VIGDESDE) > 0 )
		{
			c_parte = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);
			SetCursorFrom(c_parte, FmIFld(fm0, W0_EMP), v_vigdesde, NULL_DATE, MIN_LONG, MIN_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, W0_EMP), v_vighasta, MAX_DATE , MAX_LONG, MAX_SHORT);
		}
		else
		{
			// index dia(emp, dia, cliente, objetivo);
			
			c_parte = CreateCursor(operac|PARTEbyDIA, IO_NOT_LOCK);
			SetCursorFrom(c_parte, FmIFld(fm0, W0_EMP), FmDFld(fm0, W0_FECHAD), v_vigdesde, MIN_SHORT);
			SetCursorTo  (c_parte, FmIFld(fm0, W0_EMP), FmDFld(fm0, W0_FECHAH), v_vighasta, MAX_SHORT);
		}
		
		v_modo=0; // no es retroactivo
		while( FetchCursor(c_parte) != ERROR )
		{
			// +++++++++++++++++++++++
			/* inicializo variables */
			// +++++++++++++++++++++++

			// ++++++++++++++++++++++++++++++++++++++++++++
			// los ausentes y las vacasiones no los proceso
			// ++++++++++++++++++++++++++++++++++++++++++++
			if( strcmp(SFld(operac|PARTE_CONDIC), _TRABAJA) != 0 &&
			  	strcmp(SFld(operac|PARTE_CONDIC), _FRANCO ) != 0 )
				continue;

			//+++++++++++++++++++++
			/* filtro por fecha */
			//+++++++++++++++++++++
			if( FmLFld(fm0, W0_VIGDESDE) > 0 )
			{
				if( DFld(operac|PARTE_DIA) < FmDFld(fm0,W0_FECHAD) )
					continue;
				
				if( DFld(operac|PARTE_DIA) > FmDFld(fm0,W0_FECHAH) )
					continue;
			}

			// +++++++++++++++++++++
			// empiezo a cargar nodo
			// +++++++++++++++++++++

			s.emp    = IFld(operac|PARTE_EMP);
			s.cliente= LFld(operac|PARTE_CLIENTE);
			s.objet  = IFld(operac|PARTE_OBJETIVO);
			s.legajo = LFld(operac|PARTE_NROLEG);
			s.tpue   = IFld(operac|PARTE_PTOSER);


			DisplayMsg(FALSE, "Acumulando Francos....... Empresa %d Legajo %ld", IFld(operac|PARTE_EMP), LFld(operac|PARTE_NROLEG));
			WiRefresh();


			// ++++++++++++++++
			// busco el regimen
			// ++++++++++++++++
			// GetRegimenEfectivo(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), v_regim, DFld(operac|PARTE_DIA));

			SetKey(operac|PUESTOS, LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
			                       IFld(operac|PARTE_PTOSER),IFld(operac|PARTE_PUESTO));
			if(GetRecord(operac|PUESTOS, THIS_KEY, IO_NOT_LOCK)==ERROR)
			{
				WiMsg("No existe Puesto para el Cliente:%ld Objetivo:%d \n Puesto:%d Nroint:%d",LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO),
			                       IFld(operac|PARTE_PUESTO),IFld(operac|PARTE_NROINT));
				continue;
			}
			
			// +++++++++
			// regimen
			// +++++++++
			// strcpy(s.regim,v_regim);
			strcpy(s.regim, SFld(operac|PUESTOS_REGIM));
			s.dia = DFld(operac|PARTE_DIA);

			// +++++++++++++++++++++++++++++++++++++++++
			// Cantidad de dias 
			// por cada iteracion se va ir sumando 1 o 0
			// +++++++++++++++++++++++++++++++++++++++++
			
			s.cantdi=1;
			s.cantfr=0;
			s.fratra=0;
			s.franot=0;
						
			// Cantidad de francos, francos Trabajados y no trabajados
			if( strcmp(SFld(operac|PARTE_CONDIC), _FRANCO ) == 0 )
			{
				s.cantfr=1; // cantidad francos reales cargados
			
				// francos Trabajados
				if( TFld(operac|PARTE_HORAENT) != StrToT("0000") && TFld(operac|PARTE_HORASAL) != StrToT("0000") )
					s.fratra=1;
			
				// francos No Trabajados
				if( TFld(operac|PARTE_HORAENT) == StrToT("0000") && TFld(operac|PARTE_HORASAL) == StrToT("0000") )
					s.franot=1;
			} 
			
			/* guardo en la estrutura */
 			baseA=CargoNodoA(baseA, &s, v_modo);
		
		}
		DeleteCursor(c_parte);
		
		// +++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// Recorro Retroactivos !!!
		// +++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//index rdia(emp, dia, cliente, objetivo),
		WiMsg("aca es retro cursor vigd %ld vigh %ld fecha %.3D %.3D",v_vigdesde,v_vighasta,FmDFld(fm0, W0_FECHAD),FmDFld(fm0, W0_FECHAH));
		v_modo=1; // es retroactivo
		c_retro = CreateCursor(operac|RETRObyRDIA, IO_NOT_LOCK);
		SetCursorFrom(c_retro, FmIFld(fm0, W0_EMP), FmDFld(fm0, W0_FECHAD), NULL_LONG, NULL_SHORT);
		SetCursorTo  (c_retro, FmIFld(fm0, W0_EMP), FmDFld(fm0, W0_FECHAH), MAX_LONG, MAX_SHORT );
		while (FetchCursor(c_retro) != ERROR)
		{
			
			if( LFld(operac|RETRO_NROLEG) < v_vigdesde )
				continue;
				
			if( LFld(operac|RETRO_NROLEG) > v_vighasta )
				continue;

 			// ++++++++++++++++++++++++++++++++++++++++++++
			// proceso solo los dias francos
			// ++++++++++++++++++++++++++++++++++++++++++++
			if( strcmp(SFld(operac|RETRO_CONDIC), _FRANCO) != 0 )
				continue;

			WiMsg("Aca es retro: %s clie %ld nroleg %ld",SFld(operac|RETRO_CONDIC),LFld(operac|RETRO_CLIENTE),LFld(operac|RETRO_NROLEG) );

			s.cantdi=1;
			s.cantfr=0;
			s.fratra=0;
			s.franot=0;

			// ++++++++++++++++
			// busco el regimen
			// ++++++++++++++++
			//GetRegimenEfectivo( FmIFld(fm0, W0_EMP), LFld(operac|RETRO_NROLEG), v_regim, DFld(operac|RETRO_DIA) );

			SetKey(operac|PUESTOS, LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO),
			                       IFld(operac|RETRO_PTOSER),IFld(operac|RETRO_PUESTO));
			if(GetRecord(operac|PUESTOS, THIS_KEY, IO_NOT_LOCK)==ERROR)
			{
				WiMsg("No existe Puesto para el Cliente:%ld Objetivo:%d \n Puesto:%d Nroint:%d (retro)",LFld(operac|RETRO_CLIENTE), IFld(operac|RETRO_OBJETIVO),
			                       IFld(operac|RETRO_PUESTO),IFld(operac|RETRO_NROINT));
				continue;
			}

			// ++++++++++++++++++++++++++++++++++++++
			// empiezo a cargar nodo para retroactivo
			// ++++++++++++++++++++++++++++++++++++++
			s.emp    = IFld(operac|RETRO_EMP);
			s.cliente= LFld(operac|RETRO_CLIENTE);
			s.objet  = IFld(operac|RETRO_OBJETIVO);
			s.legajo = LFld(operac|RETRO_NROLEG);
			s.tpue   = IFld(operac|RETRO_PTOSER);
			//strcpy(s.regim,v_regim);			
            strcpy(s.regim, SFld(operac|PUESTOS_REGIM));
			
			s.dia = DFld(operac|RETRO_DIA);

			// Cantidad de francos, francos Trabajados y no trabajados
			if( strcmp(SFld(operac|RETRO_CONDIC), _FRANCO ) == 0 )
			{
				s.cantfr=1; // cantidad francos reales cargados
			
				// francos Trabajados
				if( TFld(operac|RETRO_HORAENT) != StrToT("0000") && TFld(operac|RETRO_HORASAL) != StrToT("0000") )
					s.fratra=1;
			
				// francos No Trabajados
				if( TFld(operac|RETRO_HORAENT) == StrToT("0000") && TFld(operac|RETRO_HORASAL) == StrToT("0000") )
					s.franot=1;	

			}
			
			/* guardo en la estrutura */
			baseA=CargoNodoA(baseA, &s, v_modo );
			
		}
		DeleteCursor(c_retro);

		// ++++++++++++++++++++++++
		// Fin Retro
		// ++++++++++++++++++++++++
		
		if (baseA!=NULL)
		{
			/* Abro Reporte */
			AbrirReporte();
			MuestroNodoA(baseA);
			CerrarReporte();
			DeleteNodoA(baseA);
			/* limpio pantalla */
			FmClearAllFlds(fm0);
		
			if(strcmp( FmSFld(fm0,W0_SALIDA),"T")!=0 )
				WiMsg("Fin Generacon de Reporte");
		}
		else
		{
			WiMsg("No Se encontraron Datos para la Seleccion");
		}
	}
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// modo 0 : proceso no retroactivo
// modo 1 : proceso retroactivo 
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static punt CargoNodoA(punt baseA, francoPA * s, int modo)
{
	punt aux;
	
	if (baseA==NULL) 
	{
		aux=(punt)malloc(sizeof(bnode));
		if (aux==NULL)
		{
			WiMsg(" NO HAY SUFICIENTE MEMORIA DISPONIBLE  \n LLAME CON URGENCIA A SISTEMAS ");
			return NULL;
		}
	
		aux->pi=NULL;
		aux->pd=NULL;
	
		// Cargar datos
		// sprintf(aux->data.tcom, "%s", s->tcom);
	
		aux->data.emp    = s->emp;
		aux->data.cliente= s->cliente;
		aux->data.objet  = s->objet;
		aux->data.legajo = s->legajo;
		aux->data.tpue   = s->tpue;
		strcpy(aux->data.regim, s->regim );
		
		// datos acumulados

		aux->data.cantdi=s->cantdi;
		
		aux->data.cantfr=s->cantfr;
		aux->data.fratra=s->fratra;
		aux->data.franot=s->franot;
		aux->data.dia   = s->dia;

		aux->data.retro = TRUE;
		if( modo == 0 )
			aux->data.retro = FALSE;

		return aux;
	}

	//++++++++++++
	// Acumula 
	//++++++++++++
	// se fija el modo por tema de retroactivos

	if( modo == 0 )
	{
		if(s->cliente==baseA->data.cliente && s->objet==baseA->data.objet &&
		   s->legajo==baseA->data.legajo && s->tpue==baseA->data.tpue &&
	 	  strcmp(s->regim,baseA->data.regim)==0 && s->dia==baseA->data.dia)
		{
			
			baseA->data.cantdi+=s->cantdi;
			
			baseA->data.cantfr+=s->cantfr;
			baseA->data.fratra+=s->fratra;
			baseA->data.franot+=s->franot;
	
		}
		else 
		{
	  		if( s->legajo <= baseA->data.legajo && s->cliente <= baseA->data.cliente &&
	  			s->objet <= baseA->data.objet )
				baseA->pi=CargoNodoA(baseA->pi, s, modo);
			else
				baseA->pd=CargoNodoA(baseA->pd, s, modo);
		}	
	}
	else
	{
		// **** Retroactivo ****

		if(s->cliente==baseA->data.cliente && s->objet==baseA->data.objet &&
		   s->legajo==baseA->data.legajo && s->tpue==baseA->data.tpue &&
	 	  strcmp(s->regim,baseA->data.regim)==0 && s->dia==baseA->data.dia )
		{
			// ++++++++++++++++++++++++
			// Anula el registro Existe
			// ++++++++++++++++++++++++

			baseA->data.cantdi-=s->cantdi;
			baseA->data.cantfr-=s->cantfr;
			baseA->data.fratra-=s->franot;
			baseA->data.retro = TRUE;
			baseA->data.franot=s->franot;
	
		}
		else 
		{
	  		if( s->legajo <= baseA->data.legajo && s->cliente <= baseA->data.cliente &&
	  			s->objet <= baseA->data.objet )
				baseA->pi=CargoNodoA(baseA->pi, s, modo);
			else
				baseA->pd=CargoNodoA(baseA->pd, s, modo);
		}			
	}

	return baseA;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Agrupa por fecha
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static punt CargoNodoB(punt baseB, francoPA * sB)
{
	punt aux;
	
	if (baseB==NULL) 
	{
		aux=(punt)malloc(sizeof(bnode));
		if (aux==NULL)
		{
			WiMsg(" NO HAY SUFICIENTE MEMORIA DISPONIBLE  \n LLAME CON URGENCIA A SISTEMAS ");
			return NULL;
		}
	
		aux->pi=NULL;
		aux->pd=NULL;
	
		// Cargar datos
		// sprintf(aux->data.tcom, "%s", sB->tcom);
	
		aux->data.emp    = sB->emp;
		aux->data.cliente= sB->cliente;
		aux->data.objet  = sB->objet;
		aux->data.legajo = sB->legajo;
		aux->data.tpue   = sB->tpue;
		strcpy(aux->data.regim, sB->regim );
		
		// datos acumulados

		aux->data.cantdi=sB->cantdi;
		
		aux->data.cantfr=sB->cantfr;
		aux->data.fratra=sB->fratra;
		aux->data.franot=sB->franot;
		aux->data.retro =sB->retro;
		return aux;
	}

	//++++++++++++
	// Acumula 
	//++++++++++++
	// se fija el modo por tema de retroactivos

	if(sB->cliente==baseB->data.cliente && sB->objet==baseB->data.objet &&
	   sB->legajo==baseB->data.legajo && sB->tpue==baseB->data.tpue &&
 	  strcmp(sB->regim,baseB->data.regim)==0 )

	{
		// para que no me pise en el caso que tenga ya un true y venga un false.
		if( sB->retro==TRUE &&  baseB->data.retro==FALSE )
			baseB->data.retro=TRUE;

		baseB->data.cantdi+=sB->cantdi;
		
		baseB->data.cantfr+=sB->cantfr;
		baseB->data.fratra+=sB->fratra;
		baseB->data.franot+=sB->franot;

	}
	else 
	{
  		if( sB->legajo <= baseB->data.legajo && sB->cliente <= baseB->data.cliente &&
  			sB->objet <= baseB->data.objet )
			baseB->pi=CargoNodoB(baseB->pi, sB);
		else
			baseB->pd=CargoNodoB(baseB->pd, sB);
	}

	return baseB;
}


static void MuestroNodoA(punt baseA)
{

	if (baseA->pi!=NULL)
		MuestroNodoA(baseA->pi);

//	if(baseA!=NULL)
//		CopiarNodoB(baseA);

	if (baseA->pd!=NULL)
		MuestroNodoA(baseA->pd);

}
/*
static void CopiarNodoB(punt baseA)
{
	francoPA sB;

	sB.emp    = baseA->data.emp;
	sB.cliente= baseA->data.cliente;
	sB.objet  = baseA->data.objet  ;
	sB.legajo = baseA->data.legajo ;
	sB.tpue   = baseA->data.tpue   ;

	sB.cantdi=; baseA->data.cantdi ;
	sB.cantfr=; baseA->data.cantfr ;
	sB.fratra=; baseA->data.fratra ;
	sB.franot=; baseA->data.franot ;
	sB.dia   = NULL_DATE;
	sB.retro =  baseA->data.retro  ;

	// guardo en la estrutura 
  	baseB=CargoNodoA(baseB,);

}
*/
static void PrintB(punt baseB)
{
	int v_franco_teorico;

	// armo archivo
	
	// ++++++++
	// Cliente
	// ++++++++
	SetKey(bill|CLIENTEbyCLIENTE, baseB->data.cliente );
	if( GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(bill|CLIENTE_RAZSOC, NULL_STR);

	// ++++++++++++++++
	// Tipo de servicio
	// ++++++++++++++++
	SetKey(comerc|TPTOSERbyTIPPTO, baseB->data.tpue );
	if( GetRecord(comerc|TPTOSERbyTIPPTO, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(comerc|TPTOSER_DESCRIP, NULL_STR);

	// ++++++++++++++++
	// Objetivo
	// ++++++++++++++++
	SetKey (comerc|OBJETIVO, baseB->data.cliente, baseB->data.objet );
	if (GetRecord (comerc|OBJETIVO, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(comerc|OBJETIVO_DESCRIP, NULL_STR);

	// ++++++++++++++++
	// Nombre
	// ++++++++++++++++
	SetKey(sue|PERbyEMP, baseB->data.emp, baseB->data.legajo);
	if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR)
		SetFld(sue|PER_APYNOM, NULL_STR);

	// ++++++++++++++++++++++++
	// Calculo francos teoricos
	// ++++++++++++++++++++++++

	v_franco_teorico=0;
	SetKey(comerc|REGIMENbyREGI, baseB->data.regim );
	if(GetRecord(comerc|REGIMENbyREGI, THIS_KEY, IO_NOT_LOCK)!=ERROR)
	{
		if( (IFld(comerc|REGIMEN_DIAS)+IFld(comerc|REGIMEN_DFRAN)) > 0 )
			v_franco_teorico= (int)( baseB->data.cantdi/(IFld(comerc|REGIMEN_DIAS)+
								IFld(comerc|REGIMEN_DFRAN))*IFld(comerc|REGIMEN_DFRAN));		
	}

	// empiezo a armar el archivo!!!

	// datos
 	fprintf(salida, "%d\t" , baseB->data.emp        );
 	fprintf(salida, "%s\t" , FmSFld(fm0, W0_DEMP)  );
 	fprintf(salida, "%ld\t", baseB->data.legajo     );
	fprintf(salida, "%s\t" , SFld(sue|PER_APYNOM)  );
 	fprintf(salida, "%ld\t", baseB->data.cliente); 
 	fprintf(salida, "%s\t" , SFld(bill|CLIENTE_RAZSOC)    ); 
 	fprintf(salida, "%d\t" , baseB->data.objet             );
	fprintf(salida, "%s\t" , SFld(comerc|OBJETIVO_DESCRIP));
 	fprintf(salida, "%d\t" , baseB->data.tpue              );
	fprintf(salida, "%s\t" , SFld(comerc|TPTOSER_DESCRIP) );
 	fprintf(salida, "%s\t" , baseB->data.regim             );

	// cantidades
 	fprintf(salida, "%d\t" , baseB->data.cantdi            );
 	fprintf(salida, "%d\t" , v_franco_teorico  ); // franco teorico
 	fprintf(salida, "%d\t" , baseB->data.cantfr );
 	fprintf(salida, "%d\t" , v_franco_teorico-baseB->data.cantfr ); // franco diferencia
 	fprintf(salida, "%d\t" , baseB->data.fratra );
 	fprintf(salida, "%d\t" , baseB->data.franot );

	if( baseB->data.retro == TRUE )
	 	fprintf(salida, "%B\t" , baseB->data.retro );
	else
	 	fprintf(salida, "%s\t" , NULL_STR );

 	fprintf(salida, "%.3D\t" , baseB->data.dia );

 	fprintf(salida, "%\n"); 

}

static void DeleteNodoA(punt baseA)
{
	if (baseA->pi!=NULL)
		DeleteNodoA(baseA->pi);
	if (baseA->pd!=NULL)
		DeleteNodoA(baseA->pd);
	free(baseA);
}

static void AbrirReporte()
{
	/* Abro Archivo */
	
	if(strcmp( FmSFld(fm0,W0_SALIDA),"A")==0 )
	{
		if (( salida=fopen(FmSFld(fm0, W0_NOMARCH), "w"))==NULL)
		{
			WiDialog(WD_OK, WD_OK, "Error", "No se puede abrir archivo", FmSFld(fm0, W0_NOMARCH));
		  	exit (0);
		}
		
		/* Imprimo Cabecera */
		fprintf(salida, "Legajo Desde: %ld al %ld\n"  ,FmLFld(fm0, W0_VIGDESDE),FmLFld(fm0, W0_VIGHASTA));
		fprintf(salida, "Fecha  Desde: %.3D al %.3D\n",FmDFld(fm0, W0_FECHAD),FmDFld(fm0, W0_FECHAH)    );

		/* Imprimo Titulo */
		fprintf(salida, "Empresa\tDescripcion\t");
		fprintf(salida, "Legajo\tNombre y Apellido\t");
		fprintf(salida, "Cliente\tRazón Social\t");
		fprintf(salida, "Objetivo\tDescripcion\t");
		fprintf(salida, "Puesto\tDescripcion\t");
		fprintf(salida, "Regimen\t");

		fprintf(salida, "Cant.Dias\t");
		fprintf(salida, "Franco Teorico\t");
		fprintf(salida, "Franco Real\t");
		fprintf(salida, "Diferencia\t");
		fprintf(salida, "Francos Trabajados\t");
		fprintf(salida, "Francos No Trabajados\t");
		fprintf(salida, "Mod. Retroactivo\t");
		fprintf(salida, "%\n");

	}
	else
	{
		rp0 = OpenReport("ldfranco", RP_EABORT|RP_NOBEGIN);
		
		RpSetOutput(rp0, *FmSFld(fm0,W0_SALIDA)=='I' ? RP_IO_DEFAULT
													  : RP_IO_TERM, NULL_STR );
		BeginReport(rp0, 1, NULL_STR);
		
		/* Seteo Datos Cabecera */
		
		//RpSetFld (rp0, R_MES , FmSFld(fm0,W0_DMES));
		//RpSetIFld(rp0, R_ANIO, FmIFld(fm0,W0_ANIO));
	}
}

static void CerrarReporte()
{
	if(strcmp( FmSFld(fm0,W0_SALIDA),"A")==0 )
		fclose(salida);
	else
		CloseReport(rp0);
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case W0_EMP:
		break;
	}
	return FM_OK;
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case W0_VIGDESDE:
		if(FmLFld(fm0, W0_VIGDESDE)==NULL_LONG)
		{                                  
			 FmSetFld (fm, W0_DVIGD, "El Primero");
		}
		break;
	case W0_VIGHASTA:
		if(FmLFld(fm0, W0_VIGHASTA)==NULL_LONG)
		{                                  
			 FmSetFld (fm, W0_DVIGH, "El Ultimo");
		}
		break;
	case W0_SALIDA:
		if(strcmp( FmSFld(fm0,W0_SALIDA),"A")==0 )
		{                                  
			 FmSetFld (fm, W0_NOMARCH, "ldfranco.txt");
		}
		else
			 FmSetFld (fm, W0_NOMARCH, NULL_STR);

		break;
	}
	return FM_OK;
}


