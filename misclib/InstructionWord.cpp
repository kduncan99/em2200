//  InstructionWord implementation



#include    "misclib.h"



//  static data members

const InstructionWord::ERNameEntry InstructionWord::m_ERNameTable[] =
{
    {01,    "IO$"},
    {02,    "IOI$"},
    {03,    "IOW$"},
    {04,    "EDJS$"},
    {06,    "WAIT$"},
    {07,    "WANY$"},
    {010,   "COM$"},
    {011,   "EXIT$"},
    {012,   "ABORT$"},
    {013,   "FORK$"},
    {014,   "TFORK$"},
    {015,   "READ$"},
    {016,   "PRINT$"},
    {017,   "CSF$"},
    {022,   "DATE$"},
    {023,   "TIME$"},
    {024,   "IOWI$"},
    {025,   "IOXI$"},
    {026,   "EABT$"},
    {027,   "II$"},
    {030,   "ABSAD$"},
    {032,   "FITEM$"},
    {033,   "INT$"},
    {034,   "IDENT$"},
    {035,   "CRTN$"},
    {037,   "WALL$"},
    {040,   "ERR$"},
    {041,   "MCT$"},
    {042,   "READA$"},
    {043,   "MCORE$"},
    {044,   "LCORE$"},
    {054,   "TDATE$"},
    {060,   "TWAIT$"},
    {061,   "RT$"},
    {062,   "NRT$"},
    {063,   "OPT$"},
    {064,   "PCT$"},
    {065,   "SETC$"},
    {066,   "COND$"},
    {067,   "UNLCK$"},
    {070,   "APRINT$"},
    {071,   "APRNTA$"},
    {072,   "APUNCH$"},
    {073,   "APNCHA$"},
    {074,   "APRTCN$"},
    {075,   "APCHCN$"},
    {076,   "APRTCA$"},
    {077,   "APCHCA$"},
    {0100,  "CEND$"},
    {0101,  "IALL$"},
    {0102,  "TREAD$"},
    {0103,  "SWAIT$"},
    {0104,  "PFI$"},
    {0105,  "PFS$"},
    {0106,  "PFD$"},
    {0107,  "PFUWL$"},
    {0110,  "PFWL$"},
    {0111,  "LOAD$"},
    {0112,  "RSI$"},
    {0113,  "TSQCL$"},
    {0114,  "FACIL$"},
    {0115,  "BDSPT$"},
    {0116,  "INFO$"},
    {0117,  "CQUE$"},
    {0120,  "TRMRG$"},
    {0121,  "TSQRG$"},
    {0122,  "CTSQ$"},
    {0123,  "CTS$"},
    {0124,  "CTSA$"},
    {0125,  "MSCON$"},
    {0126,  "SNAP$"},
    {0130,  "PUNCH$"},
    {0134,  "AWAIT$"},
    {0135,  "TSWAP$"},
    {0136,  "TINTL$"},
    {0137,  "PRTCN$"},
    {0140,  "ACSF$"},
    {0141,  "TOUT$"},
    {0142,  "TLBL$"},
    {0143,  "FACIT$"},
    {0144,  "PRNTA$"},
    {0145,  "PNCHA$"},
    {0146,  "NAME$"},
    {0147,  "ACT$"},
    {0150,  "DACT$"},
    {0153,  "CLIST$"},
    {0155,  "PRTCA$"},
    {0156,  "SETBP$"},
    {0157,  "PSR$"},
    {0160,  "BANK$"},
    {0161,  "ADED$"},
    {0163,  "ACCNT$"},
    {0164,  "PCHCN$"},
    {0165,  "PCHCA$"},
    {0166,  "AREAD$"},
    {0167,  "AREADA$"},
    {0170,  "ATREAD$"},
    {0175,  "CLCAL$"},
    {0176,  "SYSBAL$"},
    {0200,  "SYMB$"},
    {0202,  "ERRPR$"},
    {0207,  "LEVEL$"},
    {0210,  "LOG$"},
    {0212,  "CREG$"},
    {0213,  "SREG$"},
    {0214,  "SUVAL$"},
    {0215,  "SUMOD$"},
    {0216,  "STAB$"},
    {0222,  "SDEL$"},
    {0223,  "SPRNT$"},
    {0225,  "SABORT$"},
    {0227,  "DMSS$"},
    {0230,  "DMCM$"},
    {0231,  "DMES$"},
    {0232,  "DMRB$"},
    {0233,  "DMGC$"},
    {0234,  "ERCVS$"},
    {0235,  "MQF$"},
    {0236,  "SC$Q$"},
    {0237,  "DMABT$"},
    {0241,  "AUDIT$"},
    {0242,  "SYMINFO$"},
    {0243,  "SMOQUE$"},
    {0244,  "KEYIN$"},
    {0246,  "HMDBIT$"},
    {0247,  "CSI$"},
    {0250,  "CONFIG$"},
    {0251,  "TRTIM$"},
    {0252,  "ERTRAP$"},
    {0253,  "REGRTN$"},
    {0254,  "REGREP$"},
    {0255,  "TRAPRTN$"},
    {0263,  "TRON$"},
    {0264,  "DWTIME$"},
    {0266,  "MCODE$"},
    {0267,  "IOAID$"},
    {0270,  "AP$KEY"},
    {0271,  "AT$KEY"},
    {0272,  "SYSLOG$"},
    {0273,  "MODPS$"},
    {0274,  "TERMRUN$"},
    {0277,  "QECL$"},
    {0300,  "DQECL$"},
    {0303,  "SATTCP$"},
    {0304,  "SCDTL$"},
    {0305,  "SCDTA$"},
    {0307,  "TVSLBL$"},
    {0311,  "HOST$"},
    {0312,  "SCLDT$"},
    {0313,  "SCOMCNV$"},
    {0314,  "H2CON$"},
    {0317,  "UDSPP$"},
    {02004, "RT$INT"},
    {02005, "RT$OUT"},
    {02006, "CMS$REG"},
    {02011, "CA$ASG"},
    {02012, "CA$REL"},
    {02021, "CR$ELG"},
    {02030, "AC$NIT"},
    {02031, "VT$RD"},
    {02041, "VT$CHG"},
    {02042, "VT$PUR"},
    {02044, "TP$APL"},
    {02046, "TF$KEY"},
    {02047, "EX$CRD"},
    {02050, "DM$FAC"},
    {02051, "DM$IO"},
    {02052, "DM$IOW"},
    {02053, "DM$WT"},
    {02054, "BT$DIS"},	
    {02055, "BT$ENA"},
    {02056, "FLAGBOX"},
    {02060, "RT$PSI"},
    {02061, "RT$PSD"},
    {02061, "TPLIB$"},
    {02065, "XFR$"},
    {02066, "CALL$"},
    {02067, "RTN$"},
    {02070, "TCORE$"},
    {02071, "XRS$"},
    {02074, "CO$MIT"},
    {02075, "RL$BAK"},
    {02101, "RT$PSS"},
    {02102, "RT$PID"},
    {02103, "SEXEM$"},
    {02104, "TIP$Q"},
    {02106, "QI$NIT"},
    {02107, "QI$CON"},
    {02110, "QI$DIS"},
    {02111, "TIP$TA"},
    {02112, "TIP$TC"},
    {02113, "TIP$ID"},
    {02114, "MCABT$"},
    {02115, "MSGN$"},
    {02117, "PERF$"},
    {02120, "TIP$XMIT"},
    {02130, "TIP$SM"},
    {02131, "TIP$TALK"},
    {02132, "SC$SR"},
    {02133, "TM$SET"},
};


const InstructionWord::InstructionInfo InstructionWord::m_InstructionInfoTable[] =
{
    // mode     f    j    a    jFlag  aFlag  grsFl  immFl   ASEM    MNEM    bmSem   Handler
    {EITHER,    001, 016, 000, true,  false, false, false,  A,      "PRBA",	false,  0},
    {EITHER,    001, 000, 000, false, false, true,  false,	A,      "SA",	false,  0},
    {EITHER,    002, 000, 000, false, false, true,  false,	A,      "SNA",	false,  0},
    {EITHER,    003, 016, 000, true,  false, false, false,  A,      "PRBC",	false,  0},
    {EITHER,    003, 000, 000, false, false, true,  false,	A,      "SMA",	false,  0},
    {EITHER,    004, 000, 000, false, false, true,  false,	R,      "SR",	false,  0},
    {EITHER,    005, 000, 000, false, false, true,  false,  NONE,   "SZ",	false,  0},
    {EITHER,    005, 000, 001, false, false, true,  false,  NONE,   "SNZ",	false,  0},
    {EITHER,    005, 000, 002, false, false, true,  false,  NONE,   "SP1",	false,  0},
    {EITHER,    005, 000, 003, false, false, true,  false,  NONE,   "SN1",	false,  0},
    {EITHER,    005, 000, 004, false, false, true,  false,  NONE,   "SFS",	false,  0},
    {EITHER,    005, 000, 005, false, false, true,  false,  NONE,   "SFZ",	false,  0},
    {EITHER,    005, 000, 006, false, false, true,  false,  NONE,   "SAS",	false,  0},
    {EITHER,    005, 000, 007, false, false, true,  false,  NONE,   "SAZ",	false,  0},
    {EITHER,    005, 000, 010, false, false, true,  false,  NONE,   "INC",	false,  0},
    {EITHER,    005, 000, 011, false, false, true,  false,  NONE,   "DEC",	false,  0},
    {EITHER,    005, 000, 012, false, false, true,  false,  NONE,   "INC2",	false,  0},
    {EITHER,    005, 000, 013, false, false, true,  false,  NONE,   "DEC2",	false,  0},
    {EITHER,    005, 000, 014, false, false, true,  false,  NONE,   "ENZ",	false,  0},
    {EITHER,    005, 000, 015, false, false, true,  false,  NONE,   "ADD1",	false,  0},
    {EITHER,    005, 000, 016, false, false, true,  false,  NONE,   "SUB1",	false,  0},
    {EITHER,    006, 000, 000, false, false, true,  false,  X,      "SX",	false,  0},
    {EITHER,    007, 000, 000, true,  false, true,  0,		A,     "ADE",	false,  0},
    {EITHER,    007, 001, 000, true,  false, true,  0,		A,     "DADE",	false,  0},
    {EITHER,    007, 002, 000, true,  false, true,  0,		A,     "SDE",	false,  0},
    {EITHER,    007, 003, 000, true,  false, true,  0,		A,     "DSDE",	false,  0},
    {EITHER,    007, 004, 000, true,  false, true,  0,		A,     "LAQW",	false,  0},
    {EITHER,    007, 005, 000, true,  false, true,  0,		A,     "SAQW",	false,  0},
    {EITHER,    007, 006, 000, true,  false, true,  0,		A,     "DEI",	false,  0},
    {EITHER,    007, 007, 000, true,  false, true,  0,		A,     "DDEI",	false,  0},
    {EITHER,    007, 010, 000, true,  false, true,  0,		A,     "IDE",	false,  0},
    {EITHER,    007, 011, 000, true,  false, true,  0,		A,     "DIDE",	false,  0},
    {BASIC,     007, 012, 000, true,  false, false, 0,		X,     "LDJ",	false,  0},
    {BASIC,     007, 013, 000, true,  false, false, 0,		X,     "LIJ",	false,  0},
    {BASIC,     007, 014, 000, true,  false, false, 0,		NONE,  "LPD",	false,  0},
    {BASIC,     007, 015, 000, true,  false, false, 0,		NONE,  "SPD",	false,  0},
    {EXTENDED,  007, 016, 000, true,  true,  false, false,	NONE,  "LOCL",	true,   0},
    {EXTENDED,  007, 016, 013, true,  true,  false, 0,		NONE,  "CALL",	false,  0},
    {BASIC,     007, 017, 000, true,  false, false, 0,		X,     "LBJ",	false,  0},
    {EXTENDED,  007, 017, 000, true,  true,  false, 0,		NONE,  "GOTO",	false,  0},
    {EITHER,    010, 000, 000, false, false, true,  true,	A,     "LA",	false,  0},
    {EITHER,    011, 000, 000, false, false, true,  true,	A,     "LNA",	false,  0},
    {EITHER,    012, 000, 000, false, false, true,  true,	A,     "LMA",	false,  0},
    {EITHER,    013, 000, 000, false, false, true,  true,	A,     "LNMA",	false,  0},
    {EITHER,    014, 000, 000, false, false, true,  true,	A,     "AA",	false,  0},
    {EITHER,    015, 000, 000, false, false, true,  true,	A,     "ANA",	false,  0},
    {EITHER,    016, 000, 000, false, false, true,  true,	A,     "AMA",	false,  0},
    {EITHER,    017, 000, 000, false, false, true,  true,	A,     "ANMA",	false,  0},
    {EITHER,    020, 000, 000, false, false, true,  true,	A,     "AU",	false,  0},
    {EITHER,    021, 000, 000, false, false, true,  true,	A,     "ANU",	false,  0},
    {BASIC,     022, 000, 000, false, false, true,  false,	X,     "BT",	false,  0},
    {EXTENDED,  022, 000, 000, false, false, 0,     0,		NONE,  "",		false,  handleBT},
    {EITHER,    023, 000, 000, false, false, true,  true,	R,     "LR",	false,  0},
    {EITHER,    024, 000, 000, false, false, true,  true,	X,     "AX",	false,  0},
    {EITHER,    025, 000, 000, false, false, true,  true,	X,     "ANX",	false,  0},
    {EITHER,    026, 000, 000, false, false, true,  true,	X,     "LXM",	false,  0},
    {EITHER,    027, 000, 000, false, false, true,  true,	X,     "LX",	false,  0},
    {EITHER,    030, 000, 000, false, false, true,  true,	A,     "MI",	false,  0},
    {EITHER,    031, 000, 000, false, false, true,  true,	A,     "MSI",	false,  0},
    {EITHER,    032, 000, 000, false, false, true,  true,	A,     "MF",	false,  0},
    {EXTENDED,  033, 010, 000, true,  false, false, 0,		A,     "LS",	false,  0},
    {EXTENDED,  033, 011, 000, true,  false, false, 0,		A,     "LSA",	false,  0},
    {EXTENDED,  033, 012, 000, true,  false, false, 0,		A,     "SS",	false,  0},
    {EXTENDED,  033, 013, 000, true,  false, true,  0,		A,     "TGM",	false,  0},
    {EXTENDED,  033, 014, 000, true,  false, true,  0,		A,     "DTGM",	false,  0},
    {EXTENDED,  033, 015, 000, true,  false, true,  0,		A,     "DCB",	false,  0},
    {EXTENDED,  033, 016, 000, true,  false, false, 0,		A,     "TES",	false,  0},
    {EXTENDED,  033, 017, 000, true,  false, false, 0,		A,     "TNES",	false,  0},
    {EITHER,    034, 000, 000, false, false, true,  true,	A,     "DI",	false,  0},
    {EITHER,    035, 000, 000, false, false, true,  true,	A,     "DSF",	false,  0},
    {EITHER,    036, 000, 000, false, false, true,  true,	A,     "DF",	false,  0},
    {EXTENDED,  037, 000, 000, true,  false, false, 0,		A,     "LRD",	false,  0},
    {EXTENDED,  037, 004, 000, true,  true,  false, 0,      NONE,  "SMD",	false,  0},
    {EXTENDED,  037, 004, 001, true,  true,  false, 0,      NONE,  "SDMN",	false,  0},
    {EXTENDED,  037, 004, 002, true,  true,  false, 0,      NONE,  "SDMF",	false,  0},
    {EXTENDED,  037, 004, 003, true,  true,  false, 0,      NONE,  "SDMS",	false,  0},
    {EITHER,    037, 007, 000, true,  false, false, 0,		A,     "LMC",	false,  0},
    {BASIC,     037, 010, 000, true,  false, false, 0,      NONE,  "BIM",	false,  0},
    {BASIC,     037, 011, 000, true,  false, false, 0,      NONE,  "BIC",	false,  0},
    {BASIC,     037, 012, 000, true,  false, false, 0,      NONE,  "BIMT",	false,  0},
    {BASIC,     037, 013, 000, true,  false, false, 0,      NONE,  "BICL",	false,  0},
    {BASIC,     037, 014, 000, true,  false, false, 0,      NONE,  "BIML",	false,  0},
    {BASIC,     037, 015, 000, true,  false, false, 0,      A,     "BDE",	false,  0},
    {BASIC,     037, 016, 000, true,  false, false, 0,      A,     "DEB",	false,  0},
    {BASIC,     037, 017, 000, true,  false, false, 0,      A,     "EDDE",	false,  0},
    {EXTENDED,  037, 010, 000, true,  false, true,  0,		A,     "ENQ",	false,  0},
    {EXTENDED,  037, 011, 000, true,  false, true,  0,		A,     "ENQF",	false,  0},
    {EXTENDED,  037, 012, 000, true,  false, true,  0,		A,     "DEQ",	false,  0},
    {EXTENDED,  037, 013, 000, true,  false, true,  0,		A,     "DEQW",	false,  0},
    {EITHER,    040, 000, 000, false, false, true,  true,	A,     "OR",	false,  0},
    {EITHER,    041, 000, 000, false, false, true,  true,	A,     "XOR",	false,  0},
    {EITHER,    042, 000, 000, false, false, true,  true,	A,     "AND",	false,  0},
    {EITHER,    043, 000, 000, false, false, true,  true,	A,     "MLU",	false,  0},
    {EITHER,    044, 000, 000, false, false, true,  true,	A,     "TEP",	false,  0},
    {EITHER,    045, 000, 000, false, false, true,  true,	A,     "TOP",	false,  0},
    {EITHER,    046, 000, 000, false, false, true,  true,	X,     "LXI",	false,  0},
    {EITHER,    047, 000, 000, false, false, true,  true,	A,     "TLEM",	false,  0},
    {BASIC,     050, 000, 000, false, false, true,	true,	NONE,  "TZ",	false,  0},
    {EXTENDED,  050, 000, 000, false, true,  true,  true,	NONE,  "TNOP",	false,  0},
    {EXTENDED,  050, 000, 001, false, true,  true,  true,	NONE,  "TGZ",	false,  0},
    {EXTENDED,  050, 000, 002, false, true,  true,  true,	NONE,  "TPZ",	false,  0},
    {EXTENDED,  050, 000, 003, false, true,  true,  true,	NONE,  "TP",	false,  0},
    {EXTENDED,  050, 000, 004, false, true,  true,  true,	NONE,  "TMZ",	false,  0},
    {EXTENDED,  050, 000, 005, false, true,  true,  true,	NONE,  "TMZG",	false,  0},
    {EXTENDED,  050, 000, 006, false, true,  true,  true,	NONE,  "TZ",	false,  0},
    {EXTENDED,  050, 000, 007, false, true,  true,  true,	NONE,  "TNLZ",	false,  0},
    {EXTENDED,  050, 000, 010, false, true,  true,  true,	NONE,  "TLZ",	false,  0},
    {EXTENDED,  050, 000, 011, false, true,  true,  true,	NONE,  "TNZ",	false,  0},
    {EXTENDED,  050, 000, 012, false, true,  true,  true,	NONE,  "TPZL",	false,  0},
    {EXTENDED,  050, 000, 013, false, true,  true,  true,	NONE,  "TNMZ",	false,  0},
    {EXTENDED,  050, 000, 014, false, true,  true,  true,	NONE,  "TN",	false,  0},
    {EXTENDED,  050, 000, 015, false, true,  true,  true,	NONE,  "TNPZ",	false,  0},
    {EXTENDED,  050, 000, 016, false, true,  true,  true,	NONE,  "TNGZ",	false,  0},
    {EXTENDED,  050, 000, 017, false, true,  true,  true,	NONE,  "TSKP",	false,  0},
    {BASIC,     051, 000, 000, false, false, true,  true,	NONE,  "TNX",	false,  0},
    {EXTENDED,  051, 000, 000, false, false, true,  true,	X,     "LXSI",	false,  0},
    {EITHER,    052, 000, 000, false, false, true,  true,	A,     "TE",	false,  0},
    {EITHER,    053, 000, 000, false, false, true,  true,	A,     "TNE",	false,  0},
    {EITHER,    054, 000, 000, false, false, true,  true,	A,     "TLE",	false,  0},
    {EITHER,    055, 000, 000, false, false, true,  true,	A,     "TG",	false,  0},
    {EITHER,    056, 000, 000, false, false, true,  true,	A,     "TW",	false,  0},
    {EITHER,    057, 000, 000, false, false, true,  true,	A,     "TNW",	false,  0},
    {BASIC,     060, 000, 000, false, false, true,  true,	NONE,  "TP",	false,  0},
    {EXTENDED,  060, 000, 000, false, false, true,  true,	X,     "LSBO",	false,  0},
    {BASIC,     061, 000, 000, false, false, true,  true,	NONE,  "TN",	false,  0},
    {EXTENDED,  061, 000, 000, false, false, true,  true,	X,     "LSBL",	false,  0},
    {EITHER,    062, 000, 000, false, false, true,  true,	A,     "SE",	false,  0},
    {EITHER,    063, 000, 000, false, false, true,  true,	A,     "SNE",	false,  0},
    {EITHER,    064, 000, 000, false, false, true,  true,	A,     "SLE",	false,  0},
    {EITHER,    065, 000, 000, false, false, true,  true,	A,     "SG",	false,  0},
    {EITHER,    066, 000, 000, false, false, true,  true,	A,     "SW",	false,  0},
    {EITHER,    067, 000, 000, false, false, true,  true,	A,     "SNW",	false,  0},
    {EITHER,    070, 000, 000, false, false, 0,     0,		NONE,  "",      false,  handleJGD},
    {BASIC,     071, 000, 000, true,  false, true,  0,		A,     "MSE",	false,  0},
    {BASIC,     071, 001, 000, true,  false, true,  0,		A,     "MSNE",	false,  0},
    {BASIC,     071, 002, 000, true,  false, true,  0,		A,     "MSLE",	false,  0},
    {BASIC,     071, 003, 000, true,  false, true,  0,		A,     "MSG",	false,  0},
    {BASIC,     071, 004, 000, true,  false, true,  0,		A,     "MSW",	false,  0},
    {BASIC,     071, 005, 000, true,  false, true,  0,		A,     "MSNW",	false,  0},
    {BASIC,     071, 006, 000, true,  false, true,  0,		A,     "MASL",	false,  0},
    {BASIC,     071, 007, 000, true,  false, true,  0,		A,     "MASG",	false,  0},
    {EXTENDED,  071, 000, 000, true,  false, true,  0,		A,     "MTE",	false,  0},
    {EXTENDED,  071, 001, 000, true,  false, true,  0,		A,     "MTNE",	false,  0},
    {EXTENDED,  071, 002, 000, true,  false, true,  0,		A,     "MTLE",	false,  0},
    {EXTENDED,  071, 003, 000, true,  false, true,  0,		A,     "MTG",	false,  0},
    {EXTENDED,  071, 004, 000, true,  false, true,  0,		A,     "MTW",	false,  0},
    {EXTENDED,  071, 005, 000, true,  false, true,  0,		A,     "MTNW",	false,  0},
    {EXTENDED,  071, 006, 000, true,  false, true,  0,		A,     "MATL",	false,  0},
    {EXTENDED,  071, 007, 000, true,  false, true,  0,		A,     "MATG",	false,  0},
    {EITHER,    071, 010, 000, true,  false, true,  0,		A,     "DA",	false,  0},
    {EITHER,    071, 011, 000, true,  false, true,  0,		A,     "DAN",	false,  0},
    {EITHER,    071, 012, 000, true,  false, true,  0,		A,     "DS",	false,  0},
    {EITHER,    071, 013, 000, true,  false, true,  0,		A,     "DL",	false,  0},
    {EITHER,    071, 014, 000, true,  false, true,  0,		A,     "DLN",	false,  0},
    {EITHER,    071, 015, 000, true,  false, true,  0,		A,     "DLM",	false,  0},
    {EITHER,    071, 016, 000, true,  false, 0,     0,		NONE,  "DJZ",	true,   0},
    {EITHER,    071, 017, 000, true,  false, true,  0,		A,     "DTE",	false,  0},
    {BASIC,     072, 001, 000, true,  false, false, 0,		NONE,  "SLJ",	false,  0},
    {EITHER,    072, 002, 000, true,  false, false, 0,		A,     "JPS",	false,  0},
    {EITHER,    072, 003, 000, true,  false, false, 0,		A,     "JNS",	false,  0},
    {EITHER,    072, 004, 000, true,  false, true,  0,		A,     "AH",	false,  0},
    {EITHER,    072, 005, 000, true,  false, true,  0,		A,     "ANH",	false,  0},
    {EITHER,    072, 006, 000, true,  false, true,  0,		A,     "AT",	false,  0},
    {EITHER,    072, 007, 000, true,  false, true,  0,		A,     "ANT",	false,  0},
    {BASIC,     072, 010, 000, true,  false, false, 0,		NONE,  "EX",	false,  0},
    {EXTENDED,  072, 010, 000, true,  false, false, 0,      A,     "BDE",	false,  0},
    {BASIC,     072, 011, 000, true,  false, 0,     0,		NONE,  "",      false,  handleER},
    {EXTENDED,  072, 011, 000, true,  false, false, 0,      A,     "DEB",	false,  0},
    {EITHER,    072, 012, 000, true,  false, true,  0,		X,     "BN",	false,  0},
    {EXTENDED,  072, 013, 000, true,  false, false, 0,		A,     "BAO",	false,  0},
    {EITHER,    072, 014, 000, true,  false, true,  0,		X,     "BBN",	false,  0},
    {EITHER,    072, 015, 000, true,  false, false, 0,		X,     "TRA",	false,  0},
    {EITHER,    072, 016, 000, true,  false, false, 0,		A,     "SRS",	false,  0},
    {EITHER,    072, 017, 000, true,  false, false, 0,		A,     "LRS",	false,  0},
    {EITHER,    073, 000, 000, true,  false, false, 0,      A,      "SSC",  true,   0},
    {EITHER,    073, 001, 000, true,  false, false, 0,      A,      "DSC",  true,   0},
    {EITHER,    073, 002, 000, true,  false, false, 0,      A,      "SSL",  true,   0},
    {EITHER,    073, 003, 000, true,  false, false, 0,      A,      "DSL",  true,   0},
    {EITHER,    073, 004, 000, true,  false, false, 0,      A,      "SSA",  true,   0},
    {EITHER,    073, 005, 000, true,  false, false, 0,      A,      "DSA",  true,   0},
    {EITHER,    073, 006, 000, true,  false, false, 0,      A,      "LSC",  true,   0},
    {EITHER,    073, 007, 000, true,  false, false, 0,      A,      "DLSC", true,   0},
    {EITHER,    073, 010, 000, true,  false, false, 0,      A,      "LSSC", true,   0},
    {EITHER,    073, 011, 000, true,  false, false, 0,      A,      "LDSC", true,   0},
    {EITHER,    073, 012, 000, true,  false, false, 0,      A,      "LSSL", true,   0},
    {EITHER,    073, 013, 000, true,  false, false, 0,      A,      "LDSL", true,   0},
    {EXTENDED,  073, 014, 000, true,  true,  true,  0,      NONE,   "NOP",  false,  0},
    {EITHER,    073, 014, 001, true,  true,  true,  0,		NONE,  "LPM",	false,  0},
    {EXTENDED,  073, 014, 002, true,  true,  false, 0,		NONE,  "BUY",	false,  0},
    {EXTENDED,  073, 014, 003, true,  true,  false, 0,		NONE,  "SELL",	false,  0},
    {EXTENDED,  073, 014, 004, true,  true,  false, 0,		NONE,  "UNLK",	false,  0},
    {EXTENDED,  073, 014, 005, true,  true,  false, 0,		NONE,  "EX",	false,  0},
    {EXTENDED,  073, 014, 006, true,  true,  false, 0,		NONE,  "EXR",	false,  0},
    {EXTENDED,  073, 014, 007, true,  true,  false, 0,      NONE,  "BIMT",	false,  0},
    {EXTENDED,  073, 014, 010, true,  true,  false, 0,      NONE,  "BIM",	false,  0},
    {EXTENDED,  073, 014, 011, true,  true,  false, 0,      NONE,  "BIML",	false,  0},
    {EXTENDED,  073, 014, 012, true,  true,  false, 0,      NONE,  "BIC",	false,  0},
    {EXTENDED,  073, 014, 013, true,  true,  false, 0,      NONE,  "BICL",	false,  0},
    {EXTENDED,  073, 014, 014, true,  true,  true,  0,		NONE,  "LINC",	false,  0},
    {EXTENDED,  073, 014, 015, true,  true,  true,  0,		NONE,  "SINC",	false,  0},
    {EXTENDED,  073, 014, 016, true,  true,  true,  0,		NONE,  "LCC",	false,  0},
    {EXTENDED,  073, 014, 017, true,  true,  true,  0,		NONE,  "SCC",	false,  0},
    {EXTENDED,  073, 015, 002, true,  true,  true,  0,		NONE,  "LBRX",	false,  0},
    {EITHER,    073, 015, 003, true,  true,  false, 0,		NONE,  "ACEL",	false,  0},
    {EITHER,    073, 015, 004, true,  true,  false, 0,		NONE,  "DCEL",	false,  0},
    {EITHER,    073, 015, 005, true,  true,  true,  0,		NONE,  "SPID",	false,  0},
    {EXTENDED,  073, 015, 006, true,  true,  false, 0,		NONE,  "DABT",	false,  0},
    {EXTENDED,  073, 015, 007, true,  true,  false, 0,		NONE,  "SEND",	false,  0},
    {EXTENDED,  073, 015, 010, true,  true,  false, 0,		NONE,  "ACK",	false,  0},
    {EITHER,    073, 015, 011, true,  true,  true,  0,		NONE,  "SPM",	false,  0},
    {EXTENDED,  073, 015, 012, true,  true,  false, 0,		NONE,  "LAE",	false,  0},
    {EXTENDED,  073, 015, 013, true,  true,  true,  0,		NONE,  "SKQT",	false,  0},
    {EITHER,    073, 015, 014, true,  true,  true,  0,		NONE,  "LD",	false,  0},
    {EITHER,    073, 015, 015, true,  true,  true,  0,		NONE,  "SD",	false,  0},
    {EITHER,    073, 015, 016, true,  true,  false, 0,		NONE,  "UR",	false,  0},
    {EITHER,    073, 015, 017, true,  true,  false, 0,		NONE,  "SGNL",	false,  0},
    {EXTENDED,  073, 016, 000, true,  false, false, 0,      A,     "EDDE",	false,  0},
    {EITHER,    073, 017, 000, true,  true,  false, 0,		NONE,  "TS",	false,  0},
    {EITHER,    073, 017, 001, true,  true,  false, 0,		NONE,  "TSS",	false,  0},
    {EITHER,    073, 017, 002, true,  true,  false, 0,		NONE,  "TCS",	false,  0},
    {EXTENDED,  073, 017, 003, true,  true,  false, 0,		NONE,  "RTN",	false,  0},
    {EITHER,    073, 017, 004, true,  true,  true,  0,		NONE,  "LUD",	false,  0},
    {EITHER,    073, 017, 005, true,  true,  true,  0,		NONE,  "SUD",	false,  0},
    {EXTENDED,  073, 017, 006, true,  true,  false, 0,		NONE,  "IAR",	false,  0},
    {EXTENDED,  073, 017, 007, true,  true,  false, 0,		NONE,  "ZEROP",false,  0},
    {EXTENDED,  073, 017, 010, true,  true,  false, 0,		NONE,  "IPC",	false,  0},
    {EXTENDED,  073, 017, 011, true,  true,  false, 0,		NONE,  "CJHE",	true,   0},
    {EXTENDED,  073, 017, 012, true,  true,  false, 0,		NONE,  "SYSC",	false,  0},
    {EXTENDED,  073, 017, 013, true,  true,  false, 0,		NONE,  "LATP",	false,  0},
    {EXTENDED,  073, 017, 014, true,  true,  false, 0,		NONE,  "INV",	false,  0},
    {EXTENDED,  073, 017, 015, true,  true,  false, 0,		NONE,  "SJH",	false,  0},
    {EXTENDED,  073, 017, 016, true,  true,  false, 0,		NONE,  "SSIP",	false,  0},
    {EITHER,    074, 000, 000, true,  false, false, 0,		A,     "JZ",	true,   0},
    {EITHER,    074, 001, 000, true,  false, false, 0,		A,     "JNZ",	true,   0},
    {EITHER,    074, 002, 000, true,  false, false, 0,		A,     "JP",	true,   0},
    {EITHER,    074, 003, 000, true,  false, false, 0,		A,     "JN",	true,   0},
    {BASIC,     074, 004, 000, true,  true,  false, 0,		NONE,  "J",	    true,   0},	// a-field == 0
    {BASIC,     074, 004, 000, true,  false, false, 0,		NONE,  "JK",	true,   0},	// a-field > 0
    {BASIC,     074, 005, 000, true,  true,  false, 0,		NONE,  "HJ",	true,   0},	// a-field == 0
    {BASIC,     074, 005, 000, true,  false, false, 0,		NONE,  "HKJ",	true,   0},	// a-field > 0
    {BASIC,     074, 006, 000, true,  false, true,  0,		A,     "NOP",	false,  0},
    {BASIC,     074, 007, 000, true,  false, false, 0,		NONE,  "AAIJ",	true,   0},
    {EITHER,    074, 010, 000, true,  false, false, 0,		A,     "JNB",	true,   0},
    {EITHER,    074, 011, 000, true,  false, false, 0,		A,     "JB",	true,   0},
    {EITHER,    074, 012, 000, true,  false, false, 0,		X,     "JMGI",	true,   0},
    {EITHER,    074, 013, 000, true,  false, false, 0,		X,     "LMJ",	true,   0},
    {EITHER,    074, 014, 000, true,  true,  false, 0,		NONE,  "JO",	true,   0},
    {EITHER,    074, 014, 001, true,  true,  false, 0,		NONE,  "JFU",	true,   0},
    {EITHER,    074, 014, 002, true,  true,  false, 0,		NONE,  "JFO",	true,   0},
    {EITHER,    074, 014, 003, true,  true,  false, 0,		NONE,  "JDF",	true,   0},
    {EXTENDED,  074, 014, 004, true,  true,  false, 0,		NONE,  "JC",	true,   0},
    {EXTENDED,  074, 014, 005, true,  true,  false, 0,		NONE,  "JNC",	true,   0},
    {EXTENDED,  074, 014, 006, true,  true,  false, 0,		NONE,  "AAIJ",	true,   0},
    {EITHER,    074, 014, 007, true,  true,  false, 0,		NONE,  "PAIJ",	true,   0},
    {EITHER,    074, 015, 000, true,  true,  false, 0,		NONE,  "JNO",	true,   0},
    {EITHER,    074, 015, 001, true,  true,  false, 0,		NONE,  "JNFU",	true,   0},
    {EITHER,    074, 015, 002, true,  true,  false, 0,		NONE,  "JNFO",	true,   0},
    {EITHER,    074, 015, 003, true,  true,  false, 0,		NONE,  "JNDF",	true,   0},
    {EXTENDED,  074, 015, 004, true,  true,  false, 0,		NONE,  "J",	    true,   0},
    {EITHER,    074, 015, 005, true,  true,  false, 0,		NONE,  "HLTJ",	true,   0},
    {BASIC,     074, 016, 000, true,  false, false, 0,		NONE,  "JC",	true,   0},
    {BASIC,     074, 017, 000, true,  false, false, 0,		NONE,  "JNC",	true,   0},
    {EITHER,    075, 000, 000, true,  false, true,  0,		B,     "LBU",	false,  0},
    {EITHER,    075, 002, 000, true,  false, true,  0,		B,     "SBU",	false,  0},
    {EITHER,    075, 003, 000, true,  false, true,  0,		BEXEC, "LBE",	false,  0},
    {EITHER,    075, 004, 000, true,  false, false, 0,		BEXEC, "SBED",	false,  0},
    {EITHER,    075, 005, 000, true,  false, false, 0,		BEXEC, "LBED",	false,  0},
    {EITHER,    075, 006, 000, true,  false, false, 0,		B,     "SBUD",	false,  0},
    {EITHER,    075, 007, 000, true,  false, false, 0,		B,     "LBUD",	false,  0},
    {EITHER,    075, 010, 000, true,  false, false, 0,		X,     "TVA",	false,  0},
    {EXTENDED,  075, 012, 000, true,  false, false, 0,		A,     "RDC",	false,  0},
    {EITHER,    075, 013, 000, true,  false, true,  0,		X,     "LXLM",	false,  0},
    {EITHER,    075, 014, 000, true,  false, true,  0,		X,     "LBN",	false,  0},
    {EITHER,    075, 015, 000, true,  false, false, 0,		A,     "CR",	false,  0},
    {EITHER,    076, 000, 000, true,  false, true,  0,		A,     "FA",	false,  0},
    {EITHER,    076, 001, 000, true,  false, true,  0,		A,     "FAN",	false,  0},
    {EITHER,    076, 002, 000, true,  false, true,  0,		A,     "FM",	false,  0},
    {EITHER,    076, 003, 000, true,  false, true,  0,		A,     "FD",	false,  0},
    {EITHER,    076, 004, 000, true,  false, true,  0,		A,     "LUF",	false,  0},
    {EITHER,    076, 005, 000, true,  false, true,  0,		A,     "LCF",	false,  0},
    {EITHER,    076, 006, 000, true,  false, true,  0,		A,     "MCDU",	false,  0},
    {EITHER,    076, 007, 000, true,  false, true,  0,		A,     "CDU",	false,  0},
    {EITHER,    076, 010, 000, true,  false, true,  0,		A,     "DFA",	false,  0},
    {EITHER,    076, 011, 000, true,  false, true,  0,		A,     "DFAN",	false,  0},
    {EITHER,    076, 012, 000, true,  false, true,  0,		A,     "DFM",	false,  0},
    {EITHER,    076, 013, 000, true,  false, true,  0,		A,     "DFD",	false,  0},
    {EITHER,    076, 014, 000, true,  false, true,  0,		A,     "DFU",	false,  0},
    {EITHER,    076, 015, 000, true,  false, true,  0,		A,     "DLCF",	false,  0},
    {EITHER,    076, 016, 000, true,  false, true,  0,		A,     "FEL",	false,  0},
    {EITHER,    076, 017, 000, true,  false, true,  0,		A,     "FCL",	false,  0},
};

const char* const InstructionWord::m_JFieldNames[] =
{
    "W",        "H2",       "H1",       "XH2",
    "XH1/Q2",   "T3/Q4",    "T2/Q3",    "T1/Q1",
    "S6",       "S5",       "S4",       "S3",
    "S2",       "S1",       "U",        "XU"
};



//  private statics

//  handleBT()
//
//  BT for extended mode has a special syntax...
//      BT,j a,bd,*x,bs,*d
//  bd is base register for destination, bits 24-28 (MSB bit 0)
//  bs is base register for source, bits 20-23
//  d is displacement, bits 29-35
std::string
InstructionWord::handleBT
    (
    const InstructionWord& instruction
    )
{
    std::stringstream strm;
    strm << "BT";

    UINT32 j = instruction.getJ();
    if ( j != 0 )
        strm << "," << m_JFieldNames[j];
    strm << " ";

    while ( strm.str().size() < 12 )
        strm << " ";

//TODO:BUG Fix this:
//    When operating at PP < 2, the F0.i is used as an extension to
//    F0.bs but not F0.bd.
    strm << "X" << instruction.getA() << ",";
    strm << "B" << ((instruction.getW() & 07600) >> 7) << ",";

    if ( instruction.getH() )
        strm << "*";
    strm << "X" << instruction.getX() << ",";
    strm << "B" << (instruction.getS4() & 017) << ",";
    if ( instruction.getI() )
        strm << "*";
    strm << "0" << std::oct << (instruction.getW() & 0177);

    return strm.str();
}


//  handleER()
//
//  Special handling to interpret the u-field as an ER name.
std::string
InstructionWord::handleER
    (
    const InstructionWord& instruction
    )
{
    std::stringstream strm;
    strm << "ER          ";

    INDEX erIndex = instruction.getU();
    COUNT erCount = sizeof(m_ERNameTable) / sizeof(ERNameEntry);
    bool found = false;
    for ( INDEX tx = 0; tx < erCount; ++tx )
    {
        if ( m_ERNameTable[tx].m_Index == erIndex )
        {
            strm << m_ERNameTable[tx].m_Name;
            found = true;
            break;
        }
    }

    if ( !found )
        strm << std::hex << erIndex;

    UINT32 x = instruction.getX();
    if ( x > 0 )
    {
        strm << ",";
        if ( instruction.getH() )
            strm << "*";
        strm << "X" << x;
    }

    return strm.str();
}


//  handleJGD()
//
//  Creates the display string for the disassembly of a JGD instruction.
//  Special handling:
//      JGD a,*u,*x for either mode.
//  Concatenation of j-field and a-field limited to 7 bits, specifics a GRS location displayed in the a-field.
//  TODO:BUG need to call user conversion function (why? what does it do?)
std::string InstructionWord::handleJGD
    (
    const InstructionWord&  instruction
    )
{
    UINT32 j = instruction.getJ();
    UINT32 a = instruction.getA();
    UINT32 grsIndex = (( (j << 4) | a ) & 0x7F);

    std::stringstream strm;
    strm << "JGD         ";

    strm << GeneralRegister::m_Names[grsIndex] << ",";

    if ( instruction.getI() )
        strm << "*";
    strm << "0" << std::oct << instruction.getU();

    UINT32 x = instruction.getX();
    if ( x > 0 )
    {
        strm << ",";
        if ( instruction.getH() )
            strm << "*";
        strm << "X" << std::dec << x;
    }

    return strm.str();
}


//  interpret()
//
//  Static function which interprets a normal instruction - that is, the f-field defines an operation,
//  which is modified by the j, a, x, h, i, and u (or b and d) fields.
//
//  Parameters:
//      instruction:            instruction word being disassembled
//      mnemonic:               instruction mnemonic
//      extendedMode:           true to consider b and d fields, false to consider u field
//      aSemantics:             indicates the nature of the a field
//      jField:                 true to interpret the j-field, else false
//      grsFlag:                true to convert u-fields less than 0200 to GRS register designations
//                                  (if appropriate; not for j=U or XU, and not for EM b > 0)
//      forceBMSemantics:       true to force EM instruction to use u-field instead of b and d fields
//      execModeRegistersFlag:  true to display Exec registers instead of User registers for a and x fields
//      conversionFunction:     a function we *may* call if the calculated UField might be the result
//                                  of an external reference dereference - The called function *may*
//                                  return a string containing an alternate representation of the
//                                  u-field which we will use in place of the octal value.
//                                  If the function returns an empty string, we should go ahead and
//                                  use the octal value anyway.
//                                  If 0, we ignore this and the next parameter.
//      pClientData:            an opaque value (probably a pointer) which we pass to the function
//                                  given to us in the previous parameter.
//
//  Returns:
//      disassembled string
std::string
InstructionWord::interpret
    (
    const InstructionWord&      instruction,
    const std::string&          mnemonic,
    const bool                  extendedMode,
    const AFieldSemantics       aSemantics,
    const bool                  jField,
    const bool                  grsFlag,
    const bool                  forceBMSemantics,
    const bool                  execModeRegistersFlag,
    INTERPRETCONVERSIONFUNCTION conversionFunction,
    const void* const           pClientData
    )
{
    // setup
    UINT32 j = instruction.getJ();
    UINT32 a = instruction.getA();
    UINT32 x = instruction.getX();
    bool h = instruction.getH();
    bool i = instruction.getI();
    UINT32 u = instruction.getU();
    UINT32 b = instruction.getB();
    UINT32 d = instruction.getD();

    //  are we going to convert the u (or d) field to a GRS register?
    //  If this is normal grs conversion, but ,u or ,xu, no.
    //  If we are in extended mode and not using B0, no.
    //  If we are indexing at all, no.
    //  In each of these cases, it is exceedingly unlikely that the coder
    //  meant to reference the GRS.  Possible, but unlikely.
    bool grsConvert = grsFlag;
    if ( ( grsConvert && (j >= 016) ) || ( extendedMode && (b > 0) ) || ( x > 0 ))
        grsConvert = false;

    std::stringstream strm;
    strm << mnemonic;

    // develop j field string, with leading ',' if field is not blank
    if ( jField && (j > 0) )
        strm << "," << m_JFieldNames[j];

    strm << " ";
    while ( strm.str().size() < 12 )
        strm << " ";

    switch ( aSemantics )
    {
    case A:
        if ( execModeRegistersFlag )
            strm << "E";
        strm << "A" << a << ",";
        break;
    case B:
        strm << "B" << a << ",";
        break;
    case BEXEC:
        strm << "B" << a + 16 << ",";
        break;
    case R:
        if ( execModeRegistersFlag )
            strm << "E";
        strm << "R" << a << ",";
        break;
    case X:
        if ( execModeRegistersFlag )
            strm << "E";
        strm << "X" << a << ",";
        break;
    case NONE:
        break;
    }

    bool immediate = jField && ( j >= 016 ) ? true : false;

    if ( grsConvert && (u < 0200) )
    {
        // Use the GRS register name for the u field.
        strm << GeneralRegister::m_Names[u];
    }
    else if ( extendedMode && !forceBMSemantics && !immediate )
    {
        // Extended mode, BM not forced (i.e., not a jump or similar), and not immediate...
        // Decode the d field
        if ( conversionFunction )
        {
            InterpretConversionData icData;
            icData.m_BasicModeFlag = false;
            icData.m_DField = static_cast<UINT16>(d);
            icData.m_BaseRegister = static_cast<UINT16>(b);
            std::string result = conversionFunction( pClientData, icData );
            if ( result.size() > 0 )
                strm << result;
            else
                strm << "0" << std::oct << d;
        }
        else
            strm << "0" << std::oct << d;
    }
    else
    {
        // Decode the u field
        if ( (x > 0) && i )
            strm << "*";
        if ( conversionFunction )
        {
            InterpretConversionData icData;
            icData.m_BasicModeFlag = true;
            icData.m_UField = u;
            std::string result = conversionFunction( pClientData, icData );
            if ( result.size() > 0 )
                strm << result;
            else
                strm << "0" << std::oct << u;
        }
        else
            strm << "0" << std::oct << u;
    }

    // x field
    if ( x > 0 )
    {
        strm << ",";
        if ( h )
            strm << "*";
        if ( execModeRegistersFlag )
            strm << "E";
        strm << "X" << std::dec << x;
    }

    //  Doing EM, BM not forced, and no GRS conversion, and no ,U or ,XU, we show B register
    if ( extendedMode && !forceBMSemantics && !grsConvert && !immediate )
    {
        //  ...if there wasn't an x, we need an extra comma
        if ( x == 0 )
            strm << ",";
        int effective_b = b;
        if ( i )
            effective_b += 16;
        strm << ",B" << std::dec << effective_b;
    }

    return strm.str();
}



// constructors not implemented in the header file

InstructionWord::InstructionWord
    (
    const UINT16 f,
    const UINT8 j,
    const UINT8 a,
    const UINT8 x,
    const bool h,
    const bool i,
    const UINT16 u 
    )
{
    UINT64 value = static_cast<UINT64>(f & 077) << 30;
    value |= static_cast<UINT32>(j & 017) << 26;
    value |= static_cast<UINT32>(a & 017) << 22;
    value |= static_cast<UINT32>(x & 017) << 18;
    if ( h )
        value |= 0400000;
    if ( i )
        value |= 0200000;
    value |= u;
    setValue( value );
}


InstructionWord::InstructionWord
    (
    const UINT16 f,
    const UINT8 j,
    const UINT8 a,
    const UINT8 x,
    const bool h,
    const bool i,
    const UINT8 b,
    const UINT16 d 
    )
{
    UINT64 value = static_cast<UINT64>(f & 077) << 30;
    value |= static_cast<UINT32>(j & 017) << 26;
    value |= static_cast<UINT32>(a & 017) << 22;
    value |= static_cast<UINT32>(x & 017) << 18;
    if ( h )
        value |= 0400000;
    if ( i )
        value |= 0200000;
    value |= static_cast<UINT32>(b & 017) << 12;
    value |= d & 07777;
    setValue( value );
}


InstructionWord::InstructionWord
    (
    const UINT16 f,
    const UINT8 j,
    const UINT8 a,
    const UINT8 x,
    const UINT32 u 
    )
{
    UINT64 value = static_cast<UINT64>(f & 077) << 30;
    value |= static_cast<UINT32>(j & 017) << 26;
    value |= static_cast<UINT32>(a & 017) << 22;
    value |= static_cast<UINT32>(x & 017) << 18;
    value |= u & 0777777;
    setValue( value );
}


//  public statics

//  getMnemonic()
//
//  static function
//
//  Disassembles an instruction far enough to deduce its mnemonic
//
//  Parameters:
//      instruction:            instruction to be interpreted
//      extendedMode:           disassembly is to assume extended mode - false implies basic mode
std::string
InstructionWord::getMnemonic
    (
    const InstructionWord&  instruction,
    const bool              extendedMode
    )
{
    UINT32 f = instruction.getF();
    UINT32 j = instruction.getJ();
    UINT32 a = instruction.getA();

    COUNT ic = sizeof(m_InstructionInfoTable) / sizeof(InstructionInfo);
    for ( INDEX ix = 0; ix < ic; ++ix )
    {
        if ( m_InstructionInfoTable[ix].m_FField == f )
        {
            if ( ( m_InstructionInfoTable[ix].m_Mode == EITHER )
                || ( ( m_InstructionInfoTable[ix].m_Mode == EXTENDED ) && extendedMode )
                || ( ( m_InstructionInfoTable[ix].m_Mode == BASIC ) && !extendedMode ) )
            {
                if ( m_InstructionInfoTable[ix].m_JFlag && ( m_InstructionInfoTable[ix].m_JField != j ) )
                    continue;
                if ( m_InstructionInfoTable[ix].m_AFlag && ( m_InstructionInfoTable[ix].m_AField != a ) )
                    continue;

                return m_InstructionInfoTable[ix].m_Mnemonic;
            }
        }
    }

    return "";
}


//  interpret()
//
//  static function
//
//  Disassembles an instruction word into a displayable string
//
//  Parameters:
//      instruction:            instruction to be interpreted
//      extendedMode:           disassembly is to assume extended mode - false implies basic mode
//      execModeRegistersFlag:  true to display exec registers instead of user registers for a and x fields
//      conversionFunction:     a function we *may* call if the calculated UField might be the result
//                                  of an external reference dereference - The called function *may*
//                                  return a string containing an alternate representation of the
//                                  u-field which we will use in place of the octal value.
//                                  If the function returns an empty string, we should go ahead and
//                                  use the octal value anyway.
//                                  If 0, we ignore this and the next parameter.
//      pClientData:            an opaque value (probably a pointer) which we pass to the function
//                                  given to us in the previous parameter.
std::string
InstructionWord::interpret
    (
    const InstructionWord&      instruction,
    const bool                  extendedMode,
    const bool                  execModeRegistersFlag,
    INTERPRETCONVERSIONFUNCTION conversionFunction,
    const void* const           pClientData
    )
{
    UINT32 f = instruction.getF();
    UINT32 j = instruction.getJ();
    UINT32 a = instruction.getA();

    COUNT ic = sizeof(m_InstructionInfoTable) / sizeof(InstructionInfo);
    for ( INDEX ix = 0; ix < ic; ++ix )
    {
        if ( m_InstructionInfoTable[ix].m_FField == f )
        {
            if ( ( m_InstructionInfoTable[ix].m_Mode == EITHER )
                || ( ( m_InstructionInfoTable[ix].m_Mode == EXTENDED ) && extendedMode )
                || ( ( m_InstructionInfoTable[ix].m_Mode == BASIC ) && !extendedMode ) )
            {
                if ( m_InstructionInfoTable[ix].m_JFlag && ( m_InstructionInfoTable[ix].m_JField != j ) )
                    continue;
                if ( m_InstructionInfoTable[ix].m_AFlag && ( m_InstructionInfoTable[ix].m_AField != a ) )
                    continue;
                if ( m_InstructionInfoTable[ix].m_Handler != 0 )
                    return m_InstructionInfoTable[ix].m_Handler( instruction );
                else
                {
                    bool jField = !m_InstructionInfoTable[ix].m_JFlag;
                    return interpret( instruction,
                                    m_InstructionInfoTable[ix].m_Mnemonic,
                                    extendedMode,
                                    m_InstructionInfoTable[ix].m_ASemantics,
                                    jField,
                                    m_InstructionInfoTable[ix].m_GRSFlag,
                                    m_InstructionInfoTable[ix].m_UseBMSemantics,
                                    execModeRegistersFlag,
                                    conversionFunction,
                                    pClientData );
                }
            }
        }
    }

    // Couldn't find a way to interpret the instruction - just use octal.
    std::stringstream strm;
    strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getF() << " ";
    strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getJ() << " ";
    strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getA() << " ";
    strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getX() << " ";
    strm << (instruction.getH() ? "1 " : "0 ");
    strm << (instruction.getI() ? "1 " : "0 ");
    if ( extendedMode )
    {
        strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getB() << " ";
        strm << std::oct << std::setw( 2 ) << std::setfill( '0' ) << instruction.getD();
    }
    else
        strm << std::oct << std::setw( 6 ) << std::setfill( '0' ) << instruction.getU();

    return strm.str();
}
