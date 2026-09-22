/*******************************************************************
* MODULE & VERSION : @(#)revasig.c	1.1
* DATE             : 10/09/28
* TIME             : 15:32:04
* CREATED          :
* DESCRIPTION:
*   Reversion de Asignación de Vigiladores.
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "revasig.fmh"
#include "asist.sch"
#include "sue.sch"
#include "operac.sch"
#undef	TREFER
#undef	TREFER_TABLA
#undef	TREFER_ESQUEMA
#include "bill.sch"
#undef	SERVICIO
#undef	SERVICIO_DESCRIP
#include "comerc.sch"
#include "filial.h"

#define	_SUMA	      100  //Porque tiene dos decimales
#define	_RESTA	     -100  //Porque tiene dos decimales
#define WAR_CERRADO  "El Parte está cerrado al %.3D.\nNo podrá realizar la reversión."

#define _DELREC 0
#define _PUTREC	1
#define REG_USR  "Reversión de Asignación: Usuario %d %s\tFecha %.3D\tHora %T\n"
#define REG_ASIG "%d\t%ld\t%d\t%d\t%d\t%ld\t%s\t%s\t%D\t%T\t%T\t\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%ld\t%D\t%s\t%D\t%D\t%d\t%d\t%D\t%T\t%ld\t%D\t%T\t%ld\t%d\t%s\t%d\t%d\t%d\n"
#define REG_PUESTOS "%ld\t%d\t%d\t%d\t%T\t%T\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%.2f\t%.2f\t%D\t%T\t%ld\t%D\t%T\t%ld\t%s\t%d\t%D\t%D\t%.2f\t%.2f\t%d\t%s\t%d\t%d\t%d\t%s\n" 
#define REG_PARTE "%d\t%ld\t%d\t%D\t%ld\t%T\t%T\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%s\t%D\t%T\t%d\t%D\t%T\t%d\t%d\t%d\t%d\t%D\t%d\t%d\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\n"
#define REG_ASIGH "%d\t%ld\t%d\t%d\t%d\t%ld\t%s\t%s\t%D\t%D\t%T\t%T\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%ld\t%s\t%D\t%D\t%T\t%d\t%D\t%T\t%d\t%D\t%d\t%d\t%d\t%s\t%d\t%d\t%d\t%s\n"
#define REG_EXCEPCION "%d\t%ld\t%d\t%D\t%ld\t%d\t%d\t%.2f\t%.2f\t%.2f\t%s\t%D\t%T\t%d\t%D\t%T\t%d\t%d\t%d\t%d\n"
//#define REG_ASISTEN "%d\t%ld\t%d\t%D\t%.2f\t%d\t%ld\t%D\t%T\t%d\t%D\t%T\t%d\n"
#define REG_ASISTEN "%d\t%ld\t%d\t%D\t%.2f\t%d\t%ld\n"
#define REG_DIASPTIME "%d\t%ld\t%d\t%ld\t%D\t%T\t%T\t%D\t%T\t%d\t%D\t%T\t%d\t%d\t%d\t%d\n"
#define REG_DIASPTIMEH "%d\t%ld\t%d\t%ld\t%D\t%T\t%T\t%D\t%T\t%d\t%D\t%T\t%d\t%d\t%d\t%d\n"

static fm_status after(form, fmfield, int);
static void LeerCliente (long cliente);
static void Lectura();
void RevertirAsignacion (short emp, long nroleg, long cliente, short objetivo, short tippto, short puesto, short nroint, DATE fecasig, bool efectivo);
void ModificarPuesto (long cliente, short objetivo, short tippto, short codint, short dif);
void BorroParte (short emp, long cliente,short  objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig);
void BorroExcepcion (short emp, long cliente,short  objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig);
void BorroInasistencias(short emp, long nroleg, DATE fecasig);
void BorroAsig ();
void CreoNuevoAsig (short emp, long nroleg);
void BorroDiasPTime (short emp, long cliente, short objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig);
void ModificoDiasPTimeH (short emp, long nroleg, DATE fecasig);
void BorroAsigH (short emp, long nroleg, DATE fecasig);
void CopioAsighAsig() ;
void CreoProvisorios (short emp, long nroleg, DATE fecasig);

void ImprimirAsig(short tipreg);
void ImprimirPuestos(short tipreg);
void ImprimirParte(short tipreg);
void ImprimirAsigH(short tipreg);
void ImprimirExcepcion(short tipreg);
void ImprimirDiasPTime(short tipreg);
void ImprimirDiasPTimeH(short tipreg);
void ImprimirAsisten(short tipreg);

bool ControlDatos (int p_row);

// Declaraciones globales
FILE *fp1 = NULL;
form     fm0;
schema   sue, operac, bill, comerc, asist;
bool tiene_asigh = FALSE;
DATE new_fecasig, fechabaja, fecierre, fecierrefil;
char filial[7] = {'\0'};

wcmd(revasig, 1.1 09/28/10)
{
	fm_status cmd;         
	int i;
	char nomarch[50] = {'\0'};

	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	sue    = OpenSchema("sue",    IO_EABORT);
	asist  = OpenSchema("asist",  IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

	fm0 = OpenForm("revasig", FM_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while ((cmd = DoForm(fm0, NULLFP, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_READ:      Lectura(); break;
		case FM_READ_NEXT: Lectura(); break;
		case FM_READ_PREV: Lectura(); break;
		case FM_ADD :
		case FM_UPDATE :   
				BeginTransaction();

			  	sprintf(nomarch, "%s/logs/revasig.txt", ReadEnv("OPERAC"));

			  	if ((fp1 = fopen(nomarch, "a+")) == NULL)
			  		Error("No se pudo crear el archivo de log %s", nomarch);

				fprintf(fp1, REG_USR, GetUid(), UserName(GetUid()),  Today(), Hour());


				for (i=0; i < FmFldLen (fm0, MULTI) && !FmIsNull (fm0, FCLIENTE, i); i++) {
					if (!FmIFld (fm0, REVIERTE, i ))
						continue;

					fecierre = GetFechaCierreOpe(FmIFld (fm0, EMP));

					strcpy(filial, GetFilialDeObj(FmIFld(fm0, EMP), FmLFld(fm0, FCLIENTE, i), FmIFld(fm0, OBJET, i)));	
					fecierrefil = GetFechaCierreFilial(filial);

					// Guardo en fecierre la mayor fecha de cierre
					if (fecierre < fecierrefil)
						fecierre = fecierrefil;

					if (fecierre != NULL_DATE && FmDFld(fm0, FECASIG, i) <= fecierre)
						Error(WAR_CERRADO, fecierre);


					RevertirAsignacion (FmIFld (fm0, EMP, i), 
										FmLFld (fm0, VIGILA, i),
										FmLFld (fm0, FCLIENTE,i),
										FmIFld (fm0, OBJET, i), 
										FmIFld (fm0, TIPPTO, i), 
										FmIFld (fm0, FPUESTO, i), 
										FmIFld (fm0, I_NROINT, i), 
										FmDFld (fm0, FECASIG, i),
										*FmSFld (fm0, EFEC,i) == 'E' );
				}

				EndTransaction();
				fclose(fp1);
			break;
		case FM_IGNORE :
			break;
		}
	}
	
	FinListaXusr();
}

/* Rutina de lectura y pasaje a pantalla */
static void Lectura()
{
	int i=0;
	SetKey (operac|ASIGbyNROLEG, FmIFld (fm0, EMP), FmLFld (fm0, VIGILA), MIN_LONG, MAX_SHORT);
	while (GetRecord (operac|ASIGbyNROLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
		       	continue;
		if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
        	
		LeerCliente (LFld (operac|ASIG_CLIENTE));
		FmSetIFld (fm0, REVIERTE, FALSE, i);
		DbToFm (fm0, FCLIENTE, HSSAL, i);
		FmSetFld (fm0, EFEC, InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)),i);
		FmSetFld (fm0, DPUESTO, GetDescPto(IFld(operac|ASIG_PTOSER)), i);
		i ++ ;
		
	}
}

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case EMP:
	    if (FmChgFld(fm))
		    InicListaXusr(FmIFld(fm0, EMP));
    break;
	case REVIERTE:
			if (FmChgFld(fm))
				if (!ControlDatos (row))
					FmSetIFld(fm, fno, FALSE, row);
	break;
	case FFILIAL:
		if (FmKeyCode(fm) == K_HELP)
			HelpFilial(fm, fno, row);
	break;
	
	}
	return FM_OK;
}

static void LeerCliente (long cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	(void) GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);
}


void RevertirAsignacion (short emp, long nroleg, long cliente, short objetivo, short tippto, 
						short puesto, short nroint, DATE fecasig, bool efectivo)
{           
	
	BeginTransaction ();	
	SetKey (operac|ASIGbyEMP, emp, cliente, objetivo, nroleg, tippto, puesto, nroint);
	if (GetRecord (operac|ASIGbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR )
		WiMsg ("No se pudo revertir la asignacion \n %d %ld %d %ld %d %d %d\n Motivo: No se encontro la asignacion", emp, cliente, objetivo, nroleg, tippto, puesto, nroint);
    
    //Resto en uno la cant. de vigiladores de el puesto que doy de baja
	ModificarPuesto (cliente, objetivo, tippto, nroint, _RESTA);	

		
	BorroParte (emp, cliente, objetivo, nroleg, tippto, puesto, nroint, fecasig);
	BorroExcepcion (emp, cliente, objetivo, nroleg, tippto, puesto, nroint, fecasig);
	BorroDiasPTime (emp, cliente, objetivo, nroleg, tippto, puesto, nroint, fecasig);
	BorroAsig ();   
	if (efectivo) { //Si revierto un puesto efectivo
		CreoNuevoAsig(emp, nroleg);
		BorroAsigH (emp, nroleg, fecasig);
		CreoProvisorios (emp, nroleg, new_fecasig);     //new_facasig se setea en CreoNuevoAsig
		ModificoDiasPTimeH (emp, nroleg, new_fecasig);  //new_facasig se setea en CreoNuevoAsig
		BorroInasistencias(emp, nroleg, fecasig);
	}
	
	FreeTable (operac|ASIG);
	FreeTable (operac|PUESTOS);

	EndTransaction ();	
	
}

void ModificarPuesto (long cliente, short objetivo, short tippto, short codint, short dif) 
{
	SetKey (operac|PUESTOSbyCLIENTE, cliente, objetivo, tippto, codint);
	if (GetRecord (operac|PUESTOSbyCLIENTE, THIS_KEY, IO_LOCK) == ERROR)
		return ;

	SetIFld (operac|PUESTOS_VIGI, IFld (operac|PUESTOS_VIGI) <= 1 ? 0 : 
	                              (IFld (operac|PUESTOS_VIGI) + dif) > 9900 ? 9900 :
	                              (IFld (operac|PUESTOS_VIGI) + dif));

	ImprimirPuestos(_PUTREC);
	PutRecord (operac|PUESTOS);
}

void BorroAsig ()
{    
	/*
	Borra la asignacion que estoy revertiendo
	*/
  	
    ImprimirAsig(_DELREC);  
	DelRecord (operac|ASIG);
}

void CreoNuevoAsig (short emp, long nroleg)
{    

	/*
		Busca en ASIGH el ultimo puesto que tuvo y lo pone como actual en ASIG
	*/
	tiene_asigh = FALSE;
	new_fecasig = fechabaja = MAX_DATE;
	  	                 
	/* Busco el ultimo renglon de ASIGH */
	SetKey (operac|ASIGHbyLEGFEC, emp, nroleg, MAX_DATE, MAX_LONG, MAX_SHORT);
	while (!tiene_asigh && GetRecord (operac|ASIGHbyLEGFEC, PREV_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR ) {
		if (IFld (operac|ASIGH_MOTIVO) == ALTAPARTE )
			continue;

		// Si no es efectivo no lo tomo en cuenta
		if (*SFld (operac|ASIGH_EFECT) != 'E')
			continue;

		tiene_asigh = TRUE;	
		new_fecasig	= DFld (operac|ASIGH_FECALT);
		fechabaja   = DFld (operac|ASIGH_FECBAJ);

		// Sumo uno en la cant. de vig. que ahora pongo activo
		ModificarPuesto (LFld (operac|ASIGH_CLIENTE), IFld (operac|ASIGH_OBJETIVO), IFld (operac|ASIGH_PTOSER), IFld (operac|ASIGH_NROINT), _SUMA); 
	}

	if (!tiene_asigh)
		return;
	
	CopioAsighAsig ();        

	FreeTable (operac|ASIGH);

}

void BorroParte (short emp, long cliente,short  objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig)
{
	/*
	Borra todos los datos de la tabla PARTE para 
	el puesto que doy de baja  segun la fecha de asignacion
	*/
	dbcursor curp;
	
	curp = CreateCursor (operac|PARTEbyLEG, IO_LOCK);

	SetCursorFrom (curp, emp, cliente, objetivo, nroleg, fecasig);
	SetCursorTo   (curp, emp, cliente, objetivo, nroleg, MAX_DATE);
    
    while (FetchCursor (curp) != ERROR ) {
    	if (tippto != IFld (operac|PARTE_PTOSER) ||	
    	 	puesto != IFld (operac|PARTE_PUESTO) ||	
    	 	nroint != IFld (operac|PARTE_NROINT) )
    	 		continue;

		ImprimirParte(_DELREC);
		DelRecord (operac|PARTE);
    }
	
	DeleteCursor (curp);
	FreeTable (operac|PARTE);
}


void BorroExcepcion (short emp, long cliente,short  objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig)
{
	/*
	Borra todos los datos de la tabla EXCEPCION para 
	el puesto que doy de baja  segun la fecha de asignacion
	*/


	dbcursor cure;

	cure = CreateCursor (operac|EXCEPCIONbyEMP, IO_LOCK);

	SetCursorFrom (cure, emp, cliente, objetivo, fecasig,   nroleg,   tippto,     puesto, nroint, MIN_SHORT, MIN_SHORT);
	SetCursorTo   (cure, emp, cliente, objetivo, MAX_DATE, MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT, MAX_SHORT);
    
    while (FetchCursor (cure) != ERROR ) {
    	if (tippto != IFld (operac|EXCEPCION_PTOSER) ||	
    	 	puesto != IFld (operac|EXCEPCION_PUESTO) ||	
    	 	nroint != IFld (operac|EXCEPCION_NROINT) ||
    	 	nroleg != LFld (operac|EXCEPCION_NROLEG) )
    	 		continue;

		ImprimirExcepcion(_DELREC);
		DelRecord (operac|EXCEPCION);
    }

	DeleteCursor (cure);
	FreeTable (operac|EXCEPCION);

}

void BorroDiasPTime (short emp, long cliente, short objetivo, long nroleg, short tippto, short puesto, short nroint, DATE fecasig)
{
	/*
	Borra todos los datos de la tabla DIASPTIME para 
	el puesto que doy de baja  segun la fecha de asignacion
	*/

	SetKey (operac|DIASPTIMEbyEMP, emp, cliente, objetivo, nroleg, tippto, puesto, nroint, fecasig);
	while (GetRecord (operac|DIASPTIMEbyEMP, THIS_KEY|NEXT_KEY|PARTIAL_KEY, IO_LOCK, 7) != ERROR) {
		ImprimirDiasPTime(_DELREC);
		DelRecord (operac|DIASPTIME);
	} 
	FreeTable (operac|DIASPTIME);
}

void BorroInasistencias(short emp, long nroleg, DATE fecasig)
{
	SetIFld(asist|ASISTEN_EMPRE,  emp);
	SetLFld(asist|ASISTEN_NROLEG, nroleg);
	SetDFld(asist|ASISTEN_FECHA,  NULL_DATE);
	SetIFld(asist|ASISTEN_CODNOV, MIN_SHORT);
	while (GetRecord(asist|ASISTENbyINDLEG, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		if (DFld(asist|ASISTEN_FECHA) < fecasig)
			continue;

		ImprimirAsisten(_DELREC);
		DelRecord(asist|ASISTEN);
	}
}

void ModificoDiasPTimeH (short emp, long nroleg, DATE fecasig)
{
	SetKey (operac|DIASPTIMEHbyDIA, emp, nroleg, fecasig, MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT, MIN_SHORT);
	while (GetRecord (operac|DIASPTIMEHbyDIA, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR) {
		InitRecord (operac|DIASPTIME);
		CopyFld (operac|DIASPTIMEH_EMP,	operac|DIASPTIME_EMP);				
		CopyFld (operac|DIASPTIMEH_CLIENTE,	operac|DIASPTIME_CLIENTE);				
		CopyFld (operac|DIASPTIMEH_OBJETIVO,operac|DIASPTIME_OBJETIVO);
		CopyFld (operac|DIASPTIMEH_NROLEG,	operac|DIASPTIME_NROLEG);
		CopyFld (operac|DIASPTIMEH_OBJETIVO,operac|DIASPTIME_OBJETIVO);
		CopyFld (operac|DIASPTIMEH_DIA,	operac|DIASPTIME_DIA);
		CopyFld (operac|DIASPTIMEH_HENT,	operac|DIASPTIME_HENT);
		CopyFld (operac|DIASPTIMEH_HSAL,	operac|DIASPTIME_HSAL);
		CopyFld (operac|DIASPTIMEH_OBJETIVO,operac|DIASPTIME_OBJETIVO);
		CopyFld (operac|DIASPTIMEH_TIPPTO,	operac|DIASPTIME_TIPPTO);
		CopyFld (operac|DIASPTIMEH_PUESTO,	operac|DIASPTIME_PUESTO);
		CopyFld (operac|DIASPTIMEH_NROINT,	operac|DIASPTIME_NROINT);
		
		ImprimirDiasPTime(_PUTREC);
		PutRecord (operac|DIASPTIME);
		ImprimirDiasPTimeH(_DELREC);
		DelRecord (operac|DIASPTIMEH);

	} 

	FreeTable (operac|DIASPTIME);
	FreeTable (operac|DIASPTIMEH);

}

bool ControlDatos (int p_row)
{   
	int i;
	bool valor = FmIFld(fm0, REVIERTE, p_row);
	bool ctrlefe=FALSE;
	bool noefe= FALSE;

	switch(*FmSFld (fm0, EFEC, p_row)) {
		case 'E': 
			ctrlefe = TRUE;
			break;
		case 'P': 
			// Controlo si el efectivo esta en TRUE tengo que revertir todo 
			for (i=0; i < FmFldLen (fm0, MULTI) && !FmIsNull (fm0, FCLIENTE, i); i++) {
				if (*FmSFld (fm0, EFEC, i) == 'E' && FmIFld (fm0, REVIERTE, i)){
					if(FmDFld (fm0, FECASIG, i) <= FmDFld (fm0, FECASIG, p_row)){
						valor = FmIFld(fm0, REVIERTE, i);
						ctrlefe = TRUE;
					}
					else
						noefe= TRUE;
				 }
			}	

            if (noefe && FmIFld(fm0, REVIERTE, p_row))
            	if (WiDialog(WD_YES|WD_NO, WD_NO, "Advertencia", "Este puesto provisorio tiene una fecha anterior a la del puesto efectivo\nEsta seguro de revertirlo tambien?")==WD_NO)
            		return FALSE;
			
			break;
	}

	if (ctrlefe) {
		for (i=0; i < FmFldLen (fm0, MULTI) && !FmIsNull (fm0, FCLIENTE, i); i++) {
			if (FmDFld (fm0, FECASIG, i) < FmDFld (fm0, FECASIG, p_row))
				continue;

			FmSetIFld (fm0, REVIERTE, valor, i);
		} 
	}

	return TRUE;

}	


void BorroAsigH (short emp, long nroleg, DATE fecasig)
{    
  	                 
	/* Borro asigh */
	SetKey (operac|ASIGHbyLEGFEC, emp, nroleg, MIN_DATE, MIN_LONG, MIN_SHORT);
	while (GetRecord (operac|ASIGHbyLEGFEC, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR ) {
		if (IFld (operac|ASIGH_MOTIVO) == ALTAPARTE )
			continue;

		// Solo borro los provisorios
		if (*SFld (operac|ASIGH_EFECT) != 'P')
			continue;

        // Solo con fecha mayor = a la asignacion
		if (DFld (operac|ASIGH_FECALT) < fecasig)
			continue;

		ImprimirAsigH(_DELREC);
		DelRecord (operac|ASIGH);
	}

	FreeTable (operac|ASIGH);
}

void CreoProvisorios (short emp, long nroleg, DATE fecasig)
{  	                 
	/*
	Pone activas las asignaciones provisorias 
	*/

	SetKey (operac|ASIGHbyLEGFEC, emp, nroleg, MIN_DATE , MIN_LONG, MIN_SHORT);
	while (GetRecord (operac|ASIGHbyLEGFEC, NEXT_KEY|PARTIAL_KEY, IO_LOCK, 2) != ERROR ) {
		if (IFld (operac|ASIGH_MOTIVO) == ALTAPARTE )
			continue;

		// Si no es provisorio no lo tengo en cuenta
		if (*SFld (operac|ASIGH_EFECT) != 'P')
			continue;                        

		// La fecha tiene que ser mayor= a la de asignacion o igual fecha de baja
		if (!(DFld (operac|ASIGH_FECALT) >= fecasig || DFld (operac|ASIGH_FECBAJ) == fechabaja))
			continue;                        

		// Sumo uno en la cant. de vig. que ahora pongo activo
		ModificarPuesto (LFld (operac|ASIGH_CLIENTE), IFld (operac|ASIGH_OBJETIVO), IFld (operac|ASIGH_PTOSER), IFld (operac|ASIGH_NROINT), _SUMA); 
		
		CopioAsighAsig ();
    }
	FreeTable (operac|ASIGH);
}

void CopioAsighAsig() 
{
	InitRecord (operac|ASIG);
	CopyFld (operac|ASIGH_EMP,	operac|ASIG_EMP);
	CopyFld (operac|ASIGH_CLIENTE,	operac|ASIG_CLIENTE);
	CopyFld (operac|ASIGH_OBJETIVO,	operac|ASIG_OBJETIVO);
	CopyFld (operac|ASIGH_EMP,	operac|ASIG_EMP);
	CopyFld (operac|ASIGH_NROLEG,	operac|ASIG_NROLEG);
	CopyFld (operac|ASIGH_PTOSER,	operac|ASIG_PTOSER);
	CopyFld (operac|ASIGH_PUESTO,	operac|ASIG_PUESTO);
	CopyFld (operac|ASIGH_NROINT,	operac|ASIG_NROINT);
	CopyFld (operac|ASIGH_VIGIL,	operac|ASIG_VIGIL);
	CopyFld (operac|ASIGH_EFECT,	operac|ASIG_EFECT);
	CopyFld (operac|ASIGH_FECALT,	operac|ASIG_FECASIG);
	CopyFld (operac|ASIGH_HSENT,	operac|ASIG_HSENT);
	CopyFld (operac|ASIGH_HSSAL,	operac|ASIG_HSSAL);
	CopyFld (operac|ASIGH_DIA1,		operac|ASIG_DIA1);
	CopyFld (operac|ASIGH_DIA2,		operac|ASIG_DIA2);
	CopyFld (operac|ASIGH_DIA3,		operac|ASIG_DIA3);
	CopyFld (operac|ASIGH_DIA4,		operac|ASIG_DIA4);
	CopyFld (operac|ASIGH_DIA5,		operac|ASIG_DIA5);
	CopyFld (operac|ASIGH_DIA6,		operac|ASIG_DIA6);
	CopyFld (operac|ASIGH_DIA7,		operac|ASIG_DIA7);
	CopyFld (operac|ASIGH_FFRANCO,	operac|ASIG_FFRANCO);
	CopyFld (operac|ASIGH_NUMFRAN,	operac|ASIG_NUMFRAN);
	CopyFld (operac|ASIGH_REGIM,	operac|ASIG_REGIM);
	CopyFld (operac|ASIGH_TIPODIA,	operac|ASIG_TIPODIA);
	CopyFld (operac|ASIGH_FRANCERO,	operac|ASIG_FRANCERO);
	SetLFld (operac|ASIG_REEMPL,	NULL_LONG);
	SetDFld (operac|ASIG_FECBAJ,	NULL_DATE);
	if (*SFld (operac|ASIG_EFECT) == 'P')
		CopyFld (operac|ASIGH_FECHAS,	operac|ASIG_FECHAS);
	CopyFld (operac|ASIGH_CODROL,	operac|ASIG_CODROL);
	CopyFld (operac|ASIGH_FILA,	operac|ASIG_FILA);
	CopyFld (operac|ASIGH_COLUM,	operac|ASIG_COLUM);

	ImprimirAsigH(_DELREC);
	DelRecord (operac|ASIGH);
	ImprimirAsig(_PUTREC);
	PutRecord (operac|ASIG);

}	

void ImprimirAsig(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.ASIG\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.ASIG\n");
			break;
	}
	
	fprintf(fp1, REG_ASIG, IFld(operac|ASIG_EMP), 
	                       LFld(operac|ASIG_CLIENTE), 
	                       IFld(operac|ASIG_OBJETIVO), 
	                       IFld(operac|ASIG_PTOSER),
 	                       IFld(operac|ASIG_PUESTO),
  	                       LFld(operac|ASIG_NROLEG),
	                       SFld(operac|ASIG_VIGIL),
	                       SFld(operac|ASIG_EFECT),
	                       DFld(operac|ASIG_FECASIG),
	                       TFld(operac|ASIG_HSENT),
	                       TFld(operac|ASIG_HSSAL),
	                       SFld(operac|ASIG_DIA1),
	                       SFld(operac|ASIG_DIA2),
	                       SFld(operac|ASIG_DIA3),
	                       SFld(operac|ASIG_DIA4),
	                       SFld(operac|ASIG_DIA5),
	                       SFld(operac|ASIG_DIA6),
	                       SFld(operac|ASIG_DIA7),
	                       LFld(operac|ASIG_REEMPL),
	                       DFld(operac|ASIG_FFRANCO),
	                       SFld(operac|ASIG_REGIM),
	                       DFld(operac|ASIG_FECHAS),
	                       DFld(operac|ASIG_FECBAJ),
	                       IFld(operac|ASIG_NUMFRAN),
	                       IFld(operac|ASIG_FRANCERO),
	                       DFld(operac|ASIG_CDATE),
	                       TFld(operac|ASIG_CTIME),
	                       LFld(operac|ASIG_CUID),
	                       DFld(operac|ASIG_MDATE),
	                       TFld(operac|ASIG_MTIME),
	                       LFld(operac|ASIG_MUID),
	                       IFld(operac|ASIG_NROINT),
	                       SFld(operac|ASIG_TIPODIA),
	                       IFld(operac|ASIG_CODROL),
	                       IFld(operac|ASIG_FILA),
	                       IFld(operac|ASIG_COLUM),
	                       SFld(operac|ASIG_REGPTO)
           );	
}

void ImprimirAsigH(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.ASIGH\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.ASIGH\n");
			break;
	}
	
	fprintf(fp1, REG_ASIGH, IFld(operac|ASIGH_EMP), 
	                        LFld(operac|ASIGH_CLIENTE), 
	                        IFld(operac|ASIGH_OBJETIVO), 
	                        IFld(operac|ASIGH_PTOSER),
 	                        IFld(operac|ASIGH_PUESTO),
  	                        LFld(operac|ASIGH_NROLEG),
	                        SFld(operac|ASIGH_VIGIL),
	                        SFld(operac|ASIGH_EFECT),
	                        DFld(operac|ASIGH_FECALT),
	                        DFld(operac|ASIGH_FECBAJ),
	                        TFld(operac|ASIGH_HSENT),
	                        TFld(operac|ASIGH_HSSAL),
	                        SFld(operac|ASIGH_DIA1),
	                        SFld(operac|ASIGH_DIA2),
	                        SFld(operac|ASIGH_DIA3),
	                        SFld(operac|ASIGH_DIA4),
	                        SFld(operac|ASIGH_DIA5),
	                        SFld(operac|ASIGH_DIA6),
	                        SFld(operac|ASIGH_DIA7),
	                        IFld(operac|ASIGH_MOTIVO),
	                        LFld(operac|ASIGH_REEMPL),
	                        SFld(operac|ASIGH_REGIM),
	                        DFld(operac|ASIGH_FECHAS),
	                        DFld(operac|ASIGH_CDATE),
	                        TFld(operac|ASIGH_CTIME),
	                        LFld(operac|ASIGH_CUID),
	                        DFld(operac|ASIGH_MDATE),
	                        TFld(operac|ASIGH_MTIME),
	                        LFld(operac|ASIGH_MUID),
                            DFld(operac|ASIGH_FFRANCO),
	                        IFld(operac|ASIGH_NUMFRAN),
	                        IFld(operac|ASIGH_FRANCERO),
	                        IFld(operac|ASIGH_NROINT),
	                        SFld(operac|ASIGH_TIPODIA),
	                        IFld(operac|ASIGH_CODROL),
	                        IFld(operac|ASIGH_FILA),
	                        IFld(operac|ASIGH_COLUM),
	                        SFld(operac|ASIGH_REGPTO)
           );
}

void ImprimirPuestos(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.PUESTOS\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.PUESTOS\n");
			break;
	}

	fprintf(fp1, REG_PUESTOS, LFld(operac|PUESTOS_CLIENTE), 
	                          IFld(operac|PUESTOS_OBJET), 
	                          IFld(operac|PUESTOS_TIPPTO),
 	                          IFld(operac|PUESTOS_PUESTO),
  	                          TFld(operac|PUESTOS_HINICIO),
	                          TFld(operac|PUESTOS_HFINAL),
  	                          SFld(operac|PUESTOS_DIA1),
	                          SFld(operac|PUESTOS_DIA2),
	                          SFld(operac|PUESTOS_DIA3),
	                          SFld(operac|PUESTOS_DIA4),
	                          SFld(operac|PUESTOS_DIA5),
	                          SFld(operac|PUESTOS_DIA6),
	                          SFld(operac|PUESTOS_DIA7),
  	                          SFld(operac|PUESTOS_REGIM),
	                          IFld(operac|PUESTOS_CODINT),
	                          FFld(operac|PUESTOS_CANTVIG) > 0 ? FFld(operac|PUESTOS_CANTVIG)/100 : 0,
	                          FFld(operac|PUESTOS_VIGI) > 0 ? FFld(operac|PUESTOS_VIGI)/100 : 0,
	                          DFld(operac|PUESTOS_CDATE),
	                          TFld(operac|PUESTOS_CTIME),
	                          LFld(operac|PUESTOS_CUID),
	                          DFld(operac|PUESTOS_MDATE),
	                          TFld(operac|PUESTOS_MTIME),
	                          LFld(operac|PUESTOS_MUID),
                              SFld(operac|PUESTOS_CODFREC),
	                          IFld(operac|PUESTOS_HORAPT),
	                          DFld(operac|PUESTOS_FINICIO),
	                          DFld(operac|PUESTOS_FFINAL),
	                          FFld(operac|PUESTOS_HSNORM) > 0 ? FFld(operac|PUESTOS_HSNORM) : 0,
	                          FFld(operac|PUESTOS_HSEXTR) >0 ? FFld(operac|PUESTOS_HSEXTR) : 0,
	                          IFld(operac|PUESTOS_CANTPUE),
	                          SFld(operac|PUESTOS_TIPODIA),
	                          IFld(operac|PUESTOS_NEWINT),
	                          IFld(operac|PUESTOS_PADREINT),
	                          IFld(operac|PUESTOS_CODMOT),
	                          SFld(operac|PUESTOS_SUBREG)
           );
}

void ImprimirDiasPTime(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.DIASPTIME\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.DIASPTIME\n");
			break;
	}
	
	fprintf(fp1, REG_DIASPTIME, IFld(operac|DIASPTIME_EMP),
	         					LFld(operac|DIASPTIME_CLIENTE), 
	                            IFld(operac|DIASPTIME_OBJETIVO), 
	                            LFld(operac|DIASPTIME_NROLEG),
 	                            DFld(operac|DIASPTIME_DIA),
  	                            TFld(operac|DIASPTIME_HENT),
	                            TFld(operac|DIASPTIME_HSAL),
	                            DFld(operac|DIASPTIME_CDATE),
	                            TFld(operac|DIASPTIME_CTIME),
	                            LFld(operac|DIASPTIME_CUID),
	                            DFld(operac|DIASPTIME_MDATE),
	                            TFld(operac|DIASPTIME_MTIME),
	                            LFld(operac|DIASPTIME_MUID),
                                IFld(operac|DIASPTIME_TIPPTO),
	                            IFld(operac|DIASPTIME_PUESTO),
	                            IFld(operac|DIASPTIME_NROINT)
           );
}

void ImprimirDiasPTimeH(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.DIASPTIMEH\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.DIASPTIMEH\n");
			break;
	}
	
	fprintf(fp1, REG_DIASPTIMEH, IFld(operac|DIASPTIMEH_EMP),
	         					 LFld(operac|DIASPTIMEH_CLIENTE), 
	                             IFld(operac|DIASPTIMEH_OBJETIVO), 
	                             LFld(operac|DIASPTIMEH_NROLEG),
 	                             DFld(operac|DIASPTIMEH_DIA),
  	                             TFld(operac|DIASPTIMEH_HENT),
	                             TFld(operac|DIASPTIMEH_HSAL),
	                             DFld(operac|DIASPTIMEH_CDATE),
	                             TFld(operac|DIASPTIMEH_CTIME),
	                             LFld(operac|DIASPTIMEH_CUID),
	                             DFld(operac|DIASPTIMEH_MDATE),
	                             TFld(operac|DIASPTIMEH_MTIME),
	                             LFld(operac|DIASPTIMEH_MUID),
                                 IFld(operac|DIASPTIMEH_TIPPTO),
	                             IFld(operac|DIASPTIMEH_PUESTO),
	                             IFld(operac|DIASPTIMEH_NROINT)
           );
}

void ImprimirParte(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.PARTE\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.PARTE\n");
			break;
	}
	
	fprintf(fp1, REG_PARTE, IFld(operac|PARTE_EMP),
	                        LFld(operac|PARTE_CLIENTE),
	                        IFld(operac|PARTE_OBJETIVO),
	                        DFld(operac|PARTE_DIA),
	                        LFld(operac|PARTE_NROLEG),
                            TFld(operac|PARTE_HORAENT),
	                        TFld(operac|PARTE_HORASAL),
 	                        IFld(operac|PARTE_CONFIR),
  	                        FFld(operac|PARTE_HSNOR) > 0 ? FFld(operac|PARTE_HSNOR) : 0,
	                        FFld(operac|PARTE_HS50) > 0 ? FFld(operac|PARTE_HS50) : 0,
	                        FFld(operac|PARTE_HS100F) > 0 ? FFld(operac|PARTE_HS100F) : 0,
	                        FFld(operac|PARTE_HS100FE) > 0 ? FFld(operac|PARTE_HS100FE) : 0,
	                        SFld(operac|PARTE_CONDIC),
                            DFld(operac|PARTE_CDATE),
	                        TFld(operac|PARTE_CTIME),
	                        LFld(operac|PARTE_CUID),
	                        DFld(operac|PARTE_MDATE),
	                        TFld(operac|PARTE_MTIME),
	                        LFld(operac|PARTE_MUID),
                            IFld(operac|PARTE_CONFEX),
	                        IFld(operac|PARTE_PTOSER),
	                        IFld(operac|PARTE_PUESTO),
	                        DFld(operac|PARTE_FECGEN),
	                        IFld(operac|PARTE_NROINT),
	                        IFld(operac|PARTE_CODAUS),
	                        LFld(operac|PARTE_LIQDENA),
	                        LFld(operac|PARTE_LIQFAC),
	                        LFld(operac|PARTE_ASICBLE),
	                        LFld(operac|PARTE_NROFAC),
	                        LFld(operac|PARTE_LIQDENUS),
	                        LFld(operac|PARTE_ASICBLUS)
           );
}

void ImprimirExcepcion(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA OPERAC.EXEPCION\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA OPERAC.EXCEPCION\n");
			break;
	}
	
	fprintf(fp1, REG_EXCEPCION, IFld(operac|EXCEPCION_EMP),
	                            LFld(operac|EXCEPCION_CLIENTE),
	                            IFld(operac|EXCEPCION_OBJETIVO),
	                            DFld(operac|EXCEPCION_DIA),
	                            LFld(operac|EXCEPCION_NROLEG),
  	                            IFld(operac|EXCEPCION_CONDIC),
  	                            IFld(operac|EXCEPCION_MOTIVO),
  	                            FFld(operac|EXCEPCION_HORAS) > 0 ? FFld(operac|EXCEPCION_HORAS) : 0,
	                            FFld(operac|EXCEPCION_HS50) > 0 ? FFld(operac|EXCEPCION_HS50) : 0,
	                            FFld(operac|EXCEPCION_HS100) > 0 ? FFld(operac|EXCEPCION_HS100) : 0,
                                SFld(operac|EXCEPCION_OBS),
                                DFld(operac|EXCEPCION_CDATE),
	                            TFld(operac|EXCEPCION_CTIME),
	                            LFld(operac|EXCEPCION_CUID),
	                            DFld(operac|EXCEPCION_MDATE),
	                            TFld(operac|EXCEPCION_MTIME),
	                            LFld(operac|EXCEPCION_MUID),
  	                            IFld(operac|EXCEPCION_PTOSER),
	                            IFld(operac|EXCEPCION_PUESTO),
	                            IFld(operac|EXCEPCION_NROINT)
           );
}

void ImprimirAsisten(short tipreg)
{
	switch(tipreg) {
		case _PUTREC:
			fprintf(fp1, "ALTA ASIST.ASISTEN\n");
			break;
		case _DELREC:
			fprintf(fp1, "BAJA ASIST.ASISTEN\n");
			break;
	}
	
	fprintf(fp1, REG_ASISTEN, IFld(asist|ASISTEN_EMPRE),
	                          LFld(asist|ASISTEN_NROLEG),
	                          IFld(asist|ASISTEN_CODNOV),
	                          DFld(asist|ASISTEN_FECHA),
	                          FFld(asist|ASISTEN_VALOR) > 0 ? FFld(asist|ASISTEN_VALOR) : 0,
  	                          IFld(asist|ASISTEN_JUSTIF),
  	                          LFld(asist|ASISTEN_NROLIQ)
//                              DFld(asist|ASISTEN_CDATE),
//	                          TFld(asist|ASISTEN_CTIME),
//	                          LFld(asist|ASISTEN_CUID),
//	                          DFld(asist|ASISTEN_MDATE),
//	                          TFld(asist|ASISTEN_MTIME),
//	                          LFld(asist|ASISTEN_MUID)
           );
}











































