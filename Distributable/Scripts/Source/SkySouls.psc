scriptname SkySouls hidden

bool function IsEnabled() global native
function Enable(bool flag = true) global native
function Disable()
	SkySouls.Enable(false)
endfunction

;/ valid menu names: /;;/
	"TweenMenu"
	"InventoryMenu"
	"ContainerMenu"
	"MagicMenu"
	"FavoritesMenu"
	"Lockpicking Menu"
	"Book Menu"
	"BarterMenu"
	"GiftMenu"
	"Training Menu"
	"MessageBoxMenu"
/;
;/ valid pause behavior values: /;;/
	0 = always
	1 = never
	2 = only outside combat
	3 = only in combat
/;
function SetMenuPauseBehavior(string menuName, int pauseBehavior) global native