package provide DS9 1.0

######
# Begin autogenerated taccle (version 1.3) routines.
# Although taccle itself is protected by the GNU Public License (GPL)
# all user-supplied functions are protected by their respective
# author's license.  See http://mini.net/tcl/taccle for other details.
######

namespace eval gridsend {
    variable yylval {}
    variable table
    variable rules
    variable token {}
    variable yycnt 0
    variable yyerr 0
    variable save_state 0

    namespace export yylex
}

proc gridsend::YYABORT {} {
    return -code return 1
}

proc gridsend::YYACCEPT {} {
    return -code return 0
}

proc gridsend::YYERROR {} {
    variable yyerr
    set yyerr 1
}

proc gridsend::yyclearin {} {
    variable token
    variable yycnt
    set token {}
    incr yycnt -1
}

proc gridsend::yyerror {s} {
    puts stderr $s
}

proc gridsend::setupvalues {stack pointer numsyms} {
    upvar 1 1 y
    set y {}
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        set y [lindex $stack $pointer]
        incr pointer
    }
}

proc gridsend::unsetupvalues {numsyms} {
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        unset y
    }
}

array set gridsend::table {
  7:264,target 54
  27:0 reduce
  6:259,target 38
  6:260,target 39
  1:296 goto
  48:0 reduce
  6:257 shift
  6:258 shift
  1:279,target 18
  6:259 shift
  6:260 shift
  70:0 reduce
  69:0 reduce
  6:261 shift
  5:295,target 35
  0:275,target 5
  6:0,target 66
  11:282,target 63
  87:0,target 86
  2:0 reduce
  6:264 shift
  80:0,target 82
  79:0,target 84
  72:0,target 58
  64:0,target 36
  5:274,target 32
  6:267 shift
  56:0,target 52
  24:0 reduce
  6:268 shift
  48:0,target 14
  6:257,target 36
  41:0,target 78
  45:0 reduce
  33:0,target 23
  2:282,target 25
  6:272 shift
  25:0,target 43
  6:273 shift
  17:0,target 30
  66:0 reduce
  10:0,target 3
  12:284,target 74
  87:0 reduce
  7:259,target 51
  7:260,target 52
  21:0 reduce
  5:272,target 30
  1:296,target 22
  6:285 shift
  42:0 reduce
  6:286 shift
  7:299,target 60
  14:262 shift
  14:263 shift
  0:292,target 14
  63:0 reduce
  6:301 goto
  84:0 reduce
  5:291,target 34
  12:261,target 70
  17:0 reduce
  2:298,target 27
  7:257,target 49
  14:288,target 82
  38:0 reduce
  14:275 shift
  60:0 reduce
  59:0 reduce
  0:289,target 13
  3:0,target 8
  84:0,target 15
  76:0,target 17
  81:0 reduce
  68:0,target 63
  61:0,target 35
  79:277,target 86
  53:0,target 47
  45:0,target 72
  6:272,target 44
  37:0,target 74
  12:258,target 67
  30:0,target 24
  29:0,target 22
  22:0,target 7
  14:288 shift
  35:0 reduce
  14:290 shift
  14:302 goto
  56:0 reduce
  0:287,target 11
  6:301,target 48
  77:0 reduce
  11:264 shift
  7:274,target 57
  11:265 shift
  9:0 reduce
  0:262 shift
  11:0 reduce
  0:263 shift
  32:0 reduce
  5:265,target 29
  79:276 shift
  1:289,target 20
  79:277 shift
  53:0 reduce
  0:270 shift
  0:269 shift
  74:0 reduce
  14:263,target 80
  7:272,target 55
  0:275 shift
  6:0 reduce
  11:282 shift
  0:276 shift
  6:267,target 42
  79:287 shift
  0:278 shift
  28:0 reduce
  0:280 shift
  7:0,target 44
  88:0,target 80
  0:0,target 1
  0:281 shift
  81:0,target 79
  73:0,target 59
  50:0 reduce
  49:0 reduce
  0:283 shift
  79:303 goto
  65:0,target 10
  11:291 shift
  57:0,target 53
  71:0 reduce
  50:0,target 46
  49:0,target 45
  0:283,target 10
  42:0,target 69
  0:287 shift
  6:286,target 47
  34:0,target 21
  0:288 shift
  26:0,target 41
  0:289 shift
  3:0 reduce
  18:0,target 33
  5:282,target 33
  11:297 goto
  0:292 shift
  0:262,target 1
  11:0,target 34
  0:293 goto
  25:0 reduce
  46:0 reduce
  67:0 reduce
  0:281,target 9
  88:0 reduce
  1:264,target 16
  0:0 reduce
  12:271,target 73
  5:264 shift
  5:265 shift
  22:0 reduce
  13:294,target 78
  43:0 reduce
  5:272 shift
  5:273 shift
  12:300,target 75
  64:0 reduce
  5:274 shift
  79:287,target 87
  0:278,target 7
  85:0 reduce
  4:0,target 9
  85:0,target 87
  77:0,target 18
  18:0 reduce
  5:282 shift
  11:264,target 61
  70:0,target 62
  69:0,target 64
  6:261,target 40
  62:0,target 37
  54:0,target 50
  40:0 reduce
  39:0 reduce
  46:0,target 67
  38:0,target 76
  13:262 shift
  14:275,target 81
  61:0 reduce
  31:0,target 25
  23:0,target 40
  15:0,target 0
  2:264,target 23
  5:291 shift
  82:0 reduce
  0:276,target 6
  12:266,target 72
  5:295 goto
  15:0 accept
  2:264 shift
  2:265 shift
  6:258,target 37
  36:0 reduce
  57:0 reduce
  13:278 shift
  78:0 reduce
  12:264,target 71
  7:261,target 53
  12:0 reduce
  5:273,target 31
  14:302,target 84
  33:0 reduce
  2:282 shift
  54:0 reduce
  0:293,target 15
  8:0,target 4
  13:294 goto
  75:0 reduce
  1:0,target 27
  82:0,target 81
  74:0,target 57
  2:291 shift
  66:0,target 60
  7:0 reduce
  58:0,target 54
  51:0,target 48
  7:258,target 50
  43:0,target 70
  35:0,target 6
  30:0 reduce
  29:0 reduce
  14:290,target 83
  27:0,target 11
  20:0,target 32
  19:0,target 31
  2:298 goto
  7:257 shift
  51:0 reduce
  7:258 shift
  12:0,target 56
  7:259 shift
  7:260 shift
  11:297,target 65
  7:261 shift
  72:0 reduce
  7:264 shift
  0:270,target 4
  0:269,target 3
  4:0 reduce
  6:273,target 45
  12:259,target 68
  12:260,target 69
  26:0 reduce
  7:272 shift
  47:0 reduce
  7:273 shift
  7:274 shift
  0:288,target 12
  68:0 reduce
  1:0 reduce
  13:262,target 76
  79:276,target 85
  12:257,target 66
  23:0 reduce
  44:0 reduce
  1:291,target 21
  5:0,target 19
  86:0,target 85
  78:0,target 2
  7:289 shift
  7:290 shift
  71:0,target 65
  65:0 reduce
  63:0,target 38
  55:0,target 51
  86:0 reduce
  47:0,target 68
  7:273,target 56
  40:0,target 75
  39:0,target 77
  32:0,target 26
  24:0,target 42
  6:268,target 43
  20:0 reduce
  19:0 reduce
  16:0,target 28
  7:299 goto
  5:264,target 28
  41:0 reduce
  62:0 reduce
  13:278,target 77
  79:303,target 88
  11:291,target 64
  14:262,target 79
  83:0 reduce
  0:263,target 2
  16:0 reduce
  2:291,target 26
  12:257 shift
  37:0 reduce
  12:258 shift
  12:259 shift
  12:260 shift
  12:261 shift
  58:0 reduce
  7:289,target 58
  7:290,target 59
  12:264 shift
  80:0 reduce
  79:0 reduce
  6:285,target 46
  1:265,target 17
  12:266 shift
  9:0,target 5
  13:0 reduce
  1:264 shift
  2:0,target 39
  6:264,target 41
  12:271 shift
  83:0,target 83
  1:265 shift
  75:0,target 13
  34:0 reduce
  67:0,target 61
  60:0,target 12
  59:0,target 55
  55:0 reduce
  52:0,target 49
  44:0,target 71
  36:0,target 73
  76:0 reduce
  28:0,target 20
  0:280,target 8
  21:0,target 29
  13:0,target 16
  8:0 reduce
  10:0 reduce
  12:284 shift
  11:265,target 62
  1:279 shift
  31:0 reduce
  1:282 shift
  12:300 goto
  52:0 reduce
  1:282,target 19
  73:0 reduce
  2:265,target 24
  1:289 shift
  1:291 shift
  5:0 reduce
}

array set gridsend::rules {
  9,l 293
  11,l 293
  32,l 296
  53,l 299
  74,l 301
  6,l 293
  28,l 296
  50,l 299
  49,l 299
  71,l 301
  3,l 293
  25,l 295
  46,l 299
  67,l 301
  0,l 304
  22,l 295
  43,l 298
  64,l 300
  85,l 303
  18,l 294
  40,l 298
  39,l 298
  61,l 300
  82,l 302
  15,l 293
  36,l 297
  57,l 300
  78,l 301
  12,l 293
  33,l 296
  54,l 299
  75,l 301
  7,l 293
  29,l 296
  30,l 296
  51,l 299
  72,l 301
  4,l 293
  26,l 295
  47,l 299
  68,l 301
  1,l 293
  23,l 295
  44,l 299
  65,l 300
  86,l 303
  19,l 295
  20,l 295
  41,l 298
  62,l 300
  83,l 302
  16,l 294
  37,l 297
  58,l 300
  80,l 302
  79,l 302
  13,l 293
  34,l 297
  55,l 299
  76,l 301
  8,l 293
  10,l 293
  31,l 296
  52,l 299
  73,l 301
  5,l 293
  27,l 296
  48,l 299
  70,l 301
  69,l 301
  2,l 293
  24,l 295
  45,l 299
  66,l 301
  87,l 303
  21,l 295
  42,l 298
  63,l 300
  84,l 303
  17,l 294
  38,l 297
  60,l 300
  59,l 300
  81,l 302
  14,l 293
  35,l 297
  56,l 300
  77,l 301
}

array set gridsend::rules {
  63,dc 1
  12,dc 2
  77,dc 1
  26,dc 1
  3,dc 1
  41,dc 1
  55,dc 1
  70,dc 1
  69,dc 1
  18,dc 1
  84,dc 0
  33,dc 1
  9,dc 1
  47,dc 1
  62,dc 1
  11,dc 2
  76,dc 1
  25,dc 1
  2,dc 2
  40,dc 1
  39,dc 0
  54,dc 1
  68,dc 1
  17,dc 1
  83,dc 1
  32,dc 1
  8,dc 1
  46,dc 1
  61,dc 1
  10,dc 2
  75,dc 1
  24,dc 1
  1,dc 0
  38,dc 1
  53,dc 1
  67,dc 1
  16,dc 0
  82,dc 1
  31,dc 1
  7,dc 2
  45,dc 1
  60,dc 1
  59,dc 1
  74,dc 1
  23,dc 1
  0,dc 1
  37,dc 1
  52,dc 1
  66,dc 0
  15,dc 2
  81,dc 1
  29,dc 1
  30,dc 1
  6,dc 2
  44,dc 0
  58,dc 1
  73,dc 1
  22,dc 1
  87,dc 1
  36,dc 1
  51,dc 1
  65,dc 1
  14,dc 2
  80,dc 2
  79,dc 1
  28,dc 1
  5,dc 1
  43,dc 1
  57,dc 1
  72,dc 1
  21,dc 1
  86,dc 1
  35,dc 1
  50,dc 1
  49,dc 1
  64,dc 1
  13,dc 2
  78,dc 1
  27,dc 0
  4,dc 1
  42,dc 1
  56,dc 0
  71,dc 1
  19,dc 0
  20,dc 1
  85,dc 1
  34,dc 0
  48,dc 1
}

array set gridsend::rules {
  41,line 105
  7,line 56
  37,line 99
  4,line 53
  34,line 95
  1,line 50
  31,line 90
  86,line 164
  27,line 85
  83,line 159
  24,line 80
  80,line 156
  79,line 155
  21,line 77
  76,line 150
  17,line 71
  73,line 146
  14,line 64
  70,line 143
  69,line 142
  11,line 60
  66,line 139
  63,line 134
  60,line 130
  59,line 129
  56,line 126
  53,line 121
  50,line 118
  49,line 117
  46,line 113
  43,line 108
  9,line 58
  40,line 104
  39,line 103
  6,line 55
  36,line 97
  3,line 52
  33,line 92
  29,line 87
  30,line 89
  85,line 163
  26,line 82
  82,line 158
  23,line 79
  78,line 152
  19,line 75
  20,line 76
  75,line 148
  16,line 69
  72,line 145
  13,line 62
  68,line 141
  10,line 59
  65,line 136
  62,line 132
  58,line 128
  55,line 123
  52,line 120
  48,line 116
  45,line 112
  42,line 107
  8,line 57
  38,line 100
  5,line 54
  35,line 96
  2,line 51
  32,line 91
  87,line 165
  28,line 86
  84,line 162
  25,line 81
  81,line 157
  22,line 78
  77,line 151
  18,line 72
  74,line 147
  15,line 65
  71,line 144
  12,line 61
  67,line 140
  64,line 135
  61,line 131
  57,line 127
  54,line 122
  51,line 119
  47,line 114
  44,line 111
}

array set gridsend::lr1_table {
  66,trans {}
  35 {{6 0 2}}
  85,trans {}
  14,trans {{262 79} {263 80} {275 81} {288 82} {290 83} {302 84}}
  36 {{73 0 1}}
  33,trans {}
  37 {{74 0 1}}
  52,trans {}
  38 {{76 0 1}}
  71,trans {}
  39 {{77 0 1}}
  40 {{75 0 1}}
  18,trans {}
  1,trans {{264 16} {265 17} {279 18} {282 19} {289 20} {291 21} {296 22}}
  41 {{78 0 1}}
  37,trans {}
  42 {{69 0 1}}
  56,trans {}
  43 {{70 0 1}}
  75,trans {}
  44 {{71 0 1}}
  23,trans {}
  45 {{72 0 1}}
  5,trans {{264 28} {265 29} {272 30} {273 31} {274 32} {282 33} {291 34} {295 35}}
  42,trans {}
  46 {{67 0 1}}
  61,trans {}
  47 {{68 0 1}}
  80,trans {}
  79,trans {{276 85} {277 86} {287 87} {303 88}}
  48 {{14 0 2}}
  27,trans {}
  9,trans {}
  50 {{46 0 1}}
  49 {{45 0 1}}
  46,trans {}
  51 {{48 0 1}}
  65,trans {}
  52 {{49 0 1}}
  84,trans {}
  13,trans {{262 76} {278 77} {294 78}}
  53 {{47 0 1}}
  32,trans {}
  54 {{50 0 1}}
  51,trans {}
  55 {{51 0 1}}
  70,trans {}
  69,trans {}
  56 {{52 0 1}}
  88,trans {}
  17,trans {}
  57 {{53 0 1}}
  0,trans {{262 1} {263 2} {269 3} {270 4} {275 5} {276 6} {278 7} {280 8} {281 9} {283 10} {287 11} {288 12} {289 13} {292 14} {293 15}}
  36,trans {}
  58 {{54 0 1}}
  55,trans {}
  60 {{12 0 2}}
  59 {{55 0 1}}
  74,trans {}
  61 {{35 0 1}}
  22,trans {}
  62 {{37 0 1}}
  4,trans {}
  41,trans {}
  63 {{38 0 1}}
  60,trans {}
  59,trans {}
  64 {{36 0 1}}
  78,trans {}
  65 {{10 0 2}}
  26,trans {}
  66 {{60 0 1}}
  8,trans {}
  45,trans {}
  67 {{61 0 1}}
  64,trans {}
  68 {{63 0 1}}
  83,trans {}
  12,trans {{257 66} {258 67} {259 68} {260 69} {261 70} {264 71} {266 72} {271 73} {284 74} {300 75}}
  70 {{62 0 1}}
  69 {{64 0 1}}
  31,trans {}
  71 {{65 0 1}}
  50,trans {}
  49,trans {}
  72 {{58 0 1}}
  68,trans {}
  73 {{59 0 1}}
  87,trans {}
  16,trans {}
  74 {{57 0 1}}
  35,trans {}
  75 {{13 0 2}}
  54,trans {}
  76 {{17 0 1}}
  73,trans {}
  77 {{18 0 1}}
  21,trans {}
  78 {{2 0 2}}
  3,trans {}
  40,trans {}
  39,trans {}
  80 {{82 0 1}}
  79 {{80 0 1} {84 0 0} {85 0 0} {86 0 0} {87 0 0}}
  58,trans {}
  81 {{79 0 1}}
  10 {{3 0 1}}
  77,trans {}
  82 {{81 0 1}}
  11 {{10 0 1} {34 0 0} {35 0 0} {36 0 0} {37 0 0} {38 0 0}}
  25,trans {}
  83 {{83 0 1}}
  7,trans {{257 49} {258 50} {259 51} {260 52} {261 53} {264 54} {272 55} {273 56} {274 57} {289 58} {290 59} {299 60}}
  12 {{13 0 1} {56 0 0} {57 0 0} {58 0 0} {59 0 0} {60 0 0} {61 0 0} {62 0 0} {63 0 0} {64 0 0} {65 0 0}}
  44,trans {}
  84 {{15 0 2}}
  13 {{2 0 1} {16 0 0} {17 0 0} {18 0 0}}
  85 {{87 0 1}}
  63,trans {}
  14 {{15 0 1} {79 0 0} {80 0 0} {81 0 0} {82 0 0} {83 0 0}}
  82,trans {}
  86 {{85 0 1}}
  11,trans {{264 61} {265 62} {282 63} {291 64} {297 65}}
  15 {{0 0 1}}
  87 {{86 0 1}}
  30,trans {}
  29,trans {}
  16 {{28 0 1}}
  88 {{80 0 2}}
  48,trans {}
  0 {{0 0 0} {1 0 0} {2 0 0} {3 0 0} {4 0 0} {5 0 0} {6 0 0} {7 0 0} {8 0 0} {9 0 0} {10 0 0} {11 0 0} {12 0 0} {13 0 0} {14 0 0} {15 0 0}}
  17 {{30 0 1}}
  67,trans {}
  1 {{7 0 1} {27 0 0} {28 0 0} {29 0 0} {30 0 0} {31 0 0} {32 0 0} {33 0 0}}
  18 {{33 0 1}}
  86,trans {}
  15,trans {}
  2 {{11 0 1} {39 0 0} {40 0 0} {41 0 0} {42 0 0} {43 0 0}}
  19 {{31 0 1}}
  20 {{32 0 1}}
  34,trans {}
  3 {{8 0 1}}
  21 {{29 0 1}}
  53,trans {}
  4 {{9 0 1}}
  22 {{7 0 2}}
  72,trans {}
  5 {{6 0 1} {19 0 0} {20 0 0} {21 0 0} {22 0 0} {23 0 0} {24 0 0} {25 0 0} {26 0 0}}
  23 {{40 0 1}}
  20,trans {}
  19,trans {}
  6 {{14 0 1} {66 0 0} {67 0 0} {68 0 0} {69 0 0} {70 0 0} {71 0 0} {72 0 0} {73 0 0} {74 0 0} {75 0 0} {76 0 0} {77 0 0} {78 0 0}}
  24 {{42 0 1}}
  2,trans {{264 23} {265 24} {282 25} {291 26} {298 27}}
  38,trans {}
  7 {{12 0 1} {44 0 0} {45 0 0} {46 0 0} {47 0 0} {48 0 0} {49 0 0} {50 0 0} {51 0 0} {52 0 0} {53 0 0} {54 0 0} {55 0 0}}
  25 {{43 0 1}}
  57,trans {}
  8 {{4 0 1}}
  26 {{41 0 1}}
  76,trans {}
  9 {{5 0 1}}
  27 {{11 0 2}}
  24,trans {}
  6,trans {{257 36} {258 37} {259 38} {260 39} {261 40} {264 41} {267 42} {268 43} {272 44} {273 45} {285 46} {286 47} {301 48}}
  28 {{20 0 1}}
  43,trans {}
  29 {{22 0 1}}
  30 {{24 0 1}}
  62,trans {}
  31 {{25 0 1}}
  81,trans {}
  10,trans {}
  32 {{26 0 1}}
  28,trans {}
  33 {{23 0 1}}
  47,trans {}
  34 {{21 0 1}}
}

array set gridsend::token_id_table {
  286 TEXT2_
  286,t 0
  287 TICKMARKS_
  292,line 45
  302,line 154
  288 TITLE_
  265,title DASH
  289 TYPE_
  290 VERTICAL_
  300 title
  284,title TEXT
  291 WIDTH_
  301 labels
  292 VIEW_
  302 view
  288,line 41
  293 gridsend
  303 viewaxes
  304 start'
  294 type
  295 grid
  296 axes
  262,t 0
  297 tickmarks
  285,line 38
  298 border
  299 numerics
  283,t 0
  282,line 35
  264,title COLOR
  283,title SYSTEM
  278,line 31
  error,line 48
  258,t 0
  275,line 28
  279,t 0
  280,t 0
  272,line 25
  263,title BORDER
  282,title STYLE
  268,line 21
  276,t 0
  265,line 18
  297,t 1
  262,line 15
  262,title AXES
  0 {$}
  0,t 0
  281,title SKYFORMAT
  error,t 0
  299,title {}
  258,line 8
  273,t 0
  304,t 1
  294,t 1
  261,title FONTWEIGHT
  279,title ORIGIN
  280,title SKYFRAME
  269,t 0
  270,t 0
  298,title {}
  291,t 0
  301,t 1
  266,t 0
  260,title FONTSTYLE
  259,title FONTSLANT
  297,line 94
  278,title NUMERICS
  297,title {}
  287,t 0
  304,line 166
  294,line 67
  291,line 44
  error,title {}
  301,line 138
  263,t 0
  258,title FONTSIZE
  287,line 40
  277,title NUMBERS
  284,t 0
  296,title {}
  284,line 37
  281,line 34
  260,t 0
  259,t 0
  281,t 0
  257,title FONT
  277,line 30
  276,title LABELS
  295,title {}
  274,line 27
  271,line 24
  277,t 0
  267,line 20
  298,t 1
  275,title GRID
  304,title {}
  294,title {}
  264,line 17
  261,line 11
  274,t 0
  295,t 1
  257,line 7
  274,title GAP3
  293,title {}
  303,title {}
  271,t 0
  error error
  292,t 0
  302,t 1
  273,title GAP2
  292,title VIEW
  302,title {}
  267,t 0
  299,line 110
  288,t 0
  296,line 84
  272,title GAP1
  303,line 161
  291,title WIDTH
  293,line 49
  301,title {}
  264,t 0
  285,t 0
  289,line 42
  290,line 43
  300,line 125
  286,line 39
  271,title GAP
  261,t 0
  283,line 36
  289,title TYPE
  290,title VERTICAL
  300,title {}
  282,t 0
  279,line 32
  280,line 33
  276,line 29
  257,t 0
  269,title FORMAT1
  270,title FORMAT2
  273,line 26
  288,title TITLE
  278,t 0
  299,t 1
  269,line 22
  270,line 23
  266,line 19
  268,title DEF2
  275,t 0
  263,line 16
  287,title TICKMARKS
  296,t 1
  260,line 10
  259,line 9
  272,t 0
  267,title DEF1
  303,t 1
  257 FONT_
  286,title TEXT2
  293,t 1
  258 FONTSIZE_
  260 FONTSTYLE_
  259 FONTSLANT_
  261 FONTWEIGHT_
  262 AXES_
  263 BORDER_
  264 COLOR_
  265 DASH_
  266 DEF_
  267 DEF1_
  268,t 0
  268 DEF2_
  269 FORMAT1_
  270 FORMAT2_
  271 GAP_
  272 GAP1_
  289,t 0
  290,t 0
  300,t 1
  266,title DEF
  273 GAP2_
  274 GAP3_
  285,title TEXT1
  275 GRID_
  276 LABELS_
  298,line 102
  277 NUMBERS_
  278 NUMERICS_
  279 ORIGIN_
  280 SKYFRAME_
  281 SKYFORMAT_
  282 STYLE_
  295,line 74
  265,t 0
  283 SYSTEM_
  284 TEXT_
  285 TEXT1_
}

proc gridsend::yyparse {} {
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
                    1 { ProcessSendCmdYesNo grid view }
                    3 { ProcessSendCmdGet grid system }
                    4 { ProcessSendCmdGet grid sky }
                    5 { ProcessSendCmdGet grid skyformat }
                    8 { ProcessSendCmdGet grid format1 }
                    9 { ProcessSendCmdGet grid format2 }
                    16 { ProcessSendCmdGet grid type }
                    17 { ProcessSendCmdGet grid axis,type }
                    18 { ProcessSendCmdGet grid numlab,type }
                    19 { ProcessSendCmdYesNo grid grid }
                    20 { ProcessSendCmdGet grid grid,color }
                    21 { ProcessSendCmdGet grid grid,width }
                    22 { ProcessSendCmdYesNo grid grid,style }
                    23 { ProcessSendCmdGet grid grid,style }
                    24 { ProcessSendCmdGet grid grid,gap1 }
                    25 { ProcessSendCmdGet grid grid,gap2 }
                    26 { ProcessSendCmdGet grid grid,gap3 }
                    27 { ProcessSendCmdYesNo grid axes }
                    28 { ProcessSendCmdGet grid axes,color }
                    29 { ProcessSendCmdGet grid axes,width }
                    30 { ProcessSendCmdYesNo grid axes,style }
                    31 { ProcessSendCmdGet grid axes,style }
                    32 { ProcessSendCmdGet grid axes,type }
                    33 { ProcessSendCmdGet grid axes,origin }
                    34 { ProcessSendCmdYesNo grid tick }
                    35 { ProcessSendCmdGet grid tick,color }
                    36 { ProcessSendCmdGet grid tick,width }
                    37 { ProcessSendCmdYesNo grid tick,style }
                    38 { ProcessSendCmdGet grid tick,style }
                    39 { ProcessSendCmdYesNo grid border }
                    40 { ProcessSendCmdGet grid border,color }
                    41 { ProcessSendCmdGet grid border,width }
                    42 { ProcessSendCmdYesNo grid border,style }
                    43 { ProcessSendCmdGet grid border,style }
                    44 { ProcessSendCmdYesNo grid numlab }
                    45 { ProcessSendCmdGet grid numlab,font }
                    46 { ProcessSendCmdGet grid numlab,size }
                    47 { ProcessSendCmdGet grid numlab,weight }
                    48 { ProcessSendCmdGet grid numlab,slant }
                    49 { ProcessSendCmdGet grid numlab,weight }
                    50 { ProcessSendCmdGet grid numlab,color }
                    51 { ProcessSendCmdGet grid numlab,gap1 }
                    52 { ProcessSendCmdGet grid numlab,gap2 }
                    53 { ProcessSendCmdGet grid numlab,gap3 }
                    54 { ProcessSendCmdGet grid numlab,type }
                    55 { ProcessSendCmdGet grid numlab,vertical }
                    56 { ProcessSendCmdYesNo grid title }
                    57 { ProcessSendCmdGet grid title,text }
                    58 { ProcessSendCmdYesNo grid title,def }
                    59 { ProcessSendCmdGet grid title,gap }
                    60 { ProcessSendCmdGet grid title,font }
                    61 { ProcessSendCmdGet grid title,size }
                    62 { ProcessSendCmdGet grid title,weight }
                    63 { ProcessSendCmdGet grid title,slant }
                    64 { ProcessSendCmdGet grid title,weight }
                    65 { ProcessSendCmdGet grid title,color }
                    66 { ProcessSendCmdYesNo grid textlab }
                    67 { ProcessSendCmdGet grid textlab,text1 }
                    68 { ProcessSendCmdGet grid textlab,text2 }
                    69 { ProcessSendCmdYesNo grid textlab,def1 }
                    70 { ProcessSendCmdYesNo grid textlab,def2 }
                    71 { ProcessSendCmdGet grid textlab,gap1 }
                    72 { ProcessSendCmdGet grid textlab,gap2 }
                    73 { ProcessSendCmdGet grid textlab,font }
                    74 { ProcessSendCmdGet grid textlab,size }
                    75 { ProcessSendCmdGet grid textlab,weight }
                    76 { ProcessSendCmdGet grid textlab,slant }
                    77 { ProcessSendCmdGet grid textlab,weight }
                    78 { ProcessSendCmdGet grid textlab,color }
                    79 { ProcessSendCmdYesNo grid grid }
                    81 { ProcessSendCmdYesNo grid title }
                    82 { ProcessSendCmdYesNo grid border }
                    83 { ProcessSendCmdYesNo grid numlab,vertical }
                    84 { ProcessSendCmdYesNo grid axes }
                    85 { ProcessSendCmdYesNo grid numlab }
                    86 { ProcessSendCmdYesNo grid tick }
                    87 { ProcessSendCmdYesNo grid textlab }
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

proc gridsend::yyerror {msg} {
     variable yycnt
     variable yy_current_buffer
     variable index_

     ParserError $msg $yycnt $yy_current_buffer $index_
}