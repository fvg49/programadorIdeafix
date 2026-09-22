use operac;
set flength=43;
set fwidth=165;
set heading ="[2o[16c$empresa\t Retroactivos\t#D";

select nroleg, dia, fecreg, cliente, objetivo, horaent, horasal,hsnor,hs50,hs100f,hs100fe,condic,
dhsnor "hs. normales", dhs50 "hs. 50%", dhs100f "hs. 100 Franco", dhs100fe "hs.100 Feriado"
From retro
order by nroleg, dia,cliente,objetivo
output to $1;
