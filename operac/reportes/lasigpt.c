/* lasigpt.c LISTADO DE ASIGNACIÓN DE VIGILADORES PART TIME */
/* -------------------------------------------------------- */
#include <ideafix.h>
#include "lasigpt.fmh"
#include "lasigpt1.rph"	/* OjO el rph del lasigpt1 y lasigpt2 deben ser identicos */
#include "operac.sch"
#include "operac.h"
#include "sue.sch"
#undef  TREFER
#undef  TREFER_TABLA
#undef  TREFER_ESQUEMA
#include "bill.sch"
#undef  SERVICIO
#undef  SERVICIO_DESCRIP
#include "comerc.sch"
#include "comerc.h"

#define TERMINAL           1
#define IMPRESORA          2
#define ARCHIVO            3
#define INFO_ACTUAL        "A"
#define INFO_HISTORICA     "H"
#define ORDEN_CLIOBJ       1
#define ORDEN_VIGILADOR    2
#define VIGILADOR_PARTTIME "P"

private void ProcesarConsulta();
private void AbrirArchivo();
private void SetearSeleccion(report rp0);
private void ProcesarASIG();
private void ProcesarASIGH(bool procesamiento_fijo, int empresa, long cliente, int objetivo, long vigilador,
								DATE fecha_desde, DATE fecha_hasta);
private void ImprimirDetaASIG(report rp0);
private void ImprimirDetaASIGH(report rp0);
private void LeerCliente(long cliente);
private void LeerObjetivo(long cliente,int objetivo);
private void LeerVigilador(int emp,long nroleg);
private void LeerPuestos(long cliente, int objetivo, int ptoser, int puesto);
private bool RegValido(int empresa, long cliente, int objetivo, long vigilador, char * tipo_vigilador);
private report AbrirReporte();
private rpfield DescriptorRP(char * nom_dia);

form     fm0;
report   rp0;
schema   scOPERAC,scBILL,scCOMERC,scSUE;
dbcursor cursor_ASIGbyORDEN,   cursor_ASIGbyORDEN1, cursor_ASIGbyORDEN2, cursor_ASIGHbyORDEN,
		 cursor_ASIGHbyORDEN1, cursor_ASIGHbyORDEN2;
bool     cursor_ASIGbyORDEN1_creado,  cursor_ASIGbyORDEN2_creado, cursor_ASIGHbyORDEN_creado,
		 cursor_ASIGHbyORDEN1_creado, cursor_ASIGHbyORDEN2_creado;
FILE     * archivo_excel;
bool     salida_archivo;
long     cliente_corriente, vigilador_corriente;
int      objetivo_corriente, error_rp;
bool     fin_datosASIG, fin_datosASIGH;
char     nom_dia[20];

wcmd(lasigpt, 1.12  23/09/98)
{
	fm_status	cmd;

	scCOMERC = OpenSchema("comerc",IO_EABORT);
	scBILL   = OpenSchema("bill",IO_EABORT);
	scSUE    = OpenSchema("sue",IO_EABORT);
	scOPERAC = OpenSchema("operac",IO_EABORT);
	fm0      = OpenForm("lasigpt",FM_EABORT);

	cursor_ASIGbyORDEN1_creado  = FALSE;
	cursor_ASIGbyORDEN2_creado  = FALSE;
	cursor_ASIGHbyORDEN1_creado = FALSE;
	cursor_ASIGHbyORDEN2_creado = FALSE;

	while ((cmd = DoForm(fm0,NULLFP,NULLFP)) != FM_EXIT) {
		if (cmd == FM_UPDATE)
			ProcesarConsulta();
	}
	if (cursor_ASIGbyORDEN1_creado) {
		DeleteCursor(cursor_ASIGbyORDEN1);
	}
	if (cursor_ASIGbyORDEN2_creado) {
		DeleteCursor(cursor_ASIGbyORDEN2);
	}
	if (cursor_ASIGHbyORDEN1_creado) {
		DeleteCursor(cursor_ASIGHbyORDEN1);
	}
	if (cursor_ASIGHbyORDEN2_creado) {
		DeleteCursor(cursor_ASIGHbyORDEN2);
	}
	return;
}

private void ProcesarConsulta()
{
	salida_archivo = str_eq(FmSFld(fm0,SALIDA),"A");
	error_rp       = !ERROR;

	if (salida_archivo)
		AbrirArchivo();
	else {
		rp0=AbrirReporte();
		SetearSeleccion(rp0);
	}
	if (str_eq(FmSFld(fm0,T_INFO),INFO_ACTUAL))
		ProcesarASIG();
	else
		ProcesarASIGH(TRUE, FmIFld(fm0,C_EMP), NULL_LONG, NULL_SHORT, NULL_LONG, FmDFld(fm0,F_D), FmDFld(fm0,F_H));

	if (salida_archivo)
		fclose(archivo_excel);
	else {
		EndReport(rp0);
		CloseReport(rp0);
	}
}

private report AbrirReporte()
{
	report rp0;

	rp0 = OpenReport(FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ ? "lasigpt1" : "lasigpt2" , RP_EABORT|RP_NOBEGIN);
	RpSetOutput(rp0, str_eq(FmSFld(fm0,SALIDA),"T") ? RP_IO_TERM : RP_IO_PRINTER, NULL_STR);
	BeginReport(rp0,1,NULL_STR);
	return rp0;
}

private void AbrirArchivo()
{
	if ((archivo_excel=fopen(FmSFld(fm0,X_ARCHIVO),"w"))==NULL)
		Error("No se pudo abrir el archivo [%s] [%s]", FmSFld(fm0,X_ARCHIVO),"para grabación salida fmt excel\n");
}

private void SetearSeleccion(report rp0)
{
	if (str_eq(FmSFld(fm0,SALIDA), "I")) {
		if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ)
			RpSetFld (rp0,RP_TITULO, "[1;4mVIGILADORES ASIGNADOS POR CLIENTE/OBJETIVO [0m");
		else
			RpSetFld (rp0,RP_TITULO, "[1;4mASIGNACIONES CLIENTE/OBJETIVO POR VIGILADOR[0m");
	}
	else {
		if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ)
			RpSetFld (rp0,RP_TITULO, "VIGILADORES ASIGNADOS POR CLIENTE/OBJETIVO");
		else
			RpSetFld (rp0,RP_TITULO, "ASIGNACIONES CLIENTE/OBJETIVO POR VIGILADOR");
	}
	RpSetIFld(rp0,RP_CEMP,   FmIFld(fm0,C_EMP));
	RpSetFld (rp0,RP_XEMP,   FmSFld(fm0,X_EMP));
	RpSetFld (rp0,RP_XTINFO, FmSFld(fm0,X_TINFO));
	RpSetLFld(rp0,RP_CCLID,  FmLFld(fm0,C_CLID));
	RpSetFld (rp0,RP_XCLID,  FmSFld(fm0,X_CLID));
	RpSetLFld(rp0,RP_CCLIH,  FmLFld(fm0,C_CLIH));
	RpSetFld (rp0,RP_XCLIH,  FmSFld(fm0,X_CLIH));
	RpSetIFld(rp0,RP_OBJD,   FmIFld(fm0,OBJ_D));
	RpSetFld (rp0,RP_XOBJD,  FmSFld(fm0,X_OBJD));
	RpSetIFld(rp0,RP_OBJH,   FmIFld(fm0,OBJ_H));
	RpSetFld (rp0,RP_XOBJH,  FmSFld(fm0,X_OBJH));
	RpSetLFld(rp0,RP_VIGD,   FmLFld(fm0,VIG_D));
	RpSetFld (rp0,RP_XVIGD,  FmSFld(fm0,X_VIGD));
	RpSetLFld(rp0,RP_VIGH,   FmLFld(fm0,VIG_H));
	RpSetFld (rp0,RP_XVIGH,  FmSFld(fm0,X_VIGH));
	RpSetDFld(rp0,RP_FD,     FmDFld(fm0,F_D));
	RpSetDFld(rp0,RP_FH,     FmDFld(fm0,F_H));
	return;
}

private void ProcesarASIG()
{
	if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ) {
		if (!cursor_ASIGbyORDEN1_creado) {
			cursor_ASIGbyORDEN1 = CreateCursor(ASIGbyFECHA,IO_NOT_LOCK);
			cursor_ASIGbyORDEN1_creado = TRUE;
		}
		cursor_ASIGbyORDEN=cursor_ASIGbyORDEN1;
	}
	else {
		if (!cursor_ASIGbyORDEN2_creado) {
			cursor_ASIGbyORDEN2 = CreateCursor(ASIGbyLEGFEC,IO_NOT_LOCK);
			cursor_ASIGbyORDEN2_creado = TRUE;
		}
		cursor_ASIGbyORDEN = cursor_ASIGbyORDEN2;
	}
	if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ) {
		SetCursorFrom(cursor_ASIGbyORDEN, FmIFld(fm0,C_EMP), FmIsNull(fm0,C_CLID) ? MIN_LONG : FmLFld(fm0,C_CLID),
						!FmIsNull(fm0,C_CLID) && FmLFld(fm0,C_CLID) == FmLFld(fm0,C_CLIH) &&
						!FmIsNull(fm0,OBJ_D) ? FmIFld(fm0,OBJ_D) : MIN_SHORT, MIN_DATE, MIN_LONG);
		SetCursorTo  (cursor_ASIGbyORDEN, FmIFld(fm0,C_EMP), FmIsNull(fm0,C_CLIH) ? MAX_LONG : FmLFld(fm0,C_CLIH),
						!FmIsNull(fm0,C_CLIH) && FmLFld(fm0,C_CLID) == FmLFld(fm0,C_CLIH) &&
						!FmIsNull(fm0,OBJ_H) ? FmIFld(fm0,OBJ_H) : MAX_SHORT, MAX_DATE, MAX_LONG);
	}
	else {
		SetCursorFrom(cursor_ASIGbyORDEN, FmIFld(fm0,C_EMP), FmIsNull(fm0,VIG_D) ? MIN_LONG : FmLFld(fm0,VIG_D),
											MIN_DATE, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cursor_ASIGbyORDEN, FmIFld(fm0,C_EMP), FmIsNull(fm0,VIG_D) ? MAX_LONG : FmLFld(fm0,VIG_D),
											MAX_DATE, MAX_LONG, MAX_SHORT);
	}
	MoveCursorFirst(cursor_ASIGbyORDEN);
	fin_datosASIG = (FetchCursor(cursor_ASIGbyORDEN)==ERROR);
	while (!fin_datosASIG && !error_rp) {
		bool continuar = FALSE;
		if (Today() < DFld(ASIG_FECASIG) || (Today() > DFld(ASIG_FECHAS)  && DFld(ASIG_FECHAS) != NULL_DATE))
			continuar = TRUE;
		if (!continuar && !RegValido(IFld(ASIG_EMP), LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO),
									 LFld(ASIG_NROLEG),SFld(ASIG_VIGIL)))
			continuar = TRUE;
		if (continuar) {
			fin_datosASIG = (FetchCursor(cursor_ASIGbyORDEN)==ERROR);
			continue;
		}
		cliente_corriente   = LFld(ASIG_CLIENTE);
		objetivo_corriente  = IFld(ASIG_OBJETIVO);
		vigilador_corriente = LFld(ASIG_NROLEG);

		LeerCliente(LFld(ASIG_CLIENTE));
		LeerObjetivo(LFld(ASIG_CLIENTE),IFld(ASIG_OBJETIVO));
		LeerVigilador(IFld(ASIG_EMP),LFld(ASIG_NROLEG));

		if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ) {
			if (salida_archivo) {
				fprintf(archivo_excel,"Cliente: %ld %s\t%d %s\n", LFld(ASIG_CLIENTE),
						SFld(scBILL|CLIENTE_RAZSOC), IFld(ASIG_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
			}
			else {
				RpSetLFld(rp0,RP_CCLI, LFld(ASIG_CLIENTE));
				RpSetFld (rp0,RP_XCLI, SFld(scBILL|CLIENTE_RAZSOC));
				RpSetIFld(rp0,RP_OBJ,  IFld(ASIG_OBJETIVO));
				RpSetFld (rp0,RP_XOBJ, SFld(scCOMERC|OBJETIVO_DESCRIP));

				error_rp=DoReport(rp0,LIN_CLIOBJ);
			}
		}
		else {
			if (salida_archivo) {
				fprintf(archivo_excel,"Vigilador: %ld %s\n", LFld(ASIG_NROLEG), SFld(scSUE|PER_APYNOM));
			}
			else {
				RpSetLFld(rp0,RP_VIG,  LFld(ASIG_NROLEG));
				RpSetFld (rp0,RP_XVIG, SFld(scSUE|PER_APYNOM));

				error_rp=DoReport(rp0,LIN_VIGILADOR);
			}
		}
		while(!fin_datosASIG && !error_rp &&
		((FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ && cliente_corriente == LFld(ASIG_CLIENTE) && 
		  objetivo_corriente  == IFld(ASIG_OBJETIVO)) || vigilador_corriente==LFld(ASIG_NROLEG))) {
			bool continuar = FALSE;

			if (Today() < DFld(ASIG_FECASIG) || (Today()>DFld(ASIG_FECHAS) && DFld(ASIG_FECHAS)!=NULL_DATE))
				continuar = TRUE;
			if (!continuar && !RegValido(IFld(ASIG_EMP), LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO),
										 LFld(ASIG_NROLEG), SFld(ASIG_VIGIL)))
				continuar = TRUE;
			if (continuar) {
				fin_datosASIG = (FetchCursor(cursor_ASIGbyORDEN)==ERROR);
				continue;
			}
			if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ)
				LeerVigilador(IFld(ASIG_EMP),LFld(ASIG_NROLEG));
			else {
				LeerCliente(LFld(ASIG_CLIENTE));
				LeerObjetivo(LFld(ASIG_CLIENTE),IFld(ASIG_OBJETIVO));
			}
			if (DFld(ASIG_FECASIG) > FmDFld(fm0,F_D)) {
				ProcesarASIGH(FALSE, IFld(ASIG_EMP), LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO), LFld(ASIG_NROLEG),
							  FmDFld(fm0,F_D), DFld(ASIG_FECASIG));
			}
			ImprimirDetaASIG(rp0);
			fin_datosASIG = (FetchCursor(cursor_ASIGbyORDEN) == ERROR);
		}
		if (!salida_archivo)
			error_rp = DoReport(rp0,LIN_FIN);
	}
	return;
}

private void ProcesarASIGH(bool procesamiento_fijo, int empresa, long cliente, int objetivo, long vigilador,
						   DATE fecha_desde, DATE fecha_hasta)
{
	if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
		if (!cursor_ASIGHbyORDEN1_creado) {
			cursor_ASIGHbyORDEN1 = CreateCursor(ASIGHbyFECHABAJ,IO_NOT_LOCK);
			cursor_ASIGHbyORDEN1_creado = TRUE;
		}
		cursor_ASIGHbyORDEN = cursor_ASIGHbyORDEN1;
	}
	else {
		if (!cursor_ASIGHbyORDEN2_creado) {
			cursor_ASIGHbyORDEN2 = CreateCursor(ASIGHbyLEGFEC,IO_NOT_LOCK);
			cursor_ASIGHbyORDEN2_creado = TRUE;
		}
		cursor_ASIGHbyORDEN=cursor_ASIGHbyORDEN2;
	}
	if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
		SetCursorFrom(cursor_ASIGHbyORDEN, FmIFld(fm0, C_EMP), cliente != NULL_LONG ? cliente :
					 (FmIsNull(fm0, C_CLID) ? MIN_LONG : FmLFld(fm0, C_CLID)),
					 objetivo != NULL_SHORT ? objetivo :
					 (!FmIsNull(fm0, C_CLID) && FmLFld(fm0, C_CLID) == FmLFld(fm0, C_CLIH) &&
					  !FmIsNull(fm0, OBJ_D) ? FmIFld(fm0, OBJ_D) : MIN_SHORT), 
					 cliente != NULL_LONG ? fecha_desde : MIN_DATE, MIN_LONG, MIN_SHORT, MIN_SHORT,
					 MIN_SHORT);
		SetCursorTo  (cursor_ASIGHbyORDEN, FmIFld(fm0, C_EMP), cliente != NULL_LONG ? cliente :
					 (FmIsNull(fm0, C_CLIH) ? MAX_LONG : FmLFld(fm0, C_CLIH)),
					 objetivo != NULL_SHORT ? objetivo :
					 (!FmIsNull(fm0, C_CLIH) && FmLFld(fm0, C_CLID) == FmLFld(fm0, C_CLIH) &&
					  !FmIsNull(fm0, OBJ_H) ? FmIFld(fm0,OBJ_H) : MAX_SHORT), MAX_DATE, MAX_LONG, MAX_SHORT,
					 MAX_SHORT, MAX_SHORT);
	}
	else{
		SetCursorFrom(cursor_ASIGHbyORDEN, FmIFld(fm0, C_EMP), vigilador != NULL_LONG ? vigilador :
					 (FmIsNull(fm0, VIG_D)  ? MIN_LONG : FmLFld(fm0, VIG_D)),
					 vigilador != NULL_LONG ? fecha_desde : MIN_DATE, MIN_LONG, MIN_SHORT);
		SetCursorTo  (cursor_ASIGHbyORDEN, FmIFld(fm0, C_EMP), vigilador != NULL_LONG ? vigilador :
					 (FmIsNull(fm0, VIG_H) ? MAX_LONG : FmLFld(fm0, VIG_H)), MAX_DATE, MAX_LONG, MAX_SHORT);
	}
	MoveCursorFirst(cursor_ASIGHbyORDEN);
	fin_datosASIGH = (FetchCursor(cursor_ASIGHbyORDEN) == ERROR);
	while (!fin_datosASIGH && !error_rp) {
		bool continuar = FALSE;

		if (procesamiento_fijo && !(LFld(ASIGH_CLIENTE) == cliente && IFld(ASIGH_OBJETIVO) == objetivo &&
									LFld(ASIGH_NROLEG)  == vigilador))
			continuar = TRUE;
		if (!continuar && DFld(ASIGH_FECBAJ) < fecha_desde)
			continuar = TRUE;
		if (!continuar && DFld(ASIGH_FECALT) > fecha_hasta)
			continuar = TRUE;
		if (!continuar && !procesamiento_fijo && !RegValido(IFld(ASIGH_EMP), LFld(ASIGH_CLIENTE),
				IFld(ASIGH_OBJETIVO), LFld(ASIGH_NROLEG), SFld(ASIGH_VIGIL)))
			continuar = TRUE;
		if (continuar) {
			fin_datosASIGH = (FetchCursor(cursor_ASIGHbyORDEN) == ERROR);
			continue;
		}
		if (!procesamiento_fijo) {
			LeerCliente(LFld(ASIGH_CLIENTE));
			LeerObjetivo(LFld(ASIGH_CLIENTE),IFld(ASIGH_OBJETIVO));
			LeerVigilador(IFld(ASIGH_EMP),LFld(ASIGH_NROLEG));
			if (FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ) {
				if (salida_archivo) {
					fprintf(archivo_excel,"Cliente: %ld %s\t%d %s\n", LFld(ASIGH_CLIENTE),
							SFld(scBILL|CLIENTE_RAZSOC), IFld(ASIGH_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
				}
				else {
					RpSetLFld(rp0, RP_CCLI, LFld(ASIGH_CLIENTE));
					RpSetFld (rp0, RP_XCLI, SFld(scBILL|CLIENTE_RAZSOC));
					RpSetIFld(rp0, RP_OBJ,  IFld(ASIGH_OBJETIVO));
					RpSetFld (rp0, RP_XOBJ, SFld(scCOMERC|OBJETIVO_DESCRIP));
					error_rp = DoReport(rp0,LIN_CLIOBJ);
				}
			}
			else {
				if (salida_archivo) {
					fprintf(archivo_excel,"Vigilador: %ld %s\n", LFld(ASIGH_NROLEG), SFld(scSUE|PER_APYNOM));
				}
				else {
					RpSetLFld(rp0,RP_VIG,  LFld(ASIGH_NROLEG));
					RpSetFld (rp0,RP_XVIG, SFld(scSUE|PER_APYNOM));
					error_rp = DoReport(rp0,LIN_VIGILADOR);
				}
			}
		}
		while (!fin_datosASIGH && !error_rp &&
			  ((FmIFld(fm0,T_ORDEN) == ORDEN_CLIOBJ && cliente_corriente == LFld(ASIGH_CLIENTE) &&
			   objetivo_corriente  == IFld(ASIGH_OBJETIVO)) || vigilador_corriente == LFld(ASIGH_NROLEG))) {
			bool continuar = FALSE;

			if (procesamiento_fijo && !(LFld(ASIGH_CLIENTE) == cliente && IFld(ASIGH_OBJETIVO) == objetivo &&
			   LFld(ASIGH_NROLEG) == vigilador))
				continuar = TRUE;
			if (!continuar && DFld(ASIGH_FECBAJ) < fecha_desde)
				continuar = TRUE;
			if (!continuar && DFld(ASIGH_FECALT) > fecha_hasta)
				continuar = TRUE;
			if (!continuar && !procesamiento_fijo && !RegValido(IFld(ASIGH_EMP), LFld(ASIGH_CLIENTE),
				IFld(ASIGH_OBJETIVO), LFld(ASIGH_NROLEG), SFld(ASIGH_VIGIL)))
				continuar = TRUE;
			if (continuar) {
				fin_datosASIGH = (FetchCursor(cursor_ASIGHbyORDEN) == ERROR);
				continue;
			}
			if (!procesamiento_fijo) {
				if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ)
					LeerVigilador(IFld(ASIGH_EMP), LFld(ASIGH_NROLEG));
				else {
					LeerCliente(LFld(ASIGH_CLIENTE));
					LeerObjetivo(LFld(ASIGH_CLIENTE),IFld(ASIGH_OBJETIVO));
				}
			}
			ImprimirDetaASIGH(rp0);
			fin_datosASIGH = (FetchCursor(cursor_ASIGHbyORDEN) == ERROR);
		}
		if (!procesamiento_fijo && !salida_archivo)
			error_rp = DoReport(rp0, LIN_FIN);
	}
	return;
}

private bool RegValido(int empresa, long cliente, int objetivo, long vigilador,char * tipo_vigilador)
{
	if (empresa != FmIFld(fm0,C_EMP))
		return FALSE;

	if (!FmIsNull(fm0,C_CLID) && cliente < FmLFld(fm0,C_CLID))
		return FALSE;
	if (!FmIsNull(fm0,C_CLIH) && cliente > FmLFld(fm0,C_CLIH))
		return FALSE;
	if (!FmIsNull(fm0,OBJ_D) && objetivo  < FmIFld(fm0,OBJ_D))
		return FALSE;
	if (!FmIsNull(fm0,OBJ_H) && objetivo  > FmIFld(fm0,OBJ_H))
		return FALSE;
	if (!FmIsNull(fm0,VIG_D) && vigilador < FmLFld(fm0,VIG_D))
		return FALSE;
	if (!FmIsNull(fm0,VIG_H) && vigilador > FmLFld(fm0,VIG_H))
		return FALSE;
	if (!str_eq(tipo_vigilador, VIGILADOR_PARTTIME))
		return FALSE;
	return TRUE;
}

private void ImprimirDetaASIG(report rp0)
{
	int  i_camporp;
	bool inicio_ciclo;

	LeerPuestos(LFld(ASIG_CLIENTE), IFld(ASIG_OBJETIVO), IFld(ASIG_PTOSER), IFld(ASIG_PUESTO));
	if (salida_archivo) {
		if (FmIFld(fm0, T_ORDEN) == ORDEN_VIGILADOR) {
			fprintf(archivo_excel, "%ld %s\t%d %s\t", LFld(ASIG_CLIENTE), SFld(scBILL|CLIENTE_RAZSOC),
					IFld(ASIG_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
		}
		if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
			fprintf(archivo_excel, "%ld\t%s\t", LFld(ASIG_NROLEG), SFld(scSUE|PER_APYNOM));
		}
		fprintf(archivo_excel, "%s\t%s\t%d\t%s\t%s %s %d\t%D\t%D\t%s %s %s %s %s %s %s\t%T\t%T\n",
				InDescr(ASIG_VIGIL,SFld(ASIG_VIGIL)), InDescr(ASIG_EFECT,SFld(ASIG_EFECT)),
				IFld(ASIG_PTOSER), GetDescPto(IFld(scOPERAC|ASIG_PTOSER)), SFld(ASIG_REGIM),
				SFld(PUESTOS_CODFREC), IFld(PUESTOS_HORAPT), DFld(ASIG_FECASIG), DFld(ASIG_FECHAS),
				SFld(ASIG_DIA1), SFld(ASIG_DIA2), SFld(ASIG_DIA3), SFld(ASIG_DIA4), SFld(ASIG_DIA5),
				SFld(ASIG_DIA6), SFld(ASIG_DIA7), TFld(ASIG_HSENT), TFld(ASIG_HSSAL));
	}
	else {
		RpSetLFld(rp0,RP_CCLI,       LFld(ASIG_CLIENTE));
		RpSetFld (rp0,RP_XCLI,       SFld(scBILL|CLIENTE_RAZSOC));
		RpSetIFld(rp0,RP_OBJ,        IFld(ASIG_OBJETIVO));
		RpSetFld (rp0,RP_XOBJ,       SFld(scCOMERC|OBJETIVO_DESCRIP));
		RpSetLFld(rp0,RP_VIG,        LFld(ASIG_NROLEG));
		RpSetFld (rp0,RP_XVIG,       SFld(scSUE|PER_APYNOM));
		RpSetLFld(rp0,RP_LEGAJO,     LFld(ASIG_NROLEG));
		RpSetFld (rp0,RP_APENOM,     SFld(scSUE|PER_APYNOM));
		RpSetFld (rp0,RP_VR,         InDescr(ASIG_VIGIL,SFld(ASIG_VIGIL)));
		RpSetFld (rp0,RP_EF,         InDescr(ASIG_EFECT,SFld(ASIG_EFECT)));
		RpSetIFld(rp0,RP_PTOSER,     IFld(ASIG_PTOSER));
		RpSetFld (rp0,RP_DPTOSER,    GetDescPto(IFld(scOPERAC|ASIG_PTOSER)));
		RpSetFld (rp0,RP_REGIM,      SFld(ASIG_REGIM));
		RpSetFld (rp0,RP_FRECUENCIA, SFld(PUESTOS_CODFREC));
		RpSetIFld(rp0,RP_KHORAS,     IFld(PUESTOS_HORAPT));
		RpSetDFld(rp0,RP_FECASIG,    DFld(ASIG_FECASIG));
		RpSetDFld(rp0,RP_FECHAS,     DFld(ASIG_FECHAS));
		RpSetFld (rp0,RP_DIA1,       SFld(ASIG_DIA1));
		RpSetFld (rp0,RP_DIA2,       SFld(ASIG_DIA2));
		RpSetFld (rp0,RP_DIA3,       SFld(ASIG_DIA3));
		RpSetFld (rp0,RP_DIA4,       SFld(ASIG_DIA4));
		RpSetFld (rp0,RP_DIA5,       SFld(ASIG_DIA5));
		RpSetFld (rp0,RP_DIA6,       SFld(ASIG_DIA6));
		RpSetFld (rp0,RP_DIA7,       SFld(ASIG_DIA7));
		RpSetTFld(rp0,RP_HSENT,      TFld(ASIG_HSENT));
		RpSetTFld(rp0,RP_HSSAL,      TFld(ASIG_HSSAL));
		error_rp=DoReport(rp0,LIN_DETALLE);
	}
	if (FmIFld(fm0,M_DIAS)) {
		SetKey(DIASPTIMEbyDIA, IFld(ASIG_EMP), LFld(ASIG_NROLEG), DFld(ASIG_FECASIG), MIN_LONG, MIN_SHORT,
							   MIN_SHORT, MIN_SHORT, MIN_SHORT);
		inicio_ciclo = TRUE;
		i_camporp = 0;

		while (GetRecord(DIASPTIMEbyDIA,NEXT_KEY|PARTIAL_KEY,IO_NOT_LOCK,2) != ERROR) {
			if (LFld(DIASPTIME_CLIENTE) != LFld(ASIG_CLIENTE))
				continue;
			if (IFld(DIASPTIME_OBJETIVO) != LFld(ASIG_OBJETIVO))
				continue;
			if((DFld(ASIG_FECHAS) != NULL_DATE  && DFld(DIASPTIME_DIA) > DFld(ASIG_FECHAS)) ||
			   (DFld(ASIG_FECHAS) == NULL_DATE  && DFld(DIASPTIME_DIA)>FmDFld(fm0,F_H)))
				continue;
			if (inicio_ciclo) {
				if (!salida_archivo)
					error_rp = DoReport(rp0,RAYA_SEP);
				if (!salida_archivo)
					error_rp = DoReport(rp0,TITU_DIAS);
				inicio_ciclo = FALSE;
			}
			strcpy(nom_dia,DayName(DFld(DIASPTIME_DIA)));
			if (salida_archivo) {
				if (FmIFld(fm0, T_ORDEN) == ORDEN_VIGILADOR) {
					fprintf(archivo_excel,"%ld %s\t%d %s\t", LFld(ASIG_CLIENTE), SFld(scBILL|CLIENTE_RAZSOC),
														IFld(ASIG_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
				}
				if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
					fprintf(archivo_excel, "%ld\t%s\t", LFld(ASIG_NROLEG), SFld(scSUE|PER_APYNOM));
				}
				fprintf(archivo_excel, "%s\t%s\t%d\t%s\t%s %s %d\t%D\t%D\t%s %s %s %s %s %s %s\t%T\t%T\n",
								InDescr(ASIG_VIGIL, SFld(ASIG_VIGIL)), InDescr(ASIG_EFECT, SFld(ASIG_EFECT)),
								IFld(ASIG_PTOSER), GetDescPto(IFld(scOPERAC|ASIG_PTOSER)), SFld(ASIG_REGIM),
								SFld(PUESTOS_CODFREC), IFld(PUESTOS_HORAPT), DFld(DIASPTIME_DIA),
								DFld(DIASPTIME_DIA), str_eq(nom_dia, "LUNES") ? "L" : " ",
								str_eq(nom_dia,"MARTES") ? "M" : " ", str_eq(nom_dia,"MIERCOLES") ? "X" : " ",
								str_eq(nom_dia,"JUEVES") ? "J" : " ", str_eq(nom_dia,"VIERNES")   ? "V" : " ",
								str_eq(nom_dia,"SABADO") ? "S" : " ", str_eq(nom_dia,"DOMINGO")   ? "D" : " ",
								TFld(DIASPTIME_HENT), TFld(DIASPTIME_HSAL));
			}
			else {
				RpSetDFld(rp0,RP_FECASIG, DFld(DIASPTIME_DIA));
				RpSetDFld(rp0,RP_FECHAS,  DFld(DIASPTIME_DIA));
				RpSetFld (rp0,RP_DIA1,    NULL_STR);
				RpSetFld (rp0,RP_DIA2,    NULL_STR);
				RpSetFld (rp0,RP_DIA3,    NULL_STR);
				RpSetFld (rp0,RP_DIA4,    NULL_STR);
				RpSetFld (rp0,RP_DIA5,    NULL_STR);
				RpSetFld (rp0,RP_DIA6,    NULL_STR);
				RpSetFld (rp0,RP_DIA7,    NULL_STR);
				RpSetFld (rp0,DescriptorRP(nom_dia), nom_dia);
				if (str_eq(nom_dia, "MIERCOLES")) {
					RpSetFld (rp0,DescriptorRP(nom_dia), "X");
				}
				RpSetTFld(rp0,RP_HSENT, TFld(DIASPTIME_HENT));
				RpSetTFld(rp0,RP_HSSAL, TFld(DIASPTIME_HSAL));
				error_rp = DoReport(rp0,LIN_DETALLE);
			}
		}
		if (!inicio_ciclo && !salida_archivo)
			error_rp = DoReport(rp0,RAYA_SEP);
	}
	return;
}

private void ImprimirDetaASIGH(report rp0)
{
	int i_camporp;
	bool inicio_ciclo;

	LeerPuestos(LFld(ASIGH_CLIENTE), IFld(ASIGH_OBJETIVO), IFld(ASIGH_PTOSER), IFld(ASIGH_PUESTO));

	if (salida_archivo) {
		if (FmIFld(fm0, T_ORDEN) == ORDEN_VIGILADOR) {
			fprintf(archivo_excel,"%ld %s\t%ld %s\t", LFld(ASIGH_CLIENTE), SFld(scBILL|CLIENTE_RAZSOC),
												IFld(ASIGH_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
		}
		if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
			fprintf(archivo_excel, "%ld\t%s\t", LFld(ASIGH_NROLEG), SFld(scSUE|PER_APYNOM));
		}
		fprintf(archivo_excel, "%s\t%s\t%d\t%s\t%s %s %d\t%D\t%D\t%s %s %s %s %s %s %s\t%T\t%T\n",
			InDescr(ASIGH_VIGIL,SFld(ASIGH_VIGIL)), InDescr(ASIGH_EFECT,SFld(ASIGH_EFECT)),
			IFld(ASIGH_PTOSER), GetDescPto(IFld(scOPERAC|ASIGH_PTOSER)), SFld(ASIGH_REGIM),
			SFld(PUESTOS_CODFREC), IFld(PUESTOS_HORAPT), DFld(ASIGH_FECALT), DFld(ASIGH_FECBAJ),
			SFld(ASIGH_DIA1), SFld(ASIGH_DIA2), SFld(ASIGH_DIA3), SFld(ASIGH_DIA4), SFld(ASIGH_DIA5),
			SFld(ASIGH_DIA6), SFld(ASIGH_DIA7), TFld(ASIGH_HSENT), TFld(ASIGH_HSSAL));
	}
	else {
		RpSetLFld(rp0,RP_CCLI,       LFld(ASIGH_CLIENTE));
		RpSetFld (rp0,RP_XCLI,       SFld(scBILL|CLIENTE_RAZSOC));
		RpSetIFld(rp0,RP_OBJ,        IFld(ASIGH_OBJETIVO));
		RpSetFld (rp0,RP_XOBJ,       SFld(scCOMERC|OBJETIVO_DESCRIP));
		RpSetLFld(rp0,RP_VIG,        LFld(ASIGH_NROLEG));
		RpSetFld (rp0,RP_XVIG,       SFld(scSUE|PER_APYNOM));
		RpSetLFld(rp0,RP_LEGAJO,     LFld(ASIGH_NROLEG));
		RpSetFld (rp0,RP_APENOM,     SFld(scSUE|PER_APYNOM));
		RpSetFld (rp0,RP_VR,         InDescr(ASIG_VIGIL,SFld(ASIGH_VIGIL)));
		RpSetFld (rp0,RP_EF,         InDescr(ASIG_EFECT,SFld(ASIGH_EFECT)));
		RpSetIFld(rp0,RP_PTOSER,     IFld(ASIGH_PTOSER));
		RpSetFld (rp0,RP_DPTOSER,    GetDescPto(IFld(scOPERAC|ASIGH_PTOSER)));
		RpSetFld (rp0,RP_REGIM,      SFld(ASIGH_REGIM));
		RpSetFld (rp0,RP_FRECUENCIA, SFld(PUESTOS_CODFREC));
		RpSetIFld(rp0,RP_KHORAS,     IFld(PUESTOS_HORAPT));
		RpSetDFld(rp0,RP_FECASIG,    DFld(ASIGH_FECALT));
		RpSetDFld(rp0,RP_FECHAS,     DFld(ASIGH_FECBAJ));
		RpSetFld (rp0,RP_DIA1,       SFld(ASIGH_DIA1));
		RpSetFld (rp0,RP_DIA2,       SFld(ASIGH_DIA2));
		RpSetFld (rp0,RP_DIA3,       SFld(ASIGH_DIA3));
		RpSetFld (rp0,RP_DIA4,       SFld(ASIGH_DIA4));
		RpSetFld (rp0,RP_DIA5,       SFld(ASIGH_DIA5));
		RpSetFld (rp0,RP_DIA6,       SFld(ASIGH_DIA6));
		RpSetFld (rp0,RP_DIA7,       SFld(ASIGH_DIA7));
		RpSetTFld(rp0,RP_HSENT,      TFld(ASIGH_HSENT));
		RpSetTFld(rp0,RP_HSSAL,      TFld(ASIGH_HSSAL));
		error_rp=DoReport(rp0,LIN_DETALLE);
	}
	if (FmIFld(fm0, M_DIAS)) {
		SetKey(DIASPTIMEHbyDIA, IFld(ASIGH_EMP), LFld(ASIGH_NROLEG), DFld(ASIGH_FECALT), MIN_LONG, MIN_SHORT,
								MIN_SHORT, MIN_SHORT, MIN_SHORT);
		inicio_ciclo = TRUE;
		i_camporp = 0;
		while (GetRecord(DIASPTIMEHbyDIA, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
			if (LFld(DIASPTIMEH_CLIENTE) != LFld(ASIGH_CLIENTE))
				continue;
			if (IFld(DIASPTIMEH_OBJETIVO) != LFld(ASIGH_OBJETIVO))
				continue;
			if (DFld(ASIGH_FECBAJ) != NULL_DATE && DFld(DIASPTIMEH_DIA) > DFld(ASIGH_FECBAJ))
				continue;
			if (inicio_ciclo) {
				if (!salida_archivo)
					error_rp=DoReport(rp0,RAYA_SEP);
				if (!salida_archivo)
					error_rp=DoReport(rp0,TITU_DIAS);
				inicio_ciclo=FALSE;
			}
			strcpy(nom_dia,DayName(DFld(DIASPTIMEH_DIA)));
			if (salida_archivo) {
				if (FmIFld(fm0, T_ORDEN) == ORDEN_VIGILADOR) {
					fprintf(archivo_excel,"%ld %s\t%ld %s\t", LFld(ASIGH_CLIENTE), SFld(scBILL|CLIENTE_RAZSOC),
						IFld(ASIGH_OBJETIVO), SFld(scCOMERC|OBJETIVO_DESCRIP));
				}
				if (FmIFld(fm0, T_ORDEN) == ORDEN_CLIOBJ) {
					fprintf(archivo_excel, "%ld\t%s\t", LFld(ASIGH_NROLEG), SFld(scSUE|PER_APYNOM));
				}
				fprintf(archivo_excel,"%s\t%s\t%d\t%s\t%s %s %d\t%D\t%D\t%s %s %s %s %s %s %s\t%T\t%T\n",
					InDescr(ASIGH_VIGIL,   SFld(ASIGH_VIGIL)), InDescr(ASIGH_EFECT, SFld(ASIGH_EFECT)),
					IFld(ASIGH_PTOSER),    GetDescPto(IFld(scOPERAC|ASIGH_PTOSER)), SFld(ASIGH_REGIM),
					SFld(PUESTOS_CODFREC), IFld(PUESTOS_HORAPT), DFld(DIASPTIMEH_DIA), DFld(DIASPTIMEH_DIA),
					str_eq(nom_dia,"LUNES")     ? "L" : " ", str_eq(nom_dia,"MARTES") ? "M" : " ",
					str_eq(nom_dia,"MIERCOLES") ? "X" : " ", str_eq(nom_dia,"JUEVES") ? "J" : " ",
					str_eq(nom_dia,"VIERNES")   ? "V" : " ", str_eq(nom_dia,"SABADO") ? "S" : " ",
					str_eq(nom_dia,"DOMINGO")   ? "D" : " ", TFld(DIASPTIMEH_HENT), TFld(DIASPTIMEH_HSAL));
			}
			else {
				RpSetDFld(rp0, RP_FECASIG, DFld(DIASPTIMEH_DIA));
				RpSetDFld(rp0, RP_FECHAS,  DFld(DIASPTIMEH_DIA));
				RpSetFld (rp0, RP_DIA1,    NULL_STR);
				RpSetFld (rp0, RP_DIA2,    NULL_STR);
				RpSetFld (rp0, RP_DIA3,    NULL_STR);
				RpSetFld (rp0, RP_DIA4,    NULL_STR);
				RpSetFld (rp0, RP_DIA5,    NULL_STR);
				RpSetFld (rp0, RP_DIA6,    NULL_STR);
				RpSetFld (rp0, RP_DIA7,    NULL_STR);
				RpSetFld (rp0, DescriptorRP(nom_dia), nom_dia);
				if (str_eq(nom_dia, "MIERCOLES")) {
					RpSetFld(rp0, DescriptorRP(nom_dia), "X");
				}
				RpSetTFld(rp0,RP_HSENT, TFld(DIASPTIMEH_HENT));
				RpSetTFld(rp0,RP_HSSAL, TFld(DIASPTIMEH_HSAL));
				error_rp = DoReport(rp0,LIN_DETALLE);
			}
		}
		if (!inicio_ciclo && !salida_archivo)
			error_rp = DoReport(rp0,RAYA_SEP);
	}
	/**/
	return;
}

private void LeerCliente(long cliente)
{
	SetKey(scBILL|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(scBILL|CLIENTEbyCLIENTE,THIS_KEY,IO_NOT_LOCK) == ERROR) {
		Warning("CLIENTE no encontrado para CLIENTE=[%ld]",cliente);
		InitRecord(scBILL|CLIENTE);
	}
}

private void LeerObjetivo(long cliente,int objetivo)
{
	SetKey(scCOMERC|OBJETIVObyCLIENTE, cliente, objetivo);
	if (GetRecord(scCOMERC|OBJETIVObyCLIENTE,THIS_KEY,IO_NOT_LOCK) == ERROR)
		Warning("OBJETIVO no encontrado para CLIENTE=[%ld] OBJETIVO=[%d]", cliente, objetivo);
}

private void LeerVigilador(int emp,long nroleg)
{
	SetKey(scSUE|PERbyEMP,emp,nroleg);
	if (GetRecord(scSUE|PERbyEMP,THIS_KEY,IO_NOT_LOCK) == ERROR)
		Warning("PER no encontrado para EMP=[%d] NROLEG=[%ld]",emp,nroleg);
}

private void LeerPuestos(long cliente, int objetivo, int ptoser, int puesto)
{
	SetKey(PUESTOSbyCLIENTE, cliente, objetivo, ptoser, puesto);
	if (GetRecord(PUESTOSbyCLIENTE,THIS_KEY,IO_NOT_LOCK) == ERROR) {
		Warning("PUESTOS no encontrado para CLIENTE=[%ld] OBJET=[%d] TIPPTO=[%d] CODINT=[%d]",
					cliente, objetivo, ptoser, puesto);
	}
}

private rpfield DescriptorRP(char * nom_dia)
{
	if (str_eq(nom_dia,"LUNES"))
		return RP_DIA1;
	if (str_eq(nom_dia,"MARTES"))
		return RP_DIA2;
	if (str_eq(nom_dia,"MIERCOLES"))
		return RP_DIA3;
	if (str_eq(nom_dia,"JUEVES"))
		return RP_DIA4;
	if (str_eq(nom_dia,"VIERNES"))
		return RP_DIA5;
	if (str_eq(nom_dia,"SABADO"))
		return RP_DIA6;
	if (str_eq(nom_dia,"DOMINGO"))
		return RP_DIA7;
	Error("Descriptor de Campo para la Impresión no determinable con nom_dia [%s] !!!", nom_dia);
}
