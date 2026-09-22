use comerc, bill;
set flength=64;
set fwidth=120;
set heading ="[16c$empresa\tObjetivos por Lugar de Pagos\t#D";
select ofpag "L. Pago",cliente "Cliente",bill.cliente.razsoc(0,35) "Razon Social",
objet "Objetivo", descrip(0,35) "Descrip"
from objetivo, bill.cliente
where cliente = bill.cliente.cliente and 
ofpag between $1 and $3 and activo = true
output to $5;
