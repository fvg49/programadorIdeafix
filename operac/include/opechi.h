
#define	_HORAS_NORMALES_POR_SEMANA			4800
#define	_HORAS_NORMALES_POR_FERIADO			 800
#define	FECING_LIMITE					"010800"

//Estructura para cargar el parte
struct s_parte_lib {
	long cliente, nroleg;
	short objet, nrosem, ptoser, puesto, nroint;
	long st_hn, st_h50, st_h100fr, st_h100fe;
	short codcond, motivo;
	char  condic[3];
	DATE fecha;
	short origen;
	TIME hinicio, hfinal;
	bool cambio;
	struct s_parte_lib *next;
};

struct s_hs_sem_lib {
	long nroleg;
	short nrosem;
	long st_hn, st_h50;
	struct s_hs_sem_lib *next;
};

void ReclasificarHoras(short emp, long nroleg, DATE fecha);
short NroSemana(DATE fechad, DATE fecha);
void SemanaDesdeHasta(DATE fecbase, DATE *fdesde, DATE *fhasta);

