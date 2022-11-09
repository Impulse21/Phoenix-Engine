pushd "C:\Users\dipao\source\repos\Impulse21\Phoenix-Engine\Assets\Models\Sponza_Intel\Main"
# The directory containing your files
$directory = "textures";

# The text file to write filenames to (does not have to exist first)
$txtFile = "batch.nvdds"
Clear-Content $txtFile

# Get filenames from this directory
# Get-ChildItem is aliased by 'dir'
$files = Get-ChildItem $directory;

Write-Output "Generating Batch File"
# Loop through the files in the directory
foreach($file in $files)
{
	$format = "bc7"
	$options = ""

	if ($file.Name -Match "Normal") { 
  		$format = "bc5"
		# this option didn't work
		#$options= " --normal-invert-y"
		#$options= " --save-flip-y"
	}

	$line = $directory + "\" + $file.Name + $options + " --format " + $format + " --output textures_dds\" + $file.Basename + ".dds"
	Add-Content $txtFile $line;
}

Write-Output "Executing Batch Export of file $txtFile"

& 'C:\Program Files\NVIDIA Corporation\NVIDIA Texture Tools\nvtt_export.exe' --batch .\$txtFile

Write-Output "Completed Texture Export"
popd 

pause