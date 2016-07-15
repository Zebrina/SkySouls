scriptname SkySouls

bool function IsEnabled() global native
function Enable(bool flag = true) global native
function Disable()
	Enable(false)
endfunction
