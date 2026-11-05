[CmdletBinding()]
param(
    [ValidatePattern('^[\p{L}\p{N}_.-]+\.zip$')]
    [string]$ArchiveName = 'Homework7_submission.zip'
)

$ErrorActionPreference = 'Stop'
$assignmentRoot = $PSScriptRoot
$repositoryRoot = Split-Path -Parent $assignmentRoot
$codeRoot = Join-Path $assignmentRoot 'Assignment7_code'
$outputRoot = Join-Path $repositoryRoot 'tmp/assignment7-submission'
$runName = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$runRoot = Join-Path $outputRoot $runName
$stageRoot = Join-Path $runRoot 'Homework7'

# New staging directories only. Existing files and build trees are never removed.
if (Test-Path -LiteralPath $runRoot) {
    throw "Output directory already exists: $runRoot"
}
foreach ($required in @('CMakeLists.txt', 'README.md', 'models', 'images')) {
    if (-not (Test-Path -LiteralPath (Join-Path $codeRoot $required))) {
        throw "Missing required submission content: $required"
    }
}
New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null

$sourceFiles = Get-ChildItem -LiteralPath $codeRoot -File | Where-Object {
    $_.Extension -in @('.cpp', '.hpp') -or
    $_.Name -in @('CMakeLists.txt', 'CMakePresets.json', 'README.md')
}
foreach ($file in $sourceFiles) {
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $stageRoot $file.Name)
}

foreach ($folder in @('models', 'images')) {
    $from = Join-Path $codeRoot $folder
    $to = Join-Path $stageRoot $folder
    foreach ($file in (Get-ChildItem -LiteralPath $from -Recurse -File)) {
        $allowed = if ($folder -eq 'models') { @('.obj', '.mtl', '.png', '.jpg', '.jpeg') }
                   else { @('.png', '.ppm', '.json', '.md') }
        if ($file.Extension.ToLowerInvariant() -notin $allowed) { continue }
        $relative = $file.FullName.Substring($from.Length).TrimStart([char[]]'\/')
        $destination = Join-Path $to $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $destination
    }
}

$hashLines = foreach ($file in (Get-ChildItem -LiteralPath $stageRoot -Recurse -File | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($stageRoot.Length).TrimStart([char[]]'\/').Replace('\', '/')
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$manifest = Join-Path $stageRoot 'SUBMISSION_MANIFEST.sha256'
[System.IO.File]::WriteAllLines($manifest, [string[]]$hashLines, [System.Text.UTF8Encoding]::new($false))

$archivePath = Join-Path $runRoot $ArchiveName
$items = @(Get-ChildItem -LiteralPath $stageRoot -Force | ForEach-Object { $_.FullName })
Compress-Archive -LiteralPath $items -DestinationPath $archivePath -CompressionLevel Optimal
Write-Output "Staged project: $stageRoot"
Write-Output "Archive: $archivePath"
Write-Output "Rename the archive to your course-required name before uploading."
