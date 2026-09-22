/*[20c--------------------------------------------------------------------*/

schema	webinter	descr	"Interface a Web "  language "C";
/*--------------------------------------------------------------------*/
table	TMPVEMP				descr "Tabla de valores de variables del empleado TMP_VAREMP"
{
	emp			num(2)		descr "Empresa"
								not null,
	nropase		num(9)		descr "Numero de Pase"
								not null,
	nroleg		num(7)		descr "Legajo del empleado"
								check digit "-",
	nroliq		num(6)		descr "Número de liquidación"
								not null,
	vemp		num(3)		descr "Número de variable del empleado" not null,
	ccosto		num(8)		descr "Centro de costo si tiene apertura",
	valemp		num(8)		descr "Valores de las variables del empleado"
								default 0,
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key (emp, nropase, nroleg, nroliq, vemp, ccosto);

/*--------------------------------------------------------------------*/
table TMPASIST 				descr "Tabla de control de Asistencias" {
	emp			num(2)		descr "Empresa"
								not null,
	nropase		num(9)		descr "Numero de Pase"
								not null,
	nroliq		num(6)		descr "Número de liquidación"
								not null,
	nroleg		num(7)		descr "Código de Empleado" check digit "/"
								not null,
	codnov		num(4)		descr "Código de Novedad de la Inasistencia"
								not null,
	fecha		date    	descr "Fecha de Novedad de la Inasistencia",
	valor		num(5,2)	descr "Valor de la Inasistencia",
	justif		num(1)		descr "Ausencia justificada/no justificada",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}primary key (emp, nropase, nroliq, fecha, nroleg, codnov);

/*--------------------------------------------------------------------*/
table WPARAM 			descr "Parametros para Interfeces" {
	codpar	num(2)		descr "Código de parámetro"
							not null,
	valor	char(50)	descr "Valor del parámetro",
	cuid	num(5)		descr "Usuario que creó el registro",
	cdate	date		descr "Fecha de creación",
	ctime	time		descr "Hora de creación",
	muid	num(5)		descr "Usuario que Modificacion",
	mdate	date		descr "Fecha de Modifi",
	mtime	time		descr "Hora de creación",
}
primary key(codpar);

/*--------------------------------------------------------------------*/
table WPASELOG 				descr "Log de Pases" {
	codprog		char(30)	descr "Codigo de Programa"
								not null,
	oisesion	char(50)	descr "Oid Session"
								not null,
	fecped		date		descr "Fecha de Pedido"
								not null,
	horped		time		descr "Hora de Pedido"
								not null,
	fecini		date		descr "Fecha de Pedido",
	horini		time		descr "Hora de Pedido",
	fecfin		date		descr "Fecha de Pedido",
	horfin		time		descr "Hora de Pedido",
	usuario		char(30)	descr "Usuario"
								not null,
	estado		num(1)		descr "Estado"
								not null
								in (0:"Pendiente de Procesar",
									1:"En Ejecucion",
									2:"Termino OK",
									3:"Termino solo con Warning",
									4:"Termino con Error"),
}primary key (codprog, oisesion);

/*--------------------------------------------------------------------*/
table WPARLOG 				descr "Parametro de Log de Pases" {
	codprog		char(30)	descr "Regla de Negocio"
								not null,
	oisesion	char(50)	descr "Oid Session"
								not null,
	param		char(15)	descr "Parametro"
								not null,
	valor		char(70)	descr "Valor"
								not null,
}primary key (codprog, oisesion, param);

/*--------------------------------------------------------------------*/
table WMSGLOG 				descr "Mensajes de log de Pases" {
	codprog		char(30)	descr "Codigo de Programa"
								not null,
	oisesion	char(50)	descr "Oid Session"
								not null,
	fecha		date		descr "Fecha del pase"
								not null,
	hora		time		descr "Hora del pase"
								not null,
	nroreng		num(4)		descr "Numero de Renglon"
								not null,
	tipo		num(1)	descr "Tipo de Mensajes"
//								in (0: "Mensaje",1:"Warning", 2:"Error")
								not null,
	mensaje		char(70)	descr "Valor"
								not null,
	solucion	bool		descr "Fue solucionado?"
								in (true:"si", false:"no")
								default false
								not null,
}primary key (codprog, oisesion, fecha, hora, nroreng),
index solucion (solucion, tipo, codprog, oisesion);


/*--------------------------------------------------------------------*/

table	TMPVARNOV			descr "Tabla Temporal de VARNOV" {
	idpase		num(9)		descr "Numero de Pase"
								not null,
	codprog		char(30)	descr "Regla de Negocio"
								not null,
	nroliq		num(9)		descr "Número de liquidación"
								not null,
//								in liquid:descrip,
	intern		num(9)		descr "Nro interno-univoco de usuario (PROD)"
								not null,
	nrovar		num(3)		descr "Número de variable"
								not null,
//								in varia by nrovar(5):descrip,
	valor		num(9)		descr "Valor"
								not null,
	cuid		num(5)		descr "Usuario que creó el registro",
	cdate		date		descr "Día de creación del registro",
	ctime		time		descr "Hora de creación del registro",
	muid		num(5)		descr "Usuario que modificó el registro",
	mdate		date		descr "Día de modificación del registro",
	mtime		time		descr "Hora de modificación del registro",
}
primary key (idpase, codprog, nroliq, intern, nrovar),
index intern (idpase, codprog, intern, nroliq, nrovar);

/*--------------------------------------------------------------------*/

table TMPVARFIJ descr "Variables fijas recurrentes - por producto"
{
	idpase		num(9)		descr "Numero de Pase"
								not null,
	codprog		char(30)	descr "Regla de Negocio"
								not null,
	intern		num(9)		descr "Nro interno-univoco de usuario (PROD)"
								not null,
	nrovar		num(3)		descr "Número de variable"
								not null,
//								in varia by nrovar(2):descrip,
	valor		num(9)		descr "Valor",
	valort		num(9)		descr "Valor Temporario Post-Liq.",
	cuid		num(5)		descr "Usuario que creó el registro",
	cdate		date		descr "Día de creación del registro",
	ctime		time		descr "Hora de creación del registro",
	muid		num(5)		descr "Usuario que modificó el registro",
	mdate		date		descr "Día de modificación del registro",
	mtime		time		descr "Hora de modificación del registro",
}
primary key (idpase, codprog, intern, nrovar);

table TMPWESPO descr "Tabla temporal para el calculo de monto esporadicos"
{
	oi_sesion   char (25)	descr "oi_sesion"
								not null,
	emp			num(2)		descr "Empresa",
	cliente		num(9)		descr "Cliente",
								//in bill.cliente:(razsoc)
	objetivo	num(4)		descr "Objetivo",
	fdesde		date		descr "Fecha desde",
	fhasta		date		descr "Fecha hasta",
	totespo		num(9,2)	descr "Total esporadico",
	totrif		num(9,2)	descr "Total rif",
	totnc		num(9,2)	descr "Total nota de credito",
	cuid		num(5)		descr "Usuario que creó el registro",
	cdate		date		descr "Día de creación del registro",
	ctime		time		descr "Hora de creación del registro",
	muid		num(5)		descr "Usuario que modificó el registro",
	mdate		date		descr "Día de modificación del registro",
	mtime		time		descr "Hora de modificación del registro",
}
primary key (oi_sesion, emp, cliente, objetivo, fdesde, fhasta);

/* -------- WOT -------- */
table wot		descr		"Orden de Trabajo"
{
	oisesion   char (50)	descr "oi_sesion"
								not null,
	emp			num(2)		descr "Empresa"
								//in aurus.emps:(descrip),
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
								//in ncbtes:(descrip),
	serie    	char(15)	descr "Codigo web de serie"
								not null,
								//in nseries(emp, tipcomp):(descrip),
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
	codser		num(2)		descr "Código de Servicio"
								not null,
								//in servicio:(descrip),
	tipser		num(1)		descr "Tipo de Servicio",
								//in (1 : "Rif",
								//	2 : "Esporádico"),
//								default 1,
	estvta		num(1)		descr "Estado de la OT en Operaciones"
								//in (0: "Pendiente",
								//	1: "Aprobó Jefe Vta.",
								//	2: "Rechazó Jefe Vta.")
								default 0,
	estoper		num(1)		descr "Estado de la OT en Operaciones" 
								//in (0: "Pendiente",
								//	1: "Aprobó Operaciones",
								//	2: "Rechazó Operaciones")
								default 0,
	estadm		num(1)		descr "Estado de la OT en Administración"
								//in (0: "Pendiente",
								//	1: "Aprobó Administración",
								//	2: "Rechazó Administración")
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
								//in ("A": "Alta",
								//	"B": "Baja",
								//	"M": "Ampliación",
								//	"R": "Reducción",
								//	"C": "Actualización")
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
	tipcompmt	num(4)		descr "Tipo de Documento de MT",
								//in ncbtes:(descrip),
	seriemt    	char(15)	descr "Codigo web de serie",
								//in nseries(emp, tipcomp):(descrip),
	delegmt		char(3)		descr "Delegación de MT",
	nrootmt		num(9)		descr "Nro. de OT de MT",
	impresa		bool		descr "OT impresa?",
	usucom		char(10)	descr "Usuario de Comercial",
	usuoper		char(10)	descr "Usuario de Operaciones",
	plazo		num(4)		descr "Plazo para montar servicio",
	usuadm		char(10)	descr "Usuario de Administracion",
	origen		num(1)		descr "Origen de la Ot"
								//in (0: "Cargada Manualmente",
								//	1: "Ajuste IPC",
								//	2: "Ajuste por Indice",
								//	3: "Ajuste por Concepto")
								default 0,
	anulado		bool		descr "OT anulada"
								default FALSE,
}
primary key(oisesion, emp, tipcomp, serie, deleg, nroot),
index cliente(oisesion, emp, cliente, objet, codser, nroot);

/* -------- WPTOSER -------- */
table wptoser	descr		"Puesto de Servicio"
{
	oisesion   char (50)	descr "oi_sesion"
								not null,
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie		char(15)	descr "Codigo web de serie"
								not null,
	deleg		char(3)		descr "Delegación"
								not null,	
	nroot		num(9)		descr "Nro. de Novedad"
								not null,
	tippto		num(4)		descr "Cod. de Puesto de Servicio"
								not null,
							    //in tptoser:(descrip),
	cliente		num(9)		descr "Cliente"
								not null,
	objet		num(4)		descr "Objetivo"
								not null,
	cond		char		descr "Tipo de Novedad",
								//in ("A":"Alta",
								//	"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
	mdate		date		descr "Fecha de Modificacion",
	mtime		time		descr "Hora de Modificacion",
	muid		num(5)		descr "Usuario que Modifico",
}
primary key(oisesion, emp, tipcomp, serie, deleg, nroot, tippto),
index cliente(oisesion, emp, cliente, objet, tippto);

/* -------- WNPUESTO -------- */
table wnpuesto	descr		"Puestos de OT"
{
	oisesion	char(50)	descr "oi_sesion"
								not null,
	emp			num(2)		descr "Empresa"
								not null,
	tipcomp		num(4)		descr "Tipo de Documento"
								not null,
	serie    	char(15)	descr "Codigo web de serie"
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
	salario		num(9,2)	descr "Salario",
	cantpue		num(4)		descr "Cantidad de Puesto"
								not null,
	cantvig		num(4,2)	descr "Cantidad de Vigiladores",
	cond		char		descr "Tipo de Novedad",
								//in ("A":"Alta",
								//	"B":"Baja"),
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargó'",
	mdate		date		descr "Fecha de Modificación",
	mtime		time		descr "Hora de Modificación",
	muid		num(5)		descr "Usuario que Modificó",
	concf		num(4)		descr "Concepto de Facturación",
	codfrec		char(2)		descr "Código de Frecuencia para Part Time",
	horapt		num(4)		descr "Horas para Puestos Part Time",
	hsnorm		num(9,2)	descr "Horas Normales Costeo",
	hsextr		num(9,2)	descr "Horas Extras NO se usa",
	hs50		num(6,2)	descr "Horas Extras 50% Costeo",
	hs100		num(6,2)	descr "Horas Extras 100% Costeo",
	subreg		char(8)		descr "SubRegimen",
								//in subregim:descrip,
	tipodia		char(1)		descr "Tipo de dia"
								not null
								default "T"
								mask ">A",
								//in ("F": "Solo Feriados",
								//	"H": "Solo No Feriados",
								//	"T": "Trabaja Siempre"),
	hs_fnorm	num(9,2)	descr "Horas Normales    Facturacion"
								default 0,
	hs_f50		num(6,2)	descr "Horas Extras 50%  Facturacion"
								default 0,
	hs_f100 	num(6,2)	descr "Horas Extras 100% Facturacion"
								default 0,
	cantrvig	num(4,2)	descr "Cantidad Real de Vigiladores",
	codint		num(6)		descr "cod_puesto interno = codint de operac.puestos",
}
primary key(oisesion, emp, tipcomp, serie, deleg, nroot, tippto, nroreng),
index cliente(oisesion, emp, cliente, objet, tippto, puesto);

/* -------- WTARIFA -------- */
table wtarifa	descr		"Tarifa de Horas por Cli/Obj"
{
	codprog		char(30)	descr "Codigo de Programa"
								not null,
	oisesion	char(50)	descr "oi_sesion"
								not null,
	emp			num(2)		descr "Empresa"
								not null,
	cliente		num(9)		descr "Cliente"
								not null,
	objetivo	num(4)		descr "Objetivo"
								not null,
	grutar		char(15)		descr "Grupo Tarifario"
								not null,
	conc		num(4)		descr "Nro. del Concepto"
								not null,
	hortra		num(8,2)	descr "Cantidad de Horas Trabajadas",
	horas		num(8,2)	descr "Cantidad de Horas Facturadas",
	precio		num(9,2)	descr "Precio Unitario",
	cdate		date		descr "Fecha de Carga",
	ctime		time		descr "Hora de Carga",
	cuid		num(5)		descr "Usuario que Cargo",
}
primary key(codprog, oisesion, emp, cliente, objetivo, grutar, conc);

/* -------- WITMXPUE -------- */
table witmxpue	descr		"Items de Factur. por Puestos"
{
	oisesion	char (50)	descr "oi_sesion",
	emp			num(2)		descr "Empresa",
	tipcomp		num(4)		descr "Tipo de Documento",
	serie    	char(15)	descr "Codigo web de serie"
								not null,
	deleg		char(3)		descr "Delegación",
	nroot		num(9)		descr "Nro. de Novedad",
	codint		num(6)		descr "cod_puesto interno = codint de operac.puestos",
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
primary key (oisesion, emp, tipcomp, serie, deleg, nroot, tippto , codint, item),
index puesto(oisesion, emp, tipcomp, serie, deleg, nroot, cliente, objet, tippto, puesto, hinicio, hfinal, dia1, dia2, dia3, dia4, dia5, dia6, dia7, regim, item);


table wper descr "Tabla principal con Datos de los Empleados"
{
	oisesion	char (50)	descr "oi_sesion"
					not null,
	emp		num(3)	descr "Nro de empresa"
					not null,
	nroleg	num(7)	descr "Número de Legajo"
					not null,
	apynom	char(60)	descr "Nombre y Apellido"
					not null,
	direc	char(35)	descr "Calle y Número"
					not null
					default "S/D",
	local	num(5)	descr "Código de Localidad"
					not null
					default 1,
	prov	num(2)	descr "Provincia"
					not null
					default 1,
	telef	[2] char(16)	descr "Nro de Teléfono 1",
	codpost	char(10)	descr "Código postal"
					not null
					mask "10>x"
					default "S/C",
	fecnac	date	descr "Fecha de Nacimiento"
					not null
					default "01011990",
	fecing	date	descr "Fecha de Ingreso"
					not null
					default (today),
	fecegr	date	descr "Fecha de Egreso",
	codnac	num(2)	descr "Nacionalidad"
					not null
					default 7,
	estciv	num(1)	descr "Estado Civil"
					not null
					default 1,
	feccas	date	descr "Fecha de casamiento",
	sexo	num(1)	descr "Sexo"
					not null
					default 1,
	coddoc	char(4)	descr "Tipo de Documento"
					mask "4>a"
					default "DNI",
	nrodoc	num(9)	descr "Número de Documento",
	exped	char(10)	descr "Quién expidió el Documento?",
	relacion	num(2)	descr "Tipo de convenio"
					not null,
	codccos	num(8)	descr "Código de Centro de Costo"
					not null
					default 1,
	codestr	char(12)	descr "Código de Estructura Funcional"
					not null
					default "1",
	codubi	num(6)	descr "Código de Ubicación"
					not null
					default 1,
	codcat	num(4)	descr "Código de Categoría"
					not null
					default 101,
	codcal	num(4)	descr "Cód. de Calificación Profesional"
					not null
					default 1,
	codtar	num(8)	descr "Código de Tarea Realizada"
					not null,
	codest	num(4)	descr "Código de estudios"
					not null
					default 1,
	codtit	num(5)	descr "Código de Titulo",
	hdesde	time	descr "Hora de Ingreso"
					not null
					default "000000",
	hhasta	time	descr "Hora de Egreso",
	nrotarj	num(12)	descr "Número de Tarjeta de Crédito",
	forma	num(1)	descr "Forma de Pago"
					not null
					default 1,
	codsind	num(2)	descr "Cód. de Sindicato",
	nroasind	char(20)	descr "Número de afiliado al Sindicato",
	codos	num(4)	descr "Código de Obra Social",
	nroaos	char(20)	descr "Número de afiliado a la Obra Social",
	codssoc	num(2)	descr "Código de Seguro Social",
	nrossoc	char(20)	descr "Número de Seguro Social",
	codcj	num(2)	descr "Código de A.F.J.P."
					not null
					default 1,
	nroacj	char(20)	descr "Número de Jubilación",
	bank	num(3)	descr "Código de Banco",
	codsuc	num(3)	descr "Código de Sucursal",
	nrocta	char(20)	descr "Número de Cuenta bancaria",
	reservado	num(2)	descr "Nivel de Reservado"
					not null
					default 1,
	activo	num(1)	descr "Activo"
					not null
					default (1)
					in (0:"Inactivo", 1:"Activo", 2:"Egreso Pendiente", 3:"En Ajuste"),
	nroliq	num(6)	descr "Número de Liquidación de acumulador salida",
	nroinsig	char(20)	descr "Número de Inscripción en Imp.a las Gcias.",
	tipserv	char(2)	descr "Tipo de servicio para el ANSeS"
					not null
					default "1",
	usrid	num(5)	descr "Identificación del último que modificó",
	fecha	date	descr "Fecha de última modificación",
	hora	time	descr "Hora de última modificación",
	plan	num(2)	descr "Código de plan",
	vacs	date	descr "Fecha de Vacaciones de salida",
	vace	date	descr "Fecha de Vacaciones de reingreso",
	zonaf	num(2)	descr "Ubicación geográfica del empleado",
	pagf	num(4)	descr "Número de página en guía",
	filx	char(1)	descr "Coordenada x en la guía"
					mask ">A",
	fily	num(1)	descr "Coordenada y en la guía",
	reing	num(7)	descr "Legajo anterior a la incorporación",
	fecpues	date	descr "Fecha desde la cual trabaja en el puesto",
	pasap	char(9)	descr "Número de pasaporte",
	origen	char(10)	descr "origen del pasaporte",
	venc	date	descr "fecha de vencimiento del pasaporte",
	codmegr	num(3)	descr "Código de motivo de egreso",
	codpais	num(2)	descr "Código de Pais",
	acum	num(8)	descr "Acumulador de entrada"
					default (0),
	acums	num(8)	descr "Acumulador de salida",
	dusrid	num(5)	descr "Identificación del último que modificó",
	dfecha	date	descr "Fecha de última modificación",
	dhora	time	descr "Hora de última modificación",
	feccos	date	descr "Fecha desde la cual trabaja en el centro de costo",
	fecestr	date	descr "Fecha desde la cual trabaja en la estructura funcional",
}
primary key	(oisesion, emp, nroleg);

table wdatpers descr "Tabla de datos adicionales del Personal"
{
	oisesion	char (50)	descr "oi_sesion"
					not null,
	emp	num(3)	descr "Número de Empresa"
					not null,
	nroleg	num(7)	descr "Número de Legajo"
					not null,
	apell	char(30)	descr "Apellido",
	nombre	char(30)	descr "Nombres",
	calle	char(35)	descr "Denominación de la Calle",
	nro	char(5)	descr "Número de Puerta",
	piso	char(2)	descr "Piso",
	dpto	char(4)	descr "Departamento",
	paisnac	num(2)	descr "Pais de Nacimiento",
	ciudnac	char(25)	descr "Ciudad de Nacimiento",
	provnac	num(2)	descr "Provincia de Nacimiento",
	fecant	date	descr "Fecha de Antiguedad Reconocida",
	fectran	date	descr "Fecha de Transferencia a otro sistema",
	fecdena	date	descr "Fecha de Ingreso a DENARIUS desde otro sistema",
	cuil	char(13)	descr "C.U.I.L"
					mask "NN-8n-N",
	feinac	date	descr "Fecha de inactividad en el sistema",
	fafafjp	date	descr "Fecha de afiliación a AFJP",
	tipoper	num(2)	descr "Tipo de personal para liquidación de sueldos",
	codactiv	char(8)	descr "Código de Actividad para informe SIJP"
					mask "NN-NN-NN",
	lugpag	num(6)	descr "Lugar de Cobro",
	contrato	num(2)	descr "Tipo de Contrato del empleado",
	nrosol	num(4)	descr "Número de solicitud de ingreso",
	pjorred	num(5,2)	descr "Porcentaje jornada reducida",
	canrenov	num(2)	descr "Cantidad de renovaciones del contrato",
	duracont	num(4)	descr "Duración del contrato",
	jubilado	num(1)	descr "Es Jubilado?"
					default (0)
					in (0:"No", 1:"Sí"),
	presta	num(2)	descr "Código de Prestadora",
	planp	num(2)	descr "Código de Plan de la Prestadora",
	nroapre	char(20)	descr "Número de afiliado en la Prestadora",
	porcpre	num(5,2)	descr "Porcentaje empleado para la Prestadora"
					check (this <= 100),
	email	char(50)	descr "Dirección de E-Mail del Empleado",
	condleg	num(1)	descr "Es una persona discapacitada?"
					not null
					default (0)
					in (0:"No es discapacitado/a", 1:"ES Discapacitado/a   "),
	cat	char(20)	descr "Clave de Alta Temprana",
	fecinic	date	descr "Fecha de Inicio de Relación laboral",
	bankc	num(3)	descr "Código de Banco concentrador para la Acreditación de Haberes",
	codsucc	num(3)	descr "Código de Sucursal concentradora para la Acreditación de Hab",
}
primary key	(oisesion, emp, nroleg);

grant alter on schema webinter to public;
grant all on * to public with grant option;
