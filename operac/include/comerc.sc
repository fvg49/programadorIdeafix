/*--------------------------------------------------------------------*/
schema	comerc	descr	"Esquema de Comercial"
							language "C";
/*--------------------------------------------------------------------*/

/* -------- SERVICIO -------- */
table servicio	descr		"Tabla de Servicios"
{
	codser		num(4)		descr "Código de Servicio"
								not null,
	descrip		char(70)	descr "Descripción del Servicio"
								not null,
	descor		char(20)	descr "Descripción Corta del Servicio"
								not null,
}
primary key(codser);

/* -------- NCBTES -------- */
table ncbtes	descr		"Tabla de Comprobantes"
{
	tipcomp		num(4)		descr "Tipo de Comprobante"
									not null
									> 0,
	descrip		char(30)	descr "Descripción del Comprobante"
								not null,
	descor		char(4)		descr "Descripción Corta del Comprobante"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(tipcomp);

/* -------- NSERIES --------- */
table nseries	descr		"Series de Comprobantes"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Comprobante"
								not null,
	serie		num(4) 		descr "Serie"
								not null,
	descrip		char(30)	descr "Descripción de la Serie"
								not null,
	descor		char(4)		descr "Descripción Corta de la Serie"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, tipcomp, serie);

/* -------- CBTENRO --------- */
table cbtenro	descr		"Nros. de Comprobantes"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Comprobante"
								not null,
	serie		num(4) 		descr "Serie"
								not null,
	deleg		char(3)		descr "Código de Delegación",
	nroact		num(9)		descr "Próximo número",
	iaser		num(4)		descr "Serie de Ianus",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, tipcomp, serie, deleg);

/* -------- REGIMEN -------- */
table regimen	descr		"Tabla de Regimen"
{
	dias		num(2)		descr "Cantidad de días"
								not null,
	dfran		num(2)		descr "Cantidad de días francos"
								not null,
	hsreg		num(4,2)	descr "Horas del regimen"
								not null,
	regim		char(8)		descr "Regimen",
	descrip		char(40)	descr "Descripción del regimen",
	hsnorm		num(4,2)	descr "Horas Normales"
								default 0
								not null,
	hsextra		num(4,2)	descr "Horas Extras"
								default 0
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	hssem		num(4,2)	descr "Horas Total Semanales",
	porhnor		num(6,2)	descr "Porcentaje Horas Normales Mensuales",
	porhext		num(6,2)	descr "Porcentaje Horas Extras Mensuales",
	partime		bool		descr "Regimen Part Time",
	emp			num(2)		descr "Empresa"
								not null,
}
primary key(dias, dfran, hsreg, partime),
index reg(regim, partime),
index regi(regim),
index partime (partime, regim),
index emp(emp, partime, regim);

/* -------- CONTR -------- */
table contr		descr		"Tabla de Tipos de Contratos"
{
	codcont		num(2)		descr "Código de Contrato"
								not null,
	descrip		char(30)	descr "Descripción del Contrato"
								not null,
	descor		char(20)	descr "Descripción Corta del Contrato"
								not null,
}
primary key(codcont);

/* -------- TPTOSER -------- */
table tptoser	descr		"Tipo de Puestos de Servicios"
{
	tippto		num(4)		descr "Código de Puestos de Servicios"
								not null,
	descrip		char(30)	descr "Descripción de Puestos de Servicios"
								not null,
	descor		char(20)	descr "Descripción Corta de Puestos de Servicios"
								not null,
	armas		bool		descr "Es puesto con armas?",
	grupo		num(4)		descr "Grupo de Tipo de Puesto",
	emp			num(2)		descr "Empresa"
								not null
								default 1,
}
primary key(tippto),
index emp(emp, tippto);

/* -------- OT -------- */
table ot		descr		"Orden de Trabajo"
{
	emp			num(2)		descr "Empresa"
								//in aurus.emps:(descrip),
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null
								in ncbtes:(descrip),
	serie    	num(4)		descr "Usuario"
								not null
								in nseries(emp, tipcomp):(descrip),
	deleg		char(3)		descr "Delegación"
								//in billpro.delega:descrip,
								not null,	
	nroot		num(9)		descr "Nro. de OT",
//								not null,
	cliente		num(9)		descr "Cliente"
								//in bill.cliente:(razsoc)
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
//	resumen		num(9)		descr "Resumen"
//								not null,
	fecreg		date		descr "Fecha de Registración"
								default today,
	codser		num(4)		descr "Código de Servicio"
								not null
								in servicio:(descrip),
	tipser		num(1)		descr "Tipo de Servicio"
								in (1 : "Rif",
									2 : "Esporádico"),
//								default 1,
	estvta		num(1)		descr "Estado de la OT en Operaciones"
								in (0: "Pendiente",
									1: "Aprobó Jefe Vta.",
									2: "Rechazó Jefe Vta.")
								default 0,
	estoper		num(1)		descr "Estado de la OT en Operaciones"
								in (0: "Pendiente",
									1: "Aprobó Operaciones",
									2: "Rechazó Operaciones")
								default 0,
	estadm		num(1)		descr "Estado de la OT en Administración"
								in (0: "Pendiente",
									1: "Aprobó Administración",
									2: "Rechazó Administración")
								default 0,
	codvend		num(8)		descr "Vendedor",
	fokvta		date		descr "Fecha de Aprobación de OT por J. de Vta.",
	hokvta		time		descr "Hora de Aprobación de OT por J. de Vta.",
	fokoper		date		descr "Fecha de Aprobación de OT por Operac.",
	hokoper		time		descr "Hora de Aprobación de OT por Operac.",
	fokoadm		date		descr "Fecha de Aprobación de OT por Adminis.",
	hokoadm		time		descr "Hora de Aprobación de OT por Adminis..",
	finicio		date		descr "Fecha de Inicio de la OT",
	hinicio		time		descr "Hora de Inicio de la OT",
	ffinal		date		descr "Fecha de Finalización de la OT",
	hfinal		time		descr "Hora de Finalización de la OT",
	obsop		char(100)	descr "Observaciones de Operaciones",
	obsfac		char(100)	descr "Observaciones de Facturación",
	abm			char(1)     descr "Alta/Baja/Modificación"
								in ("A": "Alta",
									"B": "Baja",
									"M": "Ampliación",
									"R": "Reducción",
									"C": "Actualización")
								not null
								default "A"
								mask ">A",
	riff		num(12,2)	descr "Rif",
	motrechv	char(60) 	descr "Motivo de Rechazo de J. de Vta.",
	motrecha	char(60) 	descr "Motivo de Rechazo de Administracion",
	motrecho	char(60) 	descr "Motivo de Rechazo de Operaciones",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	empmt		num(2)		descr "Empresa de MT",
	tipcompmt	num(4)		descr "Tipo de Documento de MT"
								in ncbtes:(descrip),
	seriemt		num(4)		descr "Usuario de MT"
								in nseries(emp, tipcomp):(descrip),
	delegmt		char(3)		descr "Delegación de MT",
	nrootmt		num(9)		descr "Nro. de OT de MT",
	impresa		bool		descr "OT impresa?",
	usucom		char(10)	descr "Usuario de Comercial",
	usuoper		char(10)	descr "Usuario de Operaciones",
	plazo		num(4)		descr "Plazo para montar servicio",
	usuadm		char(10)	descr "Usuario de Administracion",
	origen		num(1)		descr "Origen de la Ot"
								in (0: "Cargada Manualmente",
									1: "Ajuste IPC",
									2: "Ajuste por Indice",
									3: "Ajuste por Concepto",
									4: "Importada de Marte")
								default 0,
	anulado		bool		descr "OT anulada"
								default FALSE,
}
primary key(emp, tipcomp, serie, deleg, nroot),
index cliente(emp, cliente, objet, codser, nroot),
index tipser(emp, tipser),
index nroot(emp, cliente, objet, nroot),
index fecreg(emp, cliente, objet, fecreg, nroot),
index aprobc(estadm not null, emp, tipcomp, serie, deleg, nroot),
index aprobo(estoper not null, emp, tipcomp, serie, deleg, nroot),
index aprobv(estvta not null, emp, tipcomp, serie, deleg, nroot),
index aprob1(emp,cliente, objet, estadm not null,nroot),
index aprob2(emp,cliente, objet, estoper not null,nroot),
index aprob3(emp,cliente, objet, estvta not null,nroot),
index fecha(emp, fecreg, tipcomp, serie, deleg, nroot),
index mt(empmt, tipcompmt, seriemt, delegmt, nrootmt);

/* -------- PTOSER -------- */
table ptoser	descr		"Puesto de Servicio"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,	
	nroot		num(9)		descr "Nro. de Novedad"
								not null,
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
								not null
							    in tptoser:(descrip),
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	cond		char		descr "Tipo de Novedad"
								in ("A":"Alta",
									"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, tipcomp, serie, deleg, nroot, tippto),
index cliente(emp, cliente, objet, tippto);

/* -------- NPUESTO -------- */
table npuesto	descr		"Puestos de OT"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Novedad"
								not null,
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	nroreng		num(3)		descr "Nro. de Renglón"
								not null,
	puesto		num(4)		descr "Nro. de Puesto, equivale a la categoría de DENARIUS"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	hinicio		time		descr "Hora de Inicio del puesto"
								not null,
	hfinal		time		descr "Hora de Finalización del puesto"
								not null,
	dias[7]		char(1)		descr "Días"
								mask ">A"
								in ("L":"Lunes",
									"M":"Martes",
									"X":"Miércoles",
									"J":"Jueves",
									"V":"Viernes",
									"S":"Sábado",
									"D":"Domingo",
									"P":"Part Time"),
	regim		char(8)		descr "Regimen"
								not null,
	salario		num(9,2)	descr "Salario",
	cantpue		num(4)		descr "Cantidad de Puesto"
								not null,
	cantvig		num(4,2)	descr "Cantidad de Vigiladores",
	cond		char		descr "Tipo de Novedad"
								in ("A":"Alta",
									"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargó'",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modificó",
	concf		num(4)		descr "Concepto de Facturación",
	codfrec		char(2)		descr "Código de Frecuencia para Part Time",
	horapt		num(4)		descr "Horas para Puestos Part Time",
	hsnorm		num(9,2)	descr "Horas Normales     Costeo",
	hsextr		num(9,2)	descr "Horas Extras NO se usa ",
	hs50		num(6,2)	descr "Horas Extras 50%   Costeo",
	hs100		num(6,2)	descr "Horas Extras 100%  Costeo",
	subreg		char(8)		descr "SubRegimen"
							in subregim:descrip,
	tipodia		char(1)		descr "Tipo de dia"
							not null
							default "T"
							mask ">A"
							in ("F": "Solo Feriados",
								"H": "Solo No Feriados",
								"T": "Trabaja Siempre"),
	hs_fnorm	num(9,2)	descr "Horas Normales    Facturacion"
							default 0,
	hs_f50		num(6,2)	descr "Horas Extras 50%  Facturacion"
							default 0,
	hs_f100 	num(6,2)	descr "Horas Extras 100% Facturacion"
							default 0,
	cantrvig	num(4,2)	descr "Cantidad Real de Vigiladores",
	codint		num(4)		descr "Nro. Interno para cada puesto",
}
primary key(emp, tipcomp, serie, deleg, nroot, tippto, nroreng),
index cliente(emp, cliente, objet, tippto, puesto),
index nroot(emp, cliente, objet, nroot),
index codint(emp, tipcomp, serie, deleg, nroot, tippto, codint),
index codintcl(emp, cliente, objet, tippto, codint);


/* -------- CARACT -------- 
table caract	descr		"Características de Vigiladores"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,	
	nroot		num(9)		descr "Nro. de Orden de Trabajo"
								not null,
	reng		num(2)		descr "Nro. del Renglón"
								not null,
	carac		char(50)	descr "Característica"
								not null,
}
primary key(emp, tipcomp, serie, deleg, nroot, reng);
*/
/* -------- EQUIPOT -------- */
table equipot	descr		"Equipamiento por Cli/Obj"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Orden de Trabajo"
								not null,
	nroint		num(9)		descr "Nro. del Equipo" //igral.catalog
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	canti		num(6,2)	descr "Cantidad del Equipo"
								not null,
	cond		char		descr "Tipo de Novedad"
								in ("A":"Alta",
									"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	precio		num(15,2)	descr "Monto",
	precamor    num(15,2)    descr "Monto Amortizado",
}
primary key(emp, tipcomp, serie, deleg, nroot, nroint),
index cliente(emp, cliente, objet, nroint);

/* -------- TARIFA -------- */
table tarifa	descr		"Tarifa de Horas por Cli/Obj"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Orden de Trabajo"
								not null,
	conc		num(4)		descr "Nro. del Concepto"
								not null
								in itmfac:(descrip),
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	horas		num(8,2)	descr "Cantidad de Horas Facturadas",
	precio		num(9,2)	descr "Precio Unitario",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	consrif		bool        descr "Considera Rif?",
	hortra		num(8,2)	descr "Cantidad de Horas Trabajadas",
}
primary key(emp, tipcomp, serie, deleg, nroot, conc),
index cliente(emp, cliente, objet, conc, nroot);

/* -------- OBJETIVO -------- */
table objetivo	descr		"Objetivos de Clientes"
{
	cliente		num(9)		descr "Cliente"
								not null,
	resumen		num(9)		descr "Resumen",
//								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	descrip		char(50)	descr "Descripción del Contrato"
								not null,
	tipcont		num(2)		descr "Tipo de Contrato"
								in contr:(descrip),
	fechac		date		descr "Fecha de comienzo del contrato"
								default today,
	fechaf		date		descr "Fecha de fin de contrato",
	activo		bool		descr "Activo"
								default true
								not null,
	calle		char(60)	descr "Calle del domicilio",
//								not null,
	nro			char(8)		descr "Número del domicilio",
//								not null,
	piso 		char(3)		descr "Piso del domicilio",
	depto		char(4)		descr "Depto. del domicilio",
	local		num(5)		descr "Localidad del domicilio",
//								not null,
	codpos		char(10)	descr "Código postal",
							// in aurcus.cpostal:descrip - OJO QUE ESTE ES CHAR Y EL OTRO NUM!...
	pais		num(2)		descr "Código de país",
//								not null
//								default 1,
	prov		num(2)		descr "Provincia del domicilio",
//								not null,
	svisor		num(7)		descr "Supervisor", 
								// in sue.per(emp):(apynom),
	intern		num(9)		descr "Nro interno del objetivo",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	deleg		char(5)		descr "Código de la Delegación Comercial",      //in billpro.delega
	equipo		char(5)		descr "Equipo",                                 //in billpro.equipos
	vende		num(8)		descr "Vendedor",                               //in billpro.vendedor
	rif			num(1)		descr "Rif/Esporádico",
//	clilpag		num(9)		descr "Cliente de lugar de pago",
//	objlpag		num(4)		descr "Objetivo de lugar de pago",
	ofpag		num(6)		descr "Lugar de Pago",                          //in sue.ofpag
	delega		char(5)		descr "Código de la Delegación Geográfica",     //in delega:descrip
	subcon		bool		descr "Es Subcontratado?"
								default FALSE,
	objconot	bool		descr "Var. para insertar vigi. sin ot espo"
								default TRUE,                               //TRUE: no puede insetar, FALSE: si puede. No necesita ot.
	telef1		char(20)	descr "Teléfono",
	telef2		char(20)	descr "Teléfono",
	fax			char(20)	descr "Fax",
	zona		num(2)		descr "Código de la Zona"
								in zonains:(descrip),
	inspec		bool		descr "Es Inspeccionado?"
								default FALSE,
	emp			num(2)		descr "Empresa",
	consstd		bool		descr "Considera Standard?"
								not null
								default TRUE,
	consfer		bool		descr "Considera Feriados para el calculo de horas?"
								default TRUE,
	nroccte		num(6)		descr "Cuenta Corriente",
								//in aurus cctes(emp,tipccte):descrip,
	ccosto		num(8)		descr "Código de Centro de Costos",
								//in sue.CCOSTO(emp):(denom)
	codser		num(4)		descr "Código de Servicio"
								default 1,
	juris		num(2)		descr "Jurisdicción de Altas de Brigadas",
	program		num(7)		descr "Persona que se encarga de la Programacion", 
								// in sue.per(emp):(apynom),
	presen		num(7)		descr "Persona que se encarga del Presentismo", 
								// in sue.per(emp):(apynom),
    filial		char(6)		descr "Código de Filial" 
								in filial:(descrip),
}
primary key(cliente, objet),
index est(emp, activo, cliente, objet),
index resumen(resumen not null, cliente, objet),
index svisor(svisor, cliente, objet),
index nroint(intern),
index del(deleg, equipo, vende),
index equipo(equipo, cliente, objet),
index delcli(delega, cliente, objet),
index emp(emp, cliente, objet),
index program(program, cliente, objet),
index presen(presen, cliente, objet),
index filial(filial, cliente, objet),
index mdate(mdate, cliente, objet);

/* -------- REFER -------- */
table refer		descr		"Referente/Contacto Oper."
{
	cliente		num(9)		descr "Nro de Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	refer		char(1)		descr "Referente/Contac. Operación"
								in ("R" : "Referente Policial",
									"C" : "Contacto de Operaciones"),
	apellido	char(20)	descr "Apellido del Representante",
	nombre		char(30)	descr "Nombres del Representante",
	coddoc		char(4)		descr "Tipo de Documento"
								mask "4>a"
								in ("CI": "Cedula de Identidad",
									"DNI":"Documento Nacional de Identidad",
									"DU": "Documento Unico",
									"LE": "Libreta de Enrrolamiento",
									"LC": "Libreta Unica",
									"PAS":"Pasaporte")
								not null,
	nrodoc		num(9)		descr "Nro de documento"
								not null,
	telef		char(12)	descr "Nro de Teléfono",
	cargo		char(30)	descr "Cargo",
	codnac		num(2)		descr "Nacionalidad"
								not null,
	direc		char(35)	descr "Calle y Nro"
								not null,
	prov		num(2)		descr "Cod. Provincia"
								not null,
	local		num(5)		descr "Cod. Localidad"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente, objet, refer),
index mdate(mdate, cliente, objet, refer);

/* -------- ITMFAC -------- */
table itmfac	descr		"Item de Facturación"
{
	item 		num(4)		descr "Nro. Item"
							not null
							> 0,
	descrip		char(34)	descr "Descripción"
							not null,
	descor		char(20)	descr "Descripción corta"
							not null,
	precio		num(4)		descr "Var Fija de Billing (Precio Hs.)",
	cstd		num(4)		descr "Var Fija de Billing (Cant. Hs. Std.)",
	cant		num(4)		descr "Var de Nov. de Billing (Cant. Hs.)",
	concepto	num(4)		descr "Concepto",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	subconc		bool		descr "Es Subconcepto?",
	concpad		num(4)		descr "Concepto Padre",
	porhora		bool  		descr "Se factura por hora o en forma directa"
								not null
								default true,
	factu  		bool  		descr "Se factura ?"
								not null
								default true,
	pcero  		bool  		descr "Precio Cero ?"
								not null
								default false,
	concesp		bool		descr "Es Concepto especial ?"
								not null
								default false,
	padesp		num(4)		descr "Concepto Padre de este que es especial",
	rifhora		bool  		descr "Se considera para el rif por hora"
								not null
								default true,
	tipconc     num(2)      descr "Tipo de Concepto"
								not null
								default 1, 
}
primary key(item),
index precio(precio, item),
index cstd(cstd, item),
index cant(cant, item),
index porcon(concepto,item);


/* -------- ITMXPUE -------- */
table itmxpue	descr		"Items de Factur. por Puestos"
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
	item 		num(4)		descr "Nro. Item"
								not null,
	estado		num(1)		descr "Estado del Item",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	consrif		bool		descr "Considera Rif?",
}
primary key(cliente, objet, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim, item);


/* -------- NMODELO -------- */
table nmodelo	descr		"Tabla de modelos de Listado"
{
	mod			num(4)		descr "Código de Modelo",
	descrip		char(20)	descr "Descripción del modelo",
}
primary key (mod);

/* -------- MODCOMP -------- */
table modcomp	descr		"Tabla de Comprobantes por Modelos de Listados"
{
	mod			num(4)		descr "Código de Modelo"
								in NMODELO,
	comp		num(4)		descr "Código de Comprobante",
}
primary key (mod,comp);

/*------------------- MTXCONC ----------------------------*/
table mtxconc	descr		"Conceptos de Margen Teórico"
{
	empmt		num(2)		descr "Empresa de MT",
	tipcompmt	num(4)		descr "Tipo de Documento de MT"
								in ncbtes:(descrip),
	seriemt		num(4)		descr "Usuario de MT"
								in nseries(empmt, tipcompmt):(descrip),
	delegmt		char(3)		descr "Delegación de MT",
	nrootmt		num(9)		descr "Nro. de MT",
	codconc		num(4)		descr "Codigo de Concepto",
	subconc		num(4)		descr "Codigo de SubConcepto",
	valn		num(9,3)	descr "Importe por Concepto",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	fecreg		date		descr "Fecha de registracion" 
	                        not null
	                        default today,
}
primary key (empmt, tipcompmt, seriemt, delegmt, nrootmt, codconc, subconc);

/* -------- SUBREGIMEN -------- */
table subregim	descr		"Tabla de SubRegimen"
{
	subreg		char(8)		descr "SubRegimen",
	descrip		char(40)	descr "Descripción del regimen",
	hsnorm		num(4,2)	descr "Horas Normales"
								default 0
								not null,
	hsextra		num(4,2)	descr "Horas Extras"
								default 0
								not null,
	hssem 		num(6,2)	descr "Promedio de horas semanales"
								default 0
								not null,
	porhsn		num(6,2)	descr "Porcentajes de horas normales mensuales"
								default 0
								not null,
	porhse		num(6,2)	descr "Porcentajes de horas extras mensuales"
								default 0
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	emp			num(2)		descr "Empresa"
								not null,
}
primary key(subreg),
index emp(emp, subreg);

/* -------- PROVXDEL -------- */
table provxdel	descr		"Tabla de Cross Provincia/Delegación"
{
	deleg		char(5)		descr "Delegación"
								not null,                       //in billpro.delega
	prov		num(2)		descr "Provincia"
								not null,                       //in sue.provi
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(deleg, prov),
index prov(prov, deleg);

/* -------- PROVXCC -------- */
table provxcc	descr		"Tabla de Cross Provincia del objetivo /Tipo de centro de costo de denarius"
{
	emp			num(2)		descr "Empresa"                 //in sue.emps
								not null,
	pais		num(2)		descr "Número de pais"          //in sue.pais
								> 0,
	prov		num(2)		descr "Cod. Provincia"          //in sue.provi
								not null,
	tipccto		num(4)		descr"Código del agrupamiento"
								not null,                   //in sue.tipocc
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(tipccto, emp, pais, prov),
index prov(emp, pais, prov, tipccto);

/* -------- RELCLI -------- */
table relcli	descr		"Tabla de Relación entre Clientes"
{
	clipadre	num(9)		descr "Cliente Padre"
								not null,                 //in bill.cliente
	clihijo		num(9)		descr "Cliente Hijo"
								not null,                 //in bill.cliente
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(clipadre, clihijo),
index clihijo(clihijo);


/* -------- FERCLI -------- */
table fercli	descr		"Feriados por cliente objetivo"
{
	cliente		num(9)		descr "Nro de Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	fecha		date		descr "Fecha de Feriado"
								not null,
	descrip		char(25)	descr "Descripción del Feriado"
								not null,
	feriado		bool		descr "Es feriado ?"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente, objet, fecha),
unique index mdate(mdate, cliente, objet, fecha);

/* -------- ITMXPTO -------- */
table itmxpto	descr		"Items de Factur. por Puestos"
{
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
							    in tptoser:(descrip)
							    not null,

	item 		num(4)		descr "Nro. Item"
								in itmfac:(descrip)
								not null,

	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(tippto, item);

/* --------- CONVIG ---------- */
table convig descr "Convenios de vigilancia por empresa"
{
	emp			num(2)		descr "Empresa"
								not null,
  	relacion	num(2)		descr "Tipo de convenio"
								not null,
								//in conv:descrip,
}
primary key(emp);


/* ------  TIPOMA  ------------------ */
table tipoma descr "Tipo de Medio Auxiliar"
{
	tipoma		num(4)		descr "Tipo de Medio Auxiliar"
								not null,
	descrip		char(50)	descr "Descripcion Tipo de Medio Auxiliar",

}
primary key(tipoma);

/* ------  MAXTIPO  ------------------ */
table maxtipo descr "Medio Auxiliar por Tipo"
{
	nroint		num(9)		descr "Equipamiento de IANUS" //igral.catalogo
								not null,
	tipoma		num(4)		descr "Tipo de Medio Auxiliar"
								in tipoma:(descrip)
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico"
}
primary key(nroint),
index tipoma(tipoma, nroint);

/* ------  FILIAL  ------------------ */
table filial descr "Filiales"
{
	filial		char(6)		descr "Código de Filial" 
								not null,
	deleg		char(5)		descr "Código de la Delegación"
								not null,
	descrip		char(50)	descr "Descripcion de la Filial"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico" 
}
primary key (filial),
index deleg(deleg);

/* ------  USRXFIL  ------------------ */
table usrxfil descr "Usuarios por Filial"
{
	filial		char(6)		descr "Código de Filial" 
								not null,
	codusu		num(4)		descr "Codigo de usuario"
								not null, //in bill.susuario:(descrip)  
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico" 
}

primary key (filial, codusu),
index usuario(codusu);


/* -------- TPTOXEMP -------- */
table tptoxemp	descr		"Tipo de Puestos de Servicios por Empresa"
{
	emp			num(2)		descr "Empresa"
								not null
								default 1,
	tippto		num(4)		descr "Código de Puestos de Servicios"
								not null,
}
primary key(emp, tippto);

/* -------- CIEXEMP  -------- */
table ciexemp	descr		"Fechas de cierre por Empresa"
{
	emp			num(2)		descr "Empresa"
								not null
								default 1,
	tipcie		num(2)		descr "Tipo de Cierre"
								not null,
	feccie		date		descr "Fecha de Ultimo Cierre"
								not null,
}
primary key(emp, tipcie, feccie);

/* -------- CONDPTO (No se usa) -------- */
table condpto	descr		"Condición Actual de los puestos"
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
	salario		num(6,2)	descr "Salario",
	cantpue		num(4)		descr "Cantidad de Puesto",
	cantvig		num(4,2)	descr "Cantidad de Vigiladores",
	estado		char		descr "Estado Actual"
								not null,
	fecmov		date		descr "Fecha de último movimiento"
								not null,
	nroot		num(9)      descr "Nro. de Ot. que genero el último movimiento"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente, objet, tippto, fecmov, nroot, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim),
index fecha (cliente, objet, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim, fecmov, nroot),
index lugar (cliente, objet, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim),
index puesto(cliente, objet, tippto, puesto, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim);

/* -------- CONDEQ (No se usa) -------- */
table condeq	descr		"Condición Actual de los Equipos por CLIENTE-OBJETIVO"
{
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	nroint		num(9)		descr "Nro. del Equipo" //igral.catalog
								not null,
	nroreng		num(3)      descr "Nro. de Renglon"
								not null,
	canti		num(4,2)	descr "Cantidad del Equipo",
	estado		char		descr "Estado Actual"
								not null,
	fecmov		date		descr "Fecha de último movimiento"
								not null,
	nroot 		num(9)	    descr "Nro. de Ot que efectuo el último movimiento"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente, objet,fecmov, nroot, nroint),
index fecha(cliente, objet, nroint, fecmov, nroot);

/* -------- CONDTAR (No se usa) -------- */
table condtar	descr		"Condición Actual de las Tarifas por CLIENTE-OBJETIVO"
{
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	conc		num(4)		descr "Nro. del Equipo" //igral.catalog
								not null,
	precio		num(8,2)	descr "Precio Unitario" ,
	horas		num(8,2)	descr "Cantidad de Horas",
	fecmov		date		descr "Fecha de último movimiento"
								not null,
	nroot		num(9)      descr "Nro. de Ot. que genero el último movimiento"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente, objet, fecmov, nroot, conc, precio),
index fecha(cliente, objet, conc, precio, fecmov, nroot),
index conc (cliente, objet, conc, fecmov);

/* -------- GRUPOPTO -------- */
table grupopto	descr		"Grupo de Tipos de Puestos"
{
	grupo 		num(4)		descr "Grupo"
								not null,
	descrip		char(25)	descr "Descripcion del Grupo",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(grupo);

/* -------- BONOS -------- */
table bonos		descr		"Tabla de Bonos"
{
	bono		num(4)		descr "Código de Bono"
									not null
									> 0,
	descrip		char(30)	descr "Descripción del Bono"
								not null,
	descor		char(15)		descr "Descripción Corta del Bono"
								not null,
	nivel		num(1)		descr "Nivel del Bono"
								in (1: "Nivel Objetivo",
									2: "Nivel Puesto")
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(bono),
index nivel(nivel, bono);

/* -------- LISTABON -------- */
table listabon		descr		"Tabla de Precio de Bonos"
{
	cliente		num(9)		descr "Cliente"
								not null,                       //in bill.cliente
	objetivo	num(2)		descr "Objetivo"
								not null
								in objetivo(cliente):(descrip),
	bono		num(4)		descr "Código de Bono"
								not null
								in bonos:descrip,
	fecvig		date		descr "Fecha de Vigencia"
								not null,
	precio		num(9,2)	descr "Precio Unitario"
								not null,
	remun		num(9,2)	descr "Precio Remuneración",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(cliente,objetivo,bono,fecvig),
index fecvig(fecvig, cliente, objetivo, bono);


/* -------- PRECATE -------- */
table precate	descr		"Categorias sin Presentismo"
{
	cod			num(2)		descr "Código de Convenio"
								not null, // in CONV:descrip,
	codcat		num(4)		descr "Código de Categoria"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (cod, codcat);

/* --------- CATEXPTO ---------- */
table catexpto descr "Tipos de puesto x categoria"
{
	tippto		num(4)		descr "Código de Puestos de Servicios"
								not null,
	relacion	num(2)		descr "Tipo de convenio"
								not null,
								//in conv:descrip,
	codcat		num(4)		descr "Código de Categoria"
								not null,
								//in sue.cate
	activo 		bool		descr "Activo?"
								not null
								default true,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico"
}
primary key(tippto, relacion, codcat);

/* -------- RANKVEN -------- */
table rankven	descr		"Rif Acum. a Dic 98"
{
	vende		num(8)		descr "Vendedor"                //in billpro.vendedor
								not null,
	riff		num(12,2)	descr "Rif",
}
primary key(vende);

/* -------- RIFACUM -------- */
table rifacum	descr		"Rif Acum. a Dic 98"
{
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	riff		num(12,2)	descr "Rif",
}
primary key(cliente, objet);

/* -------- BONOT -------- */
table bonot		descr		"Bono por Cli/Obj"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Orden de Trabajo"
								not null,
	bono		num(4)		descr "Código de Bono"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	canti		num(4,2)	descr "Cantidad del Bono"
								not null,
	precio		num(9,2)	descr "Precio Unitario"
								not null,
	remun		num(9,2)	descr "Precio Remuneración",
	cond		char		descr "Tipo de Novedad"
								in ("A":"Alta",
									"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, tipcomp, serie, deleg, nroot, bono),
index cliente(emp, cliente, objet, bono);

/* -------- BONXPUE -------- */
table bonxpue	descr		"Bonos por Puestos"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Novedad"
								not null,
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
	nroreng		num(3)		descr "Nro. de Renglón"
								not null,
	bono		num(4)		descr "Bono"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	cant		num(4,2)	descr "Cantidad de Bonos"
								not null,
	precio		num(9,2)	descr "Precio Unitario",
	remun		num(9,2)	descr "Precio Remuneración",
	cond		char		descr "Tipo de Novedad"
								in ("A":"Alta",
									"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargó'",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modificó",
}
primary key(emp, tipcomp, serie, deleg, nroot, tippto, nroreng, bono),
index cliente(emp, cliente, objet, tippto, bono),
index nroot(emp, cliente, objet, nroot);

/* ------  LUPAXDEL  ------------------ */
table lupaxdel descr "Lugar de Pago por Delegación"
{
	emp         num(2)      descr "Empresa" 
							not null,
	delega		char(5)		descr "Código de la Delegación"
							not null,
	ofpag		num(6)		descr "Lugar de Pago"
							not null                          //in sue.ofpag
}
primary key (emp, delega, ofpag);

/* -------- ZONAINS -------- */
table zonains	descr		"Tabla de Zonas de Inspección"
{
	zona		num(2)		descr "Código de la Zona"
								not null,
	descrip		char(25)	descr "Descripción de la Zona"
								not null,
	descor		char(20)	descr "Descripción Corta de la Zona"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(zona);

/* ------ EMPXREG   ------------------ */
table empxreg descr "Empresa por Regimen"
{
	emp         num(2)      descr "Empresa" 
							not null,
	dias		num(2)		descr "Cantidad de días"
								not null,
	dfran		num(2)		descr "Cantidad de días francos"
								not null,
	hsreg		num(4,2)	descr "Horas del regimen"
								not null,
	partime		bool		descr "Regimen Part Time"
								not null,
	regesp		bool		descr "Regimen Especial"
								not null default false, 	
}
primary key (emp, dias, dfran, hsreg, partime), 
index ptime (emp, partime, dias, dfran, hsreg);

/* ------ REGXSUB   ------------------ */
table regxsub descr "Regimen por Subregimen"
{
	emp         num(2)      descr "Empresa" 
							not null,
	dias		num(2)		descr "Cantidad de días"
								not null,
	dfran		num(2)		descr "Cantidad de días francos"
								not null,
	hsreg		num(4,2)	descr "Horas del regimen"
								not null,
	partime		bool		descr "Regimen Part Time"
								not null,
	subreg		char(8)		descr "SubRegimen"
								not null,
}
primary key (emp, dias, dfran, hsreg, partime, subreg), 
index subreg(emp, subreg, dias, dfran, hsreg, partime), 
index ptime(emp, partime, subreg, dias, dfran, hsreg);

/* -------- PLANINS -------- */
table planins	descr		"Plan de Inspección"
{
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	inspdiu		num(4)		descr "Cantidad de Inspecciones Diurnas",
	inspnoc		num(4)		descr "Cantidad de Inspecciones Nocturnas",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
	finicio		date		descr "Fecha de comienzo del nuevo plan",
}
primary key(cliente, objet, finicio);

/* -------- ZONA -------- */
table zona		descr		"Tabla de Zonas Geográficas"
{
	prov		num(2)		descr "Provincia"
								not null,                       //in sue.provi
	zona		num(2)		descr "Código de la Zona"
								not null,
	descrip		char(25)	descr "Descripción de la Zona"
								not null,
	descor		char(20)	descr "Descripción Corta de la Zona"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(prov, zona);

/* -------- PROVXZON -------- */
table provxzon	descr		"Provincias por Zonas"
{
	emp			num(2)		descr "Empresa"
								not null,
	prov		num(2)		descr "Provincia"
								not null,                       //in sue.provi
	zonag		num(2)		descr "Código de la Zona"
								not null
								in zona(prov):(descrip),
	local		num(5)		descr "Localidad"
								not null,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, prov, zonag, local),
index loc(emp, prov, local, zonag);

/* -------- OBSRECH --------- */
table obsrech	descr		"Observación de Rechazos de Comprobantes"
{
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		num(4)		descr "Usuario"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,
	nroot		num(9)		descr "Nro. de Novedad"
								not null,
	nroreng		num(4)		descr "Renglón de la Observación"
								not null,
	descrip		char(70)	descr "Descripción",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(emp, tipcomp, serie, deleg, nroot, nroreng);

/* -------- SUCXOBJ -------- */
table sucxobj descr "Tabla Cross Sucursal/Objetivo"
{
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	sucursal	num(4)		descr "Sucursal"
								not null,
	descrip		char(30)	descr "Descripción de la sucursal" 
								not null,
}
primary key(emp, cliente, objetivo);


grant alter on schema comerc to public;
grant all on * to public with grant option;
