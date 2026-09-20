# Use short imports instead of repeated preload declarations.
@import "./settings.gd" as Settings
@import "./greeting.gd" as Greeting

# Print a greeting assembled from two local modules.
func main():
	print(Greeting.message(Settings.USER))
	return 0
