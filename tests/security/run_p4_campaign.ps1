[CmdletBinding()]
param(
    [ValidateSet('Quick', 'Full', 'Release')]
    [string] $Mode = 'Full',
    [ValidateRange(1, 6)]
    [int] $Parallelism = 4,
    [switch] $PlanOnly,
    [switch] $NoBuild,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $repoRoot "tmp\security-campaign\$timestamp"
}

function New-Step([string] $Executable, [string[]] $Arguments, [string[]] $Environment = @()) {
    [pscustomobject]@{
        Executable = $Executable
        Arguments = $Arguments
        Environment = $Environment
    }
}

function New-Test([string] $Name, [int] $Minutes, [object[]] $Steps) {
    [pscustomobject]@{
        Name = $Name
        Minutes = $Minutes
        Steps = $Steps
    }
}

function Show-Plan([object[]] $Tests, [bool] $Builds, [bool] $Browser) {
    Write-Output "AURORA P4 security campaign: $Mode"
    Write-Output "Parallel host lanes: $Parallelism"
    foreach ($test in $Tests) {
        Write-Output ("  HOST  {0,-26} {1,2} min" -f $test.Name, $test.Minutes)
    }
    if ($Builds) {
        Write-Output '  BUILD waveshare-p4-rev1       5-15 min (sequential)'
        Write-Output '  BUILD waveshare-p4            5-15 min (sequential)'
        Write-Output '  POST  P4 images/policy/hashes  2-5 min'
    }
    if ($Browser) {
        Write-Output '  WEB   Chromium P4-only         2-5 min'
    }
    Write-Output 'No serial port, flash command or eFuse operation is part of this script.'
}

$python = (Get-Command python.exe -ErrorAction SilentlyContinue)
if (-not $python) { $python = Get-Command python -ErrorAction SilentlyContinue }
$node = Get-Command node.exe -ErrorAction SilentlyContinue
if (-not $node) { $node = Get-Command node -ErrorAction SilentlyContinue }
$powerShell = (Get-Command powershell.exe -ErrorAction Stop).Source
$cmd = (Get-Command cmd.exe -ErrorAction Stop).Source
if (-not $python) { throw 'Python est requis.' }
if ($Mode -ne 'Quick' -and -not $node) { throw 'Node.js est requis pour les controles Web Flasher.' }

$tests = @(
    (New-Test 'crypto-ui-lvgl' 10 @(
        (New-Step $python.Source @('tests/crypto/run.py')),
        (New-Step $python.Source @('tests/ui/run.py')),
        (New-Step $powerShell @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'tests/lvgl/run.ps1'))
    )),
    (New-Test 'entropy-rng' 6 @(
        (New-Step $cmd @('/d', '/c', 'tests\native\run.cmd', '--p4-only')),
        (New-Step $python.Source @('tests/rng/run.py', '--p4-only')),
        (New-Step $python.Source @('tests/rng/run_entropy_health.py'))
    )),
    (New-Test 'storage-memory' 6 @(
        (New-Step $python.Source @('tests/storage/run.py')),
        (New-Step $powerShell @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'tests/memory/run.ps1')),
        (New-Step $powerShell @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'tests/memory/run.ps1', '-Optimized')),
        (New-Step $python.Source @('tests/memory/test_boot_order.py'))
    )),
    (New-Test 'logging-reproducibility' 4 @(
        (New-Step $python.Source @('tests/logging/run.py')),
        (New-Step $python.Source @('tests/reproducible_build/run.py'))
    ))
)

if ($Mode -ne 'Quick') {
    $tests += New-Test 'dependencies-sbom' 3 @(
        (New-Step $python.Source @('tests/dependencies/run.py'))
    )
    $tests += New-Test 'webflasher-p4' 3 @(
        (New-Step $node.Source @('tests/release/test_vendor.cjs')),
        (New-Step $node.Source @('tests/release/test_webflasher.cjs') @('AURORA_P4_ONLY=1'))
    )
}

if ($Mode -eq 'Release') {
    $tests += New-Test 'browser-p4' 5 @(
        (New-Step $node.Source @('tests/release/test_vendor_browser.cjs') @('AURORA_P4_ONLY=1'))
    )
}

$runBuilds = ($Mode -ne 'Quick') -and (-not $NoBuild)
Show-Plan $tests $runBuilds ($Mode -eq 'Release')
if ($PlanOnly) { exit 0 }

if ($runBuilds -and -not (Get-Command pio.exe -ErrorAction SilentlyContinue) -and
    -not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw 'PlatformIO (pio) est requis pour les builds P4.'
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Build Tools/MSVC est requis.' }

Set-Location -LiteralPath $repoRoot
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$gitStatus = @(& git status --porcelain=v1)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire l etat Git.' }
if ($Mode -eq 'Release' -and $gitStatus.Count -ne 0) {
    throw 'Le mode Release exige un commit propre. Utilisez Full pendant le developpement.'
}

$started = Get-Date
$results = New-Object System.Collections.Generic.List[object]

function Invoke-SerialTest([string] $Name, [string] $Executable, [string[]] $Arguments) {
    $log = Join-Path $OutputDirectory ($Name + '.log')
    $watch = [Diagnostics.Stopwatch]::StartNew()
    Write-Host "[$Name] START"
    # Windows PowerShell 5.1 wraps native stderr lines as non-terminating
    # ErrorRecord objects.  With the campaign-wide Stop policy, harmless build
    # diagnostics would otherwise abort the wrapper before its real exit code
    # can be inspected.
    $previousErrorAction = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Executable @Arguments 2>&1 | ForEach-Object {
            $_ | Out-File -LiteralPath $log -Append -Encoding utf8
            Write-Host $_
        }
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorAction
    }
    $watch.Stop()
    $ok = ($exitCode -eq 0)
    $results.Add([pscustomobject]@{
        name = $Name; status = $(if ($ok) { 'PASS' } else { 'FAIL' })
        seconds = [math]::Round($watch.Elapsed.TotalSeconds, 1); log = $log
    })
    Write-Host ("[{0}] {1} ({2:n1}s)" -f $Name, $(if ($ok) { 'PASS' } else { 'FAIL' }), $watch.Elapsed.TotalSeconds)
    return $ok
}

function Invoke-ParallelTests([object[]] $Specifications) {
    $pending = New-Object System.Collections.Queue
    foreach ($specification in $Specifications) { $pending.Enqueue($specification) }
    $running = @{}
    while ($pending.Count -gt 0 -or $running.Count -gt 0) {
        while ($pending.Count -gt 0 -and $running.Count -lt $Parallelism) {
            $specification = $pending.Dequeue()
            $job = Start-Job -Name $specification.Name -ArgumentList $repoRoot, $specification.Steps -ScriptBlock {
                param($root, $steps)
                $ErrorActionPreference = 'Stop'
                Set-Location -LiteralPath $root
                foreach ($step in $steps) {
                    foreach ($assignment in @($step.Environment)) {
                        $parts = $assignment -split '=', 2
                        [Environment]::SetEnvironmentVariable($parts[0], $parts[1], 'Process')
                    }
                    $arguments = @($step.Arguments)
                    Write-Output ("> {0} {1}" -f $step.Executable, ($arguments -join ' '))
                    & $step.Executable @arguments
                    if ($LASTEXITCODE -ne 0) {
                        throw "Command failed with exit code ${LASTEXITCODE}: $($step.Executable)"
                    }
                }
            }
            $running[$specification.Name] = [pscustomobject]@{
                Job = $job
                Watch = [Diagnostics.Stopwatch]::StartNew()
            }
            Write-Host "[$($specification.Name)] START"
        }

        $completed = @($running.GetEnumerator() | Where-Object {
            $_.Value.Job.State -in @('Completed', 'Failed', 'Stopped', 'Disconnected')
        })
        if ($completed.Count -eq 0) {
            Wait-Job -Job @($running.Values | ForEach-Object { $_.Job }) -Any -Timeout 2 | Out-Null
            continue
        }
        foreach ($entry in $completed) {
            $entry.Value.Watch.Stop()
            $job = $entry.Value.Job
            $log = Join-Path $OutputDirectory ($entry.Key + '.log')
            @((Receive-Job -Job $job -ErrorAction Continue 2>&1)) |
                Out-File -LiteralPath $log -Encoding utf8
            $ok = ($job.State -eq 'Completed')
            $results.Add([pscustomobject]@{
                name = $entry.Key; status = $(if ($ok) { 'PASS' } else { 'FAIL' })
                seconds = [math]::Round($entry.Value.Watch.Elapsed.TotalSeconds, 1); log = $log
            })
            Write-Host ("[{0}] {1} ({2:n1}s)" -f $entry.Key, $(if ($ok) { 'PASS' } else { 'FAIL' }), $entry.Value.Watch.Elapsed.TotalSeconds)
            Remove-Job -Job $job -Force
            $running.Remove($entry.Key)
        }
    }
}

$p4Dependencies = @(
    (Join-Path $repoRoot 'targets\waveshare_p4\managed_components\lvgl__lvgl\lvgl.h'),
    (Join-Path $repoRoot 'targets\waveshare_p4\.pio\build\waveshare-p4-rev1\_deps\ubitcoin-src\src\HDWallet.cpp'),
    (Join-Path $repoRoot 'targets\waveshare_p4\.pio\build\waveshare-p4\_deps\ubitcoin-src\src\HDWallet.cpp')
)
$needsBootstrap = @($p4Dependencies | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -ne 0
$rev1Built = $false
if ($needsBootstrap) {
    if (-not $runBuilds) {
        throw 'Les dependances P4 ne sont pas configurees. Relancez en mode Full sans -NoBuild.'
    }
    $rev1Built = Invoke-SerialTest 'bootstrap-rev1' $powerShell @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        'targets/waveshare_p4/build.ps1', '-Target', 'waveshare-p4-rev1'
    )
}

Invoke-ParallelTests $tests

if ($runBuilds) {
    if (-not $rev1Built) {
        [void](Invoke-SerialTest 'build-rev1' $powerShell @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
            'targets/waveshare_p4/build.ps1', '-Target', 'waveshare-p4-rev1'
        ))
    }
    [void](Invoke-SerialTest 'build-rev3' $powerShell @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        'targets/waveshare_p4/build.ps1', '-Target', 'waveshare-p4'
    ))

    $evidence = Join-Path $OutputDirectory 'reproducibility'
    $postBuild = @(
        (New-Test 'images-policy' 5 @(
            (New-Step $python.Source @('tests/crypto/verify_p4_images.py')),
            (New-Step $python.Source @('tests/rng/verify_images.py')),
            (New-Step $python.Source @('tests/logging/verify_images.py', '--target', 'p4-rev1', '--target', 'p4-rev3')),
            (New-Step $python.Source @('tests/security_policy/run.py'))
        )),
        (New-Test 'local-build-manifests' 3 @(
            (New-Step $python.Source @('tools/reproducible_build.py', 'collect', '--repo-root', '.', '--project-dir', 'targets/waveshare_p4', '--profile', 'waveshare-p4-rev1', '--output-dir', $evidence, '--runner', 'local-windows')),
            (New-Step $python.Source @('tools/reproducible_build.py', 'collect', '--repo-root', '.', '--project-dir', 'targets/waveshare_p4', '--profile', 'waveshare-p4', '--output-dir', $evidence, '--runner', 'local-windows'))
        )),
        (New-Test 'retained-p4-releases' 3 @(
            (New-Step $powerShell @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'tests/release/verify_p4.ps1'))
        ))
    )
    Invoke-ParallelTests $postBuild
}

$finished = Get-Date
$failed = @($results | Where-Object { $_.status -ne 'PASS' })
$summary = [ordered]@{
    schema = 1
    scope = 'P4 only; host files and reports allowed; no device write'
    mode = $Mode
    started = $started.ToString('o')
    finished = $finished.ToString('o')
    elapsedSeconds = [math]::Round(($finished - $started).TotalSeconds, 1)
    gitRevision = ((& git rev-parse HEAD | Select-Object -First 1).Trim())
    gitDirty = ($gitStatus.Count -ne 0)
    result = $(if ($failed.Count -eq 0) { 'PASS' } else { 'FAIL' })
    tests = @($results | ForEach-Object { $_ })
}
$summaryPath = Join-Path $OutputDirectory 'summary.json'
$summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $summaryPath -Encoding utf8

if ($runBuilds -and $failed.Count -eq 0) {
    $versionText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'include\version.h')
    $version = [regex]::Match($versionText, 'AURORA_P4_FIRMWARE_VERSION\s+"([^"]+)"').Groups[1].Value
    $identities = foreach ($profile in @(
        @{ Name = 'rev1'; Folder = 'waveshare-p4-rev1' },
        @{ Name = 'rev3'; Folder = 'waveshare-p4' }
    )) {
        $folder = Join-Path $repoRoot "targets\waveshare_p4\.pio\build\$($profile.Folder)"
        [ordered]@{
            profile = $profile.Name
            application = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $folder 'firmware.bin')).Hash
            factory = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $folder 'firmware.factory.bin')).Hash
        }
    }
    $identityPath = Join-Path $OutputDirectory 'candidate-identities.json'
    [ordered]@{ version = $version; builds = @($identities) } |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $identityPath -Encoding utf8
    $checklist = @"
# Bloc physique P4 rev1.3 — AURORA $version

Généré par la campagne locale. Ce fichier est une feuille opérateur sur PC ; rien
n'est écrit comme journal dans le P4. Utiliser uniquement une microSD de test et
des données publiques. Aucun erase-all, aucune eFuse et aucun secret réel.

## Une seule session de production

- [ ] Autorisation de flash obtenue séparément et port/révision identifiés.
- [ ] Application rev1 sauvegardée si nécessaire, puis candidat rev1 flashé et vérifié.
- [ ] À propos : version $version, cible rev1.x, SHA appareil comparé à candidate-identities.json.
- [ ] Accueil, tactile, verrouillage, redémarrage et absence de journal/fichier sur le P4.
- [ ] Sans microSD : message « carte microSD absente ou illisible ».
- [ ] MicroSD de test : création 24 mots + passphrase publique, export .aurora, fermeture.
- [ ] Mauvais mot de passe refusé ; bon mot de passe accepté ; noter seulement les durées.
- [ ] Adresses Native SegWit/Nested SegWit/Taproot ; aucun écran « Clé publique » séparé.
- [ ] CLÉ ÉTENDUE : titre du type, valeur complète, QR et RETOUR.
- [ ] MOTS 180 s, CLÉ PRIVÉE 60 s, expiration/verrouillage et nouvelle authentification.
- [ ] Restauration BIP39, Umbrel/LND, QR, erreurs et annulations avec données publiques.
- [ ] Entropie 2.0.12 : seuil micro adaptatif, deux canaux, 512 mouvements, 6 zones,
      environ 10 s d'activité, aucune progression immobile, collecte jusqu'à TERMINER.

## Diagnostics séparés seulement si leur domaine a changé

- [ ] KDF rev1 : 3 mesures valides pour 10k/120k/300k/500k, aucun reset/échec.
- [ ] Entropie santé rev1 : microSD retirée, 3 démarrages à froid, 9/9 séries PASS.
- [ ] Pile rev1 : parcours lourds publics, minima > 6144/2048 octets, photo du verdict.
- [ ] Restaurer le candidat de production, vérifier toute la partition application et l'identité.

## Publication, après la qualification

- [ ] CI indépendante Windows/Linux rev1/rev3 strictement identique.
- [ ] Paquet et manifeste reconstruits depuis les octets CI canoniques.
- [ ] Signature dans la fenêtre PowerShell dédiée à la phrase secrète ; tests négatifs.
- [ ] Gate du dépôt public, push/tag/release/Pages, puis Web Flasher servi réellement.
- [ ] Installation Web rev1.3 à 100 %, redémarrage et identité finale identique.
"@
    Set-Content -LiteralPath (Join-Path $OutputDirectory 'PHYSICAL_CHECKLIST.md') -Value $checklist -Encoding utf8
}

Write-Host "Summary: $summaryPath"
if ($failed.Count -ne 0) {
    Write-Error ("Campaign failed: " + (($failed | ForEach-Object name) -join ', '))
    exit 1
}
Write-Host ("CAMPAIGN PASS in {0:n1} minutes" -f (($finished - $started).TotalMinutes))
exit 0
