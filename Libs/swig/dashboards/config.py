import colorcet 

doc=None

DIRECTIONS=[('0','X'),('1','Y'),('2','Z')]

PALETTES=[
	"Greys256", 
    "Inferno256", 
    "Magma256", 
    "Plasma256", 
    "Viridis256", 
    "Cividis256", 
    "Turbo256"
	] + [
		it  for it in [
		'colorcet.blueternary', 
		'colorcet.coolwarm', 
		'colorcet.cyclicgrey', 
		'colorcet.depth', 
		'colorcet.divbjy', 
		'colorcet.fire', 
		'colorcet.geographic', 
		'colorcet.geographic2', 
		'colorcet.gouldian', 
		'colorcet.gray', 
		'colorcet.greenternary', 
		'colorcet.grey', 
		'colorcet.heat', 
		'colorcet.phase2', 
		'colorcet.phase4', 
		'colorcet.rainbow', 
		'colorcet.rainbow2', 
		'colorcet.rainbow3', 
		'colorcet.rainbow4', 
		'colorcet.redternary', 
		'colorcet.reducedgrey', 
		'colorcet.yellowheat']
		if hasattr(colorcet,it[9:])
	]  