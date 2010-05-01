#This parses and generates #defines from an enum.spec type of file.

proc main {argc argv} {
    if {2 != $argc} {
	puts stderr "syntax is: [info script] input.spec output.h"
	exit 1
    }

    set fd [open [lindex $argv 0] r]
    set data [read $fd]
    close $fd

    set fd [open [lindex $argv 1] w]
    
    set state ""

    puts $fd "#define GL_VERSION_1_1 1"
    puts $fd "#define GL_VERSION_1_2 1"
    puts $fd "#define GL_VERSION_1_3 1"
    puts $fd "#define GL_VERSION_1_4 1"
    puts $fd "#define GL_VERSION_1_5 1"
    puts $fd "#define GL_VERSION_2_0 1"
    #puts $fd "#define GL_VERSION_3_0 1"

    set mask ""
    array set ar {}

    foreach line [split $data \n] {
	if {[regexp {^\S*#.*} $line] > 0} {
	    #puts COMMENT:$line
	    set state ""
	} elseif {"enum" eq $state} {
	    if {[string match "\t*" $line]} {
		if {[regexp {^\tuse.*} $line] > 0} {
		    lassign [split [string trim $line]] use usemask def
		    set usemask [string trim $usemask]
		    set def [string trim $def]
		    puts $fd "/* GL_$def */" 
		} else {
		    lassign [split [string trim $line] =] def value
		    set def [string trim $def]
		    set value [string trim $value]

		    #Trim out the data like: 0x0B00 # 4 F
		    set value [lindex [split $value] 0]

		    puts $fd "#define GL_$def $value"

		    #Save this association with the value.
		    set d $ar($mask)
		    dict set d $def $value
		    set ar($mask) $d
		}
	    } else {
		set state ""
	    }
	} elseif {[string match "* enum:*" $line]} {
	    lassign [split $line] mask _
	    puts $fd "\n/*[string trim $mask]*/"
	    set ar($mask) [dict create]
	    set state enum
  	}
    }
    
    close $fd
}
main $::argc $::argv
