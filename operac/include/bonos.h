/********************************************************************
*
* MODULE & VERSION : @(#)bonos.h	1.6
* DATE             : 02/08/29
* TIME             : 08:55:45
*
* CREATED          : 27/05/2002
*
* DESCRIPTION:	
*	Definiciones para el modulo de Bonos
********************************************************************/
#ifndef	__BONOS_H
#define __BONOS_H 

#define BONO_1_NOC			1		// Tipo de bono 1. Bono Nocturno
#define BONO_2				2		// Tipo de bono 2
#define BONO_3				3		// Tipo de bono 3

#define O_XCLIOBJ			1		// Ordenamiento por bono/cliente/objetivo/legajo
#define O_XLEGAJO			2		// Ordenamiento por bono/legajo/cliente/objetivo

#define MIN_DIAS_1_OBJ		16		// Minimo de dias en 1 obejtivo
#define MIN_DIAS_N_OBJ		21		// Minimo de dias en n objetivos

#define MAXLEG				160000	// Tamaño de la lista

/* Estructuras */
typedef struct svigil { 
	int  emp;		// empresa 
	long cli;		// cliente 
	int  obj;		// objetivo 
	int  bono;		// bono asociado al cliente/objetivo 
    long bvalor;	// valor del bono vendido
	long vigvalor;  // valor del bono a pagar al vigilador
    long bcant;		// cantidad vendida 

	long nroleg;    // legajo del vigilador
	int  cdiasT;	// cant de dias trabajados en cliente/objetivo   (OPERAC.PARTE)
	int  cdiasL;	// cant de dias de licencias en cliente/objetivo (OPERAC.PARTE)
	int  cdiasA;	// cant de dias ausente en cliente/objetivo      (OPERAC.PARTE)
	int  cdiasF;	// cant de dias franco en cliente/objetivo       (OPERAC.PARTE)
	int  cdiasV;	// cant de dias vacaciones en cliente/objetivo   (OPERAC.PARTE)
	int  diasnoc;   // cant de dias que trabaja horario nocturno     (OPERAC.PARTE)

	// datos para el calculo
	int  qbase;   	// cantidad base para el calculo del bono (q)
	long ibase;		// importe  base para el calculo del bono  (v)
	int  pbase;		// porcentaje base para el calculo del bono (p)
	long impfinal;	// importe a pagar ( q * v * ( p / 100 ) )
} SVigil;

typedef struct  sbono {
	int  emp;		// empresa 	(key)
	long cli;		// cliente  (key)
	int  obj;		// objetivo (key)
	int  bono;      // bono vendido al cliente/objetivo
	long bcant;		// cantidad vendida
	long bvalor;  	// valor del bono
	long vigvalor;  // valor del bono a pagar al vigilador
} SBono;

// Funciones publicas
void InicializarListaBonos();
void VolverInicioListaBonos();
bool ProximoListaBonos(SVigil *s);
void OrdCliOBjListaBonos();
void OrdLegajoListaBonos();
void CargarListaBonos(int bonod, int bonoh, int listapor, int emp, long clid, long clih, int objd, int objh, DATE fechad, DATE fechah, long vigd, long vigh);
bool EstaVaciaListaBonos();

#endif
