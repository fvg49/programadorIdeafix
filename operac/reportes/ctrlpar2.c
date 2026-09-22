/********************************************************************
*
* MODULE & VERSION : @(#)ctrlpar2.c	1.1
* DATE             : 11/01/05
* TIME             : 10:42:18
*
* CREATED          : 05/08/2005
*
* DESCRIPTION:
*     
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "operac.h"
#include "comerc.h"
#include "billpro.h"
#include "ctrlpar2.fmh"
#include "ctrlpar2.rph"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"
#include "asist.sch"
#include "filial.h"

/* Defines */
#define MAXPARTE 10000

/* Estructuras Globales */

typedef struct partes_typ {
	long	cliente;
	int		objetivo;
	int 	ptoser;
	int		puesto;         
	char	cond;
	int		codaus;
	long	pos;
} partes_obj, *partes_ptr;                            

typedef struct legajos_typ {
	long nroleg;
	DATE dia;
	char cond;
	int	 codaus;
	bool diff;
	struct legajos_typ * next;
} legajos_obj, *legajos_ptr;
                            


/* Funciones privadas */
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
bool CambiaDia(long nroleg, DATE dia);
void ImprimeTabla(void);
void AgregaATabla(long cliente, int objetivo, int ptoser, int puesto, char cond, int codaus);
void AbrirReporte();
void SetearCabArch();
void ImprimirCabecera();
int CmpParte(long este, long sig);
void DeleteLeg(legajos_ptr ptr);
legajos_ptr AgregarLegajos(legajos_ptr ptr, long nroleg, DATE dia, char cond, int codaus);
                            
/* Declaraciones globales */
FILE   *fp;
form   fm0;
report rp0 = ERROR;
schema comerc, operac, asist;
partes_obj PxLegxDia[MAXPARTE];
long	cParte,bParte;
long	lastleg;
DATE	lastdia;
char    paraarchivo[200];
bool listacok = FALSE, listaodok = FALSE, listaohok = FALSE;
DATE fecierre;
int g_emp;

wcmd(ctrlpar, 1.1 01/05/11)
{
	fm_cmd cmd;
	int    i = 0;
	char   buffer[50];
	char   cond=0;
	int	   codaus=0;
	bool   chgcond;
	dbcursor pBYempl, pBYemp;

	fm0    = OpenForm  ("ctrlpar2", FM_EABORT);
	comerc = OpenSchema("comerc",  IO_EABORT);
	asist  = OpenSchema("asist",  IO_EABORT);
	operac = OpenSchema("operac",  IO_EABORT);

    g_emp= StrToI(getenv("emp"));

    fecierre  = GetFechaCierreOpe(g_emp);
	InicLegajoXusr (g_emp, fecierre, fm0, COMENT, TRUE, _TIPPER_INSERTA);
        
	while ((cmd = DoForm(fm0, before, after)) != FM_EXIT) {
		switch (cmd) {
		case FM_ADD:
		case FM_UPDATE:
			AbrirReporte();

			pBYempl = CreateCursor(operac|PARTEbyEMPLE, IO_NOT_LOCK);

			if (FmIFld(fm0, LISTAPOR) == 2) {       //lista por indice Vigilador
				lastleg = MIN_LONG;
				lastdia = MIN_DATE;
				chgcond = FALSE;
				cParte  = 0;
				bParte  = -1;

				SetCursorFrom(pBYempl, FmIFld(fm0, EMP), FmIsNull (fm0, VIGILD) ? MIN_LONG : FmLFld (fm0, VIGILD), FmIsNull (fm0, FDESDE) ? MIN_DATE : FmDFld (fm0, FDESDE), FmIsNull (fm0, CLIED) ? MIN_LONG: FmLFld(fm0, CLIED) , FmIsNull (fm0, OBJETD) ? MIN_SHORT :  FmIFld(fm0, OBJETD));
				SetCursorTo  (pBYempl, FmIFld(fm0, EMP), FmIsNull (fm0, VIGILH) ? MAX_LONG : FmLFld (fm0, VIGILH), FmIsNull (fm0, FHASTA) ? MAX_DATE : FmDFld (fm0, FHASTA), FmIsNull (fm0, CLIEH) ? MAX_LONG: FmLFld(fm0, CLIEH) , FmIsNull (fm0, OBJETH) ? MAX_SHORT :  FmIFld(fm0, OBJETH));
				cParte=0;
				while (FetchCursor(pBYempl) != ERROR) {
					i = 0;

					//valida el legajo
					if (!ValidaLegajoXusr(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA)))
				       	continue;
		
					// Elimino fechas Invalidas
					// Fechas menores
					if (!FmIsNull(fm0, FDESDE) && FmDFld(fm0, FDESDE) > DFld(operac|PARTE_DIA))
						continue;
					// Fechas mayores
					if (!FmIsNull(fm0, FHASTA) && FmDFld(fm0, FHASTA) < DFld(operac|PARTE_DIA))
						continue;

					// Elimino Clientes Invalidos
					// Clientes Menor a lo pedido
					if (!FmIsNull(fm0, CLIED) && FmLFld(fm0, CLIED) > LFld(operac|PARTE_CLIENTE))
						continue;
					// Clientes Mayor a lo pedido
					if (!FmIsNull(fm0, CLIEH) && FmLFld(fm0, CLIEH) < LFld(operac|PARTE_CLIENTE))
						continue;

					sprintf(buffer, "Procesando Vigilador %ld Fecha %D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
					FmSetFld(fm0, COMENT, buffer);
					WiRefresh();
					if (CambiaDia(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA))) {
						if (chgcond) {
							ImprimeTabla();     //Incluye after y before de cada legajo dia.
							chgcond = FALSE;
						}
						cond   = *SFld(operac|PARTE_CONDIC);
						codaus = IFld(operac|PARTE_CODAUS);
						cParte = 0;
						bParte = -1;
					}
					AgregaATabla(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), IFld(operac|PARTE_PTOSER), 
						IFld(operac|PARTE_PUESTO), *SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_CODAUS));

					chgcond |= (cond != *SFld(operac|PARTE_CONDIC) || codaus!=IFld(operac|PARTE_CODAUS));
				}

				/*Por si no imprimio porque el dia desde es justo el caso*/
				if (chgcond) {
					if (*FmSFld(fm0, SALIDA) == 'A') {
						sprintf(paraarchivo, "%d\t%ld\t%s\t%02d/%02d/%04d\t", FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG), GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG)), Day(DFld(operac|PARTE_DIA)), Month(DFld(operac|PARTE_DIA)), Year(DFld(operac|PARTE_DIA)));
					}
					else {
						RpSetLFld(rp0, R_NROLEG,  LFld(operac|PARTE_NROLEG));
						RpSetFld (rp0, R_DNROLEG, GetNombreLeg(FmIFld(fm0, EMP), LFld(operac|PARTE_NROLEG)));
						RpSetDFld(rp0, R_DIA,     DFld(operac|PARTE_DIA));
					}
					ImprimeTabla();     //Incluye after y before de cada legajo dia.
					chgcond = FALSE;
				}

				FmSetFld(fm0, COMENT, NULL_STR);
			}
			else {
				legajos_ptr bleg = NULL,pleg;

				pBYemp = CreateCursor(operac|PARTEbyEMP, IO_NOT_LOCK);

				SetCursorFrom(pBYemp, FmIFld(fm0, EMP), FmIsNull (fm0, CLIED) ? MIN_LONG: FmLFld(fm0, CLIED) , FmIsNull (fm0, OBJETD) ? MIN_SHORT :  FmIFld(fm0, OBJETD), FmIsNull (fm0, FDESDE) ? MIN_DATE : FmDFld (fm0, FDESDE),
														MIN_LONG, MIN_SHORT, MIN_SHORT, MIN_SHORT);
				SetCursorTo  (pBYemp, FmIFld(fm0, EMP), FmIsNull (fm0, CLIEH) ? MAX_LONG: FmLFld(fm0, CLIEH) , FmIsNull (fm0, OBJETH) ? MAX_SHORT :  FmIFld(fm0, OBJETH), FmIsNull (fm0, FHASTA) ? MIN_DATE : FmDFld (fm0, FHASTA),
														MAX_LONG, MAX_SHORT, MAX_SHORT, MAX_SHORT);
				while (FetchCursor(pBYemp) != ERROR) {
					
					//Valida Legajo
					if (!ValidaLegajoXusr(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA)))
				       	continue;
		
					// Elimino fechas Invalidas
					// Fechas menores
					if (!FmIsNull(fm0, FDESDE) && FmDFld(fm0, FDESDE) > DFld(operac|PARTE_DIA))
						continue;
					// Fechas mayores
					if (!FmIsNull(fm0, FHASTA) && FmDFld(fm0, FHASTA) < DFld(operac|PARTE_DIA))
						continue;

					// si el legajo y fecha se repite no se ponde 2 veces
					bleg = AgregarLegajos(bleg, LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA), *SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_CODAUS));

					sprintf(buffer, "Procesando Cliente %ld Objetivo %d", LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO));
					FmSetFld(fm0, COMENT, buffer);
					WiRefresh();
				}
				for (pleg = bleg; pleg != NULL; pleg = pleg->next) {
					if (!pleg->diff) 
						continue;

					cParte = 0;
					bParte = -1;
					SetCursorFrom(pBYempl, FmIFld(fm0, EMP), pleg->nroleg, pleg->dia, FmIsNull (fm0, CLIED) ? MIN_LONG: FmLFld(fm0, CLIED) , FmIsNull (fm0, OBJETD) ? MIN_SHORT :  FmIFld(fm0, OBJETD));
					SetCursorTo  (pBYempl, FmIFld(fm0, EMP), pleg->nroleg, pleg->dia, FmIsNull (fm0, CLIEH) ? MAX_LONG: FmLFld(fm0, CLIEH) , FmIsNull (fm0, OBJETH) ? MAX_SHORT :  FmIFld(fm0, OBJETH));
					cParte = 0;
					if (*FmSFld(fm0, SALIDA) == 'A') {
						sprintf(paraarchivo, "%d\t%ld\t%s\t%02d/%02d/%04d\t", FmIFld(fm0, EMP), pleg->nroleg, GetNombreLeg(FmIFld(fm0, EMP), pleg->nroleg), Day(pleg->dia), Month(pleg->dia), Year(pleg->dia));
					}
					else {
						RpSetLFld(rp0, R_NROLEG,  pleg->nroleg);
						RpSetFld (rp0, R_DNROLEG, GetNombreLeg(FmIFld(fm0, EMP), pleg->nroleg));
						RpSetDFld(rp0, R_DIA,     pleg->dia);
					}
					while (FetchCursor(pBYempl) != ERROR) {

						//Valida Legajo
						if (!ValidaLegajoXusr(LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA)))
					       	continue;
		
						// Elimino Clientes Invalidos, esto lo deje por si modifican el fm
						// Clientes Menor a lo pedido
						if (!FmIsNull(fm0, CLIED) && FmLFld(fm0, CLIED) > LFld(operac|PARTE_CLIENTE))
							continue;
						// Clientes Mayor a lo pedido
						if (!FmIsNull(fm0, CLIEH) && FmLFld(fm0, CLIEH) < LFld(operac|PARTE_CLIENTE))
							continue;

						sprintf(buffer, "Procesando Vigilador %ld Fecha %D", LFld(operac|PARTE_NROLEG), DFld(operac|PARTE_DIA));
						FmSetFld(fm0, COMENT, buffer);
						WiRefresh();
						AgregaATabla(LFld(operac|PARTE_CLIENTE), IFld(operac|PARTE_OBJETIVO), IFld(operac|PARTE_PTOSER), 
							IFld(operac|PARTE_PUESTO), *SFld(operac|PARTE_CONDIC), IFld(operac|PARTE_CODAUS));
					}
					ImprimeTabla(); // Incluye after y before de cada legajo dia.
				}

				FmSetFld(fm0, COMENT, NULL_STR);
				DeleteLeg(bleg);
			}

			if (rp0 != ERROR)
				CloseReport(rp0);
			if (fp != (FILE*)NULL)
				fclose(fp);
			break;
		case FM_IGNORE:
			break;
		}
	}
	
	FinObjetivosXusr();
	FinClientesXusr();
}

static fm_status before(form fm, fmfield fno, int row)
{
	switch (fno) {
	case CLIED:
	   	if (listacok == FALSE || FmChgFld(fm))	{
	       	InicClientesXusr();
	    	listacok = TRUE;
	    }
    	break;
    case CLIEH:
    	break;
    case OBJETD:
		   	InicObjetivosXusr(FmLFld(fm, CLIED, row), FmIFld(fm, EMP, row));
	break;
    case OBJETH:                                 
		   	InicObjetivosXusr(FmLFld(fm, CLIEH, row), FmIFld(fm, EMP, row));
	break;
	}
	return FM_OK;				
}

static fm_status after(form fm, fmfield fno, int row)
{
	long v_nroleg = NULL_LONG;  

	switch (fno) {
		case EMP:
	    if (FmChgFld(fm))	{
		    InicListaXusr(FmIFld(fm0, EMP));
		    fecierre  = GetFechaCierreOpe(g_emp);
			InicLegajoXusr (FmIFld(fm, EMP), fecierre, fm0, COMENT, TRUE, _TIPPER_INSERTA);
        }
		    break;
		case LISTAPOR :
			switch(FmIFld(fm, fno)) {
				case 1 :
					FmSetFld(fm0, DVIGILD, NULL_STR);
					FmSetFld(fm0, DVIGILH, NULL_STR);
					break;
				case 2 :
					FmSetFld(fm0, DCLID, NULL_STR);
					FmSetFld(fm0, DCLIH, NULL_STR);
					FmSetFld(fm0, DOBJD, NULL_STR);
					FmSetFld(fm0, DOBJH, NULL_STR);
					break;
			}
			break;
		case CLIED:
			if (FmKeyCode(fm) == K_HELP)
				HelpCliente(fm, fno, row);
	  		else
			  	FmSetFld(fm, DCLID, GetDescCliente(FmLFld(fm, CLIED, row)), row);
	    break;
    	case CLIEH:
			if (FmKeyCode(fm) == K_HELP)
				HelpCliente(fm, fno, row);
  			else
				FmSetFld(fm, DCLIH, GetDescCliente(FmLFld(fm, CLIEH, row)),row);
	   	break;
    	case OBJETD:
			if (FmKeyCode(fm) == K_HELP)
				HelpObjet(fm, fno, row, FmLFld(fm, CLIED, row));
  			else
				FmSetFld(fm, DOBJD, GetObjDescrip(FmLFld(fm, CLIED, row), FmIFld(fm, OBJETD, row)), row);
		break;
    	case OBJETH:
			if (FmKeyCode(fm) == K_HELP)
				HelpObjet(fm, fno, row, FmLFld(fm, CLIEH, row));
			else	
				FmSetFld(fm, DOBJH, GetObjDescrip(FmLFld(fm, CLIEH, row) ,FmIFld(fm, OBJETH, row)), row);
		break;
		case SALIDA:
			if (*FmSFld(fm0, SALIDA) == 'A')
				FmSetFld(fm0, NOMARCH, "ctrlpar2.txt");
			else
				FmSetFld(fm0, NOMARCH, NULL_STR);
		break;
		case VIGILD:
			switch(FmKeyCode(fm)) {
			case K_HELP:
				HelpLegajo(fm, fno, row);
				break;
			case K_META:
				v_nroleg = ERROR;
				if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;

				FmSetLFld(fm, fno, v_nroleg, row);
				WiRefresh();
				break;
		}

		if (!FmIsNull(fm, fno, row)){
			if ( !ValidaLegajoXusr(FmLFld(fm, VIGILD, row), MAX_DATE))
				if (!FmIsNull(fm, fno, row)) {
					WiDialog(WD_OK, WD_OK, "Error", "El legajo no esta activo");
					return FM_REDO;
				}
		}
	   
	    FmSetFld(fm, DVIGILD, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, fno)));
			break;
  	    case VIGILH:
  	    	switch(FmKeyCode(fm)) {
			case K_HELP:
				HelpLegajo(fm, fno, row);
				break;
			case K_META:
				v_nroleg = ERROR;
				if ( (v_nroleg = MenuNOM(FmIFld(fm, EMP))) == ERROR)
					return FM_REDO;

				FmSetLFld(fm, fno, v_nroleg, row);
				WiRefresh();
				break;
		}

		if (!FmIsNull(fm, fno, row)){
			if ( !ValidaLegajoXusr(FmLFld(fm, VIGILH, row), MAX_DATE))
				if (!FmIsNull(fm, fno, row)) {
					WiDialog(WD_OK, WD_OK, "Error", "No tiene permiso para ingresar este legajo");
					return FM_REDO;
				}
		}
        
        FmSetFld(fm, DVIGILH, GetDescLegajo(FmIFld(fm0, EMP), FmLFld(fm, fno)));
  	    	break;
  	    case FDESDE:
  	    	if (FmChgFld(fm) && !FmIsNull(fm, fno) && FmDFld(fm, fno) < fecierre)	{
				InicLegajoXusr (FmIFld(fm, EMP), FmDFld(fm, fno), fm0, COMENT, TRUE, _TIPPER_INSERTA);
				FmSetFld(fm0, COMENT, "");
				WiRefresh();
		}
  	    	break;
		
	}
	return FM_OK;
}

void AbrirReporte()
{
	if (*FmSFld(fm0, SALIDA) == 'A') {
		if ((fp = fopen(FmSFld(fm0, NOMARCH),"wt")) == (FILE*)NULL)
			Error("No se pudo abrir el archivo.");
		else
			SetearCabArch();
	}
	else {
		rp0 = OpenReport("ctrlpar2", RP_EABORT|RP_NOBEGIN);

		//Si la salida es Impresora
		if (*FmSFld(fm0, SALIDA) == 'I') {
			RpSetOutput(rp0, RP_IO_DEFAULT, NULL_STR );
		}
		//Si la salida es Terminal
		if (*FmSFld(fm0, SALIDA) == 'T')
			RpSetOutput(rp0, RP_IO_TERM, NULL_STR );

		BeginReport(rp0, 1, NULL_STR);

		ImprimirCabecera();
	}
}

void SetearCabArch()
{
	fprintf(fp, "Empresa\tLegajor\tNombre y Apellido\tFecha\tCliente\tRazon Social\tObjetivo\tDescrip. Obj.\tPuesto\t\tDescr\tCondicion\tDescrip. Condic\n");
}

void ImprimirCabecera()
{
	RpSetLFld(rp0, R_VDESDE, FmLFld(fm0, VIGILD));
	RpSetLFld(rp0, R_VHASTA, FmLFld(fm0, VIGILH));

	RpSetDFld(rp0, R_FDESDE, FmDFld(fm0, FDESDE));
	RpSetDFld(rp0, R_FHASTA, FmDFld(fm0, FHASTA));

	RpSetLFld(rp0, R_CLID,  FmLFld(fm0, CLIED));
	RpSetLFld(rp0, R_CLIH,  FmLFld(fm0, CLIEH));

	RpSetIFld(rp0, R_OBJD,  FmIFld(fm0, OBJETD));
	RpSetIFld(rp0, R_OBJH,  FmIFld(fm0, OBJETH));
}

bool CambiaDia(long nroleg, DATE dia)
{
	if ((nroleg != lastleg) || (dia != lastdia)) {
		if (*FmSFld(fm0, SALIDA) == 'A') {
			sprintf(paraarchivo, "%d\t%ld\t%s\t%02d/%02d/%04d\t", FmIFld(fm0, EMP), lastleg, GetNombreLeg(FmIFld(fm0, EMP), lastleg), Day(lastdia), Month(lastdia), Year(lastdia));
		}
		else {
			RpSetLFld(rp0, R_NROLEG,  lastleg);
			RpSetFld (rp0, R_DNROLEG, GetNombreLeg(FmIFld(fm0, EMP), lastleg));
			RpSetDFld(rp0, R_DIA,     lastdia);
		}
		lastdia = dia;
		lastleg = nroleg;
		return TRUE;
	}
	return FALSE;
}
void ImprimeTabla(void)
{
	long i;
	char desaus[26];

	if (cParte <= 0)
		return;


	for (i = bParte; i != -1; i = PxLegxDia[i].pos) {

		sprintf(desaus, "%s", NULL_STR);
		SetKey(asist|INASIST, PxLegxDia[i].codaus);
		if (GetRecord(asist|INASIST, THIS_KEY, IO_NOT_LOCK)!=ERROR)
			sprintf(desaus, "%s", SFld(asist|INASIST_DESCRINAS));

		if (*FmSFld(fm0, SALIDA) == 'A') {
			fprintf(fp, "%s%ld\t%s\t%d\t%s\t%d\t%d\t%s\t%c\t%s\n",paraarchivo, PxLegxDia[i].cliente, 
				GetDescCli(PxLegxDia[i].cliente), PxLegxDia[i].objetivo, GetObjDescrip(PxLegxDia[i].cliente,
				PxLegxDia[i].objetivo), PxLegxDia[i].ptoser, PxLegxDia[i].puesto, 
				GetDescPto(PxLegxDia[i].ptoser), PxLegxDia[i].cond, desaus);
		}
		else {
			RpSetLFld(rp0, R_CLIENTE, PxLegxDia[i].cliente);
			RpSetFld(rp0, R_DCLIENTE, GetDescCli(PxLegxDia[i].cliente));
			RpSetIFld(rp0, R_OBJETIVO, PxLegxDia[i].objetivo);
			RpSetFld(rp0, R_DOBJETIVO, GetObjDescrip(PxLegxDia[i].cliente,PxLegxDia[i].objetivo));
			RpSetIFld(rp0, R_PTOSER, PxLegxDia[i].ptoser);
			RpSetIFld(rp0, R_PUESTO, PxLegxDia[i].puesto);
			RpSetFld(rp0, R_DPTOSER, GetDescPto(PxLegxDia[i].ptoser));
			RpSetFld(rp0, R_COND, &PxLegxDia[i].cond);
			RpSetFld(rp0, R_DESCOND, desaus);

			DoReport(rp0, LINEA);
		}
	}
}
void AgregaATabla(long cliente, int objetivo, int ptoser, int puesto, char cond, int codaus)
{
	long i,n;
	int cmp;

	if (cParte >= MAXPARTE) {
		WiMsg("Se supero la maxima cantidad permitida de clientes para este legajo %ld", LFld(operac|PARTE_NROLEG));
		exit(1);
	}

	PxLegxDia[cParte].cliente=cliente;
	PxLegxDia[cParte].objetivo=objetivo;
	PxLegxDia[cParte].ptoser=ptoser;
	PxLegxDia[cParte].puesto=puesto;
	PxLegxDia[cParte].cond=cond;
	PxLegxDia[cParte].codaus=codaus;
	PxLegxDia[cParte].pos=-1;

	if (bParte == -1) {
		bParte = cParte;
		cParte++;
		return;
	}
	if ((cmp = CmpParte(cParte,bParte)) < 0) {
		PxLegxDia[cParte].pos = bParte;
		bParte = cParte;
		cParte++;
		return;
	}
	if (!cmp)
		return; // es repetido y vuelvo sin mas.

	for (i = bParte, n = PxLegxDia[i].pos; n != -1; i = n, n = PxLegxDia[n].pos) {
		// es igual al siguiente -> vuelvo.
		if ((cmp = CmpParte(cParte,n)) == 0)
			return;

		// es el lugar correcto esta entre i y n.
		if (cmp < 0) {
			PxLegxDia[cParte].pos = PxLegxDia[i].pos;
			PxLegxDia[i].pos      = cParte;
			cParte++;
			return; // ya insertado vuelvo.
		}
	}
	PxLegxDia[i].pos = cParte;
	cParte++;
}

int CmpParte(long este, long sig)
{
	if ((PxLegxDia[este].cliente  == PxLegxDia[sig].cliente)  &&
		(PxLegxDia[este].objetivo == PxLegxDia[sig].objetivo) &&
		(PxLegxDia[este].ptoser   == PxLegxDia[sig].ptoser)   &&
		(PxLegxDia[este].puesto   == PxLegxDia[sig].puesto)   &&
		(PxLegxDia[este].cond     == PxLegxDia[sig].cond)     &&
		(PxLegxDia[este].codaus   == PxLegxDia[sig].codaus))
			return 0;

	if ((PxLegxDia[este].cliente  < PxLegxDia[sig].cliente  || (PxLegxDia[este].cliente  == PxLegxDia[sig].cliente  &&
		(PxLegxDia[este].objetivo < PxLegxDia[sig].objetivo || (PxLegxDia[este].objetivo == PxLegxDia[sig].objetivo &&
		(PxLegxDia[este].ptoser   < PxLegxDia[sig].ptoser   || (PxLegxDia[este].ptoser   == PxLegxDia[sig].ptoser   &&
		(PxLegxDia[este].puesto   < PxLegxDia[sig].puesto   || (PxLegxDia[este].puesto   == PxLegxDia[sig].puesto   &&
		(PxLegxDia[este].cond     < PxLegxDia[sig].cond     || (PxLegxDia[este].cond     == PxLegxDia[sig].cond     &&
		(PxLegxDia[este].codaus   < PxLegxDia[sig].codaus))))))))))))
		return -1;

		return 1;
}

void DeleteLeg(legajos_ptr ptr) {
	legajos_ptr aux;

	while (ptr != NULL) {
		aux = ptr->next;
		free(ptr);
		ptr = aux;
	}
}

legajos_ptr AgregarLegajos(legajos_ptr ptr, long nroleg, DATE dia, char cond, int codaus) {
	legajos_ptr next;

	if (ptr == NULL) { // Agrego el legajo.
		ptr = (legajos_ptr)malloc(sizeof(legajos_obj));

		ptr->next   = NULL;
		ptr->nroleg = nroleg;
		ptr->dia    = dia;
		ptr->cond   = cond;
		ptr->codaus = codaus;
		ptr->diff   = FALSE;
		return ptr;
	}
	if (nroleg < ptr->nroleg || (nroleg == ptr->nroleg && dia < ptr->dia)) {
		legajos_ptr nuevo;
		nuevo = AgregarLegajos(NULL, nroleg, dia,cond, codaus);
		nuevo->next = ptr;
		return nuevo;
	}
	for (next = ptr; next->next != NULL; next = next->next) {
		if ((next->nroleg == nroleg) && (next->dia == dia)) {
	 		next->diff |= (next->cond!=cond || next->codaus!=codaus);
			return ptr;
		}
		if (nroleg < next->next->nroleg || (nroleg == next->next->nroleg && dia < next->next->dia)) {
			legajos_ptr nuevo;
			nuevo = AgregarLegajos(NULL, nroleg, dia ,cond, codaus);
			nuevo->next = next->next;
			next->next  = nuevo;
			return ptr;
		}
	}
	if ((next->nroleg == nroleg) && (next->dia == dia)) {
		next->diff |= (next->cond!=cond || next->codaus!=codaus);
		return ptr;
	}
	next->next = AgregarLegajos(NULL, nroleg, dia,cond,codaus);
	return ptr;
}
