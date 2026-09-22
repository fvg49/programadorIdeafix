#include "ambiente.h"

//#define _GETHORASTURNO 1
#define _ROL_14x7x12 53
/* Defines */
#define HSEMP		1
#define dia(f) ((f%7==0?'D':(f%7==1?'L':(f%7==2?'M':(f%7==3?'X':(f%7==4?'J':(f%7==5?'V':'S')))))))

// campo TIPCIER de la tabla CIERRE:
#define	PARCIAL		1
#define	DEFINITIVO	2

// Clientes/Objetivos de Prosegur
#define PENFPRO       3   //Prosegur Enfernos Prolongados
#define OBJENFPRO    21   //Objetivo Enfermos Prolongados
#define PPTO2      1006   //Prosegur Plata Abril Pto 2
#define OBJPTO2       1   //Objetivo Plata Abril Pto 2
#define PSAVINT    1008   //Prosegur Sin Aviso para Intimar
#define OBJSAVINT     1   //Objetivo Sin Aviso para Intimar
#define PLICSG     1009   //Prosegur Lic. sin goce
#define OBJLICSG      1   //Objetivo Lic. sin goce
#define PPERSUSP   1010   //Prosegur Pers. Suspendidos
#define OBJPERSUP     1   //Objetivo Pers. Suspendidos
#define PPERDISP   1011   //Prosegur Pers. en Disponibilidad
#define OBJPERDISP    1   //Objetivo Pers. en Disponibilidad
#define PPERBAJ    1012   //Prosegur Pers. de Baja
#define OBJPERBAJ     1   //Objetivo Pers. de Baja
#define PPENASIG   1013   //Prosegur Pendiente ASignar
#define OBJPENASIG    1   //Objetivo Pendiente ASignar
#define	CLIPROS	   1000   //Prosegur Argentina S.A.
#define OBJPTO1       1   //Objetivo Plata Abril Pto 1
#define SUPERBS       4   //Objetivo Supervisores Bs. As.
#define	OBJPROS       6   //Objetivo Retenes
#define	PROSCOR	   1001   //Prosegur Argentina Cordoba
#define RETCOR        1   //Retenes Cordoba
#define	PROSBB	   1002   //Prosegur Argentina Bahia Blanca
#define RETBSAS       1   //Retenes Bs. As.
#define RETNEU        2   //Retenes Neuquen
#define RETRION       3   //Retenes Rio Negro
#define RETLAPA       4   //Retenes La Pampa
#define RETCHU        5   //Retenes Chubut 
#define RETSCRUZ      6   //Retenes Santa Cruz
#define RETFUEGO      7   //Retenes Tierra del Fuego
#define	PROSMEN	   1003   //Prosegur Argentina Mendoza
#define RETMEN        1   //Retenes Mendoza 
#define RETSJUAN      2   //Retenes San Juan
#define RETSLUIS      3   //Retenes San Luis
#define	PROSLIT	   1004   //Prosegur Argentina Litoral
#define RETSTAFE      1   //Retenes Santa Fe
#define RETRIOS       2   //Retenes Entre Rios
#define RETCHA        3   //Retenes Chaco
#define RETCORR       4   //Retenes Corrientes
#define RETFOR        5   //Retenes Formosa
#define RETMIS        6   //Retenes Misiones
#define	PROSNOR	   1005   //Prosegur Argentina Noroeste
#define RETTUC        1   //Retenes Tucuman
#define RETSAL        2   //Retenes Salta
#define RETSGOES      3   //Retenes Sgo. del Estero
#define RETCAT        4   //Retenes Catamarca
#define RETRIOJA      5   //Retenes La Rioja
#define RETJUJ        6   //Retenes Jujuy

//Grupos de Clientes Especiales
#define GRPRETENES		9000  //Grupo de Retenes
#define GRPPERDISPS		9001  //Grupo de Pers. en Disponibilidad
#define GRPRETPLANTA	9002  //Grupo de Retenes Planta
#define GRPBRIGADA		9003  //Grupo de Brigada
#define GRPCONAVISO		9004  //Grupo de Ausente Con Aviso
#define GRPSINAVISO		9005  //Grupo de Ausente Sin Aviso
#define GRPGRABAINS		9006  //Grupo de Grabar en Inasistencias
#define GRPNOGRABAINS	9007  //Grupo de No Graba en Inasistencias
#define GRPAUSENTE		9008  //Grupo de Condición Ausente
#define GRPTRABAJA		9009  //Grupo de Condición Trabaja
#define GRPNULLAUS		9010  //Grupo de No Graba Cod. de Ausente
#define GRPOBJPLANTA	9011  //Grupo de Objetivos Planta que generan std
#define GRPTRAFRA		9012  //Grupo de Condición Trabaja y Franco
#define GRPOTSINASIG	9014  //Grupo de OTs sin Asignar
#define GRPGENT00 		9015  //Grupo de Genera parte en T 00:00 00:00

// Grupos de Clientes Especiales Propios de Uruguay
#define GRPNOCOSTD		9100  //No considera Std
#define GRPGENAUS		9200  //Genera ausente
#define GRPNOPADEN		9300  //No pasa horas a Denarius
#define GRPBAJA			9400  //Grupo de Bajas
#define GRPPARH			9600  //Grupo que se lista con LPARTEH.EXE
#define GRPHORACERO		9700  //Grupo graba horas en cero
              

// Campo EFECT de ASIG
#define	EFECTIVO	"E"  //"E": "Efectivo",
#define	PROVISORIO	"P"  //"P": "Provisorio"

//Campo VIGIL de ASIG
#define	VIGILADOR	"V"  //"V":"Vigilador"
#define	RETEN		"R"  //"R":"Reten"
#define	PARTTIME	"P"  //"P":"PartTime"

//Estados del parte - campos confir y confex de la tabla operac.parte:
#define	A_CONF		0   //0: "A Confirmar",
#define	CONF		1   //1: "Confirmado",
#define	CERRADO		2   //2: "Cerrado"
#define	CERRADO_FAC	3   //3: "Cerrado para facturar"

//Condiciones de horas excepcion
#define FACTURABLE		1
#define ACARGO_EMP		2
#define REUBICACION		3
#define IMPRODUCTVIDAD	4

//Motivos de horas excepcion
#define REU_NORMAL	1

//Regimen Especiales
#define REG_ESP       "4x2x12"
#define REG_ESP_1     "1x1x12"
#define REG_ESP_1_SAP "1x1x8"
#define REG_ESP_2     "8x4x12"
#define REG_ESP_3     "4x1x8"
#define REG_16x12x12  "16x12x12"
#define REG_ESP_4     "4x2x12s"

//Frecuencia Regimenes de Part-times
#define SEMANAL		"S"
#define QUINCENAL	"Q"
#define MENSUAL		"M"
#define HSPT		13334 //Total de horas que puede realizar un Part Time en un mes. Dividir por 100 al usarlo.

//Motivos de Desasignacion
#define DESAUT		1
#define ALTAPARTE	80
#define DESXERROR	98
#define DADO_DE_BAJA	2
#define CAMBIO_EMPRESA	81
#define CAMBIO_PUESTO	82

//Dias de la semana
#define LUNES		"L"
#define MARTES		"M"
#define MIERCOLES	"X"
#define JUEVES		"J"
#define VIERNES		"V"
#define SABADO		"S"
#define DOMINGO		"D"
#define DIAPTIME	"P"

//Estados de Aprobación de OT
#define PENDIENTE	0
#define APROBADO	1
#define RECHAZADO	2

#define _AUSENTE		"A"
#define _TRABAJA		"T"	
#define _PEGADA			"P"	
#define _ADELANTO		"D"	
#define _VACACIONES		"V"	
#define _FRANCO			"F"	
#define _NO_TRABAJA		"X"	

#define _AUSENTE_C		'A'
#define _TRABAJA_C		'T'	
#define _PEGADA_C		'P'	
#define _ADELANTO_C		'D'	
#define _VACACIONES_C	'V'	
#define _FRANCO_C		'F'	
#define _NO_TRABAJA_C	'X'	

#define _CON_AVISO		1
#define _SIN_AVISO		2

#define _FALTO		1

#define _SEGUNDOS_POR_DIA	43200
#define _SEGUNDOS_POR_HORA	 1800
#define _HORAS_POR_DIA      24

#define _TURNO_INICIAL		"1"

#define _ROL_ESTATICO		1
#define _ROL_DINAMICO		2

/**** TIPO DE RANGO ****/
#define _CARGA_MANUAL		0
#define _DIA_DESDE_HASTA	1
#define _MES_COMPLETO		2

//Cantidad de hs a trabajar para cobrar vianda.
#define _CANT_HS_VIANDA      10

//Columnas usadas por la matriz puestos de las funciones LeePuestos() y LeeProximo()
#define COLTIPPTO 0    // Columna tipo de puesto
#define COLCODINT 1    // Columna código interno 
#define COLCANPUE 2    // Columna cantidad de puestos 

#define TMSG_WAR  1    // Warning en TituloMsg()
#define TMSG_ERR  2    // Error en TituloMsg()

#define DIAVIGI	"13/12/2008"	// Dia del vigilador

// Defines de calculo de Horas 
#define _HORAS_DE_TRABAJO	800
#define _HORAS_AL_25        200

#define _DIA_FRANCO			1
#define _DIA_FERIADO	 	2
#define _DIA_NORMAL		 	3
#define _DIA_PEGADA		 	4

// Defines para usar en funcion SumaTiempo()
#define _HORAS				1
#define _MINUTOS 			2
#define _SEGUNDOS 			3


DATE 	fechapuesto; // Para Ayuda de puestos
char    g_vigil[2],  // Para popup de puestos según el tipo de vigilador (V, P)
        g_efect[2];  // Para popup de puestos según el tipo de vigilador (V, P) y cond. de efectivo

char gg_prog[20]; 

char	g_titmsg[30];// Para TituloMsg()

/**** Funciones de OPERAC ****/
void AsigCodInOperac(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char *cond, char * codfrec,
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, int codint);
void DisplayPto(char *buffer);
void BorrarParteGenerado(int p_emp, long p_cliente, int p_objetivo, long p_nroleg, DATE p_fechad,
                         DATE p_fechah, DATE p_fecbaj, bool p_porfecha, char *p_efect, int p_ptoser,
                         int p_puesto, int p_nroint);
void VerificoDobleAsig(int p_emp, long p_nroleg);
void GetPtoEfectivo(int emp, long nroleg, int *ptoser, int *puesto);
void GetRegimenEfectivo(int emp, long nroleg, char *regimen, DATE fecha);
void GetFFrancoEfectivo(int emp, long nroleg, DATE *fecfranco, short *nfran, DATE fecha);
void CantHorasTrab(int anio, int pos);
void BorrarInasistencia(int emp, long nroleg, DATE fecha);
void AudiGrabaHorasParte(schema p_operac, char * p_programa);

int GetTipPto(int emp, long cliente, int objetivo, long legajo);
int GetPuesto(int emp, long cliente, int objetivo, long legajo);
int GetCantHoras(TIME hdesde, TIME hhasta);
int GetNumFrancoEfectivo(int emp, long nroleg, DATE fecha);
int GetConvenio(int emp, long nroleg);
int GetNextNroint(int emp, long cliente, int objetivo, long nroleg, int ptoser, int puesto);
int LegActivo (int p_emp, long p_nroleg);
int GetCategoria (int p_emp, long p_nroleg);
int Hours(TIME p_hora);
int Minutes(TIME p_hora);
int Seconds(TIME p_hora);

char * GetRegimen(long cliente, int objetivo, int tippto, int puesto);
char * GetDescCond(int condicion);
char * GetDescMotivo(int condicion, int motivo);
char * DiaLetra(DATE fecha);
int DiaNumero(DATE fecha);
char * TipoVig(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte);
char * TipoDia(long cliente, int objet, int tippto, int codint);
char * GetDescrLugPag(long lpago);
char * GetDescMotivd(int motivd);
char * TituloMsg(int p_tipomsg, int p_codigo);
char * GetCondicRol(DATE fecasi, DATE fecha, int codrol, int fila, int colum);

bool EsPuestoPartime(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte);
bool DesasigTieneHsCargadas(int emp, long cliente, int objetivo, long nroleg, DATE fdesde, DATE fhasta);
bool DiaVigilador(DATE fecha, int pais, int prov, int emp);
bool FeriadoNovia(DATE fecha, int pais, int prov);
bool Franco(int emp, long legajo, DATE fecha, char vigil[2], int numfran);
bool TrabDiaEnPtoEfec(int emp, long cliente, int objetivo, long legajo, DATE fecparte);
bool EsEfectivo(int emp, long cliente, int objetivo, long legajo, int ptoser, int puesto, int nroint, DATE fecparte);
bool Vacaciones(int emp, long nroleg, DATE fecha);
bool TieneLic  (int emp, long nroleg, DATE fecha);
bool Falto     (int emp, long nroleg, DATE fecha);
bool MotDesagRota(int motivo);
bool ValidaPuesto(schema operac, DATE p_fecha);
bool BajaPuesto(long cliente, int objet, int tippto, DATE fecasig);
bool PuestoAsignado(int p_emp, long p_cliente, int p_objet, int p_tippto, int p_codint, DATE p_fecfin);
bool ExisteParteCargado(int p_emp, long p_cliente, int p_objetivo, long p_nroleg, int tippto, int puesto, DATE p_fechad, DATE p_fechah);
bool CalculaFechaFranco(int p_rol, int p_fila, int p_colum, DATE p_fecha, long p_clieot, int p_objet,
                        int p_tippto, int p_codint, DATE *p_calcfec, int *p_lugar);
bool ControlFechaFranco(DATE fecfra, long p_clieot, int p_objet, int p_tippto, int p_codint);
bool HoraEnRangoHorario(TIME p_hora, TIME p_ini_rango, TIME p_fin_rango);
bool RangoHorarioEnRangoHorario(TIME p_horini, TIME p_horfin, TIME p_ini_rango, TIME p_fin_rango);
bool SuperposicionRangoHorario(TIME p_horini, TIME p_horfin, TIME p_ini_rango, TIME p_fin_rango);
bool Superposicion(TIME hent, TIME hsal, TIME hdesde, TIME hhasta, bool total);
bool Super(TIME hent, TIME hsal, TIME hdesde, TIME hhasta);
//  "Superposicion" sirve para:
// (a) - ver si el primer rango de horas esta incluido en el segundo rango (o son = los rangos) (total = true)
// (b) - ver si se superponen los rangos de horas (total = false).
// Devuelve TRUE si alguno de los 7 primeros dias que se pasan por parametro 
bool DiasTrabajados(char *d1, char *d2, char *d3, char *d4, char *d5, char *d6, char *d7,
					char *dia1, char *dia2, char *dia3, char *dia4, char *dia5, char *dia6, char *dia7);
// dado un día y los dia (1-7) de un puesto, esta funcion indica si el día es alguno
// de los dias 1 a 7. Esto se usa para saber si dado un día, ese día se trabaja en el
// puesto. Esto se hace pasandole a la función los 7 dias que se trabajan en el puesto.
bool SeTrabEnPuesto (char *d, char *dia1, char *dia2, char *dia3, char *dia4,
					char *dia5, char *dia6, char *dia7);
// devuelve true si el objetivo fue dado de alta en la policia
// (existe un registro en brigada.contacto)
// devuelve false si no encuentra este registro.
bool EstaEnLaPolicia(int emp, long cliente, long cont);
// devuelve TRUE si la fecha que se pasa como parametro esta dentro del periodo
// generado 
bool MenorAMinFecFranco(DATE fecha, char *vigil, DATE fecfra, char *regimen, DATE *fecmax);
// devuelve TRUE si existe ese puesto (tabla operac.puestos) y tiene algo en el campo horapt
// o en el campo codfrec. Devuelve FALSE si tiene esos campos en nulos o no encontro el puesto
bool PuestoEsPartime(long cliente, int objetivo, int tippto, int codint);
// devuelve TRUE si el Vigilador es Partime en forma Efectiva.
bool VigPartime(int emp, long legajo, DATE fecparte);
bool AlOtroDia(int emp, long legajo, DATE fecha, bool enasig);
bool CorrespondeDiaPuesto(short emp, long cliente, short objetivo, short tippto, short codint, DATE fecha);
bool ExisteCliObjEnGrp(int grupo, long cliente, int objetivo);
bool ExisteEnAsig(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint, bool prov);
bool ExisteEnAsigh(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint, bool prov);
bool FechasSuperpuestas(DATE p_fecini1, DATE p_fecfin1, DATE p_fecini2, DATE p_fecfin2);
bool EstaEnAsig(int p_emp, long  p_cliente, int p_objetivo, long p_nroleg, int  p_ptoser, int  p_puesto, int  p_nroint);
bool GetObjetivoBaja(int p_emp_o, long p_cliente_o, int p_objet_o, char *p_regim, long *p_cliente_b, int * p_objet_b, int * p_tippto_b, int * p_puesto_b);
bool TieneProvisorioVigente(int emp, long legajo, DATE fecha);
bool PuestoVigente(DATE p_dia, long cliente, int objet, int tippto, int codint);
bool EsInsertado(int emp, long cliente, int objetivo, long legajo, DATE fecparte, int ptoser, int puesto, int nroint);
bool BajarHoras(long cliente, int objetivo, int tippto,  int puesto, TIME hinicio, TIME hfinal,
					 char * dia1, char *dia2, char * dia3,  char * dia4,  char * dia5, char * dia6,
					 char * dia7,  char * regimen, char *tipodia, int cantpue, int cantvig, char * cond, char * codfrec, 
					 int horapt, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, int codint);

long CantHorasTrab2(int emp, long cliente, int objet, int mes, int anio, DATE fechad, DATE fechah);
long GetTotHsExc(int emp, long cliente, int objet, DATE fechad, DATE fechah, int condi);
long ObtenerLugPag (long cliente, short objetivo);
long ArmarCCosto(long cliente, int objetivo);
void DesArmarCCosto(long nroccte, long *vcli, short *vobj);

double ConvHraInt(TIME hdesde, TIME hhasta);
double ConvFechaHraInt (DATE fdesde, DATE fhasta, TIME hdesde, TIME hhasta);
double CantidadHorasAsignadas(int p_emp, long p_nroleg, DATE p_dia);

int HorasStd(long cliente, int objetivo, int tippto, DATE fecha);
int BusHora(TIME hora); // dada una hora, devuelve un entero que 
						// representa la cantidad de horas enteras.

// devuelve el cliente/objetivo que esta asignado en forma efectiva
void GetCliObjEfectivo(int emp, long nroleg, DATE fecha, long * cliente, int * objetivo);

/**** Funciones de Interfaz con DENARIUS  ****/
char * GetNombreLeg(int emp, long legajo);

// devuelve la descripción del codigo de frecuencia ingresado. De no existir este
// codigo en la tabla operac.frecuen, devuelve null_str
char *DescrFrecuencia(char *codfrec);

char CondicParteOtroObjetivo(int emp, long cliente, int objet, DATE fecha, long nroleg);

TIME HrEntTrabEfec(int emp, long nroleg, DATE fecha);
TIME HrSalTrabEfec(int emp, long nroleg, DATE fecha);
TIME HoraEntParte(int emp, long cliente, int objet, DATE fecha, long nroleg, int ptoser, int puesto, int nroint);
TIME HoraSalParte(int emp, long cliente, int objet, DATE fecha, long nroleg, int ptoser, int puesto, int nroint);
TIME SumaTiempo(TIME p_hora, double p_canti, int p_unidad);


DATE FecIng(int emp, long nroleg);
DATE FechaFinPuesto(long cliente, int objet, int tippto, int codint);
DATE FechaInicioPuesto(long cliente, int objet, int tippto, int codint);
DATE FechaFinalOt (long cliente, short objetivo, DATE finicio, DATE ffinal, TIME hiniot, TIME hfinot, bool baja);
DATE FechaInicioUltAsigEfec(int emp, long nroleg);
DATE GetEgresoLegajo(int p_emp, long p_nroleg);

#define MAX_ASIG 10000

struct Asig {         
	int emp;
	long cliente;
	int objetivo;
	int ptoser;
	int puesto;
	int nroint;
	long nroleg;
   	char vigil[2];
	char efect[2];
	DATE fecasig;
	TIME hsent;
	TIME hssal;
	char dia1[2];
	char dia2[2];      
	char dia3[2];
	char dia4[2];
	char dia5[2];
	char dia6[2];
	char dia7[2];
	long reempl;
	DATE ffranco;
	int  numfran;
	int  francero;
	char regim[8];
	char regpto[8];
	DATE fechas;
	DATE fecbaj;
	short rrol, rfila, rcol;
};

// Apertura de horas trabajadas.
void CalcularDetalle(int emp, long cliente, int objetivo, long nroleg, char * tipvig, DATE dia, int pais,
					 int prov, int hstot, TIME hssal, TIME hsentre, int ptoser, int puesto, int nroint,
					 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran,
					 bool vieneparte, int hsigualcli);
void CalcularDetalleProv(int emp, long cliente, int objetivo, long nroleg, DATE dia, char * tipvig, int pais,
						 int prov, int hstot, TIME hssal, TIME hsentre, int ptoser, int puesto, int nroint,
						 char * regimen, int * hsnor, int * hs50, int * hs100f, int * hs100fe, int numfran,
						 bool vieneparte);
int HsOtroCliente(int emp, long nroleg, DATE fecparte, long cliente, int obj, int ptoser, int puesto,
				  int nroint, int *hs, bool vieneparte);

void CargarCrossCCto (short emp, long cliente, short objetivo, char *descrip, short pais, short prov, 
					 long local, FILE *farch, bool borra);


void CalculoDetalleHorasUru(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,  
                            DATE p_dia , long p_nroleg, int p_ptoser, int p_puesto, int p_nroint, 
                            int *sim, int *dob, int *dym, int *dymfe );

void DistibuyeHorasUru(int p_tothoras, int p_tipodia, int *sim, int *dob, int *dym, int *dymfe);



//Devuelve la descripción del Rol.
char * GetRolDesc(int rol);
char * ValorRol(int rol, int fila, int colum);

TIME HoraInicioTurno(TIME hinicio, char *regim, char *rvalor);
TIME HoraFinTurno(long cliente, short objet, short ptoser, short puesto, TIME hinicio, 
					char *regim, char *rvalor);
void GetHorasTurno (int p_emp, long p_nroleg, long cliente, short objet, DATE fecasig, short rol, 
                    short fila, short colum, char *regim, TIME hinicio, TIME hfinal,DATE fecha,
                    short ptoser, short puesto,	char *dia1, char *dia2, char *dia3, char *dia4,
                    char *dia5, char *dia6, char *dia7, bool efect,	char *valor, char *p_vigil,int  p_numfran,
                    TIME *hdesde, TIME *hhasta);
void GetCliObjEfectivo(int emp, long nroleg, DATE fecha, long * cliente, int * objetivo);
long UbicacionPorLocalidad (short pais, short prov, long local);
DATE GetFechaCierreParcial(int emp);
DATE GetFechaCierreOpe(int emp);
void PutFechaCierreOpe(int emp, DATE fecha);
short HorasNormalesPorSemana(short emp, long nroleg, short pais, short prov, short nrosem, DATE fechad, DATE fechah);
void CalcularDetallePartime(long cliente, int objetivo, DATE dia, TIME hsentre, TIME thssal, int * hsnor, int * hs100fe);

void LeePuestos(long clie, int obj, int puestos[500][3], DATE p_fecha, bool verpadre);// Lee los puestos mimp si existes y llena matriz
bool LeeProximo(long clie, int obj, int pto, int cod, int *p_i, int puestos[500][3],
                schema operac, DATE p_fecha, int *cantpue, bool verpadre);

fm_status HelpPto(form fm, fmfield fno, int row, long p_cliente, int p_objetivo, int p_tippto, 
                  DATE p_fecha, char * p_vigil, char *p_efect);
int ValidaPto();

fm_status HelpCodInt(form fm, fmfield fno, int row, long p_cliente, int p_objetivo, DATE p_fecha);
int ValidaCodInt();
void DisplayCodInt(char *buffer);

bool ValidaXLegajo(int p_emp, long p_nroleg, DATE *p_fecha);
bool ValidaMalRangoFecha(int p_emp, long p_nroleg, DATE *p_fecha_ini);
void RecalculaParte(int p_emp, long p_nroleg, DATE p_fechad, DATE p_fechah, form fm, fmfield p_coment);
void RecalculaPartePer(int p_emp, long p_nroleg, DATE p_fechad, DATE p_fechah, form p_fm, fmfield p_coment, char * p_prog);

void CalDetHorPer2(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,  DATE p_dia , long p_nroleg, 
                            int p_ptoser, int p_puesto, int p_nroint, char p_condic, TIME p_entrada, TIME p_salida, int *nor, int *a25, int *a35, int *fra);
void DisHorPer2(int p_emp, int p_tothoras, int p_tipodia, int p_horas_regimen, int *nor, int *a25, int *a35, int *fra);
bool CalHorMisCli2(form p_fm, int p_row, double *p_hs_nor,double *p_hs_50, double *p_hs_100fr, double *p_hs_100fe);

void CalculoDetalleHorasPer(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,  DATE p_dia , long p_nroleg, 
                            int p_ptoser, int p_puesto, int p_nroint, int *nor, int *a25, int *a35, int *fra, int *peg, int *gua);
void DistibuyeHorasPer(int p_emp, int p_tothoras, int p_tipodia, int p_horas_regimen, int *nor, int *a25, int *a35, int *fra, int *peg, int *gua);

double TotalHsDia(int p_emp, long p_nroleg, DATE p_dia);



/*                Listas                       */

typedef struct stnDFld * tnDFld;
typedef struct stnDFld {
	DATE	fecha;
	tnDFld	nsig;
} stnDFld;

tnDFld AcuDFld(tnDFld, tnDFld *, DATE);
bool ExisteDFld(tnDFld nodop, DATE p_fecha);
void BorDFld(tnDFld);
tnDFld iniDFld;

char g_tcalculo[10] ;

