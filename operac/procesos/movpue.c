#include <ideafix.h>
#include "operac.sch"
#include "comgral.sch"
#include "operac.h"
#include "billpro.h"

#define EMP_DESTINO 8

dbcursor c_asig;


schema ope, cgr;
wcmd(movpue, 1.12  23/09/98)
{

	ope  = OpenSchema("operac",  IO_EABORT);
	cgr  = OpenSchema("comgral",  IO_EABORT);


	WiMsg("Este proceso actualiza los puestos y por ser peligroso se puso este comentario\nlo que hace es borrar todos los puestos de la empresa destino (esta en un define)\ny graba los puestos que este usando la asignacion de dicha empresa\ntomandolos de la empresa origen");
	return;

	fprintf(stderr, "ACCION\tcliente\tobjetivo\ttippto\tpuesto\n");

	BeginTransaction();

	//Borro Puestos Destino
	SetKey(ope|PUESTOSbyCLIENTE, 3000 , NULL_SHORT, NULL_SHORT, NULL_SHORT);
	while(GetRecord(ope|PUESTOSbyCLIENTE, NEXT_KEY, IO_NOT_LOCK)!=ERROR) {

		SetKey(cgr|MOVOBJbyDESTINO, EMP_DESTINO, LFld(ope|PUESTOS_CLIENTE), IFld(ope|PUESTOS_OBJET) );
		if(GetRecord(cgr|MOVOBJbyDESTINO, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
	        fprintf(stderr, "BORRA\t");
	        fprintf(stderr, "%ld\t", LFld(ope|PUESTOS_CLIENTE));
	        fprintf(stderr, "%d\t",  IFld(ope|PUESTOS_OBJET));
	        fprintf(stderr, "%d\t",  IFld(ope|PUESTOS_TIPPTO));
	        fprintf(stderr, "%d\n",  IFld(ope|PUESTOS_CODINT));
	        //DelRecord(ope|PUESTOS);
		}		
	} 

	c_asig	= CreateCursor(ope|ASIGbyEMP,  IO_NOT_LOCK);
	SetCursorFrom(c_asig, EMP_DESTINO, 3000, NULL_SHORT, NULL_LONG, NULL_SHORT, NULL_SHORT, NULL_SHORT);
	SetCursorTo  (c_asig, EMP_DESTINO, MAX_LONG,  MAX_SHORT,  MAX_LONG,  MAX_SHORT,  MAX_SHORT,  MAX_SHORT);
	while (FetchCursor(c_asig) != ERROR) {

		// Busco Puestos Origen
		SetKey(cgr|MOVOBJbyDESTINO, EMP_DESTINO, LFld(ope|ASIG_CLIENTE), IFld(ope|ASIG_OBJETIVO));
		if(GetRecord(cgr|MOVOBJbyDESTINO, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
			SetKey(ope|PUESTOSbyCLIENTE, LFld(cgr|MOVOBJ_CLIENTEO), IFld(cgr|MOVOBJ_OBJETIVO), IFld(ope|ASIG_PTOSER), IFld(ope|ASIG_PUESTO));
			if(GetRecord(ope|PUESTOSbyCLIENTE, THIS_KEY, IO_NOT_LOCK)!=ERROR) {
				

				//Grabo Puesto Destino
				SetLFld(ope|PUESTOS_CLIENTE, LFld(ope|ASIG_CLIENTE));
				SetIFld(ope|PUESTOS_OBJET,   IFld(ope|ASIG_OBJETIVO));
				SetDFld(ope|PUESTOS_FINICIO, StrToD("01062009"));
				SetDFld(ope|PUESTOS_FFINAL,  NULL_DATE);
		        
		        fprintf(stderr, "GRABA\t");
		        fprintf(stderr, "%ld\t", LFld(ope|PUESTOS_CLIENTE));
		        fprintf(stderr, "%d\t",  IFld(ope|PUESTOS_OBJET));
		        fprintf(stderr, "%d\t",  IFld(ope|PUESTOS_TIPPTO));
		        fprintf(stderr, "%d\n",  IFld(ope|PUESTOS_CODINT));
		        //PutRecord(ope|PUESTOS);

			}

		} 

	} 
 	DeleteCursor(c_asig);

	EndTransaction();

}
