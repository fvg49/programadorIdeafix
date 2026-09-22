/********************************************************************
*
* MODULE & VERSION : @(#)espo.c	1.5 
* DATE             : 03/10/07 
* TIME             : 17:55:58 
*
* CREATED          : 14/10/98
*
* DESCRIPTION:
*      Descripcion del proposito general del modulo
*
* ROUTINE       |  PURPOSE
*---------------+---------------------------------------------------
*
*********************************************************************/
#include <ideafix.h>
#include "espo.fmh"
#include "comerc.sch"
#include "bill.sch"

/* Funciones privadas */
private void Proceso();
private void ProcesoRif();
private void ProcesoEspo();
         
/* Declaraciones globales */
FILE *fp1 = NULL;
form fm0;
dbcursor cli, obj, liqfac, liquid;
schema comerc, bill;
double totesp = 0, totrif = 0;
char buf[40];

/* Programa principal */
wcmd(espo, 1.5 10/07/03)
{
	fm_cmd cmd;

	fm0    = OpenForm  ("espo", FM_EABORT);
	comerc = OpenSchema("comerc", IO_EABORT);
	bill   = OpenSchema("bill", IO_EABORT);

	cli    = CreateCursor(CLIENTEbyCLIENTE, IO_NOT_LOCK);
	obj    = CreateCursor(comerc|OBJETIVObyCLIENTE, IO_NOT_LOCK);
	liqfac = CreateCursor(LIQFACbyCLIPIN, IO_NOT_LOCK);
	liquid = CreateCursor(LIQUIDbyPORNUM, IO_NOT_LOCK);

	while (DoForm(fm0, NULLFP, NULLFP) != FM_UPDATE) return;

  	Proceso();
}

private void Proceso()
{
	int objant, concant, mes;
	long clieant, horas;

	SetCursorFrom(cli, MIN_LONG);
	SetCursorTo  (cli, 10000);
	while (FetchCursor(cli) != ERROR) {
		SetCursorFrom(obj, LFld(CLIENTE_CLIENTE), MIN_SHORT);
		SetCursorTo  (obj, LFld(CLIENTE_CLIENTE), MAX_SHORT);

		while (FetchCursor(obj) != ERROR) {
			if (!IFld(comerc|OBJETIVO_ACTIVO))
				continue;

			totesp = 0, totrif = 0;

			sprintf(buf,"Cliente: %ld Obj : %d", LFld(CLIENTE_CLIENTE), LFld(comerc|OBJETIVO_OBJET));
			FmSetFld(fm0, TEXTO, buf);
			FmShowFlds(fm0, TEXTO, TEXTO);
			WiRefresh();
			if (fp1 == NULL)
				if ((fp1 = fopen("espo.dat", "w")) == NULL)
					Error("No se puede abrir el archivo espo.dat");

			SetCursorFrom(liquid, TRUE, FmDFld(fm0, FECD), MIN_LONG);
			SetCursorTo  (liquid, TRUE, FmDFld(fm0, FECH), MAX_LONG);
			while (FetchCursor(liquid) != ERROR) {
				SetCursorFrom(liqfac, LFld(CLIENTE_CLIENTE), LFld(comerc|OBJETIVO_INTERN), LFld(LIQUID_NROLIQ),
																MIN_SHORT, MIN_SHORT);
				SetCursorTo  (liqfac, LFld(CLIENTE_CLIENTE), LFld(comerc|OBJETIVO_INTERN), LFld(LIQUID_NROLIQ),
																MAX_SHORT, MAX_SHORT);
				while (FetchCursor(liqfac) != ERROR) {
					if (IFld(comerc|OBJETIVO_RIF))
						ProcesoEspo();
					else
						ProcesoRif();
				}
			}
			if (totesp != 0 || totrif != 0) {
				if (IFld(comerc|OBJETIVO_RIF))
					fprintf(fp1, "%4ld\t%02d\t%-35.35s\t%9.2f\t\n",
							LFld(CLIENTE_CLIENTE), IFld(comerc|OBJETIVO_OBJET), SFld(comerc|OBJETIVO_DESCRIP), totesp/100);
				else
					fprintf(fp1, "%4ld\t%02d\t%-35.35s\t%9.2f\t\n",
							LFld(CLIENTE_CLIENTE), IFld(comerc|OBJETIVO_OBJET), SFld(comerc|OBJETIVO_DESCRIP),totrif/100);
			}
		}
	}
}

private void ProcesoEspo()
{
	if (IFld(LIQFAC_SUBCON) != IFld(LIQFAC_CODCON))
		return;

	totesp += FFld(LIQFAC_VALOR);
}

private void ProcesoRif()
{
	if (IFld(LIQFAC_SUBCON) != IFld(LIQFAC_CODCON))
		return;
	if (IFld(LIQFAC_CODCON) == 5003 || IFld(LIQFAC_CODCON) == 5006 || IFld(LIQFAC_CODCON) == 5008 ||
		IFld(LIQFAC_CODCON) == 5018 || IFld(LIQFAC_CODCON) == 5020 || IFld(LIQFAC_CODCON) == 5048 ||
		IFld(LIQFAC_CODCON) == 5051 || IFld(LIQFAC_CODCON) == 5052)
		totrif += FFld(LIQFAC_VALOR);
}
