use operac, comerc, bill;
select operac.parte.nroleg,
operac.parte.cliente,
bill.cliente.razsoc(0,40),
operac.parte.objetivo,
comerc.objetivo.descrip(0,40),
operac.parte.dia,
operac.parte.condic,
operac.parte.horaent,
operac.parte.horasal,
operac.parte.hsnor,
operac.parte.hs50,
operac.parte.hs100f,
operac.parte.hs100fe,
operac.excepcion.motivo,
operac.excepcion.horas,
operac.excepcion.hs50,
operac.excepcion.hs100
from  operac.parte ,
outer operac.excepcion,
comerc.objetivo, bill.cliente 

where operac.parte.emp = 1 and operac.parte.dia between $"2" and $"3" and
operac.parte.emp = operac.excepcion.emp and operac.excepcion.nroleg = operac.parte.nroleg and
operac.parte.cliente = operac.excepcion.cliente and 
operac.parte.objetivo = operac.excepcion.objetivo and 
operac.parte.dia = operac.excepcion.dia and
operac.parte.cliente = bill.cliente.cliente and  
operac.parte.cliente = comerc.objetivo.cliente and operac.parte.objetivo = comerc.objetivo.objet
output to terminal
;

