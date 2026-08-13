# Signs Atomik build outputs with the local "Atomik Local Dev" code-signing cert
# so Windows Application Control / Smart App Control will allow F5 runs.
param(
    [Parameter(Mandatory = $true)]
    [string] $ExePath
)

$ErrorActionPreference = 'Continue'
try {
    if (-not (Test-Path -LiteralPath $ExePath)) {
        Write-Host "SignLocal: missing $ExePath"
        exit 0
    }

    $storePath = 'Cert:\CurrentUser\My'
    $cert = Get-ChildItem $storePath -CodeSigningCert -ErrorAction SilentlyContinue |
        Where-Object { $_.Subject -eq 'CN=Atomik Local Dev' } |
        Select-Object -First 1

    if (-not $cert) {
        $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=Atomik Local Dev' `
            -CertStoreLocation $storePath -KeyExportPolicy Exportable -NotAfter (Get-Date).AddYears(5)
    }

    # Always keep cert trusted locally (Root + Trusted Publisher).
    foreach ($storeName in @('Root', 'TrustedPublisher')) {
        $store = New-Object System.Security.Cryptography.X509Certificates.X509Store($storeName, 'CurrentUser')
        $store.Open('ReadWrite')
        if (-not ($store.Certificates | Where-Object { $_.Thumbprint -eq $cert.Thumbprint })) {
            $store.Add($cert)
        }
        $store.Close()
    }

    $sig = Set-AuthenticodeSignature -FilePath $ExePath -Certificate $cert -HashAlgorithm SHA256
    Write-Host "SignLocal: $ExePath -> $($sig.Status)"
} catch {
    Write-Host "SignLocal warning: $($_.Exception.Message)"
}
exit 0
