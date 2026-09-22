/*--------------------------------------------------------------------*/
schema	comgral	descr	"Esquema de Novia - Comercial Gral"
							language "C";
/*--------------------------------------------------------------------*/

/* -------- IPC -------- */
table ipc	descr		"IPC's"
{
	ipc			num(2)		descr "Indice IPC"
								not null
								> 0,
	descrip		char(30)	descr "Descripcion"
								not null,
	tipind		num(2)		descr "Tipo de Indice"	// in aurus.tind  
									not null
									> 0,	
    meses		num(2)		descr "Meses de Actualizacion",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (ipc);

/* -------- IPCCLI -------- */
table ipccli	descr		"Clientes/IPC"
{
	cliente		num(9)		descr "Cliente" // in bill.cliente
								not null,
	fechavig	date		descr "Fecha de Vigencia"
								not null,
	ipc			num(2)		descr "Tipo de Indice"
									not null
									in ipc:descrip
									> 0,	
	mesini		num(2)		descr "Mes de Inicio"
									between 1 and 12,
	activo		bool		descr "Activo ?"
							not null
							default true,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (cliente, fechavig);

/* -------- INDOBJ -------- */
table indobj	descr		"Objetivos/IPC"
{
	cliente		num(9)		descr "Cliente" // in bill.cliente
								not null,
	objet		num(4)		descr "Objetivo" // in comerc.objet (cliente)
								not null,
	fechavig	date		descr "Fecha de Vigencia"
								not null,
	porc		num(5,2)	descr "Porcentaje"
								not null,
	meses[12]	bool		descr "Meses[Month-1]"
								not null
								default false,
	mesaniv		char		descr "Aplica Mes Aniversario"
								in ("C":"Base Fecha de Cliente",
									"I":"Base Fecha de Instalacion"),
	activo		bool		descr "Activo ?"
								not null
								default true,
	tipind		num(2)		descr "Tipo de Indice - Orden de Calculo"
									not null
									> 0,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (cliente, objet, fechavig, tipind),
unique index tipind (cliente, objet, tipind, fechavig);

/* -------- INDCPTO -------- */
table indcpto	descr		"Objetivos/IPC"
{
	cliente		num(9)		descr "Cliente" // in bill.cliente
								not null,
	objet		num(4)		descr "Objetivo" // in comerc.objet (cliente)
								not null,
	concepto	num(4)		descr "Concepto",
	fechavig	date		descr "Fecha de Vigencia"
								not null,
	porc		num(5,2)	descr "Porcentaje",
    valor		num(12,2)	descr "Valor",
	activo		bool		descr "Activo ?"
								not null
								default true,
    newval		num(12,2)	descr "Valor",
    procesado	bool		descr "Procesado ?"
    							not null
    							default false,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (cliente, objet, concepto, fechavig);

/* -------- CONDCOM -------- */
table condcom	descr		"Condiciones Comerciales"
{
	emp			num(2)		descr "Empresa"
							not null,		// in aurus.emp
	condic		num(2)		descr "Condicion"
							not null,
	descrip		char(30)	descr "Descripcion"
							not null,
	abrev		char(10)	descr "Abreviatura"
							not null,
}
primary key (emp, condic);

/* -------- CONDOBJ -------- */
table condobj	descr		"Condiciones Comerciales / Objetivo"
{
	emp			num(2)		descr "Empresa"
							not null,		// in aurus.emp
	cliente		num(9)		descr "Cliente" // in bill.cliente
								not null,
	objet		num(4)		descr "Objetivo" // in comerc.objet (cliente)
								not null,
	condic		num(2)		descr "Condicion"
							in condcom(emp):descrip
							not null,
	fcarga		date		descr "Fecha de Carga"
								not null,
}
primary key (emp, cliente, objet, condic);

/* -------- HSVIAJE -------- */
table hsviaje	descr		"Tiempo de Viaje al Objetivo"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	fecvig		date		descr "Fecha de Vigencia",
	horvia		num(4,2)	descr "Hora de Viaje",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, cliente, objet, fecvig);

/* -------- COSCPAGO -------- */
table coscpago	descr		"Condicion de pago por cliente / objetivo"
{
	cliente		num(9)		descr "Cliente" // in bill.cliente
								not null,
	objet		num(4)		descr "Objetivo" // in comerc.objet (cliente)
								not null,
	fechavig	date		descr "Fecha de Vigencia"
								not null,
	cpagi	char(4)			descr "Condición de pago individual"
							not null,
							// in ctascob.tiptran by ttran2(2):descrip,
	descrip		char(50)	descr "Descripcion",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (cliente, objet, fechavig);


/* -------- CROSSCAT -------- */
table crosscat	descr		"Cross de categorias salariales"
{
	convenio	num(2)		descr "Tipo de convenio"
								not null,
	catvieja	num(4)		descr "Categoria Salarial Vieja"
								not null,
	catnueva	num(4)		descr "Categoria Salarial Nueva"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (convenio, catvieja, catnueva),
index catnueva (convenio, catnueva, catvieja);

/* -------- OBJSUPLEM -------- */
table objsuplem	descr		"Objetivo que cobran Suplemento Especial"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	activo		bool		descr "Activo"
								default true,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, cliente, objet);

/* ------  CLIXTER  -------------------- */
table clixter descr "Cliente x Tercero"
{
	tercero 	char (18)	descr "Tercero (CUIT)",
	cliente		num (9)		descr "Cliente",
	preter		char(2) 	descr "Prefijo de subtercero",
	activo 		bool		descr "Activo?",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modifico", 
}
primary key(tercero, cliente),
index preter(tercero, preter),
index activo(activo, cliente, tercero),
index cliente(cliente, tercero);

/* ------  CATNOVIA  -------------------- */
table catnovia descr "Catálogo de Novia"
{
	nroint		num(9)		descr "Número Interno de administración"
								not null,
	emp			num(2)		descr "Empresa"
								not null,
	familia		char(6)		descr "Familia de Oracle",
	subfam		char(6)		descr "SubFamilia de Oracle",
	item        char(14)    descr "Código de Administración Interna",
	descrip     char(240)   descr "Descripción del Material"
								not null,
	origen		char(4)		descr "Origen"
								not null,
	bol_novia	bool		descr "Activo Novia ?"
								not null,
	codnovia	char(14)	descr "Código Novia"
								not null,
	tipoma		num(4)		descr "Tipo de Medio Auxiliar",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modifico", 
}
primary key(emp, nroint),
index novia(bol_novia, emp, nroint),
index origen(origen, emp, nroint),
index codnov(emp, codnovia),
index tipoma(tipoma, emp, nroint),
index coditem(emp, familia, subfam, item);


/* ------  DETCATNV  -------------------- */
table detcatnv descr "Catálogo de Costos de Novia"
{
	nroint		num(9)		descr "Número Interno de administración"
								not null,
	emp			num(2)		descr "Empresa"
								not null,
	fval		date		descr "Fecha Valor"
								not null
								default TODAY,
	monto		num(15,2)	descr "Monto"
								not null
								>= 0.0,
	mon			num(2)		descr "Moneda de expresion del monto"
								default null,
	plazo		num(3)		descr "Plazo de Amortización"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modifico", 
}
primary key(emp, nroint, fval);


/* -------- MOVOBJ  -------- */
table	movobj		descr		"Movimiento de Objetivos a otra empresa"
{
	empo		num(2)		descr "Empresa origen"
								not null,
	clienteo	num(9)		descr "Cliente Origen"
								not null,
	objetivo	num(4)		descr "Objetivo Origen"
								not null,
	empd		num(2)		descr "Empresa destino"
								not null,
	cliented	num(9)		descr "Cliente Destino"
								not null,
	objetivd	num(4)		descr "Objetivo Destino"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(empo, clienteo, objetivo),
index destino(empd, cliented, objetivd);


table	parnov		descr		"Parametros de Novia"
{
	emp			num(2)		descr "Empresa"
								not null,
	cod			num(4)		descr"Codigo de Parametro"
								not null,
	descrip		char(30)	descr "Descripcion"
							not null,
	observa		char(240)	descr "Observacion"
							not null,
}
primary key(emp, cod),
index codigo(cod, emp);

table	depano		descr		"Detalle de Parametros de Novia"
{
	emp			num(2)		descr "Empresa"
								not null,
	cod			num(4)		descr"Codigo de Parametro"
								not null,
	nroren		num(4)		descr"Numero de Renglon"
								not null,
	fecvig		date		descr "Fecha de vigencia"
								not null,
	act			bool		descr "activo"
							in (true :"Si", false:"No")
 							not null,
 	valor		char(50)	descr "Valor"
 							not null,
}
primary key(emp, cod, nroren, fecvig),
index activo(act, emp, cod, nroren, fecvig);

table	leghoras	descr	"Horas por Legajo"
{
	emp		num(2)	descr "Empresa"
								not null,
	nroleg      num(7)      descr "Legajo"
								not null,
	canths		num(12,2)	descr "Cantidad de Horas"
								not null,  
}
primary key(emp, nroleg);

/* -------- VALIFE  -------- */
table	valife	descr	"Variable por Liquidacion por Fecha"
{
	intern	num(9)	descr "Nro interno-univoco de usuario (PROD)"
					not null,
	nrovar	 num(4)	descr "Número de variable"
					not null,
	fecvig	date	descr "Fecha de vigencia"
					not null,
	nroliq	 num(9)	descr "Número de liquidación"
					not null,
	cdate	date	descr "Fecha de Carga",
	ctime	time	descr "Hora de Carga",
	cuid	num(5)	descr "Usuario que Cargo",
	mdate	date	descr "Fecha de Modificacion",
	mtime	time	descr "Hora de Modificacion",
	muid	num(5)	descr "Usuario que Modifico",
}
primary key(intern, nrovar, fecvig, nroliq),
index nroliq (nroliq, intern, nrovar, fecvig);

grant alter on schema comgral to public;
grant all on * to public with grant option;
