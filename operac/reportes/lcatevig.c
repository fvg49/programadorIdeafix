/********************************************************************
*
* MODULE & VERSION : @(#)lcatevig.c	1.10 
* DATE             : 04/09/24 
* TIME             : 13:39:11 
*
* CREATED          : 14/10/99
*
* DESCRIPTION:
*             Listado del desfazaje de categorias de los vigiladores.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "prosegur.sch"
#include "bill.sch"
#include "comerc.sch"
#include "operac.sch"
#include "operac.h"
#include "comerc.h"
#include "lcatevig.fmh"
#include "lcatevig.rph"
#include "sue.sch"
#include "filial.h"

// defines para el filtro de Vigiladores defasados
#define VTODOS		1
#define DEFAMAS		2
#define DEFAMENOS	3

// defines para el filtro de cambios de categoría
#define CTODOS		1
#define ULTIMO		2
#define CLI_INEX   "El Cliente: %ld no existe!."
#define OBJET_INEX "El Objetivo: %d del Cliente: %ld no existe!."

#define CANTSUPLE  MAXSUPLEM
#define OTRO  (CANTSUPLE)

typedef struct t_catev {
	long	cliente;
	int		objetivo;
	long	nroleg;
	char	apynom[50];
	DATE	fecasig;
	double	basico, basper;
	double	vecr[CANTSUPLE];
	double	vecp[CANTSUPLE];
	double	confreal;
	double	confper;
	int		codcatreal;
	int		codcatper;
	DATE	fecbaj;
	int     motivd;
	struct	t_catev *sgte;
} n_catev;
typedef n_catev *p_catev;

/* Funciones privadas */
bool ObjetivoValido (long cliente, short objetivo);
static fm_status after(form fm, fmfield fno, int row);
static fm_status before(form fm, fmfield fno, int row);
static double SueldoConf(double suplem[CANTSUPLE], double basico);
static int  LeerPuesto(long cliente, int objetivo, int ptoser, int puesto);
static bool LegSeImprime(struct	t_catev *aux, long leg);
static void LeerCliente (long cliente);
static void ListarCateVigi();
static void ImprimirInfo();
static void AbrirSalida();
static void ObtSuplementos(int puesto, double suplem[CANTSUPLE], double  sueldo_basico);
static void Asig(double suplep[CANTSUPLE], double confper, double basper, int codcatper, long vigilador);
static void AsigH(double suplep[CANTSUPLE],double confper, double basper, int codcatper, long vigilador);
static void FreeLista();
static void InsertarEnLista(long cliente, int objetivo, long nroleg, DATE fecasig, double suplem[CANTSUPLE],
							double suplep[CANTSUPLE],double basico, double basper, double confreal,
							double confper, int codcatreal, int codcatper, DATE fecbaj, int motivd);
static p_catev CrearNodo(long cliente, int objetivo, long nroleg, DATE fecasig, double suplem[CANTSUPLE],
						double suplep[CANTSUPLE],double basico, double basper, double confreal, double confper,
						int codcatreal, int codcatper, DATE fecbaj, int motivd);

/* Declaraciones globales */
schema operac, prosegur, sue, bill, comerc;
form fm0;
report rp0 = ERROR;
FILE *archi = NULL;
p_catev lista = NULL;
int convenio;

/* Programa principal */
wcmd(lcatevig, 1.10 09/24/04)
{
	fm0 = OpenForm("lcatevig", FM_EABORT);

	prosegur = OpenSchema("prosegur", IO_EABORT);
	comerc   = OpenSchema("comerc",   IO_EABORT);
	bill     = OpenSchema("bill",     IO_EABORT);
	sue      = OpenSchema("sue",      IO_EABORT);
	operac   = OpenSchema("operac",   IO_EABORT);

  	FmSetFld(fm0, DPROCVIG, "[1mProcesando Permisos de Filiales por Usuario[0m");
  	WiRefresh();
	InicListaXusr(StrToI(ReadEnv("emp")));
	FmSetFld(fm0, DPROCVIG, "");
	WiRefresh();

	if (DoForm(fm0, before, after) != FM_UPDATE) return;

	convenio = GetConvenioPorEmp(FmIFld(fm0, EMP));

	ListarCateVigi();
	
	FinObjetivosXusr();
	FinClientesXusr();
	FinListaXusr();
}

static void ListarCateVigi()
{
	dbcursor c_per;
	double   supper[CANTSUPLE], confper, basper, ticket, suelad, pread, tickad;

	c_per = CreateCursor(sue|PERbyEMP, IO_NOT_LOCK);

	SetCursorFrom(c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGID));
	SetCursorTo  (c_per, FmIFld(fm0, EMP), FmLFld(fm0, VIGIH));
	while (FetchCursor(c_per) != ERROR) {

		if(!ValidaConvenioXEmp(FmIFld(fm0, EMP), IFld(sue|PER_RELACION)) )
			continue;

		if (!IsNull(sue|PER_FECEGR) && DFld(sue|PER_FECEGR) < FmDFld(fm0, FDESDE))
			continue;
		FmSetLFld(fm0, PROCVIGI, LFld(sue|PER_NROLEG));
		FmSetFld (fm0, DPROCVIG, SFld(sue|PER_APYNOM));
		WiRefresh();

		// obtengo los valores del puesto donde esta asignado en sue|PER.
		// Ver porque habria que ir a suer|PERH con la ultima liquidacion.

		GetSBasico(convenio, IFld(sue|PER_CODCAT), &basper, &ticket, &suelad, &pread, &tickad);
		ObtSuplementos(IFld(sue|PER_CODCAT), supper, basper);
		basper = (basper == NULL_DOUBLE) ? 0 : basper;
		confper = SueldoConf(supper, basper);
		Asig(supper, confper, basper, IFld(sue|PER_CODCAT), LFld(sue|PER_NROLEG));
		AsigH(supper,confper, basper, IFld(sue|PER_CODCAT), LFld(sue|PER_NROLEG));
	}
	ImprimirInfo();
	FreeLista();
}

static void AsigH(double suplep[CANTSUPLE], double confper, double basper, int codcatper, long vigilador)
{
	dbcursor c_asigh;
	long nroleg = NULL_LONG;
	int puesto  = NULL_SHORT;
	double supreal[CANTSUPLE], basicoreal, confreal, ticket, suelad, pread, tickad;

	c_asigh = CreateCursor(operac|ASIGHbyNROLEG, IO_NOT_LOCK);
	SetCursorFrom(c_asigh, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asigh, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_asigh) != ERROR) {
		if (FmIFld(fm0, LISTAPOR) == 1) {
			if (!ObjetivoValido(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
				continue;
		}

		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO)))
	       	continue;
		
		if (!ValidaFilial(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), FmSFld(fm0, FDELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		if (ExisteCliObjEnGrp(GRPPERDISPS, LFld(operac|ASIGH_CLIENTE), NULL_SHORT) &&
			DFld(operac|ASIGH_FECALT) < FmDFld(fm0, FDESDE) && DFld(operac|ASIGH_FECBAJ) > FmDFld(fm0, FHASTA))
			continue;
		if (IFld(operac|ASIGH_MOTIVO) == DESXERROR)
			continue;
		if (DFld(operac|ASIGH_FECBAJ) < FmDFld(fm0, FDESDE))
			continue;
		if (DFld(operac|ASIGH_FECALT) > FmDFld(fm0, FHASTA)) {
			nroleg = LFld(operac|ASIGH_NROLEG);
			continue;
		}
		if (!strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO)) { // && DFld(operac|ASIGH_FECHAS) < FmDFld(fm0, FDESDE)) {
			nroleg = LFld(operac|ASIGH_NROLEG);
			continue;
		}

// cambiado por javier - 30/11/1999
//		if (FmIFld(fm0, CAMBCATE) == ULTIMO && strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO)) {
		if (!strcmp(SFld(operac|ASIGH_EFECT), PROVISORIO)) {
			nroleg = LFld(operac|ASIGH_NROLEG);
			continue;
		}

		// obtengo los valores del puesto donde trabajo
		puesto = LeerPuesto(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO),
							IFld(operac|ASIGH_PTOSER), IFld(operac|ASIGH_PUESTO));
		GetSBasico(convenio, puesto, &basicoreal, &ticket, &suelad, &pread, &tickad);
		ObtSuplementos(puesto, supreal, basicoreal);

		basicoreal = (basicoreal == NULL_DOUBLE) ? 0.0 : basicoreal;

		confreal = SueldoConf(supreal, basicoreal);

		if (FmIFld(fm0, DEFASA) == DEFAMAS && confper <= confreal)
			continue;

		if (FmIFld(fm0, DEFASA) == DEFAMENOS && confper >= confreal)
			continue;

		InsertarEnLista(LFld(operac|ASIGH_CLIENTE), IFld(operac|ASIGH_OBJETIVO), LFld(operac|ASIGH_NROLEG),
						DFld(operac|ASIGH_FECALT), supreal, suplep, basicoreal, basper, confreal, confper,
						puesto,	codcatper, DFld(operac|ASIGH_FECBAJ), IFld(operac|ASIGH_MOTIVO));
		nroleg = LFld(operac|ASIGH_NROLEG);
	}
}

static void Asig(double suplep[CANTSUPLE], double confper, double basper, int codcatper, long vigilador)
{
	dbcursor c_asig;
	long nroleg = NULL_LONG;
	int puesto  = NULL_SHORT;
	double supreal[CANTSUPLE], basicoreal, confreal, ticket, suelad, pread, tickad;

	c_asig = CreateCursor(operac|ASIGbyNROLEG, IO_NOT_LOCK);
	SetCursorFrom(c_asig, FmIFld(fm0, EMP), vigilador, MIN_LONG, MIN_SHORT);
	SetCursorTo  (c_asig, FmIFld(fm0, EMP), vigilador, MAX_LONG, MAX_SHORT);
	while(FetchCursor(c_asig) != ERROR) {
		if (FmIFld(fm0, LISTAPOR) == 1) {
			if (!ObjetivoValido(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
				continue;
		} 
   
		//valida el cliente/objetivo para el usuario
		if (!ValidaListaXusr(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO)))
	       	continue;
		
		if (!ValidaFilial(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), FmSFld(fm0, FDELEG), FmSFld(fm0, FDELEGA), FmSFld(fm0, FFILIAL)))
			continue;
		
		if (LFld(operac|ASIG_CLIENTE) == PPERDISP && DFld(operac|ASIG_FECASIG) < FmDFld(fm0, FDESDE))
			continue;
		if (DFld(operac|ASIG_FECASIG) > FmDFld(fm0, FHASTA)) {
			nroleg = LFld(operac|ASIG_NROLEG);
			continue;
		}
		if (!strcmp(SFld(operac|ASIG_EFECT), PROVISORIO)) { 
			nroleg = LFld(operac|ASIG_NROLEG);
			continue;
		}
		//en la opción cambcate = ultimo, solo debe considerarse el ultimo puesto efectivo del vigilador.
		if (FmIFld(fm0, CAMBCATE) == ULTIMO && !strcmp(SFld(operac|ASIG_EFECT), PROVISORIO)) {
			nroleg = LFld(operac|ASIG_NROLEG);
			continue;
		}

		// obtengo los valores del puesto donde trabajo
		puesto = LeerPuesto(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), IFld(operac|ASIG_PTOSER),
							IFld(operac|ASIG_PUESTO));
		GetSBasico(convenio, puesto, &basicoreal, &ticket, &suelad, &pread, &tickad);
		ObtSuplementos(puesto, supreal, basicoreal);
		basicoreal = (basicoreal == NULL_DOUBLE) ? 0 : basicoreal;
		confreal = SueldoConf(supreal, basicoreal);

		if (FmIFld(fm0, DEFASA) == DEFAMAS && confper <= confreal)
			continue;
		if (FmIFld(fm0, DEFASA) == DEFAMENOS && confper >= confreal)
			continue;

		InsertarEnLista(LFld(operac|ASIG_CLIENTE), IFld(operac|ASIG_OBJETIVO), LFld(operac|ASIG_NROLEG),
						DFld(operac|ASIG_FECASIG), supreal, suplep, basicoreal, basper, confreal, confper,
						puesto, codcatper, NULL_DATE, NULL_SHORT);
		nroleg = LFld(operac|ASIG_NROLEG);
	}
}

static void ImprimirInfo()
{
	bool lei = FALSE, imprimo = FALSE;
	long leg = NULL_LONG;
	p_catev aux;

	if (lista == NULL) {
		Warning("No hay datos para emitir el listado.");
		return;
	}

	for (aux = lista; aux != NULL; aux = aux->sgte) {
		if (rp0 == ERROR && archi == NULL) {
			AbrirSalida();
		}
		if (aux->nroleg != leg) {
			leg = aux->nroleg;
			imprimo = LegSeImprime(aux, leg);
		}
		// Leo el Legajo
		if (!lei) {
			SetKey(sue|PERbyEMP, FmIFld(fm0, EMP), aux->nroleg);
			if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) == ERROR) {
				Warning("No existe el Vigilador: %ld", aux->nroleg);
				return;
			}
			lei = TRUE;
		}
		if (rp0 != ERROR && imprimo &&  (aux->confper - aux->confreal) < 0) {
			RpSetLFld(rp0, RNROLEG,   aux->nroleg);
			RpSetFld (rp0, RAPYNOM,   aux->apynom);
			RpSetDFld(rp0, RFECASIG,  aux->fecasig);
			RpSetDFld(rp0, RFECBAJ,   aux->fecbaj);
			RpSetLFld(rp0, RCLIENTE,  aux->cliente);
			RpSetIFld(rp0, ROBJETIVO, aux->objetivo);
			RpSetIFld(rp0, RCATEG1,    aux->codcatreal);
			RpSetFFld(rp0, RBASICO1,   (aux->basico) / 10) ;
			RpSetLFld(rp0, R_PLUS1,    (aux->vecr[SUPLE_PLUS]) /10);
			RpSetLFld(rp0, R_TK1,       (aux->vecr[SUPLE_PORC]) /10);   
			RpSetLFld(rp0, R_OTROS1,     (aux->vecr[OTRO])/ 10);  
            RpSetFFld(rp0, RCONFCLI,  (aux->confreal) / 10 );
 
			RpSetIFld(rp0, RCATEG2,		aux->codcatper);
			RpSetFFld(rp0, RBASICO2, 	aux->basper / 10 );
			RpSetLFld(rp0, R_PLUS2, 	aux->vecp[SUPLE_PLUS] / 10);
			RpSetLFld(rp0, R_TK2,		aux->vecp[SUPLE_PORC] / 10);
			RpSetLFld(rp0, R_OTROS2,	aux->vecp[OTRO] / 10);
			RpSetFFld(rp0, RCONFCOBR, (aux->confper)  / 10);
			
			RpSetFFld(rp0, RDIFSAL,   (aux->confper - aux->confreal) / 10);
			DoReport(rp0, LINEA);
		}
		if (archi != NULL && imprimo && (aux->confper - aux->confreal) < 0) {
			LeerCliente (aux->cliente);
                           
			fprintf(archi, "%ld\t", 	aux->nroleg);
			fprintf(archi, "%s\t", 		aux->apynom);
			fprintf(archi, "%1.1D\t", 	aux->fecasig);
			fprintf(archi, "%1.1D\t", 	aux->fecbaj);
			fprintf(archi, "%ld\t", 	aux->cliente);
			fprintf(archi, "%s\t", 		SFld(bill|CLIENTE_RAZSOC));
			fprintf(archi, "%d\t", 		aux->objetivo);
			fprintf(archi, "%s\t",		GetObjDescrip(aux->cliente, aux->objetivo));
			fprintf(archi, "%d\t",		aux->codcatreal);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->basico / 1000 * 100)) /100);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->vecr[SUPLE_PLUS] / 1000 *100)) /100);
			/* El (double)(long) --> Casteo: Sirve para no redondear. Dado que un caso fue: un valor 5.7491 y en la salida por archivo era 
			%.2f en vez de imprimir 5,74 imprimira 5,75 porque se redondeaba automaticamente por el C++ 	*/
			fprintf(archi, "%.2f\t",	(double)((long)(aux->vecr[SUPLE_PORC] /1000 * 100)) / 100);  //Se 
			fprintf(archi, "%9.2f\t",   (double)((long)(aux->vecr[OTRO]/1000* 100)) / 100);    
			fprintf(archi, "%9.2f\t \t",(double)((long)(aux->confreal / 1000 * 100 )) / 100);            

			fprintf(archi, "%d\t",		aux->codcatper);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->basper / 1000 * 100)) /100);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->vecp[SUPLE_PLUS] / 1000 * 100 )) /100);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->vecp[SUPLE_PORC] / 1000 * 100 )) /100);
			fprintf(archi, "%9.2f\t",	(double)((long)(aux->vecp[OTRO] / 1000 * 100 )) /100);
			fprintf(archi, "%9.2f\t\t",	(aux->confper)  / 1000);
 
			fprintf(archi, "%9.2f\t",	(double)((long)((aux->confper - aux->confreal) / 1000 *100 ))/100);
			fprintf(archi, "%d\t",		aux->motivd);
			fprintf(archi, "%s\n",		GetDescMotivd(aux->motivd));
					
		}
	}
}

static bool LegSeImprime(struct	t_catev *aux, long leg)
{
	bool imprimo = FALSE;

	for (; aux != NULL && aux->nroleg == leg; aux = aux->sgte) {
		if (FmIFld(fm0, DIFER) && FmIFld(fm0, COMPSUE)) {
			if ((aux->confper - aux->confreal) / 10 == 0 &&
				
				(aux->vecr[SUPLE_PORC] != aux->vecp[SUPLE_PORC] ||
				 aux->vecr[SUPLE_PLUS] != aux->vecp[SUPLE_PLUS] ||
				 aux->vecr[OTRO]       != aux->vecp[OTRO])) {
				return TRUE;
			}
		}
		else {
			if ((aux->confper - aux->confreal) / 10 != 0 ||
				((aux->confper - aux->confreal) / 10 == 0 && FmIFld(fm0, DIFER)))
				imprimo = TRUE;

			if ((aux->vecr[SUPLE_PORC] != aux->vecp[SUPLE_PORC] ||
				 aux->vecr[SUPLE_PLUS] != aux->vecp[SUPLE_PLUS] ||
				 aux->vecr[OTRO]       != aux->vecp[OTRO]) && FmIFld(fm0, COMPSUE))
				imprimo = TRUE;
		}
	}
	return imprimo;
}

static void AbrirSalida()
{
	if (!strcmp(FmSFld(fm0, SALIDA), "A")) {
		if ((archi = fopen(FmSFld(fm0, NOMARCH) , "w")) == NULL)
			Error("NO se puede abrir el archivo %s", FmSFld(fm0, NOMARCH));

		fprintf(archi, "Legajo\t");
		fprintf(archi, "Apel.yNombre\t");
		fprintf(archi, "F.Asig.\t");
		fprintf(archi, "F.Hasta\tCliente\t");
		fprintf(archi, "Raz.Soc.\t");
		fprintf(archi, "Objet.\t");
		fprintf(archi, "Descrip.\t");
		fprintf(archi, "Categ.\t");
		fprintf(archi, "Bas.\t");
		fprintf(archi, "Plus.\t");
		fprintf(archi, "Tick.Valor\t");
		fprintf(archi, "Otros\t");
		fprintf(archi, "Conf.Cli.\t \t");

		fprintf(archi, "Categ.Cob.\t");
		fprintf(archi, "Bas.Cob.\t");
		fprintf(archi, "Plus. Cob\t");
		fprintf(archi, "Tick.Valor Cob\t");
		fprintf(archi, "Otros\t");
		fprintf(archi, "Conf.Cobr.\t\t");

		fprintf(archi, "Dif.Sal.\t\t");
		fprintf(archi, "Motivo Desasignacion\t");
		fprintf(archi, "Descrip Motivo Desasignacion\n");
	}
	else {
		rp0 = OpenReport("lcatevig", RP_NOBEGIN|RP_EABORT, 1);
		RpSetOutput(rp0, (!strcmp(FmSFld(fm0, SALIDA), "I") ? RP_IO_DEFAULT : RP_IO_TERM), NULL_STR);
		if (BeginReport(rp0, 1, NULL_STR) != OK) {
			WiMsg("No se pudo abrir el reporte.");
			Stop(0);
		}
		RpSetFld (rp0, RDEMP,   FmSFld(fm0, DEMP));
		RpSetDFld(rp0, RFDESDE, FmDFld(fm0, FDESDE));
		RpSetDFld(rp0, RFHASTA, FmDFld(fm0, FHASTA));
		RpSetLFld(rp0, RVIGID,  FmLFld(fm0, VIGID));
		RpSetFld (rp0, RDVIGIH, FmSFld(fm0, DVIGIH));
		RpSetLFld(rp0, RVIGIH,  FmLFld(fm0, VIGIH));
		RpSetFld (rp0, RDVIGID, FmSFld(fm0, DVIGID));
		RpSetIFld(rp0, RDEFA,   FmIFld(fm0, DEFASA));
		RpSetFld (rp0, RDDEFA,  FmSFld(fm0, DDEFASA));
		RpSetIFld(rp0, RCAMBC,  FmIFld(fm0, CAMBCATE));
		RpSetFld (rp0, RDCAMBC, FmSFld(fm0, DCAMBCAT));
	}
}

static void ObtSuplementos(int puesto, double suplem[CANTSUPLE], double sueldo_basico)
{
	int i;    
	double porcentaje = NULL_DOUBLE;

	for (i = 0 ; i < MAXSUPLEM ; i++)
		suplem[i] = 0.0;

	SetIFld(prosegur|CATESUPL_RELACION, convenio);
	SetIFld(prosegur|CATESUPL_CODCAT, puesto);
	SetIFld(prosegur|CATESUPL_CODSUPL, MIN_SHORT);
	
	while (GetRecord(prosegur|CATESUPLbyRELACION, NEXT_KEY|PARTIAL_KEY, IO_NOT_LOCK, 2) != ERROR) {
		switch (IFld(prosegur|CATESUPL_CODSUPL)) {
			case SUPLE_PLUS :
				suplem[SUPLE_PLUS] = FFld(prosegur|CATESUPL_VAL);
				break;   
					
			case SUPLE_PORC :
			    porcentaje = FFld(prosegur|CATESUPL_VAL);
				suplem[SUPLE_PORC]= ((suplem[SUPLE_PLUS] + sueldo_basico) * (porcentaje / 100)) / 1000; 
				break;
				
			default :
				suplem[OTRO] = FFld(prosegur|CATESUPL_VAL);
				break;					
		}
	}
}

static p_catev CrearNodo(long cliente, int objetivo, long nroleg, DATE fecasig, double suplem[CANTSUPLE],
						double suplep[CANTSUPLE],double basico, double basper,  double confreal, double confper,
						int codcatreal, int codcatper, DATE fecbaj, int motivd)
{
	p_catev aux = NULL;
	int i;

	if ((aux = (p_catev) Alloc (sizeof(n_catev))) == NULL) {
		WiMsg("No hay más memoria!!!!!!!!!!!!!");
		Stop(0);
	}
	aux->cliente = cliente;
	aux->objetivo = objetivo;
	aux->nroleg = nroleg;
	strcpy(aux->apynom, SFld(sue|PER_APYNOM));
	aux->fecasig = fecasig;
	aux->basico = basico;
	aux->basper = basper;
	aux->confreal = confreal;
	aux->confper = confper;
	aux->codcatreal = codcatreal;
	aux->codcatper = codcatper;
	aux->fecbaj = fecbaj;
	aux->motivd = motivd;
	aux->sgte = NULL;

	for (i = 0 ; i < MAXSUPLEM ; i++)
		aux->vecr[i] = suplem[i];

	for (i = 0 ; i < MAXSUPLEM ; i++)
		aux->vecp[i] = suplep[i];

	return aux;
}

static void InsertarEnLista(long cliente, int objetivo, long nroleg, DATE fecasig, double suplem[CANTSUPLE],
							double suplep[CANTSUPLE], double basico, double basper, double confreal,
							double confper,	int codcatreal, int codcatper, DATE fecbaj, int motivd)
{
	p_catev aux = NULL, aux1 = NULL, ant1 = NULL;

	if (FmIFld(fm0, CAMBCATE) == ULTIMO) {
		// lo busco. si encuentro el legajo, me fijo si la asignacion es posterior que la que esta en la
		// lista - recordar que con esta opcion solo necesito el ultimo puesto efectivo - borro el
		// nodo e inserto
		for (aux1 = lista, ant1 = lista ; aux1 != NULL && aux1->nroleg != nroleg ;
			ant1 = aux1, aux1 = aux1->sgte);

		if (aux1 != NULL && aux1->nroleg == nroleg) {
			if (aux1->fecasig < fecasig) {
				if (lista == ant1) {
					lista = aux1->sgte;
				}
				else {
					ant1->sgte = aux1->sgte;
				}
				Free(aux1);
			}
			else {
				return;
			}
		}
		for (aux1 = lista, ant1 = lista ; aux1 != NULL && ((aux1->cliente < cliente) ||
			(aux1->cliente == cliente && aux1->objetivo < objetivo) ||
			(aux1->cliente == cliente && aux1->objetivo == objetivo && aux1->nroleg < nroleg)) ;
			ant1 = aux1, aux1 = aux1->sgte) {
		}
	}

	if (FmIFld(fm0, CAMBCATE) == CTODOS) {
		for (aux1 = lista, ant1 = lista ; aux1 != NULL && ((aux1->nroleg < nroleg) ||
			(aux1->nroleg == nroleg && aux1->fecasig < fecasig)) ; ant1 = aux1, aux1 = aux1->sgte);
	}
	aux = CrearNodo(cliente, objetivo, nroleg, fecasig, suplem, suplep, basico, basper, confreal, confper,
					codcatreal, codcatper, fecbaj, motivd);

	if (lista == NULL) {
		lista = aux;
		return;
	}
	if (lista == aux1) {
		aux->sgte = lista;
		lista = aux;
	}
	else {
		aux->sgte = ant1->sgte;
		ant1->sgte = aux;
	}
}

static void FreeLista()
{
	p_catev	recorre, aux;
	for (recorre = lista; recorre; ) {
		aux     = recorre;
		recorre = recorre->sgte;
		Free(aux);
	}
	lista	=	NULL;
}

static int LeerPuesto(long cliente, int objetivo, int ptoser, int puesto)
{
	SetKey(operac|PUESTOSbyCLIENTE, cliente, objetivo, ptoser, puesto);
	if (GetRecord(operac|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning("No existe el puesto asociado al parte del cliente: %ld - objetivo: %d\nPuesto de Servicio: %d Puesto: %d",
				cliente, objetivo, ptoser, puesto);
		return NULL_SHORT;
	}
	return IFld(operac|PUESTOS_PUESTO);
}

static double SueldoConf(double suplem[CANTSUPLE], double basico)
{
	int i;
	double total = 0.0;
	for (i = 0 ; i < MAXSUPLEM ; i++) {
		total += suplem[i];
	}
	total += basico;
	return total;
}

static void LeerCliente(long cliente)
{
	SetKey(bill|CLIENTEbyCLIENTE, cliente);
	if (GetRecord(bill|CLIENTEbyCLIENTE, THIS_KEY, IO_NOT_LOCK) == ERROR) {
		Warning(CLI_INEX, cliente);
	}
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
//					FmSetFld(fm0, DVIGD, NULL_STR);
//					FmSetFld(fm0, DVIGH, NULL_STR);
					break;
				case 2 :
					FmSetFld(fm0, DCLID, NULL_STR);
					FmSetFld(fm0, DCLIH, NULL_STR);
					FmSetFld(fm0, DOBJD, NULL_STR);
					FmSetFld(fm0, DOBJH, NULL_STR);
					break;
			}
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

bool ObjetivoValido(long cliente, short objetivo)
{
	if (cliente < FmLFld(fm0, CLIDESDE))
		return FALSE;

	if (cliente > FmLFld(fm0, CLIHASTA))
		return FALSE;

	if (cliente == FmLFld(fm0, CLIDESDE) && objetivo < FmIFld(fm0, OBJDESDE))
		return FALSE;

	if (cliente == FmLFld(fm0, CLIHASTA) && objetivo > FmIFld(fm0, OBJHASTA))
		return FALSE;

	return TRUE;
}
