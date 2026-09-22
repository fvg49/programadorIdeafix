#include "../abms/parteb.fmh"       //fm0 (parte.fm)
#include "../abms/parteb1.fmh"     //fm1 (hsextra.fm)	Detalle de Horas


void CalculaDetalleHorasPro(bool p_usafm, form p_fm, int  p_row, int p_emp, long p_cliente, int p_objetivo,
                            long p_nroleg, DATE p_fecha, TIME p_hora_ent, TIME p_hora_sal, int p_pais,
                            int p_prov, char *p_tipvig,  int  *p_hs_agrego_nor, int *p_hs_agrego_50,
                            int *p_hs_agrego_100fr, int *p_hs_agrego_100fe);

void DistribuyeHoras(int p_emp, long p_nroleg, DATE p_fecparte, char *p_tipvig, int p_pais, int p_prov,
                     TIME p_hora_ent, TIME p_hora_sal, double p_hs_nor,double  *p_hs_agrego_nor, 
                     double *p_hs_agrego_50, double *p_hs_agrego_100fr, double *p_hs_agrego_100fe);

void ModifDiaVigilador(int p_emp, DATE p_fecha, bool p_un_dia, double p_hs_hoy, double p_hs_man, 
                       double p_tope_hs_normales, double* p_hs_agrego_nor,
                       double* p_hs_agrego_50, double* p_hs_agrego_100fr);

void ModifDistribNormal(bool p_es_partime, double p_hs_faltan_agregar, double p_tope_hs_normales, 
                        double* p_hs_agrego_nor, double* p_hs_agrego_50);

bool CalcOtrasHoras(int p_emp, long p_nroleg, DATE p_fecparte, long p_cliente, int p_obj, long p_cliente2,
                    int p_obj2, int p_ptoser2, int p_puesto2, int p_nroint, double *p_hs_nor, 
                    double *p_hs_50, double *p_hs_100fr, double *p_hs_100fe);

bool CalcHorasMismoCliente(form p_fm, int p_row, double *p_hs_nor,double *p_hs_50, double *p_hs_100fr,
                           double *p_hs_100fe);

bool AbarcaUnDia(TIME p_hora_ent, TIME p_hora_sal, double *p_horas_hoy, double *p_horas_man);


