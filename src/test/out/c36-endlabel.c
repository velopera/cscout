Table: Ids
EID	NAME	READONLY	UNDEFMACRO	MACRO	MACROARG	ORDINARY	SUETAG	SUMEMBER	LABEL	TYPEDEF	ENUM	YACC	FUN	CSCOPE	LSCOPE	UNUSED
9	foo	0	0	0	0	1	0	0	0	0	0	0	1	0	1	0
91	MAXDIGIT	0	0	1	0	0	0	0	0	0	0	0	0	0	0	1
159	a	0	0	0	0	1	0	0	0	0	0	0	0	0	0	0
331	x	0	0	0	0	1	0	0	0	0	0	0	0	0	1	1
441	main	1	0	0	0	1	0	0	0	0	0	0	1	0	1	0
489	label	0	0	0	0	0	0	0	1	0	0	0	0	0	0	1
571	qqq	0	0	0	0	1	0	0	0	0	0	0	0	0	1	1
1239	nokey	0	0	0	0	0	0	0	1	0	0	0	0	0	0	1
2299	lfor1	0	0	0	0	0	0	0	1	0	0	0	0	0	0	1
2975	__DATE__	1	0	1	0	0	0	0	0	0	0	0	0	0	0	1
3295	__TIME__	1	0	1	0	0	0	0	0	0	0	0	0	0	0	1
3575	__FILE__	1	0	1	0	0	0	0	0	0	0	0	0	0	0	1
3865	__LINE__	1	0	1	0	0	0	0	0	0	0	0	0	0	0	1
3939	label2	0	0	0	0	0	0	0	1	0	0	0	0	0	0	1
4055	__STDC__	1	0	1	0	0	0	0	0	0	0	0	0	0	0	1
5355	_cscout_dummy1	1	0	0	0	1	0	0	0	0	0	0	1	1	0	0
Table: Tokens
FID	FOFFSET	EID
2	297	2975
2	329	3295
2	357	3575
2	386	3865
2	405	4055
2	471	441
2	535	5355
2	558	5355
4	0	9
4	15	159
4	48	489
4	55	159
4	123	1239
4	130	159
4	147	159
4	164	159
4	193	159
4	229	2299
4	250	159
4	316	9
4	393	3939
5	8	91
5	32	331
5	43	441
5	56	571
5	88	9
Table: Rest
FID	FOFFSET	CODE
2	287	\u0000a\u0000a#define 
2	305	  
2	320	\u0000a#define 
2	337	 
2	348	\u0000a#define 
2	365	 
2	377	\u0000a#define 
2	394	 1\u0000a#define 
2	413	 1\u0000a\u0000a
2	466	\u0000aint 
2	475	();\u0000a
2	522	\u0000astatic void 
2	549	(void) { 
2	572	(); }\u0000a
3	152	\u0000a\u0000a\u0000a#pragma includepath 
3	195	\u0000a
3	239	\u0000astatic void _cscout_dummy2(void) { _cscout_dummy2(); }\u0000a
4	3	()\u0000d\u0000a{\u0000d\u0000a	int 
4	16	;\u0000d\u0000a\u0000d\u0000a	
4	45	\u0000d\u0000a	
4	53	: 
4	56	 = 2;\u0000d\u0000a	if (1)\u0000d\u0000a		
4	119	\u0000d\u0000a		
4	128	: 
4	131	 = 3;\u0000d\u0000a	else\u0000d\u0000a		
4	148	 = 2;\u0000d\u0000a	switch (
4	165	) {\u0000d\u0000a	case 2:\u0000d\u0000a	}\u0000d\u0000a	switch (
4	194	) {\u0000d\u0000a	default:\u0000d\u0000a	}\u0000d\u0000a	for (;;) {\u0000d\u0000a		
4	234	:\u0000d\u0000a	}\u0000d\u0000a	switch (
4	251	) {\u0000d\u0000a	
4	281	\u0000d\u0000a	case 1:\u0000d\u0000a	case 2:\u0000d\u0000a	default:\u0000d\u0000a		
4	319	();\u0000d\u0000a	}\u0000d\u0000a	
4	390	\u0000d\u0000a	
4	399	:\u0000d\u0000a}\u0000d\u0000a
5	0	#define 
5	16	 11\u0000d\u0000aextern int 
5	33	;\u0000d\u0000aextern 
5	47	();\u0000d\u0000aint 
5	59	;\u0000d\u0000a\u0000d\u0000a
5	86	\u0000d\u0000a
5	91	() {\u0000d\u0000a	
5	116	}\u0000d\u0000a\u0000d\u0000a
Table: Projects
PID	NAME
16	unspecified
17	Prj1
18	Prj2
Table: IdProj
EID	PID
9	17
159	17
441	17
489	17
1239	17
2299	17
2975	17
3295	17
3575	17
3865	17
3939	17
4055	17
5355	17
9	18
91	18
159	18
331	18
441	18
489	18
571	18
1239	18
2299	18
2975	18
3295	18
3575	18
3865	18
3939	18
4055	18
5355	18
Table: Files
FID	NAME	RO
2	host-defs.h	1
3	host-incs.h	1
4	c36-endlabel.c	0
5	prj2.c	0
Table: Filemetrics
FID	PRECPP	NCHAR	NCCOMMENT	NSPACE	NLCOMMENT	NBCOMMENT	NLINE	MAXLINELEN	MAXSTMTLEN	MAXSTMTNEST	MAXBRACENEST	MAXBRACKNEST	BRACENEST	BRACKNEST	NULINE	NPPDIRECTIVE	NPPCOND	NPPFMACRO	NPPOMACRO	NTOKEN	NSTMT	NOP	NUOP	NNCONST	NCLIT	NSTRING	NPPCONCATOP	NPPSTRINGOP	NIF	NELSE	NSWITCH	NCASE	NDEFAULT	NBREAK	NFOR	NWHILE	NDO	NCONTINUE	NGOTO	NRETURN	NASM	NTYPEOF	NPID	NFID	NMID	NID	NUPID	NUFID	NUMID	NUID	NLABEL	NMACROEXPANDTOKEN	NCOPIES	NINCFILE	NPFUNCTION	NFFUNCTION	NPVAR	NFVAR	NAGGREGATE	NAMEMBER	NENUM	NEMEMBER
2	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	16	0	1	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	18	2	0	0	0	0	0	NULL	NULL	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	NULL	0	0	0	NULL	0	0	NULL	NULL	NULL	0	1	0	0	0	0	0	0
2	1	548	367	29	0	3	22	61	47	NULL	1	1	0	0	0	5	0	0	5	37	2	0	0	2	0	3	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	1	2	5	8	1	1	5	7	0	0	1	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	NULL
3	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	15	0	1	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	0	1	0	0	0	0	0	NULL	NULL	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	NULL	0	0	0	NULL	0	0	NULL	NULL	NULL	0	0	0	0	0	0	0	0
3	1	243	187	13	0	2	13	48	28	NULL	1	1	0	0	0	1	0	0	0	16	1	0	0	0	0	1	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	1	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	NULL
4	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	41	1	2	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	76	10	3	0	7	0	0	NULL	NULL	1	1	3	3	2	0	1	0	0	0	0	0	0	0	4	0	NULL	18	1	0	NULL	2	8	NULL	NULL	NULL	1	0	0	0	0	0	0	0
4	1	374	137	112	0	4	30	63	52	NULL	2	1	0	0	0	0	0	0	0	75	10	3	0	7	0	0	0	0	1	1	3	3	2	0	1	0	0	0	0	0	0	0	2	0	0	9	1	0	0	2	4	0	1	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	NULL
5	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	8	0	1	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	18	3	0	0	0	0	0	NULL	NULL	0	0	0	0	0	0	0	0	0	0	0	0	0	0	4	0	NULL	4	4	0	NULL	4	0	NULL	NULL	NULL	1	0	2	0	0	0	0	0
5	1	106	34	26	2	0	11	21	14	NULL	1	1	0	0	0	1	0	0	1	21	3	0	0	1	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	4	0	1	5	4	0	1	5	0	0	1	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	NULL
Table: FileProj
FID	PID
2	17
3	17
4	17
1	18
2	18
3	18
4	18
5	18
Table: Definers
PID	CUID	BASEFILEID	DEFINERID
18	5	5	2
18	5	5	4
Table: Includers
PID	CUID	BASEFILEID	INCLUDERID
17	2	2	1
17	4	3	1
17	4	4	1
18	2	2	1
18	2	2	1
18	4	3	1
18	4	4	1
18	5	3	1
18	5	5	1
Table: Providers
PID	CUID	PROVIDERID
17	2	2
17	4	4
18	2	2
18	2	2
18	4	4
18	5	5
Table: IncTriggers
PID	CUID	BASEFILEID	DEFINERID	FOFFSET	LEN
18	5	5	2	471	4
18	5	5	4	0	3
Table: Functions
ID	NAME	ISMACRO	DEFINED	DECLARED	FILESCOPED	FID	FOFFSET	FANIN
891	foo	0	1	1	0	5	88	1
4715	main	0	0	1	0	2	471	0
5355	_cscout_dummy1	0	1	1	1	2	535	1
Table: FunctionDefs
FUNCTIONID	FIDBEGIN	FOFFSETBEGIN	FIDEND	FOFFSETEND
891	4	8	4	403
5355	2	557	2	577
Table: FunctionMetrics
FUNCTIONID	PRECPP	NCHAR	NCCOMMENT	NSPACE	NLCOMMENT	NBCOMMENT	NLINE	MAXLINELEN	MAXSTMTLEN	MAXSTMTNEST	MAXBRACENEST	MAXBRACKNEST	BRACENEST	BRACKNEST	NULINE	NPPDIRECTIVE	NPPCOND	NPPFMACRO	NPPOMACRO	NTOKEN	NSTMT	NOP	NUOP	NNCONST	NCLIT	NSTRING	NPPCONCATOP	NPPSTRINGOP	NIF	NELSE	NSWITCH	NCASE	NDEFAULT	NBREAK	NFOR	NWHILE	NDO	NCONTINUE	NGOTO	NRETURN	NASM	NTYPEOF	NPID	NFID	NMID	NID	NUPID	NUFID	NUMID	NUID	NLABEL	NMACROEXPANDTOKEN	NGNSOC	NMPARAM	NFPARAM	NEPARAM	FANIN	FANOUT	CCYCL1	CCYCL2	CCYCL3	CSTRUC	CHAL	IFLOW
891	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	41	1	1	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	71	10	3	1	7	0	0	NULL	NULL	1	1	3	3	2	0	1	0	0	0	0	0	0	0	1	0	NULL	8	1	0	NULL	2	4	NULL	16	NULL	0	NULL	1	1	6	6	6	1.0	59.7947	6.0
891	1	364	137	109	0	4	28	63	52	NULL	1	1	0	0	0	0	0	0	0	71	10	3	1	7	0	0	0	0	1	1	3	3	2	0	1	0	0	0	0	0	0	0	1	0	0	8	1	0	0	2	4	0	NULL	0	NULL	0	1	1	6	6	6	1.0	59.7947	6.0
5355	0	NULL	NULL	NULL	NULL	NULL	NULL	NULL	5	0	0	1	NULL	NULL	NULL	NULL	NULL	NULL	NULL	5	1	0	0	0	0	0	NULL	NULL	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	1	NULL	1	0	1	NULL	1	0	NULL	9	NULL	0	NULL	1	1	1	1	1	1.0	0.0	1.0
5355	1	22	0	3	0	0	1	21	5	NULL	0	1	0	0	0	0	0	0	0	5	1	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	0	1	0	1	0	1	0	1	0	0	NULL	0	NULL	0	1	1	1	1	1	1.0	0.0	1.0
Table: FunctionId
FUNCTIONID	ORDINAL	EID
891	0	9
4715	0	441
5355	0	5355
Table: Fcalls
SOURCEID	DESTID
891	891
5355	5355
Done
