use comerc,bill;
set flength=64;
set fwidth=80;
set heading ="[16c$empresa\tListado de Clientes por Presentismo\t#D";

select emp "Emp", presen "Svisor",cliente "Cliente",bill.cliente.razsoc(0,20) "Desc. Cliente",
       objet "Obj", descrip(0, 20) "Desc. Objetivo"
from objetivo, bill.cliente
where objetivo.emp=$1
      and presen = $3 and
      cliente = bill.cliente.cliente
output to $5;
;
