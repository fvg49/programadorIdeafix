
#define _MAXFILA_EXCEP			101
#define _MAXCOL_EXCEP			101
#define _TIPO_EXCEP_NORM	 	1
#define _TIPO_EXCEP_DESC	 	2
short _lib_tipo_expcep[_MAXFILA_EXCEP][_MAXCOL_EXCEP];
short _lib_var_expcep[_MAXFILA_EXCEP][_MAXCOL_EXCEP];

short InicioListaTipoExcepcion();
short ParteTipoExcepcion(short tipo, short motivo);
short ParteVarExcepcion(short tipo, short motivo);
