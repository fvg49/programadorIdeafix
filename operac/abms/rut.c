/********************************************************************
*
* MODULE & VERSION : @(#)rrol.c	1.1 
* DATE             : 01/11/26 
* TIME             : 13:09:52 
*
* CREATED          : 01/08/01
*
* DESCRIPTION:
*             Carga de Roles.
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "rut.fmh"

#define IsNumeric(ch) 	ch >= '0' && ch <= '9' ? 1 : 0 
#define IsCaracter(ch) 	ch >= 'A' && ch <= 'Z' ? 1 : 0 
#define RUT_SIZE		9
#define MODULO11		11

bool CuitOk(char *cuit_orig, char *error);
void SacaGuionCuit(char *cuit, int longitud);

/* Declaraciones globales */
form fm0;
int i, j;
char *msg[100], digicarac[0]; 

/* Programa principal */
wcmd(rut, 1.1 11/26/01)
{
	fm_cmd cmd;

	fm0 = OpenForm("rut", FM_EABORT);

	while ((cmd = DoForm(fm0, NULLFP, NULLFP)) != FM_EXIT)
	switch (cmd) {
	case FM_ADD :
	case FM_UPDATE:
		if (!CuitOk(FmSFld(fm0, RUT),  msg)) {
			Warning (msg);
		}
		break;
	case FM_DELETE:
		break;
	case FM_IGNORE:
		break;
	}
}

bool CuitOk(char *cuit_orig, char *error)
{
	int x = 0, y, a, resto, dig, longitud;
	char *p, cuit[30], digito[2], digfm[2];

	strcpy(cuit, cuit_orig);

	if (strcmp(cuit, "") == 0) {
		sprintf(error, "El número de Rut es nulo.");
		return FALSE;
	}
    longitud = strlen(cuit);

	SacaGuionCuit(cuit, longitud);

	// Transformo los caracteres de los números a números.
	p = cuit;
	for (;*p != '\0'; p++)
		*p = (*p - '0');

	// Calculo el dígito verificador.
	x = (cuit[7]*2) + (cuit[6]*3) + (cuit[5]*4) + (cuit[4]*5) +
		(cuit[3]*6) + (cuit[2]*7) + (cuit[1]*2) + (cuit[0]*3); 

	y = x / MODULO11;
	a = y * MODULO11;
	resto = x - a;
	dig = MODULO11 - resto;

	if (dig == 11)
		strcpy(digito, "0");
	else {
		if (dig == 10)
			strcpy(digito, "K");
		else 
			IToStr(dig, digito);
	}

	if (digicarac[0] != '\0')
		strcpy(digfm, digicarac);
	else
		IToStr(cuit[RUT_SIZE - 1], digfm);
	
	if (strcmp(digfm, digito) != 0) {
		sprintf(error, "El número de RUT es incorrecto. El dígito verificador (%s) debería ser %s.", digfm, digito);
		return FALSE;
	}
	return TRUE;
}

void SacaGuionCuit(char *cuit, int longitud)
{
	char aux[RUT_SIZE];

	cuit[RUT_SIZE] = '\0';
	digicarac[0]  = '\0';
	strcpy(aux, cuit);

	for (i=0, j=0; aux[i] != '\0'; i++) {
		if (IsNumeric(aux[i]))
			cuit[j++]=aux[i];
		else {
			if (IsCaracter(aux[i]))
				digicarac[0] = aux[i];
		}
	}
	cuit[j]='\0';
}
