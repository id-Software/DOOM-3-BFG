--
-- Use the embed action to convert all of the renderprogs scripts into C strings, which 
-- can then be built into the executable. Always embed the scripts before creating
-- a release build.
--
dofile("premake/embed.lua")

newaction
{
	trigger     = "embed",
	description = "Embed renderprogs and scripts in into the engine code; required before release builds",
	execute     = doembed
}
