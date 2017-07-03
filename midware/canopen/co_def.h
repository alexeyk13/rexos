#ifndef CO_DEF_H
#define CO_DEF_H

// Status of the SDO transmission

#define SDO_RESET                0x0      /* Transmission not started. Init state. */
#define SDO_FINISHED              0x1      /* data are available */
#define SDO_ABORTED_RCV           0x80     /* Received an abort message. Data not available */
#define SDO_ABORTED_INTERNAL     0x85     /* Aborted but not because of an abort message. */
#define SDO_DOWNLOAD_IN_PROGRESS 0x2
#define SDO_UPLOAD_IN_PROGRESS   0x3
#define SDO_BLOCK_DOWNLOAD_IN_PROGRESS 0x4
#define SDO_BLOCK_UPLOAD_IN_PROGRESS   0x5


/* Status of the node during the SDO transfer : */
#define SDO_SERVER  0x1
#define SDO_CLIENT  0x2
#define SDO_UNKNOWN 0x3             

/* SDOrx ccs: client command specifier */
#define DOWNLOAD_SEGMENT_REQUEST     0
#define INITIATE_DOWNLOAD_REQUEST    1
#define INITIATE_UPLOAD_REQUEST      2
#define UPLOAD_SEGMENT_REQUEST       3
#define ABORT_TRANSFER_REQUEST       4
#define BLOCK_UPLOAD_REQUEST         5
#define BLOCK_DOWNLOAD_REQUEST       6

/* SDOtx scs: server command specifier */
#define UPLOAD_SEGMENT_RESPONSE      0
#define DOWNLOAD_SEGMENT_RESPONSE    1
#define INITIATE_DOWNLOAD_RESPONSE   3
#define INITIATE_UPLOAD_RESPONSE     2
#define ABORT_TRANSFER_REQUEST       4
#define BLOCK_DOWNLOAD_RESPONSE      5
#define BLOCK_UPLOAD_RESPONSE        6

/* SDO block upload client subcommand */
#define SDO_BCS_INITIATE_UPLOAD_REQUEST 0
#define SDO_BCS_END_UPLOAD_REQUEST      1
#define SDO_BCS_UPLOAD_RESPONSE         2
#define SDO_BCS_START_UPLOAD            3

/* SDO block upload server subcommand */
#define SDO_BSS_INITIATE_UPLOAD_RESPONSE 0
#define SDO_BSS_END_UPLOAD_RESPONSE      1

/* SDO block download client subcommand */
#define SDO_BCS_INITIATE_DOWNLOAD_REQUEST 0
#define SDO_BCS_END_DOWNLOAD_REQUEST      1

/* SDO block download server subcommand */
#define SDO_BSS_INITIATE_DOWNLOAD_RESPONSE 0
#define SDO_BSS_END_DOWNLOAD_RESPONSE      1
#define SDO_BSS_DOWNLOAD_RESPONSE          2

#define MSK_ID     0x7F
#define MSK_FUNC   (0x0F <<7)

// Function Codes
#define NMT        (0x0 << 7)
#define SYNC       (0x1 << 7)
#define TIME_STAMP (0x2 << 7)
#define PDO1tx     (0x3 << 7)
#define PDO1rx     (0x4 << 7)
#define PDO2tx     (0x5 << 7)
#define PDO2rx     (0x6 << 7)
#define PDO3tx     (0x7 << 7)
#define PDO3rx     (0x8 << 7)
#define PDO4tx     (0x9 << 7)
#define PDO4rx     (0xA << 7)
#define SDOtx      (0xB << 7)
#define SDOrx      (0xC << 7)
#define NODE_GUARD (0xE << 7)
#define LSS        (0xF << 7)

/* NMT Command Specifier, sent by master to change a slave state */
/* ------------------------------------------------------------- */
/* Should not be modified */
#define NMT_Start_Node              0x01
#define NMT_Stop_Node               0x02
#define NMT_Enter_PreOperational    0x80
#define NMT_Reset_Node              0x81
#define NMT_Reset_Comunication      0x82

//-------------------- LSS ---------------------

#define SLSS_ADRESS    0x7E4
#define MLSS_ADRESS    0x7E5

/* Switch mode services */
#define LSS_SM_GLOBAL                4
#define LSS_SM_SELECTIVE_VENDOR      64
#define LSS_SM_SELECTIVE_PRODUCT     65
#define LSS_SM_SELECTIVE_REVISION    66
#define LSS_SM_SELECTIVE_SERIAL      67
#define LSS_SM_SELECTIVE_RESP        68
/* Configuration services */
#define LSS_CONF_NODE_ID             17
#define LSS_CONF_BIT_TIMING          19
#define LSS_CONF_ACT_BIT_TIMING      21
#define LSS_CONF_STORE               23
/* Inquire services */
#define LSS_INQ_VENDOR_ID            90
#define LSS_INQ_PRODUCT_CODE         91
#define LSS_INQ_REV_NUMBER           92
#define LSS_INQ_SERIAL_NUMBER        93
#define LSS_INQ_NODE_ID              94
/* Identification services */
#define LSS_IDENT_REMOTE_VENDOR      70
#define LSS_IDENT_REMOTE_PRODUCT     71
#define LSS_IDENT_REMOTE_REV_LOW     72
#define LSS_IDENT_REMOTE_REV_HIGH    73
#define LSS_IDENT_REMOTE_SERIAL_LOW  74
#define LSS_IDENT_REMOTE_SERIAL_HIGH 75
#define LSS_IDENT_REMOTE_NON_CONF    76
#define LSS_IDENT_SLAVE              79
#define LSS_IDENT_NON_CONF_SLAVE     80
#define LSS_IDENT_FASTSCAN           81

#endif // CO_DEF_H

