#define HSNORM 0
#define HSAL25 1
#define HSAL35 2
#define HSFRAN 3
#define HSFERI 4
#define HSNONO 5
#define HS25NO 6
#define HS35NO 7
#define HSPEGA 8
#define HSNOFE 9
#define HS25FE 10
#define HS35FE 11
#define HNOFEN 12
#define H25FEN 13
#define H35FEN 14
#define HSNOFR 15
#define HS25FR 16
#define HS35FR 17
#define HNOFRN 18
#define H25FRN 19
#define H35FRN 20

#define MAXTIPHOR 21

#define HOR_INI_DIA "06:00"
#define HOR_FIN_DIA "22:00"

#define MIN_HS_NOCT 4

#define _MIN_HS_PEGADAS 8 

#define _HORAS_REGIMEN_DEFAULT 8

#define MSG_ERR_MEM "Se Supero la Memoria Disponible,\n Avise Urgente a Sistemas"

#define DESTINO_REPORTE 1
#define DESTINO_PASE    2
#define DESTINO_ARCHIVO 3
#define DESTINO_TEST 4 //Este modo se agrego para las pruebas de aceptacion de usuario


/* Tipo de Cierre de Liquidacion */
#define TIPO_CIERRE_MENSUAL 1
#define TIPO_CIERRE_ULTIMA_SEMANA 2


typedef struct stnempres * tnempres;
typedef struct stnnodleg * tnnodleg;
typedef struct stnnodclie * tnnodclie;
typedef struct stnobjnod * tnobjnod;
typedef struct stndianod * tndianod;
typedef struct stnpuenod * tnpuenod;
typedef struct stnhordes * tnhordes;

typedef struct stnempres {
	int		empres;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tnnodleg	nnodleg;
	tnempres	nsig;
} stnempres;

typedef struct stnnodleg {
	long	nodleg;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tnnodclie	nnodclie;
	tnnodleg	nsig;
} stnnodleg;

typedef struct stnnodclie {
	long	nodclie;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tnobjnod	nobjnod;
	tnnodclie	nsig;
} stnnodclie;

typedef struct stnobjnod {
	int		objnod;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tnpuenod	npuenod;
	tnobjnod	nsig;
} stnobjnod;

typedef struct stnpuenod {
	char	puenod[8];
	double	cantih[MAXTIPHOR];
	int		n_ptoser;
	int		n_puesto;
	int		n_codint;
	TIME	n_horini;
	TIME	n_horfin;
	char	n_dia1;
	char	n_dia2;
	char	n_dia3;
	char	n_dia4;
	char	n_dia5;
	char	n_dia6;
	char	n_dia7;
	char	n_regim[12];
	long	n_canvig;
	int		n_canpto;
	char	n_tipdia;
	DATE	n_fecini;
	DATE	n_fecfin;
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tndianod	ndianod;
	tnpuenod	nsig;
} stnpuenod;

typedef struct stndianod {
	DATE	dianod;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		diasnoct;
	int		diasdiur;
	int		canpeg;
	tnhordes	nhordes;
	tndianod	nsig;
} stndianod;


typedef struct stnhordes {
	TIME	hordes;
	TIME	horhas;
	char	condic;
	int		codnov;
	double	cantih[MAXTIPHOR];
//	int		diasause;
//	int		diasfran;
//	int		diasfrat;
//	int		diasvaca;
//	int		diasvact;
	int		diasadel;
	int		canpeg;
	tnhordes	nsig;
} stnhordes;

tnempres	inicio;


// Lista de nocturnidad 
typedef struct stnlegajonoc * tnlegajonoc;
typedef struct stnfechanoc * tnfechanoc;

typedef struct stnlegajonoc {
	long	legajonoc;
	tnfechanoc	nfechanoc;
	tnlegajonoc	nsig;
} stnlegajonoc;

typedef struct stnfechanoc {
	DATE	fechanoc;
	tnfechanoc	nsig;
} stnfechanoc;

int		NivCon, partial;
int		empres, /*diasause, diasfran, diasfrat, diasvaca, diasvact, */objnod, diasnoct, diasdiur, diasadel, n_ptoser, n_puesto, canpeg, n_canpto, n_codint, codnov;
long	nodclie, nodleg, n_canvig;
double	cantih[MAXTIPHOR];
char	puenod[8], n_dia1, n_dia2, n_dia3, n_dia4, n_dia5, n_dia6, n_dia7, n_tipdia, n_regim[12], condic;
DATE	dianod, n_fecini, n_fecfin;
TIME	hordes, horhas,n_horini, n_horfin;;
bool	yacargo;

int		NivCon;
int		g_i;  

tnempres	ini;
tnlegajonoc	ini1, ini2, ini3, ini4, ini5, ini6;

long	legajonoc;
DATE	fechanoc;


tnempres CargaDistrHoras(int p_empd, int p_emph, long p_clid, long p_clih, int p_objd, int p_objh, long p_legd, long p_legh, DATE p_fecd, DATE p_fech, 
                         int p_retro, bool p_act, int p_destino, long p_liqui, bool p_ignhs0, char * p_msgerr);

bool LegajoActivo(int p_emp, long p_nodleg);
void CalcDistrHoras(int, long, DATE, char, TIME, TIME, long, int, bool, int, int, int, double*);
void CalcDistrHoras2(int, long, DATE, char, TIME, TIME, long, int, bool, int, int, int, double*);

void Asignar(double* p_origen, double* p_destino ,double* p_tope);


tnempres AcuNEmpres(tnempres, tnempres*);
tnnodleg AcuNNodleg(tnnodleg, tnnodleg*);
tnnodclie AcuNNodclie(tnnodclie, tnnodclie*);
tnobjnod AcuNObjnod(tnobjnod, tnobjnod*);
tndianod AcuNDianod(tndianod, tndianod*);
tnpuenod AcuNPuenod(tnpuenod, tnpuenod*);
tnhordes AcuNHordes(tnhordes, tnhordes*);

void BorNEmpres(tnempres);
void BorNNodleg(tnnodleg);
void BorNNodclie(tnnodclie);
void BorNObjnod(tnobjnod);
void BorNDianod(tndianod);
void BorNPuenod(tnpuenod);
void BorNHordes(tnhordes);


tnlegajonoc AcuNLegajonoc(tnlegajonoc, tnlegajonoc*);
tnfechanoc AcuNFechanoc(tnfechanoc, tnfechanoc*);

void LisNLegajonoc(tnlegajonoc);
void LisNFechanoc(tnfechanoc);

void BorNLegajonoc(tnlegajonoc);
void BorNFechanoc(tnfechanoc);

short GetTipoLiquidacionByNroLiq(short p_emp, long p_liqui);
