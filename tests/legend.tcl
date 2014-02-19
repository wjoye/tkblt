source base.tcl

set w .lgd
set graph [bltLineGraph $w]

#bltTest2 $graph legend -activebackground
#bltTest2 $graph legend -activeborderwidth
#bltTest2 $graph legend -activeforeground
#bltTest2 $graph legend -activerelief
bltTest2 $graph legend -anchor s
bltTest2 $graph legend -bg pink
bltTest2 $graph legend -background cyan
bltTest2 $graph legend -borderwidth 20
bltTest2 $graph legend -bd 20
bltTest2 $graph legend -columns 2
#bltTest2 $graph legend -exportselection
#bltTest2 $graph legend -focusdashes
#bltTest2 $graph legend -focusforeground
bltTest2 $graph legend -font "times 18 bold italic"
bltTest2 $graph legend -fg yellow
bltTest2 $graph legend -foreground purple
bltTest2 $graph legend -hide yes
bltTest2 $graph legend -ipadx 20
bltTest2 $graph legend -ipady 20
#bltTest2 $graph legend -nofocusselectbackground
#bltTest2 $graph legend -nofocusselectforeground
bltTest2 $graph legend -padx 20
bltTest2 $graph legend -pady 20
bltTest2 $graph legend -position leftmargin
bltTest2 $graph legend -position topmargin
bltTest2 $graph legend -position bottommargin
bltTest2 $graph legend -position "@250,100"
bltTest2 $graph legend -raised yes
bltTest2 $graph legend -relief groove
bltTest2 $graph legend -rows 1
#bltTest2 $graph legend -selectbackground
#bltTest2 $graph legend -selectborderwidth
#bltTest2 $graph legend -selectcommand
#bltTest2 $graph legend -selectforeground
#bltTest2 $graph legend -selectmode
#bltTest2 $graph legend -selectrelief
#bltTest2 $graph legend -takefocus
bltTest2 $graph legend -title "Hello World"
##bltTest2 $graph legend -titleanchor center
##bltTest2 $graph legend -titlecolor red
bltTest2 $graph legend -titlefont "times 24 bold italic"

#$graph active
#$graph bind
#$graph curselection
#$graph deactive
#$graph focus
#$graph get
#$graph selection anchor
#$graph selection clear
#$graph selection clearall
#$graph selection includes
#$graph selection mark
#$graph selection present
#$graph selection set
#$graph selection toggle

echo "done"
#bltPlotDestroy $w

