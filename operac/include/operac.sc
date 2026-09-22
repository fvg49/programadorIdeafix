/*--------------------------------------------------------------------*/
schema	operac	descr	"Esquema de operaciones"
							language "C";
/*--------------------------------------------------------------------*/

/* -------- FERIADO -------- */
table feriado	descr		"Feriados"
{
	fecha		date		descr "Fecha del Feriado"
								not null,
	descrip		char(25)	descr "Descripción del Feriado"
								not null,
	pais		num(2)		descr "País"
								not null,
	prov		num(2)		descr "Provincia"
								not null,
}
primary key(fecha, pais, prov),
index pais(pais, prov, fecha);


/* -------- MOTIVD --------- */
table motivd	descr		"Motivo de Desasignación"
{
	codmotd		num(2)		descr "Código Motivo de Desasignación"
								not null,
	descrip		char(25)	descr "Descripción Motivo de Desasignación"
								not null,
	descor		char(15)	descr "Descripción Corta Motivo de Desasignación",
	m_inc		bool		descr "Se incluye en parte Diario"
								not null
								default true,
	rotacion	bool		descr "Motivo para Rotacion de Vigiladores"
								default false,
	activo		bool		descr "Activo?"
								default TRUE,
}
primary key(codmotd),
index activo(activo, codmotd);

/* -------- ASIG -------- */
table asig		descr		"Asignación de Vigiladores"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	ptoser		num(4)		descr "Cod. de Puesto de Servicio",   //puestos.tippto
	puesto		num(4)		descr "Nro. de Puesto",               //puestos.codint
	nroleg		num(7)		descr "Legajo"
								not null,
	vigil		char(1)		descr "Tipo de Vigilador"
								in ("V":"Vigilador",
									"R":"Reten",
									"P":"PartTime"),
	efect		char		descr "Efectivo/Provisorio"
								in ("E": "Efectivo",
									"P": "Provisorio"),
	fecasig		date		descr "Fecha de Asignación",
	hsent		time		descr "Hora de Entrada",
	hssal		time		descr "Hora de Salida",
	dia1		char(1)		descr "Días",
	dia2		char(1)		descr "Días",
	dia3		char(1)		descr "Días",
	dia4		char(1)		descr "Días",
	dia5		char(1)		descr "Días",
	dia6		char(1)		descr "Días",
	dia7		char(1)		descr "Días",
	reempl		num(7)		descr "Vig. Reemplazado",
	ffranco		date		descr "Fecha del primer franco",
	regim		char(8)		descr "Regimen",
	fechas		date		descr "Fecha Hasta de Vig. Provisorios",
	fecbaj		date		descr "Fecha de Desasignación",
	numfran		num(2)		descr "Número del dia de franco",
	francero 	bool		descr "Es francero?",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
	tipodia		char(1)		descr "Tipo de dia"
								not null
								default "T"
								mask ">A"
								in ("F": "Solo Feriados",
									"H": "Solo No Feriados",
									"T": "Trabaja Siempre"),
	codrol		num(4)		descr "Rol",
	fila		num(2)		descr "Renglón del Rol",
	colum		num(2)		descr "Columna del Rol",
	regpto		char(8)		descr "Regimen del Puesto",
}
primary key(emp, cliente, objetivo, nroleg, ptoser, puesto, nroint),
index fecha(emp, cliente, objetivo, fecasig, nroleg),
index nroleg(emp, nroleg, cliente, objetivo),
index legfec(emp, nroleg, fecasig, cliente, objetivo),
index puesto(emp, cliente, objetivo, ptoser, puesto, nroint, nroleg);

/* -------- ASIGH -------- */
table asigh		descr		"Asignación de Vig. Histórica"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	ptoser		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	puesto		num(4)		descr "Nro. de Puesto"
								not null,
	nroleg		num(7)		descr "Legajo"
								not null,
	vigil		char(1)		descr "Tipo de Vigilador"
								in ("V":"Vigilador",
									"R":"Reten",
									"P":"PartTime"),
	efect		char(1)		descr "Efectivo/ Provisorio"
								in ("E": "Efectivo",
									"P": "Provisorio"),
	fecalt		date		descr "Fecha de Asignación"
								not null,
	fecbaj		date		descr "Fecha de Desasignación"
								not null,
	hsent		time		descr "Hora de Entrada",
	hssal		time		descr "Hora de Salida",
	dia1		char(1)		descr "Días",
	dia2		char(1)		descr "Días",
	dia3		char(1)		descr "Días",
	dia4		char(1)		descr "Días",
	dia5		char(1)		descr "Días",
	dia6		char(1)		descr "Días",
	dia7		char(1)		descr "Días",
	motivo		num(2)		descr "Motivo de Desasignación"
								in motivd:(descrip),
	reempl		num(7)      descr "Vig. Reemplazado",
	regim		char(8)		descr "Regimen",
	fechas		date		descr "Fecha Hasta de Vig. Provisorios",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	ffranco		date 		descr "Fecha del primer franco",
	numfran		num(2)		descr "Número del dia de franco",
	francero 	bool		descr "Es francero?",
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
	tipodia		char(1)		descr "Tipo de dia"
							not null
							default "T"
							mask ">A"
							in ("F": "Solo Feriados",
								"H": "Solo No Feriados",
								"T": "Trabaja Siempre"),
	codrol		num(4)		descr "Rol",
	fila		num(2)		descr "Renglón del Rol",
	colum		num(2)		descr "Columna del Rol",
	regpto		char(8)		descr "Regimen del Puesto",
}
primary key(emp, cliente, objetivo, ptoser, puesto, nroint, nroleg, fecbaj, fecalt),
index puesto(emp, cliente, objetivo, nroleg, ptoser, puesto, nroint),
index nroleg(emp, nroleg, cliente, objetivo),
index fechabaj(emp, cliente, objetivo, fecbaj, nroleg, ptoser, puesto, nroint),
index legfec(emp, nroleg,fecbaj, cliente, objetivo),
index fechaalt(emp, cliente, objetivo, nroleg, fecalt, ptoser, puesto, nroint); 


/***********************************************************************
 ****** El cuadrante real tendrá estados. 1: A Confirmar          ******
 ******                                   2: Confirmado           ******
 ****** Cuando el cuadrante fue modificado y confirmado, no se    ******
 ****** podrá modificar más. Debe quedar bloqueado.               ******
 ***********************************************************************/

/* -------- PARTE -------- */
table parte		descr		"Parte Diario"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	dia			date		descr "Fecha del Parte"
								not null,
	nroleg		num(7)		descr "Legajo"
			 					not null,
	horaent		time		descr "Hora de Entrada",
//								not null,
	horasal		time		descr "Hora de Salida",
//								not null,
	confir		num(1)		descr "Estado de Hs.Normales del Parte"
								in (0: "A Confirmar",
									1: "Confirmado",
									2: "Cerrado",
									3: "Cerrado para facturar")
								default 0,
	hsnor		num(4,2)	descr "Horas Normales"
									default 0,
	hs50		num(4,2)	descr "Horas Extras 50%"
									default 0,
	hs100f		num(4,2)	descr "Horas Extras 100% Franco"
									default 0,
	hs100fe		num(4,2)	descr "Horas Extras 100% Feriado"
									default 0,
	condic		char(1)		descr "Condición de Trabajo",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	confex		num(1)		descr "Estado de Hs.Extras del Parte"
								in (0: "A Confirmar",
									1: "Confirmado",
									2: "Cerrado"),
	ptoser		num(4)		descr "Cod. de Puesto de Servicio"
								default 1
								not null,
	puesto		num(4)		descr "Nro. de Puesto"
								default 1
								not null,
	fecgen		date		descr "Fecha de Generación",                     
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
	codaus		num(4)		descr "Código de Ausentismo",
	liqdena		num(9)		descr "Nro. Liquidación de Denarius",
	liqfac		num(9)		descr "Nro. Liquidación de Billing",
	asicble		num(9)		descr "Nro. de Asiento Contable",
	nrofac		num(9)		descr "Nro. de Factura",
	liqdenus	num(9)		descr "Nro. Liquidación de Denarius Ult. Semana",
	asicblus	num(9)		descr "Nro. de Asiento Contable Ult. Semana",
	hprog		char(20)	descr "Programa que Graba Horas",
	hdate		date		descr "Fecha de Grabacion de Horas",
	htime		time		descr "Hora de Grabacion de Horas",
	huid		num(5)		descr "Usuario de Grabacion de Horas",
}
primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
index leg(emp, cliente, objetivo, nroleg, dia),
index emple(emp, nroleg, dia, cliente, objetivo),
index puesto(emp, cliente, objetivo, dia, ptoser, puesto, nroint, nroleg),
index dia(emp, dia, cliente, objetivo), 
index rpuesto(emp, cliente, objetivo, ptoser, puesto, nroint, dia, nroleg);

/* -------- CONDICION --------- */
table condicion	descr		"Condición de Horas"
{
	codcond		num(2)		descr "Código de Condición"
								not null,
	descrip		char(25)	descr "Descripción del Condición"
								not null,
	descor		char(15)	descr "Descripción Corta del Condición",
}
primary key(codcond);

/* -------- MOTEXC --------- */
table motexc	descr		"Motivo de Excepción"
{
	codcond		num(2)		descr "Código de Motivo"
								not null
								in condicion:(descrip),
	codmot		num(2)		descr "Código de Motivo"
								not null,
	descrip		char(25)	descr "Descripción del Motivo"
								not null,
	valcli 		bool		descr "Valida cliente-objetivo en tabla motxcli"
								default false
								not null,
	vemp		num(3)		descr "Número de variable del empleado",
		 						// in sue:varemp
	tipmot		num(2)		descr "Tipo de motivo de la excepción",
}
primary key(codcond, codmot);

/* -------- EXCEPCION -------- */
table excepcion	descr		"Excepción"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	dia			date		descr "Fecha del Parte"
								not null,
	nroleg		num(7)		descr "Legajo"
			 					not null,
	condic		num(2)		descr "Condición"
								in condicion:(descrip),
	motivo		num(2)		descr "Motivo de Excepción"
								in motexc(condic):(descrip),
	horas		num(4,2)	descr "Horas",
	hs50		num(4,2)	descr "Horas Ext. 50%",
	hs100		num(4,2)	descr "Horas Ext. 100% Franco",
 	obs			char(50)	descr "Observaciones",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	ptoser		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	puesto		num(4)		descr "Nro. de Puesto"
								not null,
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
}
primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo),
index legajo (emp, nroleg, dia, cliente, objetivo);


/* -------- PUESTOS -------- */
table puestos	descr		"Codificador de Puestos para OPERACIONES"
{
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	puesto		num(4)		descr "Nro. de Puesto, equivale a la categoría de DENARIUS"
								not null,
	hinicio		time		descr "Hora de Inicio de la OT"
								not null,
	hfinal		time		descr "Hora de Finalización de la OT"
								not null,
	dia1		char(1)		descr "Días"
								mask ">A",
	dia2		char(1)		descr "Días"
								mask ">A",
	dia3		char(1)		descr "Días"
								mask ">A",
	dia4		char(1)		descr "Días"
								mask ">A",
	dia5		char(1)		descr "Días"
								mask ">A",
	dia6		char(1)		descr "Días"
								mask ">A",
	dia7		char(1)		descr "Días"
								mask ">A", 
	regim		char(8)		descr "Regimen"
								not null,
	codint		num(4)		descr "Nro. Interno para cada puesto"
								not null,
	cantvig		num(4,2)	descr "Cantidad de Vigiladores",
	vigi		num(4,2)	descr "Cantidad de Vigiladores que faltan asignar",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	codfrec		char(2)		descr "Codigo de Frecuencia"
								in frecuen:descrip,
	horapt		num(4)		descr "Horas para Puestos Part Time",
	finicio		date		descr "Fecha de Inicio de la OT",
	ffinal		date		descr "Fecha de Finalización de la OT",
	hsnorm      num(6,2)	descr "Horas Normales",
	hsextr      num(6,2)	descr "Horas Extras",
	cantpue		num(4)		descr "Cantidad de Puestos",
	tipodia		char(1)		descr "Tipo de dia"
							not null
							default "T"
							mask ">A"
							in ("F": "Solo Feriados",
								"H": "Solo No Feriados",
								"T": "Trabaja Siempre"),
	newint		num(4)		descr "Nro. Interno que reemplazara este puesto",
	padreint	num(4)		descr "Nro. Interno que reemplazara este puesto",
	codmot		num(4)		descr "Código de Motivo de horas improductivas",
	subreg		char(8)		descr "SubRegimen",
}
primary key(cliente, objet, tippto, codint),
index puesto(cliente, objet, tippto,puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim, codfrec, tipodia),
index padre(cliente, objet, tippto, padreint);

/*----------- VACAC -------------------*/
table vacac		descr		"Tabla donde se cargan las vacaciones por empleado"
{
	emp			num(2)		descr "Empresa a la que se le aplica el cierre"
								not null,
	nroleg		num(7)		descr "Nro de Legajo"
								not null,
	cantdias	num(3)		descr "Cantidad de días que se tomará",
	modulo		num(2)		descr "Módulo",
	fdesde		DATE		descr "Fecha desde a partir de la cual empiezan las vacaciones",
	fhasta		DATE		descr "Fecha hasta a partir de la cual empiezan las vacaciones",
	fpase		DATE		descr "Fecha de pasaje de esta información a Personal.",
	periodo		num(4)		descr "Periodo de Vacaciones",
	fecreg		DATE		descr "Fecha de Registración",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary	key(emp, nroleg, fdesde),
index periodo(emp, nroleg, periodo, fdesde),
index fecha(emp, fecreg, nroleg);

/* -------- PERIODOS --------*/
table periodos	descr		"Tabla de Periodos por Empresas"
{
	emp			num(2)		descr "Empresa a la que se le aplica el cierre"
								not null
							primary key,
	mescompn	bool		descr "mes completo hs. normales?"
								not null,
	ddesnor		num(2)		descr "Dia desde Horas normales",
	dhasnor		num(2)		descr "Dia hasta Horas normales",
	mescompe	bool		descr "mes completo hs. extras?"
								not null,
	ddesex		num(2)		descr "Dia desde Horas extras",
	dhasex		num(2)		descr "Dia hasta Horas extras",
};

/* -------- CIERRE -------- */
table cierre	descr		"Tabla de Periodos de cierre"
{
	emp			num(2)		descr "Empresa a la que se le aplica el cierre"
								not null,
	nrocier		num(4)		descr "Número interno de cierre",
	mesc		num(2)		descr "mes de cierre"
								not null,
	anioc		num(4)		descr "año de cierre"
								not null,
	fdesnor		date		descr "Fecha desde Horas normales",
	fhasnor		date		descr "Fecha hasta Horas normales",
	fdesex		date		descr "Fecha desde Horas extras",
	fhasex		date		descr "Fecha hasta Horas extras",
	feccie		date		descr "Fecha de Cierre Operaciones",
	horacie		time		descr "Hora de Cierre Operaciones",
	usucie		num(5)		descr "Usuario que cerro Operaciones",
	fecimp		date		descr "Fecha de Importación en Personal",
	horaimp		time		descr "Hora de Importación en Personal",
	usuimp		num(5)		descr "Usuario que Importó en Personal",
	tipocier	num(1)		descr "Tipo de Cierre"
								in (1 : "Parcial",
									2 : "Definitivo"),
	fecimpa		date		descr "Fecha de Importación en Administración",
	horaimpa	time		descr "Hora de Importación en Administración",
	usuimpa		num(5)		descr "Usuario que Importó en Administración",
	fdesret		date		descr "Fecha Desde Horas Retroactivas",
	fhasret		date		descr "Fecha Hasta Horas Retroactivas",
}
primary key (emp, nrocier),
index fecha (emp, mesc, anioc, nrocier);


/* -------- FRECUEN -------- */
table frecuen	descr 		"Frencuencia para puestos de PartTime"
{
	codfrec		char(2)		descr "Codigo de Frecuencia"
							not null,
	descrip		char(20)	descr "Descripción"
							not null,
}
primary key (codfrec);

/* -------- DIASPTIME -------- */
table diasptime	descr		"Dias que tiene asignado un vigilador parttime para det. puesto"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	nroleg		num(7)		descr "Legajo"
								not null,
	dia			date		descr "dia que trabaja en ese puesto",
	hent		time		descr "hora desde la que trabaja",
	hsal		time		descr "hora hasta la que trabaja",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	tippto		num(4)		descr "Cod. de Puesto de Servicio",
	puesto		num(4)		descr "Nro. de Puesto, equivale a la categoría de DENARIUS",
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,

} primary key(emp, cliente, objetivo, nroleg, tippto, puesto, nroint, dia),
index dia (emp, nroleg, dia, cliente, objetivo, tippto, puesto, nroint);

/* -------- DIASPTIMEH -------- */
table diasptimeh descr		"Dias que tiene asignado un vigilador parttime para det. puesto xhistorica"
{
	emp			num(2)		descr "Empresa"
 								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	nroleg		num(7)		descr "Legajo"
								not null,
	dia			date		descr "dia que trabaja en ese puesto",
	hent		time		descr "hora desde la que trabaja",
	hsal		time		descr "hora hasta la que trabaja",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	tippto		num(4)		descr "Cod. de Puesto de Servicio",
	puesto		num(4)		descr "Nro. de Puesto, equivale a la categoría de DENARIUS",
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
} primary key(emp, cliente, objetivo, nroleg, tippto, puesto, nroint, dia),
index dia (emp, nroleg, dia, cliente, objetivo, tippto, puesto, nroint);

/* -------- RETRO -------- */
table retro		descr		"Retroactivos"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	dia			date		descr "Fecha del Parte"
								not null,
	fecreg		date		descr "Fecha de Registración",
	nroleg		num(7)		descr "Legajo"
			 					not null,
	horaent		time		descr "Hora de Entrada",
	horasal		time		descr "Hora de Salida",
	confir		num(1)		descr "Estado de Hs.Normales del Parte"
								in (0: "A Confirmar",
									1: "Confirmado",
									2: "Cerrado",
									3: "Cerrado para facturar")
								default 0,
	hsnor		num(4,2)	descr "Horas Normales"
								default 0,
	hs50		num(4,2)	descr "Horas Extras 50%"
								default 0,
	hs100f		num(4,2)	descr "Horas Extras 100% Franco"
								default 0,
	hs100fe		num(4,2)	descr "Horas Extras 100% Feriado"
								default 0,
	condic		char(1)		descr "Condición de Trabajo",
	confex		num(1)		descr "Estado de las Hs. Retroactivas"
								in (0: "A Confirmar",
									1: "Confirmado",
									2: "Cerrado"),
	ptoser		num(4)		descr "Cod. de Puesto de Servicio"
								default 1
								not null,
	puesto		num(4)		descr "Nro. de Puesto"
								default 1
								not null,
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	codaus		num(4)		descr "Código de Ausentismo",
	dhsnor		num(4,2)	descr "Diferencia de Horas Normales",
	dhs50		num(4,2)	descr "Diferencia de Horas Extras 50%",
	dhs100f		num(4,2)	descr "Diferencia de Horas Extras 100% Franco",
	dhs100fe	num(4,2)	descr "Diferencia de Horas Extras 100% Feriado",
	liqdena		num(9)		descr "Nro. Liquidación de Denarius Mensual",
	liqfac		num(9)		descr "Nro. Liquidación de Billing",
	asicble		num(9)		descr "Nro. de Asiento Contable Mensual",
	nrofac		num(9)		descr "Nro. de Factura",
	liqdenus	num(9)		descr "Nro. Liquidación de Denarius Ult. Semana",
	asicblus	num(9)		descr "Nro. de Asiento Contable Ult. Semana",
	origen		num(1)		descr "Origen del retroactivo"
							in (0: "Carga Manual",
								1: "Carga Automatica"),
}
primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint),
index rleg(emp, cliente, objetivo, nroleg, dia),
index remple(emp, nroleg, dia, cliente, objetivo),
index rdia(emp, dia, cliente, objetivo),
index rfecreg(emp, fecreg, cliente, objetivo);

/* -------- RETROEXC -------- */
table retroexc	descr		"Excepción"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	dia			date		descr "Fecha del Parte"
								not null,
	nroleg		num(7)		descr "Legajo"
			 					not null,
	condic		num(2)		descr "Condición"
								in condicion:(descrip),
	motivo		num(2)		descr "Motivo de Excepción"
								in motexc(condic):(descrip),
	horas		num(4,2)	descr "Horas",
	hs50		num(4,2)	descr "Horas Ext. 50%",
	hs100		num(4,2)	descr "Horas Ext. 100% Franco",
 	obs			char(50)	descr "Observaciones",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	ptoser		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	puesto		num(4)		descr "Nro. de Puesto"
								not null,
	nroint		num(4)		descr "Número interno de puesto"
								default 1
								not null,
	dhoras		num(4,2)	descr "Diferencia de Horas Normales",
	dhs50		num(4,2)	descr "Diferencia de Horas Extras 50%",
	dhs100		num(4,2)	descr "Diferencia de Horas Extras 100%",
}
primary key(emp, cliente, objetivo, dia, nroleg, ptoser, puesto, nroint, condic, motivo),
index legajo (emp, nroleg, dia, cliente, objetivo);

/* -------- ROL -------- */
table rol		descr		"Rol"
{
	codrol		num(4)		descr "Rol"
								not null,
	descrip		char(25)	descr "Descripción del rol"
								not null,
	tiprol		num(1)		descr "Tipo de Rol"
								in (1: "Estático",
									2: "Dinámico"),
}
primary key(codrol);

/* -------- RROL -------- */
table rrol		descr		"Renglones del Rol"
{
	codrol		num(4)		descr "Rol"
								not null,
	fila		num(2)		descr "Renglón del Rol"
								not null,
	colum		num(2)		descr "Columna del Rol"
								not null,
	valor		char(1)		descr "Turno de la fila/col"
								mask ">X"
								in ("1":"Turno 1",
									"2":"Turno 2",
									"3":"Turno 3",
									"4":"Turno 4",
									"F":"Franco",
									"X":"No Trabaja"),
}
primary key(codrol, fila, colum);

/* -------- CIEFIL -------- */
table ciefil	descr		"Cierre por Filial"
{
	filial		char(6)		descr "Código de Filial" 
							not null,
	feccie		DATE		descr "Fecha de cierre"
							not null,
	idcarga	    num(5)		descr "Usuario que Carga"
							not null,
	feccarga 	DATE		descr "Fecha de Carga"
							not null,
	horacarga	TIME		descr "Hora de Carga"
							not null,
	revertido	bool		descr "Revertido?"
						  	not null
	  					  	default false,
	idrever	    num(5)		descr "Usuario que Revierte",
	fecrever 	DATE		descr "Fecha de Revercion",
	horarever	TIME		descr "Hora de Revercion",
}
primary key(filial, feccie, feccarga, horacarga),
unique index activa(revertido, filial, feccie, feccarga, horacarga);


/* -------- TPERMI -------- */
table	tpermi	descr		"Tipo de Permiso sobre vigiladores" 
{
	emp			num(2)		descr "Empresa"
							not null,
	tipper		num(2)		descr "Tipo de Permiso"
							not null,
	descrip		char(30)	descr "Descripcion"
							not null,
	descor		char(15)	descr "Descripción Corta",
	
}
primary key(emp, tipper);

/* -------- PERVIG -------- */
table	pervig	descr		"Permisos sobre vigiladores x Delegacion/Filial" 
{
	emp			num(2)		descr "Empresa"
							not null,
	nroleg		num(7)		descr "Legajo"
							not null,
	tipper		num(2)		descr "Tipo de Permiso"
							in tpermi(emp):(descrip)
							not null,
	delega		char(5)		descr "Código de la Delegación Geográfica Destino"
							not null,
	filial		char(6)		descr "Código de Filial Destino" 
							not null,
	fecini		DATE		descr "Fecha de Inicio"
							not null,
	fecfin		DATE		descr "Fecha de Finalizacion",
	delori		char(5)		descr "Código de la Delegación Geográfica Origen", 
	filori		char(6)		descr "Código de Filial Destino Origen",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	nummod		num(4)		descr "Numero de Modificacion",
	ultmod		bool		descr "Ultima Modificacion",
	activo		bool		descr "Activo?",
	fecha		date		descr "Fecha de Auditoria"
							not null, 
	hora		time		descr "Hora de Auditoria"
							not null, 
	usuari		num(5)		descr "Usuario de Auditoria"
							not null, 
}
primary key(emp, nroleg, delega, filial, nummod),
index ultmod(ultmod, emp, delega, filial, nroleg, nummod);

/* -------- GRUFIL -------- */
table	grufil	descr		"Grupos de Filiales" 
{
	emp			num(2)		descr "Empresa"
							not null,
	grupo		num(4)		descr "Codigo de grupo"
							not null,
	descrip		char(30)	descr "Descripcion"
							not null,
	descor		char(15)	descr "Descripción Corta",
}
primary key(emp, grupo);

/* -------- FILXGRUP -------- */
table	filxgrup	descr		"Filiales x Grupo"
{
	emp			num(2)		descr "Empresa"
							not null,
	grupo		num(4)		descr "Codigo de grupo"
							not null,
	delega		char(5)		descr "Código de la Delegación Geográfica" 
							not null,
	filial		char(6)		descr "Código de Filial" 
							not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, grupo, delega, filial),
index fil(emp, delega, filial, grupo);

/*------------- MOTXCLI -------------------*/
table motxcli	descr		"Motivo de Excepción"
{
	codcond		num(2)		descr "Código de Motivo"
								not null
								in condicion:(descrip),
	codmot		num(2)		descr "Código de Motivo"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(codcond, codmot, cliente, objetivo);


/* -------- TIPCIE -------- */
table tipcie    descr       "Tipo de Cierre"
{
    cotici      num(2)      descr " Codigo de Tipo de Cierre"
    							not null,
	descrip     char(30)    descr "Descripcion"
								not null,
}
primary key (cotici);

/* -------- PERIODO -------- */
table periodo   descr       "Periodo para calculo de horas trabajadas"
{
	emp         num(2)      descr "Empresa"
								not null,
	anoper      num(4)      descr "Año del Periodo"
								not null,
	numper      num(2)      descr "Numero de Periodo"
								not null,
	fecdes      date        descr "Fecha Desde"
								not null,
	fechas      date        descr "Fecha Hasta"
								not null,
	tipcie      num(2)      descr "Tipo de Cierre"
								in tipcie:(descrip)
								not null,
	estado		num(1)		descr "Estado"
								default 0
								not null
								in (0:"Pendiente de Procesar",
									1:"En Ejecucion",
									2:"Termino OK",
									3:"Termino solo con Warning",
									4:"Termino con Error"),

}
primary key (emp, anoper, tipcie, numper),
unique index fecdes (emp, tipcie, fecdes, anoper, numper);




grant alter on schema operac to public;
grant all on * to public with grant option;
