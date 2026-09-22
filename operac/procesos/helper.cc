/***********************************************************************
*
* MODULO & VERSION : $RCSfile: helper.cc,v $ $Revision: 1.10 $
* FECHA y HORA     : %D% 
* AUTOR            : Ruben Demicheli.
* MODIFICO         : 
* DENOMINACION     : 
*			         
*------------------+---------------------------------------------------
* DESCRIPCION      : Funciones para manejo de menues.
*
***********************************************************************/
#include <ideafix.h>
#include "sue.sch"
#include "sue.h"

#define SPECIAL_CHARS	"!@#$%^&*()+=-[]{}'\\<>?,.\"|}"
#define FMT_POP			"%9.9ld %-40.40s"
#define LARG_POP 10
#define ANCH_POP 55

static void SetStrDesdeHasta(char *desde, char *hasta, char *rexp);
static char *Cargo(char* h_buff,int n);
static void MuestraEst(char *buff);
static int ValidaEst(void);

/* Declaraciones globales */
dbcursor curper=ERROR;
static char h_buff[128];  /* Para el texto del PopUp */
static long actual = 0;
static bool valido = TRUE;
char rexp[51];
char _omask[30] = NULL_STR;
dbcursor _c_estr = ERROR;

/*
<0 * 
<1 * Funcion      : MenuNOM
<2 *****************************
<3 Prototipo      : extern "C" long MenuNOM(int)
<4 Parametros     : int emp
<5 Valor Retornado: long 
<6 Descripcion    : Desplegar un PopUp con las opciones seleccionadas de
   	   	            la tabla PER Ordenada Alfabeticamente en base a una
		            expresion regular
*/

extern "C" long MenuNOM(int emp)
{
	schema viejo, sue;
	int renglon = 0;
	window w;
	char desde[51], hasta[51];
	Message msgBuscExp;
	msgBuscExp = DWORD("TITUL","BUSCEXP");
	w=WiCreate(WiDefPar(), WiLine(WiCurrent()), WiCol(WiCurrent()),
				3, 52, STAND_BORDER, MsgToStr(msgBuscExp), A_NORMAL);
	*rexp='\0';
	for (;;) {
		WiMoveTo(1, 1);
		if (SGetMaskCampo(rexp, 50, 50, "50>x") == K_END) {
			WiDelete(w);
			return ERROR;
		}
		break;
	}
	WiDelete(w);

	viejo = CurrentSchema();

	if ((sue = FindSchema("sue")) == ERROR)
	{
		sue = OpenSchema("sue", IO_EABORT);
	}

	SwitchToSchema(sue);
	
	curper=CreateCursor(PERbyNOM, IO_NOT_LOCK);

	SetStrDesdeHasta(desde, hasta, rexp);
	SetCursorFrom(curper, emp, *rexp ? desde : LOW_VALUE, NULL_LONG);
	SetCursorTo(curper, emp, *rexp ? hasta : HIGH_VALUE, MAX_LONG);

	actual = 0;
	valido = TRUE;
	Message msgLeg;
	msgLeg = DVARI("VARI","LEGAJO")(rexp);
	
	MoveCursorFirst(curper);
	if ((renglon=PopUpMenu(10, 52, MsgToStr(msgLeg), Cargo, h_buff, NULLFP, NULL,
	    		POP_STATIC|POP_BORDER))  !=  ERROR)
	{
 		(void) Cargo(h_buff, renglon + 1);

		DeleteCursor(curper);
		
	 	SwitchToSchema(viejo);
	 	return StrToL(h_buff);	
	}

	DeleteCursor(curper);

	SwitchToSchema(viejo);
	return ERROR;	
}

/*
<0 * 
<1 * Funcion      : Cargo
<2 *****************************
<3 Prototipo      : char *Cargo(char* , int)
<4 Parametros     : char* h_buff,
					int n.
<5 Valor Retornado: char *
<6 Descripcion    : Setea h_buffe con el nro de legajo, el apellido y nombre  
					del registro que se encuentra en la fila n
*/

static char *Cargo(char* h_buffe,int n)
{
	if (n < 1 || (n > actual && !valido)) return NULL;
  	if (n > actual) {
		while (n > actual && (valido=(FetchCursor(curper) != ERROR))) {
			/* Salteo los legajos a los cuales no tengo acceso */
			if ( !Acceso(IFld(PER_EMP), LFld(PER_NROLEG), NULL_SHORT, NULL_LONG))
				continue;
			if ((valido=RexpMatch((UChar *)rexp,
								(UChar *)SFld(PER_APYNOM),
								REXP_NOCASE|REXP_EMBEDDED))) {
				actual++;
			}
		}
		if (n != actual && !valido)
			actual++;
	}
	else {
		if (n < actual) {
			while (n < actual &&
					(valido=(FetchCursorPrev(curper) != ERROR))) {
				if ( !Acceso(IFld(PER_EMP), LFld(PER_NROLEG), NULL_SHORT, NULL_LONG) )
					continue;
				if ((valido=RexpMatch((UChar *)rexp,
									(UChar *)SFld(PER_APYNOM),
									REXP_NOCASE|REXP_EMBEDDED))) {
					actual--;
				}
			}
			if (n != actual && !valido)
				actual--;
		}
	}
	if (n != actual || !valido) return NULL;
	
	sprintf(h_buffe, FMT_POP, LFld(PER_NROLEG), SFld(PER_APYNOM));

	return h_buffe;
}

/*
<0 * 
<1 * Funcion      : SetStrDesdeHasta
<2 *****************************
<3 Prototipo      : static void SetStrDesdeHasta(char *, char *, char *)
<4 Parametros     : char *desde, 
					char *hasta, 
					char *rexpr.
<5 Valor Retornado: void 
<6 Descripcion    : Setea en los parametro desde y hasta la rexpr pasada como
					parámetro.	
*/

static void SetStrDesdeHasta(char *desde, char *hasta, char *rexpr)
{
	UChar c;
	while ((c=*rexpr++)) {
		if (strchr(SPECIAL_CHARS, c) != NULL) 
			break;
		else {
			*desde++=c;
			*hasta++=c;
		}
	}
	*hasta++='\255';
	*hasta=*desde='\0';
}

/*
<0 * 
<1 * Funcion      : HelpESTR
<2 *****************************
<3 Prototipo      : extern "C" fm_status HelpESTR(int, char *, IFP, form, fmfield, fmfield, int)
<4 Parametros     : 
   int emp        : código de empresa
   char * mask    : máscara de la estructura funcional
   IFP vfunc      : función de Validación
   form fm        : form
   fmfield f1     : campo1
   fmfield d1     : descripción1
   row            : columna

<5 Valor Retornado: fm_status 
<6 Descripcion    : Desplegar un PopUp con las opciones seleccionables de
   	   	            la tabla ESTR (Nodos que pueden imprimirse).
*/

//extern "C++" fm_status HelpESTR(int emp, char *mask, IFP vfunc, form fm, fmfield f1, fmfield d1, int row)
fm_status HelpESTR(int emp, char *mask, IFP vfunc, form fm, fmfield f1, fmfield d1, int row)
{    
	schema viejo, sue;
	int renglon;

	viejo = CurrentSchema();

	if ((sue = FindSchema("sue")) == ERROR)
	{
		sue = OpenSchema("sue", IO_EABORT);
	}

	SwitchToSchema(sue);

	_c_estr = CreateCursor(ESTRbyEMP, IO_NOT_LOCK);

	CompileMask (mask, NULL, _omask);

	SetCursorFrom(_c_estr, emp, LOW_VALUE);
	SetCursorTo(_c_estr, emp, HIGH_VALUE);
	
	Message msgBusq = DGRAL("PROCE", "BUSCA"),
			msgConc = DWORD("GRAL", "ESTRU");
	renglon = PopUpDbMenu(LARG_POP, ANCH_POP, MsgToStr(msgBusq(msgConc,"")), 
						_c_estr, 0, vfunc == NULLFP ? ValidaEst : vfunc,
						MuestraEst);	
    
    DeleteCursor(_c_estr);
	
	if (renglon == ERROR)
	{
		SwitchToSchema(viejo);
		return FM_REDO;
	}

	if (f1 != NULL_SHORT)
	{
		FmSetFld(fm, f1, SFld(ESTR_COD), row);
		FmShowFlds(fm, f1, f1, row);
	}

	if (d1 != NULL_SHORT)
	{
		FmSetFld(fm, d1, SFld(ESTR_DENOM), row);
		FmShowFlds(fm, d1, d1, row);
	}

	SwitchToSchema(viejo);

	return FM_OK;
}


static void MuestraEst(char *buff)
{
	char buffer[30] = NULL_STR;
	
	StrMask(buffer, _omask, SFld(ESTR_COD));
	
	sprintf(buff, "%-24.24s  %-30.30s", buffer, SFld(ESTR_DENOM));
}

static int ValidaEst(void)
{
	return (IFld(ESTR_IMPRIME)) ? true : false;
}

