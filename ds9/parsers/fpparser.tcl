package provide DS9 1.0

######
# Begin autogenerated taccle (version 1.3) routines.
# Although taccle itself is protected by the GNU Public License (GPL)
# all user-supplied functions are protected by their respective
# author's license.  See http://mini.net/tcl/taccle for other details.
######

namespace eval fp {
    variable yylval {}
    variable table
    variable rules
    variable token {}
    variable yycnt 0
    variable yyerr 0
    variable save_state 0

    namespace export yylex
}

proc fp::YYABORT {} {
    return -code return 1
}

proc fp::YYACCEPT {} {
    return -code return 0
}

proc fp::YYERROR {} {
    variable yyerr
    set yyerr 1
}

proc fp::yyclearin {} {
    variable token
    variable yycnt
    set token {}
    incr yycnt -1
}

proc fp::yyerror {s} {
    puts stderr $s
}

proc fp::setupvalues {stack pointer numsyms} {
    upvar 1 1 y
    set y {}
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        set y [lindex $stack $pointer]
        incr pointer
    }
}

proc fp::unsetupvalues {numsyms} {
    for {set i 1} {$i <= $numsyms} {incr i} {
        upvar 1 $i y
        unset y
    }
}

array set fp::table {
  89:292 shift
  24:332 goto
  1:302 reduce
  89:293 shift
  0:327,target 2
  1:303 reduce
  1:304 reduce
  51:298 reduce
  27:0 reduce
  12:317,target 32
  1:305 reduce
  1:311,target 71
  66:298,target 14
  1:306 reduce
  1:307 reduce
  90:298 reduce
  48:0 reduce
  20:294,target 51
  0:306,target 45
  1:298 reduce
  1:308 reduce
  12:316 shift
  1:299 reduce
  1:309 reduce
  1:310 reduce
  21:277,target 73
  12:317 shift
  1:311 reduce
  70:0 reduce
  69:0 reduce
  12:318 shift
  1:312 reduce
  55:298,target 3
  12:320 shift
  12:319 shift
  1:313 reduce
  5:305,target 48
  60:298 reduce
  59:298 reduce
  12:321 shift
  1:314 reduce
  91:0 reduce
  87:0,target 39
  2:0 accept
  80:0,target 64
  79:0,target 27
  29:295 shift
  4:301,target 9
  72:0,target 20
  29:296 shift
  9:298 reduce
  64:0,target 12
  19:323,target 49
  56:0,target 4
  44:298,target 30
  30:298 reduce
  90:298,target 69
  48:0,target 34
  1:298,target 71
  1:308,target 71
  68:298 reduce
  45:0 reduce
  25:0,target 44
  21:322 goto
  0:304,target 45
  89:325 goto
  26:295,target 40
  66:0 reduce
  33:298,target 74
  21:275,target 71
  15:326,target 39
  78:298,target 26
  10:296,target 27
  38:298 reduce
  5:303,target 48
  12:335 goto
  87:0 reduce
  77:298 reduce
  15:295,target 26
  22:298,target 65
  67:298,target 15
  1:306,target 71
  47:298 reduce
  42:0 reduce
  29:326 goto
  26:284 reduce
  82:288,target 46
  26:285 reduce
  26:286 reduce
  19:290,target 48
  19:289,target 47
  0:302,target 45
  11:298,target 54
  86:298 reduce
  63:0 reduce
  56:298,target 4
  26:293,target 40
  26:287 reduce
  26:288 reduce
  21:273,target 69
  26:290 reduce
  26:289 reduce
  17:298 shift
  84:0 reduce
  26:291 reduce
  5:301,target 48
  26:292 reduce
  89:325,target 92
  56:298 reduce
  26:293 reduce
  45:298,target 31
  91:298,target 67
  26:295 reduce
  26:296 reduce
  6:298 shift
  38:0 reduce
  26:298 reduce
  1:304,target 71
  27:295,target 41
  82:286,target 44
  34:298,target 75
  92:0,target 61
  80:298,target 64
  79:298,target 27
  65:298 reduce
  60:0 reduce
  59:0 reduce
  19:287,target 45
  0:300,target 45
  3:0,target 42
  84:0,target 55
  26:291,target 40
  76:0,target 24
  24:313,target 21
  21:271,target 67
  83:284 shift
  81:0 reduce
  68:0,target 16
  83:285 shift
  61:0,target 9
  83:286 shift
  53:0,target 1
  35:298 reduce
  23:298,target 46
  83:287 shift
  68:298,target 16
  45:0,target 31
  83:288 shift
  14:0 reduce
  83:290 shift
  83:289 shift
  74:298 reduce
  30:0,target 53
  28:297,target 82
  22:0,target 65
  83:288,target 46
  14:0,target 58
  89:292,target 86
  24:332,target 81
  1:302,target 71
  57:298,target 5
  27:293,target 41
  82:284,target 42
  56:0 reduce
  44:298 reduce
  19:285,target 43
  26:288,target 40
  83:298 reduce
  77:0 reduce
  24:311,target 19
  21:268,target 64
  4:313,target 21
  46:298,target 32
  14:298 reduce
  92:298,target 61
  3:298,target 43
  9:0 reduce
  11:0 reduce
  53:298 reduce
  92:298 reduce
  83:286,target 44
  35:298,target 76
  3:298 reduce
  81:298,target 49
  1:300,target 71
  4:332,target 23
  53:0 reduce
  27:291,target 41
  23:298 reduce
  83:323 goto
  74:0 reduce
  62:298 reduce
  26:286,target 40
  24:308,target 16
  21:266,target 62
  4:311,target 19
  70:298,target 18
  69:298,target 17
  39:293,target 87
  32:298 reduce
  13:298,target 38
  7:0,target 50
  88:0,target 60
  71:298 reduce
  58:298,target 6
  41:326,target 89
  0:0,target 70
  0:314,target 45
  83:284,target 42
  81:0,target 49
  73:0,target 21
  50:0 reduce
  49:0 reduce
  41:295 shift
  65:0,target 13
  41:296 shift
  27:288,target 41
  57:0,target 5
  5:313,target 48
  71:0 reduce
  50:0,target 35
  49:0,target 62
  41:295,target 26
  19:284 shift
  3:328 goto
  47:298,target 33
  42:0,target 28
  29:326,target 83
  26:284,target 40
  19:285 shift
  24:306,target 14
  21:264,target 60
  19:286 shift
  4:308,target 16
  92:0 reduce
  80:298 reduce
  79:298 reduce
  39:291,target 85
  26:0,target 40
  19:287 shift
  0:300 reduce
  3:0 reduce
  19:288 shift
  0:301 reduce
  19:290 shift
  19:289 shift
  0:302 reduce
  11:0,target 54
  11:298 reduce
  29:295,target 26
  20:291 shift
  0:303 reduce
  36:298,target 77
  25:0 reduce
  18:326,target 41
  0:304 reduce
  82:298,target 68
  50:298 reduce
  49:298 reduce
  0:305 reduce
  20:294 shift
  0:312,target 45
  0:306 reduce
  46:0 reduce
  0:307 reduce
  88:298 reduce
  21:283,target 79
  0:298 reduce
  0:308 reduce
  27:286,target 41
  18:295,target 26
  0:310 reduce
  0:309 reduce
  0:299 reduce
  67:0 reduce
  0:311 reduce
  5:311,target 48
  71:298,target 19
  0:312 reduce
  0:313 reduce
  88:0 reduce
  58:298 reduce
  41:326 goto
  24:304,target 12
  21:262,target 58
  0:0 reduce
  0:314 reduce
  4:306,target 14
  0:315 shift
  14:298,target 58
  8:298 reduce
  60:298,target 8
  59:298,target 7
  28:297 shift
  22:0 reduce
  12:321,target 36
  1:314,target 71
  10:333,target 30
  43:0 reduce
  0:310,target 45
  0:309,target 45
  0:299,target 45
  67:298 reduce
  21:281,target 77
  64:0 reduce
  48:298,target 34
  27:284,target 41
  19:323 goto
  20:324 goto
  5:298,target 47
  5:308,target 48
  0:327 goto
  85:0 reduce
  37:298 shift
  24:302,target 10
  21:260,target 56
  21:259,target 55
  0:329 goto
  0:330 goto
  4:304,target 12
  85:0,target 37
  76:298 reduce
  37:298,target 84
  83:298,target 66
  77:0,target 25
  70:0,target 18
  69:0,target 17
  12:318,target 33
  0:334 goto
  1:312,target 71
  62:0,target 10
  54:0,target 2
  40:0 reduce
  46:298 reduce
  46:0,target 32
  0:307,target 45
  38:0,target 57
  26:298,target 40
  72:298,target 20
  61:0 reduce
  21:278,target 74
  85:298 reduce
  23:0,target 46
  5:300 reduce
  5:301 reduce
  5:306,target 48
  82:0 reduce
  16:298 reduce
  5:302 reduce
  5:303 reduce
  24:300,target 8
  21:257,target 53
  4:302,target 10
  5:304 reduce
  82:323,target 90
  61:298,target 9
  55:298 reduce
  5:305 reduce
  20:324,target 52
  5:306 reduce
  5:307 reduce
  12:316,target 31
  1:299,target 71
  1:309,target 71
  1:310,target 71
  5:298 reduce
  5:308 reduce
  5:299 reduce
  5:309 reduce
  5:310 reduce
  5:311 reduce
  50:298,target 35
  49:298,target 62
  0:305,target 45
  5:312 reduce
  57:0 reduce
  26:296,target 40
  5:313 reduce
  6:298,target 25
  64:298 reduce
  21:276,target 72
  5:314 reduce
  10:297,target 28
  78:0 reduce
  5:304,target 48
  82:284 shift
  82:285 shift
  38:298,target 57
  12:335,target 37
  84:298,target 55
  82:286 shift
  34:298 reduce
  15:296,target 27
  4:300,target 8
  82:287 shift
  82:288 shift
  82:290 shift
  82:289 shift
  73:298 reduce
  1:307,target 71
  27:298,target 41
  10:326,target 29
  82:290,target 48
  82:289,target 47
  73:298,target 21
  54:0 reduce
  20:291,target 50
  0:303,target 45
  43:298 reduce
  21:274,target 70
  8:0,target 52
  90:0,target 69
  75:0 reduce
  1:0,target 71
  5:331 goto
  10:295,target 26
  82:298 reduce
  82:0,target 68
  16:298,target 59
  5:302,target 48
  83:323,target 91
  74:0,target 22
  62:298,target 10
  66:0,target 14
  13:298 shift
  7:0 reduce
  58:0,target 6
  51:0,target 36
  52:298 reduce
  43:0,target 29
  30:0 reduce
  51:298,target 36
  27:0,target 41
  1:305,target 71
  91:298 reduce
  27:296,target 41
  7:298,target 50
  82:287,target 45
  51:0 reduce
  22:298 reduce
  19:288,target 46
  0:301,target 45
  26:292,target 40
  72:0 reduce
  24:314,target 22
  21:272,target 68
  82:323 goto
  61:298 reduce
  40:298,target 56
  85:298,target 37
  5:300,target 48
  21:322,target 80
  31:298 reduce
  39:291 shift
  26:0 reduce
  83:290,target 48
  83:289,target 47
  74:298,target 22
  39:292 shift
  89:293,target 87
  70:298 reduce
  69:298 reduce
  39:293 shift
  1:303,target 71
  47:0 reduce
  82:285,target 43
  19:286,target 44
  68:0 reduce
  26:290,target 40
  26:289,target 40
  17:298,target 40
  63:298,target 11
  40:298 reduce
  24:312,target 20
  21:270,target 66
  21:269,target 65
  4:314,target 22
  90:0 reduce
  1:0 reduce
  10:295 shift
  78:298 reduce
  10:296 shift
  10:297 shift
  52:298,target 63
  23:0 reduce
  8:298,target 52
  83:287,target 45
  48:298 reduce
  27:284 reduce
  89:291,target 85
  44:0 reduce
  27:285 reduce
  1:301,target 71
  5:0,target 47
  86:0,target 38
  39:325,target 88
  27:292,target 41
  27:286 reduce
  18:295 shift
  87:298 reduce
  78:0,target 26
  27:287 reduce
  18:296 shift
  71:0,target 19
  65:0 reduce
  27:288 reduce
  19:284,target 42
  3:328,target 6
  86:298,target 38
  63:0,target 11
  27:290 reduce
  27:289 reduce
  26:287,target 40
  55:0,target 3
  27:291 reduce
  24:310,target 18
  24:309,target 17
  24:299,target 7
  21:267,target 63
  4:312,target 20
  86:0 reduce
  47:0,target 33
  39:325 goto
  27:292 reduce
  57:298 reduce
  40:0,target 56
  27:293 reduce
  30:298,target 53
  27:295 reduce
  75:298,target 23
  27:296 reduce
  16:0,target 59
  7:298 reduce
  27:298 reduce
  0:315,target 1
  10:326 goto
  83:285,target 43
  66:298 reduce
  27:290,target 41
  27:289,target 41
  64:298,target 12
  62:0 reduce
  5:314,target 48
  41:296,target 27
  26:285,target 40
  10:333 goto
  83:0 reduce
  24:307,target 15
  21:265,target 61
  4:299,target 7
  4:309,target 17
  4:310,target 18
  39:292,target 86
  36:298 reduce
  18:326 goto
  0:334,target 5
  53:298,target 1
  75:298 reduce
  29:296,target 27
  16:0 reduce
  9:298,target 51
  0:313,target 45
  45:298 reduce
  42:298,target 28
  87:298,target 39
  58:0 reduce
  27:287,target 41
  18:296,target 27
  15:295 shift
  5:312,target 48
  84:298 reduce
  15:296 shift
  4:300 shift
  80:0 reduce
  79:0 reduce
  4:301 shift
  24:305,target 13
  24:300 shift
  21:263,target 59
  21:257 shift
  4:302 shift
  4:307,target 15
  31:298,target 72
  24:301 shift
  21:258 shift
  4:303 shift
  76:298,target 24
  24:302 shift
  21:260 shift
  21:259 shift
  4:304 shift
  54:298 reduce
  24:303 shift
  21:261 shift
  4:305 shift
  9:0,target 51
  91:0,target 67
  24:304 shift
  21:262 shift
  2:0,target 0
  4:306 shift
  83:0,target 66
  24:305 shift
  21:263 shift
  4:307 shift
  75:0,target 23
  24:306 shift
  21:264 shift
  4:308 shift
  5:331,target 24
  67:0,target 15
  24:307 shift
  21:265 shift
  0:311,target 45
  4:299 shift
  4:309 shift
  4:310 shift
  65:298,target 13
  60:0,target 8
  59:0,target 7
  24:308 shift
  21:266 shift
  4:311 shift
  55:0 reduce
  52:0,target 63
  24:310 shift
  24:309 shift
  24:299 shift
  21:282,target 78
  21:267 shift
  4:312 shift
  44:0,target 30
  27:285,target 41
  24:311 shift
  21:268 shift
  4:313 shift
  63:298 reduce
  24:312 shift
  21:270 shift
  21:269 shift
  4:314 shift
  5:299,target 48
  5:309,target 48
  5:310,target 48
  76:0 reduce
  24:313 shift
  21:271 shift
  24:314 shift
  21:272 shift
  54:298,target 2
  24:303,target 11
  21:273 shift
  21:261,target 57
  4:305,target 13
  21:274 shift
  8:0 reduce
  33:298 reduce
  21:275 shift
  15:326 goto
  0:329,target 3
  0:330,target 4
  21:276 shift
  21:277 shift
  12:320,target 35
  12:319,target 34
  1:313,target 71
  72:298 reduce
  21:278 shift
  43:298,target 29
  21:280 shift
  21:279 shift
  88:298,target 60
  21:281 shift
  0:298,target 70
  0:308,target 45
  52:0 reduce
  21:282 shift
  21:283 shift
  21:280,target 76
  21:279,target 75
  42:298 reduce
  73:0 reduce
  5:307,target 48
  32:298,target 73
  81:298 reduce
  77:298,target 25
  1:300 reduce
  4:332 goto
  89:291 shift
  24:301,target 9
  21:258,target 54
  1:301 reduce
  4:303,target 11
  5:0 reduce
}

array set fp::rules {
  9,l 322
  11,l 322
  32,l 323
  53,l 332
  74,l 335
  6,l 322
  28,l 323
  50,l 332
  49,l 329
  71,l 334
  3,l 322
  25,l 322
  46,l 329
  67,l 333
  0,l 336
  22,l 322
  43,l 328
  64,l 332
  18,l 322
  40,l 326
  39,l 325
  61,l 332
  15,l 322
  36,l 324
  57,l 332
  12,l 322
  33,l 323
  54,l 332
  75,l 335
  7,l 322
  29,l 323
  30,l 323
  51,l 332
  72,l 335
  4,l 322
  26,l 322
  47,l 329
  68,l 333
  1,l 322
  23,l 322
  44,l 327
  65,l 332
  19,l 322
  20,l 322
  41,l 326
  62,l 332
  16,l 322
  37,l 325
  58,l 332
  13,l 322
  34,l 323
  55,l 332
  76,l 335
  8,l 322
  10,l 322
  31,l 323
  52,l 332
  73,l 335
  5,l 322
  27,l 322
  48,l 331
  70,l 334
  69,l 333
  2,l 322
  24,l 322
  45,l 330
  66,l 333
  21,l 322
  42,l 327
  63,l 332
  17,l 322
  38,l 325
  60,l 332
  59,l 332
  14,l 322
  35,l 324
  56,l 332
  77,l 335
}

array set fp::rules {
  63,dc 2
  12,dc 1
  77,dc 1
  26,dc 1
  3,dc 1
  41,dc 1
  55,dc 3
  70,dc 0
  69,dc 3
  18,dc 1
  33,dc 1
  9,dc 1
  47,dc 1
  62,dc 2
  11,dc 1
  76,dc 1
  25,dc 1
  2,dc 1
  40,dc 1
  39,dc 1
  54,dc 1
  68,dc 2
  17,dc 1
  32,dc 1
  8,dc 1
  46,dc 2
  61,dc 4
  10,dc 1
  75,dc 1
  24,dc 1
  1,dc 1
  38,dc 1
  53,dc 2
  67,dc 3
  16,dc 1
  31,dc 1
  7,dc 1
  45,dc 0
  60,dc 3
  59,dc 1
  74,dc 1
  23,dc 1
  0,dc 1
  37,dc 1
  52,dc 1
  66,dc 2
  15,dc 1
  29,dc 1
  30,dc 1
  6,dc 1
  44,dc 3
  58,dc 1
  73,dc 1
  22,dc 1
  36,dc 1
  51,dc 1
  65,dc 1
  14,dc 1
  28,dc 1
  5,dc 1
  43,dc 0
  57,dc 2
  72,dc 1
  21,dc 1
  35,dc 1
  50,dc 1
  49,dc 3
  64,dc 2
  13,dc 1
  27,dc 1
  4,dc 1
  42,dc 1
  56,dc 2
  71,dc 1
  19,dc 1
  20,dc 1
  34,dc 1
  48,dc 0
}

array set fp::rules {
  41,line 179
  7,line 140
  37,line 174
  4,line 137
  34,line 168
  1,line 134
  31,line 165
  27,line 160
  24,line 157
  21,line 154
  76,line 224
  17,line 150
  73,line 221
  43,e 1
  14,line 147
  70,line 216
  69,line 213
  11,line 144
  66,line 210
  63,line 205
  60,line 202
  59,line 200
  56,line 197
  53,line 194
  50,line 191
  49,line 188
  46,line 186
  43,line 182
  9,line 142
  40,line 178
  39,line 176
  6,line 139
  36,line 171
  3,line 136
  33,line 167
  29,line 163
  30,line 164
  26,line 159
  23,line 156
  19,line 152
  20,line 153
  75,line 223
  16,line 149
  72,line 220
  13,line 146
  68,line 212
  10,line 143
  65,line 207
  62,line 204
  58,line 199
  55,line 196
  52,line 193
  48,line 187
  45,line 185
  42,line 182
  8,line 141
  38,line 175
  5,line 138
  35,line 170
  2,line 135
  32,line 166
  48,e 1
  28,line 162
  25,line 158
  22,line 155
  77,line 225
  45,e 0
  18,line 151
  74,line 222
  15,line 148
  71,line 217
  12,line 145
  67,line 211
  64,line 206
  61,line 203
  57,line 198
  54,line 195
  51,line 192
  47,line 187
  44,line 183
}

array set fp::lr1_table {
  35 {{76 298 1}}
  66,trans {}
  36 {{77 298 1}}
  85,trans {}
  37 {{55 {0 298} 2}}
  38 {{57 {0 298} 2}}
  39 {{40 {291 292 293} 1}}
  40 {{41 {291 292 293} 1}}
  41 {{60 {0 298} 2} {37 {0 298} 0} {38 {0 298} 0} {39 {0 298} 0}}
  42 {{56 {0 298} 2}}
  43 {{61 {0 298} 2} {40 {291 292 293} 0} {41 {291 292 293} 0}}
  44 {{28 {0 298} 1}}
  45 {{29 {0 298} 1}}
  46 {{30 {0 298} 1}}
  47 {{31 {0 298} 1}}
  48 {{32 {0 298} 1}}
  49 {{33 {0 298} 1}}
  50 {{34 {0 298} 1}}
  27,trans {}
  51 {{62 {0 298} 2}}
  46,trans {}
  52 {{35 {0 298} 1}}
  65,trans {}
  53 {{36 {0 298} 1}}
  84,trans {{284 44} {285 45} {286 46} {287 47} {288 48} {289 49} {290 50} {323 94}}
  54 {{63 {0 298} 2}}
  55 {{1 {0 298} 1}}
  56 {{2 {0 298} 1}}
  57 {{3 {0 298} 1}}
  58 {{4 {0 298} 1}}
  59 {{5 {0 298} 1}}
  60 {{6 {0 298} 1}}
  61 {{7 {0 298} 1}}
  62 {{8 {0 298} 1}}
  63 {{9 {0 298} 1}}
  64 {{10 {0 298} 1}}
  65 {{11 {0 298} 1}}
  66 {{12 {0 298} 1}}
  26,trans {}
  67 {{13 {0 298} 1}}
  45,trans {}
  68 {{14 {0 298} 1}}
  64,trans {}
  69 {{15 {0 298} 1}}
  70 {{16 {0 298} 1}}
  83,trans {}
  71 {{17 {0 298} 1}}
  72 {{18 {0 298} 1}}
  73 {{19 {0 298} 1}}
  74 {{20 {0 298} 1}}
  75 {{21 {0 298} 1}}
  76 {{22 {0 298} 1}}
  77 {{23 {0 298} 1}}
  78 {{24 {0 298} 1}}
  79 {{25 {0 298} 1}}
  80 {{26 {0 298} 1}}
  81 {{27 {0 298} 1}}
  82 {{64 {0 298} 2}}
  83 {{49 {0 298} 3}}
  25,trans {}
  84 {{68 {0 298} 2} {69 {0 298} 2} {28 {0 298} 0} {29 {0 298} 0} {30 {0 298} 0} {31 {0 298} 0} {32 {0 298} 0} {33 {0 298} 0} {34 {0 298} 0}}
  44,trans {}
  85 {{40 {0 284 285 286 287 288 289 290 298} 1}}
  63,trans {}
  86 {{41 {0 284 285 286 287 288 289 290 298} 1}}
  82,trans {}
  87 {{66 {0 298} 2} {67 {0 298} 2} {28 {0 298} 0} {29 {0 298} 0} {30 {0 298} 0} {31 {0 298} 0} {32 {0 298} 0} {33 {0 298} 0} {34 {0 298} 0}}
  0 {{0 0 0} {42 0 0} {44 0 0} {46 {0 298} 0} {47 {0 298} 0} {49 {0 298} 0} {45 {299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314} 0} {70 {0 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314} 0} {71 {0 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314} 0}}
  88 {{55 {0 298} 3}}
  1 {{71 {0 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314} 1}}
  89 {{37 {0 298} 1}}
  90 {{38 {0 298} 1}}
  2 {{0 0 1}}
  91 {{39 {0 298} 1}}
  3 {{42 0 1} {44 0 1} {43 298 0}}
  92 {{60 {0 298} 3}}
  4 {{46 {0 298} 1} {50 {0 298} 0} {51 {0 298} 0} {52 {0 298} 0} {53 {0 298} 0} {54 {0 298} 0} {55 {0 298} 0} {56 {0 298} 0} {57 {0 298} 0} {58 {0 298} 0} {59 {0 298} 0} {60 {0 298} 0} {61 {0 298} 0} {62 {0 298} 0} {63 {0 298} 0} {64 {0 298} 0} {65 {0 298} 0}}
  93 {{61 {0 298} 3} {37 {0 298} 0} {38 {0 298} 0} {39 {0 298} 0}}
  5 {{47 {0 298} 1} {49 {0 298} 1} {48 {299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314} 0}}
  94 {{69 {0 298} 3}}
  6 {{44 0 2}}
  95 {{67 {0 298} 3}}
  7 {{50 {0 298} 1}}
  96 {{61 {0 298} 4}}
  8 {{52 {0 298} 1}}
  9 {{51 {0 298} 1}}
  24,trans {{299 7} {300 8} {301 9} {302 10} {303 11} {304 12} {305 13} {306 14} {307 15} {308 16} {309 17} {310 18} {311 19} {312 20} {313 21} {314 22} {332 83}}
  43,trans {{295 39} {296 40} {326 93}}
  62,trans {}
  81,trans {}
  23,trans {}
  42,trans {}
  61,trans {}
  79,trans {}
  80,trans {}
  9,trans {}
  22,trans {}
  41,trans {{291 89} {292 90} {293 91} {325 92}}
  59,trans {}
  60,trans {}
  78,trans {}
  8,trans {}
  21,trans {{257 55} {258 56} {259 57} {260 58} {261 59} {262 60} {263 61} {264 62} {265 63} {266 64} {267 65} {268 66} {269 67} {270 68} {271 69} {272 70} {273 71} {274 72} {275 73} {276 74} {277 75} {278 76} {279 77} {280 78} {281 79} {282 80} {283 81} {322 82}}
  39,trans {}
  40,trans {}
  58,trans {}
  77,trans {}
  96,trans {}
  7,trans {}
  19,trans {{284 44} {285 45} {286 46} {287 47} {288 48} {289 49} {290 50} {323 51}}
  20,trans {{291 52} {294 53} {324 54}}
  38,trans {}
  57,trans {}
  76,trans {}
  95,trans {}
  6,trans {{298 25}}
  18,trans {{295 26} {296 27} {326 43}}
  37,trans {{298 88}}
  56,trans {}
  75,trans {}
  5,trans {{331 24}}
  94,trans {}
  17,trans {{298 42}}
  36,trans {}
  55,trans {}
  74,trans {}
  4,trans {{299 7} {300 8} {301 9} {302 10} {303 11} {304 12} {305 13} {306 14} {307 15} {308 16} {309 17} {310 18} {311 19} {312 20} {313 21} {314 22} {332 23}}
  93,trans {{291 89} {292 90} {293 91} {325 96}}
  16,trans {}
  35,trans {}
  54,trans {}
  73,trans {}
  3,trans {{328 6}}
  92,trans {}
  15,trans {{295 39} {296 40} {326 41}}
  34,trans {}
  53,trans {}
  72,trans {}
  2,trans {}
  91,trans {}
  14,trans {}
  33,trans {}
  52,trans {}
  71,trans {}
  1,trans {}
  89,trans {}
  90,trans {}
  13,trans {{298 38}}
  32,trans {}
  51,trans {}
  69,trans {}
  70,trans {}
  0,trans {{315 1} {327 2} {329 3} {330 4} {334 5}}
  88,trans {}
  12,trans {{316 31} {317 32} {318 33} {319 34} {320 35} {321 36} {335 37}}
  31,trans {}
  49,trans {}
  50,trans {}
  68,trans {}
  87,trans {{284 44} {285 45} {286 46} {287 47} {288 48} {289 49} {290 50} {323 95}}
  10 {{53 {0 298} 1} {66 {0 298} 0} {67 {0 298} 0} {68 {0 298} 0} {69 {0 298} 0} {40 {295 296} 0} {41 {295 296} 0}}
  11 {{54 {0 298} 1}}
  12 {{55 {0 298} 1} {72 298 0} {73 298 0} {74 298 0} {75 298 0} {76 298 0} {77 298 0}}
  13 {{57 {0 298} 1}}
  14 {{58 {0 298} 1}}
  11,trans {}
  15 {{60 {0 298} 1} {40 {291 292 293} 0} {41 {291 292 293} 0}}
  16 {{59 {0 298} 1}}
  29,trans {{295 85} {296 86} {326 87}}
  30,trans {}
  17 {{56 {0 298} 1}}
  48,trans {}
  18 {{61 {0 298} 1} {40 {295 296} 0} {41 {295 296} 0}}
  67,trans {}
  20 {{63 {0 298} 1} {35 {0 298} 0} {36 {0 298} 0}}
  19 {{62 {0 298} 1} {28 {0 298} 0} {29 {0 298} 0} {30 {0 298} 0} {31 {0 298} 0} {32 {0 298} 0} {33 {0 298} 0} {34 {0 298} 0}}
  86,trans {}
  21 {{64 {0 298} 1} {1 {0 298} 0} {2 {0 298} 0} {3 {0 298} 0} {4 {0 298} 0} {5 {0 298} 0} {6 {0 298} 0} {7 {0 298} 0} {8 {0 298} 0} {9 {0 298} 0} {10 {0 298} 0} {11 {0 298} 0} {12 {0 298} 0} {13 {0 298} 0} {14 {0 298} 0} {15 {0 298} 0} {16 {0 298} 0} {17 {0 298} 0} {18 {0 298} 0} {19 {0 298} 0} {20 {0 298} 0} {21 {0 298} 0} {22 {0 298} 0} {23 {0 298} 0} {24 {0 298} 0} {25 {0 298} 0} {26 {0 298} 0} {27 {0 298} 0}}
  22 {{65 {0 298} 1}}
  23 {{46 {0 298} 2}}
  24 {{49 {0 298} 2} {50 {0 298} 0} {51 {0 298} 0} {52 {0 298} 0} {53 {0 298} 0} {54 {0 298} 0} {55 {0 298} 0} {56 {0 298} 0} {57 {0 298} 0} {58 {0 298} 0} {59 {0 298} 0} {60 {0 298} 0} {61 {0 298} 0} {62 {0 298} 0} {63 {0 298} 0} {64 {0 298} 0} {65 {0 298} 0}}
  25 {{44 0 3}}
  26 {{40 {295 296} 1}}
  27 {{41 {295 296} 1}}
  28 {{68 {0 298} 1} {69 {0 298} 1}}
  30 {{53 {0 298} 2}}
  29 {{66 {0 298} 1} {67 {0 298} 1} {40 {0 284 285 286 287 288 289 290 298} 0} {41 {0 284 285 286 287 288 289 290 298} 0}}
  31 {{72 298 1}}
  32 {{73 298 1}}
  10,trans {{295 26} {296 27} {297 28} {326 29} {333 30}}
  33 {{74 298 1}}
  28,trans {{297 84}}
  34 {{75 298 1}}
  47,trans {}
}

array set fp::token_id_table {
  286 FK5_
  286,t 0
  287 J2000_
  292,line 44
  302,line 61
  288 ICRS_
  317,t 0
  265,title WCSH
  289 GALACTIC_
  290 ECLIPTIC_
  300 CLEAR_
  284,title FK4
  291 DEGREES_
  301 CLOSE_
  313,title SYSTEM
  292 ARCMIN_
  302 COORDINATE_
  332,title {}
  288,line 39
  293 ARCSEC_
  303 CROSSHAIR_
  294 SEXAGESIMAL_
  304 EXPORT_
  305 NAME_
  295 INT_
  306 PRINT_
  296 REAL_
  307 RADIUS_
  262,t 0
  297 SEXSTR_
  308 RETRIEVE_
  285,line 36
  298 STRING_
  310 SIZE_
  309 SAVE_
  299 CANCEL_
  311 SKY_
  283,t 0
  312 SKYFORMAT_
  313 SYSTEM_
  314,t 0
  314 UPDATE_
  282,line 32
  315 CXC_
  316 XML_
  264,title WCSG
  335,t 1
  317 VOT_
  283,title WCSZ
  318 SB_
  312,title SKYFORMAT
  331,title {}
  320 CSV_
  319 STARBASE_
  278,line 28
  error,line 132
  321 TSV_
  322 wcssys
  258,t 0
  323 skyframe
  324 skyformat
  325 rformat
  275,line 25
  279,t 0
  280,t 0
  326 numeric
  327 command
  328 @PSEUDO1
  311,t 0
  330 @PSEUDO2
  329 fp
  331 @PSEUDO3
  272,line 22
  332,t 1
  332 fpCmd
  333 coordinate
  263,title WCSF
  334 site
  282,title WCSY
  335 writer
  311,title SKY
  336 start'
  330,title {}
  329,title {}
  268,line 18
  334,line 215
  276,t 0
  265,line 15
  307,t 0
  297,t 0
  331,line 187
  328,t 1
  262,line 12
  327,line 181
  0,t 0
  0 {$}
  262,title WCSE
  281,title WCSX
  error,t 0
  310,title SIZE
  309,title SAVE
  299,title CANCEL
  328,title {}
  258,line 8
  273,t 0
  324,line 169
  294,t 0
  304,t 0
  325,t 1
  321,line 82
  317,line 78
  261,title WCSD
  279,title WCSV
  280,title WCSW
  308,title RETRIEVE
  269,t 0
  270,t 0
  298,title string
  327,title {}
  314,line 73
  291,t 0
  301,t 0
  322,t 1
  311,line 70
  307,line 66
  266,t 0
  260,title WCSC
  259,title WCSB
  297,line 52
  278,title WCSU
  307,title RADIUS
  297,title sexagesimal
  326,title {}
  287,t 0
  294,line 47
  304,line 63
  318,t 0
  error,title {}
  291,line 43
  301,line 60
  263,t 0
  258,title WCSA
  287,line 38
  277,title WCST
  284,t 0
  306,title PRINT
  296,title float
  325,title {}
  315,t 0
  284,line 35
  336,t 1
  281,line 31
  260,t 0
  259,t 0
  281,t 0
  257,title WCS
  277,line 27
  276,title WCSS
  312,t 0
  305,title NAME
  295,title integer
  324,title {}
  333,t 1
  274,line 24
  271,line 21
  336,line 226
  277,t 0
  308,t 0
  267,line 17
  298,t 0
  275,title WCSR
  333,line 209
  294,title SEXAGESIMAL
  304,title EXPORT
  330,t 1
  329,t 1
  323,title {}
  264,line 14
  330,line 185
  329,line 185
  261,line 11
  274,t 0
  326,line 177
  305,t 0
  295,t 0
  257,line 7
  326,t 1
  274,title WCSQ
  323,line 161
  293,title ARCSEC
  303,title CROSSHAIR
  322,title {}
  320,line 81
  319,line 80
  error error
  271,t 0
  292,t 0
  302,t 0
  316,line 77
  323,t 1
  273,title WCSP
  313,line 72
  292,title ARCMIN
  302,title COORDINATE
  321,title TSV
  267,t 0
  310,line 69
  309,line 68
  299,line 58
  288,t 0
  320,t 0
  319,t 0
  306,line 65
  296,line 50
  272,title WCSO
  291,title DEGREES
  293,line 45
  301,title CLOSE
  303,line 62
  320,title CSV
  319,title STARBASE
  264,t 0
  285,t 0
  289,line 40
  290,line 41
  300,line 59
  316,t 0
  286,line 37
  271,title WCSN
  261,t 0
  283,line 33
  289,title GALACTIC
  290,title ECLIPTIC
  300,title CLEAR
  318,title SB
  282,t 0
  279,line 29
  280,line 30
  313,t 0
  334,t 1
  276,line 26
  257,t 0
  269,title WCSL
  270,title WCSM
  273,line 23
  288,title ICRS
  317,title VOT
  278,t 0
  336,title {}
  310,t 0
  309,t 0
  299,t 0
  269,line 19
  270,line 20
  335,line 219
  331,t 1
  266,line 16
  332,line 190
  268,title WCSK
  275,t 0
  263,line 13
  287,title J2000
  316,title XML
  335,title {}
  328,line 182
  306,t 0
  296,t 0
  327,t 1
  260,line 10
  259,line 9
  325,line 173
  322,line 133
  272,t 0
  267,title WCSJ
  257 WCS_
  286,title FK5
  293,t 0
  303,t 0
  315,title CXC
  258 WCSA_
  334,title {}
  318,line 79
  260 WCSC_
  259 WCSB_
  324,t 1
  261 WCSD_
  262 WCSE_
  263 WCSF_
  264 WCSG_
  315,line 75
  265 WCSH_
  266 WCSI_
  267 WCSJ_
  268,t 0
  268 WCSK_
  269 WCSL_
  270 WCSM_
  312,line 71
  271 WCSN_
  272 WCSO_
  289,t 0
  290,t 0
  300,t 0
  266,title WCSI
  273 WCSP_
  274 WCSQ_
  285,title B1950
  321,t 0
  314,title UPDATE
  275 WCSR_
  333,title {}
  308,line 67
  276 WCSS_
  298,line 54
  277 WCST_
  278 WCSU_
  279 WCSV_
  280 WCSW_
  281 WCSX_
  305,line 64
  282 WCSY_
  295,line 49
  265,t 0
  283 WCSZ_
  284 FK4_
  285 B1950_
}

proc fp::yyparse {} {
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
                    1 { set _ wcs }
                    2 { set _ wcsa }
                    3 { set _ wcsb }
                    4 { set _ wcsc }
                    5 { set _ wcsd }
                    6 { set _ wcse }
                    7 { set _ wcsf }
                    8 { set _ wcsg }
                    9 { set _ wcsh }
                    10 { set _ wcsi }
                    11 { set _ wcsj }
                    12 { set _ wcsk }
                    13 { set _ wcsl }
                    14 { set _ wcsm }
                    15 { set _ wcsn }
                    16 { set _ wcso }
                    17 { set _ wcsp }
                    18 { set _ wcsq }
                    19 { set _ wcsr }
                    20 { set _ wcss }
                    21 { set _ wcst }
                    22 { set _ wcsu }
                    23 { set _ wcsv }
                    24 { set _ wcsw }
                    25 { set _ wcsx }
                    26 { set _ wcsy }
                    27 { set _ wcsz }
                    28 { set _ fk4 }
                    29 { set _ fk4 }
                    30 { set _ fk5 }
                    31 { set _ fk5 }
                    32 { set _ icrs }
                    33 { set _ galactic }
                    34 { set _ ecliptic }
                    35 { set _ degrees }
                    36 { set _ sexagesimal }
                    37 { set _ degrees }
                    38 { set _ arcmin }
                    39 { set _ arcsec }
                    40 { set _ $1 }
                    41 { set _ $1 }
                    43 { global ds9; if {!$ds9(init)} {YYERROR} else {yyclearin; YYACCEPT} }
                    45 { if {![FPCmdCheck]} {fp::YYABORT} }
                    47 { FPCmdRef $1 }
                    48 { FPCmdRef $1 }
                    50 { ProcessCmdCVAR0 ARCancel }
                    51 { ProcessCmdCVAR0 FPDestroy }
                    52 { ProcessCmdCVAR0 FPOff }
                    54 { ProcessCmdCVAR0 IMGSVRCrosshair }
                    55 { TBLCmdSave $3 $2 }
                    56 { TBLCmdSave $2 VOTWrite }
                    57 { ProcessCmdCVAR name $2 }
                    58 { ProcessCmdCVAR0 TBLCmdPrint }
                    59 { global cvarname; FPApply $cvarname 1 }
                    60 { TBLCmdSize $2 $3 }
                    61 { TBLCmdSize [expr ($2+$3)/2.] $4 }
                    62 { TBLCmdSkyframe $2 }
                    63 { ProcessCmdCVAR skyformat $2 }
                    64 { TBLCmdSystem $2 }
                    65 { ProcessCVAR0 IMGSVRUpdate }
                    66 { TBLCmdCoord $1 $2 fk5 }
                    67 { TBLCmdCoord $1 $2 $3 }
                    68 { TBLCmdCoord $1 $2 fk5 }
                    69 { TBLCmdCoord $1 $2 $3 }
                    70 { set _ cxc }
                    71 { set _ cxc }
                    72 { set _ VOTWrite }
                    73 { set _ VOTWrite }
                    74 { set _ starbase_write }
                    75 { set _ starbase_write }
                    76 { set _ TSVWrite }
                    77 { set _ TSVWrite }
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

proc fp::yyerror {msg} {
     variable yycnt
     variable yy_current_buffer
     variable index_

     ParserError $msg $yycnt $yy_current_buffer $index_
}
