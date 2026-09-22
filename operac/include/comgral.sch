/* SCHEMA: comgral Esquema de Novia - Comercial Gral	*/


/* TABLE: IPC 	RECORD LENGTH: 82	*/

# define IPC             (dbtable) 0x0100
# define IPC_IPC (dbfield) 0x0101	/* Integer */
# define IPC_DESCRIP (dbfield) 0x0102	/* String */
# define IPC_TIPIND (dbfield) 0x0103	/* Integer */
# define IPC_MESES (dbfield) 0x0104	/* Integer */
# define IPC_CDATE (dbfield) 0x0105	/* Date */
# define IPC_CTIME (dbfield) 0x0106	/* Time */
# define IPC_CUID (dbfield) 0x0107	/* Long */
# define IPC_MDATE (dbfield) 0x0108	/* Date */
# define IPC_MTIME (dbfield) 0x0109	/* Time */
# define IPC_MUID (dbfield) 0x010a	/* Long */
# define IPCbyIPC (dbindex) 0x0100


/* TABLE: IPCCLI 	RECORD LENGTH: 57	*/

# define IPCCLI          (dbtable) 0x0200
# define IPCCLI_CLIENTE (dbfield) 0x0201	/* Long */
# define IPCCLI_FECHAVIG (dbfield) 0x0202	/* Date */
# define IPCCLI_IPC (dbfield) 0x0203	/* Integer */
# define IPCCLI_MESINI (dbfield) 0x0204	/* Integer */
# define IPCCLI_ACTIVO (dbfield) 0x0205	/* Integer */
# define IPCCLI_CDATE (dbfield) 0x0206	/* Date */
# define IPCCLI_CTIME (dbfield) 0x0207	/* Time */
# define IPCCLI_CUID (dbfield) 0x0208	/* Long */
# define IPCCLI_MDATE (dbfield) 0x0209	/* Date */
# define IPCCLI_MTIME (dbfield) 0x020a	/* Time */
# define IPCCLI_MUID (dbfield) 0x020b	/* Long */
# define IPCCLIbyCLIENTE (dbindex) 0x0200


/* TABLE: INDOBJ 	RECORD LENGTH: 76	*/

# define INDOBJ          (dbtable) 0x0300
# define INDOBJ_CLIENTE (dbfield) 0x0301	/* Long */
# define INDOBJ_OBJET (dbfield) 0x0302	/* Integer */
# define INDOBJ_FECHAVIG (dbfield) 0x0303	/* Date */
# define INDOBJ_PORC (dbfield) 0x0304	/* Long */
# define INDOBJ_MESES (dbfield) 0x0305	/* Integer */
# define INDOBJ_MESANIV (dbfield) 0x0306	/* String */
# define INDOBJ_ACTIVO (dbfield) 0x0307	/* Integer */
# define INDOBJ_TIPIND (dbfield) 0x0308	/* Integer */
# define INDOBJ_CDATE (dbfield) 0x0309	/* Date */
# define INDOBJ_CTIME (dbfield) 0x030a	/* Time */
# define INDOBJ_CUID (dbfield) 0x030b	/* Long */
# define INDOBJ_MDATE (dbfield) 0x030c	/* Date */
# define INDOBJ_MTIME (dbfield) 0x030d	/* Time */
# define INDOBJ_MUID (dbfield) 0x030e	/* Long */
# define INDOBJbyCLIENTE (dbindex) 0x0300
# define INDOBJbyTIPIND (dbindex) 0x0301


/* TABLE: INDCPTO 	RECORD LENGTH: 80	*/

# define INDCPTO         (dbtable) 0x0400
# define INDCPTO_CLIENTE (dbfield) 0x0401	/* Long */
# define INDCPTO_OBJET (dbfield) 0x0402	/* Integer */
# define INDCPTO_CONCEPTO (dbfield) 0x0403	/* Integer */
# define INDCPTO_FECHAVIG (dbfield) 0x0404	/* Date */
# define INDCPTO_PORC (dbfield) 0x0405	/* Long */
# define INDCPTO_VALOR (dbfield) 0x0406	/* Float */
# define INDCPTO_ACTIVO (dbfield) 0x0407	/* Integer */
# define INDCPTO_NEWVAL (dbfield) 0x0408	/* Float */
# define INDCPTO_PROCESADO (dbfield) 0x0409	/* Integer */
# define INDCPTO_CDATE (dbfield) 0x040a	/* Date */
# define INDCPTO_CTIME (dbfield) 0x040b	/* Time */
# define INDCPTO_CUID (dbfield) 0x040c	/* Long */
# define INDCPTO_MDATE (dbfield) 0x040d	/* Date */
# define INDCPTO_MTIME (dbfield) 0x040e	/* Time */
# define INDCPTO_MUID (dbfield) 0x040f	/* Long */
# define INDCPTObyCLIENTE (dbindex) 0x0400


/* TABLE: CONDCOM 	RECORD LENGTH: 76	*/

# define CONDCOM         (dbtable) 0x0500
# define CONDCOM_EMP (dbfield) 0x0501	/* Integer */
# define CONDCOM_CONDIC (dbfield) 0x0502	/* Integer */
# define CONDCOM_DESCRIP (dbfield) 0x0503	/* String */
# define CONDCOM_ABREV (dbfield) 0x0504	/* String */
# define CONDCOMbyEMP (dbindex) 0x0500


/* TABLE: CONDOBJ 	RECORD LENGTH: 42	*/

# define CONDOBJ         (dbtable) 0x0600
# define CONDOBJ_EMP (dbfield) 0x0601	/* Integer */
# define CONDOBJ_CLIENTE (dbfield) 0x0602	/* Long */
# define CONDOBJ_OBJET (dbfield) 0x0603	/* Integer */
# define CONDOBJ_CONDIC (dbfield) 0x0604	/* Integer */
# define CONDOBJ_FCARGA (dbfield) 0x0605	/* Date */
# define CONDOBJbyEMP (dbindex) 0x0600


/* TABLE: HSVIAJE 	RECORD LENGTH: 59	*/

# define HSVIAJE         (dbtable) 0x0700
# define HSVIAJE_EMP (dbfield) 0x0701	/* Integer */
# define HSVIAJE_CLIENTE (dbfield) 0x0702	/* Long */
# define HSVIAJE_OBJET (dbfield) 0x0703	/* Integer */
# define HSVIAJE_FECVIG (dbfield) 0x0704	/* Date */
# define HSVIAJE_HORVIA (dbfield) 0x0705	/* Integer */
# define HSVIAJE_CDATE (dbfield) 0x0706	/* Date */
# define HSVIAJE_CTIME (dbfield) 0x0707	/* Time */
# define HSVIAJE_CUID (dbfield) 0x0708	/* Long */
# define HSVIAJE_MDATE (dbfield) 0x0709	/* Date */
# define HSVIAJE_MTIME (dbfield) 0x070a	/* Time */
# define HSVIAJE_MUID (dbfield) 0x070b	/* Long */
# define HSVIAJEbyEMP (dbindex) 0x0700


/* TABLE: COSCPAGO 	RECORD LENGTH: 112	*/

# define COSCPAGO        (dbtable) 0x0800
# define COSCPAGO_CLIENTE (dbfield) 0x0801	/* Long */
# define COSCPAGO_OBJET (dbfield) 0x0802	/* Integer */
# define COSCPAGO_FECHAVIG (dbfield) 0x0803	/* Date */
# define COSCPAGO_CPAGI (dbfield) 0x0804	/* String */
# define COSCPAGO_DESCRIP (dbfield) 0x0805	/* String */
# define COSCPAGO_CDATE (dbfield) 0x0806	/* Date */
# define COSCPAGO_CTIME (dbfield) 0x0807	/* Time */
# define COSCPAGO_CUID (dbfield) 0x0808	/* Long */
# define COSCPAGO_MDATE (dbfield) 0x0809	/* Date */
# define COSCPAGO_MTIME (dbfield) 0x080a	/* Time */
# define COSCPAGO_MUID (dbfield) 0x080b	/* Long */
# define COSCPAGObyCLIENTE (dbindex) 0x0800


/* TABLE: CROSSCAT 	RECORD LENGTH: 53	*/

# define CROSSCAT        (dbtable) 0x0900
# define CROSSCAT_CONVENIO (dbfield) 0x0901	/* Integer */
# define CROSSCAT_CATVIEJA (dbfield) 0x0902	/* Integer */
# define CROSSCAT_CATNUEVA (dbfield) 0x0903	/* Integer */
# define CROSSCAT_CDATE (dbfield) 0x0904	/* Date */
# define CROSSCAT_CTIME (dbfield) 0x0905	/* Time */
# define CROSSCAT_CUID (dbfield) 0x0906	/* Long */
# define CROSSCAT_MDATE (dbfield) 0x0907	/* Date */
# define CROSSCAT_MTIME (dbfield) 0x0908	/* Time */
# define CROSSCAT_MUID (dbfield) 0x0909	/* Long */
# define CROSSCATbyCONVENIO (dbindex) 0x0900
# define CROSSCATbyCATNUEVA (dbindex) 0x0901


/* TABLE: OBJSUPLEM 	RECORD LENGTH: 56	*/

# define OBJSUPLEM       (dbtable) 0x0a00
# define OBJSUPLEM_EMP (dbfield) 0x0a01	/* Integer */
# define OBJSUPLEM_CLIENTE (dbfield) 0x0a02	/* Long */
# define OBJSUPLEM_OBJET (dbfield) 0x0a03	/* Integer */
# define OBJSUPLEM_ACTIVO (dbfield) 0x0a04	/* Integer */
# define OBJSUPLEM_CDATE (dbfield) 0x0a05	/* Date */
# define OBJSUPLEM_CTIME (dbfield) 0x0a06	/* Time */
# define OBJSUPLEM_CUID (dbfield) 0x0a07	/* Long */
# define OBJSUPLEM_MDATE (dbfield) 0x0a08	/* Date */
# define OBJSUPLEM_MTIME (dbfield) 0x0a09	/* Time */
# define OBJSUPLEM_MUID (dbfield) 0x0a0a	/* Long */
# define OBJSUPLEMbyEMP (dbindex) 0x0a00


/* TABLE: CLIXTER 	RECORD LENGTH: 75	*/

# define CLIXTER         (dbtable) 0x0b00
# define CLIXTER_TERCERO (dbfield) 0x0b01	/* String */
# define CLIXTER_CLIENTE (dbfield) 0x0b02	/* Long */
# define CLIXTER_PRETER (dbfield) 0x0b03	/* String */
# define CLIXTER_ACTIVO (dbfield) 0x0b04	/* Integer */
# define CLIXTER_CDATE (dbfield) 0x0b05	/* Date */
# define CLIXTER_CTIME (dbfield) 0x0b06	/* Time */
# define CLIXTER_CUID (dbfield) 0x0b07	/* Long */
# define CLIXTER_MDATE (dbfield) 0x0b08	/* Date */
# define CLIXTER_MTIME (dbfield) 0x0b09	/* Time */
# define CLIXTER_MUID (dbfield) 0x0b0a	/* Long */
# define CLIXTERbyTERCERO (dbindex) 0x0b00
# define CLIXTERbyPRETER (dbindex) 0x0b01
# define CLIXTERbyACTIVO (dbindex) 0x0b02
# define CLIXTERbyCLIENTE (dbindex) 0x0b03


/* TABLE: CATNOVIA 	RECORD LENGTH: 346	*/

# define CATNOVIA        (dbtable) 0x0c00
# define CATNOVIA_NROINT (dbfield) 0x0c01	/* Long */
# define CATNOVIA_EMP (dbfield) 0x0c02	/* Integer */
# define CATNOVIA_FAMILIA (dbfield) 0x0c03	/* String */
# define CATNOVIA_SUBFAM (dbfield) 0x0c04	/* String */
# define CATNOVIA_ITEM (dbfield) 0x0c05	/* String */
# define CATNOVIA_DESCRIP (dbfield) 0x0c06	/* String */
# define CATNOVIA_ORIGEN (dbfield) 0x0c07	/* String */
# define CATNOVIA_BOL_NOVIA (dbfield) 0x0c08	/* Integer */
# define CATNOVIA_CODNOVIA (dbfield) 0x0c09	/* String */
# define CATNOVIA_TIPOMA (dbfield) 0x0c0a	/* Integer */
# define CATNOVIA_CDATE (dbfield) 0x0c0b	/* Date */
# define CATNOVIA_CTIME (dbfield) 0x0c0c	/* Time */
# define CATNOVIA_CUID (dbfield) 0x0c0d	/* Long */
# define CATNOVIA_MDATE (dbfield) 0x0c0e	/* Date */
# define CATNOVIA_MTIME (dbfield) 0x0c0f	/* Time */
# define CATNOVIA_MUID (dbfield) 0x0c10	/* Long */
# define CATNOVIAbyEMP (dbindex) 0x0c00
# define CATNOVIAbyNOVIA (dbindex) 0x0c01
# define CATNOVIAbyORIGEN (dbindex) 0x0c02
# define CATNOVIAbyCODNOV (dbindex) 0x0c03
# define CATNOVIAbyTIPOMA (dbindex) 0x0c04
# define CATNOVIAbyCODITEM (dbindex) 0x0c05


/* TABLE: DETCATNV 	RECORD LENGTH: 66	*/

# define DETCATNV        (dbtable) 0x0d00
# define DETCATNV_NROINT (dbfield) 0x0d01	/* Long */
# define DETCATNV_EMP (dbfield) 0x0d02	/* Integer */
# define DETCATNV_FVAL (dbfield) 0x0d03	/* Date */
# define DETCATNV_MONTO (dbfield) 0x0d04	/* Float */
# define DETCATNV_MON (dbfield) 0x0d05	/* Integer */
# define DETCATNV_PLAZO (dbfield) 0x0d06	/* Integer */
# define DETCATNV_CDATE (dbfield) 0x0d07	/* Date */
# define DETCATNV_CTIME (dbfield) 0x0d08	/* Time */
# define DETCATNV_CUID (dbfield) 0x0d09	/* Long */
# define DETCATNV_MDATE (dbfield) 0x0d0a	/* Date */
# define DETCATNV_MTIME (dbfield) 0x0d0b	/* Time */
# define DETCATNV_MUID (dbfield) 0x0d0c	/* Long */
# define DETCATNVbyEMP (dbindex) 0x0d00


/* TABLE: MOVOBJ 	RECORD LENGTH: 62	*/

# define MOVOBJ          (dbtable) 0x0e00
# define MOVOBJ_EMPO (dbfield) 0x0e01	/* Integer */
# define MOVOBJ_CLIENTEO (dbfield) 0x0e02	/* Long */
# define MOVOBJ_OBJETIVO (dbfield) 0x0e03	/* Integer */
# define MOVOBJ_EMPD (dbfield) 0x0e04	/* Integer */
# define MOVOBJ_CLIENTED (dbfield) 0x0e05	/* Long */
# define MOVOBJ_OBJETIVD (dbfield) 0x0e06	/* Integer */
# define MOVOBJ_CDATE (dbfield) 0x0e07	/* Date */
# define MOVOBJ_CTIME (dbfield) 0x0e08	/* Time */
# define MOVOBJ_CUID (dbfield) 0x0e09	/* Long */
# define MOVOBJ_MDATE (dbfield) 0x0e0a	/* Date */
# define MOVOBJ_MTIME (dbfield) 0x0e0b	/* Time */
# define MOVOBJ_MUID (dbfield) 0x0e0c	/* Long */
# define MOVOBJbyEMPO (dbindex) 0x0e00
# define MOVOBJbyDESTINO (dbindex) 0x0e01


/* TABLE: PARNOV 	RECORD LENGTH: 307	*/

# define PARNOV          (dbtable) 0x0f00
# define PARNOV_EMP (dbfield) 0x0f01	/* Integer */
# define PARNOV_COD (dbfield) 0x0f02	/* Integer */
# define PARNOV_DESCRIP (dbfield) 0x0f03	/* String */
# define PARNOV_OBSERVA (dbfield) 0x0f04	/* String */
# define PARNOVbyEMP (dbindex) 0x0f00
# define PARNOVbyCODIGO (dbindex) 0x0f01


/* TABLE: DEPANO 	RECORD LENGTH: 91	*/

# define DEPANO          (dbtable) 0x1000
# define DEPANO_EMP (dbfield) 0x1001	/* Integer */
# define DEPANO_COD (dbfield) 0x1002	/* Integer */
# define DEPANO_NROREN (dbfield) 0x1003	/* Integer */
# define DEPANO_FECVIG (dbfield) 0x1004	/* Date */
# define DEPANO_ACT (dbfield) 0x1005	/* Integer */
# define DEPANO_VALOR (dbfield) 0x1006	/* String */
# define DEPANObyEMP (dbindex) 0x1000
# define DEPANObyACTIVO (dbindex) 0x1001


/* TABLE: LEGHORAS 	RECORD LENGTH: 45	*/

# define LEGHORAS        (dbtable) 0x1100
# define LEGHORAS_EMP (dbfield) 0x1101	/* Integer */
# define LEGHORAS_NROLEG (dbfield) 0x1102	/* Long */
# define LEGHORAS_CANTHS (dbfield) 0x1103	/* Float */
# define LEGHORASbyEMP (dbindex) 0x1100


/* TABLE: VALIFE 	RECORD LENGTH: 60	*/

# define VALIFE          (dbtable) 0x1200
# define VALIFE_INTERN (dbfield) 0x1201	/* Long */
# define VALIFE_NROVAR (dbfield) 0x1202	/* Integer */
# define VALIFE_FECVIG (dbfield) 0x1203	/* Date */
# define VALIFE_NROLIQ (dbfield) 0x1204	/* Long */
# define VALIFE_CDATE (dbfield) 0x1205	/* Date */
# define VALIFE_CTIME (dbfield) 0x1206	/* Time */
# define VALIFE_CUID (dbfield) 0x1207	/* Long */
# define VALIFE_MDATE (dbfield) 0x1208	/* Date */
# define VALIFE_MTIME (dbfield) 0x1209	/* Time */
# define VALIFE_MUID (dbfield) 0x120a	/* Long */
# define VALIFEbyINTERN (dbindex) 0x1200
# define VALIFEbyNROLIQ (dbindex) 0x1201

# define IO_COMGRAL_CHKSUM (long) 0xb671
