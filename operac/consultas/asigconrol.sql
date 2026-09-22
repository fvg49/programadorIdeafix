use operac;
set flength=43;
set fwidth=165;
set heading ="[2o[16c$empresa\t Asignaciones con Rol\t#D";

select emp, cliente, objetivo, ptoser, puesto, nroleg, vigil, efect, regim, fecasig, hsent, hssal, codrol, fila, colum
from asig
where codrol is not null
output to $1;
