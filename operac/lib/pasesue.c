/********************************************************************
*
* MODULE & VERSION : @(#)pasesue.c	1.15
* DATE             : 09/03/20
* TIME             : 13:07:19
*
* CREATED          : 09/06/2000
*
* DESCRIPTION:
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "comdef.h"
#include "comerc.h"
#include "comgral.h"
#include "sue.sch"
#include "comerc.sch"
//#include "lisprose.sch"
#include "billpro.h"
#include "billpro.sch"
#include "bill.sch"
#include "suec.sch"
#include "prosegur.sch"

#define	_TIPO2				2		//Tipo de relacion Centro de costo - Ubicacion
#define	_DEPTO_9990			9990	//Departamento directos
#define	_DEPTO_250			250		//Seguridad de edificios
#define	_DEPTO_260			260		//Patrullas (Brigadas)
#define	_MIN_CONCEPTO		1360
#define	_MAX_CONCEPTO		1415

/* Funciones Privadas */
void CargarCCosto (short emp, long cliente, short objetivo, short  pais, short provin, char *descrip, bool sobrescribe) ;
void CargarCCAgrup (short emp, long cliente, short objetivo, short pais, short prov, bool borra);
void CargarCrossCtroCtoDepto (short emp, long cliente, short objetivo);
void CargarConceptosCtroCto (short emp, long cliente, short objetivo);
void CargarDivDptoCtroCto (short emp, long cliente, short objetivo, short pais, short prov, FILE *farch, bool borra);
void CargarRelcod (short emp, long cliente, short objetivo, short pais, short prov, long local, FILE *farch, bool borra);
void ActualizoProd(short emp, long cliente, short objetivo, short pais, short prov, long local, FILE *farch, bool borra);

static char *AcortarDescrip (char *cadena);
static char *DescripCtroCto (long cliente, short pais, short provin, char *descobj);

void CargarCrossCCto (short emp, long cliente, short objetivo, char *descrip, short pais, short prov, long local, FILE *farch, bool borra) 
{
	/*Esta funcion da de alta el centro de costo correspondiente al cliente-objetivo en DENARIUS
	y graba todas las relaciones correspondientes.
	si borra = TRUE borra los datos viejos
	*/
	
	schema  old, comerc;
	old = CurrentSchema();
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

	if( cliente == NULL_LONG || objetivo == NULL_SHORT || emp == NULL_SHORT || 
		pais == NULL_SHORT || prov == NULL_SHORT || local == NULL_LONG ||
		str_eq (descrip, NULL_STR)) {
			
			if (farch != NULL) {
				fprintf (farch, "\nFUNCION CargarCrossCCto %.3D %.3T : Se intento cargar cross pero falta un dato \nEmpresa %d Cliente %ld Objetivo %d\nPais %d Provincia %d Localidad %ld\nDenominación %s\n",
					emp, cliente, objetivo, pais, prov, local, descrip, Today(), Hour());
			}

			Error ("Se intento cargar cross pero falta un dato \nEmpresa %d Cliente %ld Objetivo %d\nPais %d Provincia %d Localidad %ld\nDenominación %s",
					emp, cliente, objetivo, pais, prov, local, descrip);
	}

	/*Carga el centro de costo */
	CargarCCosto  (emp, cliente, objetivo, pais, prov, descrip, borra);

	/*Carga el tipo de centro de costo  */
	CargarCCAgrup (emp, cliente, objetivo, pais, prov, borra);

	/*Actualizacion de division departamento por ctro. de costo */
	//CargarDivDptoCtroCto (emp, cliente, objetivo, pais, prov, farch,borra);

}


void CargarCCosto (short emp, long cliente, short objetivo, short  pais, short provin, char *descrip, bool sobrescribe) 
{
	/*Esta funcion graba en la tabla SUE_CCOSTO un registro con la empresa
	El centro de costo lo devuelve ArmarCCosto ()
	La descripcion es igual a la del objetivo
	sobrescribe = TRUE		Si ya existe el CCOSTO lo pisa
	sobrescribe = FALSE		Si ya existe el CCOSTO no lo modifica
	*/

	schema  old, sue, comerc;
	long ccosto;
	char desctrocto[100];

	old = CurrentSchema();

	sue  = OpenSchema("sue", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

	ccosto = ArmarCCosto(cliente, objetivo);
	if (!sobrescribe) {
		SetKey (sue|CCOSTObyEMP, emp, ccosto);
		if (GetRecord (sue|CCOSTObyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR  )
			return;
	}

	strcpy (desctrocto, DescripCtroCto (cliente, pais, provin, descrip));

	InitRecord (sue|CCOSTO);
	SetIFld (sue|CCOSTO_EMP, 	emp);
	SetLFld (sue|CCOSTO_COD, 	ArmarCCosto(cliente, objetivo));
	SetFld  (sue|CCOSTO_DENOM,	desctrocto);
	SetIFld (sue|CCOSTO_ASIG,	0);
	PutRecord (sue|CCOSTO);
} 

void CargarCCAgrup (short emp, long cliente, short objetivo, short pais, short prov, bool borra)
{
	/*Esta funcion graba en la tabla sue|CCAGRUP un registro con la empresa
	  El centro de costo lo devuelve ArmarCCosto ()
	  y los agrupamientos a los  que pertenece el centro de costo

	Los agrupamientos se determinan con: 
		la empresa	(parametro)
		pais		(del objetivo)
		provincia	(del objetivo)
	leyendo en la tabla comerc|PROVXCC	 

	borra = TRUE 	Borra los agrupamientos anteriores donde esta grabado el centro de costo
	borra = FALSE   Mantiene los agrupamientos anteriores donde esta grabado el centro de costo
    */

	schema  old, sue, comerc;
	long ccosto;

	old = CurrentSchema();

	sue  = OpenSchema("sue", IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

    ccosto = ArmarCCosto (cliente, objetivo);

	/* Borro los datos viejos */
	if (borra) {
		SetKey (sue|CCAGRUPbyCOS, emp, ccosto, MIN_SHORT);
		while (GetRecord (sue|CCAGRUPbyCOS, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR)
			DelRecord (sue|CCAGRUP);
	}	

	SetKey (comerc|PROVXCCbyPROV, emp, pais, prov, MIN_SHORT);
	while (GetRecord (comerc|PROVXCCbyPROV, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR ) {
		InitRecord  (sue|CCAGRUP);
		SetIFld (sue|CCAGRUP_EMP,		emp);
		SetIFld (sue|CCAGRUP_CODIGO,	IFld (comerc|PROVXCC_TIPCCTO));
		SetLFld (sue|CCAGRUP_CCOSTO,	ccosto);
		PutRecord (sue|CCAGRUP);
	}


}

void CargarRelcod (short emp, long cliente, short objetivo, short pais, short prov, long local, FILE *farch, bool borra)
{
	/*Graba la ubicacion para el centro de costo (tabla sue|RELCOD)
	La ubicacion sale con el pais-provincia-localidad en la tabla billpro.UBIXPROV

	borra = TRUE Borra los datos antes cargados para ese centro de costo
	*/

	schema  old, sue, comerc, billpro;
	long ubicacion, ccosto;  

	old = CurrentSchema();

	sue	= OpenSchema("sue", IO_EABORT);
	billpro	= OpenSchema("billpro",	IO_EABORT);
	comerc	= OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

	/* Busco la ubicación que le corresponde */
	SetKey (billpro|UBIXPROVbyCODPAIS, pais, prov, local);
	if (GetRecord (billpro|UBIXPROVbyCODPAIS, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		if (farch != NULL) {
			fprintf(farch, "\nFUNCION CargarCrossCCto %.3D %.3T: No se encontro Ubicación para Empresa %d Cliente %ld Objetivo %d \nPaís %d Provincia %d Localidad %ld\nNo se graba cross Centro de Costo con Ubicación\n", 
					emp, cliente, objetivo, pais, prov, local, Today(), Hour());
		} 

		Error ("No se encontro Ubicación para Empresa %d Cliente %ld Objetivo %d \nPaís %d Provincia %d Localidad %ld\nNo se graba cross Centro de Costo con Ubicación", 
					emp, cliente, objetivo, pais, prov, local);
		return;
	}

	/*Encontre la ubicacion que corresponde */
	ubicacion = LFld (billpro|UBIXPROV_CODUBI);
    ccosto = ArmarCCosto (cliente, objetivo);

	if (borra) {
		SetKey (sue|RELCODbyCCOSUBI, emp, ccosto, MIN_SHORT);
		while (GetRecord (sue|RELCODbyCCOSUBI, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR)
			DelRecord (sue|RELCOD);
	} 

	/*Graba la ubicacion para el centro de costo*/
	InitRecord (sue|RELCOD);
	SetIFld	(sue|RELCOD_EMP,		emp);
	SetIFld	(sue|RELCOD_TIPO,		_TIPO2);
	SetFld	(sue|RELCOD_CODESTR,	NULL_STR);
	SetLFld (sue|RELCOD_CODCCOS,	ccosto);
	SetLFld (sue|RELCOD_CODUBI,		ubicacion);	
	SetLFld (sue|RELCOD_RELAC,		0);	//No esta relacionada con algun legajo
	PutRecord (sue|RELCOD);
	
}


//void CargarCrossCtroCtoDepto (short emp, long cliente, short objetivo)
//{
//	/******************************************************
//	Graba la tabla de Cross entre Centros de Costo y Departamentos (lisprose|CROSSCC)
//	*************************************************************/
//	schema  old, lisprose;
//	long ccosto;
//
//	old = CurrentSchema();
//	lisprose = OpenSchema("lisprose", IO_EABORT);
//	SwitchToSchema(old);
//
//    ccosto = ArmarCCosto (cliente, objetivo);
//
//	InitRecord (lisprose|CROSSCC);
//	SetIFld	(lisprose|CROSSCC_EMP,		emp);
//	SetLFld	(lisprose|CROSSCC_CCOSTO,	ccosto);
//	SetIFld	(lisprose|CROSSCC_DEPTO,	_DEPTO_9990);
//	PutRecord (lisprose|CROSSCC);	
//	
//}


void CargarConceptosCtroCto (short emp, long cliente, short objetivo)
{
	/****************************************************************
	Esta funcion graba la tabla prosegur.conccos
	con los conceptos que estan entre _MIN_CONCEPTO y _MAX_CONCEPTO
	****************************************************************/

	schema  old, prosegur, comerc, sue, bill;
	long ccosto;
	dbcursor cur;       
	char descrip [200];

	old = CurrentSchema();
	prosegur= OpenSchema("prosegur",	IO_EABORT);
	bill = OpenSchema("bill", IO_EABORT);
	sue	= OpenSchema("sue",		IO_EABORT);
	comerc	= OpenSchema("comerc",	IO_EABORT);
	SwitchToSchema(old);

	/*Leo el cliente para sacar la descripcion */
	SetLFld(bill|CLIENTE_CLIENTE, cliente);
	(void) GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

	ccosto = ArmarCCosto (cliente, objetivo);

	cur = CreateCursor (sue|CONCbyCODIGO, IO_NOT_LOCK);
	SetCursorFrom	(cur, _MIN_CONCEPTO);
	SetCursorTo		(cur, _MAX_CONCEPTO);

	while (FetchCursor (cur) != ERROR) {
		sprintf (descrip, "%s Var.%s", AcortarDescrip (SFld (sue|CONC_DESCRIP)), SFld(bill|CLIENTE_RAZSOC));

		InitRecord (prosegur|CONCCOS);
		SetIFld	(prosegur|CONCCOS_EMP,	emp);
		SetLFld	(prosegur|CONCCOS_COD,	ccosto);
		SetIFld	(prosegur|CONCCOS_CODIGO,	IFld (sue|CONC_CODIGO));
		SetFld	(prosegur|CONCCOS_DESCRIP,	descrip);
		PutRecord (prosegur|CONCCOS);
	}
}


void CargarDivDptoCtroCto (short emp, long cliente, short objetivo, short pais, short prov, FILE *farch, bool borra)
{
	/******************************************************************
	Esta funcion graba en la tabla suec|DIVDEPD que es un cross entre centro de costo
	y division departamento de Aurus.
	*********************************************************************/

	schema  old, comerc, suec;
	long ccosto;
	short  divdepto,divi;

	old = CurrentSchema();
	suec	= OpenSchema("suec",	IO_EABORT);
	comerc	= OpenSchema("comerc",	IO_EABORT);
	SwitchToSchema(old);

    ccosto = ArmarCCosto (cliente, objetivo);

	// Si no encuentro la division que le tengo que cargar
	if(!GetDivDeptoNroccte(ccosto, &divi, &divdepto)) {

		if (farch != NULL) {
			fprintf(farch, "\nFUNCION GetDivDeptoNroccte %.3D %.3T: Empresa %d Cliente %ld Objetivo %d \nNo existe relacion y Division de Aurus \nNo se graba cross Centro de Costo - Division Departamento",
				emp, cliente, objetivo, pais, prov, Today(), Hour());
		}
	
		Error ("Empresa %d Cliente %ld Objetivo %d País %d Provincia %d\nNo existe relacion entre Pais-Provincia y Division de Aurus \nNo se graba cross Centro de Costo - Division Departamento",
				emp, cliente, objetivo, pais, prov);
		return;
	}

	if (borra) {
		SetKey (suec|DIVDEPDbyEMP, emp, ccosto);
		if (GetRecord (suec|DIVDEPDbyEMP, THIS_KEY, IO_LOCK) != ERROR)
			DelRecord (suec|DIVDEPD);
	}

	// Grabo la tabla de cross
	InitRecord (suec|DIVDEPD);
	SetIFld	(suec|DIVDEPD_EMP,	emp);
	SetIFld	(suec|DIVDEPD_DIV,	divi);
	SetIFld	(suec|DIVDEPD_DEP,	divdepto);
	SetLFld	(suec|DIVDEPD_COD,	ccosto);
	PutRecord (suec|DIVDEPD);	

}

static char *AcortarDescrip (char *cadena)
{
	/*Esta funcion corta una descripcion tomando como referencia el valor "-"
	Devuelve la cadena posterior al "-"
	Ejemplo: 
	Viene: 		Sup12-Desc.Fun.Tck.Obj
	Devuelve: 	Desc.Fun.Tck.Obj
	Si la cadena no contiene "-" no acorta, devuelve lo mismo que el parametro
	*/
	
	static char descrip[100];
	int old, max, new;
	bool empieza = FALSE;

	max = (int) strlen (cadena);
	for (old=0, new=0; old < max; old++) {
		if (empieza) {
			descrip[new] = cadena[old];
			new ++;		
		}	
		if (cadena[old] == '-')
			empieza = TRUE;
	}	
	descrip[new] = '\0';
	if (!empieza)
		strcpy (descrip, cadena);

	return descrip;	
}

static char *DescripCtroCto (long cliente, short pais, short provin, char *descobj)
{  
	/*************  
	La descripcion del centro de costo surge de la descripcion del cliente + 
	la descripcion del objetivo + la chapa de la provincia del objetivo
	Para el cliente  uso a lo sumo maxcli caracteres
	Para el objetivo uso maxobj caracteres + lo que no use para el cliente (si la descrip es corta)
	***************/
 
	short tamcli, tamobj, maxcli=19, maxobj=6, tamprov=3;
	char  formato[100], descli[100], despro[100];
	static  char ffinal[100];
	schema  old, bill, sue;

	old = CurrentSchema();
	sue  = OpenSchema("sue", IO_EABORT);
	bill = OpenSchema("bill", IO_EABORT);
	SwitchToSchema(old);

	strcpy (ffinal,	NULL_STR);
	strcpy (descli,	NULL_STR);
	strcpy (despro,	NULL_STR);

	/*Leo el cliente para sacar la descripcion */
	SetLFld(bill|CLIENTE_CLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy (descli,	SFld (bill|CLIENTE_RAZSOC));
	}

	SetKey (sue|PROVIbyPAIS, pais, provin);
	if (GetRecord (sue|PROVIbyPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		strcpy (despro, SFld (sue|PROVI_CHAPA));
	}

	//El cliente muestro a lo sumo maxcli caracteres
	//El objetivo muestro si o si maxcli + lo que no use para el cliente
	tamcli = strlen(descli) < maxcli ? strlen(descli) : maxcli;
	tamobj = maxobj + (maxcli - tamcli);

	//Seteo los % con los tamaños que calcule
	sprintf (formato, "%%.%ds-%%.%ds-%%.%ds", tamcli, tamobj, tamprov);
	sprintf (ffinal, formato, descli, descobj, despro);

	return ffinal;	
}

long UbicacionPorLocalidad (short pais, short prov, long local)
{

	schema  old, billpro;
	long ubicacion=NULL_LONG;

	old = CurrentSchema();

	billpro	= OpenSchema("billpro",	IO_EABORT);
	SwitchToSchema(old);

	/* Busco la ubicación que le corresponde */
	SetKey (billpro|UBIXPROVbyCODPAIS, pais, prov, local);
	if (GetRecord (billpro|UBIXPROVbyCODPAIS, THIS_KEY, IO_NOT_LOCK) != ERROR) {
		ubicacion = LFld (billpro|UBIXPROV_CODUBI);
	}

	return ubicacion;
}

/************************* GetFechaCierreParcial *************************/
DATE GetFechaCierreParcial(int emp)
{
    char fecier[15];
    DATE fechap;

	if (emp == _EMP_PSA) {
		strcpy(fecier, GetValParam(MOD_VIGI, FEC_CIERRE_OPERAC_PARCIAL));
		fechap = StrToD(fecier);
	}
	else {
		strcpy(fecier, GetValParam(MOD_VIGI, FEC_CIERRE_OPERAC_SAPE_PARCIAL));
		fechap = StrToD(fecier);
	}
	return fechap;	
}

/************************* GetFechaCierreOpe *************************/
DATE GetFechaCierreOpe(int emp)
{

	schema  old, com;
    DATE fechap;

	fechap=StrToD(GetParNov(emp, PARNOV_CIEOPERA, 1, Today() ) );

	return fechap;	

}


/************************* PutFechaCierreOpe *************************/
void PutFechaCierreOpe(int emp, DATE fecha)
{
	schema  old, com;
    DATE fechap;

	old = CurrentSchema();
	com	= OpenSchema("comerc",	IO_EABORT);
	SwitchToSchema(old);

	fechap=NULL_DATE;
	SetKey(com|CIEXEMPbyEMP, emp, TIPCIE_OPERAC, MAX_DATE);
	if(GetRecord(com|CIEXEMPbyEMP, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2)!=ERROR)
		fechap = DFld(com|CIEXEMP_FECCIE);

	if (fecha > fechap){
		SetKey(com|CIEXEMPbyEMP, emp, TIPCIE_OPERAC, fecha);
		PutRecord(com|CIEXEMP);
	}
}



/************* Actualizo Producto ***********************/
void ActualizoProd(short emp, long cliente, short objetivo, short pais, short prov, long local, FILE *farch, bool borra)	
{
	//--- Si cambio la provincia cambio en prod para facturar ---//
	schema  old, comerc, bill;
	int newdiv, newdepto;
	dbtable AOBJETIVO;

	old = CurrentSchema();
    bill = OpenSchema("bill", IO_EABORT);
 	comerc = OpenSchema("comerc",  IO_EABORT);
	AOBJETIVO = CreateAlias (comerc|OBJETIVO);

	SetLFld(AOBJETIVO_CLIENTE, cliente );
	SetIFld(AOBJETIVO_OBJET,   objetivo);
	if (GetRecord(AOBJETIVObyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR ) {
		SwitchToSchema(old);
		return;
	}

 	if (LFld(AOBJETIVO_INTERN) == NULL_LONG) {
		SwitchToSchema(old);
 		return;
 	}

	SetKey(bill|PRODbyINTERN, LFld(AOBJETIVO_INTERN));
	if(GetRecord(bill|PRODbyINTERN,THIS_KEY,IO_NOT_LOCK)==ERROR)
	{
		if (farch != NULL) {
			fprintf(farch, "\nFUNCION CargarCrossCCto %.3D %.3T: Tiene intern cargado pero no existe: Empresa %d Cliente %ld Objetivo %d \nPaís %d Provincia %d Localidad %ld\nNo se graba cross Centro de Costo con Ubicación\n", 
					emp, cliente, objetivo, pais, prov, local, Today(), Hour());
		} 

		Error ("Tiene intern cargado pero no existe para Empresa %d Cliente %ld Objetivo %d \nPaís %d Provincia %d Localidad %ld\nNo se graba cross Centro de Costo con Ubicación", 
					emp, cliente, objetivo, pais, prov, local);
		return;
	}

	newdiv = DivProv(IFld(AOBJETIVO_PAIS),NULL_STR, prov);
	newdepto = StrToI(GetValParam(MOD_FAC, DEPTO_INGRESOS));

	//fprintf(stderr, "LEO ANTES %ld %d %d ahora %d %d \n", LFld(bill|PROD_INTERN), IFld(bill|PROD_DIV), IFld(bill|PROD_DEPTO), newdiv, newdepto);
	if (newdiv != IFld(bill|PROD_DIV) || newdepto != IFld(bill|PROD_DEPTO)) {
		SetIFld (bill|PROD_DIV, newdiv);
		SetIFld (bill|PROD_DEPTO, newdepto);
		PutRecord(bill|PROD);
	}
	SwitchToSchema(old);
}

