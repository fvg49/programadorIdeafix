/* SCHEMA: operac Esquema de operaciones	*/


/* TABLE: FERIADO 	RECORD LENGTH: 62	*/

# define FERIADO         (dbtable) 0x0100
# define FERIADO_FECHA (dbfield) 0x0101	/* Date */
# define FERIADO_DESCRIP (dbfield) 0x0102	/* String */
# define FERIADO_PAIS (dbfield) 0x0103	/* Integer */
# define FERIADO_PROV (dbfield) 0x0104	/* Integer */
# define FERIADObyFECHA (dbindex) 0x0100
# define FERIADObyPAIS (dbindex) 0x0101


/* TABLE: MOTIVD 	RECORD LENGTH: 78	*/

# define MOTIVD          (dbtable) 0x0200
# define MOTIVD_CODMOTD (dbfield) 0x0201	/* Integer */
# define MOTIVD_DESCRIP (dbfield) 0x0202	/* String */
# define MOTIVD_DESCOR (dbfield) 0x0203	/* String */
# define MOTIVD_M_INC (dbfield) 0x0204	/* Integer */
# define MOTIVD_ROTACION (dbfield) 0x0205	/* Integer */
# define MOTIVD_ACTIVO (dbfield) 0x0206	/* Integer */
# define MOTIVDbyCODMOTD (dbindex) 0x0200
# define MOTIVDbyACTIVO (dbindex) 0x0201


/* TABLE: ASIG 	RECORD LENGTH: 125	*/

# define ASIG            (dbtable) 0x0300
# define ASIG_EMP (dbfield) 0x0301	/* Integer */
# define ASIG_CLIENTE (dbfield) 0x0302	/* Long */
# define ASIG_OBJETIVO (dbfield) 0x0303	/* Integer */
# define ASIG_PTOSER (dbfield) 0x0304	/* Integer */
# define ASIG_PUESTO (dbfield) 0x0305	/* Integer */
# define ASIG_NROLEG (dbfield) 0x0306	/* Long */
# define ASIG_VIGIL (dbfield) 0x0307	/* String */
# define ASIG_EFECT (dbfield) 0x0308	/* String */
# define ASIG_FECASIG (dbfield) 0x0309	/* Date */
# define ASIG_HSENT (dbfield) 0x030a	/* Time */
# define ASIG_HSSAL (dbfield) 0x030b	/* Time */
# define ASIG_DIA1 (dbfield) 0x030c	/* String */
# define ASIG_DIA2 (dbfield) 0x030d	/* String */
# define ASIG_DIA3 (dbfield) 0x030e	/* String */
# define ASIG_DIA4 (dbfield) 0x030f	/* String */
# define ASIG_DIA5 (dbfield) 0x0310	/* String */
# define ASIG_DIA6 (dbfield) 0x0311	/* String */
# define ASIG_DIA7 (dbfield) 0x0312	/* String */
# define ASIG_REEMPL (dbfield) 0x0313	/* Long */
# define ASIG_FFRANCO (dbfield) 0x0314	/* Date */
# define ASIG_REGIM (dbfield) 0x0315	/* String */
# define ASIG_FECHAS (dbfield) 0x0316	/* Date */
# define ASIG_FECBAJ (dbfield) 0x0317	/* Date */
# define ASIG_NUMFRAN (dbfield) 0x0318	/* Integer */
# define ASIG_FRANCERO (dbfield) 0x0319	/* Integer */
# define ASIG_CDATE (dbfield) 0x031a	/* Date */
# define ASIG_CTIME (dbfield) 0x031b	/* Time */
# define ASIG_CUID (dbfield) 0x031c	/* Long */
# define ASIG_MDATE (dbfield) 0x031d	/* Date */
# define ASIG_MTIME (dbfield) 0x031e	/* Time */
# define ASIG_MUID (dbfield) 0x031f	/* Long */
# define ASIG_NROINT (dbfield) 0x0320	/* Integer */
# define ASIG_TIPODIA (dbfield) 0x0321	/* String */
# define ASIG_CODROL (dbfield) 0x0322	/* Integer */
# define ASIG_FILA (dbfield) 0x0323	/* Integer */
# define ASIG_COLUM (dbfield) 0x0324	/* Integer */
# define ASIG_REGPTO (dbfield) 0x0325	/* String */
# define ASIGbyEMP (dbindex) 0x0300
# define ASIGbyFECHA (dbindex) 0x0301
# define ASIGbyNROLEG (dbindex) 0x0302
# define ASIGbyLEGFEC (dbindex) 0x0303
# define ASIGbyPUESTO (dbindex) 0x0304


/* TABLE: ASIGH 	RECORD LENGTH: 126	*/

# define ASIGH           (dbtable) 0x0400
# define ASIGH_EMP (dbfield) 0x0401	/* Integer */
# define ASIGH_CLIENTE (dbfield) 0x0402	/* Long */
# define ASIGH_OBJETIVO (dbfield) 0x0403	/* Integer */
# define ASIGH_PTOSER (dbfield) 0x0404	/* Integer */
# define ASIGH_PUESTO (dbfield) 0x0405	/* Integer */
# define ASIGH_NROLEG (dbfield) 0x0406	/* Long */
# define ASIGH_VIGIL (dbfield) 0x0407	/* String */
# define ASIGH_EFECT (dbfield) 0x0408	/* String */
# define ASIGH_FECALT (dbfield) 0x0409	/* Date */
# define ASIGH_FECBAJ (dbfield) 0x040a	/* Date */
# define ASIGH_HSENT (dbfield) 0x040b	/* Time */
# define ASIGH_HSSAL (dbfield) 0x040c	/* Time */
# define ASIGH_DIA1 (dbfield) 0x040d	/* String */
# define ASIGH_DIA2 (dbfield) 0x040e	/* String */
# define ASIGH_DIA3 (dbfield) 0x040f	/* String */
# define ASIGH_DIA4 (dbfield) 0x0410	/* String */
# define ASIGH_DIA5 (dbfield) 0x0411	/* String */
# define ASIGH_DIA6 (dbfield) 0x0412	/* String */
# define ASIGH_DIA7 (dbfield) 0x0413	/* String */
# define ASIGH_MOTIVO (dbfield) 0x0414	/* Integer */
# define ASIGH_REEMPL (dbfield) 0x0415	/* Long */
# define ASIGH_REGIM (dbfield) 0x0416	/* String */
# define ASIGH_FECHAS (dbfield) 0x0417	/* Date */
# define ASIGH_CDATE (dbfield) 0x0418	/* Date */
# define ASIGH_CTIME (dbfield) 0x0419	/* Time */
# define ASIGH_CUID (dbfield) 0x041a	/* Long */
# define ASIGH_MDATE (dbfield) 0x041b	/* Date */
# define ASIGH_MTIME (dbfield) 0x041c	/* Time */
# define ASIGH_MUID (dbfield) 0x041d	/* Long */
# define ASIGH_FFRANCO (dbfield) 0x041e	/* Date */
# define ASIGH_NUMFRAN (dbfield) 0x041f	/* Integer */
# define ASIGH_FRANCERO (dbfield) 0x0420	/* Integer */
# define ASIGH_NROINT (dbfield) 0x0421	/* Integer */
# define ASIGH_TIPODIA (dbfield) 0x0422	/* String */
# define ASIGH_CODROL (dbfield) 0x0423	/* Integer */
# define ASIGH_FILA (dbfield) 0x0424	/* Integer */
# define ASIGH_COLUM (dbfield) 0x0425	/* Integer */
# define ASIGH_REGPTO (dbfield) 0x0426	/* String */
# define ASIGHbyEMP (dbindex) 0x0400
# define ASIGHbyPUESTO (dbindex) 0x0401
# define ASIGHbyNROLEG (dbindex) 0x0402
# define ASIGHbyFECHABAJ (dbindex) 0x0403
# define ASIGHbyLEGFEC (dbindex) 0x0404
# define ASIGHbyFECHAALT (dbindex) 0x0405


/* TABLE: PARTE 	RECORD LENGTH: 140	*/

# define PARTE           (dbtable) 0x0500
# define PARTE_EMP (dbfield) 0x0501	/* Integer */
# define PARTE_CLIENTE (dbfield) 0x0502	/* Long */
# define PARTE_OBJETIVO (dbfield) 0x0503	/* Integer */
# define PARTE_DIA (dbfield) 0x0504	/* Date */
# define PARTE_NROLEG (dbfield) 0x0505	/* Long */
# define PARTE_HORAENT (dbfield) 0x0506	/* Time */
# define PARTE_HORASAL (dbfield) 0x0507	/* Time */
# define PARTE_CONFIR (dbfield) 0x0508	/* Integer */
# define PARTE_HSNOR (dbfield) 0x0509	/* Integer */
# define PARTE_HS50 (dbfield) 0x050a	/* Integer */
# define PARTE_HS100F (dbfield) 0x050b	/* Integer */
# define PARTE_HS100FE (dbfield) 0x050c	/* Integer */
# define PARTE_CONDIC (dbfield) 0x050d	/* String */
# define PARTE_CDATE (dbfield) 0x050e	/* Date */
# define PARTE_CTIME (dbfield) 0x050f	/* Time */
# define PARTE_CUID (dbfield) 0x0510	/* Long */
# define PARTE_MDATE (dbfield) 0x0511	/* Date */
# define PARTE_MTIME (dbfield) 0x0512	/* Time */
# define PARTE_MUID (dbfield) 0x0513	/* Long */
# define PARTE_CONFEX (dbfield) 0x0514	/* Integer */
# define PARTE_PTOSER (dbfield) 0x0515	/* Integer */
# define PARTE_PUESTO (dbfield) 0x0516	/* Integer */
# define PARTE_FECGEN (dbfield) 0x0517	/* Date */
# define PARTE_NROINT (dbfield) 0x0518	/* Integer */
# define PARTE_CODAUS (dbfield) 0x0519	/* Integer */
# define PARTE_LIQDENA (dbfield) 0x051a	/* Long */
# define PARTE_LIQFAC (dbfield) 0x051b	/* Long */
# define PARTE_ASICBLE (dbfield) 0x051c	/* Long */
# define PARTE_NROFAC (dbfield) 0x051d	/* Long */
# define PARTE_LIQDENUS (dbfield) 0x051e	/* Long */
# define PARTE_ASICBLUS (dbfield) 0x051f	/* Long */
# define PARTE_HPROG (dbfield) 0x0520	/* String */
# define PARTE_HDATE (dbfield) 0x0521	/* Date */
# define PARTE_HTIME (dbfield) 0x0522	/* Time */
# define PARTE_HUID (dbfield) 0x0523	/* Long */
# define PARTEbyEMP (dbindex) 0x0500
# define PARTEbyLEG (dbindex) 0x0501
# define PARTEbyEMPLE (dbindex) 0x0502
# define PARTEbyPUESTO (dbindex) 0x0503
# define PARTEbyDIA (dbindex) 0x0504
# define PARTEbyRPUESTO (dbindex) 0x0505


/* TABLE: CONDICION 	RECORD LENGTH: 75	*/

# define CONDICION       (dbtable) 0x0600
# define CONDICION_CODCOND (dbfield) 0x0601	/* Integer */
# define CONDICION_DESCRIP (dbfield) 0x0602	/* String */
# define CONDICION_DESCOR (dbfield) 0x0603	/* String */
# define CONDICIONbyCODCOND (dbindex) 0x0600


/* TABLE: MOTEXC 	RECORD LENGTH: 64	*/

# define MOTEXC          (dbtable) 0x0700
# define MOTEXC_CODCOND (dbfield) 0x0701	/* Integer */
# define MOTEXC_CODMOT (dbfield) 0x0702	/* Integer */
# define MOTEXC_DESCRIP (dbfield) 0x0703	/* String */
# define MOTEXC_VALCLI (dbfield) 0x0704	/* Integer */
# define MOTEXC_VEMP (dbfield) 0x0705	/* Integer */
# define MOTEXC_TIPMOT (dbfield) 0x0706	/* Integer */
# define MOTEXCbyCODCOND (dbindex) 0x0700


/* TABLE: EXCEPCION 	RECORD LENGTH: 126	*/

# define EXCEPCION       (dbtable) 0x0800
# define EXCEPCION_EMP (dbfield) 0x0801	/* Integer */
# define EXCEPCION_CLIENTE (dbfield) 0x0802	/* Long */
# define EXCEPCION_OBJETIVO (dbfield) 0x0803	/* Integer */
# define EXCEPCION_DIA (dbfield) 0x0804	/* Date */
# define EXCEPCION_NROLEG (dbfield) 0x0805	/* Long */
# define EXCEPCION_CONDIC (dbfield) 0x0806	/* Integer */
# define EXCEPCION_MOTIVO (dbfield) 0x0807	/* Integer */
# define EXCEPCION_HORAS (dbfield) 0x0808	/* Integer */
# define EXCEPCION_HS50 (dbfield) 0x0809	/* Integer */
# define EXCEPCION_HS100 (dbfield) 0x080a	/* Integer */
# define EXCEPCION_OBS (dbfield) 0x080b	/* String */
# define EXCEPCION_CDATE (dbfield) 0x080c	/* Date */
# define EXCEPCION_CTIME (dbfield) 0x080d	/* Time */
# define EXCEPCION_CUID (dbfield) 0x080e	/* Long */
# define EXCEPCION_MDATE (dbfield) 0x080f	/* Date */
# define EXCEPCION_MTIME (dbfield) 0x0810	/* Time */
# define EXCEPCION_MUID (dbfield) 0x0811	/* Long */
# define EXCEPCION_PTOSER (dbfield) 0x0812	/* Integer */
# define EXCEPCION_PUESTO (dbfield) 0x0813	/* Integer */
# define EXCEPCION_NROINT (dbfield) 0x0814	/* Integer */
# define EXCEPCIONbyEMP (dbindex) 0x0800
# define EXCEPCIONbyLEGAJO (dbindex) 0x0801


/* TABLE: PUESTOS 	RECORD LENGTH: 127	*/

# define PUESTOS         (dbtable) 0x0900
# define PUESTOS_CLIENTE (dbfield) 0x0901	/* Long */
# define PUESTOS_OBJET (dbfield) 0x0902	/* Integer */
# define PUESTOS_TIPPTO (dbfield) 0x0903	/* Integer */
# define PUESTOS_PUESTO (dbfield) 0x0904	/* Integer */
# define PUESTOS_HINICIO (dbfield) 0x0905	/* Time */
# define PUESTOS_HFINAL (dbfield) 0x0906	/* Time */
# define PUESTOS_DIA1 (dbfield) 0x0907	/* String */
# define PUESTOS_DIA2 (dbfield) 0x0908	/* String */
# define PUESTOS_DIA3 (dbfield) 0x0909	/* String */
# define PUESTOS_DIA4 (dbfield) 0x090a	/* String */
# define PUESTOS_DIA5 (dbfield) 0x090b	/* String */
# define PUESTOS_DIA6 (dbfield) 0x090c	/* String */
# define PUESTOS_DIA7 (dbfield) 0x090d	/* String */
# define PUESTOS_REGIM (dbfield) 0x090e	/* String */
# define PUESTOS_CODINT (dbfield) 0x090f	/* Integer */
# define PUESTOS_CANTVIG (dbfield) 0x0910	/* Integer */
# define PUESTOS_VIGI (dbfield) 0x0911	/* Integer */
# define PUESTOS_CDATE (dbfield) 0x0912	/* Date */
# define PUESTOS_CTIME (dbfield) 0x0913	/* Time */
# define PUESTOS_CUID (dbfield) 0x0914	/* Long */
# define PUESTOS_MDATE (dbfield) 0x0915	/* Date */
# define PUESTOS_MTIME (dbfield) 0x0916	/* Time */
# define PUESTOS_MUID (dbfield) 0x0917	/* Long */
# define PUESTOS_CODFREC (dbfield) 0x0918	/* String */
# define PUESTOS_HORAPT (dbfield) 0x0919	/* Integer */
# define PUESTOS_FINICIO (dbfield) 0x091a	/* Date */
# define PUESTOS_FFINAL (dbfield) 0x091b	/* Date */
# define PUESTOS_HSNORM (dbfield) 0x091c	/* Long */
# define PUESTOS_HSEXTR (dbfield) 0x091d	/* Long */
# define PUESTOS_CANTPUE (dbfield) 0x091e	/* Integer */
# define PUESTOS_TIPODIA (dbfield) 0x091f	/* String */
# define PUESTOS_NEWINT (dbfield) 0x0920	/* Integer */
# define PUESTOS_PADREINT (dbfield) 0x0921	/* Integer */
# define PUESTOS_CODMOT (dbfield) 0x0922	/* Integer */
# define PUESTOS_SUBREG (dbfield) 0x0923	/* String */
# define PUESTOSbyCLIENTE (dbindex) 0x0900
# define PUESTOSbyPUESTO (dbindex) 0x0901
# define PUESTOSbyPADRE (dbindex) 0x0902


/* TABLE: VACAC 	RECORD LENGTH: 66	*/

# define VACAC           (dbtable) 0x0a00
# define VACAC_EMP (dbfield) 0x0a01	/* Integer */
# define VACAC_NROLEG (dbfield) 0x0a02	/* Long */
# define VACAC_CANTDIAS (dbfield) 0x0a03	/* Integer */
# define VACAC_MODULO (dbfield) 0x0a04	/* Integer */
# define VACAC_FDESDE (dbfield) 0x0a05	/* Date */
# define VACAC_FHASTA (dbfield) 0x0a06	/* Date */
# define VACAC_FPASE (dbfield) 0x0a07	/* Date */
# define VACAC_PERIODO (dbfield) 0x0a08	/* Integer */
# define VACAC_FECREG (dbfield) 0x0a09	/* Date */
# define VACAC_CDATE (dbfield) 0x0a0a	/* Date */
# define VACAC_CTIME (dbfield) 0x0a0b	/* Time */
# define VACAC_CUID (dbfield) 0x0a0c	/* Long */
# define VACAC_MDATE (dbfield) 0x0a0d	/* Date */
# define VACAC_MTIME (dbfield) 0x0a0e	/* Time */
# define VACAC_MUID (dbfield) 0x0a0f	/* Long */
# define VACACbyEMP (dbindex) 0x0a00
# define VACACbyPERIODO (dbindex) 0x0a01
# define VACACbyFECHA (dbindex) 0x0a02


/* TABLE: PERIODOS 	RECORD LENGTH: 39	*/

# define PERIODOS        (dbtable) 0x0b00
# define PERIODOS_EMP (dbfield) 0x0b01	/* Integer */
# define PERIODOS_MESCOMPN (dbfield) 0x0b02	/* Integer */
# define PERIODOS_DDESNOR (dbfield) 0x0b03	/* Integer */
# define PERIODOS_DHASNOR (dbfield) 0x0b04	/* Integer */
# define PERIODOS_MESCOMPE (dbfield) 0x0b05	/* Integer */
# define PERIODOS_DDESEX (dbfield) 0x0b06	/* Integer */
# define PERIODOS_DHASEX (dbfield) 0x0b07	/* Integer */
# define PERIODOSbyEMP (dbindex) 0x0b00


/* TABLE: CIERRE 	RECORD LENGTH: 75	*/

# define CIERRE          (dbtable) 0x0c00
# define CIERRE_EMP (dbfield) 0x0c01	/* Integer */
# define CIERRE_NROCIER (dbfield) 0x0c02	/* Integer */
# define CIERRE_MESC (dbfield) 0x0c03	/* Integer */
# define CIERRE_ANIOC (dbfield) 0x0c04	/* Integer */
# define CIERRE_FDESNOR (dbfield) 0x0c05	/* Date */
# define CIERRE_FHASNOR (dbfield) 0x0c06	/* Date */
# define CIERRE_FDESEX (dbfield) 0x0c07	/* Date */
# define CIERRE_FHASEX (dbfield) 0x0c08	/* Date */
# define CIERRE_FECCIE (dbfield) 0x0c09	/* Date */
# define CIERRE_HORACIE (dbfield) 0x0c0a	/* Time */
# define CIERRE_USUCIE (dbfield) 0x0c0b	/* Long */
# define CIERRE_FECIMP (dbfield) 0x0c0c	/* Date */
# define CIERRE_HORAIMP (dbfield) 0x0c0d	/* Time */
# define CIERRE_USUIMP (dbfield) 0x0c0e	/* Long */
# define CIERRE_TIPOCIER (dbfield) 0x0c0f	/* Integer */
# define CIERRE_FECIMPA (dbfield) 0x0c10	/* Date */
# define CIERRE_HORAIMPA (dbfield) 0x0c11	/* Time */
# define CIERRE_USUIMPA (dbfield) 0x0c12	/* Long */
# define CIERRE_FDESRET (dbfield) 0x0c13	/* Date */
# define CIERRE_FHASRET (dbfield) 0x0c14	/* Date */
# define CIERREbyEMP (dbindex) 0x0c00
# define CIERREbyFECHA (dbindex) 0x0c01


/* TABLE: FRECUEN 	RECORD LENGTH: 56	*/

# define FRECUEN         (dbtable) 0x0d00
# define FRECUEN_CODFREC (dbfield) 0x0d01	/* String */
# define FRECUEN_DESCRIP (dbfield) 0x0d02	/* String */
# define FRECUENbyCODFREC (dbindex) 0x0d00


/* TABLE: DIASPTIME 	RECORD LENGTH: 71	*/

# define DIASPTIME       (dbtable) 0x0e00
# define DIASPTIME_EMP (dbfield) 0x0e01	/* Integer */
# define DIASPTIME_CLIENTE (dbfield) 0x0e02	/* Long */
# define DIASPTIME_OBJETIVO (dbfield) 0x0e03	/* Integer */
# define DIASPTIME_NROLEG (dbfield) 0x0e04	/* Long */
# define DIASPTIME_DIA (dbfield) 0x0e05	/* Date */
# define DIASPTIME_HENT (dbfield) 0x0e06	/* Time */
# define DIASPTIME_HSAL (dbfield) 0x0e07	/* Time */
# define DIASPTIME_CDATE (dbfield) 0x0e08	/* Date */
# define DIASPTIME_CTIME (dbfield) 0x0e09	/* Time */
# define DIASPTIME_CUID (dbfield) 0x0e0a	/* Long */
# define DIASPTIME_MDATE (dbfield) 0x0e0b	/* Date */
# define DIASPTIME_MTIME (dbfield) 0x0e0c	/* Time */
# define DIASPTIME_MUID (dbfield) 0x0e0d	/* Long */
# define DIASPTIME_TIPPTO (dbfield) 0x0e0e	/* Integer */
# define DIASPTIME_PUESTO (dbfield) 0x0e0f	/* Integer */
# define DIASPTIME_NROINT (dbfield) 0x0e10	/* Integer */
# define DIASPTIMEbyEMP (dbindex) 0x0e00
# define DIASPTIMEbyDIA (dbindex) 0x0e01


/* TABLE: DIASPTIMEH 	RECORD LENGTH: 71	*/

# define DIASPTIMEH      (dbtable) 0x0f00
# define DIASPTIMEH_EMP (dbfield) 0x0f01	/* Integer */
# define DIASPTIMEH_CLIENTE (dbfield) 0x0f02	/* Long */
# define DIASPTIMEH_OBJETIVO (dbfield) 0x0f03	/* Integer */
# define DIASPTIMEH_NROLEG (dbfield) 0x0f04	/* Long */
# define DIASPTIMEH_DIA (dbfield) 0x0f05	/* Date */
# define DIASPTIMEH_HENT (dbfield) 0x0f06	/* Time */
# define DIASPTIMEH_HSAL (dbfield) 0x0f07	/* Time */
# define DIASPTIMEH_CDATE (dbfield) 0x0f08	/* Date */
# define DIASPTIMEH_CTIME (dbfield) 0x0f09	/* Time */
# define DIASPTIMEH_CUID (dbfield) 0x0f0a	/* Long */
# define DIASPTIMEH_MDATE (dbfield) 0x0f0b	/* Date */
# define DIASPTIMEH_MTIME (dbfield) 0x0f0c	/* Time */
# define DIASPTIMEH_MUID (dbfield) 0x0f0d	/* Long */
# define DIASPTIMEH_TIPPTO (dbfield) 0x0f0e	/* Integer */
# define DIASPTIMEH_PUESTO (dbfield) 0x0f0f	/* Integer */
# define DIASPTIMEH_NROINT (dbfield) 0x0f10	/* Integer */
# define DIASPTIMEHbyEMP (dbindex) 0x0f00
# define DIASPTIMEHbyDIA (dbindex) 0x0f01


/* TABLE: RETRO 	RECORD LENGTH: 120	*/

# define RETRO           (dbtable) 0x1000
# define RETRO_EMP (dbfield) 0x1001	/* Integer */
# define RETRO_CLIENTE (dbfield) 0x1002	/* Long */
# define RETRO_OBJETIVO (dbfield) 0x1003	/* Integer */
# define RETRO_DIA (dbfield) 0x1004	/* Date */
# define RETRO_FECREG (dbfield) 0x1005	/* Date */
# define RETRO_NROLEG (dbfield) 0x1006	/* Long */
# define RETRO_HORAENT (dbfield) 0x1007	/* Time */
# define RETRO_HORASAL (dbfield) 0x1008	/* Time */
# define RETRO_CONFIR (dbfield) 0x1009	/* Integer */
# define RETRO_HSNOR (dbfield) 0x100a	/* Integer */
# define RETRO_HS50 (dbfield) 0x100b	/* Integer */
# define RETRO_HS100F (dbfield) 0x100c	/* Integer */
# define RETRO_HS100FE (dbfield) 0x100d	/* Integer */
# define RETRO_CONDIC (dbfield) 0x100e	/* String */
# define RETRO_CONFEX (dbfield) 0x100f	/* Integer */
# define RETRO_PTOSER (dbfield) 0x1010	/* Integer */
# define RETRO_PUESTO (dbfield) 0x1011	/* Integer */
# define RETRO_NROINT (dbfield) 0x1012	/* Integer */
# define RETRO_CDATE (dbfield) 0x1013	/* Date */
# define RETRO_CTIME (dbfield) 0x1014	/* Time */
# define RETRO_CUID (dbfield) 0x1015	/* Long */
# define RETRO_MDATE (dbfield) 0x1016	/* Date */
# define RETRO_MTIME (dbfield) 0x1017	/* Time */
# define RETRO_MUID (dbfield) 0x1018	/* Long */
# define RETRO_CODAUS (dbfield) 0x1019	/* Integer */
# define RETRO_DHSNOR (dbfield) 0x101a	/* Integer */
# define RETRO_DHS50 (dbfield) 0x101b	/* Integer */
# define RETRO_DHS100F (dbfield) 0x101c	/* Integer */
# define RETRO_DHS100FE (dbfield) 0x101d	/* Integer */
# define RETRO_LIQDENA (dbfield) 0x101e	/* Long */
# define RETRO_LIQFAC (dbfield) 0x101f	/* Long */
# define RETRO_ASICBLE (dbfield) 0x1020	/* Long */
# define RETRO_NROFAC (dbfield) 0x1021	/* Long */
# define RETRO_LIQDENUS (dbfield) 0x1022	/* Long */
# define RETRO_ASICBLUS (dbfield) 0x1023	/* Long */
# define RETRO_ORIGEN (dbfield) 0x1024	/* Integer */
# define RETRObyEMP (dbindex) 0x1000
# define RETRObyRLEG (dbindex) 0x1001
# define RETRObyREMPLE (dbindex) 0x1002
# define RETRObyRDIA (dbindex) 0x1003
# define RETRObyRFECREG (dbindex) 0x1004


/* TABLE: RETROEXC 	RECORD LENGTH: 132	*/

# define RETROEXC        (dbtable) 0x1100
# define RETROEXC_EMP (dbfield) 0x1101	/* Integer */
# define RETROEXC_CLIENTE (dbfield) 0x1102	/* Long */
# define RETROEXC_OBJETIVO (dbfield) 0x1103	/* Integer */
# define RETROEXC_DIA (dbfield) 0x1104	/* Date */
# define RETROEXC_NROLEG (dbfield) 0x1105	/* Long */
# define RETROEXC_CONDIC (dbfield) 0x1106	/* Integer */
# define RETROEXC_MOTIVO (dbfield) 0x1107	/* Integer */
# define RETROEXC_HORAS (dbfield) 0x1108	/* Integer */
# define RETROEXC_HS50 (dbfield) 0x1109	/* Integer */
# define RETROEXC_HS100 (dbfield) 0x110a	/* Integer */
# define RETROEXC_OBS (dbfield) 0x110b	/* String */
# define RETROEXC_CDATE (dbfield) 0x110c	/* Date */
# define RETROEXC_CTIME (dbfield) 0x110d	/* Time */
# define RETROEXC_CUID (dbfield) 0x110e	/* Long */
# define RETROEXC_MDATE (dbfield) 0x110f	/* Date */
# define RETROEXC_MTIME (dbfield) 0x1110	/* Time */
# define RETROEXC_MUID (dbfield) 0x1111	/* Long */
# define RETROEXC_PTOSER (dbfield) 0x1112	/* Integer */
# define RETROEXC_PUESTO (dbfield) 0x1113	/* Integer */
# define RETROEXC_NROINT (dbfield) 0x1114	/* Integer */
# define RETROEXC_DHORAS (dbfield) 0x1115	/* Integer */
# define RETROEXC_DHS50 (dbfield) 0x1116	/* Integer */
# define RETROEXC_DHS100 (dbfield) 0x1117	/* Integer */
# define RETROEXCbyEMP (dbindex) 0x1100
# define RETROEXCbyLEGAJO (dbindex) 0x1101


/* TABLE: ROL 	RECORD LENGTH: 61	*/

# define ROL             (dbtable) 0x1200
# define ROL_CODROL (dbfield) 0x1201	/* Integer */
# define ROL_DESCRIP (dbfield) 0x1202	/* String */
# define ROL_TIPROL (dbfield) 0x1203	/* Integer */
# define ROLbyCODROL (dbindex) 0x1200


/* TABLE: RROL 	RECORD LENGTH: 38	*/

# define RROL            (dbtable) 0x1300
# define RROL_CODROL (dbfield) 0x1301	/* Integer */
# define RROL_FILA (dbfield) 0x1302	/* Integer */
# define RROL_COLUM (dbfield) 0x1303	/* Integer */
# define RROL_VALOR (dbfield) 0x1304	/* String */
# define RROLbyCODROL (dbindex) 0x1300


/* TABLE: CIEFIL 	RECORD LENGTH: 58	*/

# define CIEFIL          (dbtable) 0x1400
# define CIEFIL_FILIAL (dbfield) 0x1401	/* String */
# define CIEFIL_FECCIE (dbfield) 0x1402	/* Date */
# define CIEFIL_IDCARGA (dbfield) 0x1403	/* Long */
# define CIEFIL_FECCARGA (dbfield) 0x1404	/* Date */
# define CIEFIL_HORACARGA (dbfield) 0x1405	/* Time */
# define CIEFIL_REVERTIDO (dbfield) 0x1406	/* Integer */
# define CIEFIL_IDREVER (dbfield) 0x1407	/* Long */
# define CIEFIL_FECREVER (dbfield) 0x1408	/* Date */
# define CIEFIL_HORAREVER (dbfield) 0x1409	/* Time */
# define CIEFILbyFILIAL (dbindex) 0x1400
# define CIEFILbyACTIVA (dbindex) 0x1401


/* TABLE: TPERMI 	RECORD LENGTH: 81	*/

# define TPERMI          (dbtable) 0x1500
# define TPERMI_EMP (dbfield) 0x1501	/* Integer */
# define TPERMI_TIPPER (dbfield) 0x1502	/* Integer */
# define TPERMI_DESCRIP (dbfield) 0x1503	/* String */
# define TPERMI_DESCOR (dbfield) 0x1504	/* String */
# define TPERMIbyEMP (dbindex) 0x1500


/* TABLE: PERVIG 	RECORD LENGTH: 96	*/

# define PERVIG          (dbtable) 0x1600
# define PERVIG_EMP (dbfield) 0x1601	/* Integer */
# define PERVIG_NROLEG (dbfield) 0x1602	/* Long */
# define PERVIG_TIPPER (dbfield) 0x1603	/* Integer */
# define PERVIG_DELEGA (dbfield) 0x1604	/* String */
# define PERVIG_FILIAL (dbfield) 0x1605	/* String */
# define PERVIG_FECINI (dbfield) 0x1606	/* Date */
# define PERVIG_FECFIN (dbfield) 0x1607	/* Date */
# define PERVIG_DELORI (dbfield) 0x1608	/* String */
# define PERVIG_FILORI (dbfield) 0x1609	/* String */
# define PERVIG_CDATE (dbfield) 0x160a	/* Date */
# define PERVIG_CTIME (dbfield) 0x160b	/* Time */
# define PERVIG_CUID (dbfield) 0x160c	/* Long */
# define PERVIG_MDATE (dbfield) 0x160d	/* Date */
# define PERVIG_MTIME (dbfield) 0x160e	/* Time */
# define PERVIG_MUID (dbfield) 0x160f	/* Long */
# define PERVIG_NUMMOD (dbfield) 0x1610	/* Integer */
# define PERVIG_ULTMOD (dbfield) 0x1611	/* Integer */
# define PERVIG_ACTIVO (dbfield) 0x1612	/* Integer */
# define PERVIG_FECHA (dbfield) 0x1613	/* Date */
# define PERVIG_HORA (dbfield) 0x1614	/* Time */
# define PERVIG_USUARI (dbfield) 0x1615	/* Long */
# define PERVIGbyEMP (dbindex) 0x1600
# define PERVIGbyULTMOD (dbindex) 0x1601


/* TABLE: GRUFIL 	RECORD LENGTH: 82	*/

# define GRUFIL          (dbtable) 0x1700
# define GRUFIL_EMP (dbfield) 0x1701	/* Integer */
# define GRUFIL_GRUPO (dbfield) 0x1702	/* Integer */
# define GRUFIL_DESCRIP (dbfield) 0x1703	/* String */
# define GRUFIL_DESCOR (dbfield) 0x1704	/* String */
# define GRUFILbyEMP (dbindex) 0x1700


/* TABLE: FILXGRUP 	RECORD LENGTH: 64	*/

# define FILXGRUP        (dbtable) 0x1800
# define FILXGRUP_EMP (dbfield) 0x1801	/* Integer */
# define FILXGRUP_GRUPO (dbfield) 0x1802	/* Integer */
# define FILXGRUP_DELEGA (dbfield) 0x1803	/* String */
# define FILXGRUP_FILIAL (dbfield) 0x1804	/* String */
# define FILXGRUP_CDATE (dbfield) 0x1805	/* Date */
# define FILXGRUP_CTIME (dbfield) 0x1806	/* Time */
# define FILXGRUP_CUID (dbfield) 0x1807	/* Long */
# define FILXGRUP_MDATE (dbfield) 0x1808	/* Date */
# define FILXGRUP_MTIME (dbfield) 0x1809	/* Time */
# define FILXGRUP_MUID (dbfield) 0x180a	/* Long */
# define FILXGRUPbyEMP (dbindex) 0x1800
# define FILXGRUPbyFIL (dbindex) 0x1801


/* TABLE: MOTXCLI 	RECORD LENGTH: 56	*/

# define MOTXCLI         (dbtable) 0x1900
# define MOTXCLI_CODCOND (dbfield) 0x1901	/* Integer */
# define MOTXCLI_CODMOT (dbfield) 0x1902	/* Integer */
# define MOTXCLI_CLIENTE (dbfield) 0x1903	/* Long */
# define MOTXCLI_OBJETIVO (dbfield) 0x1904	/* Integer */
# define MOTXCLI_CDATE (dbfield) 0x1905	/* Date */
# define MOTXCLI_CTIME (dbfield) 0x1906	/* Time */
# define MOTXCLI_CUID (dbfield) 0x1907	/* Long */
# define MOTXCLI_MDATE (dbfield) 0x1908	/* Date */
# define MOTXCLI_MTIME (dbfield) 0x1909	/* Time */
# define MOTXCLI_MUID (dbfield) 0x190a	/* Long */
# define MOTXCLIbyCODCOND (dbindex) 0x1900


/* TABLE: TIPCIE 	RECORD LENGTH: 64	*/

# define TIPCIE          (dbtable) 0x1a00
# define TIPCIE_COTICI (dbfield) 0x1a01	/* Integer */
# define TIPCIE_DESCRIP (dbfield) 0x1a02	/* String */
# define TIPCIEbyCOTICI (dbindex) 0x1a00


/* TABLE: PERIODO 	RECORD LENGTH: 42	*/

# define PERIODO         (dbtable) 0x1b00
# define PERIODO_EMP (dbfield) 0x1b01	/* Integer */
# define PERIODO_ANOPER (dbfield) 0x1b02	/* Integer */
# define PERIODO_NUMPER (dbfield) 0x1b03	/* Integer */
# define PERIODO_FECDES (dbfield) 0x1b04	/* Date */
# define PERIODO_FECHAS (dbfield) 0x1b05	/* Date */
# define PERIODO_TIPCIE (dbfield) 0x1b06	/* Integer */
# define PERIODO_ESTADO (dbfield) 0x1b07	/* Integer */
# define PERIODObyEMP (dbindex) 0x1b00
# define PERIODObyFECDES (dbindex) 0x1b01

# define IO_OPERAC_CHKSUM (long) 0x4c92
