:

sleep 10

LOG=`echo "noviuaud.$$.log"` 

printer="F:x.pr"
export printer

echo "Usuario   : $UID $LOGNAME"  >  $LOG 2>&1
echo "Fecha     : `date`"         >> $LOG 2>&1
echo "Servers   : $SERVERS"       >> $LOG 2>&1

wmstop

#------------------------------------------------------------------------------------------------------
#                                           NOVIUAUDI
#------------------------------------------------------------------------------------------------------

echo "Fecha Inicio noviuaud.exe 1 : `date`"         >> $LOG 2>&1
wm -r wmaudi1.txt  noviuaud.exe
echo "Fecha FIN    noviuaud.exe 1 : `date`"         >> $LOG 2>&1
sleep 5

echo "Fecha Inicio noviuaud.exe 3 : `date`"         >> $LOG 2>&1
wm -r wmaudi3.txt  noviuaud.exe
echo "Fecha FIN    noviuaud.exe 3 : `date`"         >> $LOG 2>&1
sleep 5

echo "Fecha Inicio noviuaud.exe 5 : `date`"         >> $LOG 2>&1
wm -r wmaudi5.txt  noviuaud.exe
echo "Fecha FIN    noviuaud.exe 5 : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           CTRLPAR1
#------------------------------------------------------------------------------------------------------


echo "Fecha Inicio ctrlpar1.exe  : `date`"         >> $LOG 2>&1
wm -r wmaudi6.txt  ctrlpar1.exe
echo "Fecha FIN    ctrlpar1.exe  : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           CTRLPAR2
#------------------------------------------------------------------------------------------------------


echo "Fecha Inicio ctrlpar2.exe  : `date`"         >> $LOG 2>&1
wm -r wmaudi7.txt  ctrlpar2.exe
echo "Fecha FIN    ctrlpar2.exe  : `date`"         >> $LOG 2>&1
sleep 5


#------------------------------------------------------------------------------------------------------
#                                           LNOASIG
#------------------------------------------------------------------------------------------------------

echo "Fecha Inicio lnoasig.exe : `date`"         >> $LOG 2>&1
wm -r wmaudi8.txt lnoasig.exe
grep "							" audi8.txt >audi8b.txt
 
echo "Fecha FIN    lnoasig.exe : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           LDUPLICA
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio lduplica.exe : `date`"         >> $LOG 2>&1
wm -r wmaudi9.txt  lduplica.exe 2>dupl_borrar.txt
grep DET_HSTOPE audi9.txt > audi9b.txt
grep HS_TOPE audi9.txt > audi9c.txt
grep DUPLICA audi9.txt > audi9d.txt
grep SUPERPOSICION audi9.txt > audi9e.txt
grep EXCEPCIONES audi9.txt > audi9f.txt
grep HOR_ audi9.txt > audi9g.txt
grep PARTE_SIN_CONFIR  audi9.txt > audi9h.txt
grep SIN_REGISTRAR  audi9.txt > audi9i.txt
grep ASIGH_SIN_PARTE audi9.txt > audi9j.txt 
grep SUPERA_LO_CONFORMADO audi9.txt > audi12.txt 
grep FECHA_EGRESO_SUPERADA audi9.txt > audi13.txt

echo "Fecha FIN    lduplica.exe : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           PTOVEN
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio ptoven.exe : `date`"         >> $LOG 2>&1
wm -r wmaudi11.txt  ptoven.exe 
echo "Fecha FIN    ptoven.exe : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           LOBJET
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio lobjet.exe : `date`"         >> $LOG 2>&1
wm -r wmobjet.txt lobjet.exe
echo "Fecha FIN    lobjet.exe : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           AUDRET
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio audret.exe  : `date`"         >> $LOG 2>&1
wm -r wmaudret.txt  audret.exe 
echo "Fecha FIN    audret.exe  : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           HSCLIOBJ
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio hscliobj.exe : `date`"         >> $LOG 2>&1
wm -r wmhscli.txt hscliobj.exe 
echo "Fecha FIN    hscliobj.exe : `date`"         >> $LOG 2>&1
sleep 5

#------------------------------------------------------------------------------------------------------
#                                           HSVIGIL
#------------------------------------------------------------------------------------------------------
echo "Fecha Inicio hsvigil : `date`"         >> $LOG 2>&1
wm -r wmhsvig.txt hsvigil.exe    
grep DELEGACION hsvigil.txt > hsvigilb.txt
grep "Total Vigilador" hsvigil.txt > hsvigilc.txt

echo "Fecha FIN    hsvigil.exe : `date`"         >> $LOG 2>&1
sleep 5



