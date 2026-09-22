/********************************************
* MODULE & VERSION : @(#)webinter.h	1.17
* DATE             : 06/09/04
* TIME             : 11:49:50
**********************************************/
#ifndef _WEBINTER_H
#define _WEBINTER_H

#define _WEB_URL 			"http://10.1.1.30:8080/axis/services/arSQLService"          //axis 
#define _WEB_CRM 			"http://10.1.1.30:8080/axiscrmv/services/arSQLService"      //axiscrmv 
#define _WEB_URL_GESTION 	"http://10.1.1.30:8080/axisge/services/arSQLService"       	//axisge 
#define _WEB_URL_GESNOV 	"http://10.1.1.30:8080/axisgn/services/arSQLService"        //axisgn 
#define _WEB_URL_INSPECCION "http://10.1.1.30:8080/axisinsp/services/arSQLService"  	//axisinsp
#define _WEB_URL_COMPRAS	"http://10.1.1.30:8080/axiscompras/services/arSQLService"   //axiscompras

//#define _WEB_URL "http://10.1.5.2:8080/axisn/services/arSQLService"                //axis de SRVDEVELOP

//Los parametros de alarmas del tomcat estan en el menu intcrm000/parametros generales, 
//codigo de parametro 4.-

#define _USE_NOVIA "novia"
#define _ARCH_LOG_WEB_ERROR "/export/home/webinter/pas_web_err.log"
#define _EXTENCION_ARCH_IMPORTAR_WEB ".dat"
#define _INICIO_PROC	"INICIO PROCESO %.3D %.3T\n"
#define _FIN_PROC		"FIN    PROCESO %.3D %.3T\n"

/* ----- Parametros Generales ----- */
#define _PAR_WEBINTER_ID_PASE		1
#define _PAR_WEBINTER_SLEEP			2
#define _PAR_WEBINTER_DIR_LOG		3
#define _PAR_WEBINTER_DIR_ERROR		4
#define _PAR_WEBINTER_FILE_ERROR	5
#define _PAR_WEBINTER_DIR_IMPORTAR	6

/* ----- Estado del Log WPASELOG ----- */
#define _WPASELOG_ESTADO_PENDINTE	0
#define _WPASELOG_ESTADO_EJECUCION	1
#define _WPASELOG_ESTADO_FIN_OK		2
#define _WPASELOG_ESTADO_WARNING	3
#define _WPASELOG_ESTADO_ERROR		4

/* ----- Reglas de Negocios ----- */

#define _REG_NEG_PASEDEN	"JsOPE_Paseden"
#define _REG_NEG_PASECLI	"JsCOM_Pasecli"

/* ----- Estado del Log WPASELOG ----- */

#define _WMSGLOG_TIPO_MENSAJE	0
#define _WMSGLOG_TIPO_WARNING	1
#define _WMSGLOG_TIPO_ERROR		2
#define _WMSGLOG_TIPO_SISTEMAS	3

/* ----- Largo de Variables Char en webinter.sc----- */

#define	L_WPARAM_VALOR		50

#define	L_WPASELOG_CODPROG	30
#define	L_WPASELOG_USUARIO	30

#define	L_WPARLOG_CODPROG	30
#define	L_WPARLOG_PARAM		15
#define	L_WPARLOG_VALOR		70

#define	L_WMSGLOG_CODPROG	30
#define	L_WMSGLOG_MENSAJE	70

#define	L_TMPVARNOV_CODPROG	30
#define	L_TMPVARFIJ_CODPROG	30

#define	_TAMANIO_QUERY_SQL_SERVERS		5000

/* ----- Agrupamientos de C. Costos que se pasan a Web  -----
   ----- para mostrar solo los empleados de Operaciones ----- */

#define AGRPCC_VIGILACIA_DIRECTO	1
#define AGRPCC_OPERACIONES			1102
#define AGRPCC_INSPECCION_GARITA	1103

/* ----- Parametro para indicar con que empresa se -----
   -----    esta procesando la interfase a Web     ----- */
#define _EMP_PROSEGUR	"P" 

/* ----- Licencias que no son pasadas a Web ----- */
#define LICE_VACACIONES	9999


/* ----- Funciones de Librerias ----- */

FILE *AbrirArchivo(char *archivo, FILE *_arch_error);
FILE *AbrirArchivoERROR();
FILE *AbrirArchivoImportar(char *_programa_exe, char *nomarch, char *nomarch_comp, FILE *_arch_error);
void  FinLog();

char *GetParametro( int parametro);
int   GetIParametro( int parametro);
long  GetLParametro( int parametro);
DATE  GetDParametro( int parametro);
long  GetProxIdPase();
char *GetDirLog();
char *GetDirLogError();
char *DateToChar(DATE fecha);
char *GetDirArchImportar();

void GrabaMsg(char *codprog, char *oisesion, int tipomsg, char *msg);
bool GrabaPARLOG( char *codprog, char *oisesion, char *param, char *valor, FILE *_arch_log);
bool GrabaWPASELOG(char *codprog, char *oisesion, int estado, FILE *_arch_error);

FILE *InicioLog(char *_programa, FILE *_arch_error);

void PasajeASqlServer(dbtable tab, char *use, char *prefijo, char *query);

//Funciones de webpases.c
void ActualizarLegajoWeb(short emp, long nroleg, char *programa, FILE *fper);

#endif /* _WEBINTER_H */
