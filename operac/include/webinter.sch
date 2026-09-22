/* SCHEMA: webinter Interface a Web 	*/


/* TABLE: TMPVEMP 	RECORD LENGTH: 71	*/

# define TMPVEMP         (dbtable) 0x0100
# define TMPVEMP_EMP (dbfield) 0x0101	/* Integer */
# define TMPVEMP_NROPASE (dbfield) 0x0102	/* Long */
# define TMPVEMP_NROLEG (dbfield) 0x0103	/* Long */
# define TMPVEMP_NROLIQ (dbfield) 0x0104	/* Long */
# define TMPVEMP_VEMP (dbfield) 0x0105	/* Integer */
# define TMPVEMP_CCOSTO (dbfield) 0x0106	/* Long */
# define TMPVEMP_VALEMP (dbfield) 0x0107	/* Long */
# define TMPVEMP_CDATE (dbfield) 0x0108	/* Date */
# define TMPVEMP_CTIME (dbfield) 0x0109	/* Time */
# define TMPVEMP_CUID (dbfield) 0x010a	/* Long */
# define TMPVEMP_MDATE (dbfield) 0x010b	/* Date */
# define TMPVEMP_MTIME (dbfield) 0x010c	/* Time */
# define TMPVEMP_MUID (dbfield) 0x010d	/* Long */
# define TMPVEMPbyEMP (dbindex) 0x0100


/* TABLE: TMPASIST 	RECORD LENGTH: 70	*/

# define TMPASIST        (dbtable) 0x0200
# define TMPASIST_EMP (dbfield) 0x0201	/* Integer */
# define TMPASIST_NROPASE (dbfield) 0x0202	/* Long */
# define TMPASIST_NROLIQ (dbfield) 0x0203	/* Long */
# define TMPASIST_NROLEG (dbfield) 0x0204	/* Long */
# define TMPASIST_CODNOV (dbfield) 0x0205	/* Integer */
# define TMPASIST_FECHA (dbfield) 0x0206	/* Date */
# define TMPASIST_VALOR (dbfield) 0x0207	/* Long */
# define TMPASIST_JUSTIF (dbfield) 0x0208	/* Integer */
# define TMPASIST_CDATE (dbfield) 0x0209	/* Date */
# define TMPASIST_CTIME (dbfield) 0x020a	/* Time */
# define TMPASIST_CUID (dbfield) 0x020b	/* Long */
# define TMPASIST_MDATE (dbfield) 0x020c	/* Date */
# define TMPASIST_MTIME (dbfield) 0x020d	/* Time */
# define TMPASIST_MUID (dbfield) 0x020e	/* Long */
# define TMPASISTbyEMP (dbindex) 0x0200


/* TABLE: WPARAM 	RECORD LENGTH: 100	*/

# define WPARAM          (dbtable) 0x0300
# define WPARAM_CODPAR (dbfield) 0x0301	/* Integer */
# define WPARAM_VALOR (dbfield) 0x0302	/* String */
# define WPARAM_CUID (dbfield) 0x0303	/* Long */
# define WPARAM_CDATE (dbfield) 0x0304	/* Date */
# define WPARAM_CTIME (dbfield) 0x0305	/* Time */
# define WPARAM_MUID (dbfield) 0x0306	/* Long */
# define WPARAM_MDATE (dbfield) 0x0307	/* Date */
# define WPARAM_MTIME (dbfield) 0x0308	/* Time */
# define WPARAMbyCODPAR (dbindex) 0x0300


/* TABLE: WPASELOG 	RECORD LENGTH: 158	*/

# define WPASELOG        (dbtable) 0x0400
# define WPASELOG_CODPROG (dbfield) 0x0401	/* String */
# define WPASELOG_OISESION (dbfield) 0x0402	/* String */
# define WPASELOG_FECPED (dbfield) 0x0403	/* Date */
# define WPASELOG_HORPED (dbfield) 0x0404	/* Time */
# define WPASELOG_FECINI (dbfield) 0x0405	/* Date */
# define WPASELOG_HORINI (dbfield) 0x0406	/* Time */
# define WPASELOG_FECFIN (dbfield) 0x0407	/* Date */
# define WPASELOG_HORFIN (dbfield) 0x0408	/* Time */
# define WPASELOG_USUARIO (dbfield) 0x0409	/* String */
# define WPASELOG_ESTADO (dbfield) 0x040a	/* Integer */
# define WPASELOGbyCODPROG (dbindex) 0x0400


/* TABLE: WPARLOG 	RECORD LENGTH: 201	*/

# define WPARLOG         (dbtable) 0x0500
# define WPARLOG_CODPROG (dbfield) 0x0501	/* String */
# define WPARLOG_OISESION (dbfield) 0x0502	/* String */
# define WPARLOG_PARAM (dbfield) 0x0503	/* String */
# define WPARLOG_VALOR (dbfield) 0x0504	/* String */
# define WPARLOGbyCODPROG (dbindex) 0x0500


/* TABLE: WMSGLOG 	RECORD LENGTH: 193	*/

# define WMSGLOG         (dbtable) 0x0600
# define WMSGLOG_CODPROG (dbfield) 0x0601	/* String */
# define WMSGLOG_OISESION (dbfield) 0x0602	/* String */
# define WMSGLOG_FECHA (dbfield) 0x0603	/* Date */
# define WMSGLOG_HORA (dbfield) 0x0604	/* Time */
# define WMSGLOG_NRORENG (dbfield) 0x0605	/* Integer */
# define WMSGLOG_TIPO (dbfield) 0x0606	/* Integer */
# define WMSGLOG_MENSAJE (dbfield) 0x0607	/* String */
# define WMSGLOG_SOLUCION (dbfield) 0x0608	/* Integer */
# define WMSGLOGbyCODPROG (dbindex) 0x0600
# define WMSGLOGbySOLUCION (dbindex) 0x0601


/* TABLE: TMPVARNOV 	RECORD LENGTH: 97	*/

# define TMPVARNOV       (dbtable) 0x0700
# define TMPVARNOV_IDPASE (dbfield) 0x0701	/* Long */
# define TMPVARNOV_CODPROG (dbfield) 0x0702	/* String */
# define TMPVARNOV_NROLIQ (dbfield) 0x0703	/* Long */
# define TMPVARNOV_INTERN (dbfield) 0x0704	/* Long */
# define TMPVARNOV_NROVAR (dbfield) 0x0705	/* Integer */
# define TMPVARNOV_VALOR (dbfield) 0x0706	/* Long */
# define TMPVARNOV_CUID (dbfield) 0x0707	/* Long */
# define TMPVARNOV_CDATE (dbfield) 0x0708	/* Date */
# define TMPVARNOV_CTIME (dbfield) 0x0709	/* Time */
# define TMPVARNOV_MUID (dbfield) 0x070a	/* Long */
# define TMPVARNOV_MDATE (dbfield) 0x070b	/* Date */
# define TMPVARNOV_MTIME (dbfield) 0x070c	/* Time */
# define TMPVARNOVbyIDPASE (dbindex) 0x0700
# define TMPVARNOVbyINTERN (dbindex) 0x0701


/* TABLE: TMPVARFIJ 	RECORD LENGTH: 97	*/

# define TMPVARFIJ       (dbtable) 0x0800
# define TMPVARFIJ_IDPASE (dbfield) 0x0801	/* Long */
# define TMPVARFIJ_CODPROG (dbfield) 0x0802	/* String */
# define TMPVARFIJ_INTERN (dbfield) 0x0803	/* Long */
# define TMPVARFIJ_NROVAR (dbfield) 0x0804	/* Integer */
# define TMPVARFIJ_VALOR (dbfield) 0x0805	/* Long */
# define TMPVARFIJ_VALORT (dbfield) 0x0806	/* Long */
# define TMPVARFIJ_CUID (dbfield) 0x0807	/* Long */
# define TMPVARFIJ_CDATE (dbfield) 0x0808	/* Date */
# define TMPVARFIJ_CTIME (dbfield) 0x0809	/* Time */
# define TMPVARFIJ_MUID (dbfield) 0x080a	/* Long */
# define TMPVARFIJ_MDATE (dbfield) 0x080b	/* Date */
# define TMPVARFIJ_MTIME (dbfield) 0x080c	/* Time */
# define TMPVARFIJbyIDPASE (dbindex) 0x0800


/* TABLE: TMPWESPO 	RECORD LENGTH: 97	*/

# define TMPWESPO        (dbtable) 0x0900
# define TMPWESPO_OI_SESION (dbfield) 0x0901	/* String */
# define TMPWESPO_EMP (dbfield) 0x0902	/* Integer */
# define TMPWESPO_CLIENTE (dbfield) 0x0903	/* Long */
# define TMPWESPO_OBJETIVO (dbfield) 0x0904	/* Integer */
# define TMPWESPO_FDESDE (dbfield) 0x0905	/* Date */
# define TMPWESPO_FHASTA (dbfield) 0x0906	/* Date */
# define TMPWESPO_TOTESPO (dbfield) 0x0907	/* Long */
# define TMPWESPO_TOTRIF (dbfield) 0x0908	/* Long */
# define TMPWESPO_TOTNC (dbfield) 0x0909	/* Long */
# define TMPWESPO_CUID (dbfield) 0x090a	/* Long */
# define TMPWESPO_CDATE (dbfield) 0x090b	/* Date */
# define TMPWESPO_CTIME (dbfield) 0x090c	/* Time */
# define TMPWESPO_MUID (dbfield) 0x090d	/* Long */
# define TMPWESPO_MDATE (dbfield) 0x090e	/* Date */
# define TMPWESPO_MTIME (dbfield) 0x090f	/* Time */
# define TMPWESPObyOI_SESION (dbindex) 0x0900


/* TABLE: WOT 	RECORD LENGTH: 623	*/

# define WOT             (dbtable) 0x0a00
# define WOT_OISESION (dbfield) 0x0a01	/* String */
# define WOT_EMP (dbfield) 0x0a02	/* Integer */
# define WOT_TIPCOMP (dbfield) 0x0a03	/* Integer */
# define WOT_SERIE (dbfield) 0x0a04	/* String */
# define WOT_DELEG (dbfield) 0x0a05	/* String */
# define WOT_NROOT (dbfield) 0x0a06	/* Long */
# define WOT_CLIENTE (dbfield) 0x0a07	/* Long */
# define WOT_OBJET (dbfield) 0x0a08	/* Integer */
# define WOT_FECREG (dbfield) 0x0a09	/* Date */
# define WOT_CODSER (dbfield) 0x0a0a	/* Integer */
# define WOT_TIPSER (dbfield) 0x0a0b	/* Integer */
# define WOT_ESTVTA (dbfield) 0x0a0c	/* Integer */
# define WOT_ESTOPER (dbfield) 0x0a0d	/* Integer */
# define WOT_ESTADM (dbfield) 0x0a0e	/* Integer */
# define WOT_CODVEND (dbfield) 0x0a0f	/* Long */
# define WOT_FOKVTA (dbfield) 0x0a10	/* Date */
# define WOT_HOKVTA (dbfield) 0x0a11	/* Time */
# define WOT_FOKOPER (dbfield) 0x0a12	/* Date */
# define WOT_HOKOPER (dbfield) 0x0a13	/* Time */
# define WOT_FOKOADM (dbfield) 0x0a14	/* Date */
# define WOT_HOKOADM (dbfield) 0x0a15	/* Time */
# define WOT_FINICIO (dbfield) 0x0a16	/* Date */
# define WOT_HINICIO (dbfield) 0x0a17	/* Time */
# define WOT_FFINAL (dbfield) 0x0a18	/* Date */
# define WOT_HFINAL (dbfield) 0x0a19	/* Time */
# define WOT_OBSOP (dbfield) 0x0a1a	/* String */
# define WOT_OBSFAC (dbfield) 0x0a1b	/* String */
# define WOT_ABM (dbfield) 0x0a1c	/* String */
# define WOT_RIFF (dbfield) 0x0a1d	/* Float */
# define WOT_MOTRECHV (dbfield) 0x0a1e	/* String */
# define WOT_MOTRECHA (dbfield) 0x0a1f	/* String */
# define WOT_MOTRECHO (dbfield) 0x0a20	/* String */
# define WOT_CDATE (dbfield) 0x0a21	/* Date */
# define WOT_CTIME (dbfield) 0x0a22	/* Time */
# define WOT_CUID (dbfield) 0x0a23	/* Long */
# define WOT_MDATE (dbfield) 0x0a24	/* Date */
# define WOT_MTIME (dbfield) 0x0a25	/* Time */
# define WOT_MUID (dbfield) 0x0a26	/* Long */
# define WOT_EMPMT (dbfield) 0x0a27	/* Integer */
# define WOT_TIPCOMPMT (dbfield) 0x0a28	/* Integer */
# define WOT_SERIEMT (dbfield) 0x0a29	/* String */
# define WOT_DELEGMT (dbfield) 0x0a2a	/* String */
# define WOT_NROOTMT (dbfield) 0x0a2b	/* Long */
# define WOT_IMPRESA (dbfield) 0x0a2c	/* Integer */
# define WOT_USUCOM (dbfield) 0x0a2d	/* String */
# define WOT_USUOPER (dbfield) 0x0a2e	/* String */
# define WOT_PLAZO (dbfield) 0x0a2f	/* Integer */
# define WOT_USUADM (dbfield) 0x0a30	/* String */
# define WOT_ORIGEN (dbfield) 0x0a31	/* Integer */
# define WOT_ANULADO (dbfield) 0x0a32	/* Integer */
# define WOTbyOISESION (dbindex) 0x0a00
# define WOTbyCLIENTE (dbindex) 0x0a01


/* TABLE: WPTOSER 	RECORD LENGTH: 136	*/

# define WPTOSER         (dbtable) 0x0b00
# define WPTOSER_OISESION (dbfield) 0x0b01	/* String */
# define WPTOSER_EMP (dbfield) 0x0b02	/* Integer */
# define WPTOSER_TIPCOMP (dbfield) 0x0b03	/* Integer */
# define WPTOSER_SERIE (dbfield) 0x0b04	/* String */
# define WPTOSER_DELEG (dbfield) 0x0b05	/* String */
# define WPTOSER_NROOT (dbfield) 0x0b06	/* Long */
# define WPTOSER_TIPPTO (dbfield) 0x0b07	/* Integer */
# define WPTOSER_CLIENTE (dbfield) 0x0b08	/* Long */
# define WPTOSER_OBJET (dbfield) 0x0b09	/* Integer */
# define WPTOSER_COND (dbfield) 0x0b0a	/* String */
# define WPTOSER_CDATE (dbfield) 0x0b0b	/* Date */
# define WPTOSER_CTIME (dbfield) 0x0b0c	/* Time */
# define WPTOSER_CUID (dbfield) 0x0b0d	/* Long */
# define WPTOSER_MDATE (dbfield) 0x0b0e	/* Date */
# define WPTOSER_MTIME (dbfield) 0x0b0f	/* Time */
# define WPTOSER_MUID (dbfield) 0x0b10	/* Long */
# define WPTOSERbyOISESION (dbindex) 0x0b00
# define WPTOSERbyCLIENTE (dbindex) 0x0b01


/* TABLE: WNPUESTO 	RECORD LENGTH: 227	*/

# define WNPUESTO        (dbtable) 0x0c00
# define WNPUESTO_OISESION (dbfield) 0x0c01	/* String */
# define WNPUESTO_EMP (dbfield) 0x0c02	/* Integer */
# define WNPUESTO_TIPCOMP (dbfield) 0x0c03	/* Integer */
# define WNPUESTO_SERIE (dbfield) 0x0c04	/* String */
# define WNPUESTO_DELEG (dbfield) 0x0c05	/* String */
# define WNPUESTO_NROOT (dbfield) 0x0c06	/* Long */
# define WNPUESTO_TIPPTO (dbfield) 0x0c07	/* Integer */
# define WNPUESTO_NRORENG (dbfield) 0x0c08	/* Integer */
# define WNPUESTO_PUESTO (dbfield) 0x0c09	/* Integer */
# define WNPUESTO_CLIENTE (dbfield) 0x0c0a	/* Long */
# define WNPUESTO_OBJET (dbfield) 0x0c0b	/* Integer */
# define WNPUESTO_HINICIO (dbfield) 0x0c0c	/* Time */
# define WNPUESTO_HFINAL (dbfield) 0x0c0d	/* Time */
# define WNPUESTO_DIA1 (dbfield) 0x0c0e	/* String */
# define WNPUESTO_DIA2 (dbfield) 0x0c0f	/* String */
# define WNPUESTO_DIA3 (dbfield) 0x0c10	/* String */
# define WNPUESTO_DIA4 (dbfield) 0x0c11	/* String */
# define WNPUESTO_DIA5 (dbfield) 0x0c12	/* String */
# define WNPUESTO_DIA6 (dbfield) 0x0c13	/* String */
# define WNPUESTO_DIA7 (dbfield) 0x0c14	/* String */
# define WNPUESTO_REGIM (dbfield) 0x0c15	/* String */
# define WNPUESTO_SALARIO (dbfield) 0x0c16	/* Long */
# define WNPUESTO_CANTPUE (dbfield) 0x0c17	/* Integer */
# define WNPUESTO_CANTVIG (dbfield) 0x0c18	/* Integer */
# define WNPUESTO_COND (dbfield) 0x0c19	/* String */
# define WNPUESTO_CDATE (dbfield) 0x0c1a	/* Date */
# define WNPUESTO_CTIME (dbfield) 0x0c1b	/* Time */
# define WNPUESTO_CUID (dbfield) 0x0c1c	/* Long */
# define WNPUESTO_MDATE (dbfield) 0x0c1d	/* Date */
# define WNPUESTO_MTIME (dbfield) 0x0c1e	/* Time */
# define WNPUESTO_MUID (dbfield) 0x0c1f	/* Long */
# define WNPUESTO_CONCF (dbfield) 0x0c20	/* Integer */
# define WNPUESTO_CODFREC (dbfield) 0x0c21	/* String */
# define WNPUESTO_HORAPT (dbfield) 0x0c22	/* Integer */
# define WNPUESTO_HSNORM (dbfield) 0x0c23	/* Long */
# define WNPUESTO_HSEXTR (dbfield) 0x0c24	/* Long */
# define WNPUESTO_HS50 (dbfield) 0x0c25	/* Long */
# define WNPUESTO_HS100 (dbfield) 0x0c26	/* Long */
# define WNPUESTO_SUBREG (dbfield) 0x0c27	/* String */
# define WNPUESTO_TIPODIA (dbfield) 0x0c28	/* String */
# define WNPUESTO_HS_FNORM (dbfield) 0x0c29	/* Long */
# define WNPUESTO_HS_F50 (dbfield) 0x0c2a	/* Long */
# define WNPUESTO_HS_F100 (dbfield) 0x0c2b	/* Long */
# define WNPUESTO_CANTRVIG (dbfield) 0x0c2c	/* Integer */
# define WNPUESTO_CODINT (dbfield) 0x0c2d	/* Long */
# define WNPUESTObyOISESION (dbindex) 0x0c00
# define WNPUESTObyCLIENTE (dbindex) 0x0c01


/* TABLE: WTARIFA 	RECORD LENGTH: 159	*/

# define WTARIFA         (dbtable) 0x0d00
# define WTARIFA_CODPROG (dbfield) 0x0d01	/* String */
# define WTARIFA_OISESION (dbfield) 0x0d02	/* String */
# define WTARIFA_EMP (dbfield) 0x0d03	/* Integer */
# define WTARIFA_CLIENTE (dbfield) 0x0d04	/* Long */
# define WTARIFA_OBJETIVO (dbfield) 0x0d05	/* Integer */
# define WTARIFA_GRUTAR (dbfield) 0x0d06	/* String */
# define WTARIFA_CONC (dbfield) 0x0d07	/* Integer */
# define WTARIFA_HORTRA (dbfield) 0x0d08	/* Long */
# define WTARIFA_HORAS (dbfield) 0x0d09	/* Long */
# define WTARIFA_PRECIO (dbfield) 0x0d0a	/* Long */
# define WTARIFA_CDATE (dbfield) 0x0d0b	/* Date */
# define WTARIFA_CTIME (dbfield) 0x0d0c	/* Time */
# define WTARIFA_CUID (dbfield) 0x0d0d	/* Long */
# define WTARIFAbyCODPROG (dbindex) 0x0d00


/* TABLE: WITMXPUE 	RECORD LENGTH: 171	*/

# define WITMXPUE        (dbtable) 0x0e00
# define WITMXPUE_OISESION (dbfield) 0x0e01	/* String */
# define WITMXPUE_EMP (dbfield) 0x0e02	/* Integer */
# define WITMXPUE_TIPCOMP (dbfield) 0x0e03	/* Integer */
# define WITMXPUE_SERIE (dbfield) 0x0e04	/* String */
# define WITMXPUE_DELEG (dbfield) 0x0e05	/* String */
# define WITMXPUE_NROOT (dbfield) 0x0e06	/* Long */
# define WITMXPUE_CODINT (dbfield) 0x0e07	/* Long */
# define WITMXPUE_CLIENTE (dbfield) 0x0e08	/* Long */
# define WITMXPUE_OBJET (dbfield) 0x0e09	/* Integer */
# define WITMXPUE_TIPPTO (dbfield) 0x0e0a	/* Integer */
# define WITMXPUE_PUESTO (dbfield) 0x0e0b	/* Integer */
# define WITMXPUE_HINICIO (dbfield) 0x0e0c	/* Time */
# define WITMXPUE_HFINAL (dbfield) 0x0e0d	/* Time */
# define WITMXPUE_DIA1 (dbfield) 0x0e0e	/* String */
# define WITMXPUE_DIA2 (dbfield) 0x0e0f	/* String */
# define WITMXPUE_DIA3 (dbfield) 0x0e10	/* String */
# define WITMXPUE_DIA4 (dbfield) 0x0e11	/* String */
# define WITMXPUE_DIA5 (dbfield) 0x0e12	/* String */
# define WITMXPUE_DIA6 (dbfield) 0x0e13	/* String */
# define WITMXPUE_DIA7 (dbfield) 0x0e14	/* String */
# define WITMXPUE_REGIM (dbfield) 0x0e15	/* String */
# define WITMXPUE_ITEM (dbfield) 0x0e16	/* Integer */
# define WITMXPUE_ESTADO (dbfield) 0x0e17	/* Integer */
# define WITMXPUE_CDATE (dbfield) 0x0e18	/* Date */
# define WITMXPUE_CTIME (dbfield) 0x0e19	/* Time */
# define WITMXPUE_CUID (dbfield) 0x0e1a	/* Long */
# define WITMXPUE_MDATE (dbfield) 0x0e1b	/* Date */
# define WITMXPUE_MTIME (dbfield) 0x0e1c	/* Time */
# define WITMXPUE_MUID (dbfield) 0x0e1d	/* Long */
# define WITMXPUE_CONSRIF (dbfield) 0x0e1e	/* Integer */
# define WITMXPUEbyOISESION (dbindex) 0x0e00
# define WITMXPUEbyPUESTO (dbindex) 0x0e01


/* TABLE: WPER 	RECORD LENGTH: 531	*/

# define WPER            (dbtable) 0x0f00
# define WPER_OISESION (dbfield) 0x0f01	/* String */
# define WPER_EMP (dbfield) 0x0f02	/* Integer */
# define WPER_NROLEG (dbfield) 0x0f03	/* Long */
# define WPER_APYNOM (dbfield) 0x0f04	/* String */
# define WPER_DIREC (dbfield) 0x0f05	/* String */
# define WPER_LOCAL (dbfield) 0x0f06	/* Long */
# define WPER_PROV (dbfield) 0x0f07	/* Integer */
# define WPER_TELEF (dbfield) 0x0f08	/* String */
# define WPER_CODPOST (dbfield) 0x0f09	/* String */
# define WPER_FECNAC (dbfield) 0x0f0a	/* Date */
# define WPER_FECING (dbfield) 0x0f0b	/* Date */
# define WPER_FECEGR (dbfield) 0x0f0c	/* Date */
# define WPER_CODNAC (dbfield) 0x0f0d	/* Integer */
# define WPER_ESTCIV (dbfield) 0x0f0e	/* Integer */
# define WPER_FECCAS (dbfield) 0x0f0f	/* Date */
# define WPER_SEXO (dbfield) 0x0f10	/* Integer */
# define WPER_CODDOC (dbfield) 0x0f11	/* String */
# define WPER_NRODOC (dbfield) 0x0f12	/* Long */
# define WPER_EXPED (dbfield) 0x0f13	/* String */
# define WPER_RELACION (dbfield) 0x0f14	/* Integer */
# define WPER_CODCCOS (dbfield) 0x0f15	/* Long */
# define WPER_CODESTR (dbfield) 0x0f16	/* String */
# define WPER_CODUBI (dbfield) 0x0f17	/* Long */
# define WPER_CODCAT (dbfield) 0x0f18	/* Integer */
# define WPER_CODCAL (dbfield) 0x0f19	/* Integer */
# define WPER_CODTAR (dbfield) 0x0f1a	/* Long */
# define WPER_CODEST (dbfield) 0x0f1b	/* Integer */
# define WPER_CODTIT (dbfield) 0x0f1c	/* Long */
# define WPER_HDESDE (dbfield) 0x0f1d	/* Time */
# define WPER_HHASTA (dbfield) 0x0f1e	/* Time */
# define WPER_NROTARJ (dbfield) 0x0f1f	/* Float */
# define WPER_FORMA (dbfield) 0x0f20	/* Integer */
# define WPER_CODSIND (dbfield) 0x0f21	/* Integer */
# define WPER_NROASIND (dbfield) 0x0f22	/* String */
# define WPER_CODOS (dbfield) 0x0f23	/* Integer */
# define WPER_NROAOS (dbfield) 0x0f24	/* String */
# define WPER_CODSSOC (dbfield) 0x0f25	/* Integer */
# define WPER_NROSSOC (dbfield) 0x0f26	/* String */
# define WPER_CODCJ (dbfield) 0x0f27	/* Integer */
# define WPER_NROACJ (dbfield) 0x0f28	/* String */
# define WPER_BANK (dbfield) 0x0f29	/* Integer */
# define WPER_CODSUC (dbfield) 0x0f2a	/* Integer */
# define WPER_NROCTA (dbfield) 0x0f2b	/* String */
# define WPER_RESERVADO (dbfield) 0x0f2c	/* Integer */
# define WPER_ACTIVO (dbfield) 0x0f2d	/* Integer */
# define WPER_NROLIQ (dbfield) 0x0f2e	/* Long */
# define WPER_NROINSIG (dbfield) 0x0f2f	/* String */
# define WPER_TIPSERV (dbfield) 0x0f30	/* String */
# define WPER_USRID (dbfield) 0x0f31	/* Long */
# define WPER_FECHA (dbfield) 0x0f32	/* Date */
# define WPER_HORA (dbfield) 0x0f33	/* Time */
# define WPER_PLAN (dbfield) 0x0f34	/* Integer */
# define WPER_VACS (dbfield) 0x0f35	/* Date */
# define WPER_VACE (dbfield) 0x0f36	/* Date */
# define WPER_ZONAF (dbfield) 0x0f37	/* Integer */
# define WPER_PAGF (dbfield) 0x0f38	/* Integer */
# define WPER_FILX (dbfield) 0x0f39	/* String */
# define WPER_FILY (dbfield) 0x0f3a	/* Integer */
# define WPER_REING (dbfield) 0x0f3b	/* Long */
# define WPER_FECPUES (dbfield) 0x0f3c	/* Date */
# define WPER_PASAP (dbfield) 0x0f3d	/* String */
# define WPER_ORIGEN (dbfield) 0x0f3e	/* String */
# define WPER_VENC (dbfield) 0x0f3f	/* Date */
# define WPER_CODMEGR (dbfield) 0x0f40	/* Integer */
# define WPER_CODPAIS (dbfield) 0x0f41	/* Integer */
# define WPER_ACUM (dbfield) 0x0f42	/* Long */
# define WPER_ACUMS (dbfield) 0x0f43	/* Long */
# define WPER_DUSRID (dbfield) 0x0f44	/* Long */
# define WPER_DFECHA (dbfield) 0x0f45	/* Date */
# define WPER_DHORA (dbfield) 0x0f46	/* Time */
# define WPER_FECCOS (dbfield) 0x0f47	/* Date */
# define WPER_FECESTR (dbfield) 0x0f48	/* Date */
# define WPERbyOISESION (dbindex) 0x0f00


/* TABLE: WDATPERS 	RECORD LENGTH: 384	*/

# define WDATPERS        (dbtable) 0x1000
# define WDATPERS_OISESION (dbfield) 0x1001	/* String */
# define WDATPERS_EMP (dbfield) 0x1002	/* Integer */
# define WDATPERS_NROLEG (dbfield) 0x1003	/* Long */
# define WDATPERS_APELL (dbfield) 0x1004	/* String */
# define WDATPERS_NOMBRE (dbfield) 0x1005	/* String */
# define WDATPERS_CALLE (dbfield) 0x1006	/* String */
# define WDATPERS_NRO (dbfield) 0x1007	/* String */
# define WDATPERS_PISO (dbfield) 0x1008	/* String */
# define WDATPERS_DPTO (dbfield) 0x1009	/* String */
# define WDATPERS_PAISNAC (dbfield) 0x100a	/* Integer */
# define WDATPERS_CIUDNAC (dbfield) 0x100b	/* String */
# define WDATPERS_PROVNAC (dbfield) 0x100c	/* Integer */
# define WDATPERS_FECANT (dbfield) 0x100d	/* Date */
# define WDATPERS_FECTRAN (dbfield) 0x100e	/* Date */
# define WDATPERS_FECDENA (dbfield) 0x100f	/* Date */
# define WDATPERS_CUIL (dbfield) 0x1010	/* String */
# define WDATPERS_FEINAC (dbfield) 0x1011	/* Date */
# define WDATPERS_FAFAFJP (dbfield) 0x1012	/* Date */
# define WDATPERS_TIPOPER (dbfield) 0x1013	/* Integer */
# define WDATPERS_CODACTIV (dbfield) 0x1014	/* String */
# define WDATPERS_LUGPAG (dbfield) 0x1015	/* Long */
# define WDATPERS_CONTRATO (dbfield) 0x1016	/* Integer */
# define WDATPERS_NROSOL (dbfield) 0x1017	/* Integer */
# define WDATPERS_PJORRED (dbfield) 0x1018	/* Long */
# define WDATPERS_CANRENOV (dbfield) 0x1019	/* Integer */
# define WDATPERS_DURACONT (dbfield) 0x101a	/* Integer */
# define WDATPERS_JUBILADO (dbfield) 0x101b	/* Integer */
# define WDATPERS_PRESTA (dbfield) 0x101c	/* Integer */
# define WDATPERS_PLANP (dbfield) 0x101d	/* Integer */
# define WDATPERS_NROAPRE (dbfield) 0x101e	/* String */
# define WDATPERS_PORCPRE (dbfield) 0x101f	/* Long */
# define WDATPERS_EMAIL (dbfield) 0x1020	/* String */
# define WDATPERS_CONDLEG (dbfield) 0x1021	/* Integer */
# define WDATPERS_CAT (dbfield) 0x1022	/* String */
# define WDATPERS_FECINIC (dbfield) 0x1023	/* Date */
# define WDATPERS_BANKC (dbfield) 0x1024	/* Integer */
# define WDATPERS_CODSUCC (dbfield) 0x1025	/* Integer */
# define WDATPERSbyOISESION (dbindex) 0x1000

# define IO_WEBINTER_CHKSUM (long) 0xf479
