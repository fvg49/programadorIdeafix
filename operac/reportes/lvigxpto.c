/********************************************************************
* MODULE & VERSION : @(#)lvigxpto.c	1.4 
* DATE             : 12/10/22 
* TIME             : 11:04:53 
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
#include "lvigxpto.fmh"
#include "lvigxpto.rph"
#include "operac.sch"
#include "comerc.sch"
#include "bill.sch"
#include "operac.h"
#include "comerc.h"
#include "filial.h"

#define ARCHI      0
#define TERM       1
#define IMPRE      2

/* Funciones privadas */
static fm_status after(form, fmfield, int);
static fm_status before(form, fmfield, int);
static void Proceso();
static void AbrirSalida();
static void	ImprimirVigilador();
static void	ImprimirPuesto();
static void	ArchivarVigilador();
static void	ArchivarPuesto();
static DATE DiaFranco();

/* Declaraciones globales */
form fm0;
report rp0;
FILE *fp;
schema operac, bill, comerc;
bool salida;

/* Programa principal */
wcmd(lvigxpto, 1.4 10/22/12)
{
	fm0    = OpenForm("lvigxpto", FM_EABORT);
	bill   = OpenSchema("bill",   IO_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	operac = OpenSchema("operac", IO_EABORT);

  	FmSetFld(fm0, COMENT, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, COMENT, "");
	WiRefresh();

	while(DoForm(fm0, before, after) != FM_EXIT) {
		AbrirSalida();
		Proceso();
		if (strcmp(FmSFld(fm0, SALIDA), "A")) CloseReport(rp0);
    }
    
    FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void Proceso()
{
	dbcursor c_puesto, c_asig, c_OBJ;
	long cliant = NULL_LONG;
	int  objant = NULL_SHORT;
	bool first  = TRUE;

	c_puesto = CreateCursor(operac|PUESTOSbyCLIENTE, IO_NOT_LOCK);
	c_asig   = CreateCursor(operac|ASIGbyPUESTO,     IO_NOT_LOCK);

	if (*FmSFld(fm0, OPCION) == 'P') {
		c_OBJ = CreateCursor(comerc|OBJETIVObyPRESEN, IO_NOT_LOCK);
		SetCursorFrom(c_OBJ, FmLFld(fm0, SUPERD), MIN_LONG, MIN_SHORT);
		SetCursorTo  (c_OBJ, FmLFld(fm0, SUPERH), MAX_LONG, MAX_SHORT);
	}
	else {
		c_OBJ = CreateCursor(comerc|OBJETIVO, IO_NOT_LOCK);
		SetCursorFrom(c_OBJ, FmLFld(fm0, CLID), FmIFld(fm0, OBJD));
		SetCursorTo  (c_OBJ, FmLFld(fm0, CLIH), FmIFld(fm0, OBJH));
	}

	while (FetchCursor(c_OBJ) != ERROR) {
		if (FmIFld(fm0, EMP) != IFld(comerc|OBJETIVO_EMP)) {
			//Descarto los objetivos por empresa
			continue;
		}
	    
	    //valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		       	continue;
		
		if (!ValidaFilial(LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), FmSFld(fm0, DELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;

		SetCursorFrom(c_puesto, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), MIN_SHORT, MIN_SHORT);
		SetCursorTo  (c_puesto, LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET), MAX_SHORT, MAX_SHORT);
		while (FetchCursor(c_puesto) != ERROR) {
			bool encontro = FALSE;
			short cantpue, cantvig;

			if (!IsNull(operac|PUESTOS_FFINAL) && DFld(operac|PUESTOS_FFINAL) < Today())
				continue;

			if (IFld(operac|PUESTOS_CANTPUE) == 0 && IFld(operac|PUESTOS_CANTVIG) == 0)
				continue;

			encontro = StdPuesto(FmIFld(fm0, EMP), LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET),
							 IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_PUESTO),
							 SFld(operac|PUESTOS_REGIM),  TFld(operac|PUESTOS_HINICIO),
							 TFld(operac|PUESTOS_HFINAL), Today(), &cantpue, &cantvig, FALSE, Today(), IFld(operac|PUESTOS_CODINT));

//			fprintf(stderr, "%d %d - %B %d\n", IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT), encontro, cantpue);

			if (encontro) {
				if (!cantpue)
					continue;

				if (!IsNull(operac|PUESTOS_FFINAL) && DFld(operac|PUESTOS_FFINAL) > Today()) {
					SetIFld(operac|PUESTOS_CANTPUE, cantpue);
					SetIFld(operac|PUESTOS_CANTVIG, cantvig);
				}
			}							 

			if (LFld(operac|PUESTOS_CLIENTE) != cliant || IFld(operac|PUESTOS_OBJET) != objant) {
				cliant = LFld(operac|PUESTOS_CLIENTE);
				objant = IFld(operac|PUESTOS_OBJET);

				SetLFld(bill|CLIENTE_CLIENTE, LFld(operac|PUESTOS_CLIENTE));
				GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK);

				if (strcmp(FmSFld(fm0, SALIDA), "A")) {
					RpSetLFld(rp0, R_CLI,   LFld(operac|PUESTOS_CLIENTE));
					RpSetFld (rp0, R_DCLI,  SFld(bill|CLIENTE_RAZSOC));
					RpSetIFld(rp0, R_OBJ,   IFld(operac|PUESTOS_OBJET));
					RpSetFld (rp0, R_DOBJ,  GetObjDescrip(LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET)));
					RpSetLFld(rp0, R_SUPERV,LFld(comerc|OBJETIVO_PRESEN));

					RpSetFld (rp0, R_DSUP,  GetNombreLeg(FmIFld(fm0, EMP), LFld(comerc|OBJETIVO_PRESEN)));
                    RpSetFld (rp0, R_FIL,	GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(operac|PUESTOS_OBJET)));
                    RpSetFld (rp0, R_DFIL,  GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(operac|PUESTOS_OBJET))));
					if (!first)
						RpEjectPage(rp0);
					first  = FALSE;
					DoReport (rp0, ENCAB);
				}
			}


			if (*FmSFld(fm0, SALIDA) != 'A') {
		 		ImprimirPuesto();
			}
			else
				ArchivarPuesto();

			SetCursorFrom(c_asig, FmIFld(fm0, EMP), LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET),
								  IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT), MIN_SHORT, MIN_LONG);
			SetCursorTo  (c_asig, FmIFld(fm0, EMP), LFld(operac|PUESTOS_CLIENTE), IFld(operac|PUESTOS_OBJET),
								  IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_CODINT), MAX_SHORT, MAX_LONG);
			while (FetchCursor(c_asig) != ERROR) {
				if (*FmSFld(fm0, SALIDA) != 'A')
					ImprimirVigilador();
				else 
					ArchivarVigilador();
			}
	 	}
	}
}

static void ImprimirPuesto()
{
	RpSetIFld(rp0, R_CANTPTO, IFld(operac|PUESTOS_CANTPUE));
	RpSetIFld(rp0, R_PTOSER,  IFld(operac|PUESTOS_TIPPTO));
	RpSetIFld(rp0, R_PTO,     IFld(operac|PUESTOS_CODINT));
	RpSetFld (rp0, R_DPTOSER, GetDescPto(IFld(operac|PUESTOS_TIPPTO)));
	RpSetIFld(rp0, R_CATEG,   IFld(operac|PUESTOS_PUESTO));
	RpSetIFld(rp0, R_HSPT,    IFld(operac|PUESTOS_HORAPT));
	RpSetTFld(rp0, R_HDESDE,  TFld(operac|PUESTOS_HINICIO));
	RpSetTFld(rp0, R_HHASTA,  TFld(operac|PUESTOS_HFINAL));
	RpSetFld (rp0, R_D1,      SFld(operac|PUESTOS_DIA1));
	RpSetFld (rp0, R_D2,      SFld(operac|PUESTOS_DIA2));
	RpSetFld (rp0, R_D3,      SFld(operac|PUESTOS_DIA3));
	RpSetFld (rp0, R_D4,      SFld(operac|PUESTOS_DIA4));
	RpSetFld (rp0, R_D5,      SFld(operac|PUESTOS_DIA5));
	RpSetFld (rp0, R_D6,      SFld(operac|PUESTOS_DIA6));
	RpSetFld (rp0, R_D7,      SFld(operac|PUESTOS_DIA7));
	RpSetFld (rp0, R_REG,     SFld(operac|PUESTOS_REGIM));
	RpSetFld (rp0, R_FREC,    SFld(operac|PUESTOS_CODFREC));
	RpSetIFld(rp0, R_CANTPER, IFld(operac|PUESTOS_CANTVIG));
	DoReport (rp0, LINPTO);
	RpClearZone(rp0, LINPTO);
} 

static void	ImprimirVigilador()
{
	RpSetLFld(rp0, R_LEGAJO,  LFld(operac|ASIG_NROLEG));
	RpSetFld (rp0, R_APENOM,  GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG)));
	RpSetFld (rp0, R_VR,      InDescr(operac|ASIG_VIGIL, SFld(operac|ASIG_VIGIL)));
	RpSetFld (rp0, R_EF,      InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)));
	RpSetFld (rp0, R_DIA1,    SFld(operac|ASIG_DIA1));
	RpSetFld (rp0, R_DIA2,    SFld(operac|ASIG_DIA2));
	RpSetFld (rp0, R_DIA3,    SFld(operac|ASIG_DIA3));
	RpSetFld (rp0, R_DIA4,    SFld(operac|ASIG_DIA4));
	RpSetFld (rp0, R_DIA5,    SFld(operac|ASIG_DIA5));
	RpSetFld (rp0, R_DIA6,    SFld(operac|ASIG_DIA6));
	RpSetFld (rp0, R_DIA7,    SFld(operac|ASIG_DIA7));
	RpSetFld (rp0, R_REGIM,   SFld(operac|ASIG_REGIM));
	RpSetTFld(rp0, R_HSENT,   TFld(operac|ASIG_HSENT));
	RpSetTFld(rp0, R_HSSAL,   TFld(operac|ASIG_HSSAL));
	RpSetDFld(rp0, R_FECASIG, DFld(operac|ASIG_FECASIG));
	RpSetDFld(rp0, R_FECHAS,  DFld(operac|ASIG_FECHAS));

	if (!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO))
		RpSetFld(rp0, R_FR, "F");
	else
		RpSetFld(rp0, R_FR, NULL_STR);		

	RpSetDFld(rp0, R_FFRANCO, DiaFranco());
	DoReport (rp0, LINVIG);
	RpClearZone(rp0, LINVIG);
}

static DATE DiaFranco()
{
	DATE franco;
	int loop = 1;

	franco = Today();
	if (IsNull(operac|ASIG_FFRANCO)) {
		franco = NULL_DATE;
	}
	else {
		if (GetDiasFranco(SFld(operac|ASIG_REGIM), FALSE) == 0 ) {	//si se carga algun regimen con partime true cambiar esto
			franco = NULL_DATE;
		}
		else {
			while (!Franco(IFld(operac|ASIG_EMP), LFld(operac|ASIG_NROLEG), franco, SFld(operac|ASIG_VIGIL), IFld(operac|ASIG_NUMFRAN))) {
				if (loop++ == 10000) {
					WiDialog(WD_OK, WD_OK, "Error", "El legajo %d presenta inconvenientes en su asignación.\nPor favor revisar y corregir el error antes de emitir este listado", LFld(operac|ASIG_NROLEG));
					break;
					
				} 
				franco++;
			}
		}
	}
	return franco;
}

static void	ArchivarPuesto()
{
	fprintf(fp, "Puesto\t%d\t%d\t%d\t%s\t%d\t%d\t%T\t%T\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\t%s\n",
			IFld(operac|PUESTOS_CANTPUE), IFld(operac|PUESTOS_TIPPTO), IFld(operac|PUESTOS_PUESTO),
			GetDescPto(IFld(operac|ASIG_PTOSER)), IFld(operac|PUESTOS_PUESTO), IFld(operac|PUESTOS_HORAPT),
			TFld(operac|PUESTOS_HINICIO), TFld(operac|PUESTOS_HFINAL), SFld(operac|PUESTOS_DIA1),
			SFld(operac|PUESTOS_DIA2), SFld(operac|PUESTOS_DIA3), SFld(operac|PUESTOS_DIA4),
			SFld(operac|PUESTOS_DIA5), SFld(operac|PUESTOS_DIA6), SFld(operac|PUESTOS_DIA7), 
			SFld(operac|PUESTOS_REGIM),SFld(operac|PUESTOS_CODFREC), IFld(operac|PUESTOS_CANTVIG),
			GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)),
			GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
			);
}

static void	ArchivarVigilador()
{
	fprintf(fp, "Vigi\t%ld\t%s\t%s\t%s\t%s\t%D\t%D\t%D\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%T\t%T\t%s\t%s\n",
		LFld(operac|ASIG_NROLEG),  GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|ASIG_NROLEG)),
		InDescr(operac|ASIG_VIGIL, SFld(operac|ASIG_VIGIL)), InDescr(operac|ASIG_EFECT, SFld(operac|ASIG_EFECT)),
		!IsNull(operac|ASIG_FRANCERO) && IFld(operac|ASIG_FRANCERO) ? "F" : NULL_STR,
		DFld(operac|ASIG_FECASIG), DFld(operac|ASIG_FECHAS), DiaFranco(),
		SFld(operac|ASIG_DIA1),    SFld(operac|ASIG_DIA2), SFld(operac|ASIG_DIA3), SFld(operac|ASIG_DIA4),
		SFld(operac|ASIG_DIA5),    SFld(operac|ASIG_DIA6), SFld(operac|ASIG_DIA7), SFld(operac|ASIG_REGIM),
		TFld(operac|ASIG_HSENT),   TFld(operac|ASIG_HSSAL),
		GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)),
		GetDescFilial(GetFilialDeObj(IFld(comerc|OBJETIVO_EMP), LFld(comerc|OBJETIVO_CLIENTE), IFld(comerc|OBJETIVO_OBJET)))
		);
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((fp = fopen(FmSFld(fm0, ARCHIVO) , "w")) == NULL)
			Error("No se puede abrir el archivo %s", FmSFld(fm0, ARCHIVO));
	}
	else {
		rp0 = OpenReport("lvigxpto", RP_EABORT|RP_NOBEGIN);

		if (*FmSFld(fm0, SALIDA) == 'I')
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR);
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR);
		BeginReport(rp0, 1, NULL_STR);
	}
}

static fm_status after(form fm, fmfield fno, int row)
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
				case 'S' :
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



