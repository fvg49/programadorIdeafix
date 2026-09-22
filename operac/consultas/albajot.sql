use comerc, bill;
set flength=40;
set fwidth=200;
set heading ="[2o[16cempresa\tOT de Altas y Bajas\t";
select o.emp, o.tipcomp, o.serie, o.deleg, o.nroot, o.cliente, cl.razsoc, o.objet, ob.descrip,
       o.finicio, o.ffinal, o.abm, sum(np.cantvig), sum(np.hsnorm + np.hs50 + np.hs100)
from comerc.ot o, comerc.objetivo ob, bill.cliente cl, comerc.npuesto np
where (o.abm = "A" or o.abm = "B")
and ((o.finicio between $"1" and $"2" or o.finicio is null)
and  (o.ffinal  between $"1" and $"2" or o.ffinal  is null))
and o.cliente = cl.cliente
and o.emp = ob.emp and o.cliente = ob.cliente and o.objet = ob.objet 
and o.emp = np.emp
and o.tipcomp = np.tipcomp
and o.serie = np.serie
and o.deleg = np.deleg
and o.nroot = np.nroot
group by o.emp, o.tipcomp, o.serie, o.deleg, o.nroot, o.cliente, cl.razsoc, o.objet, ob.descrip, o.finicio, o.ffinal, o.abm
order by o.emp, o.tipcomp, o.serie, o.deleg, o.nroot
output to $3
;
