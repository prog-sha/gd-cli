# Build a greeting with settings supplied by a local module.
@import "./settings.gd" as Settings

# Return one greeting using the shared prefix.
static func message(name):
	return "%s, %s!" % [Settings.TITLE, name]
