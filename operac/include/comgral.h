/********************************************
* MODULE & VERSION : @(#)comgral.h	1.3
* DATE             : 05/04/12
* TIME             : 10:44:29
**********************************************/

//#define TIP_CALCULO_NADA		0
#define TIP_CALCULO_MESES		(1 << 0)
#define TIP_CALCULO_ANIV_CLI	(1 << 1)
#define TIP_CALCULO_ANIV_INST	(1 << 2)
#define TIP_CALCULO_IPC			(1 << 3)
#define TIP_CALCULO_CPTO		(1 << 4)


#define PARNOV_EMPRESA_GENERAL      0   // Para parametros que no depende de una empresa
#define PARNOV_REGI_CANTVIGI 8      //Regimenes que se llevan 0.01 en cantida de vigiladores  (Lo usa en pasaje de ot) 

// Codigos Grupo de Parametros (tabla parnov)
#define PARNOV_AUNOPIAS	1	//Ausente no pierde asiduidad  
#define PARNOV_LINOPIAS	2	//Licencia no pierde asiduidad  
#define PARNOV_CAT_1	3	//Que pertenecen al convenio 1
#define PARNOV_CIEOPERA	4	//Cierres de Operaciones Uruguay  
#define PARNOV_LEGFORZA	5	//Legajos Forzados para los Helpers  
#define PARNOV_DESC_EMP 6   //Descripción de la EMPRESA 30 para la descripción de las OTs.
#define PARNOV_REGI_EXT 7   //Regimenes que se cubren con extras
#define PARNOV_TCALC_HS 8   //Tipo de Calculo de horas puede ser A o B
#define PARNOV_RANG_CLI 9   //Rango de Clientes Externos por Empresa
#define PARNOV_CODAUXGR 10  //Codigo de Ausentismo por Grupo
#define PARNOV_TPUHSNOR 11  //Tipos Puestos todo hs.normales
#define PARNOV_CADIARPA 12  //Cantidad de dias para arreglo del parte 
#define PARNOV_TOPE_CCOS 13 //Rango de centro de costo que determina que no es vigilador un legajo 
#define PARNOV_V_HORAS  14  //Permite cargar condicion v y rango horario  
#define PARNOV_PROC_BORRAR_LOG      17  // Lista de procesos a los que se les borra el log de mas de N dias (definido en PARNOV_CANDIAS_BORRAR_LOG)
#define PARNOV_CANDIAS_BORRAR_LOG   18  // Cantidad de dias que se dejan para los que se les borra el log
#define PARNOV_DELEGxEMP 19      // Delegaciones por Empresa
#define PARNOV_SERVIDOR_NOVIA_REAL  20  // Servidor Essentia Novia Real   
#define PARNOV_IMPARCH_ARCHIVO      21  // imparch.exe: Formato del nombre del archivo que es origen de los datos
#define PARNOV_IMPARCH_TABLA        22  // imparch.exe: Esquema y Tabla donde se grabara el archivo del parametro PARNOV_IMPARCH_ARCHIVO
#define PARNOV_IMPARCH_PROCESO      23  // imparch.exe: Procesos incorporador de datos que lee la tabla del paraametro PARNOV_IMPARCH_TABLA
#define PARNOV_PROCESOS_VERIF       50  // Procesos para verificar si se colgaron (y matarlos o enviar mail)
#define PARNOV_PAR_PROCESOS_VERIF   51  // Parametros de procesos para verificar si se colgaron (1-tiempo en minutos 2-se envia mail? 3-Se Mata?)
#define PARNOV_MAILS_WTARIF         53  // Lista de mails para wtarif (interface de tarifas Marte - Billing)
#define PARNOV_GRABAN_PARTE         54  // Lista Procesos que graban parte
#define PARNOV_FTPADJ_MAILS			57  // Datos FTP adjunto Mail
#define PARNOV_ITM_FAC_PC			60	// Items de Facturación para Puestos Compuestos
#define PARNOV_FILIAL_POR_DEFECTO   61  // Filial por defecto cuando se crea un objetivo
#define PARNOV_GENXML_NOPISALOG     62  // Genxml no pisa log (un log por corrida)
#define PARNOV_KILLZOMB             92  // Procesos zombies a matar (killzomb.exe)

#define LINEA_UNICA      1   // Para cuando es un solo parametro y no una lista

#define FTPADJ_MAILS_IP             1   // Datos FTP adjunto Mail IP      
#define FTPADJ_MAILS_USU            2   // Datos FTP adjunto Mail USUARIO 
#define FTPADJ_MAILS_PWD            3   // Datos FTP adjunto Mail PASSWORD
#define FTPADJ_MAILS_PATH           4   // Datos FTP adjunto Mail PATH    
#define FTPADJ_MAILS_PATHR          5   // Datos FTP adjunto Mail PATH RED 


// Orden de la lista de parametros
#define _ORDEN_PARNOV_NUM_ASC  1
#define _ORDEN_PARNOV_NUM_DESC 2
#define _ORDEN_PARNOV_STR_ASC  3
#define _ORDEN_PARNOV_STR_DESC 4
#define _ORDEN_PARNOV_FEC_ASC  5
#define _ORDEN_PARNOV_FEC_DESC 6
#define _ORDEN_PARNOV_HOR_ASC  7
#define _ORDEN_PARNOV_HOR_DESC 8
#define _ORDEN_PARNOV_RANG_CLI 9

#define _ORDEN_PARNOV_RANG_CLI_DES 1
#define _ORDEN_PARNOV_RANG_CLI_HAS 2

/* ------------------------------------ Lista de Parametros de Novia----------------------------------------------- */
typedef struct stnparame * tnparame;
typedef struct stnparame {
	char	parame[50];
	int		nroren;
	DATE	fecvig;
	tnparame	nsig;
} stnparame;

/* Funciones Privadas */
void LisNParame(tnparame);
void LisNParame2(tnparame);
void BorNParame(tnparame);
tnparame AcuNParame(tnparame, tnparame*, int p_orden);


int		g_nroren;
char	g_parame[50];
DATE	g_fecvig;

/* --------------------------------- Fin Lista de Parametros de Novia --------------------------------------------- */

schema GetSchemaDescriptor(char *esquema);

double IndiceAurus(int tipind, DATE fecha);
double DiffIndiceAurus(int tipind, DATE fecha_ant, DATE fecha_pos);
double CalcIPC(long cliente, double valor_ori, DATE fecha);
double CalcIndiceObjetivo(long cliente, int objetivo, int tipind, DATE fecha, double valor_ori,
						int tipo_calculo);
double CalcIndiceConcepto(long cliente, int objetivo, int concepto, DATE fecha, double valor_ori,
						double *val, long *porc);

double TotalSalario(int conv, int categ);

bool FechaEsAniversario(long cliente, int objetivo, DATE fecha, int tipo);

DATE PrimerFechaOTCliente(int emp, long cliente);

long GetNextINDOBJ(long cliente, int objetivo, DATE fecha);

void OptimizaComercialGral();

short ModeloPorNegociacion(short emp, long cliente, short objetivo, short nego, short modelo);

bool GetHorasViaje(int emp, long cliente, int objetivo, DATE fecha, int * hsviaje);

char * GetParNov(int p_emp, int p_cod, int p_nroren, DATE p_fecvig);

bool EsParNov(int p_emp, int p_cod, DATE p_fecvig , char *p_valor);
bool EsParNov2(tnparame p_inicio, int p_emp, DATE p_fecvig , char *p_valor);
int EsParNov3(int p_emp, int p_cod, DATE p_fecvig , char *p_valor);


tnparame LisParNov(int p_emp, int p_cod, DATE p_fecvig, int p_orden);
tnparame LisParNovNF(int p_emp, int p_cod, int p_orden);
bool ParameEsMenor(char *p_nodo, char *p_global, int p_orden);

