#include <ideafix.h>
#include "observa.h"
#include "comerc.h"
#include "comerc.sch"

void CargarComentario(struct comentario *principio, struct comentario **ultimo, short scod, char *scoment)
{
	if (*ultimo == &principio[MAXOBS])
		Error("Tabla interna saturada. Max %d", MAXOBS);

	(*ultimo)->cod  = scod;
	strcpy ((*ultimo)->sobs, scoment);
	(*ultimo)++;
}

void InicializoComentario(struct comentario *principio, struct comentario **ultimo)
{
	(*ultimo) = principio;
}

void PrincipioComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento)
{
	*elemento = principio;
}

int compobs_lib(struct comentario *a, struct comentario *b)
{
	return	a->cod   < b->cod   ? -1 : a->cod   > b->cod   ? 1 :
		    0;
}

bool ProximoComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento)
{
	
	if ((*elemento+1) >= ultimo) {
		return FALSE;
	}
 	(*elemento) ++;
	return TRUE;
}

bool HayComentario(struct comentario *principio, struct comentario *ultimo, struct comentario **elemento)
{
	if (ultimo > principio) {
		*elemento = principio;
		return TRUE;
	}

	return FALSE;
}

double Standard(short emp, long cliente, short objetivo, DATE fini)
{
	double cvig=0;
	struct spuesto estpue;
    _SParam_PVivo parhora;

	InicPuestosVivos(emp, cliente, objetivo,
							 NULL_SHORT, //Todos los Puestos
							 MODOTC,     //Modelo OT
							 fini,
							 fini,
							 FALSE,       //Carga Tarifas
							 FALSE,      //No cargue bonos
							 _VALIDAR_FECINI,
							 NULL_SHORT, // Todo tipo de OT
							 TRUE,
							 FALSE,
							 parhora,  //LEO Aca tiene que ser TRUE
							 TRUE);

	while (ProximoPuestoVivo (&estpue)) {
		if (!estpue.cpue)
			continue;

		cvig += estpue.crvig;
	}							 
	FinPuestosVivos ();

	return (cvig / 100.0);
}

double CalcularStd(short emp, long cliente, short objet, DATE foini, DATE fofin, 
                   DATE fini, DATE ffin, DATE maxfec, int paso, 
                   bool gcomen, struct comentario *principio, struct comentario **ultimo) 
{
	DATE fecha, fhas, nfec1, nfec2;
	bool pimpre = FALSE;
	double total=0;
	char cstd[150];
	bool exacto=FALSE;

	//Valido si es un mes exacto
	nfec1 = AddMonth(foini, 1);
	nfec2 = DMYToD(Day(foini), Month(nfec1), Year(nfec1)) -1;

	if (nfec2 != fofin){
		if (pimpre) fprintf(stderr, "EXACTO \n");
		exacto = TRUE;
	}

	if (pimpre) fprintf(stderr, "\n\n%d.- Viene a CalcularStd %ld %d fini %.3D ffin %.3D maxfec %.3D \n",
	                             paso, cliente, objet, fini, ffin, maxfec);

	if (ffin < fini) {
		if (pimpre) fprintf(stderr, "%d.- Termino \n", paso);
		return 0;
	}

	fhas = fofin;
	fecha = UltimaOt(emp, cliente, objet, maxfec);
	if (pimpre) fprintf(stderr, "%d.- Ultima Ot %.3D \n", paso, fecha);

	if (fecha == NULL_DATE) {
		if (pimpre) fprintf(stderr, "%d.- NULL_DATE return 0 \n", paso);
		return total;			
	}

	if (fecha > fini) {
		fhas = fecha-1;
		if (pimpre) fprintf(stderr, "%d.- SUMO EL STD del %.3D %.3D \n", paso, fecha, ffin);
		total += CalcularStd(emp, cliente, objet, foini, fofin, fecha, ffin, fecha, paso+1, 
								gcomen, principio, ultimo);

		if (pimpre) fprintf(stderr, "%d.- SUMO EL STD del %.3D %.3D \n", paso, fini, fhas);
		total += CalcularStd(emp, cliente, objet, foini, fofin, fini, fhas, fhas, paso+2,
								gcomen, principio, ultimo);
	}
	else {
		double porce, cantvig;

		if (pimpre) fprintf(stderr, "%d.- Calculo std al fini %.3D \n",paso, fini);

		cantvig = Standard (emp, cliente, objet, fini);

		if (exacto) {
			//Multiplicar total por la cantidad de dias
			porce = (double)(ffin - fini +1 ) / (double)COEF_DIAS_MES;
			total = cantvig * porce;
			if (pimpre) fprintf(stderr, "EXACTO %d.- Multiplico  ffin %.3D fini %.3D fofin %.3D foini %.3D fecha %.3D fhas %.3D dias %.4f porce %.4f cantv %.4f total %.4f \n", paso, ffin ,fini,fofin ,foini, fecha, fhas, (double)(ffin-fini+1), porce, cantvig, total);
		}
		else {
			//Multiplicar total por el porcentaje de dias
			if (pimpre) fprintf(stderr, "%d.- Multiplico  ffin %.3D fini %.3D fofin %.3D foini %.3D fecha %.3D fhas %.3D %.2f %.2f \n", paso, ffin ,fini,fofin ,foini, fecha, fhas, (double)(ffin-fini+1), (double)(fofin-foini+1));
			porce = 1.0 * (ffin - fini +1 ) / (double) (fofin - foini +1);
			total = cantvig * porce;
		}

		if (pimpre) fprintf(stderr, "%d.- Multiplico  por %.2f vig %.2f \n", paso, porce, cantvig);
		if (pimpre) fprintf(stderr, "%d.- Multiplico  porcen %.4f vig %.4f da %.4f \n", paso, porce, cantvig, total);
		if (exacto) {
			sprintf(cstd, "\tStandard exacto del %.3D al %.3D %d dias = cantvig std. %.2f * coef  %.3f =  %.2f ", fini, ffin, (ffin - fini +1 ), cantvig, porce, total);
		}
		else {
			sprintf(cstd, "\tStandard del %.3D al %.3D = cantvig std. %.2f * porcen  %.3f =  %.2f ", fini, ffin, cantvig, porce, total);
		}
		if (gcomen) {
			CargarComentario(principio, ultimo, paso, cstd);
		}
	}

	if (pimpre) fprintf(stderr, "%d.- Retorna %.2f \n", paso, total);
	return total;
}

DATE UltimaOt(short emp, long cliente, short objet, DATE maxfec)
{
	bool uimpre=FALSE, encontro=FALSE;
	DATE fecini;
	DATE ultfec = NULL_DATE;
	schema comerc, old;

	old = CurrentSchema();
	comerc = OpenSchema("comerc", IO_EABORT);
	SwitchToSchema(old);

	if (maxfec == NULL_DATE) {
		maxfec = MAX_DATE;
	}

	if (uimpre) fprintf(stderr, "Viene OBJ %ld  %d fecha %.3D \n",cliente, objet, maxfec);

	SetIFld(comerc|OT_EMP, emp);
	SetLFld(comerc|OT_CLIENTE, cliente);
	SetIFld(comerc|OT_OBJET, objet);
	SetDFld(comerc|OT_FECREG, MAX_DATE);

	//Leo la O.T. anterior por fecha de registracion
	while (!encontro && GetRecord(comerc|OTbyFECREG, PREV_KEY|PARTIAL_KEY, IO_NOT_LOCK, 3) != ERROR ) {
		
		if (uimpre) fprintf(stderr, "Viene OT %ld \n", LFld(comerc|OT_NROOT));

		if (IFld(comerc|OT_TIPCOMP) != OTCOMERC) {
			continue;
		}

		// if (IFld(comerc|OT_TIPSER) != RIF || IFld(comerc|OT_CODSER) != 1) {
		//	continue;
		// }

		if (IFld(comerc|OT_ESTOPER) != APROBADO || IFld(comerc|OT_ESTADM) != APROBADO ||
					IFld(comerc|OT_ESTVTA)  != APROBADO) {
			continue;
		}

  		if (uimpre) fprintf(stderr, "La OT %ld es valida \n", LFld(comerc|OT_NROOT));
		fecini = DFld(comerc|OT_FINICIO) == NULL_DATE ? DFld(comerc|OT_FFINAL) : DFld(comerc|OT_FINICIO);

		if (ultfec != NULL_DATE && (fecini+90) < ultfec ) {
			if (uimpre) fprintf(stderr, "La OT %ld fecini %.3D no sigo buscando \n", LFld(comerc|OT_NROOT), fecini);
			encontro = TRUE;
		}

		if (uimpre) fprintf(stderr, "Fecha de inicio = %.3D \n", fecini);
		if (fecini >= maxfec) {
			if (uimpre) fprintf(stderr, "Ot posterior a lo pedido no se considera \n", fecini);
			continue;
		}

		if (fecini > ultfec) {
			if (uimpre) fprintf(stderr, "Ahora la ultima OT %ld %.3D \n", LFld(comerc|OT_NROOT), fecini);
			ultfec = fecini;
		}
	}

	if (uimpre) fprintf(stderr, "Devuelve %.3D \n", ultfec);
	return ultfec;
}

