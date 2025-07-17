param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$InputDirectory,

    [Parameter(Mandatory=$true, Position=1)]
    [string]$OutputDirectory
)

# Create FileSystem objects
$inputDir  = Get-Item -LiteralPath $InputDirectory  -ErrorAction SilentlyContinue
$outputDir = Get-Item -LiteralPath $OutputDirectory -ErrorAction SilentlyContinue

# Validate input directory
if (-not $inputDir) {
    Write-Host "Input directory not found."
    exit 1
}

# Create output directory if it does not exist
if (-not $outputDir) {
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
}

# Iterate over all image files in the input directory
Get-ChildItem -Path $inputDir -File | ForEach-Object {
    $file = $_.FullName
    $name = [System.IO.Path]::GetFileNameWithoutExtension($file)
    Write-Host $_.Name

    # Run vips to convert to pyramid
    # https://kleisauke.github.io/libvips/API/current/method.Image.tiffsave.html
    & vips tiffsave $file `
        "$OutputDirectory\$name.tiff" `
        --tile `
        --pyramid `
        --compression jpeg `
        --Q 90 `
}