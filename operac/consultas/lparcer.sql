set fwidth = 80;
define Impresora as 'printer';

use operac;
select emp "PARTE", cliente, objetivo, dia, confir
from parte
where parte.emp=$1 
and parte.dia between $"3" and $"4"
and parte.cliente between $5 and $7
and parte.confir=0
order by emp, dia, cliente
output to $10;

use operac;
select emp "RETRO", cliente, objetivo, dia, fecreg "Fecha Carga", confir
from retro
where retro.emp=$1 
and retro.dia between $"3" and $"4"
and retro.cliente between $5 and $7
and retro.confir=0
order by emp, dia, cliente
output to $10;

