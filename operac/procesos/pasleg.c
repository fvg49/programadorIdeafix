/********************************************************************
* MODULE & VERSION : pasleg.c
* DATE             : 12/11/10
*
* CREATED 	       : Diego Capellán
* DESCRIPTION      : Importa Datos de los Legajos a Denarius 
*
*********************************************************************/
#include <ideafix.h>
#include "pasleg.fmh"
#include "sue.sch"

#define MAXBUF 500

/* Declaraciones globales */
static fm_status after(form, fmfield, int);
static char *GetField(char *buff, char Separ);


/* Declaraciones globales */
form fm0;
FILE * arch;
int  emp, estciv, codtar, codsex, activo, codnac, codpro, codloc, codcat;
long nroleg, nrodoc;
char buff[MAXBUF], apenom[100], direcc[100], telefo[100], codpos[50], coddoc[50];
DATE fecing, fecnac, fecegr;

wcmd(pasleg, 1.0  12/11/10)
{
	fm0 = OpenForm("pasleg", FM_EABORT);

	if (DoForm(fm0, NULLFP, after) == FM_UPDATE) {
		
		BeginTransaction();
		while (fgets(buff, MAXBUF, arch) != NULL) {


			strcpy(apenom,  GetField(buff, '\t'));
			emp    = StrToI(GetField(buff, '\t'));
			strcpy(direc,   GetField(buff, '\t'));
			estciv = StrToL(GetField(buff, '\t'));
			fecing = StrToD(GetField(buff, '\t'));
			fecnac = StrToD(GetField(buff, '\t'));
			fecegr = StrToD(GetField(buff, '\t'));
			nroleg = StrToL(GetField(buff, '\t'));
			nrodoc = StrToL(GetField(buff, '\t'));
			codtar = StrToI(GetField(buff, '\t'));
			codsex = StrToI(GetField(buff, '\t'));
			activo = StrToI(GetField(buff, '\t'));
			strcpy(telefo,  GetField(buff, '\t'));
			strcpy(coddoc,  GetField(buff, '\t'));
			GetField(buff, '\t'); //cuit
			codnac = StrToI(GetField(buff, '\t'));
			codpro = StrToI(GetField(buff, '\t'));
			codloc = StrToI(GetField(buff, '\t'));
			strcpy(codpos,  GetField(buff, '\t'));
			GetField(buff, '\t'); //relacion
			GetField(buff, '\t'); //fecan
			codcat = StrToI(GetField(buff, '\t'));

			SetKey(sue|PERbyEMP, emp, nroleg));
			if (GetRecord(sue|PERbyEMP, THIS_KEY, IO_NOT_LOCK) != ERROR) {
				
			}
			else {
				InitRecord(scue|PER);

				SetIFld(sue|PER_EMP,      emp);
				SetLFld(sue|PER_NROLEG,   nroleg);
				SetFld (sue|PER_APYNOM,   apenom);
				SetFld (sue|PER_DIREC,    direc);
				SetIFld(sue|PER_LOCAL,    codloc);
				SetIFld(sue|PER_PROVIN,   codprov);
				SetFld (sue|PER_TELEF,    telefo);
				SetFld (sue|PER_CODPOST,  codpos);
				SetDFld(sue|PER_FECNAC,   fecnac);
				SetDFld(sue|PER_FECING,   fecing);
				SetDFld(sue|PER_FECEGR,   fecegr);
				SetIFld(sue|PER_CODNAC,   codnac);
				SetIFld(sue|PER_ESTCIV,   estciv);
				SetIFld(sue|PER_SEXO,     sexo);
				SetFld (sue|PER_CODDOC,   coddoc);
				SetLFld(sue|PER_NRODOC,   nrodoc);
				SetIFld(sue|PER_RELACION, emp);
				SetLFld(sue|PER_CODCCOS,  1);
				SetFld (sue|PER_CODESTR,  "1");
				SetLFld(sue|PER_CODUBI,   1);
				SetIFld(sue|PER_CODCAT,   101);
				SetIFld(sue|PER_CODCAL,   1);
				SetLFld(sue|PER_CODTAR,   codtar);
				SetIFld(sue|PER_CODEST,   1);
				SetTFld(sue|PER_HDESDE,   StrToT("000000"));
				SetIFld(sue|PER_FORMA,    1);
				SetIFld(sue|PER_CODCJ,    1);
				SetIFld(sue|PER_RESERVADO,1);
				SetIFld(sue|PER_ACTIVO,   activo);
				SetIFld(sue|PER_TIPSERV,  1);

				PutRecord(scue|PER);
			}
        }
		EndTransaction();
	}
}	

static fm_status after(form fm, fmfield fno, int row)
{
	switch (fno) {
	case NOMARC:
		if ((arch = fopen(FmSFld(fm0, NOMARC), "r")) == NULL) {
			WiDialog(WD_OK, WD_OK, "Error", "No se pudo abrir el archivo %s", FmSFld(fm0, NOMARC));
			return FM_REDO;
		}
	    break;
	}
	return FM_OK;
}

static char *GetField(char *buffy, char Separ)
{
	static char *p;
	static char dest[LONG_BUF];
    static bool prim_fin;
	char    *q = dest;

	if (buffy){
		 p = 
		 buffy;
	     err_fin = prim_fin = FALSE;
	}     

	while (*p != Separ && *p != '\n')
		*q++ = *p++;

    
    if (prim_fin) err_fin=TRUE;
      
    if (*p == '\n')	prim_fin = TRUE;
    
	*q = '\0';
	++p;
	return dest;
}

