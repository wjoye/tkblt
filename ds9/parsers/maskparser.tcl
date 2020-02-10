package provide DS9 1.0

######
# Begin autogenerated taccle (version 1.3) routines.
# Although taccle itself is protected by the GNU Public License (GPL)
# all user-supplied functions are protected by their respective
# author's license.  See http://mini.net/tcl/taccle for other details.
######

namespace eval mask {
    variable yylval {}
    variable table
    variable rules
    variable token {}
    variable yycnt 0
    variable yyerr 0
    variable save_state 0

    namespace export yylex
}

proc mask::YYABORT {} {
    return -code return 1
}

proc mask::YYACCEPT {} {
    return -code return 0
}

proc mask::YYERROR {} {
    variable yyerr
    set yyerr 1
}

proc mask::yyclearin {} {
    variable token
    variable yycnt
    set token {}
    incr yycnt -1
}

proc mask::yyerror {s} {
    puts stderr $s
}

proc mask::setupvalues {stack pointer numsyms} {
    upvar 1 1 y
    set y {}
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        set y [lindex $stack $pointer]
        incr pointer
    }
}

proc mask::unsetupvalues {numsyms} {
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        unset y
    }
}

array set mask::table {
  21:289 reduce
  21:290 reduce
  60:290 reduce
  59:290 shift
  30:290,target 7
  29:290,target 6
  27:0 reduce
  9:288 shift
  48:0 reduce
  8:287,target 54
  9:289 shift
  30:290 reduce
  29:290 reduce
  18:290,target 52
  6:0,target 38
  8:266,target 33
  2:0 reduce
  9:306 goto
  56:0,target 54
  53:290,target 30
  38:290 reduce
  24:0 reduce
  48:0,target 25
  9:289,target 22
  41:0,target 18
  45:0 reduce
  33:0,target 10
  25:0,target 2
  0:294,target 4
  8:285,target 52
  17:0,target 49
  42:290,target 19
  10:0,target 0
  47:290 reduce
  5:303,target 19
  8:264,target 31
  31:290,target 8
  17:290 reduce
  21:0 reduce
  56:290 reduce
  42:0 reduce
  0:302,target 9
  0:292,target 2
  8:283,target 50
  11:308,target 59
  19:290,target 48
  20:290,target 43
  6:290 reduce
  26:290 reduce
  8:262,target 29
  54:290,target 31
  17:0 reduce
  38:0 reduce
  35:290 reduce
  43:290,target 20
  60:0 reduce
  0:300,target 7
  0:290,target 37
  8:281,target 48
  61:0,target 36
  5:288,target 14
  53:0,target 30
  44:290 reduce
  32:290,target 9
  8:259,target 26
  8:260,target 27
  45:0,target 22
  37:0,target 14
  14:0 reduce
  30:0,target 7
  29:0,target 6
  22:0,target 33
  35:0 reduce
  14:0,target 47
  14:290 reduce
  21:289,target 32
  21:290,target 32
  56:0 reduce
  53:290 reduce
  8:278,target 45
  3:290 shift
  23:288 shift
  55:290,target 53
  23:289 shift
  23:306,target 60
  8:257,target 24
  11:0 reduce
  32:0 reduce
  44:290,target 21
  23:306 goto
  8:257 shift
  1:290,target 41
  8:258 shift
  53:0 reduce
  32:290 reduce
  8:259 shift
  8:260 shift
  8:261 shift
  8:262 shift
  8:276,target 43
  8:263 shift
  33:290,target 10
  8:264 shift
  8:265 shift
  6:0 reduce
  8:266 shift
  8:267 shift
  41:290 reduce
  8:268 shift
  8:269 shift
  8:270 shift
  28:0 reduce
  22:290,target 33
  22:289,target 33
  8:271 shift
  0:0,target 37
  8:305,target 56
  8:272 shift
  8:273 shift
  50:0 reduce
  49:0 reduce
  8:274 shift
  11:290 reduce
  8:275 shift
  57:0,target 45
  8:276 shift
  50:290 reduce
  50:0,target 27
  49:290 reduce
  49:0,target 26
  8:274,target 41
  8:277 shift
  11:290,target 35
  56:290,target 54
  42:0,target 19
  8:278 shift
  34:0,target 11
  8:279 shift
  8:280 shift
  26:0,target 3
  0:300 shift
  0:290 reduce
  8:281 shift
  0:301 shift
  0:291 shift
  8:282 shift
  18:0,target 52
  0:302 shift
  0:292 shift
  8:283 shift
  11:0,target 34
  11:308 goto
  19:290 reduce
  20:290 reduce
  0:293 shift
  8:284 shift
  45:290,target 22
  25:0 reduce
  0:294 shift
  8:285 shift
  58:290 reduce
  2:290,target 40
  0:295 shift
  8:286 shift
  8:287 shift
  46:0 reduce
  0:307 goto
  0:309 goto
  0:299 shift
  7:288,target 21
  34:290,target 11
  28:290 reduce
  8:272,target 39
  8:304 goto
  0:0 reduce
  8:305 goto
  23:289,target 22
  22:0 reduce
  37:290 reduce
  8:311 goto
  43:0 reduce
  0:309,target 11
  0:299,target 6
  12:290,target 42
  57:290,target 45
  5:298,target 17
  8:269,target 36
  8:270,target 37
  46:290 reduce
  46:290,target 23
  3:290,target 12
  18:0 reduce
  16:290 reduce
  54:0,target 31
  40:0 reduce
  39:0 reduce
  55:290 reduce
  46:0,target 23
  0:307,target 10
  38:0,target 15
  35:290,target 12
  61:0 reduce
  31:0,target 8
  5:288 shift
  5:300 shift
  5:296,target 15
  15:0,target 50
  25:290 reduce
  8:267,target 34
  5:303 shift
  24:290,target 1
  5:296 shift
  15:0 reduce
  5:297 shift
  5:298 shift
  36:0 reduce
  5:310 goto
  34:290 reduce
  13:290,target 39
  58:290,target 46
  0:295,target 5
  8:286,target 53
  57:0 reduce
  8:265,target 32
  47:290,target 24
  43:290 reduce
  4:290,target 13
  12:0 reduce
  33:0 reduce
  9:288,target 21
  36:290,target 13
  13:290 reduce
  54:0 reduce
  0:293,target 3
  8:284,target 51
  52:290 reduce
  1:0,target 41
  25:290,target 2
  2:290 reduce
  8:263,target 30
  22:288 reduce
  22:290 reduce
  22:289 reduce
  58:0,target 46
  51:0,target 28
  43:0,target 20
  35:0,target 12
  30:0 reduce
  29:0 reduce
  14:290,target 47
  60:290,target 44
  59:290,target 61
  27:0,target 4
  19:0,target 48
  20:0,target 43
  51:0 reduce
  12:0,target 42
  31:290 reduce
  0:301,target 8
  0:291,target 1
  8:282,target 49
  48:290,target 25
  5:300,target 18
  8:261,target 28
  40:290 reduce
  39:290 reduce
  37:290,target 14
  26:0 reduce
  8:311,target 57
  47:0 reduce
  7:306,target 23
  8:279,target 46
  8:280,target 47
  26:290,target 3
  48:290 reduce
  1:0 reduce
  8:258,target 25
  15:290,target 50
  18:290 reduce
  57:290 reduce
  44:0 reduce
  21:288,target 32
  7:288 shift
  7:289 shift
  50:290,target 27
  49:290,target 26
  8:277,target 44
  27:290 reduce
  6:290,target 38
  55:0,target 53
  47:0,target 24
  40:0,target 17
  39:0,target 16
  32:0,target 9
  7:306 goto
  38:290,target 15
  24:0,target 1
  16:0,target 51
  19:0 reduce
  20:0 reduce
  36:290 reduce
  41:0 reduce
  27:290,target 4
  8:275,target 42
  45:290 reduce
  16:290,target 51
  16:0 reduce
  22:288,target 33
  15:290 reduce
  37:0 reduce
  8:304,target 55
  54:290 reduce
  51:290,target 28
  58:0 reduce
  7:289,target 22
  4:290 shift
  8:273,target 40
  24:290 reduce
  40:290,target 17
  39:290,target 16
  13:0 reduce
  2:0,target 40
  9:306,target 58
  34:0 reduce
  28:290,target 5
  60:0,target 44
  33:290 reduce
  55:0 reduce
  52:0,target 29
  44:0,target 21
  36:0,target 13
  5:310,target 20
  28:0,target 5
  8:271,target 38
  17:290,target 49
  21:0,target 32
  13:0,target 39
  42:290 reduce
  10:0 accept
  23:288,target 21
  31:0 reduce
  52:290,target 29
  12:290 reduce
  52:0 reduce
  51:290 reduce
  5:297,target 16
  41:290,target 18
  8:268,target 35
  1:290 reduce
  21:288 reduce
}

array set mask::rules {
  9,l 305
  11,l 305
  32,l 306
  53,l 311
  6,l 305
  28,l 305
  50,l 310
  49,l 310
  3,l 304
  25,l 305
  46,l 309
  0,l 312
  22,l 305
  43,l 309
  18,l 305
  40,l 309
  39,l 309
  15,l 305
  36,l 307
  12,l 305
  33,l 306
  54,l 311
  7,l 305
  29,l 305
  30,l 305
  51,l 310
  4,l 304
  26,l 305
  47,l 309
  1,l 304
  23,l 305
  44,l 309
  19,l 305
  20,l 305
  41,l 309
  16,l 305
  37,l 309
  13,l 305
  34,l 307
  8,l 305
  10,l 305
  31,l 305
  52,l 310
  5,l 305
  27,l 305
  48,l 310
  2,l 304
  24,l 305
  45,l 309
  21,l 305
  42,l 309
  17,l 305
  38,l 309
  14,l 305
  35,l 308
}

array set mask::rules {
  12,dc 1
  26,dc 1
  3,dc 1
  41,dc 1
  18,dc 1
  33,dc 1
  9,dc 1
  47,dc 2
  11,dc 1
  25,dc 1
  2,dc 1
  40,dc 1
  39,dc 2
  54,dc 1
  17,dc 1
  32,dc 1
  8,dc 1
  46,dc 2
  10,dc 1
  24,dc 1
  1,dc 1
  38,dc 1
  53,dc 1
  16,dc 1
  31,dc 1
  7,dc 1
  45,dc 2
  23,dc 1
  0,dc 1
  37,dc 0
  52,dc 1
  15,dc 1
  29,dc 1
  30,dc 1
  6,dc 1
  44,dc 3
  22,dc 1
  36,dc 3
  51,dc 1
  14,dc 1
  28,dc 1
  5,dc 1
  43,dc 2
  21,dc 1
  35,dc 0
  50,dc 1
  49,dc 1
  13,dc 1
  27,dc 1
  4,dc 1
  42,dc 2
  19,dc 1
  20,dc 1
  34,dc 1
  48,dc 1
}

array set mask::rules {
  41,line 145
  7,line 107
  37,line 141
  4,line 103
  34,line 137
  1,line 100
  31,line 131
  27,line 127
  24,line 124
  21,line 121
  17,line 117
  14,line 114
  11,line 111
  53,line 162
  50,line 157
  49,line 156
  46,line 151
  43,line 147
  9,line 109
  40,line 144
  39,line 143
  6,line 106
  36,line 138
  3,line 102
  33,line 134
  29,line 129
  30,line 130
  26,line 126
  23,line 123
  19,line 119
  20,line 120
  16,line 116
  13,line 113
  10,line 110
  52,line 159
  48,line 155
  45,line 149
  42,line 146
  8,line 108
  38,line 142
  5,line 105
  35,line 137
  2,line 101
  32,line 133
  28,line 128
  25,line 125
  22,line 122
  18,line 118
  15,line 115
  12,line 112
  54,line 163
  35,e 1
  51,line 158
  47,line 152
  44,line 148
}

array set mask::lr1_table {
  35 {{12 {0 290} 1}}
  14,trans {}
  36 {{13 {0 290} 1}}
  33,trans {}
  37 {{14 {0 290} 1}}
  52,trans {}
  38 {{15 {0 290} 1}}
  40 {{17 {0 290} 1}}
  39 {{16 {0 290} 1}}
  18,trans {}
  41 {{18 {0 290} 1}}
  1,trans {}
  37,trans {}
  42 {{19 {0 290} 1}}
  56,trans {}
  43 {{20 {0 290} 1}}
  44 {{21 {0 290} 1}}
  23,trans {{288 58} {289 59} {306 62}}
  45 {{22 {0 290} 1}}
  5,trans {{288 14} {296 15} {297 16} {298 17} {300 18} {303 19} {310 20}}
  42,trans {}
  46 {{23 {0 290} 1}}
  61,trans {{290 63}}
  47 {{24 {0 290} 1}}
  48 {{25 {0 290} 1}}
  27,trans {}
  9,trans {{288 58} {289 59} {306 60}}
  50 {{27 {0 290} 1}}
  49 {{26 {0 290} 1}}
  46,trans {}
  51 {{28 {0 290} 1}}
  52 {{29 {0 290} 1}}
  13,trans {}
  53 {{30 {0 290} 1}}
  32,trans {}
  54 {{31 {0 290} 1}}
  51,trans {}
  55 {{53 {0 290} 1}}
  56 {{54 {0 290} 1}}
  17,trans {}
  57 {{45 {0 290} 2}}
  0,trans {{291 1} {292 2} {293 3} {294 4} {295 5} {299 6} {300 7} {301 8} {302 9} {307 10} {309 11}}
  36,trans {}
  58 {{32 {0 290} 1}}
  55,trans {}
  60 {{46 {0 290} 2}}
  59 {{33 {0 290} 1}}
  61 {{36 0 2}}
  62 {{44 {0 290} 3}}
  22,trans {}
  4,trans {{290 13}}
  63 {{36 0 3}}
  41,trans {}
  60,trans {}
  59,trans {}
  26,trans {}
  8,trans {{257 24} {258 25} {259 26} {260 27} {261 28} {262 29} {263 30} {264 31} {265 32} {266 33} {267 34} {268 35} {269 36} {270 37} {271 38} {272 39} {273 40} {274 41} {275 42} {276 43} {277 44} {278 45} {279 46} {280 47} {281 48} {282 49} {283 50} {284 51} {285 52} {286 53} {287 54} {304 55} {305 56} {311 57}}
  45,trans {}
  12,trans {}
  31,trans {}
  50,trans {}
  49,trans {}
  16,trans {}
  35,trans {}
  54,trans {}
  21,trans {}
  3,trans {{290 12}}
  40,trans {}
  39,trans {}
  58,trans {}
  10 {{0 0 1}}
  11 {{34 0 1} {36 0 1} {35 290 0}}
  25,trans {}
  12 {{42 {0 290} 2}}
  7,trans {{288 21} {289 22} {306 23}}
  44,trans {}
  13 {{39 {0 290} 2}}
  63,trans {}
  14 {{47 {0 290} 2}}
  11,trans {{308 61}}
  15 {{50 {0 290} 1}}
  30,trans {}
  29,trans {}
  16 {{51 {0 290} 1}}
  48,trans {}
  0 {{0 0 0} {34 0 0} {36 0 0} {37 {0 290} 0} {38 {0 290} 0} {39 {0 290} 0} {40 {0 290} 0} {41 {0 290} 0} {42 {0 290} 0} {43 {0 290} 0} {44 {0 290} 0} {45 {0 290} 0} {46 {0 290} 0} {47 {0 290} 0}}
  17 {{49 {0 290} 1}}
  1 {{41 {0 290} 1}}
  18 {{52 {0 290} 1}}
  15,trans {}
  2 {{40 {0 290} 1}}
  19 {{48 {0 290} 1}}
  20 {{43 {0 290} 2}}
  34,trans {}
  3 {{42 {0 290} 1}}
  21 {{32 {288 289} 1}}
  53,trans {}
  4 {{39 {0 290} 1}}
  22 {{33 {288 289} 1}}
  5 {{43 {0 290} 1} {47 {0 290} 1} {48 {0 290} 0} {49 {0 290} 0} {50 {0 290} 0} {51 {0 290} 0} {52 {0 290} 0}}
  23 {{44 {0 290} 2} {32 {0 290} 0} {33 {0 290} 0}}
  20,trans {}
  19,trans {}
  6 {{38 {0 290} 1}}
  2,trans {}
  24 {{1 {0 290} 1}}
  38,trans {}
  7 {{44 {0 290} 1} {32 {288 289} 0} {33 {288 289} 0}}
  25 {{2 {0 290} 1}}
  57,trans {}
  8 {{45 {0 290} 1} {53 {0 290} 0} {54 {0 290} 0} {1 {0 290} 0} {2 {0 290} 0} {3 {0 290} 0} {4 {0 290} 0} {5 {0 290} 0} {6 {0 290} 0} {7 {0 290} 0} {8 {0 290} 0} {9 {0 290} 0} {10 {0 290} 0} {11 {0 290} 0} {12 {0 290} 0} {13 {0 290} 0} {14 {0 290} 0} {15 {0 290} 0} {16 {0 290} 0} {17 {0 290} 0} {18 {0 290} 0} {19 {0 290} 0} {20 {0 290} 0} {21 {0 290} 0} {22 {0 290} 0} {23 {0 290} 0} {24 {0 290} 0} {25 {0 290} 0} {26 {0 290} 0} {27 {0 290} 0} {28 {0 290} 0} {29 {0 290} 0} {30 {0 290} 0} {31 {0 290} 0}}
  26 {{3 {0 290} 1}}
  9 {{46 {0 290} 1} {32 {0 290} 0} {33 {0 290} 0}}
  27 {{4 {0 290} 1}}
  24,trans {}
  6,trans {}
  28 {{5 {0 290} 1}}
  43,trans {}
  29 {{6 {0 290} 1}}
  30 {{7 {0 290} 1}}
  62,trans {}
  31 {{8 {0 290} 1}}
  10,trans {}
  32 {{9 {0 290} 1}}
  28,trans {}
  33 {{10 {0 290} 1}}
  47,trans {}
  34 {{11 {0 290} 1}}
}

array set mask::token_id_table {
  286 WCSY_
  286,t 0
  287 WCSZ_
  292,line 48
  302,line 58
  288 INT_
  265,title WCSD
  289 REAL_
  290 STRING_
  300 RANGE_
  284,title WCSW
  291 CLEAR_
  301 SYSTEM_
  292 CLOSE_
  302 TRANSPARENCY_
  288,line 40
  293 COLOR_
  303 ZERO_
  304 coordsys
  294 LOAD_
  305 wcssys
  295 MARK_
  306 numeric
  296 NAN_
  307 command
  262,t 0
  297 NONNAN_
  308 @PSEUDO1
  285,line 36
  298 NONZERO_
  310 mark
  309 mask
  299 OPEN_
  311 system
  283,t 0
  312 start'
  282,line 33
  264,title WCSC
  283,title WCSV
  312,title {}
  278,line 29
  error,line 98
  258,t 0
  275,line 26
  279,t 0
  280,t 0
  311,t 1
  272,line 23
  263,title WCSB
  282,title WCSU
  311,title {}
  268,line 19
  276,t 0
  265,line 16
  307,t 1
  297,t 0
  262,line 13
  262,title WCSA
  0 {$}
  0,t 0
  281,title WCST
  310,title {}
  309,title {}
  error,t 0
  299,title OPEN
  258,line 8
  273,t 0
  304,t 1
  294,t 0
  261,title WCS
  279,title WCSR
  280,title WCSS
  308,title {}
  269,t 0
  270,t 0
  298,title NONZERO
  291,t 0
  301,t 0
  311,line 161
  307,line 136
  266,t 0
  260,title DETECTOR
  259,title AMPLIFIER
  297,line 53
  278,title WCSQ
  307,title {}
  297,title NONNAN
  287,t 0
  304,line 99
  294,line 50
  291,line 47
  301,line 57
  error,title {}
  263,t 0
  258,title PHYSICAL
  287,line 38
  277,title WCSP
  284,t 0
  306,title {}
  296,title NAN
  284,line 35
  281,line 32
  260,t 0
  259,t 0
  281,t 0
  257,title IMAGE
  277,line 28
  276,title WCSO
  312,t 1
  305,title {}
  295,title MARK
  274,line 25
  271,line 22
  277,t 0
  308,t 1
  267,line 18
  298,t 0
  275,title WCSN
  304,title {}
  294,title LOAD
  264,line 15
  261,line 12
  274,t 0
  305,t 1
  295,t 0
  257,line 7
  274,title WCSM
  293,title COLOR
  303,title ZERO
  271,t 0
  error error
  292,t 0
  302,t 0
  273,title WCSL
  292,title CLOSE
  302,title TRANSPARENCY
  267,t 0
  310,line 154
  309,line 140
  299,line 55
  288,t 0
  306,line 132
  296,line 52
  272,title WCSK
  291,title CLEAR
  293,line 49
  301,title SYSTEM
  303,line 59
  264,t 0
  285,t 0
  289,line 41
  290,line 43
  300,line 56
  286,line 37
  271,title WCSJ
  261,t 0
  283,line 34
  289,title float
  290,title string
  300,title RANGE
  282,t 0
  279,line 30
  280,line 31
  276,line 27
  257,t 0
  269,title WCSH
  270,title WCSI
  273,line 24
  288,title integer
  278,t 0
  310,t 1
  309,t 1
  299,t 0
  269,line 20
  270,line 21
  266,line 17
  268,title WCSG
  275,t 0
  263,line 14
  287,title WCSZ
  306,t 1
  296,t 0
  260,line 10
  259,line 9
  272,t 0
  267,title WCSF
  257 IMAGE_
  286,title WCSY
  293,t 0
  303,t 0
  258 PHYSICAL_
  260 DETECTOR_
  259 AMPLIFIER_
  261 WCS_
  262 WCSA_
  263 WCSB_
  264 WCSC_
  265 WCSD_
  266 WCSE_
  267 WCSF_
  268,t 0
  268 WCSG_
  269 WCSH_
  270 WCSI_
  312,line 164
  271 WCSJ_
  272 WCSK_
  289,t 0
  290,t 0
  300,t 0
  266,title WCSE
  273 WCSL_
  274 WCSM_
  285,title WCSX
  275 WCSN_
  308,line 137
  276 WCSO_
  298,line 54
  277 WCSP_
  278 WCSQ_
  279 WCSR_
  280 WCSS_
  281 WCST_
  305,line 104
  282 WCSU_
  295,line 51
  265,t 0
  283 WCSV_
  284 WCSW_
  285 WCSX_
}

proc mask::yyparse {} {
    variable yylval
    variable table
    variable rules
    variable token
    variable yycnt
    variable lr1_table
    variable token_id_table
    variable yyerr
    variable save_state

    set yycnt 0
    set state_stack {0}
    set value_stack {{}}
    set token ""
    set accepted 0
    set yyerr 0
    set save_state 0

    while {$accepted == 0} {
        set state [lindex $state_stack end]
        if {$token == ""} {
            set yylval ""
            set token [yylex]
            set buflval $yylval
	    if {$token>0} {
	        incr yycnt
            }
        }
        if {![info exists table($state:$token)] || $yyerr} {
	    if {!$yyerr} {
	        set save_state $state
	    }
            # pop off states until error token accepted
            while {[llength $state_stack] > 0 && \
                       ![info exists table($state:error)]} {
                set state_stack [lrange $state_stack 0 end-1]
                set value_stack [lrange $value_stack 0 \
                                       [expr {[llength $state_stack] - 1}]]
                set state [lindex $state_stack end]
            }
            if {[llength $state_stack] == 0} {
 
	        set rr { }
                if {[info exists lr1_table($save_state,trans)] && [llength $lr1_table($save_state,trans)] >= 1} {
                    foreach trans $lr1_table($save_state,trans) {
                        foreach {tok_id nextstate} $trans {
			    set ss $token_id_table($tok_id,title)
			    if {$ss != {}} {
			        append rr "$ss, "
                            }
                        }
                    }
                }
		set rr [string trimleft $rr { }]
		set rr [string trimright $rr {, }]
                yyerror "parse error, expecting: $rr"


                return 1
            }
            lappend state_stack [set state $table($state:error,target)]
            lappend value_stack {}
            # consume tokens until it finds an acceptable one
            while {![info exists table($state:$token)]} {
                if {$token == 0} {
                    yyerror "end of file while recovering from error"
                    return 1
                }
                set yylval {}
                set token [yylex]
                set buflval $yylval
            }
            continue
        }
        switch -- $table($state:$token) {
            shift {
                lappend state_stack $table($state:$token,target)
                lappend value_stack $buflval
                set token ""
            }
            reduce {
                set rule $table($state:$token,target)
                set ll $rules($rule,l)
                if {[info exists rules($rule,e)]} {
                    set dc $rules($rule,e)
                } else {
                    set dc $rules($rule,dc)
                }
                set stackpointer [expr {[llength $state_stack]-$dc}]
                setupvalues $value_stack $stackpointer $dc
                set _ $1
                set yylval [lindex $value_stack end]
                switch -- $rule {
                    1 { set _ image }
                    2 { set _ physical }
                    3 { set _ amplifier }
                    4 { set _ detector }
                    5 { set _ wcs }
                    6 { set _ wcsa }
                    7 { set _ wcsb }
                    8 { set _ wcsc }
                    9 { set _ wcsd }
                    10 { set _ wcse }
                    11 { set _ wcsf }
                    12 { set _ wcsg }
                    13 { set _ wcsh }
                    14 { set _ wcsi }
                    15 { set _ wcsj }
                    16 { set _ wcsk }
                    17 { set _ wcsl }
                    18 { set _ wcsm }
                    19 { set _ wcsn }
                    20 { set _ wcso }
                    21 { set _ wcsp }
                    22 { set _ wcsq }
                    23 { set _ wcsr }
                    24 { set _ wcss }
                    25 { set _ wcst }
                    26 { set _ wcsu }
                    27 { set _ wcsv }
                    28 { set _ wcsw }
                    29 { set _ wcsx }
                    30 { set _ wcsy }
                    31 { set _ wcsz }
                    32 { set _ $1 }
                    33 { set _ $1 }
                    35 { global ds9; if {!$ds9(init)} {YYERROR} else {yyclearin; YYACCEPT} }
                    37 { global parse; set parse(result) mask }
                    39 { Open $2 fits mask {} wcs }
                    40 { MaskDestroyDialog }
                    41 { MaskClear }
                    42 { ProcessCmdSet mask color $2 MaskColor }
                    43 { ProcessCmdSet mask mark $2 MaskMark }
                    44 { ProcessCmdSet2 mask low $2 high $3 MaskRange }
                    46 { ProcessCmdSet mask transparency $2 MaskTransparency }
                    47 { ProcessCmdSet mask mark $2 MaskMark }
                    48 { set _ zero }
                    49 { set _ nonzero }
                    50 { set _ nan }
                    51 { set _ nonnan }
                    52 { set _ range }
                    53 { ProcessCmdSet mask system $1 MaskSystem }
                    54 { ProcessCmdSet mask system $1 MaskSystem }
                }
                unsetupvalues $dc
                # pop off tokens from the stack if normal rule
                if {![info exists rules($rule,e)]} {
                    incr stackpointer -1
                    set state_stack [lrange $state_stack 0 $stackpointer]
                    set value_stack [lrange $value_stack 0 $stackpointer]
                }
                # now do the goto transition
                lappend state_stack $table([lindex $state_stack end]:$ll,target)
                lappend value_stack $_
            }
            accept {
                set accepted 1
            }
            goto -
            default {
                puts stderr "Internal parser error: illegal command $table($state:$token)"
                return 2
            }
        }
    }
    return 0
}

######
# end autogenerated taccle functions
######

proc mask::yyerror {msg} {
     variable yycnt
     variable yy_current_buffer
     variable index_

     ParserError $msg $yycnt $yy_current_buffer $index_
}