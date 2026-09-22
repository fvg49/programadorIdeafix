#!/usr/bin/ksh

. /etc/profile >/dev/null 2>&1
. $HOMEPROF/profile >/dev/null 2>&1 

export SERVERS=$SRV_PROSEGUR_GREAL
#export SERVERS=$SERV_PRUEBA
LOG=$HOME/gparauto_shell.log
LOGERR=$HOME/pas_web_err.log
export LOG

echo "Comienzo de interfaz Prosegur:" >$LOG
echo "Servidores $SERVERS\nUsuario $LOGNAME ">>$LOG 
echo "`hostname`" >>$LOG
date >>$LOG

gparauto.exe 0 2>>$LOGERR

echo "Fin de interfaz Prosegur:" >>$LOG
date >>$LOG
