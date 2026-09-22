/* SCHEMA: comerc Esquema de Comercial	*/


/* TABLE: SERVICIO 	RECORD LENGTH: 126	*/

# define SERVICIO        (dbtable) 0x0100
# define SERVICIO_CODSER (dbfield) 0x0101	/* Integer */
# define SERVICIO_DESCRIP (dbfield) 0x0102	/* String */
# define SERVICIO_DESCOR (dbfield) 0x0103	/* String */
# define SERVICIObyCODSER (dbindex) 0x0100


/* TABLE: NCBTES 	RECORD LENGTH: 86	*/

# define NCBTES          (dbtable) 0x0200
# define NCBTES_TIPCOMP (dbfield) 0x0201	/* Integer */
# define NCBTES_DESCRIP (dbfield) 0x0202	/* String */
# define NCBTES_DESCOR (dbfield) 0x0203	/* String */
# define NCBTES_CDATE (dbfield) 0x0204	/* Date */
# define NCBTES_CTIME (dbfield) 0x0205	/* Time */
# define NCBTES_CUID (dbfield) 0x0206	/* Long */
# define NCBTES_MDATE (dbfield) 0x0207	/* Date */
# define NCBTES_MTIME (dbfield) 0x0208	/* Time */
# define NCBTES_MUID (dbfield) 0x0209	/* Long */
# define NCBTESbyTIPCOMP (dbindex) 0x0200


/* TABLE: NSERIES 	RECORD LENGTH: 89	*/

# define NSERIES         (dbtable) 0x0300
# define NSERIES_EMP (dbfield) 0x0301	/* Integer */
# define NSERIES_TIPCOMP (dbfield) 0x0302	/* Integer */
# define NSERIES_SERIE (dbfield) 0x0303	/* Integer */
# define NSERIES_DESCRIP (dbfield) 0x0304	/* String */
# define NSERIES_DESCOR (dbfield) 0x0305	/* String */
# define NSERIES_CDATE (dbfield) 0x0306	/* Date */
# define NSERIES_CTIME (dbfield) 0x0307	/* Time */
# define NSERIES_CUID (dbfield) 0x0308	/* Long */
# define NSERIES_MDATE (dbfield) 0x0309	/* Date */
# define NSERIES_MTIME (dbfield) 0x030a	/* Time */
# define NSERIES_MUID (dbfield) 0x030b	/* Long */
# define NSERIESbyEMP (dbindex) 0x0300


/* TABLE: CBTENRO 	RECORD LENGTH: 63	*/

# define CBTENRO         (dbtable) 0x0400
# define CBTENRO_EMP (dbfield) 0x0401	/* Integer */
# define CBTENRO_TIPCOMP (dbfield) 0x0402	/* Integer */
# define CBTENRO_SERIE (dbfield) 0x0403	/* Integer */
# define CBTENRO_DELEG (dbfield) 0x0404	/* String */
# define CBTENRO_NROACT (dbfield) 0x0405	/* Long */
# define CBTENRO_IASER (dbfield) 0x0406	/* Integer */
# define CBTENRO_CDATE (dbfield) 0x0407	/* Date */
# define CBTENRO_CTIME (dbfield) 0x0408	/* Time */
# define CBTENRO_CUID (dbfield) 0x0409	/* Long */
# define CBTENRO_MDATE (dbfield) 0x040a	/* Date */
# define CBTENRO_MTIME (dbfield) 0x040b	/* Time */
# define CBTENRO_MUID (dbfield) 0x040c	/* Long */
# define CBTENRObyEMP (dbindex) 0x0400


/* TABLE: REGIMEN 	RECORD LENGTH: 118	*/

# define REGIMEN         (dbtable) 0x0500
# define REGIMEN_DIAS (dbfield) 0x0501	/* Integer */
# define REGIMEN_DFRAN (dbfield) 0x0502	/* Integer */
# define REGIMEN_HSREG (dbfield) 0x0503	/* Integer */
# define REGIMEN_REGIM (dbfield) 0x0504	/* String */
# define REGIMEN_DESCRIP (dbfield) 0x0505	/* String */
# define REGIMEN_HSNORM (dbfield) 0x0506	/* Integer */
# define REGIMEN_HSEXTRA (dbfield) 0x0507	/* Integer */
# define REGIMEN_CDATE (dbfield) 0x0508	/* Date */
# define REGIMEN_CTIME (dbfield) 0x0509	/* Time */
# define REGIMEN_CUID (dbfield) 0x050a	/* Long */
# define REGIMEN_MDATE (dbfield) 0x050b	/* Date */
# define REGIMEN_MTIME (dbfield) 0x050c	/* Time */
# define REGIMEN_MUID (dbfield) 0x050d	/* Long */
# define REGIMEN_HSSEM (dbfield) 0x050e	/* Integer */
# define REGIMEN_PORHNOR (dbfield) 0x050f	/* Long */
# define REGIMEN_PORHEXT (dbfield) 0x0510	/* Long */
# define REGIMEN_PARTIME (dbfield) 0x0511	/* Integer */
# define REGIMEN_EMP (dbfield) 0x0512	/* Integer */
# define REGIMENbyDIAS (dbindex) 0x0500
# define REGIMENbyREG (dbindex) 0x0501
# define REGIMENbyREGI (dbindex) 0x0502
# define REGIMENbyPARTIME (dbindex) 0x0503
# define REGIMENbyEMP (dbindex) 0x0504


/* TABLE: CONTR 	RECORD LENGTH: 85	*/

# define CONTR           (dbtable) 0x0600
# define CONTR_CODCONT (dbfield) 0x0601	/* Integer */
# define CONTR_DESCRIP (dbfield) 0x0602	/* String */
# define CONTR_DESCOR (dbfield) 0x0603	/* String */
# define CONTRbyCODCONT (dbindex) 0x0600


/* TABLE: TPTOSER 	RECORD LENGTH: 90	*/

# define TPTOSER         (dbtable) 0x0700
# define TPTOSER_TIPPTO (dbfield) 0x0701	/* Integer */
# define TPTOSER_DESCRIP (dbfield) 0x0702	/* String */
# define TPTOSER_DESCOR (dbfield) 0x0703	/* String */
# define TPTOSER_ARMAS (dbfield) 0x0704	/* Integer */
# define TPTOSER_GRUPO (dbfield) 0x0705	/* Integer */
# define TPTOSER_EMP (dbfield) 0x0706	/* Integer */
# define TPTOSERbyTIPPTO (dbindex) 0x0700
# define TPTOSERbyEMP (dbindex) 0x0701


/* TABLE: OT 	RECORD LENGTH: 545	*/

# define OT              (dbtable) 0x0800
# define OT_EMP (dbfield) 0x0801	/* Integer */
# define OT_TIPCOMP (dbfield) 0x0802	/* Integer */
# define OT_SERIE (dbfield) 0x0803	/* Integer */
# define OT_DELEG (dbfield) 0x0804	/* String */
# define OT_NROOT (dbfield) 0x0805	/* Long */
# define OT_CLIENTE (dbfield) 0x0806	/* Long */
# define OT_OBJET (dbfield) 0x0807	/* Integer */
# define OT_FECREG (dbfield) 0x0808	/* Date */
# define OT_CODSER (dbfield) 0x0809	/* Integer */
# define OT_TIPSER (dbfield) 0x080a	/* Integer */
# define OT_ESTVTA (dbfield) 0x080b	/* Integer */
# define OT_ESTOPER (dbfield) 0x080c	/* Integer */
# define OT_ESTADM (dbfield) 0x080d	/* Integer */
# define OT_CODVEND (dbfield) 0x080e	/* Long */
# define OT_FOKVTA (dbfield) 0x080f	/* Date */
# define OT_HOKVTA (dbfield) 0x0810	/* Time */
# define OT_FOKOPER (dbfield) 0x0811	/* Date */
# define OT_HOKOPER (dbfield) 0x0812	/* Time */
# define OT_FOKOADM (dbfield) 0x0813	/* Date */
# define OT_HOKOADM (dbfield) 0x0814	/* Time */
# define OT_FINICIO (dbfield) 0x0815	/* Date */
# define OT_HINICIO (dbfield) 0x0816	/* Time */
# define OT_FFINAL (dbfield) 0x0817	/* Date */
# define OT_HFINAL (dbfield) 0x0818	/* Time */
# define OT_OBSOP (dbfield) 0x0819	/* String */
# define OT_OBSFAC (dbfield) 0x081a	/* String */
# define OT_ABM (dbfield) 0x081b	/* String */
# define OT_RIFF (dbfield) 0x081c	/* Float */
# define OT_MOTRECHV (dbfield) 0x081d	/* String */
# define OT_MOTRECHA (dbfield) 0x081e	/* String */
# define OT_MOTRECHO (dbfield) 0x081f	/* String */
# define OT_CDATE (dbfield) 0x0820	/* Date */
# define OT_CTIME (dbfield) 0x0821	/* Time */
# define OT_CUID (dbfield) 0x0822	/* Long */
# define OT_MDATE (dbfield) 0x0823	/* Date */
# define OT_MTIME (dbfield) 0x0824	/* Time */
# define OT_MUID (dbfield) 0x0825	/* Long */
# define OT_EMPMT (dbfield) 0x0826	/* Integer */
# define OT_TIPCOMPMT (dbfield) 0x0827	/* Integer */
# define OT_SERIEMT (dbfield) 0x0828	/* Integer */
# define OT_DELEGMT (dbfield) 0x0829	/* String */
# define OT_NROOTMT (dbfield) 0x082a	/* Long */
# define OT_IMPRESA (dbfield) 0x082b	/* Integer */
# define OT_USUCOM (dbfield) 0x082c	/* String */
# define OT_USUOPER (dbfield) 0x082d	/* String */
# define OT_PLAZO (dbfield) 0x082e	/* Integer */
# define OT_USUADM (dbfield) 0x082f	/* String */
# define OT_ORIGEN (dbfield) 0x0830	/* Integer */
# define OT_ANULADO (dbfield) 0x0831	/* Integer */
# define OTbyEMP (dbindex) 0x0800
# define OTbyCLIENTE (dbindex) 0x0801
# define OTbyTIPSER (dbindex) 0x0802
# define OTbyNROOT (dbindex) 0x0803
# define OTbyFECREG (dbindex) 0x0804
# define OTbyAPROBC (dbindex) 0x0805
# define OTbyAPROBO (dbindex) 0x0806
# define OTbyAPROBV (dbindex) 0x0807
# define OTbyAPROB1 (dbindex) 0x0808
# define OTbyAPROB2 (dbindex) 0x0809
# define OTbyAPROB3 (dbindex) 0x080a
# define OTbyFECHA (dbindex) 0x080b
# define OTbyMT (dbindex) 0x080c


/* TABLE: PTOSER 	RECORD LENGTH: 71	*/

# define PTOSER          (dbtable) 0x0900
# define PTOSER_EMP (dbfield) 0x0901	/* Integer */
# define PTOSER_TIPCOMP (dbfield) 0x0902	/* Integer */
# define PTOSER_SERIE (dbfield) 0x0903	/* Integer */
# define PTOSER_DELEG (dbfield) 0x0904	/* String */
# define PTOSER_NROOT (dbfield) 0x0905	/* Long */
# define PTOSER_TIPPTO (dbfield) 0x0906	/* Integer */
# define PTOSER_CLIENTE (dbfield) 0x0907	/* Long */
# define PTOSER_OBJET (dbfield) 0x0908	/* Integer */
# define PTOSER_COND (dbfield) 0x0909	/* String */
# define PTOSER_CDATE (dbfield) 0x090a	/* Date */
# define PTOSER_CTIME (dbfield) 0x090b	/* Time */
# define PTOSER_CUID (dbfield) 0x090c	/* Long */
# define PTOSER_MDATE (dbfield) 0x090d	/* Date */
# define PTOSER_MTIME (dbfield) 0x090e	/* Time */
# define PTOSER_MUID (dbfield) 0x090f	/* Long */
# define PTOSERbyEMP (dbindex) 0x0900
# define PTOSERbyCLIENTE (dbindex) 0x0901


/* TABLE: NPUESTO 	RECORD LENGTH: 160	*/

# define NPUESTO         (dbtable) 0x0a00
# define NPUESTO_EMP (dbfield) 0x0a01	/* Integer */
# define NPUESTO_TIPCOMP (dbfield) 0x0a02	/* Integer */
# define NPUESTO_SERIE (dbfield) 0x0a03	/* Integer */
# define NPUESTO_DELEG (dbfield) 0x0a04	/* String */
# define NPUESTO_NROOT (dbfield) 0x0a05	/* Long */
# define NPUESTO_TIPPTO (dbfield) 0x0a06	/* Integer */
# define NPUESTO_NRORENG (dbfield) 0x0a07	/* Integer */
# define NPUESTO_PUESTO (dbfield) 0x0a08	/* Integer */
# define NPUESTO_CLIENTE (dbfield) 0x0a09	/* Long */
# define NPUESTO_OBJET (dbfield) 0x0a0a	/* Integer */
# define NPUESTO_HINICIO (dbfield) 0x0a0b	/* Time */
# define NPUESTO_HFINAL (dbfield) 0x0a0c	/* Time */
# define NPUESTO_DIAS (dbfield) 0x0a0d	/* String */
# define NPUESTO_REGIM (dbfield) 0x0a0e	/* String */
# define NPUESTO_SALARIO (dbfield) 0x0a0f	/* Long */
# define NPUESTO_CANTPUE (dbfield) 0x0a10	/* Integer */
# define NPUESTO_CANTVIG (dbfield) 0x0a11	/* Integer */
# define NPUESTO_COND (dbfield) 0x0a12	/* String */
# define NPUESTO_CDATE (dbfield) 0x0a13	/* Date */
# define NPUESTO_CTIME (dbfield) 0x0a14	/* Time */
# define NPUESTO_CUID (dbfield) 0x0a15	/* Long */
# define NPUESTO_MDATE (dbfield) 0x0a16	/* Date */
# define NPUESTO_MTIME (dbfield) 0x0a17	/* Time */
# define NPUESTO_MUID (dbfield) 0x0a18	/* Long */
# define NPUESTO_CONCF (dbfield) 0x0a19	/* Integer */
# define NPUESTO_CODFREC (dbfield) 0x0a1a	/* String */
# define NPUESTO_HORAPT (dbfield) 0x0a1b	/* Integer */
# define NPUESTO_HSNORM (dbfield) 0x0a1c	/* Long */
# define NPUESTO_HSEXTR (dbfield) 0x0a1d	/* Long */
# define NPUESTO_HS50 (dbfield) 0x0a1e	/* Long */
# define NPUESTO_HS100 (dbfield) 0x0a1f	/* Long */
# define NPUESTO_SUBREG (dbfield) 0x0a20	/* String */
# define NPUESTO_TIPODIA (dbfield) 0x0a21	/* String */
# define NPUESTO_HS_FNORM (dbfield) 0x0a22	/* Long */
# define NPUESTO_HS_F50 (dbfield) 0x0a23	/* Long */
# define NPUESTO_HS_F100 (dbfield) 0x0a24	/* Long */
# define NPUESTO_CANTRVIG (dbfield) 0x0a25	/* Integer */
# define NPUESTO_CODINT (dbfield) 0x0a26	/* Integer */
# define NPUESTObyEMP (dbindex) 0x0a00
# define NPUESTObyCLIENTE (dbindex) 0x0a01
# define NPUESTObyNROOT (dbindex) 0x0a02
# define NPUESTObyCODINT (dbindex) 0x0a03
# define NPUESTObyCODINTCL (dbindex) 0x0a04


/* TABLE: EQUIPOT 	RECORD LENGTH: 93	*/

# define EQUIPOT         (dbtable) 0x0b00
# define EQUIPOT_EMP (dbfield) 0x0b01	/* Integer */
# define EQUIPOT_TIPCOMP (dbfield) 0x0b02	/* Integer */
# define EQUIPOT_SERIE (dbfield) 0x0b03	/* Integer */
# define EQUIPOT_DELEG (dbfield) 0x0b04	/* String */
# define EQUIPOT_NROOT (dbfield) 0x0b05	/* Long */
# define EQUIPOT_NROINT (dbfield) 0x0b06	/* Long */
# define EQUIPOT_CLIENTE (dbfield) 0x0b07	/* Long */
# define EQUIPOT_OBJET (dbfield) 0x0b08	/* Integer */
# define EQUIPOT_CANTI (dbfield) 0x0b09	/* Long */
# define EQUIPOT_COND (dbfield) 0x0b0a	/* String */
# define EQUIPOT_CDATE (dbfield) 0x0b0b	/* Date */
# define EQUIPOT_CTIME (dbfield) 0x0b0c	/* Time */
# define EQUIPOT_CUID (dbfield) 0x0b0d	/* Long */
# define EQUIPOT_MDATE (dbfield) 0x0b0e	/* Date */
# define EQUIPOT_MTIME (dbfield) 0x0b0f	/* Time */
# define EQUIPOT_MUID (dbfield) 0x0b10	/* Long */
# define EQUIPOT_PRECIO (dbfield) 0x0b11	/* Float */
# define EQUIPOT_PRECAMOR (dbfield) 0x0b12	/* Float */
# define EQUIPOTbyEMP (dbindex) 0x0b00
# define EQUIPOTbyCLIENTE (dbindex) 0x0b01


/* TABLE: TARIFA 	RECORD LENGTH: 82	*/

# define TARIFA          (dbtable) 0x0c00
# define TARIFA_EMP (dbfield) 0x0c01	/* Integer */
# define TARIFA_TIPCOMP (dbfield) 0x0c02	/* Integer */
# define TARIFA_SERIE (dbfield) 0x0c03	/* Integer */
# define TARIFA_DELEG (dbfield) 0x0c04	/* String */
# define TARIFA_NROOT (dbfield) 0x0c05	/* Long */
# define TARIFA_CONC (dbfield) 0x0c06	/* Integer */
# define TARIFA_CLIENTE (dbfield) 0x0c07	/* Long */
# define TARIFA_OBJET (dbfield) 0x0c08	/* Integer */
# define TARIFA_HORAS (dbfield) 0x0c09	/* Long */
# define TARIFA_PRECIO (dbfield) 0x0c0a	/* Long */
# define TARIFA_CDATE (dbfield) 0x0c0b	/* Date */
# define TARIFA_CTIME (dbfield) 0x0c0c	/* Time */
# define TARIFA_CUID (dbfield) 0x0c0d	/* Long */
# define TARIFA_MDATE (dbfield) 0x0c0e	/* Date */
# define TARIFA_MTIME (dbfield) 0x0c0f	/* Time */
# define TARIFA_MUID (dbfield) 0x0c10	/* Long */
# define TARIFA_CONSRIF (dbfield) 0x0c11	/* Integer */
# define TARIFA_HORTRA (dbfield) 0x0c12	/* Long */
# define TARIFAbyEMP (dbindex) 0x0c00
# define TARIFAbyCLIENTE (dbindex) 0x0c01


/* TABLE: OBJETIVO 	RECORD LENGTH: 342	*/

# define OBJETIVO        (dbtable) 0x0d00
# define OBJETIVO_CLIENTE (dbfield) 0x0d01	/* Long */
# define OBJETIVO_RESUMEN (dbfield) 0x0d02	/* Long */
# define OBJETIVO_OBJET (dbfield) 0x0d03	/* Integer */
# define OBJETIVO_DESCRIP (dbfield) 0x0d04	/* String */
# define OBJETIVO_TIPCONT (dbfield) 0x0d05	/* Integer */
# define OBJETIVO_FECHAC (dbfield) 0x0d06	/* Date */
# define OBJETIVO_FECHAF (dbfield) 0x0d07	/* Date */
# define OBJETIVO_ACTIVO (dbfield) 0x0d08	/* Integer */
# define OBJETIVO_CALLE (dbfield) 0x0d09	/* String */
# define OBJETIVO_NRO (dbfield) 0x0d0a	/* String */
# define OBJETIVO_PISO (dbfield) 0x0d0b	/* String */
# define OBJETIVO_DEPTO (dbfield) 0x0d0c	/* String */
# define OBJETIVO_LOCAL (dbfield) 0x0d0d	/* Long */
# define OBJETIVO_CODPOS (dbfield) 0x0d0e	/* String */
# define OBJETIVO_PAIS (dbfield) 0x0d0f	/* Integer */
# define OBJETIVO_PROV (dbfield) 0x0d10	/* Integer */
# define OBJETIVO_SVISOR (dbfield) 0x0d11	/* Long */
# define OBJETIVO_INTERN (dbfield) 0x0d12	/* Long */
# define OBJETIVO_CDATE (dbfield) 0x0d13	/* Date */
# define OBJETIVO_CTIME (dbfield) 0x0d14	/* Time */
# define OBJETIVO_CUID (dbfield) 0x0d15	/* Long */
# define OBJETIVO_MDATE (dbfield) 0x0d16	/* Date */
# define OBJETIVO_MTIME (dbfield) 0x0d17	/* Time */
# define OBJETIVO_MUID (dbfield) 0x0d18	/* Long */
# define OBJETIVO_DELEG (dbfield) 0x0d19	/* String */
# define OBJETIVO_EQUIPO (dbfield) 0x0d1a	/* String */
# define OBJETIVO_VENDE (dbfield) 0x0d1b	/* Long */
# define OBJETIVO_RIF (dbfield) 0x0d1c	/* Integer */
# define OBJETIVO_OFPAG (dbfield) 0x0d1d	/* Long */
# define OBJETIVO_DELEGA (dbfield) 0x0d1e	/* String */
# define OBJETIVO_SUBCON (dbfield) 0x0d1f	/* Integer */
# define OBJETIVO_OBJCONOT (dbfield) 0x0d20	/* Integer */
# define OBJETIVO_TELEF1 (dbfield) 0x0d21	/* String */
# define OBJETIVO_TELEF2 (dbfield) 0x0d22	/* String */
# define OBJETIVO_FAX (dbfield) 0x0d23	/* String */
# define OBJETIVO_ZONA (dbfield) 0x0d24	/* Integer */
# define OBJETIVO_INSPEC (dbfield) 0x0d25	/* Integer */
# define OBJETIVO_EMP (dbfield) 0x0d26	/* Integer */
# define OBJETIVO_CONSSTD (dbfield) 0x0d27	/* Integer */
# define OBJETIVO_CONSFER (dbfield) 0x0d28	/* Integer */
# define OBJETIVO_NROCCTE (dbfield) 0x0d29	/* Long */
# define OBJETIVO_CCOSTO (dbfield) 0x0d2a	/* Long */
# define OBJETIVO_CODSER (dbfield) 0x0d2b	/* Integer */
# define OBJETIVO_JURIS (dbfield) 0x0d2c	/* Integer */
# define OBJETIVO_PROGRAM (dbfield) 0x0d2d	/* Long */
# define OBJETIVO_PRESEN (dbfield) 0x0d2e	/* Long */
# define OBJETIVO_FILIAL (dbfield) 0x0d2f	/* String */
# define OBJETIVObyCLIENTE (dbindex) 0x0d00
# define OBJETIVObyEST (dbindex) 0x0d01
# define OBJETIVObyRESUMEN (dbindex) 0x0d02
# define OBJETIVObySVISOR (dbindex) 0x0d03
# define OBJETIVObyNROINT (dbindex) 0x0d04
# define OBJETIVObyDEL (dbindex) 0x0d05
# define OBJETIVObyEQUIPO (dbindex) 0x0d06
# define OBJETIVObyDELCLI (dbindex) 0x0d07
# define OBJETIVObyEMP (dbindex) 0x0d08
# define OBJETIVObyPROGRAM (dbindex) 0x0d09
# define OBJETIVObyPRESEN (dbindex) 0x0d0a
# define OBJETIVObyFILIAL (dbindex) 0x0d0b
# define OBJETIVObyMDATE (dbindex) 0x0d0c


/* TABLE: REFER 	RECORD LENGTH: 203	*/

# define REFER           (dbtable) 0x0e00
# define REFER_CLIENTE (dbfield) 0x0e01	/* Long */
# define REFER_OBJET (dbfield) 0x0e02	/* Integer */
# define REFER_REFER (dbfield) 0x0e03	/* String */
# define REFER_APELLIDO (dbfield) 0x0e04	/* String */
# define REFER_NOMBRE (dbfield) 0x0e05	/* String */
# define REFER_CODDOC (dbfield) 0x0e06	/* String */
# define REFER_NRODOC (dbfield) 0x0e07	/* Long */
# define REFER_TELEF (dbfield) 0x0e08	/* String */
# define REFER_CARGO (dbfield) 0x0e09	/* String */
# define REFER_CODNAC (dbfield) 0x0e0a	/* Integer */
# define REFER_DIREC (dbfield) 0x0e0b	/* String */
# define REFER_PROV (dbfield) 0x0e0c	/* Integer */
# define REFER_LOCAL (dbfield) 0x0e0d	/* Long */
# define REFER_CDATE (dbfield) 0x0e0e	/* Date */
# define REFER_CTIME (dbfield) 0x0e0f	/* Time */
# define REFER_CUID (dbfield) 0x0e10	/* Long */
# define REFER_MDATE (dbfield) 0x0e11	/* Date */
# define REFER_MTIME (dbfield) 0x0e12	/* Time */
# define REFER_MUID (dbfield) 0x0e13	/* Long */
# define REFERbyCLIENTE (dbindex) 0x0e00
# define REFERbyMDATE (dbindex) 0x0e01


/* TABLE: ITMFAC 	RECORD LENGTH: 125	*/

# define ITMFAC          (dbtable) 0x0f00
# define ITMFAC_ITEM (dbfield) 0x0f01	/* Integer */
# define ITMFAC_DESCRIP (dbfield) 0x0f02	/* String */
# define ITMFAC_DESCOR (dbfield) 0x0f03	/* String */
# define ITMFAC_PRECIO (dbfield) 0x0f04	/* Integer */
# define ITMFAC_CSTD (dbfield) 0x0f05	/* Integer */
# define ITMFAC_CANT (dbfield) 0x0f06	/* Integer */
# define ITMFAC_CONCEPTO (dbfield) 0x0f07	/* Integer */
# define ITMFAC_CDATE (dbfield) 0x0f08	/* Date */
# define ITMFAC_CTIME (dbfield) 0x0f09	/* Time */
# define ITMFAC_CUID (dbfield) 0x0f0a	/* Long */
# define ITMFAC_MDATE (dbfield) 0x0f0b	/* Date */
# define ITMFAC_MTIME (dbfield) 0x0f0c	/* Time */
# define ITMFAC_MUID (dbfield) 0x0f0d	/* Long */
# define ITMFAC_SUBCONC (dbfield) 0x0f0e	/* Integer */
# define ITMFAC_CONCPAD (dbfield) 0x0f0f	/* Integer */
# define ITMFAC_PORHORA (dbfield) 0x0f10	/* Integer */
# define ITMFAC_FACTU (dbfield) 0x0f11	/* Integer */
# define ITMFAC_PCERO (dbfield) 0x0f12	/* Integer */
# define ITMFAC_CONCESP (dbfield) 0x0f13	/* Integer */
# define ITMFAC_PADESP (dbfield) 0x0f14	/* Integer */
# define ITMFAC_RIFHORA (dbfield) 0x0f15	/* Integer */
# define ITMFAC_TIPCONC (dbfield) 0x0f16	/* Integer */
# define ITMFACbyITEM (dbindex) 0x0f00
# define ITMFACbyPRECIO (dbindex) 0x0f01
# define ITMFACbyCSTD (dbindex) 0x0f02
# define ITMFACbyCANT (dbindex) 0x0f03
# define ITMFACbyPORCON (dbindex) 0x0f04


/* TABLE: ITMXPUE 	RECORD LENGTH: 89	*/

# define ITMXPUE         (dbtable) 0x1000
# define ITMXPUE_CLIENTE (dbfield) 0x1001	/* Long */
# define ITMXPUE_OBJET (dbfield) 0x1002	/* Integer */
# define ITMXPUE_TIPPTO (dbfield) 0x1003	/* Integer */
# define ITMXPUE_PUESTO (dbfield) 0x1004	/* Integer */
# define ITMXPUE_HINICIO (dbfield) 0x1005	/* Time */
# define ITMXPUE_HFINAL (dbfield) 0x1006	/* Time */
# define ITMXPUE_DIA1 (dbfield) 0x1007	/* String */
# define ITMXPUE_DIA2 (dbfield) 0x1008	/* String */
# define ITMXPUE_DIA3 (dbfield) 0x1009	/* String */
# define ITMXPUE_DIA4 (dbfield) 0x100a	/* String */
# define ITMXPUE_DIA5 (dbfield) 0x100b	/* String */
# define ITMXPUE_DIA6 (dbfield) 0x100c	/* String */
# define ITMXPUE_DIA7 (dbfield) 0x100d	/* String */
# define ITMXPUE_REGIM (dbfield) 0x100e	/* String */
# define ITMXPUE_ITEM (dbfield) 0x100f	/* Integer */
# define ITMXPUE_ESTADO (dbfield) 0x1010	/* Integer */
# define ITMXPUE_CDATE (dbfield) 0x1011	/* Date */
# define ITMXPUE_CTIME (dbfield) 0x1012	/* Time */
# define ITMXPUE_CUID (dbfield) 0x1013	/* Long */
# define ITMXPUE_MDATE (dbfield) 0x1014	/* Date */
# define ITMXPUE_MTIME (dbfield) 0x1015	/* Time */
# define ITMXPUE_MUID (dbfield) 0x1016	/* Long */
# define ITMXPUE_CONSRIF (dbfield) 0x1017	/* Integer */
# define ITMXPUEbyCLIENTE (dbindex) 0x1000


/* TABLE: NMODELO 	RECORD LENGTH: 55	*/

# define NMODELO         (dbtable) 0x1100
# define NMODELO_MOD (dbfield) 0x1101	/* Integer */
# define NMODELO_DESCRIP (dbfield) 0x1102	/* String */
# define NMODELObyMOD (dbindex) 0x1100


/* TABLE: MODCOMP 	RECORD LENGTH: 36	*/

# define MODCOMP         (dbtable) 0x1200
# define MODCOMP_MOD (dbfield) 0x1201	/* Integer */
# define MODCOMP_COMP (dbfield) 0x1202	/* Integer */
# define MODCOMPbyMOD (dbindex) 0x1200


/* TABLE: MTXCONC 	RECORD LENGTH: 71	*/

# define MTXCONC         (dbtable) 0x1300
# define MTXCONC_EMPMT (dbfield) 0x1301	/* Integer */
# define MTXCONC_TIPCOMPMT (dbfield) 0x1302	/* Integer */
# define MTXCONC_SERIEMT (dbfield) 0x1303	/* Integer */
# define MTXCONC_DELEGMT (dbfield) 0x1304	/* String */
# define MTXCONC_NROOTMT (dbfield) 0x1305	/* Long */
# define MTXCONC_CODCONC (dbfield) 0x1306	/* Integer */
# define MTXCONC_SUBCONC (dbfield) 0x1307	/* Integer */
# define MTXCONC_VALN (dbfield) 0x1308	/* Long */
# define MTXCONC_CDATE (dbfield) 0x1309	/* Date */
# define MTXCONC_CTIME (dbfield) 0x130a	/* Time */
# define MTXCONC_CUID (dbfield) 0x130b	/* Long */
# define MTXCONC_MDATE (dbfield) 0x130c	/* Date */
# define MTXCONC_MTIME (dbfield) 0x130d	/* Time */
# define MTXCONC_MUID (dbfield) 0x130e	/* Long */
# define MTXCONC_FECREG (dbfield) 0x130f	/* Date */
# define MTXCONCbyEMPMT (dbindex) 0x1300


/* TABLE: SUBREGIM 	RECORD LENGTH: 115	*/

# define SUBREGIM        (dbtable) 0x1400
# define SUBREGIM_SUBREG (dbfield) 0x1401	/* String */
# define SUBREGIM_DESCRIP (dbfield) 0x1402	/* String */
# define SUBREGIM_HSNORM (dbfield) 0x1403	/* Integer */
# define SUBREGIM_HSEXTRA (dbfield) 0x1404	/* Integer */
# define SUBREGIM_HSSEM (dbfield) 0x1405	/* Long */
# define SUBREGIM_PORHSN (dbfield) 0x1406	/* Long */
# define SUBREGIM_PORHSE (dbfield) 0x1407	/* Long */
# define SUBREGIM_CDATE (dbfield) 0x1408	/* Date */
# define SUBREGIM_CTIME (dbfield) 0x1409	/* Time */
# define SUBREGIM_CUID (dbfield) 0x140a	/* Long */
# define SUBREGIM_MDATE (dbfield) 0x140b	/* Date */
# define SUBREGIM_MTIME (dbfield) 0x140c	/* Time */
# define SUBREGIM_MUID (dbfield) 0x140d	/* Long */
# define SUBREGIM_EMP (dbfield) 0x140e	/* Integer */
# define SUBREGIMbySUBREG (dbindex) 0x1400
# define SUBREGIMbyEMP (dbindex) 0x1401


/* TABLE: PROVXDEL 	RECORD LENGTH: 55	*/

# define PROVXDEL        (dbtable) 0x1500
# define PROVXDEL_DELEG (dbfield) 0x1501	/* String */
# define PROVXDEL_PROV (dbfield) 0x1502	/* Integer */
# define PROVXDEL_CDATE (dbfield) 0x1503	/* Date */
# define PROVXDEL_CTIME (dbfield) 0x1504	/* Time */
# define PROVXDEL_CUID (dbfield) 0x1505	/* Long */
# define PROVXDEL_MDATE (dbfield) 0x1506	/* Date */
# define PROVXDEL_MTIME (dbfield) 0x1507	/* Time */
# define PROVXDEL_MUID (dbfield) 0x1508	/* Long */
# define PROVXDELbyDELEG (dbindex) 0x1500
# define PROVXDELbyPROV (dbindex) 0x1501


/* TABLE: PROVXCC 	RECORD LENGTH: 53	*/

# define PROVXCC         (dbtable) 0x1600
# define PROVXCC_EMP (dbfield) 0x1601	/* Integer */
# define PROVXCC_PAIS (dbfield) 0x1602	/* Integer */
# define PROVXCC_PROV (dbfield) 0x1603	/* Integer */
# define PROVXCC_TIPCCTO (dbfield) 0x1604	/* Integer */
# define PROVXCC_CDATE (dbfield) 0x1605	/* Date */
# define PROVXCC_CTIME (dbfield) 0x1606	/* Time */
# define PROVXCC_CUID (dbfield) 0x1607	/* Long */
# define PROVXCC_MDATE (dbfield) 0x1608	/* Date */
# define PROVXCC_MTIME (dbfield) 0x1609	/* Time */
# define PROVXCC_MUID (dbfield) 0x160a	/* Long */
# define PROVXCCbyTIPCCTO (dbindex) 0x1600
# define PROVXCCbyPROV (dbindex) 0x1601


/* TABLE: RELCLI 	RECORD LENGTH: 56	*/

# define RELCLI          (dbtable) 0x1700
# define RELCLI_CLIPADRE (dbfield) 0x1701	/* Long */
# define RELCLI_CLIHIJO (dbfield) 0x1702	/* Long */
# define RELCLI_CDATE (dbfield) 0x1703	/* Date */
# define RELCLI_CTIME (dbfield) 0x1704	/* Time */
# define RELCLI_CUID (dbfield) 0x1705	/* Long */
# define RELCLI_MDATE (dbfield) 0x1706	/* Date */
# define RELCLI_MTIME (dbfield) 0x1707	/* Time */
# define RELCLI_MUID (dbfield) 0x1708	/* Long */
# define RELCLIbyCLIPADRE (dbindex) 0x1700
# define RELCLIbyCLIHIJO (dbindex) 0x1701


/* TABLE: FERCLI 	RECORD LENGTH: 83	*/

# define FERCLI          (dbtable) 0x1800
# define FERCLI_CLIENTE (dbfield) 0x1801	/* Long */
# define FERCLI_OBJET (dbfield) 0x1802	/* Integer */
# define FERCLI_FECHA (dbfield) 0x1803	/* Date */
# define FERCLI_DESCRIP (dbfield) 0x1804	/* String */
# define FERCLI_FERIADO (dbfield) 0x1805	/* Integer */
# define FERCLI_CDATE (dbfield) 0x1806	/* Date */
# define FERCLI_CTIME (dbfield) 0x1807	/* Time */
# define FERCLI_CUID (dbfield) 0x1808	/* Long */
# define FERCLI_MDATE (dbfield) 0x1809	/* Date */
# define FERCLI_MTIME (dbfield) 0x180a	/* Time */
# define FERCLI_MUID (dbfield) 0x180b	/* Long */
# define FERCLIbyCLIENTE (dbindex) 0x1800
# define FERCLIbyMDATE (dbindex) 0x1801


/* TABLE: ITMXPTO 	RECORD LENGTH: 52	*/

# define ITMXPTO         (dbtable) 0x1900
# define ITMXPTO_TIPPTO (dbfield) 0x1901	/* Integer */
# define ITMXPTO_ITEM (dbfield) 0x1902	/* Integer */
# define ITMXPTO_CDATE (dbfield) 0x1903	/* Date */
# define ITMXPTO_CTIME (dbfield) 0x1904	/* Time */
# define ITMXPTO_CUID (dbfield) 0x1905	/* Long */
# define ITMXPTO_MDATE (dbfield) 0x1906	/* Date */
# define ITMXPTO_MTIME (dbfield) 0x1907	/* Time */
# define ITMXPTO_MUID (dbfield) 0x1908	/* Long */
# define ITMXPTObyTIPPTO (dbindex) 0x1900


/* TABLE: CONVIG 	RECORD LENGTH: 34	*/

# define CONVIG          (dbtable) 0x1a00
# define CONVIG_EMP (dbfield) 0x1a01	/* Integer */
# define CONVIG_RELACION (dbfield) 0x1a02	/* Integer */
# define CONVIGbyEMP (dbindex) 0x1a00


/* TABLE: TIPOMA 	RECORD LENGTH: 85	*/

# define TIPOMA          (dbtable) 0x1b00
# define TIPOMA_TIPOMA (dbfield) 0x1b01	/* Integer */
# define TIPOMA_DESCRIP (dbfield) 0x1b02	/* String */
# define TIPOMAbyTIPOMA (dbindex) 0x1b00


/* TABLE: MAXTIPO 	RECORD LENGTH: 54	*/

# define MAXTIPO         (dbtable) 0x1c00
# define MAXTIPO_NROINT (dbfield) 0x1c01	/* Long */
# define MAXTIPO_TIPOMA (dbfield) 0x1c02	/* Integer */
# define MAXTIPO_CDATE (dbfield) 0x1c03	/* Date */
# define MAXTIPO_CTIME (dbfield) 0x1c04	/* Time */
# define MAXTIPO_CUID (dbfield) 0x1c05	/* Long */
# define MAXTIPO_MDATE (dbfield) 0x1c06	/* Date */
# define MAXTIPO_MTIME (dbfield) 0x1c07	/* Time */
# define MAXTIPO_MUID (dbfield) 0x1c08	/* Long */
# define MAXTIPObyNROINT (dbindex) 0x1c00
# define MAXTIPObyTIPOMA (dbindex) 0x1c01


/* TABLE: FILIAL 	RECORD LENGTH: 112	*/

# define FILIAL          (dbtable) 0x1d00
# define FILIAL_FILIAL (dbfield) 0x1d01	/* String */
# define FILIAL_DELEG (dbfield) 0x1d02	/* String */
# define FILIAL_DESCRIP (dbfield) 0x1d03	/* String */
# define FILIAL_CDATE (dbfield) 0x1d04	/* Date */
# define FILIAL_CTIME (dbfield) 0x1d05	/* Time */
# define FILIAL_CUID (dbfield) 0x1d06	/* Long */
# define FILIAL_MDATE (dbfield) 0x1d07	/* Date */
# define FILIAL_MTIME (dbfield) 0x1d08	/* Time */
# define FILIAL_MUID (dbfield) 0x1d09	/* Long */
# define FILIALbyFILIAL (dbindex) 0x1d00
# define FILIALbyDELEG (dbindex) 0x1d01


/* TABLE: USRXFIL 	RECORD LENGTH: 57	*/

# define USRXFIL         (dbtable) 0x1e00
# define USRXFIL_FILIAL (dbfield) 0x1e01	/* String */
# define USRXFIL_CODUSU (dbfield) 0x1e02	/* Integer */
# define USRXFIL_CDATE (dbfield) 0x1e03	/* Date */
# define USRXFIL_CTIME (dbfield) 0x1e04	/* Time */
# define USRXFIL_CUID (dbfield) 0x1e05	/* Long */
# define USRXFIL_MDATE (dbfield) 0x1e06	/* Date */
# define USRXFIL_MTIME (dbfield) 0x1e07	/* Time */
# define USRXFIL_MUID (dbfield) 0x1e08	/* Long */
# define USRXFILbyFILIAL (dbindex) 0x1e00
# define USRXFILbyUSUARIO (dbindex) 0x1e01


/* TABLE: TPTOXEMP 	RECORD LENGTH: 35	*/

# define TPTOXEMP        (dbtable) 0x1f00
# define TPTOXEMP_EMP (dbfield) 0x1f01	/* Integer */
# define TPTOXEMP_TIPPTO (dbfield) 0x1f02	/* Integer */
# define TPTOXEMPbyEMP (dbindex) 0x1f00


/* TABLE: CIEXEMP 	RECORD LENGTH: 36	*/

# define CIEXEMP         (dbtable) 0x2000
# define CIEXEMP_EMP (dbfield) 0x2001	/* Integer */
# define CIEXEMP_TIPCIE (dbfield) 0x2002	/* Integer */
# define CIEXEMP_FECCIE (dbfield) 0x2003	/* Date */
# define CIEXEMPbyEMP (dbindex) 0x2000


/* TABLE: CONDPTO 	RECORD LENGTH: 101	*/

# define CONDPTO         (dbtable) 0x2100
# define CONDPTO_CLIENTE (dbfield) 0x2101	/* Long */
# define CONDPTO_OBJET (dbfield) 0x2102	/* Integer */
# define CONDPTO_TIPPTO (dbfield) 0x2103	/* Integer */
# define CONDPTO_PUESTO (dbfield) 0x2104	/* Integer */
# define CONDPTO_HINICIO (dbfield) 0x2105	/* Time */
# define CONDPTO_HFINAL (dbfield) 0x2106	/* Time */
# define CONDPTO_DIA1 (dbfield) 0x2107	/* String */
# define CONDPTO_DIA2 (dbfield) 0x2108	/* String */
# define CONDPTO_DIA3 (dbfield) 0x2109	/* String */
# define CONDPTO_DIA4 (dbfield) 0x210a	/* String */
# define CONDPTO_DIA5 (dbfield) 0x210b	/* String */
# define CONDPTO_DIA6 (dbfield) 0x210c	/* String */
# define CONDPTO_DIA7 (dbfield) 0x210d	/* String */
# define CONDPTO_REGIM (dbfield) 0x210e	/* String */
# define CONDPTO_SALARIO (dbfield) 0x210f	/* Long */
# define CONDPTO_CANTPUE (dbfield) 0x2110	/* Integer */
# define CONDPTO_CANTVIG (dbfield) 0x2111	/* Integer */
# define CONDPTO_ESTADO (dbfield) 0x2112	/* String */
# define CONDPTO_FECMOV (dbfield) 0x2113	/* Date */
# define CONDPTO_NROOT (dbfield) 0x2114	/* Long */
# define CONDPTO_CDATE (dbfield) 0x2115	/* Date */
# define CONDPTO_CTIME (dbfield) 0x2116	/* Time */
# define CONDPTO_CUID (dbfield) 0x2117	/* Long */
# define CONDPTO_MDATE (dbfield) 0x2118	/* Date */
# define CONDPTO_MTIME (dbfield) 0x2119	/* Time */
# define CONDPTO_MUID (dbfield) 0x211a	/* Long */
# define CONDPTObyCLIENTE (dbindex) 0x2100
# define CONDPTObyFECHA (dbindex) 0x2101
# define CONDPTObyLUGAR (dbindex) 0x2102
# define CONDPTObyPUESTO (dbindex) 0x2103


/* TABLE: CONDEQ 	RECORD LENGTH: 70	*/

# define CONDEQ          (dbtable) 0x2200
# define CONDEQ_CLIENTE (dbfield) 0x2201	/* Long */
# define CONDEQ_OBJET (dbfield) 0x2202	/* Integer */
# define CONDEQ_NROINT (dbfield) 0x2203	/* Long */
# define CONDEQ_NRORENG (dbfield) 0x2204	/* Integer */
# define CONDEQ_CANTI (dbfield) 0x2205	/* Integer */
# define CONDEQ_ESTADO (dbfield) 0x2206	/* String */
# define CONDEQ_FECMOV (dbfield) 0x2207	/* Date */
# define CONDEQ_NROOT (dbfield) 0x2208	/* Long */
# define CONDEQ_CDATE (dbfield) 0x2209	/* Date */
# define CONDEQ_CTIME (dbfield) 0x220a	/* Time */
# define CONDEQ_CUID (dbfield) 0x220b	/* Long */
# define CONDEQ_MDATE (dbfield) 0x220c	/* Date */
# define CONDEQ_MTIME (dbfield) 0x220d	/* Time */
# define CONDEQ_MUID (dbfield) 0x220e	/* Long */
# define CONDEQbyCLIENTE (dbindex) 0x2200
# define CONDEQbyFECHA (dbindex) 0x2201


/* TABLE: CONDTAR 	RECORD LENGTH: 70	*/

# define CONDTAR         (dbtable) 0x2300
# define CONDTAR_CLIENTE (dbfield) 0x2301	/* Long */
# define CONDTAR_OBJET (dbfield) 0x2302	/* Integer */
# define CONDTAR_CONC (dbfield) 0x2303	/* Integer */
# define CONDTAR_PRECIO (dbfield) 0x2304	/* Long */
# define CONDTAR_HORAS (dbfield) 0x2305	/* Long */
# define CONDTAR_FECMOV (dbfield) 0x2306	/* Date */
# define CONDTAR_NROOT (dbfield) 0x2307	/* Long */
# define CONDTAR_CDATE (dbfield) 0x2308	/* Date */
# define CONDTAR_CTIME (dbfield) 0x2309	/* Time */
# define CONDTAR_CUID (dbfield) 0x230a	/* Long */
# define CONDTAR_MDATE (dbfield) 0x230b	/* Date */
# define CONDTAR_MTIME (dbfield) 0x230c	/* Time */
# define CONDTAR_MUID (dbfield) 0x230d	/* Long */
# define CONDTARbyCLIENTE (dbindex) 0x2300
# define CONDTARbyFECHA (dbindex) 0x2301
# define CONDTARbyCONC (dbindex) 0x2302


/* TABLE: GRUPOPTO 	RECORD LENGTH: 76	*/

# define GRUPOPTO        (dbtable) 0x2400
# define GRUPOPTO_GRUPO (dbfield) 0x2401	/* Integer */
# define GRUPOPTO_DESCRIP (dbfield) 0x2402	/* String */
# define GRUPOPTO_CDATE (dbfield) 0x2403	/* Date */
# define GRUPOPTO_CTIME (dbfield) 0x2404	/* Time */
# define GRUPOPTO_CUID (dbfield) 0x2405	/* Long */
# define GRUPOPTO_MDATE (dbfield) 0x2406	/* Date */
# define GRUPOPTO_MTIME (dbfield) 0x2407	/* Time */
# define GRUPOPTO_MUID (dbfield) 0x2408	/* Long */
# define GRUPOPTObyGRUPO (dbindex) 0x2400


/* TABLE: BONOS 	RECORD LENGTH: 98	*/

# define BONOS           (dbtable) 0x2500
# define BONOS_BONO (dbfield) 0x2501	/* Integer */
# define BONOS_DESCRIP (dbfield) 0x2502	/* String */
# define BONOS_DESCOR (dbfield) 0x2503	/* String */
# define BONOS_NIVEL (dbfield) 0x2504	/* Integer */
# define BONOS_CDATE (dbfield) 0x2505	/* Date */
# define BONOS_CTIME (dbfield) 0x2506	/* Time */
# define BONOS_CUID (dbfield) 0x2507	/* Long */
# define BONOS_MDATE (dbfield) 0x2508	/* Date */
# define BONOS_MTIME (dbfield) 0x2509	/* Time */
# define BONOS_MUID (dbfield) 0x250a	/* Long */
# define BONOSbyBONO (dbindex) 0x2500
# define BONOSbyNIVEL (dbindex) 0x2501


/* TABLE: LISTABON 	RECORD LENGTH: 65	*/

# define LISTABON        (dbtable) 0x2600
# define LISTABON_CLIENTE (dbfield) 0x2601	/* Long */
# define LISTABON_OBJETIVO (dbfield) 0x2602	/* Integer */
# define LISTABON_BONO (dbfield) 0x2603	/* Integer */
# define LISTABON_FECVIG (dbfield) 0x2604	/* Date */
# define LISTABON_PRECIO (dbfield) 0x2605	/* Long */
# define LISTABON_REMUN (dbfield) 0x2606	/* Long */
# define LISTABON_CDATE (dbfield) 0x2607	/* Date */
# define LISTABON_CTIME (dbfield) 0x2608	/* Time */
# define LISTABON_CUID (dbfield) 0x2609	/* Long */
# define LISTABON_MDATE (dbfield) 0x260a	/* Date */
# define LISTABON_MTIME (dbfield) 0x260b	/* Time */
# define LISTABON_MUID (dbfield) 0x260c	/* Long */
# define LISTABONbyCLIENTE (dbindex) 0x2600
# define LISTABONbyFECVIG (dbindex) 0x2601


/* TABLE: PRECATE 	RECORD LENGTH: 51	*/

# define PRECATE         (dbtable) 0x2700
# define PRECATE_COD (dbfield) 0x2701	/* Integer */
# define PRECATE_CODCAT (dbfield) 0x2702	/* Integer */
# define PRECATE_CDATE (dbfield) 0x2703	/* Date */
# define PRECATE_CTIME (dbfield) 0x2704	/* Time */
# define PRECATE_CUID (dbfield) 0x2705	/* Long */
# define PRECATE_MDATE (dbfield) 0x2706	/* Date */
# define PRECATE_MTIME (dbfield) 0x2707	/* Time */
# define PRECATE_MUID (dbfield) 0x2708	/* Long */
# define PRECATEbyCOD (dbindex) 0x2700


/* TABLE: CATEXPTO 	RECORD LENGTH: 54	*/

# define CATEXPTO        (dbtable) 0x2800
# define CATEXPTO_TIPPTO (dbfield) 0x2801	/* Integer */
# define CATEXPTO_RELACION (dbfield) 0x2802	/* Integer */
# define CATEXPTO_CODCAT (dbfield) 0x2803	/* Integer */
# define CATEXPTO_ACTIVO (dbfield) 0x2804	/* Integer */
# define CATEXPTO_CDATE (dbfield) 0x2805	/* Date */
# define CATEXPTO_CTIME (dbfield) 0x2806	/* Time */
# define CATEXPTO_CUID (dbfield) 0x2807	/* Long */
# define CATEXPTO_MDATE (dbfield) 0x2808	/* Date */
# define CATEXPTO_MTIME (dbfield) 0x2809	/* Time */
# define CATEXPTO_MUID (dbfield) 0x280a	/* Long */
# define CATEXPTObyTIPPTO (dbindex) 0x2800


/* TABLE: RANKVEN 	RECORD LENGTH: 44	*/

# define RANKVEN         (dbtable) 0x2900
# define RANKVEN_VENDE (dbfield) 0x2901	/* Long */
# define RANKVEN_RIFF (dbfield) 0x2902	/* Float */
# define RANKVENbyVENDE (dbindex) 0x2900


/* TABLE: RIFACUM 	RECORD LENGTH: 46	*/

# define RIFACUM         (dbtable) 0x2a00
# define RIFACUM_CLIENTE (dbfield) 0x2a01	/* Long */
# define RIFACUM_OBJET (dbfield) 0x2a02	/* Integer */
# define RIFACUM_RIFF (dbfield) 0x2a03	/* Float */
# define RIFACUMbyCLIENTE (dbindex) 0x2a00


/* TABLE: BONOT 	RECORD LENGTH: 81	*/

# define BONOT           (dbtable) 0x2b00
# define BONOT_EMP (dbfield) 0x2b01	/* Integer */
# define BONOT_TIPCOMP (dbfield) 0x2b02	/* Integer */
# define BONOT_SERIE (dbfield) 0x2b03	/* Integer */
# define BONOT_DELEG (dbfield) 0x2b04	/* String */
# define BONOT_NROOT (dbfield) 0x2b05	/* Long */
# define BONOT_BONO (dbfield) 0x2b06	/* Integer */
# define BONOT_CLIENTE (dbfield) 0x2b07	/* Long */
# define BONOT_OBJET (dbfield) 0x2b08	/* Integer */
# define BONOT_CANTI (dbfield) 0x2b09	/* Integer */
# define BONOT_PRECIO (dbfield) 0x2b0a	/* Long */
# define BONOT_REMUN (dbfield) 0x2b0b	/* Long */
# define BONOT_COND (dbfield) 0x2b0c	/* String */
# define BONOT_CDATE (dbfield) 0x2b0d	/* Date */
# define BONOT_CTIME (dbfield) 0x2b0e	/* Time */
# define BONOT_CUID (dbfield) 0x2b0f	/* Long */
# define BONOT_MDATE (dbfield) 0x2b10	/* Date */
# define BONOT_MTIME (dbfield) 0x2b11	/* Time */
# define BONOT_MUID (dbfield) 0x2b12	/* Long */
# define BONOTbyEMP (dbindex) 0x2b00
# define BONOTbyCLIENTE (dbindex) 0x2b01


/* TABLE: BONXPUE 	RECORD LENGTH: 85	*/

# define BONXPUE         (dbtable) 0x2c00
# define BONXPUE_EMP (dbfield) 0x2c01	/* Integer */
# define BONXPUE_TIPCOMP (dbfield) 0x2c02	/* Integer */
# define BONXPUE_SERIE (dbfield) 0x2c03	/* Integer */
# define BONXPUE_DELEG (dbfield) 0x2c04	/* String */
# define BONXPUE_NROOT (dbfield) 0x2c05	/* Long */
# define BONXPUE_TIPPTO (dbfield) 0x2c06	/* Integer */
# define BONXPUE_NRORENG (dbfield) 0x2c07	/* Integer */
# define BONXPUE_BONO (dbfield) 0x2c08	/* Integer */
# define BONXPUE_CLIENTE (dbfield) 0x2c09	/* Long */
# define BONXPUE_OBJET (dbfield) 0x2c0a	/* Integer */
# define BONXPUE_CANT (dbfield) 0x2c0b	/* Integer */
# define BONXPUE_PRECIO (dbfield) 0x2c0c	/* Long */
# define BONXPUE_REMUN (dbfield) 0x2c0d	/* Long */
# define BONXPUE_COND (dbfield) 0x2c0e	/* String */
# define BONXPUE_CDATE (dbfield) 0x2c0f	/* Date */
# define BONXPUE_CTIME (dbfield) 0x2c10	/* Time */
# define BONXPUE_CUID (dbfield) 0x2c11	/* Long */
# define BONXPUE_MDATE (dbfield) 0x2c12	/* Date */
# define BONXPUE_MTIME (dbfield) 0x2c13	/* Time */
# define BONXPUE_MUID (dbfield) 0x2c14	/* Long */
# define BONXPUEbyEMP (dbindex) 0x2c00
# define BONXPUEbyCLIENTE (dbindex) 0x2c01
# define BONXPUEbyNROOT (dbindex) 0x2c02


/* TABLE: LUPAXDEL 	RECORD LENGTH: 43	*/

# define LUPAXDEL        (dbtable) 0x2d00
# define LUPAXDEL_EMP (dbfield) 0x2d01	/* Integer */
# define LUPAXDEL_DELEGA (dbfield) 0x2d02	/* String */
# define LUPAXDEL_OFPAG (dbfield) 0x2d03	/* Long */
# define LUPAXDELbyEMP (dbindex) 0x2d00


/* TABLE: ZONAINS 	RECORD LENGTH: 96	*/

# define ZONAINS         (dbtable) 0x2e00
# define ZONAINS_ZONA (dbfield) 0x2e01	/* Integer */
# define ZONAINS_DESCRIP (dbfield) 0x2e02	/* String */
# define ZONAINS_DESCOR (dbfield) 0x2e03	/* String */
# define ZONAINS_CDATE (dbfield) 0x2e04	/* Date */
# define ZONAINS_CTIME (dbfield) 0x2e05	/* Time */
# define ZONAINS_CUID (dbfield) 0x2e06	/* Long */
# define ZONAINS_MDATE (dbfield) 0x2e07	/* Date */
# define ZONAINS_MTIME (dbfield) 0x2e08	/* Time */
# define ZONAINS_MUID (dbfield) 0x2e09	/* Long */
# define ZONAINSbyZONA (dbindex) 0x2e00


/* TABLE: EMPXREG 	RECORD LENGTH: 39	*/

# define EMPXREG         (dbtable) 0x2f00
# define EMPXREG_EMP (dbfield) 0x2f01	/* Integer */
# define EMPXREG_DIAS (dbfield) 0x2f02	/* Integer */
# define EMPXREG_DFRAN (dbfield) 0x2f03	/* Integer */
# define EMPXREG_HSREG (dbfield) 0x2f04	/* Integer */
# define EMPXREG_PARTIME (dbfield) 0x2f05	/* Integer */
# define EMPXREG_REGESP (dbfield) 0x2f06	/* Integer */
# define EMPXREGbyEMP (dbindex) 0x2f00
# define EMPXREGbyPTIME (dbindex) 0x2f01


/* TABLE: REGXSUB 	RECORD LENGTH: 47	*/

# define REGXSUB         (dbtable) 0x3000
# define REGXSUB_EMP (dbfield) 0x3001	/* Integer */
# define REGXSUB_DIAS (dbfield) 0x3002	/* Integer */
# define REGXSUB_DFRAN (dbfield) 0x3003	/* Integer */
# define REGXSUB_HSREG (dbfield) 0x3004	/* Integer */
# define REGXSUB_PARTIME (dbfield) 0x3005	/* Integer */
# define REGXSUB_SUBREG (dbfield) 0x3006	/* String */
# define REGXSUBbyEMP (dbindex) 0x3000
# define REGXSUBbySUBREG (dbindex) 0x3001
# define REGXSUBbyPTIME (dbindex) 0x3002


/* TABLE: PLANINS 	RECORD LENGTH: 60	*/

# define PLANINS         (dbtable) 0x3100
# define PLANINS_CLIENTE (dbfield) 0x3101	/* Long */
# define PLANINS_OBJET (dbfield) 0x3102	/* Integer */
# define PLANINS_INSPDIU (dbfield) 0x3103	/* Integer */
# define PLANINS_INSPNOC (dbfield) 0x3104	/* Integer */
# define PLANINS_CDATE (dbfield) 0x3105	/* Date */
# define PLANINS_CTIME (dbfield) 0x3106	/* Time */
# define PLANINS_CUID (dbfield) 0x3107	/* Long */
# define PLANINS_MDATE (dbfield) 0x3108	/* Date */
# define PLANINS_MTIME (dbfield) 0x3109	/* Time */
# define PLANINS_MUID (dbfield) 0x310a	/* Long */
# define PLANINS_FINICIO (dbfield) 0x310b	/* Date */
# define PLANINSbyCLIENTE (dbindex) 0x3100


/* TABLE: ZONA 	RECORD LENGTH: 97	*/

# define ZONA            (dbtable) 0x3200
# define ZONA_PROV (dbfield) 0x3201	/* Integer */
# define ZONA_ZONA (dbfield) 0x3202	/* Integer */
# define ZONA_DESCRIP (dbfield) 0x3203	/* String */
# define ZONA_DESCOR (dbfield) 0x3204	/* String */
# define ZONA_CDATE (dbfield) 0x3205	/* Date */
# define ZONA_CTIME (dbfield) 0x3206	/* Time */
# define ZONA_CUID (dbfield) 0x3207	/* Long */
# define ZONA_MDATE (dbfield) 0x3208	/* Date */
# define ZONA_MTIME (dbfield) 0x3209	/* Time */
# define ZONA_MUID (dbfield) 0x320a	/* Long */
# define ZONAbyPROV (dbindex) 0x3200


/* TABLE: PROVXZON 	RECORD LENGTH: 55	*/

# define PROVXZON        (dbtable) 0x3300
# define PROVXZON_EMP (dbfield) 0x3301	/* Integer */
# define PROVXZON_PROV (dbfield) 0x3302	/* Integer */
# define PROVXZON_ZONAG (dbfield) 0x3303	/* Integer */
# define PROVXZON_LOCAL (dbfield) 0x3304	/* Long */
# define PROVXZON_CDATE (dbfield) 0x3305	/* Date */
# define PROVXZON_CTIME (dbfield) 0x3306	/* Time */
# define PROVXZON_CUID (dbfield) 0x3307	/* Long */
# define PROVXZON_MDATE (dbfield) 0x3308	/* Date */
# define PROVXZON_MTIME (dbfield) 0x3309	/* Time */
# define PROVXZON_MUID (dbfield) 0x330a	/* Long */
# define PROVXZONbyEMP (dbindex) 0x3300
# define PROVXZONbyLOC (dbindex) 0x3301


/* TABLE: OBSRECH 	RECORD LENGTH: 134	*/

# define OBSRECH         (dbtable) 0x3400
# define OBSRECH_EMP (dbfield) 0x3401	/* Integer */
# define OBSRECH_TIPCOMP (dbfield) 0x3402	/* Integer */
# define OBSRECH_SERIE (dbfield) 0x3403	/* Integer */
# define OBSRECH_DELEG (dbfield) 0x3404	/* String */
# define OBSRECH_NROOT (dbfield) 0x3405	/* Long */
# define OBSRECH_NRORENG (dbfield) 0x3406	/* Integer */
# define OBSRECH_DESCRIP (dbfield) 0x3407	/* String */
# define OBSRECH_CDATE (dbfield) 0x3408	/* Date */
# define OBSRECH_CTIME (dbfield) 0x3409	/* Time */
# define OBSRECH_CUID (dbfield) 0x340a	/* Long */
# define OBSRECH_MDATE (dbfield) 0x340b	/* Date */
# define OBSRECH_MTIME (dbfield) 0x340c	/* Time */
# define OBSRECH_MUID (dbfield) 0x340d	/* Long */
# define OBSRECHbyEMP (dbindex) 0x3400


/* TABLE: SUCXOBJ 	RECORD LENGTH: 72	*/

# define SUCXOBJ         (dbtable) 0x3500
# define SUCXOBJ_EMP (dbfield) 0x3501	/* Integer */
# define SUCXOBJ_CLIENTE (dbfield) 0x3502	/* Long */
# define SUCXOBJ_OBJETIVO (dbfield) 0x3503	/* Integer */
# define SUCXOBJ_SUCURSAL (dbfield) 0x3504	/* Integer */
# define SUCXOBJ_DESCRIP (dbfield) 0x3505	/* String */
# define SUCXOBJbyEMP (dbindex) 0x3500

# define IO_COMERC_CHKSUM (long) 0x5501
